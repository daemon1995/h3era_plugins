#pragma once
#include "framework.h"
#include <array>
namespace wog { namespace handlers {
std::array<bool, 3> ChoosePotions(H3Hero *hero, const std::array<int, 3> &doses);
} }
