using Godot;
using Godot.NativeInterop;
using InventorySystem;

public partial class Main : Node2D
{
	private TextureRect UI { get; set; }
	private Inventory _bag = new Inventory();
	private Inventory _ground = new Inventory();
	private Button _reloadButton;
	private Button _newButton;
	private bool _started = false;

	
	// Called when the node enters the scene tree for the first time.
	public override void _Ready()
	{
		UI = GetNode<TextureRect>("UI");

		UI.Connect("on_window_object_ready", Callable.From(() =>
		{
			UI.Call("bind_object", "__godot_bag", _bag);
			UI.Call("bind_object", "__godot_ground", _ground);
			StartRemoveInspectorServer();
		}));

		_reloadButton = GetNode<Button>("HBoxContainer/Reload");
		_reloadButton.Connect("pressed", Callable.From(() =>
		{
			UI.Set("htmlUrl", "file:///webfe/dist/index.html");
		}));
		
		_newButton = GetNode<Button>("HBoxContainer/New");
		_newButton.Connect("pressed", Callable.From(() =>
		{
			_ground.AddItem(ItemDB.GenerateRandomItem());
		}));
	}

	// Called every frame. 'delta' is the elapsed time since the previous frame.
	public override void _Process(double delta)
	{
	}

	private void StartRemoveInspectorServer()
	{
		if (!_started) {
			Engine.Singleton.GetSingleton("UltralightSingleton").Call("start_remove_inspector_server", "127.0.0.1", 19999);
			_started = true;
		}
	}
	
	public override void _Notification(int what)
	{
		if (what == NotificationPredelete)
		{
			_bag.Free();
			_ground.Free();
		}
	}
}
