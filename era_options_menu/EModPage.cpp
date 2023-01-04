#include "EModPage.h"

void em::EModPage::GetKey()
{
	key_ptr = parent->key_ptr.PrintfAppend("page_%d.",index);
}

void em::EModPage::SetupText()
{
}
