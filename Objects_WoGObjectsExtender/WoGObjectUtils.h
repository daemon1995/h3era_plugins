#pragma once
#include "framework.h"
#include <string>
#include <vector>

namespace wog { namespace util {

std::string Tr(const std::string &suffix, const std::vector<std::string> &parameters = {});
std::string ObjectName(int subtype);
void SetHintText(const std::string &text) noexcept;
int ChoosePictures(const char *text, const H3PictureVector &pictures);
int Random(int minValue, int maxValue) noexcept;

bool ValidCreature(int id) noexcept;
bool ValidArtifact(int id) noexcept;
bool ArtifactEnabled(int id) noexcept;
int CreatureGoldCost(int id) noexcept;
long long ArmyAiValue(const H3Army &army) noexcept;
bool CanAddArmy(const H3Army &army, const std::vector<std::pair<int, int>> &stacks) noexcept;
bool AddArmy(H3Army &army, const std::vector<std::pair<int, int>> &stacks) noexcept;

bool RemoveBackpackArtifact(H3Hero *hero, int artifactId, int subtype = -1) noexcept;
int CountBackpackArtifact(const H3Hero *hero, int artifactId, int subtype = -1) noexcept;

int ChoosePrimary(const char *text, int excluded = -1);
int ChooseArmySlot(const H3Hero *hero, const char *text, bool allowCancel = true);
int ChooseTown(const char *text, int preferred = -1); // 0..8 town, -1 neutral, -2 cancel

int NativeTownCreature(int town, int level, bool upgraded) noexcept;
int FirstCreatureAtOrBelowFightValue(int town, int level, int maxFightValue) noexcept;

bool IsLegacyEnabled(const H3MapItem *item, int optionId) noexcept;

}} // namespace wog::util
