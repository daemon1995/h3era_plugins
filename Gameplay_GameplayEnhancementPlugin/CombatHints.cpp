#include "pch.h"
#include "CombatHints.h"


namespace cmbhints
{
	constexpr float HP_LABEL_HEIGHT = 4.0f;
	constexpr float HP_LABEL_FILL = 0.4f;
	constexpr float HP_LABEL_LOSS = 0.975f;
	constexpr float HP_LABEL_SATURATION = 0.8f; // idk.mb set 1
	constexpr int HP_LABEL_MAX_OFFSET = 18;
	CombatHints* CombatHints::instance = nullptr;
	CombatHints::CombatHints() :IGamePatch(globalPatcher->CreateInstance("EraPlugin.CombatHints.daemon_n"))
	{
		CreatePatches();

		settings.reset();
		settings.load();
	}

	DllExport BOOL SetState(int isEnabled, int isHeld) noexcept
	{
		auto& settings = CombatHints::Get().settings;
		bool settingsChanged = false;

		if (settings.isEnabled != isEnabled)
		{
			settings.isEnabled = isEnabled;
			settingsChanged = true;

		}
		if (settings.isHeld != isHeld)
		{
			settings.isHeld = isHeld;
			settingsChanged = true;
		}
		return settingsChanged;
	}

	DllExport void GetState(int * isEnabled, int * isHeld) noexcept
	{
		Era::RESIZE_ALG_DOWNSCALE;
		auto& settings = CombatHints::Get().settings;
		if (isEnabled)
			*isEnabled = settings.isEnabled;
		if (isHeld)
			*isHeld = settings.isHeld;
	}

	CombatHints& CombatHints::Get()
	{
		if (instance == nullptr)
			instance = new CombatHints();
		return *instance;

	}


	void CombatHints::CreatePatches() noexcept
	{
		if (!m_isInited)
		{

			this->_pi->WriteLoHook(0x43E38B, BeforeBattleStackHintDraw);
			//_PI->WriteLoHook(0x4682C0, BattleOptionsDlg);
			this->_pi->WriteHiHook(0x4682C0, THISCALL_, BattleOptionsDlg_Show);

			// Maybe isn't needed
		//	_PI->WriteLoHook(0x4746BD, BattleMgr_ProcessActionL);


			m_isInited = true;

		}
	}

	_LHF_(CombatHints::BattleMgr_ProcessActionL)
	{
		H3Msg* msg = reinterpret_cast<H3Msg*>(c->eax);
		auto& settings = Get().settings;
		if (msg && settings.isHeld)
		{

			auto* combatHints = &Get();



			if (msg->GetKey() == settings.scanCode && (msg->IsKeyDown() || msg->IsKeyHeld()) && combatHints->needRedraw > -1)
			{
				combatHints->needRedraw = true;
			}


			if (msg->IsKeyPress() && msg->GetKey() == settings.scanCode)
			{
				//combatHints->needRedraw = true;
				THISCALL_1(void, 0x477C00, H3CombatManager::Get());

				combatHints->needRedraw = 0;

			}

		}

		return EXEC_DEFAULT;
	}





