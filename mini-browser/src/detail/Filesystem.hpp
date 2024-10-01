#pragma once
#include <Ultralight/platform/FileSystem.h>
#include <filesystem>
#include <ulbind17/mimetypes.hpp>
#include <ulbind17/platform/FileSystem.hpp>

#include "iostream"
#include "physfs.hpp"
#include <ulbind17/resources/cacert.h>
#include <ulbind17/resources/icudt67l.h>

namespace mini {
class PhysfsFileSystem : public ultralight::FileSystem {
  public:
    PhysfsFileSystem(std::filesystem::path rootpath) {
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
        {
            auto &icudt67l = bin2cpp::getIcudt67lDatFile();
            if (p == icudt67l.getFileName()) {
                return true;
            }
        }
        {
            auto &cacert = bin2cpp::getCacertPemFile();
            if (p == cacert.getFileName()) {
                return true;
            }
        }
        return PHYSFS_exists(p.c_str());
    }

    virtual ultralight::RefPtr<ultralight::Buffer> OpenFile(const ultralight::String &file_path) override {
        std::string p = file_path.utf8().data();
        {
            auto &icudt67l = bin2cpp::getIcudt67lDatFile();
            if (p == icudt67l.getFileName()) {
                return ultralight::Buffer::Create((void *)icudt67l.getBuffer(), icudt67l.getSize(), nullptr, nullptr);
            }
        }

        {
            auto &cacert = bin2cpp::getCacertPemFile();
            if (p == cacert.getFileName()) {
                return ultralight::Buffer::Create((void *)cacert.getBuffer(), cacert.getSize(), nullptr, nullptr);
            }
        }

        auto data = physfs::physfs_cat(p);
        return ultralight::Buffer::CreateFromCopy(data.data(), data.size());
    }
};
} // namespace mini
