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
//      LineOfSight/Visibility checks, uses REJECT Lookup Table.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: p_sight.c,v 1.7 1998/05/07 00:55:55 killough Exp $";

#include "doomstat.h"
#include "r_main.h"
#include "p_maputl.h"
#include "p_setup.h"
#include "m_bbox.h"

//
// P_CheckSight
//
// killough 4/19/98:
// Convert LOS info to struct for reentrancy and efficiency of data locality

typedef struct {
  fixed_t sightzstart, t2x, t2y;   // eye z of looker
  divline_t strace;                // from t1 to t2
  fixed_t topslope, bottomslope;   // slopes to top and bottom of target
  fixed_t bbox[4];
} los_t;

//
// P_DivlineSide
// Returns side 0 (front), 1 (back), or 2 (on).
//
// killough 4/19/98: made static, cleaned up
// killough 12/98: made external

int P_DivlineSide(fixed_t x, fixed_t y, const divline_t *node)
{
  fixed_t left, right;
  return
    !node->dx ? x == node->x ? 2 : x <= node->x ? node->dy > 0 : node->dy < 0 :
    !node->dy ? x == node->y ? 2 : y <= node->y ? node->dx < 0 : node->dx > 0 :
    (right = ((y - node->y) >> FRACBITS) * (node->dx >> FRACBITS)) <
    (left  = ((x - node->x) >> FRACBITS) * (node->dy >> FRACBITS)) ? 0 :
    right == left ? 2 : 1;
}

//
// P_InterceptVector2
// Returns the fractional intercept point
// along the first divline.
//
// killough 4/19/98: made static, cleaned up

static fixed_t P_InterceptVector2(const divline_t *v2, const divline_t *v1)
{
  fixed_t den;
  return (den = FixedMul(v1->dy>>8, v2->dx) - FixedMul(v1->dx>>8, v2->dy)) ?
    FixedDiv(FixedMul((v1->x - v2->x)>>8, v1->dy) +
             FixedMul((v2->y - v1->y)>>8, v1->dx), den) : 0;
}

//
// P_CrossSubsector
// Returns true
//  if strace crosses the given subsector successfully.
//
// killough 4/19/98: made static and cleaned up

static boolean P_CrossSubsector(int num, register los_t *los)
{
  seg_t *seg = segs + (subsectors[num].firstline);
  int count;

#ifdef RANGECHECK
  if (num >= numsubsectors)
    I_Error("P_CrossSubsector: ss %i with numss = %i", num, numsubsectors);
#endif

  for (count = (unsigned short)subsectors[num].numlines; --count >= 0; seg++)  // check lines
    {
      line_t *line = seg->linedef;
      divline_t divl;
      fixed_t opentop, openbottom;
      const sector_t *front, *back;
      const vertex_t *v1,*v2;
      fixed_t frac;

      // allready checked other side?
      if (line->validcount == validcount)
        continue;

      line->validcount = validcount;

      // OPTIMIZE: killough 4/20/98: Added quick bounding-box rejection test
      // haleyjd: another demo compatibility fix by cph -- who knows
      // why this is a problem, though
      if(!demo_compatibility)
      if (line->bbox[BOXLEFT  ] > los->bbox[BOXRIGHT ] ||
          line->bbox[BOXRIGHT ] < los->bbox[BOXLEFT  ] ||
          line->bbox[BOXBOTTOM] > los->bbox[BOXTOP   ] ||
          line->bbox[BOXTOP]    < los->bbox[BOXBOTTOM])
          continue;

      v1 = line->v1;
      v2 = line->v2;

      // line isn't crossed?
      if (P_DivlineSide(v1->x, v1->y, &los->strace) ==
          P_DivlineSide(v2->x, v2->y, &los->strace))
        continue;

      divl.dx = v2->x - (divl.x = v1->x);
      divl.dy = v2->y - (divl.y = v1->y);

      // line isn't crossed?
      if (P_DivlineSide(los->strace.x, los->strace.y, &divl) ==
          P_DivlineSide(los->t2x, los->t2y, &divl))
        continue;

      // stop because it is not two sided anyway
      if (!(line->flags & ML_TWOSIDED))
        return false;

      // crosses a two sided line
      // no wall to block sight with?
      if ((front = seg->frontsector)->floorheight ==
          (back = seg->backsector)->floorheight   &&
          front->ceilingheight == back->ceilingheight)
        continue;

      // possible occluder
      // because of ceiling height differences
      opentop = front->ceilingheight < back->ceilingheight ?
        front->ceilingheight : back->ceilingheight ;

      // because of floor height differences
      openbottom = front->floorheight > back->floorheight ?
        front->floorheight : back->floorheight ;

      // quick test for totally closed doors
      if (openbottom >= opentop)
        return false;               // stop

      frac = P_InterceptVector2(&los->strace, &divl);

      if (front->floorheight != back->floorheight)
        {
          fixed_t slope = FixedDiv(openbottom - los->sightzstart , frac);
          if (slope > los->bottomslope)
            los->bottomslope = slope;
        }

      if (front->ceilingheight != back->ceilingheight)
        {
          fixed_t slope = FixedDiv(opentop - los->sightzstart , frac);
          if (slope < los->topslope)
            los->topslope = slope;
        }

      if (los->topslope <= los->bottomslope)
        return false;               // stop
    }
  // passed the subsector ok
  return true;
}

