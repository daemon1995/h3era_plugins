from pathlib import Path
import importlib.util
import subprocess
import sys

project = Path(__file__).resolve().parent.parent
spec = importlib.util.spec_from_file_location("erm_artificer", project / "erm/tools/check_artificer.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
vm = module.VM((project / "erm/Artificer.functions.inc").read_text(encoding="utf-8-sig"))
rows = subprocess.run([sys.argv[1], "--rules"], capture_output=True, text=True, check=True).stdout.splitlines()
checks = 0
actual_exchanges = {}
for row in rows:
    fields = row.split()
    values = list(map(int, fields[1:]))
    if fields[0] == "U":
        artifact, boosted, enchanted, *actual = values
        expected = vm.call("WOG_Artificer_GetUpgradeInfo", [artifact, boosted, enchanted, 0, 0, 0])[3:]
        assert actual == expected, (row, expected)
    elif fields[0] == "C":
        recipe, enchanted, target, count, gold, mithril, *parts = values
        expected = vm.call("WOG_Artificer_GetCombinationInfo", [recipe, enchanted] + [0] * 10)[2:]
        assert [target, count] + parts + [gold, mithril] == expected, (row, expected)
    else:
        artifact, enchanted, target, gold, mithril = values
        actual_exchanges.setdefault((artifact, enchanted), []).append((target, gold, mithril))
    checks += 1
for artifact in range(-1, 201):
    for enchanted in (0, 1):
        expected = vm.call("WOG_Artificer_GetExchangeInfo", [artifact, enchanted, 0, 0, 0, 0, 0, 0])[3:]
        targets, gold, mithril = expected[:3], expected[3], expected[4]
        assert sorted(actual_exchanges.get((artifact, enchanted), [])) == sorted((target, gold, mithril) for target in targets if target)
        checks += 1
print(f"PASS: {checks} C++/ERM2 recipe and price comparisons")
