#include "pch.h"
#include "MithrilDisplay.h"


namespace ERI
{
	const int GetPlayerMithril(const int playerId = P_Game->GetPlayerID())
	{
		return IntAt(0x27F9A00 + playerId * 4); // mb later it should be replaced 
	}


	void ExtendedResourcesInfo::SetLastHintItemId(const int itemId) noexcept
	{
		m_resbarLastHintItemId = itemId;
	}

	ExtendedResourcesInfo::ExtendedResourcesInfo(PatcherInstance* _PI)
		:IGamePatch(_PI), m_dlgTextId(1016), defName("SMALmith.def"),
		advMapHintControl(nullptr), kingdomHintcontrol(nullptr)
	{

		CreatePatches();
	}


	_LHF_(BuildResBar)
	{
		//H3Messagebox();

		return EXEC_DEFAULT;
	}
	/// set mouse hover hints for the resources
	char __stdcall H3AdventureMgrDlg_Interface_MoveHint(HiHook* h, H3AdventureMgrDlg* dlg, const int x, const int y)
	{

		// check if not focused on chat log
		if (!CharAt(reinterpret_cast<DWORD>(dlg->screenlogEdit) + 0x6D))
		{
			auto& info = ExtendedResourcesInfo::Get();
			const int lastHintItemId = info.LastHintItemId();// IntAt(0x65F228);
			const int mithrilItemId = info.MitrilItemId();

			unsigned clickedItemId = -1;
			for (size_t i = 0; i < 8; i++)
			{
				const int itemId = i < 7 ? 1001 + i : info.MitrilItemId();
				if (auto it = dlg->GetH3DlgItem(itemId))
				{

					if (it->IsActive()
						&& it->GetAbsoluteX() <= x
						&& it->GetAbsoluteX() + it->GetWidth() > x
						&& it->GetAbsoluteY() <= y
						&& it->GetAbsoluteY() + it->GetHeight() > y
						)

					{
						clickedItemId = itemId;

						if (lastHintItemId != clickedItemId)
						{
							// get player resources
							const unsigned int resAmount = i == 7 ? GetPlayerMithril() : h3::H3ActivePlayer::Get()->playerResources[i];
							// create new hint
							sprintf(h3_TextBuffer, "%s: %s", P_ResourceName[i], H3String::FormatNumber(resAmount).String());
							// set adv manager hint text
							THISCALL_2(void, 0x40B040, P_AdventureManager->Get(), (int)h3_TextBuffer);
						}
						break;
					}
				}
			}
			// clear hint if out of any of items
			if (lastHintItemId != -1 && clickedItemId == -1)
			{
				THISCALL_2(void, 0x40B040, P_AdventureManager->Get(), (int)"");

			}
			info.SetLastHintItemId(clickedItemId);

		}

		return THISCALL_3(char, h->GetDefaultFunc(), dlg, x, y);
	}

	void ExtendedResourcesInfo::CreatePatches()
	{

		_PI->WriteLoHook(0x4021B2, ExtendedResourcesInfo::OnAdvMgrDlgResBarCreate); // H3AdventureMgrDlg
		_PI->WriteLoHook(0x51F042, ExtendedResourcesInfo::OnKingdomOverviewDlgResBarCreate); // KingdomOverviewDlg
		_PI->WriteLoHook(0x408945, ExtendedResourcesInfo::OnAdvMgrDlgRightClick); // H3AdventureMgrDlg


		_PI->WriteLoHook(0x403F00, ExtendedResourcesInfo::OnResourceBarDlgUpdate);
		//_PI->WriteLoHook(0x417380, ExtendedResourcesInfo::OnResourceBarDlgUpdate);
		_PI->WriteLoHook(0x559270, ExtendedResourcesInfo::BeforeHotseatMsgBox);
		_PI->WriteHiHook(0x521E20, THISCALL_, ExtendedResourcesInfo::KingdomOverviewDlgProc);


		_PI->WriteLoHook(0x558E16, BuildResBar);

		_PI->WriteHiHook(0x40EB47, THISCALL_, H3AdventureMgrDlg_Interface_MoveHint);
		_PI->WriteHiHook(0x40E1CC, THISCALL_, H3AdventureMgrDlg_Interface_MoveHint);
		//	_PI->WriteHiHook(0x40E2C0, THISCALL_, H3AdventureMgrDlg_Interface_MoveHint);


	}


