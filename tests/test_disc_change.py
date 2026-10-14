"""Exercise the native disc-change boundary with two complete synthetic images."""

import argparse
import subprocess
import tempfile
import re
from pathlib import Path

from tests.test_safety import CUE, sector_fixture
from tests.prepare_mantis_test import definition


def check_save_load_actor() -> None:
    """The runtime swap API alone cannot catch a missing save-load actor hook."""
    root = Path(__file__).resolve().parent.parent
    source = (root / "vendor/mgs/source/onoda/abst/ab_ch.c").read_text(encoding="utf-8")
    actor = definition(source, "func_800C71D0")
    for case, following, call in (
        (2, 3, r"MgsDiscChangeOpenFor\s*\(\s*work->disc_no\s*\)\s*;"),
        (3, 4, r"MgsDiscChangeClose\s*\(\s*\)\s*;"),
    ):
        branch = actor.split(f"case {case}:", 1)[1].split(f"case {following}:", 1)[0]
        native = re.search(r"#ifdef MGS_NATIVE\s+(.*?)#endif", branch, re.S)
        assert native and re.search(call, native.group(1)), "Missing native save-load disc request hook"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("probe", type=Path)
    args = parser.parse_args()
    check_save_load_actor()
    with tempfile.TemporaryDirectory(prefix="mgs-disc-change-") as temporary:
        paths = []
        for number in (1, 2):
            directory = Path(temporary) / f"disc{number}"
            directory.mkdir()
            content = b"".join(sector_fixture())
            if number == 2:
                content = content.replace(b"SLPM_862.47", b"SLPM_862.48")
            (directory / "disc.bin").write_bytes(content)
            cue = directory / "disc.cue"
            cue.write_text(CUE, encoding="ascii")
            paths.append(str(cue))
        subprocess.run([str(args.probe.resolve()), *paths], check=True, timeout=10)


if __name__ == "__main__":
    main()
