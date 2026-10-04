#include "ObjectBansDlg.h"
#include "OptionsMenuUI.h"
#include "GameContext.h"
#include "../headers/EraPluginsAPI/HelpDialogAPI.hpp"
#include <algorithm>

namespace
{
constexpr int TabFirst = 100, QueryId = 12, GroupId = 13, LevelId = 14, StatusId = 15;
constexpr int BanShownId = 16, AllowShownId = 17, SourceFirst = 20, CellFirst = 1000, CheckFirst = 20000;
constexpr int GridX = 18;
constexpr int OptionsId = 2;
constexpr int CellCaptionHeight = 36;
const char *SourceNames[] = {"Mage Guilds and Pyramids", "Spell Shrines", "Scholars", "Scrolls and Pandora's Boxes", "Heroes' starting spells"};
const char *SpellGroups[] = {"All schools", "Fire", "Air", "Water", "Earth", "Creature abilities"};
const char *ArtifactGroups[] = {"All classes", "Treasure", "Minor", "Major", "Relic", "Special"};
const int GroupCounts[] = {6, 6, 10, 1};
const era_help::Page HelpPages[] = {era_help::Page::Spells, era_help::Page::Artifacts,
    era_help::Page::Heroes, era_help::Page::SecondarySkills};
const char *Statuses[] = {"All objects", "Allowed", "Banned"};
const char *Text(const char *key, const char *fallback)
{
    const std::string path = std::string("era_options.bans_ui.") + key;
    const char *value = Era::tr(path.c_str());
    return value && path != value ? value : fallback;
}
H3DlgCaptionButton *Button(H3Dlg &dlg, int x, int y, int width, int id, const char *text, int height = 26)
{
    auto *button = dlg.CreateCaptionButton(x, y, width, height, id, "OVBUTN3.def", text, NH3Dlg::Text::SMALL, 0);
    if (button) button->SetClickFrame(1);
    return button;
}
class AdjustableScrollbar : public H3DlgScrollbar
{
  public:
    void SetRange(int count) const { vSetTickCount(count); }
};
unsigned GameCodePage() { return Era::GetCodePage(); }
}

era_options::BanObjectKind ObjectBansDlg::Kind() const { return static_cast<era_options::BanObjectKind>(tab); }
int ObjectBansDlg::GridRows() const { return (std::max)(1, (heightDlg - GridTop() - 66) / cellHeights[tab]); }

const era_options::ObjectBanRules &ObjectBansDlg::Rules() const
{
    return currentMap ? mapRules : era_options::ObjectBanPreferences();
}

