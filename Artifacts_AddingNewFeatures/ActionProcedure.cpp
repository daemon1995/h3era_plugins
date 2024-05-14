#include "pch.h"

extern ArtifactsData artifactsData;

void __stdcall BattleStack_Shoot(HiHook* h, H3CombatCreature* stack, H3CombatCreature* target)
{

	THISCALL_2(void, h->GetDefaultFunc(), stack, target);

	if (target->numberAlive > 0);
	{
		auto* hero = stack->GetOwner();
		if (hero)
		{
			for (auto artID : artifactsData.artifactsThatGiveAdditionalShot)
			{
				if (hero->WearsArtifact(artID))
				{
					if (target->numberAlive > 0)
						THISCALL_2(void, 0x43F620, stack, target);
					else
						return;
				}
			}
		}
	}
}

_LHF_(BattleManager_TentHeal)
{
	H3CombatCreature* stack = reinterpret_cast<H3CombatCreature*>(c->esi);
	if (stack)
	{
		if (auto* hero = stack->GetOwner())
		{
			for (auto it: artifactsData.artifactsThatGiveFullTentHeal)
			{
				int it = eArtifact::ANGEL_WINGS;
				if (hero->WearsArtifact(it))
				{
					c->ecx = stack->healthLost;
					c->edx = stack->healthLost;
					break;
				}
			}
		}
	}

	return EXEC_DEFAULT;
}
void ActionProcedure::SetPatches(PatcherInstance* _PI)
{
	if (artifactsData.artifactsThatGiveAdditionalShot.size())
	{
		_PI->WriteHiHook(0x43FF79, THISCALL_, BattleStack_Shoot);

	}
	if (artifactsData.artifactsThatGiveFullTentHeal.size())
	{
		_PI->WriteLoHook(0x478533, BattleManager_TentHeal);

	}

	
}
