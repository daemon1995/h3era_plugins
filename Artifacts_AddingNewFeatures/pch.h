// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.
#define _CRT_SECURE_NO_WARNINGS

#ifndef PCH_H
#define PCH_H
#define _WOG_

// add headers that you want to pre-compile here
#include "..\headers\header.h"
constexpr UINT MAX_SKILL_LEVELS_AMOUNT = eSecSkillLevel::EXPERT +1;
constexpr UINT MAX_ARTIFACTS_AMOUNT = 1024;
constexpr UINT MAX_SPELLS_AMOUNT = h3::limits::SPELLS;
constexpr UINT MAX_SPELL_LEVELS_AMOUNT = 4;
#include "ArtifactsData.h"
#include "SpellsProcedure.h"
#include "NPCProcedure.h"
#include "MovementProcedure.h"
#include "BuildingProcedure.h"
#include "IncomeProcedure.h"
#include "ActionProcedure.h"
#include "SecondarySkillProcedure.h"

#endif //PCH_H
