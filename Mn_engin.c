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
// New menu
//
// The menu engine. All the menus themselves are in mn_menus.c
//
// By Simon Howard
//
//-----------------------------------------------------------------------------

#include <stdarg.h>

#include "doomdef.h"
#include "doomstat.h"
#include "c_io.h"
#include "c_runcmd.h"
#include "d_main.h"
#include "g_game.h"
#include "hu_over.h"
#include "i_video.h"
#include "mn_engin.h"
#include "mn_menus.h"
#include "mn_misc.h"
#include "r_defs.h"
#include "r_draw.h"
#include "s_sound.h"
#include "w_wad.h"
#include "v_video.h"

#include "g_bind.h"    // haleyjd: dynamic key bindings

boolean inhelpscreens; // indicates we are in or just left a help screen

        // menu keys
int     key_menu_right;
int     key_menu_left;
int     key_menu_up;
int     key_menu_down;
int     key_menu_backspace;
int     key_menu_escape;
int     key_menu_enter;

// menu error message
char menu_error_message[128];
int menu_error_time = 0;

extern menu_t menu_main;
extern menu_t menu_newgame;

        // input for typing in new value
static command_t *input_command = NULL;       // NULL if not typing in
static char input_buffer[128] = "";

static int last_title_height = -1;
#define DEFAULT_TITLE_HEIGHT (15)

/////////////////////////////////////////////////////////////////////////////
// 
// MENU DRAWING
//
/////////////////////////////////////////////////////////////////////////////

        // gap from variable description to value
#define GAP 20
#define SKULL_HEIGHT 19
#define BLINK_TIME 8

// colours
#define unselect_colour    CR_RED
#define select_colour      CR_GRAY
#define var_colour         CR_GREEN
#define over_colour        CR_TAN
#define help_colour        CR_GOLD

enum
{
  slider_left,
  slider_right,
  slider_mid,
  slider_slider,
  num_slider_gfx
};
patch_t *slider_gfx[num_slider_gfx];
static menu_t *drawing_menu;
static patch_t *skulls[2];

#ifdef EPISINFO
extern char * menu_layout;
#endif

static void MN_GetItemVariable(menuitem_t *item)
{
        // get variable if neccesary
  if(!item->var)
    {
      command_t *cmd;
      // use data for variable name
      if(!(cmd = C_GetCmdForName(item->data)))
        {
          C_Printf("variable not found: %s\n", item->data);
	  item->type = it_info;   // turn into normal unselectable text
	  item->var = NULL;
	  return;
        }
      item->var = cmd->variable;
    }
}




        // width of slider, in mid-patches
#define SLIDE_PATCHES 9

// draw a 'slider' (for sound volume, etc)

static void MN_DrawSlider(int x, int y, int pct)
{
  int i;
  int draw_x = x;
  int slider_width = 0;       // find slider width in pixels
  
  V_DrawPatch(draw_x, y, 0, slider_gfx[slider_left]);
  draw_x += slider_gfx[slider_left]->width;
  
  for(i=0; i<SLIDE_PATCHES; i++)
    {
      V_DrawPatch(draw_x, y, 0, slider_gfx[slider_mid]);
      draw_x += slider_gfx[slider_mid]->width - 1;
    }
  
  V_DrawPatch(draw_x, y, 0, slider_gfx[slider_right]);
  
  // find position to draw
  
  slider_width = (slider_gfx[slider_mid]->width - 1) * SLIDE_PATCHES;
  draw_x = slider_gfx[slider_left]->width +
    (pct * (slider_width - slider_gfx[slider_slider]->width)) / 100;
  
  V_DrawPatch(x + draw_x, y, 0, slider_gfx[slider_slider]);
}

// draw a menu item. returns the height in pixels

