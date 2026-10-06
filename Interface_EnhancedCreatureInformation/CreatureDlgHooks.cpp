#include "pch.h"
#include "CreatureDlgHooks.h"
#include "CreatureDlgHandler.h"
#include "CreatureDlgLayout.h"
#include "PluginSettings.h"
#include "CreatureSpellEffects.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <map>
#include <memory>

using namespace h3;
using namespace creatureInfo;

namespace
{
constexpr int MAX_BUILD_CONTEXT_DEPTH = 8;
constexpr int EXPERIENCE_RMB_HOOK_SKIP = 21;
constexpr int COMMANDER_RMB_HOOK_SKIP = 0x20;
constexpr int EXPERIENCE_SHOW_PATTERN_OFFSET = 6;
constexpr int COMMANDER_SHOW_PATTERN_OFFSET = 15;
namespace native
{
constexpr DWORD STACK_OWNER_HERO = 0x4423B0;
constexpr DWORD BATTLE_GENERATED_START = 0x760000;
constexpr DWORD BATTLE_GENERATED_END = 0x770000;
constexpr DWORD SHOW_EXPERIENCE_DIALOG = 0x7645BB;
constexpr DWORD CURRENT_CREATURE_STACK = 0x2860280;
constexpr DWORD CREATURE_RMB_PROC = 0x5F4C5D;
constexpr DWORD BUY_CTOR = 0x5F45B0;
constexpr DWORD ARMY_CTOR = 0x5F3EF0;
constexpr DWORD BATTLE_CTOR = 0x764B38;
constexpr DWORD CREATURE_PROC = 0x5F4C00;
constexpr DWORD CREATURE_DTOR = 0x5F4980;
constexpr DWORD DLG_CTOR = 0x41AFA0;
constexpr DWORD PCX_CTOR = 0x44FFA0;
constexpr DWORD TEXT_CTOR = 0x5BC6A0;
constexpr DWORD TEXT_PCX_CTOR = 0x5BCB70;
constexpr DWORD DEF_CTOR = 0x4EA800;
constexpr DWORD BUTTON_CTOR = 0x455BD0;
constexpr DWORD CUSTOM_BUTTON_CTOR = 0x456A10;
constexpr DWORD NAME_CALLS[] = {0x5F39BF, 0x5F4129, 0x5F47A5};
constexpr DWORD DESCRIPTION_CALLS[] = {0x5F3E59, 0x5F4484, 0x5F489F};
constexpr DWORD HINT_CALLS[] = {0x5F3CF2, 0x5F44E1, 0x5F48FC};
constexpr DWORD MORALE_LUCK_CALLS[] = {0x5F3AE7, 0x5F3C27, 0x5F429C, 0x5F437F};
}
struct NativeItemRect
{
    int x, y, width, height;
    bool Matches(int px, int py, int w, int h) const { return x == px && y == py && width == w && height == h; }
};
constexpr NativeItemRect NATIVE_NAME_RECT = {20, 21, 258, 19};
constexpr NativeItemRect NATIVE_DESCRIPTION_RECT = {20, 232, 192, 41};
constexpr NativeItemRect NATIVE_HINT_RECT = {7, 285, 284, 19};
constexpr NativeItemRect NATIVE_MORALE_RECT = {23, 189, 42, 38};
constexpr NativeItemRect NATIVE_LUCK_RECT = {77, 189, 42, 38};
constexpr NativeItemRect NATIVE_UPGRADE_FRAME_RECT = {74, 236, 48, 34};
constexpr NativeItemRect NATIVE_DISMISS_FRAME_RECT = {19, 236, 48, 34};
constexpr LPCSTR NATIVE_BACKGROUND_PCX = "crstkpu.pcx";
constexpr LPCSTR NATIVE_HINT_PCX = "varback.pcx";
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

CreatureDialogBuildContext g_buildContexts[MAX_BUILD_CONTEXT_DEPTH];
int g_buildContextDepth = 0;
std::map<H3CreatureInfoDlg *, std::unique_ptr<CreatureDlgHandler>> g_dialogHandlers;
BOOL main_isRMC = false;

class RightClickDialogScope
{
    const BOOL previous = main_isRMC;
  public:
    explicit RightClickDialogScope(bool rightClick) { main_isRMC = rightClick; }
    ~RightClickDialogScope() { main_isRMC = previous; }
};

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
        if (player >= 0 && player < limits::PLAYERS)
            return player;
    }
    return -1;
}

