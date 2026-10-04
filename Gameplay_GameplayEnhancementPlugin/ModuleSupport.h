#pragma once

#include "pch.h"

namespace gem
{
// Module switches are read once at startup; existing installations default to on.
inline bool ModuleEnabled(LPCSTR key, bool defaultValue = true)
{
    bool found = false;
    LPCSTR value = EraJS::read(key, found);
    if (!found || !value)
        return defaultValue;
    return !libc::strcmpi(value, "true") || !libc::strcmpi(value, "1");
}

template <typename T> class ScopedValue
{
    T &value;
    T previous;
    bool restored = false;

  public:
    explicit ScopedValue(T &target) : value(target), previous(target) {}
    ScopedValue(T &target, const T &replacement) : ScopedValue(target) { value = replacement; }
    ScopedValue(const ScopedValue &) = delete;
    ScopedValue &operator=(const ScopedValue &) = delete;
    void Restore()
    {
        if (!restored)
        {
            value = previous;
            restored = true;
        }
    }
    ~ScopedValue() { Restore(); }
};

class ScopedPatch
{
    Patch *patch;
    bool appliedHere;

  public:
    explicit ScopedPatch(Patch *target)
        // Apply returns a patch index: zero is success, negative values are errors.
        : patch(target), appliedHere(target && !target->IsApplied() && target->Apply() >= 0) {}
    ScopedPatch(const ScopedPatch &) = delete;
    ScopedPatch &operator=(const ScopedPatch &) = delete;
    ~ScopedPatch()
    {
        if (appliedHere)
            patch->Undo();
    }
};
}
