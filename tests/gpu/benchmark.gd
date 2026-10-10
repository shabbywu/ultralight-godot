extends "res://validate.gd"

func run() -> void:
    ThemeDB.get_default_theme().default_font = load("res://龙珠体ZHS-Regular.ttf")
    var view := make_view(expect_gpu)
    view.set("htmlUrl", "file:///canvas-benchmark.html")
    await wait_loaded(view)
    check(view.call("is_accelerated") == expect_gpu, "benchmark backend mismatch")
    if not failures.is_empty():
        quit(1)
        return
    var object_count := 480
    var workload := "canvas"
    var duration := 300
    var fps_limit := 0
    for arg in OS.get_cmdline_user_args():
        if arg.begins_with("--objects="): object_count = int(arg.get_slice("=", 1))
        if arg.begins_with("--workload="): workload = arg.get_slice("=", 1)
        if arg.begins_with("--duration="): duration = int(arg.get_slice("=", 1))
        if arg.begins_with("--fps-limit="): fps_limit = int(arg.get_slice("=", 1))
    check(duration >= 300, "benchmark duration must be at least 300 seconds")
    check(object_count in [120, 240, 480, 960, 1440], "benchmark requires 120, 240, 480, 960 or 1440 objects")
    check(fps_limit >= 0, "benchmark frame cap must not be negative")
    if not failures.is_empty():
        quit(1)
        return
    Engine.max_fps = fps_limit
    if fps_limit == 0: DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_DISABLED)
    view.call("execute_script", "startCanvasBenchmark(%d)" % object_count)
    view.call("execute_script", "setBenchmarkMode('%s')" % workload)
    view.call("execute_script", "stopCanvasBenchmark()")
    check(view.call("execute_script", "objectCount") == object_count, "benchmark object count mismatch")
    if not failures.is_empty():
        quit(1)
        return
    # Drive the same deterministic animation frame on both backends. Godot
    # FPS alone can include frames in which requestAnimationFrame did not run.
    for i in 40:
        await process_frame
        view.call("execute_script", "drawBenchmarkFrame(%f)" % (float(i)/60.0))
        RenderingServer.force_draw(false)
    var before: Dictionary = view.call("get_render_statistics")
    var canvas_before = JSON.parse_string(view.call("execute_script", "JSON.stringify(canvasBenchmarkStats)"))
    var frame_ms: Array[float] = []
    var records: Array = []
    var started := Time.get_ticks_usec()
    var previous := started
    var deadline := started + duration * 1000000
    var next_progress := started + 30000000
    var frame_count := 0
    while Time.get_ticks_usec() < deadline:
        await process_frame
        view.call("execute_script", "drawBenchmarkFrame(%f)" % (float(frame_count+40)/60.0))
        # The runner disables automatic rendering: exactly one forced frame
        # per sample, including when the test window is obscured.
        RenderingServer.force_draw(false)
        var now := Time.get_ticks_usec()
        frame_ms.append(float(now-previous)/1000.0)
        previous = now
        records.append(view.call("get_render_statistics"))
        frame_count += 1
        if now >= next_progress:
            print("BENCHMARK_PROGRESS ", (now-started)/1000000.0, " seconds, ", frame_count, " frames")
            next_progress += 30000000
    var elapsed := (Time.get_ticks_usec()-started)/1000000.0
    var after: Dictionary = view.call("get_render_statistics")
    var canvas_after = JSON.parse_string(view.call("execute_script", "JSON.stringify(canvasBenchmarkStats)"))
    check(canvas_after.frames-canvas_before.frames == frame_count, "benchmark animation count mismatch")
    if expect_gpu: check(after.draws > before.draws, "GPU did not draw during measurement")
    else: check(after.cpu_page_upload_bytes > before.cpu_page_upload_bytes, "CPU did not draw during measurement")
    var name := "gpu" if expect_gpu else "cpu"
    var sorted_frames := frame_ms.duplicate()
    sorted_frames.sort()
    var summary := {
        "frames": frame_count,
        "measured_seconds": elapsed,
        "fps": frame_count/elapsed,
        "median_ms": sorted_frames[int((frame_count-1)*0.5)],
        "p95_ms": sorted_frames[int((frame_count-1)*0.95)],
    }
    var file := FileAccess.open("user://benchmark-"+name+"-"+workload+".json", FileAccess.WRITE)
    file.store_string(JSON.stringify({
        "backend": name,
        "workload": workload,
        "objects": object_count,
        "duration_seconds": duration,
        "fps_limit": fps_limit,
        "summary": summary,
        "frame_ms": frame_ms,
        "stats": records,
        "before": before,
        "after": after,
        "canvas_before": canvas_before,
        "canvas_after": canvas_after,
    }))
    file.close()
    print("BENCHMARK ", name, " ", JSON.stringify(summary))
    print("ARTIFACTS ", ProjectSettings.globalize_path("user://"))
    for port in ports: port.queue_free()
    ports.clear(); views.clear()
    await frames(10)
    quit(0 if failures.is_empty() else 1)
