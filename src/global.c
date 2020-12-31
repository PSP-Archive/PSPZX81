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

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <zlib.h>

#include <pspctrl.h>
#include <pspkernel.h>
#include <pspdebug.h>

#include "global.h"
#include "psp_fmgr.h"
#include "kbd.h"

   ZX81_type ZX81;

   int psp_screenshot_mode = 0;

void
zx81_update_save_name(char *Name)
{
  char        TmpFileName[MAX_PATH];
# ifdef LINUX_MODE
  struct stat aStat;
# else
  SceIoStat   aStat;
# endif
  int         index;
  char       *SaveName;
  char       *Scan1;
  char       *Scan2;

  SaveName = strrchr(Name,'/');
  if (SaveName != (char *)0) SaveName++;
  else                       SaveName = Name;

  if (!strncasecmp(SaveName, "sav_", 4)) {
    Scan1 = SaveName + 4;
    Scan2 = strrchr(Scan1, '_');
    if (Scan2 && (Scan2[1] >= '0') && (Scan2[1] <= '5')) {
      strncpy(ZX81.zx81_save_name, Scan1, MAX_PATH);
      ZX81.zx81_save_name[Scan2 - Scan1] = '\0';
    } else {
      strncpy(ZX81.zx81_save_name, SaveName, MAX_PATH);
    }
  } else {
    strncpy(ZX81.zx81_save_name, SaveName, MAX_PATH);
  }

  if (ZX81.zx81_save_name[0] == '\0') {
    strcpy(ZX81.zx81_save_name,"default");
  }

  for (index = 0; index < ZX81_MAX_SAVE_STATE; index++) {
    ZX81.zx81_save_state[index].used  = 0;
    memset(&ZX81.zx81_save_state[index].date, 0, sizeof(ScePspDateTime));
    ZX81.zx81_save_state[index].thumb = 0;

    snprintf(TmpFileName, MAX_PATH, "%s/save/sav_%s_%d.zst", ZX81.zx81_home_dir, ZX81.zx81_save_name, index);
# ifdef LINUX_MODE
    if (! stat(TmpFileName, &aStat)) 
# else
    if (! sceIoGetstat(TmpFileName, &aStat))
# endif
    {
      ZX81.zx81_save_state[index].used = 1;
      ZX81.zx81_save_state[index].date = aStat.st_mtime;
      snprintf(TmpFileName, MAX_PATH, "%s/save/sav_%s_%d.png", ZX81.zx81_home_dir, ZX81.zx81_save_name, index);
# ifdef LINUX_MODE
      if (! stat(TmpFileName, &aStat)) 
# else
      if (! sceIoGetstat(TmpFileName, &aStat))
# endif
      {
        if (psp_sdl_load_thumb_png(ZX81.zx81_save_state[index].surface, TmpFileName)) {
          ZX81.zx81_save_state[index].thumb = 1;
        }
      }
    }
  }
}

void
reset_save_name()
{
  zx81_update_save_name("");
}

void
zx81_kbd_load(void)
{
  char        TmpFileName[MAX_PATH + 1];
  struct stat aStat;

  snprintf(TmpFileName, MAX_PATH, "%s/kbd/%s.kbd", ZX81.zx81_home_dir, ZX81.zx81_save_name );
  if (! stat(TmpFileName, &aStat)) {
    psp_kbd_load_mapping(TmpFileName);
  }
}

int
zx81_kbd_save(void)
{
  char TmpFileName[MAX_PATH + 1];
  snprintf(TmpFileName, MAX_PATH, "%s/kbd/%s.kbd", ZX81.zx81_home_dir, ZX81.zx81_save_name );
  return( psp_kbd_save_mapping(TmpFileName) );
}

void
emulator_reset(void)
{
  reset81();
  reset_save_name();
}

void
zx81_default_settings()
{
  //LUDO:
  ZX81.zx81_render_mode   = ZX81_RENDER_NORMAL;
  ZX81.zx81_speed_limiter = 50;
  ZX81.psp_reverse_analog  = 0;
  ZX81.psp_cpu_clock       = 222;
  ZX81.psp_screenshot_id   = 0;
  ZX81.psp_display_lr      = 0;
  ZX81.zx81_color_mode     = ZX81_COLOR_BLUE;
  ZX81.zx81_view_fps       = 0;
  ZX81.zx81_vsync          = 0;

  scePowerSetClockFrequency(ZX81.psp_cpu_clock, ZX81.psp_cpu_clock, ZX81.psp_cpu_clock/2);
}

