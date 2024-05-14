#include "pch.h"
#include "CombatHints.h"

BOOL CombatHints::isInited = false;
CombatHints* CombatHints::combatHints = nullptr;

constexpr float HP_LABEL_HEIGHT = 4.0f;
constexpr float HP_LABEL_FILL = 0.4f;
constexpr float HP_LABEL_LOSS = 0.975f;
constexpr float HP_LABEL_SATURATION = 0.8f; // idk.mb set 1

#pragma comment(lib, "User32.lib")

Settings settings;

CombatHints& CombatHints::instance(PatcherInstance* _PI)
{
	if (combatHints == nullptr)
	{
		combatHints = new CombatHints(_PI);

		settings.iniPath = "./Runtime/game_enhancement_mod.ini";
		settings.section = "CombatHints";
		settings.reset();
		settings.load();
		//	SettingsDlg::text;
			//combatHints
	}
	return *combatHints;
}

//void CombatHints::Init()
//{
//
//
//}

CombatHints::CombatHints(PatcherInstance* _PI)
{
	if (isInited)
		return;

	_PI->WriteLoHook(0x43E38B, BeforebattleStackHintDraw);
	_PI->WriteLoHook(0x4682C0, BattleOptionsDlg);
	_PI->WriteLoHook(0x4746BD, BattleMgr_ProcessActionL);

	using namespace Era;

	dlgText = new DlgText{
		tr("gem_plugin.combat_hints.rmc_hint"),
		tr("gem_plugin.combat_hints.handlers"),
		tr("gem_plugin.combat_hints.input"),
		tr("gem_plugin.combat_hints.color_health"),
		tr("gem_plugin.combat_hints.color_loss"),
		tr("gem_plugin.combat_hints.height"),
		tr("gem_plugin.combat_hints.saturation"),
		tr("gem_plugin.combat_hints.default"),
		tr("gem_plugin.combat_hints.hotkey"),
		tr("gem_plugin.combat_hints.wrong_hotkey"),
		tr("gem_plugin.combat_hints.enable"),
		tr("gem_plugin.combat_hints.toggable"),
		tr("gem_plugin.combat_hints.set_hotkey"),
		tr("gem_plugin.combat_hints.press_any"),
		tr("gem_plugin.combat_hints.held")
	};

	isInited = true;
}
bool test = false;

_LHF_(CombatHints::BattleMgr_ProcessActionL)
{
	H3Msg* msg = reinterpret_cast<H3Msg*>(c->eax);
	if (msg && settings.isHeld)
	{
		using namespace Era;
		Era::y[22] = msg->IsKeyHeld();
		//Era::y[23] = msg->IsKeyPress();
		Era::y[24] = msg->IsKeyDown();



		if (msg->GetKey() == settings.scanCode && (msg->IsKeyDown()|| msg->IsKeyHeld()) && combatHints->needRedraw>-1)
		{
			combatHints->needRedraw = true;
		}


		if (msg->IsKeyPress() && msg->GetKey() == settings.scanCode)
		{
			//combatHints->needRedraw = true;
			THISCALL_1(void, 4684800, H3CombatManager::Get());
			combatHints->needRedraw = 0;

		}

	}

	return EXEC_DEFAULT;
}

_LHF_(CombatHints::BeforebattleStackHintDraw)
{
	H3CombatCreature* stack = reinterpret_cast<H3CombatCreature*>(c->ebx);

	if (stack && stack->numberAlive && settings.isEnabled)
	{


		if (settings.isHeld && !(GetKeyState(settings.vKey) & 0x800))

			return EXEC_DEFAULT; // return

		int height = static_cast<int>(settings.height);

		int x = c->edi;
		int y = c->esi;
		int s = y - height - 1;

		H3CombatManager* cmb = H3CombatManager::Get();

		bool needToDraw = THISCALL_4(bool, 0x495460, cmb, cmb->cmNumWinPcxLoaded, x, s);// draw hint back
		if (!needToDraw) return EXEC_DEFAULT;

		x += cmb->dlg->GetX() + 1;
		y += cmb->dlg->GetY() - height;
		int maxHp = stack->MaxHitPoints();

		if (maxHp)
		{
			float fillPartF = (maxHp - stack->healthLost) * 28 / maxHp * 10;
			bool remainder = fillPartF > 4;

			int filledPart = fillPartF / 10 + remainder;
			int hpBarLimit = CharAt(0x43E45B + 1);
			filledPart = Clamp(0, filledPart, hpBarLimit);


			WindMgr_DrawColoredRect(x, y, filledPart, &settings);

			if (filledPart < hpBarLimit)
			{
				WindMgr_DrawColoredRect(x + filledPart, y, filledPart * -1 + hpBarLimit, &settings, true);
			}
			if (settings.isHeld && combatHints->needRedraw >0)
			{
				combatHints->needRedraw = -1;
				THISCALL_1(void, 0x477C00, cmb);
			}

		}

	}

	return EXEC_DEFAULT;
}





