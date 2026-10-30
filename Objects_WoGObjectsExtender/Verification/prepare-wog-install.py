h3_NullString"Stage a complete, idempotent installation. Never modifies the game directory.h3_NullString"
from pathlib import Path
import argparse
import hashlib
import json
import re

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--game', default='D:/games/HoMM 3 ERA')
parser.add_argument('--out', type=Path, default=ROOT/'build'/'install')
args = parser.parse_args()
game = Path(args.game).resolve()
out = args.out.resolve()
out.mkdir(parents=True, exist_ok=True)
manifest = []

def stage(relative, content=None, source=None):
    relative = Path(relative)
    target = (game / relative).resolve()
    if not target.is_relative_to(game):
        raise RuntimeError('Target escapes game directory')
    dest = out / relative
    dest.parent.mkdir(parents=True, exist_ok=True)
    if content is None: content = source.read_bytes()
    dest.write_bytes(content)
    manifest.append({'Target': str(target), 'Source': str(dest),
                     'Sha256': hashlib.sha256(content).hexdigest()})

for name, directory in [('Objects_WoGObjectsExtender', ROOT),
                        ('RMG_CustomizeObjectProperties', ROOT.parent/'RMG_CustomizeObjectProperties')]:
    binary = directory/'build'/'Release'/(name+'.era')
    if not binary.is_file(): raise RuntimeError('Missing Release binary: '+str(binary))
    stage('Mods/WoG/EraPlugins/'+name+'.era', source=binary)
    for extension, folder in [('pdb','EraPlugins'), ('map','DebugMaps'), ('dbgmap','DebugMaps')]:
        path = directory/'build'/('Release' if extension=='pdb' else 'DebugMaps')/(name+'.'+extension)
        if path.exists(): stage(f'Mods/WoG/{folder}/{name}.{extension}',source=path)

for relative in ('wog_objects.json', 'ru/wog_objects.json'):
    source = json.loads((ROOT/'lang'/relative).read_text(encoding='utf-8-sig'))
    target = game/'Mods/WoG Scripts/Lang'/relative
    result = json.loads(target.read_text(encoding='utf-8-sig')) if target.exists() else {}
    # Replace this plugin's owned text; preserve other mods and unknown object identities.
    result.setdefault('wog_objects',{}).update(source['wog_objects'])
    result.setdefault('RMG',{}).setdefault('objectGeneration',{}).setdefault('141',{}).update(
        source['RMG']['objectGeneration']['141'])
    for script, texts in source.get('wog', {}).items():
        result.setdefault('wog', {}).setdefault(script, {}).update(texts)
    stage('Mods/WoG Scripts/Lang/'+relative, (json.dumps(result,ensure_ascii=False,indent=2)+'\n').encode())

from wog_policy_scripts import stage_scripts
stage_scripts(ROOT, game, stage)
(out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
print(f'Prepared {len(manifest)} files; 23 subtypes, both plugins and map-bound ERM guards. Game files untouched.')
