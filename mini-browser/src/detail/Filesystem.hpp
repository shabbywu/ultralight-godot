#pragma once
#include <Ultralight/platform/FileSystem.h>
#include <filesystem>
#include <ulbind17/mimetypes.hpp>
#include <ulbind17/platform/FileSystem.hpp>

#include "iostream"
#include "physfs.hpp"
#include <ulbind17/resources/embedded/SDKResources.hpp>

namespace mini {
class PhysfsFileSystem : public ultralight::FileSystem {
    ulbind17::resources::embedded::SDKResources resources;
  public:
    PhysfsFileSystem(std::filesystem::path rootpath)
        : resources(ultralight::Platform::instance().config().resource_path_prefix.utf8().data()) {
        physfs::physfs_init(rootpath.string());
    }

  public:
    virtual ultralight::String GetFileMimeType(const ultralight::String &file_path) override {
        std::filesystem::path path = file_path.utf8().data();
        std::string mimetype = ulbind17::mimetypes::getType(path.extension().string().c_str());
        return ultralight::String8(mimetype.c_str(), mimetype.length());
    }

    virtual ultralight::String GetFileCharset(const ultralight::String &file_path) override {
        return "utf-8";
    }

    virtual bool FileExists(const ultralight::String &file_path) override {
        std::string p = file_path.utf8().data();
        if (resources.FileExists(file_path))
            return true;
        return PHYSFS_exists(p.c_str());
    }

    virtual ultralight::RefPtr<ultralight::Buffer> OpenFile(const ultralight::String &file_path) override {
        std::string p = file_path.utf8().data();
        auto resource = resources.OpenFile(file_path);
        if (resource.get()) return resource;
        try {
            auto data = physfs::physfs_cat(p);
            return ultralight::Buffer::CreateFromCopy(data.data(), data.size());
        } catch (const std::exception &error) {
            std::cerr << "OpenFile: " << error.what() << '\n';
            return nullptr;
        }
    }
};
} // namespace mini
