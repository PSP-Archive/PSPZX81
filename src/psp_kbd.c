/*
 *  Copyright (C) 2006 Ludovic Jacomme (ludovic.jacomme@gmail.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <fcntl.h>

#include <pspctrl.h>
#include <pspkernel.h>
#include <pspdebug.h>
#include <SDL.h>

#include "global.h"
#include "kbd.h"
#include "psp_kbd.h"
#include "psp_menu.h"
#include "psp_sdl.h"
#include "psp_danzeff.h"
#include "psp_irkeyb.h"
#include "psp_battery.h"

# define KBD_MIN_ANALOG_TIME      150000
# define KBD_MIN_START_TIME       800000
# define KBD_MAX_EVENT_TIME       500000
# define KBD_MIN_PENDING_TIME     300000
# define KBD_MIN_HOTKEY_TIME     1000000
# define KBD_MIN_IR_PENDING_TIME  400000
# define KBD_MIN_IR_TIME          150000
# define KBD_MIN_DANZEFF_TIME     150000
# define KBD_MIN_COMMAND_TIME     100000
# define KBD_MIN_BATTCHECK_TIME 90000000 

 static SceCtrlData    loc_button_data;
 static unsigned int   loc_last_event_time = 0;
 static unsigned int   loc_last_hotkey_time = 0;
#ifdef USE_PSP_IRKEYB
 static unsigned int   loc_last_irkbd_event_time = 0;
#endif
 static unsigned int   loc_last_analog_time = 0;
 static long           first_time_stamp = -1;
 static long           first_time_batt_stamp = -1;
 static long           first_time_auto_stamp = -1;
 static char           loc_button_press[ KBD_MAX_BUTTONS ]; 
 static char           loc_button_release[ KBD_MAX_BUTTONS ]; 
 static unsigned int   loc_button_mask[ KBD_MAX_BUTTONS ] =
 {
   PSP_CTRL_UP         , /*  KBD_UP         */
   PSP_CTRL_RIGHT      , /*  KBD_RIGHT      */
   PSP_CTRL_DOWN       , /*  KBD_DOWN       */
   PSP_CTRL_LEFT       , /*  KBD_LEFT       */
   PSP_CTRL_TRIANGLE   , /*  KBD_TRIANGLE   */
   PSP_CTRL_CIRCLE     , /*  KBD_CIRCLE     */
   PSP_CTRL_CROSS      , /*  KBD_CROSS      */
   PSP_CTRL_SQUARE     , /*  KBD_SQUARE     */
   PSP_CTRL_SELECT     , /*  KBD_SELECT     */
   PSP_CTRL_START      , /*  KBD_START      */
   PSP_CTRL_HOME       , /*  KBD_HOME       */
   PSP_CTRL_HOLD       , /*  KBD_HOLD       */
   PSP_CTRL_LTRIGGER   , /*  KBD_LTRIGGER   */
   PSP_CTRL_RTRIGGER   , /*  KBD_RTRIGGER   */
 };

  static char loc_button_name[ KBD_ALL_BUTTONS ][10] =
  {
    "UP",
    "RIGHT",
    "DOWN",
    "LEFT",
    "TRIANGLE",
    "CIRCLE",
    "CROSS",
    "SQUARE",
    "SELECT",
    "START",
    "HOME",
    "HOLD",
    "LTRIGGER",
    "RTRIGGER",
    "JOY_UP",
    "JOY_RIGHT",
    "JOY_DOWN",
    "JOY_LEFT"
 };

 static char loc_button_name_L[ KBD_ALL_BUTTONS ][20] =
 {
   "L_UP",
   "L_RIGHT",
   "L_DOWN",
   "L_LEFT",
   "L_TRIANGLE",
   "L_CIRCLE",
   "L_CROSS",
   "L_SQUARE",
   "L_SELECT",
   "L_START",
   "L_HOME",
   "L_HOLD",
   "L_LTRIGGER",
   "L_RTRIGGER",
   "L_JOY_UP",
   "L_JOY_RIGHT",
   "L_JOY_DOWN",
   "L_JOY_LEFT"
 };
 
  static char loc_button_name_R[ KBD_ALL_BUTTONS ][20] =
 {
   "R_UP",
   "R_RIGHT",
   "R_DOWN",
   "R_LEFT",
   "R_TRIANGLE",
   "R_CIRCLE",
   "R_CROSS",
   "R_SQUARE",
   "R_SELECT",
   "R_START",
   "R_HOME",
   "R_HOLD",
   "R_LTRIGGER",
   "R_RTRIGGER",
   "R_JOY_UP",
   "R_JOY_RIGHT",
   "R_JOY_DOWN",
   "R_JOY_LEFT"
 };
 
 struct zx81_key_trans psp_zx81_key_to_name[ZX81_MAX_KEY]=
 {
   { ZX81_SPACE,       "SPACE" },
   { ZX81_EXCLAMATN,   "!" },
   { ZX81_DBLQUOTE,    "\"" },
   { ZX81_HASH,        "#" },
   { ZX81_DOLLAR,      "$" },
   { ZX81_PERCENT,     "%" },
   { ZX81_AMPERSAND,   "&" },
   { ZX81_QUOTE,       "'" },
   { ZX81_LEFTPAREN,   "(" },
   { ZX81_RIGHTPAREN,  ")" },
   { ZX81_STAR,        "*" },
   { ZX81_PLUS,        "+" },
   { ZX81_COMMA,       "," },
   { ZX81_MINUS,       "-" },
   { ZX81_PERIOD,      "." },
   { ZX81_SLASH,       "/" },
   { ZX81_0,           "0" },
   { ZX81_1,           "1" },
   { ZX81_2,           "2" },
   { ZX81_3,           "3" },
   { ZX81_4,           "4" },
   { ZX81_5,           "5" },
   { ZX81_6,           "6" },
   { ZX81_7,           "7" },
   { ZX81_8,           "8" },
   { ZX81_9,           "9" },
   { ZX81_COLON,       ":" },
   { ZX81_SEMICOLON,   ";" },
   { ZX81_LESS,        "<" },
   { ZX81_EQUAL,       "=" },
   { ZX81_GREATER,     ">" },
   { ZX81_QUESTION,    "?" },
   { ZX81_AT,          "@" },
   { ZX81_A,           "A" },
   { ZX81_B,           "B" },
   { ZX81_C,           "C" },
   { ZX81_D,           "D" },
   { ZX81_E,           "E" },
   { ZX81_F,           "F" },
   { ZX81_G,           "G" },
   { ZX81_H,           "H" },
   { ZX81_I,           "I" },
   { ZX81_J,           "J" },
   { ZX81_K,           "K" },
   { ZX81_L,           "L" },
   { ZX81_M,           "M" },
   { ZX81_N,           "N" },
   { ZX81_O,           "O" },
   { ZX81_P,           "P" },
   { ZX81_Q,           "Q" },
   { ZX81_R,           "R" },
   { ZX81_S,           "S" },
   { ZX81_T,           "T" },
   { ZX81_U,           "U" },
   { ZX81_V,           "V" },
   { ZX81_W,           "W" },
   { ZX81_X,           "X" },
   { ZX81_Y,           "Y" },
   { ZX81_Z,           "Z" },
   { ZX81_LBRACKET,    "[" },
   { ZX81_BACKSLASH,   "BACKSLASH" },
   { ZX81_RBRACKET,    "]" },
   { ZX81_POWER,       "^" },
   { ZX81_UNDERSCORE,  "_" },
   { ZX81_BACKQUOTE,   "`" },
   { ZX81_a,           "a" },
   { ZX81_b,           "b" },
   { ZX81_c,           "c" },
   { ZX81_d,           "d" },
   { ZX81_e,           "e" },
   { ZX81_f,           "f" },
   { ZX81_g,           "g" },
   { ZX81_h,           "h" },
   { ZX81_i,           "i" },
   { ZX81_j,           "j" },
   { ZX81_k,           "k" },
   { ZX81_l,           "l" },
   { ZX81_m,           "m" },
   { ZX81_n,           "n" },
   { ZX81_o,           "o" },
   { ZX81_p,           "p" },
   { ZX81_q,           "q" },
   { ZX81_r,           "r" },
   { ZX81_s,           "s" },
   { ZX81_t,           "t" },
   { ZX81_u,           "u" },
   { ZX81_v,           "v" },
   { ZX81_w,           "w" },
   { ZX81_x,           "x" },
   { ZX81_y,           "y" },
   { ZX81_z,           "z" },
   { ZX81_LCBRACE,     "[" },
   { ZX81_PIPE,        "|" },
   { ZX81_RCBRACE,     "]" },
   { ZX81_TILDA,       "~" },
   { ZX81_DEL,         "DEL" },
   { ZX81_ENTER,       "ENTER" },
   { ZX81_SHIFT,       "SHIFT" },
   { ZX81_EDIT,        "EDIT" },
   { ZX81_AND,         "AND" },
   { ZX81_THEN,        "THEN" },
   { ZX81_TO,          "TO" },
   { ZX81_LEFT,        "LEFT" },
   { ZX81_DOWN,        "DOWN" },
   { ZX81_UP,          "UP" },
   { ZX81_RIGHT,       "RIGHT" },
   { ZX81_GRAPHICS,    "GRAPHICS" },
   { ZX81_RUBOUT,      "RUBOUT" },
   { ZX81_DBLDBLQUOTE, "\"\"" },
   { ZX81_OR,          "OR" },
   { ZX81_STEP,        "STEP" },
   { ZX81_LE,          "<=" },
   { ZX81_NE,          "<>" },
   { ZX81_GE,          ">=" },
   { ZX81_STOP,        "STOP" },
   { ZX81_LPRINT,      "LPRINT" },
   { ZX81_SLOW,        "SLOW" },
   { ZX81_FAST,        "FAST" },
   { ZX81_LLIST,       "LLIST" },
   { ZX81_DBLSTAR,     "DBLSTAR" },
   { ZX81_FUNCTION,    "FUNCTION" },
   { ZX81_POUND,       "POUND" },
   { ZX81C_FPS,        "C_FPS" },
   { ZX81C_JOY,        "C_JOY" },
   { ZX81C_RENDER,     "C_RENDER" },
   { ZX81C_LOAD,       "C_LOAD" },
   { ZX81C_SAVE,       "C_SAVE" },
   { ZX81C_RESET,      "C_RESET" },
   { ZX81C_SCREEN,     "C_SCREEN" }
 };


 static int loc_default_mapping[ KBD_ALL_BUTTONS ] = {
   ZX81_UP              , /*  KBD_UP         */
   ZX81_RIGHT           , /*  KBD_RIGHT      */
   ZX81_DOWN            , /*  KBD_DOWN       */
   ZX81_LEFT            , /*  KBD_LEFT       */
   ZX81_ENTER           , /*  KBD_TRIANGLE   */
   ZX81_Y               , /*  KBD_CIRCLE     */
   ZX81_N               , /*  KBD_CROSS      */
   ZX81_DEL             , /*  KBD_SQUARE     */
   -1                   , /*  KBD_SELECT     */
   -1                   , /*  KBD_START      */
   -1                   , /*  KBD_HOME       */
   -1                   , /*  KBD_HOLD       */
   KBD_LTRIGGER_MAPPING , /*  KBD_LTRIGGER   */
   KBD_RTRIGGER_MAPPING , /*  KBD_RTRIGGER   */
   ZX81_A               , /*  KBD_JOY_UP     */
   ZX81_B               , /*  KBD_JOY_RIGHT  */
   ZX81_C               , /*  KBD_JOY_DOWN   */
   ZX81_D                 /*  KBD_JOY_LEFT   */
 };

 static int loc_default_mapping_L[ KBD_ALL_BUTTONS ] = {
   ZX81_UP              , /*  KBD_UP         */
   ZX81C_RENDER         , /*  KBD_RIGHT      */
   ZX81_DOWN            , /*  KBD_DOWN       */
   ZX81C_RENDER         , /*  KBD_LEFT       */
   ZX81C_LOAD           , /*  KBD_TRIANGLE   */
   ZX81C_JOY            , /*  KBD_CIRCLE     */
   ZX81C_SAVE           , /*  KBD_CROSS      */
   ZX81C_FPS            , /*  KBD_SQUARE     */
   -1                   , /*  KBD_SELECT     */
   -1                   , /*  KBD_START      */
   -1                   , /*  KBD_HOME       */
   -1                   , /*  KBD_HOLD       */
   KBD_LTRIGGER_MAPPING , /*  KBD_LTRIGGER   */
   KBD_RTRIGGER_MAPPING , /*  KBD_RTRIGGER   */
   ZX81_A               , /*  KBD_JOY_UP     */
   ZX81_B               , /*  KBD_JOY_RIGHT  */
   ZX81_C               , /*  KBD_JOY_DOWN   */
   ZX81_D                 /*  KBD_JOY_LEFT   */
 };

 static int loc_default_mapping_R[ KBD_ALL_BUTTONS ] = {
   ZX81_UP              , /*  KBD_UP         */
   ZX81_RIGHT           , /*  KBD_RIGHT      */
   ZX81_DOWN            , /*  KBD_DOWN       */
   ZX81_LEFT            , /*  KBD_LEFT       */
   ZX81_ENTER           , /*  KBD_TRIANGLE   */
   ZX81_Y               , /*  KBD_CIRCLE     */
   ZX81_N               , /*  KBD_CROSS      */
   ZX81_DEL             , /*  KBD_SQUARE     */
   -1                   , /*  KBD_SELECT     */
   -1                   , /*  KBD_START      */
   -1                   , /*  KBD_HOME       */
   -1                   , /*  KBD_HOLD       */
   KBD_LTRIGGER_MAPPING , /*  KBD_LTRIGGER   */
   KBD_RTRIGGER_MAPPING , /*  KBD_RTRIGGER   */
   ZX81_A               , /*  KBD_JOY_UP     */
   ZX81_B               , /*  KBD_JOY_RIGHT  */
   ZX81_C               , /*  KBD_JOY_DOWN   */
   ZX81_D                 /*  KBD_JOY_LEFT   */
 };

 int psp_kbd_mapping[ KBD_ALL_BUTTONS ];
 int psp_kbd_mapping_L[ KBD_ALL_BUTTONS ];
 int psp_kbd_mapping_R[ KBD_ALL_BUTTONS ];
 int psp_kbd_presses[ KBD_ALL_BUTTONS ];
 int kbd_ltrigger_mapping_active;
 int kbd_rtrigger_mapping_active;

 static int danzeff_zx81_key     = 0;
 static int danzeff_zx81_pending = 0;
 static int danzeff_mode        = 0;

