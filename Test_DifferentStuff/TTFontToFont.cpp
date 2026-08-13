#include "TTFontToFont.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <mutex>
#include <string>
#include <utility>
#include <vector>
#include <windows.h>

#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "Gdi32.lib")

namespace
{
int GameByteToUnicode(int value, UINT codePage)
{
    // H3 text is an 8-bit string. ASCII is identical to Unicode, so do not
    // pass it through the active codepage. Only bytes >= 0x80 are converted.
    if (value < 0 || value > 255 || value < 0x80)
        return value;

    const char byte = static_cast<char>(value);
    wchar_t unicode = 0;

    if (MultiByteToWideChar(codePage, MB_PRECOMPOSED, &byte, 1, &unicode, 1) == 1)
    {
        return static_cast<int>(unicode);
    }

    return 0;
}

struct CustomFontData
{
    h3::H3Font *font;
    std::vector<unsigned char> grayscale;
};

std::vector<CustomFontData> customFonts;
std::mutex customFontsMutex;

#pragma pack(push, 1)
// Bytes copied by Fnt_Create from buffer to H3Font + 0x1C.
// The palette at H3Font + 0x103C is initialized by the game constructor.
struct H3FontBinaryHeader
{
    unsigned char first;
    unsigned char last;
    unsigned char depth;
    signed char xSpacing;
    signed char ySpacing;
    unsigned char height;
    unsigned char reserved[26];

    struct FontSpacing
    {
        int leftMargin;
        int span;
        int rightMargin;
    } width[256];

    unsigned int bufferOffsets[256];
};
#pragma pack(pop)

static_assert(sizeof(H3FontBinaryHeader) == 0x1020, "H3Font constructor header must be 0x1020 bytes");

bool ReadFile(const char *path, std::vector<unsigned char> &data)
{
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input)
        return false;

    const std::streamoff size = input.tellg();
    if (size <= 0 || size > 64 * 1024 * 1024)
        return false;

    data.resize(static_cast<size_t>(size));
    input.seekg(0);
    return static_cast<bool>(input.read(reinterpret_cast<char *>(data.data()), size));
}

std::string Lower(std::string value)
{
    for (char &c : value)
        c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    return value;
}

std::string TrimStyleSuffix(std::string value)
{
    const size_t suffix = value.find(" (true type)");
    if (suffix != std::string::npos)
        value.resize(suffix);

    const size_t openType = value.find(" (opentype)");
    if (openType != std::string::npos)
        value.resize(openType);

    while (!value.empty() && value.back() == ' ')
        value.pop_back();

    return value;
}

bool ResolveInstalledFont(const char *family, const TTFontOptions &options, std::string &result,
                          bool *realBold = nullptr, bool *realItalic = nullptr)
{
    if (!family || !*family)
        return false;

    if (realBold)
        *realBold = false;
    if (realItalic)
        *realItalic = false;

    const std::string wanted = Lower(TrimStyleSuffix(family));

    struct Candidate
    {
        std::string path;
        int score = -1;
        bool bold = false;
        bool italic = false;
    } best;

    const HKEY roots[] = {HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE};
    const char *subkey = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts";

    for (HKEY root : roots)
    {
        HKEY key = nullptr;
        if (RegOpenKeyExA(root, subkey, 0, KEY_READ, &key) != ERROR_SUCCESS)
            continue;

        for (DWORD index = 0;; ++index)
        {
            char valueName[256] = {};
            char valueData[MAX_PATH] = {};

            DWORD nameSize = sizeof(valueName);
            DWORD dataSize = sizeof(valueData);
            DWORD type = 0;

            const LONG error = RegEnumValueA(key, index, valueName, &nameSize, nullptr, &type,
                                             reinterpret_cast<LPBYTE>(valueData), &dataSize);

            if (error == ERROR_NO_MORE_ITEMS)
                break;

            if (error != ERROR_SUCCESS || type != REG_SZ)
                continue;

            const std::string label = Lower(TrimStyleSuffix(valueName));

            if (label != wanted && label.find(wanted + " ") != 0)
                continue;

            const bool candidateBold = label.find(" bold") != std::string::npos;

            const bool candidateItalic =
                label.find(" italic") != std::string::npos || label.find(" oblique") != std::string::npos;

            int score = 0;

            if (candidateBold == options.bold)
                score += 4;

            if (candidateItalic == options.italic)
                score += 4;

            if (label == wanted)
                score += 2;

            if (score <= best.score)
                continue;

            std::string path(valueData, dataSize ? dataSize - 1 : 0);

            if (path.find(':') == std::string::npos && path.find('\\') == std::string::npos)
            {
                char windowsDirectory[MAX_PATH] = {};

                GetWindowsDirectoryA(windowsDirectory, sizeof(windowsDirectory));

                path = std::string(windowsDirectory) + "\\Fonts\\" + path;
            }

            best = {path, score, candidateBold, candidateItalic};
        }

        RegCloseKey(key);
    }

    if (best.score < 0)
        return false;

    result = best.path;

    if (realBold)
        *realBold = options.bold && best.bold;

    if (realItalic)
        *realItalic = options.italic && best.italic;

    return true;
}

