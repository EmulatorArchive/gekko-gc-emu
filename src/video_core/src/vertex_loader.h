/*!
* Copyright (C) 2005-2012 Gekko Emulator
*
* \file    vertex_loader.cpp
* \author  ShizZy <shizzy247@gmail.com>
* \date    2012-03-10
* \brief   Loads and decodes vertex data from CP mem
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

#ifndef VIDEO_CORE_VERTEX_LOADER_H_
#define VIDEO_CORE_VERTEX_LOADER_H_

#include "common.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Graphics Processor namespace

namespace gp {

extern f32* g_position_burst_ptr;
extern f32  g_position_burst_buffer[0x10000]; // TODO(ShizZy): Find a good size for this

extern f32* g_color_burst_ptr;
extern f32  g_color_burst_buffer[0x10000]; // TODO(ShizZy): Find a good size for this

/*!
 * \brief Decode a primitive type
 * \param type Type of primitive (e.g. points, lines, triangles, etc.)
 * \param count Number of vertices
 * \param vat Vertex attribute table
 */
void DecodePrimitive(int type, int count, u8 vat); 

/// Initialize the Vertex Loader
void VertexLoaderInit();

/// Shutdown the Vertex Loader
void VertexLoaderShutdown();

} // namespace

#endif // VIDEO_CORE_VERTEX_LOADER_H_