	_LHF_(CombatHints::BeforeBattleStackHintDraw)
	{
		H3CombatCreature* stack = reinterpret_cast<H3CombatCreature*>(c->ebx);
		auto& settings = Get().settings;

		if (stack && stack->numberAlive && settings.isEnabled)
		{

			// GetKeyState Call
			if (settings.isHeld && !(STDCALL_1(SHORT, PtrAt(0x63A294), settings.vKey) & 0x800))
				// if key is required and isnt' pressed
			{
				return EXEC_DEFAULT; // return
			}

			// if position is out of the original label - when 0 then pos is same
			// also reverse value from the settings cause yPos is ascending
			if (const int labelYOffset = static_cast<int>(settings.height) * -1)
			{

				const int nativeLabelX = c->edi;
				const int nativeLabelY = c->esi;

				const int newLabelX = nativeLabelX;

				const int newLabelY = nativeLabelY + labelYOffset; // add border


				const H3CombatManager* cmbMgr = H3CombatManager::Get();

				const BOOL labelDrawn = THISCALL_4(BOOL, 0x495460, cmbMgr, cmbMgr->cmNumWinPcxLoaded, newLabelX, newLabelY);// draw label pcx
				const int maxHp = stack->MaxHitPoints();

				// if origial hint was drawn this one will appear too
				if (labelDrawn && maxHp)
				{
					const int labelHeight = cmbMgr->cmNumWinPcxLoaded->height;

					// difference between
					const int protrusionSize = std::abs(labelYOffset);// std::abs(nativeLabelY + labelYOffset - nativeLabelY);

					//const int newLabelHeight = Clamp(0, protrusionSize, labelHeight);
					const int drawHeightMax = labelHeight - 2;
					const int labelBorderHeight = protrusionSize >= labelHeight ? 2 : 1;


					int drawHeight = Clamp(0, protrusionSize - labelBorderHeight, drawHeightMax);

					// if draw area displayed at all
					if (drawHeight > 0)
					{

						const int dlgX = cmbMgr->dlg->GetX();
						const int dlgY = cmbMgr->dlg->GetY();
						// count if it makes sense to draw health
						float fillPartF = static_cast<float>((maxHp - stack->healthLost)) * 28 / maxHp * 10;
						bool hpRemainder = fillPartF > 4;

						int drawWidth = static_cast<int>(fillPartF / 10 + hpRemainder);

						const int hpBarLimit = CharAt(0x43E45B + 1);
						drawWidth = Clamp(0, drawWidth, hpBarLimit);

						// init real draw postion cause of an hd mod offsets
						const int drawX = newLabelX + dlgX + 1; // add 1px offsets for the borders

						// fix for the drawing bar under the amount
						const BOOL isBottomDrawing = nativeLabelY < newLabelY&& protrusionSize <= drawHeightMax;

						const int drawY = dlgY + (isBottomDrawing ? nativeLabelY + labelHeight : newLabelY + 1);

						// set draw limits for bottom panel top border
						if (const auto bottomPanel = P_CombatManager->dlg->bottomPanel)
						{
							const int panelAbsY = bottomPanel->GetY() + dlgY;
							if (panelAbsY < drawY - 1 + drawHeight)
							{
								drawHeight = panelAbsY - drawY;
							}
						}
						//assert(P_CombatManager->updateRect.bottom < 800);
						WindMgr_DrawColoredRect(drawX, drawY, drawWidth, drawHeight, &settings);

						if (drawWidth < hpBarLimit)
						{
							// draw lost hp part
							WindMgr_DrawColoredRect(drawX + drawWidth, drawY, hpBarLimit - drawWidth, drawHeight, &settings, true);
						}

						auto* combatHints = &Get();

						// redraw dlg
						if (settings.isHeld && combatHints->needRedraw > 0)
						{
							combatHints->needRedraw = -1;
							THISCALL_1(void, 0x477C00, cmbMgr);
						}
					}
				}
			}
		}

		return EXEC_DEFAULT;
	}


	int __fastcall CombatHints::CombatOptionsCallback(H3Msg* msg)noexcept
	{

		if (msg->IsLeftClick())
		{
			//	using namespace Era;

			DlgText dlgText = DlgText{
		EraJS::read("gem_plugin.combat_hints.text.rmc_hint"),
	  EraJS::read("gem_plugin.combat_hints.text.handlers"),
	  EraJS::read("gem_plugin.combat_hints.text.input"),
	  EraJS::read("gem_plugin.combat_hints.text.color_health"),
	  EraJS::read("gem_plugin.combat_hints.text.color_loss"),
	  EraJS::read("gem_plugin.combat_hints.text.height"),
	  EraJS::read("gem_plugin.combat_hints.text.saturation"),
	  EraJS::read("gem_plugin.combat_hints.text.default"),
	  EraJS::read("gem_plugin.combat_hints.text.hotkey"),
	  EraJS::read("gem_plugin.combat_hints.text.wrong_hotkey"),
	  EraJS::read("gem_plugin.combat_hints.text.enable"),
	  EraJS::read("gem_plugin.combat_hints.text.toggable"),
	  EraJS::read("gem_plugin.combat_hints.text.set_hotkey"),
	  EraJS::read("gem_plugin.combat_hints.text.press_any"),
	  EraJS::read("gem_plugin.combat_hints.text.held")
			};
			auto& settings = Get().settings;

			SettingsDlg dlg(450, 450, &settings, &dlgText);

			dlg.Start();
			settings.save();

			msg->GetDlg()->Redraw();
		}

		return false;
	}


