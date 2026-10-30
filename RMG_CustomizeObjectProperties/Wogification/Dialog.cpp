#include "../Assets.h"
#include "../pch.h"

#include "../DialogInput.h"
#include "../RMG_SettingsDlg.h"
#include "ProfileDialog.h"
#include "Settings.h"
#include <map>
#include <set>

namespace wogification
{
namespace
{
constexpr char WIDGET[] = "wogification_settings_widget";
enum Control
{
    Targets = 110,
    Sources,
    Search,
    Provider,
    Reset,
    ProfileLoad,
    ProfileSave,
    Accept,
    Cancel,
    Help,
    Status
};
constexpr int ROWS = 7, ROW_BASE = 1000, ROW_STRIDE = 20;
constexpr int TABLE_Y = 171, ROW_HEIGHT = 49, SCROLL_ID = 200;
constexpr int CATEGORY_BASE = 500, CATEGORY_COUNT = 10, CATEGORY_PREV = 510, CATEGORY_NEXT = 511;
constexpr int CREATURE_TAB = 130, ARTIFACT_TAB = 131, PROCESS_BASE = 600;

class Choices : public H3Dlg
{
    int value;
    bool multiple, accepted = false;
    std::vector<std::string> names;
    void Refresh()
    {
        for (int i = 0; i < int(names.size()); ++i)
        {
            const bool selected = multiple ? (value & (1 << i)) != 0 : value == i;
            const auto caption = std::string(selected ? "[x] " : "[ ] ") + names[i];
            GetCaptionButton(10 + i)->SetText(caption.c_str());
        }
        Redraw();
    }
    void Action(int id)
    {
        if (id == 1)
        {
            accepted = true;
            Stop();
        }
        else if (id == 2)
            Stop();
        else if (id >= 10 && id < 10 + int(names.size()))
        {
            if (multiple)
                value ^= 1 << (id - 10);
            else
            {
                value = id - 10;
                accepted = true;
                Stop();
            }
            Refresh();
        }
    }
    BOOL OnLeftClick(INT id, H3Msg &) override
    {
        Action(id);
        return FALSE;
    }
    VOID OnClose(INT id) override
    {
        Action(id);
    }
    VOID OnOK() override
    {
        Action(1);
    }
    VOID OnCancel() override
    {
        Action(2);
    }

  public:
    Choices(int initial, bool mask, const std::vector<std::string> &labels, int x, int y)
        : H3Dlg(300, int(labels.size()) * 29 + 52, x, y, FALSE, FALSE), value(initial), multiple(mask), names(labels)
    {
        AddBackground(0, 0, 300, int(labels.size()) * 29 + 52, FALSE, FALSE, 1, TRUE);
        for (int i = 0; i < int(names.size()); ++i)
            CreateCaptionButton(10, 8 + i * 29, 280, 26, 10 + i, rmgassets::GSPBUT2_DEF, "", NH3Dlg::Text::SMALL, 0, 0)
                ->SetClickFrame(1);
        CreateCaptionButton(10, 12 + int(names.size()) * 29, 135, 28, 1, rmgassets::GSPBUT2_DEF, Text("chooseDone"),
                            NH3Dlg::Text::SMALL, 0, 0, FALSE, eVKey::H3VK_ENTER);
        CreateCaptionButton(155, 12 + int(names.size()) * 29, 135, 28, 2, rmgassets::GSPBUT2_DEF, Text("chooseCancel"),
                            NH3Dlg::Text::SMALL, 0, 0, FALSE, eVKey::H3VK_ESCAPE);
        Refresh();
    }
    int Select()
    {
        Start();
        return accepted ? value : -1;
    }
};

class Dialog : public H3Dlg
{
    Draft draft;
    Kind kind = Target;
    int page = Targets;
    std::vector<H3DlgItem *> tableItems, creatureItems, artifactItems;
    H3DlgDef *checks[ProcessFieldCount]{};
    H3DlgCaptionButton *selectors[ProcessFieldCount]{};
    int top = 0, category = 0, provider = 0, categoryOffset = 0;
    std::vector<size_t> filtered;
    std::vector<std::string> categories, providers, objectCategories;
    std::map<size_t, H3LoadedPcx16 *> pictures;
    H3DlgEdit *search = nullptr;
    H3DlgText *profileLabel = nullptr;
    std::string profilePath;
    H3DlgCaptionButton *providerButton = nullptr;
    H3DlgCaptionButton *categoryButtons[CATEGORY_COUNT]{};
    std::vector<size_t> categorySlots;
    H3DlgScrollbar *scrollbar = nullptr;
    std::string statusText;
    std::string query;
    bool togglePressed = false;
    struct Row
    {
        size_t index = size_t(-1);
        H3DlgDef *enabled = nullptr;
        H3DlgPcx16 *picture = nullptr;
        H3DlgText *name = nullptr, *description = nullptr;
        H3DlgEdit *rate = nullptr, *limit = nullptr, *distance = nullptr;
        H3DlgCaptionButton *group = nullptr, *levels = nullptr, *maps = nullptr;
        H3DlgDefButton *reset = nullptr;
        std::vector<H3DlgItem *> items;
    } rows[ROWS];

