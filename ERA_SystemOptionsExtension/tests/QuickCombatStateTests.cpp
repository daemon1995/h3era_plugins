#include "../QuickCombatState.h"
#include <cassert>
#include <cstdio>

using cmbsttngs::QuickCombatState;

int main()
{
    // Q and instant auto-combat use the same transition for every option pair.
    for (int initialQuick = 0; initialQuick <= 1; ++initialQuick)
        for (int initialSpells = 0; initialSpells <= 1; ++initialSpells)
        {
            QuickCombatState state;
            int quick = initialQuick;
            int spells = initialSpells;
            state.FinishInstantly(quick, spells);
            assert(quick == 1 && spells == 0);
            state.FinishInstantly(quick, spells);
            state.RestoreInstantFinish(quick, spells);
            assert(quick == initialQuick && spells == initialSpells);

            // The end-battle hook and ERA event can both restore the same battle.
            quick = 1 - initialQuick;
            spells = 1 - initialSpells;
            state.Restore(quick, spells);
            assert(quick == 1 - initialQuick && spells == 1 - initialSpells);
        }

    {
        // An already disabled spell option is still reserved by instant finish.
        QuickCombatState state;
        int quick = 0;
        int spells = 0;
        state.FinishInstantly(quick, spells);
        state.SetPersistentAutoSpells(spells, 1);
        assert(spells == 0 && state.PersistentAutoSpells(spells) == 1);
        state.FinishInstantly(quick, spells);
        state.RestoreInstantFinish(quick, spells);
        assert(quick == 0 && spells == 1);
    }
    {
        // A matching battle-type override must also defer API edits until restore.
        QuickCombatState state;
        int quick = 1;
        int spells = 0;
        state.SetAutoSpells(spells, 0);
        state.SetPersistentAutoSpells(spells, 1);
        assert(spells == 0 && state.PersistentAutoSpells(spells) == 1);
        state.RestoreInstantFinish(quick, spells);
        assert(spells == 0);
        state.Restore(quick, spells);
        assert(quick == 1 && spells == 1);
    }
    {
        // Ordinary battle-type overrides remain active through the result
        // dialog and are restored only by OnAfterBattle or FastQuit.
        QuickCombatState state;
        int quick = 1;
        int spells = 1;
        state.SetQuickCombat(quick, 0);
        state.SetAutoSpells(spells, 0);
        state.RestoreInstantFinish(quick, spells);
        assert(quick == 0 && spells == 0);
        state.Restore(quick, spells);
        assert(quick == 1 && spells == 1);
    }

    {
        // "Ask" selects manual battle, then Q finishes it. Keep the configured
        // battle type instead of replacing its snapshot with the manual state.
        QuickCombatState state;
        int quick = 1;
        int spells = 1;
        state.SetQuickCombat(quick, 0);
        state.FinishInstantly(quick, spells);
        state.Restore(quick, spells);
        assert(quick == 1 && spells == 1);
    }

    {
        // A battle-type override and quick finish must share the first snapshot
        // even when the second action sets an option back to its original value.
        QuickCombatState state;
        int quick = 1;
        int spells = 0;
        state.SetAutoSpells(spells, 1);
        state.FinishInstantly(quick, spells);
        state.Restore(quick, spells);
        assert(quick == 1 && spells == 0);
    }

    {
        // Preserve edits to options that battle-type selection did not override.
        QuickCombatState state;
        int quick = 1;
        int spells = 1;
        state.SetQuickCombat(quick, 0);
        spells = 0;
        state.Restore(quick, spells);
        assert(quick == 1 && spells == 0);
    }

    {
        // Snapshot changes made in the settings dialog before the quick finish.
        QuickCombatState state;
        int quick = 0;
        int spells = 0;
        spells = 1;
        state.FinishInstantly(quick, spells);
        state.Restore(quick, spells);
        assert(quick == 0 && spells == 1);
    }

    {
        // Reset snapshots between battles, including cleanup through FastQuit.
        QuickCombatState state;
        int quick = 0;
        int spells = 1;
        state.FinishInstantly(quick, spells);
        state.Restore(quick, spells);
        quick = 1;
        spells = 0;
        state.SetQuickCombat(quick, 0);
        state.SetAutoSpells(spells, 1);
        state.Restore(quick, spells);
        assert(quick == 1 && spells == 0);
    }

    {
        QuickCombatState state;
        int quick = 0;
        int spells = 1;
        state.FinishInstantly(quick, spells);
        assert(spells == 0 && state.PersistentAutoSpells(spells) == 1);
        // Saving another setting must keep the user's spells setting.
        state.RestoreInstantFinish(quick, spells);
        assert(spells == 1 && state.PersistentAutoSpells(spells) == 1);
    }
    {
        QuickCombatState state;
        int quick = 1;
        int spells = 0;
        state.SetAutoSpells(spells, 1);
        assert(spells == 1 && state.PersistentAutoSpells(spells) == 0);
        // API edits change the persistent value without changing the running battle.
        state.SetPersistentAutoSpells(spells, 1);
        assert(spells == 1 && state.PersistentAutoSpells(spells) == 1);
        state.FinishInstantly(quick, spells);
        assert(spells == 0 && state.PersistentAutoSpells(spells) == 1);
        state.SetPersistentAutoSpells(spells, 0);
        state.Restore(quick, spells);
        assert(spells == 0 && state.PersistentAutoSpells(spells) == 0);
        state.SetPersistentAutoSpells(spells, 1);
        assert(spells == 1);
    }
    std::puts("QuickCombatState tests passed.");
}
