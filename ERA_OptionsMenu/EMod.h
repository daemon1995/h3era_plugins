#pragma once
//#include "MenuItem.h"
#include "pch.h"
using namespace h3;
#define MOD_MAX_PAGES 20

	//class EModPage;
	class EMod :
		public MenuItem
	{

		H3Vector<int> opt_id;
		H3String tag;
	public:
//		std::vector<EModPage> pages;

		//EMod();
		EMod(H3String name, MenuItem* parent, H3String hint = "", H3String popup = "", H3String tag = "");

		void FillPages();
		void GetKey() override;
		virtual ~EMod();
	};
	/** static EMod& Create(H3String name, H3String popup = "", H3String hint = "", H3String tag = "")
	{
		return *new EMod(name, nullptr, hint, popup, tag);
	}**/