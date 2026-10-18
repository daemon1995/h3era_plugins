#pragma once
#include <cstddef>
#include <cstdint>
#include <limits>

namespace erm_functions
{
namespace hd_minimap
{
constexpr std::size_t MISSING = (std::numeric_limits<std::size_t>::max)();

struct Layout
{
    std::size_t objects16 = MISSING;
    std::size_t objects32 = MISSING;
    std::size_t markers16 = MISSING;
    std::size_t markers32 = MISSING;
    explicit operator bool() const
    {
        return objects16 != MISSING && objects32 != MISSING && markers16 != MISSING && markers32 != MISSING;
    }
};

template<std::size_t N> bool Match(const std::uint8_t *code, std::size_t size, std::size_t offset,
                                  const int (&pattern)[N])
{
    if (offset > size || N > size - offset)
        return false;
    for (std::size_t i = 0; i < N; ++i)
        if (pattern[i] >= 0 && code[offset + i] != pattern[i])
            return false;
    return true;
}

template<std::size_t N> std::size_t FindUnique(const std::uint8_t *code, std::size_t size,
                                            const int (&pattern)[N])
{
    std::size_t found = MISSING;
    if (N <= size)
        for (std::size_t i = 0; i <= size - N; ++i)
            if (Match(code, size, i, pattern))
            {
                if (found != MISSING)
                    return MISSING;
                found = i;
            }
    return found;
}

// Match only the verified ABI, including visibility/hero gates and marker data flow.
// DLL addresses are absent from these patterns; relative-call operands are wildcards.
// Thus relocation and moving the whole function do not change recognition.
inline Layout FindLayout(const std::uint8_t *code, std::size_t size)
{
    static const int objects16[] = {
        0xBA,0x20,0xD2,0x4F,0x00,0x8B,0x8D,0x7C,0xFF,0xFF,0xFF,0xFF,0xD2,
        0x89,0x85,0xF8,0xFD,0xFF,0xFF };
    static const int objects32[] = {
        0xBA,0x20,0xD2,0x4F,0x00,0x8B,0x8D,0x78,0xFF,0xFF,0xFF,0xFF,0xD2,
        0x89,0x85,0x94,0xFD,0xFF,0xFF };
    static const int markers16[] = {
        0x66,0x89,0x55,0xA8,0x8B,0x45,0xDC,0x83,0xC0,0x01,0x50,0x8D,0x4D,0xD8,
        0xE8,-1,-1,-1,-1,0x8D,0x4D,0xD8,0x8B,0x51,0x04,0x8B,0x45,0xD8,
        0x8B,0x4D,0xC4,0x89,0x4C,0x90,0xFC };
    static const int markers32[] = {
        0x66,0x89,0x55,0xB8,0x8B,0x45,0xDC,0x83,0xC0,0x01,0x50,0x8D,0x4D,0xD8,
        0xE8,-1,-1,-1,-1,0x8D,0x4D,0xD8,0x8B,0x51,0x04,0x8B,0x45,0xD8,
        0x8B,0x4D,0xBC,0x89,0x4C,0x90,0xFC };
    static const int visibility16[] = {
        0x0F,0xBE,0x95,0x72,0xFF,0xFF,0xFF,0x85,0xD2,0x0F,0x84,0x8F,0x07,0x00,0x00 };
    static const int visibility32[] = {
        0x0F,0xBE,0x95,0x73,0xFF,0xFF,0xFF,0x85,0xD2,0x0F,0x84,0x74,0x07,0x00,0x00 };
    static const int hero16[] = {
        0x8B,0x95,0x7C,0xFF,0xFF,0xFF,0x83,0x7A,0x1E,0x22,0x0F,0x85,0xBD,0x00,0x00,0x00 };
    static const int hero32[] = {
        0x8B,0x95,0x78,0xFF,0xFF,0xFF,0x83,0x7A,0x1E,0x22,0x0F,0x85,0xBA,0x00,0x00,0x00 };
    Layout result;
    result.objects16 = FindUnique(code, size, objects16);
    result.objects32 = FindUnique(code, size, objects32);
    const auto tail16 = FindUnique(code, size, markers16);
    const auto tail32 = FindUnique(code, size, markers32);
    if (result.objects16 == MISSING || result.objects32 == MISSING || tail16 == MISSING || tail32 == MISSING ||
        result.objects16 < 0x115 || result.objects32 < 0x10F ||
        tail16 - result.objects16 != 0x223 || tail32 - result.objects32 != 0x21D ||
        !Match(code, size, result.objects16 - 0x115, visibility16) ||
        !Match(code, size, result.objects32 - 0x10F, visibility32) ||
        !Match(code, size, result.objects16 - 0xCD, hero16) ||
        !Match(code, size, result.objects32 - 0xCA, hero32))
        return Layout{};
    result.markers16 = tail16 + 4; // First append instruction, after the local color assignment.
    result.markers32 = tail32 + 4;
    return result;
}
}
}
