#pragma once
#include "pch.h"
#define PAGE_MAX_CATEGORIES 20
using namespace h3;
//namespace em
//{
	//class EModPageCategory; //:public MenuItem {};
	class EModPage :
		public MenuItem
	{
	public:
		UINT8 index;
		//EModPage();
		EModPage(H3String name, MenuItem* parent, UINT8 index, H3String hint = "", H3String popup = "", H3String tag = "");
		virtual ~EModPage();
		void GetKey() override;
		void SetupText()override;

	//	std::vector<EModPageCategory> cats;// = { "", nullptr };

		//EMod* GetMod() { return reinterpret_cast<EMod*>(parent); }
	};
//}
