extends SceneTree
const CONTROL_INTERVAL_SECONDS := 10.0
var callbacks := 0
var failures: Array[String] = []

func _initialize() -> void:
    run.call_deferred()

func clicked(_count: int) -> void:
    callbacks += 1

func wait_between_controls() -> void:
    await create_timer(CONTROL_INTERVAL_SECONDS).timeout

func wait_ready(view: Control) -> void:
    for i in 600:
        await process_frame
        if view.call("execute_script", "typeof clicked === 'function'") == true:
            for j in 20: await process_frame
            return
    failures.append("Sample 4 HTML did not load")

func run() -> void:
    var packed: PackedScene = load("res://main.tscn")
    var scene := packed.instantiate()
    root.add_child(scene)
    var gui := scene.get_node("Gui")
    var view: Control = gui.get_node("SubViewport/UltralightView")
    await wait_ready(view)
    if not failures.is_empty():
        quit(1)
        return
    if not view.call("is_accelerated"): failures.append("Sample 4 did not enable GPU")
    view.call("bind_func", "godot_clicked", Callable(self, "clicked"))
    var preview: TextureRect = gui.find_child("Preview", true, false)
    var rect := Vector2(view.call("execute_script", "document.getElementById('click').getBoundingClientRect().left + 20"), view.call("execute_script", "document.getElementById('click').getBoundingClientRect().top + 15"))
    var position := preview.global_position + rect * preview.size / Vector2(1024, 768)
    await click_at(position)
    if view.call("execute_script", "window.clickCount") != 1 or callbacks != 1:
        failures.append("scaled preview input/JS callback failed")
    var fps: Label = gui.get_node("SubViewport/PerformanceOverlay/FPSLabel")
    if not fps.text.begins_with("FPS ") or not "显卡模式" in fps.text:
        failures.append("Godot FPS overlay missing")
    print("SAMPLE4_FPS ", fps.text)
    RenderingServer.force_draw.call_deferred(false)
    await RenderingServer.frame_post_draw
    root.get_texture().get_image().save_png("user://sample4.png")
    var stats: Dictionary = view.call("get_render_statistics")
    if stats.cpu_page_upload_bytes != 0 or stats.cpu_mipmap_count != 0:
        failures.append("Sample 4 used CPU page uploads/mipmaps")
    print("SAMPLE4_STATS ", JSON.stringify(stats))
    for expected_gpu in [false, true]:
        var previous_gpu_material: WeakRef
        if not expected_gpu:
            previous_gpu_material = weakref(view.material)
        var key := InputEventKey.new()
        key.physical_keycode = KEY_G; key.keycode = KEY_G; key.pressed = true
        root.push_input(key)
        await wait_between_controls()
        await wait_ready(view)
        if view.call("is_accelerated") != expected_gpu:
            failures.append("G backend toggle failed")
        var switched_stats: Dictionary = view.call("get_render_statistics")
        if switched_stats.has("draws") != expected_gpu:
            failures.append("statistics retained the previous backend's GPU fields")
        if previous_gpu_material != null and previous_gpu_material.get_ref() != null:
            failures.append("GPU View material survived CPU backend switch")
    var load_key := InputEventKey.new()
    load_key.physical_keycode = KEY_L; load_key.keycode = KEY_L; load_key.pressed = true
    root.push_input(load_key)
    await wait_between_controls()
    if view.call("execute_script", "objectCount") != 240:
        failures.append("L workload toggle failed")
    var workload_key := InputEventKey.new()
    workload_key.physical_keycode = KEY_B; workload_key.keycode = KEY_B; workload_key.pressed = true
    root.push_input(workload_key)
    await wait_between_controls()
    if view.call("execute_script", "benchmarkMode") != "canvas":
        failures.append("B canvas workload toggle failed")
    RenderingServer.force_draw.call_deferred(false)
    await RenderingServer.frame_post_draw
    root.get_texture().get_image().save_png("user://sample4-canvas.png")
    for expected_cap in [60,0]:
        var cap_key := InputEventKey.new()
        cap_key.physical_keycode = KEY_V; cap_key.keycode = KEY_V; cap_key.pressed = true
        root.push_input(cap_key)
        await wait_between_controls()
        if Engine.max_fps != expected_cap:
            failures.append("V frame cap toggle failed")
    # Exercise clickable controls independently of keyboard shortcuts.
    for item in [["CPUButton", false], ["GPUButton", true]]:
        var button: Button = gui.find_child(item[0], true, false)
        await click_at(button.get_global_rect().get_center())
        await wait_ready(view)
        if view.call("is_accelerated") != item[1]:
            failures.append("clickable backend control failed: " + item[0])
    await click_at(gui.find_child("Load480", true, false).get_global_rect().get_center())
    print("SAMPLE4_LOAD_CLICK ", view.call("execute_script", "objectCount"))
    await click_at(gui.find_child("SVGButton", true, false).get_global_rect().get_center())
    if view.call("execute_script", "objectCount") != 480 or view.call("execute_script", "benchmarkMode") != "svg":
        failures.append("clickable workload/load controls failed")
    print("SAMPLE4_CONTROLS ", view.call("execute_script", "objectCount"), " ", view.call("execute_script", "benchmarkMode"))
    if view.call("execute_script", "vectorObjects.length") != 480:
        failures.append("SVG object count mismatch")
    await click_at(gui.find_child("LimitButton", true, false).get_global_rect().get_center())
    if Engine.max_fps != 60: failures.append("clickable frame cap control failed")
    var original_size := root.size
    root.size = Vector2i(1100, 900)
    for i in 5: await process_frame
    if gui.get_node("SubViewport").size != Vector2i(1024,768) or view.size != Vector2(1024,768):
        failures.append("window resize changed rendering resolution")
    root.size = original_size
    scene.queue_free()
    for i in 10: await process_frame
    scene = packed.instantiate()
    root.add_child(scene)
    view = scene.get_node("Gui/SubViewport/UltralightView")
    await wait_ready(view)
    if not view.call("is_accelerated"): failures.append("scene reload lost GPU")
    scene.queue_free()
    for i in 10: await process_frame
    print("SAMPLE4 ", "PASS" if failures.is_empty() else "FAIL", " ", failures)
    print("ARTIFACTS ", ProjectSettings.globalize_path("user://"))
    quit(0 if failures.is_empty() else 1)

func click_at(position: Vector2) -> void:
    var motion := InputEventMouseMotion.new()
    motion.position = position
    root.push_input(motion)
    for pressed in [true, false]:
        var event := InputEventMouseButton.new()
        event.button_index = MOUSE_BUTTON_LEFT
        event.position = position
        event.pressed = pressed
        root.push_input(event)
    # Deliver the synthetic press/release together so OS mouse polling cannot
    # cancel the injected click between frames. Then let container layouts settle.
    for i in 2: await process_frame
    await wait_between_controls()
