#pragma once

#include "HelpDialogDependencies.h"

// Small adapter around EraJS used by all mod-help readers.  New documents
// live under help.<mod-folder>, while help.mods.<mod-folder> remains a
// backwards-compatible fallback for existing language files.  The supported
// shape is intentionally flat and predictable:
//   help.<folder>.hotkeys[i] = { keys, name, description, type }
//   help.<folder>.categories[i] = { name, content }
// This keeps discovery at initialization and lets pages create their controls
// lazily when a mod is selected.
class ModJsonDocument final
{
    H3String primaryRoot;
    H3String legacyRoot;

    static H3String MakePath(const H3String &root, LPCSTR relative)
    {
        if (!relative || !*relative)
            return root;
        H3String path(root);
        path.Append('.');
        path.Append(relative);
        return path;
    }

    static H3String ReadPath(const H3String &path, bool &success)
    {
        return EraJS::read(path.String(), success);
    }

  public:
    explicit ModJsonDocument(LPCSTR modFolderName)
        : primaryRoot("help."), legacyRoot("help.mods.")
    {
        primaryRoot.Append(modFolderName ? modFolderName : "");
        legacyRoot.Append(modFolderName ? modFolderName : "");
    }

    H3String Read(LPCSTR relative, bool &success) const
    {
        H3String value = ReadPath(MakePath(primaryRoot, relative), success);
        if (success)
            return value;

        return ReadPath(MakePath(legacyRoot, relative), success);
    }

    H3String Read(LPCSTR relative) const
    {
        bool success = false;
        return Read(relative, success);
    }

    int ReadInt(LPCSTR relative, bool &success) const
    {
        const H3String primaryPath = MakePath(primaryRoot, relative);
        const int value = EraJS::readInt(primaryPath.String(), success);
        if (success)
            return value;

        return EraJS::readInt(MakePath(legacyRoot, relative).String(), success);
    }

    int ReadInt(LPCSTR relative) const
    {
        bool success = false;
        return ReadInt(relative, success);
    }

    // Returns the first existing array root.  The extra probes make the
    // helper useful for both object arrays (keys/name) and text categories.
    H3String ArrayRoot(LPCSTR relative) const
    {
        const H3String roots[] = {MakePath(primaryRoot, relative), MakePath(legacyRoot, relative)};
        for (const H3String &root : roots)
        {
            bool success = false;
            H3String probe(root);
            probe.Append(".0.keys");
            EraJS::read(probe.String(), success);
            if (!success)
            {
                probe = root;
                probe.Append(".0.name");
                EraJS::read(probe.String(), success);
            }
            if (!success)
            {
                probe = root;
                probe.Append(".0.content");
                EraJS::read(probe.String(), success);
            }
            if (success)
                return root;
        }
        return h3_NullString;
    }

    const H3String &PrimaryRoot() const noexcept { return primaryRoot; }
    const H3String &LegacyRoot() const noexcept { return legacyRoot; }
};
