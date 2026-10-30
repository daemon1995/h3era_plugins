#pragma once

#include "HelpDialogDependencies.h"
#include "HelpLogic.h"
#include "DlgEnums.h"
#include "DlgPage.h"

#include <string>
#include <cstring>
#include <map>
#include <cctype>

namespace helpdlg
{
// ERA owns translated strings permanently. If an optional language key is
// missing, use a readable English label instead of exposing the JSON key.
inline LPCSTR FallbackText(LPCSTR key)
{
    static const std::map<std::string, std::string> formats = {
        {"help.formats.object_title", "%s (ID: %d)"},
        {"help.formats.object_hint", "%s - %s"},
        {"help.formats.creature_summary", "%s / Level %d"},
        {"help.formats.recruitment", "%s / Weekly growth: %d"},
        {"help.formats.shots", "Shots: %d"},
        {"help.formats.spell_charges", "Spell charges: %d"},
        {"help.formats.level", "Level %d"},
        {"help.formats.artifact_summary", "Cost: %d gold / %s"},
        {"help.formats.town_roster", "Level %d: %s / %s\n"},
        {"help.formats.results", "Results: %d"},
        {"help.formats.catalogue_results", "Results: %d / %d"},
        {"help.formats.category_count", "%s (%d)"},
        {"help.formats.spellbook", "Yes / %s"},
        {"help.formats.spell_strength", "%d %s %s x SP"},
    };
    const std::string fullKey = key ? key : "";
    const auto format = formats.find(fullKey);
    if (format != formats.end())
        return format->second.c_str();

    static std::map<std::string, std::string> labels;
    auto found = labels.find(fullKey);
    if (found == labels.end())
    {
        const auto separator = fullKey.find_last_of('.');
        std::string label = fullKey.substr(separator == std::string::npos ? 0 : separator + 1);
        for (auto &character : label)
        {
            if (character == '_')
                character = ' ';
        }
        if (!label.empty())
            label[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(label[0])));
        found = labels.emplace(fullKey, std::move(label)).first;
    }
    return found->second.c_str();
}

// ERA owns translated strings permanently; callers borrow them. Fallback
// strings are cached for the process lifetime; formatted text owns its result.
inline LPCSTR Text(LPCSTR key)
{
    bool found = false;
    LPCSTR value = EraJS::read(key, found);
    return found && value && *value ? value : FallbackText(key);
}

inline LPCSTR Borrow(LPCSTR value) noexcept
{
    return value ? value : h3_NullString;
}

inline std::string Safe(LPCSTR value)
{
    return Borrow(value);
}

template <typename... Args> inline std::string FormatText(LPCSTR key, Args... args)
{
    return H3String::Format(Text(key), args...).String();
}

inline std::string ObjectTitle(LPCSTR name, int id)
{
    return id < 0 ? Safe(name) : FormatText("help.formats.object_title", Borrow(name), id);
}

constexpr LPCSTR kSelectorPcx = "ERAHDSLT.PCX";
constexpr int kSelectorArrowWidth = 22;
constexpr LPCSTR kArtifactSlotPcx = "artslot.pcx";
constexpr int kArtifactSlotSize = 47;

