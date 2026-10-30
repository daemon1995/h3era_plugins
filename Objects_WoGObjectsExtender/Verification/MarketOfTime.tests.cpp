#include <WoGObjectPolicy.cpp>
// The runner compiles the unchanged production source against native API stubs.
#include <MarketOfTime.cpp>
#include "WoGHandlersStubs.h"
#include <WoGObjectsExtender.cpp>
#include <climits>
// This suite isolates the unchanged Market implementation; Artificer has its own native-adapter suite.
namespace wog { namespace artificer {
void RegisterEvents() {}
BOOL Initialize(H3MapItem *) noexcept { return TRUE; }
BOOL Visit(H3Hero *, H3MapItem *, H3Position, BOOL) noexcept { return TRUE; }
BOOL SetHint(H3MapItem *, const H3Hero *, BOOL) noexcept { return TRUE; }
BOOL SetAiWeight(H3MapItem *, H3Hero *, H3Position, int &weight) noexcept { weight = 0; return TRUE; }
} }

using namespace wog::marketOfTime;

static void AssertUnchanged(const H3Hero &hero, int gold = 3000, int movement = 1000)
{
    assert(fake::main.players[hero.owner].playerResources.gold == gold);
    assert(hero.movement == movement && hero.recalculations == 0);
    assert(fake::receivers == 0);
}

