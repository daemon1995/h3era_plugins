#pragma once
#ifndef HEADER_H
#define HEADER_H
//#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Исключите редко используемые компоненты из заголовков Windows

//#endif // !WIN32_LEAN_AND_MEAN
// Файлы заголовков Windows
//#define _CRT_SECURE_NO_WARNINGS
//#include <windows.h>
#define _H3API_PATCHER_X86_
#define _H3API_PLUGINS_
#define _H3API_EXEPTIONS_
#define o_HD_X (*(INT*)0x401448)
#define o_HD_Y (*(INT*)0x40144F)
#pragma comment(lib,"user32.lib")
#include ".\H3API.hpp"

h3::H3String BASE_KEY = "era_menu.";


#include "..\..\headers\WogClasses.h"

//#include "era.h"

//#include "vector"

//#include "..\..\oldHeroesHeaders\h3api_single\H3API.hpp"

#endif // !HEADER_H

