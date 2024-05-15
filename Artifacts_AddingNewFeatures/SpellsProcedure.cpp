#include "pch.h"

extern PluginText pluginText;
extern ArtifactsData artifactsData;

int SpellsProcedure::autoCastSpellArtID = eArtifact::NONE;
bool SpellsProcedure::ignoreAutoCastSelfDamage = false;
bool SpellsProcedure::spellSpecialityAlreadyAffected = false;

bool SpellBannedByArtifact(const int spellId, H3String *msg = nullptr)
{
	bool artIsFound = false;

	if (artifactsData.artifactsWhichBanSpell.count(spellId) > 0)
	{
		for (size_t i = 0; i < 2; ++i)
		{
			if (auto *hero = P_CombatManager->hero[i])
			{
				auto range = artifactsData.artifactsWhichBanSpell.equal_range(spellId);
				for (auto &it = range.first; it != range.second; it++)
				{
					if (hero->WearsArtifact(it->second))
					{
						artIsFound = true;
						if (msg)
							*msg = H3String::Format(pluginText.buffer.spellIsForbiddenBy, P_Artifacts[it->second].name, hero->name, P_Spell[spellId].name);
						break;
					}
				}
			}
		}
	}

	return artIsFound;
}

bool SpellLevelBannedByArtifact(const int spellLevel, H3String *msg = nullptr)
{

	bool artIsFound = false;

	if (!artifactsData.artifactsWhichBanSpellLevel[spellLevel].empty())
	{
		for (size_t i = 0; i < 2; ++i)
		{
			if (auto *hero = P_CombatManager->hero[i])
			{
				for (auto it : artifactsData.artifactsWhichBanSpellLevel[spellLevel]) // range.first; it != range.second; it++)
				{
					if (hero->WearsArtifact(it))
					{
						artIsFound = true;
						if (msg)
							*msg = H3String::Format(pluginText.buffer.spellLevelForbiddenBy, P_Artifacts[it].name, hero->name, spellLevel);
						break;
					}
				}
			}
		}
	}

	return artIsFound;
}

double Hero__GetResurrectionPowerMultiplier(H3Hero *hero)
{

	float resurrectionPowerBonus = 1.f;
	for (auto &it : artifactsData.artifactsWhichScaleResurrection)
	{
		if (hero->WearsArtifact(it.first))
		{
			resurrectionPowerBonus *= it.second;
		}
	}

	return resurrectionPowerBonus;
}


double __stdcall BattleMgr__BattleStack__GetMagicResistance(HiHook *h, H3CombatManager *cmbMgr,
															int spellId,
															int side,
															H3CombatCreature *stack,
															char a5,
															char a6,
															int noHero)
{
	// call native function
	double result = THISCALL_7(double, h->GetDefaultFunc(), cmbMgr, spellId, side, stack, a5, a6, noHero);


	if (result)
	{
		// add self damage spell immunity
		if (SpellsProcedure::ignoreAutoCastSelfDamage && stack->side == side)
			result = 0.f;
		// check spells
		else if (SpellBannedByArtifact(spellId) || SpellLevelBannedByArtifact(P_Spell[spellId].level))
			result = 0.f;
	}

	return result;
}

int __stdcall Hero__GetSpellCost(HiHook *h, H3Hero *hero, const int spell, const H3Army *army, const int ground)
{

	int spellCost = THISCALL_4(int, h->GetDefaultFunc(), hero, spell, army, ground);
	if (spellCost > 0 && artifactsData.artifactsWhichMakeThatSpellFree.count(spell))
	{
		auto range = artifactsData.artifactsWhichMakeThatSpellFree.equal_range(spell);
		for (auto &it = range.first; it != range.second; ++it)
		{
			if (hero->WearsArtifact(it->second))
			{
				spellCost = NULL;
				break;
			}
		}
	}

	return spellCost;
}

_LHF_(SpellBookDlg__AfterClose)
{

	int retrunType = EXEC_DEFAULT;

	int spellId = P_WindowManager->resultItemID;
	bool artIsFound = false;
	int spellLevel = -1;
	H3String msg = h3_NullString;

	artIsFound = SpellLevelBannedByArtifact(P_Spell[spellId].level, &msg);
	if (!artIsFound)
		artIsFound = SpellBannedByArtifact(spellId, &msg);

	if (artIsFound)
	{
		H3Messagebox(msg.String());
		c->return_address = 0x59EF42;
		retrunType = NO_EXEC_DEFAULT;
	}

	return retrunType;
}

