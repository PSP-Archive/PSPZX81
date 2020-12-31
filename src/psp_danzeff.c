#include <stdlib.h>
#include <string.h>
#include "psp_danzeff.h"
#include "SDL_image.h"
#include "global.h"
#include "kbd.h"
#include "psp_kbd.h"

#define false 0
#define true 1

# define PSP_KBD_MAX_SKIN    128

  int    psp_kbd_last_skin  = 0;
  int    psp_kbd_skin       = -1;
  char  *psp_kbd_skin_dir[PSP_KBD_MAX_SKIN];
  int    psp_kbd_skin_first = 1;

static /*bool*/ int holding = false;     //user is holding a button
static /*bool*/ int dirty = true;        //keyboard needs redrawing
static int mode = 0;             //charset selected. (0 - letters or 1 - numbers)
static /*bool*/ int initialized = false; //keyboard is initialized

//Position on the 3-3 grid the user has selected (range 0-2)
static int selected_x = 1;
static int selected_y = 1;

//Variable describing where each of the images is
#define guiStringsSize 15 /* size of guistrings array */
#define PICS_BASEDIR "/graphics/"

static char *guiStrings[] = 
{
	"/keys1.png", "/keys1_t.png", "/keys1_s.png",
	"/keys2.png", "/keys2_t.png", "/keys2_s.png",
	"/keys3.png", "/keys3_t.png", "/keys3_s.png",
	"/keys4.png", "/keys4_t.png", "/keys4_s.png",
	"/keys5.png", "/keys5_t.png", "/keys5_s.png"
};

#define MODE_COUNT 5
//this is the layout of the keyboard
static char modeChar[MODE_COUNT][3][3][2] = 
{
	{
		{ { '1', DANZEFF_EDIT }, { '2', DANZEFF_AND   }, { '3', DANZEFF_THEN     } },
    { { '4', DANZEFF_TO   }, { '5', DANZEFF_CUR_LEFT  }, { '6', DANZEFF_CUR_DOWN     } },
    { { '7', DANZEFF_CUR_UP   }, { '8', DANZEFF_CUR_RIGHT }, { '9', DANZEFF_GRAPHICS } }
	},

	{
		{ { '0', DANZEFF_RUBOUT }, { 'Q', DANZEFF_DBLDBLQUOTE }, { 'W', DANZEFF_OR } },
    { { 'E', DANZEFF_STEP   }, { 'R', DANZEFF_LE  }, { 'T', DANZEFF_NE     } },
    { { 'Y', DANZEFF_GE     }, { 'U', '$'           }, { 'I', '(' } }
	},

	{
		{ { 'O', ')'  }, { 'P', '"' }, { 'A', DANZEFF_STOP } },
    { { 'S', DANZEFF_LPRINT }, { 'D', DANZEFF_SLOW  }, { 'F', DANZEFF_FAST     } },
    { { 'G', DANZEFF_LLIST   }, { 'H', DANZEFF_DBLSTAR }, { 'J', '-' } }
	},

	{
		{ { 'K', '+'  }, { 'L', '=' }, { DANZEFF_FUNCTION, DANZEFF_ENTER } },
    { { 'Z', ':' }, { 'X', ';'  }, { 'C', '?'     } },
    { { 'V', '/'   }, { 'H', DANZEFF_DBLSTAR }, { 'B', '*' } }
	},
	{
		{ { 0  ,  0   }, { 'M', '>' }, { 0, 0 } },
    { { ' ', DANZEFF_POUND }, { '.', ','  }, { 'N', '<'     } },
    { { 0  , 0     }, { 0, 0 }, { 0, 0 } }
	}
	
};

int 
danzeff_isinitialized()
{
	return initialized;
}

int 
danzeff_dirty()
{
	return dirty;
}

