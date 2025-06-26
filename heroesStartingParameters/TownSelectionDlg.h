#pragma once

namespace dlg
{

struct DlgData
{
    int rows = 0;
    int columns = 0;
    RECT rect = {0, 0, 0, 0};
    static constexpr int padding = 3;
    static constexpr int margin = 18;
    static constexpr int distance = 8;
};
class TownSelectionDlg : public h3::H3Dlg
{
    static constexpr int TOWN_PANEL_FIRST_ITEM_ID = 20;
    static constexpr int HERO_PANEL_FIRST_ITEM_ID = 120;

  public:
    struct GridPanel
    {
        int firstItemId = 0;
        int lastItemId = 0;
        int selectedIndex = -2;
        const DlgData *data = nullptr;

        H3DlgBasePanel *panel = nullptr;
        H3DlgFrame *frame = nullptr;
        H3DlgPcx16 *backgroundWidget = nullptr;

      public:

        ~GridPanel();
    } townPanel, heroPanel;
    BOOL heroClicked = false;
    H3ScenarioPlayer &player;
    const H3SelectScenarioDialog *parent;

    const H3LoadedDef *itpa = nullptr;

  public:
    TownSelectionDlg(const int width, const DlgData &townsRect, const DlgData &heroesRect, int x, int y,
                     const H3SelectScenarioDialog *parent, H3ScenarioPlayer &player);
    virtual ~TownSelectionDlg();

    // virtual methods
  private:
    // virtual BOOL OnCreate() override;
    virtual BOOL DialogProc(H3Msg &msg) override;

  private:
    void SetFrameAt(GridPanel &panel, const int itemId);

    void SetFactionPortraits(const int faction, const int selectedIndex, const BOOL redraw = true);
    BOOL CreateBlueGridPanel(GridPanel &data);

  public:
    int SelectedFaction() const noexcept;
    int SelectedHero() const noexcept;
    static void ShowStaringHeroDlg(const int heroId, const int x = -1, const int y = -1);
    void CreateTownPanel(const int x, const int y, const int width, const int height);
    void CreateHeroPanel(const int x, const int y, const int width, const int height);
};

} // namespace dlg
