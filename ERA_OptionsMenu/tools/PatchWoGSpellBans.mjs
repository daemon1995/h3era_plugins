import { readFile, writeFile, mkdir } from 'node:fs/promises';
import { createHash } from 'node:crypto';
import { resolve, join, basename } from 'node:path';
import { fileURLToPath } from 'node:url';

const managed = 'i^era_options.bans.version^=1';
const marker = '; ERA_OptionsMenu spell integration v1';
const legacyGuard = `; New-map sources are processed once by ERA_MENU_Bans_Sources.\n!!FU&${managed}:E;`;
const escape = value => value.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
const pattern = value => new RegExp(value.split('\n').map(escape).join('\r?\n'), 'g');

export const spellIntegrationSpecs = [
  {
    name: '78 wog - wogify.erm',
    edits: [{
      before: '!?FU(WOG_CheckIfSpellBannedByWogOption);\n!#VA(spell:x) (result:x);\n\n!!VR(result):S(FALSE);',
      after: `!?FU(WOG_CheckIfSpellBannedByWogOption);\n!#VA(spell:x) (result:x);\n\n${marker}\n` +
        `!!if&${managed};\n  !!SN:F^ERA_OptionsMenu:EraOptions_MapObjectBanned^/0/(spell);\n` +
        '  !!VR(result):Sv1;\n  !!FU:E;\n!!en;\n\n!!VR(result):S(FALSE);'
    }]
  },
  {
    name: '53 wog - map options.erm',
    edits: [
      ...['WOG_146_BanSpells', 'WOG_BanSpellsFromObjects', 'WOG_146_BanHeroStartingSpells'].map(name => ({
        before: `!?FU(${name});`,
        after: `!?FU(${name});\n${legacyGuard}`
      })),
      {
        before: '!!SN&(banFromGuildsEnabled:y):Ex1/1/6022493/(WOG_OnConfluxGrailGiveSpells);',
        // This is a shared hook-registration function: never exit its handler chain.
        after: '!!SN&(banFromGuildsEnabled:y)/i^era_options.bans.version^<>1:Ex1/1/6022493/(WOG_OnConfluxGrailGiveSpells);'
      },
      {
        before: '!?FU(WOG_CheckIfAnySpellBannedByWoGOptions);\n!#VA(result:x) (bannedSpellsList:x);',
        after: `!?FU(WOG_CheckIfAnySpellBannedByWoGOptions);\n!#VA(result:x) (bannedSpellsList:x);\n${marker}`
      },
      {
        before: '!!FU(NewIntArray):P?(bannedSpellsList);\n\n!!UN:P152/?(isBanned:y);',
        after: '!!FU(NewIntArray):P?(bannedSpellsList);\n\n' +
          `!!if&${managed};\n  !!re (eraSpell:y)/(SPELL_FIRST)/(SPELL_LAST_WOG);\n` +
          '    !!FU(WOG_CheckIfSpellBannedByWogOption):P(eraSpell)/?(eraBanned:y);\n' +
          '    !!FU(Array_Push)&(eraBanned):P(bannedSpellsList)/(eraSpell);\n' +
          '  !!en;\n!!el;\n!!UN:P152/?(isBanned:y);'
      },
      {
        before: '!!FU(Array_Push)&(isBanned):P(bannedSpellsList)/(SPELL_DISGUISE);\n\n!!SN:M(bannedSpellsList)/?(size:y);',
        after: '!!FU(Array_Push)&(isBanned):P(bannedSpellsList)/(SPELL_DISGUISE);\n!!en;\n\n!!SN:M(bannedSpellsList)/?(size:y);'
      }
    ]
  },
  {
    name: '51 wog - spell book.erm',
    optional: true,
    edits: [
      {
        before: '*** Level 2 ***\n  !!FU(WOG_GENERATE_SPELL):P2/2/0/0/0/0;',
        after: `*** Level 2 ***\n${marker}\n!!VR(eraEnforceBans:y):S0;\n` +
          `!!VR(eraEnforceBans)&${managed}:S1;\n` +
          '  !!FU(WOG_GENERATE_SPELL):P2/2/0/0/(eraEnforceBans)/0;'
      },
      ...['1/v2', '2/v2/v3'].map(tail => ({
        before: `!!FU(WOG_GENERATE_SPELL):P2/2/0/0/0/${tail};`,
        after: `!!FU(WOG_GENERATE_SPELL):P2/2/0/0/(eraEnforceBans)/${tail};`
      }))
    ]
  }
];

export function patchWoGSpellScript(bytes, name) {
  const spec = spellIntegrationSpecs.find(item => item.name === name);
  if (!spec) throw new Error(`Unknown spell integration script: ${name}`);
  // A one-byte representation preserves UTF-8, CP1251, BOMs and mixed endings.
  // All replacements are ASCII; nothing outside the matched blocks is recoded.
  let text = bytes.toString('latin1');
  const installed = text.includes(marker);
  for (const edit of spec.edits) {
    if (installed) {
      if ([...text.matchAll(pattern(edit.after))].length !== 1)
        throw new Error(`${name}: incomplete or modified spell integration; restore/review the script`);
    } else {
      const matches = [...text.matchAll(pattern(edit.before))];
      if (matches.length !== 1) throw new Error(`${name}: expected one known patch location, found ${matches.length}`);
      const match = matches[0];
      const ending = (match[0].includes('\n') ? match[0] : text).includes('\r\n') ? '\r\n' : '\n';
      text = text.slice(0, match.index) + edit.after.replaceAll('\n', ending) + text.slice(match.index + match[0].length);
    }
  }
  return Buffer.from(text, 'latin1');
}

export async function prepareWoGSpellIntegration(gameDirectory, outputDirectory) {
  const prepared = [];
  for (const spec of spellIntegrationSpecs) {
    const target = join('Mods', 'WoG Scripts', 'Data', 's', spec.name);
    let original;
    try { original = await readFile(join(gameDirectory, target)); }
    catch (error) { if (error.code === 'ENOENT') continue; throw error; }
    prepared.push({ target, original, patched: patchWoGSpellScript(original, spec.name) });
  }
  const required = prepared.filter(item => !spellIntegrationSpecs.find(spec => spec.name === basename(item.target)).optional);
  if (prepared.length && required.length !== 2) throw new Error('WoG spell integration requires both scripts 53 and 78');
  // Validate every input before writing even the temporary installation files.
  if (prepared.length) await mkdir(outputDirectory, { recursive: true });
  const manifest = [];
  for (const item of prepared) {
    const source = resolve(outputDirectory, basename(item.target));
    await writeFile(source, item.patched);
    manifest.push({ Source: source, Target: item.target,
      ExpectedHash: createHash('sha256').update(item.original).digest('hex').toUpperCase() });
  }
  return manifest;
}

if (process.argv[1] && resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  try {
    if (process.argv.length !== 4) throw new Error('Usage: node PatchWoGSpellBans.mjs GAME_DIRECTORY OUTPUT_DIRECTORY');
    console.log(JSON.stringify(await prepareWoGSpellIntegration(resolve(process.argv[2]), resolve(process.argv[3]))));
  } catch (error) { console.error(error.message); process.exitCode = 1; }
}
