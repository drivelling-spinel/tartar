// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2000 James Haley
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//--------------------------------------------------------------------------
//
// DESCRIPTION:
//      Simple basic typedefs, isolated here to make it easier
//       separating modules.
//
//-----------------------------------------------------------------------------


#ifndef __DOOMTYPE__
#define __DOOMTYPE__

#ifndef __BYTEBOOL__
#define __BYTEBOOL__
// Fixed to use builtin bool type with C++.
#ifdef __cplusplus
typedef bool boolean;
#else
typedef enum {false, true} boolean;
#endif
typedef unsigned char byte;
#endif

// haleyjd 09/21/01: early work on portability issues for Windows --
// need to get rid of use of compiler-specific long long type

// this must be defined for the platform, so add yours if its not
// here already

// hint: keep MSVC first and the right type will show up in
//       the IDE :)

#if defined(_MSC_VER)   // MSVC
  #define __LONG64_TYPE__ __int64
#elif defined(__GNUC__) // GNU C
  #define __LONG64_TYPE__ long long
#else
  #error Need a 64-bit integer type for this platform!
#endif

typedef unsigned __LONG64_TYPE__ ULong64;
typedef __LONG64_TYPE__ Long64;

// SoM: resolve platform-specific range symbol issues

#if defined(_MSC_VER)
  #include <limits.h>
  #define D_MAXINT INT_MAX
  #define D_MININT INT_MIN
  #define D_MAXSHORT  SHRT_MAX
#elif defined(__GNUC__)
  #include <values.h>
  #define D_MAXINT MAXINT
  #define D_MININT MININT
  #define D_MAXSHORT MAXSHORT
#else
  #error Need datatype range values for this platform.
#endif

#define MAXCHAR         ((char)0x7f)
#define MINCHAR         ((char)0x80)
#endif

//----------------------------------------------------------------------------
//
// $Log: doomtype.h,v $
// Revision 1.3  1998/05/03  23:24:33  killough
// beautification
//
// Revision 1.2  1998/01/26  19:26:43  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:02:51  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
