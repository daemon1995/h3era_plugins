#pragma once

namespace artifacts
{
constexpr const char *SAVE_SECTION = "ERA_ArtifactsExtender.v1";
struct SaveHeader
{
    DWORD magic = 0x58415245; // ERAX
    DWORD version = 1;
    DWORD count = 0;
};
struct SavedArtifact
{
    int cost, position, type, combo, part;
    int nameVar, descriptionVar;
    unsigned char disabled, hasSpell, mapDisabled, mapUsed;
};
static_assert(sizeof(SavedArtifact) == 32, "Save format must not contain pointers");

inline bool ValidSaved(const SavedArtifact &art, bool extra)
{
    return (!extra || (art.cost >= 0 && art.position >= 0 && art.position <= 14 &&
           art.type >= 0 && (art.type & ~31) == 0 &&
           art.combo >= -1 && art.combo < NATIVE_COMBINATIONS &&
           art.part >= -1 && art.part < NATIVE_COMBINATIONS)) &&
           art.nameVar >= 0 && art.nameVar <= 1000 && art.descriptionVar >= 0 && art.descriptionVar <= 1000 &&
           art.disabled <= 1 && art.hasSpell <= 1 && art.mapDisabled <= 1 && art.mapUsed <= 1;
}
}
