#pragma once

#include "HelpDialogDependencies.h"

#include <algorithm>

namespace helpdlg
{

// H3API keeps the native range setter protected. This adapter calls the
// virtual setter without relying on the scrollbar's internal field layout.
class AdjustableScrollbar : public H3DlgScrollbar
{
  public:
    void SetTickCount(const INT32 tickCount) const
    {
        vSetTickCount(tickCount);
    }
};

// A native scrollbar stores the number of positions, not the last position.
// As in RMG_SettingsDlg, update the range only while scrolling is possible;
// H3DlgScrollbar::SetButtonPosition divides by (tickCount - 1).
inline INT32 UpdateScrollbar(H3DlgScrollbar *scrollBar, const INT32 maxTick, const INT32 requestedTick,
                             const BOOL visible) noexcept
{
    const INT32 safeMaxTick = std::max<INT32>(0, maxTick);
    const INT32 safeTick = std::max<INT32>(0, std::min(requestedTick, safeMaxTick));
    if (!scrollBar)
        return safeTick;

    if (safeMaxTick > 0)
    {
        const INT32 tickCount = safeMaxTick + 1;
        // vSetTickCount mutates the native control's scrolling state. Calling
        // it again from the callback of an ordinary tick change prevents the
        // control from settling on tick 0. RMG_SettingsDlg only changes the
        // range when its filtered data set changes, so do the same here.
        if (scrollBar->GetTicksCount() != tickCount)
            reinterpret_cast<AdjustableScrollbar *>(scrollBar)->SetTickCount(tickCount);
        scrollBar->SetTick(safeTick);
        if (visible)
        {
            scrollBar->SetButtonPosition();
            scrollBar->ShowActivate();
        }
        else
            scrollBar->HideDeactivate();
    }
    else
    {
        scrollBar->SetTick(0);
        scrollBar->HideDeactivate();
    }

    return safeTick;
}

} // namespace helpdlg
