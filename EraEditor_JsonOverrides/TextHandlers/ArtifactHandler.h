#pragma once
#include "../framework.h"
#include "../../headers/Era/JsonOverridesSchema.hpp"

class ArtifactHandler
{
  public:
    struct TxtHandler { DWORD rowCount = 0, objectsCount = 0, expectedObjectsCount = 0; };
    inline static TxtHandler artifactsHandler{};
    static void Init();
    static TxtHandler &Handler() noexcept { return artifactsHandler; }
    static int GetArtifactsNumber() noexcept;
};
