#pragma once
#include <Ultralight/platform/Surface.h>
#include <godot_cpp/variant/packed_byte_array.hpp>
namespace gdbind {
class PackedByteArraySurface : public ultralight::Surface {
  public:
    godot::PackedByteArray data;
    uint32_t _width;
    uint32_t _height;

  public:
    PackedByteArraySurface(uint32_t width, uint32_t height) : _width(width), _height(height) {
        data.resize(width * height * 4);
        memset(data.ptrw(), 64, _width * _height * 4);
    }

    ~PackedByteArraySurface() {
        data.clear();
    }

  public:
    virtual uint32_t width() const override {
        return _width;
    }

    virtual uint32_t height() const override {
        return _height;
    }

    virtual uint32_t row_bytes() const override {
        return _width * 4;
    }

    virtual size_t size() const override {
        return _width * _height * 4;
    }

    virtual void *LockPixels() override {
        return data.ptrw();
    }

    virtual void UnlockPixels() override {};

    virtual void Resize(uint32_t width, uint32_t height) override {
        _width = width;
        _height = height;
        data.resize(_width * _height * 4);
        memset(data.ptrw(), 64, _width * _height * 4);
    }
};

class PackedByteArraySurfaceFactory : public ultralight::SurfaceFactory {
    ///
    /// Create a native Surface with a certain width and height (in pixels).
    ///
    virtual ultralight::Surface *CreateSurface(uint32_t width, uint32_t height) {
        return new PackedByteArraySurface(width, height);
    }

    ///
    /// Destroy a native Surface previously created by CreateSurface().
    ///
    virtual void DestroySurface(ultralight::Surface *surface) {
        delete surface;
    }
};
} // namespace gdbind
