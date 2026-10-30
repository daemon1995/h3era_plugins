#pragma once

namespace mapselection
{
LPCSTR Text(LPCSTR key, LPCSTR fallback);
bool InitHdMenu();
void InstallScenarioName();
void PreviewHero(int hero);
void UpdateMenuControls(h3::H3SelectScenarioDialog* dlg);
// All modal menus defer disruptive network messages back to the parent.
// A pending message remains owned by the native receive/dispatch path.
class MenuDialog : public h3::H3Dlg
{
  protected:
    h3::H3SelectScenarioDialog* parent;
    bool interrupted = false;
    bool changedByNetwork = false;
    bool receivedSelectionSnapshot = false;
    virtual INT vDialogProc(h3::H3Msg& msg) override;
    virtual void OnCancel() override { Stop(); }
    virtual void OnOK() override { Stop(); }
    virtual BOOL OnKeyPress(h3::eVKey key, h3::eMsgFlag) override
    {
        if (key == h3::eVKey::H3VK_ESCAPE) Stop();
        return TRUE;
    }
  public:
    MenuDialog(h3::H3SelectScenarioDialog* owner, int width, int height, bool makeBackground = true);
    virtual ~MenuDialog();
    bool WasInterrupted() const { return interrupted; }
};
}
