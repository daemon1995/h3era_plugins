#include "..\__include\era.h"
#include "emerald.h"
//extern GAMEDATA2 no_save;
extern GAMEDATA save;
/*
extern int enchanted_artifacts[];
extern int enchanted_artifacts_count;
*/
extern char can_be_duplicate[NEW_ARTS_AMOUNT];

void ParseArray(char* buf, char* name, int* cortage)
{

}


void ParseFloat(char* buf, const char* name, float* result)
{
	char* c = strstr(buf, name);
	if (c != NULL)
		*result = atof(c + strlen(name));
}

void ParseInt(char* buf, const char* name, int* result)
{ 
	char *c = strstr(buf,name);
	if(c!=NULL)
		*result = atoi(c+strlen(name));
}

void ParseByte(char* buf, const char* name, char* result)
{ 
	char *c = strstr(buf,name);
	if(c!=NULL)
		*result = (char)atoi(c+strlen(name));
}


bool ParseStr(char* buf, const char* name, /*char* &target */ char res[char_table_size])
{
	// char res[char_table_size];
	char *c = strstr(buf,name);
	int l = strlen(name);
	if (c != NULL)
	{
		char* cend = strstr(c + l, "\"");
		if (cend != NULL) //мало ли, какие идиоты встречаются
		{
			//*res = (char*)malloc(cend-c+1);

			//memset(*res,0,cend-c+1);
			memset(res, 0, cend - c + 1);

			for (int i = 0, j = 0; j < (cend - c) - l; i++, j++)
			{
				if (c[l + j] != '\\')
				{
					//(*res)[i] = c[l+j];
					res[i] = c[l + j];
				}
				else
				{
					if (c[l + j + 1] == 'r')
					{
						//(*res)[i] = '\r';
						res[i] = '\r';

						j++;
						continue;
					}
					else if (c[l + j + 1] == 'n')
					{
						//(*res)[i] = '\n';
						res[i] = '\n';
						j++;
						continue;
					}
					else
					{
						//(*res)[i] = c[l+j];
						res[i] = c[l + j];

					}
				}
			}

			//strncpy(*res,c+l,cend-1-c);

			//(*res)[cend-c-l] = 0;

			res[cend - c - l] = 0;

			//res[cend - c - l] = 13; //end of WoG erm
			//res[cend - c - l +1] = 0;

			//sprintf(*res,*res);

			/*
			if (strcmp(target, res) != 0 ) {
				strcpy(target, res);
			}
			*/
		}
		return true;
	}
	else return false;
}
void ParseStr2(char* buf, const char* name, char*& target_1, char*& target_2
	/* char res[char_table_size]*/, char* target_static, int& target_int ) {
	
	char res[char_table_size];
	if (ParseStr(buf, name, res)) {
		target_1 = target_2 = target_static;
		strcpy(target_static, res);
		target_int = 0;
	}

	/*
	if (target && target!= target2 && !*target)
		strcpy(target2, target);
	ParseStr(buf, name, target2);
	target = target2;
	target3 = 0;
	*/

	/*
	char res[char_table_size];
	ParseStr(buf, name, res);
	if (!target || strcmp(target, res) != 0) {
		//char* tmp = new char[char_table_size];
		//target = tmp;
		target = target2;
		strcpy(target, res);
	}
	*/
}


int ParseTuple(char* buf, const char* name, /* int** tuple */ int tuple [int_tuple_size])
{
	  int len=0;
	  //char *tmp = (char*)malloc(strlen(buf));
	  static char tmp[32768];
	  char *c = strstr(buf,name);
	  if(c!= NULL) 
	  {
		  strncpy(tmp,c+strlen(name),int_tuple_size);
		  c = strpbrk(tmp,"\r\n\0}");
		  if(c!= NULL) 
		  {
			  tmp[c-tmp]=0;

			  char *p = strtok(tmp, ",");
			  
			  do 
			  {
				if(p) 
				{
					len++;
					// *tuple = (int*)realloc(*tuple,len*sizeof(int));
					// (*tuple)[len-1]=atoi(p);
					tuple[len - 1] = atoi(p);
				}
				p = strtok(NULL, ", ");
			  } while(p);
		  }
	  }
	//free(tmp);
	return len;
}

