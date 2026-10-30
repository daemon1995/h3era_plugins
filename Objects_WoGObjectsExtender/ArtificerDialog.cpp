#include "ArtificerDialog.h"
#include <memory>
#include <string>

namespace wog
{
namespace artificer
{
namespace
{
constexpr int WIDTH = 700, HEIGHT = 590, ROWS = 6, ROW_FIRST = 1000;
constexpr int GRID_Y = 146, CELL_H = 48, ROW_STEP = 50;
constexpr int TAB_FIRST = 10, CLOSE = 1, HELP = 2, PREVIOUS = 3, NEXT = 4, APPLY = 5, SELL_ALL = 6;
constexpr int GOLD = 7, MITHRIL = 8;

std::string Text(const char *key, const std::vector<std::string> &parameters = {})
{
    return Era::tr((std::string("wog_objects.artificer.") + key).c_str(), parameters);
}
std::string Ui(const char *key, const std::vector<std::string> &parameters = {})
{
    return Text((std::string("ui.") + key).c_str(), parameters);
}
std::string ArtifactName(Item item)
{
    if (item.id < 0 || item.id >= H3ArtifactCount::Get())
        return h3_NullString;
    const char *name = H3Artifact(item.id, item.modifier).GetName();
    return name ? name : h3_NullString;
}
std::string SlotName(int slot)
{
    if (slot >= BACKPACK)
        return Text("backpackSlot", {"slot", std::to_string(slot - BACKPACK + 1)});
    static const char *keys[] = {"head",  "shoulders", "neck",     "rightHand", "leftHand",
                                 "torso", "rightRing", "leftRing", "feet"};
    if (slot >= 0 && slot < 9)
        return Text(keys[slot]);
    return Text("slots.misc", {"slot", std::to_string(slot == 18 ? 5 : slot - 8)});
}
std::string PriceText(Price price)
{
    return Text(price.mithril ? (price.gold ? "price.both" : "price.mithril") : "price.gold",
                {"gold", std::to_string(std::abs(price.gold)), "mithril", std::to_string(std::abs(price.mithril))});
}

void Visible(H3DlgItem *item, bool visible)
{
    if (!item)
        return;
    if (visible)
        item->ShowActivate();
    else
        item->HideDeactivate();
}
void Enabled(H3DlgItem *item, bool enabled)
{
    if (!item)
        return;
    if (enabled)
    {
        item->UnShade();
        item->Activate();
    }
    else
    {
        item->Shade();
        item->DeActivate();
    }
}

struct MarketButton
{
    H3DlgDefButton *control = nullptr;
    H3DlgText *label = nullptr;
    bool shown = true, selected = false, selector = false;
    void SetText(const char *text)
    {
        if (label)
            label->SetText(text);
        else if (control)
            control->SetText(text);
    }
    void SetHints(const char *text, const char *, BOOL)
    {
        if (control)
            control->SetHints(text, nullptr, TRUE);
        if (label)
            label->SetHints(text, nullptr, TRUE);
    }
    void SetFrame(int frame)
    {
        selected = frame != 0;
        if (control)
            control->SetFrame(frame);
    }
};
void Visible(MarketButton *button, bool visible)
{
    if (!button)
        return;
    button->shown = visible;
    Visible(button->control, visible);
    Visible(button->label, visible);
    if (button->selector && button->control)
        button->control->DeActivate();
}
void Enabled(MarketButton *button, bool enabled)
{
    if (!button)
        return;
    Enabled(button->control, enabled);
    Enabled(button->label, enabled);
    if (button->selector && button->control)
        button->control->DeActivate();
}

class ArtificerDialog final : public H3Dlg
{
    Host &host;
    H3Hero *hero;
    int tab = 0, page = 0, selected = -1;
    std::vector<Offer> offers;
    SaleBatch batch;

