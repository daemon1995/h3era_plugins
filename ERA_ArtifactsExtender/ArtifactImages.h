#pragma once
#include <map>

namespace artifacts
{
class ArtifactImages
{
    // Frames are independent game allocations retained for the lifetime of patches.
    // DefGroup owns its pointer array, not these resource objects.
    std::map<std::string, H3DefFrame *> frames;
    H3DefFrame *Frame(H3LoadedDef *target, const std::string &image, int sourceFrame);
  public:
    void Apply(H3LoadedDef *target, int count, const std::array<ArtifactData, CAPACITY> &definitions);
};
}
