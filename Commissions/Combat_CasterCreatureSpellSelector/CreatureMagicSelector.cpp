#include <unordered_set>

#include "CreatureMagicSelector.h"

CreatureMagicRandom *CreatureMagicRandom::instance = nullptr;

CreatureMagicRandom::CreatureMagicRandom() : IGamePatch(_PI)
{
    CreatePatches();
}

static int GetCretureSpellPower(const H3CombatCreature *creature)
{
    switch (creature->type)
    {
    case eCreature::MASTER_GENIE:
        return 6;
    case eCreature::FAERIE_DRAGON:
        return 5 * creature->numberAlive;
    case eCreature::ENCHANTER:
        return 3;
    default:
        return 0;
    }
    return 0;
}

std::vector<eSpell> Reorder(const std::vector<eSpell> &available, const std::vector<eSpell> &priority)
{
    std::vector<eSpell> result;
    result.reserve(available.size());

    // check if exists
    std::unordered_set<eSpell> used(priority.begin(), priority.end());
    // 1. prioritized
    for (eSpell v : priority)
    {
        if (std::find(available.begin(), available.end(), v) != available.end())
            result.push_back(v);
    }

    // 2. all another
    for (eSpell v : available)
    {
        if (!used.count(v))
            result.push_back(v);
    }

    return result;
}

BOOL CreatureSpellData::CreateAvailableSpellsList(const H3CombatCreature *creature, std::vector<eSpell> &outList)
{
    // check max spell level allowed due to terrain and artifacts
    BOOL maxSpellLevel = 5;

    if (P_CombatManager->specialTerrain == 2)
    {
        maxSpellLevel = 1;
    }
    else
    {
        for (auto &hero : P_CombatManager->hero)
        {
            if (!hero)
                continue;
            if (hero->WearsArtifact(eArtifact::ORB_OF_INHIBITION))
                return FALSE;
            if (hero->WearsArtifact(eArtifact::RECANTERS_CLOAK))
                maxSpellLevel = 2;
        }
    }

    outList.clear();
    outList.reserve(h3::limits::SPELLS);
    const CreatureSpellData *spellDataArray = nullptr;
    std::vector<eSpell> *prioritySpells = nullptr;
    CreaturePrioritySpells &creaturePrioritySpells = CreatureMagicRandom::GetInstance().creaturePrioritySpells;
    switch (creature->type)
    {
    case eCreature::MASTER_GENIE:
        prioritySpells = &creaturePrioritySpells.masterGenie.spells;

        for (size_t spellId = eSpell::QUICK_SAND; spellId < eSpell::STONE; spellId++)
        {
            if (P_Spell[spellId].level > maxSpellLevel)
                continue;

            if (P_Spell[spellId].friendlyMass)
            {
                // if combat isn't vs hero, skip protection spells and anti-magic/magic mirror
                switch (spellId)
                {
                case eSpell::PROTECTION_FROM_AIR:
                case eSpell::PROTECTION_FROM_EARTH:
                case eSpell::PROTECTION_FROM_FIRE:
                case eSpell::PROTECTION_FROM_WATER:
                case eSpell::ANTI_MAGIC:
                case eSpell::MAGIC_MIRROR:
                    if (!P_CombatManager->hero[1])
                        continue;
                default:
                    break;
                }
                outList.push_back(static_cast<eSpell>(spellId));
            }
        }

        break;
    case eCreature::FAERIE_DRAGON:
        prioritySpells = &creaturePrioritySpells.faerieDragon.spells;

        spellDataArray = CreatureSpellData::GetFaerieDragonArray();
        for (size_t i = 0; i < CreatureSpellData::FAERIE_DRAGON_ARRAY_SIZE; i++)
        {
            const eSpell spellId = static_cast<eSpell>(spellDataArray[i].spellId);

            if (P_Spell[spellId].level > maxSpellLevel)
                continue;

            if (spellDataArray[i].chanceToCast > 0)
                outList.push_back(spellId);
        }
        break;
    case eCreature::ENCHANTER:
        prioritySpells = &creaturePrioritySpells.enchanter.spells;

        spellDataArray = CreatureSpellData::GetEnchantersArray();
        for (size_t i = 0; i < CreatureSpellData::ENCHANTERS_ARRAY_SIZE; i++)
        {
            const eSpell spellId = static_cast<eSpell>(spellDataArray[i].spellId);
            if (P_Spell[spellId].level > maxSpellLevel)
                continue;
            outList.push_back(spellId);
        }
        break;
    default:
        return FALSE;
    }

    if (prioritySpells && prioritySpells->size())
    {
    }
    outList = Reorder(outList, *prioritySpells);
    outList.shrink_to_fit();

    return !outList.empty();
}

