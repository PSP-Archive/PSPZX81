/* 
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

#ifndef KBD_H  
#define KBD_H  

typedef enum {
   ZX81_SPACE,
   ZX81_EXCLAMATN,
   ZX81_DBLQUOTE,
   ZX81_HASH,
   ZX81_DOLLAR,
   ZX81_PERCENT,
   ZX81_AMPERSAND,
   ZX81_QUOTE,
   ZX81_LEFTPAREN,
   ZX81_RIGHTPAREN,
   ZX81_STAR,
   ZX81_PLUS,
   ZX81_COMMA,
   ZX81_MINUS,
   ZX81_PERIOD,
   ZX81_SLASH,
   ZX81_0,
   ZX81_1,
   ZX81_2,
   ZX81_3,
   ZX81_4,
   ZX81_5,
   ZX81_6,
   ZX81_7,
   ZX81_8,
   ZX81_9,
   ZX81_COLON,
   ZX81_SEMICOLON,
   ZX81_LESS,
   ZX81_EQUAL,
   ZX81_GREATER,
   ZX81_QUESTION,
   ZX81_AT,
   ZX81_A,
   ZX81_B,
   ZX81_C,
   ZX81_D,
   ZX81_E,
   ZX81_F,
   ZX81_G,
   ZX81_H,
   ZX81_I,
   ZX81_J,
   ZX81_K,
   ZX81_L,
   ZX81_M,
   ZX81_N,
   ZX81_O,
   ZX81_P,
   ZX81_Q,
   ZX81_R,
   ZX81_S,
   ZX81_T,
   ZX81_U,
   ZX81_V,
   ZX81_W,
   ZX81_X,
   ZX81_Y,
   ZX81_Z,
   ZX81_LBRACKET,
   ZX81_BACKSLASH,
   ZX81_RBRACKET,
   ZX81_POWER,
   ZX81_UNDERSCORE,
   ZX81_BACKQUOTE,
   ZX81_a,
   ZX81_b,
   ZX81_c,
   ZX81_d,
   ZX81_e,
   ZX81_f,
   ZX81_g,
   ZX81_h,
   ZX81_i,
   ZX81_j,
   ZX81_k,
   ZX81_l,
   ZX81_m,
   ZX81_n,
   ZX81_o,
   ZX81_p,
   ZX81_q,
   ZX81_r,
   ZX81_s,
   ZX81_t,
   ZX81_u,
   ZX81_v,
   ZX81_w,
   ZX81_x,
   ZX81_y,
   ZX81_z,
   ZX81_LCBRACE,
   ZX81_PIPE,
   ZX81_RCBRACE,
   ZX81_TILDA,
   ZX81_DEL,
   ZX81_ENTER,
   ZX81_SHIFT,
   ZX81_EDIT,
   ZX81_AND,
   ZX81_THEN,
   ZX81_TO,
   ZX81_LEFT,
   ZX81_DOWN,
   ZX81_UP,
   ZX81_RIGHT,
   ZX81_GRAPHICS,
   ZX81_RUBOUT,
   ZX81_DBLDBLQUOTE,
   ZX81_OR,
   ZX81_STEP,
   ZX81_LE,
   ZX81_NE,
   ZX81_GE,
   ZX81_STOP,
   ZX81_LPRINT,
   ZX81_SLOW,
   ZX81_FAST,
   ZX81_LLIST,
   ZX81_DBLSTAR,
   ZX81_FUNCTION,
   ZX81_POUND,

   ZX81C_FPS,
   ZX81C_JOY,
   ZX81C_RENDER,
   ZX81C_LOAD,
   ZX81C_SAVE,
   ZX81C_RESET,
   ZX81C_SCREEN,

   ZX81_MAX_KEY

 } ZX81_KEYS;


#define MOD_ZX81_SHIFT   (0x01 << 8)
#define MOD_ZX81_CTRL    (0x02 << 8)
#define MOD_EMU_KEY     (0x10 << 8)

  typedef struct zx81_kbd_type {
    unsigned char port;
    unsigned char mask;
    unsigned char shift;
  } zx81_kbd_type;

  extern zx81_kbd_type zx81_kbd[ZX81_MAX_KEY];

#define MOD_PC_SHIFT    (KMOD_SHIFT << 16)
#define MOD_PC_CTRL     (KMOD_CTRL  << 16)
#define MOD_PC_MODE     (KMOD_MODE  << 16)

#define KBD_MAX_ENTRIES ZX81_MAX_KEY

  extern int  kbd_layout[KBD_MAX_ENTRIES][2];
  extern char keyboard_matrix[16];
  extern char bit_values[8];

  extern int zx81_kbd_init(void);
  extern int zx81_kbd_reset(void);
  extern int zx81_key_event(int zx81_key, int button_pressed);
  extern void zx81_kbd_swap_joy (void);

  extern int zx81_get_key_from_ascii(char ascii);

#endif
