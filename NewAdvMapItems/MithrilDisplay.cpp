#include "pch.h"
#include "MithrilDisplay.h"


MithrilDisplay* MithrilDisplay::mithrilBar;


MithrilDisplay::MithrilDisplay(PatcherInstance* _PI, const char * hint)
	:dlgTextId(1016), defName("SMALmith.def"),
	advMapHintControl(nullptr), kingdomHintcontrol(nullptr), popupText(hint)

{

	mithrilBar = this;
	setHooks(_PI);
}


_LHF_(BuildResBar)
{
	H3Messagebox();

	return EXEC_DEFAULT;
}
void MithrilDisplay::setHooks(PatcherInstance* _PI)
{

	_PI->WriteLoHook(0x4021B2, MithrilDisplay::OnAdvMgrDlgResBarCreate); // H3AdventureMgrDlg
	_PI->WriteLoHook(0x51F042, MithrilDisplay::OnKingdomOverviewDlgResBarCreate); // KingdomOverviewDlg
	_PI->WriteLoHook(0x408945, MithrilDisplay::OnAdvMgrDlgRightClick); // H3AdventureMgrDlg

	
	_PI->WriteLoHook(0x403F00, MithrilDisplay::OnResourceBarDlgUpdate);
	//_PI->WriteLoHook(0x417380, MithrilDisplay::OnResourceBarDlgUpdate);
	_PI->WriteLoHook(0x559270, MithrilDisplay::BeforeHotseatMsgBox);
	_PI->WriteHiHook(0x521E20, THISCALL_, MithrilDisplay::KingdomOverviewDlgProc);


	_PI->WriteLoHook(0x558E16, BuildResBar);

	
}


int __stdcall MithrilDisplay::KingdomOverviewDlgProc(HiHook* h, H3BaseDlg* dlg, H3Msg* msg)
{


	if (msg->subtype== eMsgSubtype::RBUTTON_DOWN)
	{
		mithrilBar->showMithrilDescription(msg, mithrilBar->kingdomHintcontrol);
	}
	else if (msg->command == eMsgCommand::MOUSE_OVER)
	{
		H3DlgItem* hintZone = mithrilBar->kingdomHintcontrol;
		int x = msg->GetX();
		int y = msg->GetY();
		bool result = x >= hintZone->GetAbsoluteX() && x <= hintZone->GetAbsoluteX() + hintZone->GetWidth() && y >= hintZone->GetAbsoluteY() && y <= hintZone->GetAbsoluteY() + hintZone->GetHeight();
		if (result)
		{
		//	mithril->lastHintIsMitril = true;

			auto it = dlg->GetText(37);
			if (it)
			{

				H3TextFile** resNames = reinterpret_cast<H3TextFile**>(0x6A5390); // get resourec name
				it->SetText((*resNames)->GetText(8));
				it->Draw();
				it->Refresh();
			}
		}
		else if (x >= hintZone->GetAbsoluteX() + hintZone->GetWidth())
		{
			auto it = dlg->GetText(37);
			if (it)
			{
				it->SetText("");
				it->Draw();
				it->Refresh();
			}
		}
	}


	THISCALL_2(int, h->GetDefaultFunc(), dlg, msg);
	return 0;
}

bool MithrilDisplay::showMithrilDescription(H3Msg* msg, H3DlgItem* hintZone)
{
	int x = msg->GetX();
	int y = msg->GetY();

	bool result = x >= hintZone->GetAbsoluteX() && x <= hintZone->GetAbsoluteX() + hintZone->GetWidth() && y >= hintZone->GetAbsoluteY() && y <= hintZone->GetAbsoluteY() + hintZone->GetHeight();

	if (result)
	{
		int width = 0;
		FASTCALL_3(void, 0x4F6930, MithrilDisplay::text(), &width, &msg);
		FASTCALL_12(void, 0x4F6C00, MithrilDisplay::text(), 4, 592 - width, (0x258 - (int)msg) / 2 - 10, -1, 0, -1, 0, -1, 0, -1, 0);
	}
	else if (msg->itemId == 1008 && x < hintZone->GetAbsoluteX())
		result = true; // prevent extra hint for the date hint

	return result;
}



_LHF_(MithrilDisplay::OnAdvMgrDlgRightClick)
{

	bool ret = EXEC_DEFAULT;
	H3Msg* msg = reinterpret_cast<H3Msg*>(c->edi);

	if (mithrilBar->showMithrilDescription(msg, mithrilBar->advMapHintControl))
	{
		c->ebx = true;
		c->return_address = 0x408974;
		ret = NO_EXEC_DEFAULT;
	}

	return ret;
}



const char* MithrilDisplay::text() noexcept
{
	return mithrilBar->popupText;
}

int MithrilDisplay::getMithrilAmout(int playerId)
{
	return IntAt(0x27F9A00 + playerId * 4); // mb later it should be replaced 
}

MithrilDisplay* MithrilDisplay::Get()
{
		return mithrilBar;
}


