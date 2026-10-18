#include "ArtificerDialog.h"
#include "../headers/EraPluginsAPI/ArtifactDescriptionApi.h"
#include <string>
#include <memory>

namespace wog { namespace artificer {
namespace {
constexpr int WIDTH = 780, HEIGHT = 560, ROWS = 5, ROW_FIRST = 1000;
constexpr int TAB_FIRST = 10, CLOSE = 1, HELP = 2, PREVIOUS = 3, NEXT = 4, APPLY = 5, SELL_ALL = 6;
constexpr int GOLD = 7, MITHRIL = 8, RESULT = 90, PART_FIRST = 100;

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
    if (item.id < 0 || item.id >= H3ArtifactCount::Get()) return "";
    const char *name = H3Artifact(item.id, item.modifier).GetName();
    return name ? name : "";
}
std::string SlotName(int slot)
{
    if (slot >= BACKPACK) return Text("backpackSlot", {"slot", std::to_string(slot - BACKPACK + 1)});
    static const char *keys[] = {"head", "shoulders", "neck", "rightHand", "leftHand", "torso",
                                 "rightRing", "leftRing", "feet"};
    if (slot >= 0 && slot < 9) return Text(keys[slot]);
    return Text("slots.misc", {"slot", std::to_string(slot == 18 ? 5 : slot - 8)});
}
std::string PriceText(Price price)
{
    return Text(price.mithril ? (price.gold ? "price.both" : "price.mithril") : "price.gold",
        {"gold", std::to_string(std::abs(price.gold)), "mithril", std::to_string(std::abs(price.mithril))});
}

void Visible(H3DlgItem *item, bool visible)
{
    if (!item) return;
    if (visible) item->ShowActivate(); else item->HideDeactivate();
}
void Enabled(H3DlgItem *item, bool enabled)
{
    if (!item) return;
    if (enabled) { item->UnShade(); item->Activate(); }
    else { item->Shade(); item->DeActivate(); }
}

// Text controls provide full-size hit areas. Native caption buttons draw their DEF at its
// original 108x16 size, which cannot fill the wider tabs and trade buttons in this layout.
struct MarketButton
{
    H3DlgText *caption = nullptr;
    H3DlgPcx16 *image = nullptr;
    H3DlgFrame *selection = nullptr;
    H3LoadedPcx16 *surface = nullptr;
    bool shown = true, selected = false;
    ~MarketButton()
    {
        if (image) image->SetPcx(static_cast<H3LoadedPcx16 *>(nullptr));
        if (surface) surface->Destroy();
    }
    void SetText(const char *text) { if (caption) caption->SetText(text); }
    void SetHints(const char *text, const char *, BOOL) { if (caption) caption->SetHints(text, nullptr, TRUE); }
    void SetFrame(int frame)
    {
        selected = frame != 0;
        Visible(selection, shown && selected);
        if (selection) selection->DeActivate();
    }
};
void Visible(MarketButton *button, bool visible)
{
    if (!button) return;
    button->shown = visible;
    Visible(button->image, visible); Visible(button->caption, visible);
    Visible(button->selection, visible && button->selected);
    if (button->selection) button->selection->DeActivate();
}
void Enabled(MarketButton *button, bool enabled)
{
    if (!button) return;
    Enabled(button->image, enabled); Enabled(button->caption, enabled);
}

std::string ObjectDef(const H3MapItem *object)
{
    const auto &attributes = P_Main->mainSetup.objectAttributes;
    if (object)
        for (const auto &drawing : object->objectDrawing)
            if (drawing.sprite < attributes.Count())
            {
                const auto &entry = attributes[drawing.sprite];
                if (entry.subtype == OBJECT_SUBTYPE && (entry.type == 63 || entry.type == 141))
                    return entry.defName.String();
            }
    for (const auto &entry : attributes)
        if (entry.subtype == OBJECT_SUBTYPE && (entry.type == 63 || entry.type == 141)) return entry.defName.String();
    return "ZArtif1.def";
}
// Bake the visible DEF frame into the header, fitting even large custom object graphics.
// Draw into a full-sized surface first: DEF margins are significant to the native decoder.
void DrawObject(H3LoadedPcx16 *target, const std::string &name)
{
    H3DefLoader def(name.c_str());
    if (!def.Get() || !def->groupsCount || !def->groups || !def->groups[0] || !def->groups[0]->count) return;
    const auto *frame = def->GetGroupFrame(0, 0);
    if (!frame || frame->width <= 0 || frame->height <= 0 || frame->width > 4096 || frame->height > 4096 ||
        frame->frameWidth <= 0 || frame->frameHeight <= 0 || frame->marginLeft < 0 || frame->marginTop < 0 ||
        frame->marginLeft + frame->frameWidth > frame->width || frame->marginTop + frame->frameHeight > frame->height) return;
    auto *source = H3LoadedPcx16::Create(frame->width, frame->height);
    if (!source) return;
    source->FillRectangle(0, 0, source->width, source->height, 27, 23, 18);
    def->DrawToPcx16(0, 0, 0, 0, source->width, source->height, source, 0, 0);
    const double scale = (std::min)(1.0, (std::min)(104.0 / frame->frameWidth, 78.0 / frame->frameHeight));
    const int w = (std::max)(1, int(frame->frameWidth * scale)), h = (std::max)(1, int(frame->frameHeight * scale));
    const int x = 20 + (104 - w) / 2, y = 16 + (78 - h) / 2;
    for (int row = 0; row < h; ++row)
        for (int col = 0; col < w; ++col)
        {
            const auto color = source->GetPixel(frame->marginLeft + col * frame->frameWidth / w,
                                                 frame->marginTop + row * frame->frameHeight / h);
            if (H3BitMode::Get() == 4) *target->GetPixel888(x + col, y + row) = color;
            else *target->GetPixel565(x + col, y + row) = H3RGB565(color);
        }
    source->Destroy();
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
        H3DlgText *button = nullptr;
        H3DlgDef *source = nullptr, *result = nullptr;
        H3DlgText *name = nullptr, *slot = nullptr, *price = nullptr, *arrow = nullptr;
        H3DlgFrame *frame = nullptr;
    } rows[ROWS];
    MarketButton *tabs[4]{}, *goldButton = nullptr, *mithrilButton = nullptr;
    MarketButton *apply = nullptr, *sellAll = nullptr, *previous = nullptr, *next = nullptr;
    std::vector<std::unique_ptr<MarketButton>> buttons;
    H3DlgText *wallet[2]{}, *status = nullptr, *count = nullptr, *paging = nullptr, *empty = nullptr;
    H3DlgText *detailTitle = nullptr, *detailName = nullptr, *partsTitle = nullptr, *amounts[2]{};
    H3DlgText *batchSummary = nullptr;
    H3DlgDef *resultImage = nullptr, *resourceImages[2]{};
    struct Part { H3DlgDef *image = nullptr; H3DlgText *name = nullptr; } parts[6];
    bool goldCurrency = true, fatal = false;

