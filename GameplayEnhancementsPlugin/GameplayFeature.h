#pragma once
#include "pch.h"

namespace features
{
class GameplayFeature : public IGamePatch
{

    GameplayFeature();
    virtual void CreatePatches() noexcept override;

  public:
    static GameplayFeature &Get();
};
} // namespace features
