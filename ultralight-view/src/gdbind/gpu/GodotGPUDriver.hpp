#pragma once

#include "SDKShaders.hpp"
#include "../GPUDisplayShader.hpp"
#include <Ultralight/platform/GPUDriver.h>
#include <godot_cpp/classes/texture2drd.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <memory>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/rendering_device.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/rd_pipeline_color_blend_state.hpp>
#include <godot_cpp/classes/rd_pipeline_color_blend_state_attachment.hpp>
#include <godot_cpp/classes/rd_pipeline_depth_stencil_state.hpp>
#include <godot_cpp/classes/rd_pipeline_multisample_state.hpp>
#include <godot_cpp/classes/rd_pipeline_rasterization_state.hpp>
#include <godot_cpp/classes/rd_sampler_state.hpp>
#include <godot_cpp/classes/rd_shader_source.hpp>
#include <godot_cpp/classes/rd_shader_spirv.hpp>
#include <godot_cpp/classes/rd_texture_format.hpp>
#include <godot_cpp/classes/rd_texture_view.hpp>
#include <godot_cpp/classes/rd_uniform.hpp>
#include <godot_cpp/classes/rd_vertex_attribute.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <functional>
#include <future>
#include <map>
#include <mutex>
#include <tuple>
#include <vector>

namespace gdbind {
using namespace godot;

// Ultralight calls this on the main thread. Only Impl's queued jobs touch RD.
class GodotGPUDriver final : public ultralight::GPUDriver {
#pragma region internal state and methods
    struct Impl;
    std::shared_ptr<Impl> impl;
    uint32_t texture_id = 0, buffer_id = 0, geometry_id = 0;
    // Dispatch copied GPU operations after SDK synchronization, or at shutdown.
    // This does not render an Ultralight frame or publish a View's texture.
    void dispatchPendingOperations();
#pragma endregion

#pragma region Godot integration API
  public:
    GodotGPUDriver();
    ~GodotGPUDriver() override;
    static GodotGPUDriver &instance();
    bool initialize();
    bool available() const;
    void shutdown();
    godot::Ref<godot::Texture2DRD> display_texture(uint32_t id) const;
    // Share cached shader resources, with separate material/UV parameters per View.
    static godot::Ref<godot::ShaderMaterial> createDisplayMaterial();
    godot::Dictionary statistics() const;
#pragma endregion

#pragma region ultralight::GPUDriver interface
  public:
    void BeginSynchronize() override;
    void EndSynchronize() override;
    uint32_t NextTextureId() override;
    void CreateTexture(uint32_t id, ultralight::RefPtr<ultralight::Bitmap> bitmap, uint32_t flags) override;
    void UpdateTexture(uint32_t id, ultralight::RefPtr<ultralight::Bitmap> bitmap, const ultralight::IntRect &dirty_rect) override;
    void DestroyTexture(uint32_t id) override;
    uint32_t NextRenderBufferId() override;
    void CreateRenderBuffer(uint32_t id, const ultralight::RenderBuffer &buffer) override;
    void DestroyRenderBuffer(uint32_t id) override;
    uint32_t NextGeometryId() override;
    void CreateGeometry(uint32_t id, const ultralight::VertexBuffer &vertices,
                        const ultralight::IndexBuffer &indices) override;
    void UpdateGeometry(uint32_t id, const ultralight::VertexBuffer &vertices,
                        const ultralight::IndexBuffer &indices) override;
    void DestroyGeometry(uint32_t id) override;
    void UpdateCommandList(const ultralight::CommandList &list) override;
    void GetDeviceCaps(ultralight::GPUDeviceCaps &caps) override;
#pragma endregion
};

#pragma region internal implementation
struct GodotGPUDriver::Impl {
    using RD = RenderingDevice;

#pragma region render thread helpers
    // A static Callable owns the job until the render thread consumes it. No Node or
    // GPUDriver address is captured; shutdown fences all jobs before library unload.
    static void run_job(int64_t address) {
        std::unique_ptr<std::function<void()>> job(reinterpret_cast<std::function<void()> *>(address));
        (*job)();
    }
    static void enqueue(std::function<void()> job) {
        auto *owned = new std::function<void()>(std::move(job));
        RenderingServer::get_singleton()->call_on_render_thread(
            callable_mp_static(run_job).bind(int64_t(reinterpret_cast<intptr_t>(owned))));
    }
    static PackedByteArray copy_bytes(const void *data, size_t size) {
        PackedByteArray result;
        result.resize(size);
        if (size) std::memcpy(result.ptrw(), data, size);
        return result;
    }
    template <typename T> static Ref<T> make_ref() { Ref<T> ref; ref.instantiate(); return ref; }
#pragma endregion

#pragma region serialized draw state
    // Explicit std430 serialization. Never upload the SDK's packed GPUState.
    struct alignas(16) DrawUniforms {
        float state[4]{};
        float transform[16]{};
        int32_t integer[8]{};
        float scalar[8]{};
        float vector[32]{};
        int32_t clip_size[4]{};
        float clip[128]{};
    };
    static_assert(sizeof(DrawUniforms) == 800);
    static_assert(offsetof(DrawUniforms, clip) == 288);
    static_assert(sizeof(ultralight::Vertex_2f_4ub_2f) == 20);
    static_assert(sizeof(ultralight::Vertex_2f_4ub_2f_2f_28f) == 140);
    static_assert(offsetof(DrawUniforms, integer) == 80);
    static_assert(offsetof(DrawUniforms, scalar) == 112);
    static_assert(sizeof(ultralight::Vertex_2f_2ui) == 16);
    static constexpr int program_count = 7;
    static int vertex_format(int shader) { return shader == 1 ? 0 : shader == 6 ? 2 : 1; }
    struct Draw {
        bool clear{}, flush{}, blend{}, scissor{}, texturing{};
        int shader{};
        ultralight::BlendFactor src = ultralight::BlendFactor::One, dst = ultralight::BlendFactor::InvSrcAlpha;
        ultralight::BlendEquation equation = ultralight::BlendEquation::Add;
        uint32_t target{}, geometry{}, count{}, offset{}, width{}, height{}, textures[4]{};
        Rect2 scissor_rect;
        DrawUniforms uniforms;
    };

#pragma endregion

#pragma region resource caches and state
    RD *rd = nullptr; // render-thread only
    std::atomic<bool> ready{false};
    bool attempted = false, stopped = false; // main-thread only
    std::vector<std::function<void(Impl &)>> pending;
    std::mutex display_mutex;
    std::map<uint32_t, Ref<Texture2DRD>> displays;
    struct Texture { RID rid, snapshot; uint32_t width{}, height{}; ultralight::BitmapFormat format{}; bool target{}; };
    struct Target { RID framebuffer; uint32_t texture{}, width{}, height{}; };
    struct Geometry {
        RID vertices, indices, array;
        uint32_t vertex_bytes{}, index_bytes{};
        int format{};
        std::map<std::pair<uint32_t, uint32_t>, RID> slices;
    };
    std::map<uint32_t, Texture> textures;
    std::map<uint32_t, Target> targets;
    std::map<uint32_t, Geometry> geometries;
    RID shaders[program_count], sampler, data_sampler, fallback, integer_fallback;
    int64_t formats[3]{};
    // Shader, blend flag, framebuffer format.
    std::map<std::tuple<int, bool, int, int, int, int64_t>, RID> pipelines;
    std::map<std::tuple<int, uint64_t, uint64_t, uint64_t, uint64_t>, RID> texture_sets;
    RID state_buffer, state_sets[program_count];
    std::atomic<uint64_t> shader_draws[program_count]{};
    uint32_t state_capacity = 0;
    std::atomic<uint64_t> bitmap_bytes{0}, geometry_bytes{0}, state_bytes{0}, draws{0}, batches{0}, feedback_copies{0}, passes{0};
    std::atomic<double> record_us{0}, gpu_us{0};

#pragma endregion

#pragma region RenderingDevice resource operations
    void free(RID &rid) { if (rid.is_valid()) rd->free_rid(rid); rid = RID(); }
    static RD::DataFormat texture_format(ultralight::BitmapFormat format) {
        using BF = ultralight::BitmapFormat;
        switch (format) {
        case BF::A8_UNORM: return RD::DATA_FORMAT_R8_UNORM;
        case BF::BGRA8_UNORM_SRGB: return RD::DATA_FORMAT_R8G8B8A8_UNORM;
        case BF::RG8_UNORM: return RD::DATA_FORMAT_R8G8_UNORM;
        case BF::RGBA16F: return RD::DATA_FORMAT_R16G16B16A16_SFLOAT;
        case BF::RGBA16UI: return RD::DATA_FORMAT_R16G16B16A16_UINT;
        case BF::RGBA32F: return RD::DATA_FORMAT_R32G32B32A32_SFLOAT;
        default: return RD::DATA_FORMAT_MAX;
        }
    }
    RID make_texture(uint32_t width, uint32_t height, ultralight::BitmapFormat bitmap_format, bool target, const PackedByteArray &data = {}) {
        auto format = make_ref<RDTextureFormat>();
        format->set_width(width); format->set_height(height);
        format->set_format(texture_format(bitmap_format));
        format->set_texture_type(RD::TEXTURE_TYPE_2D);
        format->set_usage_bits(RD::TEXTURE_USAGE_SAMPLING_BIT | RD::TEXTURE_USAGE_CAN_UPDATE_BIT |
                              RD::TEXTURE_USAGE_CAN_COPY_FROM_BIT | RD::TEXTURE_USAGE_CAN_COPY_TO_BIT |
                              (target ? RD::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT : 0));
        auto view = make_ref<RDTextureView>();
        if (bitmap_format == ultralight::BitmapFormat::A8_UNORM && !target) {
            view->set_swizzle_r(RD::TEXTURE_SWIZZLE_ZERO); view->set_swizzle_g(RD::TEXTURE_SWIZZLE_ZERO);
            view->set_swizzle_b(RD::TEXTURE_SWIZZLE_ZERO); view->set_swizzle_a(RD::TEXTURE_SWIZZLE_R);
        }
        TypedArray<PackedByteArray> initial;
        if (!data.is_empty()) initial.push_back(data);
        return rd->texture_create(format, view, initial);
    }
    bool init() {
        rd = RenderingServer::get_singleton()->get_rendering_device();
        if (!rd) return false;
        for (int i = 0; i < program_count; ++i) {
            auto source = make_ref<RDShaderSource>();
            source->set_stage_source(RD::SHADER_STAGE_VERTEX, gpu_shaders::verts[i]);
            source->set_stage_source(RD::SHADER_STAGE_FRAGMENT, gpu_shaders::frags[i]);
            auto spirv = rd->shader_compile_spirv_from_source(source);
            if (spirv.is_null()) return false;
            for (auto stage : {RD::SHADER_STAGE_VERTEX, RD::SHADER_STAGE_FRAGMENT}) {
                auto error = spirv->get_stage_compile_error(stage);
                if (!error.is_empty()) { UtilityFunctions::push_error("Ultralight GPU shader: ", i, " ", error); return false; }
            }
            shaders[i] = rd->shader_create_from_spirv(spirv);
            if (!shaders[i].is_valid()) return false;
        }
        for (int i = 0; i < 3; ++i) {
            TypedArray<RDVertexAttribute> attributes;
            uint32_t offsets[] = {0, 8, 12, 20, 28, 44, 60, 76, 92, 108, 124};
            for (int location = 0; location < (i == 1 ? 11 : 3); ++location) {
                auto attr = make_ref<RDVertexAttribute>();
                attr->set_location(location); attr->set_offset(offsets[location]);
                attr->set_stride(i == 0 ? 20 : i == 1 ? 140 : 16);
                attr->set_format(i == 2 && location > 0 ? RD::DATA_FORMAT_R32_UINT :
                    location == 1 ? RD::DATA_FORMAT_R8G8B8A8_UNORM :
                    location >= 4 ? RD::DATA_FORMAT_R32G32B32A32_SFLOAT : RD::DATA_FORMAT_R32G32_SFLOAT);
                attributes.push_back(attr);
            }
            formats[i] = rd->vertex_format_create(attributes);
        }
        auto sampler_state = make_ref<RDSamplerState>();
        sampler_state->set_min_filter(RD::SAMPLER_FILTER_LINEAR);
        sampler_state->set_mag_filter(RD::SAMPLER_FILTER_LINEAR);
        sampler_state->set_repeat_u(RD::SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE);
        sampler_state->set_repeat_v(RD::SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE);
        sampler = rd->sampler_create(sampler_state);
        sampler_state->set_min_filter(RD::SAMPLER_FILTER_NEAREST);
        sampler_state->set_mag_filter(RD::SAMPLER_FILTER_NEAREST);
        sampler_state->set_mip_filter(RD::SAMPLER_FILTER_NEAREST);
        data_sampler = rd->sampler_create(sampler_state);
        uint8_t zeros[8]{};
        integer_fallback = make_texture(1, 1, ultralight::BitmapFormat::RGBA16UI, false, copy_bytes(zeros, 8));
        uint8_t white[] = {255,255,255,255};
        fallback = make_texture(1, 1, ultralight::BitmapFormat::BGRA8_UNORM_SRGB, false, copy_bytes(white, 4));
        // Preflight framebuffer + pipelines so shader/pipeline failure falls back
        // before an accelerated Ultralight View is created.
        bool valid = sampler.is_valid() && data_sampler.is_valid() && fallback.is_valid() && integer_fallback.is_valid();
        for (auto format : {ultralight::BitmapFormat::BGRA8_UNORM_SRGB, ultralight::BitmapFormat::A8_UNORM}) {
            RID probe = make_texture(1, 1, format, true);
            TypedArray<RID> attachments; attachments.push_back(probe);
            RID fb = rd->framebuffer_create(attachments);
            valid = fb.is_valid() && valid;
            if (valid) for (int shader = 0; shader < program_count; ++shader) for (bool blend : {false, true}) {
                Draw draw; draw.shader = shader; draw.blend = blend;
                valid = pipeline(draw, rd->framebuffer_get_format(fb)).is_valid() && valid;
            }
            free(fb); free(probe);
        }
        for (auto format : {ultralight::BitmapFormat::RG8_UNORM, ultralight::BitmapFormat::RGBA16F, ultralight::BitmapFormat::RGBA32F, ultralight::BitmapFormat::RGBA16UI})
            valid = rd->texture_is_format_supported_for_usage(texture_format(format), RD::TEXTURE_USAGE_SAMPLING_BIT) && valid;
        return valid;
    }
    static RD::BlendFactor blend_factor(ultralight::BlendFactor factor) {
        static constexpr RD::BlendFactor factors[] = {RD::BLEND_FACTOR_ZERO, RD::BLEND_FACTOR_ONE,
            RD::BLEND_FACTOR_SRC_COLOR, RD::BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
            RD::BLEND_FACTOR_SRC_ALPHA, RD::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            RD::BLEND_FACTOR_DST_COLOR, RD::BLEND_FACTOR_ONE_MINUS_DST_COLOR,
            RD::BLEND_FACTOR_DST_ALPHA, RD::BLEND_FACTOR_ONE_MINUS_DST_ALPHA, RD::BLEND_FACTOR_SRC_ALPHA_SATURATE};
        return factors[static_cast<unsigned>(factor)];
    }
    RID pipeline(const Draw &draw, int64_t framebuffer_format) {
        auto key = std::make_tuple(draw.shader, draw.blend, int(draw.src), int(draw.dst), int(draw.equation), framebuffer_format);
        auto found = pipelines.find(key);
        if (found != pipelines.end()) return found->second;
        auto raster = make_ref<RDPipelineRasterizationState>();
        raster->set_cull_mode(RD::POLYGON_CULL_DISABLED);
        auto attachment = make_ref<RDPipelineColorBlendStateAttachment>();
        attachment->set_enable_blend(draw.blend);
        attachment->set_src_color_blend_factor(blend_factor(draw.src));
        attachment->set_dst_color_blend_factor(blend_factor(draw.dst));
        attachment->set_src_alpha_blend_factor(blend_factor(draw.src));
        attachment->set_dst_alpha_blend_factor(blend_factor(draw.dst));
        attachment->set_color_blend_op(static_cast<RD::BlendOperation>(draw.equation));
        attachment->set_alpha_blend_op(static_cast<RD::BlendOperation>(draw.equation));
        auto color = make_ref<RDPipelineColorBlendState>();
        TypedArray<RDPipelineColorBlendStateAttachment> attachments; attachments.push_back(attachment);
        color->set_attachments(attachments);
        RID result = rd->render_pipeline_create(shaders[draw.shader], framebuffer_format, formats[vertex_format(draw.shader)], RD::RENDER_PRIMITIVE_TRIANGLES,
            raster, make_ref<RDPipelineMultisampleState>(), make_ref<RDPipelineDepthStencilState>(), color);
        if (result.is_valid()) pipelines[key] = result;
        return result;
    }
    void invalidate_texture_sets() {
        for (auto &[key, rid] : texture_sets) free(rid);
        texture_sets.clear();
    }
    void destroy_target(uint32_t id) {
        auto found = targets.find(id); if (found == targets.end()) return;
        free(found->second.framebuffer); targets.erase(found);
    }
    void destroy_texture(uint32_t id) {
        auto found = textures.find(id); if (found == textures.end()) return;
        // Unbind Godot's wrapper before the underlying RD RID is released.
        {
            std::lock_guard lock(display_mutex);
            auto display = displays.find(id);
            if (display != displays.end()) { display->second->set_texture_rd_rid(RID()); displays.erase(display); }
        }
        for (auto it = targets.begin(); it != targets.end();) {
            if (it->second.texture == id) { free(it->second.framebuffer); it = targets.erase(it); }
            else ++it;
        }
        invalidate_texture_sets();
        free(found->second.snapshot); free(found->second.rid); textures.erase(found);
    }
    void destroy_geometry(uint32_t id) {
        auto found = geometries.find(id); if (found == geometries.end()) return;
        auto &g = found->second;
        for (auto &[key, rid] : g.slices) free(rid);
        free(g.array); free(g.indices); free(g.vertices); geometries.erase(found);
    }
    void upload_texture(uint32_t id, uint32_t width, uint32_t height, ultralight::BitmapFormat format, bool target, PackedByteArray data) {
        auto found = textures.find(id);
        if (found != textures.end() && !target && found->second.width == width && found->second.height == height &&
            found->second.format == format) {
            rd->texture_update(found->second.rid, 0, data); return;
        }
        destroy_texture(id);
        Texture entry{make_texture(width,height,format,target,data), RID(),width,height,format,target};
        if (!entry.rid.is_valid()) { ready = false; return; }
        textures[id] = entry;
        if (target) {
            // Initialize padded pixels too, before the first partial draw.
            rd->texture_clear(entry.rid, godot::Color(0,0,0,0), 0, 1, 0, 1);
            auto display = make_ref<Texture2DRD>();
            display->set_texture_rd_rid(entry.rid);
            std::lock_guard lock(display_mutex); displays[id] = display;
        }
    }
    void upload_geometry(uint32_t id, int format, PackedByteArray vertices, PackedByteArray indices) {
        auto found = geometries.find(id);
        if (found != geometries.end() && found->second.format == format && found->second.vertex_bytes == vertices.size() &&
            found->second.index_bytes == indices.size()) {
            rd->buffer_update(found->second.vertices, 0, vertices.size(), vertices);
            rd->buffer_update(found->second.indices, 0, indices.size(), indices); return;
        }
        destroy_geometry(id);
        if (vertices.is_empty() || indices.is_empty()) return;
        if (vertices.size() % (format == 0 ? 20 : format == 1 ? 140 : 16) || indices.size() % 4) {
            UtilityFunctions::push_error("Ultralight GPU: malformed geometry buffer"); ready = false; return;
        }
        Geometry g; g.format = format; g.vertex_bytes = vertices.size(); g.index_bytes = indices.size();
        g.vertices = rd->vertex_buffer_create(vertices.size(), vertices);
        g.indices = rd->index_buffer_create(indices.size()/4, RD::INDEX_BUFFER_FORMAT_UINT32, indices);
        TypedArray<RID> buffers;
        for (int attribute = 0; attribute < (format == 1 ? 11 : 3); ++attribute) buffers.push_back(g.vertices);
        g.array = rd->vertex_array_create(vertices.size()/(format == 0 ? 20 : format == 1 ? 140 : 16), formats[format], buffers);
        geometries[id] = g;
        if (!g.vertices.is_valid() || !g.indices.is_valid() || !g.array.is_valid()) {
            destroy_geometry(id); ready = false;
        }
    }
    void upload_states(const std::vector<Draw> &commands) {
        PackedByteArray bytes; bytes.resize(commands.size()*sizeof(DrawUniforms));
        for (size_t i = 0; i < commands.size(); ++i)
            std::memcpy(bytes.ptrw()+i*sizeof(DrawUniforms), &commands[i].uniforms, sizeof(DrawUniforms));
        state_bytes += bytes.size();
        if (bytes.size() > state_capacity) {
            for (auto &set : state_sets) free(set);
            free(state_buffer);
            state_capacity = bytes.size();
            state_buffer = rd->storage_buffer_create(state_capacity, bytes);
            for (int i = 0; i < program_count; ++i) {
                auto uniform = make_ref<RDUniform>(); uniform->set_uniform_type(RD::UNIFORM_TYPE_STORAGE_BUFFER);
                uniform->set_binding(0); uniform->add_id(state_buffer);
                TypedArray<RDUniform> uniforms; uniforms.push_back(uniform);
                state_sets[i] = rd->uniform_set_create(uniforms, shaders[i], 0);
            }
        } else rd->buffer_update(state_buffer, 0, bytes.size(), bytes);
        if (!state_buffer.is_valid()) ready = false;
        for (auto &set : state_sets) if (!set.is_valid()) ready = false;
    }
    RID sampled_set(int shader, const RID (&ids)[4]) {
        auto key = std::make_tuple(shader,ids[0].get_id(),ids[1].get_id(),ids[2].get_id(),ids[3].get_id());
        auto found = texture_sets.find(key); if (found != texture_sets.end()) return found->second;
        TypedArray<RDUniform> uniforms;
        for (int i = 0; i < 4; ++i) {
            if (!(gpu_shaders::texture_masks[shader] & (1u << i))) continue;
            auto uniform = make_ref<RDUniform>(); uniform->set_uniform_type(RD::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE);
            uniform->set_binding(i); uniform->add_id(shader >= 5 ? data_sampler : sampler);
            uniform->add_id(ids[i]); uniforms.push_back(uniform);
        }
        RID result = rd->uniform_set_create(uniforms, shaders[shader], 1);
        texture_sets[key] = result; return result;
    }
#pragma endregion

#pragma region RenderingDevice draw commands
    void execute(const std::vector<Draw> &commands) {
        if (commands.empty() || !ready) return;
        auto started = std::chrono::steady_clock::now();
        // Godot's timestamp results are from completed frames; no explicit sync/readback.
        uint64_t first = 0;
        for (uint32_t i = 0; i < rd->get_captured_timestamps_count(); ++i) {
            auto name = rd->get_captured_timestamp_name(i);
            if (name == "Ultralight begin") first = rd->get_captured_timestamp_gpu_time(i);
            if (name == "Ultralight end" && first) gpu_us = double(rd->get_captured_timestamp_gpu_time(i)-first)/1000.0;
        }
        rd->capture_timestamp("Ultralight begin");
        upload_states(commands);
        if (!ready) return;
        // Prepare all index slices before opening a pass. Creating a slice
        // lazily in the draw loop used to break the pass at every new draw,
        // forcing thousands of attachment store/load operations after a
        // geometry rebuild (especially costly on tile-based GPUs).
        for (const auto &c : commands) {
            if (c.clear || c.flush || !c.count) continue;
            auto geometry = geometries.find(c.geometry);
            if (geometry == geometries.end()) continue;
            auto &g = geometry->second;
            if (uint64_t(c.offset)+c.count > g.index_bytes/4) continue;
            auto key = std::make_pair(c.offset,c.count);
            if (!g.slices.count(key)) g.slices[key] = rd->index_array_create(g.indices,c.offset,c.count);
        }
        int64_t list = -1;
        uint32_t active_target = 0, active_width = 0, active_height = 0;
        auto end = [&] { if (list != -1) { rd->draw_list_end(); list = -1; } };
        for (size_t index = 0; index < commands.size(); ++index) {
            const auto &c = commands[index];
            if (c.flush) { end(); continue; }
            auto target = targets.find(c.target); if (target == targets.end()) continue;
            auto &t = target->second;
            if (c.clear) {
                end(); PackedColorArray colors; colors.push_back(godot::Color(0,0,0,0));
                rd->draw_list_begin(t.framebuffer,RD::INITIAL_ACTION_CLEAR,RD::FINAL_ACTION_READ,
                                   RD::INITIAL_ACTION_DROP,RD::FINAL_ACTION_DISCARD,colors);
                rd->draw_list_end(); continue;
            }
            auto geometry = geometries.find(c.geometry);
            if (geometry == geometries.end() || c.count == 0 || c.width == 0 || c.height == 0) continue;
            auto &g = geometry->second;
            if (g.format != vertex_format(c.shader) || uint64_t(c.offset)+c.count > g.index_bytes/4) {
                UtilityFunctions::push_error("Ultralight GPU: invalid geometry/index range"); continue;
            }
            // End the previous pass before sampling *any* previous attachment.
            // Target switches and self-sampling always break a pass.
            bool feedback = false, samples_active = false;
            for (auto id : c.textures) {
                feedback |= id != 0 && id == t.texture;
                auto active = targets.find(active_target);
                samples_active |= active != targets.end() && id != 0 && id == active->second.texture;
            }
            if (active_target != c.target || active_width != c.width || active_height != c.height || feedback || samples_active) end();
            RID sampled[4] = {fallback, fallback, fallback, integer_fallback};
            for (int i = 0; i < 4; ++i) {
                auto texture = textures.find(c.textures[i]); if (texture == textures.end()) continue;
                auto &entry = texture->second;
                if (c.textures[i] == t.texture) {
                    if (!entry.snapshot.is_valid()) entry.snapshot = make_texture(entry.width, entry.height, entry.format, false);
                    // Copy once per command, even when multiple slots use the target.
                    if (std::find(c.textures, c.textures+i, c.textures[i]) == c.textures+i) {
                        rd->texture_copy(entry.rid,entry.snapshot,Vector3(),Vector3(),Vector3(entry.width,entry.height,1),0,0,0,0);
                        ++feedback_copies;
                    }
                    sampled[i] = entry.snapshot;
                } else sampled[i] = entry.rid;
            }
            auto slice_key = std::make_pair(c.offset,c.count);
            RID pipe = pipeline(c,rd->framebuffer_get_format(t.framebuffer));
            bool textured = gpu_shaders::texture_masks[c.shader] != 0;
            RID texture_set = textured ? sampled_set(c.shader, sampled) : RID();
            if (!pipe.is_valid() || (textured && !texture_set.is_valid()) || !g.slices[slice_key].is_valid()) { end(); ready = false; break; }
            if (list == -1) {
                // Region determines viewport. Ultralight targets may have padding.
                list = rd->draw_list_begin(t.framebuffer,RD::INITIAL_ACTION_KEEP,RD::FINAL_ACTION_READ,
                    RD::INITIAL_ACTION_DROP,RD::FINAL_ACTION_DISCARD,{},1.0,0,Rect2(0,0,c.width,c.height));
                active_target = c.target; active_width = c.width; active_height = c.height;
                ++passes;
            }
            if (c.scissor) {
                Rect2 rect = c.scissor_rect.intersection(Rect2(0,0,std::min(c.width,t.width),std::min(c.height,t.height)));
                if (!rect.has_area()) continue;
                rd->draw_list_enable_scissor(list,rect);
            } else rd->draw_list_disable_scissor(list);
            rd->draw_list_bind_render_pipeline(list,pipe);
            rd->draw_list_bind_uniform_set(list,state_sets[c.shader],0);
            if (textured) rd->draw_list_bind_uniform_set(list,texture_set,1);
            rd->draw_list_bind_vertex_array(list,g.array);
            rd->draw_list_bind_index_array(list,g.slices[slice_key]);
            uint32_t record[4] = {uint32_t(index),0,0,0};
            auto push = copy_bytes(record,sizeof(record));
            rd->draw_list_set_push_constant(list,push,push.size());
            rd->draw_list_draw(list,true,1); ++draws; ++shader_draws[c.shader];
        }
        end();
        rd->capture_timestamp("Ultralight end"); ++batches;
        record_us = std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-started).count();
    }
#pragma endregion

#pragma region resource cleanup
    void cleanup() {
        ready = false;
        if (!rd) return;
        while (!textures.empty()) destroy_texture(textures.begin()->first);
        while (!targets.empty()) destroy_target(targets.begin()->first);
        while (!geometries.empty()) destroy_geometry(geometries.begin()->first);
        invalidate_texture_sets();
        for (auto &[key,rid] : pipelines) free(rid);
        pipelines.clear();
        for (auto &rid : state_sets) free(rid);
        free(state_buffer); state_capacity = 0;
        free(fallback); free(integer_fallback); free(sampler); free(data_sampler);
        for (auto &rid : shaders) free(rid);
        rd = nullptr;
    }
#pragma endregion
};

