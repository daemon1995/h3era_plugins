#pragma once

namespace artifacts
{
constexpr int TEST_ART_ID = 171; //
constexpr int ARTIFACTS_OBJECT_TYPE = eObject::ARTIFACT;
class ArtifactsExtender : public IGamePatch
{

    static ArtifactsExtender *instance;

    struct ArtifactsNumber
    {
        DWORD base = 0;
        DWORD added = 0;
        DWORD current = 0;
        static constexpr DWORD max = 1024;
    } static artifactsNumber;
    // static DWORD baseArtifactsNumber;
    // static DWORD newArtifactsNumber;
    // static DWORD newArtifactsNumber;
    //  new artifacts setups

    static H3ArtifactSetup *artifactSetups;
    static H3ArtifactSetup *wogArtifactSetupsToSwapNames;

    struct ArtifactsTextZVars
    {

        DWORD artNameZvar = 0;
        DWORD artDescriptionZvar = 0;
    } static *artifactsTextZVars;

    static DWORD *primarySkillsBonusTable;
    static DWORD primarySkillsBonusTableAddr;

    //    static DWORD primarySkillsBonusTableAddr;

    // new art event text
    static LPCSTR *eventTable;
    //    static

  public:
  private:
    ArtifactsExtender();

    virtual ~ArtifactsExtender();

  protected:
    void AfterLoadingObjectTxtProc();

  protected:
    virtual void CreatePatches() override;
    const int LoadNewArtifactsFromJson();

    void ReplaceArtifactSetupsTable();
    void ReplacePrimarySkillsBonusTable();
    void ReplaceArtEventText();
    void ReplaceCombinationArtifactsTable();
    void ReplaceAIValuesTable();
    void LoadArtifactPropertiesFromJson();
    //  private:
  public:
    static ArtifactsExtender &Get();
    static _ERH_(OnAfterWog);
    static _LHF_(LoadArtTraits);
    static void __stdcall H3GameMainSetup__LoadObjects(HiHook *h, H3MainSetup *setup);
};

} // namespace artifacts
