extends SceneTree

class Probe extends RefCounted:
    var items: Array = [1, {"value": 2}]
    var options: Dictionary = {"enabled": true}
    var label = "probe"
    func echo(value): return value

var view: Control
var probe = Probe.new()
var main_events = []
var frame_events = []
var api: Object

func check(condition: bool, message: String):
    if not condition:
        push_error(message)
        quit(1)
        assert(condition, message)

func wait_task(task):
    if not task.is_completed(): await task.completed
    return task.get_result()

func value(source: String):
    var result = view.call("evaluate", source)
    check(result.ok, "Evaluate failed: " + source + (" " + result.error.message if not result.ok else ""))
    return result.value

func _initialize():
    call_deferred("run")

func run():
    view = ClassDB.instantiate("UltralightView")
    view.size = Vector2(640, 480)
    root.add_child(view)
    view.connect("dom_ready", func(event): main_events.append(event))
    view.connect("frame_dom_ready", func(event): frame_events.append(event))
    api = UltralightAPI.create("game")
    check(api.register_function("sum", func(a, b): return a+b).ok, "register function")
    check(api.register_object("probe", probe).ok, "register object")
    check(api.register_async("later", func(request, args):
        create_timer(0.02).timeout.connect(func(): request.resolve(args[0]+args[1]))
    ).ok, "register async")
    check(api.attach(view).ok, "attach registry")
    var loaded = await wait_task(view.call("load_html", """
        <script>globalThis.first = game.sum(2, 3);globalThis.old = game.probe;</script>
        <div id="ready">ready</div><iframe srcdoc="<script>window.child=1;</script>"></iframe>
    """, "file:///ui/index.html"))
    check(loaded.ok, "HTML load")
    check(value("first") == 5, "registry must install before first script")
    check(main_events.size() == 1 and main_events[0].is_main_frame, "main DOM signal")
    for i in range(20):
        if frame_events.size() > 1: break
        await process_frame
    check(frame_events.size() >= 2, "iframe DOM signal")
    check(value("game.probe.items === game.probe.items"), "host array identity")
    check(value("Array.isArray(game.probe.items)"), "host arrays are arrays")
    value("game.probe.items.push(9);game.probe.items[1].value=7;game.probe.options.enabled=false")
    check(probe.items.size()==3 and probe.items[2]==9, "JS changes host array")
    check(probe.items[1].value==7 and not probe.options.enabled, "nested write-through")
    probe.items[0]=12
    check(value("game.probe.items[0]")==12, "host changes visible to JS")
    var live=value("globalThis.items=[1,{x:2}];items")
    check(live.is_class("UltralightJSArray"), "JS array live handle")
    check(live.append(3).ok and value("items.length")==3,"host changes JS array")
    value("items[0]=11")
    check(live.get_value(0).value==11,"JS changes visible to host")
    check(value("game.probe.echo(items)===items"),"roundtrip JS identity")
    var snapshot=live.snapshot()
    check(snapshot.ok and snapshot.value[0]==11,"explicit snapshot")
    value("items[0]=22")
    check(snapshot.value[0]==11,"snapshot independent")
    var function_handle=view.call("evaluate_handle","()=>[23]").value
    var invoked=function_handle.invoke([])
    check(invoked.ok and invoked.value.is_class("UltralightJSArray"),"explicit function handles return shared containers")
    check(invoked.value.get_value(0).value==23,"shared function result contents")
    var shared_function=value("globalThis.returnArray=()=>[3];returnArray")
    check(shared_function.call().is_class("UltralightJSArray"),"new Callable returns shared values")
    var copied_function=view.call("execute_script","returnArray")
    check(typeof(copied_function.call())==TYPE_ARRAY,"legacy Callable retains copied values")
    check(value("returnArray")==shared_function,"shared Callable identity is stable")
    var receiver=value("({value:7,read(){return [this.value]}})")
    var method_result=receiver.invoke_method("read",[])
    check(method_result.ok and method_result.value.get_value(0).value==7,"method call preserves receiver and shared result")
    var not_callable=receiver.invoke_method("value",[])
    check(not not_callable.ok and not_callable.error.code=="TypeMismatch","calling a non-function returns a checked error")
    check(not receiver.append(1).ok,"append rejects non-array handles")
    var locked=value("Object.defineProperty({},'fixed',{value:1,enumerable:true,configurable:false})")
    check(not locked.remove("fixed").ok and locked.get_value("fixed").value==1,"failed delete cannot report success")
    var invalid_graph=[]
    invalid_graph.append(invalid_graph)
    var copied_options=UltralightBindingOptions.new()
    copied_options.copy_containers=true
    var rollback=UltralightAPI.create("rollback")
    check(rollback.register_function("first",func():return 1).ok,"rollback function fixture")
    check(rollback.register_object("invalid",invalid_graph,copied_options).ok,"rollback copy fixture")
    check(not rollback.attach(view).ok,"attachment fails when a registration cannot be converted")
    check(value("typeof rollback==='undefined'"),"failed attachment removes namespace and partial exports")
    rollback.close()
    invalid_graph.clear()
    var ready_task=view.call("wait_ready")
    check(ready_task.is_completed() and ready_task.get_result().ok,"already-ready wait completes synchronously")
    var invalid_selector=view.call("wait_for_selector","")
    check(invalid_selector.is_completed() and not invalid_selector.get_result().ok,"invalid selector fails immediately")
    var cyclic=value("globalThis.cycle={};cycle.self=cycle;cycle")
    check(cyclic.get_value("self").value==cyclic,"JS cyclic identity")
    check(not cyclic.snapshot().ok,"cyclic snapshot rejected")
    check(view.call("evaluate_copy","new Map([[1,2]])").error.code=="Unsupported","unsupported objects never silently snapshot as empty dictionaries")
    check(view.call("evaluate","new Date()").error.code=="Unsupported","special objects require explicit projection or handles")
    var options=UltralightBindingOptions.new()
    options.members=PackedStringArray(["items"])
    check(api.register_object("limited",probe,options).ok,"member whitelist")
    check(value("!('label' in game.limited) && game.limited.items[1].value===7"),"member selection applies at root")
    options=UltralightBindingOptions.new();options.read_only=true
    check(api.register_object("readonly",probe,options).ok,"readonly registration")
    check(value("(()=>{try{game.readonly.items.push(2);return false}catch(e){return true}})()"),"nested readonly writes rejected")
    check(value("(()=>{try{game.$bridge.set(game.readonly.options,'enabled',true);return false}catch(e){return true}})()"),"map helper preserves readonly scope")
    value("game.probe.items.splice(0,1,19);game.probe.items.pop()")
    check(probe.items[0]==19 and probe.items.size()==2,"native shared splice and pop")
    var temporary=view.call("bind_function","temporary",func():return 31)
    value("globalThis.cachedTemporary=temporary")
    check(value("temporary()")==31,"temporary binding")
    temporary.close()
    check(value("(()=>{try{cachedTemporary();return false}catch(e){return true}})()"),"temporary callback revoked")
    var text_handle=view.call("evaluate_handle","'a'+String.fromCharCode(0)+'b'").value
    check(text_handle.utf8_bytes().value==PackedByteArray([97,0,98]),"lossless JS string bytes")
    var bad=view.call("evaluate","throw new Error('intentional')")
    check(not bad.ok and bad.error.code=="ScriptException","structured script error")
    check(view.call("evaluate","null").ok,"null is successful")
    check(value("undefined").is_class("UltralightUndefined"),"undefined distinct from null")
    var resolved=await wait_task(view.call("evaluate_async","game.later(4,5)"))
    check(resolved.ok and resolved.value==9,"JS to host async")
    var immediate=await wait_task(view.call("evaluate_async","Promise.resolve(42)"))
    check(immediate.ok and immediate.value==42,"immediate Promise completion")
    var rejected=await wait_task(view.call("evaluate_async","Promise.reject(new Error('nope'))"))
    check(not rejected.ok,"Promise rejection")
    var selected=await wait_task(view.call("wait_for_selector","#ready"))
    check(selected.ok,"selector wait")
    var timeout=await wait_task(view.call("wait_for_selector","#missing",0.03))
    check(not timeout.ok and timeout.error.code=="Timeout","selector timeout")
    var cancellation=view.call("evaluate_async","new Promise(()=>{})")
    cancellation.cancel()
    check(cancellation.get_result().error.code=="Cancelled","explicit cancellation")
    var requests=[]
    check(api.register_async("pending",func(request,_args):requests.append(request)).ok,"pending host operation fixture")
    var revoked=view.call("evaluate_async","game.pending()",1.0)
    check(requests.size()==1,"host async handler started")
    api.unregister("pending")
    var revoked_result=await wait_task(revoked)
    check(not revoked_result.ok and revoked_result.error.code!="Timeout","unregister rejects pending JS promise")
    check(requests[0].is_completed(),"unregister cancels native host request")
    api.unregister("probe")
    check(value("typeof game.probe === 'undefined'"),"unregister global entry")
    check(value("(()=>{try{old.label;return false}catch(e){return true}})()"),"cached proxy revoked")
    check(api.register_object("probe",probe).ok,"register again")
    var pending=view.call("evaluate_async","new Promise(()=>{})")
    var old_live=live
    loaded=await wait_task(view.call("load_html","<script>globalThis.restored=game.sum(8,9)</script>"))
    check(loaded.ok and value("restored")==17,"navigation restores registry")
    check(pending.get_result().error.code=="PageGone","navigation cancels old await")
    check(old_live.is_closed() and not old_live.get_value(0).ok,"navigation expires JS handle")
    for argument in OS.get_cmdline_user_args():
        if argument.begins_with("--redirect-url="):
            var redirected=view.call("load_url",argument.trim_prefix("--redirect-url="))
            var generation=view.get("navigation_id")
            var redirect_result=await wait_task(redirected)
            check(redirect_result.ok and value("redirectResult")==42,"HTTP redirect chain reaches DOM readiness")
            check(view.get("navigation_id")==generation and view.get("current_url").ends_with("/page"),"redirect keeps navigation identity and records final URL")
    check(api.attach(view).ok,"attach is idempotent")
    var other=ClassDB.instantiate("UltralightView")
    other.size=Vector2(320,240)
    root.add_child(other)
    check(api.attach(other).ok,"same registry attaches to another View")
    var other_loaded=await wait_task(other.call("load_html","<div>plain HTML without scripts</div>"))
    check(other_loaded.ok and other.call("evaluate","game.sum(2,6)").value==8,"registry available in a scriptless document")
    check(not other.call("evaluate_handle","[]").value.is_closed(),"second document handle")
    var first_handle=value("[]")
    check(api.register_function("cross",func():return first_handle).ok,"cross-context fixture")
    check(not other.call("evaluate","game.cross()").ok,"cross-document handles rejected")
    api.unregister("cross")
    var graph=[]
    graph.append(graph)
    check(api.register_object("graph",graph).ok,"root container registration")
    check(value("Array.isArray(game.graph) && game.graph[0]===game.graph"),"host cycle identity")
    check(not view.call("evaluate_copy","game.graph").ok,"host cycle snapshot rejected")
    graph.append(19)
    check(value("game.graph[1]")==19,"identity survives source resize")
    api.unregister("graph")
    graph.clear()
    api.detach(other)
    check(other.call("evaluate","typeof game==='undefined'").value,"detach second View")
    check(value("game.sum(1,2)")==3,"detaching one View preserves the other")
    other.queue_free()
    var reentrant=ClassDB.instantiate("UltralightView")
    reentrant.size=Vector2(320,240)
    root.add_child(reentrant)
    var after_close=[]
    reentrant.connect("dom_ready",func(_event):reentrant.call("close"))
    reentrant.connect("frame_dom_ready",func(event):after_close.append(event))
    var interrupted=await wait_task(reentrant.call("load_html","<script>window.ready=true</script>"))
    check(not interrupted.ok and interrupted.error.code=="Closed","signal handler can close the View")
    check(after_close.is_empty() and not reentrant.get("document_ready"),"close prevents remaining old-frame dispatch")
    reentrant.queue_free()
    var collision=UltralightAPI.create("game")
    check(not collision.attach(view).ok,"namespace ownership collision")
    api.detach(view)
    check(value("typeof game === 'undefined'"),"detach removes namespace")
    api.close()
    view.call("close")
    view.queue_free()
    await process_frame
    print("API PASS lifecycle registry shared async")
    quit(0)