	//	_LHF_(CombatHints::BattleOptionsDlg)
	void __stdcall CombatHints::BattleOptionsDlg_Show(HiHook* h, H3BaseDlg* dlg)
	{
		//	H3BaseDlg* dlg = reinterpret_cast<H3BaseDlg*>(c->ecx);
		if (dlg)
		{
			int x, y;

			if (IntAt(0x46DF20 + 1) == dlg->GetHeight())
			{
				auto def = dlg->GetDefButton(30722);
				x = def->GetX() + (def->GetWidth() >> 1) - 20;
				y = def->GetY() - 60;
			}
			else
			{
				auto def = dlg->GetDefButton(201);
				x = def->GetX() - 100;
				y = def->GetY();
			}
			x += 3;
			y += 5;

			H3DlgCustomButton* bttn = H3DlgCustomButton::Create(x, y + 5, "HpSttngs.DEF", CombatOptionsCallback, 0, 1);
			bttn->AddHotkey(eVKey::H3VK_H);
			dlg->CreatePcx(bttn->GetX() - 1, bttn->GetY() - 1, 15, NH3Dlg::Assets::BOX_64_32_PCX);

			//dlg->CreateFrame(bttn, color,0,1);
			//	H3DlgScrollbar * scrollBar = H3DlgScrollbar
			dlg->AddItem(bttn);
			H3DlgText* text = H3DlgText::Create(x, y - 33, bttn->GetWidth(), 40, EraJS::read("gem_plugin.combat_hints.text.button_name"), NH3Dlg::Text::MEDIUM);
			dlg->AddItem(text);
		}
		THISCALL_1(void, h->GetDefaultFunc(), dlg);

		//	return EXEC_DEFAULT;
	}

	void CombatHints::WindMgr_DrawColoredRect(const int x, const int y, const  int width, const  int height, const  Settings* stg, const BOOL lost) noexcept
	{
		P_WindowManager->GetDrawBuffer()->AdjustHueSaturation(x, y, width, height, lost ? stg->fcolorLoss : stg->fcolorFill, stg->fsaturation);
	}



	void __fastcall SettingsDlg::ScrollBarGeneralProc(H3BaseDlg* dlg, INT32 scrollBarId, float& valuePtr)
	{
		//auto txt = dlg->GetText(txtId);
		auto scroll = dlg->GetScrollbar(scrollBarId);
		auto label = dlg->GetPcx(5);

		if (scroll && label)//txt && )
		{
			float tick = static_cast<float>(scroll->GetTick());
			//set new settings value
			valuePtr = tick / scroll->GetTicksCount();// scrollBarId < 103 ? tick / scroll->GetTicksCount() : tick;
			// draw label changes
			if (auto stnngsDlg = dynamic_cast<SettingsDlg*>(dlg))
			{
				stnngsDlg->HitPointsBarDraw();
			}
		}
	}

