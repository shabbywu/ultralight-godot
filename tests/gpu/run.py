#!/usr/bin/env python3
"""Stage a disposable Godot project and run real-device/fallback validation."""
import argparse
from pathlib import Path
import platform
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def run(command, log, timeout=120, success_marker=None):
    timed_out = False
    with log.open('w', encoding='utf-8') as output:
        try:
            result = subprocess.run(command, stdout=output, stderr=subprocess.STDOUT, timeout=timeout)
        except subprocess.TimeoutExpired:
            timed_out = True
    text = re.sub(r'\x1b\[[0-9;]*m', '', log.read_text(encoding='utf-8', errors='replace'))
    print(text)
    if timed_out:
        raise SystemExit(f'Timed out after {timeout}s; log: {log}')
    if result.returncode or re.search(r'(?:SCRIPT )?ERROR:', text):
        raise SystemExit(f'Failed ({result.returncode}); log: {log}')
    if success_marker and not any(line.startswith(success_marker) for line in text.splitlines()):
        raise SystemExit(f'Test did not complete ({success_marker.strip()}); log: {log}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--godot', default='godot', help='Godot executable name on PATH or executable path (.NET build for interop/sample4)')
    parser.add_argument('--library', required=True, type=Path, help='Built ultralight-view shared library; SDK binaries must be beside it')
    parser.add_argument('--task', choices=['validate', 'interop', 'sample4', 'benchmark'], default='validate')
    parser.add_argument('--backend', choices=['gpu', 'cpu', 'compatibility', 'headless'], default='gpu')
    parser.add_argument('--driver', help='Override the RenderingDevice driver; otherwise use the Godot default')
    parser.add_argument('--objects', type=int, default=480, choices=[120,240,480,960,1440], help='Animated objects for the benchmark')
    parser.add_argument('--workload', choices=['canvas','svg'], default='canvas')
    parser.add_argument('--duration', type=int, default=300, help='Measured benchmark seconds after warmup (minimum 300)')
    parser.add_argument('--fps-limit', type=int, default=0, help='Benchmark frame cap; 0 disables the cap and VSync')
    parser.add_argument('--output', type=Path, help='Keep staged project and logs at this path')
    args = parser.parse_args()
    if args.duration < 300:
        parser.error('--duration must be at least 300 seconds')
    if args.fps_limit < 0:
        parser.error('--fps-limit cannot be negative')
    if args.backend == 'cpu' and args.task != 'benchmark':
        parser.error('--backend cpu is for the isolated benchmark; use compatibility/headless to validate fallback')
    if args.task == 'benchmark' and args.backend not in ['gpu', 'cpu']:
        parser.error('benchmark requires --backend gpu or cpu')
    if args.task == 'sample4' and args.backend != 'gpu':
        parser.error('sample4 verification requires --backend gpu')
    godot = shutil.which(args.godot)
    if godot is None:
        parser.error('Godot executable not found; set --godot to its executable name or path')
    dotnet = shutil.which('dotnet') if args.task in ['sample4', 'interop'] else None
    if args.task in ['sample4', 'interop'] and dotnet is None:
        parser.error('dotnet executable not found; install a .NET SDK for interop/sample4')
    library = args.library.resolve()
    if not library.is_file():
        parser.error('--library must name a built extension library')
    stage = (args.output or Path(tempfile.mkdtemp(prefix='ultralight-gpu-'))).resolve()
    if stage.exists() and (not stage.is_dir() or any(stage.iterdir())):
        parser.error('--output must be a new or empty directory to avoid stale project files')
    stage.mkdir(parents=True, exist_ok=True)
    sample = ROOT/'samples/Sample 4 - GPU/src'
    if args.task == 'sample4':
        for path in sample.iterdir():
            if path.is_file() and not path.name.endswith('.import'):
                shutil.copy2(path, stage/path.name)
    else:
        for path in (ROOT/'tests/gpu/fixtures').iterdir():
            if path.is_file():
                shutil.copy2(path, stage/path.name)
        for name in ['canvas-benchmark.html','龙珠体ZHS-Regular.ttf']:
            shutil.copy2(sample/name, stage/name)
        (stage/'project.godot').write_text('''config_version=5
[application]
config/name="Ultralight GPU Validation"
[display]
window/size/viewport_width=1024
window/size/viewport_height=768
[rendering]
renderer/rendering_method="mobile"
''', encoding='utf-8')
    for path in (ROOT/'tests/gpu').iterdir():
        if path.suffix == '.gd':
            shutil.copy2(path, stage/path.name)
    if args.task == 'interop':
        shutil.copy2(ROOT/'tests/gpu/BridgeProbe.cs', stage/'BridgeProbe.cs')
        shutil.copy2(ROOT/'tests/gpu/BridgeInterop.csproj', stage/'BridgeInterop.csproj')
        with (stage/'project.godot').open('a', encoding='utf-8') as project:
            project.write('\n[dotnet]\nproject/assembly_name="BridgeInterop"\n')
    addon = stage/'addons/ultralight-view'
    addon.mkdir(parents=True,exist_ok=True)
    binaries = addon/'bin'
    binaries.mkdir(exist_ok=True)
    for path in library.parent.iterdir():
        if path.suffix in ['.dylib', '.dll', '.so']:
            shutil.copy2(path, binaries/path.name)
    os_name = {'Darwin': 'macos', 'Linux': 'linux', 'Windows': 'windows'}[platform.system()]
    arch = {'aarch64': 'arm64', 'AMD64': 'x86_64'}.get(platform.machine(), platform.machine())
    (addon/'ultralight-view.gdextension').write_text(
        '[configuration]\nentry_symbol="ultralight_library_init"\ncompatibility_minimum="4.3"\n'
        f'[libraries]\n{os_name}.debug.{arch}="bin/{library.name}"\n{os_name}.release.{arch}="bin/{library.name}"\n',
        encoding='utf-8')
    base = [str(Path(godot).resolve())]
    base += ['--path', str(stage)]
    # Import the project font before ThemeDB tries to load it on startup.
    project = stage/'project.godot'
    original = project.read_text(encoding='utf-8')
    project.write_text(re.sub(r'^theme/custom_font=.*\n', '', original, flags=re.M), encoding='utf-8')
    if args.task in ['sample4', 'interop']:
        project_name = 'BridgeInterop.csproj' if args.task == 'interop' else 'GPU.csproj'
        run([dotnet, 'build', str(stage/project_name)], stage/'dotnet.log')
    run(base+['--headless', '--editor', '--import'], stage/'import.log')
    project.write_text(original, encoding='utf-8')
    if args.backend == 'headless':
        base += ['--headless']
    elif args.backend == 'compatibility':
        base += ['--rendering-method', 'gl_compatibility']
    elif args.driver:
        base += ['--rendering-driver', args.driver]
    script = {'validate': 'validate.gd', 'interop': 'interop.gd', 'sample4': 'sample4.gd', 'benchmark': 'benchmark.gd'}[args.task]
    command = base+['--script', script, '--']
    if args.task == 'benchmark':
        command.insert(len(base), '--disable-render-loop')
    if args.backend == 'gpu':
        command += ['--expect-gpu']
    if args.task == 'benchmark':
        command += [
            '--objects='+str(args.objects),
            '--workload='+args.workload,
            '--duration='+str(args.duration),
            '--fps-limit='+str(args.fps_limit),
        ]
    print(f'Staged project: {stage}', flush=True)
    if args.task == 'benchmark':
        timeout = args.duration + 120
    elif args.task == 'sample4':
        timeout = 360
    else:
        timeout = 120
    marker = {
        'validate': 'VALIDATION PASS ',
        'interop': 'CSHARP_INTEROP PASS ',
        'sample4': 'SAMPLE4 PASS ',
        'benchmark': 'BENCHMARK ' + args.backend + ' ',
    }[args.task]
    run(command, stage/'run.log', timeout=timeout, success_marker=marker)
    print(f'PASS; logs and project: {stage}')


if __name__ == '__main__':
    main()