int ResolveBattleDialogColor(H3CombatCreature *stack)
{
    if (stack)
    {
        H3Hero *owner = THISCALL_1(H3Hero *, native::STACK_OWNER_HERO, stack); // BattleStack::GetOwnerHero
        if (owner && owner->owner >= 0 && owner->owner < limits::PLAYERS)
            return owner->owner;
    }
    return CurrentPlayerColor();
}

int ResolveAdventureDialogColor(const H3Hero *hero)
{
    if (hero && hero->owner >= 0 && hero->owner < limits::PLAYERS)
        return hero->owner;
    return CurrentPlayerColor();
}

bool SameResource(LPCSTR lhs, LPCSTR rhs)
{
    return lhs && rhs && libc::strcmpi(lhs, rhs) == 0;
}

bool IsBattleGeneratedReturnAddress(DWORD returnAddress)
{
    return returnAddress >= native::BATTLE_GENERATED_START && returnAddress < native::BATTLE_GENERATED_END;
}

CreatureDialogBuildContext *ActiveBuildContext()
{
    auto *ctx = CurrentBuildContext();
    if (!ctx || !ctx->dlg)
        return nullptr;
    const auto &settings = GetPluginSettings();
    switch (ctx->kind)
    {
    case CreatureDialogKind::Battle: return settings.showBattleDialog ? ctx : nullptr;
    case CreatureDialogKind::NotBattle: return settings.showArmyDialog ? ctx : nullptr;
    case CreatureDialogKind::Buy: return settings.showRecruitmentDialog ? ctx : nullptr;
    default: return nullptr;
    }
}

template<size_t N>
bool IsKnownConstructorCall(DWORD address, const DWORD (&calls)[N], const CreatureDialogBuildContext *ctx)
{
    for (const DWORD call : calls)
        if (address == call)
            return true;
    return ctx->kind == CreatureDialogKind::Battle && IsBattleGeneratedReturnAddress(address);
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
    auto *ctx = ActiveBuildContext();
    // The build scope already proves that we are inside one of the three
    // H3CreatureInfoDlg constructors. Do not require a fixed return address
    // here: another HiHook may put a trampoline between the creature ctor and
    // Dlg::Ctor, making GetReturnAddress() different from the vanilla SoD
    // call site even though this is still the same dialog.
    if (ctx != nullptr && ctx->dlg == reinterpret_cast<H3CreatureInfoDlg *>(dlg) &&
        width == NATIVE_DLG_WIDTH && height == NATIVE_DLG_HEIGHT)
    {
        width = DLG_WIDTH;
        height = DLG_HEIGHT;
    }
    return THISCALL_6(H3Dlg *, hook->GetDefaultFunc(), dlg, x, y, width, height, flags);
}

