#pragma once
#include <array>

namespace rmgExport
{
enum Field { ENABLED, MAP, ZONE, VALUE, DENSITY, FIELD_COUNT };
using Settings = std::array<int, FIELD_COUNT>;

// Capture configuration before generator values, dwelling AI values and INI
// settings are applied. A missing field must continue to inherit on import.
struct Overrides
{
    Settings inherited{{1, -1, -1, -1, -1}};
    Settings configured{{1, -1, -1, -1, -1}};
    unsigned present = 0;
    unsigned required = 0;

    void Set(int field, int value)
    {
        configured[field] = value;
        present |= 1U << field;
    }
    unsigned DifferenceMask() const
    {
        unsigned result = 0;
        for (int field = 0; field < FIELD_COUNT; ++field)
            if ((present & (1U << field)) &&
                (configured[field] != inherited[field] || (required & (1U << field))))
                result |= 1U << field;
        return result;
    }
    void CaptureNativeGenerator(int value, int density)
    {
        // -1 requests automatic calculation even when its resolved native
        // value is numeric. It differs only from a fixed inherited override.
        const int generated[] = {value, density};
        for (int index = 0; index < 2; ++index)
        {
            const int field = index == 0 ? VALUE : DENSITY;
            const bool automatic = (present & (1U << field)) && configured[field] == -1;
            if (inherited[field] == -1 && !automatic && generated[index] >= 0)
                inherited[field] = generated[index];
        }
    }
};
} // namespace rmgExport