inline H3LoadedPcx *CreateSelectorBackground(int width, LPCSTR name = kSelectorPcx,
                                             int height = kPageButtonHeight, bool recessed = false,
                                             int brightness = 50)
{
    auto *pcx = H3LoadedPcx::Create(name, width, height);
    if (!pcx)
        return nullptr;
    H3PcxLoader tile(NH3Dlg::Assets::DIBOXBACK);
    for (int index = 0; index < 256; ++index)
    {
        const auto color = tile.Get() ? tile->palette888.color[index] : H3RGB888(35, 30, 24);
        pcx->palette888.color[index] =
            H3RGB888(color.r * brightness / 100, color.g * brightness / 100, color.b * brightness / 100);
    }
    pcx->palette565.InitiateFromPalette888(pcx->palette888);
    bool usedColors[256] = {};
    for (int y = 0; y < pcx->height; ++y)
        for (int x = 0; x < pcx->width; ++x)
        {
            const auto index = tile.Get() && tile->buffer && tile->width > 0 && tile->height > 0
                                   ? *tile->GetPixelIndex(x % tile->width, y % tile->height)
                                   : 0;
            *pcx->GetPixelIndex(x, y) = index;
            usedColors[index] = true;
        }
    if (recessed && width > 2 && height > 2)
    {
        int dark = -1, light = -1;
        const auto brightness = [&](int index) {
            const auto &color = pcx->palette888.color[index];
            return color.r + color.g + color.b;
        };
        // Ignore unused palette entries, including the magenta transparency color.
        for (int index = 0; index < 256; ++index)
        {
            if (!usedColors[index])
                continue;
            if (dark < 0 || brightness(index) < brightness(dark))
                dark = index;
            if (light < 0 || brightness(index) > brightness(light))
                light = index;
        }
        for (int x = 0; x < width; ++x)
        {
            *pcx->GetPixelIndex(x, 0) = dark;
            *pcx->GetPixelIndex(x, height - 1) = light;
        }
        for (int y = 1; y < height - 1; ++y)
        {
            *pcx->GetPixelIndex(0, y) = dark;
            *pcx->GetPixelIndex(width - 1, y) = light;
        }
    }
    // The dialog owns this reference. SelectorTextPcx explicitly releases the
    // references obtained by native TextPcx constructors before this owner dies.
    pcx->IncreaseReferences();
    // This H3API version's AddToResourceManager inserts only the name and leaves
    // m_item null. The native PCX loader dereferences it on a cache hit (0x55AA67).
    // Insert the complete record, as ERA's TBinaryTreeNode.AddItem does.
    H3ResourceItemData resource(pcx->GetName());
    resource.m_item = pcx;
    struct InsertResult
    {
        H3ResourceManager::Node *node;
        BOOL8 inserted;
    } result;
    THISCALL_3(InsertResult &, 0x55DDF0, H3ResourceManager::Get(), &result, &resource);
    return pcx;
}

struct SelectorTextPcx : H3DlgTextPcx
{
    void ReleaseBackground()
    {
        auto *pcx = loadedPcx;
        loadedPcx = nullptr;
        if (pcx)
            pcx->Dereference();
    }
};

inline H3DlgTextPcx *SelectorText(int x, int y, int width, int height, int id, LPCSTR text = h3_NullString,
                                  LPCSTR background = kSelectorPcx)
{
    const auto *cached = H3ResourceManager::Get()->FindItem(background);
    if (!cached || !cached->data.m_item)
        return nullptr;
    auto *item = H3ObjectAllocator<SelectorTextPcx>().allocate(1);
    if (item)
        THISCALL_12(SelectorTextPcx *, 0x5BCB70, item, x, y, width, height, text, NH3Dlg::Text::SMALL, background,
                    eTextColor::REGULAR, id, eTextAlignment::MIDDLE_CENTER, 8);
    return item;
}

inline void ReleaseSelectorText(H3DlgTextPcx *item)
{
    // The native TextPcx destructor only destroys its text; it borrows the PCX.
    if (item)
        static_cast<SelectorTextPcx *>(item)->ReleaseBackground();
}

inline H3DlgPcx16 *CreateObjectCaption(int x, int y, int width, int height, int id)
{
    auto *canvas = H3LoadedPcx16::Create(width, height);
    if (!canvas)
        return nullptr;
    auto *item = H3DlgPcx16::Create(x, y, width, height, id, nullptr);
    if (!item)
    {
        canvas->Destroy();
        return nullptr;
    }
    canvas->FillRectangle(0, 0, width, height, 35, 30, 24);
    item->SetPcx(canvas);
    return item;
}

