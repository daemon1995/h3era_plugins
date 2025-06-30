#include "framework.h"
// Note!
// origingal game iterates till the hero creatures by side number and not tghrought the all stacks
// allowFearMoraleCheck is 1 only at the combat start and works only for the first stack

BOOL8 CombatManager::FindNextStackTurn(BOOL allowFearMoraleCheck) noexcept
{

    constexpr INT32 SIDE_ATTACKER = 0;
    constexpr INT32 SIDE_DEFENDER = 1;

    if (this->currentMonIndex >= 0
        && this->currentMonSide == SIDE_ATTACKER
        && this->stacks[SIDE_ATTACKER][this->currentMonIndex].type == eCreature::CATAPULT) {
        this->currentMonSide = SIDE_DEFENDER;
        this->currentMonIndex = 0;
    }

    constexpr size_t sidesNum = std::size(heroMonCount);

    const INT32 tacticsSide = this->tacticsDifference < 0;

    H3CombatCreature* finalStackToAct = nullptr;

    do {

        finalStackToAct = nullptr;

        if (this->tacticsPhase) {
            // iterate only tactics side stacks and find the most fast
            for (size_t stackId = 0; stackId < this->heroMonCount[tacticsSide]; ++stackId) {
                H3CombatCreature& assumedStackToAct = this->stacks[tacticsSide][stackId];

                // this lambda function check is used for tactics-ONLY stack may act this round check
                if ([&]() noexcept -> BOOL { return !assumedStackToAct.HasMoved()
                                                 && !assumedStackToAct.IsWaiting()
                                                 && !assumedStackToAct.IsSiege()
                                                 && assumedStackToAct.info.speed
                                                 && !assumedStackToAct.isDone
                                                 && !assumedStackToAct.IsIncapacitated()
                                                 && assumedStackToAct.type != eCreature::AMMO_CART; }()) {
                    if (!finalStackToAct || !StackHasTurnPriorityOverAnother(*finalStackToAct, assumedStackToAct)) {

                        finalStackToAct = &assumedStackToAct;
                    }
                }
            }
        } else {

            for (size_t side = 0; side < sidesNum; ++side) {
                for (size_t stackId = 0; stackId < this->heroMonCount[side]; ++stackId) {
                    H3CombatCreature& assumedStackToAct = this->stacks[side][stackId];

                    // this lambda function check is used for NON-tactics stack may act this round check
                    if ([&]() noexcept -> BOOL { return !assumedStackToAct.HasMoved()
                                                     && !assumedStackToAct.IsWaiting()
                                                     && !assumedStackToAct.isDone
                                                     && !assumedStackToAct.IsIncapacitated()
                                                     && assumedStackToAct.type != eCreature::AMMO_CART; }()) {
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
