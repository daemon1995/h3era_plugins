#pragma once
#include "framework.h"

struct CreatureSpellData
{
    static constexpr DWORD ENCHANTERS_ARRAY_ADDRESS = 0x06608B8;
    static constexpr DWORD ENCHANTERS_ARRAY_PTR_ADDRESS = 0x0447D2B + 1;
    static constexpr size_t ENCHANTERS_ARRAY_SIZE = 8;

    static constexpr DWORD FAERIE_DRAGON_ARRAY_ADDRESS = 0x063B850;
    static constexpr DWORD FAERIE_DRAGON_ARRAY_PTR_ADDRESS = 0x0447251 + 1;
    static constexpr size_t FAERIE_DRAGON_ARRAY_SIZE = 8;

  public:
    eSpell spellId;
    int chanceToCast;

  public:
    static CreatureSpellData *GetFaerieDragonArray()
    {
        return *reinterpret_cast<CreatureSpellData **>(FAERIE_DRAGON_ARRAY_PTR_ADDRESS);
    }
    static CreatureSpellData *GetEnchantersArray()
    {
        return *reinterpret_cast<CreatureSpellData **>(ENCHANTERS_ARRAY_PTR_ADDRESS);
    }

    static BOOL CreateAvailableSpellsList(const H3CombatCreature *creature, std::vector<eSpell> &outList);
};

struct CreaturePrioritySpells
{
    static constexpr LPCSTR iniPath = "plugin_settings.ini";
    static constexpr LPCSTR iniSection = "prioritized_spells";
    std::string iniName;

    H3Ini settingsIni; // = nullptr;
    BOOL settingsLoaded = FALSE;
    struct SpellLists
    {
        eCreature creature = eCreature::UNDEFINED;
        std::vector<eSpell> spells;

        void UseSpell(const eSpell spell)
        {
            // убираем если уже есть
            auto it = std::find(spells.begin(), spells.end(), spell);
            if (it != spells.end())
                spells.erase(it);

            // вставляем в начало
            spells.insert(spells.begin(), spell);
        }
    };
    SpellLists masterGenie = {eCreature::MASTER_GENIE};
    SpellLists faerieDragon = {eCreature::FAERIE_DRAGON};
    SpellLists enchanter = {eCreature::ENCHANTER};

  public:
    CreaturePrioritySpells()
    {
        this->LoadUserSettings();
    }

    ~CreaturePrioritySpells()
    {
    }
    void SaveSpecialist(const SpellLists &spellList);
    void LoadSpecialist(SpellLists &spellList);
    BOOL LoadUserSettings(LPCSTR ini = 0);
    BOOL SaveUserSettings();
};

class CreatureMagicRandom : IGamePatch
{

  public:
    static CreatureMagicRandom *instance;

  public:
    CreaturePrioritySpells creaturePrioritySpells;
    eSpell faerieDragonSpell = eSpell::NONE;

    eSpell userSelectedSpell[2][h3::limits::TOTAL_COMBAT_CREATURES] = {eSpell::NONE};

  private:
    CreatureMagicRandom();
    virtual ~CreatureMagicRandom() {};

  protected:
    virtual void CreatePatches() override;

  public:
    static CreatureMagicRandom &GetInstance();
    static void ClearUserSelectedSpell()
    {
        auto &userSelectedSpell = instance->userSelectedSpell;
        libc::memset(userSelectedSpell, eSpell::NONE, sizeof(userSelectedSpell));
    };
};

class SpellSelectionDlg : H3Dlg
{
  protected:
    static constexpr LPCSTR ITEM_DEF_NAME = "spells.def";

    static constexpr size_t ITEMS_PER_ROW = 4;
    static constexpr size_t ITEMS_MARGIN = 16;
    static constexpr size_t ITEMS_PADDING = 4;
    static constexpr size_t DEFAULT_BUTTON_ID = 30724;

  protected:
    virtual BOOL DialogProc(H3Msg &msg) override;
    virtual VOID OnCancel() override;
    virtual BOOL OnDoubleClick(INT itemID, H3Msg &msg) override;

  protected:
    H3DlgFrame *selectionFrame = nullptr;

    const std::vector<eSpell> &availableSpells;
    eSpell preselectedSpell = eSpell::NONE;
    eSpell selectedSpell = eSpell::NONE;

    SpellSelectionDlg(const H3CombatCreature *creature, const std::vector<eSpell> &availableSpells, const BOOL isPopup,
                      const int width = 400, const int height = 300);

  public:
    static eSpell ShowSpellSelectionDialog(const H3CombatCreature *creature, const H3Msg *msg);
};
