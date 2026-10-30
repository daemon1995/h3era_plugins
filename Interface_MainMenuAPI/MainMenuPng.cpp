#include "framework.h"
#include "MainMenuPng.h"
#include <fstream>
#include <memory>
#include <unordered_map>

namespace mainmenu
{
namespace png
{
namespace
{
constexpr size_t MAX_IMAGE_BYTES = 64 * 1024 * 1024;
struct Image
{
    int width = 0, height = 0;
    std::vector<unsigned char> pixels; // Unpremultiplied RGBA, original PNG dimensions.
};
struct PngHeader
{
    unsigned width, height;
    unsigned char bitDepth, colorType, compression, filter, interlace, padding[3];
};
struct Libraries
{
    void *(__cdecl *pngNew)(int) = nullptr;
    void (__cdecl *pngFree)(void *) = nullptr;
    int (__cdecl *pngBuffer)(void *, const void *, size_t) = nullptr;
    int (__cdecl *pngLimits)(void *, unsigned, unsigned) = nullptr;
    int (__cdecl *pngHeader)(void *, PngHeader *) = nullptr;
    int (__cdecl *pngSize)(void *, int, size_t *) = nullptr;
    int (__cdecl *pngDecode)(void *, void *, size_t, int, int) = nullptr;
    void *(__cdecl *zipOpen)(const char *, int, char) = nullptr;
    void (__cdecl *zipClose)(void *) = nullptr;
    int (__cdecl *zipCount)(void *) = nullptr;
    int (__cdecl *zipIndex)(void *, int) = nullptr;
    int (__cdecl *zipEntryClose)(void *) = nullptr;
    const char *(__cdecl *zipName)(void *) = nullptr;
    int (__cdecl *zipRead)(void *, void *, size_t) = nullptr;
    long long (__cdecl *zipSize)(void *) = nullptr;