	int __stdcall ExtendedResourcesInfo::KingdomOverviewDlgProc(HiHook* h, H3BaseDlg* dlg, H3Msg* msg)
	{

		auto* mithrilBar = &Get();
		if (msg->subtype == eMsgSubtype::RBUTTON_DOWN)
		{
			mithrilBar->ShowMithrilRMCHint(msg, mithrilBar->kingdomHintcontrol);
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

	const BOOL ExtendedResourcesInfo::ShowMithrilRMCHint(H3Msg* msg, H3DlgItem* hintZone) noexcept
	{
		int x = msg->GetX();
		int y = msg->GetY();

		bool result = x >= hintZone->GetAbsoluteX() && x <= hintZone->GetAbsoluteX() + hintZone->GetWidth() && y >= hintZone->GetAbsoluteY() && y <= hintZone->GetAbsoluteY() + hintZone->GetHeight();

		if (result)
		{
			int width = 0;
			LPCSTR	hintText = Era::tr("gem_plugin.mithril_display.popup_hint");

			FASTCALL_3(void, 0x4F6930, hintText, &width, &msg);
			FASTCALL_12(void, 0x4F6C00, hintText, 4, 592 - width, (0x258 - (int)msg) / 2 - 10, -1, 0, -1, 0, -1, 0, -1, 0);
		}
		else if (msg->itemId == 1008 && x < hintZone->GetAbsoluteX())
			result = true; // prevent extra hint for the date hint

		return result;
	}



	_LHF_(ExtendedResourcesInfo::OnAdvMgrDlgRightClick)
	{

		bool ret = EXEC_DEFAULT;
		H3Msg* msg = reinterpret_cast<H3Msg*>(c->edi);
		auto* mithrilBar = &Get();

		if (mithrilBar->ShowMithrilRMCHint(msg, mithrilBar->advMapHintControl))
		{
			c->ebx = true;
			c->return_address = 0x408974;
			ret = NO_EXEC_DEFAULT;
		}

		return ret;
	}




	const INT ExtendedResourcesInfo::MitrilItemId() noexcept
	{
		return m_dlgTextId;
	}



	ExtendedResourcesInfo& ExtendedResourcesInfo::Get()
	{
		static ExtendedResourcesInfo instance(globalPatcher->CreateInstance("EraPlugin.ResourceBar.daemon_n"));
		return instance;
	}

	const int ExtendedResourcesInfo::LastHintItemId() const noexcept
	{
		return m_resbarLastHintItemId;
	}


	BOOL8 ExtendedResourcesInfo::BuildMithril(H3ResourceBarPanel* resourceBarPanel, int xOffset, bool buildFrame)
	{

		if (resourceBarPanel)
		{

			auto* mithrilBar = &ExtendedResourcesInfo::Get();

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


				H3DlgItem* mithrilDef = H3DlgDef::Create(xPos + 2, 4, 1019, mithrilBar->defName);


				int textWidth = 50;
				if (buildFrame)
				{
					auto resBarPcx = resourceBarPanel->resbarPCX->GetPcx();
					int maxPicWidth = 80;
					int mapWidth = IntAt(0x4196EA + 1);


					int defWidth = mithrilDef->GetWidth();
					int textX = xPos + defWidth;
					textWidth = textX + maxPicWidth;
					if (textWidth > mapWidth)
						textWidth = mapWidth;

					textWidth -= textX;
					if (textWidth > 50)
						textWidth = 50;

					const int pcxWidth = textWidth + defWidth + 8;
					const int pcxHeight = 20;

					H3LoadedPcx* mithrilBackPcx = H3LoadedPcx::Create(h3_NullString, pcxWidth, pcxHeight);

					H3DlgPcx* mithrilBack = H3DlgPcx::Create(xPos, 0, pcxWidth, pcxHeight, 1025, nullptr);

					resBarPcx->DrawToPcx(4, 0, pcxWidth, pcxHeight, mithrilBackPcx);
					resBarPcx->DrawToPcx(28, 0, 20, pcxHeight, mithrilBackPcx, 2);
					//resBarPcx->DrawToPcx(xPos -5 -( pcxWidth >> 1), 0, pcxWidth >> 1, pcxHeight, mithrilBackPcx, pcxWidth>>1, 0);

				///	resBarPcx->DrawToPcx(defWidth - 5, 2, spotWidth, 17, mithrilBackPcx, xPos, 2);
					//H3Messagebox(H3String::Format("%d", pcxWidth));
					mithrilBack->SetPcx(mithrilBackPcx);
					resourceBarPanel->AddItem(mithrilBack); mithrilBack->Show();


				}
				resourceBarPanel->AddItem(mithrilDef); mithrilDef->Show();


				//H3DlgTextPcx


				H3DlgText* mithriText = H3DlgText::Create(xPos + 29, 3, textWidth, 18, H3String::Format("%d", GetPlayerMithril()).String(), NH3Dlg::Text::SMALL, eTextColor::WHITE, mithrilBar->m_dlgTextId, eTextAlignment::HLEFT);
				resourceBarPanel->AddItem(mithriText);
				mithriText->Show();
				mithriText->Activate();
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

	_LHF_(ExtendedResourcesInfo::OnKingdomOverviewDlgResBarCreate)
	{
		if (H3GameHeight::Get() >= 608)
		{
			BuildMithril(reinterpret_cast<H3ResourceBarPanel*>(c->eax), 35, false);
			//mithril->lastHintIsMitril = false;
		}

		return EXEC_DEFAULT;
	}
	_LHF_(ExtendedResourcesInfo::OnAdvMgrDlgResBarCreate)
	{


		h3::H3ResourceBarPanel* resourceBarPanel = reinterpret_cast<H3ResourceBarPanel*>(c->eax);

		BuildMithril(resourceBarPanel, 2);


		return EXEC_DEFAULT;
	}

	_LHF_(ExtendedResourcesInfo::BeforeHotseatMsgBox)
	{

		H3BaseDlg* advDlg = P_AdventureMgr->dlg;
		h3::H3ResourceBarPanel* dlg = reinterpret_cast<H3ResourceBarPanel*>(c->ecx);

		if (dlg && advDlg)
		{
			if (H3DlgText* myText = advDlg->GetText(Get().MitrilItemId()))
			{
				//	mithril->SetFrameColor(advDlg);
				myText->SetText(""); //set "noText" for MP msgBox
			}
		}

		return EXEC_DEFAULT;
	}



	_LHF_(ExtendedResourcesInfo::OnResourceBarDlgUpdate)
	{

		H3BaseDlg* dlg = reinterpret_cast<H3BaseDlg*>(c->ecx);

		if (dlg)
		{
			if (H3DlgText* mithrilTextItem = dlg->GetText(Get().MitrilItemId()))
			{
				mithrilTextItem->SetText(H3String::Format("%d", GetPlayerMithril()));

			}
		}

		return EXEC_DEFAULT;
	}



}

