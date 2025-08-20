#pragma once
namespace list
{
class ModListDlg : public H3Dlg
{
    h3::H3DlgHighlightable highlightTable;
  public:
    ModListDlg(const int width, const int height, const int x = -1, const int y = -1);
    virtual ~ModListDlg();

  protected:
    virtual BOOL DialogProc(H3Msg &msg) override;

  private:
    void CreateDlgItems();

  public:
    ModInformation *ResultMod() const noexcept;
};

} // namespace list
