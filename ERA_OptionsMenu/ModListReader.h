#pragma once

#include "pch.h"
#include <fstream>

class ModListReader
{

public:

	static bool Read(std::vector<H3String> &vec)
	{
        char buff[256];
        GetCurrentDirectoryA(200, buff);
        H3String path = buff;
        path.Append("\\Mods\\list.txt");
        std::ifstream fread(path.String(), std::ifstream::in);
        //  H3SEHandler seh;

          //try
        bool is_open = fread.is_open();
        if (is_open)
        {
            //return 1;

            std::string modName;
            int i = 0;
            vec.reserve(25);

            for (i = 0; fread.good(); i++)
            {
                std::getline(fread, modName);

                if (!modName.empty())
                {
                    std::transform(modName.begin(), modName.end(), modName.begin(), ::tolower);

                   sprintf(h3_TextBuffer, "era_menu.%s.name", modName.c_str());// BASE_KEY + ".name"; //Era::tr("era_menu.internal.read_error");
                   // std::transform(key_name.begin(), key_name.end(), key_name.begin(), ::tolower);

                    if (!EraJS::isEmpty(h3_TextBuffer))
                    {
                      //  H3String key_name(h3_TextBuffer);
                        sprintf(h3_TextBuffer, "era_menu.%s", modName.c_str());
                        vec.emplace_back(h3_TextBuffer);//, key_hint.String(), key_popup.String()));
                    }
                }

            }
            vec.shrink_to_fit();
            fread.close();

        }
        else
        {
            //   H3Messagebox(Era::tr("era_menu.internal.read_error"));

        }

        return is_open;
	}
};

