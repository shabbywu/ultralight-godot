#pragma once
#include "cast__def.hpp"
#include "ulbind17/types/jsclass__def.hpp"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
using namespace godot;

namespace gdbind {
class DBBaseClass {};

class GodotObject : public ulbind17::detail::Class<godot::Object> {
  public:
    virtual JSObjectRef makeInstance(JSContextRef ctx, void *self) override {
        auto obj = (godot::Object *)self;
        return JSObjectMake(ctx, holder->value, self);
    }

    virtual JSValueRef getProperty(JSContextRef ctx, JSObjectRef object, std::string propertyName,
                                   JSValueRef *exception) override {
        auto instance = (godot::Object *)JSObjectGetPrivate(object);
        godot::String gd_propertyName(propertyName.data());
        return ulbind17::detail::generic_cast<godot::Variant, JSValueRef>(ctx,
                                                                          std::move(instance->get(gd_propertyName)));
    }

    virtual bool setProperty(JSContextRef ctx, JSObjectRef object, std::string propertyName, JSValueRef value,
                             JSValueRef *exception) override {
        auto instance = (godot::Object *)JSObjectGetPrivate(object);
        godot::String gd_propertyName(propertyName.data());
        instance->set(gd_propertyName,
                      ulbind17::detail::generic_cast<JSValueRef, godot::Variant>(ctx, std::forward<JSValueRef>(value)));
        return true;
    }

    virtual void finalizeInstance(JSObjectRef object) override {
        auto p = (godot::Object *)JSObjectGetPrivate(object);
    }

  public:
    GodotObject(JSContextRef ctx) : Class(ctx) {};

  public:
    static std::shared_ptr<GodotObject> defindJSClass(JSContextRef ctx) {
        static std::shared_ptr<GodotObject> clazz;
        static JSClassDefinition def;
        if (clazz == nullptr) {
            clazz = std::make_shared<GodotObject>(ctx);
            memset(&def, 0, sizeof(def));
            def.className = "GodotObject";
            def.version = 0;
            def.attributes = kJSClassAttributeNone;
            def.getProperty = [](JSContextRef ctx, JSObjectRef object, JSStringRef propertyName,
                                 JSValueRef *exception) {
                auto clazz = ulbind17::detail::ClassRegistry::getIntance().findJSClass<godot::Object>();
                auto name = ulbind17::detail::String(ctx, propertyName).value();
                return clazz->getProperty(ctx, object, name, exception);
            };
            def.setProperty = [](JSContextRef ctx, JSObjectRef object, JSStringRef propertyName, JSValueRef value,
                                 JSValueRef *exception) {
                auto clazz = ulbind17::detail::ClassRegistry::getIntance().findJSClass<godot::Object>();
                auto name = ulbind17::detail::String(ctx, propertyName).value();
                return clazz->setProperty(ctx, object, name, value, exception);
            };
            def.finalize = [](JSObjectRef object) {
                auto clazz = ulbind17::detail::ClassRegistry::getIntance().findJSClass<godot::Object>();
                clazz->finalizeInstance(object);
            };
            def.initialize = [](JSContextRef ctx, JSObjectRef object) {
                auto clazz = ulbind17::detail::ClassRegistry::getIntance().findJSClass<godot::Object>();
                clazz->initializeInstance(ctx, object);
            };
            clazz->end(def);
            ulbind17::detail::ClassRegistry::getIntance().registerJSClass<godot::Object>(clazz);
        }
        return clazz;
    };
};
} // namespace gdbind
