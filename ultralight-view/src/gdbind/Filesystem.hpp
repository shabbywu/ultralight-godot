#pragma once
#include "inspector.h"
#include <Ultralight/platform/FileSystem.h>
#include <filesystem>
#include <godot_cpp/classes/file_access.hpp>
#include <physfs.h>
#include <ulbind17/mimetypes.hpp>
namespace gdbind {
class GodotFileSystem : public ultralight::FileSystem {
    bool owns_physfs = false, inspector_mounted = false;
    static constexpr const char *archive_name = "ultralight-godot-inspector.zip";
    static std::string inspectorPath(const ultralight::String &path) {
        auto text = path.utf8();
        std::string name(text.data(), text.length());
        return name.starts_with("inspector/") ? "__ultralight_sdk/" + name : std::string();
    }

  public:
    GodotFileSystem() {
        owns_physfs = !PHYSFS_isInit();
        if (owns_physfs && !PHYSFS_init(nullptr))
            return;
        const auto &archive = bin2cpp::getInspectorZipFile();
        inspector_mounted = PHYSFS_mountMemory(archive.getBuffer(), archive.getSize(), nullptr, archive_name,
                                               "__ultralight_sdk", 0) != 0;
    }
    ~GodotFileSystem() override {
        if (inspector_mounted && PHYSFS_isInit())
            PHYSFS_unmount(archive_name);
        if (owns_physfs && PHYSFS_isInit())
            PHYSFS_deinit();
    }
    virtual ultralight::String GetFileCharset(const ultralight::String &file_path) override {
        return "utf-8";
    }

    virtual ultralight::String GetFileMimeType(const ultralight::String &file_path) override {
        std::filesystem::path path = file_path.utf8().data();
        std::string mimetype = ulbind17::mimetypes::getType(path.extension().string().c_str());
        return ultralight::String8(mimetype.c_str(), mimetype.length());
    }

    virtual bool FileExists(const ultralight::String &file_path) override {
        auto inspector = inspectorPath(file_path);
        if (!inspector.empty() && inspector_mounted)
            return PHYSFS_exists(inspector.c_str());
        const godot::String prefix = "res://";
        return godot::FileAccess::file_exists(prefix + godot::String::utf8(file_path.utf8().data()));
    }

    virtual ultralight::RefPtr<ultralight::Buffer> OpenFile(const ultralight::String &file_path) override {
        auto inspector = inspectorPath(file_path);
        if (!inspector.empty() && inspector_mounted) {
            auto *file = PHYSFS_openRead(inspector.c_str());
            if (!file)
                return nullptr;
            auto size = PHYSFS_fileLength(file);
            if (size < 0) {
                PHYSFS_close(file);
                return nullptr;
            }
            std::vector<unsigned char> data(size);
            auto read = PHYSFS_readBytes(file, data.data(), size);
            PHYSFS_close(file);
            if (read != size)
                return nullptr;
            return ultralight::Buffer::CreateFromCopy(data.data(), data.size());
        }
        const godot::String prefix = "res://";

        godot::String target = prefix + godot::String::utf8(file_path.utf8().data());
        UtilityFunctions::print("loading ", target);
        if (!godot::FileAccess::file_exists(target)) {
            UtilityFunctions::print("file ", target, " not found");
            return nullptr;
        }
        auto data = godot::FileAccess::get_file_as_bytes(target);
        return ultralight::Buffer::CreateFromCopy(data.ptr(), data.size());
    }
};
} // namespace gdbind
