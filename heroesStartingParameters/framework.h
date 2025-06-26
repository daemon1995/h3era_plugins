#pragma once

#define WIN32_LEAN_AND_MEAN // Исключите редко используемые компоненты из заголовков Windows
// Файлы заголовков Windows
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#define _H3API_CPLUSPLUS11_
#define _H3API_PLUGINS_
#define _WOG_
#include "..\headers\header.h"
#include "TownSelectionDlg.h"
#include <windows.h>
constexpr int HEROES_COUNT = h3::limits::HEROES;
constexpr int MAX_HEROES_PER_TOWN = 19;
