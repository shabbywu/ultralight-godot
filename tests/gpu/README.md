# GPU regression tests and benchmarks

Run commands from the repository root with Python 3 and a built Ultralight extension. Set `ULTRALIGHT_LIBRARY` to the extension library for your platform; its matching Ultralight SDK libraries must be in the same directory. The runner uses `godot` from PATH. Select another executable with `--godot` and override the rendering driver with `--driver` if needed.

The Sample 4 test requires Godot .NET and a compatible .NET SDK. RenderingDevice tests require an actual graphics device. Headless validation exercises CPU fallback.

## Regression checks

```sh
python3 tests/gpu/run.py --library "$ULTRALIGHT_LIBRARY"
python3 tests/gpu/run.py --library "$ULTRALIGHT_LIBRARY" --backend compatibility
python3 tests/gpu/run.py --library "$ULTRALIGHT_LIBRARY" --backend headless
python3 tests/gpu/run.py --library "$ULTRALIGHT_LIBRARY" --task sample4
```

`validate.gd` checks backend selection, statistics, input/JS callbacks, selected color and transparency pixels, repeated resize, independent View materials/UVs, recreation and shutdown. Pixel checks cover specific cases, not complete visual equivalence between the backends. `sample4.gd` checks preview input, native controls, backend switching, material release and scene reload. It waits 10 seconds after each button or keyboard action before proceeding; its runner timeout is 5 minutes.

Fixtures are in [fixtures](fixtures). The runner also reuses Sample 4's animation page and font. It stages a separate project with the supplied build, imports its assets and returns a failure for engine errors or failed assertions. Use `--output` to choose where the staged project and logs are saved. Screenshots and benchmark JSON are written to Godot's `user://` directory, printed at exit.

## Sustained benchmarks

Each backend measures at least **5 minutes after warmup**, with **480 objects** by default. CPU and GPU must be run sequentially with the same workload, object count, duration and frame limit. The default frame limit is zero, disabling the engine cap and requesting disabled VSync.

```sh
python3 tests/gpu/run.py --library "$ULTRALIGHT_LIBRARY" --task benchmark --workload svg
python3 tests/gpu/run.py --library "$ULTRALIGHT_LIBRARY" --task benchmark --workload svg --backend cpu
```

Use `--objects 960` for a larger workload, `--duration 600` for a longer measurement, `--workload canvas` for Canvas drawing, or `--fps-limit 60` for a capped run. The runner extends its timeout for the requested duration and the benchmark reports progress every 30 seconds.

The benchmark advances deterministic animation frames and verifies that drawing occurred on the selected backend. Results include measured duration, frame count, throughput, median/p95 frame times and resource-upload counters. This measures an isolated View, including test-harness overhead; it does not measure Sample 4's complete scene or establish performance on other devices. Compare SVG and Canvas separately, and keep the test window visible during performance runs.
