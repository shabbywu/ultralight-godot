#pragma once
#include <Ultralight/Ultralight.h>
#include <godot_cpp/variant/utility_functions.hpp>
namespace gdbind {
struct DownloadListener : public ultralight::DownloadListener {
    ultralight::DownloadId downloadId = 0;
    ///
    /// Called when the View wants to generate a unique download id.
    ///
    /// You should generally return an integer (starting at 0) that is incremented with each call
    /// to this callback.
    ///
    virtual ultralight::DownloadId NextDownloadId(ultralight::View *caller) {
        return downloadId++;
    }

    ///
    /// Called when the View wants to start downloading a resource from the network.
    ///
    /// You should return true to allow the download, or false to block the download.
    ///
    virtual bool OnRequestDownload(ultralight::View *caller, ultralight::DownloadId id, const ultralight::String &url) {
        return true;
    }

    ///
    /// Called when the View begins downloading a resource from the network.
    ///
    /// The View will not actually write any data to disk, you should open a file for writing
    /// yourself and handle the OnReceiveDataForDownload callback below.
    ///
    virtual void OnBeginDownload(ultralight::View *caller, ultralight::DownloadId id, const ultralight::String &url,
                                 const ultralight::String &filename, int64_t expected_content_length) {
    }

    ///
    /// Called when the View receives data for a certain download from the network.
    ///
    /// This may be called multiple times for each active download as data is streamed in.
    ///
    /// You should write the data to the associated file in this callback.
    ///
    virtual void OnReceiveDataForDownload(ultralight::View *caller, ultralight::DownloadId id,
                                          ultralight::RefPtr<ultralight::Buffer> data) {
    }

    ///
    /// Called when the View finishes downloading a resource from the network.
    ///
    /// You should close the associated file in this callback.
    ///
    virtual void OnFinishDownload(ultralight::View *caller, ultralight::DownloadId id) {
    }

    ///
    /// Called when the View fails downloading a resource from the network.
    ///
    /// You should close the associated file and delete it from disk in this callback.
    ///
    virtual void OnFailDownload(ultralight::View *caller, ultralight::DownloadId id) {
    }
};
} // namespace gdbind
