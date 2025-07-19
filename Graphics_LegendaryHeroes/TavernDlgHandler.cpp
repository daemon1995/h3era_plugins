#include "pch.h"

namespace lh
{
    constexpr const char* backgroundPcxName = "nhTvrnBg.pcx";
    constexpr const char* hintbarPcx = "nhtvrhnt.pcx";
    constexpr const char* framePcxName = "nhtvfrm.pcx";
    constexpr const char* selectionFramePcxName = "nhtvsclt.pcx";
    constexpr const char* videoFrame = "nhTvrvfr.pcx";
    constexpr const char* goldBack = "nhTvrGld.pcx";

}; // namespace lh

TavernDlgHandler* TavernDlgHandler::instance = nullptr;

TavernDlgHandler::TavernDlgHandler(PatcherInstance* pi) : IGamePatch(pi)
{
}

void TavernDlgHandler::Init(PatcherInstance* _pi, H3LoadedPcx16* drawBuffer[2]) noexcept
{
    if (!instance)
    {
        instance = new TavernDlgHandler(_pi);
        if (instance)

        {
            instance->m_drawBuffer[0] = drawBuffer[0];
            instance->m_drawBuffer[1] = drawBuffer[1];
            instance->CreatePatches();
        }
    }
}
void TavernDlgHandler::CreatePatches() noexcept
{
    if (!instance->m_isInited)
    {

        instance->m_isInited = true;
        // Tavern dialog

        _pi->WriteDword(0x5D759C + 1, lh::backgroundPcxName); // set new bg pcx
        _pi->WriteDword(0x5D7638 + 1, lh::hintbarPcx);        // set new hint pcx

        _pi->WriteLoHook(0x5D74D8, BeforeCreate);
        _pi->WriteLoHook(0x5D864A, AfterCreate);

        _pi->WriteDword(0x5D768A + 1, lh::selectionFramePcxName); // set new selection frame
        _pi->WriteDword(0x5D76D9 + 1, lh::selectionFramePcxName); // set new selection frame

        // buy m_heroWgt bttn
        _pi->WriteDword(0x5D7A1E + 1, 0x682A30); // set new def IBY6432.DEF

        // thieves guild
        //	_pi->WriteDword(0x5D7A75 + 1, 0x68C3B8); // set new def
    }
}

extern "C" __declspec(dllexport) void Enable(bool state)
{
    TavernDlgHandler::instance->SetEnabled(state);
}

_LHF_(TavernDlgHandler::BeforeCreate) //(LoHook* hook, HookContext* c)
{
    auto pcx = H3LoadedPcx16::Load(lh::backgroundPcxName);

    c->Push(pcx->height);
    c->Push(pcx->width);
    int yPos = (P_WindowManager->lastDlg->GetHeight() - pcx->height) >> 1;

    c->Push(yPos);
    int xPos = (P_WindowManager->lastDlg->GetWidth() - pcx->width) >> 1;

    c->Push(xPos);
    pcx->Dereference();

    c->return_address = 0x005D74E4;
    return NO_EXEC_DEFAULT;
}

