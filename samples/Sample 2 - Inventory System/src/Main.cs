using Ultralight;
using AshenReliquary;
using AshenReliquary.Backend;
using Godot;

/// <summary>Scene composition, Godot lifecycle and backend-originated UI notifications.</summary>
public partial class Main : Control
{
    private Control _view = null!;
    private GameBridge? _bridge;
    private bool _documentReady;
    private UltralightApi? _api;
    private UltralightViewClient _web = null!;
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
            _web = UltralightViewClient.Wrap(_view);
            _api = new UltralightApi("inventory");
            _api.RegisterNativeObject("backend", _bridge, copyContainers: true);
            _api.Attach(_web);
            _view.Connect("load_started", Callable.From<GodotObject>(_ => _documentReady = false));
            _view.Connect("dom_ready", Callable.From<GodotObject>(_ => _documentReady = true));
            GetNode<Button>("Layout/Toolbar/New").Pressed += () => Game.CreateRelic();
            GetNode<Button>("Layout/Toolbar/Reload").Pressed += Reload;
            _ = _web.LoadUrlAsync("file:///ui/index.html");
        }
        catch (Exception error)
        {
            GD.PushError(error.Message);
            GetNode<Label>("Layout/Toolbar/Status").Text = "Load blocked: " + error.Message;
        }
    }
    public void Reload()
    {
        _documentReady = false;
        _web.Evaluate("location.reload(); void 0");
    }
    private void OnChanged(StateChanged value)
    {
        if (!_documentReady) return;
        // Bind a detached event payload via Godot's container conversion; no JSON or polling.
        _api!.Register("change", Callable.From(() => WireCodec.Event(value)));
        _web.Evaluate("window.dispatchEvent(new CustomEvent('inventory-changed',{detail:inventory.change()})); void 0");
    }
    private static void OnObserverFailed(Exception error) => GD.PushError("Inventory notification failed: " + error.Message);
    public override void _ExitTree()
    {
        if (Game is not null) { Game.Changed -= OnChanged; Game.ObserverFailed -= OnObserverFailed; }
        _api?.Dispose();
        _bridge?.Dispose();
        _bridge = null;
    }
}