static eSpell GetUserSelectedSpell(const H3CombatCreature *creature)
{
    if (!creature)
        return eSpell::NONE;

    return CreatureMagicRandom::instance->userSelectedSpell[creature->side][creature->sideIndex];
}

static void __stdcall BattleStack_CastGenieSpell(HiHook *h, H3CombatCreature *creature, const int pos)
{

    // emulate the whole spell casting process
    // cause original function is too complex to patch in parts

    const eSpell setSpellByUser = GetUserSelectedSpell(creature);
    if (setSpellByUser != eSpell::NONE)
    {
        const auto &targetStack = P_CombatManager->squares[pos].GetMonster();
        if (targetStack->CanReceiveSpell(setSpellByUser))
        {
            CreatureMagicRandom::GetInstance().creaturePrioritySpells.masterGenie.UseSpell(setSpellByUser);
            const int spellPower = GetCretureSpellPower(creature);
            P_CombatManager->CastSpell(setSpellByUser, pos, 1, -1, eSecSkillLevel::ADVANCED, spellPower);
            return;
        }
    }

    THISCALL_2(void, h->GetDefaultFunc(), creature, pos);
}

static char __stdcall BattleStack_EnchanterCastsMassSpell(HiHook *h, H3CombatCreature *creature)
{

    // check if user has points
    eSpell setSpellByUser = GetUserSelectedSpell(creature);

    if (setSpellByUser == eSpell::NONE)
    {
        H3Msg msg;
        msg.command = eMsgCommand::MOUSE_BUTTON;
        msg.subtype = eMsgSubtype::LBUTTON_CLICK;
        setSpellByUser = SpellSelectionDlg::ShowSpellSelectionDialog(creature, &msg);
    }

    if (setSpellByUser != eSpell::NONE)
    {
        CreatureSpellData *data = CreatureSpellData::GetEnchantersArray();
        const CreatureSpellData storedData = data[0];

        // emulate first spell in array being the one selected by user
        data[0].spellId = setSpellByUser; // setSpellByUser;
        data[0].chanceToCast = INT32_MAX;

        // call pseudo original function to check if spell can be cast
        char ret = creature->UseEnchanters();
        CreatureMagicRandom::GetInstance().creaturePrioritySpells.enchanter.UseSpell(setSpellByUser);
        data[0] = storedData;
        // if spell can be cast return success
        if (ret)
        {
            return true;
        }
    }
    return THISCALL_1(char, h->GetDefaultFunc(), creature);
}
eSpell GetOriginalFaerieDragonSpellToCast()
{
    return CreatureMagicRandom::GetInstance().faerieDragonSpell;
}
static _LHF_(BattleStack_PrepareFaerieDragonSpell)
{
    const auto &creature = reinterpret_cast<H3CombatCreature *>(c->edi);
    const eSpell setSpellByUser = GetUserSelectedSpell(creature);
    CreatureMagicRandom::GetInstance().faerieDragonSpell = eSpell(c->eax);

    // if user set spell then replace and store original one
    if (setSpellByUser != eSpell::NONE)
    {
        c->eax = setSpellByUser;
        CreatureMagicRandom::GetInstance().faerieDragonSpell = eSpell::NONE;
    }

    return EXEC_DEFAULT;
}

void __stdcall BattleMgr_ShowMonStatDlg(HiHook *hook, H3CombatManager *mgr, H3CombatCreature *creature,
                                        BOOL isRightClick)
{

    const bool displayDlg = STDCALL_1(SHORT, PtrAt(0x63A294), VK_CONTROL) & 0x800 && !isRightClick;
    if (displayDlg)
    {
        H3Msg msg;
        msg.command = eMsgCommand::MOUSE_BUTTON;
        msg.subtype = eMsgSubtype::LBUTTON_CLICK;
        auto selectedSpell = SpellSelectionDlg::ShowSpellSelectionDialog(creature, &msg);
        CreatureMagicRandom::instance->userSelectedSpell[creature->side][creature->sideIndex] = selectedSpell;
        if (mgr->activeStack == creature && selectedSpell != eSpell::NONE && creature->type == eCreature::FAERIE_DRAGON)
        {
            creature->faerieDragonSpell = selectedSpell;
        }
    }
    else
    {
        THISCALL_3(void, hook->GetDefaultFunc(), mgr, creature, isRightClick);
    }
}

