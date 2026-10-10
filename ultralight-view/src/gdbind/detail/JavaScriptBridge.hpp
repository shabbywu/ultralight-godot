#pragma once

// Entry point for the header-only Godot bridge. Declarations precede all type
// adapters so each adapter can call the others without include-order coupling.
#include "JavaScriptBridge__def.hpp"
#include "JavaScriptBridge__impl.hpp"
#include "cast.hpp"
#include "godot_callable.hpp"
#include "godot_object.hpp"
#include "js_callable.hpp"
#include "js_promise.hpp"
