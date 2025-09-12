#pragma once

namespace artifacts
{
constexpr int ARTIFACTS_OBJECT_TYPE = eObject::ARTIFACT;
class ArtifactsExtender : public IGamePatch
{
    static ArtifactsExtender *instance;

    static DWORD baseArtifactsNumber;
    static DWORD newArtifactsNumber;
    // new artifacts setups
    static H3ArtifactSetup *artifactSetups;

    static DWORD *primarySkillsBonusTable;
    static DWORD primarySkillsBonusTableAddr;
//    static 

  public:
  private:
    ArtifactsExtender();

    virtual ~ArtifactsExtender();

  protected:
    void AfterLoadingObjectTxtProc(const INT16 *maxSubtypes);

  protected:
    virtual void CreatePatches() override;
    void ReplaceArtifactSetupsTable();
    void ReplacePrimarySkillsBonusTable();
	void ReplaceCombinationArtifactsTable();
	void ReplaceAIValuesTable();
    //  private:
  public:
    static ArtifactsExtender &Get();
    static _ERH_(OnAfterWog);
    static void __stdcall H3GameMainSetup__LoadObjects(HiHook *h, const H3MainSetup *setup);
};

} // namespace artifacts