    struct Row
    {
        H3DlgText *button = nullptr, *rightButton = nullptr, *name = nullptr, *resultName = nullptr;
        H3DlgDef *source = nullptr, *result = nullptr, *reward = nullptr;
        H3DlgFrame *frame = nullptr, *rightFrame = nullptr;
    } rows[ROWS];
    MarketButton *tabs[4]{}, *goldButton = nullptr, *mithrilButton = nullptr;
    MarketButton *apply = nullptr, *sellAll = nullptr, *previous = nullptr, *next = nullptr;
    std::vector<std::unique_ptr<MarketButton>> buttons;
    H3DlgText *wallet[2]{}, *status = nullptr, *count = nullptr, *paging = nullptr, *empty = nullptr;
    H3DlgText *details = nullptr, *compactDetails = nullptr;
    bool goldCurrency = true, fatal = false;
    int lastOffer = -1;
    DWORD lastPress = 0;

    Service CurrentService() const
    {
        return tab < 3 ? static_cast<Service>(tab) : goldCurrency ? Service::SellGold : Service::SellMithril;
    }
    MarketButton *Button(int x, int y, int width, int id, const std::string &text, int height = 28)
    {
        buttons.emplace_back(new MarketButton());
        auto *button = buttons.back().get();

        button->selector = IsSelection(id);
        const bool arrow = id == PREVIOUS || id == NEXT;
        const char *defName = id == CLOSE      ? NH3Dlg::Assets::CANCEL_DEF
                              : id == APPLY    ? NH3Dlg::Assets::OKAY_DEF
                              : id == PREVIOUS ? "SCNRBLF.def"
                              : id == NEXT     ? "SCNRBRT.def"
                                               : NH3Dlg::Assets::OVBUTN3;
        H3DefLoader def(defName);
        if (!def.Get())
            return button;
        const int bx = x + (width - def->widthDEF) / 2, by = y + (height - def->heightDEF) / 2;
        if (id == CLOSE)
            button->control = CreateCancelButton(bx, by, id, FALSE);
        else if (id == APPLY)
        {
            button->control = CreateOKButton(x, by, id, FALSE);
            button->label = Label(x + 70, y, width - 70, height, text, id);
        }
        else if (button->selector || arrow)
        {
            button->control = H3DlgCaptionButton::Create(bx, by, id, defName, h3_NullString, NH3Dlg::Text::SMALL, 0, 0, FALSE, 0,
                                                         eTextColor::GOLD);
            // The text surface delivers the press; native buttons otherwise wait for release.
            button->label = Label(x, y, width, height, arrow ? h3_NullString : text, id, eTextColor::GOLD, NH3Dlg::Text::SMALL,
                                  eTextAlignment::MIDDLE_CENTER);
        }
        else
            button->control = H3DlgCaptionButton::Create(bx, by, id, defName, text.c_str(), NH3Dlg::Text::SMALL, 0, 0,
                                                         FALSE, 0, eTextColor::GOLD);
        if (button->control)
        {
            if (id != CLOSE && id != APPLY)
                AddItem(button->control);
            button->control->SetClickFrame(1);
            if (arrow)
                button->control->ColorDefToPlayer(NH3Players::ePlayerColors::TAN);
            button->SetHints(text.c_str(), nullptr, TRUE);
            if (button->selector)
                button->control->DeActivate();
            if (id == CLOSE)
                button->control->AddHotkey(eVKey::H3VK_ESCAPE);
            if (id == APPLY)
                button->control->AddHotkey(eVKey::H3VK_ENTER);
        }

        return button;
    }
    H3DlgText *Label(int x, int y, int width, int height, const std::string &text = h3_NullString, int id = -1,
                     eTextColor color = eTextColor::REGULAR, const char *font = NH3Dlg::Text::SMALL,
                     eTextAlignment alignment = eTextAlignment::MIDDLE_LEFT)
    {
        return CreateText(x, y, width, height, text.c_str(), font, color, id, alignment);
    }
    H3DlgDef *Icon(int x, int y, int id, const char *name = NH3Dlg::Assets::ARTIFACT_DEF, int frame = 0)
    {
        auto *image = CreateDef(x, y, id, name, 0);
        if (image)
        {
            const auto *def = image->GetDef();
            if (def && def->groupsCount && def->groups && def->groups[0] && frame >= 0 && frame < def->groups[0]->count)
                image->SetFrame(frame);
            else
            {
                image->HideDeactivate();
                return nullptr;
            }
        }
        return image;
    }
    void ShowArtifact(H3DlgDef *image, Item item)
    {
        bool valid = image && item.id >= 0 && item.id < H3ArtifactCount::Get();
        const auto *def = image ? image->GetDef() : nullptr;
        valid = valid && def && def->groupsCount > 0 && def->groups[0] && item.id < def->groups[0]->count;
        if (valid)
            image->SetFrame(item.id);
        Visible(image, valid);
        if (image)
            image->SetHints(ArtifactName(item).c_str(), nullptr, TRUE);
    }
    void Background()
    {
        auto *canvas = H3LoadedPcx16::Create(WIDTH, HEIGHT);
        if (!canvas)
            return;
        canvas->FillRectangle(0, 0, WIDTH, HEIGHT, 35, 30, 24);
        H3PcxLoader texture(NH3Dlg::Assets::DIBOXBACK);
        if (texture.Get() && texture->width > 0 && texture->height > 0)
            for (int y = 0; y < HEIGHT; y += texture->height)
                for (int x = 0; x < WIDTH; x += texture->width)
                    texture->DrawToPcx16(0, 0, (std::min)(texture->width, WIDTH - x),
                                         (std::min)(texture->height, HEIGHT - y), canvas, x, y, FALSE);

        canvas->DarkenArea(18, 122, 336, 328, 20);
        canvas->DarkenArea(370, 122, 312, 328, 20);
        canvas->SimpleFrameRegion(18, 122, 336, 328);
        canvas->SimpleFrameRegion(370, 122, 312, 328);
        canvas->SimpleFrameRegion(18, 476, 664, 48);
        for (int i = 0; i < ROWS; ++i)
        {
            const int y = GRID_Y + i * ROW_STEP;
            canvas->SimpleFrameRegion(28, y + 1, 46, 46);
            canvas->SimpleFrameRegion(380, y + 1, 46, 46);
        }
        H3DefLoader border(NH3Dlg::Assets::DLGBOX);
        if (border.Get())
            canvas->FrameRegion(0, 0, WIDTH, HEIGHT, TRUE, 0, FALSE);
        AddBackground(canvas);
        canvas->Destroy();
    }
    std::string Hint(const Offer &offer) const
    {
        std::string text;
        for (int i = 0; i < offer.count; ++i)
        {
            if (i)
                text += ", ";
            text += ArtifactName(offer.sources[i].item) + " (" + SlotName(offer.sources[i].slot) + ")";
        }
        if (!offer.Sale())
            text += " -> " + ArtifactName(offer.result);
        return text + " | " + PriceText(offer.price);
    }
    void Refresh(bool resetPage = false)
    {
        if (resetPage)
        {
            page = 0;
            selected = -1;
            lastOffer = -1;
        }
        host.RefreshCatalog();
        const auto state = host.ReadState();
        const auto options = host.ReadOptions();
        const auto inventory = host.ReadInventory();
        if (tab == 3 && !options.mithril)
        {
            tab = 0;
            page = 0;
            selected = -1;
        }
        offers = BuildOffers(inventory, host, options, state, CurrentService());
        batch = tab == 3 ? BuildSaleBatch(inventory, host, options, state, CurrentService()) : SaleBatch{};
        page = (std::min)(page, (std::max)(0, (int(offers.size()) - 1) / ROWS));
        if (selected < page * ROWS || selected >= (page + 1) * ROWS || selected >= int(offers.size()))
            selected = offers.empty() ? -1 : page * ROWS;
        for (int i = 0; i < 4; ++i)
        {
            if (tabs[i])
                tabs[i]->SetFrame(i == tab ? 1 : 0);
            if (i == 3)
                Enabled(tabs[i], options.mithril);
        }
        Visible(goldButton, tab == 3);
        Visible(mithrilButton, tab == 3);
        if (goldButton)
            goldButton->SetFrame(goldCurrency ? 1 : 0);
        if (mithrilButton)
            mithrilButton->SetFrame(goldCurrency ? 0 : 1);
        const auto money = host.ReadWallet();
        if (wallet[0])
            wallet[0]->SetText(std::to_string(money.gold).c_str());
        if (wallet[1])
            wallet[1]->SetText(std::to_string(money.mithril).c_str());
        if (status)
            status->SetText(Ui(state.Orders() >= 2 ? (options.mithril ? "tired" : "tiredNoSales")
                               : state.Orders()    ? "secondOrder"
                                                   : "orders",
                               {"count", std::to_string(2 - state.Orders()), "movement", std::to_string(MOVEMENT_COST)})
                                .c_str());
        if (count)
            count->SetText(Ui("givingCount", {"count", std::to_string(offers.size())}).c_str());
        if (empty)
            empty->SetText(Ui(tab == 3              ? "emptyBag"
                              : state.Orders() >= 2 ? (options.mithril ? "noOrders" : "noOrdersNoSales")
                                                    : "noOffers")
                               .c_str());
        Visible(empty, offers.empty());
        if (paging)
            paging->SetText(Ui("page", {"current", std::to_string(page + 1), "total",
                                        std::to_string((std::max)(1, (int(offers.size()) + ROWS - 1) / ROWS))})
                                .c_str());
        Enabled(previous, page > 0);
        Enabled(next, (page + 1) * ROWS < int(offers.size()));
        for (int i = 0; i < ROWS; ++i)
        {
            auto &row = rows[i];
            const int index = page * ROWS + i;
            const bool visible = index < int(offers.size());

            for (H3DlgItem *item : {static_cast<H3DlgItem *>(row.button), static_cast<H3DlgItem *>(row.rightButton),
                                    static_cast<H3DlgItem *>(row.name), static_cast<H3DlgItem *>(row.resultName)})
                Visible(item, visible);
            Visible(row.frame, visible && index == selected);
            Visible(row.rightFrame, visible && index == selected);
            if (row.frame)
                row.frame->DeActivate();
            if (row.rightFrame)
                row.rightFrame->DeActivate();
            if (!visible)
            {
                Visible(row.source, false);
                Visible(row.result, false);
                Visible(row.reward, false);
                continue;
            }
            const auto &offer = offers[index];
            ShowArtifact(row.source, offer.sources[0].item);
            ShowArtifact(row.result, offer.Sale() ? Item{} : offer.result);
            Visible(row.reward, offer.Sale());
            if (row.reward)
                row.reward->SetFrame(goldCurrency ? 6 : 7);
            std::string name = ArtifactName(offer.sources[0].item);
            if (offer.count > 1)
                name += " + " + std::to_string(offer.count - 1);
            row.name->SetText(name.c_str());
            row.resultName->SetText(offer.Sale() ? PriceText(offer.price).c_str() : ArtifactName(offer.result).c_str());
            const auto hint = Hint(offer);
            for (H3DlgItem *item : {static_cast<H3DlgItem *>(row.button), static_cast<H3DlgItem *>(row.rightButton),
                                    static_cast<H3DlgItem *>(row.name), static_cast<H3DlgItem *>(row.resultName)})
                if (item)
                    item->SetHints(hint.c_str(), nullptr, TRUE);
        }

        RefreshDetails();
        Redraw();
    }

