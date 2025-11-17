#pragma once

namespace timezone
{
class TownManager : public IGamePatch
{

    TownManager();

  private:
    virtual void CreatePatches() override;

  public:
    struct BackgroundDrawInfo
    {
        // used to animate back withc switching pcx
        UINT lastFrameId;
        UINT currentFrame;
        H3String formatName;

      public:
        void Update(LPCSTR townPrefixName, const UINT thisTownTime);
        H3LoadedPcx16 *GetNextFrame();
    } backgroundDrawInfo;
    struct AnimationDrawInfo
    {
        struct DefDrawData
        {
            H3DlgDefButton *defButton;
            UINT framesNumber;
        };

        std::vector<DefDrawData> defDrawData;
        UINT defsToDraw;

        static constexpr const char *buttonNameTagFormat = "tod_nature_%d";
        const BOOL LoadFromJsonByTime(LPCSTR townPrefixName, const INT thisTownTime);

      public:
        void Update(LPCSTR townPrefixName, const UINT thisTownTime);

    } animationDrawInfo;

  private:
    static int __cdecl TownMgrDlg_CreateBackgroundImageName(HiHook *h, char *buffer, char *formatName,
                                                              char *townPrefixName);
    static void __stdcall TownMgrDlg_DrawBackgroundImageNameInProc(HiHook *h, H3DlgPcx16 *background);
    static void __stdcall TownMgrDlg_BuildingsWindowsRedraw(HiHook *h, H3WindowManager *wndMgr, const int x,
                                                            const int y, const int w, const int height);

  public:
    static TownManager &Get();
};
} // namespace timezone
