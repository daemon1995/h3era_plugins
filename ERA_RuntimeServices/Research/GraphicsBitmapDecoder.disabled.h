// External bitmap decoding remains disabled; retained for a future opt-in backend.
#if 0
#include <wincodec.h>
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "windowscodecs.lib")
namespace graphics_resources { namespace {
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
}}
#endif
