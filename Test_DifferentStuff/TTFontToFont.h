#pragma once

#include "framework.h"
#include <string>

struct TTFontOptions
{
    int pixelHeight = 16;
    int firstCharacter = 0;
    int lastCharacter = 255;
    int xSpacing = 1;
    int ySpacing = 1;
    bool bold = false;
    bool italic = false;
    bool underline = false;
    bool strikeOut = false;
    // HoMM3 classic text is normally a single-byte Windows-1251 string.
    // Set to CP_ACP for the active Windows ANSI code page when needed.
    UINT codePage = 1251;
    // Coverage threshold used when converting stb_truetype grayscale to the
    // monochrome mask required by the original H3 renderer.
    unsigned char coverageThreshold = 128;
};

// Builds a real game H3Font object. No .fnt file is written.
// The returned object is allocated with the Heroes III allocator and its
// bitmap buffer is owned by the game object.
h3::H3Font* CreateH3FontFromTTF(const char* fontPath, const char* fontName,
                                const TTFontOptions& options = TTFontOptions(),
                                bool addToResourceManager = false);

// fontPath may be a file path or an installed Windows family name such as
// "Arial". Style flags are used to select Arial Bold/Italic variants when
// those variants are registered in Windows.
h3::H3Font* CreateH3FontFromWindowsName(const char* windowsFontName,
                                        const char* fontName,
                                        const TTFontOptions& options = TTFontOptions(),
                                        bool addToResourceManager = false);

bool DrawCustomH3Glyph(h3::H3Font* font, unsigned int character,
                       h3::H3LoadedPcx16* drawBuffer, int x, int y, int color);

// Returns one justified text line. Extra ASCII spaces are distributed between
// existing words until the line reaches targetWidth pixels (or as close as
// possible using whole game-space glyphs).
std::string JustifyH3TextLine(const h3::H3Font* font, const char* text, int targetWidth);

extern "C"
{
    __declspec(dllexport) h3::H3Font* __stdcall EraCreateH3FontFromTTF(
        const char* fontPath, const char* fontName, int pixelHeight,
        bool addToResourceManager, bool bold, bool italic,
        bool underline, bool strikeOut);
    __declspec(dllexport) h3::H3Font* __stdcall EraCreateH3FontFromWindowsName(
        const char* windowsFontName, const char* fontName, int pixelHeight,
        bool addToResourceManager, bool bold, bool italic,
        bool underline, bool strikeOut);
}
