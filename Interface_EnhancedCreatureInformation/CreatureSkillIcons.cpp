#include "pch.h"

#include "CreatureDlgHandler.h"
#include "CreatureDlgHooks.h"
#include "CreatureDlgLayout.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using namespace h3;
using namespace creatureInfo;

namespace
{
constexpr int COMMANDER_MAX_SKILLS = 15;
constexpr int MAX_CREATURE_SKILLS = 8;
constexpr int COMMANDER_SKILL_FIRST_ID = 4500;
constexpr int MIN_DESCRIPTION_HEIGHT = 14;
constexpr int ICON_BORDER_SIZE = 1;
constexpr int FIRST_CREATURE_ARTIFACT_ID = 0x9C;
constexpr int MIN_COMMANDER_ICON_SIZE = 12;
constexpr int COMMANDER_ICON_SIZE_STEP = 2;
constexpr int COMMANDER_MAX_COLUMNS = 8 + 1;
constexpr int TEXT_LINE_GAP = 2;
constexpr int MIN_TEXT_LINE_HEIGHT = 10;
constexpr int MIN_SCROLLED_CONTENT_WIDTH = 32;
constexpr int POPUP_MARKUP_CAPACITY = 48;
constexpr DWORD COMMANDER_TEXT_FUNCTION = 0x77710B;
constexpr DWORD COMMANDER_TEXT_TABLE = 0x2860724;
constexpr int COMMANDER_HINT_FIRST_ROW = 28;
constexpr int COMMANDER_DESCRIPTION_FIRST_ROW = 64;
constexpr DWORD COMBAT_EXPERIENCE_INDEX_FUNCTION = 0x716C8E;
constexpr DWORD ARMY_EXPERIENCE_SOURCE_FUNCTION = 0x71A1B7;
constexpr DWORD PREPARE_EXPERIENCE_INFO_FUNCTION = 0x71EF2B;
constexpr DWORD EXPERIENCE_INFO_ADDRESS = 0x845880;
constexpr DWORD EXPERIENCE_DIALOG_FLAGS_ADDRESS = 0x841940;
constexpr DWORD TEMPORARY_COMMANDER_ADDRESS[] = {0x2861E70, 0x2861F98};
constexpr int COMMANDER_COMBAT_SIDE_COUNT =
    sizeof(TEMPORARY_COMMANDER_ADDRESS) / sizeof(TEMPORARY_COMMANDER_ADDRESS[0]);
constexpr const char *COMMANDER_SKILL_DEF = "dlg_npc3.def";
constexpr const char *CREATURE_ARTIFACT_DEF = "Artifact.def";

static_assert(sizeof(WoG::NPC) == 296, "Unexpected WoG commander structure");
static_assert(offsetof(WoG::NPC, abilities) == 0x120, "Unexpected commander ability offset");
static_assert(sizeof(WoG::_CrExpo_) == 16, "Unexpected CrExpo record size");
static_assert(offsetof(WoG::_CrExpo_, flags) == 8, "Unexpected CrExpo artifact flags offset");
static_assert(offsetof(WoG::_CreatureExpo_, TxtProperties) == 0x10, "Unexpected CrExp text array offset");
static_assert(offsetof(WoG::_CreatureExpo_, IcoProperties) == 0x14, "Unexpected CrExp icon array offset");
static_assert(offsetof(WoG::_CreatureExpo_, HintProperties) == 0x18, "Unexpected CrExp hint array offset");
static_assert(offsetof(WoG::_CreatureExpo_, IcoPropertiesCount) == 0x34, "Unexpected CrExp icon count offset");
static_assert(offsetof(WoG::_CreatureExpo_, ArtIcon) == 0x48, "Unexpected CrExp artifact icon offset");
static_assert(offsetof(WoG::_CreatureExpo_, ArtHint) == 0x4C, "Unexpected CrExp artifact hint offset");

struct IconPanelLayout
{
    int iconSize = 0;
    int columns = 0;
    int height = 0;
};

IconPanelLayout CalculateIconPanelLayout(int count, int width, int iconSize, int maxColumns)
{
    IconPanelLayout result;
    const int columns = std::min(maxColumns, (width + EXP_SKILL_GAP) / (iconSize + EXP_SKILL_GAP));
    if (count <= 0 || columns <= 0)
        return result;

    result.iconSize = iconSize;
    result.columns = std::min(count, columns);
    result.height = ((count + result.columns - 1) / result.columns) * (iconSize + EXP_SKILL_GAP);
    return result;
}

IconPanelLayout CalculateCommanderPanelLayout(int count, int width, int availableHeight)
{
    // Keep at least one description line below the commander skills.
    for (int size = EXP_SKILL_ICON_SIZE; size >= MIN_COMMANDER_ICON_SIZE; size -= COMMANDER_ICON_SIZE_STEP)
    {
        const auto layout = CalculateIconPanelLayout(count, width, size, COMMANDER_MAX_COLUMNS);
        if (layout.height && layout.height + MIN_DESCRIPTION_HEIGHT <= availableHeight)
            return layout;
    }
    return IconPanelLayout();
}

int CalculateExperiencePanelLayout(int count, int width, H3Font *font, const std::string &text, int lineHeight,
                                   IconPanelLayout &layout, H3Vector<H3String> &lines)
{
    // A narrow configured area still needs one column, as in the native panel.
    layout = CalculateIconPanelLayout(count, std::max(width, EXP_SKILL_ICON_SIZE), EXP_SKILL_ICON_SIZE, count);
    lines.RemoveAll();
    font->SplitTextIntoLines(text.c_str(), width, lines);
    return layout.height + (lines.Size() ? TEXT_LINE_GAP : 0) + static_cast<int>(lines.Size()) * lineHeight;
}

void DrawSkillIconBorder(H3LoadedPcx16 *picture, int frame, bool isArtifact)
{
    const bool active = isArtifact || (frame & 1) != 0;
    const H3RGB888 border = active ? H3RGB888(232, 212, 120) : H3RGB888(104, 104, 96);
    picture->DrawThickFrame(0, 0, picture->width, picture->height, ICON_BORDER_SIZE, border);
}

class SkillIconRenderer
{
    H3DefLoader def;
    H3LoadedPcx16 *source = nullptr;

