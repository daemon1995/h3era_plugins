#pragma once

#include "HelpDialogDependencies.h"
#include "HelpLogic.h"

// All mod-help readers use one canonical key under help.<mod-folder>.
// Missing translations retain the key returned by ERA. The supported shape is:
//   help.<folder>.hotkeys[i] = { keys, name, description, type }
//   help.<folder>.categories[i] = { name, content }
// MainDlg can therefore defer mod discovery/parsing until a mod list or the
// aggregated hotkey page is actually requested, while pages create controls
// lazily when a mod is selected.
class ModJsonDocument final
{
    H3String root;

    static H3String MakePath(const H3String &root, LPCSTR relative)
    {
        return helpdlg::JsonPath(root.String(), relative).c_str();
    }

  public:
    explicit ModJsonDocument(LPCSTR modFolderName) : root("help.")
    {
        root.Append(modFolderName);
    }

    // ERA owns returned pointers permanently, including after later reads.
    LPCSTR Read(LPCSTR relative, bool &success) const
    {
        return EraJS::read(MakePath(root, relative).String(), success);
    }

    LPCSTR Read(LPCSTR relative) const
    {
        return EraJS::read(MakePath(root, relative).String());
    }

    int ReadInt(LPCSTR relative, bool &success) const
    {
        return EraJS::readInt(MakePath(root, relative).String(), success);
    }

    int ReadInt(LPCSTR relative) const
    {
        bool success = false;
        return ReadInt(relative, success);
    }
};
