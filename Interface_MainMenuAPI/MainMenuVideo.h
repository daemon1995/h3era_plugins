#pragma once

namespace mainmenu
{
void InstallVideoPositioning();
namespace video
{
// Call after the native constructor, before adding plugin-owned dialog items.
void Track(H3BaseDlg *dlg);
void Release(H3BaseDlg *dlg);
void SetPanelBackground(H3BaseDlg *dlg, H3DlgPcx16 *item);
H3LoadedPcx16 *GetBackground(RECT &bounds);
void RestoreBackground();
}
}
