#pragma once
#include <Ultralight/platform/FileSystem.h>
#include <filesystem>
#include <godot_cpp/classes/file_access.hpp>
#include <ulbind17/mimetypes.hpp>
namespace gdbind {
class GodotFileSystem : public ultralight::FileSystem {
  public:
    virtual ultralight::String GetFileCharset(const ultralight::String &file_path) override {
        return "utf-8";
    }

    virtual ultralight::String GetFileMimeType(const ultralight::String &file_path) override {
        std::filesystem::path path = file_path.utf8().data();
        std::string mimetype = ulbind17::mimetypes::getType(path.extension().string().c_str());
        return ultralight::String8(mimetype.c_str(), mimetype.length());
    }

    virtual bool FileExists(const ultralight::String &file_path) override {
        static godot::String prefix = "res://";
        return godot::FileAccess::file_exists(prefix + file_path.utf8().data());
    }

    virtual ultralight::RefPtr<ultralight::Buffer> OpenFile(const ultralight::String &file_path) override {
        static godot::String prefix = "res://";
        auto data = godot::FileAccess::get_file_as_bytes(prefix + file_path.utf8().data());
        return ultralight::Buffer::CreateFromCopy(data.ptr(), data.size());
    }
};
} // namespace gdbind
