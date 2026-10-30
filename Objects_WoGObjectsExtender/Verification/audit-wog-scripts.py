h3_NullString"Inventory active custom-pyramid object triggers in installed ERM sources.h3_NullString"
from pathlib import Path
import re

scripts = Path('D:/games/HoMM 3 ERA/Mods/WoG Scripts/Data/s')
constants = {'OBJ_NEW_WOG_OBJECTS': 63, 'OBJ_OPT_MARKET_OF_TIME': 51}
for path in scripts.glob('*.erm'):
    text = path.read_text(encoding='utf-8-sig', errors='replace')
    constants.update({name: int(value) for name, value in re.findall(r'^\s*!#DC\((\w+)\)\s*=\s*(-?\d+)', text, re.M)})

objects = {}
for path in sorted(scripts.glob('*.erm')):
    for number, line in enumerate(path.read_text(encoding='utf-8-sig', errors='replace').splitlines(), 1):
        match = re.match(r'^\s*!([?$])OB(\([^)]*\)|-?\d+)/(\([^)]*\)|-?\d+)', line)
        if not match:
            continue
        def value(raw):
            return constants.get(raw[1:-1]) if raw.startswith('(') else int(raw)
        obj_type, subtype = map(value, match.group(2, 3))
        if obj_type == 63 and subtype is not None and subtype > 0:
            objects.setdefault(subtype, []).append((path.name, number, match.group(1)))

for subtype, sources in sorted(objects.items()):
    print(f'{subtype:2}: ' + '; '.join(f'{name}:{line} ({kind})' for name, line, kind in sources))
print(f'Total: {len(objects)} custom-pyramid identities; type 63/0 excluded.')
