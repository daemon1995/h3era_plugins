#pragma once
#include "DlgEnums.h"

namespace main
{
struct Content;
struct Category;
struct BaseMod;
struct Mod;
struct HotKeysCategory;

struct LastActiveDlgModInfo
{
    int modId = -1;
    int categoryId = -1;
    int scrollBarPos = 0;
    const Mod *mod = nullptr;
};

class H3DlgFramedPanel : public H3DlgBasePanel
{
  protected:
    H3DlgFrame *panelFrame = nullptr;
    H3DlgScrollbar *scrollBar = nullptr;

  public:
    virtual ~H3DlgFramedPanel();

  public:
    void CreateBorderFrame();
    const H3BaseDlg *Parent() const noexcept;
    void AddScrollBar(H3DlgScrollbar *scrollBar) noexcept;
    H3LoadedPcx16 *GetBackgroundPcx() const noexcept;
};

// Abstract class to create main dlg panels
class DlgPanel
{
  protected:
    H3DlgFramedPanel *dlgPanel = nullptr;
    // H3DlgScrollbar *scrollBar = nullptr;

  protected:
    DlgPanel(const int x, const int y, const int width, const int height, const H3BaseDlg *parent);
    //  DlgPanel(const DlgPanel &other);

  public:
    virtual ~DlgPanel();

  protected:
    void CreateItemsGrid(LPCSTR const defName, const int maxItems, H3DlgScrollbar_proc scrollBarProc = nullptr);

  public:
    inline int GetX() const noexcept;
    inline int GetY() const noexcept;
    virtual void InitPanelItems() = 0;
    // virtual void UpdateContent(const Content& content) =0;
};

class HeaderMenuPanel : public DlgPanel
{

    union {
        struct
        {
            H3DlgCaptionButton *modListDlg;
            H3DlgCaptionButton *hotkeys;
            H3DlgCaptionButton *creatures;
            H3DlgCaptionButton *artifacts;
            H3DlgCaptionButton *towns;
            H3DlgCaptionButton *resize;
            H3DlgCaptionButton *help;
        };
        H3DlgCaptionButton *asArray[7] = {};

    } buttons;

    H3DlgCaptionButton *activeButton = nullptr;

  public:
    HeaderMenuPanel(const int x, const int y, const int width, const int height, H3BaseDlg *parent);
    virtual ~HeaderMenuPanel();

  private:
    virtual void InitPanelItems() override;

  public:
    // virtual void InitPanelItems() override;
};

class IGridPanel
{
    IGridPanel() = default;

  protected:
    virtual void Sort() = 0;
    virtual void RedrawCategoryItems(const int firstLineId = 0) = 0;
};

class ContentPanelSwitch
{
  public:
    H3DlgEdit *search = nullptr;
    H3String searchStr;

    int state = -1;
    /*OVBUTN3.def*/
    std::vector<H3DlgCaptionButton *> menuButtons;
    H3DlgCaptionButton *activeButton = nullptr;
};
class GameInfoContentPanel : public DlgPanel
{
    ContentPanelSwitch menu;
};

class CategoriesPanel : public DlgPanel
{
    Mod *activeMod = nullptr;
    struct PanelCategory
    {

        static constexpr int textIcoXOffset = 48;

        const Category *category = nullptr;
        H3DlgTextPcx *textPcx = nullptr;
        // display current icon
        H3DlgPcx16 *icon = nullptr;

      public:
        static INT32 __fastcall Proc(H3Msg *msg);
        void SetCategory(const Category *category);
    };
    std::vector<PanelCategory> panelCategories;

  public:
    CategoriesPanel(const int x, const int y, const int width, const int height, const H3BaseDlg *parent);
    virtual ~CategoriesPanel();

  private:
    virtual void InitPanelItems() override;

  public:
    void AssignMod(Mod *mod);
    void RedrawCategoryItems(const int firstItemId = 0);
};

class ContentPanel : public DlgPanel
{
    H3DlgPcx16 *backPcx = nullptr;

  public:
    ContentPanel(const int x, const int y, const int width, const int height, const H3BaseDlg *parent);
    //	ContentPanel(const DlgPanel& other);
    virtual ~ContentPanel();

  private:
    virtual void InitPanelItems() override;

  public:
    //   virtual void UpdateContent(const Content& content) noexcept override;
    // void H3d
};
class CreaturesContentPanel : public DlgPanel
{

  private:
    std::vector<H3DlgDef *> creatureProtraits;

  public:
    CreaturesContentPanel(const int x, const int y, const int width, const int height, const H3BaseDlg *parent);
    //		CreaturesContentPanel(const ContentPanel& other);
    //   virtual ~CreaturesContentPanel();

