extends "res://validate.gd"

func wait_promise(promise, label: String) -> bool:
    if not is_instance_valid(promise) or not promise.has_method("is_completed"):
        check(false, label + ": expected a JavaScriptPromise")
        return false
    var deadline := Time.get_ticks_msec() + 10000
    while not promise.is_completed() and Time.get_ticks_msec() < deadline:
        await frames(1)
    check(promise.is_completed(), label + ": Promise did not complete within 10 seconds")
    return promise.is_completed()

func run() -> void:
    check(ClassDB.class_exists("UltralightView"), "extension not loaded")
    if not failures.is_empty():
        quit(1)
        return
    ThemeDB.get_default_theme().default_font = load("res://龙珠体ZHS-Regular.ttf")
    for accelerated in [false, true]:
        var view := make_view(accelerated)
        await wait_loaded(view)
        if not failures.is_empty(): break
        check(view.call("is_accelerated") == (accelerated and expect_gpu), "interop used the wrong backend/fallback")
        var probe = load("res://BridgeProbe.cs").new()
        view.call("bind_object", "csharp", probe)
        view.call("bind_func", "csharp_callback", probe.call("GetCallback"))
        check(view.call("execute_script", "csharp.Label='修改 🐉';csharp.Echo(csharp.Label)") == "修改 🐉", "C# object method/property binding failed")
        check(probe.get("Label") == "修改 🐉", "C# property setter did not run")
        check(view.call("execute_script", "csharp_callback({text:'中文 🐉',number:4294967297}).number") == 4294967297, "C# delegate callback/data conversion failed")
        check(view.call("execute_script", "(()=>{try{csharp.Echo();return false}catch(e){return /Godot callback failed/.test(e.message)}})()") == true, "Godot method argument errors did not reach JS")
        view.call("bind_func", "gdscript_callback", func(value): return value)
        check(view.call("execute_script", "(()=>{try{gdscript_callback();return false}catch(e){return /Godot callback failed/.test(e.message)}})()") == true, "GDScript callback argument errors did not reach JS")
        view.call("bind_func", "gdscript_default", func(value = 7): return value)
        check(view.call("execute_script", "gdscript_default()") == 7, "GDScript default arguments changed")
        var bound = (func(first, second): return [first, second]).bind("bound 🐉")
        view.call("bind_func", "gdscript_bound", bound)
        check(view.call("execute_script", "gdscript_bound('first')") == ["first", "bound 🐉"], "bound Callable arguments changed")
        view.call("execute_script", "csharp.Function=function(x){return {result:x.text,number:x.number};};true;")
        var data = probe.call("Invoke", {"text":"回调 🐉","number":4294967297})
        check(data is Dictionary and data.result == "回调 🐉" and data.number == 4294967297, "C# could not invoke returned JS Callable")
        var promise = view.call("execute_script", "(async()=>await Promise.resolve({text:'异步 🐉',number:42}))()")
        probe.call("AwaitPromise", promise)
        if not await wait_promise(promise, "async result"): break
        var promise_result = promise.get_result()
        check(not promise.is_rejected() and promise_result is Dictionary and promise_result.text == "异步 🐉" and promise_result.number == 42, "JS async/await result was not exposed to Godot")
        check(probe.get("AsyncCompleted") and probe.get("AsyncResult") == promise.get_result(), "C# could not await the JS Promise completion signal")
        view.call("bind_func", "host_promise", func(): return promise)
        var roundtrip = view.call("execute_script", "(async()=>await host_promise())()")
        if not await wait_promise(roundtrip, "Promise roundtrip"): break
        check(not roundtrip.is_rejected() and roundtrip.get_result() == promise.get_result(), "Godot Promise could not return to JS await")
        var rejected = view.call("execute_script", "(async()=>{throw new Error('async rejected')})()")
        if not await wait_promise(rejected, "Error rejection"): break
        check(rejected.is_rejected() and "async rejected" in str(rejected.get_result()), "JS Promise rejection was lost")
        var rejected_data = view.call("execute_script", "Promise.reject({text:'失败 🐉',number:7})")
        if not await wait_promise(rejected_data, "structured rejection"): break
        var reason = rejected_data.get_result()
        check(rejected_data.is_rejected() and reason is Dictionary and reason.text == "失败 🐉" and reason.number == 7, "structured Promise rejection was lost")
        view.call("execute_script", "csharp.Function=async function(x){return await Promise.resolve(x);};true")
        var async_function_result = probe.call("Invoke", {"text":"异步函数 🐉","number":23})
        if not await wait_promise(async_function_result, "async Callable"): break
        var async_data = async_function_result.get_result()
        check(not async_function_result.is_rejected() and async_data is Dictionary and async_data.text == "异步函数 🐉" and async_data.number == 23, "C# could not invoke an async JS Callable")
        var abandoned = view.call("execute_script", "new Promise(()=>{})")
        var old: Callable = probe.get("Function")
        view.set("htmlUrl", "file:///gpu-validation.html?csharp-navigation=1")
        await frames(30)
        await wait_loaded(view)
        check(not old.is_valid() or old.call({"text":"expired","number":1}) == null, "C# JS Callable executed after navigation")
        check(abandoned.is_completed() and abandoned.is_rejected(), "pending JS Promise did not cancel on navigation")
        probe = null
    for port in ports: port.queue_free()
    views.clear(); ports.clear()
    await frames(10)
    print("CSHARP_INTEROP ", "PASS" if failures.is_empty() else "FAIL", " ", failures)
    quit(0 if failures.is_empty() else 1)
