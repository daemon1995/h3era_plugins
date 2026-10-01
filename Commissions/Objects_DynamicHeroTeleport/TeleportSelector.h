#pragma once

#include "TeleportConfig.h"

DllExport int __stdcall DisplayTeleportSelector(const DWORD *visitedObjectIndexes, const DWORD currentObjectIndex);

class TeleportSelector : public H3Dlg
{
    static constexpr int VARIANT_FIRST_ID = 100;
    static constexpr int CARD_X[2] = {26, 322};
    static constexpr int CARD_WIDTH = 250;
    static constexpr int CARD_HEIGHT = 72;
    static constexpr int CARD_FIRST_Y = 217;
    static constexpr int CARD_STEP_Y = 105;

    DWORD visitedObjectIndexes[mrart::TELEPORT_VARIANT_COUNT]{};
    DWORD currentObjectIndex;
    H3DlgFrame *selectionFrame = nullptr;
    std::vector<H3DlgTransparentItem *> hitAreas;

  public:
    int selectedVariant = -1;

  public:
    TeleportSelector(const DWORD *visitedObjectIndexes, const DWORD currentObjectIndex);

    virtual BOOL DialogProc(H3Msg &msg) override;
    virtual VOID OnOK() override;
    virtual VOID OnCancel() override;

  private:
    void CreateItems();
    void SelectVariant(const int variant);
    BOOL IsAvailable(const int variant) const noexcept;
};
