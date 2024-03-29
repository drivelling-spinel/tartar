// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
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
//
// Operators
//
// Handler code for all the operators. The 'other half'
// of the parsing.
//
// By Simon Howard
//
//----------------------------------------------------------------------------

/* includes ************************/

#include "c_io.h"
#include "doomstat.h"
#include "doomtype.h"
#include "z_zone.h"

#include "t_parse.h"
#include "t_vari.h"

#define evaluate_leftnright(a, b, c) {\
  left = evaluate_expression((a), (b)-1); \
  right = evaluate_expression((b)+1, (c)); }\

svalue_t OPequals(int, int, int);           // =

svalue_t OPplus(int, int, int);             // +
svalue_t OPminus(int, int, int);            // -
svalue_t OPmultiply(int, int, int);         // *
svalue_t OPdivide(int, int, int);           // /
svalue_t OPremainder(int, int, int);        // %

svalue_t OPor(int, int, int);               // ||
svalue_t OPand(int, int, int);              // &&
svalue_t OPnot(int, int, int);              // !

svalue_t OPor_bin(int, int, int);           // |
svalue_t OPand_bin(int, int, int);          // &
svalue_t OPnot_bin(int, int, int);          // ~

svalue_t OPcmp(int, int, int);              // ==
svalue_t OPnotcmp(int, int, int);           // !=
svalue_t OPlessthan(int, int, int);         // <
svalue_t OPgreaterthan(int, int, int);      // >

svalue_t OPincrement(int, int, int);        // ++
svalue_t OPdecrement(int, int, int);        // --

svalue_t OPstructure(int, int, int);    // in t_vari.c

// haleyjd: Eternity operator extensions
// sorely missing compound comparison operators
svalue_t OPlessthanorequal(int, int, int);     // <=
svalue_t OPgreaterthanorequal(int, int, int);  // >=

operator_t operators[]=
{
  {"=",   OPequals,               backward},
  {"||",  OPor,                   forward},
  {"&&",  OPand,                  forward},
  {"|",   OPor_bin,               forward},
  {"&",   OPand_bin,              forward},
  {"==",  OPcmp,                  forward},
  {"!=",  OPnotcmp,               forward},
  {"<",   OPlessthan,             forward},
  {">",   OPgreaterthan,          forward},

  // haleyjd: Eternity Extensions
  {"<=",  OPlessthanorequal,      forward},
  {">=",  OPgreaterthanorequal,   forward},

  {"+",   OPplus,                 forward},
  {"-",   OPminus,                forward},
  {"*",   OPmultiply,             forward},
  {"/",   OPdivide,               forward},
  {"%",   OPremainder,            forward},
  {"~",   OPnot_bin,              forward}, // haleyjd
  {"!",   OPnot,                  forward},
  {"++",  OPincrement,            forward},
  {"--",  OPdecrement,            forward},
  {".",   OPstructure,            forward},
};

int num_operators = sizeof(operators) / sizeof(operator_t);

/***************** logic *********************/

// = operator

svalue_t OPequals(int start, int n, int stop)
{
  svariable_t *var;
  svalue_t evaluated;
  
  var = find_variable(tokens[start]);
  
  if(var)
    {
      evaluated = evaluate_expression(n+1, stop);
      setvariablevalue(var, evaluated);
    }
  else
    {
      script_error("unknown variable '%s'\n", tokens[start]);
      return nullvar;
    }
  
  return evaluated;
}


svalue_t OPor(int start, int n, int stop)
{
  svalue_t returnvar;
  int exprtrue = false;
  svalue_t eval;
  
  // if first is true, do not evaluate the second
  
  eval = evaluate_expression(start, n-1);
  
  if(intvalue(eval))
    exprtrue = true;
  else
    {
      eval = evaluate_expression(n+1, stop);
      exprtrue = !!intvalue(eval);
    }
  
  returnvar.type = svt_int;
  returnvar.value.i = exprtrue;
  return returnvar;
  
  return returnvar;
}


svalue_t OPand(int start, int n, int stop)
{
  svalue_t returnvar;
  int exprtrue = true;
  svalue_t eval;

  // if first is false, do not eval second
  
  eval = evaluate_expression(start, n-1);
  
  if(!intvalue(eval) )
    exprtrue = false;
  else
    {
      eval = evaluate_expression(n+1, stop);
      exprtrue = !!intvalue(eval);
    }

  returnvar.type = svt_int;
  returnvar.value.i = exprtrue;

  return returnvar;
}

// haleyjd: reformatted as per 8-17
svalue_t OPcmp(int start, int n, int stop)
{
  svalue_t left, right, returnvar;
  
  evaluate_leftnright(start, n, stop);
  
  returnvar.type = svt_int;        // always an int returned

  if(left.type == svt_string && right.type == svt_string)
  {
      returnvar.value.i = !strcmp(left.value.s, right.value.s);
      return returnvar;
  }

  // haleyjd: direct mobj comparison when both are mobj
  if(left.type == svt_mobj && right.type == svt_mobj)
  {
     // we can safely assume reference equivalency for
     // mobj_t's in all cases since they are static for the
     // duration of a level
     returnvar.value.i = (left.value.mobj == right.value.mobj);
     return returnvar;
  }

  if(left.type == svt_fixed || right.type == svt_fixed)
  {
      returnvar.value.i = (fixedvalue(left) == fixedvalue(right));
      return returnvar;
  }

  returnvar.value.i = (intvalue(left) == intvalue(right));

  return returnvar;

}