//
// P_CrossBSPNode
// Returns true
//  if strace crosses the given node successfully.
//
// killough 4/20/98: rewritten to remove tail recursion, clean up, and optimize

static boolean P_CrossBSPNode(int bspnum, register los_t *los)
{
  while (!(bspnum & NFX_SUBSECTOR))
    {
      register const node_t *bsp = nodes + bspnum;
      int side = P_DivlineSide(los->strace.x,los->strace.y,(divline_t *)bsp)&1;
      if (side == P_DivlineSide(los->t2x, los->t2y, (divline_t *) bsp))
         bspnum = bsp->children[side]; // doesn't touch the other side
      else         // the partition plane is crossed here
        if (!P_CrossBSPNode(bsp->children[side], los))
          return 0;  // cross the starting side
        else
          bspnum = bsp->children[side^1];  // cross the ending side
    }
  return P_CrossSubsector(bspnum == -1 ? 0 : bspnum & ~NFX_SUBSECTOR, los);
}

//
// P_CheckSight
// Returns true
//  if a straight line between t1 and t2 is unobstructed.
// Uses REJECT.
//
// killough 4/20/98: cleaned up, made to use new LOS struct

static boolean P_CheckSight_12(mobj_t *t1, mobj_t *t2);

boolean P_CheckSight(mobj_t *t1, mobj_t *t2)
{
  const sector_t *s1 = t1->subsector->sector;
  const sector_t *s2 = t2->subsector->sector;
  int pnum;
  los_t los;

  if (v12_compat)
  {
    return P_CheckSight_12(t1, t2);
  }
  pnum = (s1-sectors)*numsectors + (s2-sectors);

  // First check for trivial rejection.
  // Determine subsector entries in REJECT table.
  //
  // Check in REJECT table.
  if (rejectmatrix)
    if (rejectmatrix[pnum>>3] & (1 << (pnum&7)))   // can't possibly be connected
      return false;

  // killough 4/19/98: make fake floors and ceilings block monster view
  if ((s1->heightsec != -1 &&
       ((t1->z + t1->height <= sectors[s1->heightsec].floorheight &&
         t2->z >= sectors[s1->heightsec].floorheight) ||
        (t1->z >= sectors[s1->heightsec].ceilingheight &&
         t2->z + t1->height <= sectors[s1->heightsec].ceilingheight)))
      ||
      (s2->heightsec != -1 &&
       ((t2->z + t2->height <= sectors[s2->heightsec].floorheight &&
         t1->z >= sectors[s2->heightsec].floorheight) ||
        (t2->z >= sectors[s2->heightsec].ceilingheight &&
         t1->z + t2->height <= sectors[s2->heightsec].ceilingheight))))
    return false;

  // killough 11/98: shortcut for melee situations
  // same subsector? obviously visible
  // haleyjd: compatibility optioned for old demos
  //          fix thanks to cph   
  if ((t1->subsector == t2->subsector) && !demo_compatibility)
    return true;

  // An unobstructed LOS is possible.
  // Now look from eyes of t1 to any part of t2.

  validcount++;

  los.topslope = (los.bottomslope = t2->z - (los.sightzstart =
                                             t1->z + t1->height -
                                             (t1->height>>2))) + t2->height;
  los.strace.dx = (los.t2x = t2->x) - (los.strace.x = t1->x);
  los.strace.dy = (los.t2y = t2->y) - (los.strace.y = t1->y);

  if (t1->x > t2->x)
    los.bbox[BOXRIGHT] = t1->x, los.bbox[BOXLEFT] = t2->x;
  else
    los.bbox[BOXRIGHT] = t2->x, los.bbox[BOXLEFT] = t1->x;

  if (t1->y > t2->y)
    los.bbox[BOXTOP] = t1->y, los.bbox[BOXBOTTOM] = t2->y;
  else
    los.bbox[BOXTOP] = t2->y, los.bbox[BOXBOTTOM] = t1->y;

  // the head node is the last node output
  return P_CrossBSPNode(numnodes-1, &los);
}

