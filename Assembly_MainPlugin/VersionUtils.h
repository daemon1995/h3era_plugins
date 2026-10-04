#pragma once

#include <algorithm>
#include <string>
#include <vector>

namespace assemblyVersion
{
struct ParsedVersion
{
    std::vector<std::string> numbers;
    std::vector<std::string> prerelease;
};

inline bool IsDigit(const char c) noexcept
{
    return c >= '0' && c <= '9';
}

inline bool ReadIdentifiers(const std::string &text, std::vector<std::string> &identifiers)
{
    size_t start = 0;
    while (start < text.size())
    {
        const size_t end = text.find('.', start);
        const std::string identifier = text.substr(start, end == std::string::npos ? end : end - start);
        if (identifier.empty() ||
            !std::all_of(identifier.begin(), identifier.end(), [](const char c) {
                return IsDigit(c) || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '-';
            }))
            return false;
        identifiers.push_back(identifier);
        if (end == std::string::npos)
            return true;
        start = end + 1;
    }
    return false;
}

inline bool Parse(const std::string &input, ParsedVersion &result)
{
    result = ParsedVersion{};
    const size_t first = input.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return false;
    std::string text = input.substr(first, input.find_last_not_of(" \t\r\n") - first + 1);
    if (text[0] == 'v' || text[0] == 'V')
        text.erase(0, 1);

    const size_t build = text.find('+');
    if (build != std::string::npos)
    {
        std::vector<std::string> ignored;
        if (!ReadIdentifiers(text.substr(build + 1), ignored))
            return false;
        text.resize(build);
    }
    const size_t prerelease = text.find('-');
    if (prerelease != std::string::npos)
    {
        if (!ReadIdentifiers(text.substr(prerelease + 1), result.prerelease))
            return false;
        text.resize(prerelease);
    }
    size_t start = 0;
    while (start < text.size())
    {
        const size_t end = text.find('.', start);
        std::string number = text.substr(start, end == std::string::npos ? end : end - start);
        if (number.empty() || !std::all_of(number.begin(), number.end(), IsDigit))
            return false;
        const size_t nonzero = number.find_first_not_of('0');
        result.numbers.push_back(nonzero == std::string::npos ? "0" : number.substr(nonzero));
        if (end == std::string::npos)
            return true;
        start = end + 1;
    }
    return false;
}

inline int CompareNumbers(std::string left, std::string right)
{
    const size_t l = left.find_first_not_of('0');
    const size_t r = right.find_first_not_of('0');
    left = l == std::string::npos ? "0" : left.substr(l);
    right = r == std::string::npos ? "0" : right.substr(r);
    if (left.size() != right.size())
        return left.size() < right.size() ? -1 : 1;
    return left.compare(right);
}

inline bool IsNewer(const char *remote, const char *local)
{
    ParsedVersion lhs, rhs;
    if (!remote || !local || !Parse(remote, lhs) || !Parse(local, rhs))
        return false;
    const size_t count = std::max(lhs.numbers.size(), rhs.numbers.size());
    for (size_t i = 0; i < count; ++i)
    {
        const int order = CompareNumbers(i < lhs.numbers.size() ? lhs.numbers[i] : "0",
                                         i < rhs.numbers.size() ? rhs.numbers[i] : "0");
        if (order)
            return order > 0;
    }
    if (lhs.prerelease.empty() || rhs.prerelease.empty())
        return lhs.prerelease.empty() && !rhs.prerelease.empty();
    for (size_t i = 0; i < std::min(lhs.prerelease.size(), rhs.prerelease.size()); ++i)
    {
        const auto &left = lhs.prerelease[i];
        const auto &right = rhs.prerelease[i];
        const bool leftNumeric = std::all_of(left.begin(), left.end(), IsDigit);
        const bool rightNumeric = std::all_of(right.begin(), right.end(), IsDigit);
        if (leftNumeric != rightNumeric)
            return !leftNumeric;
        const int order = leftNumeric ? CompareNumbers(left, right) : left.compare(right);
        if (order)
            return order > 0;
    }
    return lhs.prerelease.size() > rhs.prerelease.size();
}
} // namespace assemblyVersion
