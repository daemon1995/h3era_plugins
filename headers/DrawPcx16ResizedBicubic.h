#pragma once
// #include "Sharpen.cpp"
#include "header.h"
using namespace h3;

namespace resized
{

class H3LoadedPcx16Resized : public H3LoadedPcx16
{

    // H3LoadedPcx16Resized();

  public:
    void DrawResizedBicubic(H3LoadedPcx16 *src_pcx, int s_w, int s_h, int d_x, int d_y, int d_w, int d_h);

    static void DrawPcx16ResizedBicubic(H3LoadedPcx16 *dstPic, H3LoadedPcx16 *src_pcx, int s_w, int s_h, int d_x,
                                        int d_y, int d_w, int d_h);
    static void DrawPcx16ResizedBicubic(H3LoadedPcx16 *dstPic, H3LoadedPcx16 *src_pcx, int scale);

    static H3LoadedPcx16Resized *Create(LPCSTR name, INT width, INT height);
    /** @brief creates an unnamed pcx16 drawing canvas*/
    static H3LoadedPcx16Resized *Create(INT width, INT height);
};
} // namespace resized
