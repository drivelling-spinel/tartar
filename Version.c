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
//------------------------------------------------------------------------------
//
// $Id: version.c,v 1.2 1998/05/03 22:59:31 killough Exp $
//
// Description: Version ID constants
//
//-----------------------------------------------------------------------------

static const char rcsid[] = "$Id: version.c,v 1.2 1998/05/03 22:59:31 killough Exp $";

#include "version.h"

// sf: made int from define
int VERSION = 329;

// haleyjd: subversion -- range from 0 to 255
unsigned char SUBVERSION = 5;

const char version_date[] = __DATE__;
const char version_time[] = __TIME__; // haleyjd

// sf: version name -- at the suggestion of mystican
const char version_name[] = "development beta 5 joel-2";
                                             
// haleyjd: Eternity release history
// private alpha	-- 3.29 private       09/14/00
// public beta   	-- 3.29 public beta 1 01/08/01
// public beta   	-- 3.29 public beta 2 01/09/01 - fixed mouse buttons
// public beta          -- 3.29 public beta 3 05/10/01
// public beta		-- 3.29 public beta 4 06/30/01

//----------------------------------------------------------------------------
//
// $Log: version.c,v $
// Revision 1.2  1998/05/03  22:59:31  killough
// beautification
//
// Revision 1.1  1998/02/02  13:21:58  killough
// version information files
//
//----------------------------------------------------------------------------