H3DlgPcx *__stdcall CreatureDlg_PcxCtor(HiHook *hook, H3DlgPcx *item, int x, int y, int width, int height, int id,
                                        LPCSTR pcxName, int flags)
{
    auto *ctx = ActiveBuildContext();
    if (ctx != nullptr)
    {
        const bool isBackground = id == DLG_BACKGROUND_ID && x == 0 && y == 0 && width == NATIVE_DLG_WIDTH && height == NATIVE_DLG_HEIGHT &&
                                  SameResource(pcxName, NATIVE_BACKGROUND_PCX);
        if (isBackground)
        {
            // Same rule as for Dlg::Ctor: active creature-info build context +
            // the original background signature is sufficient and survives
            // hook chaining by other plugins.
            width = DLG_WIDTH;
            height = DLG_HEIGHT;
            pcxName = DIALOG_BACKGROUND_PCX;
        }
        else if (id == DLG_OK_FRAME_ID) // OK frame
        {
            x = OK_BUTTON_X - BUTTON_FRAME_BORDER;
            y = BUTTON_Y - BUTTON_FRAME_BORDER;
            width = BUTTON_FRAME_WIDTH;
            height = BUTTON_FRAME_HEIGHT;
            pcxName = BUTTON_FRAME_PCX;
        }
        else if (NATIVE_UPGRADE_FRAME_RECT.Matches(x, y, width, height)) // upgrade/cast frame
        {
            // Exact coordinates are part of the native helper signature.
            // The PCX name is deliberately not checked so a previous hook in
            // the chain cannot prevent the relocation.
            x = (ctx->kind == CreatureDialogKind::Battle) ? CAST_BUTTON_X - BUTTON_FRAME_BORDER : UPGRADE_BUTTON_X - BUTTON_FRAME_BORDER;
            y = BUTTON_Y - BUTTON_FRAME_BORDER;
        }
        else if (NATIVE_DISMISS_FRAME_RECT.Matches(x, y, width, height)) // dismiss frame
        {
            x = CAST_BUTTON_X;
            y = BUTTON_Y - BUTTON_FRAME_BORDER;
        }
    }

    return THISCALL_8(H3DlgPcx *, hook->GetDefaultFunc(), item, x, y, width, height, id, pcxName, flags);
}

H3DlgText *__stdcall CreatureDlg_TextCtor(HiHook *hook, H3DlgText *item, int x, int y, int width, int height,
                                          LPCSTR text, LPCSTR font, int color, int itemId, int align, int bkColor,
                                          int unused)
{
    auto *ctx = ActiveBuildContext();
    if (ctx != nullptr)
    {
        const DWORD ret = static_cast<DWORD>(hook->GetReturnAddress());
        switch (itemId)
        {
        case DLG_NAME_ID:
            if (NATIVE_NAME_RECT.Matches(x, y, width, height) && IsKnownConstructorCall(ret, native::NAME_CALLS, ctx))
                x = (DLG_WIDTH - width) / 2;
            break;
        case DLG_DESCRIPTION_ID:
            if (NATIVE_DESCRIPTION_RECT.Matches(x, y, width, height) &&
                IsKnownConstructorCall(ret, native::DESCRIPTION_CALLS, ctx))
            {
                ReadDescriptionRect(x, y, width, height);
                align = GetPluginSettings().descriptionAlignment;
            }
            break;
        default:
            break;
        }
    }

    return THISCALL_12(H3DlgText *, hook->GetDefaultFunc(), item, x, y, width, height, text, font, color, itemId,
                       align, bkColor, unused);
}

H3DlgTextPcx *__stdcall CreatureDlg_TextPcxCtor(HiHook *hook, H3DlgTextPcx *item, int x, int y, int width,
                                                int height, LPCSTR text, LPCSTR font, LPCSTR pcxName, int color,
                                                int itemId, int align, int unused)
{
    auto *ctx = ActiveBuildContext();
    if (ctx != nullptr && itemId == DLG_HINT_ID && NATIVE_HINT_RECT.Matches(x, y, width, height) &&
        SameResource(pcxName, NATIVE_HINT_PCX))
    {
        const DWORD ret = static_cast<DWORD>(hook->GetReturnAddress());
        if (IsKnownConstructorCall(ret, native::HINT_CALLS, ctx))
        {
            y = DLG_HEIGHT - height - HINT_MARGIN;
            width = DLG_WIDTH - HINT_MARGIN * 2;
            pcxName = DIALOG_HINT_BAR_PCX;
        }
    }

    return THISCALL_12(H3DlgTextPcx *, hook->GetDefaultFunc(), item, x, y, width, height, text, font, pcxName, color,
                       itemId, align, unused);
}

