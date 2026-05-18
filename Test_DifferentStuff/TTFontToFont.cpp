//#include "TTFontToFont.h"
//std::vector<unsigned char> ttf_buffer;
//// читаешь файл сюда
//
//static bool LoadFile(const char* path, std::vector<uint8_t>& out)
//{
//    FILE* f = fopen(path, "rb");
//    if (!f) return false;
//
//    fseek(f, 0, SEEK_END);
//    size_t sz = ftell(f);
//    fseek(f, 0, SEEK_SET);
//
//    out.resize(sz);
//    fread(out.data(), 1, sz, f);
//    fclose(f);
//    return true;
//}
//
//
//static int AnsiToUnicode(int ch, int codepage)
//{
//    char c = (char)ch;
//    wchar_t w = 0;
//    MultiByteToWideChar(codepage, 0, &c, 1, &w, 1);
//    return (int)w;
//}
//
//BOOL __stdcall BuildFontFromTTF(
//    const char* path,
//    int pixelHeight,
//    int charset,
//    void** outFont)
//{
//   
//
//    return TRUE;
//}