int __fastcall CombatHints::CombatOptionsCallback(H3Msg* msg)noexcept
{

	if (msg->subtype == eMsgSubtype::LBUTTON_CLICK)
	{
		SettingsDlg dlg(450, 450, &settings, combatHints->dlgText);
		dlg.Start();
		settings.save();

		msg->GetDlg()->Redraw();
	}

	return false;

}




_LHF_(CombatHints::BattleOptionsDlg)
{
	H3BaseDlg* dlg = reinterpret_cast<H3BaseDlg*>(c->ecx);
	if (dlg)
	{
		int x, y;
		if (IntAt(0x46DF20 + 1) == dlg->GetHeight())
		{
			auto def = dlg->GetDefButton(30722);
			x = def->GetX() + (def->GetWidth()>>1);
			y = def->GetY() - 80;
		}
		else
		{
			auto def = dlg->GetDefButton(201);
			x = def->GetX() - 100;
			y = def->GetY();
		}
		H3DlgCustomButton* bttn = H3DlgCustomButton::Create(x, y, "ScnrAll.DEF", CombatOptionsCallback, 0, 1);

		
		//	H3DlgScrollbar * scrollBar = H3DlgScrollbar
		dlg->AddItem(bttn);
	}

	return EXEC_DEFAULT;
}

void CombatHints::WindMgr_DrawColoredRect(int x, int y, int width, Settings* stg, bool lost) noexcept
{
	THISCALL_7(void, 0x44E610, P_WindowManager->GetDrawBuffer(), x, y, width, static_cast<int>(stg->height), lost ? stg->fcolorLoss : stg->fcolorFill, stg->fsaturation);
}


CombatHints::CombatHints()
{
}
CombatHints::~CombatHints()
{
	if (dlgText)
	{
		delete dlgText;
		dlgText = nullptr;
	}
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
		valuePtr = scrollBarId < 103 ? tick / scroll->GetTicksCount() : tick;
		// draw label changes
		HitPointsBarDraw(label);
	}
}

void SettingsDlg::HitPointsBarDraw(H3DlgItem* label)noexcept
{
	constexpr int lossValue = 16;
	//draw label colors
	CombatHints::WindMgr_DrawColoredRect(label->GetAbsoluteX() + 1, label->GetAbsoluteY() + 1, lossValue - 1, &settings);
	CombatHints::WindMgr_DrawColoredRect(label->GetAbsoluteX() + lossValue, label->GetAbsoluteY() + 1, label->GetWidth() - lossValue - 1, &settings, true);

	// show changes
	label->Refresh();
	label->ParentRedraw();

}

void SettingsDlg::LabelBarDraw(H3DlgItem* originalLabel)noexcept
{
	int x = originalLabel->GetAbsoluteX();
	int y = originalLabel->GetAbsoluteY();
	int width = originalLabel->GetWidth();
	// draw label colors
	THISCALL_7(void, 0x44E610, P_WindowManager->GetDrawBuffer(), x + 1, y + 1, width - 2, originalLabel->GetHeight() - 2, FloatAt(0x43E3AC + 1), FloatAt(0x43E3A7 + 1));
	//draw text over the color
	THISCALL_10(void, 0x4B51F0, *reinterpret_cast<H3Font**>(0x698A54), "505", P_WindowManager->GetDrawBuffer(), x, y, width, 15, 4, 1, -1);

	// show changes
	originalLabel->Refresh();
	originalLabel->ParentRedraw();
}

