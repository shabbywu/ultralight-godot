#pragma once
#include "Binding.hpp"
#include "Types.hpp"
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <map>
#include <set>

namespace gdbind {
class UltralightAPI : public RefCounted {
    GDCLASS(UltralightAPI, RefCounted);
    String namespace_;
    bool closed = false;
    std::set<ObjectID> views;
    Dictionary entries;
    static bool identifier(const String &name) {
        if (name.is_empty())
            return false;
        for (int64_t i = 0; i < name.length(); ++i) {
            auto c = name[i];
            if (!(c == '_' || c == '$' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                  (i > 0 && c >= '0' && c <= '9')))
                return false;
        }
        return true;
    }
    Ref<UltralightResult> put(const String &name, const Variant &value, bool async,
                              const Ref<UltralightBindingOptions> &options) {
        Ref<UltralightAPI> keep(this);
        if (closed)
            return UltralightResult::failure("Closed", "API registry is closed");
        if (!identifier(name) || name == "$bridge")
            return UltralightResult::failure("InvalidArgument", "Entry name must be a JavaScript identifier");
        Dictionary entry;
        entry["value"] = value;
        entry["async"] = async;
        entry["members"] = options.is_valid() ? options->members : PackedStringArray();
        entry["copy"] = options.is_valid() && options->copy_containers;
        entry["read_only"] = options.is_valid() && options->read_only;
        auto previous = entries.get(name, Variant());
        entries[name] = entry;
        auto targets = views;
        for (auto id : targets) {
            if (auto view = ObjectDB::get_instance(id)) {
                Ref<UltralightResult> r = view->call("_refresh_api", Ref<UltralightAPI>(this), name);
                if (closed)
                    return UltralightResult::failure("Closed", "API registry was closed during registration");
                if (r.is_valid() && !r->is_ok()) {
                    if (previous.get_type() == Variant::NIL)
                        entries.erase(name);
                    else
                        entries[name] = previous;
                    for (auto rollback : targets)
                        if (auto v = ObjectDB::get_instance(rollback))
                            v->call("_refresh_api", Ref<UltralightAPI>(this), name);
                    return r;
                }
            } else
                views.erase(id);
        }
        return UltralightResult::success();
    }

  public:
    static Ref<UltralightAPI> create(const String &name) {
        if (!identifier(name))
            return {};
        static const std::set<std::string> reserved = {
            "await",     "break",  "case",     "catch",  "class",      "const",   "continue",  "debugger",
            "default",   "delete", "do",       "else",   "enum",       "export",  "extends",   "false",
            "finally",   "for",    "function", "if",     "implements", "import",  "in",        "instanceof",
            "interface", "let",    "new",      "null",   "package",    "private", "protected", "public",
            "return",    "static", "super",    "switch", "this",       "throw",   "true",      "try",
            "typeof",    "var",    "void",     "while",  "with",       "yield"};
        if (reserved.contains(name.utf8().get_data()))
            return {};
        Ref<UltralightAPI> api;
        api.instantiate();
        api->namespace_ = name;
        return api;
    }
    void forget_view(ObjectID id) {
        views.erase(id);
    }
    String get_namespace() const {
        return namespace_;
    }
    bool is_closed() const {
        return closed;
    }
    Dictionary definitions() const {
        return entries;
    }
    Ref<UltralightResult> register_function(const String &name, const Callable &function,
                                            const Ref<UltralightBindingOptions> &options = {}) {
        if (!function.is_valid())
            return UltralightResult::failure("InvalidArgument", "Invalid Callable");
        return put(name, function, false, options);
    }
    Ref<UltralightResult> register_async(const String &name, const Callable &handler,
                                         const Ref<UltralightBindingOptions> &options = {}) {
        if (!handler.is_valid())
            return UltralightResult::failure("InvalidArgument", "Invalid Callable");
        return put(name, handler, true, options);
    }
    Ref<UltralightResult> register_object(const String &name, const Variant &object,
                                          const Ref<UltralightBindingOptions> &options = {}) {
        if (object.get_type() == Variant::NIL)
            return UltralightResult::failure("InvalidArgument", "Object or container is required");
        return put(name, object, false, options);
    }
    Ref<UltralightResult> attach(Object *view) {
        if (closed)
            return UltralightResult::failure("Closed", "API registry is closed");
        if (namespace_.is_empty())
            return UltralightResult::failure("InvalidArgument",
                                             "Create the registry with UltralightAPI.create(namespace)");
        if (!view || !view->is_class("UltralightView"))
            return UltralightResult::failure("InvalidArgument", "Expected UltralightView");
        if (views.contains(ObjectID(view->get_instance_id())))
            return UltralightResult::success();
        auto id = ObjectID(view->get_instance_id());
        Ref<UltralightAPI> keep(this);
        Ref<UltralightResult> r = view->call("_attach_api", keep);
        if (r.is_valid() && r->is_ok() && ObjectDB::get_instance(id) == view && !closed)
            views.insert(id);
        return r;
    }
    void detach(Object *view) {
        if (!view || !views.contains(ObjectID(view->get_instance_id())))
            return;
        views.erase(ObjectID(view->get_instance_id()));
        view->call("_detach_api", Ref<UltralightAPI>(this));
    }
    void unregister(const String &name) {
        if (!entries.has(name))
            return;
        entries.erase(name);
        Ref<UltralightAPI> keep(this);
        auto targets = views;
        for (auto id : targets)
            if (auto view = ObjectDB::get_instance(id))
                view->call("_refresh_api", Ref<UltralightAPI>(this), name);
    }
    void close() {
        if (closed)
            return;
        Ref<UltralightAPI> keep(this);
        closed = true;
        auto targets = std::exchange(views, {});
        for (auto id : targets)
            if (auto view = ObjectDB::get_instance(id))
                view->call("_detach_api", keep);
        entries.clear();
    }

  protected:
    static void _bind_methods() {
        ClassDB::bind_static_method("UltralightAPI", D_METHOD("create", "namespace"), &UltralightAPI::create);
        ClassDB::bind_method(D_METHOD("get_namespace"), &UltralightAPI::get_namespace);
        ClassDB::bind_method(D_METHOD("is_closed"), &UltralightAPI::is_closed);
        ClassDB::bind_method(D_METHOD("register_function", "name", "function", "options"),
                             &UltralightAPI::register_function, DEFVAL(Ref<UltralightBindingOptions>()));
        ClassDB::bind_method(D_METHOD("register_async", "name", "handler", "options"), &UltralightAPI::register_async,
                             DEFVAL(Ref<UltralightBindingOptions>()));
        ClassDB::bind_method(D_METHOD("register_object", "name", "object", "options"), &UltralightAPI::register_object,
                             DEFVAL(Ref<UltralightBindingOptions>()));
        ClassDB::bind_method(D_METHOD("unregister", "name"), &UltralightAPI::unregister);
        ClassDB::bind_method(D_METHOD("attach", "view"), &UltralightAPI::attach);
        ClassDB::bind_method(D_METHOD("detach", "view"), &UltralightAPI::detach);
        ClassDB::bind_method(D_METHOD("close"), &UltralightAPI::close);
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "namespace"), "", "get_namespace");
    }
};
} // namespace gdbind
