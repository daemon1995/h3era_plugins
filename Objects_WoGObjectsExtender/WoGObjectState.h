#pragma once
#include "framework.h"
#include "WoGObjectSetup.h"
#include <string>
#include <utility>

namespace wog
{
namespace state
{

int Day() noexcept;   // 1-based absolute day
int Week() noexcept;  // 1-based absolute week
int Month() noexcept; // 1-based absolute month
int MapRandomSeed() noexcept;
void ResetMapRandomSeed() noexcept;
H3Point Position(const H3MapItem *item) noexcept;

// Visits can receive a temporary map-item copy (not an address inside the map array).
// Preserve the API-supplied position throughout nested dialogs and AI evaluation.
class ObjectPositionScope
{
    const H3MapItem *previousItem;
    H3Point previousPosition;

  public:
    ObjectPositionScope(const H3MapItem *item, H3Position position) noexcept;
    ~ObjectPositionScope();
};

std::string ObjectKey(const H3MapItem *item, int subtype, const char *field, int player = -1);
std::string HeroKey(int subtype, int heroId, const char *field);
std::string PlayerKey(int subtype, int player, const char *field);

int Get(const std::string &key, int defaultValue = 0) noexcept;
void Set(const std::string &key, int value) noexcept;
// A write-through value: ERA owns storage and serializes it with the current game.
// Never keep a cached reference across a load, a dialog or an ERM callback.
class IntValue
{
    std::string key;
    int fallback;

  public:
    explicit IntValue(std::string name, int initial = 0) : key(std::move(name)), fallback(initial)
    {
    }
    operator int() const
    {
        return Get(key, fallback);
    }
    IntValue &operator=(int value)
    {
        Set(key, value);
        return *this;
    }
    IntValue &operator=(const IntValue &other)
    {
        return *this = int(other);
    }
    int operator++()
    {
        const int value = int(*this) + 1;
        *this = value;
        return value;
    }
    int operator--()
    {
        const int value = int(*this) - 1;
        *this = value;
        return value;
    }
    IntValue &operator|=(int value)
    {
        return *this = (int(*this) | value);
    }
};
bool IsStamp(const std::string &key, int stamp) noexcept;
void Stamp(const std::string &key, int stamp) noexcept;

bool HeroEver(int subtype, int heroId, const char *field = "visited") noexcept;
void SetHeroEver(int subtype, int heroId, const char *field = "visited") noexcept;
void ResetHero(int heroId) noexcept;
void StoreSetup(H3MapItem *item, uint32_t word, H3Hero *hero = nullptr) noexcept;
void SetObjectAsVisited(H3MapItem *item, int player, H3Hero *hero = nullptr) noexcept;
bool ObjectDay(const H3MapItem *item, int day) noexcept;
void SetObjectDay(H3MapItem *item, int day, H3Hero *hero) noexcept;
bool ObjectPlayerPeriod(const H3MapItem *item, int subtype, int player, const char *field, int period) noexcept;
void SetObjectPlayerPeriod(H3MapItem *item, int subtype, int player, const char *field, int period,
                           H3Hero *hero = nullptr) noexcept;

} // namespace state
} // namespace wog
