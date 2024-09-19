#pragma once
#include "UltralightRenderer.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
using namespace ultralight;
using namespace godot;

namespace gdbind {
class UltralightSingleton : public Object {
	GDCLASS(UltralightSingleton, Object);

public:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("update_logic"), &UltralightSingleton::updateLogic);
		ClassDB::bind_method(D_METHOD("update_frame"), &UltralightSingleton::updateFrame);
		ClassDB::bind_static_method("UltralightSingleton", D_METHOD("get_singleton"), &UltralightSingleton::get_singleton);
	}

	static UltralightSingleton *get_singleton() {
		auto singleton = (UltralightSingleton *)godot::Engine::get_singleton()->get_singleton("UltralightSingleton");
		singleton->init();
		return singleton;
	}

protected:
	bool inited = false;
	void init() {
		if (!inited) {
			RenderingServer::get_singleton()->connect("frame_pre_draw", Callable(this, "update_frame"));
			inited = true;
		}
	}

	void deinit() {
		if (inited) {
			RenderingServer::get_singleton()->disconnect("frame_pre_draw", Callable(this, "update_frame"));
		}
	}

#pragma region proxy to UltralightRenderer
public:
	/// @brief Update timers and dispatch callbacks.
	/// You should call this as often as you can from your application's run loop.
	void updateLogic() {
		UltralightRenderer::get_singleton()->updateLogic();
	}

	/// @brief Render all active views to their respective surfaces and render targets.
	/// You should call this once per frame (usually in synchrony with the monitor's refresh rate).
	void updateFrame() {
		UltralightRenderer::get_singleton()->updateFrame();
	}

	UltralightRenderer *getRender() {
		return UltralightRenderer::get_singleton();
	}

#pragma endregion
};
} //namespace gdbind
