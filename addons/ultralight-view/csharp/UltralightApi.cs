#nullable enable
using Godot;
using System;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;
using GArray = Godot.Collections.Array;

namespace Ultralight;

/// <summary>
/// Main-thread API registry. Definitions survive navigation; Dispose revokes
/// exports in every attached View without closing the Views themselves.
/// </summary>
public sealed class UltralightApi : IDisposable
{
    public GodotObject Native { get; }
    private readonly Dictionary<string, object> _handlers = new();
    internal readonly ManagedConversion Conversion = new();
    public UltralightApi(string @namespace)
    {
        Native = ClassDB.ClassCallStatic("UltralightAPI", "create", @namespace).AsGodotObject()
            ?? throw new ArgumentException("Namespace must be a JavaScript identifier", nameof(@namespace));
        UltralightDispatcher.Install();
    }
    public void Attach(UltralightViewClient view) => UltralightViewClient.Unwrap(Native.Call("attach", view.Native).AsGodotObject());
    public void Detach(UltralightViewClient view) => Native.Call("detach", view.Native);
    public void Register(string name, Callable function)
    {
        UltralightViewClient.Unwrap(Native.Call("register_function", name, function).AsGodotObject());
        _handlers[name] = function;
    }
    public void RegisterObject(string name, object instance, params string[] members)
    {
        var adapted = Conversion.Wrap(instance);
        using var options = ClassDB.Instantiate("UltralightBindingOptions").AsGodotObject();
        options.Set("members", members);
        UltralightViewClient.Unwrap(Native.Call("register_object", name, adapted, options).AsGodotObject());
        _handlers[name] = adapted;
    }
    public void RegisterNativeObject(string name, GodotObject instance, bool copyContainers = false, params string[] members)
    {
        using var options = ClassDB.Instantiate("UltralightBindingOptions").AsGodotObject();
        options.Set("copy_containers", copyContainers); options.Set("members", members);
        UltralightViewClient.Unwrap(Native.Call("register_object", name, instance, options).AsGodotObject());
        _handlers[name] = instance;
    }
    public void RegisterAsync(string name, Func<GArray, CancellationToken, Task<Variant>> handler)
    {
        var callable = Callable.From<GodotObject, GArray>((request, args) =>
            RunHandler(request, args, handler));
        UltralightViewClient.Unwrap(Native.Call("register_async", name, callable).AsGodotObject());
        _handlers[name] = callable;
    }
    public void RegisterAsync<T>(string name, Func<GArray, CancellationToken, Task<T>> handler)
        // Keep the Godot synchronization context for the managed-to-Variant conversion.
        => RegisterAsync(name, async (args, token) => Conversion.ToVariant(await handler(args, token)));
    public void RegisterAsync(string name, Func<GArray, CancellationToken, Task> handler)
        => RegisterAsync(name, async (args, token) => { await handler(args, token); return default(Variant); });
    public void RegisterValueTask<T>(string name, Func<GArray, CancellationToken, ValueTask<T>> handler)
        => RegisterAsync(name, async (args, token) => Conversion.ToVariant(await handler(args, token)));
    public void RegisterValueTask(string name, Func<GArray, CancellationToken, ValueTask<Variant>> handler)
        => RegisterAsync(name, (args, token) => handler(args, token).AsTask());
    public void RegisterValueTask(string name, Func<GArray, CancellationToken, ValueTask> handler)
        => RegisterAsync(name, async (args, token) => { await handler(args, token); return default(Variant); });
    private static async void RunHandler(GodotObject request, GArray args, Func<GArray, CancellationToken, Task<Variant>> handler)
    {
        var source = new CancellationTokenSource();
        var cancelled = Callable.From(() =>
        {
            var result = request.Call("get_result").AsGodotObject();
            if (!result.Get("ok").AsBool()) source.Cancel();
        });
        request.Connect("completed", cancelled);
        try
        {
            Variant value = await handler(args, source.Token);
            UltralightDispatcher.Post(() => { if (!request.Call("is_completed").AsBool()) request.Call("resolve", value); });
        }
        catch (OperationCanceledException)
        {
            UltralightDispatcher.Post(() => request.Call("cancel"));
        }
        catch (Exception error)
        {
            string message = error.ToString();
            UltralightDispatcher.Post(() => request.Call("reject", "HostException", message));
        }
        finally
        {
            UltralightDispatcher.Post(() =>
            {
                if (GodotObject.IsInstanceValid(request) && request.IsConnected("completed", cancelled))
                    request.Disconnect("completed", cancelled);
                source.Dispose();
            });
        }
    }
    public void Unregister(string name) { Native.Call("unregister", name); _handlers.Remove(name); }
    public void Dispose() { Native.Call("close"); _handlers.Clear(); Conversion.Clear(); }
}