H3DlgDef *__stdcall CreatureDlg_DefCtor(HiHook *hook, H3DlgDef *item, int x, int y, int width, int height, int itemId,
                                        LPCSTR defName, int frame, int group, int mirror, int closeDialog, int flags)
{
    auto *ctx = ActiveBuildContext();
    const bool isMorale = itemId == DLG_MORALE_ID && NATIVE_MORALE_RECT.Matches(x, y, width, height);
    const bool isLuck = itemId == DLG_LUCK_ID && NATIVE_LUCK_RECT.Matches(x, y, width, height);
    if (ctx != nullptr && (isMorale || isLuck))
    {
        const DWORD ret = static_cast<DWORD>(hook->GetReturnAddress());
        if (IsKnownConstructorCall(ret, native::MORALE_LUCK_CALLS, ctx))
        {
            x = itemId == DLG_MORALE_ID ? MORALE_X : LUCK_X;
            y = DLG_HEIGHT - height - MORALE_LUCK_BOTTOM_MARGIN;
        }
    }
    return THISCALL_12(H3DlgDef *, hook->GetDefaultFunc(), item, x, y, width, height, itemId, defName, frame, group,
                       mirror, closeDialog, flags);
}

H3DlgDefButton *__stdcall CreatureDlg_DefButtonCtor(HiHook *hook, H3DlgDefButton *item, int x, int y, int width,
                                                    int height, int itemId, LPCSTR defName, int frame, int clickFrame,
                                                    int closeDialog, int hotKey, int flags)
{
    auto *ctx = ActiveBuildContext();
    if (ctx)
    {
        // Item ids are unique inside H3CreatureInfoDlg. Once the build scope
        // proves the parent dialog, do not depend on original coordinates or
        // resource names: an earlier HiHook in the chain may already have
        // changed those arguments.
        if (GetDialogButtonX(itemId, x))
        {
            y = BUTTON_Y;
            width = BUTTON_WIDTH;
            height = BUTTON_HEIGHT;
            if (itemId == DLG_OK_ID)
                defName = OK_BUTTON_DEF;
        }
    }

    return THISCALL_12(H3DlgDefButton *, hook->GetDefaultFunc(), item, x, y, width, height, itemId, defName, frame,
                       clickFrame, closeDialog, hotKey, flags);
}

