#include "MainDlg.h"
#include "GuideDlg.h"
#include "HeaderPage.h"
#include "HotkeysPage.h"
#include "ModPage.h"
#include "ModListDlg.h"
#include "PlaceholderPage.h"
#include "SearchPanel.h"
#include <shellapi.h>

#pragma comment(lib, "Shell32.lib")
// Stable undecorated API names for GetProcAddress, in addition to stdcall names.
#pragma comment(linker, "/EXPORT:ERAHelp_ShowDialog=_ERAHelp_ShowDialog@8")
#pragma comment(linker, "/EXPORT:ERAHelp_ShowObject=_ERAHelp_ShowObject@8")
#pragma comment(linker, "/EXPORT:ERAHelp_ShowObjectHint=_ERAHelp_ShowObjectHint@12")
extern const H3Town *townFromClick;
extern bool helpDialogInitialized;

namespace main
{
MainDlg *MainDlg::instance = nullptr;
namespace
{
struct Session
{
    bool loaded = false;
    std::map<eHelpPage, helpdlg::CatalogueFilter> filters;
    std::map<std::string, int> modCategories;
    std::string modFolder;
    helpdlg::HotkeyFilter hotkeys;
    std::map<std::string, int> modScroll;
} session;
const eHelpPage kPages[] = {eHelpPage::CREATURES, eHelpPage::ARTIFACTS,        eHelpPage::HEROES,
                            eHelpPage::SPELLS,    eHelpPage::SECONDARY_SKILLS, eHelpPage::TOWNS};
bool IsCatalogue(eHelpPage page)
{
    return std::find(std::begin(kPages), std::end(kPages), page) != std::end(kPages);
}
bool ValidPage(eHelpPage page)
{
    return IsCatalogue(page) || page == eHelpPage::MODS || page == eHelpPage::HOTKEYS;
}
std::string ReadIni(LPCSTR key)
{
    h3_TextBuffer[0] = 0;
    if (Era::ReadStrFromIni)
        Era::ReadStrFromIni(key, "Help", MainDlg::iniPath, h3_TextBuffer);
    return h3_TextBuffer;
}
int ReadInt(LPCSTR key, int fallback = 0)
{
    return helpdlg::ParseInt(ReadIni(key), fallback);
}
void WriteIni(LPCSTR key, const std::string &value)
{
    if (Era::WriteStrToIni)
        Era::WriteStrToIni(key, value.c_str(), "Help", MainDlg::iniPath);
}
void LoadSession()
{
    if (session.loaded)
        return;
    session.loaded = true;
    session.modFolder = ReadIni("LastMod");
    session.hotkeys.context = ReadInt("HotkeyContext");
    session.hotkeys.firstRow = ReadInt("HotkeyRow");
    session.hotkeys.query = ReadIni("HotkeyQuery");
    const bool migrateSort = ReadInt("SortSchema") < 1;
    for (auto page : kPages)
    {
        auto &filter = session.filters[page];
        const H3String prefix = H3String::Format("Page.%d", static_cast<int>(page));
        filter.category = ReadInt(H3String::Format("%s.Category", prefix.String()).String());
        filter.levels = static_cast<unsigned>(ReadInt(H3String::Format("%s.Levels", prefix.String()).String()));
        filter.sort = migrateSort ? 1 : ReadInt(H3String::Format("%s.Sort", prefix.String()).String(), 1);
        filter.firstRow = ReadInt(H3String::Format("%s.Row", prefix.String()).String());
        filter.selectedId = ReadInt(H3String::Format("%s.Selected", prefix.String()).String(), -1);
        filter.query = ReadIni(H3String::Format("%s.Query", prefix.String()).String());
        for (int facet = 0; facet < 4; ++facet)
            filter.facets[facet] = ReadInt(H3String::Format("%s.Facet%d", prefix.String(), facet).String());
    }
}
} // namespace

MainDlg::MainDlg(int width, int height, int x, int y, eHelpPage page, int subtype)
    : H3Dlg(width, height, x, y, false, false), initialPage(page), activePage(page), initialSubtype(subtype)
{
    helpdlg::AddSafeBackground(*this);
    instance = this;
    LoadSession();
    bodyHeight = height - bodyY - 56;
    contentWidth = width - contentX - 8;
    header = new HeaderPage(8, 8, width - 16, 84, this);
    auto *ok = helpdlg::CloseButtonRight(*this);
    if (ok)
    {
        ok->AddHotkey(eVKey::H3VK_ESCAPE);
        ok->AddHotkey(eVKey::H3VK_F1);
    }
    hintBar = CreateHint(10, height - 42, width - 100, 30);
}
MainDlg::~MainDlg()
{
    StoreState();
    if (activeSection)
        activeSection->SetVisible(FALSE);
    delete header;
    for (auto &section : catalogueSections)
        delete section.second;
    delete modSection;
    delete hotkeysSection;
    delete placeholder;
    delete searchPanel;
    for (auto *mod : mods)
        delete mod;
    instance = nullptr;
    if (resultItemId == buttons::RESIZE_DLG)
        P_WindowManager->resultItemID = buttons::RESIZE_DLG;
}
const helpdlg::Catalogue &MainDlg::EnsureCatalogue(eHelpPage page)
{
    auto found = catalogues.find(page);
    if (found == catalogues.end())
        found = catalogues.emplace(page, helpdlg::BuildCatalogue(page)).first;
    return found->second;
}
CatalogueSection *MainDlg::EnsureCatalogueSection(eHelpPage page)
{
    auto found = catalogueSections.find(page);
    if (found != catalogueSections.end())
        return found->second;
    auto *section =
        new CatalogueSection(8, bodyY, 220, bodyHeight, contentX, contentWidth, this, EnsureCatalogue(page));
    section->SetFilter(session.filters[page]);
    catalogueSections.emplace(page, section);
    return section;
}
bool MainDlg::EnsureModsLoaded()
{
    if (modsLoaded)
        return !mods.empty();
    modsLoaded = true;
    std::vector<std::string> names = modList::GetEraModList(TRUE);
    // Built-in references are independent of physical mod folders.
    // Every remaining entry comes from the active VFS mod list.
    names.insert(names.begin(), "era help");
    names.push_back("heroes iii");
    if (GetModuleHandleA("_HD3_.dll") || GetModuleHandleA("HD_WOG.dll"))
        names.push_back("hd mod");
    std::vector<std::string> seen;
    UINT id = 0;
    for (const auto &name : names)
    {
        const std::string folder = helpdlg::Lower(name);
        if (std::find(seen.begin(), seen.end(), folder) != seen.end())
            continue;
        seen.push_back(folder);
        auto *mod = new ModInformation(name.c_str(), id++,
                                       folder != "era help" && folder != "heroes iii" && folder != "hd mod");
        if (mod->hasSomeInfo)
            mods.push_back(mod);
        else
            delete mod;
    }
    for (auto *mod : mods)
    {
        const std::string folder = mod->path.String();
        if (session.modCategories.find(folder) == session.modCategories.end())
            session.modCategories[folder] = ReadInt(("Mod." + folder + ".Category").c_str());
        if (session.modFolder == mod->path.String())
            activeMod = mod;
    }
    if (!activeMod && !mods.empty())
        activeMod = mods.front();
    return !mods.empty();
}
void MainDlg::ShowSection(DlgSection *section)
{
    for (auto &entry : catalogueSections)
        if (entry.second != section)
            entry.second->SetVisible(FALSE);
    if (modSection && modSection != section)
        modSection->SetVisible(FALSE);
    if (hotkeysSection && hotkeysSection != section)
        hotkeysSection->SetVisible(FALSE);
    if (placeholder && placeholder != section)
        placeholder->SetVisible(FALSE);
    if (searchPanel && searchPanel != section)
        searchPanel->SetVisible(FALSE);
    activeSection = section;
    if (section)
    {
        section->SetVisible(TRUE);
        section->Redraw();
        activeSubtype = section->Subtype();
    }
    header->SetActiveButton(section == searchPanel ? buttons::SEARCH : static_cast<int>(activePage));
    if (P_WindowManager->lastDlg == this)
        Redraw();
}
void MainDlg::ShowEmpty(eHelpPage page, LPCSTR message)
{
    if (!placeholder)
        placeholder = new PlaceholderSection(contentX, bodyY, contentWidth, bodyHeight, this);
    placeholder->SetTitle(message);
    activePage = page;
    activeSubtype = 0;
    ShowSection(placeholder);
}
void MainDlg::ShowMod(ModInformation *mod, int subtype)
{
    if (!mod)
        return;
    if (activeMod && modSection)
    {
        session.modCategories[activeMod->path.String()] = modSection->Subtype();
        session.modScroll[std::string(activeMod->path.String()) + "." + std::to_string(modSection->Subtype())] =
            modSection->ScrollPosition();
    }
    activeMod = mod;
    session.modFolder = mod->path.String();
    if (!modSection)
        modSection = new ModSection(8, bodyY, 220, bodyHeight, contentX, bodyY, contentWidth, bodyHeight, this);
    modSection->SetMod(mod);
    const int category = subtype >= 0 ? subtype : session.modCategories[session.modFolder];
    modSection->SetSubtype(category);
    const std::string scrollKey = session.modFolder + "." + std::to_string(modSection->Subtype());
    if (session.modScroll.find(scrollKey) == session.modScroll.end())
        session.modScroll[scrollKey] = ReadInt(("ModScroll." + scrollKey).c_str());
    modSection->SetScrollPosition(session.modScroll[scrollKey]);
    activePage = eHelpPage::MODS;
    ShowSection(modSection);
}
BOOL MainDlg::ShowPage(eHelpPage page, int subtype)
{
    if (!ValidPage(page))
        return FALSE;
    if (IsCatalogue(page))
    {
        auto *section = EnsureCatalogueSection(page);
        if (subtype >= 0)
            section->SetSubtype(subtype);
        activePage = page;
        ShowSection(section);
    }
    else if (page == eHelpPage::MODS)
    {
        if (EnsureModsLoaded())
            ShowMod(activeMod, subtype);
        else
            ShowEmpty(page, helpdlg::Text("help.ui.no_mods",
                                          "No mod help is available. Add help data to a language JSON file."));
    }
    else
    {
        EnsureModsLoaded();
        if (!hotkeysSection)
        {
            hotkeysSection =
                new HotkeysSection(8, bodyY, 220, bodyHeight, contentX, bodyY, contentWidth, bodyHeight, this, mods);
            hotkeysSection->SetFilter(session.hotkeys);
        }
        if (subtype >= 0)
            hotkeysSection->SetSubtype(subtype);
        activePage = page;
        ShowSection(hotkeysSection);
    }
    return TRUE;
}
BOOL MainDlg::OnCreate()
{
    header->SetVisible(TRUE);
    ShowPage(initialPage, initialSubtype);
    if (pendingObject >= 0 && IsCatalogue(initialPage))
        EnsureCatalogueSection(initialPage)->FocusObject(pendingObject, false);
    return TRUE;
}
void MainDlg::OnOK()
{
    Stop();
}
void MainDlg::OnCancel()
{
    Stop();
}
void MainDlg::OnClose(INT)
{
    Stop();
}
BOOL MainDlg::OnMouseWheel(INT)
{
    return TRUE;
}

void MainDlg::Search()
{
    if (searchPanel)
    {
        searchPanel->SetQuery(header->Query());
        if (activeSection != searchPanel)
        {
            ShowSection(searchPanel);
            header->FocusSearch();
        }
        return;
    }
    std::vector<helpdlg::SearchEntry> entries;
    for (auto page : kPages)
        for (const auto &object : EnsureCatalogue(page).entries)
        {
            helpdlg::SearchEntry entry;
            entry.page = page;
            entry.objectId = object.id;
            entry.label = std::string(helpdlg::PageName(page)) + " / " + object.name;
            entry.searchable = entry.label;
            entries.push_back(entry);
        }
    EnsureModsLoaded();
    for (auto *mod : mods)
        for (size_t category = 0; category < mod->categories.size(); ++category)
        {
            auto *item = mod->categories[category];
            helpdlg::SearchEntry entry;
            entry.modId = mod->id;
            entry.category = static_cast<int>(category);
            entry.label = std::string(mod->name.String()) + " / " + item->name.String();
            entry.searchable = entry.label;
            entries.push_back(entry);
            if (item == mod->hotkeysCategory)
                for (const auto &key : mod->hotkeysCategory->hotkeys)
                {
                    entry.page = eHelpPage::HOTKEYS;
                    entry.hotkeyContext = key.type;
                    entry.hotkeyId = key.id;
                    entry.hotkeyQuery = std::string(key.keys.String()) + " " + key.name.String();
                    entry.label =
                        std::string(mod->name.String()) + " / [" + key.keys.String() + "] " + key.name.String();
                    entry.searchable = entry.label;
                    entries.push_back(entry);
                }
        }
    searchPanel = new helpdlg::SearchPanel(8, bodyY, 220, bodyHeight, contentX, contentWidth, this, std::move(entries));
    searchPanel->SetQuery(header->Query());
    ShowSection(searchPanel);
    header->FocusSearch();
}

void MainDlg::CloseSearch()
{
    header->SetQuery("");
    searchQuery.clear();
    if (activeSection == searchPanel)
        ShowPage(activePage);
}

void MainDlg::NavigateSearch(const helpdlg::SearchEntry &target)
{
    ReleaseInputFocus();
    if (target.modId >= 0)
    {
        for (auto *mod : mods)
            if (mod->id == static_cast<UINT>(target.modId))
            {
                if (target.page == eHelpPage::HOTKEYS)
                {
                    ShowPage(eHelpPage::HOTKEYS);
                    hotkeysSection->FocusHotkey(mod->path.String(), target.hotkeyContext, target.hotkeyQuery);
                    Redraw();
                }
                else
                    ShowMod(mod, target.category);
                break;
            }
    }
    else
    {
        ShowPage(target.page, 0);
        EnsureCatalogueSection(target.page)->FocusObject(target.objectId, false);
    }
}

void MainDlg::PreviewSearch(const helpdlg::SearchEntry &target)
{
    if (target.modId < 0)
    {
        const auto &data = EnsureCatalogue(target.page);
        for (const auto &entry : data.entries)
            if (entry.id == target.objectId)
            {
                helpdlg::ShowObjectDetails(entry, true);
                return;
            }
        return;
    }
    for (auto *mod : mods)
    {
        if (mod->id != static_cast<UINT>(target.modId))
            continue;
        helpdlg::CatalogueEntry preview{};
        preview.name = target.label;
        preview.summary = mod->name.String();
        if (target.page == eHelpPage::HOTKEYS)
        {
            if (!mod->hotkeysCategory)
                return;
            for (const auto &key : mod->hotkeysCategory->hotkeys)
                if (key.id == target.hotkeyId)
                {
                    helpdlg::ShowHotkeyPreview(key, mod->name.String());
                    return;
                }
            return;
        }
        if (target.category < 0 || target.category >= static_cast<int>(mod->categories.size()))
            return;
        auto *category = mod->categories[target.category];
        if (!category)
            return;
        if (category == mod->hotkeysCategory)
        {
            std::vector<helpdlg::HotkeyLine> keys;
            for (const auto &key : mod->hotkeysCategory->hotkeys)
                keys.push_back({key.type, "", helpdlg::Safe(key.keys.String()), helpdlg::Safe(key.name.String()),
                                static_cast<int>(mod->id), key.id, helpdlg::Safe(key.description.String())});
            preview.description = helpdlg::GroupHotkeys(keys, helpdlg::ContextName);
        }
        else if (category && category->content)
        {
            preview.description = category->content->text.String();
            for (const auto &object : category->content->objects)
            {
                if (object.kind == eHelpObjectKind::Text)
                    preview.description += "\n\n" + helpdlg::Safe(object.value.String());
                else if (object.kind == eHelpObjectKind::Image && !preview.def && preview.pcx.empty())
                {
                    const std::string resource = helpdlg::Lower(object.value.String());
                    if (resource.size() >= 4 && resource.substr(resource.size() - 4) == ".def")
                    {
                        preview.def = object.value.String();
                        preview.frame = object.frame;
                    }
                    else
                        preview.pcx = object.value.String();
                }
            }
        }
        if (preview.description.empty())
            preview.description = helpdlg::Text("help.ui.empty_category", "No text in this category.");
        helpdlg::ShowObjectDetails(preview, true);
        return;
    }
}

void MainDlg::ExecuteAction(LPCSTR command)
{
    const std::string action = command ? command : "";
    if (action == "search")
    {
        ReleaseInputFocus();
        Search();
        header->FocusSearch();
        return;
    }
    if (action == "guide")
    {
        help::GuideDlg guide(620, 540);
        guide.Start();
        return;
    }
    const size_t separator = action.find(':');
    if (separator == std::string::npos)
        return;
    const std::string kind = helpdlg::Lower(action.substr(0, separator));
    const std::string value = action.substr(separator + 1);
    if (kind == "http" || kind == "https")
    {
        ShellExecuteA(nullptr, "open", action.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        return;
    }
    if (kind == "erm")
    {
        if (!Era::AllocErmFunc || !Era::ExecErmCmd || value.empty())
            return;
        int function = 0;
        const int numeric = helpdlg::ParseInt(value, -1);
        if (numeric >= 0)
            function = numeric;
        else
            Era::AllocErmFunc(value.c_str(), function);
        if (function > 0)
            Era::ExecErmCmd(H3String::Format("!!FU%d:P;", function).String());
        return;
    }
    if (kind == "mod")
    {
        EnsureModsLoaded();
        const size_t colon = value.rfind(':');
        const std::string folder = helpdlg::Lower(value.substr(0, colon));
        const int category = colon == std::string::npos ? 0 : helpdlg::ParseInt(value.substr(colon + 1), -1);
        if (category < 0)
            return;
        for (auto *mod : mods)
            if (folder == helpdlg::Lower(mod->path.String()))
            {
                ShowMod(mod, category);
                return;
            }
        return;
    }
    const LPCSTR kinds[] = {"creatures", "artifacts", "heroes", "spells", "skills", "towns"};
    for (int index = 0; index < 6; ++index)
        if (kind == kinds[index])
        {
            const int id = helpdlg::ParseInt(value, -1);
            if (id < 0)
                return;
            ShowObject(kPages[index], id);
            return;
        }
}
void MainDlg::ReleaseInputFocus(int keepItemId)
{
    header->ReleaseInputFocus(keepItemId);
    for (auto &entry : catalogueSections)
        entry.second->ReleaseInputFocus(keepItemId);
    if (hotkeysSection)
        hotkeysSection->ReleaseInputFocus(keepItemId);
}
BOOL MainDlg::DialogProc(H3Msg &msg)
{
    const std::string query = header->Query();
    if (query != searchQuery)
    {
        searchQuery = query;
        if (!query.empty())
            Search();
        else if (activeSection == searchPanel)
            ShowPage(activePage);
    }
    if (msg.IsLeftDown() || msg.IsLeftClick() || msg.IsRightClick())
        ReleaseInputFocus(msg.itemId);
    if (msg.IsKeyPress() && msg.IsCtrlPressed() && msg.GetKey() == eVKey::H3VK_F)
    {
        ReleaseInputFocus();
        Search();
        header->FocusSearch();
        return FALSE;
    }
    if (activeSection)
    {
        activeSection->UpdateMousePosition(msg);
        if (activeSection->ProcessMessage(msg))
        {
            activeSubtype = activeSection->Subtype();
            if (activeSection == searchPanel)
            {
                if (searchPanel->TakeBackRequest())
                    CloseSearch();
                else
                {
                    bool popup = false;
                    if (const auto *target = searchPanel->TakeSelection(popup))
                    {
                        if (popup)
                            PreviewSearch(*target);
                        else
                            NavigateSearch(*target);
                    }
                }
            }
            else if (activeSection == modSection)
            {
                const H3String action = modSection->TakeAction();
                if (!action.Empty())
                    ExecuteAction(action.String());
            }
            Redraw();
            return FALSE;
        }
    }
    if (msg.IsLeftClick())
    {
        if (msg.itemId == buttons::MODLIST)
        {
            if (!EnsureModsLoaded())
            {
                ShowPage(eHelpPage::MODS);
                return FALSE;
            }
            auto *button = GetH3DlgItem(buttons::MODLIST);
            if (!button)
                return FALSE;
            const auto layout = helpdlg::FitModDropdown(
                GetX() + button->GetX(), GetY() + button->GetY(), button->GetWidth(), button->GetHeight(),
                static_cast<int>(mods.size()), H3GameWidth::Get(), H3GameHeight::Get());
            list::ModListDlg picker(layout, mods, activeMod);
            picker.Start();
            if (picker.ResultMod())
                ShowMod(picker.ResultMod(), -1);
            return FALSE;
        }
        if (msg.itemId == buttons::SEARCH_CLEAR)
        {
            CloseSearch();
            return FALSE;
        }
        if (msg.itemId == buttons::HELP)
        {
            help::GuideDlg guide(620, 540);
            guide.Start();
            return FALSE;
        }
        if (msg.itemId == buttons::RESIZE_DLG)
        {
            resultItemId = buttons::RESIZE_DLG;
            Stop();
            return FALSE;
        }
        const eHelpPage page = static_cast<eHelpPage>(msg.itemId);
        if (ValidPage(page))
        {
            ShowPage(page);
            return FALSE;
        }
    }
    if (hintBar && msg.IsMouseOver())
        hintBar->ShowHint(&msg);
    return TRUE;
}
void MainDlg::StoreState()
{
    for (const auto &section : catalogueSections)
        session.filters[section.first] = section.second->Filter();
    if (modSection && activeMod)
    {
        session.modCategories[activeMod->path.String()] = modSection->Subtype();
        session.modScroll[std::string(activeMod->path.String()) + "." + std::to_string(modSection->Subtype())] =
            modSection->ScrollPosition();
    }
    if (hotkeysSection)
        session.hotkeys = hotkeysSection->Filter();
    WriteIni("LastMod", session.modFolder);
    WriteIni("SortSchema", "1");
    WriteIni("HotkeyContext", std::to_string(session.hotkeys.context));
    WriteIni("HotkeyRow", std::to_string(session.hotkeys.firstRow));
    WriteIni("HotkeyQuery", session.hotkeys.query);
    for (const auto &entry : session.filters)
    {
        const auto &filter = entry.second;
        const H3String prefix = H3String::Format("Page.%d", static_cast<int>(entry.first));
        WriteIni(H3String::Format("%s.Category", prefix.String()).String(), std::to_string(filter.category));
        WriteIni(H3String::Format("%s.Levels", prefix.String()).String(), std::to_string(filter.levels));
        WriteIni(H3String::Format("%s.Sort", prefix.String()).String(), std::to_string(filter.sort));
        WriteIni(H3String::Format("%s.Row", prefix.String()).String(), std::to_string(filter.firstRow));
        WriteIni(H3String::Format("%s.Selected", prefix.String()).String(), std::to_string(filter.selectedId));
        WriteIni(H3String::Format("%s.Query", prefix.String()).String(), filter.query);
        for (int facet = 0; facet < 4; ++facet)
            WriteIni(H3String::Format("%s.Facet%d", prefix.String(), facet).String(),
                     std::to_string(filter.facets[facet]));
    }
    for (const auto &mod : session.modCategories)
        WriteIni(("Mod." + mod.first + ".Category").c_str(), std::to_string(mod.second));
    for (const auto &scroll : session.modScroll)
        WriteIni(("ModScroll." + scroll.first).c_str(), std::to_string(scroll.second));
}
void MainDlg::AssignWithCalledDlg(const H3Town *town, eCreature creature) noexcept
{
    if (town)
    {
        ShowPage(eHelpPage::TOWNS, 0);
        EnsureCatalogueSection(eHelpPage::TOWNS)->FocusObject(town->type, false);
    }
    else if (creature != eCreature::UNDEFINED)
    {
        ShowPage(eHelpPage::CREATURES, 0);
        EnsureCatalogueSection(eHelpPage::CREATURES)->FocusObject(static_cast<int>(creature), false);
    }
}
void MainDlg::PrepareMainDlg(HookContext *)
{
    if (P_WindowManager->lastDlg && *reinterpret_cast<DWORD *>(P_WindowManager->lastDlg) == 0x640704 && townFromClick)
    {
        RunMainDlg(eHelpPage::TOWNS, 0, FALSE, townFromClick->type);
        return;
    }
    townFromClick = nullptr;
    eHelpPage page = static_cast<eHelpPage>(ReadInt("LastPage", static_cast<int>(eHelpPage::MODS)));
    if (!ValidPage(page))
        page = eHelpPage::MODS;
    RunMainDlg(page, -1, TRUE);
}
BOOL MainDlg::PrepareMainDlg(eHelpPage page, int subtype)
{
    return RunMainDlg(page, subtype, FALSE);
}
BOOL MainDlg::ShowObject(eHelpPage page, int objectId)
{
    if (!IsCatalogue(page) || objectId < 0)
        return FALSE;
    if (instance)
    {
        const auto &data = instance->EnsureCatalogue(page);
        if (std::none_of(data.entries.begin(), data.entries.end(),
                         [objectId](const helpdlg::CatalogueEntry &entry) { return entry.id == objectId; }))
            return FALSE;
        instance->ShowPage(page, 0);
        instance->EnsureCatalogueSection(page)->FocusObject(objectId);
        return TRUE;
    }
    const auto data = helpdlg::BuildCatalogue(page);
    if (std::none_of(data.entries.begin(), data.entries.end(),
                     [objectId](const helpdlg::CatalogueEntry &entry) { return entry.id == objectId; }))
        return FALSE;
    return RunMainDlg(page, 0, FALSE, objectId);
}
BOOL MainDlg::RunMainDlg(eHelpPage page, int subtype, BOOL remember, int objectId)
{
    if (DlgExists() || !ValidPage(page))
        return FALSE;
    const int savedResult = P_WindowManager->resultItemID;
    bool full = ReadInt("FullScreen") != 0;
    int result = 0;
    do
    {
        const int gameWidth = H3GameWidth::Get(), gameHeight = H3GameHeight::Get();
        const int width = full ? gameWidth : std::min(gameWidth, 800);
        const int height = full ? gameHeight : std::min(gameHeight, 600);
        {
            MainDlg dialog(width, height, -1, -1, page, subtype);
            dialog.pendingObject = objectId;
            dialog.Start();
            page = dialog.activePage;
            subtype = -1;
            objectId = -1;
            result = dialog.resultItemId;
        }
        if (result == buttons::RESIZE_DLG)
        {
            full = !full;
            WriteIni("FullScreen", full ? "1" : "0");
        }
    } while (result == buttons::RESIZE_DLG);
    if (remember)
    {
        WriteIni("LastPage", std::to_string(static_cast<int>(page)));
        WriteIni("LastSubtype", "0");
    }
    if (Era::SaveIni)
        Era::SaveIni(iniPath);
    P_WindowManager->resultItemID = savedResult;
    return TRUE;
}
int __fastcall MainDlg::MainMenuButtonProc(void *message)
{
    if (message && static_cast<H3Msg *>(message)->IsLeftClick())
        PrepareMainDlg();
    return TRUE;
}
} // namespace main
DllExport BOOL __stdcall ERAHelp_ShowDialog(main::eHelpPage page, int subtype)
{
    return main::MainDlg::PrepareMainDlg(page, subtype);
}
DllExport BOOL __stdcall ERAHelp_ShowObject(main::eHelpPage page, int objectId)
{
    return main::MainDlg::ShowObject(page, objectId);
}
DllExport BOOL __stdcall ERAHelp_ShowObjectHint(main::eHelpPage page, int objectId, BOOL popup)
{
    static bool showing = false;
    if (!helpDialogInitialized || showing || !P_WindowManager)
        return FALSE;
    struct CallState
    {
        bool &showing;
        H3WindowManager *window;
        int result;
        CallState(bool &busy, H3WindowManager *manager) : showing(busy), window(manager), result(manager->resultItemID)
        {
            showing = true;
        }
        ~CallState()
        {
            window->resultItemID = result;
            showing = false;
        }
    } state(showing, P_WindowManager);
    try
    {
        helpdlg::CatalogueEntry entry{};
        if (!helpdlg::TryBuildObjectEntry(page, objectId, entry))
            return FALSE;
        return helpdlg::ShowObjectDetails(entry, popup != FALSE) ? TRUE : FALSE;
    }
    catch (const std::exception &)
    {
        return FALSE;
    }
}
