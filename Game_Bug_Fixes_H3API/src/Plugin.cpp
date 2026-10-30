#include "../include/Plugin.h"

namespace gbf {
Patcher* patcher = nullptr;
PatcherInstance* instance = nullptr;
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        Era::ConnectEra(module, "game bug fixes H3API");
        gbf::patcher = GetPatcher();
        if (!gbf::patcher)
            return TRUE;
        gbf::instance = gbf::patcher->CreateInstance("game bug fixes H3API");
        if (!gbf::instance)
            return TRUE;
        // All compiled modules below are installed. Runtime compatibility still
        // depends on the target ERA build and the other loaded hook owners.
        gbf::InstallBen(gbf::instance);
        gbf::InstallScouting(gbf::instance);
        gbf::InstallHeroMovement(gbf::instance);
        gbf::InstallRuntimePatches(gbf::instance);
        gbf::InstallAI(gbf::instance);
        gbf::InstallGameLogicCore(gbf::instance);
        gbf::game_logic_remaining::Install(gbf::instance);
        gbf::InstallPrimarySkills(gbf::instance);
        gbf::InstallCrExpo(gbf::instance);
        gbf::InstallSpells(gbf::instance);
        gbf::InstallSummon(gbf::instance);
        gbf::InstallLuck(gbf::instance);
        gbf::InstallNpc(gbf::instance);
        gbf::InstallAudio(gbf::instance);
        gbf::InstallMonsters(gbf::instance);
        gbf::InstallGraphics(gbf::instance);



    }
    return TRUE;
}