#ifdef USE_PSP_IRKEYB
 static int irkeyb_zx81_key      = 0;
 static int irkeyb_zx81_pending  = 0;
# endif

extern unsigned char keyports[9];

int
zx81_key_event(int zx81_idx, int button_pressed)
{
  if (zx81_idx <= ZX81_POUND) {
    if (button_pressed) {
      /* press SHIFT ? */
      if (zx81_kbd[zx81_idx].shift) keyports[0] &=  0xfe;
      else                          keyports[0] |= ~0xfe;
      keyports[zx81_kbd[zx81_idx].port]&= zx81_kbd[zx81_idx].mask;
    } else {
# if 0
      /* disable SHIFT */
      keyports[0] |= ~0xfe;
# endif
      keyports[zx81_kbd[zx81_idx].port]|= ~zx81_kbd[zx81_idx].mask;
    }
  } else
  if ((zx81_idx >= ZX81C_FPS) &&
      (zx81_idx <= ZX81C_SCREEN)) {

    if (button_pressed) {
      SceCtrlData c;
      sceCtrlPeekBufferPositive(&c, 1);
      if ((c.TimeStamp - loc_last_hotkey_time) > KBD_MIN_HOTKEY_TIME) {
        loc_last_hotkey_time = c.TimeStamp;
        zx81_treat_command_key(zx81_idx);
      }
    }
  }
}
int
psp_kbd_reset_mapping(void)
{
  memcpy(psp_kbd_mapping, loc_default_mapping, sizeof(loc_default_mapping));
  memcpy(psp_kbd_mapping_L, loc_default_mapping_L, sizeof(loc_default_mapping_L));
  memcpy(psp_kbd_mapping_R, loc_default_mapping_R, sizeof(loc_default_mapping_R));
  return 0;
}