    void RefreshDetails()
    {
        const auto *offer = selected >= 0 && selected < int(offers.size()) ? &offers[selected] : nullptr;
        std::string inputs;
        if (offer)
            for (int i = 0; i < offer->count; ++i)
                inputs += (inputs.empty() ? h3_NullString : ", ") + ArtifactName(offer->sources[i].item);
        if (details)
        {
            static const char *services[] = {"upgradeDetails", "exchangeDetails", "combineDetails", "saleDetails"};
            const std::string text =
                offer ? Ui(services[tab], {"items", inputs, "price", PriceText(offer->price), "movement",
                                           std::to_string(MOVEMENT_COST), "count", std::to_string(batch.offers.size()),
                                           "total", PriceText(batch.price)})
                      : Ui("selectTrade");
            auto *font = details->GetFont();
            const bool compact = font && font->GetLinesCountInText(text.c_str(), 648) * font->height > 44;
            Visible(details, !compact);
            Visible(compactDetails, compact);
            (compact ? compactDetails : details)->SetText(text.c_str());
        }
        static const char *actions[] = {"upgrade", "exchange", "combine", "sell"};
        if (apply)
            apply->SetText(Ui(actions[tab]).c_str());
        Enabled(apply, offer && host.ReadWallet().CanPay(offer->price));
        if (apply)
            apply->SetHints(Ui(offer && !host.ReadWallet().CanPay(offer->price) ? "cannotPay"
                               : tab == 3                                       ? "saleHint"
                                                                                : "workHint")
                                .c_str(),
                            nullptr, TRUE);
        Visible(sellAll, tab == 3);
        if (sellAll)
            sellAll->SetText(Ui("sellAll", {"count", std::to_string(batch.offers.size())}).c_str());
        Enabled(sellAll, batch.valid && host.ReadWallet().CanPay(batch.price));
        if (sellAll)
            sellAll->SetHints(Ui("sellAllHint", {"price", PriceText(batch.price)}).c_str(), nullptr, TRUE);
    }
    void Report(CommitResult result)
    {
        lastOffer = -1;
        if (result != CommitResult::Success)
            H3Messagebox::Show(Text(result == CommitResult::NoResources       ? "noResources"
                                    : result == CommitResult::RecoveryFailure ? "recoveryFailure"
                                                                              : "changed")
                                   .c_str());
        if (result == CommitResult::RecoveryFailure)
        {
            fatal = true;
            Stop();
            return;
        }
        Refresh();
        if (result == CommitResult::Success && GetHintBar())
            GetHintBar()->ShowMessage(Ui("completed").c_str());
    }
    void Describe(Item item, int = -1)
    {
        if (item.id < 0 || item.id >= H3ArtifactCount::Get())
            return;
        const H3Artifact artifact(item.id, item.modifier);
        const int saved = P_WindowManager->resultItemID;
        H3Messagebox::RMC(artifact.GetDescription());
        P_WindowManager->resultItemID = saved;
        Refresh();
    }

