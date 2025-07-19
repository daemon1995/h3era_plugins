#include "pch.h"

// using namespace EraJS;
POINT WgtSettings::artSlotPositions[19];
WgtSettings::ButtonsAlignment WgtSettings::buttonsAlignment;

POINT WgtSettings::sacrifaceArtSlotPositions[19] = {
    {0x1AE, 0x11}, {0x1f3, 0x11},  {0x169, 0x11}, {0x137, 0x11},  {0x225, 0x11}, {0x169, 0x13F}, {0x137, 0x43},
    {0x225, 0x43}, {0x1F3, 0x13F}, {0x137, 0x77}, {0x137, 0xA9},  {0x137, 0xDB}, {0x137, 0x10D}, {0x225, 0xA9},
    {0x225, 0xDB}, {0x225, 0x10D}, {0x225, 0x77}, {0x225, 0x13F}, {0x137, 0x13F} };

void WgtSettings::loadArtSettings() noexcept
{

    for (size_t i = 0; i < 19; i++)
    {
        sprintf(h3_TextBuffer, "nhd.artifact_slots.slot_%d.x", i);
        artSlotPositions[i].x = EraJS::readInt(h3_TextBuffer);
        sprintf(h3_TextBuffer, "nhd.artifact_slots.slot_%d.y", i);
        artSlotPositions[i].y = EraJS::readInt(h3_TextBuffer);
    }
}
WgtSettings::WgtSettings(const std::string& jsonKey)
//: jsonKy(jsonKey)
{

    startPos.x = EraJS::readInt(jsonKey + ".x");
    startPos.y = EraJS::readInt(jsonKey + ".y");

    width = EraJS::readInt(jsonKey + ".width");
    height = EraJS::readInt(jsonKey + ".height");

    arrows[0].x = EraJS::readInt(jsonKey + ".left_arrow.x");
    arrows[0].y = EraJS::readInt(jsonKey + ".left_arrow.y");
    arrows[1].x = EraJS::readInt(jsonKey + ".right_arrow.x");
    arrows[1].y = EraJS::readInt(jsonKey + ".right_arrow.y");

    backpack.pos.x = EraJS::readInt(jsonKey + ".backpack.x");
    backpack.pos.y = EraJS::readInt(jsonKey + ".backpack.y");
    backpack.interval = EraJS::readInt(jsonKey + ".backpack.interval");
    backpack.altitude = EraJS::readInt(jsonKey + ".backpack.altitude");

    bgId = EraJS::readInt(jsonKey + ".bg_item_id");
}

#include <sstream>
HeroWgt::HeroWgt(const std::string& jsonKey) : WgtSettings(jsonKey)
{
    npcBttn.pos.x = EraJS::readInt(jsonKey + ".commander.x");
    npcBttn.pos.y = EraJS::readInt(jsonKey + ".commander.y");
    npcBttn.id = EraJS::readInt(jsonKey + ".commander.x");
    npcBttn.id = EraJS::readInt(jsonKey + ".commander.x");
    npcBttn.id = EraJS::readInt(jsonKey + ".commander.x");
    npcBttn.id = EraJS::readInt(jsonKey + ".commander.x");
    npcBttn.defName = EraJS::read(jsonKey + ".commander.def");

    mgrBttn.id = EraJS::readInt(jsonKey + ".manager.id");
    mgrBttn.defName = EraJS::read(jsonKey + ".manager.def");
    mgrBttn.pos.x = EraJS::readInt(jsonKey + ".manager.x");
    mgrBttn.pos.y = EraJS::readInt(jsonKey + ".manager.y");
    mgrBttn.altPos.x = EraJS::readInt(jsonKey + ".manager.xNoNpc");
    mgrBttn.altPos.y = EraJS::readInt(jsonKey + ".manager.yNoNpc");

    mgrBttn.hint = Era::tr("nhd.button.manager");

    buttonsAlignment.buttonsToAline.Resize(0);
    // buttonsToAline.Reserve(20);

    std::stringstream ss(EraJS::read(jsonKey + ".buttons_alignement.tags"));
    int counter{};
    while (ss.good() || counter++ < 10)
    {
        std::string substr;
        getline(ss, substr, ',');
        buttonsAlignment.buttonsToAline.Push(substr);
    }

    buttonsAlignment.x = EraJS::readInt(jsonKey + ".buttons_alignement.x");
    buttonsAlignment.y = EraJS::readInt(jsonKey + ".buttons_alignement.y");
    buttonsAlignment.width = EraJS::readInt(jsonKey + ".buttons_alignement.width");
    buttonsAlignment.height = EraJS::readInt(jsonKey + ".buttons_alignement.height");
    buttonsAlignment.totalWidth = EraJS::readInt(jsonKey + ".buttons_alignement.field_width");
    // buttonsToAline.Resize(1);
}

MeetWgt::MeetWgt(const std::string& jsonKey) : WgtSettings(jsonKey)
{
    bg.pos.x = EraJS::readInt(jsonKey + ".background.x");
    bg.pos.y = EraJS::readInt(jsonKey + ".background.y");
    bg.width = EraJS::readInt(jsonKey + ".background.width");
    bg.height = EraJS::readInt(jsonKey + ".background.height");
}