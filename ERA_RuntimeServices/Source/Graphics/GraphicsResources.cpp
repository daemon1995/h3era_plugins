#include "../framework.h"
#include "GraphicsResources.h"
#include "GraphicsPixels.h"
#include "../Core/HookGroup.h"
#include "../../../headers/EraPluginsAPI/GraphicsAPI.h"
#include "../../../headers/Era/GameResources.hpp"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <map>
#include <set>
#include <new>

namespace graphics_resources
{
namespace
{
using namespace era_graphics;
struct Record { H3DefFrame *frame; H3LoadedPcx *source; std::string targetPalette; };
struct State
{
    bool ready = false;
    bool registered = false;
    std::vector<Record> frames;
    std::map<std::string, FrameHandle> cache;
    std::map<H3DefFrame *, H3LoadedPcx *> palettes;
    std::set<H3LoadedDef *> pinned;
    std::set<std::string> building;
};
State &Get()
{
    // Engine atlases keep frame pointers through map resets and native shutdown.
    static auto *state = new State;
    return *state;
}
void Log(const char *message)
{
    erm_functions::Log("Graphics resources", message);
}
std::string Lower(const char *name)
{
    std::string result = name ? name : "";
    for (auto &c : result) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return result;
}
struct Image
{
    int width = 0, height = 0;
    std::vector<Pixel> pixels;
    std::vector<BYTE> indices;
    H3PcxLoader source;
};
void ReadCanvas(H3LoadedPcx16 *canvas, Image &image)
{
    image.pixels.resize(image.width * image.height);
    for (int y = 0; y < image.height; ++y)
        for (int x = 0; x < image.width; ++x)
        {
            const auto c = canvas->GetPixel(x, y);
            image.pixels[y * image.width + x] = {c.b, c.g, c.r, 255};
        }
}
int ReadImage(const FrameRequest &request, const std::string &name, Image &image)
{
    const auto dot = name.find_last_of('.');
    const std::string ext = dot == std::string::npos ? "" : name.substr(dot);
    if (ext == ".pcx")
    {
        if (!game_resources::Exists(name, false)) return MissingResource;
        H3PcxLoader pcx(name.c_str());
        if (!pcx || !pcx->buffer || pcx->width <= 0 || pcx->height <= 0 ||
            pcx->width > 4096 || pcx->height > 4096) return InvalidFrame;
        image.width = pcx->width;
        image.height = pcx->height;
        const bool png = Era::PcxPngExists && Era::PcxPngExists(name.c_str());
        if (!png && request.paletteMode == PreserveNativePcx)
        {
            image.indices.resize(image.width * image.height);
            for (int y = 0; y < image.height; ++y)
                std::copy_n(pcx->GetRow(y), image.width, image.indices.begin() + y * image.width);
            image.source.Set(pcx.Release());
            return Ok;
        }
        // PNG-backed PCX contains a name/size stub, not decoded pixels. The same
        // native draw also handles external PCX whose RGB888 table is unfilled.
        H3Pcx16Loader canvas(H3LoadedPcx16::Create(image.width, image.height));
        if (!canvas || !canvas->buffer) return OutOfMemory;
        canvas->FillRectangle(0, 0, image.width, image.height, 0, 255, 255);
        pcx->DrawToPcx16(canvas.Get(), 0, 0, TRUE);
        ReadCanvas(canvas.Get(), image);
        return Ok;
    }
    if (ext != ".def") return UnsupportedFormat;
    if (Get().building.count(name)) return RecursiveSource;
    if (!game_resources::Exists(name, true)) return MissingResource;
    H3DefLoader def(name.c_str());
    if (!def || !def->groups || request.sourceGroup >= def->groupsCount || !def->groups[request.sourceGroup] ||
        !def->groups[request.sourceGroup]->frames || request.sourceFrame >= def->groups[request.sourceGroup]->count ||
        !def->groups[request.sourceGroup]->frames[request.sourceFrame] || !def->palette565 ||
        def->widthDEF <= 0 || def->heightDEF <= 0 || def->widthDEF > 4096 || def->heightDEF > 4096) return InvalidFrame;
    image.width = def->widthDEF;
    image.height = def->heightDEF;
    H3Pcx16Loader canvas(H3LoadedPcx16::Create(image.width, image.height));
    if (!canvas || !canvas->buffer) return OutOfMemory;
    canvas->FillRectangle(0, 0, image.width, image.height, 0, 255, 255);
    def->DrawToPcx16(request.sourceGroup, request.sourceFrame, canvas.Get(), 0, 0);
    ReadCanvas(canvas.Get(), image);
    return Ok;
}
int __stdcall DrawFrame(HiHook *hook, H3DefFrame *frame, int srcX, int srcY, int width, int height,
    void *buffer, int dstX, int dstY, int dstWidth, int dstHeight, int stride,
    H3Palette565 *palette, int mirror, int mode)
{
    const auto &palettes = Get().palettes;
    const auto found = palettes.find(frame);
    if (found != palettes.end()) palette = reinterpret_cast<H3Palette565 *>(&found->second->palette565);
    return THISCALL_14(int, hook->GetDefaultFunc(), frame, srcX, srcY, width, height,
        buffer, dstX, dstY, dstWidth, dstHeight, stride, palette, mirror, mode);
}
int __stdcall IsReady() { return Get().ready ? 1 : 0; }

int Create(const FrameRequest &request, FrameHandle &handle)
{
    auto &state = Get();
    auto *target = static_cast<H3LoadedDef *>(request.targetDef);
    const int width = target->widthDEF, height = target->heightDEF;
    if (!target->palette888 || width <= 0 || height <= 0 || width > 512 || height > 512) return InvalidArgument;
    const std::string name = Lower(request.resource);
    std::string key = name + ":" + std::to_string(request.sourceGroup) + ":" + std::to_string(request.sourceFrame) +
        ":" + std::to_string(width) + ":" + std::to_string(height) + ":" + std::to_string(request.paletteMode);
    key.append(reinterpret_cast<const char *>(target->palette888->color), sizeof(target->palette888->color));
    const auto cached = state.cache.find(key);
    if (cached != state.cache.end()) { handle = cached->second; return Ok; }
    const std::string targetName = Lower(target->GetName());
    if (!state.building.insert(targetName).second) return RecursiveSource;
    struct BuildingGuard
    {
        State &state; const std::string &name;
        ~BuildingGuard() { state.building.erase(name); }
    } building{state, targetName};
    std::vector<BYTE> data(width * height, 0);
    Image image;
    if (name.empty())
    {
        const char *glyph[] = {"01110", "10001", "00001", "00010", "00100", "00000", "00100"};
        const int scale = (std::max)(1, (std::min)(width / 7, height / 9));
        const BYTE color = PaletteIndex(Pixel{220, 220, 220, 255}, *target->palette888);
        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x)
            {
                const int gx = x - (width - 5 * scale) / 2, gy = y - (height - 7 * scale) / 2;
                if (gx >= 0 && gy >= 0 && gx < 5 * scale && gy < 7 * scale && glyph[gy / scale][gx / scale] == '1')
                    data[y * width + x] = color;
            }
    }
    else
    {
        const int result = ReadImage(request, name, image);
        if (result != Ok) return result;
        const double scale = (std::min)(1.0, (std::min)(double(width) / image.width, double(height) / image.height));
        const int w = (std::max)(1, int(image.width * scale)), h = (std::max)(1, int(image.height * scale));
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
            {
                const int at = (y * image.height / h) * image.width + x * image.width / w;
                data[(y + (height - h) / 2) * width + x + (width - w) / 2] = image.source ?
                    image.indices[at] : PaletteIndex(image.pixels[at], *target->palette888);
            }
    }
    const auto encoded = EncodeFrame(data, width, height);
    auto *frame = H3ObjectAllocator<H3DefFrame>().allocate(1);
    if (!frame) return OutOfMemory;
    frame = THISCALL_7(H3DefFrame *, 0x47BBD0, frame, "", width, height, encoded.data(), int(encoded.size()), 1);
    struct FrameGuard
    {
        H3DefFrame *frame;
        ~FrameGuard() { if (frame) THISCALL_2(void, 0x47BBA0, frame, 1); }
    } guard{frame};
    if (!frame->rawData) return OutOfMemory;
    if (state.frames.size() >= UINT32_MAX) return OutOfMemory;
    const FrameHandle id = static_cast<FrameHandle>(state.frames.size() + 1);
    state.frames.push_back({frame, image.source.Get(), std::string(
        reinterpret_cast<const char *>(target->palette888->color), sizeof(target->palette888->color))});
    try
    {
        state.cache.emplace(key, id);
        if (image.source) state.palettes.emplace(frame, image.source.Get());
    }
    catch (...)
    {
        state.cache.erase(key);
        state.frames.pop_back();
        throw;
    }
    image.source.Release();
    guard.frame = nullptr;
    handle = id;
    return Ok;
}
int __stdcall CreateFrame(const FrameRequest *request, FrameHandle *output)
{
    if (!output) return InvalidArgument;
    *output = 0;
    if (!request || request->structSize < sizeof(FrameRequest) || !request->targetDef ||
        request->sourceGroup < 0 || request->sourceFrame < 0 || request->paletteMode > PreserveNativePcx) return InvalidArgument;
    try
    {
        if (!Get().ready) return NotReady;
        return Create(*request, *output);
    }
    catch (const std::bad_alloc &) { return OutOfMemory; }
}
int __stdcall ApplyFrames(const UpdateRequest *request)
{
    if (!request || request->structSize < sizeof(UpdateRequest) || !request->targetDef || request->targetGroup < 0 ||
        !request->bindings || !request->count || request->count > 65536 ||
        request->bindingStride < sizeof(FrameBinding) || request->bindingStride > 4096) return InvalidArgument;
    try
    {
        auto &state = Get();
        if (!state.ready) return NotReady;
        auto *target = static_cast<H3LoadedDef *>(request->targetDef);
        if (!target->groups || request->targetGroup >= target->groupsCount || !target->groups[request->targetGroup]) return InvalidArgument;
        auto &group = *target->groups[request->targetGroup];
        if (group.count < 0 || (group.count && !group.frames) || request->count < unsigned(group.count)) return InvalidArgument;
        std::vector<H3DefFrame *> frames;
        frames.reserve(request->count);
        for (unsigned i = 0; i < request->count; ++i)
        {
            FrameBinding binding;
            std::memcpy(&binding, reinterpret_cast<const char *>(request->bindings) + std::size_t(i) * request->bindingStride, sizeof binding);
            H3DefFrame *frame = nullptr;
            if (binding.frame)
            {
                if (binding.frame > state.frames.size()) return InvalidFrame;
                const auto &record = state.frames[binding.frame - 1];
                frame = record.frame;
                if (frame->width != target->widthDEF || frame->height != target->heightDEF) return InvalidFrame;
                if (!record.source && (!target->palette888 || std::memcmp(record.targetPalette.data(),
                    target->palette888->color, record.targetPalette.size()))) return InvalidFrame;
            }
            else
            {
                if (binding.sourceGroup < 0 || binding.sourceGroup >= target->groupsCount || !target->groups[binding.sourceGroup]) return InvalidFrame;
                const auto &source = *target->groups[binding.sourceGroup];
                if (!source.frames || binding.sourceFrame < 0 || binding.sourceFrame >= source.count) return InvalidFrame;
                frame = source.frames[binding.sourceFrame];
                if (!frame) return InvalidFrame;
            }
            frames.push_back(frame);
        }
        const bool pin = state.pinned.insert(target).second;
        if (!UpdateFrameTable(group, frames, [](void *p, std::size_t n) { return H3Realloc(p, n); }))
        {
            if (pin) state.pinned.erase(target);
            return OutOfMemory;
        }
        if (pin) target->IncreaseReferences();
        return Ok;
    }
    catch (const std::bad_alloc &) { return OutOfMemory; }
}
void __stdcall AfterCreateWindow(Era::TEvent *)
{
    auto &state = Get();
    if (state.ready) return;
    auto *normal = _PI->WriteHiHook(0x47BE90, THISCALL_, DrawFrame);
    auto *transparent = _PI->WriteHiHook(0x47E880, THISCALL_, DrawFrame);
    Patch *hooks[] = {normal, transparent};
    if (!erm_functions::CheckHooks(hooks, "Graphics resources"))
    {
        Log("Cannot install palette render hooks; graphics service unavailable.");
        return;
    }
    state.ready = true;
    Log("Graphics API v1 ready; palette hooks have one runtime owner.");
    Era::FireEvent(READY_EVENT, nullptr, 0);
}
const Api api{sizeof(Api), API_VERSION, Pcx8 | EraPngOverrides | DefSources | NativePcxPalette, IsReady, CreateFrame, ApplyFrames};
} // namespace

void RegisterEvents()
{
    if (Get().registered) return;
    if (!era_services::Publish(era_graphics::SERVICE, &api))
    {
        Log("Another provider owns the graphics service; this provider remains inactive.");
        return;
    }
    Era::RegisterHandler(AfterCreateWindow, "OnAfterCreateWindow");
    Get().registered = true;
}
} // namespace graphics_resources

extern "C" const era_graphics::Api *__stdcall Erm_GetGraphicsApi(std::uint32_t version) noexcept
{
    return version == era_graphics::API_VERSION ? &graphics_resources::api : nullptr;
}
