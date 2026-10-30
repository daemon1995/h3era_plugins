#pragma once
#include "../header.h"
#include <string>

namespace game_resources
{
// ERA/SoD resource lookup without the native loaders' missing-resource dialogs.
inline bool Exists(const std::string &name, bool sprite)
{
    auto *cached = h3::H3ResourceManager::Get()->FindItem(name.c_str());
    if (cached && cached->data.m_item) return true;
    const auto exists = [](const std::string &file) {
        const DWORD attributes = GetFileAttributesA(file.c_str());
        return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
    };
    if (exists(name) || exists("Data\\" + name)) return true;
    if (!sprite && Era::PcxPngExists && Era::PcxPngExists(name.c_str())) return true;
    const int version = **reinterpret_cast<int **>(0x69928C);
    if (version < 0 || version > 3) return false;
    const int count = h3::IntAt(h3::DwordAt(sprite ? 0x55CA3B : 0x55AB9F) + version * 24);
    auto *indexes = *reinterpret_cast<int **>(h3::DwordAt(sprite ? 0x55CA34 : 0x55AB98) + version * 24);
    const DWORD lods = h3::DwordAt(sprite ? 0x55CA56 : 0x55ABBA) - 4;
    for (int i = 0; i < count; ++i)
        if (reinterpret_cast<h3::H3Lod *>(lods + indexes[i] * sizeof(h3::H3Lod))->FindFileInLod(name.c_str())) return true;
    return false;
}
}
