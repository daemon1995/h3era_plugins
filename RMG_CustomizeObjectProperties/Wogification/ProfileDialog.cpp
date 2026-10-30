#include "../pch.h"
#include "../Assets.h"
#include "ProfileDialog.h"
#include "../DialogInput.h"

namespace wogification
{
namespace
{
bool Image(const char *name) { return Era::PcxPngExists && Era::PcxPngExists(name); }
bool PresetSkin()
{
    return Image(rmgassets::WOGOBASE_PCX) && Image(rmgassets::WOGOPRST_PCX) && Image(rmgassets::WOGOLST_PCX) &&
           Image(rmgassets::WOGOTXT_PCX) && Image(rmgassets::WOGOPHNT_PCX);
}
std::string Absolute(const std::string &path)
{
    char buffer[MAX_PATH]{};
    const DWORD size = GetFullPathNameA(path.c_str(),MAX_PATH,buffer,nullptr);
    return size && size < MAX_PATH ? std::string(buffer) : std::string();
}
std::string Join(const std::string &directory, const std::string &name)
{ return directory + (directory.empty() || directory.back() != '\\' ? "\\" : "") + name; }
std::string FileName(const std::string &path)
{
    const auto separator = path.find_last_of("/\\");
    return separator == std::string::npos ? path : path.substr(separator+1);
}
std::string Directory(const std::string &path)
{
    const auto separator = path.find_last_of("/\\");
    return separator == std::string::npos ? std::string() : Absolute(path.substr(0,separator+1));
}
bool IsDirectory(const std::string &path)
{
    const DWORD attributes = GetFileAttributesA(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY);
}
bool ProfileName(std::string &name)
{
    if (name.empty() || name.size() > 180 || name.back() == '.' || name.back() == ' ') return false;
    for (unsigned char c : name) if (c < 32 || std::strchr("<>:\"/\\|?*",c)) return false;
    if (name == "." || name == ".." || !lstrcmpiA(name.c_str(),".ini")) return false;
    if (name.size() < 4 || lstrcmpiA(name.c_str()+name.size()-4,".ini")) name += ".ini";
    std::string device = name.substr(0,name.find('.'));
    if (!device.empty()) CharUpperBuffA(&device[0],static_cast<DWORD>(device.size()));
    return device != "CON" && device != "PRN" && device != "AUX" && device != "NUL" &&
        !(device.size() == 4 && (device.substr(0,3) == "COM" || device.substr(0,3) == "LPT") && device[3] >= '0' && device[3] <= '9');
}
void SkinScrollbar(H3DlgScrollbar *scrollbar)
{
    if (!scrollbar || !Image(rmgassets::WOGOSCV_PCX)) return;
    auto *def = H3LoadedDef::Load(rmgassets::WOGOSCR_DEF);
    auto *track = H3LoadedPcx::Load(rmgassets::WOGOSCV_PCX);
    if (!def || !track || def->widthDEF != 16 || def->heightDEF != 16 ||
        def->groupsCount <= 0 || !def->groups || !def->groups[0] || def->groups[0]->count < 5 ||
        track->width < 16 || track->height < 218)
    {
        if (def) def->Dereference();
        if (track) track->Dereference();
        return;
    }
    // Same ownership transfer as WoGOptionSkin::ApplyPresetScrollbarSkin.
    struct Access : H3DlgScrollbar
    {
        void Replace(H3LoadedDef *buttons, H3LoadedPcx *background)
        {
            auto *oldButtons = loadedBtnDef; auto *oldBackground = loadedPcx;
            loadedBtnDef = buttons; loadedPcx = background;
            if (oldButtons) oldButtons->Dereference();
            if (oldBackground) oldBackground->Dereference();
        }
    };
    reinterpret_cast<Access *>(scrollbar)->Replace(def,track);
}

class ProfileDialog : public H3Dlg
{
    enum { Accept = 30725, Cancel = 30726, Edit = 256, Path = 5, ScrollId = 9, RowBase = 1000 };
    static constexpr int Rows = 13, StartY = 144, RowHeight = 19;
    struct Entry { std::string name; bool directory; bool parent; };
    std::vector<Entry> entries;
    std::string directory, root, selectedPath, nameText;
    Draft loaded;
    const Draft &source;
    bool save, accepted = false, skin;
    int top = 0, selected = -1;
    H3DlgEdit *edit = nullptr;
    H3DlgScrollbar *scrollbar = nullptr;
    H3DlgDefButton *ok = nullptr;
    H3DlgDef *icons[Rows]{};
    H3DlgText *labels[Rows]{};
    H3DlgFrame *selection[Rows]{};

