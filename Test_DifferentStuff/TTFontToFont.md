# Windows fonts as H3Font objects

The converter does not write a `.fnt` file. It rasterizes a TTF/OTF font in
memory and calls the Heroes III `Fnt_Create` constructor at `0x4B4DD0`, so the
result is an actual `H3Font` object with a game-owned bitmap buffer.

```cpp
TTFontOptions options;
options.pixelHeight = 16;
options.bold = true;
options.italic = true;
options.coverageThreshold = 128;
options.codePage = 1251;

h3::H3Font* font = CreateH3FontFromTTF(
    "Fonts\\DejaVuSans.ttf", "DejaVu16.fnt", options, true);
if (font)
    font->TextDraw(P_WindowManager->screenPcx16, "Hello", 10, 10, 200, 30);
```

The first argument can also be an installed Windows family name:

```cpp
TTFontOptions options;
options.pixelHeight = 16;
options.bold = true;
options.italic = true;

h3::H3Font* font = CreateH3FontFromWindowsName(
    "Arial", "Arial16BI.fnt", options, true);
```

The resolver checks both per-user and machine font registry entries and then
loads the matching file from the Windows Fonts directory. The existing
`CreateH3FontFromTTF` function also accepts a family name when the supplied
value is not an existing file path.

`codePage = 1251` converts HoMM3 Cyrillic bytes to Unicode codepoints before
TTF/OTF rasterization. Use `CP_ACP` or another single-byte Windows code page
when the game text is stored in a different locale encoding.

The exported entry point is `EraCreateH3FontFromTTF`. Set
`addToResourceManager` to `true` to register the object both in
`H3ResourceManager` and in the legacy font tree used by `H3Font::Load` and
`DlgText::Ctor`.

`bold`, `italic`, `underline` and `strikeOut` are baked into the bitmap.
The bitmap is stored as a monochrome mask (`depth = 1`): transparent pixels
are `0`, opaque pixels are `0xFF`. The latter value is required for the game
to select `palette[color]`; other non-zero values select a fixed palette entry.
The threshold can be lowered for a denser glyph or raised for a thinner glyph.

`dllmain.cpp` installs a hook at `Fnt_DrawSymbol` (`0x4B4F00`). For generated
fonts it uses the retained grayscale raster and alpha-blends it into the
16-bit destination, while the original game function still performs text
layout, wrapping and alignment.