// -------------- DOOM v1.2 COMPATIBILITY ONLY, FROM PRBOOM ------------------
// -------------- vvvvvvvvvvvvvvvvv BELOW vvvvvvvvvvvvvvvvv ------------------

/*
==============================================================================

P_CheckSight
This uses specialized forms of the maputils routines for optimized performance

==============================================================================
*/

static fixed_t sightzstart;            // eye z of looker
static fixed_t topslope, bottomslope;  // slopes to top and bottom of target

/*
==============
=
= PTR_SightTraverse
=
==============
*/

static boolean PTR_SightTraverse(intercept_t *in)
{
  line_t *li;
  fixed_t slope;

  li = in->d.line;

  //
  // crosses a two sided line
  //
  P_LineOpening(li);

  if (openbottom >= opentop)  // quick test for totally closed doors
    return false;  // stop

  if (li->frontsector->floorheight != li->backsector->floorheight)
  {
    slope = FixedDiv(openbottom - sightzstart , in->frac);
    if (slope > bottomslope)
      bottomslope = slope;
  }

  if (li->frontsector->ceilingheight != li->backsector->ceilingheight)
  {
    slope = FixedDiv(opentop - sightzstart, in->frac);
    if (slope < topslope)
      topslope = slope;
  }

  if (topslope <= bottomslope)
    return false;  // stop

  return true;  // keep going
}

/*
==================
=
= P_SightBlockLinesIterator
=
===================
*/

static boolean P_SightBlockLinesIterator(int x, int y)
{
  int offset;
  long *list;
  line_t *ld;
  int s1, s2;
  divline_t dl;

  offset = y*bmapwidth+x;

  offset = *(blockmap+offset);

  for (list = blockmaplump+offset; *list != -1; list++)
  {
    ld = &lines[*list];
    if (ld->validcount == validcount)
      continue;    // line has already been checked
    ld->validcount = validcount;

    s1 = P_PointOnDivlineSide(ld->v1->x, ld->v1->y, &trace);
    s2 = P_PointOnDivlineSide(ld->v2->x, ld->v2->y, &trace);
    if (s1 == s2)
      continue;    // line isn't crossed
    P_MakeDivline (ld, &dl);
    s1 = P_PointOnDivlineSide(trace.x, trace.y, &dl);
    s2 = P_PointOnDivlineSide(trace.x+trace.dx, trace.y+trace.dy, &dl);
    if (s1 == s2)
      continue; // line isn't crossed

    // try to early out the check
    if (!ld->backsector)
      return false; // stop checking

    check_intercept();    // killough

    // store the line for later intersection testing
    intercept_p->d.line = ld;
    intercept_p++;

  }

  return true; // everything was checked
}

/*
====================
=
= P_SightTraverseIntercepts
=
= Returns true if the traverser function returns true for all lines
====================
*/

static boolean P_SightTraverseIntercepts(void)
{
  int count;
  fixed_t dist;
  intercept_t *scan, *in;
  divline_t dl;

  count = intercept_p - intercepts;
  //
  // calculate intercept distance
  //
  for (scan = intercepts; scan<intercept_p; scan++)
  {
    P_MakeDivline(scan->d.line, &dl);
    scan->frac = P_InterceptVector(&trace, &dl);    
  }

  //
  // go through in order
  //  
  in = 0; // shut up compiler warning

  while (count--)
  {
    dist = INT_MAX;
    for (scan = intercepts ; scan<intercept_p ; scan++)
      if (scan->frac < dist)
      {
        dist = scan->frac;
        in = scan;
      }

      if (!PTR_SightTraverse(in))
        return false;      // don't bother going farther
      in->frac = INT_MAX;
  }

  return true;    // everything was traversed
}

/*
==================
=
= P_SightPathTraverse
=
= Traces a line from x1,y1 to x2,y2, calling the traverser function for each
= Returns true if the traverser function returns true for all lines
==================
*/

