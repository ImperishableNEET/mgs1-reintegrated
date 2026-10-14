"""Check every authored virtual-drive prompt keeps its open/close handshake."""
from pathlib import Path
import re
root = Path(__file__).resolve().parents[1]
for name, target in [('open/open.c', '1'), ('demosel/demosel.c', '1'),
                     ('abst/ab_ch.c', 'work->disc_no')]:
    source = (root / 'vendor/mgs/source/onoda' / name).read_text()
    call = 'MgsDiscChangeOpenFor(' + target + ');'
    assert source.count(call) == 1, name
    assert source.count('MgsDiscChangeClose();') == 1, name
    assert re.search(r'case 2:\s*#ifdef MGS_NATIVE\s*' + re.escape(call) +
                     r'\s*#endif\s*param', source), name
    assert re.search(r'case 3:\s*#ifdef MGS_NATIVE\s*MgsDiscChangeClose\(\);'
                     r'\s*#endif\s*param', source), name
    # The authored confirmation and file-system remount stay in place.
    assert 'PAD_START' in source and 'FS_ResetCdFilePosition' in source, name
print('New-game, demo-selection, and save-load drive handshakes passed.')