static _LHF_(BattleStack_CastFaerieDragonSpell)
{
    auto *creature = reinterpret_cast<H3CombatCreature *>(c->esi);
    const eSpell setSpellByUser = GetUserSelectedSpell(creature);
    if (setSpellByUser != eSpell::NONE)
    {
        CreatureMagicRandom::GetInstance().creaturePrioritySpells.faerieDragon.UseSpell(setSpellByUser);
        creature->faerieDragonSpell = setSpellByUser;
    }

    return EXEC_DEFAULT;
}
_ERH_(OnSetupBattlefield)
{
    CreatureMagicRandom::ClearUserSelectedSpell();
}
_ERH_(OnAfterBattleUniversal)
{
    CreatureMagicRandom::ClearUserSelectedSpell();
}

_LHF_(H3CreatureInfoDlg__Battle__Create)
{
    H3CombatCreature *creature = reinterpret_cast<H3CombatCreature *>(c->edi);
    if (GetCretureSpellPower(creature) > 0)
    {
        c->return_address = 0x005F3D5D;
        return NO_EXEC_DEFAULT;
    }

    return EXEC_DEFAULT;
}
_LHF_(H3CreatureInfoDlg__Battle__AfterClose)
{
    H3CombatCreature *creature = *reinterpret_cast<H3CombatCreature **>(c->ebp + 0x8);
    if (P_WindowManager->resultItemID == 302)
    {

        H3Msg msg;
        msg.command = eMsgCommand::MOUSE_BUTTON;
        msg.subtype = eMsgSubtype::LBUTTON_CLICK;
        auto selectedSpell = SpellSelectionDlg::ShowSpellSelectionDialog(creature, &msg);
        CreatureMagicRandom::instance->userSelectedSpell[creature->side][creature->sideIndex] = selectedSpell;

        if (P_CombatManager->activeStack == creature && selectedSpell != eSpell::NONE &&
            creature->type == eCreature::FAERIE_DRAGON)
        {
            creature->faerieDragonSpell = selectedSpell;
        }

        c->return_address = 0x0468546;
        return NO_EXEC_DEFAULT;
    }

    return EXEC_DEFAULT;
}
void CreatureMagicRandom::CreatePatches()
{

    if (m_isInited)
        return;

    m_isInited = true;
    // casting creature spells
    WriteHiHook(0x0448357, THISCALL_, BattleStack_CastGenieSpell);
    WriteHiHook(0x04650F9, THISCALL_, BattleStack_EnchanterCastsMassSpell);
    WriteLoHook(0x04472A8, BattleStack_PrepareFaerieDragonSpell);

    WriteLoHook(0x044837B, BattleStack_CastFaerieDragonSpell);
    WriteHiHook(0x468440, THISCALL_, BattleMgr_ShowMonStatDlg);
    WriteLoHook(0x05F3D57, H3CreatureInfoDlg__Battle__Create);
    WriteLoHook(0x0468515, H3CreatureInfoDlg__Battle__AfterClose);
    IntAt(0x05F53FB + 3) = 302; // set new callback button result
    _REH_(OnSetupBattlefield);
    _REH_(OnAfterBattleUniversal);
}

BOOL CreaturePrioritySpells::LoadUserSettings(LPCSTR _nni)
{

    iniName = h3_NullString;

    LPCSTR ini = iniName.c_str();

    if (settingsIni.Open(ini))
    {
        LoadSpecialist(masterGenie);
        LoadSpecialist(faerieDragon);
        LoadSpecialist(enchanter);
        settingsLoaded = true;
    }

    return 0;
}
static std::unordered_set<eSpell> GetUniqueSpells(const eCreature creature)
{

    std::unordered_set<eSpell> uniqueSpells;
    switch (creature)
    {
    case eCreature::MASTER_GENIE:
        for (size_t i = eSpell::QUICK_SAND; i < eSpell::STONE; i++)
        {
            if (P_Spell[i].friendlyMass)
            {
                uniqueSpells.insert(static_cast<eSpell>(i));
            }
        }
        break;
    case eCreature::FAERIE_DRAGON:
        for (size_t i = 0; i < CreatureSpellData::FAERIE_DRAGON_ARRAY_SIZE; i++)
        {
            uniqueSpells.insert(static_cast<eSpell>(CreatureSpellData::GetFaerieDragonArray()[i].spellId));
        }
        break;
    case eCreature::ENCHANTER:
        for (size_t i = 0; i < CreatureSpellData::ENCHANTERS_ARRAY_SIZE; i++)
        {
            uniqueSpells.insert(static_cast<eSpell>(CreatureSpellData::GetEnchantersArray()[i].spellId));
        }
        break;
    default:
        break;
    }
    return uniqueSpells;
}

