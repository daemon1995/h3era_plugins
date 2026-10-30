from __future__ import print_function
import json
import re
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
EXPECTED = {2,3,8,9,14,15,18,20,21,22,23,24,29,32,33,48,50,57,62,65,66}
NEW_SOURCES = {
    'WoGObjectIds.cpp','WoGObjectState.cpp','WoGObjectUtils.cpp','WoGObjectBasic.cpp',
    'WoGObjectTrading.cpp','WoGObjectSkull.cpp','WoGObjectCombat.cpp','WoGObjectEmerald.cpp',
    'WoGObjectHandlers.cpp','WoGObjectPotions.cpp','WoGObjectLearning.cpp','WoGObjectCave.cpp'
}
errors = []

def check(ok, msg):
    if not ok:
        errors.append(msg)

def read(name):
    return (ROOT / name).read_text(encoding='utf-8')

ids_h = read('WoGObjectIds.h')
ids_cpp = read('WoGObjectIds.cpp')
handler = read('WoGObjectHandlers.cpp')
extender = read('WoGObjectsExtender.cpp')
project = read('Objects_WoGObjectsExtender.vcxproj')

# Parse descriptor subtype symbols through the enum.
enum_block = re.search(r'enum Subtype\s*:\s*int\s*\{(.*?)\};', ids_h, re.S)
check(enum_block is not None, 'Subtype enum not found')
name_to_id = {}
if enum_block:
    for name, value in re.findall(r'([A-Z0-9_]+)\s*=\s*(\d+)', enum_block.group(1)):
        name_to_id[name] = int(value)

desc_block = re.search(r'const Descriptor kDescriptors\[\]\s*=\s*\{(.*?)\};', ids_cpp, re.S)
check(desc_block is not None, 'Descriptor table not found')
actual = set()
if desc_block:
    for sym in re.findall(r'\{\s*([A-Z0-9_]+)\s*,', desc_block.group(1)):
        if sym in name_to_id:
            actual.add(name_to_id[sym])
check(actual == EXPECTED, 'Descriptor subtypes mismatch: expected %r got %r' % (sorted(EXPECTED), sorted(actual)))
check('= 80' not in enum_block.group(1) if enum_block else False, 'Subtype enum unexpectedly contains type/subtype 80')
check('Commander Sanctuary' in ids_h, 'Sanctuary exclusion comment/assert missing')
check('AddUniqueObjectInfo(80' not in extender and 'WOG_OBJECT_TYPE = 80' not in extender,
      'Universal extender appears to register type 80')

for source in NEW_SOURCES:
    check(('Include="%s"' % source) in project, 'Project is missing source: %s' % source)

for locale in ['lang/wog_objects.json', 'lang/ru/wog_objects.json']:
    try:
        data = json.loads(read(locale))
        entries = data['RMG']['objectGeneration']['141']
        for subtype in EXPECTED:
            check(str(subtype) in entries, '%s missing RMG 141/%d' % (locale, subtype))
            if str(subtype) in entries:
                entry = entries[str(subtype)]
                check(bool(entry.get('properties')), '%s missing native prototype for 141/%d' % (locale, subtype))
                for row in entry.get('properties', []):
                    columns = row.split()
                    check(len(columns) == 9 and columns[5:7] == ['141', str(subtype)], 'Invalid object prototype: '+row)
        check('chooseMana' in data['wog_objects']['native']['altar'], '%s missing altar.chooseMana' % locale)
        for source in ROOT.glob('WoGObject*.cpp'):
            for key in re.findall(r'util::Tr\("([^\"]+)"', source.read_text(encoding='utf-8')):
                if key.endswith('.'): continue
                node = data['wog_objects']['native']
                for component in key.split('.'):
                    node = node.get(component) if isinstance(node,dict) else None
                check(node is not None, '%s missing native text %s' % (locale,key))
    except Exception as exc:
        errors.append('%s JSON error: %s' % (locale, exc))

constants = dict(re.findall(r'constexpr int ([A-Z_]+)\s*=\s*(\d+);', ids_h))
expected_ids = {
    'HELL_BARON':'153','BLOOD_DRAGON':'154','DARKNESS_DRAGON':'155','GHOST':'159',
    'FIRST_EMISSARY':'160','LAST_EMISSARY':'163','GORYNYCH':'168',
    'ARCTIC_SHARPSHOOTER':'170','LAVA_SHARPSHOOTER':'171','SORCERESS':'193'
}
for key, value in expected_ids.items():
    check(constants.get(key) == value, '%s expected %s, got %s' % (key, value, constants.get(key)))

check('H3CreatureCount::Get()' in (read('WoGObjectBasic.cpp') + read('WoGObjectEmerald.cpp') + read('WoGObjectUtils.cpp')),
      'No dynamic creature-count use found')
check('H3ArtifactCount::Get()' in (read('WoGObjectTrading.cpp') + read('WoGObjectSkull.cpp') + read('WoGObjectUtils.cpp')),
      'No dynamic artifact-count use found')
check('LegacyObjectClickHint' in extender and 'interactPlayerId' in extender,
      'Legacy human-aware hint path missing')
check('state::Month()' in read('WoGObjectEmerald.cpp'), 'Emerald monthly state missing')
check('0x4AA820' in read('WoGObjectSkull.cpp'), 'Living Skull native removal path missing')

for source in ROOT.glob('*.cpp'):
    text=source.read_text(encoding='utf-8')
    check('HeroVariable' not in text and '0xA4AB10' not in text,
          source.name+' still uses w storage')
    check(not re.search(r'Era::[vzwf]\s*\[',text), source.name+' uses fixed ERM variables')
    check('EraRegistryIntValue' not in text, source.name+' uses process registry for map state')
    if source.name!='WoGObjectUtils.cpp':
        check('ExecErmCmd' not in text, source.name+' bypasses the isolated WoG bridge')
for source in ('MarketOfTime.cpp','Artificer.cpp','WoGObjectBasic.cpp','WoGObjectLearning.cpp','WoGObjectTrading.cpp','WoGObjectSkull.cpp'):
    check('Receiver(' not in read(source), source+' still uses an ERM receiver')

if errors:
    print('FAILED (%d):' % len(errors))
    for e in errors:
        print(' -', e)
    sys.exit(1)
print('OK: %d native WoG subtypes validated; Commander Sanctuary remains excluded.' % len(EXPECTED))