svalue_t OPnotcmp(int start, int n, int stop)
{
  svalue_t returnvar;
  
  returnvar = OPcmp(start, n, stop);
  returnvar.value.i = !returnvar.value.i;
  
  return returnvar;
}

svalue_t OPlessthan(int start, int n, int stop)
{
  svalue_t left, right, returnvar;
  
  evaluate_leftnright(start, n, stop);
  returnvar.type = svt_int;

  // haleyjd: 8-17
  if(left.type == svt_fixed || right.type == svt_fixed)
    returnvar.value.i = (fixedvalue(left) < fixedvalue(right));
  else
    returnvar.value.i = (intvalue(left) < intvalue(right));

  return returnvar;
}

svalue_t OPgreaterthan(int start, int n, int stop)
{
  svalue_t left, right, returnvar;
  
  evaluate_leftnright(start, n, stop);

  // haleyjd: 8-17
  if(left.type == svt_fixed || right.type == svt_fixed)
    returnvar.value.i = (fixedvalue(left) > fixedvalue(right));
  else
    returnvar.value.i = (intvalue(left) > intvalue(right));
  
  return returnvar;
}

svalue_t OPnot(int start, int n, int stop)
{
  svalue_t right, returnvar;
  
  right = evaluate_expression(n+1, stop);
  
  returnvar.type = svt_int;
  returnvar.value.i = !intvalue(right);
  return returnvar;
}

/************** simple math ***************/

svalue_t OPplus(int start, int n, int stop)
{
  svalue_t left, right, returnvar;
  
  evaluate_leftnright(start, n, stop);

  // haleyjd: 8-17
  if(left.type == svt_fixed || right.type == svt_fixed)
  {
      returnvar.type = svt_fixed;
      returnvar.value.f = fixedvalue(left) + fixedvalue(right);
  }
  else
  {
      returnvar.type = svt_int;
      returnvar.value.i = intvalue(left) + intvalue(right);
  }

  return returnvar;
}

svalue_t OPminus(int start, int n, int stop)
{
  svalue_t left, right, returnvar;
  
  // do they mean minus as in '-1' rather than '2-1'?
  if(start == n)
  {
      // kinda hack, hehe
      left.value.i = 0; left.type = svt_int;
      right = evaluate_expression(n+1, stop);
  }
  else
    evaluate_leftnright(start, n, stop);

  // haleyjd: 8-17
  if(left.type == svt_fixed || right.type == svt_fixed)
  {
      returnvar.type = svt_fixed;
      returnvar.value.f = fixedvalue(left) - fixedvalue(right);
  }
  else
  {
      returnvar.type = svt_int;
      returnvar.value.i = intvalue(left) - intvalue(right);
  }
  
  return returnvar;
}

svalue_t OPmultiply(int start, int n, int stop)
{
  svalue_t left, right, returnvar;
  
  evaluate_leftnright(start, n, stop);

  // haleyjd: 8-17
  if(left.type == svt_fixed || right.type == svt_fixed)
  {
      returnvar.type = svt_fixed;
      returnvar.value.f = FixedMul(fixedvalue(left), fixedvalue(right));
  }
  else
  {
      returnvar.type = svt_int;
      returnvar.value.i = intvalue(left) * intvalue(right);
  }
  
  return returnvar;
}

svalue_t OPdivide(int start, int n, int stop)
{
  svalue_t left, right, returnvar;
  
  evaluate_leftnright(start, n, stop);

  // haleyjd: 8-17
  if(left.type == svt_fixed || right.type == svt_fixed)
  {
      fixed_t fr;
	
      if((fr = fixedvalue(right)) == 0)
	script_error("divide by zero\n");
      else
      {
	  returnvar.type = svt_fixed;
	  returnvar.value.f = FixedDiv(fixedvalue(left), fr);
      }
  }
  else
  {
      int ir;
      
      if(!(ir = intvalue(right)))
	script_error("divide by zero\n");
      else
      {
	  returnvar.type = svt_int;
	  returnvar.value.i = intvalue(left) / ir;
      }
  }
  
  return returnvar;
}

svalue_t OPremainder(int start, int n, int stop)
{
  svalue_t left, right, returnvar;
  int ir;
  
  evaluate_leftnright(start, n, stop);
  
  if(!(ir = intvalue(right)))
    script_error("divide by zero\n");
  else
    {
      returnvar.type = svt_int;
      returnvar.value.i = intvalue(left) % ir;
    }
  return returnvar;
}

        /********** binary operators **************/

