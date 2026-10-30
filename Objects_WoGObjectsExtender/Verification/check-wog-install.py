h3_NullString"Install twice into a disposable game fixture, check guards/merging and exercise rollback.h3_NullString"
import hashlib
import json
import os
import re
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT=Path(__file__).resolve().parents[1]
GAME=Path('D:/games/HoMM 3 ERA')
fixture=Path(tempfile.mkdtemp(prefix='H3EraWoGInstall-'))
scripts=fixture/'Mods/WoG Scripts/Data/s'
scripts.mkdir(parents=True)
for path in (GAME/'Mods/WoG Scripts/Data/s').glob('*.erm'):
    shutil.copyfile(path,scripts/path.name)
for name in ('52 wog - artificer.erm','78 wog - wogify.erm'):
    with (scripts/name).open('ab') as stream:
        stream.write(b'\n; Fixture source preservation marker\n')
lang=fixture/'Mods/WoG Scripts/Lang'
lang.mkdir(parents=True)
custom={'customVendor':{'preserve':True},'wog_objects':{'otherPlugin':{'preserve':True}},
        'RMG':{'objectGeneration':{'141':{'999':{'name':'Keep me'}}}}}
(lang/'wog_objects.json').write_text(json.dumps(custom),encoding='utf-8')
installer=ROOT/'Install-WoGObjects.ps1'
command=['powershell.exe','-NoProfile','-ExecutionPolicy','Bypass','-File',str(installer),'-GameRoot',str(fixture),'-Python',sys.executable]
def run(command):
    environment={k.upper():v for k,v in os.environ.items()}
    environment.pop('PSMODULEPATH',None) # Let Windows PowerShell discover its own modules, not PowerShell 7's.
    result=subprocess.run(command,env=environment,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    if result.returncode: raise RuntimeError(result.stdout.decode('utf-8',errors='replace'))
run(command)
manifest=json.loads((ROOT/'build/install/manifest.json').read_text())
def digest(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
first={row['Target']:digest(row['Target']) for row in manifest}
data=json.loads((lang/'wog_objects.json').read_text(encoding='utf-8-sig'))
assert data['customVendor']['preserve'] and data['wog_objects']['otherPlugin']['preserve']
objects=data['RMG']['objectGeneration']['141']
expected={2,3,8,9,14,15,18,20,21,22,23,24,29,32,33,48,50,51,52,57,62,65,66}
assert set(map(int,objects))==expected|{999}
assert all(objects[str(x)]['properties'] for x in expected)
assert len(data['wog_objects']['native']['hint'])>=21
for row in manifest:
    path=Path(row['Target'])
    if path.suffix=='.erm' and path.name not in ('52 wog - artificer.erm','69 wog - market of time.erm','78 wog - wogify.erm','21 wog - neutral units.erm'):
        lines=path.read_text(encoding='utf-8-sig').splitlines()
        for index,line in enumerate(lines):
            if line.startswith(('!?','!$')):
                assert 'Native map handler' in lines[index+1],(path.name,line)
market=(scripts/'69 wog - market of time.erm').read_text(encoding='utf-8-sig')
assert '!?OB(OBJ_BLANK_2)/0&(ERM_FLAG_IS_HUMAN);\n!!FU(WOG_109_VisitMarketOfTime):P;' in market
assert '!?FU(WOG_109_VisitMarketOfTime);\n!!UN:' in market
for row in manifest:
    if Path(row['Target']).suffix!='.erm':continue
    text=Path(row['Target']).read_text(encoding='utf-8-sig')
    assert not re.search(r'GetEraRegistryIntValue\^/\^wog_objects\.',text),row['Target']

# Shared scripts must not use the first member's policy for every OB trigger.
for name,identities in [('68 wog - magic mushrooms.erm',[(63,x) for x in (14,18,23,29)]),
                         ('61 wog - potion fountains.erm',[(63,2),(63,48),(30,0)])]:
    lines=(scripts/name).read_text(encoding='utf-8-sig').splitlines()
    family={'wog_objects.native.plugin_handles.%d.%d'%pair for pair in identities}
    callbacks=0
    for index,line in enumerate(lines):
        if not line.startswith(('!?','!$')):continue
        owned=set(re.findall(r'i\^(wog_objects.native.plugin_handles.\d+.\d+)\^=1',lines[index+2]))
        match=re.match(r'![?$]OB(\d+)/(\d+)',line)
        if match:
            assert owned=={'wog_objects.native.plugin_handles.%s.%s'%match.groups()}
        else:
            assert owned==family;callbacks+=1
    assert callbacks>0

wogify=(scripts/'78 wog - wogify.erm').read_text(encoding='utf-8-sig').splitlines()
for index,line in enumerate(wogify):
    match=re.match(r'^\s*!!VRvy10[^:]*:C63/\s*(\d+)/',line)
    if not match or int(match[1]) not in expected:continue
    for entry in wogify[index-3:index+1]:
        assert 'i^wog_objects.native.plugin_handles.63.'+match[1]+'^=0' in entry
for name in ('52 wog - artificer.erm','78 wog - wogify.erm'):
    assert '; Fixture source preservation marker' in (scripts/name).read_text(encoding='utf-8-sig')
run(command)
assert first=={path:digest(path) for path in first},'Repeated installation is not idempotent'
backups=list((fixture/'Mods/WoG/Backup').glob('WoGObjects-*'))
assert len(backups)==2

# Corrupt one installed DLL, then lock a later target. Earlier writes must be rolled back on failure.
plugin=fixture/'Mods/WoG/EraPlugins/Objects_WoGObjectsExtender.era'
plugin.write_bytes(b'fixture-old-binary')
before=digest(plugin)
locked=fixture/'Mods/WoG/EraPlugins/RMG_CustomizeObjectProperties.era'
ps_script=fixture/'rollback.ps1'
ps_script.write_text('''param($Installer,$Game,$Python,$Locked)
$stream=[IO.File]::Open($Locked,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::Read)
try {
    try { & $Installer -GameRoot $Game -Python $Python; throw 'Expected an installation failure' }
    catch { if ($_.Exception.Message -eq 'Expected an installation failure') { throw } }
} finally { $stream.Dispose() }
''',encoding='utf-8')
run(['powershell.exe','-NoProfile','-ExecutionPolicy','Bypass','-File',str(ps_script),str(installer),str(fixture),sys.executable,str(locked)])
assert digest(plugin)==before,'Rollback did not restore the earlier DLL'
assert all(digest(path)==value for path,value in first.items() if Path(path)!=plugin)
print('PASS: complete 23-object installation, native prototypes, ERM guards, JSON preservation, backups, idempotence and rollback')
print('Fixture:',fixture)