void __stdcall BattleMgr__CastResurrection(HiHook *h, H3CombatManager *cmbMgr, int pos, int power, DWORD a4)
{
	// 005A1C6F
	THISCALL_4(void, h->GetDefaultFunc(), cmbMgr, pos, power , a4);
}

void __stdcall BattleMgr__UseCureSpell(HiHook *h, H3CombatManager *cmbMgr, int SchoolLevel, int SpellPower, H3Hero *Hero)
{

	int storeValue = P_Spell[eSpell::CURE].baseValue[SchoolLevel];

	H3Hero *_hero = Hero ? Hero : cmbMgr->hero[cmbMgr->currentActiveSide];

	if (_hero)
	{

		P_Spell[eSpell::CURE].baseValue[SchoolLevel] <<= 1;
		SpellPower <<= 1;
	}

	THISCALL_4(void, h->GetDefaultFunc(), cmbMgr, SchoolLevel, SpellPower, Hero);

	P_Spell[eSpell::CURE].baseValue[SchoolLevel] = storeValue;
}

int __stdcall Hero__GetSchoolLevelOfSpell(HiHook *h, H3Hero *hero, const eSpell spell, const int ground)
{

	int schoolLevel = THISCALL_3(int, h->GetDefaultFunc(), hero, spell, ground);
	const UINT MAX_LEVEL = artifactsData.MAX_SKILL_LEVEL;

	if (schoolLevel < MAX_LEVEL)
	{
		const UINT16 spell16 = spell;
		for (size_t i = MAX_LEVEL; i > schoolLevel; --i)
		{

			UINT schoolSpellBitSet = (static_cast<UINT>(i) << 16) | spell16;
			if (artifactsData.artifactsBySpellSchoolBitSet.count(schoolSpellBitSet) > 0)
			{
				auto range = artifactsData.artifactsBySpellSchoolBitSet.equal_range(schoolSpellBitSet);
				for (auto &it = range.first; it != range.second; ++it)
				{
					if (hero->WearsArtifact(it->second))
					{
						schoolLevel = i;
						break;
					}
				}
			}
		}
	}

	return schoolLevel;
}

_LHF_(Art__GetSpellsList)
{

	bool returnType = EXEC_DEFAULT;
	UINT artId = c->esi;
	if (artId && artifactsData.spellsAddedByArtifactsId.count(artId) > 0)
	{
		const H3SpellsBitset *s = reinterpret_cast<H3SpellsBitset *>(c->ebp - 0x1C);

		auto range = artifactsData.spellsAddedByArtifactsId.equal_range(artId);
		for (auto &it = range.first; it != range.second; ++it)
		{
			int spell = it->second;
			THISCALL_3(void, 0x4E67A0, s, it->second, 1);
		}

		returnType = NO_EXEC_DEFAULT;
		c->return_address = 0x4D979D;
	}

	return returnType;
}

_LHF_(BattleMgr__BeforeArtifactAutoCast)
{

	H3Hero *hero = reinterpret_cast<H3Hero *>(c->edi);
	if (hero && !artifactsData.autoCastedSpellsByArtifactId.empty())
	{
		int side = c->eax;
		auto &container = artifactsData.autoCastedSpellsByArtifactId;
		bool addCasterSideImmunity = false;
		for (auto it = container.begin(); it != container.end(); ++it)
		{
			int artID = it->first;
			if (hero->WearsArtifact(artID))
			{
				auto &spell = it->second;
				addCasterSideImmunity = spell.affectOnlyEnemy;

				int spellId = spell.spellID;
				if (THISCALL_6(char, 0x5A43E0, P_CombatManager->Get(), spellId, spell.skillLevel, side, 1, 2))
				{

					// if (P_Spell[spellId].SingleTarget())

					SpellsProcedure::autoCastSpellArtID = artID;
					SpellsProcedure::ignoreAutoCastSelfDamage = spell.affectOnlyEnemy;

					P_CombatManager->CastSpell(spell.spellID, -1, 2, -1, spell.skillLevel, spell.power < 0 ? P_CombatManager->heroSpellPower[side] : spell.power);

					SpellsProcedure::ignoreAutoCastSelfDamage = false;
					SpellsProcedure::autoCastSpellArtID = eArtifact::NONE;
				}
			}
		}
	}

	return EXEC_DEFAULT;
}

_LHF_(BattleMgr__AfterArtifactAutoCast__BeforeReport)
{
	if (SpellsProcedure::autoCastSpellArtID != eArtifact::NONE)
		c->eax = SpellsProcedure::autoCastSpellArtID << 5;

	return EXEC_DEFAULT;
}


