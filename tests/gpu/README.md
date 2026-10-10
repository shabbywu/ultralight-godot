# GPU and JavaScript bridge checks

Run from the repository root with Python 3 and a built extension. Set `ULTRALIGHT_LIBRARY` to the extension library; its matching SDK libraries must be beside it. The runner uses `godot` from PATH, or the executable supplied with `--godot`.

GPU checks require a graphics device with Godot RenderingDevice support. The `interop` and `sample4` tasks also require Godot .NET and a compatible .NET SDK. Compatibility and headless modes check CPU fallback.

The C# projects target `Godot.NET.Sdk/4.4.1` and .NET 8.

## Regression checks

```sh
python3 tests/gpu/run.py --library "$ULTRALIGHT_LIBRARY"
python3 tests/gpu/run.py --library "$ULTRALIGHT_LIBRARY" --backend compatibility
python3 tests/gpu/run.py --library "$ULTRALIGHT_LIBRARY" --backend headless
python3 tests/gpu/run.py --library "$ULTRALIGHT_LIBRARY" --task interop
python3 tests/gpu/run.py --library "$ULTRALIGHT_LIBRARY" --task sample4
```

| Task | Coverage |
| --- | --- |
| `validate` (default) | Backend selection, input, transparency and color pixels, CSS filters, SVG paths, resize/recreation, GDScript bindings and document expiry |
| `interop` | C# objects and delegates, JS Callables, Promise/async results and rejection, GDScript default/bound arguments, callback call errors, navigation cancellation |
| `sample4` | Sample scene input, controls, backend switching, object presets, frame-limit controls and scene reload; requires `--backend gpu` |

Fixtures are in [fixtures](fixtures); the runner also uses Sample 4's animation page and font. It stages an isolated project, imports assets and requires a test completion marker with no engine errors. Use `--driver` to choose a rendering driver and `--output` to retain the staged project and logs in a new or empty directory. The runner prints the Godot `user://` location containing screenshots and benchmark JSON.

Pixel assertions cover selected cases rather than complete CPU/GPU visual equivalence. Shader initialization and actual draw coverage are separate: not every page exercises all seven SDK programs.

## Benchmarks

```sh
python3 tests/gpu/run.py --library "$ULTRALIGHT_LIBRARY" --task benchmark --workload canvas --backend gpu
python3 tests/gpu/run.py --library "$ULTRALIGHT_LIBRARY" --task benchmark --workload canvas --backend cpu
```

Run the two backends sequentially with the same workload, object count, duration and frame limit. Each run measures at least five minutes after warmup; the default is 480 objects. Options:

- `--objects`: 120, 240, 480, 960 or 1440.
- `--workload`: `canvas` (default) or `svg`.
- `--duration`: measured seconds, at least 300.
- `--fps-limit`: Godot frame cap; 0 (default) also requests disabled VSync.

The benchmark drives deterministic animation steps and checks that the selected backend drew. JSON records loop throughput, median/p95 step times and rendering counters, including test-harness overhead. The `summary.fps` field measures harness steps per second, not presented View frames. GPU batches can include offscreen Canvas drawing, while CPU counters track page uploads.

Ultralight Free edition still caps View animations/repaints at 60 FPS when Godot's cap is removed. Compare Canvas and SVG separately, and keep the window visible. Results describe the isolated View workload; they do not establish Sample 4 scene performance or performance on another device. Keep machine-specific measurements and logs outside tracked source.
