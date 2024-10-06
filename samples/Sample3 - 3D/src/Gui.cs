using System;
using Godot;

namespace ThreeD;

public partial class Gui : Node3D {
    private SubViewport _nodeViewPort;
    private TextureRect _ultralightView;
    private MeshInstance3D _nodeQuad;
    private Area3D _nodeArea;

    private bool _isMouseInside = false;
    private Vector2 _lastEventPos2d = Vector2.Zero;
    private float _lastEventTime = 0;

    [Signal]
    public delegate void OnDomReadyEventHandler();

    [Signal]
    public delegate void OnWindowObjectReadyEventHandler();

    // Called when the node enters the scene tree for the first time.
    public override void _Ready() {
        _nodeViewPort = GetNode<SubViewport>("SubViewport");
        _ultralightView = GetNode<TextureRect>("SubViewport/UltralightView");
        _nodeQuad = GetNode<MeshInstance3D>("Quad");
        _nodeArea = GetNode<Area3D>("Quad/Area3D");

        _nodeArea.MouseEntered += OnMouseEnteredArea;
        _nodeArea.MouseExited += OnMouseExitedArea;
        _nodeArea.InputEvent += OnMouseInputEvent;

        _ultralightView.Connect("on_dom_ready", Callable.From(() => { backing_OnDomReady?.Invoke(); }));

        _ultralightView.Connect("on_window_object_ready",
                                Callable.From(() => { backing_OnWindowObjectReady?.Invoke(); }));

        _ultralightView.Call("init_view");
        Engine.Singleton.GetSingleton("UltralightSingleton").Call("start_remove_inspector_server", "127.0.0.1", 19999);
    }

    // Called every frame. 'delta' is the elapsed time since the previous frame.
    public override void _Process(double delta) {
    }

    void OnMouseEnteredArea() {
        _isMouseInside = true;
    }

    void OnMouseExitedArea() {
        _isMouseInside = false;
    }

    void OnMouseInputEvent(Node camera, InputEvent @event, Vector3 eventPosition, Vector3 normal, long shapeIdx) {
        if (@event is not InputEventMouse) {
            return;
        }

        var mouseEvent = (InputEventMouse) @event;

        var quadMeshSize = (_nodeQuad.Mesh as QuadMesh).Size;
        var now = Time.GetTicksMsec() / 1000.0f;
        var eventPos3d = _nodeQuad.GlobalTransform.AffineInverse() * eventPosition;
        var eventPos2d = _lastEventPos2d;

        if (_isMouseInside) {
            // Convert the relative event position from 3D to 2D.
            eventPos2d = new Vector2(eventPos3d.X, -eventPos3d.Y);

            // Right now the event position's range is the following: (-quad_size/2) -> (quad_size/2)
            // We need to convert it into the following range: -0.5 -> 0.5
            eventPos2d.X /= quadMeshSize.X;
            eventPos2d.Y /= quadMeshSize.Y;

            // Then we need to convert it into the following range: 0 -> 1
            eventPos2d.X += 0.5f;
            eventPos2d.Y += 0.5f;

            // Finally, we convert the position to the following range: 0 -> viewport.size
            eventPos2d.X *= _nodeViewPort.Size.X;
            eventPos2d.Y *= _nodeViewPort.Size.Y;
        }
        mouseEvent.Position = eventPos2d;
        mouseEvent.GlobalPosition = eventPos2d;
        if (@event is InputEventMouseMotion mouseMotion) {
            if (_lastEventPos2d == Vector2.Zero) {
                mouseMotion.Relative = Vector2.Zero;
            } else {
                mouseMotion.Relative = eventPos2d - _lastEventPos2d;
                mouseMotion.Velocity = mouseMotion.Relative / (now - _lastEventTime);
            }
        }

        _lastEventPos2d = eventPos2d;
        _lastEventTime = now;
        _nodeViewPort.PushInput(mouseEvent);
    }
}
