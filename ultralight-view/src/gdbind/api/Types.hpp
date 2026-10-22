#pragma once
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <thread>
#include <ulbind17/Error.hpp>

namespace gdbind {
using namespace godot;
using String = godot::String;
class UltralightError : public RefCounted {
    GDCLASS(UltralightError, RefCounted);

  public:
    String code, message, stack, source_url;
    int64_t line = 0, column = 0;
    String get_code() const {
        return code;
    }
    String get_message() const {
        return message;
    }
    String get_stack() const {
        return stack;
    }
    String get_source_url() const {
        return source_url;
    }
    int64_t get_line() const {
        return line;
    }
    int64_t get_column() const {
        return column;
    }
    static Ref<UltralightError> make(const String &code, const String &message) {
        Ref<UltralightError> e;
        e.instantiate();
        e->code = code;
        e->message = message;
        return e;
    }
    static Ref<UltralightError> from(const std::exception &error) {
        auto result = make("ScriptException", String::utf8(error.what()));
        if (auto e = dynamic_cast<const ulbind17::Error *>(&error)) {
            result->stack = String::utf8(e->stack.data(), e->stack.size());
            result->source_url = String::utf8(e->source_url.data(), e->source_url.size());
            result->line = e->line;
            result->column = e->column;
            if (e->code == ulbind17::ErrorCode::PageGone)
                result->code = "PageGone";
            if (e->code == ulbind17::ErrorCode::CrossContext)
                result->code = "CrossContext";
            if (e->code == ulbind17::ErrorCode::WrongThread)
                result->code = "WrongThread";
            if (e->code == ulbind17::ErrorCode::TraversalLimit)
                result->code = "TraversalLimit";
            if (e->code == ulbind17::ErrorCode::Unsupported)
                result->code = "Unsupported";
            if (e->code == ulbind17::ErrorCode::TypeMismatch)
                result->code = "TypeMismatch";
            if (e->code == ulbind17::ErrorCode::InvalidArgument)
                result->code = "InvalidArgument";
            if (e->code == ulbind17::ErrorCode::Closed)
                result->code = "Closed";
            if (e->code == ulbind17::ErrorCode::Busy)
                result->code = "Busy";
            if (e->code == ulbind17::ErrorCode::HostException)
                result->code = "HostException";
        }
        return result;
    }

  protected:
    static void _bind_methods() {
#define UL_ERROR_PROP(type, name)                                                                                      \
    ClassDB::bind_method(D_METHOD("get_" #name), &UltralightError::get_##name);                                        \
    ADD_PROPERTY(PropertyInfo(type, #name), "", "get_" #name)
        UL_ERROR_PROP(Variant::STRING, code);
        UL_ERROR_PROP(Variant::STRING, message);
        UL_ERROR_PROP(Variant::STRING, stack);
        UL_ERROR_PROP(Variant::STRING, source_url);
        UL_ERROR_PROP(Variant::INT, line);
        UL_ERROR_PROP(Variant::INT, column);
#undef UL_ERROR_PROP
    }
};
class UltralightResult : public RefCounted {
    GDCLASS(UltralightResult, RefCounted);

  public:
    Variant value;
    Ref<UltralightError> error;
    bool is_ok() const {
        return error.is_null();
    }
    Variant get_value() const {
        return value;
    }
    Ref<UltralightError> get_error() const {
        return error;
    }
    static Ref<UltralightResult> success(const Variant &value = Variant()) {
        Ref<UltralightResult> r;
        r.instantiate();
        r->value = value;
        return r;
    }
    static Ref<UltralightResult> failure(const String &code, const String &message) {
        auto r = success();
        r->error = UltralightError::make(code, message);
        return r;
    }
    static Ref<UltralightResult> failure(const std::exception &error) {
        auto r = success();
        r->error = UltralightError::from(error);
        return r;
    }

  protected:
    static void _bind_methods() {
        ClassDB::bind_method(D_METHOD("is_ok"), &UltralightResult::is_ok);
        ClassDB::bind_method(D_METHOD("get_value"), &UltralightResult::get_value);
        ClassDB::bind_method(D_METHOD("get_error"), &UltralightResult::get_error);
        ADD_PROPERTY(PropertyInfo(Variant::BOOL, "ok"), "", "is_ok");
        ADD_PROPERTY(PropertyInfo(Variant::NIL, "value", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NIL_IS_VARIANT), "",
                     "get_value");
        ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "error", PROPERTY_HINT_RESOURCE_TYPE, "UltralightError"), "",
                     "get_error");
    }
};
class UltralightTask : public RefCounted {
    GDCLASS(UltralightTask, RefCounted);
    std::thread::id thread = std::this_thread::get_id();
    Ref<UltralightResult> result;

  public:
    bool is_completed() const {
        return result.is_valid();
    }
    Ref<UltralightResult> get_result() const {
        return result;
    }
    void finish(const Ref<UltralightResult> &r) {
        if (std::this_thread::get_id() != thread || result.is_valid())
            return;
        Ref<UltralightTask> keep(this);
        result = r;
        emit_signal("completed");
    }
    void resolve(const Variant &value = Variant()) {
        finish(UltralightResult::success(value));
    }
    void reject(const String &code, const String &message) {
        finish(UltralightResult::failure(code, message));
    }
    void cancel() {
        reject("Cancelled", "Operation cancelled");
    }

  protected:
    static void _bind_methods() {
        ClassDB::bind_method(D_METHOD("is_completed"), &UltralightTask::is_completed);
        ClassDB::bind_method(D_METHOD("get_result"), &UltralightTask::get_result);
        ClassDB::bind_method(D_METHOD("resolve", "value"), &UltralightTask::resolve, DEFVAL(Variant()));
        ClassDB::bind_method(D_METHOD("reject", "code", "message"), &UltralightTask::reject);
        ClassDB::bind_method(D_METHOD("cancel"), &UltralightTask::cancel);
        ADD_SIGNAL(MethodInfo("completed"));
    }
};
class UltralightFrameEvent : public RefCounted {
    GDCLASS(UltralightFrameEvent, RefCounted);

  public:
    int64_t navigation_id = 0, frame_id = 0;
    bool main_frame = true;
    String url;
    int64_t get_navigation_id() const {
        return navigation_id;
    }
    int64_t get_frame_id() const {
        return frame_id;
    }
    bool is_main_frame() const {
        return main_frame;
    }
    String get_url() const {
        return url;
    }

  protected:
    static void _bind_methods() {
        ClassDB::bind_method(D_METHOD("get_navigation_id"), &UltralightFrameEvent::get_navigation_id);
        ClassDB::bind_method(D_METHOD("get_frame_id"), &UltralightFrameEvent::get_frame_id);
        ClassDB::bind_method(D_METHOD("is_main_frame"), &UltralightFrameEvent::is_main_frame);
        ClassDB::bind_method(D_METHOD("get_url"), &UltralightFrameEvent::get_url);
        ADD_PROPERTY(PropertyInfo(Variant::INT, "navigation_id"), "", "get_navigation_id");
        ADD_PROPERTY(PropertyInfo(Variant::INT, "frame_id"), "", "get_frame_id");
        ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_main_frame"), "", "is_main_frame");
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "url"), "", "get_url");
    }
};
} // namespace gdbind