int
psp_kbd_reset_hotkeys(void)
{
  int index;
  int key_id;
  for (index = 0; index < KBD_ALL_BUTTONS; index++) {
    key_id = loc_default_mapping[index];
    if ((key_id >= ZX81C_FPS) && (key_id <= ZX81C_SCREEN)) {
      psp_kbd_mapping[index] = key_id;
    }
    key_id = loc_default_mapping_L[index];
    if ((key_id >= ZX81C_FPS) && (key_id <= ZX81C_SCREEN)) {
      psp_kbd_mapping_L[index] = key_id;
    }
    key_id = loc_default_mapping_R[index];
    if ((key_id >= ZX81C_FPS) && (key_id <= ZX81C_SCREEN)) {
      psp_kbd_mapping_R[index] = key_id;
    }
  }
  return 0;
}

int
psp_kbd_load_mapping_file(FILE *KbdFile)
{
  char     Buffer[512];
  char    *Scan;
  int      tmp_mapping[KBD_ALL_BUTTONS];
  int      tmp_mapping_L[KBD_ALL_BUTTONS];
  int      tmp_mapping_R[KBD_ALL_BUTTONS];
  int      zx81_key_id = 0;
  int      kbd_id = 0;

  memcpy(tmp_mapping, loc_default_mapping, sizeof(loc_default_mapping));
  memcpy(tmp_mapping_L, loc_default_mapping_L, sizeof(loc_default_mapping_L));
  memcpy(tmp_mapping_R, loc_default_mapping_R, sizeof(loc_default_mapping_R));

  while (fgets(Buffer,512,KbdFile) != (char *)0) {
      
      Scan = strchr(Buffer,'\n');
      if (Scan) *Scan = '\0';
      /* For this #@$% of windows ! */
      Scan = strchr(Buffer,'\r');
      if (Scan) *Scan = '\0';
      if (Buffer[0] == '#') continue;

      Scan = strchr(Buffer,'=');
      if (! Scan) continue;
    
      *Scan = '\0';
      zx81_key_id = atoi(Scan + 1);

      for (kbd_id = 0; kbd_id < KBD_ALL_BUTTONS; kbd_id++) {
        if (!strcasecmp(Buffer,loc_button_name[kbd_id])) {
          tmp_mapping[kbd_id] = zx81_key_id;
          //break;
        }
      }
      for (kbd_id = 0; kbd_id < KBD_ALL_BUTTONS; kbd_id++) {
        if (!strcasecmp(Buffer,loc_button_name_L[kbd_id])) {
          tmp_mapping_L[kbd_id] = zx81_key_id;
          //break;
        }
      }
      for (kbd_id = 0; kbd_id < KBD_ALL_BUTTONS; kbd_id++) {
        if (!strcasecmp(Buffer,loc_button_name_R[kbd_id])) {
          tmp_mapping_R[kbd_id] = zx81_key_id;
          //break;
        }
      }
  }

  memcpy(psp_kbd_mapping, tmp_mapping, sizeof(psp_kbd_mapping));
  memcpy(psp_kbd_mapping_L, tmp_mapping_L, sizeof(psp_kbd_mapping_L));
  memcpy(psp_kbd_mapping_R, tmp_mapping_R, sizeof(psp_kbd_mapping_R));
  
  return 0;
}

