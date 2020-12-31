/* z81/xz81, Linux console and X ZX81/ZX80 emulators.
 * Copyright (C) 1994 Ian Collier. xz81 changes (C) 1995-2001 Russell Marks.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* most of this file is heavily based on xz80's xspectrum.c,
 * to the extent that some of the indentation is a crazy
 * mix-and-match affair. :-)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <psptypes.h>
#include "SDL.h"


#include "common.h"
#include "global.h"
#include "kbd.h"
#include "z80.h"
#include "allmain.h"
#include "psp_sdl.h"

#define MAX_DISP_LEN 256

int mitshm=1;

void 
closedown()
{
  psp_sdl_exit(0);
}


void 
exit_program(void)
{
  closedown();
  exit(0);
}

static int 
image_init()
{
   return 0;
}


static void 
notify()
{
}

void 
doCleanUp (void)
{
  //audio_shutdown();
  psp_sdl_exit(0);
}


void 
startup()
{
  refresh_screen=1;
}

/* x is in chars, y in pixels. d is byte with bitmap in. */
static inline void 
drawpix_blit(int x,int y,int d, u16 bg_color, u16 fg_color)
{
  u16 *vram_ptr = (u16 *)blit_surface->pixels;
  vram_ptr += (y * ZX81_WIDTH) + x*8;
  int mask=256;
  while (mask>>=1) {
    *vram_ptr++=(d&mask)?fg_color:bg_color;
  }
}

/* draw everything which has changed, then find the smallest rectangle
 * which covers all the changes, and update that.
 */
void 
update_scrn_blit()
{
  unsigned char *ptr;
  int x,y,d;
  int xmin,ymin,xmax,ymax;
     
  u16 bg_color;
  u16 fg_color;

  xmax=0;
  ymax=0;
  xmin=ZX_VID_X_WIDTH-1;
  ymin=ZX_VID_X_HEIGHT-1;

  if (ZX81.zx81_color_mode == ZX81_COLOR_BLUE) { 
    fg_color = psp_sdl_rgb(0xff,0xff,0xff);
    bg_color = psp_sdl_rgb(0,0,0xff); 
  } else
  if (ZX81.zx81_color_mode == ZX81_COLOR_WHITE) { 
    fg_color = 0x0; 
    bg_color = psp_sdl_rgb(0xff,0xff,0xff); 
  } else
  if (ZX81.zx81_color_mode == ZX81_COLOR_BLACK) { 
    bg_color = 0x0; 
    fg_color = psp_sdl_rgb(0xff,0xff,0xff); 
  } else {
    fg_color = 0x0; 
    bg_color = psp_sdl_rgb(0x80,0x80,0x80);
  }


  for(y=0;y<ZX_VID_X_HEIGHT;y++)
  {
    ptr=scrnbmp+(y+ZX_VID_X_YOFS)*ZX_VID_FULLWIDTH/8+ZX_VID_X_XOFS/8;

    for(x=0;x<ZX_VID_X_WIDTH;x+=8,ptr++)
    {
      d=*ptr;
      /* update size of area to be drawn */
      if(x<xmin) xmin=x;
      if(y<ymin) ymin=y;
      if(x+7>xmax) xmax=x+7;
      if(y>ymax) ymax=y;
      drawpix_blit(x>>3, y, d, bg_color, fg_color);
    }
  }
  xmin=0,ymin=0,xmax=ZX_VID_X_WIDTH-1,ymax=ZX_VID_X_HEIGHT-1;
}

/* x is in chars, y in pixels. d is byte with bitmap in. */
static inline void 
drawpix_normal(int x,int y,int d, u16 bg_color, u16 fg_color)
{
  u16 *vram_ptr = (ushort *)back_surface->pixels;
  vram_ptr += 86 + (x*8) + ((y+20) * PSP_LINE_SIZE);
  int mask=256;
  while (mask>>=1) {
    *vram_ptr++=(d&mask)?fg_color:bg_color;
  }
}

/* draw everything which has changed, then find the smallest rectangle
 * which covers all the changes, and update that.
 */
