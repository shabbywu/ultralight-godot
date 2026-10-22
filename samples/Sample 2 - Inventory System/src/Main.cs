using AshenReliquary;
using AshenReliquary.Backend;
using Godot;

/// <summary>Scene composition, Godot lifecycle and backend-originated UI notifications.</summary>
public partial class Main : Control
{
    private Control _view = null!;
    private GameBridge? _bridge;
    private bool _documentReady;
    private bool _viewInitialized;
    public Game? Game { get; private set; }

    public override void _Ready()
    {
        _view = GetNode<Control>("Layout/UI");
        try
        {
            using var file = Godot.FileAccess.Open("res://Backend/catalog.json", Godot.FileAccess.ModeFlags.Read);
            var items = ItemDB.FromJson(file.GetAsText());
            Game = new Game(items, new SnapshotStore(ProjectSettings.GlobalizePath("user://inventory-v1.json")));
            _bridge = new GameBridge();
            _bridge.Initialize(Game);
            Game.Changed += OnChanged;
            Game.ObserverFailed += OnObserverFailed;
            _view.Connect("on_window_object_ready", Callable.From(BindDocument));
            _view.Connect("on_dom_ready", Callable.From(() => _documentReady = true));
            GetNode<Button>("Layout/Toolbar/New").Pressed += () => Game.CreateRelic();
            GetNode<Button>("Layout/Toolbar/Reload").Pressed += Reload;
            _view.Set("htmlUrl", "file:///ui/index.html");
            _view.Resized += InitializeView;
            Callable.From(InitializeView).CallDeferred();
        }
        catch (Exception error)
        {
            GD.PushError(error.Message);
            GetNode<Label>("Layout/Toolbar/Status").Text = "Load blocked: " + error.Message;
        }
    }
    private void InitializeView()
    {
        // Containers assign a nonzero size after child nodes become ready.
        if (_viewInitialized || _view.Size.X <= 0 || _view.Size.Y <= 0) return;
        _viewInitialized = true;
        _view.Resized -= InitializeView;
        _view.Call("init_view");
    }

    private void BindDocument()
    {
        _documentReady = false;
        _view.Call("bind_object", "reliquaryBackend", _bridge!);
        _view.Call("execute_script", "window.inventory={backend:reliquaryBackend}; void 0");
    }
    public void Reload()
    {
        _documentReady = false;
        _view.Call("execute_script", "location.reload(); void 0");
    }
    private void OnChanged(StateChanged value)
    {
        if (!_documentReady) return;
        // Bind a detached event payload via Godot's container conversion; no JSON or polling.
        _view.Call("bind_func", "reliquaryChange", Callable.From(() => WireCodec.Event(value)));
        _view.Call("execute_script", "window.dispatchEvent(new CustomEvent('inventory-changed',{detail:reliquaryChange()})); void 0");
    }
    private static void OnObserverFailed(Exception error) => GD.PushError("Inventory notification failed: " + error.Message);
    public override void _ExitTree()
    {
        if (Game is not null) { Game.Changed -= OnChanged; Game.ObserverFailed -= OnObserverFailed; }
        if (_view.IsConnected(Control.SignalName.Resized, Callable.From(InitializeView)))
            _view.Resized -= InitializeView;
        _bridge?.Dispose();
        _bridge = null;
    }
}