typedef h3::H3Font *(__thiscall *FntCreate)(h3::H3Font *, const char *, const void *, int, const void *);

#pragma pack(push, 1)
struct FontBinTree
{
    FontBinTree *left;
    FontBinTree *parent;
    FontBinTree *right;
    char name[12];
    unsigned int nameEnd;
    h3::H3Font *item;
    unsigned int field20;
};
#pragma pack(pop)

typedef void(__thiscall *AddItem2BinTree)(FontBinTree *, void *, const void *);

typedef FontBinTree *(__thiscall *SearchFontBinTree)(FontBinTree *, const char *);

bool RegisterInFontTree(h3::H3Font *font, const char *name)
{
    if (!font || !name || !*name || std::strlen(name) >= 12)
        return false;

    FontBinTree *tree = reinterpret_cast<FontBinTree *>(0x69E560);

    FontBinTree *found = reinterpret_cast<SearchFontBinTree>(0x55EE00)(tree, name);

    if (found && found != tree->parent && _stricmp(found->name, name) == 0)
    {
        return found->item == font;
    }

    char namedItem[20] = {};
    std::memcpy(namedItem, name, std::strlen(name));

    *reinterpret_cast<h3::H3Font **>(namedItem + 16) = font;

    void *result[2] = {};

    reinterpret_cast<AddItem2BinTree>(0x55DDF0)(tree, result, namedItem);

    // DoLoadFont increments this reference after inserting the item.
    font->IncreaseReferences();

    return true;
}

struct GdiFontContext
{
    HFONT font = nullptr;
    HDC dc = nullptr;
    HGDIOBJ oldFont = nullptr;
    HANDLE privateFont = nullptr;
};

bool ReadBigEndian16(const std::vector<unsigned char> &data, size_t offset, unsigned short &value)
{
    if (offset + 2 > data.size())
        return false;

    value = static_cast<unsigned short>((static_cast<unsigned short>(data[offset]) << 8) |
                                        static_cast<unsigned short>(data[offset + 1]));

    return true;
}

bool ReadBigEndian32(const std::vector<unsigned char> &data, size_t offset, unsigned int &value)
{
    if (offset + 4 > data.size())
        return false;

    value = (static_cast<unsigned int>(data[offset]) << 24) | (static_cast<unsigned int>(data[offset + 1]) << 16) |
            (static_cast<unsigned int>(data[offset + 2]) << 8) | static_cast<unsigned int>(data[offset + 3]);

    return true;
}

