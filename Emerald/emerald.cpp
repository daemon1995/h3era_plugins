// применение основного кода: расширение таблиц

#include "emerald.h"
#include "pch.h"

typedef Era::TEvent* PEvent;


ART_RECORD newtable[NEW_ARTS_AMOUNT];
ART_BONUS newbtable[NEW_ARTS_AMOUNT];
char* arteventtable[NEW_ARTS_AMOUNT+1]; 

char  artspelltable[NEW_ARTS_AMOUNT]; //новая таблица заклятий, даваемых артефактами
int artspellswitch[256];      //новый свитч для этой таблицы
unsigned char _magic[22] = {0x6A,0x01,0x6A,0x09,
							0x8D,0x4D,0xE4,0xB8,
							0xA0,0x67,0x4E,0x00,
							0xFF,0xD0,0xB8,0x9D,
							0x97,0x4D,0x00,0xFF,
							0xE0,0x90}; //машкод кейса, ответственного за добавление заклинания артефакту.
 char new_cases[22*128];  //буфер для новых кейсов, дающих единичные заклинания

 int enchanted_artifacts_count = 9;
 int enchanted_artifacts[] = 
 {0x01,0x80,0x7B,0x7C,
  0x56,0x57,0x58,0x59,
  0x87};	//список артефактов, дающих заклинания. 
			//Для добавления - дописываем свой арт в конец списка 
			//и задаем ему заклинание через artspelltable

 const char *dummy_hint = "Artifact sponsored by emerald.dll. Version from: "\
					__DATE__ \
					" "\
					__TIME__;


extern int __stdcall NewHasArtifactInBP(HiHook* h, HERO* hero, int art);
extern int __stdcall NewHasArtifact(HiHook* h, HERO* hero, int art);

extern int __stdcall LuckTextHook(LoHook* h, HookContext* c);
extern int __stdcall LuckValueHook(LoHook* h, HookContext* c);
extern int __stdcall MoraleTextHook(LoHook* h, HookContext* c);
extern int __stdcall MoraleValueHook(LoHook* h, HookContext* c);

extern double __stdcall ResistanceHook(HiHook* h, int spell, int creature, HERO* dhero, HERO* ahero);
extern int __stdcall OnCreatureParamInit(HiHook* h, HERO* hero, int creature, MONSTER_PROP* monster);

extern int __stdcall BattleStartHook(LoHook* h, HookContext* c);


extern void __stdcall OnEquip(PEvent e);
extern void __stdcall OnUnequip(PEvent e);
extern void __stdcall OnBattleStart(PEvent e);
extern void __stdcall OnBattleEnd(PEvent e);
extern void __stdcall OnNewDay(PEvent e);


int __stdcall ArtTypesHook(LoHook* h, HookContext* c)
{
	if (c->eax == 'W') 
		{*(int*)(0x0C + c->ebx) = 0x20; return NO_EXEC_DEFAULT;} //боевые машины
	if (c->eax == 'B') 
		{*(int*)(0x0C + c->ebx) = 0x40; return NO_EXEC_DEFAULT;} //книги

	return EXEC_DEFAULT;
}


void CastArtifactSpell(int artifact, int spell, int duration)
{
		char* combatManager = (char*)(*((int*)0x699420));
        char *tmp;

        tmp = GetArtifactRecord(spell)->name;
        GetArtifactRecord(spell)->name = GetArtifactRecord(artifact)->name;
        if ( (unsigned char)sub_5A43E0( combatManager, spell, 3, *(int *)(combatManager + 78528), 1, 2) )
          CastBattleSpell(combatManager, spell, -1, 2, -1, 3, duration);
        GetArtifactRecord(spell)->name = tmp;
}


void ExecErmSequence(char* _command)
{
	char command[512];
	memset(command,0,512);
	strcpy(command, _command);

	  char *p;

	  p = strtok(command, "!");
	  Era::ExecErmCmd(p);
	  do {
		p = strtok(NULL, "!");
		if(p) Era::ExecErmCmd(p);
	  } while(p);
}

