#pragma once
#include "EOption.cpp"
namespace em {

	class EModPageCategory :
		public MenuItem
	{
	public:
		std::vector<EOption> options;
		EModPageCategory(H3String name, MenuItem* parent, H3String hint = "", H3String popup = "", H3String tag = "")
			:MenuItem(name, parent, hint, popup)
		{
			options.clear();
			options.reserve(150);

			//	this->name = name;
			//	this->popup = popup;
			//	this->hint = hint;
				//this->tag = tag;
		}
		~EModPageCategory()
		{
			options.clear();
		}
		void SetupText() override 
			{
				this->Disable();
			} 

	};


}