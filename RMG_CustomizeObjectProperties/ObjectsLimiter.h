#pragma once
#include "pch.h"

class ObjectsLimiter : public IGamePatch
{


	static IGamePatch* instance;

	static _LHF_(RMG_OnBeforeMapGeneration);

	virtual ~ObjectsLimiter();

public:
	 void Init() ;

	 static IGamePatch* Get() noexcept  ;

};

