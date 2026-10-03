#include "SpellDescriptionOverlay.h"
#include "SpellDescriptionLogic.h"
#include "SpellDescriptionText.h"
#include "SpellDescriptionTranslations.h"
#include "SpellDescriptionOverlayPixels.h"

#include <algorithm>
#include <cstdio>
#include <new>

namespace SpellDescriptions
{
namespace Overlay
{
namespace
{
H3CombatManager *owner = nullptr;
Detail::ChainRoute route;
int spell = -1;
int side = -1;
int turn = -1;
eCombatAction action = eCombatAction::CANCEL;
int actionParameter = -1;
H3MouseManager::Cursor cursor = {};
bool painting = false;
Detail::PixelBackup pixels;
H3LoadedPcx16 *paintedCanvas = nullptr;
PUINT8 paintedBuffer = nullptr;
int paintedStride = 0, paintedWidth = 0, paintedHeight = 0, paintedBitMode = 0;

void RemoveLayer()
{
    if (!paintedCanvas)
        return;
    const auto windows = H3WindowManager::Get();
    const auto canvas = windows ? windows->GetDrawBuffer() : nullptr;
    // The saved pointer is only an identity: never dereference an old surface.
    if (canvas == paintedCanvas && canvas->buffer == paintedBuffer && canvas->scanlineSize == paintedStride &&
        canvas->width == paintedWidth && canvas->height == paintedHeight && H3BitMode::Get() == paintedBitMode)
        pixels.RestoreUnchanged(canvas->buffer, canvas->scanlineSize);
    paintedCanvas = nullptr;
}

bool VisibleBattle(H3CombatManager *combat)
{
    const auto windows = H3WindowManager::Get();
    return combat && combat == H3CombatManager::Get() && !combat->finished && !combat->IsHiddenBattle() &&
           combat->dlg && windows && windows->lastDlg == combat->dlg;
}

bool CollectingBounds(H3CombatManager *combat)
{
    // H3API leaves this field unnamed. DrawToBuffer checks this DWORD at
    // 0x43E28E to skip pixel output in SetAllRedrawBorders.
    return combat && IntAt(reinterpret_cast<UINT_PTR>(combat) + 0x13D34) != 0;
}

bool Current(H3CombatManager *combat)
{
    const auto mouse = H3MouseManager::Get();
    if (!route.count || combat != owner || !VisibleBattle(combat) || !mouse || combat->autoCombat ||
        combat->currentActiveSide != side || combat->turn != turn || combat->action != action ||
        combat->actionParameter != actionParameter)
        return false;
    if (!Detail::SameTargetCursor(mouse->GetType(), mouse->GetFrame(), cursor.type, cursor.frame,
                                  mouse->GetType() == eCursor::SPELL)) return false;
    if (spell < 0 || spell >= limits::SPELLS) return false;
    if (Detail::GetDamageShape(P_Spell[spell].flags) != Detail::DamageShape::Chain) return false;
    for (std::size_t i = 0; i < route.count; ++i)
    {
        const auto &target = route.targets[i];
        const auto stack = &combat->stacks[target.stack / 21][target.stack % 21];
        if (stack->type < 0 || stack->numberAlive <= 0 || stack->position != target.hex)
            return false;
        if (combat->squares[target.hex].GetMonster() != stack)
            return false;
        if (target.secondHex >= 0 && stack->GetSecondSquare() != target.secondHex)
            return false;
    }
    return true;
}

void Highlight(H3LoadedPcx16 *buffer, const H3CombatSquare &square, int dx, int dy)
{
    const int x = square.left + dx;
    const int y = square.top + dy;
    for (int row = 0; row < 52; ++row)
    {
        const int inset = Detail::HexInset(row);
        const int left = x + inset;
        const int right = x + 44 - inset;
        const int py = y + row;
        if (py < 0 || py >= buffer->height)
            continue;
        const int clippedLeft = (std::max)(0, left);
        const int clippedRight = (std::min)(buffer->width - 1, right);
        if (clippedLeft > clippedRight)
            continue;
        buffer->LightenArea(clippedLeft, py, clippedRight - clippedLeft + 1, 1, 25);
        if (left >= 0 && left < buffer->width)
            buffer->FillRectangle(left, py, (std::min)(2, clippedRight - left + 1), 1, 255, 215, 70);
        if (right >= 0 && right < buffer->width)
            buffer->FillRectangle((std::max)(clippedLeft, right - 1), py,
                                  right - (std::max)(clippedLeft, right - 1) + 1, 1, 255, 215, 70);
    }
}

void Paint(H3CombatManager *combat, H3LoadedPcx16 *buffer, H3Font *font, LPCSTR format,
           const bool *drawHex)
{
    // Paint into the screen canvas after native stack drawing. RemoveLayer
    // strips our previous pixels before the next native draw, avoiding tint buildup.
    const int dx = combat->dlg->GetX();
    const int dy = combat->dlg->GetY();
    for (std::size_t i = 0; i < route.count; ++i)
    {
        const auto &target = route.targets[i];
        if (drawHex[target.hex])
            Highlight(buffer, combat->squares[target.hex], dx, dy);
        if (target.secondHex >= 0 && drawHex[target.secondHex])
            Highlight(buffer, combat->squares[target.secondHex], dx, dy);
    }
    // Badges are painted after all highlights, so overlapping cells cannot tint
    // a previous number. Double-width troops get one badge on their main cell.
    for (std::size_t i = 0; i < route.count; ++i)
    {
        char label[32] = {};
        const auto number = Detail::DecimalNumber(i + 1);
        const int length = std::snprintf(label, sizeof(label), format, number.c_str());
        if (length <= 0 || length >= static_cast<int>(sizeof(label)))
            continue;
        const auto &square = combat->squares[route.targets[i].hex];
        const int width = (std::min)(45, (std::max)(22, font->GetMaxLineWidth(label) + 8));
        const int height = 22;
        const int x = square.left + dx + 22 - width / 2;
        const int y = square.top + dy + 26 - height / 2;
        if (x < 0 || y < 0 || x + width > buffer->width || y + height > buffer->height)
            continue;
        buffer->FillRectangle(x, y, width, height, 15, 15, 15);
        buffer->DrawFrame(x, y, width, height, 255, 215, 70);
        buffer->TextDraw(font, label, x, y, width, height, eTextColor::WHITE);
    }
}

Detail::PixelRect CellRect(H3CombatManager *combat, H3LoadedPcx16 *buffer, int hex)
{
    const auto &square = combat->squares[hex];
    return Detail::ClipPixels(square.left + combat->dlg->GetX(), square.top + combat->dlg->GetY(), 45, 52,
                              buffer->width, buffer->height);
}

void PresentChangedCells(const Detail::ChainRoute &previous, const Detail::ChainRoute &next)
{
    if (painting || !VisibleBattle(owner))
        return;
    auto windows = H3WindowManager::Get();
    auto buffer = windows->GetDrawBuffer();
    if (!buffer)
        return;
    bool cells[187] = {};
    for (const auto candidate : {&previous, &next})
        for (std::size_t i = 0; i < candidate->count; ++i)
        {
            cells[candidate->targets[i].hex] = true;
            if (candidate->targets[i].secondHex >= 0)
                cells[candidate->targets[i].secondHex] = true;
        }
    // Present existing pixels of individual old/new cells. Never call battle
    // Refresh, copy its background, invalidate troop state, or redraw the bar.
    for (int hex = 0; hex < 187; ++hex)
        if (cells[hex])
        {
            const auto rect = CellRect(owner, buffer, hex);
            if (!rect.Empty())
                windows->H3Redraw(rect.x, rect.y, rect.width, rect.height);
        }
}

void ComposeLayer()
{
    if (painting || !route.count || CollectingBounds(H3CombatManager::Get()))
        return;
    RemoveLayer();
    if (!Current(owner))
    {
        Clear(false);
        return;
    }
    auto canvas = H3WindowManager::Get()->GetDrawBuffer();
    auto font = H3SmallFont::Get();
    const auto format = Translation::GetText(Translation::Text::ChainIndex);
    if (!canvas || !canvas->buffer || !font || !format)
    {
        return;
    }
    bool drawHex[187] = {};
    for (std::size_t i = 0; i < route.count; ++i)
    {
        const auto &target = route.targets[i];
        drawHex[target.hex] = true;
        if (target.secondHex >= 0)
            drawHex[target.secondHex] = true;
    }
    Detail::PixelRect rects[84] = {};
    std::size_t count = 0;
    for (int hex = 0; hex < 187; ++hex)
        if (drawHex[hex])
        {
            const auto rect = CellRect(owner, canvas, hex);
            if (!rect.Empty())
                rects[count++] = rect;
        }
    if (!count)
    {
        return;
    }
    try
    {
        if (!pixels.Capture(canvas->buffer, canvas->width, canvas->height, canvas->scanlineSize,
                            H3BitMode::Get() == 4 ? 4 : 2, rects, count))
        {
            return;
        }
    }
    catch (const std::bad_alloc &)
    {
        return;
    }
    Detail::ScopedValues<bool, 1> guard({&painting});
    painting = true;
    Paint(owner, canvas, font, format, drawHex);
    pixels.RecordPaint(canvas->buffer, canvas->scanlineSize);
    paintedCanvas = canvas;
    paintedBuffer = canvas->buffer;
    paintedStride = canvas->scanlineSize;
    paintedWidth = canvas->width;
    paintedHeight = canvas->height;
    paintedBitMode = H3BitMode::Get();
}

// Verified against h3era.exe (ret 0Ch) and NH3API's DrawToBuffer:
// void __thiscall H3CombatCreature::Draw(int x, int y, BOOL numberBoxOnly).
void __stdcall DrawStack(HiHook *hook, H3CombatCreature *stack, int x, int y, BOOL numberBoxOnly)
{
    const bool boundsOnly = CollectingBounds(H3CombatManager::Get());
    if (!painting && !boundsOnly)
        RemoveLayer();
    THISCALL_4(void, hook->GetDefaultFunc(), stack, x, y, numberBoxOnly);
    // No refresh/presentation here. The game's normal output will use the
    // annotated screen buffer, including callers that bypass H3Redraw.
    // Reapply every target: a large neighbouring sprite can overlap its hex.
    if (!boundsOnly)
        ComposeLayer();
}

void __stdcall PresentWindow(HiHook *hook, H3WindowManager *windows, int x, int y, int width, int height)
{
    if (!painting && route.count && windows == H3WindowManager::Get())
    {
        if (!Current(owner))
        {
            const auto previous = route;
            Clear(false);
            THISCALL_5(void, hook->GetDefaultFunc(), windows, x, y, width, height);
            PresentChangedCells(previous, route);
            return;
        }
        const auto canvas = windows->GetDrawBuffer();
        if (canvas)
        {
            const auto update = Detail::ClipPixels(x, y, width, height, canvas->width, canvas->height);
            for (std::size_t i = 0; i < route.count; ++i)
            {
                const auto &target = route.targets[i];
                if (CellRect(owner, canvas, target.hex).Intersects(update) ||
                    (target.secondHex >= 0 && CellRect(owner, canvas, target.secondHex).Intersects(update)))
                {
                    ComposeLayer();
                    break;
                }
            }
        }
    }
    THISCALL_5(void, hook->GetDefaultFunc(), windows, x, y, width, height);
}

void __stdcall ChangeCursor(HiHook *hook, H3MouseManager *mouse, int frame, int type)
{
    THISCALL_3(void, hook->GetDefaultFunc(), mouse, frame, type);
    // The requested frame is NOT the resulting cursor frame: SetCursor uses
    // frame 0 for spell cursors (type 3), and -1 arguments mean keep unchanged.
    // Spell cursor frames also advance during animation without SetCursor.
    // Compare selection identity, not the current animation frame.
    if (route.count && mouse == H3MouseManager::Get() &&
        !Detail::SameTargetCursor(mouse->GetType(), mouse->GetFrame(), cursor.type, cursor.frame,
                                  mouse->GetType() == eCursor::SPELL))
        Clear();
}

_LHF_(BeforeCast)
{
    (void)h;
    (void)c;
    Clear();
    return EXEC_DEFAULT;
}

_ERH_(Reset)
{
    (void)event;
    RemoveLayer();
    route.count = 0;
    owner = nullptr;
    pixels.Release();
}
} // namespace

void Clear(bool redraw)
{
    RemoveLayer();
    if (!route.count)
        return;
    const auto previous = route;
    route.count = 0;
    if (redraw && previous.count)
        PresentChangedCells(previous, route);
}

void Update(H3CombatManager *combat, int spellId, const Detail::ChainRoute &newRoute)
{
    if (!newRoute.count || !VisibleBattle(combat) || !H3MouseManager::Get())
    {
        Clear();
        return;
    }
    const auto previous = route;
    const auto newCursor = H3MouseManager::Get()->CurrentCursor();
    const bool changed = owner != combat || spell != spellId || side != combat->currentActiveSide ||
                         turn != combat->turn || action != combat->action ||
                         actionParameter != combat->actionParameter ||
                         !Detail::SameTargetCursor(newCursor.type, newCursor.frame, cursor.type, cursor.frame,
                                                   newCursor.type == eCursor::SPELL) ||
                         !route.SameTargets(newRoute);
    if (changed)
        RemoveLayer();
    owner = combat;
    route = newRoute;
    spell = spellId;
    side = combat->currentActiveSide;
    turn = combat->turn;
    action = combat->action;
    actionParameter = combat->actionParameter;
    cursor = newCursor;
    if (changed)
        PresentChangedCells(previous, route);
}

void Install(PatcherInstance *pi)
{
    pi->WriteHiHook(0x43DE60, THISCALL_, DrawStack);
    pi->WriteHiHook(0x603190, THISCALL_, PresentWindow);
    pi->WriteHiHook(0x50CEA0, THISCALL_, ChangeCursor);
    pi->WriteLoHook(0x5A0140, BeforeCast);
    Era::RegisterHandler(Reset, "OnSetupBattleField");
    Era::RegisterHandler(Reset, "OnAfterBattleUniversal");
    Era::RegisterHandler(Reset, "OnGameLeave");
}
} // namespace Overlay
} // namespace SpellDescriptions
