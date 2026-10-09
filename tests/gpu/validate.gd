extends SceneTree

var failures: Array[String] = []
var callbacks: Array[int] = []
var views: Array[Control] = []
var ports: Array[SubViewport] = []
var expect_gpu := "--expect-gpu" in OS.get_cmdline_user_args()

func _initialize() -> void:
    run.call_deferred()

func check(condition: bool, message: String) -> void:
    if not condition:
        failures.append(message)
        push_error(message)

func frames(count: int) -> void:
    for i in count:
        await process_frame
        if DisplayServer.get_name() == "headless":
            Engine.get_singleton("UltralightSingleton").call("update_frame")
            for view in views: view.call("on_update_frame")
        else:
            # Advance drawing and scroll timers even when the test window
            # is obscured. Forced drawing is limited to the test harness.
            RenderingServer.force_draw(false)

func make_view(gpu: bool) -> Control:
    var port := SubViewport.new()
    port.size = Vector2i(1024, 768)
    port.transparent_bg = true
    port.render_target_update_mode = SubViewport.UPDATE_ALWAYS
    root.add_child(port)
    ports.append(port)
    var view: Control = ClassDB.instantiate("UltralightView")
    var initial_stats: Dictionary = view.call("get_render_statistics")
    check(initial_stats.accelerated == false and not initial_stats.has("gpu_available") and not initial_stats.has("draws"), "uninitialized View queried GPU statistics")
    view.set("lazy", true)
    view.set("transparent", true)
    view.set("accelerated", gpu)
    view.set("htmlUrl", "file:///gpu-validation.html")
    view.size = Vector2(1024, 768)
    view.connect("on_window_object_ready", func(): view.call("bind_func", "godot_clicked", func(n): callbacks.append(int(n))))
    port.add_child(view)
    view.call("init_view")
    views.append(view)
    return view

func wait_loaded(view: Control) -> void:
    # Loading local files and fonts is asynchronous. Bound callback availability
    # also ensures OnWindowObjectReady and the JS bridge have run.
    for i in 600:
        await process_frame
        if view.call("execute_script", "typeof clicked === 'function' && !!document.getElementById('click')") == true:
            await frames(20)
            return
    check(false, "local HTML did not load")

func snapshot(port: Viewport, name: String) -> Image:
    # Explicit validation readback only, never part of the driver/display loop.
    # Force a capture so frame_post_draw can complete for an obscured window.
    RenderingServer.force_draw.call_deferred(false)
    await RenderingServer.frame_post_draw
    var image := port.get_texture().get_image()
    image.save_png("user://" + name + ".png")
    return image

