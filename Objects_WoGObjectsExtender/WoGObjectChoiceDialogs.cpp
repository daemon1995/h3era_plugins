#include <algorithm>
#include <vector>
#include "WoGObjectChoiceDialogs.h"
#include "WoGObjectIds.h"
#include "WoGObjectUtils.h"

namespace wog { namespace handlers { namespace {
H3RGB565 SelectionColor()
{
    // ShowParsedDlg8Items (0x4F798B): palette at 0x6AAD18, entry 45.
    const auto *palette = H3GamePalette::Get();
    return palette ? palette->color[45] : H3RGB565(248, 252, 0);
}
std::string Text(const char *key, const std::vector<std::string> &args = {})
{
    const std::string name = key;
    const char *script = name.find("junk") == 0 ? "108" : name.find("palace") == 0 ? "11" :
        name.find("arcane") == 0 || name == "give" || name == "receive" ? "104" :
        name.find("altar") == 0 || name == "neutral" || name == "before" || name == "after" ||
        name == "unchanged" || name == "randomTown" ? "14" : "12";
    return Era::tr((std::string("wog.") + script + ".native.ui." + name).c_str(), args);
}
void Visible(H3DlgItem *item, bool show)
{
    if (item) { if (show) item->ShowActivate(); else item->HideDeactivate(); }
}
void Enabled(H3DlgItem *item, bool enable)
{
    if (item) { if (enable) { item->UnShade(); item->Activate(); }
                else { item->Shade(); item->DeActivate(); } }
}
class ObjectDialog : public H3Dlg
{
    bool accepted = false;
    int lastSelection = -1;
    DWORD lastPress = 0;
  protected:
    void ResetDoubleClick() { lastSelection = -1; }
    bool DoubleClick(int selection)
    {
        const DWORD now = GetTime();
        const bool twice = lastSelection == selection && now - lastPress < 300;
        lastSelection = twice ? -1 : selection; lastPress = now;
        if (twice && P_SoundMgr) P_SoundMgr->ClickSound();
        return twice;
    }
    H3DlgDefButton *okay = nullptr;
    H3DlgText *Label(int x, int y, int w, int h, const std::string &text, int id = -1,
                     eTextColor color = eTextColor::REGULAR)
    {
        return CreateText(x, y, w, h, text.c_str(), NH3Dlg::Text::SMALL, color, id,
                          eTextAlignment::MIDDLE_CENTER);
    }
    H3DlgDefButton *Button(int x, int y, int id, const std::string &text)
    {
        auto *button = H3DlgCaptionButton::Create(x, y, id, NH3Dlg::Assets::OVBUTN3, text.c_str(),
            NH3Dlg::Text::SMALL, 0, 0, FALSE, 0, eTextColor::REGULAR);
        if (button)
        {
            button->SetClickFrame(1); AddItem(button);
            // Native DEF buttons consume the press and report only LBUTTON_CLICK on release.
            // Let a text control deliver LBUTTON_DOWN while the button supplies its appearance.
            button->DeActivate();
            Label(x, y, button->GetWidth(), button->GetHeight(), h3_NullString, id);
        }
        return button;
    }
    H3DlgFrame *Border(int x, int y, int width, int height)
    {
        auto *frame = CreateFrame(x, y, width, height, -1, H3RGB565(116, 100, 70));
        if (frame) { frame->Disable(); frame->DeActivate(); }
        return frame;
    }
    virtual bool CanAccept() const { return true; }
    void UpdateOkay() { Enabled(okay, CanAccept()); }
    void SetSlider(H3DlgScrollbar *slider, int maximum, int value)
    {
        if (!slider) return;
        // The native slider needs at least two ticks (its thumb calculation divides by ticks - 1).
        const int ticks = (std::max)(2, maximum + 1);
        if (slider->GetTicksCount() != ticks) THISCALL_2(void, 0x5973B0, slider, ticks);
        THISCALL_2(void, 0x5964D0, slider, (std::max)(0, (std::min)(maximum, value)));
        slider->SetButtonPosition();
        Enabled(slider, maximum > 0);
    }
  public:
    ObjectDialog(int width, int height, const std::string &title)
        : H3Dlg(width, height, -1, -1, FALSE, FALSE)
    {
        util::AddDialogBackground(*this, width, height);
        CreateText(20, 12, width - 40, 30, title.c_str(), NH3Dlg::Text::BIG, eTextColor::GOLD,
                   -1, eTextAlignment::MIDDLE_CENTER);
        okay = CreateOKButton(width / 2 - 82, height - 62, 1, FALSE);
        CreateCancelButton(width / 2 + 18, height - 62, 2, FALSE);
    }
    virtual BOOL Select(INT, H3Msg &) { return TRUE; }
    BOOL DialogProc(H3Msg &msg) override
    {
        if (msg.IsLeftDown()) return Select(msg.itemId, msg);
        // Handle a press before a native button enters its modal release loop too.
        if (msg.command == eMsgCommand::LCLICK_OUTSIDE)
        {
            auto *item = ItemAtPosition(msg);
            if (item) return Select(item->GetID(), msg);
        }
        return TRUE;
    }
    BOOL OnLeftClick(INT id, H3Msg &) override
    {
        if (id == 1 && CanAccept()) { accepted = true; Stop(); return FALSE; }
        if (id == 2) { Stop(); return FALSE; }
        return TRUE;
    }
    BOOL OnKeyPress(eVKey key, eMsgFlag) override
    {
        if (key == eVKey::H3VK_ESCAPE || (key == eVKey::H3VK_ENTER && CanAccept()))
        { accepted = key == eVKey::H3VK_ENTER; Stop(); return FALSE; }
        return TRUE;
    }
    bool Run()
    {
        const int previous = P_WindowManager->resultItemID;
        Start(); P_WindowManager->resultItemID = previous;
        return accepted;
    }
};

class SkullDialog final : public ObjectDialog
{
    std::function<void(const SkullTarget &)> searchTarget;
    int tab = 0, row = 0, selected = -1;
    std::vector<int> artifacts, filtered;
    H3DlgEdit *search = nullptr;
    std::string query;
    H3DlgDef *icons[2][64]{};
    H3DlgFrame *frames[64]{};
    H3DlgText *detail = nullptr;
    H3DlgScrollbar *scroll = nullptr;
    H3DlgDefButton *tabs[2]{};
    int Count() const { return int(filtered.size()); }
    int MaximumRow() const { return (std::max)(0, (Count() + 7) / 8 - 8); }
    int Value(int cell) const
    {
        const int index = row * 8 + cell;
        return index >= 0 && index < Count() ? filtered[index] : -1;
    }
    bool CanAccept() const override { return selected >= 0; }
    void Filter()
    {
        ResetDoubleClick();
        query = search ? search->GetText() : h3_NullString;
        filtered.clear();
        const int count = tab == 0 ? int(artifacts.size()) : 70;
        for (int i = 0; i < count; ++i)
        {
            const int id = tab == 0 ? artifacts[i] : i;
            const char *name = tab == 0 ? H3Artifact(id, -1).GetName() : H3Spell::Get()[id].name;
            // Game strings use the current ANSI code page; CompareString handles Cyrillic case too.
            const int length = int(query.size());
            bool match = length == 0;
            if (name)
                for (int pos = 0, end = int(libc::strlen(name)) - length; !match && pos <= end; ++pos)
                    match = CompareStringA(LOCALE_USER_DEFAULT, NORM_IGNORECASE, name + pos, length,
                                           query.c_str(), length) == CSTR_EQUAL;
            if (match) filtered.push_back(id);
        }
        row = 0; selected = -1;
        SetSlider(scroll, MaximumRow(), 0); Refresh();
    }
    static void __fastcall Scrolled(int tick, H3BaseDlg *dialog)
    {
        auto *self = static_cast<SkullDialog *>(dialog);
        self->ResetDoubleClick();
        self->row = (std::min)(tick, self->MaximumRow()); self->Refresh(); self->Redraw();
    }
    void Refresh()
    {
        for (int cell = 0; cell < 64; ++cell)
        {
            const int value = Value(cell);
            for (int sheet = 0; sheet < 2; ++sheet)
            {
                if (icons[sheet][cell])
                {
                    if (value >= 0 && sheet == tab)
                    {
                        const auto *def = icons[sheet][cell]->GetDef();
                        const int frame = tab == 1 ? value + 1 : value;
                        icons[sheet][cell]->SetFrame(def && def->groupsCount && def->groups[0] &&
                            frame < def->groups[0]->count ? frame : 0);
                    }
                    Visible(icons[sheet][cell], value >= 0 && sheet == tab);
                }
            }
            Visible(frames[cell], value >= 0 && value == selected);
        }
        if (detail)
        {
            const std::string description = filtered.empty() ? Text("noMatches") : selected < 0 ? Text("skullSelect") :
                tab == 0 ? H3Artifact(selected, -1).GetName() : H3Spell::Get()[selected].name;
            detail->SetText(description.c_str());
        }
        UpdateOkay();
        for (int i = 0; i < 2; ++i) if (tabs[i]) tabs[i]->SetFrame(i == tab ? 1 : 0);
    }
    void SwitchTab(int next)
    {
        tab = next;
        if (search) { search->SetText(h3_NullString); search->SetFocus(FALSE); }
        Filter();
    }
  public:
    explicit SkullDialog(const std::function<void(const SkullTarget &)> &searchTarget = {})
        : ObjectDialog(504, 590, util::ObjectName(objects::LIVING_SKULL)), searchTarget(searchTarget)
    {
        for (int id = 0; id < H3ArtifactCount::Get(); ++id)
            if (id != eArtifact::SPELL_SCROLL && id != 144 && id != 145 && util::ArtifactEnabled(id)) artifacts.push_back(id);
        tabs[0] = Button(36, 54, 10, Text("artifacts")); tabs[1] = Button(192, 54, 11, Text("scrolls"));
        Label(314, 48, 56, 28, Text("search"));
        search = CreateEdit(370, 48, 108, 28, 80, h3_NullString, NH3Dlg::Text::SMALL, eTextColor::REGULAR,
                            eTextAlignment::MIDDLE_LEFT, nullptr, 30, TRUE);
        if (search) search->SetAutoredraw(TRUE);
        for (int cell = 0; cell < 64; ++cell)
        {
            const int x = 24 + (cell % 8) * 54, y = 86 + (cell / 8) * 50;
            icons[0][cell] = CreateDef(x + 3, y + 3, 100 + cell, NH3Dlg::Assets::ARTIFACT_DEF, 0);
            icons[1][cell] = CreateDef(x + 1, y + 6, 200 + cell, NH3Dlg::Assets::SPELL_SMALL, 0);
            Border(x, y, 50, 48);
            frames[cell] = CreateFrame(x, y, 50, 48, 300 + cell, SelectionColor());
        }
        scroll = CreateScrollbar(464, 86, 16, 398, 20, (std::max)(2, MaximumRow() + 1), Scrolled, FALSE, 8);
        detail = Label(24, 490, 456, 30, Text("skullSelect"));
        Filter();
    }
    BOOL Select(INT id, H3Msg &msg) override
    {
        if (id >= 10 && id <= 11) SwitchTab(id - 10);
        else if (id >= 100 && id < 364 && id % 100 < 64 && Value(id % 100) >= 0)
        {
            selected = Value(id % 100); Refresh();
            if (DoubleClick(tab * 1000 + selected)) return OnLeftClick(1, msg);
        }
        else return TRUE;
        Redraw(); return FALSE;
    }
    BOOL DialogProc(H3Msg &msg) override
    {
        // DefaultProc delivers editing first; the next UI message observes the updated buffer.
        if (search && query != search->GetText()) { Filter(); Redraw(); }
        return ObjectDialog::DialogProc(msg);
    }
    BOOL OnRightClick(H3DlgItem *item) override
    {
        const int id = item ? item->GetID() : -1;
        const int cell = id >= 100 && id < 364 ? id % 100 : -1;
        if (cell >= 0 && cell < 64 && Value(cell) >= 0)
        {
            const int value = Value(cell);
            H3Messagebox::RMC(tab == 0 ? H3Artifact(value, -1).GetDescription() :
                               H3Spell::Get()[value].description[0]);
        }
        return TRUE;
    }
    BOOL OnMouseWheel(INT direction) override
    {
        ResetDoubleClick();
        row = (std::max)(0, (std::min)(MaximumRow(), row + (direction > 0 ? -1 : 1)));
        SetSlider(scroll, MaximumRow(), row); Refresh(); Redraw(); return TRUE;
    }
    BOOL OnLeftClick(INT id, H3Msg &msg) override
    {
        if (id == 1 && searchTarget && CanAccept())
        {
            searchTarget({tab == 0 ? eObject::ARTIFACT : eObject::SPELL_SCROLL, selected});
            Redraw(); return FALSE;
        }
        return ObjectDialog::OnLeftClick(id, msg);
    }
    BOOL OnKeyPress(eVKey key, eMsgFlag flag) override
    {
        if (key == eVKey::H3VK_ENTER && searchTarget)
        { H3Msg msg{}; return OnLeftClick(1, msg); }
        return ObjectDialog::OnKeyPress(key, flag);
    }
    SkullTarget Choose()
    {
        if (!Run()) return {};
        return {tab == 0 ? eObject::ARTIFACT : eObject::SPELL_SCROLL, selected};
    }
};

class SkullServiceDialog final : public ObjectDialog
{
    SkullService service;
    std::function<void(int, SkullService &)> action;
    H3DlgText *message = nullptr, *choices[3]{};
    H3DlgFrame *frames[3]{};
    int selected = 0, revision = 0;
    void Refresh()
    {
        message->SetText(service.text.c_str());
        for (int i = 0; i < 3; ++i)
        {
            const bool show = i < int(service.actions.size());
            Visible(choices[i], show); Visible(frames[i], show && i == selected);
            if (show) choices[i]->SetText(service.actions[i].c_str());
        }
    }
    BOOL Execute(H3Msg &msg)
    {
        if (service.actions.empty()) return ObjectDialog::OnLeftClick(1, msg);
        action(selected, service); selected = 0; ++revision;
        Refresh(); Redraw(); return FALSE;
    }
  public:
    SkullServiceDialog(SkullService service, const std::function<void(int, SkullService &)> &action)
        : ObjectDialog(504, 440, util::ObjectName(objects::LIVING_SKULL)), service(std::move(service)), action(action)
    {
        if (this->service.artifact >= 0)
            CreateDef(230, 52, -1, this->service.spell >= 0 ? NH3Dlg::Assets::SPELL_SMALL :
                NH3Dlg::Assets::ARTIFACT_DEF, this->service.spell >= 0 ? this->service.spell + 1 : this->service.artifact);
        message = Label(32, 104, 440, 118, this->service.text);
        for (int i = 0; i < 3; ++i)
        {
            choices[i] = Label(32, 230 + i * 44, 440, 40, h3_NullString, 100 + i);
            frames[i] = CreateFrame(32, 230 + i * 44, 440, 40, 100 + i, SelectionColor());
        }
        Refresh();
    }
    BOOL Select(INT id, H3Msg &msg) override
    {
        if (id < 100 || id >= 100 + int(service.actions.size())) return TRUE;
        selected = id - 100; Refresh(); Redraw();
        return DoubleClick(revision * 4 + selected) ? Execute(msg) : FALSE;
    }
    BOOL OnLeftClick(INT id, H3Msg &msg) override
    { return id == 1 ? Execute(msg) : ObjectDialog::OnLeftClick(id, msg); }
    BOOL OnKeyPress(eVKey key, eMsgFlag flag) override
    {
        if (key == eVKey::H3VK_ENTER) { H3Msg msg{}; return Execute(msg); }
        return ObjectDialog::OnKeyPress(key, flag);
    }
};

class DwellingDialog final : public ObjectDialog
{
  public:
    DwellingDialog(const std::vector<std::pair<int, int>> &stacks, int gold)
        : ObjectDialog((std::max)(304, 56 + (std::min)(4, (std::max)(1, int(stacks.size()))) * 146),
                       186 + ((int(stacks.size()) + 3) / 4) * 112,
                       util::ObjectName(objects::MYSTERIOUS_DWELLING))
    {
        const int width = GetWidth(), height = GetHeight();
        Label(24, 48, width - 48, 38, util::Tr("dwelling.offer", {"gold", std::to_string(gold)}));
        for (int i = 0; i < int(stacks.size()) && i < 7; ++i)
        {
            const int columns = (std::min)(4, int(stacks.size()) - (i / 4) * 4);
            const int x = (width - columns * 146) / 2 + (i % 4) * 146, y = 94 + (i / 4) * 112;
            const auto &stack = stacks[i];
            CreateDef(x + 42, y, 100 + i, NH3Dlg::Assets::CREATURE_LARGE, stack.first + 2);
            Label(x + 2, y + 66, 136, 38,
                std::to_string(stack.second) + " " + P_CreatureInformation[stack.first].namePlural);
            Border(x, y - 2, 140, 108);
        }
        CreateDef(width / 2 - 70, height - 94, -1, NH3Dlg::Assets::RESOURCE_DEF, 6);
        Label(width / 2 - 30, height - 96, 100, 28, std::to_string(gold), -1, eTextColor::GOLD);
    }
};

class PalaceGameDialog final : public ObjectDialog
{
    int selected = 0;
    bool artifacts;
    H3DlgFrame *frames[2]{};
  public:
    explicit PalaceGameDialog(bool available)
        : ObjectDialog(520, 290, util::ObjectName(objects::PALACE_OF_DREAMS)), artifacts(available)
    {
        CreateDef(124, 82, 100, NH3Dlg::Assets::RESOURCE_DEF, 6);
        CreateDef(350, 76, 101, NH3Dlg::Assets::ARTIFACT_DEF, 7);
        Label(48, 132, 180, 46, Text("palaceResources"), 100);
        Label(292, 132, 180, 46, Text("palaceArtifacts"), 101);
        for (int i = 0; i < 2; ++i)
        {
            Label(48 + i * 244, 66, 180, 118, h3_NullString, 100 + i);
            frames[i] = CreateFrame(48 + i * 244, 66, 180, 118, 100 + i, SelectionColor());
        }
        Visible(frames[1], false);
        Label(28, 190, 464, 28, Text("palaceCost"));
    }
    BOOL Select(INT id, H3Msg &msg) override
    {
        if (id != 100 && id != 101) return TRUE;
        if (id == 101 && !artifacts) return FALSE;
        selected = id - 100;
        for (int i = 0; i < 2; ++i) Visible(frames[i], i == selected);
        if (DoubleClick(selected)) return ObjectDialog::OnLeftClick(1, msg);
        Redraw(); return FALSE;
    }
    int Choose() { return Run() ? selected : -1; }
};

class PalaceSpinDialog final : public ObjectDialog
{
    bool artifacts, repeat;
    std::array<int,3> reels;
  public:
    PalaceSpinDialog(bool artifacts, const std::array<int,3> &reels, const std::string &message, bool repeat)
        : ObjectDialog(520, 330, util::ObjectName(objects::PALACE_OF_DREAMS)), artifacts(artifacts), repeat(repeat), reels(reels)
    {
        for (int i = 0; i < 3; ++i)
        {
            auto *icon = CreateDef(112 + i * 120, 68, 100 + i,
                artifacts ? NH3Dlg::Assets::ARTIFACT_DEF : NH3Dlg::Assets::RESOURCE_DEF, 0);
            if (icon)
            {
                auto *def = icon->GetDef();
                if (def && def->groupsCount && def->groups[0] && reels[i] < def->groups[0]->count)
                    icon->SetFrame(reels[i]);
            }
        }
        Label(28, 124, 464, 102, message);
        Label(28, 230, 464, 28, Text(repeat ? "palaceAgain" : "palaceClose"));
    }
    BOOL OnRightClick(H3DlgItem *item) override
    {
        const int index = item ? item->GetID() - 100 : -1;
        if (artifacts && index >= 0 && index < 3 && util::ValidArtifact(reels[index]))
            H3Messagebox::RMC(H3Artifact(reels[index], -1).GetDescription());
        return TRUE;
    }
    bool Again() { return Run() && repeat; }
};

class JunkDialog final : public ObjectDialog
{
    std::vector<JunkOffer> offers;
    std::function<void(std::vector<JunkOffer> &)> update;
    std::function<bool(const JunkOffer &)> trade;
    int first = 0, selected = -1, traded = 0;
    H3DlgDef *icons[8]{}, *rewards[8]{};
    H3DlgText *names[8]{}, *counts[8]{}, *title = nullptr, *reward = nullptr, *stock = nullptr, *status = nullptr;
    H3DlgFrame *frames[8]{};
    H3DlgScrollbar *scroll = nullptr;
    bool CanAccept() const override { return selected >= 0 && selected < int(offers.size()) && offers[selected].available; }
    static void __fastcall Scrolled(int tick, H3BaseDlg *dialog)
    {
        auto *self = static_cast<JunkDialog *>(dialog);
        self->first = (std::max)(0, (std::min)(int(self->offers.size()) - 8, tick)); self->Refresh(); self->Redraw();
    }
    std::string Name(const JunkOffer &offer) const
    {
        return offer.artifact == eArtifact::SPELL_SCROLL && offer.subtype >= 0 ?
            std::string(H3Artifact(offer.artifact, -1).GetName()) + ": " + H3Spell::Get()[offer.subtype].name :
            H3Artifact(offer.artifact, -1).GetName();
    }
    void Refresh()
    {
        update(offers);
        for (int row = 0; row < 8; ++row)
        {
            const int index = first + row; const bool shown = index < int(offers.size());
            Visible(icons[row], shown); Visible(names[row], shown); Visible(counts[row], shown);
            Visible(frames[row], shown && selected == index);
            if (!shown) continue;
            const auto &offer = offers[index];
            if (icons[row]) icons[row]->SetFrame(offer.artifact);
            if (names[row]) names[row]->SetText(Name(offer).c_str());
            if (counts[row]) counts[row]->SetText(Text("junkCount", {"count", std::to_string(offer.count)}).c_str());
        }
        const auto *offer = selected >= 0 && selected < int(offers.size()) ? &offers[selected] : nullptr;
        if (title) title->SetText(offer ? Name(*offer).c_str() : Text("junkSelect").c_str());
        if (reward) reward->SetText(offer ? offer->reward.c_str() : h3_NullString);
        if (stock) stock->SetText(offer ? Text(offer->available ? "junkReady" : "junkBlocked").c_str() : h3_NullString);
        for (int kind = 1; kind < 8; ++kind)
        {
            Visible(rewards[kind], offer && offer->rewardIcon == kind);
            if (offer && offer->rewardIcon == kind && rewards[kind])
            {
                auto *def = rewards[kind]->GetDef();
                if (def && def->groupsCount && def->groups[0] && offer->rewardFrame >= 0 && offer->rewardFrame < def->groups[0]->count)
                    rewards[kind]->SetFrame(offer->rewardFrame);
            }
        }
        UpdateOkay();
    }
    BOOL Exchange()
    {
        ResetDoubleClick();
        Refresh();
        if (!CanAccept()) return FALSE;
        const auto quote = offers[selected];
        const bool done = trade(quote);
        if (done) ++traded;
        if (status) status->SetText(Text(done ? "junkDone" : "junkChanged", {"count", std::to_string(traded)}).c_str());
        Refresh(); Redraw(); return FALSE;
    }
  public:
    JunkDialog(std::vector<JunkOffer> offers, const std::function<void(std::vector<JunkOffer> &)> &update,
               const std::function<bool(const JunkOffer &)> &trade)
        : ObjectDialog(760, 570, util::ObjectName(objects::JUNK_MERCHANT)), offers(std::move(offers)), update(update), trade(trade)
    {
        for (int row = 0; row < 8; ++row)
        {
            const int y = 70 + row * 50;
            icons[row] = CreateDef(28, y + 2, 100 + row, NH3Dlg::Assets::ARTIFACT_DEF, 0);
            names[row] = Label(80, y, 316, 28, h3_NullString, 100 + row);
            counts[row] = Label(80, y + 28, 316, 18, h3_NullString, 100 + row);
            frames[row] = CreateFrame(22, y, 382, 46, 100 + row, SelectionColor());
        }
        scroll = CreateScrollbar(412, 70, 16, 396, 20, (std::max)(2, int(this->offers.size()) - 7), Scrolled, FALSE, 1);
        title = Label(448, 70, 286, 46, Text("junkSelect"));
        const char *defs[] = {nullptr, NH3Dlg::Assets::ARTIFACT_DEF, "resour82.def",
            NH3Dlg::Assets::CREATURE_LARGE, NH3Dlg::Assets::PSKILL_42, NH3Dlg::Assets::SPELLS_DEF,
            "PSKILL.def", "SECSK82.DEF"};
        for (int kind = 1; kind < 8; ++kind)
        {
            H3DefLoader def(defs[kind]);
            rewards[kind] = CreateDef(590 - (def.Get() ? def->widthDEF : 44) / 2, 124, 400 + kind, defs[kind], 0);
        }
        Border(440, 62, 302, 398); Border(20, 62, 412, 406);
        reward = Label(448, 198, 286, 104, h3_NullString);
        stock = Label(448, 312, 286, 82, h3_NullString);
        status = Label(28, 474, 704, 28, Text("junkInstruction"));
        SetSlider(scroll, (std::max)(0, int(this->offers.size()) - 8), 0); Refresh();
    }
    BOOL Select(INT id, H3Msg &) override
    {
        if (id < 100 || id >= 108 || first + id - 100 >= int(offers.size())) return TRUE;
        selected = first + id - 100; Refresh(); Redraw();
        return DoubleClick(selected) ? Exchange() : FALSE;
    }
    BOOL OnLeftClick(INT id, H3Msg &msg) override
    { return id == 1 ? Exchange() : ObjectDialog::OnLeftClick(id, msg); }
    BOOL OnKeyPress(eVKey key, eMsgFlag flag) override
    { return key == eVKey::H3VK_ENTER ? Exchange() : ObjectDialog::OnKeyPress(key, flag); }
    BOOL OnMouseWheel(INT direction) override
    {
        first = (std::max)(0, (std::min)((std::max)(0, int(offers.size()) - 8), first + (direction > 0 ? -1 : 1)));
        SetSlider(scroll, (std::max)(0, int(offers.size()) - 8), first); Refresh(); Redraw(); return TRUE;
    }
    BOOL OnRightClick(H3DlgItem *item) override
    {
        const int row = item ? item->GetID() - 100 : -1;
        if (row >= 0 && row < 8 && first + row < int(offers.size()))
            H3Messagebox::RMC(H3Artifact(offers[first + row].artifact, offers[first + row].subtype).GetDescription());
        return TRUE;
    }
};

class SummoningDialog final : public ObjectDialog
{
    std::vector<SummoningOffer> offers;
    std::function<bool(int)> recruit;
    int first = 0, selected = -1;
    H3DlgDef *portraits[6]{};
    H3DlgText *names[6]{}, *towns[6]{}, *prices[6]{};
    H3DlgFrame *frames[6]{};
    H3DlgScrollbar *scroll = nullptr;
    bool CanAccept() const override { return selected >= 0 && selected < int(offers.size()); }
    void Refresh()
    {
        for (int row = 0; row < 6; ++row)
        {
            const int index = first + row;
            const bool shown = index < int(offers.size());
            Visible(portraits[row], shown); Visible(names[row], shown);
            Visible(towns[row], shown); Visible(prices[row], shown);
            Visible(frames[row], shown && index == selected);
            if (!shown) continue;
            const auto &offer = offers[index];
            portraits[row]->SetFrame(offer.creature + 2);
            names[row]->SetText((std::to_string(offer.count) + " " + P_CreatureInformation[offer.creature].namePlural).c_str());
            towns[row]->SetText(offer.town.c_str()); prices[row]->SetText(offer.price.c_str());
        }
        UpdateOkay();
    }
    BOOL Open()
    {
        if (CanAccept() && recruit(selected)) Stop();
        Redraw(); return FALSE;
    }
    static void __fastcall Scrolled(int tick, H3BaseDlg *dialog)
    {
        auto *self = static_cast<SummoningDialog *>(dialog);
        self->first = (std::max)(0, (std::min)(tick, int(self->offers.size()) - 6));
        self->Refresh(); self->Redraw();
    }
  public:
    SummoningDialog(const std::vector<SummoningOffer> &offers, const std::function<bool(int)> &recruit)
        : ObjectDialog(660, 590, util::ObjectName(objects::SUMMONING_STONES)), offers(offers), recruit(recruit)
    {
        Label(24, 46, 612, 30, util::Tr("summon.choose"));
        for (int row = 0; row < 6; ++row)
        {
            const int y = 80 + row * 64, id = 100 + row;
            Label(24, y, 590, 64, h3_NullString, id);
            portraits[row] = CreateDef(28, y, id, NH3Dlg::Assets::CREATURE_LARGE, 0);
            names[row] = Label(100, y, 506, 24, h3_NullString, id, eTextColor::GOLD);
            towns[row] = Label(100, y + 24, 506, 18, h3_NullString, id);
            prices[row] = Label(100, y + 42, 506, 20, h3_NullString, id);
            frames[row] = CreateFrame(24, y, 590, 64, id, SelectionColor());
        }
        scroll = CreateScrollbar(620, 80, 16, 384, 20, (std::max)(2, int(offers.size()) - 5), Scrolled, FALSE, 1);
        Label(24, 476, 612, 42, util::Tr("summon.listHint"));
        SetSlider(scroll, (std::max)(0, int(offers.size()) - 6), 0); Refresh();
    }
    BOOL Select(INT id, H3Msg &) override
    {
        if (id < 100 || id >= 106 || first + id - 100 >= int(offers.size())) return TRUE;
        selected = first + id - 100; Refresh(); Redraw(); return Open();
    }
    BOOL OnLeftClick(INT id, H3Msg &msg) override
    { return id == 1 ? Open() : ObjectDialog::OnLeftClick(id, msg); }
    BOOL OnKeyPress(eVKey key, eMsgFlag flag) override
    { return key == eVKey::H3VK_ENTER ? Open() : ObjectDialog::OnKeyPress(key, flag); }
    BOOL OnMouseWheel(INT direction) override
    {
        first = (std::max)(0, (std::min)((std::max)(0, int(offers.size()) - 6), first + (direction > 0 ? -1 : 1)));
        SetSlider(scroll, (std::max)(0, int(offers.size()) - 6), first); Refresh(); Redraw(); return TRUE;
    }
};

class ArcaneDialog final : public ObjectDialog
{
    H3Hero *hero;
    int paid = 0, wanted = 1, amount = 0;
    std::array<int, 4> clean{};
    H3DlgText *values[8]{}, *summary = nullptr;
    H3DlgFrame *frames[8]{};
    H3DlgScrollbar *slider = nullptr;
    int Maximum() const
    {
        if (paid == wanted) return 0;
        const int available = (std::min)(clean[paid] - (paid > 1 ? 1 : 0),
                                         int(static_cast<unsigned char>(hero->primarySkill[paid])) - (paid > 1 ? 1 : 0));
        return (std::max)(0, (std::min)(available, 125 - int(static_cast<unsigned char>(hero->primarySkill[wanted]))));
    }
    bool CanAccept() const override { return amount > 0 && amount <= Maximum(); }
    static void __fastcall Scrolled(int tick, H3BaseDlg *dialog)
    {
        auto *self = static_cast<ArcaneDialog *>(dialog);
        self->amount = self->Maximum() > 0 ? (std::min)(tick + 1, self->Maximum()) : 0;
        self->Refresh(); self->Redraw();
    }
    void Refresh()
    {
        for (int skill = 0; skill < 4; ++skill)
        {
            const int current = static_cast<unsigned char>(hero->primarySkill[skill]);
            const auto left = std::to_string(current) + " > " + std::to_string(current - (skill == paid ? amount : 0));
            const auto right = std::to_string(current) + " > " + std::to_string(current + (skill == wanted ? amount : 0));
            if (values[skill]) values[skill]->SetText(left.c_str());
            if (values[4 + skill]) values[4 + skill]->SetText(right.c_str());
            Visible(frames[skill], skill == paid); Visible(frames[4 + skill], skill == wanted);
        }
        if (summary) summary->SetText(Text("arcaneAmount", {"amount", std::to_string(amount),
            "max", std::to_string(Maximum())}).c_str());
        UpdateOkay();
    }
  public:
    explicit ArcaneDialog(H3Hero *hero) : ObjectDialog(740, 440, util::ObjectName(objects::ARCANE_TOWER)), hero(hero)
    {
        for (int skill = 0; skill < 4; ++skill) clean[skill] = util::CleanPrimary(hero, skill);
        Label(26, 50, 220, 28, Text("give")); Label(494, 50, 220, 28, Text("receive"));
        for (int side = 0; side < 2; ++side)
            for (int skill = 0; skill < 4; ++skill)
            {
                const int x = side ? 494 : 26, y = 88 + skill * 62, index = side * 4 + skill, id = 100 + index;
                CreateDef(x + 6, y + 6, id, NH3Dlg::Assets::PSKILL_42, skill);
                Label(x + 54, y, 158, 26, P_PrimarySkillName[skill], id);
                values[index] = Label(x + 54, y + 26, 158, 28, h3_NullString, id);
                frames[index] = CreateFrame(x, y, 220, 56, id, SelectionColor());
            }
        Label(256, 94, 228, 90, Text("arcaneRate"));
        Border(24, 82, 224, 252); Border(492, 82, 224, 252);
        slider = CreateScrollbar(256, 206, 228, 16, 20, 2, Scrolled, FALSE, 1);
        summary = Label(256, 238, 228, 90, h3_NullString);
        amount = Maximum() > 0 ? 1 : 0; SetSlider(slider, (std::max)(0, Maximum() - 1), 0); Refresh();
    }
    BOOL Select(INT id, H3Msg &) override
    {
        if (id >= 100 && id < 108)
        {
            if (id < 104) paid = id - 100; else wanted = id - 104;
            amount = Maximum() > 0 ? 1 : 0;
            SetSlider(slider, (std::max)(0, Maximum() - 1), 0); Refresh(); Redraw(); return FALSE;
        }
        return TRUE;
    }
    ArcaneTrade Choose() { return Run() ? ArcaneTrade{paid, wanted, amount} : ArcaneTrade{}; }
    BOOL OnMouseWheel(INT direction) override
    {
        amount = Maximum() > 0 ? (std::max)(1, (std::min)(Maximum(), amount + (direction > 0 ? 1 : -1))) : 0;
        SetSlider(slider, (std::max)(0, Maximum() - 1), (std::max)(0, amount - 1));
        Refresh(); Redraw(); return TRUE;
    }
};

class AltarDialog final : public ObjectDialog
{
    const std::array<AltarRow, 7> &rows;
    int available, mana = 0, town;
    H3DlgScrollbar *slider = nullptr;
    H3DlgText *summary = nullptr, *targets[7]{};
    H3DlgDef *portraits[7]{};
    H3DlgFrame *frames[10]{};
    static void __fastcall Scrolled(int tick, H3BaseDlg *dialog)
    {
        auto *self = static_cast<AltarDialog *>(dialog);
        self->mana = (std::min)(tick, self->available); self->Refresh(); self->Redraw();
    }
    void Refresh()
    {
        if (summary) summary->SetText(Text("altarCost", {"mana", std::to_string(mana),
            "left", std::to_string(available - mana), "chance", std::to_string(mana)}).c_str());
        for (int i = 0; i < 10; ++i) Visible(frames[i], i == town + 1);
        for (int slot = 0; slot < 7; ++slot)
        {
            const auto &row = rows[slot];
            const char *name = !row.eligible ? nullptr : mana == 0 ? nullptr : town < 0 ? nullptr :
                P_CreatureInformation[row.targets[town]].namePlural;
            const std::string target = !row.eligible ? Text("unchanged") : mana == 0 ? Text("randomTown") :
                town < 0 ? Text("neutral") : name;
            if (targets[slot]) targets[slot]->SetText(target.c_str());
            const int creature = row.eligible ? (mana > 0 && town >= 0 ? row.targets[town] : -1) : row.source;
            Visible(portraits[slot], util::ValidCreature(creature));
            if (portraits[slot] && util::ValidCreature(creature)) portraits[slot]->SetFrame(creature + 2);
        }
    }
  public:
    AltarDialog(H3Hero *hero, const std::array<AltarRow, 7> &rows)
        : ObjectDialog(740, 590, util::ObjectName(objects::ALTAR_OF_TRANSFORMATION)), rows(rows),
          available((std::max)(0, int(hero->spellPoints))), town((std::max)(0, (std::min)(8, hero->hero_class / 2)))
    {
        Label(26, 48, 688, 28, Text("altarDestination"));
        for (int i = 0; i < 10; ++i)
        {
            const int x = 26 + (i % 5) * 140, y = 78 + (i / 5) * 38;
            Label(x, y, 128, 34, i == 0 ? Text("neutral") : P_TownNames[i - 1], 100 + i);
            frames[i] = CreateFrame(x, y, 128, 34, 100 + i, SelectionColor());
        }
        Label(26, 158, 330, 26, Text("before")); Label(380, 158, 330, 26, Text("after"));
        for (int slot = 0; slot < 7; ++slot)
        {
            const auto &row = rows[slot]; const int y = 190 + slot * 36;
            if (util::ValidCreature(row.source) && row.count > 0)
            {
                CreateDef(30, y, -1, NH3Dlg::Assets::CREATURE_SMALL, row.source + 2);
                Border(28, y - 1, 34, 34);
                Label(68, y, 288, 32, std::to_string(row.count) + " " + P_CreatureInformation[row.source].namePlural);
                portraits[slot] = CreateDef(380, y, -1, NH3Dlg::Assets::CREATURE_SMALL, row.source + 2);
                Border(378, y - 1, 34, 34);
                targets[slot] = Label(420, y, 290, 32, h3_NullString);
            }
        }
        slider = CreateScrollbar(40, 456, 660, 16, 20, (std::max)(2, (std::min)(100, available) + 1), Scrolled, FALSE, 1);
        summary = Label(26, 478, 688, 42, h3_NullString);
        mana = (std::min)(100, available); SetSlider(slider, (std::min)(100, available), mana); Refresh();
    }
    BOOL Select(INT id, H3Msg &) override
    {
        if (id >= 100 && id < 110) { town = id - 101; Refresh(); Redraw(); return FALSE; }
        return TRUE;
    }
    AltarChoice Choose() { return Run() ? AltarChoice{mana, mana ? town : -2, true} : AltarChoice{}; }
};
} // namespace
SkullTarget ChooseSkullTarget() { return SkullDialog().Choose(); }
void ShowSkullSearch(const std::function<void(const SkullTarget &)> &search) { SkullDialog(search).Run(); }
void ShowSkullService(SkullService service, const std::function<void(int, SkullService &)> &action)
{ SkullServiceDialog(std::move(service), action).Run(); }
bool ConfirmDwellingOffer(const std::vector<std::pair<int, int>> &stacks, int gold)
{ return DwellingDialog(stacks, gold).Run(); }
int ChoosePalaceGame(bool available) { return PalaceGameDialog(available).Choose(); }
bool ShowPalaceSpin(bool artifacts, const std::array<int,3> &reels, const std::string &message, bool repeat)
{ return PalaceSpinDialog(artifacts, reels, message, repeat).Again(); }
void ShowJunkMerchant(std::vector<JunkOffer> offers, const std::function<void(std::vector<JunkOffer> &)> &refresh,
                      const std::function<bool(const JunkOffer &)> &trade)
{ JunkDialog(std::move(offers), refresh, trade).Run(); }
void ShowSummoningOffers(const std::vector<SummoningOffer> &offers, const std::function<bool(int)> &recruit)
{ SummoningDialog(offers, recruit).Run(); }
ArcaneTrade ChooseArcaneTrade(H3Hero *hero) { return ArcaneDialog(hero).Choose(); }
AltarChoice ChooseAltarTransformation(H3Hero *hero, const std::array<AltarRow, 7> &rows)
{ return AltarDialog(hero, rows).Choose(); }
} }
