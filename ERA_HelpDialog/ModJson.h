#pragma once

#include "HelpDialogDependencies.h"
#include "HelpLogic.h"

// Small adapter around EraJS used by all mod-help readers.  New documents
// live under help.<mod-folder>, while help.mods.<mod-folder> remains a
// backwards-compatible fallback for existing language files.  The supported
// shape is intentionally flat and predictable:
//   help.<folder>.hotkeys[i] = { keys, name, description, type }
//   help.<folder>.categories[i] = { name, content }
// MainDlg can therefore defer mod discovery/parsing until a mod list or the
// aggregated hotkey page is actually requested, while pages create controls
// lazily when a mod is selected.
class ModJsonDocument final
{
    H3String primaryRoot;
    H3String legacyRoot;

    static H3String MakePath(const H3String &root, LPCSTR relative)
    {
        return helpdlg::JsonPath(root.String(), relative ? relative : "").c_str();
    }

    static H3String ReadPath(const H3String &path, bool &success)
    {
        LPCSTR value = EraJS::read(path.String(), success);
        return success && value ? value : h3_NullString;
    }

  public:
    explicit ModJsonDocument(LPCSTR modFolderName) : primaryRoot("help."), legacyRoot("help.mods.")
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
};
