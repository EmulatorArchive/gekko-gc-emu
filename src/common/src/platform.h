/*!
 * Copyright (C) 2005-2012 Gekko Emulator
 *
 * \file    platform.h
 * \author  ShizZy <shizzy247@gmail.com>
 * \date    2012-02-11
 * \brief   Platform detection macros for portable compilation
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

#ifndef COMMON_PLATFORM_H_
#define COMMON_PLATFORM_H_

////////////////////////////////////////////////////////////////////////////////////////////////////
// Platform definitions

/// Enumeration for defining the supported platforms
#define PLATFORM_NULL 0
#define PLATFORM_WINDOWS 1
#define PLATFORM_MACOSX 2
#define PLATFORM_LINUX 3
#define PLATFORM_ANDROID 4
#define PLATFORM_IOS 5

////////////////////////////////////////////////////////////////////////////////////////////////////
// Platform detection
extern char *kGekkoOS;

#ifndef EMU_PLATFORM

#if defined( __WIN32__ ) || defined( _WIN32 )
#define EMU_PLATFORM PLATFORM_WINDOWS

#elif defined( __APPLE__ ) || defined( __APPLE_CC__ )
#define EMU_PLATFORM PLATFORM_MAXOSX

#elif defined(__linux__)
#define EMU_PLATFORM PLATFORM_LINUX

#else // Assume linux otherwise
#define EMU_PLATFORM PLATFORM_LINUX

#endif

#endif

#if defined(__x86_64__) || defined(_M_X64) || defined(__alpha__) || defined(__ia64__)
#define EMU_ARCHITECTURE_X64
#else
#define EMU_ARCHITECTURE_X86
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
// Compiler-Specific Definitions

#if EMU_PLATFORM == PLATFORM_WINDOWS

#define EMU_FASTCALL __fastcall

#else

#define EMU_FASTCALL __attribute__((fastcall))
#define __stdcall
#define __cdecl

#define sprintf_s               _snprintf           ///< Unsafe but only available on Windows
#define strcpy_s(dst, len, src) strcpy(dst, src)    ///< Unsafe but only available on Windows
#define strcat_s(dst, len, src) strcat(dst, src)    ///< Unsafe but only available on Windows

#define LONG long
#define BOOL bool
#define DWORD u32

#endif

#if EMU_PLATFORM != PLATFORM_WINDOWS

// TODO: Hacks..
#include <limits.h>
#define MAX_PATH PATH_MAX

#include <strings.h>
#define stricmp(str1, str2) strcasecmp(str1, str2)
#define _stricmp(str1, str2) strcasecmp(str1, str2)
#define _snprintf snprintf
#define _getcwd getcwd
#define _tzset tzset

// TODO: EXCEPTION_POINTERS wasn't defined anywhere...
typedef void EXCEPTION_POINTERS;

#define EMU_IGNORE_RECOMPILER

#endif

#endif // COMMON_PLATFORM_H_
