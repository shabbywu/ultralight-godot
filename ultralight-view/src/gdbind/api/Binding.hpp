#pragma once
#include "Types.hpp"
#include <functional>
#include <godot_cpp/variant/packed_string_array.hpp>
namespace gdbind {
class UltralightBindingOptions : public RefCounted {
    GDCLASS(UltralightBindingOptions, RefCounted);

  public:
    bool copy_containers = false, read_only = false;
    PackedStringArray members;
    bool get_copy_containers() const {
        return copy_containers;
    }
    void set_copy_containers(bool value) {
        copy_containers = value;
    }
    bool get_read_only() const {
        return read_only;
    }
    void set_read_only(bool value) {
        read_only = value;
    }
    PackedStringArray get_members() const {
        return members;
    }
    void set_members(const PackedStringArray &value) {
        members = value;
    }

  protected:
    static void _bind_methods() {
#define UL_OPTION(type, name)                                                                                          \
    ClassDB::bind_method(D_METHOD("get_" #name), &UltralightBindingOptions::get_##name);                               \
    ClassDB::bind_method(D_METHOD("set_" #name, "value"), &UltralightBindingOptions::set_##name);                      \
    ADD_PROPERTY(PropertyInfo(type, #name), "set_" #name, "get_" #name)
        UL_OPTION(Variant::BOOL, copy_containers);
        UL_OPTION(Variant::BOOL, read_only);
        UL_OPTION(Variant::PACKED_STRING_ARRAY, members);
#undef UL_OPTION
    }
};
class UltralightBinding : public RefCounted {
    GDCLASS(UltralightBinding, RefCounted);

  public:
    std::function<void()> revoke;
    std::function<bool()> valid;
    bool is_closed() const {
        return !valid || !valid();
    }
    void close() {
        if (revoke) {
            auto action = std::exchange(revoke, {});
            action();
        }
    }

  protected:
    static void _bind_methods() {
        ClassDB::bind_method(D_METHOD("close"), &UltralightBinding::close);
        ClassDB::bind_method(D_METHOD("is_closed"), &UltralightBinding::is_closed);
    }
};
} // namespace gdbind
