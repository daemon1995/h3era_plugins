#pragma once
#include "pch.h"

namespace features
{
class GameplayFeature : public IGamePatch
{
    static constexpr LPCSTR THIEVES_GUILD_JSON_TEXT = "gem_plugin.tavern_dlg.captured_taverns";
	static GameplayFeature* instance;

    GameplayFeature();
    virtual void CreatePatches() noexcept override;

  public:
    static GameplayFeature &Get();
};
} // namespace features
