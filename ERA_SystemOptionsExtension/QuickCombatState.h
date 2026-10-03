#pragma once

namespace cmbsttngs
{
// Keep the first value overridden in a battle, even if another quick-combat
// action overrides the same option again before restoration.
struct QuickCombatState
{
  private:
    struct OptionOverride
    {
        int originalValue = 0;
        bool active = false;

        void Set(int &option, const int temporaryValue) noexcept
        {
            // Reserve the temporary value even when it already matches. API
            // edits must affect the restored setting, not the running battle.
            if (!active)
            {
                originalValue = option;
                active = true;
            }
            option = temporaryValue;
        }

        void Restore(int &option) noexcept
        {
            if (active)
            {
                option = originalValue;
                active = false;
            }
        }

        int PersistentValue(const int option) const noexcept
        {
            return active ? originalValue : option;
        }

        void SetPersistentValue(int &option, const int value) noexcept
        {
            if (active)
                originalValue = value;
            else
                option = value;
        }
    };

    OptionOverride quickCombat;
    OptionOverride autoSpells;
    bool instantFinish = false;

  public:
    int PersistentAutoSpells(const int option) const noexcept
    {
        return autoSpells.PersistentValue(option);
    }

    void SetPersistentAutoSpells(int &option, const int value) noexcept
    {
        autoSpells.SetPersistentValue(option, value);
    }

    void SetQuickCombat(int &option, const int temporaryValue) noexcept
    {
        quickCombat.Set(option, temporaryValue);
    }

    void SetAutoSpells(int &option, const int temporaryValue) noexcept
    {
        autoSpells.Set(option, temporaryValue);
    }

    void FinishInstantly(int &quickCombatOption, int &autoSpellsOption) noexcept
    {
        SetQuickCombat(quickCombatOption, 1);
        SetAutoSpells(autoSpellsOption, 0);
        instantFinish = true;
    }

    void Restore(int &quickCombatOption, int &autoSpellsOption) noexcept
    {
        quickCombat.Restore(quickCombatOption);
        autoSpells.Restore(autoSpellsOption);
        instantFinish = false;
    }

    void RestoreInstantFinish(int &quickCombatOption, int &autoSpellsOption) noexcept
    {
        if (instantFinish)
            Restore(quickCombatOption, autoSpellsOption);
    }
};
} // namespace cmbsttngs
