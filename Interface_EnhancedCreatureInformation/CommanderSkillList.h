#pragma once

namespace commanderPreview
{
enum
{
    MAX_SKILLS = 15,
    FIRST_ITEM_ID = 4500
};

struct SkillList
{
    unsigned char frames[MAX_SKILLS] = {};
    int count = 0;
};

// The creature information window shows only learned abilities, in DEF order.
inline SkillList Collect(unsigned learned)
{
    SkillList result;
    for (int skill = 0; skill < MAX_SKILLS; ++skill)
        if (learned & (1u << skill))
            result.frames[result.count++] = static_cast<unsigned char>(2 * skill + 1);
    return result;
}

inline bool IsSkillItem(int id)
{
    return id >= FIRST_ITEM_ID && id < FIRST_ITEM_ID + MAX_SKILLS;
}

struct PanelLayout
{
    int iconSize = 0;
    int columns = 0;
    int height = 0;
};

inline PanelLayout Layout(int count, int width, int availableHeight)
{
    PanelLayout result;
    // Leave at least one line of creature description below the icons.
    for (int size = 24; size >= 12; size -= 2)
    {
        int columns = (width + 4) / (size + 4);
        if (columns > 8)
            columns = 8;
        if (count <= 0 || count > MAX_SKILLS || columns <= 0)
            continue;
        int height = ((count + columns - 1) / columns) * (size + 4);
        if (height + 14 <= availableHeight)
        {
            result.iconSize = size;
            result.columns = columns;
            result.height = height;
            break;
        }
    }
    return result;
}
} // namespace commanderPreview