inline void GodotGPUDriver::dispatchPendingOperations() {
    if (impl->pending.empty()) return;
    auto jobs = std::move(impl->pending); impl->pending.clear();
    if (impl->stopped) return;
    Impl::enqueue([state=impl,jobs=std::move(jobs)] { if (state->rd) for (auto &job : jobs) job(*state); });
}
#pragma endregion

#pragma region Godot integration method implementations
inline GodotGPUDriver::GodotGPUDriver() : impl(std::make_shared<Impl>()) {}
inline GodotGPUDriver::~GodotGPUDriver() = default;
inline GodotGPUDriver &GodotGPUDriver::instance() { static GodotGPUDriver driver; return driver; }
inline bool GodotGPUDriver::initialize() {
    if (impl->attempted) return available();
    impl->attempted = true;
    if (DisplayServer::get_singleton()->get_name() == "headless") return false;
    auto fence = std::make_shared<std::promise<void>>(); auto future = fence->get_future();
    Impl::enqueue([state=impl,fence] { state->ready = state->init(); if (!state->ready) state->cleanup(); fence->set_value(); });
    future.get(); // Startup fence only. Never submit/sync the main device.
    return available();
}
inline bool GodotGPUDriver::available() const { return impl->ready && !impl->stopped; }
inline void GodotGPUDriver::shutdown() {
    if (impl->stopped) return;
    dispatchPendingOperations(); impl->stopped = true;
    if (!impl->attempted) return;
    auto fence = std::make_shared<std::promise<void>>(); auto future = fence->get_future();
    Impl::enqueue([state=impl,fence] { state->cleanup(); fence->set_value(); }); future.get();
}
inline Ref<Texture2DRD> GodotGPUDriver::display_texture(uint32_t id) const {
    std::lock_guard lock(impl->display_mutex);
    auto found = impl->displays.find(id); return found == impl->displays.end() ? Ref<Texture2DRD>() : found->second;
}
inline Ref<ShaderMaterial> GodotGPUDriver::createDisplayMaterial() {
    return GPUDisplayShader::createMaterial();
}
inline Dictionary GodotGPUDriver::statistics() const {
    Dictionary result;
    result["gpu_available"] = available();
    result["bitmap_upload_bytes"] = int64_t(impl->bitmap_bytes.load());
    result["geometry_upload_bytes"] = int64_t(impl->geometry_bytes.load());
    result["state_upload_bytes"] = int64_t(impl->state_bytes.load());
    result["draws"] = int64_t(impl->draws.load()); result["batches"] = int64_t(impl->batches.load());
    result["feedback_copies"] = int64_t(impl->feedback_copies.load());
    result["render_passes"] = int64_t(impl->passes.load());
    result["rd_record_us"] = impl->record_us.load(); result["gpu_us"] = impl->gpu_us.load();
    Dictionary counts;
    const char *names[] = {"Fill", "FillPath", "FilterBasic", "FilterBlur", "FilterDropShadow", "FillPhoton", "FillPhotonGrid"};
    for (int i = 0; i < Impl::program_count; ++i) counts[names[i]] = int64_t(impl->shader_draws[i].load());
    result["gpu_draws_by_shader"] = counts;
    return result;
}
#pragma endregion

