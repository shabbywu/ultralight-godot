#!/usr/bin/env python3
"""Install a locally built extension into Sample 4 and rebuild its C# scripts."""
import argparse
from pathlib import Path
import platform
import shutil
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--library', required=True, type=Path)
    args = parser.parse_args()
    library = args.library.resolve(strict=True)
    project = Path(__file__).resolve().parent/'src'
    addon = project/'bin/addons/ultralight-view'
    binaries = addon/'bin'/library.parent.name
    binaries.mkdir(parents=True, exist_ok=True)
    for source in library.parent.iterdir():
        if source.suffix in ('.dylib', '.dll', '.so'):
            # Replacing the file atomically preserves any library already
            # mapped by the editor. Restart Godot to load the new version.
            temporary = binaries/(source.name+'.new')
            shutil.copy2(source, temporary)
            temporary.replace(binaries/source.name)
    os_name = {'Darwin':'macos','Linux':'linux','Windows':'windows'}[platform.system()]
    arch = {'aarch64':'arm64','AMD64':'x86_64'}.get(platform.machine(),platform.machine())
    (addon/'ultralight-view.gdextension').write_text(
        '[configuration]\nentry_symbol="ultralight_library_init"\ncompatibility_minimum="4.3"\n'
        f'[libraries]\n{os_name}.debug.{arch}="bin/{library.parent.name}/{library.name}"\n'
        f'{os_name}.release.{arch}="bin/{library.parent.name}/{library.name}"\n')
    # Canonical paths also prevent Roslyn from generating incorrect res://
    # script paths when this workspace is reached through a filesystem alias.
    subprocess.run(['dotnet','build',str(project/'GPU.csproj'),'-t:Rebuild'],cwd=project,check=True)
    print(f'Installed {library.name} into {binaries}. Restart Godot to load it.')


if __name__ == '__main__':
    main()
