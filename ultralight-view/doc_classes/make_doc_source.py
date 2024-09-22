import sys
import zlib
from pathlib import Path


def make_doc_source(target: Path, source: Path):
    bufs = []
    docbegin = ""
    docend = ""
    for src in source.iterdir():
        if not src.suffix == ".xml":
            continue
        content = src.read_text(encoding="utf-8")
        bufs.append(content)

    buf = (docbegin + "".join(bufs) + docend).encode("utf-8")
    decomp_size = len(buf)

    # Use maximum zlib compression level to further reduce file size
    # (at the cost of initial build times).
    buf = zlib.compress(buf, zlib.Z_BEST_COMPRESSION)

    with target.open("w", encoding="utf-8") as g:
        g.write("/* THIS FILE IS GENERATED DO NOT EDIT */\n")
        g.write("#pragma once\n")
        g.write("\n")
        g.write("#include <godot_cpp/godot.hpp>\n")
        g.write("\n")

        g.write('static const char *_doc_data_hash = "' + str(hash(buf)) + '";\n')
        g.write(
            "static const int _doc_data_uncompressed_size = " + str(decomp_size) + ";\n"
        )
        g.write("static const int _doc_data_compressed_size = " + str(len(buf)) + ";\n")
        g.write("static const unsigned char _doc_data_compressed[] = {\n")
        for i in range(len(buf)):
            g.write("\t" + str(buf[i]) + ",\n")
        g.write("};\n")
        g.write("\n")

        g.write(
            "static godot::internal::DocDataRegistration _doc_data_registration(_doc_data_hash, _doc_data_uncompressed_size, _doc_data_compressed_size, _doc_data_compressed);\n"
        )
        g.write("\n")


if __name__ == "__main__":
    print(sys.argv)
    make_doc_source(Path(sys.argv[1]), Path(sys.argv[2]))
