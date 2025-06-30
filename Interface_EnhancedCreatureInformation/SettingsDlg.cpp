#include "SettingsDlg.h"
extern Settings settings;

BOOL SettingsDlg::DialogProc(H3Msg& msg)
{
	//HitPointsBarDraw(label);
	if (needRedraw)
	{
		needRedraw = false;
		HitPointsBarDraw(labelForHp);
		// draw blue hp

		LabelBarDraw(originalLabel);

		GetH3DlgItem(9)->DrawTempFrame(1, 12, 12, 12);

	}
	if (msg.command == eMsgCommand::MOUSE_WHEEL
		|| msg.command == eMsgCommand::WHEEL_BUTTON)
	{
		return 0;
	}


	if (!hotkeyChoiceCallbackBttn->IsVisible())
	{
		if (msg.IsRightClick())
		{

		}

		if (msg.IsKeyDown())
		{
			for (auto it : dlgItems)
				it->Activate();
			eVKey scanCode = msg.GetKey();

			if (settings.validateScanCode(scanCode))
			{
				settings.vKey = MapVirtualKey(scanCode, MAPVK_VSC_TO_VK);
				//	str = H3String::Format("\'{%s}\'", getVirtualKeyName(settings.vKey).String());
				hotkeyName->SetText(getVirtualKeyName(settings.vKey));
				hotkeyName->Refresh();
				hotkeyName->ParentRedraw();
			}
			else
			{
				H3Messagebox::RMB(this->text->wrong);
			}

			hotkeyChoiceCallbackBttn->Show();
			Redraw();
			HitPointsBarDraw(labelForHp);
			LabelBarDraw(originalLabel);

		}

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
			}
			else if (itemId == 9)
			{

				//reset settings values
				settings.reset();
				constexpr int SIZE = 4;
				float values[SIZE] = { settings.fcolorFill ,settings.fcolorLoss ,settings.fsaturation, settings.height };
				H3DlgScrollbar_proc procs[SIZE] = { ColorFillScrollBarProc ,ColorLossScrollBarProc,SaturationScrollBarProc, HeightScrollBarProc };

				// set correct postitions
				for (size_t i = 0; i < SIZE; i++)
				{
					auto scrollBar = GetScrollbar(100 + i);
					scrollBar->SetTick(values[i] * static_cast<float>(scrollBar->GetTicksCount()));
					if (i == 3) (scrollBar->SetTick(values[i]));
					scrollBar->SetButtonPosition();
					procs[i](scrollBar->GetTick(), this);
					scrollBar->ParentRedraw();
				}
				bttn = GetDefButton(7);
				bttn->SetFrame(settings.isEnabled);
				bttn->SetClickFrame(settings.isEnabled);
				bttn = GetDefButton(8);
				bttn->SetFrame(settings.isHeld);
				bttn->SetClickFrame(settings.isHeld);
				hotkeyName->SetText(getVirtualKeyName(settings.vKey));

			}

			//show changes
			Redraw();
			// return changed label view
			HitPointsBarDraw(labelForHp);
			LabelBarDraw(originalLabel);
		}
	}

	return 0;
}


void __fastcall SettingsDlg::ScrollBarGeneralProc(H3BaseDlg* dlg, INT32 scrollBarId, INT32 txtId, float& valuePtr)
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


void __fastcall SettingsDlg::ColorFillScrollBarProc(INT32 value, H3BaseDlg* dlg)
{
	ScrollBarGeneralProc(dlg, 100, 100, settings.fcolorFill);
}

void __fastcall SettingsDlg::ColorLossScrollBarProc(INT32 value, H3BaseDlg* dlg)
{
	ScrollBarGeneralProc(dlg, 101, 101, settings.fcolorLoss);
}

void __fastcall SettingsDlg::SaturationScrollBarProc(INT32 value, H3BaseDlg* dlg)
{
	ScrollBarGeneralProc(dlg, 102, 102, settings.fsaturation);
}

