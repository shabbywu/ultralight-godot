# Sample 4 - GPU

A dedicated desktop rendering test bench. The large webpage preview sits beside native Godot performance metrics and clickable controls. Sample 3 remains the original 3D demo.

## Run locally

Use the Godot .NET editor and a .NET SDK compatible with [GPU.csproj](src/GPU.csproj). Set `ULTRALIGHT_LIBRARY` to the extension library built for your platform, with the Ultralight SDK libraries beside it. From the repository root, build and install the extension:

```sh
cmake --build build/default --target ultralight-view -j4
python3 "samples/Sample 4 - GPU/install_local.py" --library "$ULTRALIGHT_LIBRARY"
```

Open **[src/project.godot](src/project.godot)** and run the main scene. The installer rebuilds its C# assembly. Restart an already open Godot editor after installing to load the updated native library.

## Controls and metrics

All controls have buttons; keyboard shortcuts work when the game window has focus:

| Control | Shortcut | Options |
| --- | --- | --- |
| Rendering backend | G | 显卡 GPU / 软件 CPU |
| Animation workload | B | SVG vectors / Canvas |
| Object count | L | 120 / 240 / 480 / 960 / 1440 (cycles back to 120) |
| Godot frame limit | V | Godot uncapped / 60 FPS with VSync |

**帧率说明：Ultralight 2.0 免费版不支持解锁网页帧率，最高为 60 FPS。样例的“解锁”按钮仅解除 Godot 的帧率限制，不能解除 SDK 的 60 FPS 上限。**

The default is **GPU + SVG + 480 objects + Godot uncapped**. The public SDK 2.0 package is the Free edition, whose Views are capped at **60 FPS** even when Godot's cap and VSync are disabled. The panel reports the SDK edition limit and the webpage's own animation FPS separately from Godot FPS. Setting `UltralightView.max_render_fps` to 0 removes the per-View cap; the edition limit still applies. See [Ultralight frame limits](https://ultralig.ht/docs/2.0/updating-and-rendering#slowing-down-or-pausing-views).

The backend badge reports the actual backend; unavailable RenderingDevice falls back to CPU. Keep workload and object count identical when switching backends. The HTML uses no network resources, and the internal resolution stays **1024×768** as the window resizes. Click the webpage's counter to verify input forwarding.

Godot provides FPS and frame duration, with a graph of the last 120 frames. The overlay is also appended to the webpage surface using a SubViewport. **网页绘制 CPU 耗时** averages Ultralight RefreshDisplay/Render and command preparation; it excludes completed GPU execution and JavaScript outside those calls. Upload throughput measures GPU resource bitmaps or CPU whole-page pixels according to the active backend. Measurements reset when changing backend, workload or object count.

SVG exercises curved paths, strokes, transforms and transparent overlap. Canvas adds gradients, shadows, clipping and alpha compositing; SDK resource bitmap drawing/uploads can limit it even with the GPU backend. Removing the frame cap requests disabled VSync; platform presentation can still affect FPS.

The `Gui` node exports `UseGpu`, `CanvasObjects`, `UseSvg`, `LimitFrameRate` and `EnableInspector`. Inspector is disabled by default; enable it when debugging the webpage.

## Verification

```sh
python3 tests/gpu/run.py --library "$ULTRALIGHT_LIBRARY" --task sample4
```

This stages a fresh project and checks GPU rendering, the native FPS overlay, scaled webpage input/JS callbacks, buttons and G/B/L/V shortcuts, fixed resolution during window resizing, scene reload and normal exit.

The runner uses `godot` from PATH by default. Use `--godot` to select another executable and `--driver` to select a supported RenderingDevice driver. See [GPU tests](../../tests/gpu/README.md) for regression checks and sustained CPU/GPU benchmarks.
