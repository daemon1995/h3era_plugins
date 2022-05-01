#include "functions.h"    


using namespace Era;
using namespace h3;


int __stdcall OnAdventureDlgCreate(LoHook* h, HookContext* c)
{

    if (o_HD_X > 810)
    {
        h3::H3ResourceBarPanel* dlg = (H3ResourceBarPanel*)c->eax;
        if (dlg)
        {

            H3SEHandler seh;// exception handler - try create items
            try
            {

                int saveY = Era::y[1]; // save ERA y1 var;
             
                int meId = Era::y[1] = P_Game->GetPlayerID(); 
                Era::ExecErmCmd("OW:Ry1/7/?y1");

                H3RGB565 shadowColor = P_PlayersPalette->color[meId * 32 +1];

                H3DlgItem* mithrilFrameShadowL = H3DlgFrame::Create(601, 3, 1, 17, MITHRIL_FRAME_ID, shadowColor);
                dlg->AddItem(mithrilFrameShadowL);   mithrilFrameShadowL->Show();

                H3DlgItem* mithrilFrameShadowR = H3DlgFrame::Create(688, 3, 1, 17, MITHRIL_FRAME_ID + 1, shadowColor);
                dlg->AddItem(mithrilFrameShadowR);   mithrilFrameShadowR->Show();

                shadowColor = P_PlayersPalette->color[meId * 32];
                H3DlgItem* mithrilFrameShadowUp = H3DlgFrame::Create(601, 3, 87, 1, MITHRIL_FRAME_ID + 2, shadowColor);
                dlg->AddItem(mithrilFrameShadowUp);   mithrilFrameShadowUp->Show();

                H3DlgItem* mithrilFrameShadowDown = H3DlgFrame::Create(601, 19, 87, 1, MITHRIL_FRAME_ID + 3, shadowColor);
                dlg->AddItem(mithrilFrameShadowDown);   mithrilFrameShadowDown->Show();


                H3RGB565 frameColor = P_PlayersPalette->color[meId * 32 + 30];

                H3DlgItem* mithrilFrameL = H3DlgFrame::Create(600, 2, 1, 17, MITHRIL_FRAME_ID + 4, frameColor);
                dlg->AddItem(mithrilFrameL);   mithrilFrameL->Show();

                H3DlgItem* mithrilFrameR = H3DlgFrame::Create(687, 2, 1, 17, MITHRIL_FRAME_ID + 5, frameColor);
                dlg->AddItem(mithrilFrameR);   mithrilFrameR->Show();
                frameColor = P_PlayersPalette->color[meId * 32 + 31];

                H3DlgItem* mithrilFrameUp = H3DlgFrame::Create(600, 2, 88, 1, MITHRIL_FRAME_ID + 6, frameColor);
                dlg->AddItem(mithrilFrameUp);   mithrilFrameUp->Show();

                H3DlgItem* mithrilFrameDown = H3DlgFrame::Create(600, 18, 88, 1, MITHRIL_FRAME_ID + 7, frameColor);
                dlg->AddItem(mithrilFrameDown);   mithrilFrameDown->Show();

                H3DlgItem* mithrilDef = H3DlgDef::Create(601, 3, 16, 10, MITHRIL_DEF_ID, "SMALmith.def");
                dlg->AddItem(mithrilDef); mithrilDef->Show();

                H3DlgItem* mithriText = H3DlgText::Create(630, 3, 78, 22, std::to_string(Era::y[1]).c_str(), "smalfont.fnt", eTextColor::WHITE, MITHRIL_TEXT_ID, eTextAlignment::HLEFT);
                dlg->AddItem(mithriText); mithriText->Show();

                Era::y[1] = saveY;

            }
            catch (const H3Exception & e)
            {

                e.ShowInGame();
                Era::ExecErmCmd("IF:M^Disable \"NewAdvDlgItems.era\" plugin from Game Enhancement Mod\\eraplugins \
                    and report about that to daemon_n in Discord ^");

                //*
            }

        }
    }
    return EXEC_DEFAULT;
}

int __stdcall OnResourceBarDlgUpdate(LoHook* h, HookContext* c)
{
    // P_AdventureManager->dlg->CreateBlackBox(10, 10, 25, 12);

    if (o_HD_X > 810)
    {
        H3BaseDlg* dlg = (H3BaseDlg*)c->ecx;

        if (dlg)
        {
            H3DlgText* myText = dlg->GetText(MITHRIL_TEXT_ID);
            if (myText)
            {
                // Era::y[1] = THISCALL_1(INT, 0x4CE6E0, *(INT*)0x699538);
                int saveY = Era::y[1];
                Era::y[1] = P_Game->GetPlayerID();
                Era::ExecErmCmd("OW:Ry1/7/?y1");

                myText->SetText(std::to_string(Era::y[1]).c_str());
                Era::y[1] = saveY;
            }
        }
    }

    return EXEC_DEFAULT;
}


void SetFrameColor( H3BaseDlg* advDlg, int playerId)
{
    H3RGB565 color = P_PlayersPalette->color[playerId * 32 + 1];
    H3DlgFrame* frameIt = advDlg->GetFrame(MITHRIL_FRAME_ID);
    frameIt->ChangeColor(color);

    frameIt = advDlg->GetFrame(MITHRIL_FRAME_ID + 1);
    frameIt->ChangeColor(color);

    color = P_PlayersPalette->color[playerId * 32];
    frameIt = advDlg->GetFrame(MITHRIL_FRAME_ID + 2);
    frameIt->ChangeColor(color);
    frameIt = advDlg->GetFrame(MITHRIL_FRAME_ID + 3);
    frameIt->ChangeColor(color);

    color = P_PlayersPalette->color[playerId * 32 + 30];
    frameIt = advDlg->GetFrame(MITHRIL_FRAME_ID + 4);
    frameIt->ChangeColor(color);
    frameIt = advDlg->GetFrame(MITHRIL_FRAME_ID + 5);
    frameIt->ChangeColor(color);

    color = P_PlayersPalette->color[playerId * 32 + 31];
    frameIt = advDlg->GetFrame(MITHRIL_FRAME_ID + 6);
    frameIt->ChangeColor(color);
    frameIt = advDlg->GetFrame(MITHRIL_FRAME_ID + 7);
    frameIt->ChangeColor(color);

    return;
}

int __stdcall BeforeHotseatMsgBox(LoHook* h, HookContext* c)
{

    if (o_HD_X > 810)
    {
        H3AdventureManager* advMan = P_AdventureMgr;
        H3BaseDlg* advDlg = advMan->dlg;
        h3::H3ResourceBarPanel* dlg = (H3ResourceBarPanel*)c->ecx;

        if (dlg && advDlg)
        {

            H3DlgText* myText = advDlg->GetText(MITHRIL_TEXT_ID);
            if (myText)
            {
                UINT8 meId = P_Game->GetPlayerID();
                SetFrameColor(advDlg, meId);
                myText->SetText(""); //set "noText" for MP msgBox
               // myText->Refresh();
            }
        }
    }

    return EXEC_DEFAULT;
}