int
psp_kbd_load_mapping(char *kbd_filename)
{
  FILE    *KbdFile;
  int      error = 0;
  
  KbdFile = fopen(kbd_filename, "r");
  error   = 1;

  if (KbdFile != (FILE*)0) {
  psp_kbd_load_mapping_file(KbdFile);
  error = 0;
    fclose(KbdFile);
  }

  kbd_ltrigger_mapping_active = 0;
  kbd_rtrigger_mapping_active = 0;
    
  return error;
}

int
psp_kbd_save_mapping(char *kbd_filename)
{
  FILE    *KbdFile;
  int      kbd_id = 0;
  int      error = 0;

  KbdFile = fopen(kbd_filename, "w");
  error   = 1;

  if (KbdFile != (FILE*)0) {

    for (kbd_id = 0; kbd_id < KBD_ALL_BUTTONS; kbd_id++)
    {
      fprintf(KbdFile, "%s=%d\n", loc_button_name[kbd_id], psp_kbd_mapping[kbd_id]);
    }
    for (kbd_id = 0; kbd_id < KBD_ALL_BUTTONS; kbd_id++)
    {
      fprintf(KbdFile, "%s=%d\n", loc_button_name_L[kbd_id], psp_kbd_mapping_L[kbd_id]);
    }
    for (kbd_id = 0; kbd_id < KBD_ALL_BUTTONS; kbd_id++)
    {
      fprintf(KbdFile, "%s=%d\n", loc_button_name_R[kbd_id], psp_kbd_mapping_R[kbd_id]);
    }
    error = 0;
    fclose(KbdFile);
  }

  return error;
}

