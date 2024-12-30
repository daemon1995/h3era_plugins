
#include "DrawPcx16ResizedBicubic.h"

using namespace h3;

namespace resized
{
// void DrawPcx16ResizedBicubic(H3LoadedPcx16* _this, H3LoadedPcx16* src_pcx, int s_w, int s_h, int d_x, int d_y, int
// d_w, int d_h);

void H3LoadedPcx16Resized::DrawResizedBicubic(H3LoadedPcx16 *src_pcx, int s_w, int s_h, int d_x, int d_y, int d_w,
                                              int d_h)
{

    DrawPcx16ResizedBicubic(this, src_pcx, s_w, s_h, d_x, d_y, d_w, d_h);
}

H3LoadedPcx16Resized *H3LoadedPcx16Resized::Create(INT width, INT height)
{
    auto pcx = H3LoadedPcx16::Create(width, height);
    return reinterpret_cast<H3LoadedPcx16Resized *>(pcx);
}

H3LoadedPcx16Resized *H3LoadedPcx16Resized::Create(LPCSTR name, INT width, INT height)
{

    return Create(h3_NullString, width, height);
}

double BiCubicKernel(double x)
{
    if (x > 2.0)
        return 0.0;

    double a, b, c, d;
    double xm1 = x - 1.0;
    double xp1 = x + 1.0;
    double xp2 = x + 2.0;

    a = (xp2 <= 0.0) ? 0.0 : xp2 * xp2 * xp2;
    b = (xp1 <= 0.0) ? 0.0 : xp1 * xp1 * xp1;
    c = (x <= 0.0) ? 0.0 : x * x * x;
    d = (xm1 <= 0.0) ? 0.0 : xm1 * xm1 * xm1;

    return (0.16666666666666666667 * (a - (4.0 * b) + (6.0 * c) - (4.0 * d)));
}

void H3LoadedPcx16Resized::DrawPcx16ResizedBicubic(H3LoadedPcx16 *_this, H3LoadedPcx16 *src_pcx, const int scale)
{

    DrawPcx16ResizedBicubic(_this, src_pcx, src_pcx->width, src_pcx->height, 0, 0, src_pcx->width / scale,
                            src_pcx->height / scale);
}

void H3LoadedPcx16Resized::DrawPcx16ResizedBicubic(H3LoadedPcx16 *_this, H3LoadedPcx16 *src_pcx, const int s_w,
                                                   const int s_h, const int d_x, const int d_y, const int d_w,
                                                   const int d_h)
{
    UINT8 bitMod = H3BitMode::Get() << 3;

    unsigned src_pitch = src_pcx->scanlineSize;
    unsigned dst_pitch = _this->scanlineSize;
    PUINT8 src_bits = src_pcx->buffer;
    PUINT8 dst_bits = _this->buffer;
    PUINT8 linek, lined;

    double xFactor = static_cast<double>(s_w) / d_w;
    double yFactor = static_cast<double>(s_h) / d_h;
    // coordinates of source points and coefficients
    double ox, oy, dx, dy, k1, k2;
    int ox1, oy1, ox2, oy2;

    int ymax = s_h - 1;
    int xmax = s_w - 1;

    if (bitMod == 32)
    {

        // destination pixel values
        double *p_g = new double[3];

        for (int y = 0; y < d_h; y++)
        {
            // Y coordinates
            oy = static_cast<double>(y) * yFactor + 0.5;
            oy1 = (int)oy;
            dy = oy - static_cast<double>(oy1);

            lined = dst_bits + (y + d_y) * dst_pitch;

            for (int x = 0; x < d_w; x++)
            {
                // X coordinates
                ox = static_cast<double>(x) * xFactor + 0.5;
                ox1 = (int)ox;
                dx = ox - static_cast<double>(ox1);

                // initial pixel value
                memset(p_g, 0, sizeof(double) * 3);

                for (int n = -1; n < 3; n++)
                {
                    // get Y coefficient
                    k1 = resized::BiCubicKernel(dy - static_cast<double>(n));

                    oy2 = oy1 + n;
                    if (oy2 < 0)
                        oy2 = 0;
                    if (oy2 > ymax)
                        oy2 = ymax;

                    linek = src_bits + oy2 * src_pitch;

                    for (int m = -1; m < 3; m++)
                    {
                        // get X coefficient
                        k2 = k1 * resized::BiCubicKernel(static_cast<double>(m) - dx);

                        ox2 = ox1 + m;
                        if (ox2 < 0)
                            ox2 = 0;
                        if (ox2 > xmax)
                            ox2 = xmax;

                        p_g[2] += k2 * linek[ox2 * 4 + 2];
                        p_g[1] += k2 * linek[ox2 * 4 + 1];
                        p_g[0] += k2 * linek[ox2 * 4 + 0];
                    }
                }
                lined[(x + d_x) * 4 + 2] = static_cast<BYTE>(p_g[2]);
                lined[(x + d_x) * 4 + 1] = static_cast<BYTE>(p_g[1]);
                lined[(x + d_x) * 4 + 0] = static_cast<BYTE>(p_g[0]);
            }
        }

        delete[] p_g;
    }
    else
    {

        H3ARGB888 temp888;
        H3RGB565 temp565;
        for (int y = 0; y < d_h; y++)
        {
            // Y coordinates
            oy = static_cast<double>(y) * yFactor - 0.5;
            oy1 = static_cast<int>(oy);
            dy = oy - static_cast<double>(oy1);

            lined = dst_bits + (y + d_y) * dst_pitch;

            for (int x = 0; x < d_w; x++)
            {
                // X coordinates
                ox = static_cast<double>(x) * xFactor - 0.5f;
                ox1 = static_cast<int>(ox);
                dx = ox - static_cast<double>(ox1);

                // reset pixel color values
                memset(&temp888, 0, sizeof(temp888));

                for (int n = -1; n < 3; n++)
                {
                    // get Y coefficient
                    k1 = resized::BiCubicKernel(dy - static_cast<double>(n));

                    oy2 = oy1 + n;
                    if (oy2 < 0)
                        oy2 = 0;
                    if (oy2 > ymax)
                        oy2 = ymax;

                    linek = src_bits + oy2 * src_pitch;

                    for (int m = -1; m < 3; m++)
                    {
                        // get X coefficient
                        k2 = k1 * resized::BiCubicKernel(static_cast<double>(m) - dx);

                        ox2 = ox1 + m;
                        if (ox2 < 0)
                            ox2 = 0;
                        if (ox2 > xmax)
                            ox2 = xmax;

                        temp565 = WordAt(UINT(linek + ox2 * 2));

                        DWORD dw = temp565.GetRGB888();

                        temp888.r += static_cast<UINT8>(k2 * ((dw >> 16) & 0xFF));
                        temp888.g += static_cast<UINT8>(k2 * ((dw >> 8) & 0xFF));
                        temp888.b += static_cast<UINT8>(k2 * (dw & 0xFF));
                    }
                }

                temp565 = temp888;
                WordAt(UINT(lined + (x + d_x) * 2)) = temp565;
            }
        }
    }
}

} // namespace resized
