#pragma once
#include <string>

namespace external
{

std::string GetLoadedGameMods();
std::string ExtractModNameFromPath();
std::string GetExecutableDirectory();

}