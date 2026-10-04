#include "pch.h"
#include "CreatureDlgHooks.h"
#include "CreatureDlgHandler.h"
#include "CreatureDlgLayout.h"
#include "PluginSettings.h"

#include <algorithm>
#include <map>
#include <memory>

using namespace h3;
using namespace creatureInfo;

namespace
{
enum class CreatureDialogKind
{
    None,
    Battle,
    NotBattle,
    Buy
};

struct CreatureDialogBuildContext
{
    CreatureDialogKind kind = CreatureDialogKind::None;
    H3CreatureInfoDlg *dlg = nullptr;
};

CreatureDialogBuildContext g_buildContexts[8];
int g_buildContextDepth = 0;
std::map<H3CreatureInfoDlg *, std::unique_ptr<CreatureDlgHandler>> g_dialogHandlers;
BOOL main_isRMC = false;

CreatureDialogBuildContext *CurrentBuildContext()
{
    return g_buildContextDepth > 0 ? &g_buildContexts[g_buildContextDepth - 1] : nullptr;
}

class CreatureDialogBuildScope
{
    bool pushed = false;

  public:
    CreatureDialogBuildScope(CreatureDialogKind kind, H3CreatureInfoDlg *dlg)
    {
        if (g_buildContextDepth < static_cast<int>(sizeof(g_buildContexts) / sizeof(g_buildContexts[0])))
        {
            g_buildContexts[g_buildContextDepth].kind = kind;
            g_buildContexts[g_buildContextDepth].dlg = dlg;
            ++g_buildContextDepth;
            pushed = true;
        }
    }

    ~CreatureDialogBuildScope()
    {
        if (pushed && g_buildContextDepth > 0)
        {
            --g_buildContextDepth;
            g_buildContexts[g_buildContextDepth] = CreatureDialogBuildContext();
        }
    }
};

CreatureDlgHandler *GetDialogHandler(H3CreatureInfoDlg *dlg)
{
    auto it = g_dialogHandlers.find(dlg);
    return it == g_dialogHandlers.end() ? nullptr : it->second.get();
}

void RegisterDialogHandler(H3CreatureInfoDlg *dlg, CreatureDlgHandler *handler)
{
    if (!dlg)
    {
        delete handler;
        return;
    }

    g_dialogHandlers[dlg].reset(handler);
}

void DestroyDialogHandler(H3CreatureInfoDlg *dlg)
{
    g_dialogHandlers.erase(dlg);
}

int CurrentPlayerColor()
{
    if (P_Game)
    {
        const int player = P_Game->GetPlayerID();
        if (player >= 0 && player < 8)
            return player;
    }
    return -1;
}

int ResolveBattleDialogColor(H3CombatCreature *stack)
{
    if (stack)
    {
        H3Hero *owner = THISCALL_1(H3Hero *, 0x4423B0, stack); // BattleStack::GetOwnerHero
        if (owner && owner->owner >= 0 && owner->owner < 8)
            return owner->owner;
    }
    return CurrentPlayerColor();
}

int ResolveAdventureDialogColor(const H3Hero *hero)
{
    if (hero && hero->owner >= 0 && hero->owner < 8)
        return hero->owner;
    return CurrentPlayerColor();
}

bool SameResource(LPCSTR lhs, LPCSTR rhs)
{
    return lhs && rhs && libc::strcmpi(lhs, rhs) == 0;
}

bool IsBattleGeneratedReturnAddress(DWORD returnAddress)
{
    return returnAddress >= 0x760000 && returnAddress < 0x770000;
}

bool IsCurrentCreatureDialogBuild()
{
    auto *ctx = CurrentBuildContext();
    if (!ctx || ctx->kind == CreatureDialogKind::None || !ctx->dlg)
        return false;

    const auto &settings = GetPluginSettings();
    switch (ctx->kind)
    {
    case CreatureDialogKind::Battle:
        return settings.showBattleDialog;
    case CreatureDialogKind::NotBattle:
        return settings.showArmyDialog;
    case CreatureDialogKind::Buy:
        return settings.showRecruitmentDialog;
    default:
        return false;
    }
}

void GetDialogDrawBounds(int &width, int &height)
{
    width = H3GameWidth::Get();
    height = H3GameHeight::Get();

    if (H3WindowManager *windowManager = H3WindowManager::Get())
    {
        if (H3LoadedPcx16 *drawBuffer = windowManager->GetDrawBuffer())
        {
            if (drawBuffer->width > 0)
                width = drawBuffer->width;
            if (drawBuffer->height > 0)
                height = drawBuffer->height;
        }
    }
}

void KeepBattleCreatureDialogInsideDrawBuffer(H3CreatureInfoDlg *dlg)
{
    if (!dlg)
        return;

    int drawWidth = 0;
    int drawHeight = 0;
    GetDialogDrawBounds(drawWidth, drawHeight);

    const int maxX = std::max(0, drawWidth - dlg->GetWidth() - DLG_SHADOW_SIZE);
    const int maxY = std::max(0, drawHeight - dlg->GetHeight() - DLG_SHADOW_SIZE);
    dlg->SetX(Clamp(0, dlg->GetX(), maxX));
    dlg->SetY(Clamp(0, dlg->GetY(), maxY));
}

} // namespace

