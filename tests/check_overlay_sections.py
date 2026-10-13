"""Reject split/read-only overlay state sections in an actual linked PE image."""
import struct
import sys
from pathlib import Path


def check(path):
    data = Path(path).read_bytes()
    pe = struct.unpack_from("<I", data, 0x3c)[0]
    if data[pe:pe + 4] != b"PE\0\0":
        raise ValueError("Not a PE image")
    count = struct.unpack_from("<H", data, pe + 6)[0]
    optional_size = struct.unpack_from("<H", data, pe + 20)[0]
    sections = pe + 24 + optional_size
    found = []
    for index in range(count):
        offset = sections + index * 40
        name = data[offset:offset + 8].rstrip(b"\0")
        if not name.startswith(b".mgsst"):
            continue
        flags = struct.unpack_from("<I", data, offset + 36)[0]
        rva = struct.unpack_from("<I", data, offset + 12)[0]
        found.append((rva, flags))
        if flags & 0xc0000000 != 0xc0000000:
            raise ValueError(f"Overlay section at RVA {rva:#x} is not read/write: {flags:#x}")
    if not found:
        raise ValueError("No overlay state section found")
    if len(found) != 1:
        raise ValueError(f"Overlay state split across {len(found)} PE sections")
    print("Linked overlay state is one read/write PE section.")


if __name__ == "__main__":
    check(sys.argv[1])