  public:
    explicit SkillIconRenderer(LPCSTR defName) : def(defName)
    {
        if (FrameCount() > 0)
            source = H3LoadedPcx16::Create(def->widthDEF, def->heightDEF);
    }

    ~SkillIconRenderer()
    {
        if (source)
            source->Destroy();
    }

    SkillIconRenderer(const SkillIconRenderer &) = delete;
    SkillIconRenderer &operator=(const SkillIconRenderer &) = delete;

    int FrameCount()
    {
        return def.Get() && def->groups && def->groups[0] && def->widthDEF > 0 && def->heightDEF > 0
                   ? def->groups[0]->count
                   : 0;
    }

    H3LoadedPcx16 *CreateImage(int frame, int size, bool isArtifact = false)
    {
        if (!source || frame < 0 || frame >= FrameCount() || size <= 2 * ICON_BORDER_SIZE)
            return nullptr;

        auto *picture = H3LoadedPcx16::Create(size, size);
        if (!picture)
            return nullptr;

        source->FillRectangle(0, 0, source->width, source->height, 0, 0, 0);
        def->DrawToPcx16(0, frame, source, 0, 0);
        // Reserve a pixel around the image so the border does not cover the skill.
        resized::H3LoadedPcx16Resized::DrawPcx16ResizedBicubic(
            picture, source, source->width, source->height, ICON_BORDER_SIZE, ICON_BORDER_SIZE,
            size - 2 * ICON_BORDER_SIZE, size - 2 * ICON_BORDER_SIZE);

        DrawSkillIconBorder(picture, frame, isArtifact);
        return picture;
    }
};

LPCSTR MakeSkillPopup(std::vector<char> &buffer, LPCSTR defName, int frame, LPCSTR text)
{
    text = text ? text : h3_NullString;
    buffer.resize(std::strlen(defName) + std::strlen(text) + POPUP_MARKUP_CAPACITY);
    std::snprintf(buffer.data(), buffer.size(), "{~>%s:0:%d block}\n\n%s", defName, frame, text);
    return buffer.data();
}

H3DlgPcx16 *AddSkillIcon(H3CreatureInfoDlg *dlg, int x, int y, int itemId, H3LoadedPcx16 *&picture, LPCSTR hint,
                         LPCSTR popup, H3DlgPcx16 *item = nullptr)
{
    if (!picture)
        return nullptr;

    const bool newItem = item == nullptr;
    if (newItem)
        item = H3DlgPcx16::Create(x, y, picture->width, picture->height, itemId, nullptr);
    if (!item)
        return nullptr;

    item->SetX(x);
    item->SetY(y);
    // WoG's prepared strings are shared scratch buffers. Keep just raw pointers
    // while building the panel, then let the item copy each hint exactly once.
    item->SetHints(hint ? hint : h3_NullString, popup, TRUE);
    // The native item destructor dereferences its image. Transfer each image
    // exactly once; a handler only releases images that have no item yet.
    auto *previousPicture = item->GetPcx();
    item->SetPcx(picture);
    picture = nullptr;
    if (previousPicture)
        previousPicture->Dereference();
    if (newItem)
        dlg->AddItem(item);
    return item;
}

struct CommanderSkillFrames
{
    std::array<int, COMMANDER_MAX_SKILLS> frames = {};
    int count = 0;
};

CommanderSkillFrames CollectCommanderSkillFrames(unsigned learned)
{
    CommanderSkillFrames result;
    for (int skill = 0; skill < COMMANDER_MAX_SKILLS; ++skill)
        if (learned & (1u << skill))
            result.frames[result.count++] = 2 * skill + 1;
    return result;
}

struct PreparedSkill
{
    int frame = -1;
    const char *text = nullptr;
    const char *hint = nullptr;
};

struct PreparedSkillList
{
    std::array<PreparedSkill, MAX_CREATURE_SKILLS> skills = {};
    int count = 0;
};

PreparedSkillList ReadPreparedCreatureSkills(const WoG::_CreatureExpo_ &info, int frameCount, bool activeOnly)
{
    PreparedSkillList result;
    const int count = std::max(0, std::min(info.IcoPropertiesCount, MAX_CREATURE_SKILLS));
    if (!info.IcoPropertiesInt)
        return result;

    for (int i = 0; i < count; ++i)
    {
        const int frame = info.IcoPropertiesInt[i];
        if (frame <= 0 || frame >= frameCount || (activeOnly && (frame & 1) == 0))
            continue;

        const char *text = info.TxtProperties ? info.TxtProperties[i] : nullptr;
        const char *hint = info.HintProperties ? info.HintProperties[i] : nullptr;
        PreparedSkill skill;
        skill.frame = frame;
        skill.text = text && *text ? text : (hint ? hint : h3_NullString);
        skill.hint = hint && *hint ? hint : skill.text;
        result.skills[result.count++] = skill;
    }
    return result;
}

int ReadCreatureArtifactFrame(const WoG::_CrExpo_ *crExpo)
{
    // SetArt stores (artifactId - 0x9C) & 3 in mArt. Artifact.def starts
    // directly with artifact 0; the prepared WoG string adds an empty-slot offset.
    return crExpo && crExpo->flags.mHasArt ? FIRST_CREATURE_ARTIFACT_ID + crExpo->flags.mArt : -1;
}

class SkillScrollbar : public H3DlgScrollbar
{
  public:
    void UpdateRange(int count, int position)
    {
        // H3API exposes this native virtual setter only as a protected method.
        vSetTickCount(count);
        SetTick(std::max(0, std::min(position, count - 1)));
        SetButtonPosition();
    }
};
} // namespace

