#pragma once

class CombatManager : public H3CombatManager
{
  private:
    BOOL8 FindNextStackTurn(const BOOL isNotStartBattle); // 00464C60
    BOOL8 StackHasTurnPriorityOverAnother(const H3CombatCreature &first, const H3CombatCreature &second) const noexcept;
    BOOL CheckBadMorale(const INT side, const INT indexBySide);
    BOOL8 CheckFear(H3CombatCreature *stack);
    void SelectNewMonsterToAct(const INT side, const INT indexBySide);
};
