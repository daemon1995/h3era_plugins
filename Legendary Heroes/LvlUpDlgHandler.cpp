#include "pch.h"
namespace lh
{
constexpr const char *bgPcxName = "nhLvlUp0.pcx";
constexpr const char *framePcxName = "nhLvlUp1.pcx";
// constexpr const char* lvlupDlgShadow = "nhLvlUp2.pcx";
}; // namespace lh
H3LoadedPcx16 *LvlUpDlgHandler::m_drawBuffer = nullptr;
BOOL LvlUpDlgHandler::m_isInited = false;
void LvlUpDlgHandler::Init(PatcherInstance *_pi, H3LoadedPcx16 *drawBuffer) noexcept
{
    if (m_isInited)
        return;
    m_isInited = true;

    m_drawBuffer = drawBuffer;

    // Hero LvlUp dialog
    auto pcx = H3LoadedPcx16::Load(lh::bgPcxName); // change background picture size
    int width = pcx->width;
    int height = pcx->height;

    _pi->WriteDword(0x4F9040 + 1, lh::bgPcxName); // change background picture name
    // change name + lvl text
    _pi->WriteDword(0x4F91C5 + 1, width - 46); // width
    _pi->WriteByte(0x4F9144 + 1, 35);          // x
    _pi->WriteByte(0x4F9142 + 1, 0x12);        // y

    // change name + class text
    _pi->WriteDword(0x4F913D + 1, width - 46); // width
    _pi->WriteByte(0x4F91CF + 1, 30);          // x
    _pi->WriteDword(0x4F91CA + 1, 0x2E);       // y

    // change prim skill text
    _pi->WriteDword(0x4F9241 + 1, 0x152);      // y
    _pi->WriteByte(0x4F9246 + 1, 30);          // x
    _pi->WriteDword(0x4F923C + 1, width - 46); //  width

    // change prim skill def
    _pi->WriteDword(0x4F9289 + 1, 0x6601F0); // def name

    _pi->WriteByte(0x4F9295 + 1, 82); // width
    _pi->WriteByte(0x4F9293 + 1, 93); // height

    // 1 Skill per level skill def
    _pi->WriteDword(0x4F97DB + 1, 0x6600F8);          // def name
    _pi->WriteByte(0x4F97E7 + 1, 82);                 // width
    _pi->WriteByte(0x4F97E5 + 1, 93);                 // height
    _pi->WriteDword(0x4F97EE + 1, (width - 82) >> 1); // x
    _pi->WriteDword(0x4F97E9 + 1, 400);               // y
    // skill frame
    _pi->WriteByte(0x4F978D + 1, 84);                       // width
    _pi->WriteByte(0x4F978B + 1, 95);                       // height
    _pi->WriteDword(0x4F9794 + 1, ((width - 82) >> 1) - 1); // x
    _pi->WriteDword(0x4F978F + 1, 399);                     // y
    // skill name
    _pi->WriteByte(0x4F9867 + 1, (width - 82) >> 1); // x
    _pi->WriteDword(0x4F9862 + 1, 490);              // y

    // 2 Skills per level
    // left skill def
    _pi->WriteDword(0x4F956E + 1, 0x6600F8); // def name
    _pi->WriteByte(0x4F9521 + 1, 82);        // width
    _pi->WriteByte(0x4F951F + 1, 93);        // height
    _pi->WriteByte(0x4F9528 + 1, 103);       // x
    _pi->WriteDword(0x4F9523 + 1, 397);      // y
    // left skill frame
    _pi->WriteByte(0x4F9448 + 1, 84);      // width
    _pi->WriteByte(0x4F9446 + 1, 95);      // height
    _pi->WriteByte(0x4F944F + 1, 103 - 1); // x
    _pi->WriteDword(0x4F944A + 1, 396);    // y
    // skill name
    _pi->WriteByte(0x4F95FE + 1, 103);  // x
    _pi->WriteDword(0x4F95F9 + 1, 487); // y

    // right skill def
    _pi->WriteDword(0x4F9515 + 1, 0x6600F8); // def name
    _pi->WriteByte(0x4F957A + 1, 82);        // width
    _pi->WriteByte(0x4F9578 + 1, 93);        // height
    _pi->WriteDword(0x4F9581 + 1, 234);      // x
    _pi->WriteDword(0x4F957C + 1, 397);      // y
    // right skill frame
    // skill frame
    _pi->WriteByte(0x4F94B4 + 1, 84);       // width
    _pi->WriteByte(0x4F94B2 + 1, 95);       // height
    _pi->WriteDword(0x4F94BB + 1, 234 - 1); // x
    _pi->WriteDword(0x4F94B6 + 1, 396);     // y
    // skill name
    _pi->WriteByte(0x4F967B + 1, 234);  // x
    _pi->WriteDword(0x4F9676 + 1, 487); // y

    // "or" skills text
    _pi->WriteDword(0x4F93F0 + 1, (width - ByteAt(0x4F93E9 + 1)) >> 1); // y
    _pi->WriteDword(0x4F93EB + 1, 417);                                 // y

    // change iOkay button def
    _pi->WriteDword(0x4F98CB + 1, 330); // x
    _pi->WriteDword(0x4F98C6 + 1, 495); // y

    _pi->WriteDword(0x4F904F + 1, width);  // change bg picture width
    _pi->WriteDword(0x4F904A + 1, height); // change bg picture height
    pcx->Dereference();

    _pi->WriteLoHook(0x4F8F30, BeforeCreate);
    _pi->WriteLoHook(0x4F908C, BeforeHeroPortraitCreate);
    _pi->WriteLoHook(0x4F9293, BeforePrimarySkillDefCreate);
    _pi->WriteLoHook(0x4F9D36, AfterCreate);
}

