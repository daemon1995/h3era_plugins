#pragma once

class LvlUpDlgHandler : public IGamePatch
{

    static _LHF_(BeforeCreate);
    static _LHF_(BeforeHeroPortraitCreate);
    static _LHF_(BeforePrimarySkillDefCreate);
    static _LHF_(AfterCreate);

    static H3LoadedPcx16 *m_drawBuffer;
    static BOOL m_isInited;

  public:
    static void Init(PatcherInstance *_pi, H3LoadedPcx16 *drawBuffer) noexcept;
};
