#include "pch.h"
#include "LegendHeroes.h"


LegendHeroes* LegendHeroes::instance = nullptr;
H3LoadedPcx16* LegendHeroes::m_drawBuffer[] = {};

LegendHeroes::LegendHeroes(PatcherInstance* pi)
{

	WgtSettings::loadArtSettings();
	m_marketWgt = new WgtSettings("nhd.artifact_merchant");

	altarWgt = new WgtSettings("nhd.altar_of_sacr");
	m_swapWgt[0] = new MeetWgt("nhd.hero_left");
	m_swapWgt[1] = new MeetWgt("nhd.hero_right");
	m_heroWgt = new HeroWgt("nhd.hero_screen");


	m_heroLooks = HeroLook::heroLook;
	HeroLook::ResetSettings();
	//HeroLook::LoadIniSettings(m_heroLooks);


}

void LegendHeroes::Init(PatcherInstance* pi)
{

	 if (!instance)
	 {
		 instance = new LegendHeroes(pi);
		 if (instance)
		 {
			 instance->_pi = pi;
			 instance->CreatePatches();
			 instance->m_isInited = true;
		 }
	 }
}


void LegendHeroes::CreatePatches() noexcept
{
	m_drawBuffer[0] = H3LoadedPcx16::Create(800, 600);
	m_drawBuffer[1] = H3LoadedPcx16::Create(800, 600);

	_pi->WriteLoHook(0x5AEAF0, SwapDlg_BeforeShow);
	// _PI->WriteLoHook(0x5AEB81, OnBeforeHeroSwapDlgShow);

   //  _PI->WriteLoHook(0x4E1A70, OnBefore_HeroDlg_Create);

	_pi->WriteLoHook(0x4DEF7A, HeroDlg_BeforeArtifactsPlacement);

	_pi->WriteLoHook(0x4E1BD4, HeroDlg_AfterCreate);
	Era::RegisterHandler(OnPreHeroScreen, "OnPreHeroScreen");
	//Era::RegisterHandler(OnGameLeave, "OnGameLeave");

//	Era::RegisterHandler(OnGameLeave, "OnGameLeave");
	Era::RegisterHandler(OnAfterErmInstructions, "OnAfterErmInstructions");

	
	//_pi->WriteHiHook(0x4E1CC0, THISCALL_, H3HeroDlgUodate);
	_pi->WriteHiHook(0x4DD540, THISCALL_, DlgHero_Proc);


	//0x5E5C6F

	_pi->WriteLoHook(0x5E5D64, MarketDlg_BeforeArtifactsPlacement);
	_pi->WriteLoHook(0x5EC280, MarketDlg_AfterArtifactsPlacement);

	_pi->WriteLoHook(0x56073A, SacrifaceDlg_BeforeArtifactsPlacement);


	LvlUpDlgHandler::Init(globalPatcher->CreateInstance("LegendHeroes.LvlUpDlg.daemon_n"), m_drawBuffer[0]);
	TavernDlgHandler::Init(globalPatcher->CreateInstance("LegendHeroes.TavernDlg.daemon_n"), m_drawBuffer);




	auto& wgt = *altarWgt;

	_pi->WriteDword(0x560761 + 0x3, (int)&WgtSettings::sacrifaceArtSlotPositions[0].y); //replace artifacts.def original pos

	_pi->WriteDword(0x5609B6 + 1, wgt.arrows[0].y); //replace left arrow original y pos
	_pi->WriteByte(0x5609BB + 1, wgt.arrows[0].x);  //replace left arrow original x pos

	_pi->WriteDword(0x560A39 + 1, wgt.arrows[1].y); //replace right_arrow arrow original y pos
	_pi->WriteDword(0x560A3E + 1, wgt.arrows[1].x); //replace right_arrow arrow original x pos
	_pi->WriteDword(0x5641A2 + 2, 0x6A673C); //replace right_arrow arrow original x pos

	

	_pi->WriteDword(0x5608FB + 1, wgt.backpack.pos.y); //replace bacpack artifacts.def original y pos
	_pi->WriteDword(0x5608B6 + 3, wgt.backpack.pos.x); //replace bacpack artifacts.def original start x pos
	_pi->WriteByte(0x560921 + 2, wgt.backpack.interval); //replace bacpack artifacts.def x interval

	for (int i = 0; i < 19; i++) // Fill Table with values from json
	{

		int xPos = WgtSettings::artSlotPositions[i].x + wgt.startPos.x;
		int yPos = WgtSettings::artSlotPositions[i].y + wgt.startPos.y;

		WgtSettings::sacrifaceArtSlotPositions[i] = { xPos, yPos };
	}


	m_isInited = true;
}
constexpr int HEROES_MAX_AMOUNT = 155;
constexpr UINT16 NPC_BUTTON_ID = 4445;
constexpr int DLG_CMD_SET_DEF = 9;
constexpr int DLG_CMD_SET_PCX = 11;
constexpr int BG_PCX_HEIGHT = 407;
constexpr int BG_PCX_WIDTH = 287;

