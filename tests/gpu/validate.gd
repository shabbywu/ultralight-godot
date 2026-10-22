extends SceneTree

var failures: Array[String] = []
var callbacks: Array[int] = []
var views: Array[Control] = []
var ports: Array[SubViewport] = []
var expect_gpu := "--expect-gpu" in OS.get_cmdline_user_args()

class BridgeProbe extends RefCounted:
    var label := "中文 🐉"
    func echo(value): return value

class QueryProbe extends RefCounted:
    var getter_reads := 0
    var dynamic_enabled := false
    var counted: String:
        get:
            getter_reads += 1
            return "读取 🐉"
    func echo(value): return value
    func _get_property_list() -> Array[Dictionary]:
        if not dynamic_enabled: return []
        return [
            {"name":"Bridge Group", "type":TYPE_NIL, "usage":PROPERTY_USAGE_GROUP},
            {"name":"dynamic_value", "type":TYPE_STRING, "usage":PROPERTY_USAGE_DEFAULT},
            {"name":"dynamic_value", "type":TYPE_STRING, "usage":PROPERTY_USAGE_DEFAULT},
            {"name":"hidden_value", "type":TYPE_STRING, "usage":PROPERTY_USAGE_INTERNAL},
        ]
    func _get(property: StringName):
        if property == &"dynamic_value": return "动态 🐉"
        return null

func check_members(view: Control) -> void:
    for query in [false, true]:
        var holder: Array = [QueryProbe.new()]
        view.call("bind_object_legacy", "reentrant_probe", holder[0], query)
        view.call("bind_func", "free_reentrant_probe", func(): holder.clear())
        check(view.call("execute_script", "(()=>{try{reentrant_probe.getter_reads={get x(){free_reentrant_probe();return 1}};return false}catch(e){return /no longer valid/.test(e.message)}})()") == true, "setter accessed an object freed by JS conversion")
    var probe := QueryProbe.new()
    view.call("bind_object_legacy", "query_probe", probe, true)
    var reads := probe.getter_reads
    var keys = view.call("execute_script", "Object.keys(query_probe)")
    check(keys is Array and "counted" in keys and "echo" not in keys, "query mode did not enumerate properties only")
    check(view.call("execute_script", "'counted' in query_probe && Reflect.has(query_probe,'echo') && !Reflect.has(query_probe,'missing_member')") == true, "query mode member existence failed")
    check(probe.getter_reads == reads, "member query or enumeration evaluated a getter")
    check(view.call("execute_script", "typeof query_probe.missing_member === 'undefined'") == true, "missing member did not fall through to JS lookup")
    check(view.call("execute_script", "query_probe.echo(query_probe.counted)") == "读取 🐉", "query mode property or method access failed")
    check(probe.getter_reads == reads + 1, "query mode read did not call getter once")
    probe.dynamic_enabled = true
    keys = view.call("execute_script", "Object.keys(query_probe)")
    check(keys is Array and keys.count("dynamic_value") == 1 and "Bridge Group" not in keys and "hidden_value" not in keys, "metadata update, filtering or deduplication failed")
    check(view.call("execute_script", "query_probe.dynamic_value") == "动态 🐉", "dynamic metadata member unavailable")
    probe.dynamic_enabled = false
    check(view.call("execute_script", "!Reflect.has(query_probe,'dynamic_value')") == true, "removed metadata member still visible")
    check(view.call("execute_script", "query_probe") == probe, "extended proxy lost Godot object identity")
    probe = null
    check(view.call("execute_script", "!Reflect.has(query_probe,'counted') && Object.keys(query_probe).length===0") == true, "freed query object was accessed")

