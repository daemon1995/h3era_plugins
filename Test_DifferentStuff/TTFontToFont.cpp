#define STB_TRUETYPE_IMPLEMENTATION
#include "..\headers\stb_truetype.h"

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

namespace
{
int GameByteToUnicode(int value, UINT codePage)
{
    if (value < 0 || value > 255 || value < 0x80)
        return value;
    char byte = static_cast<char>(value);
    wchar_t unicode = 0;
    if (MultiByteToWideChar(codePage, MB_PRECOMPOSED, &byte, 1, &unicode, 1) == 1)
        return static_cast<int>(unicode);
    return '?';
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
            DWORD nameSize = sizeof(valueName), dataSize = sizeof(valueData);
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
        return found->item == font;

    char namedItem[20] = {};
    std::memcpy(namedItem, name, std::strlen(name));
    *reinterpret_cast<h3::H3Font **>(namedItem + 16) = font;

    void *result[2] = {};
    reinterpret_cast<AddItem2BinTree>(0x55DDF0)(tree, result, namedItem);

    // DoLoadFont increments this reference after inserting the item.
    font->IncreaseReferences();
    return true;
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

    // H3Font can only draw complete space glyphs, so this is deliberately
    // pixel-based rather than a character-count approximation.
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

    const CustomFontData *data = nullptr;
    for (const CustomFontData &candidate : customFonts)
        if (candidate.font == font)
        {
            data = &candidate;
            break;
        }
    if (!data)
        return false;

    // This is intentionally a literal copy of Fnt_DrawSymbol from 0x4B4F00.
    // The original renderer has no alpha blending: 0 is transparent, 0xFF
    // uses the requested color, and every other non-zero value uses palette[32].
    const unsigned char *source = reinterpret_cast<unsigned char*>( font->bitmapBuffer + font->bufferOffsets[character]);
    const int span = font->width[character].span;
    const int height = static_cast<unsigned char>(font->height);
    const WORD requestedColor = font->palette.color[color];
    const WORD fixedColor = font->palette.color[32];
    const int startX = x + font->width[character].leftMargin;
    unsigned char *destinationRow = drawBuffer->buffer +
                                    2 * startX + y * drawBuffer->scanlineSize;

    for (int row = 0; row < height; ++row)
    {
        WORD *destination = reinterpret_cast<WORD *>(destinationRow);
        for (int column = 0; column < span; ++column)
        {
            const unsigned char pixel = *source++;
            if (pixel)
                destination[column] = pixel == 0xFF ? requestedColor : fixedColor;
        }
        destinationRow += drawBuffer->scanlineSize;
    }
    return true;
}

h3::H3Font *CreateH3FontFromTTF(const char *fontPath, const char *fontName, const TTFontOptions &options,
                                bool addToResourceManager)
{
    if (!fontPath || !fontName || !*fontName || std::strlen(fontName) >= 12 || options.pixelHeight < 1 ||
        options.pixelHeight > 255 || options.firstCharacter < 0 || options.firstCharacter > 255 ||
        options.lastCharacter < options.firstCharacter || options.lastCharacter > 255)
        return nullptr;

    std::string resolvedPath;
    bool realBold = false;
    bool realItalic = false;
    std::vector<unsigned char> file;
    if (!ReadFile(fontPath, file))
    {
        if (!ResolveInstalledFont(fontPath, options, resolvedPath, &realBold, &realItalic) ||
            !ReadFile(resolvedPath.c_str(), file))
            return nullptr;
    }

    stbtt_fontinfo font = {};
    const int offset = stbtt_GetFontOffsetForIndex(file.data(), 0);
    if (offset < 0 || !stbtt_InitFont(&font, file.data(), offset))
        return nullptr;

    const float scale = stbtt_ScaleForPixelHeight(&font, static_cast<float>(options.pixelHeight));
    int ascent = 0, descent = 0, lineGap = 0;
    stbtt_GetFontVMetrics(&font, &ascent, &descent, &lineGap);
    (void)descent;
    (void)lineGap;
    const int baseline = static_cast<int>(ascent * scale + 0.5f);

    H3FontBinaryHeader header = {};
    header.first = static_cast<unsigned char>(options.firstCharacter);
    header.last = static_cast<unsigned char>(options.lastCharacter);
    // With depth 1 the bitmap is a mask. Fnt_DrawSymbol recognizes 0 as
    // transparent and 0xFF as an opaque pixel using palette[color].
    header.depth = 1;
    header.xSpacing = static_cast<signed char>(std::max(-128, std::min(127, options.xSpacing)));
    header.ySpacing = static_cast<signed char>(std::max(-128, std::min(127, options.ySpacing)));
    header.height = static_cast<unsigned char>(options.pixelHeight);

    std::vector<unsigned char> bitmap;
    std::vector<unsigned char> grayscale;
    const DWORD codePage = Era::GetCodePage();
    for (int ch = 0; ch < 256; ++ch)
    {
        const bool inRange = ch >= options.firstCharacter && ch <= options.lastCharacter;
        int codepoint = inRange ? GameByteToUnicode(ch, codePage) : '?';
        if (codepoint != '?' && stbtt_FindGlyphIndex(&font, codepoint) == 0)
            codepoint = '?';
        int advance = 0, lsb = 0;
        stbtt_GetCodepointHMetrics(&font, codepoint, &advance, &lsb);

        int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
        stbtt_GetCodepointBitmapBox(&font, codepoint, scale, scale, &x0, &y0, &x1, &y1);
        const int glyphWidth = std::max(1, x1 - x0);
        const size_t glyphOffset = bitmap.size();
        header.bufferOffsets[ch] = static_cast<unsigned int>(glyphOffset);
        bitmap.resize(glyphOffset + static_cast<size_t>(glyphWidth) * options.pixelHeight, 0);
        grayscale.resize(glyphOffset + static_cast<size_t>(glyphWidth) * options.pixelHeight, 0);

        const int glyphAdvance = std::max(glyphWidth, static_cast<int>(advance * scale + 0.5f));
        header.width[ch].leftMargin = x0;
        header.width[ch].span = glyphWidth;
        header.width[ch].rightMargin = glyphAdvance - glyphWidth - x0;

        if (!inRange || x1 <= x0 || y1 <= y0)
            continue;

        int renderedWidth = 0, renderedHeight = 0, renderedX = 0, renderedY = 0;
        unsigned char *rendered = stbtt_GetCodepointBitmap(&font, scale, scale, codepoint, &renderedWidth,
                                                           &renderedHeight, &renderedX, &renderedY);
        if (!rendered)
            continue;

        const int destinationY = baseline + y0;
        for (int y = 0; y < renderedHeight; ++y)
        {
            if (destinationY + y < 0 || destinationY + y >= options.pixelHeight)
                continue;
            const int copyWidth = std::min(renderedWidth, glyphWidth);
            unsigned char *destination =
                bitmap.data() + glyphOffset + static_cast<size_t>(destinationY + y) * glyphWidth;
            const unsigned char *source = rendered + static_cast<size_t>(y) * renderedWidth;
            unsigned char *grayDestination =
                grayscale.data() + glyphOffset + static_cast<size_t>(destinationY + y) * glyphWidth;
            for (int x = 0; x < copyWidth; ++x)
                grayDestination[x] = source[x];

            if (options.bold && !realBold)
                for (int x = copyWidth - 1; x > 0; --x)
                    grayDestination[x] = std::max(grayDestination[x], grayDestination[x - 1]);

            if (options.italic && !realItalic)
            {
                const int shift = (options.pixelHeight - (destinationY + y)) / 5;
                if (shift > 0)
                {
                    for (int x = glyphWidth - 1; x >= shift; --x)
                        grayDestination[x] = grayDestination[x - shift];
                    std::memset(grayDestination, 0, static_cast<size_t>(shift));
                }
            }

            if (options.underline && destinationY + y >= options.pixelHeight - 2)
                std::memset(grayDestination, 0xFF, static_cast<size_t>(glyphWidth));
            if (options.strikeOut && destinationY + y == options.pixelHeight / 2)
                std::memset(grayDestination, 0xFF, static_cast<size_t>(glyphWidth));

            // Fnt_DrawSymbol has three cases: 0 = transparent, 0xFF = use
            // palette[color], every other non-zero value = fixed palette
            // entry at 0x20. Use 0xFF or widget colors are ignored.
            for (int x = 0; x < glyphWidth; ++x)
                destination[x] = grayDestination[x] >= options.coverageThreshold ? 0xFF : 0;
        }
        stbtt_FreeBitmap(rendered, nullptr);
    }

    h3::H3Font *object = h3::H3Alloc<h3::H3Font>();
    if (!object)
        return nullptr;

    // Fnt_Create(this, name, header, bitmapSize, bitmap), identified from the
    // game disassembly supplied with the project.
    const FntCreate create = reinterpret_cast<FntCreate>(0x4B4DD0);
    h3::H3Font *result = create(object, fontName, &header, static_cast<int>(bitmap.size()), bitmap.data());
    if (!result)
    {
        h3::H3Free(object);
        return nullptr;
    }
    THISCALL_2(void, 0x04B4EE0, result, THISCALL_1(DWORD, 0x055B5F0, 0x067F7CC));

    // Fnt_Create initializes the palette object, but a generated font has no
    // palette resource of its own. Reuse the game's already initialized font
    // palette so REGULAR/BLACK/RED/etc. resolve exactly like native fonts.

    if (addToResourceManager)
    {
        // H3ResourceManager is used by the general resource API, while
        // LoadFont/DlgText use the legacy font tree at 0x69E560. Both must
        // contain the same object for normal widget-based loading to work.
        // result->AddToResourceManager();
        RegisterInFontTree(result, fontName);
    }

    {
        std::lock_guard<std::mutex> lock(customFontsMutex);
        customFonts.push_back({result, std::move(grayscale)});
    }
    return result;
}

h3::H3Font *CreateH3FontFromWindowsName(const char *windowsFontName, const char *fontName, const TTFontOptions &options,
                                        bool addToResourceManager)
{
    std::string path;
    bool realBold = false;
    bool realItalic = false;
    if (!ResolveInstalledFont(windowsFontName, options, path, &realBold, &realItalic))
        return nullptr;

    TTFontOptions rasterOptions = options;
    if (realBold)
        rasterOptions.bold = false;
    if (realItalic)
        rasterOptions.italic = false;
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
