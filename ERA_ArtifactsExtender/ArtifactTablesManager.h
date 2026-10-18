#pragma once

namespace artifacts
{
struct TextZVars
{
    int name = 0;
    int description = 0;
};
class ArtifactTablesManager
{
  public:
    H3ArtifactSetup *setups = nullptr;
    H3ArtifactSetup *backup = nullptr;
    signed char *primary = nullptr;
    TextZVars *textVars = nullptr;
    LPCSTR *events = nullptr;
    int baseCount = 0;
    int count = 0;
    int primarySpan = 0;
    int aiSpan = 0;
    int eventSpan = 0;

    bool Compatible() const;
    static bool CompatibleLayout(int base, DWORD bonuses, DWORD end, DWORD eventBytes,
                                 bool unrelocated, bool supportedCode);
    void Install(int artifactCount);
    void BuildAI(const std::array<ArtifactData, CAPACITY> &definitions);

  private:
    DWORD *aiVectors = nullptr;
    std::vector<int> aiProperties;
};
} // namespace artifacts