BOOL CreatureDlgHandler::AddCommanderSkills()
{
    if (!dlg || !Era::IsCommanderId(dlg->creatureId))
        return FALSE;

    WoG::NPC *npc = nullptr;
    if (stack && stack->side >= 0 && stack->side < COMMANDER_COMBAT_SIDE_COUNT)
    {
        // The actual side also remains correct when the stack is hypnotized.
        auto *owner = P_CombatManager->hero[stack->side];
        npc = owner ? WoG::NPC::Get(owner->id) : reinterpret_cast<WoG::NPC *>(TEMPORARY_COMMANDER_ADDRESS[stack->side]);
    }
    else if (!stack && hero)
        npc = WoG::NPC::Get(hero->id);
    if (!npc)
        return FALSE;

    const auto frames = CollectCommanderSkillFrames(npc->abilities.bits);
    const auto layout = CalculateCommanderPanelLayout(frames.count, descriptionWidth, CONTENT_BOTTOM - descriptionY);
    if (!layout.height)
        return FALSE;

    SkillIconRenderer renderer(COMMANDER_SKILL_DEF);
    if (renderer.FrameCount() < 2 * COMMANDER_MAX_SKILLS)
        return FALSE;

    constexpr unsigned char textRows[COMMANDER_MAX_SKILLS] = {1, 2, 3, 4, 5, 8, 9, 10, 11, 15, 16, 17, 22, 23, 29};
    int added = 0;
    for (int skill = 0; skill < frames.count; ++skill)
    {
        const int frame = frames.frames[skill];
        const int row = textRows[(frame - 1) / 2];
        auto *picture = renderer.CreateImage(frame, layout.iconSize);
        if (!picture)
            break;

        const char *hint =
            CDECL_3(char *, COMMANDER_TEXT_FUNCTION, COMMANDER_HINT_FIRST_ROW + row, 1, COMMANDER_TEXT_TABLE);
        const char *description =
            CDECL_3(char *, COMMANDER_TEXT_FUNCTION, COMMANDER_DESCRIPTION_FIRST_ROW + row, 1, COMMANDER_TEXT_TABLE);
        const char *popup = MakeSkillPopup(skillPopupBuffer, COMMANDER_SKILL_DEF, frame, description);
        if (!AddSkillIcon(dlg, descriptionX + (added % layout.columns) * (layout.iconSize + EXP_SKILL_GAP),
                          descriptionY + (added / layout.columns) * (layout.iconSize + EXP_SKILL_GAP),
                          COMMANDER_SKILL_FIRST_ID + added, picture, hint, popup))
        {
            picture->Destroy();
            break;
        }
        ++added;
    }
    if (!added)
        return FALSE;

    commanderPanelHeight = ((added + layout.columns - 1) / layout.columns) * (layout.iconSize + EXP_SKILL_GAP);
    const int totalHeight = std::min(CONTENT_BOTTOM - descriptionY,
                                     std::max(descriptionHeight, commanderPanelHeight + MIN_DESCRIPTION_HEIGHT));
    descriptionY += commanderPanelHeight;
    descriptionHeight = totalHeight - commanderPanelHeight;
    return TRUE;
}

