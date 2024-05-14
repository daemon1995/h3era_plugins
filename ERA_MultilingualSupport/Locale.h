#pragma once
class Locale
{
public:
	Locale(const char* name, const char* displayedName);
	std::string name;
	std::string displayedName;

	const bool operator==(const Locale& other) const;
	const bool operator!=(const Locale& other) const;
	~Locale();
};

