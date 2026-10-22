extends SceneTree
var view:Control
var inspector:Control
func check(condition:bool,message:String):
    if not condition:push_error(message);quit(1);assert(condition,message)
func result(source:String):
    var value=view.call("evaluate",source)
    check(value.ok,"JS: "+source)
    return value.value
func frames(count:int):
    for i in range(count):await process_frame
func key(code:Key):
    var event=InputEventKey.new()
    event.keycode=code;event.pressed=true
    if OS.get_name()=="macOS":event.meta_pressed=true
    else:event.ctrl_pressed=true
    view.call("fire_key_event",event)
    event.pressed=false
    view.call("fire_key_event",event)
func _initialize():call_deferred("run")
func run():
    view=ClassDB.instantiate("UltralightView")
    view.size=Vector2(500,500)
    root.add_child(view)
    var loaded=view.call("load_html","<input id='text' style='position:absolute;left:10px;top:10px;width:200px;height:40px'><button style='position:absolute;left:10px;top:100px;width:200px;height:50px;cursor:pointer'>button</button>")
    if not loaded.is_completed():await loaded.completed
    check(loaded.get_result().ok,"experience page loaded")
    view.grab_focus()
    result("document.querySelector('#text').focus()")
    view.call("input_text","你好，Godot")
    await frames(3)
    check(result("document.querySelector('#text').value")=="你好，Godot","committed text input")
    DisplayServer.clipboard_set("剪贴板 paste")
    key(KEY_A)
    key(KEY_V)
    await frames(8)
    check(result("document.querySelector('#text').value")=="剪贴板 paste","system clipboard paste")
    result("document.querySelector('#text').value='copy 测试';document.querySelector('#text').select()")
    key(KEY_C)
    await frames(5)
    check(DisplayServer.clipboard_get()=="copy 测试","system clipboard copy")
    view.call("fire_mouse_event",Vector2(80,125),MOUSE_BUTTON_NONE,false)
    await frames(5)
    check(view.get_default_cursor_shape()==Control.CURSOR_POINTING_HAND,"CSS hand cursor")
    var other=Button.new();other.position=Vector2(0,600);root.add_child(other);other.grab_focus()
    await frames(2)
    check(not result("document.hasFocus()"),"Control focus loss forwarded")
    inspector=ClassDB.instantiate("UltralightInspector")
    inspector.position=Vector2(510,0);inspector.size=Vector2(500,500)
    inspector.set("target_view",view)
    root.add_child(inspector)
    var inspected=false
    for i in range(300):
        await process_frame
        var state=inspector.call("evaluate_copy","location.href.includes('inspector') && document.readyState==='complete' && document.querySelectorAll('script').length > 0")
        if state.ok and state.value:inspected=true;break
    check(inspected,"embedded Inspector HTML/CSS/JS loaded")
    view.call("rebuild")
    await frames(30)
    check(inspector.call("is_initialized"),"Inspector reattaches after target rebuild")
    view.queue_free()
    await frames(3)
    check(not inspector.call("is_initialized"),"Inspector releases removed target")
    inspector.queue_free();other.queue_free()
    await frames(2)
    print("EXPERIENCE PASS clipboard text cursor focus inspector")
    quit(0)