void 
update_scrn_normal()
{
  unsigned char *ptr;
  int x,y,d;
  int xmin,ymin,xmax,ymax;
     
  u16 bg_color;
  u16 fg_color;

  xmax=0;
  ymax=0;
  xmin=ZX_VID_X_WIDTH-1;
  ymin=ZX_VID_X_HEIGHT-1;

  if (ZX81.zx81_color_mode == ZX81_COLOR_BLUE) { 
    fg_color = psp_sdl_rgb(0xff,0xff,0xff);
    bg_color = psp_sdl_rgb(0,0,0xff); 
  } else
  if (ZX81.zx81_color_mode == ZX81_COLOR_WHITE) { 
    fg_color = 0x0; 
    bg_color = psp_sdl_rgb(0xff,0xff,0xff); 
  } else
  if (ZX81.zx81_color_mode == ZX81_COLOR_BLACK) { 
    bg_color = 0x0; 
    fg_color = psp_sdl_rgb(0xff,0xff,0xff); 
  } else {
    fg_color = 0x0; 
    bg_color = psp_sdl_rgb(0x80,0x80,0x80);
  }


  for(y=0;y<ZX_VID_X_HEIGHT;y++)
  {
    ptr=scrnbmp+(y+ZX_VID_X_YOFS)*ZX_VID_FULLWIDTH/8+ZX_VID_X_XOFS/8;

    for(x=0;x<ZX_VID_X_WIDTH;x+=8,ptr++)
    {
      d=*ptr;
      /* update size of area to be drawn */
      if(x<xmin) xmin=x;
      if(y<ymin) ymin=y;
      if(x+7>xmax) xmax=x+7;
      if(y>ymax) ymax=y;
      drawpix_normal(x>>3, y, d, bg_color, fg_color);
    }
  }
  xmin=0,ymin=0,xmax=ZX_VID_X_WIDTH-1,ymax=ZX_VID_X_HEIGHT-1;
}

void
update_scrn_fit_height()
{
  SDL_Rect srcRect;
  SDL_Rect dstRect;

  srcRect.x = 0;
  srcRect.y = 0;
  srcRect.w = ZX81_WIDTH;
  srcRect.h = ZX81_HEIGHT;
  dstRect.x = 62;
  dstRect.y = 0;
  dstRect.w = 356;
  dstRect.h = 272;

  update_scrn_blit();
  psp_sdl_gu_stretch(&srcRect, &dstRect);
}

void
update_scrn_fit()
{
  SDL_Rect srcRect;
  SDL_Rect dstRect;

  srcRect.x = 0;
  srcRect.y = 0;
  srcRect.w = ZX81_WIDTH;
  srcRect.h = ZX81_HEIGHT;
  dstRect.x = 0;
  dstRect.y = 0;
  dstRect.w = 480;
  dstRect.h = 272;

  update_scrn_blit();
  psp_sdl_gu_stretch(&srcRect, &dstRect);
}

# if 0
void
update_scrn_normal()
{
  SDL_Rect srcRect;
  SDL_Rect dstRect;

  srcRect.x = 0;
  srcRect.y = 0;
  srcRect.w = ZX81_WIDTH;
  srcRect.h = ZX81_HEIGHT;
  dstRect.x = 86;
  dstRect.y = 20;
  dstRect.w = ZX81_WIDTH;
  dstRect.h = ZX81_HEIGHT;

  update_scrn_blit();
  psp_sdl_gu_stretch(&srcRect, &dstRect);
}
# endif

void
update_scrn()
{
  if (ZX81.zx81_render_mode == ZX81_RENDER_NORMAL   ) update_scrn_normal();
  else
  if (ZX81.zx81_render_mode == ZX81_RENDER_FIT_HEIGHT) update_scrn_fit_height();
  else
  if (ZX81.zx81_render_mode == ZX81_RENDER_FIT      ) update_scrn_fit();
}

int 
SDL_main(int argc,char **argv)
{
  zx81_initialize();

  startup();
  initmem();

  mainloop();

  /* doesn't ever get here really, but all the same... */
  exit_program();

  return 0;
}
