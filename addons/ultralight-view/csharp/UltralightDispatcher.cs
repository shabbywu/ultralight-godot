#nullable enable
using Godot;
using System;
using System.Collections.Concurrent;

namespace Ultralight;

/// <summary>Queues managed completions onto the Godot main thread.</summary>
public partial class UltralightDispatcher : Node
{
    private static readonly ConcurrentQueue<Action> Pending = new();
    private static UltralightDispatcher? _instance;
    internal static void Install()
    {
        if (_instance != null && IsInstanceValid(_instance)) return;
        _instance = new UltralightDispatcher { Name = "UltralightDispatcher", ProcessMode = ProcessModeEnum.Always };
        ((SceneTree)Engine.GetMainLoop()).Root.AddChild(_instance);
    }
    internal static void Post(Action action) => Pending.Enqueue(action);
    public override void _Process(double delta)
    {
        int count = Pending.Count;
        while (count-- > 0 && Pending.TryDequeue(out var action))
        {
            try { action(); } catch (Exception error) { GD.PushError(error.ToString()); }
        }
    }
    public override void _ExitTree() { _instance = null; }
}