BOOL CreatureDlgHandler::CreateCreatureSkillsList()
{
    ReleaseUnownedSkillImages();
    if (!dlg || !wogStackExperience)
        return FALSE;

    const int creatureId = dlg->creatureId;
    const bool hasArmyExperienceSource = army && armySlotIndex >= 0 && armySlotIndex < limits::ARMY_SLOTS;
    WoG::_CrExpo_ *crExpo = nullptr;
    int stackId = -1;
    if (stack)
    {
        stackId = CDECL_1(int, COMBAT_EXPERIENCE_INDEX_FUNCTION, stack);
        crExpo = WoG::_CrExpo_::GetFromCombatCreatureIndex(stackId);
    }
    else if (hasArmyExperienceSource)
    {
        WoG::eExpType expType = WoG::CE_UNKNOWN;
        WoG::_CrExpo_::UniData expData = {};
        if (CDECL_4(int, ARMY_EXPERIENCE_SOURCE_FUNCTION, army, armySlotIndex, &expType, &expData) &&
            expType >= WoG::CE_FIRST && expType <= WoG::CE_LAST)
            crExpo = WoG::_CrExpo_::Find(expType, expData);
    }

    WoG::CrExpBon::MakeCurrent(stackId, creatureId);
    int creatureCount = dlg->numberCreatures;
    if (creatureCount <= 0 && hasArmyExperienceSource)
        creatureCount = army->count[armySlotIndex];
    creatureCount = std::max(1, creatureCount);

    IntAt(EXPERIENCE_DIALOG_FLAGS_ADDRESS) = 0;
    CDECL_5(void, PREPARE_EXPERIENCE_INFO_FUNCTION, creatureId, creatureCount, crExpo ? crExpo->experience : 0, crExpo,
            hero ? 1 : 0);
    const auto &info = *reinterpret_cast<WoG::_CreatureExpo_ *>(EXPERIENCE_INFO_ADDRESS);
    SkillIconRenderer renderer(CREATURE_EXP_DEF);
    const bool activeOnly = !GetPluginSettings().showInactiveCreatureSkills && (stack || hasArmyExperienceSource);
    const auto preparedSkills = ReadPreparedCreatureSkills(info, renderer.FrameCount(), activeOnly);
    creatureSkills.reserve(preparedSkills.count + 1);
    for (int i = 0; i < preparedSkills.count; ++i)
    {
        const auto &prepared = preparedSkills.skills[i];
        auto *picture = renderer.CreateImage(prepared.frame, EXP_SKILL_ICON_SIZE);
        if (!picture)
            continue;

        SkillIcon skill;
        skill.defName = CREATURE_EXP_DEF;
        skill.frame = prepared.frame;
        skill.text = prepared.text;
        skill.hint = prepared.hint;
        skill.pcx16 = picture;
        creatureSkills.push_back(skill);
    }

    if (crExpo && crExpo->flags.mHasArt)
    {
        SkillIconRenderer artifactRenderer(CREATURE_ARTIFACT_DEF);
        const int frame = ReadCreatureArtifactFrame(crExpo);
        auto *picture = artifactRenderer.CreateImage(frame, EXP_SKILL_ICON_SIZE, true);
        if (picture)
        {
            SkillIcon artifact;
            artifact.defName = CREATURE_ARTIFACT_DEF;
            artifact.frame = frame;
            artifact.text = info.ArtHint ? info.ArtHint : h3_NullString;
            artifact.hint = artifact.text;
            artifact.pcx16 = picture;
            creatureSkills.push_back(artifact); // equipped artifact is always last
        }
    }
    return !creatureSkills.empty();
}

