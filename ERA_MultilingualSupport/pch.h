// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H
#define ERA4
//#define CREATE_JSON

#ifdef CREATE_JSON
#define NLOHMAN_JSON
#endif // CREATE_JSON



#include "..\headers\header.h"
// add headers that you want to pre-compile here
#include "Locale.h"

#include "DlgStyle.h"
#include "H3DlgTextPcxLocale.h"
#include "LocaleManager.h"

#include "LanguageSelectionDlg.h"
#include "MonsterHandler.h"
#include "ArtifactHandler.h"
#include "HeroHandler.h"
#include "MapObjectHandler.h"
#endif // PCH_H
