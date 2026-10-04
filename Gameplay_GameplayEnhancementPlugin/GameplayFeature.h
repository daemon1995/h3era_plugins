#pragma once
#include "pch.h"

namespace features
{
class GameplayFeature : public IGamePatch
{
    static GameplayFeature *instance;

    GameplayFeature();
    virtual void CreatePatches() noexcept override;

  public:
    static constexpr LPCSTR THIEVES_GUILD_TEXT_FORMAT = "gem_plugin.thieves_guild_dlg.displayed_info.text_format";
    static GameplayFeature &Get();
};
} // namespace features