ObjectBansDlg::ObjectBansDlg(int width, int height, const State &state, bool allowEditing, bool currentMap) : OptionsDialog(width, height),
    tab(state.tab >= 0 && state.tab < era_options::BanObjectKindCount ? state.tab : 0),
    currentMap(currentMap || era_options::IsOnMap()),
    allowEditing(era_options::AllowDialogEditing(allowEditing, this->currentMap)), filters(state.filters)
{
    if (this->currentMap) mapRules = era_options::CurrentMapObjectBans();
    background = era_options::TiledBackground(width, height);
    if (background)
    {
        FrameRegion(0, 0, width, height, TRUE, 0);
        if (auto *item = H3DlgPcx16::Create(0, 0, width, height, 0, nullptr)) { item->SetPcx(background); AddItem(item); }
    }
    hintBar = CreateHint();
    CreateText(18, 12, width - 274, 26, this->currentMap ? Text("map_title", "Current map bans (read only)") :
        this->allowEditing ? Text("title", "Object bans for new maps") : Text("view_title", "Object bans (read only)"), NH3Dlg::Text::BIG,
        eTextColor::GOLD, -1, eTextAlignment::MIDDLE_LEFT);
    Button(*this, width - 246, 12, 134, OptionsId, Text("options", "WoG / ERA options"));
    auto *close = Button(*this, width - 100, 12, 80, 1, Text("back", "Back"));
    if (close) close->AddHotkey(eVKey::H3VK_ESCAPE);
    const char *tabKeys[] = {"spells", "artifacts", "heroes", "skills"};
    const char *tabNames[] = {"Spells", "Artifacts", "Heroes", "Skills"};
    for (int kind = 0; kind < era_options::BanObjectKindCount; ++kind)
        tabs[kind] = Button(*this, 18 + kind * 112, 44, 104, TabFirst + kind, Text(tabKeys[kind], tabNames[kind]));
    for (auto *button : {Button(*this, width - 282, 44, 126, BanShownId, Text("ban_shown", "Ban shown")),
                        Button(*this, width - 148, 44, 126, AllowShownId, Text("allow_shown", "Allow shown"))})
        if (button && !this->allowEditing) { button->Shade(); button->DeActivate(); }
    CreateText(18, 78, 52, 22, Text("search", "Search:"), NH3Dlg::Text::SMALL, eTextColor::REGULAR, -1, eTextAlignment::MIDDLE_LEFT);
    query = CreateEdit(74, 78, width - 448, 22, 255, "", NH3Dlg::Text::SMALL, eTextColor::REGULAR,
        eTextAlignment::MIDDLE_LEFT, nullptr, QueryId, TRUE, 2, 2);
    if (query) query->SetAutoredraw(TRUE);
    group = Button(*this, width - 366, 76, 142, GroupId, "", 26);
    level = Button(*this, width - 216, 76, 70, LevelId, "", 26);
    status = Button(*this, width - 138, 76, 116, StatusId, "", 26);
    const int sourceWidth = (width - 36) / 3;
    for (int source = 0; source < era_options::BanSourceCount; ++source)
    {
        const int x = 18 + source % 3 * sourceWidth, y = 110 + source / 3 * 26;
        auto *box = H3DlgDefButton::Create(x, y + 2, SourceFirst + source, "checkbox.def", 0, 1, false, 0);
        if (box) AddItem(box);
        sources.push_back(box);
        const std::string key = std::string("source_") + era_options::BanSourceKey(source);
        sourceNames.push_back(CreateText(x + 24, y, sourceWidth - 28, 24, Text(key.c_str(), SourceNames[source]),
            NH3Dlg::Text::SMALL, eTextColor::REGULAR, SourceFirst + source, eTextAlignment::MIDDLE_LEFT));
    }
    H3DefLoader spellDef(NH3Dlg::Assets::SPELLS_DEF), artifactDef(NH3Dlg::Assets::ARTIFACT_DEF), skillDef(NH3Dlg::Assets::SSKILL_44);
    const int portraitWidth = (std::max)(58, (std::max)(spellDef.Get() ? spellDef->widthDEF : 78,
        (std::max)(artifactDef.Get() ? artifactDef->widthDEF : 44, skillDef.Get() ? skillDef->widthDEF : 44)));
    columns = (std::max)(1, (width - 56) / (portraitWidth + 18));
    cellWidth = (width - 56) / columns;
    const int portraitHeights[] = {spellDef.Get() ? spellDef->heightDEF : 65,
        artifactDef.Get() ? artifactDef->heightDEF : 44, 64, skillDef.Get() ? skillDef->heightDEF : 44};
    int maximumRows = 1;
    for (int kind = 0; kind < era_options::BanObjectKindCount; ++kind)
    {
        // Compact 44px icons need less vertical space than spells and heroes.
        // Keep the same columns and caption area, with four rows at 800x600.
        cellHeights[kind] = (std::max)(1, portraitHeights[kind]) + CellCaptionHeight + 6;
        const int top = kind ? 112 : 170;
        maximumRows = (std::max)(maximumRows, (height - top - 66) / cellHeights[kind]);
        objects[kind] = era_options::NativeBanObjects(static_cast<era_options::BanObjectKind>(kind));
    }
    const int slots = columns * maximumRows;
    for (int slot = 0; slot < slots; ++slot)
    {
        Cell cell;
        cell.name = Button(*this, 0, 0, cellWidth - 6, CellFirst + slot, "", CellCaptionHeight);
        // Shared loaded resources are owned by their native controls. DEF tabs
        // keep separate portraits; there is no manual DEF pointer replacement.
        const char *defs[] = {NH3Dlg::Assets::SPELLS_DEF, NH3Dlg::Assets::ARTIFACT_DEF, NH3Dlg::Assets::SSKILL_44};
        for (int image = 0; image < 3; ++image)
        {
            cell.images[image] = H3DlgDef::Create(0, 0, CellFirst + slot, defs[image], 0);
            if (cell.images[image]) AddItem(cell.images[image]);
        }
        cell.hero = H3DlgPcx::Create(0, 0, 58, 64, CellFirst + slot, nullptr);
        if (cell.hero) AddItem(cell.hero);
        cell.check = H3DlgDefButton::Create(0, 0, CheckFirst + slot, "checkbox.def", 0, 1, false, 0);
        if (cell.check) AddItem(cell.check);
        cell.bannedFrame = H3DlgFrame::Create(0, 0, cellWidth - 6, cellHeights[0] - 3, -1, H3RGB565(H3RGB888(235,70,60)));
        if (cell.bannedFrame) { AddItem(cell.bannedFrame); cell.bannedFrame->DeActivate(); }
        cells.push_back(cell);
    }
    for (int kind = 0; kind < era_options::BanObjectKindCount; ++kind)
    {
        const int top = kind ? 112 : 170;
        scrollbars[kind] = H3DlgScrollbar::Create(width - 32, top, 16, height - top - 66, 40 + kind, 2, ScrollProc, false, 1, true);
        if (scrollbars[kind]) AddItem(scrollbars[kind]);
    }
    count = CreateText(18, height - 62, width - 36, 22, "", NH3Dlg::Text::SMALL, eTextColor::GOLD, -1, eTextAlignment::MIDDLE_LEFT);
    if (query) query->SetText(filters[tab].query.c_str());
    SwitchTab(tab);
}