void __fastcall CreatureSkillsScrollbarProc(INT32 tick, H3BaseDlg *baseDlg)
{
    auto *dlg = reinterpret_cast<H3CreatureInfoDlg *>(baseDlg);
    if (auto *handler = GetDialogHandler(dlg))
        handler->ScrollTo(tick);
}

namespace
{
H3Dlg *__stdcall CreatureDlg_DlgCtor(HiHook *hook, H3Dlg *dlg, int x, int y, int width, int height, int flags)
{
    auto *ctx = CurrentBuildContext();
    // The build scope already proves that we are inside one of the three
    // H3CreatureInfoDlg constructors. Do not require a fixed return address
    // here: another HiHook may put a trampoline between the creature ctor and
    // Dlg::Ctor, making GetReturnAddress() different from the vanilla SoD
    // call site even though this is still the same dialog.
    if (IsCurrentCreatureDialogBuild() && ctx->dlg == reinterpret_cast<H3CreatureInfoDlg *>(dlg) &&
        width == 298 && height == 311)
    {
        width = DLG_WIDTH;
        height = DLG_HEIGHT;
    }
    return THISCALL_6(H3Dlg *, hook->GetDefaultFunc(), dlg, x, y, width, height, flags);
}

H3DlgPcx *__stdcall CreatureDlg_PcxCtor(HiHook *hook, H3DlgPcx *item, int x, int y, int width, int height, int id,
                                        LPCSTR pcxName, int flags)
{
    auto *ctx = CurrentBuildContext();
    if (IsCurrentCreatureDialogBuild())
    {
        const bool isBackground = id == 200 && x == 0 && y == 0 && width == 298 && height == 311 &&
                                  SameResource(pcxName, "crstkpu.pcx");
        if (isBackground)
        {
            // Same rule as for Dlg::Ctor: active creature-info build context +
            // the original background signature is sufficient and survives
            // hook chaining by other plugins.
            width = DLG_WIDTH;
            height = DLG_HEIGHT;
            pcxName = DIALOG_BACKGROUND_PCX;
        }
        else if (id == 225) // OK frame
        {
            x = OK_BUTTON_X - 1;
            y = BUTTON_Y - 1;
            width = 48;
            height = 34;
            pcxName = "box46x32.pcx";
        }
        else if (x == 74 && y == 236 && width == 48 && height == 34) // upgrade/cast frame
        {
            // Exact coordinates are part of the native helper signature.
            // The PCX name is deliberately not checked so a previous hook in
            // the chain cannot prevent the relocation.
            x = (ctx->kind == CreatureDialogKind::Battle) ? 125 : 232;
            y = BUTTON_Y - 1;
        }
        else if (x == 19 && y == 236 && width == 48 && height == 34) // dismiss frame
        {
            x = 126;
            y = BUTTON_Y - 1;
        }
    }

    return THISCALL_8(H3DlgPcx *, hook->GetDefaultFunc(), item, x, y, width, height, id, pcxName, flags);
}

H3DlgText *__stdcall CreatureDlg_TextCtor(HiHook *hook, H3DlgText *item, int x, int y, int width, int height,
                                          LPCSTR text, LPCSTR font, int color, int itemId, int align, int bkColor,
                                          int unused)
{
    auto *ctx = CurrentBuildContext();
    if (IsCurrentCreatureDialogBuild())
    {
        const DWORD ret = static_cast<DWORD>(hook->GetReturnAddress());
        const bool battleGenerated = ctx->kind == CreatureDialogKind::Battle && IsBattleGeneratedReturnAddress(ret);

        if (itemId == 203 && x == 20 && y == 21 && width == 258 && height == 19 &&
            (ret == 0x5F39BF || ret == 0x5F4129 || ret == 0x5F47A5 || battleGenerated))
        {
            x = (DLG_WIDTH - width) / 2;
        }
        else if (itemId == -1 && x == 20 && y == 232 && width == 192 && height == 41 &&
                 (ret == 0x5F3E59 || ret == 0x5F4484 || ret == 0x5F489F || battleGenerated))
        {
            ReadDescriptionRect(x, y, width, height);
            align = GetPluginSettings().descriptionAlignment;
        }
    }

    return THISCALL_12(H3DlgText *, hook->GetDefaultFunc(), item, x, y, width, height, text, font, color, itemId,
                       align, bkColor, unused);
}

H3DlgTextPcx *__stdcall CreatureDlg_TextPcxCtor(HiHook *hook, H3DlgTextPcx *item, int x, int y, int width,
                                                int height, LPCSTR text, LPCSTR font, LPCSTR pcxName, int color,
                                                int itemId, int align, int unused)
{
    auto *ctx = CurrentBuildContext();
    if (IsCurrentCreatureDialogBuild() && itemId == 224 && x == 7 && y == 285 && width == 284 && height == 19 &&
        SameResource(pcxName, "varback.pcx"))
    {
        const DWORD ret = static_cast<DWORD>(hook->GetReturnAddress());
        const bool knownCall = ret == 0x5F3CF2 || ret == 0x5F44E1 || ret == 0x5F48FC;
        if (knownCall || (ctx->kind == CreatureDialogKind::Battle && IsBattleGeneratedReturnAddress(ret)))
        {
            y = DLG_HEIGHT - height - 7;
            width = DLG_WIDTH - 14;
            pcxName = DIALOG_HINT_BAR_PCX;
        }
    }

    return THISCALL_12(H3DlgTextPcx *, hook->GetDefaultFunc(), item, x, y, width, height, text, font, pcxName, color,
                       itemId, align, unused);
}

H3DlgDef *__stdcall CreatureDlg_DefCtor(HiHook *hook, H3DlgDef *item, int x, int y, int width, int height, int itemId,
                                        LPCSTR defName, int frame, int group, int mirror, int closeDialog, int flags)
{
    auto *ctx = CurrentBuildContext();
    const bool isMorale = itemId == 219 && x == 23 && y == 189 && width == 42 && height == 38;
    const bool isLuck = itemId == 220 && x == 77 && y == 189 && width == 42 && height == 38;
    if (IsCurrentCreatureDialogBuild() && (isMorale || isLuck))
    {
        const DWORD ret = static_cast<DWORD>(hook->GetReturnAddress());
        const bool knownCall = ret == 0x5F3AE7 || ret == 0x5F3C27 || ret == 0x5F429C || ret == 0x5F437F;
        if (knownCall || (ctx->kind == CreatureDialogKind::Battle && IsBattleGeneratedReturnAddress(ret)))
        {
            x = itemId == 219 ? 24 : 78;
            y = DLG_HEIGHT - height - 46;
        }
    }
    return THISCALL_12(H3DlgDef *, hook->GetDefaultFunc(), item, x, y, width, height, itemId, defName, frame, group,
                       mirror, closeDialog, flags);
}

H3DlgDefButton *__stdcall CreatureDlg_DefButtonCtor(HiHook *hook, H3DlgDefButton *item, int x, int y, int width,
                                                    int height, int itemId, LPCSTR defName, int frame, int clickFrame,
                                                    int closeDialog, int hotKey, int flags)
{
    if (IsCurrentCreatureDialogBuild())
    {
        // Item ids are unique inside H3CreatureInfoDlg. Once the build scope
        // proves the parent dialog, do not depend on original coordinates or
        // resource names: an earlier HiHook in the chain may already have
        // changed those arguments.
        if (itemId == 30722)
        {
            x = OK_BUTTON_X;
            y = BUTTON_Y;
            width = BUTTON_WIDTH;
            height = BUTTON_HEIGHT;
            defName = OK_BUTTON_DEF;
        }
        else if (itemId == 300)
        {
            x = 233;
            y = BUTTON_Y;
            width = BUTTON_WIDTH;
            height = BUTTON_HEIGHT;
        }
        else if (itemId == 30723)
        {
            x = 127;
            y = BUTTON_Y;
            width = BUTTON_WIDTH;
            height = BUTTON_HEIGHT;
        }
    }

    return THISCALL_12(H3DlgDefButton *, hook->GetDefaultFunc(), item, x, y, width, height, itemId, defName, frame,
                       clickFrame, closeDialog, hotKey, flags);
}

H3DlgCustomButton *__stdcall CreatureDlg_CustomButtonCtor(HiHook *hook, H3DlgCustomButton *item, int x, int y,
                                                          int width, int height, int itemId, LPCSTR defName,
                                                          H3DlgButton_proc callback, int frame, int clickFrame)
{
    auto *ctx = CurrentBuildContext();
    if (IsCurrentCreatureDialogBuild() && ctx->kind == CreatureDialogKind::Battle && itemId == 301)
    {
        x = 126;
        y = BUTTON_Y;
        width = BUTTON_WIDTH;
        height = BUTTON_HEIGHT;
        defName = CAST_BUTTON_DEF;
    }

    return THISCALL_10(H3DlgCustomButton *, hook->GetDefaultFunc(), item, x, y, width, height, itemId, defName,
                       callback, frame, clickFrame);
}

int __stdcall H3CreatureInfoDlg_Dtor(HiHook *hook, H3CreatureInfoDlg *dlg)
{
    DestroyDialogHandler(dlg);
    return THISCALL_1(int, hook->GetDefaultFunc(), dlg);
}

BOOL ShowStackActiveSpells(H3CombatCreature *stack, bool isRMC, H3DlgItem *clickedItem)
{
    if (!stack)
        return FALSE;
    const int arr_size = sizeof(stack->activeSpellDuration) / sizeof(INT32);
    int activeSpellsNum = 0;
    for (int i = 0; i < arr_size; ++i)
        activeSpellsNum += stack->activeSpellDuration[i] != 0;
    if (!activeSpellsNum)
        return FALSE;

    int columns = static_cast<int>(floor(sqrt(activeSpellsNum)));
    int rows = activeSpellsNum / columns;
    if (activeSpellsNum % columns)
        ++rows;
    if (rows > columns)
    {
        --rows;
        ++columns;
    }

    H3DefLoader def("spellint.def");
    const int defWidth = def->widthDEF;
    const int defHeight = def->heightDEF;
    const int width = (defWidth + 5) * columns + 35;
    const int height = (defHeight + 5) * rows + 35;
    H3Dlg *dlg = new H3Dlg(width, height);

    int x = 20;
    int y = 20;
    int counter = 0;
    for (INT32 i = 0; i < arr_size; ++i)
    {
        const int duration = stack->activeSpellDuration[i];
        if (!duration)
            continue;

        H3DlgDef *spell = H3DlgDef::Create(x, y, def->GetName(), i + 1);
        dlg->AddItem(spell);
        libc::sprintf(h3_TextBuffer, "x%d", duration);
        H3DlgText *text = H3DlgText::Create(x, y + 25, defWidth, 14, h3_TextBuffer, NH3Dlg::Text::TINY, 1, 0,
                                            eTextAlignment::MIDDLE_RIGHT);
        if (i != NH3Spells::eSpell::BERSERK && i != NH3Spells::eSpell::DISRUPTING_RAY && i != NH3Spells::eSpell::BIND)
            dlg->AddItem(text);

        if (++counter == columns)
        {
            counter = 0;
            x = 20;
            y += defHeight + 5;
        }
        else
            x += defWidth + 5;
    }

    if (isRMC)
        dlg->PlaceAtMouse();
    else if (clickedItem)
    {
        const int xPos = clickedItem->GetAbsoluteX();
        const int yPos = clickedItem->GetAbsoluteY();
        IntAt(reinterpret_cast<int>(dlg) + 0x18) = Clamp(0, xPos, H3GameWidth::Get() - width - 200);
        IntAt(reinterpret_cast<int>(dlg) + 0x1C) = Clamp(0, yPos, H3GameHeight::Get() - height - 48);
    }
    dlg->RMB_Show();
    delete dlg;
    return FALSE;
}

_LHF_(Dlg_CreatureInfo_RmcProc)
{
    if (!GetPluginSettings().showBattleDialog)
        return EXEC_DEFAULT;

    H3Msg *msg = reinterpret_cast<H3Msg *>(c->esi);
    const int itemId = msg->itemId;
    auto *creatureDlgStack = *reinterpret_cast<H3CombatCreature **>(0x2860280);
    if (((itemId > 220 && itemId < 224) || (itemId >= 3000 && itemId < 3003)) &&
        msg->subtype == eMsgSubtype::RBUTTON_DOWN && creatureDlgStack && creatureDlgStack->activeSpellNumber)
    {
        ShowStackActiveSpells(creatureDlgStack, true, nullptr);
        msg->itemId = -1;
    }
    return EXEC_DEFAULT;
}

int __stdcall H3CreatureInfoDlg_Proc(HiHook *hook, H3CreatureInfoDlg *dlg, H3Msg *msg)
{
    const int result = THISCALL_2(int, hook->GetDefaultFunc(), dlg, msg);
    switch (msg->command)
    {
    case eMsgCommand::MOUSE_OVER:
    {
        auto *item = dlg->ItemAtPosition(msg);
        auto *hint = dlg->GetTextPcx(224);
        if (item && hint && item->GetHint())
        {
            const char *text = item->GetHint();
            const char *previous = hint->GetH3String().String();
            if (!previous || libc::strcmp(previous, text))
            {
                hint->SetText(text);
                hint->Draw();
                hint->Refresh();
            }
        }
        return result;
    }
    case eMsgCommand::MOUSE_BUTTON:
    {
        auto *item = dlg->GetH3DlgItem(msg->itemId);
        //if (item && IsExperienceSkillItem(msg->itemId) && msg->subtype == eMsgSubtype::RBUTTON_DOWN)
        //{
        //    if (item->GetRightClickHint())
        //        H3Messagebox::RMB(item->GetRightClickHint());
        //    return result;
        //}

        if (item && (msg->itemId == WOG_CREATURE_EXP_BUTTON_ID || msg->itemId == DLG_SPELLS_BTTN_ID))
        {
            if (msg->subtype == eMsgSubtype::LBUTTON_CLICK || msg->subtype == eMsgSubtype::RBUTTON_DOWN)
            {
                const BOOL previousRmc = main_isRMC;
                main_isRMC = msg->subtype == eMsgSubtype::RBUTTON_DOWN;
                if (msg->itemId == WOG_CREATURE_EXP_BUTTON_ID)
                    CDECL_0(signed int, 0x7645BB);
                else
                    ShowStackActiveSpells(*reinterpret_cast<H3CombatCreature **>(0x2860280), main_isRMC, item);
                main_isRMC = previousRmc;
            }
            return result;
        }
        break;
    }
    default:
        break;
    }
    return result;
}

H3CreatureInfoDlg *__stdcall H3CreatureInfoDlg_BattleCtor(HiHook *hook, H3CreatureInfoDlg *dlg, H3CombatCreature *mon,
                                                          int x, int y, int z)
{
    if (!GetPluginSettings().showBattleDialog)
        return THISCALL_5(H3CreatureInfoDlg *, hook->GetDefaultFunc(), dlg, mon, x, y, z);

    // Generic H3Dlg shadow rendering extends 8 pixels to the right and
    // bottom. Keep that area inside the real game buffer as well; otherwise
    // Pcx16_DrawShadowRect may receive an out-of-bounds destination near the
    // right/bottom edge of the battlefield.
    int drawWidth = 0;
    int drawHeight = 0;
    GetDialogDrawBounds(drawWidth, drawHeight);
    const int maxX = std::max(0, drawWidth - DLG_WIDTH - DLG_SHADOW_SIZE);
    const int maxY = std::max(0, drawHeight - DLG_HEIGHT - DLG_SHADOW_SIZE);
    x = Clamp(0, x - 30, maxX);
    y = Clamp(0, y - 30, maxY);
    H3CreatureInfoDlg *result = nullptr;
    {
        CreatureDialogBuildScope scope(CreatureDialogKind::Battle, dlg);
        result = THISCALL_5(H3CreatureInfoDlg *, hook->GetDefaultFunc(), dlg, mon, x, y, z);
    }
    if (result)
    {
        KeepBattleCreatureDialogInsideDrawBuffer(result);
        auto *handler = new CreatureDlgHandler(result, mon, nullptr, -1, nullptr, ResolveBattleDialogColor(mon));
        RegisterDialogHandler(result, handler);
        // Handler/other chained hooks may normalize dimensions or coordinates;
        // make the final visible object safe for the 8-pixel shadow as well.
        KeepBattleCreatureDialogInsideDrawBuffer(result);
    }
    return result;
}

H3CreatureInfoDlg *__stdcall H3CreatureInfoDlg_NotBattleCtor(HiHook *hook, H3CreatureInfoDlg *dlg, H3Army *army,
                                                             int slotId, const DWORD hero, const H3Town *town, int x,
                                                             int y, const int monGrade, const int addDismiss,
                                                             const int addOk, const int hasAngelicAlliance)
{
    if (!GetPluginSettings().showArmyDialog)
        return THISCALL_11(H3CreatureInfoDlg *, hook->GetDefaultFunc(), dlg, army, slotId, hero, town, x, y,
                           monGrade, addDismiss, addOk, hasAngelicAlliance);

    H3CreatureInfoDlg *result = nullptr;
    {
        CreatureDialogBuildScope scope(CreatureDialogKind::NotBattle, dlg);
        result = THISCALL_11(H3CreatureInfoDlg *, hook->GetDefaultFunc(), dlg, army, slotId, hero, town, x, y, monGrade,
                             addDismiss, addOk, hasAngelicAlliance);
    }
    if (!result)
        return result;

    if (P_AdventureMgr && P_AdventureMgr->dlg && result->GetY() + DLG_HEIGHT + 145 > P_AdventureMgr->dlg->GetHeight())
        result->SetY(P_AdventureMgr->dlg->GetHeight() - DLG_HEIGHT - 145);

    const H3Hero *heroPtr = reinterpret_cast<const H3Hero *>(hero);
    auto *handler = new CreatureDlgHandler(result, nullptr, army, slotId, heroPtr, ResolveAdventureDialogColor(heroPtr));
    RegisterDialogHandler(result, handler);
    return result;
}

H3CreatureInfoDlg *__stdcall H3CreatureInfoDlg_BuyCtor(HiHook *hook, H3CreatureInfoDlg *dlg, const eCreature monId,
                                                       int x, int y, const DWORD flags)
{
    if (!GetPluginSettings().showRecruitmentDialog)
        return THISCALL_5(H3CreatureInfoDlg *, hook->GetDefaultFunc(), dlg, monId, x, y, flags);

    H3CreatureInfoDlg *result = nullptr;
    {
        CreatureDialogBuildScope scope(CreatureDialogKind::Buy, dlg);
        result = THISCALL_5(H3CreatureInfoDlg *, hook->GetDefaultFunc(), dlg, monId, x, y, flags);
    }
    if (result)
    {
        auto *handler = new CreatureDlgHandler(result, nullptr, nullptr, -1, nullptr, CurrentPlayerColor());
        RegisterDialogHandler(result, handler);
    }
    return result;
}

_LHF_(Wnd_BeforeExpoDlgShow)
{
    if (main_isRMC)
    {
        H3Dlg *wndDlg = reinterpret_cast<H3Dlg *>(c->esi);
        wndDlg->RMB_Show();
        c->return_address = h->GetAddress() + 21;
        main_isRMC = false;
        return NO_EXEC_DEFAULT;
    }
    return EXEC_DEFAULT;
}

_LHF_(Before_WndNPC_DLG)
{
    if (main_isRMC)
    {
        H3Dlg *wndDlg = reinterpret_cast<H3Dlg *>(c->esi);
        wndDlg->RMB_Show();
        c->return_address = h->GetAddress() + 0x20;
        main_isRMC = false;
        return NO_EXEC_DEFAULT;
    }
    return EXEC_DEFAULT;
}

} // namespace

