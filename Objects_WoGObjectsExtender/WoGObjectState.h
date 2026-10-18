#pragma once
#include "framework.h"
#include <string>

namespace wog { namespace state {

int Day() noexcept;       // 1-based absolute day
int Week() noexcept;      // 1-based absolute week
int Month() noexcept;     // 1-based absolute month
H3Point Position(const H3MapItem *item) noexcept;

std::string ObjectKey(const H3MapItem *item, int subtype, const char *field, int player = -1);
std::string HeroKey(int subtype, int heroId, const char *field);
std::string PlayerKey(int subtype, int player, const char *field);

int Get(const std::string &key, int defaultValue = 0) noexcept;
void Set(const std::string &key, int value) noexcept;
bool IsStamp(const std::string &key, int stamp) noexcept;
void Stamp(const std::string &key, int stamp) noexcept;

bool HeroEver(int subtype, int heroId, const char *field = "visited") noexcept;
void SetHeroEver(int subtype, int heroId, const char *field = "visited") noexcept;
bool ObjectPlayerPeriod(const H3MapItem *item, int subtype, int player, const char *field, int period) noexcept;
void SetObjectPlayerPeriod(const H3MapItem *item, int subtype, int player, const char *field, int period) noexcept;

}} // namespace wog::state