static int MN_DrawMenuItem(menuitem_t *item, int x, int y, int colour)
{
  boolean leftaligned =
    (drawing_menu->flags & mf_skullmenu) ||
    (drawing_menu->flags & mf_leftaligned);
  
  boolean descr_collapsed = false;

#ifdef EPISINFO
  if((drawing_menu->flags & mf_fixedlayout) && item->type == it_gap)
    return 0;
#endif
  if(item->type == it_gap) return M_LINE;    // skip drawing if a gap

  item->x = x; item->y = y;       // save x,y to item
 
  // draw an alternate patch?
 
  if(item->patch && item->type != it_togglehint)
    {
      patch_t *patch;
      int lumpnum;
      
      lumpnum = W_CheckNumForName(item->patch);
      
      // default to text-based message if patch missing
      if(lumpnum >= 0)
	{
	  int height;
	  
	  patch = W_CacheLumpNum(lumpnum, PU_CACHE);
	  height = patch->height;
	  
	  // check for left-aligned
	  if(!leftaligned) x-= patch->width;
	  
	  // adjust x if a centered title
	  if(item->type == it_title)
            {
#ifdef EPISINFO
              if(drawing_menu->flags & mf_fixedlayout)
                {
                   x = 54;
                   y = 38;
                }
              else
#endif
              x = (SCREENWIDTH-patch->width)/2;
            }

#ifdef EPISINFO
          if(item->type == it_info && (drawing_menu->flags & mf_fixedlayout))
            {
               x = 96;
               y = 14;
            }
#endif

	  V_DrawPatchTranslated(x, y, 0, patch, colrngs[colour], 0);
	  
	  if(item->type == it_title)
	    {
#ifdef EPISINFO
              if(drawing_menu->flags & mf_fixedlayout)
                return 0;
#endif
	      last_title_height = height + 1;
	    }
#ifdef EPISINFO
          if(drawing_menu->flags & mf_fixedlayout)
            {
               return item->type == it_info ? 0 : 16;
            }
#endif
	  return height + 1;   // 1 pixel gap
	}
    }
    
    descr_collapsed = drawing_menu->flags & mf_collapsedescr && item->type == it_variable &&
        input_command && item->var == input_command->variable;

  // draw description text
  
  if(item->type == it_title)
    {
      // if it_title, we draw the description centered
      MN_WriteTextColoured
	(
         item->description,
         colour,
         (SCREENWIDTH-MN_StringWidth(item->description))/2,
	 y
	 );
	   last_title_height = M_LINE;
    }
  else if(item->description)
    {
      if(!descr_collapsed && ((menutime / BLINK_TIME / 2) % 2 == 1 || colour != select_colour ))
        // write description
        MN_WriteTextColoured
          (
            item->description,
            colour,
            x - (leftaligned ? 0 : MN_StringWidth(item->description)),
            y
          );
    }

  // draw other data: variable data etc.
  
  switch(item->type)      
    {
    case it_titlegap:
      {
        int off;
        if(last_title_height == -1)
          return M_LINE;
        off = (DEFAULT_TITLE_HEIGHT - last_title_height);
        if(off > - M_LINE)
          return M_LINE + off;
        return 0;
      }
    case it_title:              // just description drawn
    case it_info:
    case it_runcmd:
    case it_gap:                // just a gap, draw nothing
      {
        break;
      }

    case it_binding:            // key binding
      {
	 char *boundkeys = G_BoundKeys(item->data);
	 
         if(drawing_menu->flags & mf_background)
	 {
	    // include gap on fullscreen menus
	    x += GAP;
	    // adjust colour for different coloured variables
	    if(colour == unselect_colour) colour = var_colour;
	 }
	  
	 // write variable value text
	 MN_WriteTextColoured
	   (
	    boundkeys,
	    colour,
	    x + (leftaligned ? MN_StringWidth(item->description): 0),
	    y
	   );
	 
	 break;
      }

    // it_toggle and it_variable are drawn the same

    case it_toggle:
    case it_togglehint:
    case it_variable:
      {
        char varvalue[1024];             // temp buffer
	memset(varvalue, 0, 1024);

	MN_GetItemVariable(item);
	
	// create variable description:
	// Use console variable descriptions.
	
	// display input buffer if inputting new var value
	if(input_command && item->var == input_command->variable)
          sprintf(varvalue, "%s_", input_buffer);
	else
          strncpy(varvalue, C_VariableStringValue(item->var), 1024);

	if(drawing_menu->flags & mf_background)
	  {
	    // include gap on fullscreen menus
	    x += GAP;
	    // adjust colour for different coloured variables
	    if(colour == unselect_colour) colour = var_colour;
	  }

        // draw it
        MN_WriteTextColoured
	  (
	   varvalue,
           colour,
           x + (leftaligned && !descr_collapsed ? MN_StringWidth(item->description) : 0),
	   y
	   );
	break;
      }

    // slider

    case it_slider:
      { 
	MN_GetItemVariable(item);
	
	// draw slider
	// only ints

	if(item->var && item->var->type == vt_int)
	  {
	    int range = item->var->max - item->var->min;
	    int posn = *(int *)item->var->variable - item->var->min;
	    
	    MN_DrawSlider(x + GAP, y, (posn*100) / range);
	  }
	
	break;
      }

    // automap colour block

    case it_automap:
      {
	int bx, by;
	int colour;
	char block[BLOCK_SIZE*BLOCK_SIZE];
	
	MN_GetItemVariable(item);

	if(!item->var || item->var->type != vt_int) break;
	
	// find colour of this variable from console variable
	colour = *(int *)item->var->variable;

	// create block
	// border
	memset(block, 0, BLOCK_SIZE*BLOCK_SIZE);
	    
	if(colour)
	  {	
	    // middle
	    for(bx=1; bx<BLOCK_SIZE-1; bx++)
	      for(by=1; by<BLOCK_SIZE-1; by++)
		block[by*BLOCK_SIZE+bx] = colour;
	  }
	// draw it
	
	V_DrawBlock(x+GAP, y-1, 0, BLOCK_SIZE, BLOCK_SIZE, block);
	
	if(!colour)
	  {
	    // draw patch w/cross
	    V_DrawPatch(x+GAP+1, y, 0, W_CacheLumpName("M_PALNO", PU_CACHE));
	  }
      }    

    default:
      {
        break;
      }
    }
  
  return M_LINE;   // text height
}