// Read the family name from a normal TrueType/OpenType font.
// We only need the family name here so GDI can select the font that was
// temporarily installed with AddFontMemResourceEx.
bool ReadTtfFamilyName(const std::vector<unsigned char> &data, std::wstring &familyName)
{
    familyName.clear();

    if (data.size() < 12)
        return false;

    unsigned short numTables = 0;
    if (!ReadBigEndian16(data, 4, numTables))
        return false;

    size_t nameTableOffset = 0;
    size_t nameTableLength = 0;

    for (unsigned short i = 0; i < numTables; ++i)
    {
        const size_t record = 12 + static_cast<size_t>(i) * 16;
        if (record + 16 > data.size())
            return false;

        char tag[5] = {};
        std::memcpy(tag, data.data() + record, 4);

        unsigned int offset = 0;
        unsigned int length = 0;

        if (!ReadBigEndian32(data, record + 8, offset) || !ReadBigEndian32(data, record + 12, length))
        {
            return false;
        }

        if (std::memcmp(tag, "name", 4) == 0)
        {
            nameTableOffset = offset;
            nameTableLength = length;
            break;
        }
    }

    if (!nameTableOffset || nameTableOffset + nameTableLength > data.size() || nameTableLength < 6)
    {
        return false;
    }

    const size_t table = nameTableOffset;

    unsigned short count = 0;
    unsigned short stringOffset = 0;

    if (!ReadBigEndian16(data, table + 2, count) || !ReadBigEndian16(data, table + 4, stringOffset))
    {
        return false;
    }

    const size_t records = table + 6;

    // Prefer Unicode family names (platform 3), then Unicode platform 0.
    // Name ID 1 is the family name.  Name ID 16 is the typographic family
    // and is useful for fonts whose family/style split is stored there.
    int bestScore = -1;
    std::wstring bestName;

    for (unsigned short i = 0; i < count; ++i)
    {
        const size_t record = records + static_cast<size_t>(i) * 12;

        if (record + 12 > data.size() || record + 12 > table + nameTableLength)
        {
            return false;
        }

        unsigned short platform = 0;
        unsigned short encoding = 0;
        unsigned short language = 0;
        unsigned short nameId = 0;
        unsigned short length = 0;
        unsigned short offset = 0;

        if (!ReadBigEndian16(data, record + 0, platform) || !ReadBigEndian16(data, record + 2, encoding) ||
            !ReadBigEndian16(data, record + 4, language) || !ReadBigEndian16(data, record + 6, nameId) ||
            !ReadBigEndian16(data, record + 8, length) || !ReadBigEndian16(data, record + 10, offset))
        {
            return false;
        }

        if (nameId != 1 && nameId != 16)
            continue;

        const size_t stringStart = table + stringOffset + offset;

        if (stringStart + length > data.size() || stringStart + length > table + nameTableLength)
        {
            continue;
        }

        std::wstring candidate;

        if (platform == 0 || platform == 3)
        {
            if ((length & 1) != 0)
                continue;

            candidate.reserve(length / 2);

            for (unsigned short p = 0; p < length; p += 2)
            {
                const unsigned short ch =
                    static_cast<unsigned short>((static_cast<unsigned short>(data[stringStart + p]) << 8) |
                                                static_cast<unsigned short>(data[stringStart + p + 1]));

                candidate.push_back(static_cast<wchar_t>(ch));
            }
        }
        else if (platform == 1)
        {
            // Macintosh Roman is not required for normal Windows fonts.
            continue;
        }
        else
        {
            continue;
        }

        while (!candidate.empty() && (candidate.back() == L' ' || candidate.back() == L'\0'))
        {
            candidate.pop_back();
        }

        if (candidate.empty())
            continue;

        int score = 0;

        if (nameId == 1)
            score += 10;
        else
            score += 5;

        if (platform == 3)
            score += 4;
        else if (platform == 0)
            score += 2;

        if (language == 0x0409 || language == 0)
            score += 2;

        if (score > bestScore)
        {
            bestScore = score;
            bestName = candidate;
        }
    }

    if (bestScore < 0)
        return false;

    familyName = bestName;
    return true;
}

void DestroyGdiFontContext(GdiFontContext &context)
{
    if (context.dc)
    {
        if (context.oldFont)
            SelectObject(context.dc, context.oldFont);

        DeleteDC(context.dc);
        context.dc = nullptr;
    }

    if (context.font)
    {
        DeleteObject(context.font);
        context.font = nullptr;
    }

    if (context.privateFont)
    {
        RemoveFontMemResourceEx(context.privateFont);
        context.privateFont = nullptr;
    }
}