  public:
    ArtificerDialog(Host &host, H3Hero *hero, const H3MapItem *)
        : H3Dlg(WIDTH, HEIGHT, -1, -1, FALSE, FALSE), host(host), hero(hero)
    {
        if (GetX() + WIDTH + 8 > H3GameWidth::Get() || GetY() + HEIGHT + 8 > H3GameHeight::Get())
            flags &= ~0x10;
        Background();
        Label(24, 16, 652, 30, Text("name"), -1, eTextColor::GOLD, NH3Dlg::Text::BIG, eTextAlignment::MIDDLE_CENTER);
        Label(24, 50, 340, 24, Ui("hero", {"name", hero->name}), -1, eTextColor::REGULAR, NH3Dlg::Text::MEDIUM);
        status = Label(24, 78, 652, 22, h3_NullString, -1, eTextColor::GOLD, NH3Dlg::Text::SMALL, eTextAlignment::MIDDLE_CENTER);
        for (int i = 0; i < 2; ++i)
        {
            auto *icon = Icon(400 + i * 142, 48, -1, NH3Dlg::Assets::RESOURCE_DEF, 6 + i);
            if (icon)
                icon->SetHints(i ? Text("mithril").c_str() : P_ResourceName[eResource::GOLD], nullptr, TRUE);
            wallet[i] = Label(438 + i * 142, 50, 96, 24, h3_NullString, -1, eTextColor::GOLD, NH3Dlg::Text::MEDIUM);
        }
        static const char *names[] = {"upgrades", "exchanges", "combinations", "sales"};
        for (int i = 0; i < 4; ++i)
            tabs[i] = Button(24 + i * 164, 100, 160, TAB_FIRST + i, Ui(names[i]), 24);

        count = Label(26, 126, 320, 20, h3_NullString, -1, eTextColor::GOLD, NH3Dlg::Text::SMALL, eTextAlignment::MIDDLE_CENTER);
        Label(386, 126, 280, 20, Ui("receiving"), -1, eTextColor::GOLD, NH3Dlg::Text::SMALL,
              eTextAlignment::MIDDLE_CENTER);
        goldButton = Button(384, 450, 140, GOLD, P_ResourceName[eResource::GOLD], 26);
        mithrilButton = Button(530, 450, 140, MITHRIL, Text("mithril"), 26);
        empty =
            Label(38, 180, 296, 230, h3_NullString, -1, eTextColor::REGULAR, NH3Dlg::Text::MEDIUM, eTextAlignment::MIDDLE_CENTER);
        const auto *palette = H3GamePalette::Get();
        const H3RGB565 selectionColor = palette ? palette->color[45] : H3RGB565(248, 252, 0);
        for (int i = 0; i < ROWS; ++i)
        {
            const int y = GRID_Y + i * ROW_STEP, id = ROW_FIRST + i;
            auto &row = rows[i];
            row.button = Label(24, y, 324, CELL_H, h3_NullString, id);
            row.rightButton = Label(376, y, 300, CELL_H, h3_NullString, id);
            row.source = Icon(29, y + 2, id);
            row.result = Icon(381, y + 2, id);
            row.reward = Icon(387, y + 8, id, NH3Dlg::Assets::RESOURCE_DEF, 6);
            row.name = Label(82, y, 262, CELL_H, h3_NullString, id);
            row.resultName = Label(434, y, 236, CELL_H, h3_NullString, id, eTextColor::GOLD);
            row.frame = H3DlgFrame::Create(24, y, 324, CELL_H, id, selectionColor);
            row.rightFrame = H3DlgFrame::Create(376, y, 300, CELL_H, id, selectionColor);
            if (row.frame)
            {
                AddItem(row.frame);
                row.frame->DeActivate();
            }
            if (row.rightFrame)
            {
                AddItem(row.rightFrame);
                row.rightFrame->DeActivate();
            }
        }
        previous = Button(24, 450, 40, PREVIOUS, h3_NullString);
        next = Button(306, 450, 40, NEXT, h3_NullString);
        paging =
            Label(68, 450, 234, 26, h3_NullString, -1, eTextColor::REGULAR, NH3Dlg::Text::SMALL, eTextAlignment::MIDDLE_CENTER);
        details =
            Label(26, 478, 648, 44, h3_NullString, -1, eTextColor::REGULAR, NH3Dlg::Text::SMALL, eTextAlignment::MIDDLE_LEFT);
        compactDetails =
            Label(26, 478, 648, 44, h3_NullString, -1, eTextColor::REGULAR, NH3Dlg::Text::TINY, eTextAlignment::MIDDLE_LEFT);
        Button(24, 528, 140, HELP, Ui("helpButton"), 30);
        Button(272, 528, 64, CLOSE, Ui("close"), 30);
        apply = Button(378, 528, 144, APPLY, h3_NullString, 30);
        sellAll = Button(530, 528, 144, SELL_ALL, h3_NullString, 30);
        CreateHint(18, 572, 664, 14);

        Refresh();
    }
    bool IsSelection(INT id) const
    {
        return (id >= ROW_FIRST && id < ROW_FIRST + ROWS) || (id >= TAB_FIRST && id < TAB_FIRST + 4) || id == GOLD ||
               id == MITHRIL || id == PREVIOUS || id == NEXT;
    }
    BOOL DialogProc(H3Msg &msg) override
    {
        if (msg.IsLeftDown() && IsSelection(msg.itemId))
            return HandleClick(msg.itemId, msg);
        if (msg.command == eMsgCommand::LCLICK_OUTSIDE)
        {
            auto *item = ItemAtPosition(msg);
            if (item && IsSelection(item->GetID()))
                return HandleClick(item->GetID(), msg);
        }
        return TRUE;
    }
    BOOL OnLeftClick(INT id, H3Msg &msg) override
    {
        return IsSelection(id) ? TRUE : HandleClick(id, msg);
    }
    BOOL HandleClick(INT id, H3Msg &)
    {
        if (fatal)
            return FALSE;
        if (id >= ROW_FIRST && id < ROW_FIRST + ROWS)
            P_SoundMgr->ClickSound();
        if (id == CLOSE)
        {
            Stop();
            return FALSE;
        }
        if (id == HELP)
        {
            H3Messagebox::Show(Ui("help").c_str());
            Refresh();
            return FALSE;
        }
        if (id >= TAB_FIRST && id < TAB_FIRST + 4)
        {
            if (id == TAB_FIRST + 3 && !host.ReadOptions().mithril)
                return FALSE;
            tab = id - TAB_FIRST;
            Refresh(true);
            return FALSE;
        }
        if (id == GOLD || id == MITHRIL)
        {
            goldCurrency = id == GOLD;
            Refresh(true);
            return FALSE;
        }
        if (id == PREVIOUS || id == NEXT)
        {
            lastOffer = -1;
            page += id == NEXT ? 1 : -1;
            page = (std::max)(0, page);
            selected = -1;
            Refresh();
            return FALSE;
        }
        if (id >= ROW_FIRST && id < ROW_FIRST + ROWS)
        {
            selected = page * ROWS + id - ROW_FIRST;
            const DWORD now = GetTime();
            const bool confirm = selected == lastOffer && now - lastPress < 300;
            lastOffer = selected;
            lastPress = now;
            Refresh();
            if (confirm && selected < int(offers.size()) && host.ReadWallet().CanPay(offers[selected].price))
            {
                const auto quote = offers[selected];
                Report(Commit(host, quote));
            }
            return FALSE;
        }
        if (id == APPLY && selected >= 0 && selected < int(offers.size()))
        {
            const auto quote = offers[selected];
            Report(Commit(host, quote));
            return FALSE;
        }
        if (id == SELL_ALL && tab == 3 && batch.valid)
        {
            const auto quote = batch;
            const auto message =
                Ui("confirmAll", {"count", std::to_string(quote.offers.size()), "price", PriceText(quote.price)});
            const int saved = P_WindowManager->resultItemID;
            const bool confirmed = H3Messagebox::Choice(message.c_str()) != 0;
            P_WindowManager->resultItemID = saved;
            if (confirmed)
                Report(CommitSaleBatch(host, quote));
            else
                Refresh();
            return FALSE;
        }
        return TRUE;
    }
    BOOL OnRightClick(H3DlgItem *item) override
    {
        const int id = item ? item->GetID() : -1;
        if (id >= ROW_FIRST && id < ROW_FIRST + ROWS)
        {
            const int index = page * ROWS + id - ROW_FIRST;
            if (index < int(offers.size()))
            {
                const auto &offer = offers[index];
                if (item == rows[id - ROW_FIRST].source || item == rows[id - ROW_FIRST].name ||
                    item == rows[id - ROW_FIRST].button)
                    Describe(offer.sources[0].item, offer.sources[0].slot);
                else if (offer.Sale())
                    return TRUE;
                else
                    Describe(offer.result);
            }
            return FALSE;
        }
        return TRUE;
    }

    BOOL OnMouseHover(H3DlgItem *item) override
    {
        if (GetHintBar())
            GetHintBar()->ShowMessage(item && item->GetHint() ? item->GetHint() : Ui("hint").c_str());
        return TRUE;
    }
    BOOL OnMouseWheel(INT direction) override
    {
        lastOffer = -1;
        page += direction > 0 ? -1 : 1;
        page = (std::max)(0, page);
        selected = -1;
        Refresh();
        return FALSE;
    }
    BOOL OnKeyPress(eVKey key, eMsgFlag) override
    {
        if (key == eVKey::H3VK_ESCAPE)
        {
            Stop();
            return FALSE;
        }
        if (key == eVKey::H3VK_ENTER && selected >= 0 && selected < int(offers.size()) &&
            host.ReadWallet().CanPay(offers[selected].price))
        {
            const auto quote = offers[selected];
            Report(Commit(host, quote));
            return FALSE;
        }
        return TRUE;
    }
};
} // namespace

void ShowDialog(Host &host, H3Hero *hero, const H3MapItem *object)
{
    ArtificerDialog dialog(host, hero, object);
    dialog.Start();
}
} // namespace artificer
} // namespace wog
