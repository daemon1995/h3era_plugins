#pragma once
#include "framework.h"
class CombatEmulator
{

	static CombatEmulator* instance;











  public:
    static void CreatePatches(PatcherInstance *_pi);
	static void Init();
};