/** Attempts to read a character from the controller
* If no character is pressed then we return 0
* Other special values: 1 = move left, 2 = move right, 3 = select, 4 = start
* Every other value should be a standard ascii value.
* An unsigned int is returned so in the future we can support unicode input
*/
unsigned int 
danzeff_readInput(SceCtrlData pspctrl)
{
	//Work out where the analog stick is selecting
	int x = 1;
	int y = 1;
	if (pspctrl.Lx < 85)     x -= 1;
	else if (pspctrl.Lx > 170) x += 1;
	
	if (pspctrl.Ly < 85)     y -= 1;
	else if (pspctrl.Ly > 170) y += 1;
	
	if (selected_x != x || selected_y != y) //If they've moved, update dirty
	{
		dirty = true;
		selected_x = x;
		selected_y = y;
	}

	unsigned int pressed = 0; //character they have entered, 0 as that means 'nothing'
	
	if (!holding)
	{
		if (pspctrl.Buttons& (PSP_CTRL_CROSS|PSP_CTRL_CIRCLE|PSP_CTRL_TRIANGLE|PSP_CTRL_SQUARE)) //pressing a char select button
		{
			int innerChoice = 0;
			if      (pspctrl.Buttons & PSP_CTRL_TRIANGLE)
				innerChoice = 1;
			else if (pspctrl.Buttons & PSP_CTRL_SQUARE)
				innerChoice = 0;
			else if (pspctrl.Buttons & PSP_CTRL_CROSS)
				innerChoice = 0;
			else //if (pspctrl.Buttons & PSP_CTRL_CIRCLE)
				innerChoice = 1;
			
			//Now grab the value out of the array
			pressed = modeChar[mode][y][x][innerChoice];
		}
		else if (pspctrl.Buttons& PSP_CTRL_RTRIGGER)
		{
			dirty = true;
			mode++;
			mode %= MODE_COUNT;
		}
		else if (pspctrl.Buttons& PSP_CTRL_LTRIGGER)
		{
			dirty = true;
      if (mode > 0) mode--;
      else          mode = MODE_COUNT - 1;
		}
		else if (pspctrl.Buttons& PSP_CTRL_LEFT)
		{
			pressed = DANZEFF_LEFT;
		}
		else if (pspctrl.Buttons& PSP_CTRL_RIGHT)
		{
			pressed = DANZEFF_RIGHT;
		}
		else if (pspctrl.Buttons& PSP_CTRL_DOWN)
		{
			pressed = DANZEFF_DOWN;
		}
		else if (pspctrl.Buttons& PSP_CTRL_UP)
		{
			pressed = DANZEFF_UP;
		}
		else if (pspctrl.Buttons& PSP_CTRL_SELECT)
		{
			pressed = DANZEFF_SELECT; //SELECT
		}
		else if (pspctrl.Buttons& PSP_CTRL_START)
		{
			pressed = DANZEFF_START; //START
		}
	}

	holding = pspctrl.Buttons & ~(PSP_CTRL_RTRIGGER|PSP_CTRL_LTRIGGER);
	
	return pressed;
}

///-----------------------------------------------------------
///These are specific to the implementation, they should have the same behaviour across implementations.
///-----------------------------------------------------------


///This is the original SDL implementation
#ifdef DANZEFF_SDL

static SDL_Surface* keyBits[guiStringsSize];
static int keyBitsSize = 0;
static int moved_x = 0, moved_y = 0; // location that we are moved to

///variable needed for rendering in SDL, the screen surface to draw to, and a function to set it!
static SDL_Surface* danzeff_screen;
static SDL_Rect danzeff_screen_rect;

void 
danzeff_set_screen(SDL_Surface* screen)
{
	danzeff_screen = screen;
	danzeff_screen_rect.x = 0;
	danzeff_screen_rect.y = 0;
	danzeff_screen_rect.h = screen->h;
	danzeff_screen_rect.w = screen->w;

  moved_x = danzeff_screen->w - 150;
  moved_y = danzeff_screen->h - 150;
}


///Internal function to draw a surface internally offset
//Render the given surface at the current screen position offset by screenX, screenY
//the surface will be internally offset by offsetX,offsetY. And the size of it to be drawn will be intWidth,intHeight
void 
surface_draw_offset(SDL_Surface* pixels, int screenX, int screenY, int offsetX, int offsetY, int intWidth, int intHeight)
{
	//move the draw position
	danzeff_screen_rect.x = moved_x + screenX;
	danzeff_screen_rect.y = moved_y + screenY;

	//Set up the rectangle
	SDL_Rect pixels_rect;
	pixels_rect.x = offsetX;
	pixels_rect.y = offsetY;
	pixels_rect.w = intWidth;
	pixels_rect.h = intHeight;
	
	SDL_BlitSurface(pixels, &pixels_rect, danzeff_screen, &danzeff_screen_rect);
}

