#pragma once

namespace artifacts
{

class ArtifactData
{
    // used to read json keys
    static char jsonKeyNameBuffer[512];

    // initial data to clamp
    eArtifact id = eArtifact::NONE;

    // original setup copy -- default ctor is in H3API
    H3ArtifactSetup setup;

    // 
    char primarySkillBonuses[4] = {};

  public:
    ArtifactData(LPCSTR modName, const int arrayIndex);
    eArtifact getId() const
    {
        return id;
    }
    const H3ArtifactSetup &getSetup() const
    {
        return setup;
    }
    const char *getPrimarySkillBonuses() const
    {
        return primarySkillBonuses;
    }
};

} // namespace artifacts
