#pragma once
#include "pch.h"

 class ObjectsExtender : public IGamePatch
{
protected:

	ObjectsExtender(PatcherInstance* pi);
	void CreatePatches()  override;

	virtual ~ObjectsExtender();

};

