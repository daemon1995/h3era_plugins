#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace SpellDescriptions
{
namespace Detail
{
// Decimal output with no CRT formatting or heap allocation. Copies retain
// their own storage; no pointer refers into a temporary Number object.
class DecimalNumber
{
    char digits[21];
    std::size_t first = sizeof(digits) - 1;

public:
    explicit DecimalNumber(std::int64_t value) noexcept
    {
        auto remaining = value > 0 ? static_cast<std::uint64_t>(value) : 0;
        digits[first] = 0;
        do
        {
            digits[--first] = static_cast<char>('0' + remaining % 10);
            remaining /= 10;
        } while (remaining);
    }
    const char *c_str() const noexcept { return digits + first; }
};

// Bounded composition: reject an incomplete segment without changing output.
template <std::size_t N>
class FixedText
{
    static_assert(N > 0, "Text needs a terminator");
    char text[N] = {};
    std::size_t length = 0;

public:
    bool Append(const char *segment, std::size_t limit = N) noexcept
    {
        if (!segment)
            return false;
        if (limit > N)
            limit = N;
        const auto added = std::strlen(segment);
        if (!limit || length >= limit || added >= limit - length)
            return false;
        std::memcpy(text + length, segment, added + 1);
        length += added;
        return true;
    }
    const char *c_str() const noexcept { return text; }
};

// Only exact %s/%d signatures and escaped %% are accepted from JSON. This
// excludes %n, wrong vararg types and extra/missing arguments. An optional
// suffix with no arguments may be translated as an empty string.
inline bool ValidFormat(const char *format, const char *arguments) noexcept
{
    if (!format || !arguments)
        return false;
    for (; *format; ++format)
    {
        if (*format != '%')
            continue;
        ++format;
        if (*format == '%')
            continue;
        if (*format != 's' && *format != 'd')
            return false;
        if (!*arguments || *format != *arguments++)
            return false;
    }
    return !*arguments;
}
}
}
