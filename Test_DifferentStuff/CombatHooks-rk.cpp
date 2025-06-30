#include "framework.h"

/*
 * would recommend lambda functions unless it's something that wouid be repeated elsewhere
 */
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

/*
 * That's the normal way of iterating over stacks, because they don't properly "kill" unused stacks
 */
// Note!
// origingal game iterates till the hero creatures by side number and not tghrought the all stacks
BOOL8 CombatManager::FindNextStackTurn(const BOOL isNotStartBattle)
{

    constexpr INT32 SIDE_ATTACKER = 0;
    constexpr INT32 SIDE_DEFENDER = 1;

    /*
     * not a useful variable, let the compiler handle this optimization if need be
     */
    const int currentMonIndex = this->currentMonIndex;

    if (currentMonIndex >= 0
        && this->currentMonSide == SIDE_ATTACKER
        && this->stacks[SIDE_ATTACKER][currentMonIndex].type == eCreature::CATAPULT) {
        this->currentMonSide = SIDE_DEFENDER;
        this->currentMonIndex = 0;
    }
    
    /*
     * use std::size() instead of C pattern (this is a defined constant)
     */    
    constexpr INT32 sidesNum = sizeof(this->heroMonCount) / sizeof(this->heroMonCount[0]);

    /* 
     * don't create a new variable unless needed 
     * this comment is not needed, if you name the function parameter allowFearMoraleCheck instead
     */
    // morale /fear check can't be used at combat start
    BOOL allowFearMoraleCheck = isNotStartBattle;

    const INT32 tacticsSide = this->tacticsDifference < 0;

    H3CombatCreature* finalStackToAct = nullptr;

    /*
     * you removed the interesting part here:
     *
     *      // got rid of that part
     *      // INT32 nextWaitPhaseId = static_cast<bool>(this->waitPhase) + 1;
     *
     *      // probably protection from other function that increment waitPhase
     *      // this code makes no sense
     *      // never can be "true"; see 0x464CAF
     *      if (nextWaitPhaseId > 2)
     *      {
     *              return false;
     *      }
     *
     * In my opinion this transforms the do() while () into something like
     *
     *      const int numPhases = this->waitPhase ? 2 : 1;
     *  
     *      for (int currentPhase = 1; currentPhase <= numPhases; ++currentPhase) {
     *          ... etc
     *      }
     *  
     *      return nullptr;
     *
     * They could have done [0..2[ instead of [1..2], not clear why not.     
     */
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