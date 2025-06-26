
#include "framework.h"

namespace dlg
{
TownSelectionDlg::TownSelectionDlg(const int width, const DlgData &townsRect, const DlgData &heroesRect, int x, int y,
                                   const H3SelectScenarioDialog *parent, H3ScenarioPlayer &playerId)
    : H3Dlg(width, townsRect.rect.bottom, x, y, 0, 0, IntAt(0x69CCF4)), player(playerId), parent(parent)
{

    flags ^= 0x10;
    townPanel.data = &townsRect;
    heroPanel.data = &heroesRect;

    itpa = parent->itpaDef;
    // CreateDoubleBackground();
    this->CreateTownPanel(0, 0, 0, 0);
    this->CreateHeroPanel(0, 0, 0, 0);
    SetFactionPortraits(player.town, player.selectedHeroIndex);

    // AddBackground(0, 0, width, height, 1, 0, IntAt(0x69CCF4), 1);
}

inline int GetHeroId(H3ScenarioPlayer &player, const int heroIndex)
{
    return *reinterpret_cast<UINT8 *>(reinterpret_cast<UINT8 *>(&player.heroes) + heroIndex);
}
BOOL TownSelectionDlg::DialogProc(H3Msg &msg)
{
    if (msg.IsLeftClick() && heroClicked || msg.ClickOutside())
    {
        Stop();

        return 1;
    }
    if (msg.IsLeftDown())
    {
        if (msg.itemId >= townPanel.firstItemId && msg.itemId <= townPanel.lastItemId)
        {
            const int factionId = msg.itemId - townPanel.firstItemId - 1;
            if (factionId != player.town)
            {
                SetFactionPortraits(factionId, -1);
            }
            return 1;
        }
        else if (msg.itemId >= heroPanel.firstItemId && msg.itemId <= heroPanel.lastItemId)
        {
            const int heroIndex = msg.itemId - heroPanel.firstItemId - 1;
            SetFactionPortraits(townPanel.selectedIndex, heroIndex);
            heroClicked = true;
            return 1;
        }
    }

    if (msg.IsRightClick())
    {
        if (msg.itemId >= townPanel.firstItemId && msg.itemId <= townPanel.lastItemId)
        {
            const int factionId = msg.itemId - townPanel.firstItemId - 1;
            return 1;
        }
        else if (msg.itemId >= heroPanel.firstItemId && msg.itemId <= heroPanel.lastItemId)
        {
            const int heroIndex = msg.itemId - heroPanel.firstItemId - 1;
            if (heroIndex >= 0 && heroIndex < MAX_HEROES_PER_TOWN)
            {
                ShowStaringHeroDlg(GetHeroId(player, heroIndex));
            }
            return 1;
        }
    }

    return 0;
}

TownSelectionDlg::~TownSelectionDlg()
{
    // if (townPanel.backgroundWidget)
    //{
    //     if (auto back = townPanel.backgroundWidget->GetPcx())
    //     {
    //         back->Destroy();
    //         townPanel.backgroundWidget->SetPcx(nullptr);
    //     }
    // }
    // if (heroPanel.backgroundWidget)
    //{
    //     if (auto back = heroPanel.backgroundWidget->GetPcx())
    //     {
    //         back->Destroy();
    //         heroPanel.backgroundWidget->SetPcx(nullptr);
    //     }
    // }
    // if (heroPanel.panel)
    //{
    //     // delete heroPanel.panel;
    //     THISCALL_1(void, 0x0451470, heroPanel.panel);
    //     heroPanel.panel = nullptr;
    // }
    // if (townPanel.panel)
    //{
    //     // delete townPanel.panel;
    //     THISCALL_1(void, 0x0451470, townPanel.panel);
    //     townPanel.panel = nullptr;
    // }
}
int TownSelectionDlg::SelectedFaction() const noexcept
{
    return townPanel.selectedIndex;
}
int TownSelectionDlg::SelectedHero() const noexcept
{
    return heroPanel.selectedIndex;
}

void TownSelectionDlg::ShowStaringHeroDlg(const int heroId, const int x, const int y)
{

    if (heroId >= 0 && heroId < limits::TOTAL_HEROES)
    {
        H3PcxLoader back = H3LoadedPcx::Load("HStInf.pcx");
        if (back)
        {

            struct _HeroType_
            {
                int Belong2Town;
                char *TypeName;
                int Agression;
                char PSkillStart[4];
                char ProbPSkillToLvl9[4];
                char ProbPSkillAfterLvl10[4];
                char ProbSSkill[28];
                char ProbInTown[9];
                char field_3D[3];
            };

            const auto &typesTable = reinterpret_cast<_HeroType_ *>(0x067D868);
            volatile int itemId = 0;
            const int width = back->width;
            const int height = back->height;
            const auto &heroInfo = P_HeroInfo[heroId];
            const auto &specInfo = P_HeroSpecialty[heroId];
            const auto &typeInfo = typesTable[heroInfo.heroClass];
            H3Dlg dlg(width, height, x, y, 0, 0, IntAt(0x69CCF4));
            auto backWidget = dlg.CreatePcx(0, 0, itemId++, back->GetName());

            // create portrait

            dlg.CreatePcx(0, 0, itemId++, heroInfo.largePortrait);
            dlg.CreateText(85, 22, 200, 32, heroInfo.name, NH3Dlg::Text::BIG, eTextColor::GOLD, itemId++);
            dlg.CreateText(85, 52, 200, 32, typeInfo.TypeName, NH3Dlg::Text::MEDIUM, 1, itemId++);

            H3DefLoader skillsDef = H3LoadedDef::Load("PSKil42.def");
            const int frameIds[4] = {0, 1, 2, 5};
            // primary skills
            for (size_t i = 0; i < 4; i++)
            {
                const int x = 31 + i * (29 + 42);
                const int y = 111;
                constexpr int textWidth = 71;
                dlg.CreateText(textWidth * i + 16, 90, textWidth, 16, P_PrimarySkillName[i], NH3Dlg::Text::SMALL,
                               eTextColor::REGULAR, itemId++);

                dlg.CreateDef(x, y, itemId++, skillsDef->GetName(), frameIds[i]);

                libc::sprintf(h3_TextBuffer, "%d", typeInfo.PSkillStart[i]);
                dlg.CreateText(textWidth * i + 16, 156, textWidth, 16, h3_TextBuffer, NH3Dlg::Text::SMALL,
                               eTextColor::REGULAR, itemId++);
            }

            // specialization
            H3DefLoader specDef = H3LoadedDef::Load("un44.def");
            const int frameId = 3;
            const int x = 31 + 4 * (29 + 42);
            const int y = 111;
            dlg.CreateDef(477, 432, itemId++, specDef->GetName(), heroId);
            if (heroInfo.hasSpellbook)
            {
            }
            //   backWidget->SetPcx(back);
            //  H3DlgPcx =
            dlg.RMB_Show();
        }
    }
}

void TownSelectionDlg::CreateTownPanel(const int x, const int y, const int width, const int height)
{
    if (CreateBlueGridPanel(townPanel))
    {
        const auto &data = townPanel.data;

        const int itemWidth = itpa->widthDEF;
        const int itemHeight = itpa->heightDEF;

        auto &panel = townPanel.panel;

        const int framesNum = itpa->groups[0]->count;
        townPanel.firstItemId = TOWN_PANEL_FIRST_ITEM_ID;

        auto randomTownDlgPcx = H3DlgPcx::Create(data->margin, data->margin, townPanel.firstItemId, "HPSRAND0.PCX");
        panel->AddItem(randomTownDlgPcx);

        int townsToDraw = (framesNum - 3) >> 2;

        int itemId = townPanel.firstItemId + 1;

        for (size_t row = 1; row < data->rows && townsToDraw; row++)
        {
            const int _y = data->margin + row * (itemHeight + data->padding);
            for (size_t col = 0; col < data->columns && townsToDraw; col++)
            {
                const int _x = data->margin + col * (itemWidth + data->padding);
                const int frameId = ((row * data->columns + col) - 2) << 1;
                auto townDlgDef = H3DlgDef::Create(_x, _y, itemId++, itpa->GetName(), frameId);
                panel->AddItem(townDlgDef);
                --townsToDraw;
            }
        }
        townPanel.lastItemId = itemId - 1;

        // townPanel.Show();
        panel->Show();
    }

    return;
}

void TownSelectionDlg::CreateHeroPanel(const int x, const int y, const int width, const int height)
{
    if (CreateBlueGridPanel(heroPanel))
    {
        const auto &data = heroPanel.data;

        const int itemWidth = itpa->widthDEF;
        const int itemHeight = itpa->heightDEF;

        auto &panel = heroPanel.panel;
        auto randomHeroDlgPcx = H3DlgPcx::Create(data->margin, data->margin, HERO_PANEL_FIRST_ITEM_ID, "HPSRAND1.PCX");
        panel->AddItem(randomHeroDlgPcx);
        heroPanel.firstItemId = HERO_PANEL_FIRST_ITEM_ID;
        int itemId = HERO_PANEL_FIRST_ITEM_ID + 1;
        int itemsToDraw = MAX_HEROES_PER_TOWN;

        for (size_t row = 1; row < data->rows && itemsToDraw; row++)
        {
            const int _y = data->margin + row * (itemHeight + data->padding);
            for (size_t col = 0; col < data->columns && itemsToDraw; col++)
            {
                const int _x = data->margin + col * (itemWidth + data->padding);
                auto heroDlgPcx = H3DlgPcx::Create(_x, _y, itemId++, "HPSRAND1.PCX");
                panel->AddItem(heroDlgPcx);
                --itemsToDraw;
            }
        }
        heroPanel.lastItemId = itemId - 1;

        //        heroPanel.Show();
        panel->Show();
    }

    return;
}

BOOL TownSelectionDlg::CreateBlueGridPanel(GridPanel &gridPanel)
{
    const auto &data = gridPanel.data;

    const int x = data->rect.left;
    const int y = data->rect.top;
    const int width = data->rect.right - data->rect.left;
    const int height = data->rect.bottom - data->rect.top;

    // create panel item
    H3DlgBasePanel *panel = H3ObjectAllocator<H3DlgBasePanel>().allocate(1);
    panel = THISCALL_6(H3DlgBasePanel *, 0x05AA6D0, panel, x, y, width, height, this);

    // create blue background
    // static int _id = 1;
    auto backgroundWidget = H3DlgPcx16::Create(0, 0, nullptr);
    backgroundWidget->SetWidth(width);
    backgroundWidget->SetHeight(height);

    H3LoadedPcx16 *pcx = H3LoadedPcx16::Create(width, height);
    pcx->BackgroundRegion(0, 0, width, height, true);
    // add border frame
    pcx->FrameRegion(0, 0, width, height, false, 0, true);

    panel->AddItem(backgroundWidget);
    //  backgroundWidget->ShowActivate();
    backgroundWidget->SetPcx(pcx);

    // create grid
    const int itemWidth = itpa->widthDEF;
    const int itemHeight = itpa->heightDEF;
    for (size_t row = 0; row < data->rows; row++)
    {
        const int _y = data->margin + row * (itemHeight + data->padding);
        for (size_t col = 0; col < data->columns; col++)
        {
            const int _x = data->margin + col * (itemWidth + data->padding);
            pcx->SinkArea(_x, _y, itemWidth, itemHeight);
        }
    }

    // create selection frame
    const H3RGB565 color(H3RGB888::Highlight());

    auto frame = H3DlgFrame::Create(0, 0, itemWidth + 2, itemHeight + 2, color);
    panel->AddItem(frame);
    // frame->ShowActivate();

    gridPanel.frame = frame;
    gridPanel.backgroundWidget = backgroundWidget;
    gridPanel.panel = panel;
    return TRUE;
}

void TownSelectionDlg::SetFactionPortraits(const int faction, const int heroIndex, const BOOL redraw)
{

    if (townPanel.selectedIndex != faction)
    {
        townPanel.selectedIndex = faction < 0 ? -1 : faction;

        player.town = townPanel.selectedIndex;
        heroPanel.selectedIndex = heroIndex;

        SetFrameAt(townPanel, TOWN_PANEL_FIRST_ITEM_ID + faction + 1);
        //  SetFrameAt(heroPanel, heroPanel.firstItemId + heroPanel.selectedIndex);

        const int firstPortraitIndex = heroPanel.firstItemId + 1;

        //        THISCALL_1(void, 0x0584F10, parent);

        // fill heroes list
        THISCALL_1(void, 0x0583B60, parent);

        const auto &portraits = parent->heroSmallPortraitsPcx;
        for (size_t i = firstPortraitIndex; i <= heroPanel.lastItemId; i++)
        {
            if (auto portraitPcx = GetPcx(i))
            {
                const int heroIndex = i - firstPortraitIndex;
                const int heroId = GetHeroId(player, heroIndex);

                BOOL heroIsUsed = false;
                for (size_t i = 0; i < limits::PLAYERS; i++)
                {
                    const auto &otherPlayer = parent->mapPlayersHuman[i];
                    if (&otherPlayer != &player)
                    {
                        if (otherPlayer.town == player.town && otherPlayer.selectedHeroIndex == heroIndex)
                        {
                            heroIsUsed = true;
                            break;
                        }
                    }
                }

                if (heroIndex < player.heroesCount && !heroIsUsed && faction >= 0)
                {
                    portraitPcx->ShowActivate();
                    portraitPcx->SetPcx(portraits[heroId]);
                }
                else
                {
                    portraitPcx->HideDeactivate();
                }
            }
        }
    }
    else
    {

        player.selectedHeroIndex = heroIndex;
        heroPanel.selectedIndex = heroIndex;
    }
    SetFrameAt(heroPanel, heroPanel.firstItemId + heroPanel.selectedIndex + 1);

    if (redraw)
        Redraw();
}

void TownSelectionDlg::SetFrameAt(GridPanel &panel, const int itemId)
{
    if (auto dlgDef = this->GetH3DlgItem(itemId))
    {
        panel.frame->SetX(dlgDef->GetX() - 1);
        panel.frame->SetY(dlgDef->GetY() - 1);
    }
}

TownSelectionDlg::GridPanel::~GridPanel()
{
    // destroy allocated background
    if (backgroundWidget)
    {
        if (auto back = backgroundWidget->GetPcx())
        {
            back->Destroy();
            backgroundWidget->SetPcx(nullptr);
        }
    }

    // destroy panel
    if (panel)
    {
        panel->Hide();
        // delete panel;
        THISCALL_1(void, 0x05AA720, panel);
        panel = nullptr;
    }
}

} // namespace dlg