void  __stdcall LegendHeroes::OnAfterErmInstructions(Era::TEvent* event)
{
	auto heroLook = instance->m_heroLooks;
	for (int i = 0; i < HEROES_MAX_AMOUNT; i++)
	{
		if (!heroLook->forceOverride && !heroLook[i].original)
		{
			sprintf(const_cast<char*>(P_HeroInfo[i].largePortrait), "nhl%d_%d.pcx", heroLook[i].faction, heroLook[i].portraitIndex);
			sprintf(const_cast<char*>(P_HeroInfo[i].smallPortrait), "nhs%d_%d.pcx", heroLook[i].faction, heroLook[i].portraitIndex);
		}
	}
}


//#define WoG_NoNPC (*(INT32*)0x277192C) // опция командиров (0-вкл, 1-выкл)

_LHF_(LegendHeroes::SwapDlg_BeforeShow)
{
	H3Dlg* dlg = reinterpret_cast<H3Dlg*>(c->eax);
	H3SwapManager* swapMgr = H3SwapManager::Get(); // get swapMgr

	for (UINT16 side = 0; side < 2; side++)
	{

		BaseDlg_AddBackGroundPcx(dlg, *instance->m_swapWgt[side], swapMgr->hero[side]->id, 1);

		auto& wgt = *instance->m_swapWgt[side];

		int bkgId = wgt.bgId;
		const int xOff = instance->m_swapWgt[side]->startPos.x;
		const int yOff = instance->m_swapWgt[side]->startPos.y;
		for (int i = 0; i < 19; i++)
		{

			const int artSlotId = side * 19 + i + 27;// +idOffset;
			const int artSlotBgId = side * 19 + i + 150;// +idOffset;

			const int xPos = wgt.artSlotPositions[i].x + xOff;
			const int yPos = wgt.artSlotPositions[i].y + yOff;


			if (H3DlgItem* originalArtBgWgt = dlg->GetH3DlgItem(artSlotBgId))
			{
				WordAt(reinterpret_cast<int>(originalArtBgWgt) + 0x10) += 20000;// set newID
				originalArtBgWgt->HideDeactivate();
			}
			dlg->AddItem(H3DlgDef::Create(xPos, yPos, 44, 44, artSlotBgId, "artifact.def", 0, 0, FALSE, 0x10)); //add new items to dlg

			if (H3DlgItem* originalArtWdgt = dlg->GetH3DlgItem(artSlotId))
			{
				WordAt(reinterpret_cast<int>(originalArtWdgt) + 0x10) += 20000;// set newID
				originalArtWdgt->HideDeactivate();
			}
			dlg->AddItem(H3DlgDef::Create(xPos, yPos, 44, 44, artSlotId, "artifact.def", 0, 0, FALSE, 0x10)); //add new items to dlg

		}

		for (int i = 0; i < 5; i++)
		{
			int artSlotId = side * 5 + i + 89;

			int xPos = i * wgt.backpack.interval + wgt.backpack.pos.x;// std::atoi(GetEraJSON((jsonSubStr + ".backpack.interval").c_str())) + std::atoi(GetEraJSON((jsonSubStr + ".backpack.x").c_str()));// +xOffset;
			int yPos = wgt.backpack.pos.y;// +yOffset;

			if (H3DlgItem* originalArtBgWgt = dlg->GetH3DlgItem(artSlotId))
			{
				WordAt(reinterpret_cast<int>(originalArtBgWgt) + 0x10) += 20000;// set newID
				originalArtBgWgt->HideDeactivate();
			}

			dlg->AddItem(H3DlgDef::Create(xPos, yPos, 44, 44, artSlotId, "artifact.def", 0, 0, FALSE, 0x10)); //add new items to dlg
		}


		for (int i = 0; i < 2; i++)
		{
			int arrowId = 99 + side + i * 2;

			int xPos = wgt.arrows[i].x;// std::atoi(GetEraJSON((jsonSubStr + ".left_arrow.x").c_str()));
			int yPos = wgt.arrows[i].y;


			if (H3DlgItem* arrowWgt = dlg->GetH3DlgItem(arrowId))
			{
				WordAt(reinterpret_cast<int>(arrowWgt) + 0x10) += 20000;// set newID
				arrowWgt->HideDeactivate();
			}
			dlg->AddItem(H3DlgDefButton::Create(xPos, yPos, 22, 46, arrowId, H3String::Format("hsbtns%d.def", i * 2 + 3).String(), 0, 1, 0, 0)); //add new items to dlg

		}


		// align questlogs buttons for swap dlg
		if (H3DlgItem* questWidget = dlg->GetH3DlgItem(85 + side))
		{
			questWidget->SetWidth(wgt.buttonsAlignment.width);
			questWidget->SetHeight(wgt.buttonsAlignment.height);

			int x = questWidget->GetX();
			questWidget->SetX(x + 8);
		}
		// allign backpacks buttons
		if (H3DlgItem* backpackWidget = dlg->GetH3DlgItem(8000 + side))
		{
			backpackWidget->SetWidth(wgt.buttonsAlignment.width);
			backpackWidget->SetHeight(wgt.buttonsAlignment.height);
			if (side ) // or right side only
			{
				int x = backpackWidget->GetX();
				backpackWidget->SetX(x + 12);
			}
		}
	}

	return EXEC_DEFAULT;
}




