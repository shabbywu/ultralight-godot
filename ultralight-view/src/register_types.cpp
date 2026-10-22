/* godot-cpp integration testing project.
 *
 * This is free and unencumbered software released into the public domain.
 */

#include <gdextension_interface.h>

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/binder_common.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include "gdbind/api/Types.hpp"
#include "gdbind/api/UltralightAPI.hpp"
#include "gdbind/detail/JavaScriptBridge.hpp"

#include "doc_data_registration.hpp"
#include "gdbind/CPUSurfaceShader.hpp"
#include "gdbind/GPUDisplayShader.hpp"
#include "gdbind/UltralightInspector.hpp"
#include "gdbind/UltralightRenderer.hpp"
#include "gdbind/UltralightSingleton.hpp"
#include "gdbind/UltralightView.hpp"
#include "setup.hpp"

using namespace godot;

static void initialize_module(ModuleInitializationLevel p_level) {
    if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
        gdbind::setup::setup_ultralight_platform();
        GDREGISTER_CLASS(gdbind::UltralightError);
        GDREGISTER_CLASS(gdbind::UltralightResult);
        GDREGISTER_CLASS(gdbind::UltralightTask);
        GDREGISTER_CLASS(gdbind::UltralightFrameEvent);
        GDREGISTER_CLASS(gdbind::UltralightJSValue);
        GDREGISTER_CLASS(gdbind::UltralightJSArray);
        GDREGISTER_CLASS(gdbind::UltralightJSObject);
        GDREGISTER_CLASS(gdbind::UltralightUndefined);
        GDREGISTER_CLASS(gdbind::UltralightBindingOptions);
        GDREGISTER_CLASS(gdbind::UltralightBinding);
        GDREGISTER_CLASS(gdbind::UltralightAPI);
        GDREGISTER_CLASS(gdbind::UltralightView);
        GDREGISTER_CLASS(gdbind::UltralightInspector);
        GDREGISTER_CLASS(gdbind::UltralightSingleton);
        static gdbind::UltralightSingleton *singleton = memnew(gdbind::UltralightSingleton);
        godot::Engine::get_singleton()->register_singleton("UltralightSingleton", singleton);

        GDREGISTER_CLASS(gdbind::JavascrtipCallableTrampoline);
        GDREGISTER_CLASS(gdbind::JavaScriptPromise);
    }
}

static void uninitialize_module(ModuleInitializationLevel p_level) {
    if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
        gdbind::BridgeContext::shutdown();
        gdbind::UltralightUndefined::shutdown();
        auto singleton = static_cast<gdbind::UltralightSingleton *>(
            godot::Engine::get_singleton()->get_singleton("UltralightSingleton"));
        godot::Engine::get_singleton()->unregister_singleton("UltralightSingleton");
        singleton->shutdown();
        ulbind17::jsc::Bridge::collectFinalizers();
        godot::memdelete(singleton);
        gdbind::GPUDisplayShader::shutdown();
        gdbind::CPUSurfaceShader::shutdown();
    }
}

extern "C" {
// Initialization.
GDExtensionBool GDE_EXPORT ultralight_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address,
                                                   GDExtensionClassLibraryPtr p_library,
                                                   GDExtensionInitialization *r_initialization) {
    godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);
    init_obj.register_initializer(initialize_module);
    init_obj.register_terminator(uninitialize_module);
    init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

    return init_obj.init();
}
}
