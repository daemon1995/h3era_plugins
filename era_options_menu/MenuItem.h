#pragma once
#include "header.h"
#include "Era.h"

#include <vector>

using namespace h3;
const bool JSONKeyExists(H3String str);

namespace em
{
	//class EMod;

	//class EModPage;
	//class EModPageCategory;
	//class EOption;

	class MenuItem
	{
	//protected:
		static int item_id;
		int id;

	public:
		UINT8 index;
		//H3DlgItem* it = nullptr;
		MenuItem* parent;
		H3String key_ptr;

		H3String name;
		H3String hint;

		H3String popup;
		bool enabled;
		UINT8 state;
		MenuItem()
		{

		}
		MenuItem(H3String name, MenuItem* parent = nullptr, H3String hint = "", H3String popup = "")
			:  name(name), parent(parent), hint(hint), popup(popup)
		{
			//parent = nullptr;
			if (parent != nullptr)
				key_ptr = parent->key_ptr.PrintfAppendH3(".%s", name);
			else
				key_ptr = BASE_KEY + name;
			GetKey();
			enabled = true;
			id =item_id++;

		}


		void SendCommand(h3::H3DlgItem &it, h3::H3Msg& msg)
		{

		}
		MenuItem& GetParent() { return *parent; }
		void Enable() { enabled = false; }
		void Disable() { enabled = true; }
		//virtual MenuItem& GetParent() =0;
		int GetId() { return id; }
		virtual void GetKey();
		virtual void SetupText();

		H3String GetName();
		H3String GetHint();
		H3String GetPopup();

		virtual ~MenuItem()
		{

			//delete parent;
			--item_id;
		}
	//	virtual MenuItem& GetParent() = 0;
	//	virtual MenuItem& GetChild() = 0;


	};
} /*end namespace em*/
