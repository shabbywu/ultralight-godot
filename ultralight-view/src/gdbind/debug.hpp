#pragma once
#include <chrono>

#include <godot_cpp/variant/utility_functions.hpp>

namespace gdbind {
namespace detail {
struct time_guard {
	std::chrono::steady_clock::time_point start;
	std::string scope;
	time_guard(std::string scope = "") :
			scope(scope), start(std::chrono::steady_clock::now()) {
	}
	~time_guard() {
		if (scope.empty())
			godot::UtilityFunctions::print("Time difference = ", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count(), "[ms]");
		else
			godot::UtilityFunctions::print("[", godot::String(scope.data()), "]", "Time difference = ", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count(), "[ms]");
	}
};
} // namespace detail
} // namespace gdbind