int main()
{
    H3MapItem object;
    assert(Handles(&object));
    object.objectType = 63; assert(Handles(&object));
    object.objectType = 141; object.objectSubtype = 52; assert(!Handles(&object));
    assert(!Handles(nullptr));

    H3Hero hero;
    fake::Reset(hero);
    assert(Visit(nullptr, TRUE));
    assert(Visit(&hero, FALSE));
    AssertUnchanged(hero);
    assert(fake::messages.empty());

    extender::ObjectExtender &host = wog::WoGObjectsExtender::Get();
    std::array<INT16, 232> maxSubtypes{};
    maxSubtypes[63] = 300; maxSubtypes[141] = 51;
    host.AfterLoadingObjectsTxtProc(maxSubtypes.data());
    assert(host.GetObjectSubtypesInfo().size() == 323);
    assert(host.GetObjectSubtypesInfo().front().type == 63 && host.GetObjectSubtypesInfo().front().subtype == 1);
    assert(host.GetObjectSubtypesInfo().back().type == 141 && host.GetObjectSubtypesInfo().back().subtype == 66);
    host.AfterLoadingObjectsTxtProc(maxSubtypes.data());
    assert(host.GetObjectSubtypesInfo().size() == 323);
    for (int type : {63, 141})
    {
        for (BOOL preview : {FALSE, TRUE})
        {
            fake::option = 0;
            auto *generator = host.CreateRMGObjectGen(extender::RMGObjectProperties(type, 51), preview);
            assert((generator != nullptr) == (type == 141));
            if (generator) { assert(generator->type == 141 && generator->subtype == 51); delete generator; }
        }
    }
    assert(!host.CreateRMGObjectGen(extender::RMGObjectProperties(63, 52), FALSE));
    assert(!host.CreateRMGObjectGen(extender::RMGObjectProperties(141, 53), FALSE));
    assert(!wog::WoGObjectsExtender::WoGObjectHasOptionEnabled(extender::RMGObjectProperties(63, -1)));
    assert(!wog::WoGObjectsExtender::WoGObjectHasOptionEnabled(extender::RMGObjectProperties(63, 301)));
    assert(!wog::WoGObjectsExtender::IsWoGOptionEnabled(-1));
    assert(!wog::WoGObjectsExtender::IsWoGOptionEnabled(1000));

    for (int type : {63, 141})
    {
        fake::Reset(hero); object = {type, 51}; hero.secSkill[5] = 3;
        fake::choices = {0};
        assert(host.VisitMapItem(&hero, &object, 0, TRUE));
        assert(fake::receivers == 0 && hero.secSkill[5] == 0);
        assert(hero.movement == 500 && fake::main.players[3].playerResources.gold == 1000);
        assert(host.SetHintInH3TextBuffer(&object, &hero, hero.owner, FALSE));
        assert(std::strstr(h3_TextBuffer, ".name"));
        assert(host.SetHintInH3TextBuffer(&object, &hero, hero.owner, TRUE));
        assert(std::strstr(h3_TextBuffer, "gold 2000"));
        int aiWeight = 123;
        assert(host.SetAiMapItemWeight(&object, &hero, nullptr, aiWeight, nullptr, 0) && aiWeight == 0);
    }
    for (int type : {63,141})
    {
        fake::Reset(hero); object = {type,51}; hero.secSkill[5] = 1;
        wog::policy::InitializeDefaults();
        Era::SetAssocVarIntValue(wog::policy::Key(type,51).c_str(),0);
        int weight=123; std::strcpy(h3_TextBuffer,"unchanged");
        assert(!host.VisitMapItem(&hero,&object,0,TRUE));
        assert(!host.SetHintInH3TextBuffer(&object,&hero,3,TRUE));
        assert(!host.SetAiMapItemWeight(&object,&hero,nullptr,weight,nullptr,0));
        assert(weight==123 && std::strcmp(h3_TextBuffer,"unchanged")==0);
        AssertUnchanged(hero); assert(fake::messages.empty());
        object.objectType=type==63?141:63; fake::choices={0}; fake::option=0;
        assert(host.VisitMapItem(&hero,&object,0,TRUE));
        assert(hero.secSkill[5]==0); // The other identity is independent, including with WoG option off.
    }
    fake::Reset(hero); object = {63, 53};
    assert(!host.VisitMapItem(&hero, &object, 0, TRUE));
    assert(!host.SetHintInH3TextBuffer(&object, &hero, 0, TRUE));
    object = {141, 53}; assert(!host.VisitMapItem(&hero, &object, 0, TRUE));

    // The WoG wrapper skips the generic RMG visit hook; its own hook must dispatch the legacy alias once.
    std::array<size_t, 6> frame{};
    static_assert(sizeof(size_t) == 4, "The native hook test requires x86");
    HookContext visitContext;
    visitContext.ebp = reinterpret_cast<size_t>(frame.data());
    frame[2] = reinterpret_cast<size_t>(&hero);
    frame[3] = reinterpret_cast<size_t>(&object);
    frame[4] = 12345;
    frame[5] = TRUE;
    fake::Reset(hero); object = {63, 51}; hero.secSkill[5] = 1; fake::choices = {0};
    assert(fakePatcher.hooks.at(0x705B18)(nullptr, &visitContext) == NO_EXEC_DEFAULT);
    assert(visitContext.eax == 1 && visitContext.return_address == 0x705B1D);
    assert(fake::receivers == 0 && hero.movement == 500 && fake::main.players[3].playerResources.gold == 1000);
    for (int subtype : {0, 10, 53})
    {
        fake::Reset(hero); object = {63, subtype}; visitContext.eax = visitContext.return_address = 0;
        assert(fakePatcher.hooks.at(0x705B18)(nullptr, &visitContext) == EXEC_DEFAULT);
        assert(visitContext.eax == 0 && visitContext.return_address == 0 && fake::receivers == 0);
    }
    fake::Reset(hero); object = {141, 51};
    assert(fakePatcher.hooks.at(0x705B18)(nullptr, &visitContext) == EXEC_DEFAULT);
    fake::Reset(hero); object = {63, 51}; fake::option = 0;
    wog::policy::InitializeDefaults(); Era::SetAssocVarIntValue(wog::policy::Key(63,51).c_str(),0);
    assert(fakePatcher.hooks.at(0x705B18)(nullptr, &visitContext) == EXEC_DEFAULT);
    fake::Reset(hero);
    assert(fake::messages.empty() && fake::receivers == 0);

    // Legacy hint hooks replace only the migrated Pyramid subtype, never native Pyramid 63/0.
    HookContext context;
    for (size_t address : {0x40C5A1u, 0x414F66u})
    {
        object = {63, 51}; context.ebx = reinterpret_cast<size_t>(&object); context.return_address = 0;
        assert(fakePatcher.hooks.at(address)(nullptr, &context) == NO_EXEC_DEFAULT);
        assert(context.return_address == (address == 0x40C5A1u ? 0x40D09Bu : 0x4159C0u));
        object.objectSubtype = 0; context.return_address = 0;
        assert(fakePatcher.hooks.at(address)(nullptr, &context) == EXEC_DEFAULT && context.return_address == 0);
        object.objectSubtype = 53;
        assert(fakePatcher.hooks.at(address)(nullptr, &context) == EXEC_DEFAULT);
    }

    maxSubtypes[63] = 50; maxSubtypes[141] = 0;
    host.AfterLoadingObjectsTxtProc(maxSubtypes.data());
    assert(host.GetObjectSubtypesInfo().size() == 89);
    assert(host.GetObjectSubtypesInfo().back().type == 141 && host.GetObjectSubtypesInfo().back().subtype == 66);
    assert(host.GetObjectSubtypesInfo()[50].type == 63 && host.GetObjectSubtypesInfo()[50].subtype == 51);

    fake::Reset(hero);
    hero.secSkill[0] = 1; hero.movement = 499;
    Visit(&hero, TRUE);
    AssertUnchanged(hero, 3000, 499);
    assert(fake::messages.back().find("noMoves") != std::string::npos);
    assert(fake::pages.empty());

    fake::Reset(hero);
    Visit(&hero, TRUE);
    AssertUnchanged(hero);
    assert(fake::messages.back().find("noSkills") != std::string::npos);

    fake::Reset(hero);
    hero.secSkill[5] = 3; fake::choices = {0}; fake::confirm = false;
    Visit(&hero, TRUE);
    AssertUnchanged(hero);
    assert(hero.secSkill[5] == 3 && fake::questions == 1);
    assert(fake::pages[0][0].skill == 5 && fake::pages[0][0].level == 3);

    fake::Reset(hero);
    hero.secSkill[5] = 1; fake::choices = {0};
    fake::main.players[hero.owner].playerResources.gold = 1999;
    Visit(&hero, TRUE);
    AssertUnchanged(hero, 1999);
    assert(fake::messages.back().find("noMoney") != std::string::npos);
    assert(hero.secSkill[5] == 1);

    fake::Reset(hero);
    hero.secSkill[27] = 2; hero.movement = 500; fake::option = 3;
    fake::choices = {0}; fake::main.players[hero.owner].playerResources.gold = 2000;
    Visit(&hero, TRUE);
    assert(hero.secSkill[27] == 0 && hero.movement == 0 && hero.recalculations == 1);
    assert(fake::main.players[hero.owner].playerResources.gold == 0);
    assert(fake::main.players[0].playerResources.gold == 3000);
    assert(fake::receivers == 0 && hero.secSkill[27] == 0);

    fake::Reset(hero);
    hero.secSkill.fill(1); fake::choices = {-1, -1, -1, 3};
    Visit(&hero, TRUE);
    assert(fake::pages.size() == 4 && fake::pages.back().size() == 4);
    assert(hero.secSkill[27] == 0 && hero.secSkill[26] == 1);
    assert(hero.movement == 500 && fake::main.players[3].playerResources.gold == 1000);

    fake::Reset(hero);
    hero.secSkill.fill(1); fake::choices = {-1, -1, -1, -1};
    Visit(&hero, TRUE);
    AssertUnchanged(hero);
    assert(fake::pages.size() == 4 && fake::questions == 0);

    for (int representative : {10, 20, 27, 0})
    {
        fake::Reset(hero);
        fake::warfare = 1;
        hero.secSkillPosition[representative ? representative : 20] = 1; hero.secSkillCount = 1;
        hero.secSkill[10] = hero.secSkill[20] = hero.secSkill[27] = 3;
        fake::choices = {0};
        Visit(&hero, TRUE);
        assert(fake::pages[0].size() == 1);
        const int expected = representative ? representative : 20;
        assert(fake::pages[0][0].skill == expected);
        assert(hero.secSkill[10] == 0 && hero.secSkill[20] == 0 && hero.secSkill[27] == 0);
        assert(hero.secSkillCount == 0);
    }

    fake::Reset(hero);
    hero.secSkill[10] = hero.secSkill[20] = hero.secSkill[27] = 1;
    fake::choices = {2};
    Visit(&hero, TRUE);
    assert(fake::pages[0].size() == 3 && hero.secSkill[27] == 0);

    fake::Reset(hero);
    fake::modernDialogs = false; hero.secSkill.fill(1);
    fake::choices.assign(14, -1); fake::choices.back() = 1;
    Visit(&hero, TRUE);
    assert(fake::pages.size() == 14 && hero.secSkill[27] == 0);

    // Remove every possible visible position; hidden skills must not reduce the displayed count.
    for (int removed = 0; removed < 28; ++removed)
    {
        fake::Reset(hero); hero.secSkill.fill(1); hero.secSkillCount = 28;
        for (int skill = 0; skill < 28; ++skill) hero.secSkillPosition[skill] = static_cast<signed char>(skill + 1);
        assert(wog::native::ForgetSkill(hero, removed, false));
        assert(hero.secSkillCount == 27 && !hero.secSkillPosition[removed] && !hero.secSkill[removed]);
        for (int skill = 0; skill < 28; ++skill)
            if (skill != removed) assert(hero.secSkillPosition[skill] == skill + 1 - (skill > removed));
        assert(!wog::native::ForgetSkill(hero, removed, false));
    }
    fake::Reset(hero); hero.secSkill[10] = 3;
    assert(wog::native::ForgetSkill(hero, 10, true) && hero.secSkillCount == 0);
    assert(!wog::native::ForgetSkill(hero, -1, true) && !wog::native::ForgetSkill(hero, 28, true));

    fake::Reset(hero);
    hero.secSkill[0] = 1;
    const auto skills = CollectSkills(hero.secSkill, false, 0);
    assert(skills.SkillOnPage(0, 0) == 0);
    assert(skills.SkillOnPage(0, -1) == -1 && skills.SkillOnPage(0, 1) == -1);
    assert(skills.SkillOnPage(-1, 0) == -1 && skills.SkillOnPage(INT_MAX, 7) == -1);
    assert(SetHint(FALSE) && std::strstr(h3_TextBuffer, ".name"));
    assert(SetHint(TRUE) && std::strstr(h3_TextBuffer, "gold 2000") && std::strstr(h3_TextBuffer, "movement 500"));
    fake::Reset(hero);
    Era::handlers.at("OnBeforeErmInstructions")(nullptr);
    assert(wog::policy::Handles(63,51) && wog::policy::Handles(141,51));
    assert(!wog::policy::Handles(63,0) && !wog::policy::Handles(80,51));
    Era::SetAssocVarIntValue(wog::policy::Key(63,51).c_str(),0);
    const auto mapA=fake::assoc;
    fake::assoc.clear(); Era::handlers.at("OnBeforeErmInstructions")(nullptr);
    assert(wog::policy::Handles(63,51)); // Starting map B must not inherit map A's settings.
    fake::assoc=mapA; fake::assoc.erase(wog::policy::Key(63,51)); // ERA omits zero assoc values on load.
    Era::handlers.at("OnBeforeLoadGame")(nullptr); assert(wog::policy::LoadingGame());
    Era::handlers.at("OnAfterLoadGame")(nullptr);
    assert(!wog::policy::LoadingGame() && !wog::policy::Handles(63,51) && wog::policy::Handles(141,51));
    Era::SetAssocVarIntValue(wog::policy::Key(63,14).c_str(),0);
    assert(!wog::policy::Handles(63,14) && wog::policy::Handles(63,18));
    Era::SetAssocVarIntValue(wog::policy::Key(63,14).c_str(),1);
    assert(wog::policy::Handles(63,14)); // Live edits use the same source of truth as ERM guards.
    fake::assoc.clear(); Era::handlers.at("OnAfterLoadGame")(nullptr);
    assert(wog::policy::Handles(63,51)); // A save predating the policy gets the native defaults once.
    puts("PASS: Market rules, dual registration, map policy, zero-valued save/load, map isolation, delegation and RMG 141 only.");
}
