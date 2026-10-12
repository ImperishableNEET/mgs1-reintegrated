"""Audit staged/tracked payloads; no game data is needed."""
from pathlib import Path
import subprocess,sys,re
root=Path(__file__).resolve().parents[1]
# Reviewed upstream payloads: source inventory metadata and generated synthetic fixtures.
files=subprocess.check_output(['git','ls-files','-z'],cwd=root).decode().split('\0')
errors=[]
for name in filter(None,files):
 p=root/name
 if not p.exists():continue
 if p.is_symlink():errors.append((name,'symlink'));continue
 if name.split('/')[0] in {'vendor','content','discs','saves','bu00','bu10','postboss-saves','port-audit'}:errors.append((name,'private directory'))
 if p.suffix.lower() in {'.bin','.cue','.iso','.chd','.mcr','.mcd','.sav','.exe','.dll','.pdb','.obj','.dat','.dir','.mgslang'}:errors.append((name,'excluded file type'))
 data=p.read_bytes()
 if len(data)>2_000_000 and name not in {'docs/data/stage_inventory.json','tests/fixtures/mgs_pocket_back_fixtures.h'}:errors.append((name,'large payload requires review'))
 if name != 'tools/check_public_tree.py' and re.search(rb'(?:C:[/\\]Users[/\\]1996r|/home/rj/)',data,re.I):errors.append((name,'machine-specific path'))
 if re.search(rb'(?:gh[pousr]_[A-Za-z0-9]{30,}|-----BEGIN (?:RSA |OPENSSH )?PRIVATE KEY-----)',data):errors.append((name,'possible secret'))
for name,why in errors:print(f'{name}: {why}')
print(f'Public-tree audit: {len(errors)} findings, {len(files)-1} tracked files')
sys.exit(bool(errors))
