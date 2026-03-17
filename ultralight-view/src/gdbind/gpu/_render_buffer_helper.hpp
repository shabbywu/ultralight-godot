#pragma once

#include <godot_cpp/classes/rendering_device.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/vector3i.hpp>
#include <vector>

namespace gdbind {

/**
 * 使用 godot-cpp RenderingDevice API 从纹理创建 Framebuffer (Render Buffer)
 *
 * @param rd RenderingDevice 实例
 * @param texture_id 颜色附件纹理的 RID (由 create_texture 创建的纹理)
 * @return 返回创建的 Framebuffer RID
 */
static godot::RID create_render_buffer(godot::RenderingDevice *rd, godot::RID texture_id) {
    // 将想要渲染进去的纹理存入数组
    godot::TypedArray<godot::RID> attachments;
    attachments.push_back(texture_id);

    // 创建 Framebuffer
    godot::RID framebuffer_rid = rd->framebuffer_create(attachments);
    return framebuffer_rid;
}

/**
 * 销毁 render buffer (framebuffer)
 *
 * @param rd RenderingDevice 实例
 * @param framebuffer 要销毁的 Framebuffer RID
 */
static void destroy_render_buffer(godot::RenderingDevice *rd, godot::RID framebuffer) {
    if (framebuffer.is_valid()) {
        rd->free_rid(framebuffer);
    }
}

} // namespace gdbind