extern char art_does_not_exist[2000];
int enchanted_artifacts[] =
{ 0x01,0x80,0x7B,0x7C,
	0x56,0x57,0x58,0x59,
	0x87 };	//список артефактов, дающих заклинания. 
			//Для добавления - дописываем свой арт в конец списка 
			//и задаем ему заклинание через artspelltable
extern "C" __declspec(dllexport) void ConfigureArt(int artifact, char* config) {

	char* buf = config; int target = artifact;
	int tuple[int_tuple_size]; int len = 0;
	art_does_not_exist[target] = 0;

	ParseByte(buf, "Allowed=", &save.allowed_artifacts[target]);
	ParseByte(buf, "Used=", &save.used_artifacts[target]);
	ParseByte(buf, "can_be_duplicate=", &can_be_duplicate[target]);

	ParseInt(buf, "Cost=", &(EmeraldArtNewTable[target].cost));
	ParseInt(buf, "Rank=", &(EmeraldArtNewTable[target].rank));
	ParseInt(buf, "SlotID=", &(EmeraldArtNewTable[target].slot));
	ParseInt(buf, "ComboID=", &(EmeraldArtNewTable[target].combonum));
	ParseInt(buf, "ComboPart=", &(EmeraldArtNewTable[target].partof));

	ParseByte(buf, "Attack=", &(EmeraldArtNewBTable[target].att));
	ParseByte(buf, "Defence=", &(EmeraldArtNewBTable[target].def));
	ParseByte(buf, "Knowledge=", &(EmeraldArtNewBTable[target].knw));
	ParseByte(buf, "Spellpower=", &(EmeraldArtNewBTable[target].spp));

	ParseInt(buf, "Luck=", &no_save.luck_bonuses[target]);
	ParseInt(buf, "Morale=", &no_save.morale_bonuses[target]);
	ParseInt(buf, "LuckBP=", &no_save.luck_bonuses_bp[target]);
	ParseInt(buf, "MoraleBP=", &no_save.morale_bonuses_bp[target]);

	//// Not Working
	ParseInt(buf, "LuckEnemy=", &no_save.enemy_luck_bonuses[target]);
	ParseInt(buf, "MoraleEnemy=", &no_save.enemy_morale_bonuses[target]);

	ParseInt(buf, "Fly=", &no_save.allow_fly[target]);
	ParseInt(buf, "Waterwalk=", &no_save.allow_water[target]);

	//// spellID here
	ParseInt(buf, "Autospell=",  &no_save.autocast[target][0]);
	ParseInt(buf, "Autospell2=", &no_save.autocast[target][1]);
	ParseInt(buf, "Autospell3=", &no_save.autocast[target][2]);
	ParseInt(buf, "Autospell4=", &no_save.autocast[target][3]);
	ParseInt(buf, "Autospell5=", &no_save.autocast[target][4]);
	ParseInt(buf, "Autospell6=", &no_save.autocast[target][5]);
	ParseInt(buf, "Autospell7=", &no_save.autocast[target][6]);
	ParseInt(buf, "Autospell8=", &no_save.autocast[target][7]);
	ParseInt(buf, "Autospell9=", &no_save.autocast[target][8]);

	//// creatureID here
	ParseInt(buf, "Autosummon=",  &no_save.autosummon[target][0]);
	ParseInt(buf, "Autosummon2=", &no_save.autosummon[target][1]);
	ParseInt(buf, "Autosummon3=", &no_save.autosummon[target][2]);
	ParseInt(buf, "Autosummon4=", &no_save.autosummon[target][3]);
	ParseInt(buf, "Autosummon5=", &no_save.autosummon[target][4]);
	ParseInt(buf, "Autosummon6=", &no_save.autosummon[target][5]);
	ParseInt(buf, "Autosummon7=", &no_save.autosummon[target][6]);
	ParseInt(buf, "Autosummon8=", &no_save.autosummon[target][7]);
	ParseInt(buf, "Autosummon9=", &no_save.autosummon[target][8]);
	//// positive for constant, zero for hero spellpower, negative for promiles of spellpower
	ParseInt(buf, "AutosummonPower=",  &no_save.autosummon_power[target][0]);
	ParseInt(buf, "Autosummon2Power=", &no_save.autosummon_power[target][1]);
	ParseInt(buf, "Autosummon3Power=", &no_save.autosummon_power[target][2]);
	ParseInt(buf, "Autosummon4Power=", &no_save.autosummon_power[target][3]);
	ParseInt(buf, "Autosummon5Power=", &no_save.autosummon_power[target][4]);
	ParseInt(buf, "Autosummon6Power=", &no_save.autosummon_power[target][5]);
	ParseInt(buf, "Autosummon7Power=", &no_save.autosummon_power[target][6]);
	ParseInt(buf, "Autosummon8Power=", &no_save.autosummon_power[target][7]);
	ParseInt(buf, "Autosummon9Power=", &no_save.autosummon_power[target][8]);

	ParseInt(buf, "UpgradeFrom=", &no_save.upgrade_from[target][0]);
	ParseInt(buf, "UpgradeTo=", &no_save.upgrade_to[target][0]);
	ParseInt(buf, "UpgradeFrom2=", &no_save.upgrade_from[target][1]);
	ParseInt(buf, "UpgradeTo2=", &no_save.upgrade_to[target][1]);
	ParseInt(buf, "UpgradeFrom3=", &no_save.upgrade_from[target][2]);
	ParseInt(buf, "UpgradeTo3=", &no_save.upgrade_to[target][2]);
	ParseInt(buf, "UpgradeFrom4=", &no_save.upgrade_from[target][3]);
	ParseInt(buf, "UpgradeTo4=", &no_save.upgrade_to[target][3]);
	ParseInt(buf, "UpgradeFrom5=", &no_save.upgrade_from[target][4]);
	ParseInt(buf, "UpgradeTo5=", &no_save.upgrade_to[target][4]);
	ParseInt(buf, "UpgradeFrom6=", &no_save.upgrade_from[target][5]);
	ParseInt(buf, "UpgradeTo6=", &no_save.upgrade_to[target][5]);
	ParseInt(buf, "UpgradeFrom7=", &no_save.upgrade_from[target][6]);
	ParseInt(buf, "UpgradeTo7=", &no_save.upgrade_to[target][6]);
	ParseInt(buf, "UpgradeFrom8=", &no_save.upgrade_from[target][7]);
	ParseInt(buf, "UpgradeTo8=", &no_save.upgrade_to[target][7]);
	ParseInt(buf, "UpgradeFrom9=", &no_save.upgrade_from[target][8]);
	ParseInt(buf, "UpgradeTo9=", &no_save.upgrade_to[target][8]);

	ParseInt(buf, "RegenerateSpellpoints=", &no_save.regenerate_spellpoints[target]);

	//ParseStr(buf, "Name=\"", &(newtable[target].name));
	ParseStr2(buf, "Name=\"", (EmeraldArtNewTable[target].name), 
		no_save.ArtSetUpBack[artifact].name,
		no_save.artNameTable[artifact],no_save.ArtNames[artifact].NameVar);

	//ParseStr(buf, "Description=\"", &(newtable[target].desc));
	ParseStr2(buf, "Description=\"", (EmeraldArtNewTable[target].desc), 
		no_save.ArtSetUpBack[artifact].desc,
		no_save.artDescTable[artifact], no_save.ArtNames[artifact].DescVar);

	//ParseStr(buf, "MapDesc=\"", &(arteventtable[target+1]));
	ParseStr(buf, "MapDesc=\"", (no_save.arteventtable[target + 1]));

	ParseStr(buf, "OnFirstAction=\"", (no_save.erm_on_battlestart[target]));
	ParseStr(buf, "OnCreatureProp=\"", (no_save.erm_on_creature_param_init[target]));

	ParseStr(buf, "OnUnequip=\"", (no_save.erm_on_ae0[target]));
	ParseStr(buf, "OnEquip=\"", (no_save.erm_on_ae1[target]));
	ParseStr(buf, "OnBattleStart=\"", (no_save.erm_on_ba52[target]));
	ParseStr(buf, "OnBattleEnd=\"", (no_save.erm_on_ba53[target]));
	ParseStr(buf, "OnEveryDay=\"", (no_save.erm_on_timer[target]));


	ParseInt(buf, "AttackCR=", &no_save.crattack_bonuses[target]);
	ParseInt(buf, "DefenceCR=", &no_save.crdefence_bonuses[target]);

	ParseInt(buf, "DamageMin=", &no_save.dmgmin_bonuses[target]);
	ParseInt(buf, "DamageMax=", &no_save.dmgmax_bonuses[target]);

	ParseInt(buf, "Speed=", &no_save.speed_bonuses[target]);
	ParseInt(buf, "Health=", &no_save.hp_bonuses[target]);

	ParseInt(buf, "Shots=", &no_save.shots_bonuses[target]);
	ParseInt(buf, "Casts=", &no_save.casts_bonuses[target]);


	ParseFloat(buf, "Learning=", &no_save.art_learning_bonus[target]);

	len = ParseTuple(buf, "Immunities=", tuple);
	for (int i = 0; i != len; i++)
		no_save.spell_immunity[target][tuple[i]] = 1;

	int spell = -1;
	ParseInt(buf, "GivesSpell=", &spell);
	if (spell != -1)
	{
		spell ^= 0x80;
		//realloc(enchanted_artifacts,(enchanted_artifacts_count+1)*4);
		//realloc(no_save.enchanted_artifacts, (no_save.enchanted_artifacts_count + 1) * 4);
		//realloc(enchanted_artifacts, (no_save.enchanted_artifacts_count + 1) * 4);

		//no_save.enchanted_artifacts[no_save.enchanted_artifacts_count /* + 1 */] = target;
		enchanted_artifacts[no_save.enchanted_artifacts_count /* + 1 */] = target;


		no_save.enchanted_artifacts_count++;

		EmeraldArtNewTable[target].spellflag = 1;
		no_save.artspelltable[target] = spell;

		/*
		int spell = -1;
		ParseInt(buf, "GivesSpell2=", &spell);
		if (spell != -1)
		{
		}
		*/
	}

	int spell_set= -1;
	ParseInt(buf, "GivesSpellSet=", &spell_set);
	if (spell_set != -1 && spell == -1)
	{
		EmeraldArtNewTable[target].spellflag = 1;
		no_save.artspelltable[target] = spell_set;
	}

}

 void LoadArtifactConfig(int target)
{
		//char *buf, fname[256];
		static char buf[65535];
		static char fname[1024];

		//int* tuple = NULL;
		// int tuple[int_tuple_size];

		// int len = 0;

        FILE *fdesc;

		//sprintf(fname, "data\\artifacts\\%u.cfg",target);
		/* sprintf(fname, "Mods\\Amethyst Upgrades\\Data\\artifacts\\%u.cfg", target);
		if (!FileExists(fname)) */
		
		//sprintf(fname, "Mods\\Knightmare Kingdoms\\Data\\artifacts\\%u.cfg", target);
		//if (!FileExists(fname))
			sprintf(fname, "Data\\artifacts\\%u.cfg", target);

		/*
		if (!FileExists(fname))
			MessageBoxA(0, fname, "File not found: ", 0);
		*/

        if(fdesc=fopen(fname,"r"))
        {

            
			//----------
            fseek (fdesc , 0 , SEEK_END);
            int fdesc_size=ftell(fdesc);
            rewind(fdesc);
            //----------
            //buf=(char*)malloc(fdesc_size+1);
			memset(buf,0,fdesc_size+1);
            fread(buf,1,fdesc_size,fdesc);
            //buf[fdesc_size]=0;
            fclose(fdesc);

			ConfigureArt(target, buf);
			art_does_not_exist[target] = 0;


            //free(buf);
        }
		
}

 /*
void __stdcall blank(int first)
{
		memset(no_save.luck_bonuses + first * sizeof(int), 0, sizeof(no_save.luck_bonuses) - first * sizeof(int));
		memset(no_save.luck_bonuses_bp + first * sizeof(int), 0, sizeof(no_save.luck_bonuses_bp) - first * sizeof(int));
		memset(no_save.morale_bonuses + first * sizeof(int), 0, sizeof(no_save.morale_bonuses) - first * sizeof(int));
		memset(no_save.morale_bonuses_bp + first * sizeof(int), 0, sizeof(no_save.morale_bonuses_bp) - first * sizeof(int));
		memset(no_save.spell_immunity + first * sizeof(int), 0, sizeof(no_save.spell_immunity) - first * sizeof(int));
		memset(no_save.allow_fly + first * sizeof(int), 0, sizeof(no_save.allow_fly) - first * sizeof(int) );
		memset(no_save.allow_water + first * sizeof(int), 0, sizeof(no_save.allow_water) - first * sizeof(int) );
		memset(no_save.autocast + first * sizeof(int), 0, sizeof(no_save.autocast) - first * sizeof(int) );
		memset(no_save.crattack_bonuses + first * sizeof(int), 0, sizeof(no_save.crattack_bonuses) - first * sizeof(int) );
		memset(no_save.crdefence_bonuses + first * sizeof(int), 0, sizeof(no_save.crdefence_bonuses) - first * sizeof(int) );
		memset(no_save.dmgmin_bonuses + first * sizeof(int), 0, sizeof(no_save.dmgmin_bonuses) - first * sizeof(int) );
		memset(no_save.dmgmax_bonuses + first * sizeof(int), 0, sizeof(no_save.dmgmin_bonuses) - first * sizeof(int) );
		memset(no_save.speed_bonuses + first * sizeof(int), 0, sizeof(no_save.speed_bonuses) - first * sizeof(int) );
		memset(no_save.hp_bonuses + first * sizeof(int), 0, sizeof(no_save.hp_bonuses) - first * sizeof(int) );
		memset(no_save.shots_bonuses + first * sizeof(int), 0, sizeof(no_save.shots_bonuses) - first * sizeof(int) );
		memset(no_save.casts_bonuses + first * sizeof(int), 0, sizeof(no_save.casts_bonuses) - first * sizeof(int) );
		
		
		if (first == 0 && false ) {
			//memset(no_save.arteventtable_txt, 0, (NEW_ARTS_AMOUNT+1)*char_table_size);

			
			//memset(no_save.artname, 0, NEW_ARTS_AMOUNT*char_table_size);
			//memset(no_save.artdesc, 0, NEW_ARTS_AMOUNT*char_table_size);
			

		}
		else {
			//memset(save.ERM_Z_name + first * 2 * sizeof(int), 0, (sizeof(save.ERM_Z_name) / 2 - first * sizeof(int)) * 2);
		}
		
	}
*/

 extern int is_6th_misc_slot_enabled;

 extern int SuperVulnerabilityArt;
 extern int SuperNegativityArt;
 extern int NormalFrostArt;
 extern int SuperFrostArt;
 extern int NormalEarthArt;
 extern int SuperEarthArt;

 extern int fix_bp_pos;
 extern int DisableLearningHook;
 extern int SpeedUP;

 extern int EnableCompoundArtifactHints;

 void GlobalConfig() {
	static char buf[65535];
	// static char fname[1024];
	const char* fname = "Data\\emerald.cfg";
	 FILE* fdesc;

	 // sprintf(fname, "Data\\artifacts\\%u.cfg", target);
	 if (!FileExists(fname))
		 MessageBoxA(0, fname, "File not found: ", 0);

	 fix_bp_pos = true;

	 SuperVulnerabilityArt = -99;
	 SuperNegativityArt = -99;

	 NormalFrostArt = -99;
	 SuperFrostArt = -99;

	 NormalEarthArt = -99;
	 SuperEarthArt = -99;

	 SpeedUP = 0;

	 if (fdesc = fopen(fname, "r"))
	 {


		 //----------
		 fseek(fdesc, 0, SEEK_END);
		 int fdesc_size = ftell(fdesc);
		 rewind(fdesc);
		 //----------
		 //buf=(char*)malloc(fdesc_size+1);
		 memset(buf, 0, fdesc_size + 1);
		 fread(buf, 1, fdesc_size, fdesc);
		 //buf[fdesc_size]=0;
		 fclose(fdesc);

		 ParseInt(buf, "SuperVulnerabilityArt=", &SuperVulnerabilityArt);
		 ParseInt(buf, "SuperNegativityArt=", &SuperNegativityArt);

		 ParseInt(buf, "NormalFrostArt=", &NormalFrostArt);
		 ParseInt(buf, "SuperFrostArt=", &SuperFrostArt);

		 ParseInt(buf, "NormalEarthArt=", &NormalEarthArt);
		 ParseInt(buf, "SuperEarthArt=", &SuperEarthArt);
		 
		 ParseInt(buf, "DisableLearningHook=", &DisableLearningHook);

		 ParseInt(buf, "EnableNewBackpackPostion=", &fix_bp_pos);

		 ParseInt(buf, "Enable6thSlot", &is_6th_misc_slot_enabled);
		 if (is_6th_misc_slot_enabled) is_6th_misc_slot_enabled = 1;

		 // SpeedUP, but disables some features
		 ParseInt(buf, "EnableSpeedUP=", &SpeedUP);

		 ParseInt(buf, "EnableCompoundArtifactHints=", &EnableCompoundArtifactHints);

		 //free(buf);

	 }
 }

