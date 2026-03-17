#pragma once

#include <godot_cpp/classes/rd_vertex_attribute.hpp>
#include <godot_cpp/classes/rendering_device.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <unordered_map>

// 承载几何体所有底层资源的结构体
struct RDGeometry {
    godot::RID vertex_array;  // VAO (包含格式与绑定的 VBO)
    godot::RID vertex_buffer; // VBO (实际的顶点数据内存)
    godot::RID index_array;   // Index Array (用于绑定到 Draw List)
    godot::RID index_buffer;  // IBO (实际的索引数据内存)

    // 辅助判定函数
    inline bool is_valid() const {
        return vertex_array.is_valid();
    }
};

namespace gdbind {

/**
 * 使用 godot-cpp RenderingDevice API 创建 Geometry (2f_4ub_2f 格式)
 *
 * 顶点格式: pos(2 floats) + color(4 uint8_t) + tex_coord(2 floats) = 16 bytes
 *
 * @param rd RenderingDevice 实例
 * @param vertex_data 顶点数据
 * @param index_data 索引数据
 * @return 返回 RDGeometry
 */
static RDGeometry create_geometry__2f_4ub_2f(godot::RenderingDevice *rd, const godot::PackedByteArray &vertex_data,
                                             const godot::PackedByteArray &index_data) {
    RDGeometry geo;
    if (vertex_data.is_empty() || index_data.is_empty()) {
        return geo;
    }

    // 1. 创建底层 Buffer
    geo.vertex_buffer = rd->vertex_buffer_create(vertex_data.size(), vertex_data);

    // 假设索引是 32 位整型 (PackedInt32Array.to_byte_array())
    uint32_t index_count = index_data.size() / 4;
    geo.index_buffer = rd->index_buffer_create(
        index_count * 4, godot::RenderingDevice::IndexBufferFormat::INDEX_BUFFER_FORMAT_UINT32, index_data);

    // 单独创建 Index Array
    geo.index_array = rd->index_array_create(geo.index_buffer, 0, index_count);

    // 2. 配置顶点格式
    godot::TypedArray<godot::RDVertexAttribute> attrs;
    uint32_t stride = 20;

    godot::Ref<godot::RDVertexAttribute> pos;
    pos.instantiate();
    pos->set_location(0);
    pos->set_format(godot::RenderingDevice::DATA_FORMAT_R32G32_SFLOAT);
    pos->set_offset(0);
    pos->set_stride(stride);
    attrs.push_back(pos);

    godot::Ref<godot::RDVertexAttribute> col;
    col.instantiate();
    col->set_location(1);
    col->set_format(godot::RenderingDevice::DATA_FORMAT_R8G8B8A8_UNORM);
    col->set_offset(8);
    col->set_stride(stride);
    attrs.push_back(col);

    godot::Ref<godot::RDVertexAttribute> uv;
    uv.instantiate();
    uv->set_location(2);
    uv->set_format(godot::RenderingDevice::DATA_FORMAT_R32G32_SFLOAT);
    uv->set_offset(12);
    uv->set_stride(stride);
    attrs.push_back(uv);

    int64_t format = rd->vertex_format_create(attrs);

    // 3. 创建 Vertex Array
    godot::TypedArray<godot::RID> buffers;
    buffers.push_back(geo.vertex_buffer);

    godot::PackedInt64Array offsets;
    offsets.push_back(0);

    uint32_t vertex_count = vertex_data.size() / stride;
    geo.vertex_array = rd->vertex_array_create(vertex_count, format, buffers, offsets);

    return geo;
}

/**
 * 更新 Geometry 数据 (2f_4ub_2f 格式)
 *
 * @param rd RenderingDevice 实例
 * @param geo RDGeometry 结构体
 * @param vertex_data 新的顶点数据
 * @param index_data 新的索引数据
 */
static void update_geometry__2f_4ub_2f(godot::RenderingDevice *rd, const RDGeometry &geo,
                                       const godot::PackedByteArray &vertex_data,
                                       const godot::PackedByteArray &index_data) {
    if (!geo.is_valid()) {
        godot::UtilityFunctions::printerr("Geometry Update Failed: Invalid Geometry Struct.");
        return;
    }

    if (!vertex_data.is_empty()) {
        rd->buffer_update(geo.vertex_buffer, 0, vertex_data.size(), vertex_data);
    }
    if (!index_data.is_empty()) {
        rd->buffer_update(geo.index_buffer, 0, index_data.size(), index_data);
    }
}

/**
 * 使用 godot-cpp RenderingDevice API 创建 Geometry (2f_4ub_2f_2f_28f 格式)
 *
 * 顶点格式: pos(2 floats) + color(4 uint8_t) + tex(2 floats) + obj(2 floats) + data_0~6(28 floats)
 * 总大小: 8 + 4 + 8 + 8 + 112 = 140 bytes (对齐后可能是144)
 *
 * @param rd RenderingDevice 实例
 * @param vertex_data 顶点数据
 * @param index_data 索引数据
 * @return 返回创建的 RDGeometry 结构体
 */
static RDGeometry create_geometry__2f_4ub_2f_2f_28f(godot::RenderingDevice *rd,
                                                    const godot::PackedByteArray &vertex_data,
                                                    const godot::PackedByteArray &index_data) {
    RDGeometry geo;
    if (vertex_data.is_empty() || index_data.is_empty()) {
        return geo;
    }

    // 1. 创建底层 Buffer
    geo.vertex_buffer = rd->vertex_buffer_create(vertex_data.size(), vertex_data);

    uint32_t index_count = index_data.size() / 4;
    geo.index_buffer = rd->index_buffer_create(
        index_count * 4, godot::RenderingDevice::IndexBufferFormat::INDEX_BUFFER_FORMAT_UINT32, index_data);
    geo.index_array = rd->index_array_create(geo.index_buffer, 0, index_count);

    // 2. 配置顶点格式
    godot::TypedArray<godot::RDVertexAttribute> attrs;
    uint32_t stride = 140;

    godot::Ref<godot::RDVertexAttribute> pos;
    pos.instantiate();
    pos->set_location(0);
    pos->set_format(godot::RenderingDevice::DATA_FORMAT_R32G32_SFLOAT);
    pos->set_offset(0);
    pos->set_stride(stride);
    attrs.push_back(pos);

    godot::Ref<godot::RDVertexAttribute> col;
    col.instantiate();
    col->set_location(1);
    col->set_format(godot::RenderingDevice::DATA_FORMAT_R8G8B8A8_UNORM);
    col->set_offset(8);
    col->set_stride(stride);
    attrs.push_back(col);

    godot::Ref<godot::RDVertexAttribute> uv1;
    uv1.instantiate();
    uv1->set_location(2);
    uv1->set_format(godot::RenderingDevice::DATA_FORMAT_R32G32_SFLOAT);
    uv1->set_offset(12);
    uv1->set_stride(stride);
    attrs.push_back(uv1);

    godot::Ref<godot::RDVertexAttribute> uv2;
    uv2.instantiate();
    uv2->set_location(3);
    uv2->set_format(godot::RenderingDevice::DATA_FORMAT_R32G32_SFLOAT);
    uv2->set_offset(20);
    uv2->set_stride(stride);
    attrs.push_back(uv2);

    // 处理附加的 28 个 float
    for (int i = 0; i < 7; ++i) {
        godot::Ref<godot::RDVertexAttribute> custom;
        custom.instantiate();
        custom->set_location(4 + i);
        custom->set_format(godot::RenderingDevice::DATA_FORMAT_R32G32B32A32_SFLOAT);
        custom->set_offset(28 + (i * 16));
        custom->set_stride(stride);
        attrs.push_back(custom);
    }

    int64_t format = rd->vertex_format_create(attrs);

    // 3. 创建 Vertex Array
    godot::TypedArray<godot::RID> buffers;
    buffers.push_back(geo.vertex_buffer);

    godot::PackedInt64Array offsets;
    offsets.push_back(0);

    uint32_t vertex_count = vertex_data.size() / stride;
    geo.vertex_array = rd->vertex_array_create(vertex_count, format, buffers, offsets);

    return geo;
}

/**
 * 更新 Geometry 数据 (2f_4ub_2f_2f_28f 格式)
 *
 * @param rd RenderingDevice 实例
 * @param geom Geometry 结构体
 * @param vertex_data 新的顶点数据
 * @param index_data 新的索引数据
 */
static void update_geometry__2f_4ub_2f_2f_28f(godot::RenderingDevice *rd, const RDGeometry &geo,
                                              const godot::PackedByteArray &vertex_data,
                                              const godot::PackedByteArray &index_data) {
    // 注意：在底层层面，更新数据的逻辑完全不关心格式！
    // 只要有合法的 VBO/IBO，调用方式和上一格式是完全一样的。
    update_geometry__2f_4ub_2f(rd, geo, vertex_data, index_data);
}

/**
 * 销毁 Geometry
 *
 * @param rd RenderingDevice 实例
 * @param geom 要销毁的 Geometry 结构体
 */
static void destroy_geometry(godot::RenderingDevice *rd, RDGeometry &geo) {
    if (!geo.is_valid()) {
        return;
    }

    // 释放顺序建议：先释放依赖方(Array/View)，再释放内存实体(Buffer)
    if (geo.vertex_array.is_valid()) {
        rd->free_rid(geo.vertex_array);
        geo.vertex_array = godot::RID();
    }
    if (geo.index_array.is_valid()) {
        rd->free_rid(geo.index_array);
        geo.index_array = godot::RID();
    }
    if (geo.vertex_buffer.is_valid()) {
        rd->free_rid(geo.vertex_buffer);
        geo.vertex_buffer = godot::RID();
    }
    if (geo.index_buffer.is_valid()) {
        rd->free_rid(geo.index_buffer);
        geo.index_buffer = godot::RID();
    }
}

static void execute_geometry_draw(godot::RenderingDevice *rd, int64_t draw_list, const RDGeometry &geo) {
    if (!geo.is_valid())
        return;

    // 1. 绑定顶点数组
    rd->draw_list_bind_vertex_array(draw_list, geo.vertex_array);

    // 2. 绑定索引数组
    rd->draw_list_bind_index_array(draw_list, geo.index_array);

    // 3. 执行基于索引的绘制 (use_indices = true)
    rd->draw_list_draw(draw_list, true, 1);
}

} // namespace gdbind