#pragma region ultralight::GPUDriver interface implementations
inline void GodotGPUDriver::BeginSynchronize() {}
inline void GodotGPUDriver::EndSynchronize() { dispatchPendingOperations(); }
inline uint32_t GodotGPUDriver::NextTextureId() { return ++texture_id; }
inline uint32_t GodotGPUDriver::NextRenderBufferId() { return ++buffer_id; }
inline uint32_t GodotGPUDriver::NextGeometryId() { return ++geometry_id; }
inline void GodotGPUDriver::CreateTexture(uint32_t id, ultralight::RefPtr<ultralight::Bitmap> bitmap, uint32_t flags) {
    uint32_t width = bitmap->width(), height = bitmap->height();
    bool target = (flags & ultralight::kGPUTextureFlag_RenderTarget) != 0;
    auto format = bitmap->format();
    PackedByteArray data;
    if (Impl::texture_format(format) == Impl::RD::DATA_FORMAT_MAX) {
        impl->pending.push_back([](Impl &state) { state.ready = false; }); return;
    }
    if (!target) {
        uint32_t row = width * ultralight::GetBitmapFormatInfo(format).block_bytes;
        data.resize(uint64_t(row) * height);
        auto pixels = bitmap->LockPixelsSafe();
        for (uint32_t y = 0; y < height; ++y) {
            auto *dst = data.ptrw() + size_t(y) * row;
            const auto *src = static_cast<const uint8_t *>(pixels.data()) + size_t(y) * bitmap->row_bytes();
            if (format != ultralight::BitmapFormat::BGRA8_UNORM_SRGB) std::memcpy(dst, src, row);
            else for (uint32_t x = 0; x < width; ++x) {
                dst[4*x] = src[4*x+2]; dst[4*x+1] = src[4*x+1]; dst[4*x+2] = src[4*x]; dst[4*x+3] = src[4*x+3];
            }
        }
        impl->bitmap_bytes += data.size();
    }
    impl->pending.push_back([=](Impl &state) { state.upload_texture(id,width,height,format,target,data); });
}
inline void GodotGPUDriver::UpdateTexture(uint32_t id, ultralight::RefPtr<ultralight::Bitmap> bitmap, const ultralight::IntRect &) {
    if (bitmap->IsEmpty()) return;
    CreateTexture(id, bitmap, 0); // Full upload; the rectangle is an optional optimization hint.
}
inline void GodotGPUDriver::GetDeviceCaps(ultralight::GPUDeviceCaps &caps) {
    // No optional MSAA/compression/partial-redraw claims. Mandatory programs are preflighted.
    caps = {};
}
inline void GodotGPUDriver::DestroyTexture(uint32_t id) { impl->pending.push_back([=](Impl &state) { state.destroy_texture(id); }); }
inline void GodotGPUDriver::CreateRenderBuffer(uint32_t id, const ultralight::RenderBuffer &buffer) {
    uint32_t texture=buffer.texture_id, width=buffer.width, height=buffer.height;
    impl->pending.push_back([=](Impl &state) {
        state.destroy_target(id);
        auto found = state.textures.find(texture); if (found == state.textures.end()) return;
        TypedArray<RID> attachments; attachments.push_back(found->second.rid);
        state.targets[id] = {state.rd->framebuffer_create(attachments),texture,width,height};
        if (!state.targets[id].framebuffer.is_valid()) state.ready = false;
    });
}
inline void GodotGPUDriver::DestroyRenderBuffer(uint32_t id) { impl->pending.push_back([=](Impl &state) { state.destroy_target(id); }); }
inline void GodotGPUDriver::CreateGeometry(uint32_t id, const ultralight::VertexBuffer &vertices, const ultralight::IndexBuffer &indices) {
    int format = static_cast<int>(vertices.format);
    auto v = Impl::copy_bytes(vertices.data,vertices.size), i = Impl::copy_bytes(indices.data,indices.size);
    impl->geometry_bytes += v.size()+i.size();
    impl->pending.push_back([=](Impl &state) { state.upload_geometry(id,format,v,i); });
}
inline void GodotGPUDriver::UpdateGeometry(uint32_t id, const ultralight::VertexBuffer &vertices, const ultralight::IndexBuffer &indices) { CreateGeometry(id,vertices,indices); }
inline void GodotGPUDriver::DestroyGeometry(uint32_t id) { impl->pending.push_back([=](Impl &state) { state.destroy_geometry(id); }); }
inline void GodotGPUDriver::UpdateCommandList(const ultralight::CommandList &list) {
    std::vector<Impl::Draw> commands; commands.reserve(list.size);
    for (uint32_t i=0; i<list.size; ++i) {
        const auto &src = list.commands[i]; const auto &s = src.gpu_state;
        Impl::Draw c;
        if (src.command_type == ultralight::CommandType::Flush) {
            c.flush = true; commands.push_back(c); continue;
        }
        c.clear = src.command_type == ultralight::CommandType::ClearRenderBuffer;
        c.target=s.render_buffer_id;
        if (c.clear) { commands.push_back(c); continue; }
        c.geometry=src.geometry_id; c.count=src.indices_count; c.offset=src.indices_offset;
        c.width=s.viewport_width; c.height=s.viewport_height; c.blend=s.enable_blend;
        c.shader = static_cast<int>(s.shader_type); c.texturing=s.enable_texturing;
        c.src=s.blend_src_factor; c.dst=s.blend_dst_factor; c.equation=s.blend_equation;
        if (c.shader < 0 || c.shader >= Impl::program_count) { impl->pending.push_back([](Impl &state) { state.ready = false; }); return; }
        c.textures[0]=s.texture_1_id; c.textures[1]=s.texture_2_id;
        c.textures[2]=s.texture_3_id; c.textures[3]=s.texture_4_id;
        for (int slot = 0; slot < 4; ++slot)
            if (!(gpu_shaders::texture_masks[c.shader] & (1u << slot)) || (!c.texturing && c.shader < 5)) c.textures[slot] = 0;
        c.scissor=s.enable_scissor;
        c.scissor_rect=Rect2(s.scissor_rect.left,s.scissor_rect.top,s.scissor_rect.width(),s.scissor_rect.height());
        if (!c.clear && c.width && c.height) {
            ultralight::Matrix model, projection; model.Set(s.transform);
            projection.SetOrthographicProjection(c.width,c.height,false); projection.Transform(model);
            auto transform = projection.GetMatrix4x4();
            for (int j=0; j<16; ++j) c.uniforms.transform[j]=transform.data[j];
            c.uniforms.state[1]=c.width; c.uniforms.state[2]=c.height; c.uniforms.state[3]=1;
            for (int j=0; j<8; ++j) {
                c.uniforms.integer[j]=s.uniform_integer[j];
                c.uniforms.scalar[j]=s.uniform_scalar[j];
                c.uniforms.vector[4*j]=s.uniform_vector[j].x; c.uniforms.vector[4*j+1]=s.uniform_vector[j].y;
                c.uniforms.vector[4*j+2]=s.uniform_vector[j].z; c.uniforms.vector[4*j+3]=s.uniform_vector[j].w;
            }
            c.uniforms.clip_size[0]=std::min<uint32_t>(s.clip_size,8);
            for (uint32_t j=0; j<c.uniforms.clip_size[0]; ++j)
                for (int k=0; k<16; ++k) c.uniforms.clip[j*16+k]=s.clip[j].data[k];
        }
        commands.push_back(c);
    }
    impl->pending.push_back([commands=std::move(commands)](Impl &state) { state.execute(commands); });
}
#pragma endregion
} // namespace gdbind
