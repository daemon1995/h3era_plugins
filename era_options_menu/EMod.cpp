#include "EMod.h"

void em::EMod::FillPages() 
{
	for (size_t i = 0; i < MOD_MAX_PAGES; i++)
	{
		H3String str = key_ptr.PrintfAppend(".%s.page_%d.name", name, i);// +name.Append("page._").Append(i).Append(".name").PrintfAppend;

		if (JSONKeyExists(str))
		{

		}
		
	}
}

void em::EMod::GetKey() { key_ptr = BASE_KEY.PrintfAppend("asd%d",1); }

//void em::EMod::SetupText()
//{
//}
