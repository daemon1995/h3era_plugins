#pragma once
namespace list
{
	class ModListDlg:
		public H3Dlg
	{
	public:
		ModListDlg(const int width, const int height, const int x = -1, const int y = -1);
		virtual	~ModListDlg();



	private:
		BOOL DialogProc(H3Msg& msg) override;


	public:
		main::Mod* ResultMod() const noexcept;
	};

}