_LHF_(LegendHeroes::HeroDlg_BeforeArtifactsPlacement)
{
	H3Dlg* dlg = *reinterpret_cast<H3Dlg**>(c->ebp - 0x14);

	auto& wgt = *instance->m_heroWgt;
	BaseDlg_AddBackGroundPcx(dlg, wgt, P_DialogHero->id);

	int btnId = wgt.mgrBttn.id;

	UINT8 nymOffset = instance->m_heroLooks->interfaceMod * 2;

	// create mgr button only if owner is here
	if (P_DialogHero->owner==P_Game->GetPlayerID()
		&& P_Game->GetPlayerID() == P_CurrentPlayerID)
	{
		H3DlgDefButton* managerButton = H3DlgDefButton::Create(0, 0, wgt.mgrBttn.id, wgt.mgrBttn.defName, 0 + nymOffset, 1 + nymOffset, 0, NULL);

		managerButton->SetHints(wgt.mgrBttn.hint, wgt.mgrBttn.hint, true);
		managerButton->AddHotkey(eVKey::H3VK_M);
		dlg->AddItem(managerButton, false);
	}



	return EXEC_DEFAULT;
}


_LHF_(LegendHeroes::HeroDlg_AfterCreate)
{

	H3Dlg* dlg = reinterpret_cast<H3Dlg*>(c->ecx);

	// CALL_1(H3Dlg*, __thiscall, hook->GetDefaultFunc(), dlg);
	H3DlgItem* it, * itBg;

	auto& wgt = *instance->m_heroWgt;
	int xOffset = wgt.startPos.x;// std::atoi(GetEraJSON("nhd.hero_screen.x"));
	int yOffset = wgt.startPos.y; //std::atoi(GetEraJSON("nhd.hero_screen.y"));
	instance->m_heroLooks->needRedraw = true;

	for (int i = 0; i < 19; i++)
	{
		int artSlotId = i + 2;
		int artSlotPressedId = i + 21;
		it = dlg->GetH3DlgItem(artSlotId);
		itBg = dlg->GetH3DlgItem(artSlotPressedId);

		if (it && itBg)
		{
			int xPos = wgt.artSlotPositions[i].x + xOffset;
			int yPos = wgt.artSlotPositions[i].y + yOffset;
			it->SetX(xPos);
			it->SetY(yPos);
			itBg->SetX(xPos);
			itBg->SetY(yPos);
		}
	}

	for (UINT16 i = 0; i < 5; i++)
	{
		UINT16 artSlotId = i + 40;
		it = dlg->GetH3DlgItem(artSlotId);
		if (it)
		{
			it->SetX(i * wgt.backpack.interval + wgt.backpack.pos.x);
			it->SetY(wgt.backpack.pos.y);
		}
	}

	// стрелки рюкзака
	for (UINT16 i = 0; i < 2; i++)
	{
		it = dlg->GetH3DlgItem(77 + i);
		if (it)
		{
			it->SetX(wgt.arrows[i].x);
			it->SetY(wgt.arrows[i].y);
		}
	}

	// кнопка командира
	it = dlg->GetH3DlgItem(NPC_BUTTON_ID);
	if (it)
	{
		// Поменять def кнопки
		it->SendCommand(DLG_CMD_SET_DEF, (int)wgt.npcBttn.defName);

		INT8 nymOffset = instance->m_heroLooks->interfaceMod * 2;
		H3DlgDefButton* defBttn = dlg->GetDefButton(NPC_BUTTON_ID);

		defBttn->SetFrame(0 + nymOffset);
		defBttn->SetClickFrame(1 + nymOffset);
	}

	it = dlg->GetDefButton(wgt.mgrBttn.id);
	if (it && !it->GetHint())
		it->SetHints(wgt.mgrBttn.hint, wgt.mgrBttn.hint, true);


	// Выравниваем добавленные кнопки окна героя
	instance->AlignButtons(dlg);

	instance->lastHeroLookSet = P_DialogHero->id; //reinterpret_cast<H3Hero*>(0x00698B70)->id;

	return EXEC_DEFAULT;
}


