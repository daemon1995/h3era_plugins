#pragma once
namespace help
{
class GuideDlg : public H3Dlg
{
  public:
    GuideDlg(const int width, const int height, const int x = -1, const int y = -1);
    virtual ~GuideDlg();

  private:
    virtual BOOL DialogProc(H3Msg &msg) override;
};

} // namespace help
