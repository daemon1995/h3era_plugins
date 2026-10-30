#pragma once
#include "framework.h"
#include <string>
#include <vector>

namespace wog { namespace util {

std::string Tr(const std::string &suffix, const std::vector<std::string> &parameters = {});
std::string ObjectName(int subtype);
void SetHintText(const std::string &text) noexcept;
int ChoosePictures(const char *text, const H3PictureVector &pictures);
bool AddDialogBackground(H3Dlg &dialog,int width,int height,bool statusBar=false);
int Random(int minValue, int maxValue) noexcept;
// Private WoG queries write to ?i^wog_objects.native.bridge.result^; nested calls preserve it.
int ReadReceiver(const std::string &command, int initial = 0);
void Receiver(const std::string &command);
int Resource(int player, int resource);
bool ChangeResource(H3Hero *hero, eResource resource, int amount);
bool GiveExperience(H3Hero *hero, int amount);
bool GivePrimary(H3Hero *hero, int skill, int amount);
bool LearnSkill(H3Hero *hero, int skill, int level);
bool LearnSpell(H3Hero *hero, int spell);
bool ConfirmPictures(const char *text, const H3PictureVector &pictures);
int StackAt(int hex) noexcept;
void CastCreatureSpell(H3CombatCreature *caster, int spell, int hex, int school, int power);
void CastHeroSpell(int side, int spell, int hex, int school);
bool Fight(H3Hero *hero, H3Position position, int creature, int count);
bool TacticsEnabled(const H3Hero *hero) noexcept;
void SetTactics(H3Hero *hero, bool enabled) noexcept;
void ClearObstacles();
void PlaceObstacle(int object, int hex);
void PlaceBattleStack(H3CombatCreature *stack, int hex);
void RevealArea(int x, int y, int z, int player, int radius, bool reveal = true);
int CleanPrimary(const H3Hero *hero, int skill);

bool ValidCreature(int id) noexcept;
bool ValidArtifact(int id) noexcept;
bool ArtifactEnabled(int id) noexcept;
int CreatureGoldCost(int id) noexcept;
long long ArmyAiValue(const H3Army &army) noexcept;
bool CanAddArmy(const H3Army &army, const std::vector<std::pair<int, int>> &stacks) noexcept;
bool AddArmy(H3Army &army, const std::vector<std::pair<int, int>> &stacks) noexcept;
bool TransferArmyStack(H3Army &source, int slot, H3Army &destination) noexcept;
void TransformArmyStack(H3Hero *hero, int slot, int creature) noexcept;

bool RemoveBackpackArtifact(H3Hero *hero, int artifactId, int subtype = -1) noexcept;
int CountBackpackArtifact(const H3Hero *hero, int artifactId, int subtype = -1) noexcept;

int ChoosePrimary(const char *text, int excluded = -1);
int ChooseArmySlot(const H3Hero *hero, const char *text, bool allowCancel = true);
int ChooseTown(const char *text, int preferred = -1); // 0..8 town, -1 neutral, -2 cancel

int NativeTownCreature(int town, int level, bool upgraded) noexcept;
int FirstCreatureAtOrBelowFightValue(int town, int level, int maxFightValue) noexcept;

bool IsLegacyEnabled(const H3MapItem *item, int optionId) noexcept;
bool OptionEnabled(int optionId) noexcept;

}} // namespace wog::util