	void SettingsDlg::HitPointsBarDraw()noexcept
	{
		//draw label colors

		const int labelYOffset = static_cast<int>(settings->height) * -1;

		const int nativeLabelY = originalLabel->GetY();

		//const int newLabelX = labelForHp->GetX();
		const int newLabelY = nativeLabelY + labelYOffset; // add border
		//labelForHp->SetY(newLabelY);
		const H3CombatManager* cmbMgr = H3CombatManager::Get();
		const int labelHeight = labelForHp->GetHeight();
		const int protrusionSize = std::abs(labelYOffset);
		const int drawHeightMax = labelHeight - 2;
		const int labelBorderHeight = protrusionSize >= labelHeight ? 2 : 1;
		int drawHeight = Clamp(0, protrusionSize - labelBorderHeight, drawHeightMax);

		//	const int newLabelHeight = Clamp(0, std::abs(newLabelY - nativeLabelY), labelForHp->GetHeight());
			//if (newLabelHeight > 1)
		{


			const int drawHeightLimit = labelForHp->GetHeight() - 2;

			const int drawX = labelForHp->GetAbsoluteX() + 1; // add 1px offsets for the borders
			//const int drawY = originalLabel->GetAbsoluteY() + originalLabel->GetHeight() + 1;
			int drawHeight = Clamp(0, protrusionSize - labelBorderHeight, drawHeightMax);
			// if draw area displayed at all
			if (drawHeight > 0)
			{

				//	const int newLabelHeight = Clamp(0, std::abs(newLabelY - nativeLabelY), originalLabel->GetHeight());
				const BOOL isBottomDrawing = nativeLabelY < newLabelY&& protrusionSize <= drawHeightMax;

				const int drawY = this->yDlg + (isBottomDrawing ? nativeLabelY + labelHeight : newLabelY + 1);

				constexpr int filledHp = 16;

				// draw hp
				CombatHints::WindMgr_DrawColoredRect(drawX, drawY, filledHp, drawHeight, settings);
				CombatHints::WindMgr_DrawColoredRect(drawX + filledHp, drawY, labelForHp->GetWidth() - filledHp - 2, drawHeight, settings, true);
				labelForHp->Refresh();
				labelForHp->ParentRedraw();
				// show changes
			}
		}

	}

	void SettingsDlg::LabelBarDraw()noexcept
	{
		const int x = originalLabel->GetAbsoluteX();
		const int y = originalLabel->GetAbsoluteY();
		const int width = originalLabel->GetWidth();
		// draw label colors
		THISCALL_7(void, 0x44E610, P_WindowManager->GetDrawBuffer(), x + 1, y + 1, width - 2, originalLabel->GetHeight() - 2, FloatAt(0x43E3AC + 1), FloatAt(0x43E3A7 + 1));
		//draw text over the color
		THISCALL_10(void, 0x4B51F0, *reinterpret_cast<H3Font**>(0x698A54), "505", P_WindowManager->GetDrawBuffer(), x, y, width, 15, 4, 1, -1);

		// show changes
		originalLabel->Refresh();
		//	originalLabel->ParentRedraw();
	}

	void __fastcall SettingsDlg::ColorFillScrollBarProc(INT32 value, H3BaseDlg* dlg)
	{
		if (auto sDlg = dynamic_cast<SettingsDlg*>(dlg))
		{
			ScrollBarGeneralProc(dlg, 100, sDlg->settings->fcolorFill);

		}
	}

	void __fastcall SettingsDlg::ColorLossScrollBarProc(INT32 value, H3BaseDlg* dlg)
	{
		if (auto sDlg = dynamic_cast<SettingsDlg*>(dlg))

			ScrollBarGeneralProc(dlg, 101, sDlg->settings->fcolorLoss);
	}

	void __fastcall SettingsDlg::SaturationScrollBarProc(INT32 value, H3BaseDlg* dlg)
	{
		if (auto sDlg = dynamic_cast<SettingsDlg*>(dlg))

			ScrollBarGeneralProc(dlg, 102, sDlg->settings->fsaturation);
	}

	void __fastcall SettingsDlg::HeightScrollBarProc(INT32 value, H3BaseDlg* dlg)
	{

		if (auto stnngsDlg = dynamic_cast<SettingsDlg*>(dlg))
		{
			stnngsDlg->settings->height = value - HP_LABEL_MAX_OFFSET;

			stnngsDlg->labelForHp->SetY(stnngsDlg->originalLabel->GetY() - stnngsDlg->settings->height);
			dlg->Redraw();

			stnngsDlg->HitPointsBarDraw();
			stnngsDlg->LabelBarDraw();
		}
		//ScrollBarGeneralProc(dlg, 103, settings.height);
		//HitPointsBarDraw(hpBar);

	//	LabelBarDraw(label);

	}



