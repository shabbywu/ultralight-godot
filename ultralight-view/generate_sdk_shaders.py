"""Adapt the selected SDK's stock GLSL to Godot RD's batched draw-state layout."""
import pathlib
import re
import sys

PROGRAMS = (
    ("Fill", "vertex_quad", "fill"),
    ("FillPath", "vertex_path", "fill_path"),
    ("FilterBasic", "vertex_quad", "filter_basic"),
    ("FilterBlur", "vertex_quad", "filter_blur"),
    ("FilterDropShadow", "vertex_quad", "filter_dropshadow"),
    ("FillPhoton", "vertex_quad", "fill_photon"),
    ("FillPhotonGrid", "vertex_photon_grid", "fill_photon_grid"),
)


def adapt(header, vertex=False):
    source = header.read_text().split('R"GLSL(', 1)[1].split(')GLSL"', 1)[0]
    source = source.replace("#version 420", "#version 450", 1)
    pattern = r"layout\(binding = 0, std140\) uniform type_Uniforms\s*\{(.*?)\} Uniforms;"
    match = re.search(pattern, source, re.S)
    if not match:
        raise ValueError(f"SDK uniform layout changed: {header}")
    fields = match[1].replace("layout(row_major) ", "")
    replacement = ("struct DrawState {" + fields + "};\n"
                   "layout(set = 0, binding = 0, std430, row_major) readonly buffer DrawStates { DrawState draws[]; };\n"
                   "layout(push_constant, std430) uniform DrawIndex { uint index; } draw_index;\n"
                   "#define Uniforms draws[draw_index.index]\n")
    source = source[:match.start()] + replacement + source[match.end():]
    bindings = sorted({int(x) for x in re.findall(r"layout\(binding = (\d+)\) uniform [ui]?sampler2D", source)})
    source = re.sub(r"layout\(binding = (\d+)\) uniform ([ui]?sampler2D)",
                    r"layout(set = 1, binding = \1) uniform \2", source)
    if vertex:
        # Ultralight supplies a D3D-style projection; RD's positive viewport is downward.
        end = source.rfind("}")
        source = source[:end] + "    gl_Position.y = -gl_Position.y;\n" + source[end:]
    return source, sum(1 << x for x in bindings)


def generate(directory, output):
    pieces = ["// Generated from the selected Ultralight SDK. See SDK licenses.\n#pragma once\nnamespace gdbind::gpu_shaders {\n"]
    masks = []
    for name, vert, frag in PROGRAMS:
        vs, vm = adapt(directory / (vert + "_vs.h"), True)
        fs, fm = adapt(directory / (frag + "_fs.h"))
        # VS 2022 accepts large concatenated strings, but individual literals in
        # older MSVC toolsets still have a 16 KiB limit. Photon exceeds 300 KiB.
        def literal(source):
            if ')ULGLSL"' in source:
                raise ValueError("GLSL contains the C++ raw-string delimiter")
            return "\n".join(f'R"ULGLSL({source[i:i + 8192]})ULGLSL"' for i in range(0, len(source), 8192))
        pieces.extend((f'inline constexpr const char *{name}_vert = {literal(vs)};\n',
                       f'inline constexpr const char *{name}_frag = {literal(fs)};\n'))
        masks.append(vm | fm)
    for suffix in ("vert", "frag"):
        names = ", ".join(name + "_" + suffix for name, _, _ in PROGRAMS)
        pieces.append(f"inline constexpr const char *{suffix}s[] = {{{names}}};\n")
    pieces.append("inline constexpr unsigned texture_masks[] = {" + ", ".join(map(str, masks)) + "};\n}\n")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text("".join(pieces))


if __name__ == "__main__":
    generate(pathlib.Path(sys.argv[1]), pathlib.Path(sys.argv[2]))