BOOL CreatureDlgHandler::BuildExperienceSkillsPanel(H3DlgText *description, const std::string &descriptionText)
{
    if (creatureSkills.empty())
        return FALSE;

    scrolledItems.clear();
    viewportX = descriptionX;
    viewportY = descriptionY;
    viewportWidth = descriptionWidth;
    viewportHeight = std::max(descriptionHeight, CONTENT_BOTTOM - descriptionY);
    maxScrollTick = 0;
    scrollRowOffsets.clear();

    const char *fontName =
        description && description->GetFont() ? description->GetFont()->GetName() : NH3Dlg::Text::TINY;
    const int textColor = description ? description->color : eTextColor::REGULAR;
    const int textAlign = description ? description->alignment : GetPluginSettings().descriptionAlignment;
    H3Font *font = H3Font::Load(fontName);
    if (!font)
    {
        ReleaseUnownedSkillImages();
        return FALSE;
    }

    int contentWidth = descriptionWidth;
    IconPanelLayout layout;
    const int lineHeight = std::max(MIN_TEXT_LINE_HEIGHT, static_cast<int>(font->height) + TEXT_LINE_GAP);

    H3Vector<H3String> lines;
    int totalHeight = CalculateExperiencePanelLayout(static_cast<int>(creatureSkills.size()), contentWidth, font,
                                                     descriptionText, lineHeight, layout, lines);
    const bool needScrollbar = totalHeight > viewportHeight;

    if (needScrollbar)
    {
        contentWidth = std::max(MIN_SCROLLED_CONTENT_WIDTH, descriptionWidth - EXP_SCROLLBAR_WIDTH - EXP_SCROLLBAR_GAP);
        totalHeight = CalculateExperiencePanelLayout(static_cast<int>(creatureSkills.size()), contentWidth, font,
                                                     descriptionText, lineHeight, layout, lines);
    }

    const int columns = layout.columns;
    const int rows = (static_cast<int>(creatureSkills.size()) + columns - 1) / columns;
    const int skillBlockHeight = layout.height;
    // Build logical rows after the final content width is known.
    // One icon row == one tick; one wrapped text line == one tick.
    scrollRowOffsets.reserve(static_cast<size_t>(rows) + lines.Size());
    for (int row = 0; row < rows; ++row)
        scrollRowOffsets.push_back(row * (EXP_SKILL_ICON_SIZE + EXP_SKILL_GAP));

    const int textStartOffset = skillBlockHeight + (lines.Size() ? TEXT_LINE_GAP : 0);
    for (UINT32 lineIndex = 0; lineIndex < lines.Size(); ++lineIndex)
        scrollRowOffsets.push_back(textStartOffset + static_cast<int>(lineIndex) * lineHeight);

    if (needScrollbar && totalHeight > viewportHeight && !scrollRowOffsets.empty())
    {
        const int overflowPixels = totalHeight - viewportHeight;

        // Find how many complete logical rows have to be skipped before the
        // remaining content fits in the viewport. This row index is max tick.
        maxScrollTick = static_cast<int>(scrollRowOffsets.size()) - 1;
        for (size_t rowIndex = 1; rowIndex < scrollRowOffsets.size(); ++rowIndex)
        {
            if (scrollRowOffsets[rowIndex] >= overflowPixels)
            {
                maxScrollTick = static_cast<int>(rowIndex);
                break;
            }
        }
    }

    if (skillItems.size() < creatureSkills.size())
        skillItems.resize(creatureSkills.size(), nullptr);
    int addedSkills = 0;
    for (size_t i = 0; i < creatureSkills.size(); ++i)
    {
        SkillIcon &skill = creatureSkills[i];
        const int baseY = descriptionY + static_cast<int>(i / columns) * (EXP_SKILL_ICON_SIZE + EXP_SKILL_GAP);
        const int x = descriptionX + static_cast<int>(i % columns) * (EXP_SKILL_ICON_SIZE + EXP_SKILL_GAP);
        const char *popup = MakeSkillPopup(skillPopupBuffer, skill.defName, skill.frame, skill.text);
        auto *item = AddSkillIcon(dlg, x, baseY, CREATURE_EXP_SKILL_FIRST_ID + static_cast<int>(i), skill.pcx16,
                                  skill.hint, popup, skillItems[i]);
        if (!item)
            continue;
        skillItems[i] = item;
        scrolledItems.push_back({item, baseY});
        ++addedSkills;
    }

    if (!addedSkills)
    {
        font->Dereference();
        ReleaseUnownedSkillImages();
        scrolledItems.clear();
        return FALSE;
    }

    const int textStartY = descriptionY + skillBlockHeight + (lines.Size() ? TEXT_LINE_GAP : 0);
    if (descriptionLineItems.size() < lines.Size())
        descriptionLineItems.resize(lines.Size(), nullptr);
    for (UINT32 i = 0; i < lines.Size(); ++i)
    {
        auto *line = descriptionLineItems[i];
        if (line)
        {
            line->SetX(descriptionX);
            line->SetY(textStartY + static_cast<int>(i) * lineHeight);
            line->SetWidth(contentWidth);
            line->SetHeight(lineHeight);
            line->SetText(lines[i].String());
        }
        else
        {
            line = H3DlgText::Create(descriptionX, textStartY + static_cast<int>(i) * lineHeight, contentWidth,
                                     lineHeight, lines[i].String(), fontName, textColor,
                                     CREATURE_EXP_TEXT_FIRST_ID + static_cast<int>(i), textAlign);
            if (line)
            {
                descriptionLineItems[i] = line;
                dlg->AddItem(line);
            }
        }
        if (!line)
            continue;
        scrolledItems.push_back({line, line->GetY()});
    }

    font->Dereference();
    ReleaseUnownedSkillImages();

    if (needScrollbar && maxScrollTick > 0)
    {
        if (!skillsScrollbar)
        {
            skillsScrollbar =
                H3DlgScrollbar::Create(descriptionX + descriptionWidth - EXP_SCROLLBAR_WIDTH, descriptionY,
                                       EXP_SCROLLBAR_WIDTH, viewportHeight, CREATURE_EXP_SCROLLBAR_ID,
                                       maxScrollTick + 1, CreatureSkillsScrollbarProc, false, 1, true);
            if (skillsScrollbar)
                dlg->AddItem(skillsScrollbar);
        }
        if (skillsScrollbar)
        {
            reinterpret_cast<SkillScrollbar *>(skillsScrollbar)->UpdateRange(maxScrollTick + 1, 0);
            skillsScrollbar->ShowActivate();
        }
        else
        {
            maxScrollTick = 0;
            scrollRowOffsets.clear();
        }
    }

    ScrollTo(0);
    return TRUE;
}

