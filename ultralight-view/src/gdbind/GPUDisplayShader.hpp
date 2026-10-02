#pragma once
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/shader_material.hpp>

namespace gdbind {
// Shared GPU display resources; View materials keep independent UV parameters.
struct GPUDisplayShader {
#pragma region cached GPU display resources
  private:
    inline static godot::Ref<godot::Shader> gpu_shader;
    inline static godot::Ref<godot::ShaderMaterial> gpu_material;
#pragma endregion

#pragma region shader source
    inline static constexpr const char *source = R"ULGODOT(
shader_type canvas_item;
render_mode blend_premul_alpha;
uniform vec4 uv_rect = vec4(0.0, 0.0, 1.0, 1.0);
varying vec4 tint;
void vertex() { tint = COLOR; }
void fragment() {
    vec2 uv = mix(uv_rect.xy, uv_rect.zw, UV);
    COLOR = texture(TEXTURE, uv) * vec4(tint.rgb * tint.a, tint.a);
}
        )ULGODOT";
#pragma endregion

#pragma region GPU display material API
  public:
    static godot::Ref<godot::ShaderMaterial> createMaterial() {
        if (gpu_material.is_null()) {
            gpu_shader.instantiate();
            gpu_shader->set_code(source);
            gpu_material.instantiate();
            gpu_material->set_shader(gpu_shader);
        }
        // Shallow copies share the Shader but own their mutable uniform values.
        godot::Ref<godot::ShaderMaterial> material = gpu_material->duplicate(false);
        return material;
    }
    static void shutdown() { gpu_material.unref(); gpu_shader.unref(); }
#pragma endregion
};
} // namespace gdbind