ObjectBansDlg::~ObjectBansDlg()
{
    // Native PCX controls keep raw pointers. Detach before releasing the cache
    // so changing tabs, searches and closing the dialog cannot leave them stale.
    for (auto &cell : cells) if (cell.hero) cell.hero->SetPcx(static_cast<H3LoadedPcx *>(nullptr));
    for (auto *portrait : heroPortraits) if (portrait) portrait->Dereference();
}

void ObjectBansDlg::SwitchTab(int next)
{
    if (next < 0 || next >= era_options::BanObjectKindCount) return;
    if (query && query->GetText()) filters[tab].query = query->GetText();
    tab = next;
    if (query) query->SetText(filters[tab].query.c_str());
    for (int source = 0; source < era_options::BanSourceCount; ++source)
    {
        for (auto *item : {static_cast<H3DlgItem *>(sources[source]), static_cast<H3DlgItem *>(sourceNames[source])})
            if (item) { if (!tab) item->ShowActivate(); else item->HideDeactivate(); }
    }
    for (int kind = 0; kind < era_options::BanObjectKindCount; ++kind)
    {
        if (tabs[kind]) tabs[kind]->SetFrame(tab == kind ? 1 : 0);
        if (scrollbars[kind]) scrollbars[kind]->HideDeactivate();
    }
    if (level) { if (!tab) level->ShowActivate(); else level->HideDeactivate(); }
    if (group) { if (GroupCounts[tab] > 1) group->ShowActivate(); else group->HideDeactivate(); }
    Rebuild(false);
}

void ObjectBansDlg::Rebuild(bool reset)
{
    if (reset) filters[tab].firstRow = 0;
    filtered = era_options::FilterBanObjects(objects[tab], Rules(), Kind(), filters[tab], GameCodePage());
    Refresh();
}

