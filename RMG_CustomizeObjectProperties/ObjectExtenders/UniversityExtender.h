#pragma once

namespace university
{
constexpr int UNI_OBJECT_SUBTYPE = 1;

class UniversityExtender : public extender::ObjectsExtender
{
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

    //	virtual void GetObjectPreperties() noexcept override final;
    //	void SetRmgObjectGenData(const int objectSubtype)  noexcept;

  private:
    static void __stdcall Game_SetupUniversity(HiHook *h, H3Main *game, const H3MapItem *university);
    /*        static _LHF_(Game__AtShrineOfMagicIncantationSettingSpell);
            static _LHF_(Game__AtShrineOfMagicGestureSettingSpell);
            static _LHF_(Shrine__AtVisit);
            static _LHF_(Shrine__AtWisdomCheck);
            static _LHF_(Shrine__AtGetName);
            static _LHF_(Shrine__AtGetHint)*/
    ;

  public:
    static UniversityExtender &Get();
};

} // namespace university
