#pragma once
#ifndef RESIZED_PCX
#define RESIZED_PCX
#endif
#ifndef ERA4
#define ERA4
#endif
#ifndef ERA_MODLIST
#define ERA_MODLIST
#endif
#ifndef MAIN_MENU_API
#define MAIN_MENU_API
#endif
#ifndef ERA4
#define ERA4
#endif
#ifndef OBJECTS_EXTENDER_API
#define OBJECTS_EXTENDER_API
#endif

#ifndef PCH_H
#define PCH_H
// Добавьте сюда заголовочные файлы для предварительной компиляции
// #include "framework.h"

#include "..\headers\header.h"
#include "h3functions.hpp"
constexpr UINT16 H3_MAX_OBJECTS = h3::limits::OBJECTS;
#include "ObjectExtenderManager.h"

// 1. Place all the obj props into mod/data/objects/*.txt and parse all the mods

// 2. Add read data and add all new objects to game objects list (if insert == true);

// 3. only 2 GENERAL json keys (objectsRmgListEditor and objectsToAddInRmgList)
//	3.1 objectsRmgListEditor handles all RMG add/set/get info;
//	3.2 objectsToAddInRmgList is actuallyabout objects extensios adds new objects and allows generation if data is
// properly set; 	3.3 display data from default.exe/json files 	3.4
// 3. Setting Gen Data first from New objects data
// 4. Get data from object Editor and merge/override data if contains same json fields
// 5. In dlg show in one line info about / gen data

#endif // PCH_H
