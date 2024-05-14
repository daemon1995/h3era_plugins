#pragma once
#include "pch.h"

//namespace em {
	class EModPageCategory :
		public MenuItem
	{
	public:
		//std::vector<EOption*> options;

		EModPageCategory(H3String name, MenuItem* parent, H3String hint = "", H3String popup = "", H3String tag = "");
		~EModPageCategory();
		void SetupText() override;


	};


//}