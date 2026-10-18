#pragma once
#include "framework.h"
#include "MarketOfTimeLogic.h"

namespace wog
{
namespace marketOfTime
{
BOOL Handles(const H3MapItem *mapItem) noexcept;
BOOL Visit(H3Hero *hero, BOOL isHuman) noexcept;
BOOL SetHint(BOOL isRightClick) noexcept;
} // namespace marketOfTime
} // namespace wog