///////////////////////////////////////////////////////////////////////
//
// MAIN FUNCTION
//
// Draw a menu
//

void MN_DrawMenu(menu_t *menu)
{
  int y;
  int itemnum;

  last_title_height = -1;

  drawing_menu = menu;    // needed by DrawMenuItem
  y = menu->y;
  // draw background

  if(menu->flags & mf_background)
    V_DrawBackground(background_flat, screens[0]);
  
  // menu-specific drawer function

  if(menu->drawer) menu->drawer();

  // draw items in menu

  for(itemnum = 0; menu->menuitems[itemnum].type != it_end; itemnum++)
    {
      int item_height;
      int item_colour;

      // choose item colour based on selected item

      item_colour = menu->selected == itemnum &&
	!(menu->flags & mf_skullmenu) ?	select_colour : unselect_colour;
      
      // draw item

      item_height =
	MN_DrawMenuItem
	(
	 &menu->menuitems[itemnum],
	 menu->x,
	 y,
	 item_colour
	 );
      
      // if selected item, draw skull next to it

      if(menu->flags & mf_skullmenu && menu->selected == itemnum)
#ifdef EPISINFO
        if(menu->flags & mf_fixedlayout)
          {
            V_DrawPatch
              (
                menu->x - 32,
                y - 5,
                0,
                skulls[(menutime / BLINK_TIME) % 2]
              );
          }
        else
#endif
        V_DrawPatch
	  (
	   menu->x - 30,                                // 30 left
	   y + (item_height - SKULL_HEIGHT) / 2,        // midpoint
	   0,
	   skulls[(menutime / BLINK_TIME) % 2]
	   );
      
      y += item_height;            // go down by item height
    }

  if(menu->flags & mf_skullmenu) return; // no help msg in skull menu

  // choose help message to print
  
  if(menu_error_time)             // error message takes priority
    {
      // make it flash
      if((menu_error_time / 8) % 2)
	MN_WriteTextColoured(menu_error_message, CR_TAN, 10, SCREENHEIGHT - M_LINE);
    }
  else
    {
      char *helpmsg = "";
      int col = help_colour;

      // write some help about the item
      menuitem_t *menuitem = &menu->menuitems[menu->selected];
      
      if(menuitem->type == it_variable)       // variable
	helpmsg = "press enter to change";

      if(menuitem->type == it_togglehint)
        {
          static char shint[1024];
          const char * v = "";
          command_t * cmd = C_GetCmdForName(menuitem->patch);
          if(cmd) v = C_VariableValue(cmd->variable);
          *shint = 0;
          if(*v)
            {
              strncpy(shint, v, 1024);
              col = over_colour;
            }          
          helpmsg = shint;
        }
      
      if(menuitem->type == it_toggle          // togglable variable
        || (menuitem->type == it_togglehint && !*helpmsg)) 
	{
	  // enter to change boolean variables
	  // left/right otherwise

	  if(menuitem->var->type == vt_int &&
	     menuitem->var->max - menuitem->var->min == 1)
	    helpmsg = "press enter to change";
	  else
	    helpmsg = "use left/right to change value";
	}
      MN_WriteTextColoured(helpmsg, col, 10, SCREENHEIGHT - M_LINE);
    }
}