void CreatureDlgHandler::ResetExperienceSkillsPanel()
{
    for (auto *item : skillItems)
    {
        if (!item)
            continue;
        item->HideDeactivate();
        auto *picture = item->GetPcx();
        item->SetPcx(nullptr);
        if (picture)
            picture->Dereference();
    }
    for (auto *line : descriptionLineItems)
        if (line)
            line->HideDeactivate();
    if (skillsScrollbar)
        skillsScrollbar->HideDeactivate();
    scrolledItems.clear();
    scrollRowOffsets.clear();
    maxScrollTick = 0;
    currentScrollTick = 0;
}

void CreatureDlgHandler::RefreshCreatureArtifact()
{
    if (!dlg || Era::IsCommanderId(dlg->creatureId))
        return;

    const int previousScrollTick = currentScrollTick;
    // PrepareInfo must run again: the experience dialog can remove the artifact,
    // change its mode, and change the effects listed among the creature skills.
    BuildDescriptionArea();
    const int scrollTick = std::max(0, std::min(previousScrollTick, maxScrollTick));
    if (skillsScrollbar && maxScrollTick > 0)
        reinterpret_cast<SkillScrollbar *>(skillsScrollbar)->UpdateRange(maxScrollTick + 1, scrollTick);
    ScrollTo(scrollTick);
    if (auto *hint = dlg->GetTextPcx(DLG_HINT_ID))
        hint->SetText(h3_NullString);
    dlg->Redraw();
}

void CreatureDlgHandler::ReleaseUnownedSkillImages()
{
    for (auto &skill : creatureSkills)
    {
        if (skill.pcx16)
        {
            skill.pcx16->Destroy();
            skill.pcx16 = nullptr;
        }
    }
    creatureSkills.clear();
}

EXTERN_C __declspec(dllexport) void AddExternalCreatureSkill(const char *name, const char *description,
                                                             const char *pcx16Name)
{
    // Reserved ABI entry point. Creature Experience skills are currently read
    // from WoG's prepared _CreatureExpo_ structure.
}