bool CreateGdiFontContext(const std::vector<unsigned char> &file, const TTFontOptions &options, GdiFontContext &context)
{
    DWORD fontsAdded = 0;

    context.privateFont =
        AddFontMemResourceEx(const_cast<BYTE *>(file.data()), static_cast<DWORD>(file.size()), nullptr, &fontsAdded);

    if (!context.privateFont || fontsAdded == 0)
    {
        DestroyGdiFontContext(context);
        return false;
    }

    context.dc = CreateCompatibleDC(nullptr);
    if (!context.dc)
    {
        DestroyGdiFontContext(context);
        return false;
    }

    std::wstring familyName;
    if (!ReadTtfFamilyName(file, familyName))
    {
        DestroyGdiFontContext(context);
        return false;
    }

    LOGFONTW lf = {};
    lf.lfHeight = -options.pixelHeight;

    wcsncpy_s(lf.lfFaceName, LF_FACESIZE, familyName.c_str(), _TRUNCATE);
    lf.lfWidth = 0;
    lf.lfEscapement = 0;
    lf.lfOrientation = 0;

    // GDI uses a 0..1000 weight range. 700 is a normal "Bold".
    lf.lfWeight = options.bold ? FW_BOLD : FW_NORMAL;
    lf.lfItalic = options.italic ? TRUE : FALSE;
    lf.lfUnderline = options.underline ? TRUE : FALSE;
    lf.lfStrikeOut = options.strikeOut ? TRUE : FALSE;

    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfOutPrecision = OUT_TT_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;

    // Let GDI choose the font that was just installed privately.
    lf.lfQuality = ANTIALIASED_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;

    context.font = CreateFontIndirectW(&lf);
    if (!context.font)
    {
        DestroyGdiFontContext(context);
        return false;
    }

    context.oldFont = SelectObject(context.dc, context.font);

    if (!context.oldFont)
    {
        DestroyGdiFontContext(context);
        return false;
    }

    SetBkMode(context.dc, TRANSPARENT);
    SetTextColor(context.dc, RGB(255, 255, 255));

    return true;
}

// GGO_GRAY8_BITMAP returns one byte per pixel in the range 0..64.
// Convert that coverage to the 0..255 range used by our renderer.
unsigned char GdiGray8ToCoverage(unsigned char value)
{
    if (value == 0)
        return 0;

    if (value >= 64)
        return 255;

    return static_cast<unsigned char>((static_cast<unsigned int>(value) * 255u + 32u) / 64u);
}

// Blend RGB565 foreground over RGB565 background using 8-bit coverage.
WORD Blend565(WORD background, WORD foreground, unsigned char coverage)
{
    if (coverage == 0)
        return background;

    if (coverage == 255)
        return foreground;

    const unsigned int br = (background >> 11) & 0x1F;
    const unsigned int bg = (background >> 5) & 0x3F;
    const unsigned int bb = background & 0x1F;

    const unsigned int fr = (foreground >> 11) & 0x1F;
    const unsigned int fg = (foreground >> 5) & 0x3F;
    const unsigned int fb = foreground & 0x1F;

    const unsigned int inv = 255u - coverage;

    const unsigned int r = (br * inv + fr * coverage + 127u) / 255u;

    const unsigned int g = (bg * inv + fg * coverage + 127u) / 255u;

    const unsigned int b = (bb * inv + fb * coverage + 127u) / 255u;

    return static_cast<WORD>((r << 11) | (g << 5) | b);
}

