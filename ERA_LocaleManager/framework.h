#ifndef PCH_H
#define PCH_H
#define CREATE_JSON

#ifdef CREATE_JSON
#define NLOHMAN_JSON
#endif // CREATE_JSON
#ifndef MAIN_MENU_API
#define MAIN_MENU_API
#endif // MAIN_MENU_API
#include "..\headers\header.h"
// add headers that you want to pre-compile here
#include "Locale.h"
#include "LocaleManager.h"

#endif // PCH_H
