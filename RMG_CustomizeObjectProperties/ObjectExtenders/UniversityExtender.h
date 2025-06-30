#pragma once

namespace university
{
class UniversityExtender : public extender::ObjectsExtender
{
    static UniversityExtender* instance;

  public:
    static INT8 bannedSkillsCopy[limits::SECONDARY_SKILLS];
    static INT objectSubtype;

  private:
    struct UniversityData
    {
        DWORD allowedSkills = -1;
        DWORD obligatorySkills = NULL;
        BOOL ignoreBannedSkills = false;
    };
    std::vector<UniversityData> universitiesData;

  private:
    UniversityExtender();
    virtual ~UniversityExtender();

  protected:
    virtual void CreatePatches() override;

    virtual H3RmgObjectGenerator *CreateRMGObjectGen(const RMGObjectInfo &objectInfo) const noexcept override final;

    virtual void AfterLoadingObjectTxtProc(const INT16 *maxSubtypes) override final;
    virtual BOOL SetHintInH3TextBuffer(H3MapItem *mapItem, const H3Hero *currentHero, const H3Player *activePlayer,
                                       const BOOL isRightClick) const noexcept override final;
    virtual BOOL RMGDlg_ShowCustomObjectHint(const RMGObjectInfo &info, const H3ObjectAttributes *attributes,
                                             const H3String &defaltText) noexcept override final;

  private:
    static void __stdcall Game_SetupUniversity(HiHook *h, H3Main *game, const H3MapItem *university);

  public:
    static UniversityExtender &Get();
};

} // namespace university