inline void SetObjectCaptionText(H3Dlg &dialog, H3DlgPcx16 *item, LPCSTR text)
{
    auto *canvas = item ? item->GetPcx() : nullptr;
    if (!canvas)
        return;
    const int width = item->GetWidth(), height = item->GetHeight();
    // Always restore the original background before shading or changing text.
    // The dialog background remains untouched when the widget is hidden.
    if (auto *background = dialog.GetBackgroundPcx())
        background->DrawToPcx16(0, 0, FALSE, canvas, item->GetX(), item->GetY());
    else
        canvas->FillRectangle(0, 0, width, height, 35, 30, 24);
    canvas->DarkenArea(1, 1, width - 2, height - 2, 30);
    // The reference captions have a recessed edge: dark top/left, light bottom/right.
    canvas->SinkArea(0, 0, width, height);
    H3BinaryLoader<H3Font> font(NH3Dlg::Text::SMALL);
    if (font.Get())
        canvas->TextDraw(font.Get(), text, 2, 0, width - 4, height, eTextColor::REGULAR,
                         eTextAlignment::MIDDLE_CENTER);
}

inline void ReleaseObjectCaption(H3DlgPcx16 *item)
{
    if (auto *canvas = item ? item->GetPcx() : nullptr)
    {
        item->SetPcx(nullptr);
        canvas->Destroy();
    }
}

inline H3DlgHintBar *CreateHintBar(H3Dlg &dialog, int x, int y, int width, int height, LPCSTR background)
{
    const auto *cached = H3ResourceManager::Get()->FindItem(background);
    if (!cached || !cached->data.m_item)
        return nullptr;
    auto *hint = H3ObjectAllocator<H3DlgHintBar>().allocate(1);
    if (!hint)
        return nullptr;
    // H3DlgHintBar appends a private lastHint pointer to the native TextPcx.
    // Initialize its storage before constructing the base, as H3API's factory does.
    std::memset(hint, 0, sizeof(*hint));
    THISCALL_12(H3DlgHintBar *, 0x5BCB70, hint, x, y, width, height, h3_NullString, NH3Dlg::Text::SMALL,
                background, eTextColor::REGULAR, 0, eTextAlignment::MIDDLE_CENTER, 8);
    dialog.AddItem(hint);
    return hint;
}

inline H3DlgFrame *GoldFrame(H3Dlg &dialog, int x, int y, int width, int height)
{
    auto *frame = dialog.CreateFrame(x, y, width, height, -1, H3RGB565(H3RGB888::Highlight()));
    if (frame)
    {
        frame->Disable();
        frame->DeActivate();
    }
    return frame;
}

inline void PlaceSelectorFrame(H3DlgFrame *frame, const H3DlgItem *row)
{
    if (!frame)
        return;
    if (row && row->IsVisible())
    {
        frame->SetX(row->GetX());
        frame->SetY(row->GetY());
        frame->SetWidth(row->GetWidth());
        frame->SetHeight(row->GetHeight());
        frame->Show()->DeActivate();
    }
    else
        frame->HideDeactivate();
}

inline bool AddSafeBackground(H3Dlg &dialog, bool decorativeFrame = true)
{
    const int width = dialog.GetWidth(), height = dialog.GetHeight();
    auto *canvas = H3LoadedPcx16::Create(width, height);
    if (!canvas)
        return false;
    canvas->FillRectangle(0, 0, width, height, 35, 30, 24);
    H3PcxLoader tile(NH3Dlg::Assets::DIBOXBACK);
    if (tile.Get())
        ForEachBackgroundTile(width, height, tile->width, tile->height,
                              [&](int x, int y, int w, int h) { tile->DrawToPcx16(0, 0, w, h, canvas, x, y, FALSE); });
    // Avoid H3API::BackgroundRegion: it passes the remaining dialog width
    // instead of the tile width, reading beyond DIBOXBACK in HD's renderer.
    if (decorativeFrame)
        canvas->SimpleFrameRegion(0, 0, width, height);
    const bool added = dialog.AddBackground(canvas) != FALSE;
    canvas->Destroy();
    return added;
}

