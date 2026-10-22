#nullable enable
using Godot;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using GArray = Godot.Collections.Array;

namespace Ultralight;

/// <summary>A live JS Array. Dispose invalidates aliases of this handle; Snapshot makes an independent copy.</summary>
public sealed class UltralightJsArray : IList<Variant>, IDisposable
{
    public GodotObject Native { get; }
    public UltralightJsArray(GodotObject value)
    {
        if (!value.IsClass("UltralightJSArray")) throw new ArgumentException("Expected a shared JS Array");
        Native = value;
    }
    public int Count
    {
        get { int count = Native.Call("size").AsInt32(); return count < 0 ? throw new ObjectDisposedException(nameof(UltralightJsArray)) : count; }
    }
    public bool IsReadOnly => false;
    public Variant this[int index]
    {
        get { CheckIndex(index); return UltralightViewClient.Unwrap(Native.Call("get_value", index).AsGodotObject()); }
        set { CheckIndex(index); UltralightViewClient.Unwrap(Native.Call("set_value", index, value).AsGodotObject()); }
    }
    private void CheckIndex(int index) { if (index < 0 || index >= Count) throw new ArgumentOutOfRangeException(nameof(index)); }
    private Variant Invoke(string method, params Variant[] arguments)
        => UltralightViewClient.Unwrap(Native.Call("invoke_method", method, new GArray(arguments)).AsGodotObject());
    public void Add(Variant value) => UltralightViewClient.Unwrap(Native.Call("append", value).AsGodotObject());
    public void Clear() => UltralightViewClient.Unwrap(Native.Call("set_value", "length", 0).AsGodotObject());
    public void Insert(int index, Variant value)
    {
        if (index < 0 || index > Count) throw new ArgumentOutOfRangeException(nameof(index));
        Invoke("splice", index, 0, value);
    }
    public void RemoveAt(int index) { CheckIndex(index); Invoke("splice", index, 1); }
    public int IndexOf(Variant value) { for (int i = 0; i < Count; i++) if (this[i].Equals(value)) return i; return -1; }
    public bool Contains(Variant value) => IndexOf(value) >= 0;
    public bool Remove(Variant value) { int index = IndexOf(value); if (index < 0) return false; RemoveAt(index); return true; }
    public void CopyTo(Variant[] array, int arrayIndex) { foreach (var value in this) array[arrayIndex++] = value; }
    public IEnumerator<Variant> GetEnumerator() { for (int i = 0; i < Count; i++) yield return this[i]; }
    IEnumerator IEnumerable.GetEnumerator() => GetEnumerator();
    public GArray Snapshot() => UltralightViewClient.Unwrap(Native.Call("snapshot").AsGodotObject()).AsGodotArray();
    public void Dispose() => Native.Call("close");
}

/// <summary>A live JS object's own enumerable string properties.</summary>
public sealed class UltralightJsObject : IDictionary<string, Variant>, IDisposable
{
    public GodotObject Native { get; }
    public UltralightJsObject(GodotObject value)
    {
        if (!value.IsClass("UltralightJSObject")) throw new ArgumentException("Expected a shared JS Object");
        Native = value;
    }
    public ICollection<string> Keys => UltralightViewClient.Unwrap(Native.Call("keys").AsGodotObject()).AsGodotArray().Select(v => v.AsString()).ToArray();
    public ICollection<Variant> Values => Keys.Select(key => this[key]).ToArray();
    public int Count => Keys.Count;
    public bool IsReadOnly => false;
    public Variant this[string key]
    {
        get { if (!ContainsKey(key)) throw new KeyNotFoundException(key); return UltralightViewClient.Unwrap(Native.Call("get_value", key).AsGodotObject()); }
        set => UltralightViewClient.Unwrap(Native.Call("set_value", key, value).AsGodotObject());
    }
    public void Add(string key, Variant value) { if (ContainsKey(key)) throw new ArgumentException("Key already exists", nameof(key)); this[key] = value; }
    public bool ContainsKey(string key) => Keys.Contains(key);
    public bool Remove(string key) { if (!ContainsKey(key)) return false; UltralightViewClient.Unwrap(Native.Call("remove", key).AsGodotObject()); return true; }
    public bool TryGetValue(string key, out Variant value) { if (!ContainsKey(key)) { value = default; return false; } value = this[key]; return true; }
    public void Add(KeyValuePair<string, Variant> item) => Add(item.Key, item.Value);
    public void Clear() { foreach (string key in Keys) Remove(key); }
    public bool Contains(KeyValuePair<string, Variant> item) => TryGetValue(item.Key, out var value) && value.Equals(item.Value);
    public void CopyTo(KeyValuePair<string, Variant>[] array, int arrayIndex) { foreach (var item in this) array[arrayIndex++] = item; }
    public bool Remove(KeyValuePair<string, Variant> item) => Contains(item) && Remove(item.Key);
    public IEnumerator<KeyValuePair<string, Variant>> GetEnumerator() { foreach (string key in Keys) yield return new(key, this[key]); }
    IEnumerator IEnumerable.GetEnumerator() => GetEnumerator();
    public Godot.Collections.Dictionary Snapshot() => UltralightViewClient.Unwrap(Native.Call("snapshot").AsGodotObject()).AsGodotDictionary();
    public void Dispose() => Native.Call("close");
}