bool GetGdiGlyph(HDC dc, wchar_t character, GLYPHMETRICS &metrics, std::vector<unsigned char> &coverage, int &pitch)
{
    std::memset(&metrics, 0, sizeof(metrics));

    MAT2 matrix = {};
    matrix.eM11.value = 1;
    matrix.eM22.value = 1;

    const DWORD required = GetGlyphOutlineW(dc, character, GGO_GRAY8_BITMAP, &metrics, 0, nullptr, &matrix);

    if (required == GDI_ERROR)
        return false;

    const int width = static_cast<int>(metrics.gmBlackBoxX);

    const int height = static_cast<int>(metrics.gmBlackBoxY);

    if (width <= 0 || height <= 0)
    {
        coverage.clear();
        pitch = 0;
        return true;
    }

    // GDI aligns every grayscale bitmap scanline to DWORD.
    pitch = (width + 3) & ~3;

    std::vector<unsigned char> raw(static_cast<size_t>(pitch) * height);

    const DWORD actual = GetGlyphOutlineW(dc, character, GGO_GRAY8_BITMAP, &metrics, static_cast<DWORD>(raw.size()),
                                          raw.data(), &matrix);

    if (actual == GDI_ERROR)
        return false;

    coverage.resize(static_cast<size_t>(width) * height);

    for (int y = 0; y < height; ++y)
    {
        const unsigned char *source = raw.data() + static_cast<size_t>(y) * pitch;

        unsigned char *destination = coverage.data() + static_cast<size_t>(y) * width;

        for (int x = 0; x < width; ++x)
            destination[x] = GdiGray8ToCoverage(source[x]);
    }

    return true;
}

void ApplyDecoration(std::vector<unsigned char> &coverage, int width, int height, bool underline, bool strikeOut)
{
    if (width <= 0 || height <= 0)
        return;

    if (underline)
    {
        const int thickness = std::max(1, height >= 8 ? 1 : 1);

        for (int y = height - thickness; y < height; ++y)
        {
            unsigned char *row = coverage.data() + static_cast<size_t>(y) * width;

            std::memset(row, 255, static_cast<size_t>(width));
        }
    }

    if (strikeOut)
    {
        const int y = height / 2;

        unsigned char *row = coverage.data() + static_cast<size_t>(y) * width;

        std::memset(row, 255, static_cast<size_t>(width));
    }
}

} // namespace

std::string JustifyH3TextLine(const h3::H3Font *font, const char *text, int targetWidth)
{
    if (!font || !text || targetWidth <= 0)
        return text ? std::string(text) : std::string();

    const auto glyphWidth = [font](unsigned char character) {
        const h3::H3Font::FontSpacing &spacing = font->width[character];

        return spacing.leftMargin + spacing.span + spacing.rightMargin;
    };

    int currentWidth = 0;
    int spaceCount = 0;

    for (const unsigned char *p = reinterpret_cast<const unsigned char *>(text); *p; ++p)
    {
        currentWidth += glyphWidth(*p);

        if (*p == ' ')
            ++spaceCount;
    }

    if (spaceCount == 0 || currentWidth >= targetWidth)
        return std::string(text);

    const int oneSpaceWidth = glyphWidth(' ');

    if (oneSpaceWidth <= 0)
        return std::string(text);

    const int extraSpaces = (targetWidth - currentWidth) / oneSpaceWidth;

    if (extraSpaces <= 0)
        return std::string(text);

    const int spacesPerGap = extraSpaces / spaceCount;

    const int remainder = extraSpaces % spaceCount;

    std::string result;
    result.reserve(std::strlen(text) + static_cast<size_t>(extraSpaces));

    int gap = 0;

    for (const unsigned char *p = reinterpret_cast<const unsigned char *>(text); *p; ++p)
    {
        result.push_back(static_cast<char>(*p));

        if (*p == ' ')
        {
            const int add = spacesPerGap + (gap < remainder ? 1 : 0);

            result.append(static_cast<size_t>(add), ' ');

            ++gap;
        }
    }

    return result;
}

