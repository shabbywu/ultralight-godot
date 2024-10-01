#pragma once
#include <format>
#include <iostream>
#include <physfs.h>
#include <sstream>
#include <string>
#include <vector>

namespace physfs {
class PhysfsRuntimeError : std::runtime_error {
    typedef std::runtime_error super;

  public:
    PhysfsRuntimeError(std::string message) : std::runtime_error(message) {};
};

class PhysfsNotFound : public std::exception {
    std::string buf;

  public:
    PhysfsNotFound(std::string message) {
        buf = "Not Found: '" + message + "'";
    };
    const char *what() const throw() {
        return buf.c_str();
    }
};

class PhysfsUninitialedError : public std::exception {
  public:
    const char *what() const throw() {
        return "PHYSFS is not initialized, please call init() first";
    }
};

inline void physfs_init(std::string rootpath) {
    if (!PHYSFS_init(rootpath.c_str())) {
        throw PhysfsRuntimeError(std::format("PHYSFS init failure. Reason: [{}]", PHYSFS_getLastError()));
    }
}

inline void physfs_deinit() {
    if (!PHYSFS_isInit()) {
        throw PhysfsUninitialedError();
    }

    if (!PHYSFS_deinit()) {
        throw PhysfsRuntimeError(std::format("PHYSFS deinit failure. Reason: [{}]", PHYSFS_getLastError()));
    }
}

inline void physfs_mount(std::string source, std::string mountPoint, bool appendToPath) {
    if (!PHYSFS_isInit()) {
        throw PhysfsUninitialedError();
    }

    if (!PHYSFS_mount(source.c_str(), mountPoint.c_str(), appendToPath)) {
        throw PhysfsRuntimeError(std::format("PHYSFS mount failure. Reason: [{}]", PHYSFS_getLastError()));
    }
}

inline void physfs_unmount(std::string oldSource) {
    if (!PHYSFS_isInit()) {
        throw PhysfsUninitialedError();
    }

    if (!PHYSFS_unmount(oldSource.c_str())) {
        throw PhysfsRuntimeError(std::format("PHYSFS unmount failure. Reason: [{}]", PHYSFS_getLastError()));
    }
}

inline std::vector<std::string> physfs_ls(std::string dir) {
    if (!PHYSFS_isInit()) {
        throw PhysfsRuntimeError("PHYSFS is not initialized, please call init() first");
    }

    char **rc;
    rc = PHYSFS_enumerateFiles(dir.c_str());

    std::vector<std::string> result;
    char **i;
    for (i = rc; *i != NULL; i++) {
        result.push_back(std::string(*i));
    }
    return result;
}

inline PHYSFS_Stat physfs_stat(std::string file) {
    if (!PHYSFS_isInit()) {
        throw PhysfsUninitialedError();
    }

    PHYSFS_Stat stat;

    if (!PHYSFS_stat(file.c_str(), &stat)) {
        auto error_code = PHYSFS_getLastErrorCode();
        if (error_code == PHYSFS_ERR_NOT_FOUND) {
            throw PhysfsNotFound(file);
        }
        throw PhysfsRuntimeError(std::format("failed to stat. Reason [{}]", PHYSFS_getLastError()));
    }
    return stat;
}

inline std::string physfs_cat(std::string file) {
    if (!PHYSFS_isInit()) {
        throw PhysfsUninitialedError();
    }

    PHYSFS_File *f;
    f = PHYSFS_openRead(file.c_str());
    if (f == NULL) {
        auto error_code = PHYSFS_getLastErrorCode();
        if (error_code == PHYSFS_ERR_NOT_FOUND) {
            throw PhysfsNotFound(file);
        }
        throw PhysfsRuntimeError(std::format("failed to open. Reason: [{}]", PHYSFS_getLastError()));
    }

    std::stringstream output;
    while (1) {
        char buffer[1024];
        PHYSFS_sint64 rc;
        PHYSFS_sint64 i;
        rc = PHYSFS_readBytes(f, buffer, sizeof(buffer));
        if (rc == -1) {
            throw PhysfsRuntimeError(std::format("Error condition in reading. Reason: [{}]", PHYSFS_getLastError()));
        }

        output.write(buffer, rc);
        if (rc < sizeof(buffer)) {
            if (!PHYSFS_eof(f)) {
                PHYSFS_close(f);
                throw PhysfsRuntimeError(
                    std::format("Error condition in reading. Reason: [{}]", PHYSFS_getLastError()));
            }
            PHYSFS_close(f);
            break;
        }
    }
    return output.str();
}

} // namespace physfs
