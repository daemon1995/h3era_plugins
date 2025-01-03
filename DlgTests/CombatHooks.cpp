#include "framework.h"

// this function check is used for NON-tactics stack may act this round check
inline BOOL CombatCreatureMayAct(const H3CombatCreature& combatStack)
{
    return !combatStack.HasMoved()
        && !combatStack.IsWaiting()
        && !combatStack.isDone
        && !combatStack.IsIncapacitated()
        && combatStack.type != eCreature::AMMO_CART;
}

// this function check is used for tactics-ONLY stack may act this round check
inline BOOL CombatCreatureMayActWhileTactics(const H3CombatCreature& combatStack)
{
    return !combatStack.HasMoved()
        && !combatStack.IsWaiting()
        && !combatStack.IsSiege()
        && combatStack.info.speed
        && !combatStack.isDone
        && !combatStack.IsIncapacitated()
        && combatStack.type != eCreature::AMMO_CART;
}

// Note!
// origingal game iterates till the hero creatures by side number and not tghrought the all stacks
BOOL8 CombatManager::FindNextStackTurn(const BOOL isNotStartBattle)
{

    constexpr INT32 SIDE_ATTACKER = 0;
    constexpr INT32 SIDE_DEFENDER = 1;
    const int currentMonIndex = this->currentMonIndex;

    if (currentMonIndex >= 0
        && this->currentMonSide == SIDE_ATTACKER
        && this->stacks[SIDE_ATTACKER][currentMonIndex].type == eCreature::CATAPULT) {
        this->currentMonSide = SIDE_DEFENDER;
        this->currentMonIndex = 0;
    }

    constexpr INT32 sidesNum = sizeof(this->heroMonCount) / sizeof(this->heroMonCount[0]);

    // morale /fear check can't be used at combat start
    BOOL allowFearMoraleCheck = isNotStartBattle;

    const INT32 tacticsSide = this->tacticsDifference < 0;

    H3CombatCreature* finalStackToAct = nullptr;

    do {

        finalStackToAct = nullptr;

        if (this->tacticsPhase) {
            // iterate only tactics side stacks and find the most fast
            for (size_t stackId = 0; stackId < this->heroMonCount[tacticsSide]; ++stackId) {
                H3CombatCreature& assumedStackToAct = this->stacks[tacticsSide][stackId];

                if (CombatCreatureMayActWhileTactics(assumedStackToAct)) {
                    if (!finalStackToAct || !StackHasTurnPriorityOverAnother(*finalStackToAct, assumedStackToAct)) {

                        finalStackToAct = &assumedStackToAct;
                    }
                }
            }
        } else {

            for (size_t side = 0; side < sidesNum; ++side) {
                for (size_t stackId = 0; stackId < this->heroMonCount[side]; ++stackId) {
                    H3CombatCreature& assumedStackToAct = this->stacks[side][stackId];

                    if (CombatCreatureMayAct(assumedStackToAct)) {
                        if (!finalStackToAct || !StackHasTurnPriorityOverAnother(*finalStackToAct, assumedStackToAct)) {

                            finalStackToAct = &assumedStackToAct;
                        }
                    }
                }
            }
        }

        if (!finalStackToAct) {

            if (this->waitPhase)
                return false;

            this->waitPhase = true;
            allowFearMoraleCheck = false;

            // clear previously set "waiting" status so game may check them again
            // probably here we can have bug with resurrection / summon at waiting phase?
            for (size_t side = 0; side < sidesNum; ++side) {
                for (size_t stackId = 0; stackId < this->heroMonCount[side]; ++stackId) {
                    this->stacks[side][stackId].info.waiting = false;
                }
            }

        } else {

            if (!this->waitPhase) {
                finalStackToAct->RegeneratePhase(); // 0x446B50 non-const

                // not allowed at combat start (first stack move) mb for artifacts auto spell casting?
                if (allowFearMoraleCheck
                    && !this->tacticsPhase
                    && (!CheckBadMorale(finalStackToAct->side, finalStackToAct->sideIndex)
                        || !CheckFear(finalStackToAct))) {
                    continue;
                }
            }

            SelectNewMonsterToAct(finalStackToAct->side, finalStackToAct->sideIndex);
            return true;
        }

    } while (true);

    return false;
}