bool DrawCustomH3Glyph(h3::H3Font *font, unsigned int character, h3::H3LoadedPcx16 *drawBuffer, int x, int y, int color)
{
    if (!font || !drawBuffer || character > 0xFF)
        return false;

    std::lock_guard<std::mutex> lock(customFontsMutex);

    const CustomFontData *data = nullptr;

    for (const CustomFontData &candidate : customFonts)
    {
        if (candidate.font == font)
        {
            data = &candidate;
            break;
        }
    }

    if (!data)
        return false;

    const int span = font->width[character].span;

    const int height = static_cast<unsigned char>(font->height);

    if (span <= 0 || height <= 0)
        return true;

    const size_t glyphSize = static_cast<size_t>(span) * height;

    const unsigned int glyphOffset = font->bufferOffsets[character];

    if (glyphOffset > data->grayscale.size() || glyphSize > data->grayscale.size() - glyphOffset)
    {
        return false;
    }

    const unsigned char *source = data->grayscale.data() + glyphOffset;

    const WORD requestedColor = font->palette.color[color];

    const int startX = x + font->width[character].leftMargin;

    const int bufferWidth = drawBuffer->scanlineSize / 2;

    for (int row = 0; row < height; ++row)
    {
        const int dstY = y + row;

        if (dstY < 0)
        {
            source += span;
            continue;
        }

        const int firstColumn = std::max(0, -startX);

        const int lastColumn = std::min(span, bufferWidth - startX);

        if (firstColumn < lastColumn)
        {
            WORD *destination = reinterpret_cast<WORD *>(
                drawBuffer->buffer + static_cast<size_t>(dstY) * drawBuffer->scanlineSize + 2 * (startX + firstColumn));

            const unsigned char *coverage = source + firstColumn;

            for (int column = firstColumn; column < lastColumn; ++column, ++coverage, ++destination)
            {
                if (*coverage)
                {
                    *destination = Blend565(*destination, requestedColor, *coverage);
                }
            }
        }

        source += span;
    }

    return true;
}