int 
psp_kbd_is_danzeff_mode()
{
  return danzeff_mode;
}

int
psp_kbd_enter_danzeff()
{
  unsigned int danzeff_key = 0;
  int          zx81_key     = 0;
  int          key_event   = 0;
  SceCtrlData  c;

  if (! danzeff_mode) {
    psp_init_keyboard();
    danzeff_mode = 1;
  }

  sceCtrlPeekBufferPositive(&c, 1);
  c.Buttons &= PSP_ALL_BUTTON_MASK;

  if (danzeff_zx81_pending) 
  {
    if ((c.TimeStamp - loc_last_event_time) > KBD_MIN_PENDING_TIME) {
      loc_last_event_time = c.TimeStamp;
      danzeff_zx81_pending = 0;
      zx81_key_event(danzeff_zx81_key, 0);
    }

    return 0;
  }

  if ((c.TimeStamp - loc_last_event_time) > KBD_MIN_DANZEFF_TIME) {
    loc_last_event_time = c.TimeStamp;
  
    sceCtrlPeekBufferPositive(&c, 1);
    c.Buttons &= PSP_ALL_BUTTON_MASK;
# ifdef USE_PSP_IRKEYB
    psp_irkeyb_set_psp_key(&c);
# endif
    danzeff_key = danzeff_readInput(c);
  }

  if (danzeff_key == DANZEFF_LEFT) {
    danzeff_key = DANZEFF_DEL;
  } else if (danzeff_key == DANZEFF_DOWN) {
    danzeff_key = DANZEFF_ENTER;
  } else if (danzeff_key == DANZEFF_RIGHT) {
  } else if (danzeff_key == DANZEFF_UP) {
  }

  if (danzeff_key > DANZEFF_START) {
    zx81_key = zx81_get_key_from_ascii(danzeff_key);

    if (zx81_key != -1) {
      danzeff_zx81_key     = zx81_key;
      danzeff_zx81_pending = 1;
      zx81_key_event(danzeff_zx81_key, 1);
    }

    return 1;

  } else if (danzeff_key == DANZEFF_START) {
    danzeff_mode          = 0;
    danzeff_zx81_pending = 0;
    danzeff_zx81_key     = 0;

    psp_kbd_wait_no_button();

  } else if (danzeff_key == DANZEFF_SELECT) {
    danzeff_mode        = 0;
    danzeff_zx81_pending = 0;
    danzeff_zx81_key     = 0;
    psp_main_menu();
    psp_init_keyboard();

    psp_kbd_wait_no_button();
  }

  return 0;
}

