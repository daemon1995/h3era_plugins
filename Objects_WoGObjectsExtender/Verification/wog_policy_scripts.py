h3_NullString"Route legacy entry points through the same per-map assoc keys as C++.h3_NullString"
import json
import re

FAMILIES = {
 '61 wog - potion fountains.erm': [(63,2),(63,48),(30,0)],
 '63 wog - mysterious creature dwelling.erm':[(63,3)],
 '70 wog - junk merchant.erm':[(63,8)], '74 wog - arcane tower.erm':[(63,9)],
 '68 wog - magic mushrooms.erm':[(63,x) for x in (14,18,23,29)],
 '8 wog - death chamber.erm':[(63,15)], '64 wog - altar of transformation.erm':[(63,20)],
 '46 wog - summoning stones.erm':[(63,21)], '18 wog - forgotten shrine.erm':[(63,22)],
 '60 wog - alms house.erm':[(63,24)], '62 wog - battle academy.erm':[(63,32)],
 '67 wog - palace of dreams.erm':[(63,33)], '66 wog - living skull.erm':[(63,50)],
 '48 wog - adventure cave.erm':[(63,57)], '50 wog - school of wizardry.erm':[(63,62)],
 '34 wog - emerald tower.erm':[(63,65)], '26 wog - mirror of the home-way.erm':[(63,66)],
 '52 wog - artificer.erm':[(63,52)],
}

def policy_key(identity):
    return 'wog_objects.native.plugin_handles.%d.%d'%identity

def guard(identities):
    conditions='/'.join('i^'+policy_key(pair)+'^=1' for pair in identities)
    return '; Native map handler: skip ERM only for identities owned by C++.\n!!FU&'+conditions+':E;\n'

OLD_GUARD=re.compile(r'(?m)^; (?:Native WoG handler guard[^\n]*|Native Artificer[^\n]*|'
    r'C\+\+ handles the migrated object[^\n]*|Skip this legacy visit[^\n]*|Native RMG owns[^\n]*)\n'
    r'!!VR[^\n]+:Sv1;\n!!SN:F\^GetEraRegistryIntValue\^/\^wog_objects\.[^\n]+\n'
    r'!!VR[^\n]+:Sv1;\n!!VRv1:S[^\n]+\n!!(?:FU|VRy3)[^\n]+\n')
NEW_GUARD=re.compile(r'(?m)^; Native map handler:[^\n]*\n!!FU&[^\n]+\n')

def patch_triggers(text, selector):
    result=[]
    for line in text.splitlines(keepends=True):
        result.append(line)
        identities=selector(line)
        if identities:result.append(guard(identities))
    return ''.join(result)

def stage_scripts(root, game, stage):
    script_dir=game/'Mods/WoG Scripts/Data/s'
    old_specs=json.loads((root/'legacy/artificer-native-handler.edits.json').read_text(encoding='utf-8-sig'))
    def normalize(name):
        original=(script_dir/name).read_bytes()
        text=original.decode('utf-8-sig').replace('\r\n','\n')
        for spec in old_specs:
            if spec['file']!=name:continue
            for edit in spec['edits']:
                if 'append' in edit:text=text.replace(edit['append'],'')
                elif edit['after'] in text:text=text.replace(edit['after'],edit['before'])
        text=OLD_GUARD.sub('',text)
        text=NEW_GUARD.sub('',text)
        # The Artificer reads these two shared options even when native visits own 63/52.
        if name=='52 wog - artificer.erm':
            text=text.replace('!!UN:P102/?i^wog_102_enabled^ P71/?i^wog_71_enabled^;\n','')
        return original,text
    def save(name,original,text):
        if b'\r\n' in original:text=text.replace('\n','\r\n')
        content=text.encode('utf-8-sig' if original.startswith(b'\xef\xbb\xbf') else 'utf-8')
        stage('Mods/WoG Scripts/Data/s/'+name,content)
    for name,identities in FAMILIES.items():
        original,text=normalize(name)
        def select(line):
            if not re.match(r'^\s*![?$][A-Z]{2}',line):return None
            match=re.match(r'^\s*![?$]OB(63|30|\(OBJ_NEW_WOG_OBJECTS\))/(\d+)(?:[;&]|$)',line)
            if match:
                pair=(63 if match[1].startswith('(') else int(match[1]),int(match[2]))
                if pair in identities:return [pair]
            # Shared callbacks must remain available when ANY family member uses ERM.
            return identities
        text=patch_triggers(text,select)
        if name=='52 wog - artificer.erm':
            text=text.replace('!?FU(OnAfterErmInstructions);\n','!?FU(OnAfterErmInstructions);\n'
                '!!UN:P102/?i^wog_102_enabled^ P71/?i^wog_71_enabled^;\n')
        save(name,original,text)
    name='21 wog - neutral units.erm';original,text=normalize(name)
    def neutral_selector(line):
        match=re.match(r'^\s*![?$]OB63/(15|57)\s*;',line)
        if match:return [(63,int(match[1]))]
        if line.startswith('!?FU(WOG_231_RefillDeathChamber)'):return [(63,15)]
        return None
    save(name,original,patch_triggers(text,neutral_selector))
    # Titan's Winter 140/0 calls the Market helpers independently of the 63/51 policy.
    name='69 wog - market of time.erm';original,text=normalize(name)
    text=patch_triggers(text,lambda line:[(63,51)] if line.startswith((
        '!?FU(WOG_OnIterateAllMapObjects)','!?FU(OnAfterErmInstructions)',
        '!?OB(OBJ_NEW_WOG_OBJECTS)/(OBJ_OPT_MARKET_OF_TIME)','!?OB63/51')) else None)
    save(name,original,text)
    # Each entry has three adjacent index/counter operations. Gate all four together,
    # including entries enclosed by if&y3=1; mushroom/potion ownership is independent.
    name='78 wog - wogify.erm';original,text=normalize(name)
    lines=text.splitlines(keepends=True)
    ported={pair[1] for family in FAMILIES.values() for pair in family if pair[0]==63}|{51}
    for index,line in enumerate(lines):
        match=re.match(r'^\s*!!VRvy10(?:&[^:]*)?:C63/\s*(\d+)/',line)
        if not match or int(match[1]) not in ported:continue
        key=policy_key((63,int(match[1])))
        for at in range(index-3,index+1):
            if not re.match(r'^\s*!!VR(?:y20|y11|y10|vy10)(?:&[^:]*)?:',lines[at]):
                raise RuntimeError('Replacement-table structure changed: '+line)
            if 'i^'+key+'^' in lines[at]:continue
            colon=lines[at].index(':')
            condition=('/' if '&' in lines[at][:colon] else '&')+'i^'+key+'^=0'
            lines[at]=lines[at][:colon]+condition+lines[at][colon:]
    save(name,original,''.join(lines))
