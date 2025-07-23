// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H
#ifndef ERA4
#define ERA4
#endif // ERA4
#define CREATE_JSON

#ifdef CREATE_JSON
#define NLOHMAN_JSON
#endif // CREATE_JSON
#ifndef RESIZED_PCX
// #define RESIZED_PCX
#endif // !RESIZED_PCX
#ifndef MAIN_MENU_API
#define MAIN_MENU_API
#endif // MAIN_MENU_API
#include "..\headers\header.h"
// add headers that you want to pre-compile here
#include "Locale.h"

#include "H3DlgTextPcxLocale.h"
#include "LocaleManager.h"

#include "LanguageSelectionDlg.h"
#include "TextHandlers/HandlersList.h"
#endif // PCH_H
