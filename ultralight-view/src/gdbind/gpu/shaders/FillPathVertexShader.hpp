#pragma once

namespace gdbind::gpu_shaders {
inline constexpr const char *path_vert = R"ULGLSL(// Ultralight SDK 1.4.0b.081c48b GLSL port. See https://ultralig.ht for licensing.
#version 450

// Program Uniforms
struct DrawState {
  vec4 state;
  mat4 transform;
  vec4 scalar4[2];
  vec4 vector[8];
  uvec4 clip_size;
  mat4 clip[8];
};
layout(set = 0, binding = 0, std430) readonly buffer DrawStates { DrawState draws[]; };
layout(push_constant, std430) uniform DrawIndex { uint index; } draw_index;
#define State draws[draw_index.index].state
#define Transform draws[draw_index.index].transform
#define Scalar4 draws[draw_index.index].scalar4
#define Vector draws[draw_index.index].vector
#define ClipSize draws[draw_index.index].clip_size.x
#define Clip draws[draw_index.index].clip


// Uniform Accessor Functions
float Time() { return State[0]; }
float ScreenWidth() { return State[1]; }
float ScreenHeight() { return State[2]; }
float ScreenScale() { return State[3]; }
float Scalar(uint i) { if (i < 4u) return Scalar4[0][i]; else return Scalar4[1][i - 4u]; }
vec4 sRGBToLinear(vec4 val) { return vec4(val.xyz * (val.xyz * (val.xyz * 0.305306011 + 0.682171111) + 0.012522878), val.w); }

// Vertex Attributes
layout(location = 0) in vec2 in_Position;
layout(location = 1) in vec4 in_Color;
layout(location = 2) in vec2 in_TexCoord;

// Out Params
layout(location = 0) out vec4 ex_Color;
layout(location = 2) out vec2 ex_ObjectCoord;
layout(location = 3) out vec2 ex_ScreenCoord;

void main(void)
{
  ex_ObjectCoord = in_TexCoord;
  gl_Position = Transform * vec4(in_Position, 0.0, 1.0);
  // RD uses a downward-positive viewport. Ultralight's projection is D3D-style.
  gl_Position.y = -gl_Position.y;
  gl_Position.z = 0.0;
  ex_ScreenCoord = in_Position;
  ex_Color = in_Color;
}

)ULGLSL";
} // namespace gdbind::gpu_shaders
