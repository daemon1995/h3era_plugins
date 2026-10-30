#pragma once
#include "framework.h"

namespace wog { namespace native {
inline bool ArtifactBanned(int artifact) noexcept
{
    // common.cpp::ArtDisabled, also hooked by ERA_ArtifactsExtender for expanded tables.
    return artifact < 0 || artifact >= int(H3ArtifactCount::Get()) || CDECL_1(int, 0x714EA7, artifact) != 0;
}
// MithrillVal[8] is outside H3Resources. ERA may relocate it; the engine operand
// points at the active array (original WoG base 0x27F9A00).
inline int *Mithril() noexcept
{
    return reinterpret_cast<int *>(DwordAt(0x073764C + 3));
}
inline int Mithril(const int player) noexcept
{
    const auto values = Mithril();
    return values && player >= 0 && player < 8 ? values[player] : 0;
}
inline void SetMithril(const int player, const int value) noexcept
{
    if (auto values = Mithril())
        if (player >= 0 && player < 8) values[player] = value;
}
} }