#ifdef USE_PSP_IRKEYB
int
psp_kbd_enter_irkeyb()
{
  int zx81_key   = 0;
  int psp_irkeyb = PSP_IRKEYB_EMPTY;

  SceCtrlData  c;
  sceCtrlPeekBufferPositive(&c, 1);

  if (irkeyb_zx81_pending) 
  {
    if ((c.TimeStamp - loc_last_irkbd_event_time) > KBD_MIN_IR_PENDING_TIME) {
      loc_last_irkbd_event_time = c.TimeStamp;
      irkeyb_zx81_pending = 0;
      zx81_key_event(irkeyb_zx81_key, 0);
    }
    return 0;
  }

  if ((c.TimeStamp - loc_last_event_time) > KBD_MIN_IR_TIME) {
    loc_last_irkbd_event_time = c.TimeStamp;
    psp_irkeyb = psp_irkeyb_read_key();
  }

  if (psp_irkeyb != PSP_IRKEYB_EMPTY) {

    if (psp_irkeyb == 0x8) {
      zx81_key = ZX81_DEL;
    } else
    if (psp_irkeyb == 0x9) {
      zx81_key = ZX81_GRAPHICS;
    } else
    if (psp_irkeyb == 0xd) {
      zx81_key = ZX81_ENTER;
    } else
    if (psp_irkeyb == 0x1b) {
      zx81_key = ZX81_RUBOUT;
    } else
    if (psp_irkeyb == PSP_IRKEYB_UP) {
      zx81_key = ZX81_UP;
    } else
    if (psp_irkeyb == PSP_IRKEYB_DOWN) {
      zx81_key = ZX81_DOWN;
    } else
    if (psp_irkeyb == PSP_IRKEYB_LEFT) {
      zx81_key = ZX81_LEFT;
    } else
    if (psp_irkeyb == PSP_IRKEYB_RIGHT) {
      zx81_key = ZX81_RIGHT;
    } else {
      zx81_key = zx81_get_key_from_ascii(psp_irkeyb);
    }
    if (zx81_key != -1) {
      irkeyb_zx81_key     = zx81_key;
      irkeyb_zx81_pending = 1;
      zx81_key_event(zx81_key, 1);
    }
    return 1;
  }
  return 0;
}
# endif

void
psp_kbd_display_active_mapping()
{
  if (kbd_ltrigger_mapping_active) {
    psp_sdl_fill_rectangle(0, 0, 10, 3, psp_sdl_rgb(0x0, 0x0, 0xff), 0);
  } else {
    psp_sdl_fill_rectangle(0, 0, 10, 3, 0x0, 0);
  }

  if (kbd_rtrigger_mapping_active) {
    psp_sdl_fill_rectangle(470, 0, 10, 3, psp_sdl_rgb(0x0, 0x0, 0xff), 0);
  } else {
    psp_sdl_fill_rectangle(470, 0, 10, 3, 0x0, 0);
  }
}

