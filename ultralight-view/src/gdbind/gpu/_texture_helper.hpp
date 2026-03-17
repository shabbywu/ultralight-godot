#pragma once

#include <godot_cpp/classes/rd_texture_format.hpp>
#include <godot_cpp/classes/rd_texture_view.hpp>
#include <godot_cpp/classes/rendering_device.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/rid.hpp>

namespace gdbind {

/**
 * 使用 godot-cpp RenderingDevice API 创建纹理
 *
 * @param rd RenderingDevice 实例
 * @param width 纹理宽度
 * @param height 纹理高度
 * @param data 纹理像素数据 (RGBA8 格式)
 * @return 返回创建的纹理 RID
 */
static godot::RID create_texture(godot::RenderingDevice *rd, int width, int height, godot::PackedByteArray data) {
    godot::Ref<godot::RDTextureFormat> tf;
    tf.instantiate();
    // 设置纹理格式 (RGBA8 Unorm)
    tf->set_width(width);
    tf->set_height(height);
    tf->set_format(godot::RenderingDevice::DATA_FORMAT_R8G8B8A8_UNORM);
    // 允许该纹理作为采样器(Shader读取)和颜色附件(渲染目标)
    tf->set_usage_bits(godot::RenderingDevice::TEXTURE_USAGE_SAMPLING_BIT |
                       godot::RenderingDevice::TEXTURE_USAGE_CAN_UPDATE_BIT);

    godot::Ref<godot::RDTextureView> tv;
    tv.instantiate();

    // 传入包含像素数据的数组（如果需要空纹理，可以传空的 Array）
    TypedArray<PackedByteArray> texture_data;
    if (!data.is_empty()) {
        texture_data.push_back(data);
    }

    godot::RID texture = rd->texture_create(tf, tv, texture_data);
    return texture;
}

/**
 * 更新已存在的纹理数据
 *
 * @param rd RenderingDevice 实例
 * @param texture 要更新的纹理 RID
 * @param data 新的纹理像素数据 (RGBA8 格式)
 */
static void update_texture(godot::RenderingDevice *rd, godot::RID texture, godot::PackedByteArray data) {
    rd->texture_update(texture, false, data);
}

/**
 * 销毁纹理
 *
 * @param rd RenderingDevice 实例
 * @param texture 要销毁的纹理 RID
 */
static void destroy_texture(godot::RenderingDevice *rd, godot::RID texture) {
    if (texture.is_valid()) {
        rd->free_rid(texture);
    }
}

} // namespace gdbind
