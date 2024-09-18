#pragma once
#include "pch.h"


struct _DlgCreatureExpoInfo_
{
	char* Caption;         // заголовок диалога
	char* Info;            // информация о твари
	char* Picture;         // изображение твари
	char* PictureHint;     // хинт к изображению твари
	char** TxtProperties;  // [6] текстовые доп. свойства
	char** IcoProperties;  // [6] иконки доп. свойств, "NONE" - для отображения заглушки
	char** HintProperties; // [6] подсказки к  доп. свойствам
	char* ColCaptions;     // заголовки столбцов - одна строка, по 7 символов на столбец (11*7)
	char* ColHint;         // заголовки столбцов - подсказки
	char** RowCaptions;    // [16] заголовки строк
	char** RowCaptionHints;// [16] заголовки строк, подсказки
	char** Rows;           // [16] значения ячеек таблицы - 16 строк по 7 символов на столбец (11*7)
	char** RowHints;       // [16] подсказки к строкам
	int IcoPropertiesCount; // [0 - 6] количество иконок доп. свойств
	int ShowSpecButton;     // показывать кнопку
	char* SpecButtonHint;   // подсказка к кнопке
	int CurPropColLeft;     // начало столбца с текущими координатами твари в символах
	int CurPropColRight;    // конец столбца с текущими координатами твари в символах
	char* ArtIcon;          // иконка артифакта ("NONE" - пустой)
	char* ArtHint;          // хинт для иконки артифакта
	int ArtOutput;          // артифакт отдается - 1, оставляется - 0
	int Flags;              // флаги - см. ниже
							// Flags & 0x00000001 - можно передавать артефакт
	int ArtCopy;            // значения:
							// 0,... - значения
							// <0 - скрыть

};

struct StackActiveSpells
{
	H3Vector<INT32> activeSpellsId;
};

struct CrExpBonLine
{

	unsigned __int32 Act : 1;
	unsigned __int32 _un : 31;
	char Type;
	char Mod;
	char Lvls[11];
};

struct CreatureSkill
{
	int expId;
	const char* name;
	const char* description;
	const char* pcx16Name;
	H3LoadedPcx16* pcx16;
	~CreatureSkill();

};
class CreatureDlgHandler
{
	H3CreatureInfoDlg* dlg = nullptr;
	H3CombatCreature* stack = nullptr;
	H3Army* army = nullptr;
	int armySlotIndex = -1;
	bool wogStackExperience = false;

public:

	CreatureDlgHandler(H3CreatureInfoDlg* dlg, H3CombatCreature* stack = nullptr, H3Army * army = nullptr, int armySlotIndex =-1);

	BOOL AlignItems();
	BOOL AddExperienceButton();
	BOOL AddSpellEfects();
	~CreatureDlgHandler();

	BOOL AddCommanderSkills();

	BOOL CreateCreatureSkillsList();

	//bool Adjust(Crex,
	static std::vector<H3DlgPcx16*> dlgSkillPcx;
	static std::vector<CreatureSkill> creatureSkills;
};
