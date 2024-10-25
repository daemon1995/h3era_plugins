#pragma once
namespace main
{
	class HelpDlg
		:public H3Dlg
	{


	public:
		HelpDlg(const int width, const int height, const int x = -1, const int y = -1);
		virtual	~HelpDlg();

	private:
		void CallHelpInHelpDlg() const noexcept;
		BOOL DialogProc(H3Msg& msg) override;
	public:

		static HelpDlg* instance;
	};

}