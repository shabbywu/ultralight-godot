extends SceneTree
class Model extends RefCounted:
    var rows: Array = []
var view: Control
func _initialize(): call_deferred("run")
func measure(source: String):
    var samples=[]
    for i in range(7):
        var start=Time.get_ticks_usec()
        var result=view.call("evaluate",source)
        if not result.ok: push_error(result.error.message)
        assert(result.ok)
        samples.append(Time.get_ticks_usec()-start)
    samples.sort()
    return {"median_us":samples[3],"p95_us":samples[6]}
func run():
    view=ClassDB.instantiate("UltralightView")
    view.size=Vector2(640,480)
    root.add_child(view)
    var loaded=view.call("load_html","<script>globalThis.ready=true</script>")
    if not loaded.is_completed():await loaded.completed
    assert(loaded.get_result().ok)
    var results=[]
    for count in [1000,10000,100000]:
        var source_start=Time.get_ticks_usec()
        var model=Model.new()
        for i in range(count):model.rows.append({"id":i,"detail":{"name":"item","value":i}})
        var constructed=Time.get_ticks_usec()-source_start
        var shared=UltralightAPI.create("shared")
        var copied=UltralightAPI.create("copied")
        var options=UltralightBindingOptions.new()
        options.copy_containers=true
        shared.register_object("model",model)
        copied.register_object("model",model,options)
        var first=Time.get_ticks_usec()
        assert(shared.attach(view).ok)
        var exposed=Time.get_ticks_usec()-first
        assert(copied.attach(view).ok)
        var sparse=measure("shared.model.rows[0].detail.value + shared.model.rows.length")
        var after_sparse=view.call("get_bridge_statistics")
        var copy=measure("copied.model.rows[0].detail.value + copied.model.rows.length")
        var scan=measure("shared.model.rows.reduce((sum,row)=>sum+row.id,0)")
        var snapshot=measure("shared.$bridge.copy(shared.model.rows).reduce((sum,row)=>sum+row.id,0)")
        var report={"nodes":count,"source_construction_us":constructed,"first_exposure_us":exposed,"shared_sparse":sparse,"copy_sparse":copy,"shared_full_scan":scan,"snapshot_full_scan":snapshot,"after_sparse":after_sparse}
        results.append(report)
        shared.close();copied.close()
        await process_frame
    var output=FileAccess.open("user://containers.json",FileAccess.WRITE)
    output.store_string(JSON.stringify(results,"  "));output.close()
    print("CONTAINERS PASS "+JSON.stringify(results))
    view.queue_free()
    await process_frame
    quit(0)
