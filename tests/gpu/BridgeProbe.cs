using Godot;

public partial class BridgeProbe : RefCounted
{
    public string Label { get; set; } = "中文 🐉";
    private string _exportedLabel = "导出 🐉";
    public int GetterReads { get; private set; }
    [Export]
    public string ExportedLabel
    {
        get { GetterReads++; return _exportedLabel; }
        set { _exportedLabel = value; }
    }
    public Callable Function { get; set; }
    public bool AsyncCompleted { get; private set; }
    public Variant AsyncResult { get; private set; }
    public async void AwaitPromise(GodotObject promise)
    {
        if (!promise.Call("is_completed").AsBool()) await ToSignal(promise, "completed");
        AsyncResult = promise.Call("get_result");
        AsyncCompleted = true;
    }
    public Variant Echo(Variant value) => value;
    public Variant Invoke(Variant value) => Function.Call(value);
    public Callable GetCallback() => Callable.From<Variant, Variant>(Echo);
}
