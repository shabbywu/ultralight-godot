#pragma once
#include <Ultralight/Ultralight.h>
#include <godot_cpp/variant/utility_functions.hpp>
namespace gdbind {
struct NetworkListener : public ultralight::NetworkListener {
    ///
    /// Called when the View is about to begin a network request.
    ///
    /// You can use this to block or modify network requests before they are sent.
    ///
    /// Return true to allow the request, return false to block it.
    ///
    virtual bool OnNetworkRequest(ultralight::View *caller, ultralight::NetworkRequest &request) {
        godot::String url(request.url().utf8().data());
        return true;
    }
};
} // namespace gdbind
