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

#include "gdbind/detail/cast.hpp"

#include "doc_data_registration.hpp"
#include "gdbind/UltralightRenderer.hpp"
#include "gdbind/UltralightSingleton.hpp"
#include "gdbind/UltralightView.hpp"
#include "setup.hpp"

using namespace godot;

static void initialize_module(ModuleInitializationLevel p_level) {
    if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
        gdbind::setup::setup_ultralight_platform();
        GDREGISTER_CLASS(gdbind::UltralightView);
        GDREGISTER_CLASS(gdbind::UltralightSingleton);
        static gdbind::UltralightSingleton *singleton = memnew(gdbind::UltralightSingleton);
        godot::Engine::get_singleton()->register_singleton("UltralightSingleton", singleton);
    }
}

static void uninitialize_module(ModuleInitializationLevel p_level) {
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
