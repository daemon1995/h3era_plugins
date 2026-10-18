#include "pch.h"
#include "ArtifactImages.h"
#include "ArtifactImageAlgorithms.h"
#include <wincodec.h>
#include <algorithm>
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "windowscodecs.lib")

namespace artifacts
{
namespace
{
template <class T> class ComPtr
{
    T *value = nullptr;
  public:
    ComPtr() = default;
    ComPtr(const ComPtr &) = delete;
    ComPtr &operator=(const ComPtr &) = delete;
    ~ComPtr() { if (value) value->Release(); }
    T *Get() const { return value; }
    T **GetAddressOf() { return &value; }
    T *operator->() const { return value; }
};
struct Pixel { unsigned char b, g, r, a; };
struct Image
{
    int width = 0, height = 0;
    std::vector<Pixel> pixels;
};

bool ResourceExists(const std::string &name, bool sprite)
{
    auto *cached = H3ResourceManager::Get()->FindItem(name.c_str());
    if (cached && cached->data.m_item) { Log("Image resource found in cache: %s", name.c_str()); return true; }
    const auto fileExists = [](const std::string &file)
    {
        const DWORD attributes = GetFileAttributesA(file.c_str());
        return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
    };
    if (fileExists(name) || fileExists("Data\\" + name)) { Log("Image resource found as file: %s", name.c_str()); return true; }
    if (!sprite && Era::PcxPngExists && Era::PcxPngExists(name.c_str())) { Log("Image resource found as PNG: %s", name.c_str()); return true; }

    // Use the operands actually installed by WoG: its LOD arrays are relocated.
    // These are the same lists and safe FindFileInLod call used by the loaders,
    // before their modal missing-resource errors and fallback resource loading.
    const int version = **reinterpret_cast<int **>(0x69928C);
    if (version < 0 || version > 3) return false; // WoG also uses SoD + AB (3).
    const int count = IntAt(DwordAt(sprite ? 0x55CA3B : 0x55AB9F) + version * 24);
    auto *indexes = *reinterpret_cast<int **>(DwordAt(sprite ? 0x55CA34 : 0x55AB98) + version * 24);
    const DWORD lods = DwordAt(sprite ? 0x55CA56 : 0x55ABBA) - 4;
    for (int i = 0; i < count; ++i)
        if (reinterpret_cast<H3Lod *>(lods + indexes[i] * sizeof(H3Lod))->FindFileInLod(name.c_str())) { Log("Image resource found in LOD %d: %s", indexes[i], name.c_str()); return true; }
    return false;
}

bool LoadBitmap(const std::string &name, Image &image)
{
    const HRESULT init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    struct Apartment
    {
        bool release;
        ~Apartment() { if (release) CoUninitialize(); }
    } apartment{SUCCEEDED(init)};
    ComPtr<IWICImagingFactory> factory;
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                               IID_PPV_ARGS(factory.GetAddressOf())))) return false;
    std::string file = name;
    if (GetFileAttributesA(file.c_str()) == INVALID_FILE_ATTRIBUTES) file = "Data\\" + name;
    const int length = MultiByteToWideChar(CP_ACP, 0, file.c_str(), -1, nullptr, 0);
    if (!length) return false;
    std::vector<wchar_t> wide(length);
    MultiByteToWideChar(CP_ACP, 0, file.c_str(), -1, wide.data(), length);
    ComPtr<IWICBitmapDecoder> decoder;
    if (FAILED(factory->CreateDecoderFromFilename(wide.data(), nullptr, GENERIC_READ,
                WICDecodeMetadataCacheOnLoad, decoder.GetAddressOf()))) return false;
    ComPtr<IWICBitmapFrameDecode> frame;
    ComPtr<IWICFormatConverter> converter;
    UINT width = 0, height = 0;
    if (FAILED(decoder->GetFrame(0, frame.GetAddressOf())) ||
        FAILED(frame->GetSize(&width, &height)) || !width || !height || width > 4096 || height > 4096 ||
        FAILED(factory->CreateFormatConverter(converter.GetAddressOf())) ||
        FAILED(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone,
                                    nullptr, 0, WICBitmapPaletteTypeCustom))) return false;
    image.width = width;
    image.height = height;
    image.pixels.resize(width * height);
    return SUCCEEDED(converter->CopyPixels(nullptr, width * sizeof(Pixel),
                     static_cast<UINT>(image.pixels.size() * sizeof(Pixel)),
                     reinterpret_cast<BYTE *>(image.pixels.data())));
}

