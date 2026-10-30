#include <unordered_set>

#include "CreatureMagicSelector.h"
#include <cstring>

CreatureMagicRandom *CreatureMagicRandom::instance = nullptr;

CreatureMagicRandom::CreatureMagicRandom() : IGamePatch(_PI)
{
    libc::memset(userSelectedSpell, 0xFF, sizeof(userSelectedSpell));
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
    outList.clear();
    if (!creature || creature->numberAlive < 1) return FALSE;
    int maxSpellLevel = 5;

    if (P_CombatManager->specialTerrain == 2)
    {
        maxSpellLevel = 1;
    }
    {
        for (auto &hero : P_CombatManager->hero)
        {
            if (!hero)
                continue;
            if (hero->WearsArtifact(eArtifact::ORB_OF_INHIBITION))
                return FALSE;
            if (hero->WearsArtifact(eArtifact::RECANTERS_CLOAK))
                maxSpellLevel = std::min(maxSpellLevel, 2);
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
            if (spellId < eSpell::QUICK_SAND || spellId >= eSpell::STONE || P_Spell[spellId].level > maxSpellLevel)
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
                    if (!P_CombatManager->hero[1 - creature->GetSide()])
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

            if (spellId < eSpell::QUICK_SAND || spellId >= eSpell::STONE || P_Spell[spellId].level > maxSpellLevel)
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
            if (spellId < eSpell::QUICK_SAND || spellId >= eSpell::STONE || P_Spell[spellId].level > maxSpellLevel)
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
    if (!creature || creature->side < 0 || creature->side > 1 || creature->sideIndex < 0 ||
        creature->sideIndex >= h3::limits::TOTAL_COMBAT_CREATURES ||
        !P_CombatManager->isHuman[creature->GetSide()]) return eSpell::NONE;

    const eSpell selected = CreatureMagicRandom::instance->userSelectedSpell[creature->side][creature->sideIndex];
    if (selected == eSpell::NONE) return selected;
    std::vector<eSpell> allowed;
    CreatureSpellData::CreateAvailableSpellsList(creature, allowed);
    return std::find(allowed.begin(), allowed.end(), selected) != allowed.end() ? selected : eSpell::NONE;
}

static void __stdcall BattleStack_CastGenieSpell(HiHook *h, H3CombatCreature *creature, const int pos)
{

    // emulate the whole spell casting process
    // cause original function is too complex to patch in parts

    const eSpell setSpellByUser = GetUserSelectedSpell(creature);
    if (setSpellByUser != eSpell::NONE)
    {
        const auto targetStack = pos >= 0 && pos < 187 ? P_CombatManager->squares[pos].GetMonster() : nullptr;
        if (targetStack && targetStack->CanReceiveSpell(setSpellByUser))
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

    // Enchanters cast automatically; prompt only on the local human's eligible turn.
    if (setSpellByUser == eSpell::NONE && P_CombatManager->isHuman[creature->GetSide()] &&
        !P_CombatManager->autoCombat && !P_CombatManager->IsHiddenBattle() &&
        EraJS::readInt("mrart.spell_selector.prompt_enchanter"))
    {
        H3Msg msg{};
        msg.command = eMsgCommand::MOUSE_BUTTON;
        msg.subtype = eMsgSubtype::LBUTTON_CLICK;
        setSpellByUser = SpellSelectionDlg::ShowSpellSelectionDialog(creature, &msg);
        CreatureMagicRandom::instance->userSelectedSpell[creature->side][creature->sideIndex] = setSpellByUser;
    }

    if (setSpellByUser != eSpell::NONE)
    {
        CreatureSpellData *data = CreatureSpellData::GetEnchantersArray();
        CreatureSpellData storedData[CreatureSpellData::ENCHANTERS_ARRAY_SIZE];
        libc::memcpy(storedData, data, sizeof(storedData));

        // emulate first spell in array being the one selected by user
        data[0].spellId = setSpellByUser; // setSpellByUser;
        data[0].chanceToCast = 100;
        for (size_t i = 1; i < CreatureSpellData::ENCHANTERS_ARRAY_SIZE; ++i)
            data[i] = data[0];

        // call pseudo original function to check if spell can be cast
        char ret = THISCALL_1(char, h->GetDefaultFunc(), creature);
        CreatureMagicRandom::GetInstance().creaturePrioritySpells.enchanter.UseSpell(setSpellByUser);
        libc::memcpy(data, storedData, sizeof(storedData));
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
    }

    return EXEC_DEFAULT;
}

void __stdcall BattleMgr_ShowMonStatDlg(HiHook *hook, H3CombatManager *mgr, H3CombatCreature *creature,
                                        BOOL isRightClick)
{

    const bool displayDlg = creature && GetCretureSpellPower(creature) > 0 &&
        P_CombatManager->isHuman[creature->GetSide()] &&
        EraJS::readInt("mrart.spell_selector.ctrl_click") &&
        (STDCALL_1(SHORT, PtrAt(0x63A294), VK_CONTROL) & 0x8000) && !isRightClick;
    if (displayDlg)
    {
        H3Msg msg{};
        msg.command = eMsgCommand::MOUSE_BUTTON;
        msg.subtype = eMsgSubtype::LBUTTON_CLICK;
        auto selectedSpell = SpellSelectionDlg::ShowSpellSelectionDialog(creature, &msg);
        CreatureMagicRandom::instance->userSelectedSpell[creature->side][creature->sideIndex] = selectedSpell;
        if (mgr->activeStack == creature && creature->type == eCreature::FAERIE_DRAGON)
        {
            const auto natural = CreatureMagicRandom::instance->faerieDragonSpell;
            if (selectedSpell != eSpell::NONE || natural != eSpell::NONE)
                creature->faerieDragonSpell = selectedSpell != eSpell::NONE ? selectedSpell : natural;
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
    if (creature && GetCretureSpellPower(creature) > 0 && P_CombatManager->isHuman[creature->GetSide()])
    {
        c->return_address = 0x005F3D5D;
        return NO_EXEC_DEFAULT;
    }

    return EXEC_DEFAULT;
}
_LHF_(EnchanterSpellButton)
{
    auto creature = reinterpret_cast<H3CombatCreature*>(c->edi);
    if (creature && creature->type == eCreature::ENCHANTER &&
        P_CombatManager->isHuman[creature->GetSide()])
    {
        c->return_address = 0x5F3D6B;
        return NO_EXEC_DEFAULT;
    }
    return EXEC_DEFAULT;
}

_LHF_(H3CreatureInfoDlg__Battle__AfterClose)
{
    H3CombatCreature *creature = *reinterpret_cast<H3CombatCreature **>(c->ebp + 0x8);
    if (creature && GetCretureSpellPower(creature) > 0 && P_WindowManager->resultItemID == 302)
    {

        H3Msg msg{};
        msg.command = eMsgCommand::MOUSE_BUTTON;
        msg.subtype = eMsgSubtype::LBUTTON_CLICK;
        auto selectedSpell = SpellSelectionDlg::ShowSpellSelectionDialog(creature, &msg);
        CreatureMagicRandom::instance->userSelectedSpell[creature->side][creature->sideIndex] = selectedSpell;

        if (P_CombatManager->activeStack == creature &&
            creature->type == eCreature::FAERIE_DRAGON)
        {
            const auto natural = CreatureMagicRandom::instance->faerieDragonSpell;
            if (selectedSpell != eSpell::NONE || natural != eSpell::NONE)
                creature->faerieDragonSpell = selectedSpell != eSpell::NONE ? selectedSpell : natural;
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
    WriteLoHook(0x05F3D65, EnchanterSpellButton);
    WriteLoHook(0x0468515, H3CreatureInfoDlg__Battle__AfterClose);
    _pi->WriteDword(0x05F53FB + 3, 302); // set new callback button result
    _REH_(OnSetupBattlefield);
    _REH_(OnAfterBattleUniversal);
}

BOOL CreaturePrioritySpells::LoadUserSettings(LPCSTR)
{
    SpellLists* lists[] = {&masterGenie, &faerieDragon, &enchanter};
    for (auto list : lists)
    {
        list->spells.clear();
        H3String key = H3String::Format("mrart.spell_selector.priority.%d", list->creature);
        bool found = false;
        const char* cursor = EraJS::read(key.String(), found);
        if (!found) continue;
        while (*cursor)
        {
            char* end = nullptr;
            long id = std::strtol(cursor, &end, 10);
            if (end == cursor) break;
            if (id >= eSpell::QUICK_SAND && id < eSpell::STONE &&
                std::find(list->spells.begin(), list->spells.end(), eSpell(id)) == list->spells.end())
                list->spells.push_back(eSpell(id));
            cursor = end;
            while (*cursor == ',' || *cursor == ' ') ++cursor;
        }
    }
    return TRUE;
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

    bool found = false;
    LPCSTR defName = EraJS::read("mrart.spell_selector.spells_def", found);
    if (!found || !defName || !*defName) defName = SpellSelectionDlg::ITEM_DEF_NAME;

    const auto isUsableSpellDef = [](const H3LoadedDef *def) {
        return def && def->widthDEF > 0 && def->heightDEF > 0 && def->groups && def->groupsCount > 0 &&
               def->groups[0] && def->groups[0]->count > 0;
    };
    H3LoadedDef *spellDef = H3LoadedDef::Load(defName);
    if (!isUsableSpellDef(spellDef) && std::strcmp(defName, SpellSelectionDlg::ITEM_DEF_NAME) != 0)
    {
        if (spellDef) spellDef->Dereference();
        defName = SpellSelectionDlg::ITEM_DEF_NAME;
        spellDef = H3LoadedDef::Load(defName);
    }
    if (!isUsableSpellDef(spellDef))
    {
        if (spellDef) spellDef->Dereference();
        return;
    }
    canShow = true;
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
    const int frameColor = owner ? owner->owner : H3CurrentPlayerID::Get();

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
    for (int row = 0; row < rows; row++)
    {
        for (int column = 0; column < itemsPerRow; column++)
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
    spellDef->Dereference();

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
        if (defaultDef)
        {
            const int defaultX = (widthDlg - defaultDef->widthDEF) >> 1;
            const int defaultY = okBttn ? okBttn->GetY() + ((okBttn->GetHeight() - defaultDef->heightDEF) >> 1)
                                        : heightDlg - defaultDef->heightDEF - 8;
            auto defaultBttn = H3DlgDefButton::Create(defaultX, defaultY, DEFAULT_BUTTON_ID, defaultDef->GetName(), 0,
                                                      1, false, eVKey::H3VK_D);
            if (defaultBttn)
            {
                H3RGB565 color = H3RGB888::Highlight();
                CreateFrame(defaultBttn, color, 0, 1);
                AddItem(defaultBttn);
                found = false;
                LPCSTR hint = EraJS::read("mrart.spell_selector.random_hint", found);
                defaultBttn->SetHints(found && hint && *hint ? hint : "Choose a random spell (D).", nullptr, false);
            }
            defaultDef->Dereference();
        }
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
    if (!creature || GetCretureSpellPower(creature) <= 0) return eSpell::NONE;
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

    auto dlg = SpellSelectionDlg(creature, availableSpells, isPopup); // , dlgSize.width, dlgSize.height);
    if (!dlg.canShow) return eSpell::NONE;
    isPopup ? dlg.RMB_Show() : dlg.Start();

    return dlg.selectedSpell;
}