    void Surface(int x, int y, int w, int h, int id, const char *resource)
    {
        if (auto *item = H3DlgPcx::Create(x,y,w,h,id,resource))
        { item->DeActivate(); AddItem(item); }
    }
    void SyncScrollbar()
    {
        if (!scrollbar) return;
        if (entries.size() <= Rows) { scrollbar->SetTick(0); scrollbar->HideDeactivate(); return; }
        struct Access : H3DlgScrollbar { using H3DlgScrollbar::vSetTickCount; };
        const int ticks = static_cast<int>(entries.size())-Rows+1;
        if (scrollbar->GetTicksCount() != ticks) reinterpret_cast<Access *>(scrollbar)->vSetTickCount(ticks);
        scrollbar->SetTick(top); scrollbar->SetButtonPosition(); scrollbar->ShowActivate();
    }
    void Refresh(bool redraw = true)
    {
        top = std::max(0,std::min(top,std::max(0,int(entries.size())-Rows)));
        GetText(Path)->SetText(directory.c_str());
        GetText(Path)->SetHints(directory.c_str(),directory.c_str(),TRUE);
        SyncScrollbar();
        for (int row = 0; row < Rows; ++row)
        {
            const int index = top+row;
            if (index >= int(entries.size()))
            { icons[row]->HideDeactivate(); labels[row]->HideDeactivate(); selection[row]->Hide(); continue; }
            const auto &entry = entries[index];
            icons[row]->SetFrame(entry.parent ? 0 : entry.directory ? 1 : 2);
            labels[row]->SetText(entry.name.c_str());
            labels[row]->SetHints(entry.name.c_str(),entry.name.c_str(),TRUE);
            icons[row]->ShowActivate(); labels[row]->ShowActivate();
            if (selected == index) selection[row]->Show(); else selection[row]->Hide();
        }
        SyncName();
        if (redraw) Redraw();
    }
    void SyncName()
    {
        nameText = edit->GetString()->String();
        std::string name = nameText;
        const bool named = ProfileName(name);
        const auto path = Join(directory,name);
        const DWORD attributes = GetFileAttributesA(path.c_str());
        const bool exists = attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
        const bool openFolder = selected >= 0 && entries[selected].directory && nameText.empty();
        const bool valid = openFolder || (named && (save || exists));
        ok->EnableItem(valid);
    }
    void Rebuild()
    {
        entries.clear(); selected = -1; top = 0;
        const auto parent = Absolute(Join(directory,".."));
        if (!parent.empty() && lstrcmpiA(parent.c_str(),directory.c_str())) entries.push_back({"..",true,true});
        WIN32_FIND_DATAA file{};
        const HANDLE search = FindFirstFileA(Join(directory,"*").c_str(),&file);
        if (search != INVALID_HANDLE_VALUE)
        {
            do
            {
                const std::string name(file.cFileName);
                if (name == "." || name == "..") continue;
                const bool folder = (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
                if (folder || (name.size() > 4 && !lstrcmpiA(name.c_str()+name.size()-4,".ini")))
                    entries.push_back({name,folder,false});
            } while (FindNextFileA(search,&file));
            FindClose(search);
        }
        std::sort(entries.begin(),entries.end(),[](const Entry &a, const Entry &b) {
            if (a.parent != b.parent) return a.parent;
            if (a.directory != b.directory) return a.directory;
            const int order = lstrcmpiA(a.name.c_str(),b.name.c_str());
            return order ? order < 0 : a.name < b.name;
        });
    }
    void Open(const std::string &path)
    {
        const auto target = Absolute(path);
        if (target.empty() || !IsDirectory(target))
        { H3Messagebox(Text("profileFolderError")); return; }
        directory = target; edit->SetText(""); edit->SetFocus(FALSE);
        Rebuild(); Refresh();
    }
    int RowIndex(int id) const
    { return id >= RowBase && id < RowBase+Rows*2 ? top+(id-RowBase)/2 : -1; }
    void Select(int index)
    {
        if (index < 0 || index >= int(entries.size())) return;
        selected = index; edit->SetFocus(FALSE);
        edit->SetText(entries[index].directory ? "" : entries[index].name.c_str());
        if (selected < top) top = selected;
        if (selected >= top+Rows) top = selected-Rows+1;
        Refresh();
    }
    bool Activate(int id)
    {
        if (id == Accept) { OnOK(); return true; }
        if (id == Cancel) { OnCancel(); return true; }
        return false;
    }
    static void __fastcall Scroll(INT32 value, H3BaseDlg *owner)
    {
        auto *dlg = static_cast<ProfileDialog *>(owner);
        dlg->top = value; dlg->Refresh();
    }
    INT vDialogProc(H3Msg &msg) override
    {
        H3Msg event = msg;
        rmginput::BlurOutside(*this,event,[](H3DlgEdit *) {});
        const int result = H3Dlg::vDialogProc(event);
        if (!endDialog && nameText != edit->GetString()->String())
        { SyncName(); RedrawItem(Accept); }
        msg = event; return result;
    }
    INT vShow(INT zorder, BOOL8 draw) override
    {
        const int result = H3Dlg::vShow(zorder,draw);
        if (save) edit->SetFocus(TRUE);
        return result;
    }
    BOOL OnLeftClick(INT id, H3Msg &) override
    {
        if (Activate(id)) return FALSE;
        const int index = RowIndex(id);
        if (index >= 0 && index < int(entries.size())) { Select(index); return FALSE; }
        return TRUE;
    }
    VOID OnClose(INT id) override { Activate(id); }
    BOOL OnDoubleClick(INT id, H3Msg &) override
    {
        const int index = RowIndex(id);
        if (index < 0 || index >= int(entries.size())) return TRUE;
        Select(index);
        if (entries[index].directory) Open(Join(directory,entries[index].name)); else OnOK();
        return FALSE;
    }
    BOOL OnMouseWheel(INT direction) override
    { if (direction) { top += direction > 0 ? -1 : 1; Refresh(); } return FALSE; }
    BOOL OnKeyPress(eVKey key, eMsgFlag) override
    {
        if (key == eVKey::H3VK_UP || key == eVKey::H3VK_DOWN)
        { Select(std::max(0,std::min(int(entries.size())-1,selected+(key == eVKey::H3VK_UP ? -1 : 1)))); return FALSE; }
        if (!edit->IsFocused())
        {
            if (key == eVKey::H3VK_BACKSPACE || key == eVKey::H3VK_LEFT) { Open(Join(directory,"..")); return FALSE; }
            if (key == eVKey::H3VK_HOME) { Open(root); return FALSE; }
            if (key == eVKey::H3VK_RIGHT && selected >= 0 && entries[selected].directory)
            { Open(Join(directory,entries[selected].name)); return FALSE; }
        }
        return TRUE;
    }
    BOOL DialogProc(H3Msg &msg) override
    {
        if (msg.command == eMsgCommand::MOUSE_BUTTON && msg.subtype == eMsgSubtype::END_DIALOG) return FALSE;
        if (msg.IsRightClick())
        {
            if (rmginput::RightClick(msg))
                if (auto *item = GetH3DlgItem(msg.itemId))
                    if (const char *hint = item->GetRightClickHint()) H3Messagebox::RMB(hint);
            return FALSE;
        }
        if (msg.IsMouseOver())
        {
            auto *item = ItemAtPosition(msg);
            if (hintBar && (!item || !item->GetHint())) hintBar->ShowMessage(Text("profileNavigationHint"));
        }
        return TRUE;
    }
    VOID OnOK() override
    {
        if (selected >= 0 && entries[selected].directory && edit->GetString()->String()[0] == '\0')
        { Open(Join(directory,entries[selected].name)); return; }
        std::string name(edit->GetString()->String());
        if (!ProfileName(name)) { H3Messagebox(Text("profileFileNameError")); return; }
        const auto path = Join(directory,name);
        const DWORD attributes = GetFileAttributesA(path.c_str());
        if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY)) return;
        if (save)
        {
            if (attributes != INVALID_FILE_ATTRIBUTES)
            {
                const std::string question = std::string(Text("overwriteProfile")) + "\n" + path;
                if (!H3Messagebox::Choice(question.c_str())) return;
            }
            if (!SaveProfileFile(path,source)) { H3Messagebox(Text("saveError")); return; }
        }
        else
        {
            std::string error;
            if (!LoadProfileFile(path,loaded,error)) { H3Messagebox(error.c_str()); return; }
            if (!error.empty()) H3Messagebox(error.c_str());
        }
        selectedPath = path; accepted = true; Stop();
    }
    VOID OnCancel() override { Stop(); }
public:
    ProfileDialog(bool saving, const Draft &draft, const std::string &previous)
        : H3Dlg(512,520,-1,-1,FALSE,TRUE), source(draft), save(saving), skin(PresetSkin())
    {
        flags = 2; EnableDoubleClick(TRUE);
        root = ProfileDirectory();
        directory = Directory(previous);
        if (directory.empty() || !IsDirectory(directory)) directory = root;
        if (directory.empty()) directory = Absolute(".");
        if (skin) Surface(0,0,512,520,80,rmgassets::WOGOPRST_PCX);
        CreateText(24,20,464,26,save ? Text("profileSaveTitle") : Text("profileLoadTitle"),NH3Dlg::Text::MEDIUM,eTextColor::HIGHLIGHT,3,eTextAlignment::MIDDLE_CENTER);
        CreateText(28,50,456,66,save ? Text("profileSaveDescription") : Text("profileLoadDescription"),NH3Dlg::Text::SMALL,eTextColor::REGULAR,4,eTextAlignment::MIDDLE_CENTER);
        CreateText(32,116,448,24,"",NH3Dlg::Text::SMALL,eTextColor::HIGHLIGHT,Path,eTextAlignment::MIDDLE_LEFT);
        Surface(24,140,464,254,81,skin ? rmgassets::WOGOLST_PCX : rmgassets::RMG_BACK_PCX);
        if (!skin) if (auto *frame = CreateFrame(24,140,464,254,82,H3RGB565(H3RGB888::Highlight()))) frame->DeActivate();
        for (int row = 0; row < Rows; ++row)
        {
            selection[row] = CreateFrame(27,StartY+row*RowHeight,440,18,100+row,H3RGB565(H3RGB888::Highlight()));
            selection[row]->DeActivate()->Hide();
            icons[row] = CreateDef(29,StartY+1+row*RowHeight,16,16,RowBase+row*2,rmgassets::WNDFOLDER_DEF,0);
            labels[row] = CreateText(49,StartY+row*RowHeight,416,18,"",NH3Dlg::Text::SMALL,eTextColor::REGULAR,RowBase+row*2+1,eTextAlignment::MIDDLE_LEFT);
        }
        scrollbar = H3DlgScrollbar::Create(470,142,16,250,ScrollId,1,Scroll,FALSE,1,TRUE);
        if (scrollbar) { scrollbar->catchKeys = FALSE; if (skin) SkinScrollbar(scrollbar); AddItem(scrollbar); }
        CreateText(24,392,464,26,Text("profileSelectedFile"),NH3Dlg::Text::MEDIUM,eTextColor::HIGHLIGHT,6,eTextAlignment::MIDDLE_CENTER);
        edit = CreateEdit(26,420,460,16,180,FileName(previous).empty() ? (save ? "default.ini" : "") : FileName(previous).c_str(),NH3Dlg::Text::SMALL,eTextColor::REGULAR,eTextAlignment::MIDDLE_LEFT,skin ? rmgassets::WOGOTXT_PCX : rmgassets::WOGTEXTEDIT_PCX,Edit,TRUE,0,0);
        edit->SetAutoredraw(TRUE);
        const char *buttons = ProfileButtonDef();
        ok = CreateButton(177,447,64,30,Accept,buttons,0,1,FALSE,eVKey::H3VK_ENTER);
        auto *cancel = CreateButton(273,447,64,30,Cancel,buttons,3,4,FALSE,eVKey::H3VK_ESCAPE);
        ok->SetHint(Text("profileConfirmHint")); cancel->SetHint(Text("cancelHint"));
        hintBar = H3ObjectAllocator<H3DlgHintBar>().allocate(1);
        if (hintBar)
        {
            THISCALL_12(H3DlgHintBar *,0x5BCB70,hintBar,48,494,416,18,h3_NullString,NH3Dlg::Text::SMALL,
                        skin ? rmgassets::WOGOPHNT_PCX : rmgassets::RMGHNTBR_PCX,eTextColor::REGULAR,0,eTextAlignment::MIDDLE_CENTER,8);
            hintBar->DeActivate(); AddItem(hintBar);
        }
        Rebuild(); Refresh(false);
    }
    bool Result(Draft &draft, std::string &path)
    {
        if (!accepted) return false;
        if (!save) draft.swap(loaded);
        path = selectedPath; return true;
    }
};
}
const char *ProfileButtonDef()
{
    if (Image(rmgassets::WOGOBASE_PCX))
    {
        auto *def = H3LoadedDef::Load(rmgassets::WOGOBTN_DEF);
        if (def)
        {
            const bool valid = def->widthDEF == 64 && def->heightDEF == 30 && def->groupsCount > 0 &&
                def->groups && def->groups[0] && def->groups[0]->count >= 11;
            def->Dereference();
            if (valid) return rmgassets::WOGOBTN_DEF;
        }
    }
    return rmgassets::WOGBTTN_DEF;
}
bool SelectProfileFile(bool save, Draft &draft, std::string &path)
{
    ProfileDialog dlg(save,draft,path); dlg.Start(); return dlg.Result(draft,path);
}
}
