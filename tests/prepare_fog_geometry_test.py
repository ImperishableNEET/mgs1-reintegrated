"""Extract live fog packet projection for asset-free CPU regressions."""
import argparse
from pathlib import Path
from prepare_mantis_test import definition

ROOT = Path(__file__).resolve().parent.parent

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    source = (ROOT / "vendor/mgs/source/takabe/libdg2.c").read_text(encoding="utf-8")
    start = source.index("// modified DG_WriteObjVerticesIndirect")
    end = source.index("#ifdef MGS_NATIVE\n#define FOG_HALF_WIDTH", start)
    first = source[start:end]
    start = source.index("// Modified DG_TransObj")
    end = source.index("void FogTransChanl_800D63B0", start)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(first + "\n" + source[start:end] + "\n" +
                          definition(source, "FogTransChanl_800D63B0"), encoding="utf-8")

if __name__ == "__main__":
    main()
