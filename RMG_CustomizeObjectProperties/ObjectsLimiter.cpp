#include "pch.h"
IGamePatch* ObjectsLimiter::instance = nullptr;

ObjectsLimiter::~ObjectsLimiter()
{
}

void ObjectsLimiter::Init()
{

	_PI->WriteLoHook(0x54C50C, RMG_OnBeforeMapGeneration);

	
	m_isInited = true;
}

IGamePatch* ObjectsLimiter::Get() noexcept
{
	if (instance)
	{
		return instance;
	}

	return 0;// new ObjectsLimiter();
}



_LHF_(ObjectsLimiter::RMG_OnBeforeMapGeneration)
{
	h3::H3RmgRandomMapGenerator* rmgGnr = (h3::H3RmgRandomMapGenerator*)c->ecx;

	if (rmgGnr)
	{
		int objNumber = sizeof(rmgGnr->objectCountByType) / sizeof(INT32);

		H3RmgZoneGenerator* zone;
		int* ObjectLimitPerMap = (int*)0x69CE9C;
		int* ObjectlimitPerZone = (int*)0x69D244;

		H3String keyBaseName = "RMG.object_limit.";
		for (UINT32 i = 0; i < objNumber; i++)
		{
		/*	int perMapFromJson = GetIntFromJson((keyBaseName + std::to_string(i).c_str() + ".map").String());
			if (perMapFromJson != -1)
				ObjectLimitPerMap[i] = perMapFromJson;
			else
			{
				if (perMapFromJson)
				{
					int perZoneFromJson = GetIntFromJson((keyBaseName + std::to_string(i).c_str() + ".zone").String());
					if (perZoneFromJson != -1)
						ObjectlimitPerZone[i] = perZoneFromJson;
				}
				else
					ObjectlimitPerZone[i] = 0;
			}*/
		}
	}

	return EXEC_DEFAULT;
}