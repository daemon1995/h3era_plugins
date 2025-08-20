//#pragma once
//#include "DlgEnums.h"
//class H3DlgFramedPanel : public H3DlgBasePanel
//{
//protected:
//    H3DlgFrame* panelFrame = nullptr;
//    H3DlgScrollbar* scrollBar = nullptr;
//
//public:
//    void CreateBorderFrame();
//    ~H3DlgFramedPanel();
//
//public:
//    const H3BaseDlg* Parent() const noexcept;
//    void AddScrollBar(H3DlgScrollbar* scrollBar) noexcept;
//    H3LoadedPcx16* GetBackgroundPcx() const noexcept;
//};
//
//class DlgPanel
//{
//protected:
//    H3DlgFramedPanel* dlgPanel = nullptr;
//    H3DlgScrollbar* scrollBar = nullptr;
//
//protected:
//    DlgPanel(const int x, const int y, const int width, const int height, const H3BaseDlg* parent);
//    //  DlgPanel(const DlgPanel &other);
//
//public:
//    virtual ~DlgPanel();
//
//protected:
//    void CreateItemsGrid(LPCSTR const defName, const int maxItems, H3DlgScrollbar_proc scrollBarProc = nullptr);
//
//public:
//    inline int GetX() const noexcept;
//    inline int GetY() const noexcept;
//    virtual void InitPanelItems() = 0;
//    // virtual void UpdateContent(const Content& content) =0;
//};
//
//class HeaderMenuPanel : public DlgPanel
//{
//
//    union {
//        struct
//        {
//            H3DlgCaptionButton* modListDlg;
//            H3DlgCaptionButton* hotkeys;
//            H3DlgCaptionButton* creatures;
//            H3DlgCaptionButton* artifacts;
//            H3DlgCaptionButton* towns;
//            H3DlgCaptionButton* resize;
//            H3DlgCaptionButton* help;
//        };
//        H3DlgCaptionButton* asArray[7] = {};
//
//    } buttons;
//
//    H3DlgCaptionButton* activeButton = nullptr;
//
//public:
//    HeaderMenuPanel(const int x, const int y, const int width, const int height, H3BaseDlg* parent);
//    virtual ~HeaderMenuPanel();
//
//private:
//    virtual void InitPanelItems() override;
//
//public:
//    // virtual void InitPanelItems() override;
//};
//
//class IGridPanel
//{
//    IGridPanel() = default;
//
//protected:
//    virtual void Sort() = 0;
//    virtual void RedrawCategoryItems(const int firstLineId = 0) = 0;
//};
//
//class ContentPanelSwitch
//{
//public:
//    H3DlgEdit* search = nullptr;
//    H3String searchStr;
//
//    int state = -1;
//    /*OVBUTN3.def*/
//    std::vector<H3DlgCaptionButton*> menuButtons;
//    H3DlgCaptionButton* activeButton = nullptr;
//};
//class GameInfoContentPanel : public DlgPanel
//{
//    ContentPanelSwitch menu;
//};
//
//class CategoriesPanel : public DlgPanel
//{
//    ModInformation* activeMod = nullptr;
//    struct PanelCategory
//    {
//
//        static constexpr int textIcoXOffset = 48;
//
//        const Category* category = nullptr;
//        H3DlgTextPcx* textPcx = nullptr;
//        // display current icon
//        H3DlgPcx16* icon = nullptr;
//
//    public:
//        static INT32 __fastcall Proc(H3Msg* msg);
//        void SetCategory(const Category* category);
//    };
//    std::vector<PanelCategory> panelCategories;
//
//public:
//    CategoriesPanel(const int x, const int y, const int width, const int height, const H3BaseDlg* parent);
//    virtual ~CategoriesPanel();
//
//private:
//    virtual void InitPanelItems() override;
//
//public:
//    void AssignMod(ModInformation* mod);
//    void RedrawCategoryItems(const int firstItemId = 0);
//};
//
//class ContentPanel : public DlgPanel
//{
//    H3DlgPcx16* backPcx = nullptr;
//
//public:
//    ContentPanel(const int x, const int y, const int width, const int height, const H3BaseDlg* parent);
//    //	ContentPanel(const DlgPanel& other);
//    virtual ~ContentPanel();
//
//private:
//    virtual void InitPanelItems() override;
//
//public:
//    //   virtual void UpdateContent(const Content& content) noexcept override;
//    // void H3d
//};
//class CreaturesContentPanel : public DlgPanel
//{
//
//private:
//    std::vector<H3DlgDef*> creatureProtraits;
//
//public:
//    CreaturesContentPanel(const int x, const int y, const int width, const int height, const H3BaseDlg* parent);
//    //		CreaturesContentPanel(const ContentPanel& other);
//    //   virtual ~CreaturesContentPanel();
//
//private:
//    virtual void InitPanelItems() override;
//
//private:
//    static void __fastcall ScrollProc(INT32 tick, H3BaseDlg* dlg);
//
//public:
//    //  virtual void UpdateContent(const Content& content) noexcept override;
//};
//
//class ArtifactsContentPanel : public DlgPanel
//{
//
//private:
//    std::vector<H3DlgDef*> creatureProtraits;
//
//public:
//    ArtifactsContentPanel(const int x, const int y, const int width, const int height, const H3BaseDlg* parent);
//    //		CreaturesContentPanel(const ContentPanel& other);
//private:
//    virtual void InitPanelItems() override;
//
//private:
//    static void __fastcall ScrollProc(INT32 tick, H3BaseDlg* dlg);
//
//public:
//    //   virtual void UpdateContent(const Content& content)  noexcept override;
//};
//
