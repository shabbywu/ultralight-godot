using System.Collections.Generic;
using Godot;

namespace GPUExample;

public partial class FrameGraph : Control {
    private readonly Queue<double> _frames = new();
    public void AddFrame(double milliseconds) {
        _frames.Enqueue(milliseconds);
        while (_frames.Count > 120) _frames.Dequeue();
        QueueRedraw();
    }
    public void ClearFrames() { _frames.Clear(); QueueRedraw(); }
    public override void _Draw() {
        var maxMs = 33.4;
        foreach (var frame in _frames) maxMs = System.Math.Max(maxMs, frame);
        var threshold = Size.Y * (1 - 16.7 / maxMs);
        DrawDashedLine(new Vector2(0, (float)threshold), new Vector2(Size.X, (float)threshold), new Color("41546d"), 1, 4);
        if (_frames.Count < 2) return;
        var points = new Vector2[_frames.Count];
        var i = 0;
        foreach (var frame in _frames) {
            points[i] = new Vector2(i * Size.X / (_frames.Count - 1), (float)(Size.Y * (1 - frame / maxMs)));
            i++;
        }
        DrawPolyline(points, new Color("5ee3b7"), 1.5f, true);
    }
}