void Dlg_CreatureSpellInfo_HooksInit(PatcherInstance *pi)
{
    pi->WriteLoHook(0x5F4C5D, Dlg_CreatureInfo_RmcProc);
}

void Dlg_CreatureInfo_HooksInit(PatcherInstance *pi)
{
    // High-level runtime hooks for the three creature-info entry points.
    pi->WriteHiHook(0x5F45B0, THISCALL_, H3CreatureInfoDlg_BuyCtor);
    pi->WriteHiHook(0x5F3EF0, THISCALL_, H3CreatureInfoDlg_NotBattleCtor);
    pi->WriteHiHook(0x764B38, THISCALL_, H3CreatureInfoDlg_BattleCtor);
    pi->WriteHiHook(0x5F4C00, THISCALL_, H3CreatureInfoDlg_Proc);
    pi->WriteHiHook(0x5F4980, THISCALL_, H3CreatureInfoDlg_Dtor);

    // Replace constructor arguments only while one of the creature-info
    // constructors is on the call stack. Unique item ids/signatures are used
    // where possible; return addresses remain only for ambiguous native items.
    pi->WriteHiHook(0x41AFA0, SPLICE_, EXTENDED_, THISCALL_, CreatureDlg_DlgCtor);
    pi->WriteHiHook(0x44FFA0, SPLICE_, EXTENDED_, THISCALL_, CreatureDlg_PcxCtor);
    pi->WriteHiHook(0x5BC6A0, SPLICE_, EXTENDED_, THISCALL_, CreatureDlg_TextCtor);
    pi->WriteHiHook(0x5BCB70, SPLICE_, EXTENDED_, THISCALL_, CreatureDlg_TextPcxCtor);
    pi->WriteHiHook(0x4EA800, SPLICE_, EXTENDED_, THISCALL_, CreatureDlg_DefCtor);
    pi->WriteHiHook(0x455BD0, SPLICE_, EXTENDED_, THISCALL_, CreatureDlg_DefButtonCtor);
    pi->WriteHiHook(0x456A10, SPLICE_, EXTENDED_, THISCALL_, CreatureDlg_CustomButtonCtor);

    H3DLL wndPlugin("wog native dialogs.era");
    if (wndPlugin.dataSize)
    {
        int pluginHookAddress = wndPlugin.NeedleSearch<6>({0x0F, 0x8C, 0xC8, 0xFE, 0xFF, 0xFF}, 6);
        if (pluginHookAddress)
            pi->WriteLoHook(pluginHookAddress, Wnd_BeforeExpoDlgShow);
        pluginHookAddress = wndPlugin.NeedleSearch<3>({0x3D, 0x68, 0x02}, 15);
        if (pluginHookAddress)
            pi->WriteLoHook(pluginHookAddress, Before_WndNPC_DLG);
    }
}
