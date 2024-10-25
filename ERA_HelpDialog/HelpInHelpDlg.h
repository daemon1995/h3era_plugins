#pragma once
namespace help
{
	class HelpInHelpDlg :
		public H3Dlg
	{
	public:
		HelpInHelpDlg(const int width, const int height, const int x = -1, const int y = -1);
		virtual	~HelpInHelpDlg();

	private:
		BOOL DialogProc(H3Msg& msg) override;
	};

}
