#include "ModInformation.h"
#include "HelpUI.h"
#include "PluginFiles.h"
namespace
{
constexpr int kMaxEntries = 4096;
void ReadObjects(const ModJsonDocument &document, const H3String &category, Content &content)
{
    for (int index = 0; index < kMaxEntries; ++index)
    {
        const H3String prefix = H3String::Format("%s.objects.%d", category.String(), index);
        bool found = false;
        const LPCSTR kind = document.Read(H3String::Format("%s.kind", prefix.String()).String(), found);
        if (!found)
            break;
        const std::string type = helpdlg::Lower(kind);
        HelpObject object;
        if (type == "image")
            object.kind = eHelpObjectKind::Image;
        else if (type == "button")
            object.kind = eHelpObjectKind::Button;
        else if (type == "link")
            object.kind = eHelpObjectKind::Link;
        else if (type == "erm" || type == "ermfunction")
            object.kind = eHelpObjectKind::ErmFunction;
        else if (type != "text")
            continue;
        object.value = document.Read(H3String::Format("%s.value", prefix.String()).String());
        if (object.kind == eHelpObjectKind::Button || object.kind == eHelpObjectKind::Link ||
            object.kind == eHelpObjectKind::ErmFunction)
        {
            const auto action = document.Read(H3String::Format("%s.action", prefix.String()).String(), found);
            if (found)
                object.action = action;
        }
        object.x = document.ReadInt(H3String::Format("%s.x", prefix.String()).String());
        object.y = document.ReadInt(H3String::Format("%s.y", prefix.String()).String());
        object.width = document.ReadInt(H3String::Format("%s.width", prefix.String()).String());
        object.height = document.ReadInt(H3String::Format("%s.height", prefix.String()).String());
        object.frame = document.ReadInt(H3String::Format("%s.frame", prefix.String()).String());
        object.zOrder = document.ReadInt(H3String::Format("%s.zOrder", prefix.String()).String());
        object.overlay = document.ReadInt(H3String::Format("%s.overlay", prefix.String()).String()) != 0;
        content.objects.push_back(object);
    }
    std::stable_sort(content.objects.begin(), content.objects.end(),
                     [](const HelpObject &a, const HelpObject &b) { return a.zOrder < b.zOrder; });
}
// Enumerate only physical files belonging to this active mod. No plugin is loaded here.
Category *MakePluginsCategory(LPCSTR folder, const ModJsonDocument &document)
{
    std::vector<std::string> files;
    std::vector<helpdlg::PluginNote> notes;
    int missing = 0;
    for (int index = 0; index < kMaxEntries && missing < 32; ++index)
    {
        const auto prefix = H3String::Format("plugins.%d", index);
        bool found = false;
        const auto file = document.Read(H3String::Format("%s.file", prefix.String()).String(), found);
        if (!found || !*file)
        {
            ++missing;
            continue;
        }
        missing = 0;
        helpdlg::PluginNote note;
        note.file = file;
        const auto name = document.Read(H3String::Format("%s.name", prefix.String()).String(), found);
        if (found)
            note.name = name;
        const auto description = document.Read(H3String::Format("%s.description", prefix.String()).String(), found);
        if (found)
            note.description = description;
        notes.push_back(std::move(note));
    }
    char executable[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameA(nullptr, executable, MAX_PATH);
    const std::string modFolder = folder ? folder : h3_NullString;
    std::string root(executable);
    const auto separator = root.find_last_of("/\\");
    if (length && length < MAX_PATH && separator != std::string::npos && !modFolder.empty() && modFolder != "." &&
        modFolder != ".." && modFolder.find_first_of("/\\:") == std::string::npos)
    {
        root.resize(separator);
        root += "\\Mods\\" + modFolder;
        files = helpdlg::EnumeratePluginFiles(root);
    }

    if (files.empty())
        return nullptr;

    auto *category = new Category();
    category->name = helpdlg::Text("help.ui.plugins");
    category->content = new Content();
    auto &text = category->content->generatedText;
    text = helpdlg::Text("help.ui.plugins_intro");
    text += "\n\n";
    text += helpdlg::GroupPluginFiles(files, notes);
    category->content->text = text.c_str();
    return category;
}
} // namespace
ModInformation::ModInformation(LPCSTR folder, UINT identifier, bool physicalMod)
    : id(identifier), path(folder), document(folder)
{
    bool hasName = false;
    name = document.Read("name", hasName);
    if (!hasName || !*name)
        name = path.String();
    std::vector<HotKey> hotkeys;
    for (const auto &record : helpdlg::ReadHotkeyRecords(
             [this](const std::string &key, bool &found) { return document.Read(key.c_str(), found); }))
        hotkeys.push_back(HotKey{static_cast<hkcategories::eType>(record.context), record.keys, record.name,
                                 record.description, record.id});
    if (!hotkeys.empty())
    {
        hotkeysCategory = new HotKeysCategory();
        hotkeysCategory->hotkeys.swap(hotkeys);
        hotkeysCategory->name = document.Read("hotkeys.name");
        hotkeysCategory->content = new Content();
        categories.push_back(hotkeysCategory);
    }
    int missing = 0;
    for (int index = 0; index < kMaxEntries && missing < 32; ++index)
    {
        const H3String prefix = H3String::Format("categories.%d", index);
        bool hasName = false, hasText = false, hasObjects = false;
        LPCSTR title = document.Read(H3String::Format("%s.name", prefix.String()).String(), hasName);
        LPCSTR text = document.Read(H3String::Format("%s.content", prefix.String()).String(), hasText);
        document.Read(H3String::Format("%s.objects.0.kind", prefix.String()).String(), hasObjects);
        if (!hasName && !hasText && !hasObjects)
        {
            ++missing;
            continue;
        }
        missing = 0;
        auto *category = new Category();
        category->name = title;
        category->content = new Content();
        if (hasText)
            category->content->text = text;
        ReadObjects(document, prefix, *category->content);
        categories.push_back(category);
    }
    bool hasDescription = false;
    const LPCSTR description = document.Read("description", hasDescription);
    if (hasDescription && *description)
    {
        auto *overview = new Category();
        overview->name = helpdlg::Text("help.ui.overview");
        overview->content = new Content();
        overview->content->text = description;
        categories.push_back(overview);
    }
    // Append generated sections so existing navigation indices stay valid.
    if (physicalMod)
        if (auto *plugins = MakePluginsCategory(folder, document))
            categories.push_back(plugins);
    hasSomeInfo = !categories.empty();
}
ModInformation::~ModInformation()
{
    for (auto *category : categories)
        delete category;
}