H3DlgCustomButton *__stdcall CreatureDlg_CustomButtonCtor(HiHook *hook, H3DlgCustomButton *item, int x, int y,
                                                          int width, int height, int itemId, LPCSTR defName,
                                                          H3DlgButton_proc callback, int frame, int clickFrame)
{
    auto *ctx = ActiveBuildContext();
    if (ctx != nullptr && ctx->kind == CreatureDialogKind::Battle && itemId == DLG_CAST_ID)
    {
        x = CAST_BUTTON_X;
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
    const auto spells = CollectActiveSpells(stack);
    if (!spells.count)
        return FALSE;
    int columns = static_cast<int>(sqrt(spells.count));
    int rows = (spells.count + columns - 1) / columns;
    if (rows > columns)
    {
        --rows;
        ++columns;
    }
    H3DefLoader def(NH3Dlg::Assets::SPELL_SMALL);
    if (!def.Get() || def->widthDEF <= 0 || def->heightDEF <= 0)
        return FALSE;
    const int width = (def->widthDEF + SPELL_POPUP_GAP) * columns + SPELL_POPUP_SIZE_PADDING;
    const int height = (def->heightDEF + SPELL_POPUP_GAP) * rows + SPELL_POPUP_SIZE_PADDING;
    H3Dlg popup(width, height);
    for (int slot = 0; slot < spells.count; ++slot)
    {
        const int spellId = spells.ids[slot];
        const int x = SPELL_POPUP_MARGIN + (slot % columns) * (def->widthDEF + SPELL_POPUP_GAP);
        const int y = SPELL_POPUP_MARGIN + (slot / columns) * (def->heightDEF + SPELL_POPUP_GAP);
        if (auto *icon = H3DlgDef::Create(x, y, def->GetName(), spellId + SPELL_DEF_FRAME_OFFSET))
        {
            SetCreatureSpellHints(icon, stack, spellId);
            popup.AddItem(icon);
        }
        if (HasVisibleSpellDuration(spellId))
        {
            char duration[SPELL_DURATION_BUFFER_SIZE];
            std::snprintf(duration, sizeof(duration), "x%d", stack->activeSpellDuration[spellId]);
            if (auto *text = H3DlgText::Create(x, y + SPELL_POPUP_DURATION_Y, def->widthDEF,
                                               SPELL_POPUP_DURATION_HEIGHT, duration, NH3Dlg::Text::TINY,
                                               eTextColor::WHITE, DLG_SPELL_DURATION_FIRST_ID + slot,
                                               eTextAlignment::MIDDLE_RIGHT))
                popup.AddItem(text);
        }
    }
    if (isRMC)
        popup.PlaceAtMouse();
    else if (clickedItem)
    {
        popup.SetX(Clamp(0, clickedItem->GetAbsoluteX(), std::max(0, H3GameWidth::Get() - width - SPELL_POPUP_RIGHT_MARGIN)));
        popup.SetY(Clamp(0, clickedItem->GetAbsoluteY(), std::max(0, H3GameHeight::Get() - height - SPELL_POPUP_BOTTOM_MARGIN)));
    }
    popup.RMB_Show();
    return TRUE;
}
_LHF_(Dlg_CreatureInfo_RmcProc)
{
    if (!GetPluginSettings().showBattleDialog)
        return EXEC_DEFAULT;

    H3Msg *msg = reinterpret_cast<H3Msg *>(c->esi);
    const int itemId = msg->itemId;
    auto *creatureDlgStack = *reinterpret_cast<H3CombatCreature **>(native::CURRENT_CREATURE_STACK);
    if (((itemId >= DLG_NATIVE_SPELL_FIRST_ID && itemId <= DLG_NATIVE_SPELL_LAST_ID) ||
         (itemId >= DLG_NATIVE_SPELL_TEXT_FIRST_ID && itemId <= DLG_NATIVE_SPELL_TEXT_LAST_ID)) &&
        msg->subtype == eMsgSubtype::RBUTTON_DOWN && creatureDlgStack && creatureDlgStack->activeSpellNumber)
    {
        ShowStackActiveSpells(creatureDlgStack, true, nullptr);
        msg->itemId = DLG_DESCRIPTION_ID;
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
        if (!item || !item->GetHint())
            return result;
        auto *hint = dlg->GetTextPcx(DLG_HINT_ID);
        if (!hint)
            return result;
        const char *previous = hint->GetH3String().String();
        if (!previous || libc::strcmp(previous, item->GetHint()))
        {
            hint->SetText(item->GetHint());
            hint->Draw();
            hint->Refresh();
        }
        break;
    }
    case eMsgCommand::MOUSE_BUTTON:
    {
        switch (msg->subtype)
        {
        case eMsgSubtype::LBUTTON_CLICK:
        case eMsgSubtype::RBUTTON_DOWN:
            break;
        default:
            return result;
        }
        switch (msg->itemId)
        {
        case WOG_CREATURE_EXP_BUTTON_ID:
            if (!dlg->GetH3DlgItem(WOG_CREATURE_EXP_BUTTON_ID))
                return result;
            {
                RightClickDialogScope scope(msg->subtype == eMsgSubtype::RBUTTON_DOWN);
                CDECL_0(signed int, native::SHOW_EXPERIENCE_DIALOG);
            }
            if (auto *handler = GetDialogHandler(dlg))
                handler->RefreshCreatureArtifact();
            break;
        case DLG_SPELLS_BTTN_ID:
            if (auto *item = dlg->GetH3DlgItem(DLG_SPELLS_BTTN_ID))
            {
                RightClickDialogScope scope(msg->subtype == eMsgSubtype::RBUTTON_DOWN);
                ShowStackActiveSpells(*reinterpret_cast<H3CombatCreature **>(native::CURRENT_CREATURE_STACK), main_isRMC, item);
            }
            break;
        default:
            break;
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
    x = Clamp(0, x - BATTLE_DIALOG_POSITION_OFFSET, maxX);
    y = Clamp(0, y - BATTLE_DIALOG_POSITION_OFFSET, maxY);
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

    if (P_AdventureMgr && P_AdventureMgr->dlg && result->GetY() + DLG_HEIGHT + ADVENTURE_DIALOG_BOTTOM_MARGIN > P_AdventureMgr->dlg->GetHeight())
        result->SetY(P_AdventureMgr->dlg->GetHeight() - DLG_HEIGHT - ADVENTURE_DIALOG_BOTTOM_MARGIN);

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
        c->return_address = h->GetAddress() + EXPERIENCE_RMB_HOOK_SKIP;
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
        c->return_address = h->GetAddress() + COMMANDER_RMB_HOOK_SKIP;
        main_isRMC = false;
        return NO_EXEC_DEFAULT;
    }
    return EXEC_DEFAULT;
}

} // namespace

void Dlg_CreatureSpellInfo_HooksInit(PatcherInstance *pi)
{
    pi->WriteLoHook(native::CREATURE_RMB_PROC, Dlg_CreatureInfo_RmcProc);
}

void Dlg_CreatureInfo_HooksInit(PatcherInstance *pi)
{
    // High-level runtime hooks for the three creature-info entry points.
    pi->WriteHiHook(native::BUY_CTOR, THISCALL_, H3CreatureInfoDlg_BuyCtor);
    pi->WriteHiHook(native::ARMY_CTOR, THISCALL_, H3CreatureInfoDlg_NotBattleCtor);
    pi->WriteHiHook(native::BATTLE_CTOR, THISCALL_, H3CreatureInfoDlg_BattleCtor);
    pi->WriteHiHook(native::CREATURE_PROC, THISCALL_, H3CreatureInfoDlg_Proc);
    pi->WriteHiHook(native::CREATURE_DTOR, THISCALL_, H3CreatureInfoDlg_Dtor);

    // Replace constructor arguments only while one of the creature-info
    // constructors is on the call stack. Unique item ids/signatures are used
    // where possible; return addresses remain only for ambiguous native items.
    pi->WriteHiHook(native::DLG_CTOR, SPLICE_, EXTENDED_, THISCALL_, CreatureDlg_DlgCtor);
    pi->WriteHiHook(native::PCX_CTOR, SPLICE_, EXTENDED_, THISCALL_, CreatureDlg_PcxCtor);
    pi->WriteHiHook(native::TEXT_CTOR, SPLICE_, EXTENDED_, THISCALL_, CreatureDlg_TextCtor);
    pi->WriteHiHook(native::TEXT_PCX_CTOR, SPLICE_, EXTENDED_, THISCALL_, CreatureDlg_TextPcxCtor);
    pi->WriteHiHook(native::DEF_CTOR, SPLICE_, EXTENDED_, THISCALL_, CreatureDlg_DefCtor);
    pi->WriteHiHook(native::BUTTON_CTOR, SPLICE_, EXTENDED_, THISCALL_, CreatureDlg_DefButtonCtor);
    pi->WriteHiHook(native::CUSTOM_BUTTON_CTOR, SPLICE_, EXTENDED_, THISCALL_, CreatureDlg_CustomButtonCtor);

    H3DLL wndPlugin("wog native dialogs.era");
    if (wndPlugin.dataSize)
    {
        int pluginHookAddress = wndPlugin.NeedleSearch<6>({0x0F, 0x8C, 0xC8, 0xFE, 0xFF, 0xFF}, EXPERIENCE_SHOW_PATTERN_OFFSET);
        if (pluginHookAddress)
            pi->WriteLoHook(pluginHookAddress, Wnd_BeforeExpoDlgShow);
        pluginHookAddress = wndPlugin.NeedleSearch<3>({0x3D, 0x68, 0x02}, COMMANDER_SHOW_PATTERN_OFFSET);
        if (pluginHookAddress)
            pi->WriteLoHook(pluginHookAddress, Before_WndNPC_DLG);
    }
}
