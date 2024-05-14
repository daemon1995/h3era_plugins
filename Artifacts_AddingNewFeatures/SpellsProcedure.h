#pragma once
using namespace h3;
class SpellsProcedure
{

public:
	static int autoCastSpellArtID;
	static bool ignoreAutoCastSelfDamage;

public:

	static void SetPatches(PatcherInstance* _PI);

};