func run() -> void:
    check(ClassDB.class_exists("UltralightView"), "extension not loaded")
    if not failures.is_empty():
        quit(1)
        return
    ThemeDB.get_default_theme().default_font = load("res://龙珠体ZHS-Regular.ttf")
    var cpu := make_view(false)
    var gpu := make_view(true)
    await wait_loaded(cpu)
    await wait_loaded(gpu)
    if not failures.is_empty():
        quit(1)
        return
    check(gpu.call("is_accelerated") == expect_gpu, "wrong backend/fallback")
    check(cpu.call("is_accelerated") == false, "CPU default changed")
    var cpu_stats: Dictionary = cpu.call("get_render_statistics")
    check(not cpu_stats.has("gpu_available") and not cpu_stats.has("bitmap_upload_bytes") and not cpu_stats.has("draws"), "CPU statistics include unrelated GPU activity")
    var backend_stats: Dictionary = gpu.call("get_render_statistics")
    check(backend_stats.has("draws") == expect_gpu, "GPU statistics presence does not match actual backend")
    for view in views:
        # Actual input injection uses the same public forwarding interface as Sample 4.
        var pos: Vector2 = Vector2(view.call("execute_script", "document.getElementById('click').getBoundingClientRect().left + 20"), view.call("execute_script", "document.getElementById('click').getBoundingClientRect().top + 15"))
        view.call("fire_mouse_event", pos, MOUSE_BUTTON_LEFT, true)
        view.call("fire_mouse_event", pos, MOUSE_BUTTON_LEFT, false)
        await frames(10)
        check(view.call("execute_script", "window.clickCount") == 1, "mouse click failed")
        view.call("fire_mouse_event", Vector2(100, 410), MOUSE_BUTTON_NONE, false)
        view.call("fire_mouse_event", Vector2(100, 410), MOUSE_BUTTON_WHEEL_DOWN, true)
        await create_timer(0.2).timeout
        await frames(10)
        check(float(view.call("execute_script", "document.querySelector('.scroll').scrollTop")) > 0, "scroll failed")
        view.call("execute_script", "document.querySelector('.scroll').scrollTop = 0")
    check(callbacks.size() == 2, "JS callback failed")
    if DisplayServer.get_name() != "headless":
        var cpu_image := await snapshot(ports[0], "cpu")
        var gpu_image := await snapshot(ports[1], "gpu")
        var nonempty := 0
        for y in range(40, 450, 20):
            for x in range(40, 650, 20):
                if gpu_image.get_pixel(x, y).a > 0.5: nonempty += 1
        check(nonempty > 100, "GPU output blank")
        if expect_gpu:
            var panel := gpu_image.get_pixel(700,110)
            check(abs(panel.r-248.0/255.0*0.85) < 0.02 and abs(panel.a-0.85) < 0.02, "premultiplied alpha incorrect")
            check(gpu_image.get_pixel(800,700).a == 0, "transparent padding incorrect")
        # Opaque bitmap interiors provide a color-channel/orientation oracle.
        var bitmap_x := int(gpu.call("execute_script", "document.querySelector('img').getBoundingClientRect().left"))
        var bitmap_y := int(gpu.call("execute_script", "document.querySelector('img').getBoundingClientRect().top")) + 20
        for x in [bitmap_x+16, bitmap_x+48, bitmap_x+80]:
            var a := cpu_image.get_pixel(x, bitmap_y)
            var b := gpu_image.get_pixel(x, bitmap_y)
            check(abs(a.r-b.r)+abs(a.g-b.g)+abs(a.b-b.b) < 0.12, "bitmap RGB mismatch at %s" % x)
    print("INITIAL_STATS ", JSON.stringify(gpu.call("get_render_statistics")))
    var other_gpu: Control
    var other_uv: Vector4
    if expect_gpu:
        other_gpu = make_view(true)
        other_gpu.size = Vector2(779, 519)
        ports[2].size = Vector2i(779, 519)
        await wait_loaded(other_gpu)
        check(gpu.material != other_gpu.material, "GPU Views share mutable display material")
        check(gpu.material.shader == other_gpu.material.shader, "GPU Views do not share cached display Shader")
        other_uv = other_gpu.material.get_shader_parameter("uv_rect")
    for i in 45:
        var size := Vector2(777+i%7, 513+i%9)
        cpu.size = size
        ports[0].size = Vector2i(size)
        gpu.size = size
        ports[1].size = Vector2i(size)
        await frames(2)
        check(cpu.texture != null and cpu.texture.get_width() == int(size.x) and cpu.texture.get_height() == int(size.y), "CPU texture not refreshed after resize")
        check(gpu.texture != null, "GPU/fallback texture not published after resize")
        if expect_gpu:
            check(other_gpu.material.get_shader_parameter("uv_rect") == other_uv, "resizing one GPU View changed another View's UV parameters")
    cpu.size = Vector2(1024, 768)
    ports[0].size = Vector2i(1024, 768)
    gpu.size = Vector2(1024, 768)
    ports[1].size = Vector2i(1024, 768)
    await frames(20)
    gpu.call("init_view")
    await wait_loaded(gpu)
    var final_stats: Dictionary = gpu.call("get_render_statistics")
    if expect_gpu:
        check(final_stats.cpu_page_upload_bytes == 0, "GPU path uploaded a CPU page")
        check(final_stats.cpu_mipmap_count == 0, "GPU path generated CPU mipmaps")
        check(final_stats.draws > 0, "no RD draws")
    print("FINAL_STATS ", JSON.stringify(final_stats))
    # View destruction and a second generation exercise scene-reload lifetime.
    for port in ports: port.queue_free()
    views.clear(); ports.clear()
    await frames(10)
    var reloaded := make_view(true)
    await wait_loaded(reloaded)
    check(reloaded.call("is_accelerated") == expect_gpu, "reload backend changed")
    for port in ports: port.queue_free()
    views.clear(); ports.clear()
    await frames(10)
    print("VALIDATION ", "PASS" if failures.is_empty() else "FAIL", " ", failures)
    print("ARTIFACTS ", ProjectSettings.globalize_path("user://"))
    quit(0 if failures.is_empty() else 1)
