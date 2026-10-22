extends SceneTree
func _initialize(): call_deferred("run")
func run():
    var probe=load("res://ApiProbe.cs").new()
    root.add_child(probe)
    probe.connect("Finished", func(success): quit(0 if success else 1))
    probe.Run()
