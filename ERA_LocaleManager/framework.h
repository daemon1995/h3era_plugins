#ifndef PCH_H
#define PCH_H
#ifndef CREATE_TEXT_JSON_EXPORTS
#define CREATE_TEXT_JSON_EXPORTS

#endif // !CREATE_TEXT_JSON_EXPORTS

#ifdef CREATE_TEXT_JSON_EXPORTS
#define NLOHMAN_JSON
#endif // CREATE_TEXT_JSON_EXPORTS
#ifndef MAIN_MENU_API
#define MAIN_MENU_API
#endif // MAIN_MENU_API
#include "..\headers\header.h"
// add headers that you want to pre-compile here
#include "Locale.h"
#include "LocaleManager.h"

#endif // PCH_H