inline H3DlgDefButton *CloseButtonRight(H3Dlg &dialog, int rightEdge = -1, int buttonY = -1)
{
    H3PcxLoader box(NH3Dlg::Assets::BOX_64_30_PCX);
    const int boxWidth = box.Get() ? box->width : 64;
    const int x = rightEdge < 0 ? dialog.GetWidth() - 82 : rightEdge + 1 - boxWidth;
    const int y = buttonY < 0 ? dialog.GetHeight() - 48 : buttonY;
    auto *button = dialog.CreateOKButton(x, y);
    if (button)
    {
        // Set the position after construction as other plugins can hook the
        // native button constructor. Its surrounding box is created at x/y.
        button->SetX(x);
        button->SetY(y);
        button->AddHotkey(eVKey::H3VK_ESCAPE);
    }
    return button;
}
inline void ShowDetailsDialog(H3Dlg &dialog, bool popup)
{
    if (popup)
        dialog.RMB_Show();
    else
    {
        CloseButtonRight(dialog);
        dialog.Start();
    }
}

inline H3DlgCaptionButton *Button(H3Dlg *dialog, int x, int y, int width, int height, int id, LPCSTR text,
                                  LPCSTR defName = "OVBUTN3.def")
{
    auto *button = H3DlgCaptionButton::Create(x, y, width, height, id, defName, text, NH3Dlg::Text::SMALL, 0, 0, false,
                                              0, eTextColor::REGULAR);
    if (button)
    {
        button->SetClickFrame(1);
        if (dialog)
            dialog->AddItem(button);
    }
    return button;
}

inline H3DlgItem *IconButton(H3Dlg *dialog, int x, int y, int width, int height, int id, LPCSTR defName,
                             LPCSTR fallbackText = h3_NullString)
{
    H3DlgItem *item = nullptr;
    H3DefLoader def(defName);
    if (def.Get() && def->widthDEF > 0 && def->heightDEF > 0)
        item = H3DlgDefButton::Create(x, y, width, height, id, defName, 0, 1, FALSE, 0);
    if (!item)
        item = Button(nullptr, x, y, width, height, id, fallbackText, "OVBUTN2.def");
    if (item && dialog)
        dialog->AddItem(item);
    return item;
}

inline H3DlgCaptionButton *CategoryButton(int x, int y, int id, LPCSTR text)
{
    auto *button = H3DlgCaptionButton::Create(x, y, id, main::DlgPage::CATEGORY_BUTTON_DEF, text, NH3Dlg::Text::SMALL,
                                              0, 0, false, 0, eTextColor::REGULAR);
    if (button)
        button->SetClickFrame(1);
    return button;
}

inline int CategoryButtonStep()
{
    H3DefLoader def(main::DlgPage::CATEGORY_BUTTON_DEF);
    return def.Get() && def->heightDEF > 0 ? def->heightDEF + 4 : 40;
}

inline void SetCategoryState(H3DlgCaptionButton *button, bool selected)
{
    if (!button)
        return;
    button->SendCommand(6 - selected, 4096);
}

inline LPCSTR PageName(main::eHelpPage page)
{
    switch (page)
    {
    case main::eHelpPage::MODS:
        return Text("help.ui.mods");
    case main::eHelpPage::HOTKEYS:
        return Text("help.ui.hotkeys");
    case main::eHelpPage::CREATURES:
        return Text("help.ui.creatures");
    case main::eHelpPage::ARTIFACTS:
        return Text("help.ui.artifacts");
    case main::eHelpPage::TOWNS:
        return Text("help.ui.towns");
    case main::eHelpPage::HEROES:
        return Text("help.ui.heroes");
    case main::eHelpPage::SECONDARY_SKILLS:
        return Text("help.ui.skills");
    case main::eHelpPage::SPELLS:
        return Text("help.ui.spells");
    default:
        return Text("help.ui.help");
    }
}

inline LPCSTR ContextName(int context)
{
    if (context == -1)
        return Text("help.contexts.everywhere");
    return Text(H3String::Format("help.contexts.%d", Bound(context, 0, 6)).String());
}
} // namespace helpdlg