	BOOL SettingsDlg::DialogProc(H3Msg& msg)
	{

		if (needRedraw)
		{
			needRedraw = false;
			HitPointsBarDraw();
			LabelBarDraw();
		}


		if (!hk.bttn->IsVisible() && msg.IsKeyDown())
		{
			for (auto it : dlgItems)
				it->Activate();

			if (msg.IsKeyDown())
			{

				eVKey scanCode = msg.GetKey();

				if (settings->validateScanCode(scanCode))
				{
					settings->vKey = MapVirtualKeyA(scanCode, MAPVK_VSC_TO_VK);
					settings->scanCode = scanCode;
					//	str = H3String::Format("\'{%s}\'", getVirtualKeyName(settings.vKey).String());
					hk.name->SetText(getVirtualKeyName(settings->vKey));
				}
				else
				{
					H3Messagebox::RMB(this->text->wrong);
					//hk.text->SetText(text->hotkey);

				}
				hk.text->SetText(text->setHk);

			}

			hk.bttn->Show();
			Redraw();
			needRedraw = true;

		}


		// click on the check boxes
		if (msg.subtype == eMsgSubtype::LBUTTON_CLICK)
		{
			int itemId = msg.itemId;

			auto bttn = GetDefButton(itemId);

			if (bttn)
			{

				if (itemId == 7 || itemId == 8)
				{
					BOOL* optionPtr = itemId == 7 ? &settings->isEnabled : &settings->isHeld;
					*optionPtr ^= 1;
					bttn->SetFrame(*optionPtr);
					bttn->SetClickFrame(*optionPtr);
					bttn->Draw();
					bttn->Refresh();
				}
				else if (itemId == 9)
				{

					//reset settings values
					settings->reset();
					constexpr UINT16 SIZE = 4;
					float values[SIZE] = { settings->fcolorFill ,settings->fcolorLoss ,settings->fsaturation, settings->height + HP_LABEL_MAX_OFFSET };
					H3DlgScrollbar_proc procs[SIZE] = { ColorFillScrollBarProc ,ColorLossScrollBarProc,SaturationScrollBarProc, HeightScrollBarProc };

					// set correct postitions
					for (UINT16 i = 0; i < SIZE; i++)
					{
						auto scrollBar = GetScrollbar(100 + i);
						if (i == 3)
						{
							scrollBar->SetTick(static_cast<int>(values[i]));
						}
						else
						{
							scrollBar->SetTick(static_cast<int>(values[i] * static_cast<float>(scrollBar->GetTicksCount())));
						}

						scrollBar->SetButtonPosition();
						procs[i](scrollBar->GetTick(), this);
						scrollBar->ParentRedraw();
					}
					bttn = GetDefButton(7);
					bttn->SetFrame(settings->isEnabled);
					bttn->SetClickFrame(settings->isEnabled);

					//bttn->Draw();
					//bttn->Refresh();

					bttn = GetDefButton(8);
					bttn->SetFrame(settings->isHeld);
					bttn->SetClickFrame(settings->isHeld);

					hk.name->SetText(getVirtualKeyName(settings->vKey));
					Redraw();
					needRedraw = true;
				}
			}
		}

		hintBar->ShowHint(&msg);

		return 0;
	}



	void SinkItem(H3LoadedPcx16* pcx, H3DlgItem* it)
	{
		pcx->SinkArea(it->GetX() - 5, it->GetY(), it->GetWidth() + 10, it->GetHeight());
	}