void ObjectBansDlg::Refresh()
{
    auto &filter = filters[tab];
    const int cellHeight = cellHeights[tab];
    const auto &rules = Rules();
    if (group)
    {
        const char *label = tab == 0 ? SpellGroups[filter.group] : tab == 1 ? ArtifactGroups[filter.group] :
            tab == 2 ? (filter.group ? P_TownNames[filter.group - 1] : Text("all_towns", "All towns")) : "";
        group->SetText(label ? label : "");
    }
    if (status) status->SetText(Statuses[filter.status]);
    if (level) level->SetText(filter.level ? ("Level " + std::to_string(filter.level)).c_str() : "All levels");
    for (int source = 0; source < era_options::BanSourceCount; ++source)
        if (sources[source])
        {
            const int frame = allowEditing ? (rules.sources[source] ? 2 : 0) : 4 + rules.sources[source];
            sources[source]->SetFrame(frame);
            sources[source]->SetClickFrame(allowEditing ? frame + 1 : frame);
        }
    const int rows = GridRows(), maximum = (std::max)(0, (static_cast<int>(filtered.size()) + columns - 1) / columns - rows);
    filter.firstRow = (std::max)(0, (std::min)(filter.firstRow, maximum));
    auto *scroll = scrollbars[tab];
    if (scroll)
    {
        if (maximum)
        {
            if (scroll->GetTicksCount() != maximum + 1) reinterpret_cast<AdjustableScrollbar *>(scroll)->SetRange(maximum + 1);
            scroll->SetTick(filter.firstRow);
            scroll->SetButtonPosition();
            scroll->ShowActivate();
        }
        else scroll->HideDeactivate();
    }
    for (size_t slot = 0; slot < cells.size(); ++slot)
    {
        auto &cell = cells[slot];
        const int index = filter.firstRow * columns + static_cast<int>(slot);
        cell.object = index < static_cast<int>(filtered.size()) && static_cast<int>(slot) < rows * columns ? filtered[index] : -1;
        for (auto *item : {static_cast<H3DlgItem *>(cell.name), static_cast<H3DlgItem *>(cell.images[0]), static_cast<H3DlgItem *>(cell.images[1]),
            static_cast<H3DlgItem *>(cell.images[2]), static_cast<H3DlgItem *>(cell.hero),
            static_cast<H3DlgItem *>(cell.check), static_cast<H3DlgItem *>(cell.bannedFrame)}) if (item) item->HideDeactivate();
        if (cell.hero) cell.hero->SetPcx(static_cast<H3LoadedPcx *>(nullptr));
        if (cell.object < 0) continue;
        const auto &entry = objects[tab][cell.object];
        const int x = GridX + static_cast<int>(slot % columns) * cellWidth, y = GridTop() + static_cast<int>(slot / columns) * cellHeight;
        const bool banned = rules.IsBanned(Kind(), entry.id);
        const std::string hint = entry.name + (!entry.selectable ? " - creature ability. " : banned ? " - banned. " : " - allowed. ") +
            (entry.selectable && allowEditing ? "Left: toggle ban. Right: hold for preview." :
                "Read only. Right: hold for preview.");
        if (cell.name)
        {
            cell.name->SetX(x); cell.name->SetY(y + cellHeight - CellCaptionHeight - 4);
            cell.name->SetText(entry.name.c_str()); cell.name->SetFrame(banned ? 1 : 0);
            cell.name->SetHints(hint.c_str(), nullptr, true); cell.name->ShowActivate();
            cell.name->UnShade();
        }
        auto *image = tab == 2 ? nullptr : cell.images[tab == 3 ? 2 : tab];
        if (image)
        {
            auto *def = image->GetDef();
            const int frame = tab == 3 ? 3 + entry.id * 3 : entry.id;
            if (def && def->groups && def->groupsCount && def->groups[0] && frame >= 0 && frame < def->groups[0]->count)
            {
                image->SetX(x + (cellWidth - def->widthDEF) / 2); image->SetY(y);
                image->SetFrame(frame); image->SetHints(hint.c_str(), nullptr, true); image->ShowActivate();
            }
        }
        if (tab == 2 && cell.hero && entry.id >= 0 && entry.id < era_options::BanHeroCount)
        {
            auto *&portrait = heroPortraits[entry.id];
            const char *path = P_HeroInfo[entry.id].largePortrait;
            if (!portrait && path && *path) portrait = H3LoadedPcx::Load(path);
            if (portrait && portrait->width > 0 && portrait->width <= 58 && portrait->height > 0 && portrait->height <= 64)
            {
                cell.hero->SetPcx(portrait); cell.hero->SetX(x + (cellWidth - portrait->width) / 2); cell.hero->SetY(y);
                cell.hero->SetHints(hint.c_str(), nullptr, true); cell.hero->ShowActivate();
            }
        }
        if (cell.check)
        {
            const bool editable = entry.selectable && allowEditing;
            const int frame = editable ? (banned ? 2 : 0) : 4 + banned;
            cell.check->SetX(x + 2); cell.check->SetY(y + 2); cell.check->SetFrame(frame);
            cell.check->SetClickFrame(editable ? frame + 1 : frame);
            const std::string checkHint = entry.name + (editable ? (banned ? " - allow this object." : " - ban this object.") :
                " - read only; hold right click for preview.");
            cell.check->SetHints(checkHint.c_str(), nullptr, true); cell.check->ShowActivate();
        }
        if (cell.bannedFrame && banned)
        {
            cell.bannedFrame->SetX(x); cell.bannedFrame->SetY(y); cell.bannedFrame->SetHeight(cellHeight - 3); cell.bannedFrame->Show();
        }
    }
    if (count)
    {
        const auto &banned = rules.Ids(Kind());
        const char *contexts[] = {" | Select spell sources above", " | Random artifact pool", " | Tavern recruitment", " | New skill offers at level-up"};
        const std::string label = std::to_string(filtered.size()) + " / " + std::to_string(objects[tab].size()) +
            " | Banned: " + std::to_string(banned.size()) + (currentMap ? " | Current map: read only" :
                allowEditing ? contexts[tab] : " | Read only");
        count->SetText(label.c_str());
    }
    if (P_WindowManager->lastDlg == this) Redraw();
}

