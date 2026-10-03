"""Validate installed EN/RU spell texts against C++ keys and printf signatures.

Translations live only in the mod. This tool reads files without modifying them.
"""

import argparse
import json
from pathlib import Path
import re

PLUGIN = Path(__file__).resolve().parents[1]


def unique_pairs(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"Duplicate JSON key: {key}")
        result[key] = value
    return result


def read_json(path):
    return json.loads(path.read_text(encoding="utf-8-sig"), object_pairs_hook=unique_pairs)


def flatten(node, prefix=""):
    if isinstance(node, dict):
        return {
            key: value
            for name, child in node.items()
            for key, value in flatten(child, f"{prefix}.{name}" if prefix else name).items()
        }
    if isinstance(node, list):
        return {
            key: value
            for index, child in enumerate(node)
            for key, value in flatten(child, f"{prefix}.{index}").items()
        }
    return {prefix: node}


def signature(text):
    if not isinstance(text, str):
        raise ValueError("Translation must be a string")
    result = ""
    index = 0
    while index < len(text):
        if text[index] == "%":
            index += 1
            if index == len(text) or text[index] not in "%sd":
                raise ValueError(f"Unsupported printf token in {text!r}")
            if text[index] != "%":
                result += text[index]
        index += 1
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--game-dir", type=Path, required=True)
    args = parser.parse_args()
    source = (PLUGIN / "SpellDescriptionTranslations.h").read_text(encoding="utf-8-sig")
    definitions = re.findall(r'\{ERA_SPELL_TEXT\((\w+),\s*(\w+)\),\s*"([sd]*)"\}', source)
    if not definitions:
        raise ValueError("No spell text definitions found")
    prefix_match = re.search(r'#define ERA_SPELL_TEXT\(section, field\)\s+"([^"]+)"', source)
    if not prefix_match:
        raise ValueError("No spell text prefix found")
    prefix = prefix_match.group(1)
    expected = {f"{prefix}{section}.{key}": tokens for section, key, tokens in definitions}
    if len(expected) != len(definitions):
        raise ValueError("Duplicate spell text definition")

    languages = {}
    for language, relative in (("en", Path()), ("ru", Path("ru"))):
        directory = args.game_dir / "Mods" / "WoG" / "lang" / relative
        target = directory / "interface_spells_description.json"
        fragment = read_json(target)
        translations = flatten(fragment)
        if translations.keys() != expected.keys():
            missing = sorted(expected.keys() - translations.keys())
            extra = sorted(translations.keys() - expected.keys())
            raise ValueError(f"{language}: missing keys {missing}; unused keys {extra}")
        for key, tokens in expected.items():
            if signature(translations[key]) != tokens:
                raise ValueError(f"{language}: printf signature mismatch at {key}")
            if not translations[key] and tokens:
                raise ValueError(f"{language}: empty required text at {key}")
        languages[language] = translations
        print(f"Verified {language}: {len(translations)} installed spell keys, matching printf signatures")

    missing_ru = languages["en"].keys() - languages["ru"].keys()
    print(f"Spell keys: EN={len(languages['en'])}, RU={len(languages['ru'])}, missing RU={len(missing_ru)}")
    if languages['en'].keys() != languages['ru'].keys():
        raise ValueError("English/Russian spell keys differ")


if __name__ == "__main__":
    main()