    bool ProcessPage() const
    {
        return page == CREATURE_TAB || page == ARTIFACT_TAB;
    }
    std::vector<std::string> Options(int field) const
    {
        std::vector<std::string> result;
        if (field == CreatureLevels)
            for (int level = 1; level <= 8; ++level)
                result.push_back(std::string(Text("creatureLevel")) + std::to_string(level));
        else if (field == ArtifactLevels)
            for (const char *name : {"special", "treasure", "minor", "major", "relic"})
                result.emplace_back(Text((std::string("artifactClass_") + name).c_str()));
        else
            for (const char *name : {"keep", "random", "chest", "gold"})
                result.emplace_back(Text((std::string("scrollMode_") + name).c_str()));
        return result;
    }
    void Check(std::vector<H3DlgItem *> &items, int field, int y, const char *key, const char *hint)
    {
        auto *box = H3DlgDef::Create(48, y + 2, PROCESS_BASE + field * 2, rmgassets::RMG_CHBX_DEF, 0, 0);
        AddItem(box);
        checks[field] = box;
        auto *label = CreateText(88, y, 655, 30, Text(key), NH3Dlg::Text::MEDIUM, eTextColor::HIGHLIGHT,
                                 PROCESS_BASE + field * 2 + 1, eTextAlignment::MIDDLE_LEFT);
        for (H3DlgItem *item : {static_cast<H3DlgItem *>(box), static_cast<H3DlgItem *>(label)})
        {
            item->SetHints(hint, hint, TRUE);
            items.push_back(item);
        }
    }
    void Select(std::vector<H3DlgItem *> &items, int field, int y, const char *key)
    {
        items.push_back(CreateText(88, y, 330, 28, Text(key), NH3Dlg::Text::SMALL, eTextColor::REGULAR, -1,
                                   eTextAlignment::MIDDLE_LEFT));
        selectors[field] = Button(426, y, 315, PROCESS_BASE + field * 2, "", 28);
        selectors[field]->SetHints(Text("chooseHint"), Text("chooseHint"), TRUE);
        items.push_back(selectors[field]);
    }
    void RefreshProcess()
    {
        for (auto *item : tableItems)
            item->HideDeactivate();
        HideCurrentPage();
        for (auto *item : creatureItems)
            item->HideDeactivate();
        for (auto *item : artifactItems)
            item->HideDeactivate();
        auto &items = page == CREATURE_TAB ? creatureItems : artifactItems;
        for (auto *item : items)
            item->ShowActivate();
        for (int f = 0; f < ProcessFieldCount; ++f)
        {
            const bool enabled = f == CreatureEnabled || f == ArtifactEnabled || f == ScrollEnabled ||
                                 (f <= QuestCreatures                      ? draft.process[CreatureEnabled] != 0
                                  : f >= ScrollLevel1 && f <= ScrollLevel5 ? draft.process[ScrollEnabled] != 0
                                                                           : draft.process[ArtifactEnabled] != 0);
            if (checks[f])
                checks[f]->SetFrame(draft.process[f]);
            for (auto *item : items)
                if (item->GetID() == PROCESS_BASE + f * 2 || item->GetID() == PROCESS_BASE + f * 2 + 1)
                    item->EnableItem(enabled);
            if (!selectors[f])
                continue;
            const auto options = Options(f);
            std::string caption;
            if (f == CreatureLevels || f == ArtifactLevels)
            {
                const int all = (1 << options.size()) - 1;
                if (draft.process[f] == all)
                    caption = Text("allLevels");
                else if (!draft.process[f])
                    caption = Text("noLevels");
                else
                    for (int i = 0; i < int(options.size()); ++i)
                        if (draft.process[f] & (1 << i))
                        {
                            if (!caption.empty())
                                caption += ", ";
                            caption += f == CreatureLevels ? std::to_string(i + 1) : options[i];
                        }
            }
            else
                caption = options[draft.process[f]];
            caption += "  v";
            selectors[f]->SetText(caption.c_str());
            selectors[f]->EnableItem(enabled);
        }
        ShowStatus(Text("processHint"));
    }
    void UpdateTabs()
    {
        const int tabs[] = {Targets, Sources, CREATURE_TAB, ARTIFACT_TAB};
        const int next = int(std::find(tabs, tabs + 4, page) - tabs + 1) % 4;
        for (int i = 0; i < 4; ++i)
        {
            auto *button = GetCaptionButton(tabs[i]);
            button->SetFrame(page == tabs[i] ? 1 : 0);
            button->SendCommand(page == tabs[i] ? 5 : 6, 4096);
            button->RemoveHotkeys();
            if (i == next)
                button->AddHotkey(eVKey::H3VK_TAB);
        }
    }
    static std::string Lower(std::string text)
    {
        if (!text.empty())
            CharLowerBuffA(&text[0], static_cast<DWORD>(text.size()));
        return text;
    }
    static std::string NativeCategory(int page)
    {
        if (page < 0 || page >= 9)
            return Text("otherCategories");
        bool found = false;
        const char *name = EraJS::read("RMG.text.dlg.pages." + std::to_string(page) + ".name", found);
        return found ? name : Text(("category" + std::to_string(page)).c_str());
    }
    H3DlgCaptionButton *Button(int x, int y, int width, int id, const char *text, int height = 24, int hotkey = 0)
    {
        auto *button = CreateCaptionButton(x, y, width, height, id, rmgassets::GSPBUT2_DEF, text, NH3Dlg::Text::SMALL,
                                           0, 0, FALSE, hotkey, eTextColor::HIGHLIGHT);
        if (button)
            button->SetClickFrame(1);
        return button;
    }
    H3DlgDefButton *AssetButton(int x, int y, int id, const char *asset, const char *hint, bool frame = true,
                                int normal = 0, int pressed = 1, int hotkey = 0)
    {
        auto *button = H3DlgDefButton::Create(x, y, id, asset, normal, pressed, FALSE, hotkey);
        if (!button)
            return nullptr;
        button->SetHints(hint, hint, FALSE);
        // As in RMG: decoration is behind the control and never captures input.
        if (frame)
            if (auto *border = CreateFrame(button, H3RGB565(H3RGB888::Highlight()), 0, 1))
                border->DeActivate();
        AddItem(button);
        return button;
    }
    void ShowStatus(const std::string &text)
    {
        statusText = text;
        if (hintBar)
            hintBar->SetText(statusText.c_str());
    }
    H3DlgEdit *Edit(int x, int y, int width, int length, int id, const char *text)
    {
        auto *edit = CreateEdit(x, y, width, 22, length, text, NH3Dlg::Text::SMALL, eTextColor::REGULAR,
                                eTextAlignment::MIDDLE_CENTER, rmgassets::RMG_EDIT_PCX, id, TRUE, 2, 2);
        if (edit)
            edit->SetAutoredraw(TRUE);
        return edit;
    }
    std::string GroupName(int group) const
    {
        if (group < 0)
            return Text("allGroups");
        const auto key = "wogify.objectReplacement.objects." + std::to_string(group) + ".name";
        bool found = false;
        const char *name = EraJS::read(key, found);
        return found ? name : "#" + std::to_string(group);
    }
    bool CommitRows(bool report = true, size_t ignored = size_t(-1))
    {
        auto replacement = draft;
        for (auto &row : rows)
        {
            if (row.index == size_t(-1) || row.index == ignored)
                continue;
            auto &v = replacement[row.index][kind];
            if (!row.rate || !row.limit || !ParseInteger(row.rate->GetString()->String(), v[Rate]) ||
                !ParseInteger(row.limit->GetString()->String(), v[Limit]) ||
                (kind == Target &&
                 (!row.distance || !ParseInteger(row.distance->GetString()->String(), v[Distance]))) ||
                !Valid(v, kind))
            {
                if (report)
                    H3Messagebox(Text("invalidValues"));
                return false;
            }
        }
        draft.swap(replacement);
        return true;
    }
    void Unfocus()
    {
        rmginput::BlurAll(*this, [](H3DlgEdit *) {});
    }
    void HideCurrentPage()
    {
        Unfocus();
        for (auto &row : rows)
        {
            for (auto *item : row.items)
                item->HideDeactivate();
            row.index = size_t(-1);
        }
        for (auto *button : categoryButtons)
        {
            button->RemoveHotkeys();
            button->HideDeactivate();
        }
        for (int id : {CATEGORY_PREV, CATEGORY_NEXT})
            GetH3DlgItem(id)->HideDeactivate();
        if (scrollbar)
            scrollbar->HideDeactivate();
    }
    void Filter(bool redraw = true)
    {
        filtered.clear();
        const auto &objects = Objects();
        for (size_t i = 0; i < objects.size(); ++i)
        {
            const auto &object = objects[i];
            if (Pickup(object.type) || !Participates(object, kind))
                continue;
            if (category && objectCategories[i] != categories[category])
                continue;
            if (provider && object.provider != providers[provider])
                continue;
            if (!query.empty() && Lower(object.name + " " + object.id + " " + object.provider + " " +
                                        std::to_string(object.type) + ":" + std::to_string(object.subtype))
                                          .find(query) == std::string::npos)
                continue;
            filtered.push_back(i);
        }
        top = std::max(0, std::min(top, std::max(0, int(filtered.size()) - ROWS)));
        Refresh(redraw);
    }
    void UpdateCategories()
    {
        std::vector<size_t> available{0};
        for (size_t c = 1; c < categories.size(); ++c)
            for (size_t i = 0; i < Objects().size(); ++i)
                if (!Pickup(Objects()[i].type) && Participates(Objects()[i], kind) &&
                    objectCategories[i] == categories[c])
                {
                    available.push_back(c);
                    break;
                }
        if (std::find(available.begin(), available.end(), category) == available.end())
            category = 0;
        categoryOffset =
            std::max(0, std::min(categoryOffset, int((available.size() - 1) / CATEGORY_COUNT) * CATEGORY_COUNT));
        categorySlots.clear();
        for (int slot = 0; slot < CATEGORY_COUNT; ++slot)
        {
            auto *button = categoryButtons[slot];
            const size_t position = categoryOffset + slot;
            button->RemoveHotkeys();
            if (position >= available.size())
            {
                button->HideDeactivate();
                continue;
            }
            const size_t c = available[position];
            categorySlots.push_back(c);
            const char *label = c ? categories[c].c_str() : Text("allCategories");
            const auto hint = std::string(label) + " [" + std::to_string(slot < 9 ? slot + 1 : 0) + "]";
            button->SetText(label);
            button->SetHints(hint.c_str(), hint.c_str(), TRUE);
            const bool active = c == category;
            button->SetFrame(active ? 1 : 0);
            button->ShowActivate();
            button->SendCommand(active ? 5 : 6, 4096);
            if (!active)
                button->AddHotkey(slot < 9 ? eVKey::H3VK_1 + slot : eVKey::H3VK_0);
        }
        for (int id : {CATEGORY_PREV, CATEGORY_NEXT})
            if (available.size() > CATEGORY_COUNT)
                GetH3DlgItem(id)->ShowActivate();
            else
                GetH3DlgItem(id)->HideDeactivate();
    }
    void SyncScrollbar()
    {
        if (!scrollbar)
            return;
        if (filtered.size() <= ROWS)
        {
            scrollbar->SetTick(0);
            scrollbar->HideDeactivate();
            return;
        }
        struct ScrollbarAccess : H3DlgScrollbar
        {
            using H3DlgScrollbar::vSetTickCount;
        };
        const int ticks = static_cast<int>(filtered.size()) - ROWS + 1;
        if (scrollbar->GetTicksCount() != ticks)
            reinterpret_cast<ScrollbarAccess *>(scrollbar)->vSetTickCount(ticks);
        scrollbar->SetTick(top);
        scrollbar->SetButtonPosition();
        scrollbar->ShowActivate();
    }
    static void __fastcall Scroll(INT32 tick, H3BaseDlg *owner)
    {
        auto *dlg = static_cast<Dialog *>(owner);
        if (!dlg->CommitRows())
        {
            dlg->SyncScrollbar();
            return;
        }
        dlg->Unfocus();
        dlg->top = std::max(0, std::min(tick, std::max(0, int(dlg->filtered.size()) - ROWS)));
        dlg->Refresh();
    }
    void Refresh(bool redraw = true)
    {
        UpdateTabs();
        if (ProcessPage())
        {
            RefreshProcess();
            if (redraw)
                Redraw();
            return;
        }
        for (auto *item : creatureItems)
            item->HideDeactivate();
        for (auto *item : artifactItems)
            item->HideDeactivate();
        for (auto *item : tableItems)
            item->ShowActivate();
        const auto &objects = Objects();
        const bool creaturePage = category && categories[category] == Text("creatures");
        const bool pickupPage = creaturePage || (category && categories[category] == Text("artifacts"));
        const char *headers[5] = {kind == Target ? (creaturePage ? Text("chance") : Text("weightChance"))
                                                 : Text("chance"),
                                  kind == Target ? Text("mapLimit") : Text("preserve"),
                                  pickupPage       ? ""
                                  : kind == Target ? Text("distance")
                                                   : Text("group"),
                                  Text("levels"), Text("mapKinds")};
        for (int f = 0; f < 5; ++f)
            if (auto *text = GetText(300 + f))
                text->SetText(headers[f]);
        providerButton->SetText(provider ? providers[provider].c_str() : Text("allMods"));
        UpdateCategories();
        SyncScrollbar();
        for (int r = 0; r < ROWS; ++r)
        {
            auto &row = rows[r];
            const int position = top + r;
            row.picture->SetPcx(nullptr);
            row.index = position < int(filtered.size()) ? filtered[position] : size_t(-1);
            for (auto *item : row.items)
                if (row.index == size_t(-1))
                    item->HideDeactivate();
                else
                    item->ShowActivate();
            if (row.index == size_t(-1))
                continue;
            if (kind == Target)
            {
                row.group->HideDeactivate();
                row.distance->ShowActivate();
            }
            else
            {
                row.distance->HideDeactivate();
                row.group->ShowActivate();
            }
            const auto &object = objects[row.index];
            const auto &v = draft[row.index][kind];
            if (Pickup(object.type))
            {
                row.distance->HideDeactivate();
                row.group->HideDeactivate();
            }
            row.enabled->SetFrame(v[Enabled]);
            row.name->SetText(object.name.c_str());
            const std::string description = object.provider + " | " + std::to_string(object.type) + ":" +
                                            (object.subtype < 0 ? "*" : std::to_string(object.subtype));
            row.description->SetText(description.c_str());
            row.rate->SetText(std::to_string(v[Rate]).c_str());
            row.limit->SetText(std::to_string(v[Limit]).c_str());
            row.distance->SetText(std::to_string(v[Distance]).c_str());
            const auto groupCaption = v[Distance] < 0 ? std::string("*") : "#" + std::to_string(v[Distance]);
            row.group->SetText(groupCaption.c_str());
            row.levels->SetText(v[Levels] == 3 ? Text("both") : v[Levels] == 1 ? Text("surface") : Text("underground"));
            row.maps->SetText(v[MapKinds] == 3   ? Text("allMaps")
                              : v[MapKinds] == 1 ? Text("authored")
                                                 : Text("random"));
            row.enabled->SetHint(Text("enabledHint"));
            row.rate->SetHint(kind == Source ? Text("chanceHint") : Text("weightHint"));
            if (kind == Target && object.type == eObject::MONSTER)
                row.rate->SetHint(Text("creatureChanceHint"));
            row.limit->SetHint(kind == Source ? Text("preserveHint") : Text("limitHint"));
            row.distance->SetHint(Text("distanceHint"));
            const std::string groupHint = GroupName(v[Distance]);
            row.group->SetHints(groupHint.c_str(), groupHint.c_str(), TRUE);
            row.levels->SetHint(Text("levelsHint"));
            row.maps->SetHint(Text("mapsHint"));
            row.name->SetHints(object.name.c_str(), object.name.c_str(), TRUE);
            row.picture->SetHints(object.name.c_str(), object.name.c_str(), TRUE);
            if (!pictures.count(row.index))
                pictures[row.index] = rmgdlg::RMG_SettingsDlg::CreateWogificationPreview(object.type, object.subtype);
            row.picture->SetPcx(pictures[row.index]);
        }
        const std::string count = std::to_string(filtered.size()) + " " + Text("objects");
        GetText(Status)->SetText(count.c_str());
        ShowStatus(kind == Source ? Text("sourceHint") : Text("targetHint"));
        if (redraw)
            Redraw();
    }
    void Details(size_t index)
    {
        const auto &object = Objects()[index];
        std::string text = "{" + object.name + "}\n" + object.provider + "\nID: " + object.id + "\n";
        if (kind == Source)
            text += GroupName(draft[index][Source][Distance]) + "\n\n";
        if (Pickup(object.type))
        {
            text += Text("pickupHelp");
            H3Messagebox::RMB(text.c_str());
            return;
        }
        if (kind == Target && object.targetGroups.empty())
        {
            text += Text("unassignedTarget");
            text += "\n\n";
        }
        text += Text("permittedGroups");
        for (int group : (kind == Source ? object.sourceGroups : object.targetGroups))
            text += "\n" + GroupName(group);
        text += "\n\n";
        text += Text("geometryHint");
        H3Messagebox::RMB(text.c_str());
    }
    void ProfileAction(bool save)
    {
        if (save && !CommitRows())
            return;
        Unfocus();
        if (!SelectProfileFile(save, draft, profilePath))
            return;
        profileLabel->SetText(profilePath.c_str());
        profileLabel->SetHints(profilePath.c_str(), profilePath.c_str(), TRUE);
        if (save)
            ShowStatus(Text("profileSaved"));
        else
            Refresh();
        Redraw();
    }
    INT vDialogProc(H3Msg &msg) override
    {
        // Focus changes/redraws can dispatch nested native messages. Keep the
        // current event on our stack rather than reusing the input-manager buffer.
        H3Msg event = msg;
        if ((event.command == eMsgCommand::MOUSE_BUTTON && event.subtype == eMsgSubtype::LBUTTON_CLICK) ||
            event.ClickOutside())
            togglePressed = false;
        if (rmginput::BlurOutside(*this, event, [](H3DlgEdit *) {}))
            CommitRows(false);
        const int result = H3Dlg::vDialogProc(event);
        if (!endDialog)
        {
            const std::string nextQuery = ProcessPage() ? query : Lower(search->GetString()->String());
            if (nextQuery != query && CommitRows(false))
            {
                query = nextQuery;
                top = 0;
                Filter();
            }
        }
        msg = event;
        return result;
    }
    bool ToggleObject(H3Msg &msg)
    {
        if ((msg.command != eMsgCommand::MOUSE_BUTTON && msg.command != eMsgCommand::ITEM_COMMAND) ||
            msg.subtype != eMsgSubtype::LBUTTON_DOWN)
            return false;
        auto *item = ItemAtPosition(msg);
        if (!item || item->GetID() != msg.itemId)
            return false;
        if (ProcessPage() && msg.itemId >= PROCESS_BASE && msg.itemId < PROCESS_BASE + ProcessFieldCount * 2)
        {
            const int field = (msg.itemId - PROCESS_BASE) / 2;
            if (!checks[field] || !item->IsEnabled())
                return false;
            if (!togglePressed)
            {
                togglePressed = true;
                draft.process[field] = !draft.process[field];
                P_SoundManager->ClickSound();
                Refresh();
            }
            return true;
        }
        for (auto &row : rows)
            if (row.index != size_t(-1) &&
                (item == row.enabled || item == row.picture || item == row.name || item == row.description))
            {
                if (!togglePressed)
                {
                    togglePressed = true;
                    auto &enabled = draft[row.index][kind][Enabled];
                    enabled = !enabled;
                    P_SoundManager->ClickSound();
                    row.enabled->SetFrame(enabled);
                    const int firstId = row.enabled->GetID();
                    RedrawItemRange(TRUE, firstId, firstId + ROW_STRIDE - 1);
                }
                return true;
            }
        return false;
    }
    BOOL OnMouseWheel(INT direction) override
    {
        if (!ProcessPage() && direction && CommitRows())
        {
            Unfocus();
            top += direction > 0 ? -1 : 1;
            Filter();
        }
        return FALSE;
    }
    bool Action(int id)
    {
        if (id == Accept)
        {
            OnOK();
            return true;
        }
        if (id == Cancel)
        {
            OnCancel();
            return true;
        }
        if (id == ProfileLoad || id == ProfileSave)
        {
            ProfileAction(id == ProfileSave);
            return true;
        }
        if (id == Help)
        {
            H3Messagebox(Text("help"));
            return true;
        }
        if (id == CREATURE_TAB || id == ARTIFACT_TAB || id == Targets || id == Sources)
        {
            if (!CommitRows())
                return true;
            HideCurrentPage();
            page = id;
            if (!ProcessPage())
            {
                kind = id == Targets ? Target : Source;
                category = 0;
                categoryOffset = 0;
                top = 0;
                Filter();
            }
            else
                Refresh();
            return true;
        }
        if (ProcessPage() && id >= PROCESS_BASE && id < PROCESS_BASE + ProcessFieldCount * 2)
        {
            const int field = (id - PROCESS_BASE) / 2;
            if (selectors[field] && selectors[field]->IsEnabled())
            {
                const auto options = Options(field);
                Choices choices(draft.process[field], field == CreatureLevels || field == ArtifactLevels, options,
                                GetX() + 426, GetY() + std::min(230, int(selectors[field]->GetY()) + 28));
                const int value = choices.Select();
                if (value >= 0)
                    draft.process[field] = value;
                Refresh();
            }
            return true;
        }
        if (id == Reset)
        {
            Unfocus();
            if (ProcessPage())
            {
                const auto defaults = ProcessDefaults();
                const int first = page == CREATURE_TAB ? CreatureEnabled : ArtifactEnabled;
                const int last = page == CREATURE_TAB ? QuestCreatures : ScrollLevel5;
                for (int f = first; f <= last; ++f)
                    draft.process[f] = defaults[f];
                Refresh();
                return true;
            }
            for (size_t index : filtered)
                draft[index][kind] = Objects()[index].defaults[kind];
            Refresh();
            return true;
        }
        if (id >= ROW_BASE && id < ROW_BASE + ROWS * ROW_STRIDE)
        {
            const int field = (id - ROW_BASE) % ROW_STRIDE;
            if (field <= 3)
                return true; // Toggled on press; release never toggles or opens a popup.
            if (field >= 4 && field <= 6)
                return false; // Native edit focus/input must run.
            if (field > 10)
                return false; // Row backgrounds/frames are not actions.
        }
        if ((id >= Targets && id <= Reset && id != Search) || (id >= CATEGORY_BASE && id <= CATEGORY_NEXT) ||
            (id >= ROW_BASE && id < ROW_BASE + ROWS * ROW_STRIDE))
        {
            const size_t ignored = id >= ROW_BASE && (id - ROW_BASE) % ROW_STRIDE == 10
                                       ? rows[(id - ROW_BASE) / ROW_STRIDE].index
                                       : size_t(-1);
            if (!CommitRows(true, ignored))
                return true;
            Unfocus();
        }
        else
            return false;
        if (id >= CATEGORY_BASE && id < CATEGORY_BASE + CATEGORY_COUNT)
        {
            const size_t slot = id - CATEGORY_BASE;
            if (slot < categorySlots.size())
            {
                category = static_cast<int>(categorySlots[slot]);
                top = 0;
                Filter();
            }
        }
        else if (id == CATEGORY_PREV || id == CATEGORY_NEXT)
        {
            categoryOffset += id == CATEGORY_PREV ? -CATEGORY_COUNT : CATEGORY_COUNT;
            Refresh();
        }
        else if (id == Provider)
        {
            provider = (provider + 1) % providers.size();
            top = 0;
            Filter();
        }
        else if (id >= ROW_BASE && id < ROW_BASE + ROWS * ROW_STRIDE)
        {
            const int r = (id - ROW_BASE) / ROW_STRIDE, field = (id - ROW_BASE) % ROW_STRIDE;
            auto &row = rows[r];
            if (row.index == size_t(-1))
                return true;
            auto &v = draft[row.index][kind];
            if (field == 7)
            {
                const auto &groups = Objects()[row.index].sourceGroups;
                const auto old = std::find(groups.begin(), groups.end(), v[Distance]);
                v[Distance] = old == groups.end()       ? (groups.empty() ? -1 : groups[0])
                              : old + 1 == groups.end() ? -1
                                                        : *(old + 1);
            }
            else if (field == 8)
                v[Levels] = v[Levels] % 3 + 1;
            else if (field == 9)
                v[MapKinds] = v[MapKinds] % 3 + 1;
            else if (field == 10)
                v = Objects()[row.index].defaults[kind];
            else
                return false;
            Refresh();
        }
        return true;
    }
    BOOL OnLeftClick(INT id, H3Msg &) override
    {
        return Action(id) ? FALSE : TRUE;
    }
    VOID OnClose(INT id) override
    {
        togglePressed = false;
        Action(id);
    }
    BOOL DialogProc(H3Msg &msg) override
    {
        if (ToggleObject(msg))
            return FALSE;
        // Native buttons may report END_DIALOG even with a custom item ID;
        // H3Dlg has already dispatched it to OnClose/OnOK/OnCancel above.
        if (msg.command == eMsgCommand::MOUSE_BUTTON && msg.subtype == eMsgSubtype::END_DIALOG)
            return FALSE;
        if (hintBar && msg.IsMouseOver())
        {
            auto *item = ItemAtPosition(msg);
            if (!item || !item->GetHint())
                hintBar->ShowMessage(statusText.c_str());
        }
        const bool rightClick = rmginput::RightClick(msg);
        if (msg.IsRightClick() && !rightClick)
            return FALSE;
        if (rightClick)
        {
            auto *item = ItemAtPosition(msg);
            if (!item || item->GetID() != msg.itemId)
                return FALSE;
            for (const auto &row : rows)
                if (row.index != size_t(-1) &&
                    (item == row.enabled || item == row.picture || item == row.name || item == row.description))
                {
                    Details(row.index);
                    return FALSE;
                }
        }
        if (rightClick)
        {
            if (auto *item = GetH3DlgItem(msg.itemId))
                if (const char *hint = item->GetRightClickHint())
                {
                    H3Messagebox::RMB(hint);
                    return FALSE;
                }
            return FALSE;
        }
        return TRUE;
    }
    VOID OnOK() override
    {
        if (!CommitRows())
            return;
        if (!SaveCurrent(draft))
        {
            H3Messagebox(Text("saveError"));
            return;
        }
        Stop();
    }
    VOID OnCancel() override
    {
        Stop();
    }