void __stdcall Emerald(PEvent e)	//основная функция
{
        memcpy(newtable,(void*)*(int*)0x660B68,sizeof(ART_RECORD)*OLD_ARTS_AMOUNT);

		emerald->WriteDword(0x660B68,(int)newtable); //перенос основной таблицы

		emerald->WriteDword(0x7324BD,NEW_ARTS_AMOUNT); //UN:A
		emerald->WriteDword(0x4DC358,0xA0); //MoP said
		

		//текстовики
		emerald->WriteDword(0x44CB32,(int)newtable);
		emerald->WriteDword(0x44CD1E,(int)newtable);
		emerald->WriteDword(0x44CD6C,(int)newtable);


		//
		emerald->WriteDword(0x44CCDF,(int)newtable+0x1C);
		emerald->WriteDword(0x44CCFA,(int)newtable+0x1D);
		emerald->WriteDword(0x716F8D,(int)newtable+0x18);

		emerald->WriteDword(0x716F9E,(int)newtable+0x18);
		emerald->WriteDword(0x716FAE,(int)newtable+0x14);
		emerald->WriteDword(0x716FBF,(int)newtable+0x14);

		emerald->WriteDword(0x717117,(int)newtable+0x14);
		emerald->WriteDword(0x717146,(int)newtable+0x18);
		emerald->WriteDword(0x754A42,(int)newtable);

		emerald->WriteDword(0x717117,(int)newtable+0x14);
		
		//ерм
		emerald->WriteDword(0x714ECA,NEW_ARTS_AMOUNT);
		emerald->WriteDword(0x714F46,NEW_ARTS_AMOUNT);
		emerald->WriteDword(0x716F7F,NEW_ARTS_AMOUNT);
		emerald->WriteDword(0x717020,NEW_ARTS_AMOUNT);
		emerald->WriteDword(0x7324BD,NEW_ARTS_AMOUNT);
		emerald->WriteDword(0x732849,NEW_ARTS_AMOUNT);
		emerald->WriteDword(0x75120D,NEW_ARTS_AMOUNT);

		//бонусы
        memcpy(newbtable,(void*)0x7B8358,4*OLD_ARTS_AMOUNT);
		
		emerald->WriteDword(0x4E2D26, (int)newbtable);
		emerald->WriteDword(0x4E2DF1, (int)newbtable);
		emerald->WriteDword(0x4E2E95, (int)newbtable);
	    emerald->WriteDword(0x4E2F7C, (int)newbtable);

	    emerald->WriteDword(0x4E2D3D, (int)newbtable + 4 * 160);
	    emerald->WriteDword(0x4E2DD7, (int)newbtable + 4 * 160);
	    emerald->WriteDword(0x4E2EAC, (int)newbtable + 4 * 160);
	    emerald->WriteDword(0x4E2F41, (int)newbtable + 4 * 160);

		newbtable[ARTIFACT_CRIMSON_SHLD].def = +2; //восстанавливаем "щит возмездия", будь он неладен
	
		//emerald->WriteDataPatch(0x4e2eb3,"4D F4 E8 E6 17");
		//emerald->WriteDataPatch(0x4E2D43,"8B 4D F4 E8 55");
		

		
		//очистка таблиц
               for(int i=OLD_ARTS_AMOUNT; i!=NEW_ARTS_AMOUNT; i++)
               {
                   newtable[i].slot=0;
                   newtable[i].desc=dummy_hint;
                   newtable[i].name=dummy_hint;
                   newtable[i].partof=-1;
                   newtable[i].combonum=-1;
                   newtable[i].cost=1000;
                   newtable[i].rank=16;

                   newbtable[i].att=0;
                   newbtable[i].def=0;
                   newbtable[i].knw=0;
                   newbtable[i].spp=0;

				   arteventtable[i+1] = dummy_hint;
               }


		//тексты на карте
		
	    emerald->WriteDword(0x49F2E7, (int)arteventtable+4);
	    emerald->WriteDword(0x49F51E, (int)arteventtable+4);
	    emerald->WriteDword(0x49F5DA, (int)arteventtable+4);

	    emerald->WriteDword(0x49DD9A, (int)arteventtable);

	    emerald->WriteDword(0x44CCA8, NEW_ARTS_AMOUNT *4 +8); //artraits.txt
	    emerald->WriteDword(0x44CACA, NEW_ARTS_AMOUNT *4 +8); //artraits.txt
	    emerald->WriteDword(0x49DD90, NEW_ARTS_AMOUNT *4 +0); //artevent.txt


		//таблица заклинаний, даваемых артефактами

        memset(artspelltable,8,NEW_ARTS_AMOUNT);
        memcpy(artspelltable+86,(void*)0x4D9800,136-86); 

		emerald->WriteByte(0x4D95DC,0x06);
		emerald->WriteDword(0x4D95DD,0x90909090);
		emerald->WriteDword(0x4D95EB,(int)artspelltable);


		
        memset(new_cases, 0x90, 22*128);
        for(unsigned char i=0; i!=128; i++)
        {
            memcpy(new_cases+i*22,_magic,22);
            new_cases[i*22+3]=i;
            artspellswitch[i+0x80]=i*22+(int)(new_cases);
        }
        memcpy((void*)artspellswitch,(void*)0x4D97DC,4*9);

		emerald->WriteDword(0x4D95F2,(int)artspellswitch);

		emerald->WriteDword(0x44CCE7,(int)enchanted_artifacts);
		emerald->WriteDword(0x44CCF4,(int)enchanted_artifacts+sizeof(enchanted_artifacts));


		//новые типы артефактов
		emerald->WriteLoHook(0x44CC86, (void*)ArtTypesHook);


		//сплайсим для возможности ограниченного копирования функций артефактов
		emerald->WriteHiHook(0x4D9460, SPLICE_, EXTENDED_, THISCALL_, (void*)NewHasArtifact);
		emerald->WriteHiHook(0x4D9420, SPLICE_, EXTENDED_, THISCALL_, (void*)NewHasArtifactInBP);

		//удача
		emerald->WriteLoHook(0x4DCDA6, (void*)LuckTextHook);
		emerald->WriteLoHook(0x4E3A46, (void*)LuckValueHook);

		//мораль
		emerald->WriteLoHook(0x4DC606, (void*)MoraleTextHook);
		emerald->WriteLoHook(0x4E3C9E, (void*)MoraleValueHook);

		//резисты
		emerald->WriteHiHook(0x44A1A0, SPLICE_, EXTENDED_, FASTCALL_, (void*)ResistanceHook);

		//начало боя
		emerald->WriteLoHook(0x464FBA, (void*)BattleStartHook);

		emerald->WriteHiHook(0x4E6390, SPLICE_, EXTENDED_,THISCALL_, (void*)OnCreatureParamInit );


		//костыли для бога костылей!
		Era::ExecErmCmd  = (void (__stdcall *)(char*))GetProcAddress(LoadLibraryA("era.dll"), "ExecErmCmd");
		Era::RegisterHandler  = (Era::TRegisterHandler)GetProcAddress(LoadLibraryA("era.dll"), "RegisterHandler");

		Era::RegisterHandler(OnEquip, "OnEquipArt");
		Era::RegisterHandler(OnUnequip, "OnUnequipArt");
		//RegisterHandler(OnBattleStart, "OnSetupBattlefield");//
		Era::RegisterHandler(OnBattleEnd, "OnAfterBattleUniversal");//
		Era::RegisterHandler(OnNewDay, "OnGlobalTimer");
			
		//emerald->WriteHiHook(0x5A7BF0, SPLICE_, EXTENDED_, FASTCALL_, (void*)MagicDamageHook);

		//загрузка из конфигов


	
	//костыли для бога костылей!
}


