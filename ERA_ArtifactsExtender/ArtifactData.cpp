#include "pch.h"
char artifacts::ArtifactData::jsonKeyNameBuffer[512];

artifacts::ArtifactData::ArtifactData(LPCSTR modName, const int arrayIndex)
{
	bool readSuccess = false;
    libc::sprintf(jsonKeyNameBuffer, "%s.artifacts.%d.id", modName, arrayIndex);

    const int artId = EraJS::readInt(jsonKeyNameBuffer, readSuccess);
	if (!readSuccess || artId < 0 || artId >= 1024)
    {
		return;
    }
    id = static_cast<eArtifact>(artId);

}
