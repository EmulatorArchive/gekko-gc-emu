/*!
* Copyright (C) 2005-2012 Gekko Emulator
*
* \file    xf_mem.h
* \author  ShizZy <shizzy247@gmail.com>
* \date    2012-03-12
* \brief   Implementation of CXF for the graphics processor
*
* \section LICENSE
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

#include <GL/glew.h>

#include "common.h"
#include "memory.h"

#include "video_core.h"

#include "xf_mem.h"
#include "cp_mem.h"

#define XF_VIEWPORT_ZMAX            16777215.0f

////////////////////////////////////////////////////////////////////////////////////////////////////
// Graphics Processor namespace

namespace gp {

u32         g_tf_mem[0x800];    ///< Transformation memory
XFMemory    g_xf_regs;          ///< XF registers

/// Write data into a XF register
void XFRegisterWrite(u16 length, u16 addr, u32 regs[64]) {
    int i;

    // write data to xf memory/registers
    switch((addr & XF_ADDR_MASK) >> 8) {
    case 0x00:
    case 0x01:
    case 0x02:
    case 0x03: // matrix transformation memory
    case 0x04: // normal transformation memory
    case 0x05: // dual texture transformation memory
    case 0x06: 
    case 0x07: // light transformation memory
        for (i = 0; i < length; i++){		
            ((f32*)g_tf_mem)[addr + i] = toFLOAT(regs[i]);
        }

        break;

    case 0x10: // registers
        u8 maddr = (addr & 0xff);
        for (i = 0; i <length; i++) {
            g_xf_regs.mem[maddr + i] =regs[i];
        }

        switch(maddr) {
        case 0x1a:
        case 0x1b:
        case 0x1d:
        case 0x1e:
            double znear, zfar;
            int x, y, width, height;

            // Decode z far distance
            zfar = ((GLclampd)toFLOAT(XF_VIEWPORT_OFFSET_Z) / XF_VIEWPORT_ZMAX);

            // Decode z near distance
            znear = (-((GLclampd)toFLOAT(XF_VIEWPORT_SCALE_Z) / 
                (GLclampd)XF_VIEWPORT_ZMAX) + zfar);

            // Decode viewport dimensions
            width = (int)(toFLOAT(XF_VIEWPORT_SCALE_X) * 2);
            height = (int)(-toFLOAT(XF_VIEWPORT_SCALE_Y) * 2);
            x = (int)(toFLOAT(XF_VIEWPORT_OFFSET_X) - (342 + toFLOAT(XF_VIEWPORT_SCALE_X)));
            y = (int)(toFLOAT(XF_VIEWPORT_OFFSET_Y) - (342 - toFLOAT(XF_VIEWPORT_SCALE_Y)));

            // Send to renderer
            video_core::g_renderer->SetViewport(x, y, width, height);
            video_core::g_renderer->SetDepthRange(znear, zfar);
            break;

        case 0x20:
	        t32 mtx[16] = {0};

	        // Set orthographic mode...
	        if(XF_PROJECTION_ORTHOGRAPHIC) { 
		        mtx[0]._u32 = XF_PROJECTION_A;
		        mtx[5]._u32 = XF_PROJECTION_C;
		        mtx[10]._u32 = XF_PROJECTION_E;
		        mtx[12]._u32 = XF_PROJECTION_B;
		        mtx[13]._u32 = XF_PROJECTION_D;
		        mtx[14]._u32 = XF_PROJECTION_F;
		        mtx[15]._f32 = 1.0f;
	        // Set perspective mode
	        }else{ 
		        mtx[0]._u32 = XF_PROJECTION_A;
		        mtx[5]._u32 = XF_PROJECTION_C;
		        mtx[8]._u32 = XF_PROJECTION_B;
		        mtx[9]._u32 = XF_PROJECTION_D;
		        mtx[10]._u32 = XF_PROJECTION_E;
		        mtx[11]._f32 = -1.0f;
		        mtx[14]._u32 = XF_PROJECTION_F;
	        }
            break;
        }
        break;
    }
}

/// Write data into a XF register indexed-form
void XFLoadIndexed(u8 n, u16 index, u8 length, u16 addr) {
    for (int i = 0; i < length; i++) {
        g_tf_mem[addr + i] = Memory_Read32(CP_IDX_ADDR(index, n) + (i * 4));
    }
}

} // namespace