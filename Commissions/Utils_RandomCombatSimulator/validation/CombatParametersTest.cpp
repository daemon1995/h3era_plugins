#include "../CombatParameters.h"
#include "../EquipmentSelector.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <set>

using namespace randomcombat;

int main()
{
    Generator random(123456);
    Settings settings;
    const std::vector<int> creatures{0, 64, 144, 173, 196};
    std::set<int> experiences, quantities;
    for (int run = 0; run < 20000; ++run)
    {
        const auto hero = random.Hero(run % 156, settings, creatures);
        assert(hero.experience >= 0 && hero.experience <= 18975933);
        assert(hero.artifactCount >= 0 && hero.artifactCount <= 14);
        experiences.insert(hero.experience);
        quantities.insert(hero.artifactCount);
        std::set<int> skillOrder;
        int skills = 0;
        for (int i = 0; i < 28; ++i)
            if (hero.secondary[i])
            {
                ++skills;
                assert(hero.secondary[i] >= 1 && hero.secondary[i] <= 3);
                assert(hero.secondaryOrder[i] >= 1 && hero.secondaryOrder[i] <= hero.secondaryCount);
                assert(skillOrder.insert(hero.secondaryOrder[i]).second);
            }
        assert(skills == hero.secondaryCount && skills <= 8);
        for (int i = 0; i < 7; ++i)
            if (hero.creatures[i] >= 0)
            {
                assert(std::find(creatures.begin(), creatures.end(), hero.creatures[i]) != creatures.end());
                assert(hero.counts[i] >= 1 && hero.counts[i] <= 200);
            }
    }
    assert(experiences.size() > 10000 && quantities.size() == 15);
    settings.experience = {18975933, 18975933};
    settings.artifacts = {14, 14};
    auto hero = random.Hero(155, settings, {144});
    assert(hero.experience == 18975933 && hero.artifactCount == 14);
    settings.experience = {0, 0};
    settings.artifacts = {0, 0};
    hero = random.Hero(0, settings, {0});
    assert(hero.experience == 0 && hero.artifactCount == 0);

    // A fixture with individual body artifacts, two interchangeable rings and
    // five interchangeable miscellaneous items. It exercises the slots where
    // repeated artifact IDs used to be possible.
    const std::array<int, 14> slots{{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 18}};
    std::vector<int> artifacts;
    for (int i = 0; i < 14; ++i)
        artifacts.push_back(100 + i);
    std::set<int> singleItemSlots;
    auto select = [&](int requested, const std::vector<int> &pool) {
        std::array<int, 19> body;
        body.fill(-1);
        std::set<int> equippedIds;
        auto canPlace = [&](int id, int slot) {
            assert(slot >= 0 && slot < 19 && (slot <= 12 || slot == 18));
            if (body[slot] != -1 || id < 100 || id >= 114)
                return false;
            const int index = id - 100;
            if (index == 6 || index == 7)
                return slot == 6 || slot == 7;
            if (index >= 9)
                return (slot >= 9 && slot <= 12) || slot == 18;
            return slot == slots[index];
        };
        const int equipped = EquipUniqueArtifacts(random, requested, pool, canPlace, [&](int id, int slot) {
            assert(canPlace(id, slot));
            assert(equippedIds.insert(id).second);
            body[slot] = id;
            if (requested == 1)
                singleItemSlots.insert(slot);
        });
        assert(equipped == static_cast<int>(equippedIds.size()));
        return equipped;
    };
    for (int run = 0; run < 1000; ++run)
        for (int requested = 0; requested <= 14; ++requested)
            assert(select(requested, artifacts) == requested);
    assert(singleItemSlots.size() == 14);
    assert(select(14, {}) == 0);
    assert(select(14, {500}) == 0);
    assert(select(14, {106}) == 1);
    assert(select(14, {109, 110, 111}) == 3);
    std::cout << "PASS: 20000 heroes, experience/count ranges, distinct compatible artifacts, all wearable slots and scarce pools\n";
}