static int
loc_zx81_save_settings(char *chFileName)
{
  FILE* FileDesc;
  int   error = 0;

  FileDesc = fopen(chFileName, "w");
  if (FileDesc != (FILE *)0 ) {

    fprintf(FileDesc, "psp_cpu_clock=%d\n"      , ZX81.psp_cpu_clock);
    fprintf(FileDesc, "psp_reverse_analog=%d\n" , ZX81.psp_reverse_analog);
    fprintf(FileDesc, "psp_display_lr=%d\n"     , ZX81.psp_display_lr);
    fprintf(FileDesc, "psp_skip_max_frame=%d\n" , ZX81.psp_skip_max_frame);
    fprintf(FileDesc, "zx81_view_fps=%d\n"      , ZX81.zx81_view_fps);
    fprintf(FileDesc, "zx81_vsync=%d\n"         , ZX81.zx81_vsync);
    fprintf(FileDesc, "zx81_render_mode=%d\n"  , ZX81.zx81_render_mode);
    fprintf(FileDesc, "zx81_color_mode=%d\n"  , ZX81.zx81_color_mode);
    fprintf(FileDesc, "zx81_speed_limiter=%d\n", ZX81.zx81_speed_limiter);

    fclose(FileDesc);

  } else {
    error = 1;
  }

  return error;
}

int
zx81_save_settings(void)
{
  char  FileName[MAX_PATH+1];
  int   error;

  error = 1;

  snprintf(FileName, MAX_PATH, "%s/set/%s.set", ZX81.zx81_home_dir, ZX81.zx81_save_name);
  error = loc_zx81_save_settings(FileName);

  return error;
}

static int
loc_zx81_load_settings(char *chFileName)
{
  char  Buffer[512];
  char *Scan;
  unsigned int Value;
  FILE* FileDesc;

  FileDesc = fopen(chFileName, "r");
  if (FileDesc == (FILE *)0 ) return 0;

  while (fgets(Buffer,512, FileDesc) != (char *)0) {

    Scan = strchr(Buffer,'\n');
    if (Scan) *Scan = '\0';
    /* For this #@$% of windows ! */
    Scan = strchr(Buffer,'\r');
    if (Scan) *Scan = '\0';
    if (Buffer[0] == '#') continue;

    Scan = strchr(Buffer,'=');
    if (! Scan) continue;

    *Scan = '\0';
    Value = atoi(Scan+1);

    if (!strcasecmp(Buffer,"psp_cpu_clock"))      ZX81.psp_cpu_clock = Value;
    else
    if (!strcasecmp(Buffer,"psp_reverse_analog")) ZX81.psp_reverse_analog = Value;
    else
    if (!strcasecmp(Buffer,"psp_display_lr"))     ZX81.psp_display_lr = Value;
    else
    if (!strcasecmp(Buffer,"zx81_view_fps"))     ZX81.zx81_view_fps = Value;
    else
    if (!strcasecmp(Buffer,"zx81_vsync"))     ZX81.zx81_vsync = Value;
    else
    if (!strcasecmp(Buffer,"psp_skip_max_frame")) ZX81.psp_skip_max_frame = Value;
    else
    if (!strcasecmp(Buffer,"zx81_render_mode"))  ZX81.zx81_render_mode = Value;
    else
    if (!strcasecmp(Buffer,"zx81_color_mode"))  ZX81.zx81_color_mode = Value;
    else
    if (!strcasecmp(Buffer,"zx81_speed_limiter"))  ZX81.zx81_speed_limiter = Value;
  }

  fclose(FileDesc);

  scePowerSetClockFrequency(ZX81.psp_cpu_clock, ZX81.psp_cpu_clock, ZX81.psp_cpu_clock/2);

  return 0;
}

int
zx81_load_settings()
{
  char  FileName[MAX_PATH+1];
  int   error;

  error = 1;

  snprintf(FileName, MAX_PATH, "%s/set/%s.set", ZX81.zx81_home_dir, ZX81.zx81_save_name);
  error = loc_zx81_load_settings(FileName);

  return error;
}

int
zx81_load_file_settings(char *FileName)
{
  return loc_zx81_load_settings(FileName);
}