	SettingsDlg::SettingsDlg(int width, int height, Settings* incomingSettings, DlgText* text)
		:H3Dlg(width, height, -1, -1, true), text(text), needRedraw(true)
	{
		settings = incomingSettings;
		auto okBttn = CreateOK32Button(widthDlg - 100, heightDlg - 80);
		okBttn->AddHotkey(eVKey::H3VK_ESCAPE);
		okBttn->AddHotkey(eVKey::H3VK_H);


		auto cmb = H3CombatManager::Get();
		LPCSTR barPcxName = cmb->cmNumWinPcxLoaded->GetName();

		//H3DlgDef* creatureDef = H3DlgDef::Create(1, 25, 100, 130, 12, P_CreatureInformation[12].defName, 0, 2);

		H3DlgDef* d = H3ObjectAllocator<H3DlgDef>().allocate(1);
		H3DlgDef* creatureDef = THISCALL_12(H3DlgDef*, 0x4EA800, d, 30, 12, 100, 130, 225, P_CreatureInformation[eCreature::HORNED_DEMON].defName, 0, 2, 0, 0, 0x12);

		//THISCALL_16(H3DlgDef *,0x4EA800, d,)
		AddItem(creatureDef);
		//SinkItem(background, creatureDef);
		background->SinkArea(creatureDef->GetX() - 5, creatureDef->GetY() + 11, creatureDef->GetWidth() + 25, creatureDef->GetHeight() - 10);
		eTextColor textColor = eTextColor::WHITE;

		originalLabel = H3DlgPcx::Create(creatureDef->GetX() + creatureDef->GetWidth() - 25, creatureDef->GetY() + creatureDef->GetHeight() - 30, 5, barPcxName);
		if (originalLabel)
		{
			//H3RGB888 color{ 12,12,12 };

			labelForHp = H3DlgPcx::Create(originalLabel->GetX(), originalLabel->GetY() - static_cast<int>(settings->height), 6, barPcxName);
			if (labelForHp)
			{
				AddItem(labelForHp);
			}
			AddItem(originalLabel);

			//glob = new Settings{};





			textColor = eTextColor::REGULAR;
			H3DlgDefButton* enabledChebox = H3DlgDefButton::Create(160, 30, 7, NH3Dlg::Assets::ON_OFF_CHECKBOX, settings->isEnabled, settings->isEnabled, false, -1);
			AddItem(enabledChebox);

			H3DlgText* enabledText = CreateText(enabledChebox->GetX() + enabledChebox->GetWidth() + 12, enabledChebox->GetY() - 7, 200, 40, text->enable, NH3Dlg::Text::MEDIUM, textColor, 0, eTextAlignment::MIDDLE_LEFT);
			SinkItem(background, enabledText);

			H3DlgDefButton* onlyHeldCheckBox = CreateButton(enabledChebox->GetX(), enabledChebox->GetY() + enabledText->GetHeight(), 8, NH3Dlg::Assets::ON_OFF_CHECKBOX, settings->isHeld, settings->isHeld, false, -1);

			H3DlgText* onlyHeldText = CreateText(enabledText->GetX(), enabledText->GetY() + enabledText->GetHeight(), enabledText->GetWidth(), 40, text->held, NH3Dlg::Text::MEDIUM, textColor, 0, eTextAlignment::MIDDLE_LEFT);
			SinkItem(background, onlyHeldText);

			//getVirtualKeyName(settings.vKey).

			auto fnt = H3MediumFont::Get();
			int textWidth = fnt->GetMaxLineWidth("'{CAPS LOCK}'");
			int textX = widthDlg - textWidth - 16;

			hk.bttn = CreateCustomButton(onlyHeldCheckBox->GetX(), onlyHeldCheckBox->GetY() + enabledText->GetHeight() - 3, 15, "iam009.DEF", SettingsHotkeyCallback, 0, 1);
			int textY = 85;

			H3DlgDefButton* bttn = hk.bttn;
			hk.text = CreateText(onlyHeldText->GetX(), onlyHeldText->GetY() + onlyHeldText->GetHeight(), enabledText->GetWidth(), 40, text->setHk, NH3Dlg::Text::MEDIUM, textColor, 0, eTextAlignment::MIDDLE_LEFT);
			SinkItem(background, hk.text);


			//SinkItem(background, hk.name);

			CreateBlackBox(onlyHeldText->GetX(), hk.text->GetY() + hk.text->GetHeight(), enabledText->GetWidth(), enabledText->GetHeight());
			hk.name = CreateText(onlyHeldText->GetX(), hk.text->GetY() + hk.text->GetHeight(), enabledText->GetWidth(), enabledText->GetHeight(), getVirtualKeyName(settings->vKey).String(), fnt->GetName(), eTextColor::WHITE, 17);

			// create default bttn
			bttn = H3DlgDefButton::Create(100 - okBttn->GetWidth(), okBttn->GetY(), 9, "wogbttn.def", 12, 13, false, -1);
			if (bttn)
			{
				bttn->SetHint(text->dfltName);
				CreatePcx(bttn->GetX() - 1, bttn->GetY() - 1, 15, NH3Dlg::Assets::BOX_64_30_PCX);
				AddItem(bttn);
			}

			constexpr int SIZE = 4;
			int ticks[SIZE] = { 100, 100, 10, HP_LABEL_MAX_OFFSET * 2 + 1 };
			float values[SIZE] = { settings->fcolorFill ,settings->fcolorLoss ,settings->fsaturation, settings->height + HP_LABEL_MAX_OFFSET };

			LPCSTR hints[SIZE] = { text->health,text->loss, text->density,text->height };
			H3DlgScrollbar_proc procs[SIZE] = { ColorFillScrollBarProc ,ColorLossScrollBarProc,SaturationScrollBarProc, HeightScrollBarProc };
			H3DlgText* description;

			//background->DrawShadow(160, originalLabel->GetY() + 20, widthDlg - 60, heightDlg / 3);	


			H3DlgText* dlgText = CreateText(45, hk.text->GetY() + hk.text->GetHeight() + 45, widthDlg - 90, 24, text->handlers, NH3Dlg::Text::BIG, 7, 0);

			for (size_t i = 0; i < SIZE; i++)
			{
				H3DlgScrollbar* scrollBar = H3DlgScrollbar::Create(20, 34 * i + dlgText->GetY() + 27, 148, 16, i + 100, ticks[i], procs[i], false, 2, true);

				//disable catch keys
				if (scrollBar)
				{
					IntAt(reinterpret_cast<int>(scrollBar) + 0x5C) = NULL;
					AddItem(scrollBar);
					scrollBar->SetTick(static_cast<int>(values[i] * ticks[i]));


					if (i == 3)
					{
						scrollBar->SetTick(static_cast<int>(values[i]));
					}
					scrollBar->SetButtonPosition();

				}

				description = H3DlgText::Create(scrollBar->GetX() + scrollBar->GetWidth() + 12, scrollBar->GetY() - 2, 224, 20, hints[i], NH3Dlg::Text::MEDIUM, textColor, i + 100, eTextAlignment::MIDDLE_LEFT);
				if (description)
				{
					background->SinkArea(description->GetX() - 5, description->GetY(), description->GetWidth() + 10, description->GetHeight());
					AddItem(description);
				}

			}











		}
	}

