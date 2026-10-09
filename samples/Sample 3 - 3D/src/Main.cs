using Godot;
using System;
namespace ThreeD;
public partial class Main : Node3D {
    private Gui _gui;
    private RigidBody3D _box;
    private RigidBody3D _sphere;

    // Called when the node enters the scene tree for the first time.
    public override void _Ready()
    {
        _sphere = GetNode<RigidBody3D>("Sphere");
        _box = GetNode<RigidBody3D>("Box");
        
        _gui = GetNode<Gui>("Gui");
        _gui.OnDomReady += () =>
        {
            _sphere.Freeze = false;
            _box.Freeze = false;
        };
    }

    // Called every frame. 'delta' is the elapsed time since the previous frame.
    public override void _Process(double delta) {
    }
}