  public:
    Dialog() : H3Dlg(800, 600, -1, -1, FALSE, FALSE), draft(Current())
    {
        flags = 2;
        AddBackground(0, 0, 800, 600, FALSE, FALSE, 1, TRUE);
        if (background)
            background->SimpleFrameRegion(0, 0, 800, 600);
        AddItem(H3DlgPcx16::Create(25, 6, 750, 45, -1, rmgassets::RMG_BACK_PCX));
        const int tabIds[] = {Targets, Sources, CREATURE_TAB, ARTIFACT_TAB};
        for (int id : tabIds)
        {
            const int slot = id == Targets ? 0 : id == Sources ? 1 : id == CREATURE_TAB ? 2 : 3;
            auto *tab = CreateCaptionButton(25 + slot * 173, 9, 165, 39, id, rmgassets::RMGMENBT_DEF,
                                            id == Targets        ? Text("targets")
                                            : id == Sources      ? Text("sources")
                                            : id == CREATURE_TAB ? Text("creatures")
                                                                 : Text("artifacts"),
                                            NH3Dlg::Text::SMALL, 0, 0, FALSE, eVKey::H3VK_TAB, eTextColor::HIGHLIGHT);
            tab->SetClickFrame(1);
            tab->SetHint(id == CREATURE_TAB   ? Text("creatureOptions")
                         : id == ARTIFACT_TAB ? Text("artifactOptions")
                         : id == Targets      ? Text("targetsHint")
                                              : Text("sourcesHint"));
        }
        AssetButton(720, 10, Help, rmgassets::RMG_HELP_DEF, Text("helpHint"), false, 0, 1, eVKey::H3VK_H);
        for (int slot = 0; slot < CATEGORY_COUNT; ++slot)
            categoryButtons[slot] = Button(32 + (slot % 5) * 138, 56 + (slot / 5) * 22, 128, CATEGORY_BASE + slot, "",
                                           20, slot < 9 ? eVKey::H3VK_1 + slot : eVKey::H3VK_0);
        Button(726, 77, 24, CATEGORY_PREV, "<");
        Button(752, 77, 24, CATEGORY_NEXT, ">");
        tableItems.push_back(CreateText(25, 107, 60, 22, Text("search"), NH3Dlg::Text::SMALL, eTextColor::HIGHLIGHT, 91,
                                        eTextAlignment::MIDDLE_LEFT));
        search = Edit(86, 107, 300, 80, Search, "");
        search->SetHint(Text("searchHint"));
        CreateText(396, 107, 180, 22, "", NH3Dlg::Text::SMALL, eTextColor::REGULAR, Status,
                   eTextAlignment::MIDDLE_CENTER);
        providerButton = Button(590, 106, 168, Provider, Text("allMods"));
        providerButton->SetHint(Text("modHint"));
        auto *header = H3DlgPcx16::Create(25, 135, 743, 28, -1, rmgassets::RMG_BACK_PCX);
        AddItem(header);
        tableItems.push_back(header);
        tableItems.push_back(CreateFrame(header, H3RGB565(H3RGB888::Highlight())));
        CreateText(26, 138, 300, 22, Text("object"), NH3Dlg::Text::SMALL, eTextColor::HIGHLIGHT, 299);
        for (int f = 0; f < 5; ++f)
            CreateText(350 + f * 76, 138, 74, 22, "", NH3Dlg::Text::SMALL, eTextColor::HIGHLIGHT, 300 + f,
                       eTextAlignment::MIDDLE_CENTER);
        for (int r = 0; r < ROWS; ++r)
        {
            auto &row = rows[r];
            const int y = TABLE_Y + r * ROW_HEIGHT, id = ROW_BASE + r * ROW_STRIDE;
            auto *panel = H3DlgPcx16::Create(24, y, 744, ROW_HEIGHT - 2, id + 11, rmgassets::RMG_BACK_PCX);
            if (panel)
            {
                AddItem(panel);
                row.items.push_back(panel);
            }
            if (panel)
                if (auto *frame = H3DlgFrame::Create(panel, H3RGB565(H3RGB888::Highlight()), id + 12))
                {
                    AddItem(frame);
                    row.items.push_back(frame);
                }
            row.enabled = H3DlgDef::Create(30, y + 7, id, rmgassets::RMG_CHBX_DEF, 1, 0);
            AddItem(row.enabled);
            row.picture = H3DlgPcx16::Create(66, y + 2, 44, 44, id + 1, nullptr);
            AddItem(row.picture);
            row.name = CreateText(116, y + 2, 226, 25, "", NH3Dlg::Text::SMALL, eTextColor::HIGHLIGHT, id + 2,
                                  eTextAlignment::MIDDLE_LEFT);
            row.description = CreateText(116, y + 27, 226, 17, "", NH3Dlg::Text::SMALL, eTextColor::REGULAR, id + 3,
                                         eTextAlignment::MIDDLE_LEFT);
            row.rate = Edit(356, y + 13, 60, 7, id + 4, "");
            row.limit = Edit(432, y + 13, 60, 7, id + 5, "");
            row.distance = Edit(508, y + 13, 60, 7, id + 6, "");
            row.group = Button(502, y + 12, 72, id + 7, "");
            row.levels = Button(578, y + 12, 72, id + 8, "");
            row.maps = Button(654, y + 12, 72, id + 9, "");
            row.reset = AssetButton(734, y + 13, id + 10, rmgassets::RMG_DFMN_DEF, Text("resetRowHint"), false);
            row.items.insert(row.items.end(), {row.enabled, row.picture, row.name, row.description, row.rate, row.limit,
                                               row.distance, row.group, row.levels, row.maps, row.reset});
        }
        scrollbar = H3DlgScrollbar::Create(776, TABLE_Y, 16, ROWS * ROW_HEIGHT, SCROLL_ID, 1, Scroll, TRUE, 1, TRUE);
        if (scrollbar)
        {
            AddItem(scrollbar);
            scrollbar->SetHint(Text("scrollHint"));
        }
        CreateText(25, 523, 84, 22, Text("profile"), NH3Dlg::Text::SMALL, eTextColor::HIGHLIGHT, 400);
        profileLabel = CreateText(113, 523, 652, 22, Text("noProfile"), NH3Dlg::Text::SMALL, eTextColor::REGULAR, 401,
                                  eTextAlignment::MIDDLE_LEFT);
        const char *profileButtons = ProfileButtonDef();
        AssetButton(25, 550, ProfileLoad, profileButtons, Text("loadHint"), false, 6, 7, eVKey::H3VK_L);
        AssetButton(97, 550, ProfileSave, profileButtons, Text("saveHint"), false, 9, 10, eVKey::H3VK_S);
        AssetButton(177, 550, Reset, rmgassets::RMG_DFLT_DEF, Text("resetVisible"), true, 0, 1, eVKey::H3VK_R);
        AssetButton(657, 550, Accept, rmgassets::RMG_OKAY_DEF, Text("applyHint"), true, 0, 1, eVKey::H3VK_ENTER);
        AssetButton(720, 550, Cancel, rmgassets::RMG_CNCL_DEF, Text("cancelHint"), true, 0, 1, eVKey::H3VK_ESCAPE);
        hintBar = H3ObjectAllocator<H3DlgHintBar>().allocate(1);
        if (hintBar)
        {
            // Same constructor, resource and placement as RMG_SettingsDlg.
            THISCALL_12(H3DlgHintBar *, 0x5BCB70, hintBar, 240, 549, 409, 34, h3_NullString, NH3Dlg::Text::SMALL,
                        rmgassets::RMGHNTBR_PCX, eTextColor::REGULAR, 0, eTextAlignment::MIDDLE_CENTER, 8);
            hintBar->DeActivate();
            AddItem(hintBar);
        }
        tableItems.insert(tableItems.end(), {search, providerButton, GetText(Status), GetText(299)});
        for (int f = 0; f < 5; ++f)
            tableItems.push_back(GetText(300 + f));
        Check(creatureItems, CreatureEnabled, 84, "creatureEnabled", Text("creatureEnabledHint"));
        Select(creatureItems, CreatureLevels, 133, "creatureLevels");
        Check(creatureItems, MaxAggression, 192, "maxAggression", Text("maxAggressionHint"));
        Check(creatureItems, QuestCreatures, 245, "questCreatures", Text("questCreaturesHint"));
        Check(artifactItems, ArtifactEnabled, 75, "artifactEnabled", Text("artifactEnabledHint"));
        Select(artifactItems, ArtifactLevels, 119, "artifactLevels");
        Check(artifactItems, BankArtifacts, 163, "bankRewards", Text("bankRewardsHint"));
        Check(artifactItems, BankBanners, 207, "bankBanners", Text("bankBannersHint"));
        Check(artifactItems, ScrollEnabled, 263, "scrollEnabled", Text("scrollEnabledHint"));
        for (int f = ScrollLevel1; f <= ScrollLevel5; ++f)
        {
            const auto key = "scrollLevel" + std::to_string(f - ScrollLevel1 + 1);
            Select(artifactItems, f, 308 + (f - ScrollLevel1) * 36, key.c_str());
        }
        std::set<std::string> cats, mods;
        for (const auto &object : Objects())
        {
            const int page = static_cast<int>(
                rmgdlg::RMG_SettingsDlg::GetObjectAttributesPageByObjectType(object.type, object.subtype));
            objectCategories.push_back(Pickup(object.type)
                                           ? (object.type == eObject::ARTIFACT ? Text("artifacts") : Text("creatures"))
                                       : object.category.empty() ? NativeCategory(page)
                                                                 : object.category);
            if (Pickup(object.type) || (!Participates(object, Target) && !Participates(object, Source)))
                continue;
            cats.insert(objectCategories.back());
            mods.insert(object.provider);
        }
        categories.push_back("");
        for (const char *name : {Text("creatures"), Text("artifacts")})
            if (cats.erase(name))
                categories.emplace_back(name);
        for (int page = 0; page < static_cast<int>(extender::ePandoraBox) + 1; ++page)
        {
            if (page == extender::ePagePrison)
                continue;
            const auto name = NativeCategory(page);
            if (cats.erase(name))
                categories.emplace_back(name);
        }
        categories.insert(categories.end(), cats.begin(), cats.end());
        providers.push_back("");
        providers.insert(providers.end(), mods.begin(), mods.end());
        Filter(false);
    }
    ~Dialog()
    {
        for (auto &row : rows)
            if (row.picture)
                row.picture->SetPcx(nullptr);
        for (auto &picture : pictures)
            if (picture.second)
                picture.second->Destroy();
    }
};
int __fastcall Widget(void *message)
{
    if (auto *msg = static_cast<H3Msg *>(message))
        if (msg->IsLeftClick())
            ShowDialog();
    return TRUE;
}
void __stdcall Reload(Era::TEvent *)
{
    mainmenu::MainMenu_SetDialogButtonText(WIDGET, Text("title"));
}
} // namespace
void ShowDialog()
{
    Dialog dlg;
    dlg.Start();
}
void Install()
{
    static bool installed = false;
    if (installed)
        return;
    installed = true;
    mainmenu::MenuWidgetInfo widget{};
    widget.name = WIDGET;
    widget.text = Text("title");
    widget.customProc = Widget;
    widget.menuList =
        static_cast<mainmenu::eMenuFlags>(mainmenu::eMenuFlags::NEW_GAME | mainmenu::eMenuFlags::AT_BOTTOM);
    mainmenu::MainMenu_RegisterWidget(widget);
    Era::RegisterHandler(Reload, "OnAfterReloadLanguageData");
}
} // namespace wogification
