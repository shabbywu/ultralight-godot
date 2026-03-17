#pragma once

#include <Ultralight/Bitmap.h>
#include <Ultralight/platform/GPUDriver.h>
#include <godot_cpp/classes/rd_pipeline_color_blend_state.hpp>
#include <godot_cpp/classes/rd_pipeline_depth_stencil_state.hpp>
#include <godot_cpp/classes/rd_pipeline_multisample_state.hpp>
#include <godot_cpp/classes/rd_pipeline_rasterization_state.hpp>
#include <godot_cpp/classes/rd_shader_source.hpp>
#include <godot_cpp/classes/rd_shader_spirv.hpp>
#include <godot_cpp/classes/rd_texture_format.hpp>
#include <godot_cpp/classes/rd_texture_view.hpp>
#include <godot_cpp/classes/rd_vertex_attribute.hpp>
#include <godot_cpp/classes/rendering_device.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2i.hpp>

#include <cstring>
#include <map>
#include <memory>
#include <vector>

// Include generated GLSL shader headers
#include "shader_fill_frag.h"
#include "shader_fill_path_frag.h"
#include "shader_v2f_c4f_t2f_t2f_d28f_vert.h"
#include "shader_v2f_c4f_t2f_vert.h"

// Include gpu render implement
#include "./_geometry_helper.hpp"
#include "./_render_buffer_helper.hpp"
#include "./_texture_helper.hpp"

using namespace godot;

namespace gdbind {

class GodotGPUDriver : public ultralight::GPUDriver {
  public:
    GodotGPUDriver();
    virtual ~GodotGPUDriver();

    // Inherited from GPUDriver

    ///
    /// Called before any state (eg, CreateTexture(), UpdateTexture(), DestroyTexture(), etc.) is
    /// updated during a call to Renderer::Render().
    ///
    /// This is a good time to prepare the GPU for any state updates.
    ///
    virtual void BeginSynchronize() override {
    }

    ///
    /// Called after all state has been updated during a call to Renderer::Render().
    ///
    virtual void EndSynchronize() override;

    ///
    /// Get the next available texture ID.
    ///
    /// This is used to generate a unique texture ID for each texture created by the library. The
    /// GPU driver implementation is responsible for mapping these IDs to a native ID.
    ///
    /// @note Numbering should start at 1, 0 is reserved for "no texture".
    ///
    /// @return Returns the next available texture ID.
    ///
    virtual uint32_t NextTextureId() {
        return next_texture_id_++;
    }

    ///
    /// Create a texture with a certain ID and optional bitmap.
    ///
    /// @param texture_id  The texture ID to use for the new texture.
    ///
    /// @param bitmap      The bitmap to initialize the texture with (can be empty).
    ///
    /// @note If the Bitmap is empty (Bitmap::IsEmpty), then a RTT Texture should be created instead.
    ///       This will be used as a backing texture for a new RenderBuffer.
    ///
    /// @warning A deep copy of the bitmap data should be made if you are uploading it to the GPU
    ///          asynchronously, it will not persist beyond this call.
    ///
    virtual void CreateTexture(uint32_t texture_id, ultralight::RefPtr<ultralight::Bitmap> bitmap) override;

    ///
    /// Update an existing non-RTT texture with new bitmap data.
    ///
    /// @param texture_id  The texture to update.
    ///
    /// @param bitmap      The new bitmap data.
    ///
    /// @warning A deep copy of the bitmap data should be made if you are uploading it to the GPU
    ///          asynchronously, it will not persist beyond this call.
    ///
    virtual void UpdateTexture(uint32_t texture_id, ultralight::RefPtr<ultralight::Bitmap> bitmap) override;

    ///
    /// Destroy a texture.
    ///
    /// @param texture_id  The texture to destroy.
    ///
    virtual void DestroyTexture(uint32_t texture_id) override;

    ///
    /// Get the next available render buffer ID.
    ///
    /// This is used to generate a unique render buffer ID for each render buffer created by the
    /// library. The GPU driver implementation is responsible for mapping these IDs to a native ID.
    ///
    /// @note Numbering should start at 1, 0 is reserved for "no render buffer".
    ///
    /// @return Returns the next available render buffer ID.
    ///
    virtual uint32_t NextRenderBufferId() {
        return next_render_buffer_id_++;
    }

    ///
    /// Create a render buffer with certain ID and buffer description.
    ///
    /// @param render_buffer_id  The render buffer ID to use for the new render buffer.
    ///
    /// @param buffer           The render buffer description.
    ///
    virtual void CreateRenderBuffer(uint32_t render_buffer_id, const ultralight::RenderBuffer &buffer) override;

    ///
    /// Destroy a render buffer.
    ///
    /// @param render_buffer_id  The render buffer to destroy.
    ///
    virtual void DestroyRenderBuffer(uint32_t render_buffer_id) override;

    ///
    /// Get the next available geometry ID.
    ///
    /// This is used to generate a unique geometry ID for each geometry created by the library. The
    /// GPU driver implementation is responsible for mapping these IDs to a native ID.
    ///
    /// @note Numbering should start at 1, 0 is reserved for "no geometry".
    ///
    /// @return Returns the next available geometry ID.
    ///
    virtual uint32_t NextGeometryId() {
        return next_geometry_id_++;
    }

    ///
    /// Create geometry with certain ID and vertex/index data.
    ///
    /// @param geometry_id  The geometry ID to use for the new geometry.
    ///
    /// @param vertices     The vertex buffer data.
    ///
    /// @param indices      The index buffer data.
    ///
    /// @warning A deep copy of the vertex/index data should be made if you are uploading it to the
    ///          GPU asynchronously, it will not persist beyond this call.
    ///
    virtual void CreateGeometry(uint32_t geometry_id, const ultralight::VertexBuffer &vertices,
                                const ultralight::IndexBuffer &indices) override;

    ///
    /// Update existing geometry with new vertex/index data.
    ///
    /// @param geometry_id  The geometry to update.
    ///
    /// @param vertices     The new vertex buffer data.
    ///
    /// @param indices      The new index buffer data.
    ///
    /// @warning A deep copy of the vertex/index data should be made if you are uploading it to the
    ///          GPU asynchronously, it will not persist beyond this call.
    ///
    virtual void UpdateGeometry(uint32_t geometry_id, const ultralight::VertexBuffer &vertices,
                                const ultralight::IndexBuffer &indices) override;

    ///
    /// Update the pending command list with commands to execute on the GPU.
    ///
    /// Commands are dispatched to the GPU driver asynchronously via this method. The GPU driver
    /// implementation should consume these commands and execute them at an appropriate time.
    ///
    /// @param list  The list of commands to execute.
    ///
    /// @warning Implementations should make a deep copy of the command list, it will not persist
    ///          beyond this call.
    ///
    virtual void UpdateCommandList(const ultralight::CommandList &list) override;

  protected:
    uint32_t next_texture_id_ = 1;
    uint32_t next_render_buffer_id_ = 1; // render buffer id 0 is reserved for default render target view.
    uint32_t next_geometry_id_ = 1;

    // virtual id -> godot native id
    std::map<uint32_t, godot::RID> texture_map_;
    std::map<uint32_t, godot::RID> render_buffer_map_;
    std::map<uint32_t, RDGeometry> geometry_map_;
    std::vector<ultralight::Command> command_list_;
    int batch_count_;
};

} // namespace gdbind
