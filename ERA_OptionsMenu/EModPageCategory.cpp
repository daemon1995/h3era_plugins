#include "pch.h"

EModPageCategory::EModPageCategory(H3String name, MenuItem* parent, H3String hint, H3String popup, H3String tag)
	:MenuItem(name, parent, hint, popup)

{
//
		//options.clear();
	//	options.reserve(150);

		//	this->name = name;
		//	this->popup = popup;
		//	this->hint = hint;
			//this->tag = tag;
}

EModPageCategory::~EModPageCategory()
{
//	options.clear();
}
void EModPageCategory::SetupText()
{
	this->SetEnabled(false);
}