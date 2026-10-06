#pragma once

#include <array>
#include <random>
#include <utility>
#include <vector>

namespace randomcombat
{
struct Range
{
    int min;
    int max;
};

struct Settings
{
    Range stacks{1, 7};
    Range creatures{0, 196};
    Range count{1, 200};
    Range primary{0, 30};
    Range secondary{0, 8};
    Range experience{0, 18975933}; // Native experience threshold for level 50.
    Range artifacts{0, 14};
    Range mana{0, 300};
    Range morale{-3, 3};
    Range luck{-3, 3};
    int spellChance = 50;
    int spellbookChance = 80;
    int warMachineChance = 50;
    bool randomTerrain = true;
};

struct HeroParameters
{
    int id;
    int experience;
    int mana;
    int morale;
    int luck;
    int artifactCount;
    int secondaryCount;
    std::array<int, 4> primary;
    std::array<int, 28> secondary{};
    std::array<int, 28> secondaryOrder{};
    std::array<bool, 70> spells{};
    std::array<int, 7> creatures{{-1, -1, -1, -1, -1, -1, -1}};
    std::array<int, 7> counts{};
};

// Keep this generator independent of the game RNG: restarting a map may restore
// its seed, but the next simulated battle must have new parameters.
class Generator
{
    std::mt19937 engine;

  public:
    explicit Generator(unsigned seed) : engine(seed) {}

    int Between(int min, int max)
    {
        return std::uniform_int_distribution<int>(min, max)(engine);
    }

    bool Chance(int percent) { return Between(1, 100) <= percent; }

    HeroParameters Hero(int id, const Settings &settings, const std::vector<int> &creaturePool)
    {
        HeroParameters result{};
        result.id = id;
        result.experience = Between(settings.experience.min, settings.experience.max);
        result.mana = Between(settings.mana.min, settings.mana.max);
        result.morale = Between(settings.morale.min, settings.morale.max);
        result.luck = Between(settings.luck.min, settings.luck.max);
        result.artifactCount = Between(settings.artifacts.min, settings.artifacts.max);
        for (auto &value : result.primary)
            value = Between(settings.primary.min, settings.primary.max);

        result.secondaryCount = Between(settings.secondary.min, settings.secondary.max);
        std::array<int, 28> skills{};
        for (int i = 0; i < 28; ++i)
            skills[i] = i;
        for (int i = 0; i < result.secondaryCount; ++i)
        {
            const int selected = Between(i, 27);
            std::swap(skills[i], skills[selected]);
            result.secondary[skills[i]] = Between(1, 3);
            result.secondaryOrder[skills[i]] = i + 1;
        }
        for (auto &spell : result.spells)
            spell = Chance(settings.spellChance);

        const int stacks = Between(settings.stacks.min, settings.stacks.max);
        for (int i = 0; i < stacks; ++i)
        {
            result.creatures[i] = creaturePool[Between(0, static_cast<int>(creaturePool.size()) - 1)];
            result.counts[i] = Between(settings.count.min, settings.count.max);
        }
        return result;
    }
};
} // namespace randomcombat
