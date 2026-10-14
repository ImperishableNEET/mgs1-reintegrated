"""Extract physical keyboard state and the Windows raw-release adapter."""
import argparse
from pathlib import Path
from prepare_mantis_test import definition

ROOT = Path(__file__).resolve().parent.parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    helper = (ROOT / "vendor/psyz/psyz/src/platform/mantis_keyboard.h").read_text()
    helper = helper[helper.index("typedef struct PsyzMantisKeyboard"):
                    helper.index("/* Returns connection status")]
    windows = (ROOT / "vendor/psyz/external/SDL/src/video/windows/SDL_windowsevents.c").read_text()
    adapter = definition(windows, "WIN_SendRawKeyboardKey")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(helper + "\n" + adapter)


if __name__ == "__main__":
    main()