/////////////////////////////////////////////////////////////////////////
//
// Menu Module Functions
//
// Drawer, ticker etc.
//
/////////////////////////////////////////////////////////////////////////

#define MENU_HISTORY 128

boolean menuactive = false;             // menu active?
menu_t *current_menu;   // the current menu_t being displayed
static menu_t *menu_history[MENU_HISTORY];   // previously selected menus
static int menu_history_num;                 // location in history
int hide_menu = 0;      // hide the menu for a duration of time
int menutime = 0;

// menu widget for alternate drawer + responder
menuwidget_t *current_menuwidget = NULL; 

int quickSaveSlot;  // haleyjd 02/23/02: restored from MBF

        // init menu
void MN_Init()
{
  skulls[0] = W_CacheLumpName("M_SKULL1", PU_STATIC);
  skulls[1] = W_CacheLumpName("M_SKULL2", PU_STATIC);
  
  // load slider gfx
  
  slider_gfx[slider_left]   = W_CacheLumpName("M_SLIDEL", PU_STATIC);
  slider_gfx[slider_right]  = W_CacheLumpName("M_SLIDER", PU_STATIC);
  slider_gfx[slider_mid]    = W_CacheLumpName("M_SLIDEM", PU_STATIC);
  slider_gfx[slider_slider] = W_CacheLumpName("M_SLIDEO", PU_STATIC);
  
  quickSaveSlot = -1; // haleyjd: -1 == no slot selected yet

  MN_InitMenus();   // create menu commands in mn_menus.c
}

//////////////////////////////////
// ticker

void MN_Ticker()
{
  if(menu_error_time)
    menu_error_time--;
  if(hide_menu)                   // count down hide_menu
    hide_menu--;
  menutime++;
}

////////////////////////////////
// drawer

void MN_Drawer()
{ 
  // redraw needed if menu hidden
  if(hide_menu) redrawsbar = redrawborder = true;

  // activate menu if displaying widget
  if(current_menuwidget) menuactive = true; 

  if(!menuactive || hide_menu) return;

  if(current_menuwidget)
    {
      // alternate drawer
      if(current_menuwidget->drawer)
	current_menuwidget->drawer();
      return;
    }
 
  MN_DrawMenu(current_menu);  
}

// whether a menu item is a 'gap' item
// ie. one that cannot be selected

#define is_a_gap(it) ((it)->type == it_gap || (it)->type == it_info ||  \
                      (it)->type == it_title || (it)->type == it_titlegap )

extern menu_t menu_sound;

boolean MN_TempResponder(int key)
{
  if(key == key_help)
  {
     C_RunTextCmd("help");
     return true;
  }
  if(key == key_setup)
  {
     C_RunTextCmd("mn_options");
     return true;
  }
  return false;
}
                
/////////////////////////////////
// Responder