	BOOL Settings::validateScanCode(eVKey scanCode) const noexcept
	{
		return scanCode >= 2 && scanCode <= 14 || scanCode >= 16 && scanCode <= 27 || scanCode >= 29 && scanCode <= 58;
	}

	H3String SettingsDlg::getVirtualKeyName(int vKey) const noexcept
	{
		H3String str;
		switch (vKey)
		{
		case VK_SHIFT:
			str = "'{SHIFT}'";
			break;

		case VK_CONTROL:
			str = "'{CTRL}'";
			break;

		case VK_MENU:
			str = "'{ALT}'";
			break;

		case VK_CAPITAL:
			str = "'{CAPS LOCK}'";
			break;

		case VK_SPACE:
			str = "'{SPACEBAR}'";
			break;

		case VK_OEM_3:
			str = "'{TILDE (~)}'";
			break;
		default:
			str = str.Format("'{%c}'", MapVirtualKeyA(vKey, MAPVK_VK_TO_CHAR));
			break;
		}

		return str;
	}

	int __fastcall SettingsDlg::SettingsHotkeyCallback(H3Msg* msg)noexcept
	{
		if (msg->subtype == eMsgSubtype::LBUTTON_CLICK)
		{
			auto bttn = msg->GetDlg()->GetCustomButton(15);
			if (bttn)
			{
				auto dlg = dynamic_cast<SettingsDlg*>(msg->GetDlg());

				dlg->hk.text->SetText(dlg->text->press);

				for (auto it : dlg->dlgItems)
					it->DeActivate();

				bttn->Hide();

				dlg->Redraw();
				dlg->needRedraw = true;
			}
		}

		return false;
	}
	SettingsDlg::~SettingsDlg()
	{

	}