void LegendHeroes::AlignButtons(H3BaseDlg* dlg)
{
	H3Vector<H3DlgItem*> buttons; //create vector of buttons
	H3DlgItem* it;

	auto& wgt = *m_heroWgt;

	buttons.Reserve(wgt.buttonsAlignment.buttonsToAline.Size());
	int counter{};
	for (auto& s : wgt.buttonsAlignment.buttonsToAline)
	{
		// first try to get button Id by tag
		INT16 buttonId = Era::GetButtonID(s.c_str());

		if (buttonId <= 0)// If failed
			buttonId = atoi(s.c_str());// get directly by id


		if (buttonId > 0)			// if it is real Id
			if (it = dlg->GetDefButton(buttonId)) //  try to find that button* by Id
				buttons.Push(it); // add to the vector
	}

	int buttonsNum = buttons.Size();

	if (buttonsNum) // if at least one button
	{
		for (auto& b : buttons)
		{

			b->SetWidth(wgt.buttonsAlignment.width);
			b->SetHeight(wgt.buttonsAlignment.height);
			b->SetY(wgt.buttonsAlignment.y);
		}
		int totalFreeSpace = wgt.buttonsAlignment.totalWidth - buttonsNum * wgt.buttonsAlignment.width;

		int interval = 0;
		if (totalFreeSpace > 0)
			interval = totalFreeSpace / (buttonsNum + 1);

		UINT16 x = wgt.buttonsAlignment.x;

		for (UINT16 i = 0; i < buttonsNum; i++)
		{
			buttons[i]->SetX(x);
			x += interval + wgt.buttonsAlignment.width;
		}
	}

}


void __stdcall LegendHeroes::OnPreHeroScreen(Era::TEvent* e)
{

	auto& wgt = *instance->m_heroWgt;

	int bkgId = wgt.bgId;// std::atoi(GetEraJSON("nhd.hero_screen.bg_item_id"));
	int heroId = P_DialogHero->id;

	H3BaseDlg* dlg = *reinterpret_cast<H3BaseDlg**>(0x698AC8);

	if (instance->m_heroLooks->needRedraw || instance->lastHeroLookSet != heroId)
	{
		HeroLook::CorrectPcxNames(heroId);
		instance->lastHeroLookSet = heroId;
		//adjust pictures for needed m_heroWgt
		for (int i = 0; i < 4; i++)
			if (auto it = dlg->GetPcx16(bkgId++)) //it->SetPcx();
				it->SendCommand(DLG_CMD_SET_PCX, int(HeroLook::pcxNames[i].String()));
	}

}


// Market dlg - add new items before dlg created
_LHF_(LegendHeroes::MarketDlg_BeforeArtifactsPlacement)
{
	H3Dlg* dlg = reinterpret_cast<H3Dlg*>(c->edi);
	H3Hero* m_heroWgt = *reinterpret_cast<H3Hero**>(0x6AAAE0);
	BaseDlg_AddBackGroundPcx(dlg, *instance->m_marketWgt, m_heroWgt->id);

	return EXEC_DEFAULT;
}