    Service CurrentService() const
    {
        return tab < 3 ? static_cast<Service>(tab) : goldCurrency ? Service::SellGold : Service::SellMithril;
    }
    MarketButton *Button(int x, int y, int width, int id, const std::string &text, int height = 28)
    {
        buttons.emplace_back(new MarketButton());
        auto *button = buttons.back().get();
        button->surface = H3LoadedPcx16::Create(width, height);
        if (button->surface)
        {
            auto *surface = button->surface;
            surface->FillRectangle(0, 0, width, height, 61, 49, 29);
            surface->DrawFrame(0, 0, width, height, 139, 112, 62);
            surface->DrawFrame(1, 1, width - 2, height - 2, 94, 77, 44);
            surface->FillRectangle(2, height - 3, width - 4, 1, 27, 22, 14);
            button->image = CreatePcx16(x, y, width, height, id, nullptr);
            if (button->image) button->image->SetPcx(surface);
        }
        button->caption = Label(x + 2, y + 2, width - 4, height - 4, text, id, eTextColor::REGULAR,
            NH3Dlg::Text::SMALL, eTextAlignment::MIDDLE_CENTER);
        button->selection = H3DlgFrame::Create(x, y, width, height, id, H3RGB565(238, 199, 96));
        if (button->selection) { AddItem(button->selection); button->selection->HideDeactivate(); }
        return button;
    }
    H3DlgText *Label(int x, int y, int width, int height, const std::string &text = "", int id = -1,
                     eTextColor color = eTextColor::REGULAR, const char *font = NH3Dlg::Text::SMALL,
                     eTextAlignment alignment = eTextAlignment::MIDDLE_LEFT)
    {
        return CreateText(x, y, width, height, text.c_str(), font, color, id, alignment);
    }
    H3DlgDef *Icon(int x, int y, int id, const char *name = NH3Dlg::Assets::ARTIFACT_DEF, int frame = 0)
    {
        auto *image = CreateDef(x, y, id, name, frame);
        return image;
    }
    void ShowArtifact(H3DlgDef *image, Item item)
    {
        bool valid = image && item.id >= 0 && item.id < H3ArtifactCount::Get();
        const auto *def = image ? image->GetDef() : nullptr;
        valid = valid && def && def->groupsCount > 0 && def->groups[0] && item.id < def->groups[0]->count;
        if (valid) image->SetFrame(item.id);
        Visible(image, valid);
        if (image) image->SetHints(ArtifactName(item).c_str(), nullptr, TRUE);
    }
    void Background(const H3MapItem *object)
    {
        auto *canvas = H3LoadedPcx16::Create(WIDTH, HEIGHT);
        if (!canvas) return;
        canvas->FillRectangle(0, 0, WIDTH, HEIGHT, 35, 30, 24);
        H3PcxLoader texture(NH3Dlg::Assets::DIBOXBACK);
        if (texture.Get() && texture->width > 0 && texture->height > 0)
            for (int y = 0; y < HEIGHT; y += texture->height)
                for (int x = 0; x < WIDTH; x += texture->width)
                    texture->DrawToPcx16(0, 0, (std::min)(texture->width, WIDTH - x),
                        (std::min)(texture->height, HEIGHT - y), canvas, x, y, FALSE);
        canvas->FillRectangle(18, 14, 108, 82, 27, 23, 18);
        canvas->DarkenArea(18, 176, 424, 300, 110);
        canvas->DarkenArea(456, 176, 306, 300, 100);
        canvas->DrawFrame(18, 14, 108, 82, 133, 103, 52);
        canvas->DrawFrame(18, 176, 424, 300, 133, 103, 52);
        canvas->DrawFrame(456, 176, 306, 300, 133, 103, 52);
        for (int i = 1; i < ROWS; ++i) canvas->FillRectangle(22, 176 + i * 60, 416, 1, 94, 75, 43);
        DrawObject(canvas, ObjectDef(object));
        H3DefLoader border(NH3Dlg::Assets::DLGBOX);
        if (border.Get()) canvas->FrameRegion(0, 0, WIDTH, HEIGHT, FALSE, 0, FALSE);
        AddBackground(canvas);
        canvas->Destroy();
    }
    std::string Hint(const Offer &offer) const
    {
        std::string text;
        for (int i = 0; i < offer.count; ++i)
        {
            if (i) text += ", ";
            text += ArtifactName(offer.sources[i].item);
        }
        if (!offer.Sale()) text += " -> " + ArtifactName(offer.result);
        return text + " | " + PriceText(offer.price);
    }
    void Refresh(bool resetPage = false)
    {
        if (resetPage) { page = 0; selected = -1; }
        host.RefreshCatalog();
        const auto state = host.ReadState();
        const auto options = host.ReadOptions();
        const auto inventory = host.ReadInventory();
        if (tab == 3 && !options.mithril) { tab = 0; page = 0; selected = -1; }
        offers = BuildOffers(inventory, host, options, state, CurrentService());
        batch = tab == 3 ? BuildSaleBatch(inventory, host, options, state, CurrentService()) : SaleBatch{};
        page = (std::min)(page, (std::max)(0, (int(offers.size()) - 1) / ROWS));
        if (selected < page * ROWS || selected >= (page + 1) * ROWS || selected >= int(offers.size()))
            selected = offers.empty() ? -1 : page * ROWS;
        for (int i = 0; i < 4; ++i)
        {
            if (tabs[i]) tabs[i]->SetFrame(i == tab ? 1 : 0);
            if (i == 3) Enabled(tabs[i], options.mithril);
        }
        Visible(goldButton, tab == 3); Visible(mithrilButton, tab == 3);
        if (goldButton) goldButton->SetFrame(goldCurrency ? 1 : 0);
        if (mithrilButton) mithrilButton->SetFrame(goldCurrency ? 0 : 1);
        const auto money = host.ReadWallet();
        if (wallet[0]) wallet[0]->SetText(std::to_string(money.gold).c_str());
        if (wallet[1]) wallet[1]->SetText(std::to_string(money.mithril).c_str());
        if (status) status->SetText(Ui(state.Orders() >= 2 ? (options.mithril ? "tired" : "tiredNoSales") : state.Orders() ? "secondOrder" : "orders",
            {"count", std::to_string(2 - state.Orders()), "movement", std::to_string(MOVEMENT_COST)}).c_str());
        if (count) count->SetText(Ui(tab == 3 ? "saleCount" : "offerCount", {"count", std::to_string(offers.size())}).c_str());
        if (empty) empty->SetText(Ui(tab == 3 ? "emptyBag" : state.Orders() >= 2 ?
            (options.mithril ? "noOrders" : "noOrdersNoSales") : "noOffers").c_str());
        Visible(empty, offers.empty());
        if (paging) paging->SetText(Ui("page", {"current", std::to_string(page + 1),
            "total", std::to_string((std::max)(1, (int(offers.size()) + ROWS - 1) / ROWS))}).c_str());
        Enabled(previous, page > 0); Enabled(next, (page + 1) * ROWS < int(offers.size()));
        for (int i = 0; i < ROWS; ++i)
        {
            auto &row = rows[i];
            const int index = page * ROWS + i;
            const bool visible = index < int(offers.size());
            for (H3DlgItem *item : {static_cast<H3DlgItem *>(row.button), static_cast<H3DlgItem *>(row.name),
                static_cast<H3DlgItem *>(row.slot), static_cast<H3DlgItem *>(row.price)}) Visible(item, visible);
            Visible(row.frame, visible && index == selected);
            if (row.frame) row.frame->DeActivate();
            if (!visible)
            {
                Visible(row.source, false); Visible(row.result, false); Visible(row.arrow, false);
                continue;
            }
            const auto &offer = offers[index];
            ShowArtifact(row.source, offer.sources[0].item);
            ShowArtifact(row.result, offer.Sale() ? Item{} : offer.result);
            Visible(row.arrow, !offer.Sale());
            if (row.name) row.name->SetText(ArtifactName(offer.Sale() ? offer.sources[0].item : offer.result).c_str());
            if (row.slot) row.slot->SetText(offer.service == Service::Combine ? Ui("combination").c_str() : SlotName(offer.sources[0].slot).c_str());
            if (row.price) row.price->SetText(PriceText(offer.price).c_str());
            const auto hint = Hint(offer);
            for (H3DlgItem *item : {static_cast<H3DlgItem *>(row.button), static_cast<H3DlgItem *>(row.name),
                static_cast<H3DlgItem *>(row.slot), static_cast<H3DlgItem *>(row.price)})
                if (item) item->SetHints(hint.c_str(), nullptr, TRUE);
        }
        RefreshDetails();
        Redraw();
    }
    void RefreshDetails()
    {
        const auto *offer = selected >= 0 && selected < int(offers.size()) ? &offers[selected] : nullptr;
        if (detailTitle) detailTitle->SetText(Ui(tab == 3 ? "selling" : "receiving").c_str());
        const Item result = offer ? (offer->Sale() ? offer->sources[0].item : offer->result) : Item{};
        ShowArtifact(resultImage, result);
        if (detailName) detailName->SetText(ArtifactName(result).c_str());
        if (partsTitle) partsTitle->SetText(offer ? Ui(offer->Sale() ? "saleRules" : "components").c_str() : "");
        Visible(batchSummary, tab == 3 && !batch.offers.empty());
        if (batchSummary) batchSummary->SetText(batch.valid ? Ui("batchSummary",
            {"count", std::to_string(batch.offers.size()), "price", PriceText(batch.price)}).c_str() : Ui("batchUnavailable").c_str());
        for (int i = 0; i < 6; ++i)
        {
            const bool visible = offer && !offer->Sale() && i < offer->count;
            ShowArtifact(parts[i].image, visible ? offer->sources[i].item : Item{});
            Visible(parts[i].name, visible);
            if (visible && parts[i].name)
            {
                parts[i].name->SetText(ArtifactName(offer->sources[i].item).c_str());
                parts[i].name->SetHints(SlotName(offer->sources[i].slot).c_str(), nullptr, TRUE);
            }
        }
        const Price price = offer ? offer->price : Price{};
        for (int i = 0; i < 2; ++i)
        {
            const int amount = i ? price.mithril : price.gold;
            Visible(resourceImages[i], offer && amount != 0);
            Visible(amounts[i], offer && amount != 0);
            if (amounts[i]) amounts[i]->SetText(std::to_string(std::abs(amount)).c_str());
        }
        static const char *actions[] = {"upgrade", "exchange", "combine", "sell"};
        if (apply) apply->SetText(Ui(actions[tab]).c_str());
        Enabled(apply, offer && host.ReadWallet().CanPay(offer->price));
        if (apply) apply->SetHints(Ui(offer && !host.ReadWallet().CanPay(offer->price) ? "cannotPay" :
            tab == 3 ? "saleHint" : "workHint").c_str(), nullptr, TRUE);
        Visible(sellAll, tab == 3);
        if (sellAll) sellAll->SetText(Ui("sellAll", {"count", std::to_string(batch.offers.size())}).c_str());
        Enabled(sellAll, batch.valid && host.ReadWallet().CanPay(batch.price));
        if (sellAll) sellAll->SetHints(Ui("sellAllHint", {"price", PriceText(batch.price)}).c_str(), nullptr, TRUE);
    }
    void Report(CommitResult result)
    {
        if (result != CommitResult::Success)
            H3Messagebox::Show(Text(result == CommitResult::NoResources ? "noResources" :
                result == CommitResult::RecoveryFailure ? "recoveryFailure" : "changed").c_str());
        if (result == CommitResult::RecoveryFailure) { fatal = true; Stop(); return; }
        Refresh();
        if (result == CommitResult::Success && GetHintBar()) GetHintBar()->ShowMessage(Ui("completed").c_str());
    }
    void Describe(Item item, int slot = -1)
    {
        if (item.id < 0 || item.id >= H3ArtifactCount::Get()) return;
        const H3Artifact artifact(item.id, item.modifier);
        const int saved = P_WindowManager->resultItemID;
        const auto *api = globalPatcher->VarGetValue<const ArtifactDescriptionApi::Api *>(ArtifactDescriptionApi::VARIABLE, nullptr);
        bool shown = false;
        if (api && api->magic == ArtifactDescriptionApi::MAGIC && api->size >= sizeof(*api) &&
            api->version == ArtifactDescriptionApi::VERSION && api->show)
        {
            const ArtifactDescriptionApi::Request request{sizeof(request), ArtifactDescriptionApi::VERSION, hero, &artifact, slot};
            shown = api->show(&request) != 0;
        }
        if (!shown)
        {
            H3String description;
            THISCALL_2(H3String *, 0x4DB650, &artifact, &description);
            H3Messagebox::RMB(description.String(), H3PictureCategories::Artifact(static_cast<eArtifact>(item.id)));
        }
        P_WindowManager->resultItemID = saved;
        Refresh();
    }
  public:
    ArtificerDialog(Host &host, H3Hero *hero, const H3MapItem *object)
        : H3Dlg(WIDTH, HEIGHT, -1, -1, FALSE, FALSE), host(host), hero(hero)
    {
        if (GetX() + WIDTH + 8 > H3GameWidth::Get() || GetY() + HEIGHT + 8 > H3GameHeight::Get()) flags &= ~0x10;
        Background(object);
        Label(140, 18, 320, 30, Text("name"), -1, eTextColor::GOLD, NH3Dlg::Text::BIG);
        Label(140, 50, 320, 22, Ui("hero", {"name", hero->name}), -1, eTextColor::REGULAR, NH3Dlg::Text::MEDIUM);
        status = Label(140, 76, 616, 24, "", -1, eTextColor::GOLD);
        for (int i = 0; i < 2; ++i)
        {
            auto *icon = Icon(482 + i * 140, 22, -1, NH3Dlg::Assets::RESOURCE_DEF, 0);
            if (icon)
            {
                const int frame = i ? 7 : 6;
                auto *def = icon->GetDef();
                if (def && def->groupsCount && def->groups[0] && frame < def->groups[0]->count) icon->SetFrame(frame);
                else icon->HideDeactivate();
            }
            wallet[i] = Label(518 + i * 140, 18, 102, 24, "", -1, eTextColor::GOLD, NH3Dlg::Text::MEDIUM);
            Label(518 + i * 140, 44, 102, 22, i ? Text("mithril") : P_ResourceName[eResource::GOLD]);
        }
        static const char *names[] = {"upgrades", "exchanges", "combinations", "sales"};
        for (int i = 0; i < 4; ++i) tabs[i] = Button(20 + i * 170, 110, 160, TAB_FIRST + i, Ui(names[i]));
        Button(706, 110, 54, HELP, "?");
        count = Label(22, 148, 416, 24, "", -1, eTextColor::GOLD);
        goldButton = Button(460, 144, 144, GOLD, P_ResourceName[eResource::GOLD]);
        mithrilButton = Button(614, 144, 146, MITHRIL, Text("mithril"));
        empty = Label(36, 198, 388, 250, "", -1, eTextColor::REGULAR, NH3Dlg::Text::MEDIUM, eTextAlignment::MIDDLE_CENTER);
        for (int i = 0; i < ROWS; ++i)
        {
            const int y = 177 + i * 60, id = ROW_FIRST + i;
            auto &row = rows[i];
            row.button = Label(20, y, 420, 58, "", id);
            row.source = Icon(27, y + 7, id);
            row.arrow = Label(72, y + 17, 14, 24, ">", id, eTextColor::GOLD);
            row.result = Icon(86, y + 7, id);
            row.name = Label(140, y + 2, 290, 32, "", id);
            row.slot = Label(140, y + 34, 140, 20, "", id);
            row.price = Label(282, y + 34, 148, 20, "", id, eTextColor::GOLD, NH3Dlg::Text::SMALL, eTextAlignment::MIDDLE_RIGHT);
            row.frame = H3DlgFrame::Create(20, y, 420, 58, id, H3RGB565(224, 184, 80));
            if (row.frame) { AddItem(row.frame); row.frame->DeActivate(); }
        }
        detailTitle = Label(470, 183, 276, 22, "", -1, eTextColor::GOLD);
        resultImage = Icon(480, 219, RESULT);
        detailName = Label(538, 208, 208, 62, "", RESULT, eTextColor::GOLD, NH3Dlg::Text::MEDIUM);
        partsTitle = Label(470, 274, 276, 38);
        batchSummary = Label(478, 326, 266, 106, "", -1, eTextColor::GOLD, NH3Dlg::Text::MEDIUM);
        for (int i = 0; i < 6; ++i)
        {
            const int x = 468 + i % 3 * 96, y = 316 + i / 3 * 76;
            parts[i].image = Icon(x + 24, y, PART_FIRST + i);
            parts[i].name = Label(x, y + 45, 92, 30, "", PART_FIRST + i, eTextColor::REGULAR,
                NH3Dlg::Text::SMALL, eTextAlignment::MIDDLE_CENTER);
        }
        previous = Button(20, 482, 40, PREVIOUS, "<");
        next = Button(400, 482, 40, NEXT, ">");
        paging = Label(68, 482, 324, 26, "", -1, eTextColor::REGULAR, NH3Dlg::Text::SMALL, eTextAlignment::MIDDLE_CENTER);
        for (int i = 0; i < 2; ++i)
        {
            resourceImages[i] = Icon(462 + i * 150, 477, -1, NH3Dlg::Assets::RESOURCE_DEF);
            if (resourceImages[i])
            {
                auto *def = resourceImages[i]->GetDef();
                const int frame = i ? 7 : 6;
                if (def && def->groupsCount && def->groups[0] && frame < def->groups[0]->count) resourceImages[i]->SetFrame(frame);
                else { resourceImages[i]->HideDeactivate(); resourceImages[i] = nullptr; }
            }
            amounts[i] = Label(498 + i * 150, 478, 110, 24, "", -1, eTextColor::GOLD, NH3Dlg::Text::MEDIUM);
        }
        Button(338, 512, 102, CLOSE, Ui("close"));
        apply = Button(460, 512, 144, APPLY, "");
        sellAll = Button(614, 512, 146, SELL_ALL, "");
        CreateHint(18, 542, 744, 14);
        Refresh();
    }
    BOOL OnLeftClick(INT id, H3Msg &) override
    {
        if (fatal) return FALSE;
        if ((id >= CLOSE && id < TAB_FIRST + 4) || (id >= ROW_FIRST && id < ROW_FIRST + ROWS))
            P_SoundMgr->ClickSound();
        if (id == CLOSE) { Stop(); return FALSE; }
        if (id == HELP) { H3Messagebox::Show(Ui("help").c_str()); Refresh(); return FALSE; }
        if (id >= TAB_FIRST && id < TAB_FIRST + 4)
        {
            if (id == TAB_FIRST + 3 && !host.ReadOptions().mithril) return FALSE;
            tab = id - TAB_FIRST; Refresh(true); return FALSE;
        }
        if (id == GOLD || id == MITHRIL) { goldCurrency = id == GOLD; Refresh(true); return FALSE; }
        if (id == PREVIOUS || id == NEXT)
        {
            page += id == NEXT ? 1 : -1; page = (std::max)(0, page); selected = -1; Refresh(); return FALSE;
        }
        if (id >= ROW_FIRST && id < ROW_FIRST + ROWS)
        {
            selected = page * ROWS + id - ROW_FIRST; Refresh(); return FALSE;
        }
        if (id == APPLY && selected >= 0 && selected < int(offers.size()))
        {
            const auto quote = offers[selected];
            Report(Commit(host, quote)); return FALSE;
        }
        if (id == SELL_ALL && tab == 3 && batch.valid)
        {
            const auto quote = batch;
            const auto message = Ui("confirmAll", {"count", std::to_string(quote.offers.size()), "price", PriceText(quote.price)});
            const int saved = P_WindowManager->resultItemID;
            const bool confirmed = H3Messagebox::Choice(message.c_str()) != 0;
            P_WindowManager->resultItemID = saved;
            if (confirmed) Report(CommitSaleBatch(host, quote)); else Refresh();
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
                if (offer.Sale() || item == rows[id - ROW_FIRST].source) Describe(offer.sources[0].item, offer.sources[0].slot);
                else Describe(offer.result);
            }
            return FALSE;
        }
        if (selected >= 0 && selected < int(offers.size()))
        {
            const auto &offer = offers[selected];
            if (id == RESULT) { Describe(offer.Sale() ? offer.sources[0].item : offer.result); return FALSE; }
            if (id >= PART_FIRST && id < PART_FIRST + offer.count)
            {
                const auto source = offer.sources[id - PART_FIRST]; Describe(source.item, source.slot); return FALSE;
            }
        }
        return TRUE;
    }
    BOOL OnMouseHover(H3DlgItem *item) override
    {
        if (GetHintBar()) GetHintBar()->ShowMessage(item && item->GetHint() ? item->GetHint() : Ui("hint").c_str());
        return TRUE;
    }
    BOOL OnMouseWheel(INT direction) override
    {
        page += direction > 0 ? -1 : 1;
        page = (std::max)(0, page); selected = -1; Refresh(); return FALSE;
    }
    BOOL OnKeyPress(eVKey key, eMsgFlag) override
    {
        if (key == eVKey::H3VK_ESCAPE) { Stop(); return FALSE; }
        if (key == eVKey::H3VK_ENTER && selected >= 0 && selected < int(offers.size()) &&
            host.ReadWallet().CanPay(offers[selected].price))
        {
            const auto quote = offers[selected]; Report(Commit(host, quote)); return FALSE;
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
} }
