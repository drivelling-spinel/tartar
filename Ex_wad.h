// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 2022 Ludicrous_peridot
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
//      Service interface for W_Wad.
//
//-----------------------------------------------------------------------------

#ifndef __EX_WAD__
#define __EX_WAD__


int Ex_DynamicLumpFilterProc(lumpinfo_t * lump, int lumpnum, char * wadname, const extra_file_t extra);
void Ex_DynamicLumpsInWad(int handle, int start, int count, extra_file_t extra);
void Ex_DynamicLumpCoalesceProc(lumpinfo_t * lump, int oldnum, int newnum);
void Ex_InitDynamicLumpNames();
int Ex_SetDefaultDynamicLumpNames();

#endif
