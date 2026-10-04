#pragma once

#include <cstdint>
#include <cstddef>
#include <set>
#include <utility>
#include <vector>

namespace monoliths
{
constexpr int ORIGINAL_GROUP_COUNT = 8;
constexpr int MAX_SUBTYPE = 32767; // H3MapItem::objectSubtype is a signed 16-bit field.
constexpr uint32_t SAVE_MAGIC = 0x314C4F4D; // MOL1
constexpr uint32_t SAVE_VERSION = 1;
constexpr std::size_t SAVE_HEADER_WORDS = 4;
constexpr std::size_t MAX_MAP_POSITIONS = 1024u * 1024u * 2u;

inline bool IsExtendedSubtype(int subtype) noexcept
{
    return subtype >= ORIGINAL_GROUP_COUNT && subtype <= MAX_SUBTYPE;
}

struct SavedGroup
{
    uint32_t subtype;
    std::vector<uint32_t> positions;
};

inline bool GetSaveWordCount(const uint32_t *header, std::size_t &wordCount) noexcept
{
    if (header[0] != SAVE_MAGIC || header[1] != SAVE_VERSION ||
        header[2] > MAX_SUBTYPE + 1u - ORIGINAL_GROUP_COUNT || header[3] > MAX_MAP_POSITIONS)
        return false;
    wordCount = SAVE_HEADER_WORDS + std::size_t(header[2]) * 2 + header[3];
    return true;
}

class SaveDataBuilder
{
    std::vector<uint32_t> words;

  public:
    SaveDataBuilder(std::size_t groupCount, std::size_t positionCount)
    {
        words.reserve(SAVE_HEADER_WORDS + groupCount * 2 + positionCount);
        words.assign({SAVE_MAGIC, SAVE_VERSION, 0, 0});
    }

    template <typename Positions> void AppendGroup(uint32_t subtype, const Positions &positions)
    {
        const std::size_t start = words.size();
        words.push_back(subtype);
        words.push_back(0);
        for (const auto position : positions)
            words.push_back(uint32_t(position));
        const auto count = uint32_t(words.size() - start - 2);
        words[start + 1] = count;
        ++words[2];
        words[3] += count;
    }

    std::vector<uint32_t> Take() noexcept
    {
        return std::move(words);
    }
};

inline std::vector<uint32_t> EncodeGroups(const std::vector<SavedGroup> &groups)
{
    std::size_t positionCount = 0;
    for (const auto &group : groups)
        positionCount += group.positions.size();
    SaveDataBuilder builder(groups.size(), positionCount);
    for (const auto &group : groups)
        builder.AppendGroup(group.subtype, group.positions);
    return builder.Take();
}

// Decode into temporary storage, so a damaged section cannot partially replace live groups.
inline bool DecodeGroups(const std::vector<uint32_t> &words, std::vector<SavedGroup> &result)
{
    std::size_t expectedWords = 0;
    if (words.size() < SAVE_HEADER_WORDS || !GetSaveWordCount(words.data(), expectedWords) ||
        expectedWords != words.size())
        return false;

    std::vector<SavedGroup> decoded;
    std::set<uint32_t> subtypes;
    std::set<uint32_t> positions;
    std::size_t cursor = SAVE_HEADER_WORDS;
    std::size_t totalPositions = 0;
    for (uint32_t i = 0; i < words[2]; ++i)
    {
        if (words.size() - cursor < 2)
            return false;
        const uint32_t subtype = words[cursor++];
        const uint32_t count = words[cursor++];
        if (subtype > MAX_SUBTYPE || !IsExtendedSubtype(int(subtype)) || !subtypes.insert(subtype).second ||
            count > words.size() - cursor)
            return false;
        SavedGroup group{subtype, {}};
        group.positions.reserve(count);
        for (uint32_t j = 0; j < count; ++j)
        {
            const uint32_t position = words[cursor++];
            if (!positions.insert(position).second)
                return false;
            group.positions.push_back(position);
        }
        totalPositions += count;
        decoded.push_back(std::move(group));
    }
    if (cursor != words.size() || totalPositions != words[3])
        return false;
    result.swap(decoded);
    return true;
}
} // namespace monoliths