// binary or |

svalue_t OPor_bin(int start, int n, int stop)
{
  svalue_t left, right, returnvar;
  
  evaluate_leftnright(start, n, stop);
  
  returnvar.type = svt_int;
  returnvar.value.i = intvalue(left) | intvalue(right);
  return returnvar;
}


// binary and &

svalue_t OPand_bin(int start, int n, int stop)
{
  svalue_t left, right, returnvar;
  
  evaluate_leftnright(start, n, stop);
  
  returnvar.type = svt_int;
  returnvar.value.i = intvalue(left) & intvalue(right);
  return returnvar;
}

// haleyjd: binary invert - ~
svalue_t OPnot_bin(int start, int n, int stop)
{
  svalue_t right, returnvar;
  
  right = evaluate_expression(n+1, stop);
  
  returnvar.type = svt_int;
  returnvar.value.i = ~intvalue(right);
  return returnvar;
}


        // ++
svalue_t OPincrement(int start, int n, int stop)
{
  if(start == n)          // ++n
    {
      svalue_t value;
      svariable_t *var;
      
      var = find_variable(tokens[stop]);
      if(!var)
	{
	  script_error("unknown variable '%s'\n", tokens[stop]);
	  return nullvar;
	}
      value = getvariablevalue(var);
      
      // haleyjd
      if(var->type != svt_fixed)
      {
	 value.value.i = intvalue(value) + 1;
	 value.type = svt_int;
	 setvariablevalue(var, value);
      }
      else
      {
	 value.value.f = fixedvalue(value) + FRACUNIT;
	 value.type = svt_fixed;
	 setvariablevalue(var, value);
      }
      
      return value;
    }
  else if(stop == n)     // n++
    {
      svalue_t origvalue, value;
      svariable_t *var;
      
      var = find_variable(tokens[start]);
      if(!var)
	{
	  script_error("unknown variable '%s'\n", tokens[start]);
	  return nullvar;
	}
      origvalue = getvariablevalue(var);
      
      // haleyjd
      if(var->type != svt_fixed)
      {
	 value.type = svt_int;
	 value.value.i = intvalue(origvalue) + 1;
	 setvariablevalue(var, value);
      }
      else
      {
	 value.type = svt_fixed;
	 value.value.f = fixedvalue(origvalue) + FRACUNIT;
	 setvariablevalue(var, value);
      }
      
      return origvalue;
    }
  
  script_error("incorrect arguments to ++ operator\n");
  return nullvar;
}

        // --
svalue_t OPdecrement(int start, int n, int stop)
{
  if(start == n)          // ++n
    {
      svalue_t value;
      svariable_t *var;
      
      var = find_variable(tokens[stop]);
      if(!var)
	{
	  script_error("unknown variable '%s'\n", tokens[stop]);
	  return nullvar;
	}
      value = getvariablevalue(var);
      
      // haleyjd
      if(var->type != svt_fixed)
      {
	 value.value.i = intvalue(value) - 1;
	 value.type = svt_int;
	 setvariablevalue(var, value);
      }
      else
      {
	 value.value.f = fixedvalue(value) - FRACUNIT;
	 value.type = svt_fixed;
	 setvariablevalue(var, value);
      }
      
      return value;
    }
  else if(stop == n)   // n++
    {
      svalue_t origvalue, value;
      svariable_t *var;
      
      var = find_variable(tokens[start]);
      if(!var)
	{
	  script_error("unknown variable '%s'\n", tokens[start]);
	  return nullvar;
	}
      origvalue = getvariablevalue(var);
      
      // haleyjd
      if(var->type != svt_fixed)
      {
	 value.type = svt_int;
	 value.value.i = intvalue(origvalue) - 1;
	 setvariablevalue(var, value);
      }
      else
      {
	 value.type = svt_fixed;
	 value.value.f = fixedvalue(origvalue) - FRACUNIT;
	 setvariablevalue(var, value);
      }
      
      return origvalue;
    }
  
  script_error("incorrect arguments to ++ operator\n");
  return nullvar;
}

// haleyjd: Eternity extensions

svalue_t OPlessthanorequal(int start, int n, int stop)
{
  svalue_t left, right, returnvar;
  
  evaluate_leftnright(start, n, stop);
  
  returnvar.type = svt_int;

  if(left.type == svt_fixed || right.type == svt_fixed)
     returnvar.value.i = (fixedvalue(left) <= fixedvalue(right));
  else
     returnvar.value.i = (intvalue(left) <= intvalue(right));
  return returnvar;
}

svalue_t OPgreaterthanorequal(int start, int n, int stop)
{
  svalue_t left, right, returnvar;
  
  evaluate_leftnright(start, n, stop);
  
  returnvar.type = svt_int;

  if(left.type == svt_fixed || right.type == svt_fixed)
     returnvar.value.i = (fixedvalue(left) >= fixedvalue(right));
  else
     returnvar.value.i = (intvalue(left) >= intvalue(right));
  return returnvar;
}

// EOF
