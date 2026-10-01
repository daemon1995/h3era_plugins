#pragma once

#include "framework.h"

namespace mrart
{

constexpr int TELEPORT_VARIANT_COUNT = 8;
constexpr int TELEPORT_QUADRANT_COUNT = 12;

struct TeleportQuadrantConfig
{
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    LPCSTR greenPcx = h3_NullString;
    LPCSTR purplePcx = h3_NullString;
};

struct TeleportVariantConfig
{
    int index = 0;
    LPCSTR name = h3_NullString;
    LPCSTR minimapPcx = h3_NullString;
    LPCSTR previewGreenPcx = h3_NullString;
    LPCSTR previewPurplePcx = h3_NullString;
    int quadrantCount = 0;
    TeleportQuadrantConfig quadrants[TELEPORT_QUADRANT_COUNT]{};
};

class TeleportConfig
{
    static LPCSTR ReadText(const H3String &key, LPCSTR fallback) noexcept
    {
        bool success = false;
        LPCSTR value = EraJS::read(key.String(), success);
        return success ? value : fallback;
    }

    static int ReadInt(const H3String &key, const int fallback) noexcept
    {
        bool success = false;
        const int value = EraJS::readInt(key.String(), success);
        return success ? value : fallback;
    }

    static H3String VariantKey(const char *format, const int variant)
    {
        return H3String::Format(format, variant);
    }

    static H3String QuadrantKey(const char *format, const int variant, const int quadrant)
    {
        return H3String::Format(format, variant, quadrant);
    }

  public:
    static TeleportVariantConfig LoadVariant(const int variant)
    {
        TeleportVariantConfig result;
        result.index = variant;

        if (variant < 0 || variant >= TELEPORT_VARIANT_COUNT)
            return result;

        static constexpr int defaultQuadrantCounts[TELEPORT_VARIANT_COUNT] = {4, 4, 8, 4, 4, 8, 4, 12};
        result.quadrantCount = ReadInt(VariantKey("mrart.teleport_dlg.variants.%d.quadrant_count", variant),
                                       defaultQuadrantCounts[variant]);
        result.quadrantCount = std::max(0, std::min(result.quadrantCount, TELEPORT_QUADRANT_COUNT));
        result.name = ReadText(VariantKey("mrart.teleport_dlg.variants.%d.name", variant), h3_NullString);
        result.minimapPcx =
            ReadText(VariantKey("mrart.teleport_dlg.variants.%d.minimap_pcx", variant), h3_NullString);
        result.previewGreenPcx =
            ReadText(VariantKey("mrart.teleport_dlg.variants.%d.preview_green_pcx", variant), h3_NullString);
        result.previewPurplePcx =
            ReadText(VariantKey("mrart.teleport_dlg.variants.%d.preview_purple_pcx", variant), h3_NullString);

        for (int quadrant = 0; quadrant < result.quadrantCount; ++quadrant)
        {
            auto &item = result.quadrants[quadrant];
            item.x = ReadInt(QuadrantKey("mrart.teleport_dlg.variants.%d.quadrants.%d.x", variant, quadrant), 0);
            item.y = ReadInt(QuadrantKey("mrart.teleport_dlg.variants.%d.quadrants.%d.y", variant, quadrant), 0);
            item.width =
                ReadInt(QuadrantKey("mrart.teleport_dlg.variants.%d.quadrants.%d.width", variant, quadrant), 0);
            item.height =
                ReadInt(QuadrantKey("mrart.teleport_dlg.variants.%d.quadrants.%d.height", variant, quadrant), 0);
            item.greenPcx = ReadText(
                QuadrantKey("mrart.teleport_dlg.variants.%d.quadrants.%d.green_pcx", variant, quadrant),
                h3_NullString);
            item.purplePcx = ReadText(
                QuadrantKey("mrart.teleport_dlg.variants.%d.quadrants.%d.purple_pcx", variant, quadrant),
                h3_NullString);
        }

        return result;
    }

    static int DialogInt(const char *key, const int fallback) noexcept
    {
        return ReadInt(H3String(key), fallback);
    }

    static LPCSTR DialogText(const char *key, LPCSTR fallback) noexcept
    {
        return ReadText(H3String(key), fallback);
    }
};

inline int PackSelection(const int variant, const int quadrant) noexcept
{
    // Both values are exposed to ERM as one DWORD. They are intentionally 1-based.
    return (variant << 16) | (quadrant & 0xFFFF);
}

} // namespace mrart
