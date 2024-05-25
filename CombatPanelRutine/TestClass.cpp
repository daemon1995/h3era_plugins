#include "pch.h"
#include "TestClass.h"

void TestClass::ShowMsbox()
{
	H3Messagebox::Show(m_txt.c_str());
}

void TestClass::SetText(const char* txt)
{
	m_txt = txt;
}
