
#include "emerald.h"

extern int enchanted_artifacts[];
extern int enchanted_artifacts_count;

void ParseArray(char* buf, char* name, int* cortage)
{

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


void ParseStr(char* buf, const char* name, char** res)
{
	char *c = strstr(buf,name);
	int l = strlen(name);
	if(c!=NULL)
	{
		char *cend = strstr(c+l,"\"");
		if (cend!=NULL) //мало ли, какие идиоты встречаются
				{
					*res = (char*)malloc(cend-c+1);
					memset(*res,0,cend-c+1);

					for(int i=0,j=0; j<(cend-c)-l; i++,j++)
					{
						if (c[l+j]!='\\')
						{
							(*res)[i] = c[l+j];
						}
						else
						{
							if (c[l+j+1] == 'r')
							{
								(*res)[i] = '\r';
								j++;
								continue;
							}
							else if (c[l+j+1] == 'n')
							{
								(*res)[i] = '\n';
								j++;
								continue;
							} 
							else (*res)[i] = c[l+j];
						}
					}

					//strncpy(*res,c+l,cend-1-c);
					(*res)[cend-c-l] = 0;
					//sprintf(*res,*res);
				}
	}
}

int ParseTuple(char* buf, const char* name, int** tuple) 
{
	  int len=0;
	  char *tmp = (char*)malloc(strlen(buf));
	  char *c = strstr(buf,name);
	  if(c!= NULL) 
	  {
		  strcpy(tmp,c+strlen(name));
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
					*tuple = (int*)realloc(*tuple,len*sizeof(int));
					(*tuple)[len-1]=atoi(p);

				}
				p = strtok(NULL, ", ");
			  } while(p);
		  }
	  }
	free(tmp);
	return len;
}


void LoadCreatureConfig(int target)
{
		char *buf, fname[256];

		int* tuple = NULL;
		int len = 0;

        FILE *fdesc;

		sprintf(fname, "data\\artifacts\\%u.cfg",target);
        if(fdesc=fopen(fname,"r"))
        {
            //----------
            fseek (fdesc , 0 , SEEK_END);
            int fdesc_size=ftell(fdesc);
            rewind(fdesc);
            //----------
            buf=(char*)malloc(fdesc_size+1);
			memset(buf,0,fdesc_size+1);
            fread(buf,1,fdesc_size,fdesc);
            //buf[fdesc_size]=0;
            fclose(fdesc);

			ParseInt(buf, "Cost=", &(newtable[target].cost));
			ParseInt(buf, "Rank=", &(newtable[target].rank));
			ParseInt(buf, "SlotID=", &(newtable[target].slot));
			ParseInt(buf, "ComboID=", &(newtable[target].combonum));
			ParseInt(buf, "ComboPart=", &(newtable[target].partof));

			ParseByte(buf, "Attack=", &(newbtable[target].att));
			ParseByte(buf, "Defence=", &(newbtable[target].def));
			ParseByte(buf, "Knowledge=", &(newbtable[target].knw));
			ParseByte(buf, "Spellpower=", &(newbtable[target].spp));

			ParseInt(buf, "Luck=", luck_bonuses+target);
			ParseInt(buf, "Morale=", morale_bonuses+target);
			ParseInt(buf, "LuckBP=", luck_bonuses_bp+target);
			ParseInt(buf, "MoraleBP=", morale_bonuses_bp+target);

			ParseInt(buf, "Fly=", allow_fly+target);
			ParseInt(buf, "Waterwalk=", allow_water+target);

			ParseInt(buf, "Autospell=", autocast+target);

			ParseStr(buf, "Name=\"", &(newtable[target].name));
			ParseStr(buf, "Description=\"", &(newtable[target].desc));
			ParseStr(buf, "MapDesc=\"", &(arteventtable[target+1]));

			ParseStr(buf, "OnFirstAction=\"", &(erm_on_battlestart[target]));
			ParseStr(buf, "OnCreatureProp=\"", &(erm_on_creature_param_init[target]));

			ParseStr(buf, "OnUnequip=\"", &(erm_on_ae0[target]));
			ParseStr(buf, "OnEquip=\"", &(erm_on_ae1[target]));
			ParseStr(buf, "OnBattleStart=\"", &(erm_on_ba52[target]));
			ParseStr(buf, "OnBattleEnd=\"", &(erm_on_ba53[target]));
			ParseStr(buf, "OnEveryDay=\"", &(erm_on_timer[target]));

			
			ParseInt(buf, "AttackCR=", crattack_bonuses+target);
			ParseInt(buf, "DefenceCR=", crdefence_bonuses+target);

			ParseInt(buf, "DamageMin=", dmgmin_bonuses+target);
			ParseInt(buf, "DamageMax=", dmgmax_bonuses+target);

			ParseInt(buf, "Speed=", speed_bonuses+target);
			ParseInt(buf, "Health=", hp_bonuses+target);

			ParseInt(buf, "Shots=", shots_bonuses+target);
			ParseInt(buf, "Casts=", casts_bonuses+target);

			
			
			len=ParseTuple(buf,"Immunities=",&tuple);
			for(int i=0; i!=len; i++)
				spell_immunity[target][tuple[i]] = 1;

			int spell=-1;
			ParseInt(buf,"GivesSpell=",&spell);
			if(spell!=-1)
			{
				spell^=0x80;
				realloc(enchanted_artifacts,(enchanted_artifacts_count+1)*4);
				enchanted_artifacts[enchanted_artifacts_count]=target;
				enchanted_artifacts_count++;

				newtable[target].spellflag = 1; 
				artspelltable[target] = spell;  
			}

			
            free(buf);
        }
}


void __stdcall LoadConfigs(PEvent e)
{
	memset(luck_bonuses, 0, sizeof(luck_bonuses));
	memset(luck_bonuses_bp, 0, sizeof(luck_bonuses_bp));
	memset(morale_bonuses, 0, sizeof(morale_bonuses));
	memset(morale_bonuses_bp, 0, sizeof(morale_bonuses_bp));
	memset(spell_immunity, 0, sizeof(spell_immunity));
	memset(allow_fly, 0, sizeof(allow_fly));
	memset(allow_water, 0, sizeof(allow_water));
	memset(autocast, 0, sizeof(autocast));
	memset(crattack_bonuses, 0, sizeof(crattack_bonuses));
	memset(crdefence_bonuses, 0, sizeof(crdefence_bonuses));
	memset(dmgmin_bonuses, 0, sizeof(dmgmin_bonuses));
	memset(dmgmax_bonuses, 0, sizeof(dmgmin_bonuses));
	memset(speed_bonuses, 0, sizeof(speed_bonuses));
	memset(hp_bonuses, 0, sizeof(hp_bonuses));
	memset(shots_bonuses, 0, sizeof(shots_bonuses));
	memset(casts_bonuses, 0, sizeof(casts_bonuses));


   WIN32_FIND_DATAA ffd;
   HANDLE hFind = INVALID_HANDLE_VALUE;

   hFind = FindFirstFileA("data\\artifacts\\*.cfg", &ffd);

   if (INVALID_HANDLE_VALUE == hFind)  {return;} 

   do
   {
      if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
      {
		 int artifact = atoi(ffd.cFileName);  //а не запилить ли своё atoi, с шахматами и прочая?
		 if (artifact!=0)
		 {
			LoadCreatureConfig(artifact);
		 }
      }
   }
   while (FindNextFileA(hFind, &ffd) != 0);
   FindClose(hFind);
}