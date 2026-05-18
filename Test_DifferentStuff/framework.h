#pragma once

#define WIN32_LEAN_AND_MEAN // Исключите редко используемые компоненты из
                            // заголовков Windows
#define STB_TRUETYPE_IMPLEMENTATION

// Файлы заголовков Windows
#include <windows.h>

#include "..\headers\header.h"
//#include "..\headers\stb_truetype.h"
template <typename... Ints> void Debug(Ints... values) noexcept;

// #include "CombatHooks.h"