int
zx81_decode_key(int psp_b, int button_pressed)
{
  int wake = 0;
  int reverse_analog = ZX81.psp_reverse_analog;

  if (reverse_analog) {
    if ((psp_b >= KBD_JOY_UP  ) &&
        (psp_b <= KBD_JOY_LEFT)) {
      psp_b = psp_b - KBD_JOY_UP + KBD_UP;
    } else
    if ((psp_b >= KBD_UP  ) &&
        (psp_b <= KBD_LEFT)) {
      psp_b = psp_b - KBD_UP + KBD_JOY_UP;
    }
  }

  if (psp_b == KBD_START) {
     if (button_pressed) psp_kbd_enter_danzeff();
  } else
  if (psp_b == KBD_SELECT) {
    if (button_pressed) {
      psp_main_menu();
      psp_init_keyboard();
    }
  } else {
 
    if (psp_kbd_mapping[psp_b] >= 0) {
      wake = 1;
      if (button_pressed) {
        // Determine which buton to press first (ie which mapping is currently active)
        if (kbd_ltrigger_mapping_active) {
          // Use ltrigger mapping
          psp_kbd_presses[psp_b] = psp_kbd_mapping_L[psp_b];
          zx81_key_event(psp_kbd_presses[psp_b], button_pressed);
        } else
        if (kbd_rtrigger_mapping_active) {
          // Use rtrigger mapping
          psp_kbd_presses[psp_b] = psp_kbd_mapping_R[psp_b];
          zx81_key_event(psp_kbd_presses[psp_b], button_pressed);
        } else {
          // Use standard mapping
          psp_kbd_presses[psp_b] = psp_kbd_mapping[psp_b];
          zx81_key_event(psp_kbd_presses[psp_b], button_pressed);
        }
      } else {
          // Determine which button to release (ie what was pressed before)
          zx81_key_event(psp_kbd_presses[psp_b], button_pressed);
      }

    } else {
      if (psp_kbd_mapping[psp_b] == KBD_LTRIGGER_MAPPING) {
        kbd_ltrigger_mapping_active = button_pressed;
        kbd_rtrigger_mapping_active = 0;
      } else
      if (psp_kbd_mapping[psp_b] == KBD_RTRIGGER_MAPPING) {
        kbd_rtrigger_mapping_active = button_pressed;
        kbd_ltrigger_mapping_active = 0;
      }
    }
  }
  return 0;
}

# define ANALOG_THRESHOLD 60

void 
kbd_get_analog_direction(int Analog_x, int Analog_y, int *x, int *y)
{
  int DeltaX = 255;
  int DeltaY = 255;
  int DirX   = 0;
  int DirY   = 0;

  *x = 0;
  *y = 0;

  if (Analog_x <=        ANALOG_THRESHOLD)  { DeltaX = Analog_x; DirX = -1; }
  else 
  if (Analog_x >= (255 - ANALOG_THRESHOLD)) { DeltaX = 255 - Analog_x; DirX = 1; }

  if (Analog_y <=        ANALOG_THRESHOLD)  { DeltaY = Analog_y; DirY = -1; }
  else 
  if (Analog_y >= (255 - ANALOG_THRESHOLD)) { DeltaY = 255 - Analog_y; DirY = 1; }

  *x = DirX;
  *y = DirY;
}

static int 
kbd_reset_button_status(void)
{
  int b = 0;
  /* Reset Button status */
  for (b = 0; b < KBD_MAX_BUTTONS; b++) {
    loc_button_press[b]   = 0;
    loc_button_release[b] = 0;
  }
  psp_init_keyboard();
  return 0;
}

int
kbd_scan_keyboard(void)
{
  SceCtrlData c;
  long        delta_stamp;
  int         event;
  int         b;

  int new_Lx;
  int new_Ly;
  int old_Lx;
  int old_Ly;

  event = 0;
  sceCtrlPeekBufferPositive( &c, 1 );
  c.Buttons &= PSP_ALL_BUTTON_MASK;

# ifdef USE_PSP_IRKEYB
  psp_irkeyb_set_psp_key(&c);
# endif

  if ((c.Buttons & (PSP_CTRL_LTRIGGER|PSP_CTRL_RTRIGGER|PSP_CTRL_START)) ==
      (PSP_CTRL_LTRIGGER|PSP_CTRL_RTRIGGER|PSP_CTRL_START)) {
    /* Exit ! */
    psp_sdl_exit(0);
  }

  delta_stamp = c.TimeStamp - first_time_stamp;
  if ((delta_stamp < 0) || (delta_stamp > KBD_MIN_BATTCHECK_TIME)) {
    first_time_stamp = c.TimeStamp;
    if (psp_is_low_battery()) {
      psp_main_menu();
      psp_init_keyboard();
      return 0;
    }
  }

  /* Check Analog Device */
  kbd_get_analog_direction(loc_button_data.Lx,loc_button_data.Ly,&old_Lx,&old_Ly);
  kbd_get_analog_direction( c.Lx, c.Ly, &new_Lx, &new_Ly);

  /* Analog device has moved */
  if (new_Lx > 0) {
    if (old_Lx  > 0) zx81_decode_key(KBD_JOY_LEFT , 0);
    zx81_decode_key(KBD_JOY_RIGHT, 1);
  } else 
  if (new_Lx < 0) {
    if (old_Lx  < 0) zx81_decode_key(KBD_JOY_RIGHT, 0);
    zx81_decode_key(KBD_JOY_LEFT , 1);
  } else {
    if (old_Lx  > 0) zx81_decode_key(KBD_JOY_LEFT , 0);
    else
    if (old_Lx  < 0) zx81_decode_key(KBD_JOY_RIGHT, 0);
    else {
      zx81_decode_key(KBD_JOY_LEFT  , 0);
      zx81_decode_key(KBD_JOY_RIGHT , 0);
    }
  }

  if (new_Ly < 0) {
    if (old_Ly  > 0) zx81_decode_key(KBD_JOY_DOWN , 0);
    zx81_decode_key(KBD_JOY_UP   , 1);

  } else 
  if (new_Ly > 0) {
    if (old_Ly  < 0) zx81_decode_key(KBD_JOY_UP   , 0);
    zx81_decode_key(KBD_JOY_DOWN , 1);
  } else {
    if (old_Ly  > 0) zx81_decode_key(KBD_JOY_DOWN , 0);
    else
    if (old_Ly  < 0) zx81_decode_key(KBD_JOY_UP   , 0);
    else {
      zx81_decode_key(KBD_JOY_DOWN , 0);
      zx81_decode_key(KBD_JOY_UP   , 0);
    }
  }

  for (b = 0; b < KBD_MAX_BUTTONS; b++) 
  {
    if (c.Buttons & loc_button_mask[b]) {
      if (!(loc_button_data.Buttons & loc_button_mask[b])) {
        loc_button_press[b] = 1;
        event = 1;
      }
    } else {
      if (loc_button_data.Buttons & loc_button_mask[b]) {
        loc_button_release[b] = 1;
        loc_button_press[b] = 0;
        event = 1;
      }
    }
  }
  memcpy(&loc_button_data,&c,sizeof(SceCtrlData));

  return event;
}

