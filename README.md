# ultralight-godot

Display Web-Content in Godot. You can use html/css/JavaScript as like godot native part.

## Installation

1. download prebuilt binaray from [github release page](https://github.com/shabbywu/ultralight-godot/releases/latest)
2. unzip archive to your godot project rootdir, you will get a folder named addons.
3. launcher godot editor, add new Node `UltralightView` to display web-content in Godot.


## Features
- Render html in godot.
- Multiple platform support:
  - Windows x86_64
  - macOS arm64 (Apple Silicon)
  - Linux x86_64
- Use CSS to control styles, which is easier to use and controllable than Godot's native UI.
- JavaScript Interop with godot, just like native part of godot.
  - Execute JavaScript from csharp/gdscript.
  - Bind csharp/gdscript callback to ultralight JavaScript context.
  - Bind csharp/gdscript object inhereit from godot::Object to ultralight JavaScript context.
  - Call JavaScript function in csharp/gdscript.
- Built in docs, can check at godot editor.
- Supports transparent background, helpful for use as a UI system.
- Input
    - Mouse Input
    - Keyboard Input
    - Mouse Scroll Input
- Remote UI Inspector, aka, DevTools.

## SDK compatibility

The current source targets **Ultralight 2.0.0-beta.2**. Install the extension together with its matching SDK libraries.

**Ultralight Free edition limits View animations and repaints to 60 FPS.** Removing Godot's frame cap, disabling VSync or setting `max_render_fps` to 0 cannot remove the SDK edition limit.

## Release automation

Publishing a GitHub Release (including a prerelease) runs both build workflows
against its tag and uploads these assets:

- `ultralight-view.<tag>.gdextension.7z`: the `addons/ultralight-view` directory,
  including Debug and Release extensions and matching Ultralight SDK runtime
  libraries for Windows x86_64, Linux x86_64 and macOS arm64. Extract it into your
  Godot project root; no separate SDK download is needed.
- `mini-browser.linux.7z`, `mini-browser.win64.7z` and
  `mini-browser.macos-arm64.7z`: each contains the browser and its SDK runtime libraries.

The workflows must be included in the release tag. Packages are uploaded after
that workflow's builds and existing validation steps succeed. Packaging fails if
an expected extension, executable or SDK runtime library is missing.

To rebuild assets for an existing release, run the corresponding workflow from
Actions and set `release_tag` to its tag. This checks out that tag and replaces
assets with the same names, preserving the release description. Leave `release_tag`
empty to build packages as Actions artifacts without uploading to a release.
Uploads use the built-in `GITHUB_TOKEN`; no additional secret is required.

## JavaScript object bindings

The extension uses ulbind17 for checked JavaScript operations, retained handles,
Promise observation and traversal protection. Godot continues to own Variant
conversion, Callable trampolines, signals, View lifecycle, input and texture upload.
JavaScript objects still use the existing property enumeration when converted to
Dictionary, including enumerable prototype properties. null and undefined both
map to Nil; numbers map to double. Container conversion rejects cycles or excessive
nesting (64 levels).
Godot's current String conversion truncates embedded NUL characters at the
boundary; this existing behavior also applies to Dictionary keys. ulbind17's
standalone JSC string helpers support NUL, but Godot String does not preserve it
through this adapter.

Existing bindings keep their behavior:

```gdscript
view.bind_object("probe", probe)
```

To enable member queries for a particular binding:

```gdscript
view.bind_object("probe", probe, true)
view.execute_script("Object.keys(probe)")
view.execute_script("'label' in probe && Reflect.has(probe, 'echo')")
```

In query mode, Object.keys() enumerates Godot property metadata; methods are
queryable but not enumerable. Categories, groups, subgroups and internal entries
are excluded. Member queries do not read property values or run their getters,
and reflect current metadata. Missing members follow ordinary JavaScript lookup.
A freed object's Godot members disappear from queries and enumeration.

**C# properties require `[Export]` in query mode.** Ordinary public C# properties
remain accessible through the default compatibility mode. The query option applies
only to the explicit binding; objects returned from callbacks or nested containers
keep compatibility mode. Bindings must be installed again for each document,
normally in `on_window_object_ready`. Returned JS Callables and pending Promises
expire when the document is replaced or its View is destroyed.

The resource filesystem uses ulbind17's embedded resource overlay with Godot's
filesystem as fallback. Runtime-library staging also uses ulbind17's helper: the
extension stages the three core SDK libraries, while mini-browser additionally
stages AppCore. `COPY_ULTRALIGHT_BINARY=OFF` disables copying.

## Samples
- [Sample 1 - Load Html And Bind CSharp Callback](samples/Sample%201%20-%20Load%20Html%20And%20Bind%20CSharp%20Callback)
- [Sample 2 - Grid-based Inventory System](samples/Sample%202%20-%20Inventory%20System)
- [Sample 3 - 3D](samples/Sample%203%20-%203D)
- [Sample 4 - GPU](samples/Sample%204%20-%20GPU) — GPU/CPU comparison with Canvas/SVG animation, FPS and rendering timings.

## In Progress
- [ ] More Samples about how to use ultralight-godot as UI system.
- [ ] Tutorial...


## Screenshot

![image](https://github.com/user-attachments/assets/6216a3bb-6f5d-4d63-9aa7-102b0001135b)

https://github.com/user-attachments/assets/b193f148-a446-4772-89aa-aed81d1bd360

https://github.com/user-attachments/assets/bbc25e23-3827-4581-b3c0-ba3ac2a38baa
