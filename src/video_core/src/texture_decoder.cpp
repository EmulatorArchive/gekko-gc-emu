/**
 * Copyright (C) 2005-2012 Gekko Emulator
 *
 * @file    texture_decoder.cpp
 * @author  ShizZy <shizzy247@gmail.com>
 * @date    2012-06-01
 * @brief   Simple texture decoder for GameCube graphics texture formats
 *
 * @section LICENSE
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * Official project repository can be found at:
 * http://code.google.com/p/gekko-gc-emu/
 */

#include "sys/stat.h"
#include "common.h"
#include "config.h"
#include "memory.h"
#include "hw/hw_gx.h"
#include "bp_mem.h"
#include "texture_decoder.h"
#include "video_core.h"
#include "fifo_player.h"

#include <vector>
using namespace std;

namespace gp {

////////////////////////////////////////////////////////////////////////////////////////////////////

u8 tmem[TMEM_SIZE];

////////////////////////////////////////////////////////////////////////////////////////////////////
// TEXTURE FORMAT DECODING

static inline u32 __decode_col_rgb5a3(u16 _data) {
    u8 r, g, b, a;

    if (_data & SIGNED_BIT16) // rgb5
    {
        r = (u8)(255.0f * (((_data >> 10) & 0x1f) / 32.0f));
        g = (u8)(255.0f * (((_data >> 5) & 0x1f) / 32.0f));
        b = (u8)(255.0f * ((_data & 0x1f) / 32.0f));
        a = 0xff;	
    }else{ // rgb4a3
        r = 17 * ((_data >> 8) & 0xf);
        g = 17 * ((_data >> 4) & 0xf);
        b = 17 * (_data & 0xf);
        a = (u8)(255.0f * (((_data >> 12) & 7) / 8.0f));
    }

    return (a << 24) | (b << 16) | (g << 8) | r;
}

static inline u32 __decode_col_rgb565(u16 _data) {
    u32 r, g, b;
    r = (_data >> 11) << 3;
    g = _data & 0x7E0; 
    b = _data & 0x1F;
    return 0xff000000 | (b << 19) | (g << 5) | r;
}

static inline void DecodeDtxBlock(const u8 *_src, u32 *_dst, u32 _width) {
    u16 color1 = *(u16*)(_src + 2);
    u16 color2 = *(u16*)(_src);
    u32 bits = *(u32*)(_src + 4);

    // Prepare color table
    u32 table[4];
    table[0] = __decode_col_rgb565(color1);
    table[1] = __decode_col_rgb565(color2);

    if (color1 > color2) {
        table[2] =
            ((((2*((table[0] >>  0) & 0xff) + ((table[1] >>  0) & 0xff))/3) & 0xFF) <<  0) |    // R
            ((((2*((table[0] >>  8) & 0xff) + ((table[1] >>  8) & 0xff))/3) & 0xFF) <<  8) |    // G
            ((((2*((table[0] >> 16) & 0xff) + ((table[1] >> 16) & 0xff))/3) & 0xFF) << 16) |    // B
            0xff000000;

        table[3] =
            ((((2*((table[1] >>  0) & 0xff) + ((table[0] >>  0) & 0xff))/3) & 0xFF) <<  0) |    // R
            ((((2*((table[1] >>  8) & 0xff) + ((table[0] >>  8) & 0xff))/3) & 0xFF) <<  8) |    // G
            ((((2*((table[1] >> 16) & 0xff) + ((table[0] >> 16) & 0xff))/3) & 0xFF) << 16) |    // B
            0xff000000;
    } else {
        table[2] =
            ((((((table[0] >>  0) & 0xff) + ((table[1] >>  0) & 0xff)) / 2) & 0xFF) <<  0) |    // R
            ((((((table[0] >>  8) & 0xff) + ((table[1] >>  8) & 0xff)) / 2) & 0xFF) <<  8) |    // G
            ((((((table[0] >> 16) & 0xff) + ((table[1] >> 16) & 0xff)) / 2) & 0xFF) << 16) |    // B
            0xff000000;

        table[3] = 0x00000000;		//alpha
    }
    // Decode image 4x4 layout
    for (int iy = 3; iy >= 0; iy--) {
        _dst[(iy * _width) + 0] = table[(bits >> 6) & 0x3];
        _dst[(iy * _width) + 1] = table[(bits >> 4) & 0x3];
        _dst[(iy * _width) + 2] = table[(bits >> 2) & 0x3];
        _dst[(iy * _width) + 3] = table[(bits >> 0) & 0x3];
        bits >>= 8;
    }
}

static inline void DecompressDxt1(u32* _dst, const u8* _src, int _width, int _height) {
    u8*	runner = (u8 *)_src;

    //#pragma omp for ordered schedule(dynamic)
    for (int y = 0; y < _height; y += 8) {
        for (int x = 0; x < _width; x += 8) {

            //#pragma omp ordered
            DecodeDtxBlock(runner, &_dst[(y*_width)+x], _width);
            runner += 8;
            DecodeDtxBlock(runner, &_dst[(y*_width)+x+4], _width);
            runner += 8;
            DecodeDtxBlock(runner, &_dst[((y+4)*_width)+x], _width);
            runner += 8;
            DecodeDtxBlock(runner, &_dst[((y+4)*_width)+x+4], _width);
            runner += 8;
        }
    }
}

void unpackPixel(int _idx, u8* _dst, u16* _palette, u8 _fmt) {
    //note, the _palette pointer is to Mem_RAM which is
    //already swapped around for normal 32bit reads
    //we have to swap which of the 16bit entries we read
    _idx ^= 1;

    switch(_fmt) {
    case 0: // ia8
        _dst[0] = _palette[_idx] >> 8; //& 0xFF;
        _dst[1] = _palette[_idx] >> 8; //& 0xFF;
        _dst[2] = _palette[_idx] >> 8; //& 0xFF;
        _dst[3] = _palette[_idx] & 0xFF;
        break;

    case 1: // rgb565

        *((u32*)_dst) = __decode_col_rgb565(_palette[_idx]);
        break;

    case 2: // rgb5a3

        *((u32*)_dst) = __decode_col_rgb5a3(_palette[_idx]);
        break;
    }
}

void unpack8(u8* dst, u8* src, int w, int h, u16* palette, u8 paletteFormat, int neww)
{
    u8* runner = dst;

    //printf("fmt: %d", paletteFormat);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < neww; ++x, runner += 4)
            unpackPixel(src[y*w + x], runner, palette, paletteFormat);
}

/**
 * Get the size of a texture
 * @param format Format of the texture
 * @param width Width in pixels of the texture
 * @param height Height in pixels of the texture
 */
size_t TextureDecoder_GetSize(TextureFormat format, int width, int height) {
    switch (format) {
    case kTextureFormat_Intensity4:
        return (width * height) / 2;
    case kTextureFormat_Intensity8:
        return (width * height);
    case kTextureFormat_IntensityAlpha4:
        return (width * height);
    case kTextureFormat_IntensityAlpha8:
        return (width * height) * 2;
    case kTextureFormat_RGB565:
        return (width * height) * 2;
    case kTextureFormat_RGB5A3:
        return (width * height) * 2;
    case kTextureFormat_RGBA8:
        return (width * height) * 4;
    case kTextureFormat_C4:
        return (width * height) / 2;
    case kTextureFormat_C8:
        return (width * height);
    case kTextureFormat_C14X2:
        return (width * height) * 2;
    case kTextureFormat_CMPR:
        return (width * height);
    }
    _ASSERT_MSG(TGP, 0, "Unknown texture format for TextureDecoder_GetSize %d!", format);
    return 0;
}

/**
 * Decode a texture to RGBA8 format
 * @param format Format of the source texture
 * @param width Width in pixels of the texture
 * @param height Height in pixels of the texture
 * @param src Source data buffer of texture to decode
 * @param dst Destination data buffer for decoded RGBA8 texture
 */
void TextureDecoder_Decode(TextureFormat format, int width, int height, const u8* src, u8* dst) {
    int	x = 0, y = 0, dx = 0, dy = 0, i = 0, j = 0;
    u32 val = 0;
    int _8_width = (width + 7) & ~7;
    int _4_width = (width + 3) & ~3;
    u8 *dst8 = new u8[width * height * 32];
    u32 *dst32 = (u32*)dst8;
    const u16 *src16 = (const u16*)src;

    // TODO(Neobrain): Do something with me...
    //if (fifo_player::IsRecording()) {
    //    fifo_player::MemUpdate(addr, src8, width * height * 4); // TODO: Use proper size!
    //}

    u8 pallette_fmt = (gp::g_bp_regs.mem[0x98] >> 10) & 3;
    u32 pallette_addr = ((gp::g_bp_regs.mem[0x98] & 0x3ff) << 5);
    u16	*pal16 = ((u16*)&gp::tmem[pallette_addr & TMEM_MASK]);

    switch(format) {
    case kTextureFormat_Intensity4:
        for (y = 0; y < height; y += 8) {
            for (x = 0; x < _8_width; x += 8) {
                for (dy = 0; dy < 8; dy++) {
                    for (dx = 0; dx < 8; dx+=2, src++) {
                        // Convert 2 4-bit instensity pixels to 32-bit RGBA
						val = ((*(u8 *)((uintptr_t)src ^ 3) & 0x0f)) * 0x11111111;
						dst32[(_8_width * (y + dy) + x + dx + 1)] = val;
						val = ((*(u8 *)((uintptr_t)src ^ 3) & 0xf0) >> 4) * 0x11111111;
						dst32[(_8_width * (y + dy) + x + dx)] = val;
 					}
                }
            }
        }
        for (y = 0; y < height; y++) {
            memcpy(&dst[y*width*4], &dst8[y*_8_width*4], width*4);
        }
        break;

    case kTextureFormat_Intensity8:
        for (y = 0; y < height; y += 4) {
            for (x = 0; x < width; x += 8) {
                for (dy = 0; dy < 4; dy++) {
                    for (dx = 0; dx < 8; dx++) {
                        // Converts one 8-bit instensity pixel to 32-bit RGBA
						val = (*(u8 *)((uintptr_t)src ^ 3)) * 0x1010101;
						dst32[(_8_width * (y + dy) + x + dx)] = val;
                        src++;
					}
                }
            }
        }
        for (y=0; y < height; y++) {
            memcpy(&dst[y*width*4], &dst8[y*_8_width*4], width*4);
        }
        break;

    case kTextureFormat_IntensityAlpha4:
        for (y = 0; y < height; y += 4) {
            for (x = 0; x < _8_width; x += 8) {
                for (dy = 0; dy < 4; dy++) {
                    for (dx = 0; dx < 8; dx++, src++) {
		 				val = (((*(u8 *)((uintptr_t)src ^ 3) & 0x0f)) * 0x111111) | 
                            ((17 * ((*(u8 *)((uintptr_t)src ^ 3) & 0xf0) >> 4)) << 24);
		 				dst32[(_8_width * (y + dy) + x + dx)] = val;
 		 			}
                }
            }
        }
        for (y=0; y < height; y++) {
            memcpy(&dst[y*width*4], &dst8[y*_8_width*4], width*4);
        }
		break;

    case kTextureFormat_IntensityAlpha8:
        for (y = 0; y < height; y += 4) {
            for (x = 0; x < _4_width; x += 4) {
                for (dy = 0; dy < 4; dy++) {
                    for (dx = 0; dx < 4; dx++, src+=2) {
						val = (((u32)*(u8 *)(((uintptr_t)src + 1) ^ 3)) * 0x00010101) | 
                            ((u32)*(u8 *)((uintptr_t)src ^ 3) << 24);
						dst32[(_4_width * (y + dy) + x + dx)] = val;
					}
                }
            }
        }
        for (y=0; y < height; y++) {
            memcpy(&dst[y*width*4], &dst8[y*_4_width*4], width*4);
        }
		break;

    case kTextureFormat_RGB565:
		for (y = 0; y < height; y += 4) {
			for (x = 0; x < _4_width; x += 4) {
				for (dy = 0; dy < 4; dy++) {
					for (dx = 0; dx < 4; dx++) {
						// memory is not already swapped.. use this to grab high word first
						j ^= 1; 
						// decode color
						dst32[_4_width * (y + dy) + x + dx] = __decode_col_rgb565((*((u16*)(src16 + j))));
						// only increment every other time otherwise you get address doubles
						if (!j) src16 += 2; 
					}
                }
            }
        }
        for (y=0; y < height; y++) {
            memcpy(&dst[y*width*4], &dst8[y*_4_width*4], width*4);
        }
        break;

    case kTextureFormat_RGB5A3:
		for (y = 0; y < height; y += 4) {
			for (x = 0; x < _4_width; x += 4) {
				for (dy = 0; dy < 4; dy++) {
					for (dx = 0; dx < 4; dx++) {
						// memory is not already swapped.. use this to grab high word first
						j ^= 1;
						// decode color
						dst32[_4_width * (y + dy) + x + dx] = __decode_col_rgb5a3((*((u16*)(src16 + j))));
						// only increment every other time otherwise you get address doubles
						if (!j) src16 += 2;
					}
                }
            }
        }
		for (y=0; y < height; y++) {
            memcpy(&dst[y*width*4], &dst8[y*_4_width*4], width*4);
        }
		break;

    case kTextureFormat_RGBA8: // rgba8
		for (y = 0; y < height; y += 4) {
			for (x = 0; x < _4_width; x += 4) {
				for (dy = 0; dy < 4; dy++) {
					for (dx = 0; dx < 4; dx++) {
						// memory is not already swapped.. use this to grab high word first
						j ^= 1;
						// fetch data
						dst32[_4_width * (y + dy) + x + dx] = ((*((u16*)(src16 + j))) << 16);
						// only increment every other time otherwise you get address doubles
						if (!j) src16 += 2;
					}
                }
				for (dy = 0; dy < 4; dy++) {
					for (dx = 0; dx < 4; dx++) {
						// memory is not already swapped.. use this to grab high word first
						j ^= 1;
						// fetch color data
						val = dst32[_4_width * (y + dy) + x + dx] | ((*((u16*)(src16 + j))));
						// decode color data
						dst32[_4_width * (y + dy) + x + dx] = (val & 0xff00ff00) | 
										((val & 0xff0000) >> 16) | 
										((val & 0xff) << 16);
						// only increment every other time otherwise you get address doubles
						if (!j) src16 += 2;
					}
                }
			}
        }
		for (y=0; y < height; y++) {
            memcpy(&dst[y*width*4], &dst8[y*_4_width*4], width*4);
        }
		break;

    case kTextureFormat_C4:
        for (y = 0; y < height; y += 8) {
            for (x = 0; x < _8_width; x += 8) {
                for (dy = 0; dy < 8; dy++) {
                    dst8[_8_width * (y + dy) + x + 0] = (*(u8 *)((uintptr_t)src ^ 3) >> 4);
                    dst8[_8_width * (y + dy) + x + 1] = (*(u8 *)((uintptr_t)src ^ 3) & 0x0f);
                    src++;

                    dst8[_8_width * (y + dy) + x + 2] = (*(u8 *)((uintptr_t)src ^ 3) >> 4);
                    dst8[_8_width * (y + dy) + x + 3] = (*(u8 *)((uintptr_t)src ^ 3) & 0x0f);
                    src++;

                    dst8[_8_width * (y + dy) + x + 4] = (*(u8 *)((uintptr_t)src ^ 3) >> 4);
                    dst8[_8_width * (y + dy) + x + 5] = (*(u8 *)((uintptr_t)src ^ 3) & 0x0f);
                    src++;

                    dst8[_8_width * (y + dy) + x + 6] = (*(u8 *)((uintptr_t)src ^ 3) >> 4);
                    dst8[_8_width * (y + dy) + x + 7] = (*(u8 *)((uintptr_t)src ^ 3) & 0x0f);
                    src++;
                }
            }
        }
        unpack8(dst, dst8, _8_width, height, pal16, pallette_fmt, width);
        break;

    case kTextureFormat_C8:
        for (y = 0; y < height; y += 4) {
            for (x = 0; x < _8_width; x += 8) {
                *(u32 *)&dst8[_8_width * (y + 0) + x] = BSWAP32(*(u32 *)((uintptr_t)src));
                *(u32 *)&dst8[_8_width * (y + 0) + x + 4] = BSWAP32(*(u32 *)((uintptr_t)src + 4));

                *(u32 *)&dst8[_8_width * (y + 1) + x] = BSWAP32(*(u32 *)((uintptr_t)src + 8));
                *(u32 *)&dst8[_8_width * (y + 1) + x + 4] = BSWAP32(*(u32 *)((uintptr_t)src + 12));

                *(u32 *)&dst8[_8_width * (y + 2) + x] = BSWAP32(*(u32 *)((uintptr_t)src + 16));
                *(u32 *)&dst8[_8_width * (y + 2) + x + 4] = BSWAP32(*(u32 *)((uintptr_t)src + 20));

                *(u32 *)&dst8[_8_width * (y + 3) + x] = BSWAP32(*(u32 *)((uintptr_t)src + 24));
                *(u32 *)&dst8[_8_width * (y + 3) + x + 4] = BSWAP32(*(u32 *)((uintptr_t)src + 28));
                src+=32;
            }
        }
        unpack8(dst, dst8, _8_width, height, pal16, pallette_fmt, width);
        break;

    case kTextureFormat_CMPR:
        DecompressDxt1(dst32, src, _8_width, height);
        for (y=0; y < height; y++) {
            memcpy(&dst[y*_8_width*4], &dst8[y*_8_width*4], _8_width*4);
        }
        break;

    default:
        LOG_ERROR(TGP, "Unsupported texture format %d!", format);
        break;
    }
    delete dst8;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//

} // namespace
