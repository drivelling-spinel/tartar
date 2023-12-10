// Emacs style mode select -*- C++ -*-
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
//
//   Array support for FraggleScript
//
//   By James Haley, with special thanks to SoM
//
//--------------------------------------------------------------------------

#include "z_zone.h"
#include "t_array.h"
#include "t_vari.h"
#include "p_enemy.h"

// Save list for arrays -- put ALL arrays spawned into this list,
// and they'll live until the end of the level. This is very lazy
// garbage collection, but its the only real solution given the
// state of the FS source (the word "haphazard" comes to mind...)
sfarray_t sfsavelist =
{
   NULL,
   0,
   0,
   NULL,
};

// add an array into the save list
void T_AddArray(sfarray_t *array)
{
   sfarray_t *temp;

   // always insert at head of list
   temp = sfsavelist.next;
   sfsavelist.next = array;
   array->next = temp;
}

// call from P_SetupLevel -- all arrays in the list will have been
// destroyed by the zone allocator already
void T_InitSaveList(void)
{
   sfsavelist.next = NULL;
}

// SF Handler functions for calling from scripts

//
// SF_NewArray
// 
//  Create a new sfarray_t and initialize it with values
//
//  Implements: array newArray(...)
//
void SF_NewArray(void)
{
   int i;
   sfarray_t *newArray;

   if(!t_argc) // empty, do nothing
      return;

   // allocate a sfarray_t
   newArray = Z_Malloc(sizeof(sfarray_t), PU_LEVEL, 0);

   // init all fields to zero
   memset(newArray, 0, sizeof(sfarray_t));
   
   // allocate t_argc number of values, set length
   newArray->values = Z_Malloc(t_argc*sizeof(svalue_t), PU_LEVEL, 0);
   memset(newArray->values, 0, t_argc*sizeof(svalue_t));
   
   newArray->length = t_argc;

   for(i=0; i<t_argc; i++)
   {
      // strings, arrays are ignored
      if(t_argv[i].type == svt_string || t_argv[i].type == svt_array)
	 continue;

      // copy all the argument values into the local array
      memcpy(&(newArray->values[i]), &t_argv[i], sizeof(svalue_t));
   }

   T_AddArray(newArray); // add the new array to the save list
   
   t_return.type = svt_array;
   // t_return is an internal value which may not be captured in
   // an svariable_t, so we don't count it as a reference --
   // in the cases of immediate value usage, the garbage collector
   // won't have a chance to free it until it has been used
   t_return.value.a = newArray;
}

//
// SF_ArrayCopyInto
//
// Copies the values from one array into the values of another.
// Arrays must be non-empty and must be of equal length.
//
// Implements: void copyInto(array source, array target)
//
void SF_ArrayCopyInto(void)
{
   int i;
   sfarray_t *source, *target;
   
   if(t_argc != 2)
   {
      script_error("insufficient arguments to function\n");
      return;
   }

   if(t_argv[0].type != svt_array || t_argv[1].type != svt_array)
   {
      script_error("copyinto must be called on arrays\n");
      return;
   }

   source = t_argv[0].value.a;
   target = t_argv[1].value.a;

   if(!source || !target)
   {
      script_error("copyinto cannot function on empty arrays\n");
      return;
   }

   if(source->length != target->length)
   {
      script_error("copyinto must be passed arrays of equal length\n");
      return;
   }

   for(i=0; i<source->length; i++)
   {
      memcpy(&(target->values[i]), &(source->values[i]), 
	     sizeof(svalue_t));
   }
}

//
// SF_ArrayElementAt
//
// Retrieves a value at a specific index
//
// Implements: 'a elementAt(array x, int i)
//
// This function is somewhat unique at it has a polymorphic
// return type :)
//
void SF_ArrayElementAt(void)
{
   int index;
   
   if(t_argc != 2)
   {
      script_error("incorrect arguments to function");
      return;
   }

   if(t_argv[0].type != svt_array || !t_argv[0].value.a)
   {
      script_error("elementat must be called on a non-empty array\n");
      return;
   }

   // get index from second arg
   index = intvalue(t_argv[1]);

   if(index < 0 || index >= t_argv[0].value.a->length)
   {
      script_error("array index out of bounds\n");
      return;
   }

   // copy full svalue_t to t_return
   memcpy(&t_return, &(t_argv[0].value.a->values[index]), 
          sizeof(svalue_t));
}

//
// SF_ArraySetElementAt
//
// Sets a specific value in an array
//
// Implements: void setElementAt(array x, 'a val, int i)
//
void SF_ArraySetElementAt(void)
{
   int index;
   
   if(t_argc != 3)
   {
      script_error("incorrect arguments to function");
      return;
   }

   if(t_argv[0].type != svt_array || !t_argv[0].value.a)
   {
      script_error("setelementat must be called on a non-empty array\n");
      return;
   }

   // get index from third arg this time...
   index = intvalue(t_argv[2]);

   if(index < 0 || index >= t_argv[0].value.a->length)
   {
      script_error("array index out of bounds\n");
      return;
   }

   // type checking on second arg: restricted types
   if(t_argv[1].type == svt_array || t_argv[1].type == svt_string)
   {
      script_error("%s cannot be an array element\n",
	 t_argv[1].type == svt_array ? "an array" : "a string");
      return;
   }

   // copy full svalue_t into array at given index
   memcpy(&(t_argv[0].value.a->values[index]), &t_argv[1],
          sizeof(svalue_t));
}

//
// SF_ArrayLength
//
// Retrieves the length of an array
//
// Implements: int length(array x)
//
void SF_ArrayLength(void)
{
   if(!t_argc)
   {
      script_error("insufficient arguments to function\n");
      return;
   }

   if(t_argv[0].type != svt_array)
   {
      script_error("length must be called on an array\n");
      return;
   }

   t_return.type = svt_int;

   if(!t_argv[0].value.a)
      t_return.value.i = 0;
   else
      t_return.value.i = t_argv[0].value.a->length;
}

//
// Functions to maintain global script arrays
//

sfarray_t *sfcnodes; // array pointing to camera nodes

// just builds the camera nodes for now -- will rewrite to use
// a more structured system once there are more game arrays
void T_BuildGameArrays(void)
{
   int i;

   // zone allocator will have killed the array from the last
   // level already
   sfcnodes = Z_Malloc(sizeof(sfarray_t), PU_LEVEL, 0);

   sfcnodes->length = numcameranodes;
   sfcnodes->values = Z_Malloc(numcameranodes*sizeof(svalue_t),
                               PU_LEVEL, 0);

   for(i=0; i<numcameranodes; i++)
   {
      sfcnodes->values[i].type = svt_mobj;
      sfcnodes->values[i].value.mobj = cameranodes[i];
   }
}

// EOF