int
zx81_snapshot_save_slot(int save_id)
{
  char      FileName[MAX_PATH+1];
# ifdef LINUX_MODE
  struct stat aStat;
# else
  SceIoStat   aStat;
# endif
  int       error;

  error = 1;

  if (save_id < ZX81_MAX_SAVE_STATE) {
    snprintf(FileName, MAX_PATH, "%s/save/sav_%s_%d.zst", ZX81.zx81_home_dir, ZX81.zx81_save_name, save_id);
    error = zx81_save_state(FileName);
    if (! error) {
# ifdef LINUX_MODE
      if (! stat(FileName, &aStat)) 
# else
      if (! sceIoGetstat(FileName, &aStat))
# endif
      {
        ZX81.zx81_save_state[save_id].used = 1;
        ZX81.zx81_save_state[save_id].thumb = 0;
        ZX81.zx81_save_state[save_id].date  = aStat.st_mtime;
        snprintf(FileName, MAX_PATH, "%s/save/sav_%s_%d.png", ZX81.zx81_home_dir, ZX81.zx81_save_name, save_id);
        if (psp_sdl_save_thumb_png(ZX81.zx81_save_state[save_id].surface, FileName)) {
          ZX81.zx81_save_state[save_id].thumb = 1;
        }
      }
    }
  }

  return error;
}

int
zx81_snapshot_load_slot(int load_id)
{
  char  FileName[MAX_PATH+1];
  int   error;

  error = 1;

  if (load_id < ZX81_MAX_SAVE_STATE) {
    snprintf(FileName, MAX_PATH, "%s/save/sav_%s_%d.zst", ZX81.zx81_home_dir, ZX81.zx81_save_name, load_id);
    error = zx81_load_state(FileName);
  }
  return error;
}

int
zx81_snapshot_del_slot(int save_id)
{
  char  FileName[MAX_PATH+1];
# ifdef LINUX_MODE
  struct stat aStat;
# else
  SceIoStat   aStat;
# endif
  int   error;

  error = 1;

  if (save_id < ZX81_MAX_SAVE_STATE) {
    snprintf(FileName, MAX_PATH, "%s/save/sav_%s_%d.zst", ZX81.zx81_home_dir, ZX81.zx81_save_name, save_id);
    error = remove(FileName);
    if (! error) {
      ZX81.zx81_save_state[save_id].used  = 0;
      ZX81.zx81_save_state[save_id].thumb = 0;
      memset(&ZX81.zx81_save_state[save_id].date, 0, sizeof(ScePspDateTime));

      snprintf(FileName, MAX_PATH, "%s/save/sav_%s_%d.png", ZX81.zx81_home_dir, ZX81.zx81_save_name, save_id);
# ifdef LINUX_MODE
      if (! stat(FileName, &aStat)) 
# else
      if (! sceIoGetstat(FileName, &aStat))
# endif
      {
        remove(FileName);
      }
    }
  }

  return error;
}

void
zx81_initialize(void)
{
  memset(&ZX81, 0, sizeof(ZX81_type));
  getcwd(ZX81.zx81_home_dir,MAX_PATH);
  zx81_default_settings();

  psp_sdl_init();

  zx81_update_save_name("");
  zx81_load_settings();

  scePowerSetClockFrequency(ZX81.psp_cpu_clock, ZX81.psp_cpu_clock, ZX81.psp_cpu_clock/2);
}

void
zx81_treat_command_key(int zx81_idx)
{
  int new_render;

  switch (zx81_idx) 
  {
    case ZX81C_FPS: ZX81.zx81_view_fps = ! ZX81.zx81_view_fps;
    break;
    case ZX81C_JOY: ZX81.psp_reverse_analog = ! ZX81.psp_reverse_analog;
    break;
    case ZX81C_RENDER: 
      psp_sdl_black_screen();
      new_render = ZX81.zx81_render_mode + 1;
      if (new_render > ZX81_LAST_RENDER) new_render = 0;
      ZX81.zx81_render_mode = new_render;
    break;
    case ZX81C_LOAD: psp_main_menu_load_current();
    break;
    case ZX81C_SAVE: psp_main_menu_save_current(); 
    break;
    case ZX81C_RESET: 
       psp_sdl_black_screen();
       emulator_reset(); 
       reset_save_name();
    break;
    case ZX81C_SCREEN: psp_screenshot_mode = 10;
    break;
  }
}