static boolean P_SightPathTraverse(fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2)
{
  fixed_t xt1,yt1,xt2,yt2;
  fixed_t xstep,ystep;
  fixed_t partial;
  fixed_t xintercept, yintercept;
  int mapx, mapy, mapxstep, mapystep;
  int count;

  validcount++;
  intercept_p = intercepts;

  if (((x1-bmaporgx)&(MAPBLOCKSIZE-1)) == 0)
    x1 += FRACUNIT;        // don't side exactly on a line
  if (((y1-bmaporgy)&(MAPBLOCKSIZE-1)) == 0)
    y1 += FRACUNIT;        // don't side exactly on a line
  trace.x = x1;
  trace.y = y1;
  trace.dx = x2 - x1;
  trace.dy = y2 - y1;

  x1 -= bmaporgx;
  y1 -= bmaporgy;
  xt1 = x1>>MAPBLOCKSHIFT;
  yt1 = y1>>MAPBLOCKSHIFT;

  x2 -= bmaporgx;
  y2 -= bmaporgy;
  xt2 = x2>>MAPBLOCKSHIFT;
  yt2 = y2>>MAPBLOCKSHIFT;

  // points should never be out of bounds, but check once instead of
  // each block
  if (xt1<0 || yt1<0 || xt1>=bmapwidth || yt1>=bmapheight
    || xt2<0 || yt2<0 || xt2>=bmapwidth || yt2>=bmapheight)
    return false;

  if (xt2 > xt1)
  {
    mapxstep = 1;
    partial = FRACUNIT - ((x1>>MAPBTOFRAC)&(FRACUNIT-1));
    ystep = FixedDiv (y2-y1,abs(x2-x1));
  }
  else if (xt2 < xt1)
  {
    mapxstep = -1;
    partial = (x1>>MAPBTOFRAC)&(FRACUNIT-1);
    ystep = FixedDiv (y2-y1,abs(x2-x1));
  }
  else
  {
    mapxstep = 0;
    partial = FRACUNIT;
    ystep = 256*FRACUNIT;
  }  
  yintercept = (y1>>MAPBTOFRAC) + FixedMul (partial, ystep);


  if (yt2 > yt1)
  {
    mapystep = 1;
    partial = FRACUNIT - ((y1>>MAPBTOFRAC)&(FRACUNIT-1));
    xstep = FixedDiv (x2-x1,abs(y2-y1));
  }
  else if (yt2 < yt1)
  {
    mapystep = -1;
    partial = (y1>>MAPBTOFRAC)&(FRACUNIT-1);
    xstep = FixedDiv (x2-x1,abs(y2-y1));
  }
  else
  {
    mapystep = 0;
    partial = FRACUNIT;
    xstep = 256*FRACUNIT;
  }  
  xintercept = (x1>>MAPBTOFRAC) + FixedMul (partial, xstep);


  //
  // step through map blocks
  // Count is present to prevent a round off error from skipping the break
  mapx = xt1;
  mapy = yt1;


  for (count = 0; count < 64; count++)
  {
    if (!P_SightBlockLinesIterator(mapx, mapy))
    {
      return false;  // early out
    }

    if (mapx == xt2 && mapy == yt2)
      break;

    if ((yintercept >> FRACBITS) == mapy)
    {
      yintercept += ystep;
      mapx += mapxstep;
    }
    else if ((xintercept >> FRACBITS) == mapx)
    {
      xintercept += xstep;
      mapy += mapystep;
    }

  }


  //
  // couldn't early out, so go through the sorted list
  //

  return P_SightTraverseIntercepts();
}

/*
=====================
=
= P_CheckSight_12
=
= Returns true if a straight line between t1 and t2 is unobstructed
= look from eyes of t1 to any part of t2
=
=====================
*/

static boolean P_CheckSight_12(mobj_t *t1, mobj_t *t2)
{
  int s1, s2;
  int pnum, bytenum, bitnum;

  //
  // check for trivial rejection
  //
  s1 = (t1->subsector->sector - sectors);
  s2 = (t2->subsector->sector - sectors);
  pnum = s1*numsectors + s2;
  bytenum = pnum>>3;
  bitnum = 1 << (pnum&7);

  if (rejectmatrix && rejectmatrix[bytenum]&bitnum)
  {
    return false;    // can't possibly be connected
  }

  //
  // check precisely
  //    
  sightzstart = t1->z + t1->height - (t1->height>>2);
  topslope = (t2->z+t2->height) - sightzstart;
  bottomslope = (t2->z) - sightzstart;

  return P_SightPathTraverse (t1->x, t1->y, t2->x, t2->y);
}


// -------------- ^^^^^^^^^^^^^^^^^ ABOVE ^^^^^^^^^^^^^^^^^ ------------------
// -------------- DOOM v1.2 COMPATIBILITY ONLY, FROM PRBOOM ------------------


//----------------------------------------------------------------------------
//
// $Log: p_sight.c,v $
// Revision 1.7  1998/05/07  00:55:55  killough
// Make monsters directly tangent to water surface blind
//
// Revision 1.6  1998/05/03  22:34:33  killough
// beautification, header cleanup
//
// Revision 1.5  1998/05/01  14:52:09  killough
// beautification
//
// Revision 1.4  1998/04/24  11:43:08  killough
// minor optimization
//
// Revision 1.3  1998/04/20  11:13:41  killough
// Fix v1.9 demo sync probs, make monsters blind across water
//
// Revision 1.2  1998/01/26  19:24:24  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:00  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
