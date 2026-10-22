# ultralight-godot

**Build Godot interfaces with HTML, CSS, and JavaScript.**

English | [简体中文](README.zh-CN.md)

ultralight-godot integrates [Ultralight](https://ultralig.ht/) into Godot through a
GDExtension. Display web content with `UltralightView`, connect it to GDScript or
C#, and use transparent web interfaces in 2D and 3D scenes.

![HTML interface rendered in Godot](https://github.com/user-attachments/assets/6216a3bb-6f5d-4d63-9aa7-102b0001135b)

[Showcase](https://shabbywu.github.io/ultralight-godot/en/) · [API reference](https://shabbywu.github.io/ultralight-godot/en/api/) · [Install](#installation) · [Examples](#examples) · [JavaScript bindings](#javascript-bindings) · [Release automation](#release-automation)

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
           ├── csharp/
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
| [Ashen Reliquary](samples/Sample%202%20-%20Inventory%20System) | An inventory interface built with web technologies |
| [3D interface](samples/Sample%203%20-%203D) | Integrating `UltralightView` into a 3D scene |
| [GPU / CPU comparison](samples/Sample%204%20-%20GPU) | Canvas/SVG animation, FPS, and rendering timings |

Looping WebP recordings from the running Godot samples. Follow the sample links
above for setup instructions.

### 1 · HTML and C# callbacks

![Button hover effects and JavaScript calls to C#](docs/media/sample-1.webp)

### 2 · Ashen Reliquary inventory

![Equipment, sockets, shortcuts and backend item generation](docs/media/sample-2.webp)

### 3 · Web interfaces in 3D

![Scrolling web content and physics objects in a 3D scene](docs/media/sample-3.webp)

### 4 · GPU / CPU comparison

![Switching rendering backends and SVG / Canvas animation](docs/media/sample-4.webp)

## JavaScript bindings

Register once; bindings are restored after navigation under your chosen namespace.

```gdscript
var api := UltralightAPI.create("game")

func _ready():
    api.register_function("save", save_game)
    api.register_object("inventory", inventory)
    api.attach($WebView)
    var loading = $WebView.load_url("file:///ui/index.html")
    if not loading.is_completed():
        await loading.completed
    var result = loading.get_result()
    if not result.ok:
        push_error(result.error.message)
```

```javascript
await game.save();
game.inventory.items.push(item);
```

The new API provides main-document and frame signals, explicit Result/Task values,
C# async/await, and bidirectional shared containers. Containers are wrapped lazily;
writes update their source. Use `evaluate_copy()` or `snapshot()` for independent
copies. `UltralightBindingOptions` controls root members, nested writes and copying.

See the [usage and migration guide](docs/api-guide.md) for API contracts and
changes from earlier releases. Use the matching tag's documentation for an older release.
C# helpers target Godot .NET 4.4.1 / .NET 8 or a compatible environment; compile the
package's `csharp/*.cs` sources in the project.

Add `UltralightInspector` and set `target_view` for embedded developer tools, or use
the [split-view scene](samples/inspector.tscn). Text clipboard, CSS cursors and Control
focus use Godot integration without AppCore. Full IME preedit is not implemented.
The extension uses Ultralight, UltralightCore and WebCore; the standalone mini-browser
also uses AppCore.

## Showcase and API reference

The [showcase](https://shabbywu.github.io/ultralight-godot/en/) includes recordings
of all four samples. The [API reference](https://shabbywu.github.io/ultralight-godot/en/api/)
covers plugin properties, methods and signals, generated from the Godot XML docs
in `ultralight-view/doc_classes/`.
Chinese descriptions and search summaries use `site/locales/api.zh-CN.json`;
signatures and defaults share the XML source, with translation completeness checked during the build.

Preview locally:

```sh
python3 scripts/build_site.py
python3 -m http.server 8000 --directory dist
```

The `Showcase Pages` workflow builds pull requests and deploys `main`.
Set the repository's **Settings → Pages → Source** to **GitHub Actions**.

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