void __fastcall SettingsDlg::HeightScrollBarProc(INT32 value, H3BaseDlg* dlg)
{
	auto hpBar = dlg->GetPcx(5);
	auto label = dlg->GetPcx(6);
	if (hpBar && label)
	{
		hpBar->SetY(label->GetY() - 1 - value);
		dlg->Redraw();
		ScrollBarGeneralProc(dlg, 103, 103, settings.height);
		LabelBarDraw(label);
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

		originalLabel = H3DlgPcx::Create(labelForHp->GetX(), labelForHp->GetY() + 1 + settings.height, 6, barPcxName);
		if (originalLabel) AddItem(originalLabel);

		//glob = new Settings{};

		constexpr int SIZE = 4;
		float values[SIZE] = { settings.fcolorFill ,settings.fcolorLoss ,settings.fsaturation, settings.height };
		int ticks[SIZE] = { 100, 100, 10, labelForHp->GetHeight() + 2 };
		LPCSTR hints[SIZE] = { text->health,text->loss, text->density,text->height };
		H3DlgScrollbar_proc procs[SIZE] = { ColorFillScrollBarProc ,ColorLossScrollBarProc,SaturationScrollBarProc, HeightScrollBarProc };
		H3DlgText* description;
		for (size_t i = 0; i < SIZE; i++)
		{
			H3DlgScrollbar* scrollBar = H3DlgScrollbar::Create(20, 30 * i + originalLabel->GetY() + 25, 148, 16, i + 100, ticks[i], procs[i], false, 2, true);

			//disable catch keys
			IntAt(reinterpret_cast<int>(scrollBar) + 0x5C) = NULL;
			if (scrollBar) AddItem(scrollBar);
			scrollBar->SetTick(values[i] * (float)ticks[i]);
			if (i == 3) (scrollBar->SetTick(values[i]));
			scrollBar->SetButtonPosition();

			description = H3DlgText::Create(scrollBar->GetX() + scrollBar->GetWidth() + 12, scrollBar->GetY(), 224, 16, hints[i], NH3Dlg::Text::MEDIUM, 7, i + 100, eTextAlignment::MIDDLE_LEFT);
			if (description) AddItem(description);
		}

		H3DlgDefButton* bttn = H3DlgDefButton::Create(description->GetX(), 25, 7, NH3Dlg::Assets::ON_OFF_CHECKBOX, settings.isEnabled, settings.isEnabled, false, -1);
		if (bttn)  AddItem(bttn);

		H3DlgText* dlgText = H3DlgText::Create(bttn->GetX() + bttn->GetWidth() + 12, bttn->GetY(), 200, bttn->GetHeight(), text->enable, NH3Dlg::Text::MEDIUM, 7, 0, eTextAlignment::MIDDLE_LEFT);
		if (dlgText) AddItem(dlgText);


		bttn = H3DlgDefButton::Create(description->GetX(), 55, 8, NH3Dlg::Assets::ON_OFF_CHECKBOX, settings.isHeld, settings.isHeld, false, -1);
		if (bttn) AddItem(bttn);

		dlgText = H3DlgText::Create(bttn->GetX() + bttn->GetWidth() + 12, bttn->GetY(), 200, bttn->GetHeight(), text->toggable, NH3Dlg::Text::MEDIUM, 7, 0, eTextAlignment::MIDDLE_LEFT);
		if (dlgText) AddItem(dlgText);

		//getVirtualKeyName(settings.vKey).

		auto fnt = H3MediumFont::Get();
		H3String keyName(getVirtualKeyName(settings.vKey));
		int textWidth = fnt->GetMaxLineWidth("'{CAPS LOCK}'");
		CreateBlackBox(bttn->GetX(), 85, textWidth, bttn->GetHeight());

		hotkeyName = H3DlgText::Create(bttn->GetX(), 85, textWidth, bttn->GetHeight(), keyName.String(), fnt->GetName());
		AddItem(hotkeyName);
		hotkeyChoiceCallbackBttn = H3DlgCustomButton::Create(bttn->GetX() + 123, 85, 15, "iam017.DEF", SettingsHotkeyCallback, 0, 1);
		AddItem(hotkeyChoiceCallbackBttn);


		// create default bttn
		bttn = H3DlgDefButton::Create(100 - okBttn->GetWidth(), okBttn->GetY(), 9, "wogbttn.def", 12, 13, false, -1);
		if (bttn)
		{
			bttn->SetHint(text->default);
			AddItem(H3DlgPcx::Create(bttn->GetX() - 1, bttn->GetY() - 1, NH3Dlg::Assets::BOX_64_30_PCX));
			AddItem(bttn);
		}


	}
}

int __fastcall SettingsDlg::SettingsHotkeyCallback(H3Msg* msg)noexcept
{
	if (msg->subtype == eMsgSubtype::LBUTTON_CLICK)
	{

		auto bttn = msg->GetDlg()->GetCustomButton(15);
		if (bttn)
		{
			H3Vector<H3DlgItem*> items = msg->GetDlg()->GetList();
			for (auto it : items)
			{
				it->DeActivate();
			}
			bttn->Hide();
			bttn->ParentRedraw();
			auto dlg = msg->GetDlg();
			dlg->Redraw();
			auto label = dlg->GetPcx(5);
			auto originalLabel = dlg->GetPcx(6);

			HitPointsBarDraw(label);
			LabelBarDraw(originalLabel);
		}

	}

	return false;

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


SettingsDlg::~SettingsDlg()
{

}