_LHF_(TavernDlgHandler::AfterCreate) //(LoHook* hook, HookContext* c)
{
    H3Dlg* dlg = *reinterpret_cast<H3Dlg**>(0x6AAA54); // get Dlg

    if (!dlg)
        return EXEC_DEFAULT;
    // set tavern video y pos

    instance->_pi->WriteByte(0x5D8230 + 1, 0x27);
    auto pcx = H3LoadedPcx16::Load(lh::videoFrame);
    if (pcx)
    {
        H3DlgPcx16* dlgPcx = H3DlgPcx16::Create(124, 0x26, pcx->width, pcx->height, 0, 0);
        dlgPcx->SetPcx(pcx);
        dlg->AddItem(dlgPcx);
    }
    if (auto headerText = dlg->GetText(1))
        headerText->SetY(headerText->GetY() - 2);

    if (auto goldBack = dlg->GetH3DlgItem(15))
    {
        goldBack->SetX(224);
        goldBack->SetY(381);
        goldBack->SetWidth(56);
    }
    if (auto goldText = dlg->GetText(4))
    {
        dlg->AddItem(H3DlgPcx::Create(224, 381, 56, 58, 15, lh::goldBack));
        goldText->SetX(224);
        goldText->SetY(419);
        goldText->SetWidth(56);

        dlg->AddItem(H3DlgText::Create(224, 419, 56, 20, goldText->GetH3String().String(), NH3Dlg::Text::SMALL, 1, 4));
    }

    if (auto descriptionText = dlg->GetText(7))
    {
        // constexpr int width = 350;
        descriptionText->SetX((dlg->GetWidth() - descriptionText->GetWidth()) >> 1);
        descriptionText->SetY(493);

        //	descriptionText->SetWidth(width);
    }
    if (auto descriptionTextBack = dlg->GetPcx(10))
    {
        // constexpr int width = 350;

        descriptionTextBack->SetX((dlg->GetWidth() - descriptionTextBack->GetWidth()) >> 1);
        descriptionTextBack->SetY(493);
        //	descriptionTextBack->SetWidth(width);
    }

    // hint bar
    if (auto hintbar = dlg->GetPcx(13))
    {
        hintbar->SetX(7);
        hintbar->SetY(dlg->GetHeight() - 26);
        hintbar->SetWidth(dlg->GetWidth() - 14);
    }
    if (auto hintbar = dlg->GetText(14))
    {
        hintbar->SetX(7);
        hintbar->SetY(dlg->GetHeight() - 26);
        hintbar->SetWidth(dlg->GetWidth() - 14);
    }

    // invite button stuff
    for (size_t i = 0; i < 3; i++)
    {
        if (auto hdModButton = dlg->GetH3DlgItem(400 + i))
        {
            hdModButton->SetX(hdModButton->GetX() + 13);
            hdModButton->SetY(hdModButton->GetY() + 103);
        }
    }

    if (auto rumor = dlg->GetText(2))
    {
        rumor->SetX(86);
        rumor->SetY(160);
        // rumor->SetWidth(443);

        // iCancel->SetY(528);
    }

    if (auto heroesForHire = dlg->GetText(3))
    {
        heroesForHire->SetWidth(70);
        heroesForHire->SetHeight(200);
        heroesForHire->SetX((dlg->GetWidth() - heroesForHire->GetWidth()) >> 1);
        heroesForHire->SetY(233);
    }

    H3RGB565 _color(189, 149, 57); // golden frame
    if (auto iCancel = dlg->GetDefButton(30720))
    {
        iCancel->SetX(421);
        iCancel->SetY(529);
        auto fr = dlg->CreateFrame(iCancel, _color, 0, 1);
        fr->DeActivate();
    }

    if (auto iGuild = dlg->GetDefButton(11))
    {
        iGuild->SetX(20);
        iGuild->SetY(529);
        auto fr = dlg->CreateFrame(iGuild, _color, 0, 1);
        fr->DeActivate();
    }
    if (auto iBuy = dlg->GetDefButton(12))
    {
        iBuy->SetX(220);
        iBuy->SetY(445);
        iBuy->SetWidth(64);
        iBuy->SetHeight(32);
        auto fr = dlg->CreateFrame(iBuy, _color, 0, 1);
        fr->DeActivate();
    }

    H3Vector<H3DlgItem*>* wgts = reinterpret_cast<H3Vector<H3DlgItem*> *>(reinterpret_cast<int>(dlg) + 0x30);
    //
    // change portraits and selection frames
    for (size_t i = 0; i < 2; i++)
    {
        if (auto portrait = dlg->GetPcx(5 + i))
        {
            portrait->SetWidth(196);
            portrait->SetHeight(260);

            portrait->SetX(i * 270 + 19);
            portrait->SetY(230);
            if (portrait->IsVisible())
            {
                const int heroId = IntAt(reinterpret_cast<DWORD>(P_Game->GetPlayer()) + i * 4 + 0x28);
                if (heroId >= 0)
                {
                    if (auto pcx = instance->CreatePortraitPcx(portrait, heroId, instance->m_drawBuffer[i]))
                    {
                        dlg->AddItem(pcx);
                    }
                }
            }
        }
        if (auto frame = dlg->GetPcx(8 + i))
        {
            frame->SetWidth(200);
            frame->SetHeight(264);

            frame->SetX(i * 270 + 17);
            frame->SetY(228);
        }
    }

    return EXEC_DEFAULT;
}

H3DlgPcx16* TavernDlgHandler::CreatePortraitPcx(H3DlgPcx* sourceItem, int heroId, H3LoadedPcx16* buffer)
{
    H3DlgPcx16* result = nullptr;
    if (sourceItem && sourceItem->GetPcx())
    {
        auto framePcx = H3LoadedPcx16::Load(lh::framePcxName);

        int frameWidth = framePcx->width;
        int frameHeight = framePcx->height;

        result = H3DlgPcx16::Create(sourceItem->GetX(), sourceItem->GetY(), frameWidth, frameHeight,
            sourceItem->GetID(), nullptr);
        if (result)
        {
            HeroLook::CorrectPcxNames(heroId);
            for (size_t i = 0; i < 2; i++)
            {
                auto sourcePcx = H3LoadedPcx16::Load(HeroLook::pcxNames[i].String());

                const int srcX = ((sourcePcx->width - frameWidth) >> 1) + 3;
                constexpr int srcY = 50; // (sourcePcx->height - frameHeight) >> 1;
                sourcePcx->DrawToPcx16(0, 0, 1, buffer, srcX, srcY);
                sourcePcx->Dereference();
            }

            framePcx->DrawToPcx16(0, 0, 1, buffer, 0, 0);
            result->SetPcx(buffer);
        }
        framePcx->Dereference();
    }

    return result;
}