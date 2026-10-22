#nullable enable
using Godot;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Threading;
using System.Threading.Tasks;
using GArray = Godot.Collections.Array;

namespace Ultralight;

internal sealed class ManagedConversion
{
    // Keep identity without retaining every object ever exposed by a long-lived
    // registry. Native bindings retain adapters while they are actually in use.
    private readonly ConditionalWeakTable<object, UltralightManagedObject> _objects = new();
    internal UltralightManagedObject Wrap(object source)
    {
        if (_objects.TryGetValue(source, out var cached)) return cached;
        if (source.GetType().IsValueType || source is Delegate or Type or Task)
            throw new NotSupportedException($"Unsupported managed value type: {source.GetType().FullName}");
        var result = new UltralightManagedObject();
        result.Initialize(source, this);
        _objects.Add(source, result);
        return result;
    }
    internal Variant ToVariant(object? value) => value switch
    {
        null => default,
        Variant variant => variant,
        bool boolean => boolean,
        string text => text,
        byte number => (long)number,
        short number => (long)number,
        int number => (long)number,
        long number => number,
        float number => (double)number,
        double number => number,
        GodotObject native => native,
        GArray array => array,
        Godot.Collections.Dictionary dictionary => dictionary,
        _ => Wrap(value)
    };
    internal static object? FromVariant(Variant value, Type expected)
    {
        if (expected == typeof(Variant)) return value;
        if (value.VariantType == Variant.Type.Nil) return null;
        if (value.VariantType == Variant.Type.Object)
        {
            var native = value.AsGodotObject();
            if (native is UltralightManagedObject adapted && expected.IsInstanceOfType(adapted.Source)) return adapted.Source;
            if (expected.IsInstanceOfType(native)) return native;
            throw new InvalidCastException($"Cannot convert shared JS handle to {expected.Name}; request a snapshot explicitly");
        }
        if (expected == typeof(GArray) && value.VariantType == Variant.Type.Array) return value.AsGodotArray();
        if (expected == typeof(Godot.Collections.Dictionary) && value.VariantType == Variant.Type.Dictionary) return value.AsGodotDictionary();
        if (expected == typeof(string)) return value.AsString();
        if (expected == typeof(bool)) return value.AsBool();
        if (expected == typeof(int)) return value.AsInt32();
        if (expected == typeof(long)) return value.AsInt64();
        if (expected == typeof(float)) return value.AsSingle();
        if (expected == typeof(double)) return value.AsDouble();
        if (expected == typeof(object)) return value.Obj;
        throw new NotSupportedException($"Unsupported managed argument type: {expected.FullName}");
    }
    internal void Clear() => _objects.Clear();
}

