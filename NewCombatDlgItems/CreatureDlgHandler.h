#pragma once
#include "pch.h"


struct _DlgCreatureExpoInfo_
{
	char* Caption;         // ��������� �������
	char* Info;            // ���������� � �����
	char* Picture;         // ����������� �����
	char* PictureHint;     // ���� � ����������� �����
	char** TxtProperties;  // [6] ��������� ���. ��������
	char** IcoProperties;  // [6] ������ ���. �������, "NONE" - ��� ����������� ��������
	char** HintProperties; // [6] ��������� �  ���. ���������
	char* ColCaptions;     // ��������� �������� - ���� ������, �� 7 �������� �� ������� (11*7)
	char* ColHint;         // ��������� �������� - ���������
	char** RowCaptions;    // [16] ��������� �����
	char** RowCaptionHints;// [16] ��������� �����, ���������
	char** Rows;           // [16] �������� ����� ������� - 16 ����� �� 7 �������� �� ������� (11*7)
	char** RowHints;       // [16] ��������� � �������
	int IcoPropertiesCount; // [0 - 6] ���������� ������ ���. �������
	int ShowSpecButton;     // ���������� ������
	char* SpecButtonHint;   // ��������� � ������
	int CurPropColLeft;     // ������ ������� � �������� ������������ ����� � ��������
	int CurPropColRight;    // ����� ������� � �������� ������������ ����� � ��������
	char* ArtIcon;          // ������ ��������� ("NONE" - ������)
	char* ArtHint;          // ���� ��� ������ ���������
	int ArtOutput;          // �������� �������� - 1, ����������� - 0
	int Flags;              // ����� - ��. ����
							// Flags & 0x00000001 - ����� ���������� ��������
	int ArtCopy;            // ��������:
							// 0,... - ��������
							// <0 - ������

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