	void Settings::reset()
	{
		isEnabled = true;
		isHeld = false;
		vKey = VK_CONTROL;
		scanCode = eVKey::H3VK_CTRL;
		height = HP_LABEL_HEIGHT;
		fcolorFill = HP_LABEL_FILL;
		fcolorLoss = HP_LABEL_LOSS;
		fsaturation = HP_LABEL_SATURATION;
	}

	BOOL Settings::save()
	{
		constexpr int SIZE = 4;
		constexpr const char* keys[SIZE] = { "fillColor","lossColor", "colorSaturation","yLabelShift" };
		float values[SIZE] = { fcolorFill ,fcolorLoss ,fsaturation, height };

		//char buf[15];


		for (size_t i = 0; i < SIZE; i++)
		{
			Era::WriteStrToIni(keys[i], std::to_string(values[i]).erase(4).c_str(), section, iniPath);
		}

		Era::WriteStrToIni("enabled", std::to_string(static_cast<bool>(isEnabled)).c_str(), section, iniPath);
		Era::WriteStrToIni("held", std::to_string(static_cast<bool>(isHeld)).c_str(), section, iniPath);
		Era::WriteStrToIni("keyCode", std::to_string(vKey).c_str(), section, iniPath);


		Era::SaveIni(iniPath);

		return 0;
	}

	BOOL Settings::load()
	{

		sprintf(h3_TextBuffer, "%d", 1); // set default buffer
		if (Era::ReadStrFromIni("enabled", section, iniPath, h3_TextBuffer))
			isEnabled = atoi(h3_TextBuffer);

		sprintf(h3_TextBuffer, "%d", 0); // set default buffer
		if (Era::ReadStrFromIni("held", section, iniPath, h3_TextBuffer))
			isHeld = atoi(h3_TextBuffer);

		sprintf(h3_TextBuffer, "%d", MapVirtualKeyA(VK_CONTROL, MAPVK_VK_TO_CHAR)); // set default buffer
		if (Era::ReadStrFromIni("keyCode", section, iniPath, h3_TextBuffer))
		{
			int tempVirtualKey = atoi(h3_TextBuffer);
			int _scanCode = MapVirtualKeyA(tempVirtualKey, MAPVK_VK_TO_VSC);
			if (validateScanCode(eVKey(_scanCode)))
			{
				vKey = tempVirtualKey;
				scanCode = _scanCode;
			}
		}


		constexpr int SIZE = 4;
		constexpr const char* keys[SIZE] = { "fillColor","lossColor", "colorSaturation","yLabelShift" };
		float* values[SIZE] = { &fcolorFill , &fcolorLoss, &fsaturation, &height };
		//Era::ReadStrFromIni("1", "12","Runtime/game_enhancement_mod.ini", "12");
		sprintf(h3_TextBuffer, "%.2f", -1.0f); // set default buffer

		for (size_t i = 0; i < SIZE; i++)
		{
			if (Era::ReadStrFromIni(keys[i], section, iniPath, h3_TextBuffer))
			{
				double temp = atof(h3_TextBuffer);
				if (i == 3)
				{
					if (temp >= -HP_LABEL_MAX_OFFSET && temp <= HP_LABEL_MAX_OFFSET)
					{
						*values[i] = temp;
					}
				}
				else
				{
					if (temp >= 0.0f && temp <= 1.f)
					{
						*values[i] = temp;

					}
				}
			}

		}

		return 0;
	}
}