///Draw a surface at the current moved_x, moved_y
void 
surface_draw(SDL_Surface* pixels)
{
	surface_draw_offset(pixels, 0, 0, 0, 0, pixels->w, pixels->h);
}

void
danzeff_init_skin()
{
  char skin_path[128];
  strcpy(skin_path, ZX81.zx81_home_dir);
  strcat(skin_path, PICS_BASEDIR);

  psp_kbd_last_skin = psp_fmgr_get_dir_list(skin_path, PSP_KBD_MAX_SKIN, psp_kbd_skin_dir) - 1;
 
  /* Should not happen ! */
  if (psp_kbd_last_skin < 0) {
    fprintf(stdout, "no keyboard skin in %s directory !\n", skin_path);
    exit(1);
  }

  if ((psp_kbd_skin == -1) || (psp_kbd_skin > psp_kbd_last_skin)) {
    psp_kbd_skin_first = 0;
    for (psp_kbd_skin = 0; psp_kbd_skin <= psp_kbd_last_skin; psp_kbd_skin++) {
      if (!strcasecmp(psp_kbd_skin_dir[psp_kbd_skin], "default/")) break;
    }
    if (psp_kbd_skin > psp_kbd_last_skin) psp_kbd_skin = 0;
  }
}

/* load all the guibits that make up the OSK */
int 
danzeff_load()
{
  char tmp_filename[128];

	if (initialized) return 1;

  if (psp_kbd_skin_first) {
    danzeff_init_skin();
  }
	
	int a;
	for (a = 0; a < guiStringsSize; a++)
	{
    strcpy(tmp_filename, ZX81.zx81_home_dir );
    strcat(tmp_filename, PICS_BASEDIR);
    strcat(tmp_filename, psp_kbd_skin_dir[psp_kbd_skin] );
    strcat(tmp_filename, guiStrings[a] );
		keyBits[a] = IMG_Load(tmp_filename);
		if (keyBits[a] == NULL)
		{
			//ERROR! out of memory.
			//free all previously created surfaces and set initialized to false
			int b;
			for (b = 0; b < a; b++)
			{
				SDL_FreeSurface(keyBits[b]);
				keyBits[b] = NULL;
			}
			initialized = false;
      printf("can't load image %s\n", tmp_filename); 
			exit(1);
		}
	}
	initialized = true;
  return 1;
}

/* remove all the guibits from memory */
void 
danzeff_free()
{
	if (!initialized) return;
	
	int a;
	for (a = 0; a < guiStringsSize; a++)
	{
		SDL_FreeSurface(keyBits[a]);
		keyBits[a] = NULL;
	}
	initialized = false;
}

/* draw the keyboard at the current position */
void 
danzeff_render()
{
	dirty = false;
	
	///Draw the background for the selected keyboard either transparent or opaque
	///this is the whole background image, not including the special highlighted area
	//if center is selected then draw the whole thing opaque
	if (selected_x == 1 && selected_y == 1)
		surface_draw(keyBits[3*mode]);
	else
		surface_draw(keyBits[3*mode + 1]);
	
	///Draw the current Highlighted Selector (orange bit)
	surface_draw_offset(keyBits[3*mode + 2], 
	//Offset from the current draw position to render at
	selected_x*50, selected_y*50, 
	//internal offset of the image
	selected_x*50,selected_y*50,
	//size to render (always the same)
	50, 50);
}

/* move the position the keyboard is currently drawn at */
void 
danzeff_moveTo(const int newX, const int newY)
{
	moved_x = danzeff_screen->w - 150 + newX;
	moved_y = danzeff_screen->h - 150 + newY;
}

void
danzeff_change_skin()
{
  danzeff_free();
  danzeff_load();
}

#endif //DANZEFF_SDL
