#include "pch.h"
namespace shrine

{


	ShrineManager::ShrineManager()
		:ObjectsExtender(globalPatcher->CreateInstance("EraPlugin.ShrineManager.daemon_n"))
	{

		CreatePatches();
	}


	ShrineManager::~ShrineManager()
	{
	}


	_LHF_(ShrineManager::Game__AtShrineOfMagicIncantationSettingSpell)
	{

		// set spell level generated coresponding to map item subtype (allows any level spells);
		if (const H3MapItem* shrine = reinterpret_cast<H3MapItem*>(c->esi))
		{
			int spellLevel = shrine->objectSubtype;
			spellLevel = Clamp(0, spellLevel, 4); //restrict spell levels 1-5 (0_4)
			ByteAt(0x4C1995 + 1) = spellLevel;

		}
		return EXEC_DEFAULT;
	}


	_LHF_(ShrineManager::Shrine__AtGetName)
	{
		if (const H3MapItem* shrine = reinterpret_cast<H3MapItem*>(c->edx))
		{
			if (shrine->objectSubtype != 0)
			{
				bool readSucces = false;
				int strPtr = int(EraJS::read(H3String::Format("RMG.objectGeneration.88.%d.name", shrine->objectSubtype).String(), readSucces));

				if (readSucces)
				{
					c->edi = strPtr;
					// return after original text set
					c->return_address = 0x40D85F;
					return NO_EXEC_DEFAULT;
				}
			}
		}

		return EXEC_DEFAULT;
	}

	void ShrineManager::CreatePatches()
	{
		if (!m_isInited)
		{

			_pi->WriteLoHook(0x4C1974, Game__AtShrineOfMagicIncantationSettingSpell);
			_pi->WriteLoHook(0x40D858, Shrine__AtGetName);

			m_isInited = true;
		}
	}

	void ShrineManager::AferLoadingObjectTxtProc(const INT16* maxSubtypes)
	{
		const int maxShrineSubtype = maxSubtypes[eObject::SHRINE_OF_MAGIC_INCANTATION];

		return;

		for (INT16 i = 0; i < maxShrineSubtype; i++)
		{
			SetRmgObjectGenData(i);
		}
		const int maxPyramiSubtype = maxSubtypes[eObject::PYRAMID];
		for (INT16 i = 0; i < maxPyramiSubtype; i++)
		{
			const int value = EraJS::readInt(H3String::Format("RMG.objectGeneration.63.%d.value", i).String());
			const int density = EraJS::readInt(H3String::Format("RMG.objectGeneration.63.%d.density", i).String());
			if (value && density)
			{
				RMGObjectInfo rmgObjectInfo(eObject::PYRAMID, i);
				rmgObjectInfo.value = value;
				rmgObjectInfo.density = density;
				additionalRmgObjects.emplace_back(rmgObjectInfo);
			}
		}

		const int maxWareHouseSubtype = maxSubtypes[142];
		for (INT16 i = 0; i < maxWareHouseSubtype; i++)
		{
			const int value = EraJS::readInt(H3String::Format("RMG.objectGeneration.142.%d.value", i).String());
			const int density = EraJS::readInt(H3String::Format("RMG.objectGeneration.142.%d.density", i).String());
			if (value && density)
			{
				RMGObjectInfo rmgObjectInfo(142, i);
				rmgObjectInfo.value = value;
				rmgObjectInfo.density = density;
				additionalRmgObjects.emplace_back(rmgObjectInfo);
			}
		}
	}

	void ShrineManager::SetRmgObjectGenData(const int objectSubtype)  noexcept
	{

		const int value = EraJS::readInt(H3String::Format("RMG.objectGeneration.88.%d.value", objectSubtype).String());
		const int density = EraJS::readInt(H3String::Format("RMG.objectGeneration.88.%d.density", objectSubtype).String());
		if (value && density)
		{
			RMGObjectInfo rmgObjectInfo(eObject::SHRINE_OF_MAGIC_INCANTATION, objectSubtype);
			rmgObjectInfo.value = value;
			rmgObjectInfo.density = density;
			additionalRmgObjects.emplace_back(rmgObjectInfo);
		}
		const int _value = EraJS::readInt(H3String::Format("RMG.objectGeneration.142.%d.value", objectSubtype).String());
		const int _density = EraJS::readInt(H3String::Format("RMG.objectGeneration.142.%d.density", objectSubtype).String());
		if (_value && _density)
		{
			RMGObjectInfo rmgObjectInfo(142, objectSubtype);
			rmgObjectInfo.value = _value;
			rmgObjectInfo.density = _density;
			additionalRmgObjects.emplace_back(rmgObjectInfo);
		}

	}



	ShrineManager& ShrineManager::Get()
	{
		// TODO: insert return statement here

		static ShrineManager _instance;
		return _instance;

	}

}
