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
        int temporaryValue = 0;
        bool active = false;

        void Set(int &option, const int value) noexcept
        {
            // Reserve the temporary value even when it already matches. API
            // edits must affect the restored setting, not the running battle.
            if (!active)
            {
                originalValue = option;
                active = true;
            }
            temporaryValue = value;
            option = value;
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

        int RuntimeValue(const int option) const noexcept
        {
            return active ? temporaryValue : option;
        }

        void SetPersistentValue(int &option, const int value) noexcept
        {
            if (active)
                originalValue = value;
            else
                option = value;
        }

        void ExposePersistentValue(int &option) const noexcept
        {
            option = PersistentValue(option);
        }

        void ResumeTemporaryValue(int &option) const noexcept
        {
            if (active)
                option = temporaryValue;
        }
    };

    OptionOverride quickCombat;
    OptionOverride autoSpells;
    bool instantFinish = false;
    bool quickCombatBeforeFinishActive = false;
    bool autoSpellsBeforeFinishActive = false;
    int quickCombatBeforeFinish = 0;
    int autoSpellsBeforeFinish = 0;
    unsigned int settingsWriteDepth = 0;

  public:
    int PersistentQuickCombat(const int option) const noexcept
    {
        return quickCombat.PersistentValue(option);
    }

    void SetPersistentQuickCombat(int &option, const int value) noexcept
    {
        quickCombat.SetPersistentValue(option, value);
        if (settingsWriteDepth)
            quickCombat.ExposePersistentValue(option);
    }

    int PersistentAutoSpells(const int option) const noexcept
    {
        return autoSpells.PersistentValue(option);
    }

    void SetPersistentAutoSpells(int &option, const int value) noexcept
    {
        autoSpells.SetPersistentValue(option, value);
        if (settingsWriteDepth)
            autoSpells.ExposePersistentValue(option);
    }

    void SetQuickCombat(int &option, const int temporaryValue) noexcept
    {
        quickCombat.Set(option, temporaryValue);
        if (settingsWriteDepth)
            quickCombat.ExposePersistentValue(option);
    }

    void SetAutoSpells(int &option, const int temporaryValue) noexcept
    {
        autoSpells.Set(option, temporaryValue);
        if (settingsWriteDepth)
            autoSpells.ExposePersistentValue(option);
    }

    // The native writer reads the game's globals directly. Expose configured
    // values for the whole call, then resume only overrides that still exist.
    void BeginSettingsWrite(int &quickCombatOption, int &autoSpellsOption) noexcept
    {
        ++settingsWriteDepth;
        quickCombat.ExposePersistentValue(quickCombatOption);
        autoSpells.ExposePersistentValue(autoSpellsOption);
    }

    void EndSettingsWrite(int &quickCombatOption, int &autoSpellsOption) noexcept
    {
        if (settingsWriteDepth && --settingsWriteDepth == 0)
        {
            quickCombat.ResumeTemporaryValue(quickCombatOption);
            autoSpells.ResumeTemporaryValue(autoSpellsOption);
        }
    }

    void FinishInstantly(int &quickCombatOption, int &autoSpellsOption) noexcept
    {
        if (!instantFinish)
        {
            quickCombatBeforeFinishActive = quickCombat.active;
            autoSpellsBeforeFinishActive = autoSpells.active;
            quickCombatBeforeFinish = quickCombat.RuntimeValue(quickCombatOption);
            autoSpellsBeforeFinish = autoSpells.RuntimeValue(autoSpellsOption);
        }
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
        {
            // Q/A is a second override inside the battle. Keep the selected
            // battle mode until OnAfterBattle, and keep API edits to its backup.
            if (quickCombatBeforeFinishActive)
                SetQuickCombat(quickCombatOption, quickCombatBeforeFinish);
            else
                quickCombat.Restore(quickCombatOption);
            if (autoSpellsBeforeFinishActive)
                SetAutoSpells(autoSpellsOption, autoSpellsBeforeFinish);
            else
                autoSpells.Restore(autoSpellsOption);
            instantFinish = false;
        }
    }
};
} // namespace cmbsttngs
