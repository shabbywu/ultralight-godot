# ultralight-godot

**Build Godot interfaces with HTML, CSS, and JavaScript.**

English | [简体中文](README.zh-CN.md)

ultralight-godot integrates [Ultralight](https://ultralig.ht/) into Godot through a
GDExtension. Display web content with `UltralightView`, connect it to GDScript or
C#, and use transparent web interfaces in 2D and 3D scenes.

![HTML interface rendered in Godot](https://github.com/user-attachments/assets/6216a3bb-6f5d-4d63-9aa7-102b0001135b)

[Install](#installation) · [Examples](#examples) · [JavaScript bindings](#javascript-bindings) · [Release automation](#release-automation)

## Features

- Render HTML and style interfaces with CSS, including transparent backgrounds.
- Execute JavaScript and call JavaScript functions from GDScript or C#.
- Expose Godot objects and callbacks to JavaScript.
- Handle mouse movement, clicks, scrolling, and keyboard input.
- Inspect web content with the included mini-browser remote inspector.
- Read class documentation directly in the Godot editor.

## Requirements

| Component | Supported version / platform |
| --- | --- |
| Godot | 4.3 or later (extension compatibility minimum) |
| Ultralight SDK | Current source requires **2.0.0-beta.2** |
| Windows | x86_64 |
| Linux | x86_64 |
| macOS | arm64 (Apple Silicon) |

Release packages produced by the current workflows include matching SDK runtime
libraries. Older releases may target a different SDK; follow their release notes
and keep the extension and SDK versions matched.

**Ultralight Free edition limits View animations and repaints to 60 FPS.**
Disabling Godot's frame cap or VSync, or setting `max_render_fps` to `0`, does not
remove this SDK edition limit.

## Installation

1. Open the [Releases page](https://github.com/shabbywu/ultralight-godot/releases).
2. Download `ultralight-view.<tag>.gdextension.7z` for your chosen release.
3. Extract the archive into your Godot project root. For packages produced by the
   current workflows, the layout is:

   ```text
   your-project/
   ├── project.godot
   └── addons/
       └── ultralight-view/
           ├── ultralight-view.gdextension
           └── bin/
               ├── win64/
               ├── x11/
               └── macos-arm64/
   ```

4. Open the project in Godot and add an `UltralightView` node.
5. Follow an [example](#examples) to load content and connect it to your game.

The combined package contains Debug and Release extensions and the matching
Ultralight runtime libraries for all three platforms. No separate SDK download is
needed to use it. When upgrading, replace the previous `addons/ultralight-view`
directory to avoid mixing versions.

For the remote inspector, download the `mini-browser` archive for your platform
from the same release. Each browser package includes its SDK runtime libraries.

## Examples

| Example | What it demonstrates |
| --- | --- |
| [Load HTML and bind a C# callback](samples/Sample%201%20-%20Load%20Html%20And%20Bind%20CSharp%20Callback) | Loading web content and calling C# from JavaScript |
| [Grid inventory](samples/Sample%202%20-%20Inventory%20System) | An inventory interface built with web technologies |
| [3D interface](samples/Sample%203%20-%203D) | Integrating `UltralightView` into a 3D scene |
| [GPU / CPU comparison](samples/Sample%204%20-%20GPU) | Canvas/SVG animation, FPS, and rendering timings |

Demo videos:

https://github.com/user-attachments/assets/b193f148-a446-4772-89aa-aed81d1bd360

https://github.com/user-attachments/assets/bbc25e23-3827-4581-b3c0-ba3ac2a38baa

## JavaScript bindings

Expose a Godot object to JavaScript with the default compatibility mode:

```gdscript
view.bind_object("probe", probe)
```

Enable member queries for an individual binding with the third argument:

```gdscript
view.bind_object("probe", probe, true)
view.execute_script("Object.keys(probe)")
view.execute_script("'label' in probe && Reflect.has(probe, 'echo')")
```

In query mode:

- `Object.keys()` enumerates Godot property metadata. Methods can be queried but
  are not enumerable.
- Categories, groups, subgroups, and internal entries are excluded.
- Member queries use current metadata without reading property values or running
  getters. Missing members follow ordinary JavaScript lookup.
- A freed object's Godot members disappear from queries and enumeration.
- **C# properties require `[Export]`.** Ordinary public C# properties remain
  accessible through the default compatibility mode.

The query option applies only to the explicit binding. Objects returned from
callbacks or nested containers keep compatibility mode. Install bindings again
for each document, normally in `on_window_object_ready`. Returned JavaScript
Callables and pending Promises expire when the document is replaced or its View
is destroyed.

### Conversion and implementation details

The extension uses ulbind17 for checked JavaScript operations, retained handles,
Promise observation, and traversal protection. Godot handles Variant conversion,
Callable trampolines, signals, View lifecycle, input, and texture upload.

- JavaScript objects converted to `Dictionary` include enumerable prototype
  properties.
- `null` and `undefined` map to Nil; numbers map to double.
- Container conversion rejects cycles and nesting deeper than 64 levels.
- Godot String conversion truncates embedded NUL characters, including in
  Dictionary keys. ulbind17's standalone JSC string helpers preserve NUL, but this
  adapter does not.

The resource filesystem uses ulbind17's embedded resource overlay with Godot's
filesystem as fallback. Runtime staging copies the three core SDK libraries
(`Ultralight`, `UltralightCore`, and `WebCore`) beside the extension; mini-browser
also includes `AppCore`. Set `COPY_ULTRALIGHT_BINARY=OFF` when building from source
to disable copying.

## Release automation

Publishing a GitHub Release, including a prerelease, starts two independent
workflows against the release tag:

| Workflow | Uploaded assets |
| --- | --- |
| [Build GDExtension](.github/workflows/build-gdextension.yaml) | `ultralight-view.<tag>.gdextension.7z` — extensions and SDK runtime libraries for all supported platforms |
| [Build Mini Browser](.github/workflows/build-mini-browser.yaml) | `mini-browser.linux.7z`, `mini-browser.win64.7z`, `mini-browser.macos-arm64.7z` |

1. Commit the workflow changes and include them in the version tag.
2. Create a release for that tag and select **Publish release**. Saving a draft
   does not trigger the builds.
3. Each workflow builds Windows x86_64, Linux x86_64, and macOS arm64.
4. After its builds and existing validation steps succeed, each workflow packages
   and uploads its own assets to that release. A failure in one workflow does not
   block the other workflow's upload.

Packaging fails if an expected extension, executable, or SDK runtime library is
missing. Uploads use the built-in `GITHUB_TOKEN`; no additional secret is required.

To rebuild an existing release, manually run the relevant workflow from Actions
and set `release_tag` to its tag. The workflow checks out that tag and replaces
assets with the same names, preserving the release description. Leave
`release_tag` empty to build downloadable Actions artifacts without uploading to
a release.
