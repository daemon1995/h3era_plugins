#pragma once
#include "framework.h"
#include "ArtificerLogic.h"

namespace wog { namespace artificer {
void RegisterEvents();
BOOL Initialize(H3MapItem *mapItem) noexcept;
BOOL Visit(H3Hero *hero, H3MapItem *mapItem, H3Position position, BOOL isHuman) noexcept;
BOOL SetHint(H3MapItem *mapItem, const H3Hero *hero, BOOL isRightClick) noexcept;
BOOL SetAiWeight(H3MapItem *mapItem, H3Hero *hero, H3Position position, int &weight) noexcept;
} }