void __fastcall SettingsDlg::ColorFillScrollBarProc(INT32 value, H3BaseDlg* dlg)
{
	ScrollBarGeneralProc(dlg, 100, settings.fcolorFill);
}

void __fastcall SettingsDlg::ColorLossScrollBarProc(INT32 value, H3BaseDlg* dlg)
{
	ScrollBarGeneralProc(dlg, 101, settings.fcolorLoss);
}

void __fastcall SettingsDlg::SaturationScrollBarProc(INT32 value, H3BaseDlg* dlg)
{
	ScrollBarGeneralProc(dlg, 102, settings.fsaturation);
}

void __fastcall SettingsDlg::HeightScrollBarProc(INT32 value, H3BaseDlg* dlg)
{
	auto hpBar = dlg->GetPcx(5);
	auto label = dlg->GetPcx(6);
	if (hpBar && label)
	{
		hpBar->SetY(label->GetY() - 1 - value);
		dlg->Redraw();
		ScrollBarGeneralProc(dlg, 103, settings.height);
		LabelBarDraw(label);
	}
}



BOOL SettingsDlg::DialogProc(H3Msg& msg)
{

	if (needRedraw)
	{
		needRedraw = false;
		HitPointsBarDraw(labelForHp);
		LabelBarDraw(originalLabel);
	}


	if (!hk.bttn->IsVisible() && msg.IsKeyDown() )
	{
		for (auto it : dlgItems)
			it->Activate();

		if (msg.IsKeyDown())
		{

			eVKey scanCode = msg.GetKey();

			if (settings.validateScanCode(scanCode))
			{
				settings.vKey = MapVirtualKey(scanCode, MAPVK_VSC_TO_VK);
				settings.scanCode = scanCode;
				//	str = H3String::Format("\'{%s}\'", getVirtualKeyName(settings.vKey).String());
				hk.name->SetText(getVirtualKeyName(settings.vKey));
				hk.text->SetText(text->setHk);
			}
			else
				H3Messagebox::RMB(this->text->wrong);
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
				BOOL* optionPtr = itemId == 7 ? &settings.isEnabled : &settings.isHeld;
				*optionPtr ^= 1;
				bttn->SetFrame(*optionPtr);
				bttn->SetClickFrame(*optionPtr);
				bttn->Draw();
				bttn->Refresh();
			}
			else if (itemId == 9)
			{

				//reset settings values
				settings.reset();
				constexpr UINT16 SIZE = 4;
				float values[SIZE] = { settings.fcolorFill ,settings.fcolorLoss ,settings.fsaturation, settings.height };
				H3DlgScrollbar_proc procs[SIZE] = { ColorFillScrollBarProc ,ColorLossScrollBarProc,SaturationScrollBarProc, HeightScrollBarProc };

				// set correct postitions
				for (UINT16 i = 0; i < SIZE; i++)
				{
					auto scrollBar = GetScrollbar(100 + i);
					scrollBar->SetTick(static_cast<int>(values[i] * static_cast<float>(scrollBar->GetTicksCount())));
					if (i == 3) (scrollBar->SetTick(static_cast<int>(values[i])));
					scrollBar->SetButtonPosition();
					procs[i](scrollBar->GetTick(), this);
					scrollBar->ParentRedraw();
				}
				bttn = GetDefButton(7);
				bttn->SetFrame(settings.isEnabled);
				bttn->SetClickFrame(settings.isEnabled);

				//bttn->Draw();
				//bttn->Refresh();

				bttn = GetDefButton(8);
				bttn->SetFrame(settings.isHeld);
				bttn->SetClickFrame(settings.isHeld);

				hk.name->SetText(getVirtualKeyName(settings.vKey));
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
	auto okBttn = CreateOK32Button(widthDlg - 100, heightDlg - 80);
	if (okBttn)		okBttn->AddHotkey(eVKey::H3VK_ESCAPE);
	auto cmb = H3CombatManager::Get();
	LPCSTR barPcxName = cmb->cmNumWinPcxLoaded->GetName();

	//H3DlgDef* creatureDef = H3DlgDef::Create(1, 25, 100, 130, 12, P_CreatureInformation[12].defName, 0, 2);

	H3DlgDef* d = H3ObjectAllocator<H3DlgDef>().allocate(1);
	H3DlgDef* creatureDef = THISCALL_12(H3DlgDef*, 0x4EA800, d, 40, 0, 100, 130, 225, P_CreatureInformation[eCreature::HORNED_DEMON].defName, 0, 2, 0, 0, 0x12);

	//THISCALL_16(H3DlgDef *,0x4EA800, d,)
	AddItem(creatureDef);

	labelForHp = H3DlgPcx::Create(creatureDef->GetX() + creatureDef->GetWidth() - 25, creatureDef->GetY() + creatureDef->GetHeight() - 30, 5, barPcxName);
	if (labelForHp)
	{
		//H3RGB888 color{ 12,12,12 };
		AddItem(labelForHp);

		originalLabel = H3DlgPcx::Create(labelForHp->GetX(), labelForHp->GetY() + 1 + static_cast<int>(settings.height), 6, barPcxName);
		if (originalLabel) AddItem(originalLabel);

		//glob = new Settings{};

		constexpr int SIZE = 4;
		float values[SIZE] = { settings.fcolorFill ,settings.fcolorLoss ,settings.fsaturation, settings.height };
		int ticks[SIZE] = { 100, 100, 10, labelForHp->GetHeight() + 2 };
		LPCSTR hints[SIZE] = { text->health,text->loss, text->density,text->height };
		H3DlgScrollbar_proc procs[SIZE] = { ColorFillScrollBarProc ,ColorLossScrollBarProc,SaturationScrollBarProc, HeightScrollBarProc };
		H3DlgText* description;
		
		eTextColor textColor = eTextColor(7);// eTextColor::REGULAR;
		//background->DrawShadow(160, originalLabel->GetY() + 20, widthDlg - 60, heightDlg / 3);
		
		H3DlgText* dlgText = CreateText( 168 , originalLabel->GetY() + 30, 200, 24, text->handlers, NH3Dlg::Text::BIG, textColor, 0, eTextAlignment::MIDDLE_LEFT);

		for (size_t i = 0; i < SIZE; i++)
		{
			H3DlgScrollbar* scrollBar = H3DlgScrollbar::Create(20, 30 * i + dlgText->GetY() + 25, 148, 16, i + 100, ticks[i], procs[i], false, 2, true);

			//disable catch keys
			IntAt(reinterpret_cast<int>(scrollBar) + 0x5C) = NULL;
			if (scrollBar) AddItem(scrollBar);
			scrollBar->SetTick(values[i] * (float)ticks[i]);
			if (i == 3) (scrollBar->SetTick(values[i]));
			scrollBar->SetButtonPosition();
			description = H3DlgText::Create(scrollBar->GetX() + scrollBar->GetWidth() + 12, scrollBar->GetY(), 224, 16, hints[i], NH3Dlg::Text::MEDIUM, textColor, i + 100, eTextAlignment::MIDDLE_LEFT);
			background->SinkArea(description->GetX() -5, description->GetY(), description->GetWidth() +10, description->GetHeight());
			if (description) AddItem(description);
		}



		textColor = eTextColor::REGULAR;
		H3DlgDefButton* bttn = H3DlgDefButton::Create(description->GetX(), 25, 7, NH3Dlg::Assets::ON_OFF_CHECKBOX, settings.isEnabled, settings.isEnabled, false, -1);
		if (bttn)  AddItem(bttn);

		dlgText = CreateText(bttn->GetX() + bttn->GetWidth() + 12, bttn->GetY(), 200, bttn->GetHeight(), text->enable, NH3Dlg::Text::MEDIUM, textColor, 0, eTextAlignment::MIDDLE_LEFT);
		SinkItem(background, dlgText);
		bttn = CreateButton(description->GetX(), 55, 8, NH3Dlg::Assets::ON_OFF_CHECKBOX, settings.isHeld, settings.isHeld, false, -1);
		dlgText = CreateText(bttn->GetX() + bttn->GetWidth() + 12, bttn->GetY(), 200, bttn->GetHeight(), text->held, NH3Dlg::Text::MEDIUM, textColor, 0, eTextAlignment::MIDDLE_LEFT);
		SinkItem(background, dlgText);

		//getVirtualKeyName(settings.vKey).

		auto fnt = H3MediumFont::Get();
		int textWidth = fnt->GetMaxLineWidth("'{CAPS LOCK}'");
		int textX = widthDlg - textWidth - 16;
		int textY = 85;

		hk.bttn = CreateCustomButton(bttn->GetX(), textY, 15, "iam009.DEF", SettingsHotkeyCallback, 0, 1);
		bttn = hk.bttn;
		hk.text = CreateText(bttn->GetX() + bttn->GetWidth() + 12, bttn->GetY(), 200, bttn->GetHeight(), text->setHk, NH3Dlg::Text::MEDIUM, textColor, 0, eTextAlignment::MIDDLE_LEFT);
		SinkItem(background, hk.text);

		CreateBlackBox(textX, textY, textWidth, bttn->GetHeight());

		hk.name = CreateText(textX, textY, textWidth, bttn->GetHeight(), getVirtualKeyName(settings.vKey).String(), fnt->GetName(), eTextColor::WHITE, 17);
		//SinkItem(background, hk.name);

		//h3::RGB565 color = eTextColor::GOLD;

		//CreateFrame(hotkeyName, color);

		// create default bttn
		bttn = H3DlgDefButton::Create(100 - okBttn->GetWidth(), okBttn->GetY(), 9, "wogbttn.def", 12, 13, false, -1);
		if (bttn)
		{
			bttn->SetHint(text->dfltName);
			CreatePcx(bttn->GetX() - 1, bttn->GetY() - 1, 15, NH3Dlg::Assets::BOX_64_30_PCX);
			AddItem(bttn);
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
	float values[SIZE] = { settings.fcolorFill ,settings.fcolorLoss ,settings.fsaturation, settings.height };

	//char buf[15];


	const char* _section = section.String();
	for (size_t i = 0; i < SIZE; i++)
	{
		Era::WriteStrToIni(keys[i], std::to_string(values[i]).erase(4).c_str(), _section, iniPath.String());
	}

	Era::WriteStrToIni("enabled", std::to_string(static_cast<bool>(isEnabled)).c_str(), _section, iniPath.String());
	Era::WriteStrToIni("held", std::to_string(static_cast<bool>(isHeld)).c_str(), _section, iniPath.String());
	Era::WriteStrToIni("keyCode", std::to_string(vKey).c_str(), _section, iniPath.String());


	Era::SaveIni(iniPath.String());

	return 0;
}

BOOL Settings::load()
{
	const char* _section = section.String();

	sprintf(h3_TextBuffer, "%d", 1); // set default buffer
	if (Era::ReadStrFromIni("enabled", _section, iniPath.String(), h3_TextBuffer))
		isEnabled = atoi(h3_TextBuffer);

	sprintf(h3_TextBuffer, "%d", 0); // set default buffer
	if (Era::ReadStrFromIni("held", _section, iniPath.String(), h3_TextBuffer))
		isHeld = atoi(h3_TextBuffer);

	sprintf(h3_TextBuffer, "%d", MapVirtualKeyA(VK_CONTROL, MAPVK_VK_TO_CHAR)); // set default buffer
	if (Era::ReadStrFromIni("keyCode", _section, iniPath.String(), h3_TextBuffer))
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
	float* values[SIZE] = { &settings.fcolorFill , &settings.fcolorLoss, &settings.fsaturation, &settings.height };
	//Era::ReadStrFromIni("1", "12","Runtime/game_enhancement_mod.ini", "12");
	sprintf(h3_TextBuffer, "%.2f", -1.0f); // set default buffer

	for (size_t i = 0; i < SIZE; i++)
	{
		if (Era::ReadStrFromIni(keys[i], _section, iniPath.String(), h3_TextBuffer))
		{
			float temp = atof(h3_TextBuffer);
			if (temp >= 0.0f)
				*values[i] = temp;
		}

	}

	return 0;
}
