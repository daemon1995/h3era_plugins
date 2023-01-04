#pragma once
#include "MenuItem.cpp"

namespace em
{
	//class EModPageCategory;
	class EOption :
		public MenuItem
	{
		int option_id;

	public:
		int GetId();

		EOption& GetById();
		void GetKey() override;
		void SetupText()override;

		
	};
}

enum eFlags : INT32
{
	HEROES = 0,
	TOWNS = 1,
	CREATURES = 2,
	SPELLS = 4,
	COMBAT = 8,
	MAP = 16,
	RESOURCE = 32,
	PLAYER = 64,
	SKILL = 128,
	ARTIFACT = 256,
	CREATURE_BANK = 512,
	INCOME = 1024,
	NPC = 2048,
};