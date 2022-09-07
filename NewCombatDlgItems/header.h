#pragma once

#ifndef HEADER_H
#define HEADER_H
#define WIN32_LEAN_AND_MEAN             // Исключите редко используемые компоненты из заголовков Windows
// Файлы заголовков Windows
//#define _CRT_SECURE_NO_WARNINGS
#define o_BPP ByteAt(0x5FA228 + 3) << 3

#define _H3API_PATCHER_X86_
#define _H3API_PLUGINS_
#include "..\..\oldHeroesHeaders\h3api_single\H3API.hpp"
#ifndef ERA_H
#define ERA_H
#include "..\..\oldHeroesHeaders\headers\era.h"
#endif
Patcher* globalPatcher;
PatcherInstance* _PI;

#define DLG_HEIGHT_ADD 56
#include "map"
#include "string"

#endif // !HEADER_H


