#pragma once
#include <Ultralight/platform/Surface.h>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <cstring>
namespace gdbind {
class PackedByteArraySurface : public ultralight::Surface {
  public:
    godot::PackedByteArray data;
    uint32_t _width;
    uint32_t _height;
    void *locked_pixels = nullptr;

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
        locked_pixels = data.ptrw();
        return locked_pixels;
    }

    virtual void UnlockPixels() override { locked_pixels = nullptr; }

    bool Scroll(const ultralight::IntRect &rect, int dx, int dy) override {
        ultralight::Surface::ShiftPixels(locked_pixels, row_bytes(), rect, dx, dy);
        return false; // The display texture is updated from the dirty pixel buffer.
    }

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