h3::H3Font *CreateH3FontFromTTF(const char *fontPath, const char *fontName, const TTFontOptions &options,
                                bool addToResourceManager)
{
    if (!fontPath || !fontName || !*fontName || std::strlen(fontName) >= 12 || options.pixelHeight < 1 ||
        options.pixelHeight > 255 || options.firstCharacter < 0 || options.firstCharacter > 255 ||
        options.lastCharacter < options.firstCharacter || options.lastCharacter > 255)
    {
        return nullptr;
    }

    std::string resolvedPath;
    std::vector<unsigned char> file;

    if (!ReadFile(fontPath, file))
    {
        if (!ResolveInstalledFont(fontPath, options, resolvedPath) || !ReadFile(resolvedPath.c_str(), file))
        {
            return nullptr;
        }
    }

    GdiFontContext gdi;

    if (!CreateGdiFontContext(file, options, gdi))
    {
        return nullptr;
    }

    TEXTMETRICW textMetrics = {};

    if (!GetTextMetricsW(gdi.dc, &textMetrics))
    {
        DestroyGdiFontContext(gdi);
        return nullptr;
    }

    const int fontHeight = options.pixelHeight;

    const int baseline = textMetrics.tmAscent;

    H3FontBinaryHeader header = {};

    // SPACE is required by virtually every text widget. If the caller starts
    // the range above 0x20, include SPACE rather than letting it fall through
    // to the '?' fallback glyph.
    const int firstCharacter = std::min(options.firstCharacter, 0x20);

    header.first = static_cast<unsigned char>(firstCharacter);

    header.last = static_cast<unsigned char>(options.lastCharacter);

    // Native H3 bitmap remains binary.  The original renderer expects
    // 0/0xFF, while DrawCustomH3Glyph uses the separate grayscale buffer.
    header.depth = 1;

    header.xSpacing = static_cast<signed char>(std::max(-128, std::min(127, options.xSpacing)));

    header.ySpacing = static_cast<signed char>(std::max(-128, std::min(127, options.ySpacing)));

    header.height = static_cast<unsigned char>(fontHeight);

    std::vector<unsigned char> bitmap;
    std::vector<unsigned char> grayscale;

    const DWORD codePage = Era::GetCodePage();

    for (int ch = 0; ch < 256; ++ch)
    {
        const bool inRange = ch >= firstCharacter && ch <= options.lastCharacter;

        // SPACE and NBSP are special: they normally have no black box at all.
        // Their useful information is their advance width, not a bitmap.
        const bool isSpace = ch == 0x20;
        const bool isNbsp = ch == 0xA0;

        int codepoint = 0;

        if (inRange)
            codepoint = GameByteToUnicode(ch, codePage);

        if (isSpace)
        {
            // ASCII SPACE must never be converted to '?'.
            codepoint = L' ';
        }
        else if (isNbsp)
        {
            // NBSP is Unicode U+00A0. Keep it explicit because some game
            // codepages do not map byte 0xA0 to NBSP.
            codepoint = 0x00A0;
        }
        else if (!codepoint)
        {
            codepoint = L'?';
        }

        wchar_t wideCharacter = static_cast<wchar_t>(codepoint);

        GLYPHMETRICS metrics = {};
        std::vector<unsigned char> glyphCoverage;
        int glyphPitch = 0;

        bool glyphOk = GetGdiGlyph(gdi.dc, wideCharacter, metrics, glyphCoverage, glyphPitch);

        if (!glyphOk && !isSpace && !isNbsp && wideCharacter != L'?')
        {
            // Missing glyph: use the font's actual question-mark glyph.
            wideCharacter = L'?';

            glyphOk = GetGdiGlyph(gdi.dc, wideCharacter, metrics, glyphCoverage, glyphPitch);
        }

        if (!glyphOk)
        {
            // Last-resort metrics. This path should normally only be reached
            // for a malformed/missing font.
            metrics.gmCellIncX = std::max<LONG>(1, textMetrics.tmAveCharWidth);

            metrics.gmBlackBoxX = 0;
            metrics.gmBlackBoxY = 0;
            metrics.gmptGlyphOrigin.x = 0;
            metrics.gmptGlyphOrigin.y = 0;
            glyphCoverage.clear();
        }

        // GetGlyphOutlineW returns an empty black box for SPACE/NBSP. Keep a
        // one-pixel empty storage cell so H3's bitmap offset remains valid,
        // while all actual advance comes from gmCellIncX.
        const int glyphWidth = (isSpace || isNbsp) ? 1 : std::max(1, static_cast<int>(metrics.gmBlackBoxX));

        const int glyphHeight = static_cast<int>(metrics.gmBlackBoxY);

        const int glyphOriginX = (isSpace || isNbsp) ? 0 : static_cast<int>(metrics.gmptGlyphOrigin.x);

        const int glyphOriginY = (isSpace || isNbsp) ? 0 : static_cast<int>(metrics.gmptGlyphOrigin.y);

        int advance = static_cast<int>(metrics.gmCellIncX);

        if (advance <= 0)
        {
            // Space width from GDI metrics. This is preferable to guessing
            // from the average character width.
            ABC abc = {};
            if (GetCharABCWidthsW(gdi.dc, static_cast<UINT>(wideCharacter), static_cast<UINT>(wideCharacter), &abc))
            {
                advance = static_cast<int>(abc.abcA + abc.abcB + abc.abcC);
            }
        }

        if (advance <= 0)
            advance = std::max(1, static_cast<int>(textMetrics.tmAveCharWidth));

        // GDI's glyph origin is relative to the baseline.
        // H3's bitmap starts at the top of the character cell.
        const int destinationY = baseline - glyphOriginY;

        const size_t glyphOffset = bitmap.size();

        header.bufferOffsets[ch] = static_cast<unsigned int>(glyphOffset);

        bitmap.resize(glyphOffset + static_cast<size_t>(glyphWidth) * fontHeight, 0);

        grayscale.resize(glyphOffset + static_cast<size_t>(glyphWidth) * fontHeight, 0);

        header.width[ch].leftMargin = glyphOriginX;

        header.width[ch].span = glyphWidth;

        header.width[ch].rightMargin = advance - glyphWidth - glyphOriginX;

        if ((isSpace || isNbsp) || glyphHeight <= 0 || glyphCoverage.empty())
        {
            // Deliberately leave the bitmap/grayscale storage zero-filled.
            // The character advances normally, but no pixel is drawn.
            continue;
        }

        ApplyDecoration(glyphCoverage, static_cast<int>(metrics.gmBlackBoxX), glyphHeight, options.underline,
                        options.strikeOut);

        const int sourceWidth = static_cast<int>(metrics.gmBlackBoxX);

        for (int gy = 0; gy < glyphHeight; ++gy)
        {
            const int dstY = destinationY + gy;

            if (dstY < 0 || dstY >= fontHeight)
            {
                continue;
            }

            const unsigned char *source = glyphCoverage.data() + static_cast<size_t>(gy) * sourceWidth;

            unsigned char *grayDestination = grayscale.data() + glyphOffset + static_cast<size_t>(dstY) * glyphWidth;

            unsigned char *binaryDestination = bitmap.data() + glyphOffset + static_cast<size_t>(dstY) * glyphWidth;

            const int copyWidth = std::min(sourceWidth, glyphWidth);

            for (int gx = 0; gx < copyWidth; ++gx)
            {
                const unsigned char coverage = source[gx];

                grayDestination[gx] = coverage;

                // Keep the native H3 bitmap usable by code that still
                // calls the game's original Fnt_DrawSymbol.
                //
                // coverageThreshold belongs to TTFontOptions and lets
                // callers choose how the fallback binary representation
                // is produced.
                binaryDestination[gx] = coverage >= options.coverageThreshold ? 0xFF : 0;
            }
        }

        (void)glyphPitch;
    }

    h3::H3Font *object = h3::H3Alloc<h3::H3Font>();

    if (!object)
    {
        DestroyGdiFontContext(gdi);
        return nullptr;
    }

    const FntCreate create = reinterpret_cast<FntCreate>(0x4B4DD0);

    h3::H3Font *result = create(object, fontName, &header, static_cast<int>(bitmap.size()), bitmap.data());

    if (!result)
    {
        h3::H3Free(object);
        DestroyGdiFontContext(gdi);
        return nullptr;
    }

    THISCALL_2(void, 0x04B4EE0, result, THISCALL_1(DWORD, 0x055B5F0, 0x067F7CC));

    // Fnt_Create initializes the palette object, but a generated font has no
    // palette resource of its own. Reuse the game's already initialized font
    // palette so REGULAR/BLACK/RED/etc. resolve exactly like native fonts.

    if (addToResourceManager)
    {
        // H3ResourceManager is used by the general resource API, while
        // LoadFont/DlgText use the legacy font tree at 0x69E560.
        // Both must contain the same object for normal widget-based loading.
        RegisterInFontTree(result, fontName);
    }

    {
        std::lock_guard<std::mutex> lock(customFontsMutex);

        customFonts.push_back({result, std::move(grayscale)});
    }

    DestroyGdiFontContext(gdi);

    return result;
}

