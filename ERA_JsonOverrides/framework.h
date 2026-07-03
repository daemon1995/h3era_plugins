#pragma once

#define WIN32_LEAN_AND_MEAN // Исключите редко используемые компоненты из
// заголовков Windows
// Файлы заголовков Windows
#include <windows.h>
// #define ERA_MODLIST
// #define _WOG_
#include "..\headers\header.h"

#define DECLARE_JSON_FIELD(type, field) static constexpr LPCSTR field = "era." #type ".%d." #field;
#define DECLARE_JSON_ARRAY_FIELD(type, field) static constexpr LPCSTR field = "era." #type ".%d." #field ".%d";

#define GENERATE_FORMAT_STR(struct_type, json_type, field)                                                             \
    static constexpr LPCSTR field = std::is_array_v<decltype(std::declval<struct_type>().field)>                       \
                                        ? "era." #json_type ".%d." #field ".%d"                                        \
                                        : "era." #json_type ".%d." #field;

#define GENERATE_PARSER_BLOCK(instance, field)                                                                         \
    EraJS::ParseFieldDispatch((instance).field, formats::field, i, std::is_array<decltype((instance).field)>{});
