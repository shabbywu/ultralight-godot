#pragma once
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/shader_material.hpp>

namespace gdbind {
// Godot display material for CPU Surface pixels, including BGRA channel correction.
struct CPUSurfaceShader {
#pragma region cached CPU display resources
    inline static godot::Ref<godot::Shader> cpu_shader;
    inline static godot::Ref<godot::ShaderMaterial> cpu_material;
#pragma endregion

#pragma region CPU Surface display API
    static godot::Ref<godot::ShaderMaterial> getMaterial() {
        if (cpu_material.is_null()) {
            cpu_shader.instantiate();
            cpu_shader->set_code(R"(
shader_type canvas_item;
void fragment() {
    float b = COLOR.r;
    COLOR.r = COLOR.b;
    COLOR.b = b;
}
            )");
            cpu_material.instantiate();
            cpu_material->set_shader(cpu_shader);
        }
        return cpu_material;
    }
    static void shutdown() { cpu_material.unref(); cpu_shader.unref(); }
#pragma endregion
};
} // namespace gdbind