/// <summary>Lazy adapter used by UltralightApi for CLR objects and mutable collections.</summary>
public partial class UltralightManagedObject : RefCounted
{
    internal object Source { get; private set; } = null!;
    private ManagedConversion _conversion = null!;
    private Dictionary<string, PropertyInfo> _properties = new();
    private Dictionary<string, FieldInfo> _fields = new();
    private Dictionary<string, MethodInfo[]> _methods = new();
    internal void Initialize(object source, ManagedConversion conversion)
    {
        Source = source; _conversion = conversion;
        if (source is IList or IDictionary) return;
        const BindingFlags flags = BindingFlags.Public | BindingFlags.Instance;
        _properties = source.GetType().GetProperties(flags).Where(p => p.GetIndexParameters().Length == 0 && p.CanRead).ToDictionary(p => p.Name);
        _fields = source.GetType().GetFields(flags).ToDictionary(f => f.Name);
        _methods = source.GetType().GetMethods(flags).Where(m => !m.IsSpecialName && m.DeclaringType != typeof(object))
            .GroupBy(m => m.Name).ToDictionary(g => g.Key, g => g.ToArray());
    }
    private static Godot.Collections.Dictionary Result(Func<Variant> operation)
    {
        var result = new Godot.Collections.Dictionary();
        try { result["value"] = operation(); result["ok"] = true; }
        catch (Exception error) { result["ok"] = false; result["error"] = error.ToString(); }
        return result;
    }
    public Godot.Collections.Dictionary _ultralight_get_result(string name) => Result(() => _ultralight_get(name));
    public Godot.Collections.Dictionary _ultralight_keys_result() => Result(() => _ultralight_keys());
    public Godot.Collections.Dictionary _ultralight_set_result(string name, Variant value) => Result(() => { _ultralight_set(name, value); return default; });
    public Godot.Collections.Dictionary _ultralight_delete_result(string name) => Result(() => { _ultralight_delete(name); return default; });
    public Godot.Collections.Dictionary _ultralight_call_result(string name, GArray arguments) => Result(() => _ultralight_call(name, arguments));
    public bool _ultralight_alive() => Source is not GodotObject native || GodotObject.IsInstanceValid(native);
    public string _ultralight_kind() => Source is IList ? "array" : Source is IDictionary ? "dictionary" : "object";
    public bool _ultralight_method(string name) => _methods.ContainsKey(name);
    public bool _ultralight_has(string name)
    {
        if (Source is IList list) return name == "length" || int.TryParse(name, out int index) && index >= 0 && index < list.Count;
        if (Source is IDictionary dictionary) return dictionary.Contains(name);
        return _properties.ContainsKey(name) || _fields.ContainsKey(name) || _methods.ContainsKey(name);
    }
    public GArray _ultralight_keys()
    {
        var result = new GArray();
        if (Source is IList list) { for (int i = 0; i < list.Count; i++) result.Add(i.ToString()); }
        else if (Source is IDictionary dictionary)
        {
            foreach (var key in dictionary.Keys)
                if (key is string text) result.Add(text);
                else throw new NotSupportedException("Managed shared dictionaries require string keys");
        }
        else foreach (string name in _properties.Keys.Concat(_fields.Keys)) result.Add(name);
        return result;
    }
    public Variant _ultralight_get(string name)
    {
        if (Source is IList list) return name == "length" ? list.Count : _conversion.ToVariant(list[int.Parse(name)]);
        if (Source is IDictionary dictionary) return _conversion.ToVariant(dictionary[name]);
        if (_properties.TryGetValue(name, out var property)) return _conversion.ToVariant(property.GetValue(Source));
        if (_fields.TryGetValue(name, out var field)) return _conversion.ToVariant(field.GetValue(Source));
        return default;
    }
    public void _ultralight_set(string name, Variant value)
    {
        if (Source is IList list)
        {
            Type element = Source.GetType().IsArray ? Source.GetType().GetElementType()! : Source.GetType().GenericTypeArguments.FirstOrDefault() ?? typeof(object);
            if (name == "length")
            {
                int size = value.AsInt32();
                if (size < 0 || size > list.Count) throw new NotSupportedException("Managed typed lists cannot grow with undefined elements");
                while (list.Count > size) list.RemoveAt(list.Count - 1);
            }
            else
            {
                int index = int.Parse(name); var item = ManagedConversion.FromVariant(value, element);
                if (index == list.Count) list.Add(item); else list[index] = item;
            }
            return;
        }
        if (Source is IDictionary dictionary)
        {
            Type element = Source.GetType().GenericTypeArguments.LastOrDefault() ?? typeof(object);
            dictionary[name] = ManagedConversion.FromVariant(value, element); return;
        }
        if (_properties.TryGetValue(name, out var property) && property.CanWrite)
            property.SetValue(Source, ManagedConversion.FromVariant(value, property.PropertyType));
        else if (_fields.TryGetValue(name, out var field) && !field.IsInitOnly)
            field.SetValue(Source, ManagedConversion.FromVariant(value, field.FieldType));
        else throw new InvalidOperationException($"Member is read-only: {name}");
    }
    public void _ultralight_delete(string name)
    {
        if (Source is IDictionary dictionary) dictionary.Remove(name);
        else if (Source is IList list && int.TryParse(name, out int index) && index == list.Count - 1)
            list.RemoveAt(index);
        else throw new NotSupportedException("Managed typed lists cannot contain undefined holes");
    }
    public Variant _ultralight_call(string name, GArray arguments)
    {
        var candidates = _methods[name].Where(m => m.GetParameters().Length == arguments.Count ||
            m.GetParameters().Length == arguments.Count + 1 && m.GetParameters().Last().ParameterType == typeof(CancellationToken)).ToArray();
        if (candidates.Length != 1) throw new ArgumentException($"Expected one matching overload of {name}");
        var method = candidates[0]; var parameters = method.GetParameters();
        var values = new object?[parameters.Length];
        for (int i = 0; i < arguments.Count; i++) values[i] = ManagedConversion.FromVariant(arguments[i], parameters[i].ParameterType);
        var cancellation = new CancellationTokenSource();
        if (parameters.Length > arguments.Count) values[^1] = cancellation.Token;
        object? result;
        try { result = method.Invoke(Source, values); }
        catch (TargetInvocationException error) { cancellation.Dispose(); throw error.InnerException ?? error; }
        Task? task = result as Task;
        if (result is ValueTask valueTask) task = valueTask.AsTask();
        else if (result != null && result.GetType().IsGenericType && result.GetType().GetGenericTypeDefinition() == typeof(ValueTask<>))
            task = (Task)result.GetType().GetMethod("AsTask")!.Invoke(result, null)!;
        if (task == null) { cancellation.Dispose(); return _conversion.ToVariant(result); }
        var request = ClassDB.Instantiate("UltralightTask").AsGodotObject();
        var onCompleted = Callable.From(() =>
        {
            var completion = request.Call("get_result").AsGodotObject();
            if (!completion.Get("ok").AsBool()) cancellation.Cancel();
        });
        request.Connect("completed", onCompleted);
        Complete(task, request, cancellation, onCompleted);
        return request;
    }
    private async void Complete(Task task, GodotObject request, CancellationTokenSource cancellation, Callable onCompleted)
    {
        object? value = null; Exception? failure = null;
        try
        {
            await task.ConfigureAwait(false);
            if (task.GetType().IsGenericType) value = task.GetType().GetProperty("Result")?.GetValue(task);
        }
        catch (Exception error) { failure = error; }
        UltralightDispatcher.Post(() =>
        {
            try
            {
                if (!request.Call("is_completed").AsBool())
                {
                    if (failure is OperationCanceledException) request.Call("cancel");
                    else if (failure != null) request.Call("reject", "HostException", failure.ToString());
                    else request.Call("resolve", _conversion.ToVariant(value));
                }
            }
            catch (Exception error)
            {
                if (!request.Call("is_completed").AsBool())
                    request.Call("reject", "HostException", error.ToString());
            }
            finally { request.Disconnect("completed", onCompleted); cancellation.Dispose(); }
        });
    }
}
