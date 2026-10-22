extends SceneTree
func _initialize():
    var schema={}
    for name in ["UltralightView","UltralightAPI","UltralightBindingOptions","UltralightBinding","UltralightInspector","UltralightResult","UltralightError","UltralightTask","UltralightFrameEvent","UltralightJSValue","UltralightJSArray","UltralightJSObject","UltralightUndefined","UltralightSingleton"]:
        var properties=ClassDB.class_get_property_list(name,true)
        for property in properties:
            # Older Godot versions do not expose property accessor names.
            if ClassDB.has_method("class_get_property_getter"):
                property["getter"]=ClassDB.call("class_get_property_getter",name,property.name)
            if ClassDB.has_method("class_get_property_setter"):
                property["setter"]=ClassDB.call("class_get_property_setter",name,property.name)
        schema[name]={"inherits":ClassDB.get_parent_class(name),"methods":ClassDB.class_get_method_list(name,true),"signals":ClassDB.class_get_signal_list(name,true),"properties":properties}
    var file=FileAccess.open("user://api-schema.json",FileAccess.WRITE)
    file.store_string(JSON.stringify(schema,"  "));file.close()
    print("SCHEMA "+ProjectSettings.globalize_path("user://api-schema.json"))
    quit(0)