  private:
    virtual void InitPanelItems() override;

  private:
    static void __fastcall ScrollProc(INT32 tick, H3BaseDlg *dlg);

  public:
    //  virtual void UpdateContent(const Content& content) noexcept override;
};

class ArtifactsContentPanel : public DlgPanel
{

  private:
    std::vector<H3DlgDef *> creatureProtraits;

  public:
    ArtifactsContentPanel(const int x, const int y, const int width, const int height, const H3BaseDlg *parent);
    //		CreaturesContentPanel(const ContentPanel& other);
  private:
    virtual void InitPanelItems() override;

  private:
    static void __fastcall ScrollProc(INT32 tick, H3BaseDlg *dlg);

  public:
    //   virtual void UpdateContent(const Content& content)  noexcept override;
};

struct BaseMod
{

  protected:
    static constexpr LPCSTR jsonBase = "help.mods.";
};

struct Mod : public BaseMod
{
  private:
    static LastActiveDlgModInfo lastActiveModInfo;

  public:
    // to check if some data is stored in the json
    BOOL hasSomeInfo;
    const UINT id;
    H3String name;
    H3String path;
    H3String json;
    int m_lastActiveCategoryId = -1;
    Category *activeCategory = nullptr;
    HotKeysCategory *hotkeysCategory = nullptr;
    std::vector<Category *> categories;

  public:
    Mod(LPCSTR modFolderName, const UINT id);
    virtual ~Mod();

  private:
    HotKeysCategory *CreateHotkeysCategory() const noexcept;
    Category *CreateNativeCategory(const int index) const noexcept;

  public:
    //	virtual void Activate();
    const Category &ActiveCategory() const noexcept;
    size_t Size() const noexcept;
    void GetInfo();
    void SetVisible(const BOOL state);
    void StoreModInfoAsActive() const noexcept;
    static const LastActiveDlgModInfo &GetLastActiveModInfo() noexcept;

    //		void DrawCategory(const int id) const noexcept;
};

class CreaturesMod : public Mod
{

    CreaturesContentPanel *panel;
};
class ArtifactsMod : public Mod
{
    ArtifactsContentPanel *panel;
};

struct Category
{
    H3LoadedPcx16 *iconPcx = nullptr;
    H3String name;

    Content *content = nullptr;
    //	const UINT id;
  public:
    //	Category(LPCSTR jsonKeyName/*, const UINT id*/);

    ~Category();

  public:
    const Category &Content() const noexcept;
    void ShowContent() const noexcept;

    // void
};

struct HotKey
{
    hkcategories::eType type;
    H3String combination;
    H3String description;
};

struct HotKeysCategory : public Category
{
    std::vector<HotKey> hotkeys;
};

struct Content
{
    // some hard/complex string
    H3String text;
};

class HelpDlg : public H3Dlg
{

    static HelpDlg *instance;
    BOOL needResize;
    BOOL isFullScreen = false;

    union {
        struct
        {
            HeaderMenuPanel *headerMenuPanel;
            CategoriesPanel *categoriesPanel;
            ContentPanel *contentPanel;
        };

        DlgPanel *asArray[3];

    } panels;
    CreaturesContentPanel *creaturesContentPanel;
    ArtifactsContentPanel *artifactsContentPanel;

    Mod *m_activeMod = nullptr;
    int lastActiveModId = -1;

    std::vector<HotKey *> hotkeys;
    std::vector<Mod *> mods;

  public:
    static constexpr LPCSTR iniPath = "ERA_HelpDialog.ini";

  public:
    HelpDlg(const int width, const int height, const int x = -1, const int y = -1);
    virtual ~HelpDlg();

  private:
    const Content *ActiveContent() const noexcept;

    void CallHelpInHelpDlg() const noexcept;
    void DisplayAllHotkeys() /*const*/ noexcept;

    Mod *CallModListDlg(const Mod *activeMod) const noexcept;

    BOOL GetLoadedModsJsonInformation(const std::vector<std::string> &modNames);

    void SetActiveMod(/*const */ Mod *mod);

    virtual BOOL DialogProc(H3Msg &msg) override;

  public:
    // void AssignWithCalledDlg(const buttons::eButton sourceButton = buttons::eButton::NONE,
    //     const int categoryId = 0) noexcept;
    void AssignWithCalledDlg(const H3Town *town = nullptr, const eCreature creature = eCreature::UNDEFINED) noexcept;
    // check if dlg exists
    static BOOL DlgExists();
    BOOL NeedResizeScreen() const noexcept;

  public:
};

} // namespace main