boolean MN_Responder (event_t *ev)
{
  char tempstr[128];
  unsigned char ch;

  // we only care about key presses

  if(ev->type != ev_keydown)
    return G_KeyCmdResponder(ev);

  // are we displaying a widget?

  if(current_menuwidget)
    return
      current_menuwidget->responder ?
      current_menuwidget->responder(ev) : false;

  // are we inputting a new value into a variable?
  
  if(input_command)
    {
      unsigned char ch = ev->data1;
      variable_t *var = input_command->variable;
      
      if(ev->data1 == KEYD_ESCAPE)        // cancel input
	input_command = NULL;
      
      if(ev->data1 == KEYD_ENTER && input_buffer[0])
	{
	  char *temp;

	  // place " marks round the new value
	  temp = strdup(input_buffer);
	  sprintf(input_buffer, "\"%s\"", temp);
	  free(temp);

	  // set the command
	  cmdtype = c_menu;
	  C_RunCommand(input_command, input_buffer);
	  input_command = NULL;
	  return true; // eat key
	}

      // check for backspace
      if(ev->data1 == KEYD_BACKSPACE && input_buffer[0])
	{
	  input_buffer[strlen(input_buffer)-1] = '\0';
	  return true; // eatkey
	}
      // probably just a normal character
      
      // only care about valid characters
      // dont allow too many characters on one command line

      if(ch > 31 && ch < 127
         && (strlen(input_buffer) <=
           (var->type == vt_string ?
             var->max :
               (var->type == vt_int ? 10 : 20)
           )
         )
      )
	 {
	   input_buffer[strlen(input_buffer) + 1] = 0;
	   input_buffer[strlen(input_buffer)] = ch;
	 }
	   
      return true;
    } 

  if ((devparm && ev->data1 == key_help)
      || ev->data1 == key_screenshot)
    {
      G_ScreenShot ();
      return true;
    }                             
  
  if(ev->data1 == key_escape)
    {
      // toggle menu
      
      // start up main menu or kill menu
      if(menuactive) MN_ClearMenus();
      else MN_StartControlPanel();
      
      S_StartSound(NULL, menuactive ? sfx_swtchn : sfx_swtchx);

      return true;
    }

  if(MN_TempResponder(ev->data1)) return true;
  
  // not interested in keys if not in menu
  if(!menuactive) return G_KeyCmdResponder(ev);

  if(ev->data1 == key_menu_up)
    {
      // skip gaps
      do
	{
	  if(--current_menu->selected < 0)
            {
	      // jump to end of menu
              int i;
              for(i=0; current_menu->menuitems[i].type != it_end; i++);
              current_menu->selected = i-1;
            }
	}
      while(is_a_gap(&current_menu->menuitems[current_menu->selected]));
      
      S_StartSound(NULL,sfx_pstop);  // make sound
      
      return true;  // eatkey
    }
  
  if(ev->data1 == key_menu_down)
    {
      do
	{
	  ++current_menu->selected;
	  if(current_menu->menuitems[current_menu->selected].type == it_end)
	    {
	      current_menu->selected = 0;     // jump back to start
	    }
	}
      while(is_a_gap(&current_menu->menuitems[current_menu->selected]));
      
      S_StartSound(NULL,sfx_pstop);  // make sound
      
      return true;  // eatkey
    }
  
  if(ev->data1 == key_menu_enter)
    {
      menuitem_t *menuitem = &current_menu->menuitems[current_menu->selected];
      
      switch(menuitem->type)
	{
	case it_runcmd:
	  {
	    S_StartSound(NULL,sfx_pistol);  // make sound
	    cmdtype = c_menu;
	    C_RunTextCmd(menuitem->data);
	    break;
	  }
	
	case it_toggle:
        case it_togglehint:
	  {
	    // boolean values only toggled on enter
	    if(menuitem->var->type != vt_int ||
	       menuitem->var->max-menuitem->var->min > 1) break;
	    
	    // toggle value now
	    sprintf(tempstr, "%s /", menuitem->data);
	    cmdtype = c_menu;
	    C_RunTextCmd(tempstr);
	    
	    S_StartSound(NULL,sfx_pistol);  // make sound
	    break;
	  }
	
	case it_variable:
	  {
	    menuitem_t *menuitem =
	      &current_menu->menuitems[current_menu->selected];
	    
	    // get input for new value
	    input_command = C_GetCmdForName(menuitem->data);
	    input_buffer[0] = 0;             // clear input buffer
	    if(menuitem->var) strncpy(input_buffer, C_VariableStringValue(menuitem->var), sizeof(input_buffer) - 1);
	    break;
	  }

	case it_automap:
	  {
	    menuitem_t *menuitem =
	      &current_menu->menuitems[current_menu->selected];

	    MN_SelectColour(menuitem->data);

	    return true;
	  }

	case it_binding:
	  {
	     G_EditBinding(menuitem->data);
	      
	     return true;
	  }

	default: break;
	}
      return true;
    }
  
  if(ev->data1 == key_menu_backspace)
    {
      MN_PrevMenu();
      return true;          // eatkey
    }
  
  // decrease value of variable
  if(ev->data1 == key_menu_left)
    {
      menuitem_t *menuitem =
	&current_menu->menuitems[current_menu->selected];
      
      switch(menuitem->type)
	{
	case it_slider:
	case it_toggle:
        case it_togglehint:
	  {
	    // no on-off int values
	    if(menuitem->var->type == vt_int &&
	       menuitem->var->max-menuitem->var->min == 1) break;
	    
	    // change variable
	    sprintf(tempstr, "%s -", menuitem->data);
	    cmdtype = c_menu;
	    C_RunTextCmd(tempstr);
	  }
	default:
	  {
	    break;
	  }
	}
      return true;
    }
  
  // increase value of variable
  if(ev->data1 == key_menu_right)
    {
      menuitem_t *menuitem =
	&current_menu->menuitems[current_menu->selected];
      
      switch(menuitem->type)
	{
	case it_slider:
	case it_toggle:
        case it_togglehint:
	  {
	    // no on-off int values
	    if(menuitem->var->type == vt_int &&
	       menuitem->var->max-menuitem->var->min == 1) break;
	    
	    // change variable
	    sprintf(tempstr, "%s +", menuitem->data);
	    cmdtype = c_menu;
	    C_RunTextCmd(tempstr);
	  }
	
	default:
	  {
	    break;
	  }
	}
      return true;
    }

  // search for matching item in menu

  ch = tolower(ev->data1);
  if(ch >= 'a' && ch <= 'z')
    {
      
      // sf: experimented with various algorithms for this
      //     this one seems to work as it should

      int n = current_menu->selected;

      do
	{
	  n++;
	  if(current_menu->menuitems[n].type == it_end) n = 0; // loop round

	  // ignore unselectables
	  if(!is_a_gap(&current_menu->menuitems[n])) 
	    if(tolower(current_menu->menuitems[n].description[0]) == ch)
	      {
		// found a matching item!
		current_menu->selected = n;
		return true; // eat key
	      }
      	} while(n != current_menu->selected);
    }

  return G_KeyCmdResponder(ev);
}

