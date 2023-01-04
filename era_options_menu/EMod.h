#pragma once
//#include "MenuItem.h"
#include "EModPage.cpp"
using namespace h3;
#define MOD_MAX_PAGES 20
namespace em
{
	//class EModPage;
	class EMod :
		public MenuItem
	{

		H3Vector<int> opt_id;
		H3String tag;
	public:
		std::vector<EModPage> pages;

		EMod()
		{

		}
		EMod(H3String name, MenuItem* parent, H3String hint = "", H3String popup = "", H3String tag = "")
			:MenuItem(name, parent,hint, popup), tag(tag)
		{
			pages.clear();
			pages.reserve(MOD_MAX_PAGES);
		//	this->name = name;
		//	this->popup = popup;
		//	this->hint = hint;
			//this->tag = tag;
		}
		void FillPages();
		void GetKey() override;
		~EMod()
		{
			pages.clear();
		}
	};
	/** static EMod& Create(H3String name, H3String popup = "", H3String hint = "", H3String tag = "")
	{
		return *new EMod(name, nullptr, hint, popup, tag);
	}**/
}