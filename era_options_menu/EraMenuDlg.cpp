#include "EraMenuDlg.h"

bool EraMenuDlg::NeedRestart() const { return need_restart; }

BOOL EraMenuDlg::DialogProc(H3Msg& msg)
{
	if (msg.IsLeftClick())
	{
		if (msg.itemId >= 1 && msg.itemId <= 8)
		{




		//	if (H3RGB565 rgb = this->GetFrame(msg.itemId)->GetColor())
		//	{
		//		BYTE r = rgb.GetRed8();
		//		BYTE g = rgb.GetGreen8();
		//		BYTE b = rgb.GetBlue8();

			//	this->background->FillRectangle(0 + 12, 0 + 12, this->widthDlg - 24, this->heightDlg - 24, r, g, b);
				//this->background->
			//	this->Redraw();
			//	H3DlgItem* fr = this->GetH3DlgItem(msg.itemId);
				//fr->Cast<H3DlgFrame>()->no
		//	}

		}
		else if (msg.itemId>=h3::eControlId::OK)
		{
			this->Stop();
		}
	}


	return 0;
}
 
BOOL EraMenuDlg::OnLeftClick(INT itemId, H3Msg& msg)
{
	if (msg.subtype == eMsgSubtype::LBUTTON_CLICK && itemId >= 110 && itemId <= 115)
	{
		need_restart = true;
		ratio = itemId - 110 +1;
		this->Stop();
		//this->widthDlg = itemi;
	}

	return 0;
}

BOOL EraMenuDlg::OnLeftClickOutside()
{
	this->Stop();	
	return 0;
}

BOOL EraMenuDlg::OnMouseWheel(INT32 direction)
{
	//this->GetScrollbar(40)->Refresh();
	return 0;
}

BOOL EraMenuDlg::OnNotify(H3DlgItem* it, H3Msg& msg)
{

	it->GetID();
	return 0;
}
