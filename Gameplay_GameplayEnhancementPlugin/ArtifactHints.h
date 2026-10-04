#pragma once

#include "pch.h"
#include "..\headers\EraPluginsAPI\ArtifactDescriptionApi.h"

namespace artifacts
{
struct StatBytes
{
    INT stats[4];
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
    BOOL enabled = true;
    BOOL placeBelowText = false;
    BOOL addAsExtraObject = false;

  public:
    virtual void Load() override;
};

class ArtifactHints : public IGamePatch
{
    BOOL active = true;
    BOOL isUniteComboArtifactCall = false;
    struct DescriptionContext
    {
        const H3Hero *hero;
        int slot;
    };
    struct Message
    {
        H3String description;
        H3String widget;
        HintsText text;
        int additionalHeight = 0;
    };
    DescriptionContext *context = nullptr;
    Message pendingMessage;
    Message *currentMessage = nullptr;
    class HeightScope;
    HeightScope *currentHeight = nullptr;
    HintsText settings;

    static constexpr LPCSTR PRIMARY_SKILLS_ERM_VARIABLE_FORMAT = "gem_artifact_hints_primary_skills_%d";
    static constexpr LPCSTR COMBINATIONS_ERM_VARIABLE_FORMAT = "gem_artifact_hints_combinations_%d";

    static constexpr LPCSTR COMPARED_STATS_FORMAT = "%d %s";
    static constexpr LPCSTR STATS_FORMAT = "%d";

    static ArtifactHints *instance;

    ArtifactHints();

    virtual void CreatePatches() noexcept final;

  protected:
    BOOL CreateStatsString(const H3Artifact *artifact, const H3Hero *heroToCompareStats, const int slot,
                          H3String *result) noexcept;
    BOOL CreateCombinePartsString(const H3Artifact *artifact, const H3Hero *heroToCompareStats,
                                  H3String *result) noexcept;

  private:
    static void __stdcall SwapMgr_InteractArtifactSlot(HiHook *h, H3SwapManager *mgr, const int side, int slotIndex,
                                                       int a4) noexcept;
    static H3String *__stdcall BuildUpArtifactDescription(HiHook *h, const H3Artifact *artifact,
                                                          H3String *result) noexcept;
    static int __stdcall UniteComboArtifacts(HiHook *h, const H3Hero *hero, const int artId) noexcept;
    static int __stdcall BuildMultiPicDlg(HiHook *h, H3Game *game);
    static void __stdcall ShowMessageBox(HiHook *h, LPCSTR text, int type, int x, int y, int pic1, int value1,
                                         int pic2, int value2, int pic3, int value3, int pic4, int value4);
    static int __stdcall ShowDescription(const ArtifactDescriptionApi::Request *request);

  public:
    static ArtifactHints &Get();
};

} // namespace artifacts
