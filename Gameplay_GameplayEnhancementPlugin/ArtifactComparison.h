#pragma once

namespace artifacts
{
// An unknown destination must not silently select one of several ring/misc slots.
inline int ComparisonSlot(const H3Artifact &artifact, const H3Hero *hero, int selectedSlot)
{
    if (!hero)
        return -1;
    if (selectedSlot >= 0 && selectedSlot < 19)
        return hero->CanPlaceArtifact(artifact.id, selectedSlot) ||
                       hero->CanReplaceArtifact(artifact.id, selectedSlot) ? selectedSlot : -1;
    for (int i = 0; i < 19; ++i)
        if (hero->bodyArtifacts[i] == artifact)
            return i;
    int candidate = -1;
    for (int i = 0; i < 19; ++i)
    {
        if (hero->CanPlaceArtifact(artifact.id, i) || hero->CanReplaceArtifact(artifact.id, i))
        {
            if (candidate != -1)
                return -1;
            candidate = i;
        }
    }
    return candidate;
}
}