BOOL8 MithrilDisplay::buildMithril(H3ResourceBarPanel* resourceBarPanel, int xOffset, bool buildFrame)
{

	if (resourceBarPanel)
	{

		mithrilBar = MithrilDisplay::Get();

		H3SEHandler seh;// exception handler - try create items
		try
		{

			int spotWidth = resourceBarPanel->resourceText[6]->GetWidth();
			int span = resourceBarPanel->resourceText[6]->GetX() - resourceBarPanel->resourceText[5]->GetX() - spotWidth;
			int xPos = resourceBarPanel->resourceText[6]->GetX() + spotWidth + span + 12 - resourceBarPanel->resourceText[0]->GetX();
			int yPos = resourceBarPanel->resourceText[6]->GetY();

			for (int i = 0; i < 7; ++i)
			{
				resourceBarPanel->resourceText[i]->SetX(resourceBarPanel->resourceText[i]->GetX() - 8);
				resourceBarPanel->resourceText[i]->SetWidth(88);
			}
			//	H3Messagebox(H3String::Format("%d", resourceBarPanel->resourceText[1]->GetY()));


			H3DlgItem* mithrilDef = H3DlgDef::Create(xPos+2, 4,1019, mithrilBar->defName);


			int textWidth = 50;
			if (buildFrame)
			{
				auto resBarPcx = resourceBarPanel->resbarPCX->GetPcx();
				int maxPicWidth = 80;
				int mapWidth = IntAt(0x4196EA + 1);


				int defWidth = mithrilDef->GetWidth();
				int textX = xPos + defWidth;
				textWidth = textX  + maxPicWidth;
				if (textWidth> mapWidth)
					textWidth = mapWidth;

				textWidth -= textX;
				if (textWidth >50)
					textWidth = 50;

				const int pcxWidth = textWidth + defWidth +8;
				const int pcxHeight = 20;

				H3LoadedPcx* mithrilBackPcx = H3LoadedPcx::Create(h3_NullString, pcxWidth, pcxHeight);
				
				H3DlgPcx* mithrilBack = H3DlgPcx::Create(xPos, 0, pcxWidth, pcxHeight, 1025, nullptr);

				resBarPcx->DrawToPcx(4, 0, pcxWidth, pcxHeight, mithrilBackPcx);
				resBarPcx->DrawToPcx(28, 0, 20, pcxHeight, mithrilBackPcx,2);
				//resBarPcx->DrawToPcx(xPos -5 -( pcxWidth >> 1), 0, pcxWidth >> 1, pcxHeight, mithrilBackPcx, pcxWidth>>1, 0);

			///	resBarPcx->DrawToPcx(defWidth - 5, 2, spotWidth, 17, mithrilBackPcx, xPos, 2);
				//H3Messagebox(H3String::Format("%d", pcxWidth));
				mithrilBack->SetPcx(mithrilBackPcx);
				resourceBarPanel->AddItem(mithrilBack); mithrilBack->Show();


			}
			resourceBarPanel->AddItem(mithrilDef); mithrilDef->Show();


			//H3DlgTextPcx

			int mithrilNumber = mithrilBar->getMithrilAmout();

			H3DlgText* mithriText = H3DlgText::Create(xPos + 29, 3, textWidth, 18, H3String::Format("%d", mithrilNumber).String(), NH3Dlg::Text::SMALL, eTextColor::WHITE, mithrilBar->dlgTextId, eTextAlignment::HLEFT);
			resourceBarPanel->AddItem(mithriText); mithriText->Show();

			if (buildFrame)
				mithrilBar->advMapHintControl = mithriText;
			else
				mithrilBar->kingdomHintcontrol = mithriText;

		}
		catch (const H3Exception& e)
		{

			e.ShowInGame();
			H3Messagebox("Disable \"NewAdvDlgItems.era\" plugin from Game Enhancement Mod\\eraplugins \n\
                    and report about that to daemon_n in Discord");
		}
	}


	return EXEC_DEFAULT;
}

_LHF_(MithrilDisplay::OnKingdomOverviewDlgResBarCreate)
{
	if (H3GameHeight::Get() >= 608)
	{
		buildMithril(reinterpret_cast<H3ResourceBarPanel*>(c->eax), 35, false);
		//mithril->lastHintIsMitril = false;
	}

	return EXEC_DEFAULT;
}
_LHF_(MithrilDisplay::OnAdvMgrDlgResBarCreate)
{


	h3::H3ResourceBarPanel* resourceBarPanel = reinterpret_cast<H3ResourceBarPanel*>(c->eax);

	buildMithril(resourceBarPanel, 2);


	return EXEC_DEFAULT;
}

_LHF_(MithrilDisplay::BeforeHotseatMsgBox)
{

	H3BaseDlg* advDlg = P_AdventureMgr->dlg;
	h3::H3ResourceBarPanel* dlg = reinterpret_cast<H3ResourceBarPanel*>(c->ecx);

	if (dlg && advDlg)
	{
		H3DlgText* myText = advDlg->GetText(mithrilBar->dlgTextId);
		if (myText)
		{
			//	mithril->SetFrameColor(advDlg);
			myText->SetText(""); //set "noText" for MP msgBox
		}
	}

	return EXEC_DEFAULT;
}



_LHF_(MithrilDisplay::OnResourceBarDlgUpdate)
{

	H3BaseDlg* dlg = reinterpret_cast<H3BaseDlg*>(c->ecx);

	if (dlg)
	{
		H3DlgText* mithrilTextItem = dlg->GetText(mithrilBar->dlgTextId);
		if (mithrilTextItem)
			mithrilTextItem->SetText(H3String::Format("%d", mithrilBar->getMithrilAmout()));
	}

	return EXEC_DEFAULT;
}

