#include "pch.h"

Locale::Locale(const char* name, const char* displayedName)
	:name(name), displayedName(displayedName)
{

}
Locale::~Locale()
{
	//name.Dereference();
	//displayedName.Dereference();
}
const bool Locale::operator==(const Locale& other) const
{

	return !(*this == other);
}

const bool Locale::operator!=(const Locale& other) const
{

	return strcmp(name.c_str(), other.name.c_str()) || strcmp(displayedName.c_str(), other.displayedName.c_str());
}