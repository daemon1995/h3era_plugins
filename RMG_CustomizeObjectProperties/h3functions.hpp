#pragma once
#include "pch.h"

namespace h3functions
{

inline int NeedExpoToNextLevel(int lvl)
{
	return FASTCALL_1(int, 0x04DA690, lvl);
}

inline float GetHeroLearningPower(const H3Hero* hero)
{
	return THISCALL_1(float, 0x04E4AB0, hero);
}

//void AdvMgr_DisplayInfoPanel(H3String objName)
//{
//	return THISCALL_4(void, 0x415FC0, P_AdventureMgr->Get(), objName.String(), -1, -1);
//}

} // namespace