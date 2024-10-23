#pragma once

namespace WoG
{
	class NPC
	{
	public:
		int on;
		int alive;
		int id;
		int type;
		int type_Hero;
		int LastExpaInBattle;
		DWORD CustomPrimary;

		union 
		{

			struct
			{
				int attack;
				int defence;
				int hit_points;
				int damage;
				int spell_power;
				int speed;
				int resistance;
			};

			INT32 asArray[7];

		} parameters;



		union
		{

			struct 
			{
				int lvl_attack;
				int lvl_defence;
				int lvl_hit_points;
				int lvl_damage;
				int lvl_spell_power;
				int lvl_speed;
				int lvl_resistance;
			};
			INT32 asArray[7];
		} skills;

		WORD arts[10][8];
		char name[32];
		int old_expa;
		int now_expa;
		int now_level;
		union 
		{
			struct
			{
				unsigned defence_50_decrease : 1;
				unsigned fear : 1;
				unsigned max_damage : 1;
				unsigned no_retaliation: 1;
				unsigned shooting : 1;
				unsigned always_retaliation: 1;
				unsigned strike_all_around : 1;
				unsigned fireshield: 1;
				unsigned block_30_damage : 1;
				unsigned double_strike : 1;
				unsigned paralize_strike : 1;
				unsigned regeneration : 1;
				unsigned death_stare : 1;
				unsigned chavalary_bonus : 1;
				unsigned fly : 1;
			};

			DWORD bits;

		} abilities, bannedAbilities;
		//DWORD abilitiesBits;
		//DWORD bannedAbilitiesBits;

	public:
		static NPC* Get(int hero_id) { return  hero_id > -1 && hero_id < h3::limits::HEROES ? reinterpret_cast<NPC*>(0x28620C0 + 296 * hero_id) : nullptr; }

	};

}

