#pragma once

#include "TeleportConfig.h"

class TeleportDlg : public H3Dlg
{
    static constexpr int MAP_X = 56;
    static constexpr int MAP_Y = 94;
    static constexpr int MAP_SIZE = 480;
    static constexpr int QUADRANT_FIRST_ID = 200;

    struct QuadrantPanel
    {
        H3DlgPcx16 *picture = nullptr;
        H3DlgTransparentItem *hitArea = nullptr;
    };

    const int variant;
    const DWORD currentObjectIndex;
    mrart::TeleportVariantConfig config;
    H3DlgFrame *selectionFrame = nullptr;
    H3DlgPcx16 *minimap = nullptr;
    std::vector<QuadrantPanel> panels;

  public:
    int selectedQuadrant = -1;

  public:
    TeleportDlg(const int variant, const DWORD currentObjectIndex);

    virtual BOOL DialogProc(H3Msg &msg) override;
    virtual VOID OnOK() override;
    virtual VOID OnCancel() override;

  private:
    void CreateDialogItems();
    void SelectQuadrant(const int quadrant);
};
