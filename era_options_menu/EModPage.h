#pragma once
#include "EModPageCategory.cpp"
//#include "EMod.h"
#define PAGE_MAX_CATEGORIES 20
using namespace h3;
namespace em
{
	//class EModPageCategory; //:public MenuItem {};
	class EModPage :
		public MenuItem
	{
	public:
		std::vector<EModPageCategory> cats;// = { "", nullptr };
		UINT8 index;
		EModPage()
		{

		}
		EModPage(H3String name, MenuItem * parent, UINT8 index, H3String hint = "", H3String popup = "", H3String tag = "")
			:MenuItem(name, parent, hint, popup), index(index)
		{
			cats.clear();
			cats.reserve(PAGE_MAX_CATEGORIES);
			//	this->name = name;
			//	this->popup = popup;
			//	this->hint = hint;
				//this->tag = tag;
		}
		~EModPage()
		{
			cats.clear();

		}
		void GetKey() override;
		void SetupText()override;

		//EMod* GetMod() { return reinterpret_cast<EMod*>(parent); }
	};
}
