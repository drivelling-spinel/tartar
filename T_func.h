// Emacs style mode select -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2000 Simon Howard
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
#ifndef __FUNC_H__
#define __FUNC_H__

// haleyjd: these two should cover the below...
#include "m_fixed.h"
#include "tables.h"
#include "t_parse.h"

void init_functions();
// haleyjd: SoM's fixed-point angle conversion macros

#define AngleToFixed(x)  (((double) x) / ((double) ANG45/45)) * FRACUNIT
#define FixedToAngle(x)  (((double) x) / FRACUNIT) * ANG45/45;

#endif

// EOF