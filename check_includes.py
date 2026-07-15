from pathlib import Path

source_roots = [Path("src"), Path("include"), Path("lib/custom")]
source_files = []
for root in source_roots:
    for path in root.rglob("*"):
        if path.suffix in {".c", ".cpp", ".h", ".hpp"}:
            source_files.append(path)

header_index = {}
for root in [Path("lib"), Path("sys"), Path("include")]:
    for path in root.rglob("*"):
        if path.is_file():
            header_index[path.name] = path


def resolve_header(name):
    return header_index.get(name)


for file in source_files:
    original_data = file.read_text()
    data = original_data.split("\n")

    i = 0
    line = data[i].strip()
    hasIWYU = False
    while line.startswith(("#include", "/*", " *", "#pragma")) or len(line) == 0:
        if "#pragma once" in line or "*" in line or len(line) == 0:
            i += 1
            line = data[i].strip()
            continue
        if line.endswith("  // IWYU pragma: keep"):
            line = line.removesuffix("  // IWYU pragma: keep").strip()
            hasIWYU = True
        if '"' in line:
            line = line.removeprefix('#include "').removesuffix('"')
        elif "<" in line:
            line = line.removeprefix("#include <").removesuffix(">")

        # print(line)
        if line.startswith(("al/", "sead/", "nn/", "game/", "custom/", "hk/")):
            i += 1
            line = data[i].strip()
            hasIWYU = False
            continue

        header_path = resolve_header(line.split("/")[-1])
        headerFile = (
            "" if header_path is None else str(header_path).removeprefix("lib/")
        )
        # print(headerFile)

        if headerFile.startswith("std"):
            data[i] = (
                f"#include <{headerFile.removeprefix("std/llvm-project/build/include/c++/v1/").removeprefix("std/musl/include/")}>"
            )
        elif headerFile.startswith("OdysseyHeaders/NintendoSDK"):
            data[i] = (
                f'#include "{headerFile.removeprefix("OdysseyHeaders/NintendoSDK/")}"'
            )
        elif headerFile.startswith("OdysseyHeaders"):
            data[i] = f'#include "{headerFile.removeprefix("OdysseyHeaders/")}"'
        elif headerFile.startswith("sys/"):
            data[i] = (
                f'#include "{headerFile[headerFile.find("include/hk")+len("include/")::]}"'
            )
        elif headerFile.startswith("include"):
            data[i] = f'#include "{headerFile.removeprefix("include/")}"'
        elif headerFile.startswith("imgui/"):
            data[i] = f'#include "{headerFile.removeprefix("imgui/")}"'
        elif len(headerFile) != 0:
            data[i] = f'#include "{headerFile}"'

        if hasIWYU:
            data[i] += "// IWYU pragma: keep"
            hasIWYU = False

        i += 1
        line = data[i].strip()
    updated_data = "\n".join(data)
    if updated_data != original_data:
        file.write_text(updated_data)
