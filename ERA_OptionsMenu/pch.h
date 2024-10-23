// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.
#define _CRT_SECURE_NO_WARNINGS

#ifndef PCH_H
#define PCH_H

// add headers that you want to pre-compile here
#include "..\headers\header.h"

#include "ModListReader.h"
#include "MenuItem.h"
#include "EOption.h"
#include "EModPageCategory.h"
#include "EModPage.h"
#include "EMod.h"
#include "EraMenuDlg.h"
extern std::vector<H3String> modList;
#endif //PCH_H