func check_bridge(view: Control) -> void:
    check(view.call("execute_script", "window.bridgeAtFirstScript") == true, "page globals were not bound before the first script")
    view.call("bind_func", "godot_echo", func(value): return value)
    var data = view.call("execute_script", "godot_echo({text:'中文 🐉',number:4294967297,values:[true,null,1.25,{x:'嵌套'}]})")
    check(data is Dictionary and data.text == "中文 🐉" and data.number == 4294967297 and data.values == [true,null,1.25,{"x":"嵌套"}], "JS nested data/Unicode/large number roundtrip failed")
    check(view.call("execute_script", "godot_echo(undefined)") == null, "undefined mapping failed")
    # Godot's String conversion truncates NUL even with an explicit UTF-16 length.
    check(view.call("execute_script", "godot_echo({['a'+String.fromCharCode(0)+'b']:'中'+String.fromCharCode(0)+'文 🐉'})") == {"a":"中"}, "Godot NUL boundary behavior changed")
    check(view.call("execute_script", "godot_echo([,true,,3])") == [null,true,null,3.0], "sparse array conversion failed")
    check(view.call("execute_script", "godot_echo(Object.create({inherited:7},{own:{value:3,enumerable:true}}))") == {"own":3.0,"inherited":7.0}, "prototype enumeration semantics changed")
    check(view.call("execute_script", "(()=>{let shared={x:7};return godot_echo([shared,shared])})()") == [{"x":7.0},{"x":7.0}], "shared sibling conversion was rejected")
    check(view.call("execute_script", "(()=>{let x={};for(let i=0;i<63;i++)x={child:x};godot_echo(x);return true})()") == true, "64-container JS traversal was rejected")
    check(view.call("execute_script", "(()=>{try{let x={};for(let i=0;i<64;i++)x={child:x};godot_echo(x);return false}catch(e){return /nested/.test(e.message)}})()") == true, "JS traversal depth was not limited")
    var cycle: Array = []
    cycle.append(cycle)
    view.call("bind_func", "godot_cycle", func(): return cycle)
    check(view.call("execute_script", "(()=>{try{godot_cycle();return false}catch(e){return /nested/.test(e.message)}})()") == true, "cyclic Godot data was accepted")
    cycle.clear()
    var nested: Array = []
    for i in 64: nested = [nested]
    view.call("bind_func", "godot_nested", func(): return nested)
    check(view.call("execute_script", "(()=>{try{godot_nested();return false}catch(e){return /nested/.test(e.message)}})()") == true, "Godot write depth was not limited")
    var fn: Callable = view.call("execute_script", "(function(value){return godot_echo(value);})")
    var returned = fn.call({"数字":4294967297,"值":[true,null,"中文 🐉"]})
    print("BRIDGE_FN ", fn.is_valid(), " ", returned)
    check(fn.is_valid() and returned is Dictionary and returned["数字"] == 4294967297 and returned["值"] == [true,null,"中文 🐉"], "JS function Callable conversion failed")
    var cached: Callable = view.call("execute_script", "window.cachedFunction=function(x){return x};cachedFunction")
    check(view.call("execute_script", "cachedFunction") == cached, "same-document JS Callable identity changed")
    view.call("execute_script", "(()=>{let f=document.createElement('iframe');f.srcdoc='<script>1</script>';document.body.appendChild(f);return true})()")
    await frames(10)
    check(fn.is_valid() and fn.call("child frame") == "child frame", "child frame invalidated main JS Callable")
    check(view.call("execute_script", "(function(){try{var x={};x.self=x;godot_echo(x);return false;}catch(e){return /Cyclic/.test(e.message);}})()") == true, "cyclic JS data did not raise a catchable exception")
    check(view.call("execute_script", "(function(){try{godot_echo({get x(){throw new Error('getter failed');}});return false;}catch(e){return /getter failed/.test(e.message);}})()") == true, "throwing getter escaped the bridge")
    var object := BridgeProbe.new()
    view.call("bind_object_legacy", "probe", object)
    check(view.call("execute_script", "probe.label='修改 🐉';probe.echo(probe.label)") == "修改 🐉" and object.label == "修改 🐉", "Godot object method/property binding failed")
    object = null
    check(view.call("execute_script", "(function(){try{probe.echo('freed');return false;}catch(e){return /no longer valid/.test(e.message);}})()") == true, "freed Godot object was accessed")
    check_members(view)
    # An old document's handles must expire after real navigation.
    view.set("htmlUrl", "file:///gpu-validation.html?bridge-navigation=1")
    await frames(30)
    await wait_loaded(view)
    check(not fn.is_valid(), "old JS Callable survived navigation")
    check(view.call("execute_script", "typeof godot_clicked") == "function", "global callback was not rebound after navigation")

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
    var deadline := Time.get_ticks_msec() + 10000
    while Time.get_ticks_msec() < deadline:
        await frames(1)
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
    for view in views:
        await check_bridge(view)
    if DisplayServer.get_name() != "headless":
        var cpu_image := await snapshot(ports[0], "cpu")
        var gpu_image := await snapshot(ports[1], "gpu")
        var nonempty := 0
        for y in range(40, 450, 20):
            for x in range(40, 650, 20):
                if gpu_image.get_pixel(x, y).a > 0.5: nonempty += 1
        check(nonempty > 100, "GPU output blank")
        var glyph_pixels := 0
        for y in range(40, 85):
            for x in range(770, 1010):
                if gpu_image.get_pixel(x,y).a > 0.3: glyph_pixels += 1
        check(glyph_pixels > 100, "small raster glyph atlas output blank")
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
    if expect_gpu:
        # Exercise actual CSS filter passes, including offscreen targets and Flush.
        gpu.call("execute_script", "installValidationFilters()")
        await frames(30)
        var filtered := await snapshot(ports[1], "gpu-filters")
        check(filtered.get_pixel(80,610).g > 0.8 and filtered.get_pixel(80,610).b > 0.8, "CSS invert output incorrect")
        check(filtered.get_pixel(210,610).r > 0.8, "CSS blur output blank")
        check(filtered.get_pixel(390,638).b > 0.1, "CSS drop-shadow output blank")
        var programs: Dictionary = gpu.call("get_render_statistics").gpu_draws_by_shader
        for name in ["Fill", "FilterBasic", "FilterBlur", "FilterDropShadow", "FillPhoton"]:
            check(programs[name] > 0, "SDK 2 program did not execute: " + name)
        gpu.call("execute_script", "installValidationPaths()")
        await frames(30)
        var path_image := await snapshot(ports[1], "gpu-svg-paths")
        check(path_image.get_pixel(18,12).g > 0.9, "complex analytic path output incorrect")
        print("PROGRAM_COVERAGE ", JSON.stringify(gpu.call("get_render_statistics").gpu_draws_by_shader))
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
