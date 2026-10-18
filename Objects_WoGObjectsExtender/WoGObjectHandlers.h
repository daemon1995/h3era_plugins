#pragma once
#include "framework.h"

namespace wog { namespace handlers {

BOOL Init(H3MapItem *item) noexcept;
BOOL Visit(H3Hero *hero, H3MapItem *item, H3Position pos, BOOL human) noexcept;
BOOL Hint(H3MapItem *item, const H3Hero *hero, int interactPlayerId, BOOL rightClick) noexcept;
BOOL AiWeight(H3MapItem *item, H3Hero *hero, const H3Player *player, int &weight, int *moveDistance,
              H3Position pos) noexcept;
void RegisterEvents() noexcept;

}} // namespace wog::handlers