const era_options::BanObjectInfo *ObjectBansDlg::AtControl(int id) const
{
    const int slot = id - (id >= CheckFirst ? CheckFirst : CellFirst);
    return slot >= 0 && slot < static_cast<int>(cells.size()) && cells[slot].object >= 0 ? &objects[tab][cells[slot].object] : nullptr;
}
void ObjectBansDlg::SetVisibleBans(bool banned)
{
    if (!allowEditing) return;
    for (const auto index : filtered)
        if (objects[tab][index].selectable) era_options::SetObjectBan(Kind(), objects[tab][index].id, banned);
    Rebuild(false);
}
BOOL ObjectBansDlg::OnLeftClick(INT id, H3Msg &)
{
    if (id == 1) { SaveAndClose(); return FALSE; }
    if (id == OptionsId) { if (SaveAndClose()) optionsRequested = true; return FALSE; }
    if (id >= TabFirst && id < TabFirst + era_options::BanObjectKindCount) { SwitchTab(id - TabFirst); return FALSE; }
    if (id == BanShownId || id == AllowShownId) { SetVisibleBans(id == BanShownId); return FALSE; }
    if (id == GroupId) { filters[tab].group = (filters[tab].group + 1) % GroupCounts[tab]; Rebuild(); return FALSE; }
    if (id == LevelId) { filters[tab].level = (filters[tab].level + 1) % 6; Rebuild(); return FALSE; }
    if (id == StatusId) { filters[tab].status = (filters[tab].status + 1) % 3; Rebuild(); return FALSE; }
    const int source = id - SourceFirst;
    if (!tab && source >= 0 && source < era_options::BanSourceCount)
    { if (allowEditing) era_options::SetObjectBanSource(source, !Rules().sources[source]); Refresh(); return FALSE; }
    if (const auto *entry = AtControl(id))
    {
        if (allowEditing && entry->selectable) era_options::SetObjectBan(Kind(), entry->id, !Rules().IsBanned(Kind(), entry->id));
        Rebuild(false);
        return FALSE;
    }
    return TRUE;
}
BOOL ObjectBansDlg::OnRightClick(H3DlgItem *item)
{
    const int id = item ? item->GetID() : -1;
    if (id == GroupId) { filters[tab].group = (filters[tab].group + GroupCounts[tab] - 1) % GroupCounts[tab]; Rebuild(); return FALSE; }
    if (id == LevelId) { filters[tab].level = (filters[tab].level + 5) % 6; Rebuild(); return FALSE; }
    if (id == StatusId) { filters[tab].status = (filters[tab].status + 2) % 3; Rebuild(); return FALSE; }
    if (const auto *entry = AtControl(id))
    {
        ShowPreview(*entry); return FALSE;
    }
    return TRUE;
}
void ObjectBansDlg::ShowPreview(const era_options::BanObjectInfo &entry)
{
    const int savedResult = P_WindowManager->resultItemID;
    if (!era_help::ShowObjectHint(HelpPages[tab], entry.id, TRUE))
    {
        const std::string text = "{" + entry.name + "}\n\n" + entry.description;
        H3Messagebox::RMB(text.c_str());
    }
    P_WindowManager->resultItemID = savedResult;
    Refresh();
}
BOOL ObjectBansDlg::OnMouseHover(H3DlgItem *item)
{
    if (GetHintBar()) GetHintBar()->ShowMessage(item && item->GetHint() ? item->GetHint() : "");
    return TRUE;
}
void ObjectBansDlg::SyncQuery()
{
    const std::string value = query && query->GetText() ? query->GetText() : "";
    if (value != filters[tab].query) { filters[tab].query = value; Rebuild(); }
}
INT ObjectBansDlg::vDialogProc(H3Msg &msg)
{
    const auto result = H3Dlg::vDialogProc(msg);
    if (!endDialog) SyncQuery();
    return result;
}
INT ObjectBansDlg::vPreProcess(H3Msg &msg)
{
    const auto cursor = H3POINT::GetCursorPosition();
    if (auto *scroll = scrollbars[tab])
        if (scroll->IsVisible())
        {
            if (cursor.x >= xDlg + GridX && cursor.x < xDlg + widthDlg - 16 &&
                cursor.y >= yDlg + GridTop() && cursor.y < yDlg + heightDlg - 66) scroll->Activate();
            else scroll->DeActivate();
        }
    return H3Dlg::vPreProcess(msg);
}
BOOL ObjectBansDlg::OnKeyPress(eVKey key, eMsgFlag)
{
    if (key == eVKey::H3VK_ESCAPE) { SaveAndClose(); return FALSE; }
    if (key == eVKey::H3VK_F && (GetKeyState(VK_CONTROL) & 0x8000)) { if (query) query->SetFocus(TRUE); return FALSE; }
    return TRUE;
}
bool ObjectBansDlg::SaveAndClose()
{
    std::string error;
    if (allowEditing && !era_options::SaveObjectBanPreferences(error))
    {
        H3Messagebox((std::string(Text("save_error", "Could not save object bans.")) + "\n\n" + error).c_str());
        return false;
    }
    Stop();
    return true;
}
void ObjectBansDlg::OnClose(INT) { SaveAndClose(); }
void __fastcall ObjectBansDlg::ScrollProc(INT32 tick, H3BaseDlg *dlg)
{
    auto *self = static_cast<ObjectBansDlg *>(dlg);
    self->filters[self->tab].firstRow = tick;
    self->Refresh();
}