// Market dlg - change widgets positions after creation
_LHF_(LegendHeroes::MarketDlg_AfterArtifactsPlacement)
{
	H3Dlg* dlg = reinterpret_cast<H3Dlg*>(c->ecx);
	auto& wgt = *instance->m_marketWgt;

	H3DlgItem* it;
	int xOffset = wgt.startPos.x;
	int yOffset = wgt.startPos.y;
	H3Hero* m_heroWgt = *reinterpret_cast<H3Hero**>(0x6AAAE0);
	int lookId = wgt.bgId;


	for (UINT16 i = 0; i < 19; i++)
	{
		UINT16 artSlotId = i + 84;
		UINT16 artSlotPressedId = i + 107;

		if (i == 18) // if misc 5 slot = only for HD mod
		{
			artSlotId = 1126;
			artSlotPressedId = 1127;

			constexpr UINT16 miscFiveBackgroundItemId = 2126;
			if (it = dlg->GetH3DlgItem(miscFiveBackgroundItemId))
				it->Hide();
		}

		if (it = dlg->GetH3DlgItem(artSlotId))
		{
			int xPos = wgt.artSlotPositions[i].x + xOffset;
			int yPos = wgt.artSlotPositions[i].y + yOffset;
			it->SetX(xPos);
			it->SetY(yPos);
		}

		if (it = dlg->GetH3DlgItem(artSlotPressedId))
		{
			int xPos = wgt.artSlotPositions[i].x + xOffset - 2;
			int yPos = wgt.artSlotPositions[i].y + yOffset - 2;
			it->SetX(xPos);
			it->SetY(yPos);
		}
	}


	for (UINT16 i = 0; i < 5; i++)
	{
		int artSlotId = i + 102;
		int artSlotPressedId = i + 125;

		if (it = dlg->GetH3DlgItem(artSlotId))
		{
			it->SetX(i * wgt.backpack.interval + wgt.backpack.pos.x);
			it->SetY(wgt.backpack.pos.y);
		}

		if (it = dlg->GetH3DlgItem(artSlotPressedId))
		{
			it->SetX(i * wgt.backpack.interval + wgt.backpack.pos.x - 2);
			it->SetY(wgt.backpack.pos.y - 2);
		}
	}

	// стрелки рюкзака
	for (UINT16 i = 0; i < 2; i++)
	{
		if (it = dlg->GetH3DlgItem(130 + i))
		{
			it->SetX(wgt.arrows[i].x);
			it->SetY(wgt.arrows[i].y);
		}
	}


	if (it = dlg->GetH3DlgItem(14))
		it->Hide();

	return EXEC_DEFAULT;
}


int __stdcall LegendHeroes::DlgHero_Proc(HiHook* hook, H3Dlg* dlg, H3Msg* msg)
{
	if (msg->command == eMsgCommand::MOUSE_OVER) {
		auto* it = dlg->ItemAtPosition(msg);
		if (it && it->GetID() == instance->m_heroWgt->mgrBttn.id)
		{
			dlg->SendCommandToAllItems(eMsgSubtype::SET_TEXT, 115, (int)it->GetHint());
			THISCALL_4(void, 0x005FF5E0, dlg, true, 114, 115);
			return 0;
		}
	}

	return THISCALL_2(int, hook->GetDefaultFunc(), dlg, msg);
}


void LegendHeroes::BaseDlg_AddBackGroundPcx(H3Dlg* dlg, WgtSettings& wgt, int heroId, bool initiate)
{

	int width = wgt.width;
	int height = wgt.height;

	if (width < 1)      width = BG_PCX_WIDTH;
	if (height < 1)     height = BG_PCX_HEIGHT;

	int bkgId = wgt.bgId;
	int xPos = wgt.startPos.x;
	int yPos = wgt.startPos.y;
	HeroLook::CorrectPcxNames(heroId);

	dlg->AddItem(H3DlgPcx16::Create(xPos, yPos, width, height, bkgId, HeroLook::pcxNames[0].String()), initiate);
	dlg->AddItem(H3DlgPcx16::Create(xPos, yPos, width, height, ++bkgId, HeroLook::pcxNames[1].String()), initiate);
	dlg->AddItem(H3DlgPcx16::Create(xPos + 1, yPos, width, BG_PCX_HEIGHT - 57, ++bkgId, HeroLook::pcxNames[2].String()), initiate);
	dlg->AddItem(H3DlgPcx16::Create(xPos + 1, wgt.backpack.altitude, width, 56, ++bkgId, HeroLook::pcxNames[3].String()), initiate);
}


_LHF_(LegendHeroes::SacrifaceDlg_BeforeArtifactsPlacement)//(LoHook* hook, HookContext* c)
{
	H3Dlg* dlg = *reinterpret_cast<H3Dlg**>(c->ebp - 0x10); //get Dlg
	H3Player* playerPtr = P_Game->GetPlayer();

	BaseDlg_AddBackGroundPcx(dlg, *instance->altarWgt, playerPtr->currentHero);

	return EXEC_DEFAULT;
}