void __stdcall LoadConfigs(PEvent e)
{
	static bool firsttime = true;
	if (firsttime) {
		/*
		long VanillaSpellArts[] = { 86,87,88,89,123,124,128,135,-1 };
		for (int i = 0; VanillaSpellArts[i] >= 0; ++i)
			no_save.newtable[VanillaSpellArts[i]].spellflag = true;
		*/
		//blank(0);
	}
	else {
		//blank(OLD_ARTS_AMOUNT);
		//blank(NON_BLANK_ARTS_AMOUNT);
		//blank(0);
	}

   WIN32_FIND_DATAA ffd;
   HANDLE hFind = INVALID_HANDLE_VALUE;

   for(int i=0; i<NEW_ARTS_AMOUNT;i++)
   {
	   no_save.erm_on_ae0  [i][0] = '\0';
	   no_save.erm_on_ae1  [i][0] = '\0';
	   no_save.erm_on_ba52 [i][0] = '\0';
	   no_save.erm_on_ba53 [i][0] = '\0';
	   no_save.erm_on_timer[i][0] = '\0';
   }

   {
	   auto z = can_be_duplicate;
	   // for (int i = 45; i <= 140; ++i) z[i] = 1;
   }
   // GlobalConfig();

   hFind = FindFirstFileA("data\\artifacts\\*.cfg", &ffd);
   /*
   hFind = FindFirstFileA(LPCSTR("Mods\\Amethyst Upgrades\\Data\\artifacts\\*.cfg"), &ffd);
   if (INVALID_HANDLE_VALUE == hFind)
	   hFind = FindFirstFileA(LPCSTR("Mods\\Knightmare Kingdoms\\Data\\artifacts\\*.cfg"), &ffd);
   if (INVALID_HANDLE_VALUE == hFind)
	   hFind = FindFirstFileA(LPCSTR("Data\\artifacts\\*.cfg"), &ffd);
	*/

   if (INVALID_HANDLE_VALUE == hFind)  {return;} 

   do
   {
      if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
      {
		 int artifact = atoi(ffd.cFileName);  //а не запилить ли своё atoi, с шахматами и прочая?
		 if (artifact!=0)
		 {
			LoadArtifactConfig(artifact);
		 }
      }
   }
   while (FindNextFileA(hFind, &ffd) != 0);
   FindClose(hFind);
   firsttime = false;

   // no_save.ArtSetUpBack[3].slot = 0;
}


extern "C" __declspec(dllexport) void SoulBind_many_ARTs(int heronum, int artifact, int count) {
	save.SoulBound_Artifacts[heronum][artifact] = count;
}

extern "C" __declspec(dllexport) void SoulBind_ART(int heronum, int artifact) {
	save.SoulBound_Artifacts[heronum][artifact] = 1;
}


extern "C" __declspec(dllexport) void SoulUnBind_ART(int heronum, int artifact) {
	save.SoulBound_Artifacts[heronum][artifact] = 0;
}

extern "C" __declspec(dllexport) int isSoulBound_ART(int heronum, int artifact) {
	return save.SoulBound_Artifacts[heronum][artifact];
}