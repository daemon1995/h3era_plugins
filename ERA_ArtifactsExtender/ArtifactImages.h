#pragma once
namespace artifacts
{
class ArtifactImages
{
    bool reportedUnavailable = false;
  public:
    bool Ready();
    void Apply(H3LoadedDef *target, int count, const std::array<ArtifactData, CAPACITY> &definitions);
};
}
