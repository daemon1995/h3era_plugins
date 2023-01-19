#pragma once


#ifndef HEADER_H
#define HEADER_H

#define WIN32_LEAN_AND_MEAN             // Исключите редко используемые компоненты из заголовков Windows
// Файлы заголовков Windows
#define _CRT_SECURE_NO_WARNINGS

#define _H3API_PATCHER_X86_
#define _H3API_PLUGINS_


#include "..\..\oldHeroesHeaders\h3api_single\H3API.hpp"
#include "..\..\oldHeroesHeaders\headers\era.h"



Patcher* globalPatcher;
PatcherInstance* _PI;
void DebugInt(int i)
{
	h3::H3Messagebox(Era::IntToStr(i).c_str());
}
#include "map"
#include "string"



#endif // !HEADER_H
