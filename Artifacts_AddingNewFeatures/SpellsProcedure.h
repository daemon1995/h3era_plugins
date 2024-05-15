#pragma once
using namespace h3;
class SpellsProcedure
{

public:
	static int autoCastSpellArtID;
	static bool ignoreAutoCastSelfDamage;

	static bool spellSpecialityAlreadyAffected;

public:

	static void SetPatches(PatcherInstance* _PI);

};
