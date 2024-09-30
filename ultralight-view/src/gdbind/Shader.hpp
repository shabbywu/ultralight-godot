#pragma once

#include <godot_cpp/classes/shader_material.hpp>

namespace gdbind {
#pragma region shader use to convert BGRA to RGBA
struct Shader {
    static godot::Shader *buildin_shader() {
        // convert BGRA(ultralight format) to RGBA(godot format)
        static godot::Shader *shader;
        if (shader == nullptr) {
            shader = memnew(godot::Shader);
            shader->set_code(R"(
shader_type canvas_item;

void fragment(){
  float b = COLOR.r;
  COLOR.r = COLOR.b;
  COLOR.b = b;
}
            )");
        }
        return shader;
    }

    static godot::ShaderMaterial *bulitin_material() {
        static godot::ShaderMaterial *material;
        if (material == nullptr) {
            material = memnew(godot::ShaderMaterial);
            material->set_shader(buildin_shader());
        }
        return material;
    }
};

#pragma endregion
} // namespace gdbind
