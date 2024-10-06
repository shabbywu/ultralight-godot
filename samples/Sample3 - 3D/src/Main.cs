using Godot;
using System;
namespace ThreeD;
public partial class Main : Node3D {
    private Gui _gui;

    // Called when the node enters the scene tree for the first time.
    public override void _Ready() {
        _gui = GetNode<Gui>("Gui");
        _gui.OnDomReady += () => { GD.Print("on dom ready"); };
    }

    // Called every frame. 'delta' is the elapsed time since the previous frame.
    public override void _Process(double delta) {
    }
}