float Hero__GetAdvancedSpellMultiplierBonus(H3Hero* hero, const eSpell spell)
{
	float bonus = 1.f;

	std::map < UINT, double>* bonusValuesContainer = nullptr;

	switch (spell)
	{

	case eSpell::CURE:
		bonusValuesContainer = &artifactsData.artifactsWhichScaleCure;
		break;

	case eSpell::RESURRECTION:
	case eSpell::ANIMATE_DEAD:
		bonusValuesContainer = &artifactsData.artifactsWhichScaleResurrection;
		break;

	default:
		if (P_Spell[spell].damageSpell)
			bonusValuesContainer = &artifactsData.artifactsWhichScaleDamage;

		break;
	}

	// if spells has art that affect that container
	if (bonusValuesContainer)
	{

		for (auto& it : *bonusValuesContainer)
		{
			if (hero->WearsArtifact(it.first))
			{
				bonus *= it.second;
			}
		}
	}

	return bonus;
}




signed __int64 __stdcall Hero__GetSorceryEffect(HiHook *h, H3Hero *hero, const eSpell spell, const int damage, const H3CombatCreature *stack)
{



	SpellsProcedure::spellSpecialityAlreadyAffected = true;

	signed __int64 result = THISCALL_4(signed __int64, h->GetDefaultFunc(), hero, spell, damage, stack);

	SpellsProcedure::spellSpecialityAlreadyAffected = false;

	for (auto &it : artifactsData.artifactsWhichScaleDamage)
	{
		if (hero->WearsArtifact(it.first))
		{
			result *= it.second;
		}
	}
	return result;
}




int __stdcall Hero__GetSpellSpecialityEffect(HiHook *h, H3Hero *hero, const eSpell spell, const int monLevel, int effect)
{

	int additionalOriginalEffect = 0;

	if (!SpellsProcedure::spellSpecialityAlreadyAffected)
	{
		float bonus = 1.f;
		bonus = Hero__GetAdvancedSpellMultiplierBonus(hero, spell);
		if (bonus !=1.f)
		{
			additionalOriginalEffect = effect * (bonus - 1.f);
			effect += additionalOriginalEffect;
		}
	}



	int result = THISCALL_4(signed __int64, h->GetDefaultFunc(), hero, spell, monLevel, effect);

	return result + additionalOriginalEffect;
}


void SpellsProcedure::SetPatches(PatcherInstance *_PI)
{
	if (!artifactsData.autoCastedSpellsByArtifactId.empty())
	{
		_PI->WriteLoHook(0x464FBA, BattleMgr__BeforeArtifactAutoCast);
		_PI->WriteLoHook(0x5A8E8C, BattleMgr__AfterArtifactAutoCast__BeforeReport);
	}

	bool dontSetSpellBansHooks = artifactsData.artifactsWhichBanSpell.empty();
	if (dontSetSpellBansHooks)
		for (size_t i = 0; i < 6 && dontSetSpellBansHooks; i++)
			dontSetSpellBansHooks = artifactsData.artifactsWhichBanSpellLevel[i].empty();


	if (!dontSetSpellBansHooks)
	{
		_PI->WriteHiHook(0x5A83A0, THISCALL_, BattleMgr__BattleStack__GetMagicResistance);
		_PI->WriteLoHook(0x59EE0B, SpellBookDlg__AfterClose);

	}




	if (!artifactsData.artifactsWhichScaleDamage.empty()
		|| !artifactsData.artifactsWhichScaleResurrection.empty()
		|| !artifactsData.artifactsWhichScaleCure.empty())
	{
		_PI->WriteHiHook(0x4E59D0, THISCALL_, Hero__GetSorceryEffect);
		_PI->WriteHiHook(0x4E6260, THISCALL_, Hero__GetSpellSpecialityEffect);

		// if there is hook from Game Bug Fixes then place another hook to adjust resurrection over it
		if (globalPatcher->GetLastHiHookAt(0x5A1C7B))
			_PI->WriteHiHook(0x5A1C7B, THISCALL_, Hero__GetSpellSpecialityEffect);
		
	}



	if (!artifactsData.artifactsBySpellSchoolBitSet.empty())
	{
		_PI->WriteHiHook(0x4E52F0, THISCALL_, Hero__GetSchoolLevelOfSpell);
		_PI->WriteLoHook(0x4D95DB, Art__GetSpellsList);

		// if we have any arts that make cost free
		if (!artifactsData.artifactsWhichMakeThatSpellFree.empty())
			_PI->WriteHiHook(0x4E54B0, THISCALL_, Hero__GetSpellCost);
	}
}
