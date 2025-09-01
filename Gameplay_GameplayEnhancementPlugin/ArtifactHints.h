#pragma once

namespace artifacts
{
struct StatBytes
{
    INT8 stats[4];
    StatBytes(const H3Artifact &art);
    StatBytes();
    operator bool() const;
    StatBytes &operator+=(const StatBytes &other);
};
constexpr LPCSTR endl = "\n";
constexpr LPCSTR doubleEndl = "\n\n";
struct HintsText : public IPluginText
{

    LPCSTR textFormat = "{~>PSKIL21.def:0:0}%s {~>PSKIL21.def:0:1}%s {~>PSKIL21.def:0:2}%s {~>PSKIL21.def:0:3}%s}";
    LPCSTR increaseFormat = "{~LightGreen}(+%d)}";
    LPCSTR decreaseFormat = "{~r}(%d)}";
    LPCSTR fontName = NH3Dlg::Text::MEDIUM;
    BOOL placeBelowText = false;
    BOOL addAsExtraObject = false;

  public:
    virtual void Load() override;
};

class ArtifactHints : public IGamePatch
{
    BOOL active = true;
    INT swapSide = 0;
	BOOL isUniteComboArtifactCall = false;
    Patch *drawMultiPicDlgPatch = nullptr;
    INT messageboxHeightChange = 0;
    H3String hintTextBuffer;
    HintsText settings;

    static constexpr LPCSTR PRIMARY_SKILLS_ERM_VARIABLE_FORMAT = "gem_artifact_hints_primary_skills_%d";
    static constexpr LPCSTR COMBINATIONS_ERM_VARIABLE_FORMAT = "gem_artifact_hints_combinations_%d";

    static constexpr LPCSTR HINT_FORMAT_KEY = "gem_plugin.artifact_hints.primary_skills.text_format";
    static constexpr LPCSTR COMPARED_STATS_FORMAT = "%d %s";
    static constexpr LPCSTR STATS_FORMAT = "%d";

    static ArtifactHints *instance;

    ArtifactHints();

    virtual void CreatePatches() noexcept final;

  protected:
    BOOL CreateStatsString(const H3Artifact *artifact, const H3Hero *heroToCompareStats, H3String *result) noexcept;
    BOOL CreateCombinePartsString(const H3Artifact *artifact, const H3Hero *heroToCompareStats,
                                  H3String *result) noexcept;
	void ChangeMessageBoxHeight(const int additionalHeight) noexcept;
  private:
    static void __stdcall SwapMgr_InteractArtifactSlot(HiHook *h, H3SwapManager *mgr, const int side, int slotIndex,
                                                       int a4) noexcept;
    static H3String *__stdcall BuildUpArtifactDescription(HiHook *h, const H3Artifact *artifact,
                                                          H3String *result) noexcept;
    static int __stdcall UniteComboArtifacts(HiHook* h, const H3Hero *hero, const int artId) noexcept;
    static int __stdcall BuildMultiPicDlg(HiHook *h, H3Game *game);

  public:
    static ArtifactHints &Get();
};

} // namespace artifacts
