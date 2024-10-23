#pragma once
#include "era.h"



namespace EraJS
{
#ifndef ERA4
#define TR _tr
#else
#define TR  Era::tr
#endif 

	inline Era::static_str _tr(const char* key)
	{

		char* buf = Era::_tr(key, NULL, -1);
		char* result = Era::ToStaticStr(buf);
		Era::MemFree(buf);

		return result;
	}
	inline const bool isEmpty(const char* keyName) noexcept
	{
		return !strcmp(TR(keyName), keyName);
	}


	inline const int readInt(const char* keyName) noexcept
	{
		return atoi(TR(keyName));
	}

	inline const int readInt(const std::string& keyName) noexcept
	{
		return readInt(keyName.data());
	}

	inline const int readInt(const char* keyName, bool& success) noexcept
	{
		char* result = TR(keyName);
		success = strcmp(result, keyName) != 0;
		return atoi(result);
	}
	inline const int readInt(const std::string& keyName, bool& success)  noexcept
	{
		return readInt(keyName.data(), success);
	}

	inline char* read(const char* keyName) noexcept
	{
		return TR(keyName);
	}
	inline char* read(const char* keyName, bool& success) noexcept
	{
		char* result = TR(keyName);
		success = strcmp(result, keyName) != 0;
		return result;
	}
	inline char* read(const std::string& keyName) noexcept
	{
		return read(keyName.c_str());
	}
	inline char* read(const std::string& keyName, bool& success) noexcept
	{
		return read(keyName.c_str(), success);
	}
	inline const double readFloat(const char* keyName) noexcept
	{
		return atof(TR(keyName));
	}
	inline const double readFloat(const std::string& keyName) noexcept
	{
		return readFloat(keyName.data());
	}
	inline const double readFloat(const char* keyName, bool& success) noexcept
	{
		char* result = TR(keyName);
		success = strcmp(result, keyName) != 0;
		return atof(result);
	}
	inline const double readFloat(const std::string& keyName, bool& success) noexcept
	{
		return readFloat(keyName.data(), success);
	}
	//inline int readFloat(char* keyName)
	//{
	//	if (isEmpty(keyName))
	//		return NAN;
	//	return atof(Era::tr(keyName));
	//}

}