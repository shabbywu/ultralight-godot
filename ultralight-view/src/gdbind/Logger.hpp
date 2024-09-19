#pragma once
#include <Ultralight/Ultralight.h>
#include <godot_cpp/variant/utility_functions.hpp>

namespace gdbind {
class GodotLogger : public ultralight::Logger {
public:
	virtual void LogMessage(ultralight::LogLevel log_level, const ultralight::String &message) {
		godot::UtilityFunctions::print(message.utf8().data());
	}

	static GodotLogger *instance() {
		static GodotLogger instance;
		return &instance;
	}
};
} //namespace gdbind
