#pragma once


#ifndef HEADER_H
#define HEADER_H

#define WIN32_LEAN_AND_MEAN             // Исключите редко используемые компоненты из заголовков Windows
// Файлы заголовков Windows
#define _CRT_SECURE_NO_WARNINGS

#define _H3API_PATCHER_X86_
#define _H3API_PLUGINS_

namespace Era
{
	char* tr(const char* key);
	void ExecErmCommand(const char* key);
}
#include "..\..\oldHeroesHeaders\h3api_single\H3API.hpp"






#endif // !HEADER_H