    Libraries()
    {
        const auto spng = GetModuleHandleA("libspng.dll");
        const auto zip = GetModuleHandleA("kubazip.dll");
#define PNG_IMPORT(field, module, symbol) field = reinterpret_cast<decltype(field)>(GetProcAddress(module, symbol))
        PNG_IMPORT(pngNew, spng, "spng_ctx_new");
        PNG_IMPORT(pngFree, spng, "spng_ctx_free");
        PNG_IMPORT(pngBuffer, spng, "spng_set_png_buffer");
        PNG_IMPORT(pngLimits, spng, "spng_set_image_limits");
        PNG_IMPORT(pngHeader, spng, "spng_get_ihdr");
        PNG_IMPORT(pngSize, spng, "spng_decoded_image_size");
        PNG_IMPORT(pngDecode, spng, "spng_decode_image");
        PNG_IMPORT(zipOpen, zip, "zip_open");
        PNG_IMPORT(zipClose, zip, "zip_close");
        PNG_IMPORT(zipCount, zip, "zip_entries_total");
        PNG_IMPORT(zipIndex, zip, "zip_entry_openbyindex");
        PNG_IMPORT(zipEntryClose, zip, "zip_entry_close");
        PNG_IMPORT(zipName, zip, "zip_entry_name");
        PNG_IMPORT(zipRead, zip, "zip_entry_noallocread");
        PNG_IMPORT(zipSize, zip, "zip_entry_size");
#undef PNG_IMPORT
    }
    bool CanDecode() const { return pngNew && pngFree && pngBuffer && pngLimits && pngHeader && pngSize && pngDecode; }
    bool CanReadZip() const
    {
        return zipOpen && zipClose && zipCount && zipIndex && zipEntryClose && zipName && zipRead && zipSize;
    }
};

std::string Normalize(std::string path)
{
    for (auto &c : path)
    {
        if (c == '/')
            c = '\\';
        else if (c >= 'A' && c <= 'Z')
            c += 'a' - 'A';
    }
    return path;
}

std::string GamePath(const std::string &relative)
{
    char exe[MAX_PATH];
    if (!GetModuleFileNameA(nullptr, exe, MAX_PATH))
        return relative;
    const std::string path = exe;
    return path.substr(0, path.find_last_of('\\') + 1) + relative;
}

struct ZipEntry { std::string archive; int index; };
std::unordered_map<std::string, ZipEntry> zipEntries;
std::unordered_map<std::string, Image> images;
bool zipIndexed = false;

void IndexZips(Libraries &lib)
{
    if (zipIndexed || !lib.CanReadZip())
        return;
    zipIndexed = true;
    WIN32_FIND_DATAA found;
    const auto search = FindFirstFileA(GamePath("Data\\*.zip").c_str(), &found);
    if (search == INVALID_HANDLE_VALUE)
        return;
    do
    {
        if (found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;
        const auto archive = GamePath(std::string("Data\\") + found.cFileName);
        auto *zip = lib.zipOpen(archive.c_str(), 6, 'r');
        if (!zip)
            continue;
        const int count = lib.zipCount(zip);
        for (int i = 0; i < count; ++i)
        {
            if (lib.zipIndex(zip, i) != 0)
                continue;
            const char *name = lib.zipName(zip);
            if (name)
            {
                auto key = Normalize(name);
                // ERA's ZipFs also keeps the first archive containing a path.
                if (key.compare(0, 10, "data\\defs\\") == 0 || key.compare(0, 9, "data\\pcx\\") == 0)
                    zipEntries.emplace(key, ZipEntry{archive, i});
            }
            lib.zipEntryClose(zip);
        }
        lib.zipClose(zip);
    } while (FindNextFileA(search, &found));
    FindClose(search);
}

bool ReadPng(Libraries &lib, const std::string &path, std::vector<unsigned char> &bytes)
{
    std::ifstream file(GamePath(path), std::ios::binary | std::ios::ate);
    if (file)
    {
        const auto size = file.tellg();
        if (size <= 0 || size > MAX_IMAGE_BYTES)
            return false;
        bytes.resize(static_cast<size_t>(size));
        file.seekg(0);
        return static_cast<bool>(file.read(reinterpret_cast<char *>(bytes.data()), bytes.size()));
    }
    IndexZips(lib);
    const auto entry = zipEntries.find(path);
    if (entry == zipEntries.end())
        return false;
    auto *zip = lib.zipOpen(entry->second.archive.c_str(), 6, 'r');
    if (!zip)
        return false;
    bool success = false;
    if (lib.zipIndex(zip, entry->second.index) == 0)
    {
        const auto size = lib.zipSize(zip);
        if (size > 0 && size <= MAX_IMAGE_BYTES)
        {
            bytes.resize(static_cast<size_t>(size));
            success = lib.zipRead(zip, bytes.data(), bytes.size()) >= 0;
        }
        lib.zipEntryClose(zip);
    }
    lib.zipClose(zip);
    return success;
}

const Image *LoadImage(const std::string &path)
{
    const auto cached = images.find(path);
    if (cached != images.end())
        return cached->second.pixels.empty() ? nullptr : &cached->second;
    Image &image = images[path];
    // Only replace frames whose static composition was explicitly configured.
    const auto key = "era.png_backs." + path + ".file";
    bool configured = false;
    const char *back = EraJS::read(key.c_str(), configured);
    if (!configured || !back || !*back)
        return nullptr;
    static Libraries lib;
    std::vector<unsigned char> bytes;
    if (!lib.CanDecode() || !ReadPng(lib, path, bytes))
        return nullptr;
    void *context = lib.pngNew(0);
    if (!context)
        return nullptr;
    PngHeader header{};
    size_t size = 0;
    if (!lib.pngBuffer(context, bytes.data(), bytes.size()) && !lib.pngLimits(context, 4096, 4096) &&
        !lib.pngHeader(context, &header) && !lib.pngSize(context, 1, &size) && size > 0 &&
        size <= MAX_IMAGE_BYTES && size == static_cast<size_t>(header.width) * header.height * 4)
    {
        image.pixels.resize(size);
        // SPNG_FMT_RGBA8 / SPNG_DECODE_TRNS, matching ERA's decoder.
        if (lib.pngDecode(context, image.pixels.data(), size, 1, 1))
            image.pixels.clear();
        else
        {
            image.width = header.width;
            image.height = header.height;
        }
    }
    lib.pngFree(context);
    return image.pixels.empty() ? nullptr : &image;
}

struct Backdrop
{
    H3LoadedPcx16 *image = nullptr;
    int bytesPerPixel = 0;
    ~Backdrop() { if (image) image->Destroy(); }
};
std::unordered_map<H3BaseDlg *, std::unique_ptr<Backdrop>> backdrops;
H3BaseDlg *capturingMenu = nullptr;
LoHook *renderHooks[4] = {};
bool hooksEnabled = false;

void SetHooksEnabled(bool enabled)
{
    if (enabled == hooksEnabled)
        return;
    for (auto *hook : renderHooks)
        if (hook)
            enabled ? hook->Apply() : hook->Undo();
    hooksEnabled = enabled;
}

bool HasBackdrop(void *buffer)
{
    auto *wm = H3WindowManager::Get();
    auto *screen = wm->GetDrawBuffer();
    return screen && screen->buffer == buffer &&
           backdrops.count(capturingMenu ? capturingMenu : wm->lastDlg) != 0;
}

// Draw from the clean screen snapshot, never from pixels left by a previous frame.
bool DrawImage(const std::string &path, int mirrorWidth, int sx, int sy, int width, int height,
          void *buffer, int dx, int dy, int dstWidth, int dstHeight, int pitch, bool mirror)
{
    auto *wm = H3WindowManager::Get();
    auto *dlg = capturingMenu ? capturingMenu : wm->lastDlg;
    const auto entry = backdrops.find(dlg);
    auto *screen = wm->GetDrawBuffer();
    if (entry == backdrops.end() || !screen || buffer != screen->buffer)
        return false;
    const auto &back = *entry->second;
    const int bpp = H3BitMode::Get();
    if (!back.image || bpp != back.bytesPerPixel || (bpp != 2 && bpp != 4) ||
        screen->width != back.image->width || screen->height != back.image->height ||
        pitch != screen->scanlineSize)
        return false;
    const auto *png = LoadImage(path);
    if (!png)
        return false;
    if (mirror)
        sx = mirrorWidth - sx - width;
    const int left = std::max(0, -dx), top = std::max(0, -dy);
    const int right = std::min(width, std::min(dstWidth, screen->width) - dx);
    const int bottom = std::min(height, std::min(dstHeight, screen->height) - dy);
    for (int y = top; y < bottom; ++y)
        for (int x = left; x < right; ++x)
        {
            auto *dst = screen->buffer + (dy + y) * pitch + (dx + x) * bpp;
            const auto *clean = back.image->buffer + (dy + y) * back.image->scanlineSize + (dx + x) * bpp;
            const int px = sx + (mirror ? width - 1 - x : x), py = sy + y;
            const unsigned char *pixel = px >= 0 && py >= 0 && px < png->width && py < png->height
                                             ? &png->pixels[(py * png->width + px) * 4] : nullptr;
            const unsigned alpha = pixel ? pixel[3] : 0;
            if (bpp == 4)
            {
                for (int c = 0; c < 3; ++c)
                    dst[c] = alpha ? (pixel[2 - c] * alpha + clean[c] * (255 - alpha) + 127) / 255 : clean[c];
                dst[3] = clean[3];
            }
            else
            {
                const unsigned color = *reinterpret_cast<const unsigned short *>(clean);
                const unsigned r = (color >> 11) * 255 / 31;
                const unsigned g = ((color >> 5) & 63) * 255 / 63;
                const unsigned b = (color & 31) * 255 / 31;
                if (!alpha)
                    *reinterpret_cast<unsigned short *>(dst) = static_cast<unsigned short>(color);
                else
                {
                    const unsigned red = (pixel[0] * alpha + r * (255 - alpha) + 127) / 255;
                    const unsigned green = (pixel[1] * alpha + g * (255 - alpha) + 127) / 255;
                    const unsigned blue = (pixel[2] * alpha + b * (255 - alpha) + 127) / 255;
                    *reinterpret_cast<unsigned short *>(dst) = (red >> 3 << 11) | (green >> 2 << 5) | (blue >> 3);
                }
            }
        }
    return true;
}

bool Draw(H3LoadedDef *def, int group, int frame, int sx, int sy, int width, int height,
          void *buffer, int dx, int dy, int dstWidth, int dstHeight, int pitch, bool mirror)
{
    if (!def || !HasBackdrop(buffer))
        return false;
    auto *wm = H3WindowManager::Get();
    auto *dlg = capturingMenu ? capturingMenu : wm->lastDlg;
    auto *panel = NotificationPanel::instance;
    if (group == 0 && !mirror && panel && panel->parentDlg == dlg &&
        panel->DrawButtonBackground(def, frame, sx, sy, width, height, buffer, dx, dy,
                                    dstWidth, dstHeight, pitch))
        return true;
    if (!background::Enabled())
        return false;
    const auto path = Normalize(H3String::Format("data\\defs\\%s\\%d_%d.png", def->GetName(), group, frame).String());
    return DrawImage(path, def->widthDEF, sx, sy, width, height, buffer, dx, dy, dstWidth, dstHeight, pitch, mirror);
}

int ReturnFromDraw(HookContext *c, int argumentCount)
{
    c->return_address = c->Pop();
    c->esp += argumentCount * sizeof(int);
    return NO_EXEC_DEFAULT;
}

_LHF_(DrawGroup)
{
    const auto *a = reinterpret_cast<const int *>(c->esp);
    if (!HasBackdrop(reinterpret_cast<void *>(a[7])))
        return EXEC_DEFAULT;
    if (Draw(reinterpret_cast<H3LoadedDef *>(c->ecx), a[1], a[2], a[3], a[4], a[5], a[6],
             reinterpret_cast<void *>(a[7]), a[8], a[9], a[10], a[11], a[12], (a[13] & 0xFF) != 0))
        return ReturnFromDraw(c, 14);
    return EXEC_DEFAULT;
}

_LHF_(DrawGroupRelative)
{
    const auto *a = reinterpret_cast<const int *>(c->esp);
    if (!HasBackdrop(reinterpret_cast<void *>(a[7])))
        return EXEC_DEFAULT;
    auto *dlg = H3WindowManager::Get()->lastDlg;
    if (dlg && Draw(reinterpret_cast<H3LoadedDef *>(c->ecx), a[1], a[2], a[3], a[4], a[5], a[6],
                    reinterpret_cast<void *>(a[7]), a[8] + dlg->GetX(), a[9] + dlg->GetY(), a[10], a[11],
                    a[12], (a[13] & 0xFF) != 0))
        return ReturnFromDraw(c, 14);
    return EXEC_DEFAULT;
}

_LHF_(DrawTitle)
{
    const auto *a = reinterpret_cast<const int *>(c->esp);
    void *buffer = reinterpret_cast<void *>(a[5]);
    if (!HasBackdrop(buffer) || !background::Enabled())
        return EXEC_DEFAULT;
    const char *title = nullptr;
    const auto *pcx = reinterpret_cast<H3ResourceItem *>(c->ecx);
    if (pcx)
    {
        const char *name = pcx->GetName();
        // WoG names and their original-game aliases (see ERA's missing redirections).
        if (!lstrcmpiA(name, "znewgam.pcx") || !lstrcmpiA(name, "newgame.pcx"))
            title = "znewgam";
        else if (!lstrcmpiA(name, "zloadgam.pcx") || !lstrcmpiA(name, "loadgame.pcx"))
            title = "zloadgam";
    }
    if (title && DrawImage(std::string("data\\pcx\\mainmenu\\") + title + ".png", 0, a[1], a[2], a[3], a[4],
                           buffer, a[6], a[7], a[8], a[9], a[10], false))
        return ReturnFromDraw(c, 11);
    return EXEC_DEFAULT;
}

void __stdcall Reload(Era::TEvent *)
{
    background::ReleaseCache();
    images.clear();
    zipEntries.clear();
    zipIndexed = false;
}
} // namespace

void Capture(H3BaseDlg *dlg)
{
    if (!dlg)
        return;
    auto *screen = H3WindowManager::Get()->GetDrawBuffer();
    if (!screen || !screen->buffer)
        return;
    auto &back = backdrops[dlg];
    const int bpp = H3BitMode::Get();
    if (bpp != 2 && bpp != 4)
        return;
    if (!back || !back->image || back->bytesPerPixel != bpp ||
        back->image->width != screen->width || back->image->height != screen->height)
    {
        back.reset(new Backdrop);
        back->image = H3LoadedPcx16::Create(screen->width, screen->height);
    }
    if (!back->image)
    {
        backdrops.erase(dlg);
        return;
    }
    back->image->CopyRegion(screen, 0, 0);
    back->bytesPerPixel = bpp;
    SetHooksEnabled(true);
}

H3BaseDlg *SetDrawingMenu(H3BaseDlg *dlg)
{
    auto *previous = capturingMenu;
    capturingMenu = dlg;
    return previous;
}

void Release(H3BaseDlg *dlg)
{
    backdrops.erase(dlg);
    if (capturingMenu == dlg)
        capturingMenu = nullptr;
    if (backdrops.empty())
        SetHooksEnabled(false);
}

void Install(PatcherInstance *pi)
{
    // Pass-through must preserve the original machine context for ERA/HD's
    // mixed hook chain, including non-menu DEFs used by ERM dialogs.
    // Created inactive; only live menu snapshots enable the render hooks.
    renderHooks[0] = pi->CreateLoHook(0x47B610, DrawGroup);
    renderHooks[1] = pi->CreateLoHook(0x47BA90, DrawGroupRelative);
    renderHooks[2] = pi->CreateLoHook(0x44DF80, DrawTitle); // PCX16
    renderHooks[3] = pi->CreateLoHook(0x44F940, DrawTitle); // PCX8
    Era::RegisterHandler(Reload, "OnAfterReloadLanguageData");
}
} // namespace png
} // namespace mainmenu