h3::H3Font *CreateH3FontFromWindowsName(const char *windowsFontName, const char *fontName, const TTFontOptions &options,
                                        bool addToResourceManager)
{
    std::string path;
    if (!ResolveInstalledFont(windowsFontName, options, path))
    {
        return nullptr;
    }

    TTFontOptions rasterOptions = options;

    // GDI now receives the requested style directly.  These flags are still
    // kept for the synthetic fallback in case the requested face is absent.
    return CreateH3FontFromTTF(path.c_str(), fontName, rasterOptions, addToResourceManager);
}

extern "C" h3::H3Font *__stdcall EraCreateH3FontFromTTF(const char *fontPath, const char *fontName, int pixelHeight,
                                                        bool addToResourceManager, bool bold, bool italic,
                                                        bool underline, bool strikeOut)
{
    TTFontOptions options;

    options.pixelHeight = pixelHeight;
    options.bold = bold;
    options.italic = italic;
    options.underline = underline;
    options.strikeOut = strikeOut;

    return CreateH3FontFromTTF(fontPath, fontName, options, addToResourceManager);
}

extern "C" h3::H3Font *__stdcall EraCreateH3FontFromWindowsName(const char *windowsFontName, const char *fontName,
                                                                int pixelHeight, bool addToResourceManager, bool bold,
                                                                bool italic, bool underline, bool strikeOut)
{
    TTFontOptions options;

    options.pixelHeight = pixelHeight;
    options.bold = bold;
    options.italic = italic;
    options.underline = underline;
    options.strikeOut = strikeOut;

    return CreateH3FontFromWindowsName(windowsFontName, fontName, options, addToResourceManager);
}