bool LoadImage(const std::string &name, int index, Image &image)
{
    const auto dot = name.find_last_of('.');
    const std::string ext = dot == std::string::npos ? "" : name.substr(dot);
    if (!_stricmp(ext.c_str(), ".pcx"))
    {
        if (!ResourceExists(name, false)) return false;
        H3PcxLoader pcx(name.c_str());
        if (!pcx || !pcx->buffer || pcx->width <= 0 || pcx->height <= 0 ||
            pcx->width > 4096 || pcx->height > 4096) return false;
        image.width = pcx->width;
        image.height = pcx->height;
        image.pixels.resize(image.width * image.height);
        for (int y = 0; y < image.height; ++y)
            for (int x = 0; x < image.width; ++x)
            {
                const int colorIndex = *pcx->GetPixelIndex(x, y);
                const auto &c = pcx->palette888.color[colorIndex];
                image.pixels[y * image.width + x] = {c.b, c.g, c.r, static_cast<BYTE>(colorIndex ? 255 : 0)};
            }
        return true;
    }
    if (!_stricmp(ext.c_str(), ".def"))
    {
        // Never recurse into the shared artifact resource from its own loader.
        if (!_stricmp(name.c_str(), "artifact.def") || !_stricmp(name.c_str(), "artifacts.def")) return false;
        if (!ResourceExists(name, true)) return false;
        H3DefLoader source(name.c_str());
        if (!source || !source->groups || source->groupsCount <= 0 || !source->groups[0] ||
            index < 0 || index >= source->groups[0]->count || !source->groups[0]->frames[index] ||
            !source->palette565 || source->widthDEF <= 0 || source->heightDEF <= 0 ||
            source->widthDEF > 4096 || source->heightDEF > 4096) return false;
        image.width = source->widthDEF;
        image.height = source->heightDEF;
        H3Pcx16Loader canvas(H3LoadedPcx16::Create(image.width, image.height));
        if (!canvas || !canvas->buffer) return false;
        canvas->FillRectangle(0, 0, image.width, image.height, 0, 255, 255);
        source->DrawToPcx16(0, index, canvas.Get(), 0, 0);
        image.pixels.resize(image.width * image.height);
        for (int y = 0; y < image.height; ++y)
            for (int x = 0; x < image.width; ++x)
            {
                const auto c = canvas->GetPixel(x, y);
                image.pixels[y * image.width + x] = {c.b, c.g, c.r, 255};
            }
        return true;
    }
    // PNG/BMP/JPEG/TIFF use the Windows decoder, including PNG alpha.
    return LoadBitmap(name, image);
}

} // namespace

H3DefFrame *ArtifactImages::Frame(H3LoadedDef *target, const std::string &image, int sourceFrame)
{
    const int width = target->widthDEF, height = target->heightDEF;
    if (width <= 0 || height <= 0 || width > 512 || height > 512 || !target->palette888) return nullptr;
    std::string key = image + ":" + std::to_string(sourceFrame) + ":" + std::to_string(width) + ":" + std::to_string(height);
    key.append(reinterpret_cast<const char *>(target->palette888->color), sizeof(target->palette888->color));
    const auto found = frames.find(key);
    if (found != frames.end()) return found->second;
    std::vector<BYTE> data(width * height, 0);
    if (image.empty())
    {
        // A visible question mark also makes an icon-less artifact selectable.
        const char *glyph[] = {"01110", "10001", "00001", "00010", "00100", "00000", "00100"};
        const int scale = (std::max)(1, (std::min)(width / 7, height / 9));
        const BYTE color = ArtifactPaletteIndex(Pixel{220, 220, 220, 255}, *target->palette888);
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
        Image source;
        Log("Loading artifact image: %s", image.c_str());
        if (!LoadImage(image, sourceFrame, source))
        {
            Log("Cannot load artifact image: %s (frame %d)", image.c_str(), sourceFrame);
            frames.emplace(key, nullptr);
            return nullptr;
        }
        const double scale = (std::min)(1.0, (std::min)(double(width) / source.width, double(height) / source.height));
        const int w = (std::max)(1, int(source.width * scale));
        const int h = (std::max)(1, int(source.height * scale));
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
            {
                const auto &pixel = source.pixels[(y * source.height / h) * source.width + x * source.width / w];
                data[(y + (height - h) / 2) * width + x + (width - w) / 2] = ArtifactPaletteIndex(pixel, *target->palette888);
            }
    }
    auto *frame = H3ObjectAllocator<H3DefFrame>().allocate(1);
    if (!frame) return nullptr;
    frame = THISCALL_7(H3DefFrame *, 0x47BBD0, frame, "", width, height, data.data(), int(data.size()), 0);
    if (!frame->rawData)
    {
        THISCALL_2(void, 0x47BBA0, frame, 1);
        return nullptr;
    }
    frames.emplace(std::move(key), frame);
    return frame;
}

void ArtifactImages::Apply(H3LoadedDef *target, int count, const std::array<ArtifactData, CAPACITY> &definitions)
{
    if (!target->groups || target->groupsCount <= 0 || !target->groups[0]) return;
    auto *group = target->groups[0];
    if (group->count < 0 || (group->count && !group->frames)) return;
    auto *placeholder = Frame(target, "", 0);
    if (!placeholder) return;
    const int oldCount = group->count;
    if (oldCount < count)
    {
        if (!GrowArtifactFrames(*group, count, placeholder, [](void *buffer, size_t size) { return H3Realloc(buffer, size); }))
        {
            Log("Cannot allocate artifact DEF frame table");
            return;
        }
        Log("Artifact DEF: %d original frames, %d appended placeholders", oldCount, count - oldCount);
    }
    for (int id = 0; id < count; ++id)
    {
        const auto &definition = definitions[id];
        if (!definition.image.empty())
        {
            auto *frame = Frame(target, definition.image, definition.imageFrame);
            group->frames[id] = frame ? frame : placeholder;
        }
    }
}
} // namespace artifacts
