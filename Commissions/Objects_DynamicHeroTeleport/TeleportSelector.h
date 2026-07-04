#pragma once
#include "framework.h"
class TeleportSelector : public H3Dlg
{
    static constexpr int ITEMS_TOP_MARGIN = 200;
    struct TeleportItem
    {
        static constexpr int ITEM_WIDTH = 200;
        static constexpr int ITEM_HEIGHT = 70;

        size_t type;
        union {
            struct
            {
                H3DlgPcx16 *picture;
                H3DlgText *name;
                H3DlgFrame* frame;

            };
            H3DlgItem *asArray[3] = {};
        } items;
    };
    std::vector<TeleportItem> teleportItems;
    //H3DlgPcx16* selectionFra
  public:
    TeleportSelector() : TeleportSelector(COMMON_DLG_WIDTH, COMMON_DLG_HEIGHT, -1, -1) {};
    TeleportSelector(int width, int height, int x, int y);
    virtual ~TeleportSelector();

  protected:
    //  virtual BOOL OnCreate() override;
      virtual BOOL DialogProc(H3Msg& msg) override;
    //  virtual BOOL OnLeftClick(INT itemId, H3Msg& msg) override;
  protected:
    void CreateItems();
};