///////////////////////////////////////////////////////////////////////////
//
// Other Menu Functions
//

// ?
void MN_ResetMenu()
{
}

// make menu 'clunk' sound on opening

void MN_ActivateMenu()
{
  if(!menuactive)  // activate menu if not already
    {
      menuactive = true;
      S_StartSound(NULL, sfx_swtchn);
    }
}

// start a menu:

void MN_StartMenu(menu_t *menu)
{
  if(!menuactive)
    {
      MN_ActivateMenu();
      current_menu = menu;
      menu_history_num = 0;  // reset history
    }
  else
    {
      menu_history[menu_history_num++] = current_menu;
      current_menu = menu;
    }
  
  menu_error_time = 0;      // clear error message
  redrawsbar = redrawborder = true;  // need redraw
}

// go back to a previous menu

void MN_PrevMenu()
{
  if(--menu_history_num < 0) MN_ClearMenus();
  else
    current_menu = menu_history[menu_history_num];
      
  menu_error_time = 0;          // clear errors
  redrawsbar = redrawborder = true;  // need redraw
  S_StartSound(NULL, sfx_swtchx);
}

// turn off menus
void MN_ClearMenus()
{
  menuactive = false;
  redrawsbar = redrawborder = true;  // need redraw
}

CONSOLE_COMMAND(mn_clearmenus, 0)
{
  MN_ClearMenus();
}