_LHF_(LvlUpDlgHandler::BeforeCreate) //(LoHook* hook, HookContext* c)
{

    auto pcx = H3LoadedPcx16::Load(lh::bgPcxName);

    c->Push(pcx->height);
    c->Push(pcx->width);
    int yPos = (P_WindowManager->lastDlg->GetHeight() - pcx->height) >> 1;

    c->edi = c->ecx;
    c->Push(yPos);
    int xPos = (P_WindowManager->lastDlg->GetWidth() - pcx->width) >> 1;

    c->Push(xPos);
    pcx->Dereference();

    c->return_address = 0x4F8F43;

    return NO_EXEC_DEFAULT;
}
_LHF_(LvlUpDlgHandler::BeforeHeroPortraitCreate)
{
    // init m_heroWgt
    H3Hero *m_heroWgt = reinterpret_cast<H3Hero *>(c->edi);
    HeroLook::CorrectPcxNames(m_heroWgt->id);

    // skip portrait creation
    c->return_address = 0x4F90F1;

    return NO_EXEC_DEFAULT;
}

_LHF_(LvlUpDlgHandler::AfterCreate)
{

    H3Dlg *dlg = *reinterpret_cast<H3Dlg **>(0x699684); // get Dlg
    if (dlg)
    {
        if (auto txt = dlg->GetText(2005))
        {
            txt->SetWidth(dlg->GetWidth() - 82);
            txt->SetHeight(60);
            txt->SetY(334);
            txt->SetX(46);
            txt->SetAlignment(eTextAlignment::MIDDLE_CENTER);
        }
    }

    return EXEC_DEFAULT;
}

H3DlgPcx16 *CreateComposeedDlgPcx(H3LoadedPcx16 *buffer, int itemId)
{

    auto framePcx = H3LoadedPcx16::Load(lh::framePcxName);

    int frameWidth = framePcx->width;
    int frameHeight = framePcx->height;

    H3DlgPcx16 *dlgPcx = H3DlgPcx16::Create(110, 75, frameWidth, frameHeight, itemId, nullptr);

    for (size_t i = 0; i < 2; i++)
    {
        auto sourcePcx = H3LoadedPcx16::Load(HeroLook::pcxNames[i].String());

        const int srcX = ((sourcePcx->width - frameWidth) >> 1) + 3;
        constexpr int srcY = 50; // (sourcePcx->height - frameHeight) >> 1;
        sourcePcx->DrawToPcx16(3, 3, 1, buffer, srcX, srcY);
        sourcePcx->Dereference();
    }

    framePcx->DrawToPcx16(0, 0, 1, buffer, 0, 0);

    dlgPcx->SetPcx(buffer);

    return dlgPcx;
}
_LHF_(LvlUpDlgHandler::BeforePrimarySkillDefCreate) //(LoHook* hook, HookContext* c)
{
    H3Dlg *dlg = *reinterpret_cast<H3Dlg **>(0x699684); // get Dlg
    if (dlg)
    {
        auto def = H3DlgDef::Create(22, 75, "pskill.def", 4);
        dlg->AddItem(def, false);
        dlg->AddItem(
            H3DlgPcx::Create(IntAt(0x4F98CB + 1) - 1, IntAt(0x4F98C6 + 1) - 1, 0, NH3Dlg::Assets::BOX_64_30_PCX),
            false);

        dlg->AddItem(CreateComposeedDlgPcx(m_drawBuffer, 2001), false);

        //	THISCALL_1(void, 0x5FEDC0, def);

        c->Push(82);
        c->Push(82);
        c->Push(75);
        c->Push(313);
        c->return_address = 0x4F92A1;
        return NO_EXEC_DEFAULT;
    }

    return EXEC_DEFAULT;
}