std::vector<eSpell> Deserialize(const std::string &str, const eCreature creature)
{
    std::vector<eSpell> result;
    auto uniqueSpells = GetUniqueSpells(creature);

    if (uniqueSpells.empty())
        return result;

    size_t start = 0;

    while (start < str.size())
    {
        size_t end = str.find(',', start);

        std::string token = str.substr(start, end == std::string::npos ? std::string::npos : end - start);

        eSpell v = static_cast<eSpell>(std::stoi(token));
        if (uniqueSpells.find(v) == uniqueSpells.end())
        {
            // invalid spell id, skip
            start = end == std::string::npos ? std::string::npos : end + 1;
            continue;
        }
        // защита от дублей
        if (std::find(result.begin(), result.end(), v) == result.end())
            result.push_back(v);

        if (end == std::string::npos)
            break;

        start = end + 1;
    }

    return result;
}

static std::string Serialize(const std::vector<eSpell> &v)
{
    std::string s = " ";
    for (size_t i = 0; i < v.size(); ++i)
    {
        if (i)
            s += ',';
        s += std::to_string(v[i]);
    }
    return s;
}
void CreaturePrioritySpells::SaveSpecialist(const SpellLists &spellList)
{

    std::string s = Serialize(spellList.spells);
    auto &section = settingsIni.Get(iniSection);

    auto &line = section->Get(std::to_string(spellList.creature).c_str());

    H3String strr = s.c_str();
    line->SetString(strr);
}
void CreaturePrioritySpells::LoadSpecialist(SpellLists &spellList)
{
    auto &section = settingsIni.Get(iniSection);
    std::string iniStr = section->GetString(std::to_string(spellList.creature).c_str(), h3_NullString).String();
    spellList.spells = Deserialize(iniStr, spellList.creature);
}

BOOL SaveCreaturePrioritySpells()
{
    return CreatureMagicRandom::GetInstance().creaturePrioritySpells.SaveUserSettings();
}

BOOL CreaturePrioritySpells::SaveUserSettings()
{
    if (settingsLoaded)
    {
        SaveSpecialist(masterGenie);
        SaveSpecialist(enchanter);
        SaveSpecialist(faerieDragon);

        LPCSTR ini = iniName.c_str();
        return settingsIni.Save(ini, 0);
    }

    return false;
}

CreatureMagicRandom &CreatureMagicRandom::GetInstance()
{
    if (!instance)
    {
        instance = new CreatureMagicRandom();
    }

    return *instance;
}

