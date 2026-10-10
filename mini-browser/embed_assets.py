"""Embed UI and the selected SDK inspector using the host Python, on every platform."""
import io
import pathlib
import sys
import zipfile


def embed(source, output, name, getter):
    archive = io.BytesIO()
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as zipped:
        for path in sorted(source.rglob("*")):
            if path.is_file():
                # Stable timestamps make rebuilding deterministic.
                info = zipfile.ZipInfo(path.relative_to(source.parent).as_posix(), (2026, 1, 1, 0, 0, 0))
                info.compress_type = zipfile.ZIP_DEFLATED
                zipped.writestr(info, path.read_bytes())
    data = archive.getvalue()
    (output / (name + ".h")).write_text(
        '#pragma once\n#include "archive.h"\nnamespace bin2cpp { const EmbeddedArchive &' + getter + '(); }\n')
    values = "\n".join(",".join(str(byte) for byte in data[i:i + 32]) + "," for i in range(0, len(data), 32))
    (output / (name + ".cpp")).write_text(
        '#include "' + name + '.h"\nnamespace bin2cpp {\nstatic const unsigned char data[] = {\n' + values +
        '\n};\nconst EmbeddedArchive &' + getter + '() { static const EmbeddedArchive file{data, sizeof(data), "' +
        name + '.zip"}; return file; }\n}\n')


if __name__ == "__main__":
    ui, inspector, output = map(pathlib.Path, sys.argv[1:])
    if not ui.is_dir() or not inspector.is_dir():
        raise ValueError("UI or SDK inspector directory missing")
    output.mkdir(parents=True, exist_ok=True)
    (output / "archive.h").write_text('''#pragma once
#include <cstddef>
namespace bin2cpp {
struct EmbeddedArchive {
    const unsigned char *data; std::size_t size; const char *name;
    const unsigned char *getBuffer() const { return data; }
    std::size_t getSize() const { return size; }
    const char *getFileName() const { return name; }
};
}
''')
    embed(ui, output, "ui", "getUiZipFile")
    embed(inspector, output, "inspector", "getInspectorZipFile")