CONSOLE_COMMAND(mn_prevmenu, 0)
{
  MN_PrevMenu();
}

        // ??
void MN_ForcedLoadGame(const char *msg)
{
}

// display error msg in popup display at bottom of screen

void MN_ErrorMsg(char *s, ...)
{
  va_list args;
  
  va_start(args, s);
  vsprintf(menu_error_message, s, args);
  va_end(args);
  
  menu_error_time = 140;
}

// activate main menu

void MN_StartControlPanel()
{
#ifdef EPISINFO
  if (!stricmp(menu_layout, "doom"))
    {
      menu_main.x = 97;
      menu_main.y = gamemode == commercial ? 72 : 64;
      menu_main.flags |= mf_fixedlayout;
    }

  if (gamemode == commercial && menu_main.menuitems[5].type != it_end)
    {
      menu_main.menuitems[4] = menu_main.menuitems[5];
      menu_main.menuitems[5].type = it_end;
    }
#endif
  MN_StartMenu(&menu_main);
  
  S_StartSound(NULL,sfx_swtchn);
}

///////////////////////////////////////////////////////////////////////////
//
// Menu Font Drawing
//
// copy of V_* functions
// these do not leave a 1 pixel-gap between chars, I think it looks
// better for the menu

extern patch_t* v_font[V_FONTSIZE];

void MN_WriteText(unsigned char *s, int x, int y)
{
  int   w;
  unsigned char* ch;
  char *colour = cr_red;
  unsigned int c;
  int   cx;
  int   cy;
  patch_t *patch;

  ch = s;
  cx = x;
  cy = y;
  
  while(1)
    {
      c = *ch++;
      if (!c)
	break;
      if (c >= 128)     // new colour
      {
           colour = colrngs[c - 128];
           continue;
      }
      if (c == '\t')
        {
          cx = (cx/40)+1;
          cx = cx*40;
        }
      if (c == '\n')
	{
	  cx = x;
          cy += M_LINE;
	  continue;
	}
  
      c = toupper(c) - V_FONTSTART;
      // haleyjd  02/23/02: added null check
      if(c >= V_FONTSIZE || !v_font[c])
	{
	  cx += 4;
	  continue;
	}

      patch = v_font[c];
      if(!patch) continue;

      w = SHORT (patch->width);
      if (cx+w > SCREENWIDTH)
	break;

      V_DrawPatchTranslated(cx, cy, 0, patch, colour, 0);

      cx+=w-1;
    }
}

        // write text in a particular colour

void MN_WriteTextColoured(unsigned char *s, int colour, int x, int y)
{
   static char *tempstr = NULL;
   static int allocedsize=-1;

        // if string bigger than allocated, realloc bigger
   if(!tempstr || strlen(s) > allocedsize)
   {
      if(tempstr)       // already alloced?
        tempstr = realloc(tempstr, strlen(s) + 5);
      else
        tempstr = malloc(strlen(s) + 5);

      allocedsize = strlen(s);  // save for next time
   }

   tempstr[0] = 128 + colour;
   strcpy(&tempstr[1], s);

   MN_WriteText(tempstr, x, y);
}


int MN_StringWidth(unsigned char *s)
{
  int length = 0;
  unsigned char c;
  
  for(; *s; s++)
    {
      c = *s;
      if(c >= 128)         // colour
	continue;
      c = toupper(c) - V_FONTSTART;

      // haleyjd 02/23/02: restructured, added null ptr check
      if(c >= V_FONTSIZE || !v_font[c])
      {
	 length += 4;
      }
      else
	 length += SHORT(v_font[c]->width) - 1;
    }
  return length;
}

/////////////////////////////////////////////////////////////////////////
//
// Console Commands
//

extern void MN_AddMenus();              // mn_menus.c
extern void MN_AddMiscCommands();       // mn_misc.c

void MN_AddCommands()
{
  C_AddCommand(mn_clearmenus);
  C_AddCommand(mn_prevmenu);

  MN_AddMenus();               // add commands to call the menus
  MN_AddMiscCommands();
}

// EOF
