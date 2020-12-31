/* ZX81 - Emulator

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <SDL.h>

# define ZX81_RENDER_NORMAL     0
# define ZX81_RENDER_FIT_HEIGHT 1
# define ZX81_RENDER_FIT        2
# define ZX81_LAST_RENDER       2

# define ZX81_COLOR_BLUE        0
# define ZX81_COLOR_WHITE       1
# define ZX81_COLOR_BLACK       2
# define ZX81_COLOR_GRAY        3
# define ZX81_LAST_COLOR        3

# define MAX_PATH   256
# define ZX81_MAX_SAVE_STATE 5

#include <psptypes.h>

  typedef struct zx81_save_t {

    SDL_Surface    *surface;
    char            used;
    char            thumb;
    ScePspDateTime  date;

  } zx81_save_t;

  typedef struct ZX81_type {
 
    zx81_save_t zx81_save_state[ZX81_MAX_SAVE_STATE];

    char zx81_save_name[MAX_PATH];
    char zx81_home_dir[MAX_PATH];
    int  psp_screenshot_id;
    int  psp_cpu_clock;
    int  psp_reverse_analog;
    int  psp_display_lr;
    int  zx81_view_fps;
    int  zx81_current_fps;
    int  zx81_render_mode;
    int  zx81_vsync;
    int  psp_skip_max_frame;
    int  psp_skip_cur_frame;
    int  zx81_color_mode;
    int  zx81_speed_limiter;

  } ZX81_type;

  extern ZX81_type ZX81;

  extern int psp_screenshot_mode;

  extern int zx81_save_configuration(void);
  extern int zx81_parse_configuration(void);
  extern void zx81_update_save_name(char *Name);
  extern void reset_save_name();
  extern void zx81_kbd_load(void);
  extern int zx81_kbd_save(void);
  extern void emulator_reset(void);
  extern int zx81_load_rom(char *FileName, int zip_format);
  extern int zx81_load_state(char *FileName);
  extern int zx81_snapshot_save_slot(int save_id);
  extern int zx81_snapshot_load_slot(int load_id);
  extern int zx81_snapshot_del_slot(int save_id);

#endif