void
kbd_wait_start(void)
{
  while (1)
  {
    SceCtrlData c;
    sceCtrlReadBufferPositive(&c, 1);
    c.Buttons &= PSP_ALL_BUTTON_MASK;
    if (c.Buttons & PSP_CTRL_START) break;
  }
  psp_kbd_wait_no_button();
}

void
psp_init_keyboard(void)
{
  zx81_kbd_reset();
  kbd_ltrigger_mapping_active = 0;
  kbd_rtrigger_mapping_active = 0;
}

void
psp_kbd_wait_no_button(void)
{
  SceCtrlData c;

  do {
   sceCtrlPeekBufferPositive(&c, 1);
   c.Buttons &= PSP_ALL_BUTTON_MASK;

  } while (c.Buttons != 0);
} 

void
psp_kbd_wait_button(void)
{
  SceCtrlData c;

  do {
   sceCtrlReadBufferPositive(&c, 1);
  } while (c.Buttons == 0);
} 

int
psp_update_keys(void)
{
  int         b;

  static char first_time = 1;
  static int release_pending = 0;

  if (first_time) {

    memcpy(psp_kbd_mapping, loc_default_mapping, sizeof(loc_default_mapping));
    memcpy(psp_kbd_mapping_L, loc_default_mapping_L, sizeof(loc_default_mapping_L));
    memcpy(psp_kbd_mapping_R, loc_default_mapping_R, sizeof(loc_default_mapping_R));

    zx81_kbd_load();

    SceCtrlData c;
    sceCtrlPeekBufferPositive(&c, 1);
    c.Buttons &= PSP_ALL_BUTTON_MASK;

    if (first_time_stamp == -1) first_time_stamp = c.TimeStamp;
    if ((! c.Buttons) && ((c.TimeStamp - first_time_stamp) < KBD_MIN_START_TIME)) return 0;

    first_time      = 0;
    release_pending = 0;

    for (b = 0; b < KBD_MAX_BUTTONS; b++) {
      loc_button_release[b] = 0;
      loc_button_press[b] = 0;
    }
    sceCtrlPeekBufferPositive(&loc_button_data, 1);
    loc_button_data.Buttons &= PSP_ALL_BUTTON_MASK;

    psp_main_menu();
    psp_init_keyboard();

    return 0;
  }

  if (danzeff_mode) {
    psp_kbd_enter_danzeff();
    return 0;
  }

# ifdef USE_PSP_IRKEYB
  if (psp_kbd_enter_irkeyb()) {
    return 1;
  }
# endif
  if (release_pending)
  {
    release_pending = 0;
    for (b = 0; b < KBD_MAX_BUTTONS; b++) {
      if (loc_button_release[b]) {
        loc_button_release[b] = 0;
        loc_button_press[b] = 0;
        zx81_decode_key(b, 0);
      }
    }
  }

  kbd_scan_keyboard();

  /* check press event */
  for (b = 0; b < KBD_MAX_BUTTONS; b++) {
    if (loc_button_press[b]) {
      loc_button_press[b] = 0;
      release_pending     = 0;
      zx81_decode_key(b, 1);
    }
  }
  /* check release event */
  for (b = 0; b < KBD_MAX_BUTTONS; b++) {
    if (loc_button_release[b]) {
      release_pending = 1;
      break;
    } 
  }

  return 0;
}
