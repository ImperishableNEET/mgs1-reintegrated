"""Check the six-entry HZD_NearZones contract used by both enemy callers."""

import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / "vendor/mgs/source/libhzd/navigate.c").read_text(encoding="utf-8", errors="replace")
near_zones = re.search(
    r"static int NearZones\(.*?\n\}\n", source, re.DOTALL
)
assert near_zones, "NearZones implementation missing"
for name in ("think.c", "merylthink.c"):
    caller = (ROOT / "vendor/mgs/source/enemy" / name).read_text(encoding="utf-8", errors="replace")
    call = caller.index("res = HZD_NearZones( hzd, addr, unk )")
    sizes = re.findall(r"int unk\[(\d+)\]", caller[:call])
    assert sizes and int(sizes[-1]) >= 6, f"{name}: undersized near-zone array"

program = r"""
#include <assert.h>
typedef unsigned char u_char;
enum { HZD_NO_ZONE = 255 };
typedef struct { u_char nears[6]; } HZD_ZON;
typedef struct { HZD_ZON zones[1]; } HZD_DEF;
typedef struct { HZD_DEF *def; } HZD_HDL;
""" + near_zones.group(0) + r"""
int main(void) {
    HZD_DEF def = {.zones = {{.nears = {1, 2, 3, 4, 5, HZD_NO_ZONE}}}};
    HZD_HDL hzd = {&def};
    struct { int near[6]; int guard; } output = {{0}, 0x12345678};
    assert(NearZones(&hzd, 0, output.near) == 5);
    assert(output.near[5] == HZD_NO_ZONE);
    assert(output.guard == 0x12345678);
    return 0;
}
"""
compiler = os.environ.get("CC") or shutil.which("gcc") or shutil.which("clang")
assert compiler, "C compiler required"
with tempfile.TemporaryDirectory() as directory:
    source_file = Path(directory) / "near_zones.c"
    executable = Path(directory) / "near_zones.exe"
    source_file.write_text(program)
    subprocess.run([compiler, "-std=c11", "-Wall", "-Wextra", "-Werror",
                    str(source_file), "-o", str(executable)], check=True)
    subprocess.run([str(executable)], check=True)
print("NearZones six-entry capacity: PASS")