SpellSelectionDlg::SpellSelectionDlg(const H3CombatCreature *creature, const std::vector<eSpell> &availableSpells,
                                     const BOOL isPopup, const int width, const int height)
    : H3Dlg(width, height, -1, -1, false, false), availableSpells(availableSpells), selectedSpell(eSpell::NONE)
{

    LPCSTR defName = SpellSelectionDlg::ITEM_DEF_NAME;

    H3DefLoader spellDef(defName);
    const int itemWidth = spellDef->widthDEF;
    const int itemHeight = spellDef->heightDEF;

    const size_t spellCount = availableSpells.size();
    constexpr size_t spacing = ITEMS_PADDING;
    constexpr size_t margin = ITEMS_MARGIN;

    const int itemsPerRow = spellCount < 21 ? std::min(ITEMS_PER_ROW, spellCount) : 7;
    const int rows = (spellCount + itemsPerRow - 1) / itemsPerRow;

    const BOOL createHint = !isPopup;
    enableDoubleClick = !isPopup;
    const size_t hintHeight = createHint ? 20 : 0;
    const size_t okCancelHeight = !isPopup ? 44 : 0;

    // change dialog size based on items
    this->widthDlg = margin * 2 + itemsPerRow * itemWidth + (itemsPerRow - 1) * spacing;
    this->heightDlg = margin * 2 + rows * itemHeight + (rows - 1) * spacing + hintHeight + okCancelHeight;

    xDlg = (H3GameWidth::Get() - widthDlg) >> 1;
    yDlg = (H3GameHeight::Get() - heightDlg) >> 1;

    const auto owner = creature->GetOwner();
    const int frameColor = owner ? owner->owner : P_CombatManager->hero[0]->owner;

    this->AddBackground(true, createHint, frameColor);

    int itemId = 1;
    H3DlgDef *spellItem = nullptr;
    H3DlgItem *selectedSpellItem = nullptr;

    selectedSpell = preselectedSpell = GetUserSelectedSpell(creature);
    if (preselectedSpell == eSpell::NONE && creature == P_CombatManager->activeStack &&
        creature->type == eCreature::FAERIE_DRAGON)
    {
        preselectedSpell = eSpell(creature->faerieDragonSpell);
    }
    for (size_t row = 0; row < rows; row++)
    {
        for (size_t column = 0; column < itemsPerRow; column++)
        {
            const size_t spellIndex = row * itemsPerRow + column;
            if (spellIndex < spellCount)
            {
                const int x = margin + column * (itemWidth + spacing);
                const int y = margin + row * (itemHeight + spacing);

                const eSpell spell = availableSpells[spellIndex];
                spellItem = H3DlgDef::Create(x, y, spell, defName, spell);
                AddItem(spellItem);
                if (preselectedSpell == spell)
                {
                    selectedSpellItem = spellItem;
                }
            }
            else
            {
                break;
            }
        }
    }
    spellDef.Release();

    // create selection frame
    if (spellItem)
    {
        H3RGB565 highlightColor = H3RGB888::Highlight();
        selectionFrame = CreateFrame(spellItem, highlightColor, itemId++, -3);
        selectionFrame->DeActivate();
        if (selectedSpellItem)
        {
            selectionFrame->SetX(selectedSpellItem->GetX());
            selectionFrame->SetY(selectedSpellItem->GetY());
        }
        else
        {
            selectionFrame->Hide();
        }
    }

    if (!isPopup)
    {
        auto okBttn = CreateOKButton(25, heightDlg - 50 - hintHeight);
        auto cancelBttn = CreateCancelButton(widthDlg - 25 - 64, heightDlg - 50 - hintHeight);
        H3LoadedDef *defaultDef = H3LoadedDef::Load("hd_xchng.def");
        if (!defaultDef)
            return;
        const int defaultX = (widthDlg - defaultDef->widthDEF) >> 1;
        const int defaultY = okBttn->GetY() + (okBttn->GetHeight() - defaultDef->heightDEF >> 1);

        auto defaultBttn = H3DlgDefButton::Create(defaultX, defaultY, DEFAULT_BUTTON_ID, defaultDef->GetName(), 0, 1,
                                                  false, eVKey::H3VK_D);

        H3RGB565 color = H3RGB888::Highlight();
        CreateFrame(defaultBttn, color, 0, 1);
        AddItem(defaultBttn);
    }
}

BOOL SpellSelectionDlg::DialogProc(H3Msg &msg)
{
    if (msg.IsLeftDown())
    {
        const int itemId = msg.itemId;
        if (itemId >= eSpell::QUICK_SAND && itemId <= eSpell::AIR_ELEMENTAL)
        {
            selectedSpell = static_cast<eSpell>(itemId);
            H3DlgItem *clickedItem = GetDef(itemId);
            if (clickedItem)
            {
                selectionFrame->SetX(clickedItem->GetX());
                selectionFrame->SetY(clickedItem->GetY());
                selectionFrame->Show();
                Redraw();
            }

            return 1;
        }
    }
    else if (msg.IsLeftClick() && msg.itemId == DEFAULT_BUTTON_ID) // default button
    {
        selectedSpell = eSpell::NONE;
        selectionFrame->Hide();
        Redraw();
        return 1;
    }

    return 0;
}

VOID SpellSelectionDlg::OnCancel()
{
    selectedSpell = preselectedSpell;
    return VOID();
}

BOOL SpellSelectionDlg::OnDoubleClick(INT itemID, H3Msg &msg)
{
    if (itemID >= eSpell::QUICK_SAND && itemID <= eSpell::AIR_ELEMENTAL)
    {
        selectedSpell = static_cast<eSpell>(itemID);
        Stop();
        return 1;
    }

    return 0;
}
eSpell SpellSelectionDlg::ShowSpellSelectionDialog(const H3CombatCreature *creature, const H3Msg *msg)
{
    const int side = creature->GetSide();
    if (!P_CombatManager->isHuman[side])
    {
        return eSpell::NONE;
    }

    const BOOL isPopup = msg->IsRightClick();
    std::vector<eSpell> availableSpells;
    CreatureSpellData::CreateAvailableSpellsList(creature, availableSpells);
    if (availableSpells.empty())
    {
        return eSpell::NONE;
    }

    auto &dlg = SpellSelectionDlg(creature, availableSpells, isPopup); // , dlgSize.width, dlgSize.height);
    isPopup ? dlg.RMB_Show() : dlg.Start();

    return dlg.selectedSpell;
}
