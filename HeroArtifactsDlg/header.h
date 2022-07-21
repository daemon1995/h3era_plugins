#pragma once

#define WIN32_LEAN_AND_MEAN             // Исключите редко используемые компоненты из заголовков Windows
// Файлы заголовков Windows
#define _CRT_SECURE_NO_WARNINGS
//#include <windows.h>
#define _H3API_PATCHER_X86_
#define _H3API_PLUGINS_
#define MAX_BP_ARTIFACTS 64

#define ART_NOT_PLACED 0
#define ART_PLACED 1
#define o_HD_X (*(INT*)0x401448)
#define o_HD_Y (*(INT*)0x40144F)

#include "vector"
#include "..\..\oldHeroesHeaders\h3api_single\H3API.hpp"
#include "HeroArts_CustomDlg.cpp"


