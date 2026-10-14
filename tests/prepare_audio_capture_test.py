"""Extract the actual optional PCM writer for isolated CPU failure tests."""
import argparse
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    source = (ROOT / "vendor/psyz/psyz/src/platform/sdl3_audio.c").read_text()
    helpers = source[source.index("#define PCM_CAPTURE_LIMIT"):
                     source.index("static void ObserveRefill")]
    # Both serviced output paths capture only samples actually sent to SDL.
    assert source.count("CapturePcmSamples(buffer, frames);") == 2
    assert source.count("CapturePcmSamples(buffer, frames);\n    if (!SDL_PutAudioStreamData") == 2
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(helpers)


if __name__ == "__main__":
    main()
