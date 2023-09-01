#include "header.h"

//#define o_BPP (ByteAt(0x5FA228 + 3) << 3)

_Pcx16_* _Pcx16_::CreateNew(char* n, int w, int h)
{
	if ((w <= 0) || (h <= 0))
		return NULL;

	_Pcx16_* pcx16 = (_Pcx16_*)o_New(56);
	if (pcx16 == NULL) return NULL;

	pcx16->v_table = (_ptr_*)0x63B9C8;
	pcx16->ref_count = 0;
	MemCopy(pcx16->name, n, 12);
	pcx16->width = w;
	pcx16->height = h;
	if (o_BPP == 32)
		pcx16->scanline_size = ((((w * 32) + 31) & ~31) >> 3);
	else // == 16
		pcx16->scanline_size = ((((w * 16) + 31) & ~31) >> 3);
	pcx16->buf_size = pcx16->pic_size = pcx16->scanline_size * h;
	pcx16->have_ddsurface_buffer = 0;
	pcx16->buffer = o_New(pcx16->buf_size);
	if (pcx16->buffer == NULL)
	{
		o_Delete((_ptr_)pcx16);
		return NULL;
	}

	return pcx16;
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

void _Pcx16_::Sharpen(double z)
{
	double mask[] = { 0,   -z  ,   0,
					 -z,  1 + 4 * z,  -z,
					  0,   -z  ,   0 };

	_Pcx16_* src = _Pcx16_::CreateNew((char*)"", width, height);
	MemCopy(src->buffer, buffer, buf_size);

	_byte_* src_bits = (_byte_*)src->buffer;
	_byte_* dst_bits = (_byte_*)this->buffer;

	if (o_BPP == 32)
	{
		for (int y = 1; y < height - 1; y++)
		{
			for (int x = 1; x < width - 1; x++)
			{
				double linc_r = 0, linc_g = 0, linc_b = 0;

				for (int sy = 0; sy <= 2; sy++)
				{
					for (int sx = 0; sx <= 2; sx++)
					{
						_byte_* s = src_bits + (x + sx - 1) * 4 + (y + sy - 1) * scanline_size;
						linc_r += (s[2] * mask[sx + sy * 3]);
						linc_g += (s[1] * mask[sx + sy * 3]);
						linc_b += (s[0] * mask[sx + sy * 3]);
					}
				}

				_byte_* d = dst_bits + x * 4 + y * scanline_size;
				d[2] = (byte)min(255, max(0, linc_r));
				d[1] = (byte)min(255, max(0, linc_g));
				d[0] = (byte)min(255, max(0, linc_b));
			}
		}
	}
	else
	{
		for (int y = 1; y < height - 1; y++)
		{
			for (int x = 1; x < width - 1; x++)
			{
				double linc_r = 0, linc_g = 0, linc_b = 0;

				for (int sy = 0; sy <= 2; sy++)
				{
					for (int sx = 0; sx <= 2; sx++)
					{
						// _dword_ sc = XRGB8888_fromRGB565(WordAt(src_bits + (x+sx-1)*2 + (y+sy-1)* scanline_size));
						//_byte_* s = (_byte_*)&sc;
						//linc_r +=( s[2] * mask[sx + sy*3] );
						//linc_g +=( s[1] * mask[sx + sy*3] );
						//linc_b +=( s[0] * mask[sx + sy*3] );
						_word_ s = WordAt(src_bits + (x + sx - 1) * 2 + (y + sy - 1) * scanline_size);
						linc_r += (RGB565_R(s) * mask[sx + sy * 3]);
						linc_g += (RGB565_G(s) * mask[sx + sy * 3]);
						linc_b += (RGB565_B(s) * mask[sx + sy * 3]);
					}
				}

				//WordAt(dst_bits + x*2 + y*scanline_size) = RGB565_fromR8G8B8((byte)min(255, max(0, linc_r)),
					  //													  (byte)min(255, max(0, linc_g)), 
					  //													  (byte)min(255, max(0, linc_b)));
				WordAt(dst_bits + x * 2 + y * scanline_size) = RGB565_fromR5G6B5(min(rgb565_r_mask >> 11, max(0, linc_r)),
					min(rgb565_g_mask >> 5, max(0, linc_g)),
					min(rgb565_b_mask, max(0, linc_b)));
			}
		}
	}

	o_Delete(src);
	//_Pcx16_::Delete(src);
}


void _Pcx16_::DrawPcx16ResizedBicubic(_Pcx16_* src_pcx, int s_w, int s_h, int d_x, int d_y, int d_w, int d_h)
{

	if (o_BPP == 32)
	{
		unsigned src_pitch = src_pcx->scanline_size;
		unsigned dst_pitch = this->scanline_size;
		BYTE* src_bits = (BYTE*)src_pcx->buffer;
		BYTE* dst_bits = (BYTE*)this->buffer;
		BYTE* linek, * lined;
		unsigned d;
		double xFactor = (double)s_w / d_w;
		double yFactor = (double)s_h / d_h;
		// coordinates of source points and coefficients
		double  ox, oy, dx, dy, k1, k2;
		int     ox1, oy1, ox2, oy2;

		// destination pixel values
		double* p_g = new double[3];

		// s_w and s_h decreased by 1
		int ymax = s_h - 1;
		int xmax = s_w - 1;


		for (int y = 0; y < d_h; y++)
		{
			// Y coordinates
			oy = (double)y * yFactor + 0.5;
			oy1 = (int)oy;
			dy = oy - (double)oy1;

			lined = dst_bits + (y + d_y) * dst_pitch;

			for (int x = 0; x < d_w; x++)
			{
				// X coordinates
				ox = (double)x * xFactor + 0.5;
				ox1 = (int)ox;
				dx = ox - (double)ox1;

				// initial pixel value
				MemSet(p_g, 0, sizeof(double) * 3);

				for (int n = -1; n < 3; n++)
				{
					// get Y coefficient
					k1 = BiCubicKernel(dy - (double)n);

					oy2 = oy1 + n;
					if (oy2 < 0)
						oy2 = 0;
					if (oy2 > ymax)
						oy2 = ymax;

					linek = src_bits + oy2 * src_pitch;

					for (int m = -1; m < 3; m++)
					{
						// get X coefficient
						k2 = k1 * BiCubicKernel((double)m - dx);

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
				lined[(x + d_x) * 4 + 2] = (BYTE)p_g[2];
				lined[(x + d_x) * 4 + 1] = (BYTE)p_g[1];
				lined[(x + d_x) * 4 + 0] = (BYTE)p_g[0];
			}
		}

		delete[] p_g;
	}
	else
	{
		unsigned src_pitch = src_pcx->scanline_size;
		unsigned dst_pitch = this->scanline_size;
		BYTE* src_bits = (BYTE*)src_pcx->buffer;
		BYTE* dst_bits = (BYTE*)this->buffer;
		BYTE* linek, * lined;
		unsigned d;
		double xFactor = (double)s_w / d_w;
		double yFactor = (double)s_h / d_h;
		// coordinates of source points and coefficients
		double  ox, oy, dx, dy, k1, k2;
		int     ox1, oy1, ox2, oy2;

		// destination pixel values
		double* p_g = new double[3];

		// s_w and s_h decreased by 1
		int ymax = s_h - 1;
		int xmax = s_w - 1;


		for (int y = 0; y < d_h; y++)
		{
			// Y coordinates
			oy = (double)y * yFactor - 0.5;
			oy1 = (int)oy;
			dy = oy - (double)oy1;

			lined = dst_bits + (y + d_y) * dst_pitch;

			for (int x = 0; x < d_w; x++)
			{
				// X coordinates
				ox = (double)x * xFactor - 0.5f;
				ox1 = (int)ox;
				dx = ox - (double)ox1;

				// initial pixel value
				MemSet(p_g, 0, sizeof(double) * 3);

				for (int n = -1; n < 3; n++)
				{
					// get Y coefficient
					k1 = BiCubicKernel(dy - (double)n);

					oy2 = oy1 + n;
					if (oy2 < 0)
						oy2 = 0;
					if (oy2 > ymax)
						oy2 = ymax;

					linek = src_bits + oy2 * src_pitch;

					for (int m = -1; m < 3; m++)
					{
						// get X coefficient
						k2 = k1 * BiCubicKernel((double)m - dx);

						ox2 = ox1 + m;
						if (ox2 < 0)
							ox2 = 0;
						if (ox2 > xmax)
							ox2 = xmax;


						_dword_ ck = XRGB8888_fromRGB565(WordAt(linek + ox2 * 2));
						p_g[2] += k2 * ((_byte_*)&ck)[2];
						p_g[1] += k2 * ((_byte_*)&ck)[1];
						p_g[0] += k2 * ((_byte_*)&ck)[0];
					}
				}

				WordAt(lined + (x + d_x) * 2) = RGB565_fromR8G8B8((_byte_)p_g[2], (_byte_)p_g[1], (_byte_)p_g[0]);
			}
		}

		delete[] p_g;
	}
}