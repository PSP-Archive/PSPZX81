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

#include <stdio.h>
#include <pspctrl.h>
#include <zlib.h>
#include "SDL.h"

#include "kbd.h"
#include "psp_danzeff.h"

   zx81_kbd_type zx81_kbd[ZX81_MAX_KEY] = {
  /*  SP      !          "          #          $          %          &          '  */
  {7,0xfe,0},{8,0xff,0},{3,0xfd,1},{8,0xff,0},{5,0xf7,1},{8,0xff,0},{8,0xff,0},{8,0xff,0},
  /*  (       )          *          +          ,          -          .          /  */
  {5,0xfb,1},{5,0xfd,1},{4,0xfb,1},{6,0xfb,1},{7,0xfd,1},{6,0xf7,1},{7,0xfd,0},{0,0xef,1},
  /*  0       1          2          3          4        5        6        7  */
  {4,0xfe,0},{3,0xfe,0},{3,0xfd,0},{3,0xfb,0},{3,0xf7,0},{3,0xef,0},{4,0xef,0},{4,0xf7,0},
  /*  8       9          :          ;          <          =          >          ?  */
  {4,0xfb,0},{4,0xfd,0},{0,0xfd,1},{0,0xfb,1},{7,0xf7,1},{6,0xfb,1},{7,0xfb,1},{0,0xef,1},
  /*  @       A          B          C          D          E          F        G  */
  {8,0xff,0},{1,0xfe,0},{7,0xef,0},{0,0xf7,0},{1,0xfb,0},{2,0xfb,0},{1,0xf7,0},{1,0xef,0},
  /*  H       I          J          K          L        M        N        O  */
  {6,0xef,0},{5,0xfb,0},{6,0xf7,0},{6,0xfb,0},{6,0xfd,0},{7,0xfb,0},{7,0xf7,0},{5,0xfd,0},
  /*  P       Q          R          S          T        U        V        W  */
  {5,0xfe,0},{2,0xfe,0},{2,0xf7,0},{1,0xfd,0},{2,0xef,0},{5,0xf7,0},{0,0xef,0},{2,0xfd,0},
  /*  X       Y          Z          [          \        ]        ^        _  */
  {0,0xfb,0},{5,0xef,0},{0,0xfd,0},{8,0xff,0},{8,0xff,0},{8,0xff,0},{8,0xff,0},{8,0xff,0},
  /*  `       a          b          c          d        e        f        g  */
  {8,0xff,0},{1,0xfe,0},{7,0xef,0},{0,0xf7,0},{1,0xfb,0},{2,0xfb,0},{1,0xf7,0},{1,0xef,0},
  /*  h       i          j          k          l        m        n        o  */
  {6,0xef,0},{5,0xfb,0},{6,0xf7,0},{6,0xfb,0},{6,0xfd,0},{7,0xfb,0},{7,0xf7,0},{5,0xfd,0},
  /*  p       q          r          s          t        u        v          w  */
  {5,0xfe,0},{2,0xfe,0},{2,0xf7,0},{1,0xfd,0},{2,0xef,0},{5,0xf7,0},{0,0xef,0},{2,0xfd,0},
  /*  x       y          z          {          |          }          ~          DEL */
  {0,0xfb,0},{5,0xef,0},{0,0xfd,0},{8,0xff,0},{8,0xff,0},{8,0xff,0},{8,0xff,0},{4,0xfe,1},
  /* ENTER    SHIFT  */
  {6,0xfe,0},{0,0xfe,0},

  /*  EDIT    AND        THEN       TO         LEFT       DOWN      UP */
  {3,0xfe,1},{3,0xfd,1},{3,0xfb,1},{3,0xf7,1},{3,0xef,1},{4,0xef,1},{4,0xf7,1},
  /*  LEFT    GRAPHICS   RUBOUT */
  {4,0xfb,1},{4,0xfd,1},{4,0xfe,1},

  {2,0xfe,1}, /* DBLDBLQUOTE */
  {2,0xfd,1}, /* OR */
  {2,0xfb,1}, /* STEP */
  {2,0xf7,1}, /* LE */
  {2,0xef,1}, /* NE */
  {5,0xef,1}, /* GE */
  {1,0xfe,1}, /* STOP */

  {1,0xfd,1}, /* LPRINT */
  {1,0xfb,1}, /* SLOW */
  {1,0xf7,1}, /* FAST */
  {1,0xef,1}, /* LLIST */
  {6,0xef,1}, /* DBLSTAR */
  {6,0xfe,1}, /* FUNCTION */
  {7,0xfe,1}  /* POUND */
  };

  int kbd_layout[KBD_MAX_ENTRIES][2] = {
    {  ZX81_SPACE,      ' ' },
    {  ZX81_EXCLAMATN,  '?' },
    {  ZX81_DBLQUOTE,   '"' },
    {  ZX81_HASH,       '#' },
    {  ZX81_DOLLAR,     '$' },
    {  ZX81_PERCENT,    '%' },
    {  ZX81_AMPERSAND,  '&' },
    {  ZX81_QUOTE,      '\'' },
    {  ZX81_LEFTPAREN,  '(' },
    {  ZX81_RIGHTPAREN, ')' },
    {  ZX81_STAR,       '*' },
    {  ZX81_PLUS,       '+' },
    {  ZX81_COMMA,      ',' },
    {  ZX81_MINUS,      '-' },
    {  ZX81_PERIOD,     '.' },
    {  ZX81_SLASH,      '/' },
    {  ZX81_0,          '0' },
    {  ZX81_1,          '1' },
    {  ZX81_2,          '2' },
    {  ZX81_3,          '3' },
    {  ZX81_4,          '4' },
    {  ZX81_5,          '5' },
    {  ZX81_6,          '6' },
    {  ZX81_7,          '7' },
    {  ZX81_8,          '8' },
    {  ZX81_9,          '9' },
    {  ZX81_COLON,      ':' },
    {  ZX81_SEMICOLON,  ';' },
    {  ZX81_LESS,       '<' },
    {  ZX81_EQUAL,      '=' },
    {  ZX81_GREATER,    '>' },
    {  ZX81_QUESTION,   '?' },
    {  ZX81_AT,         '@' },
    {  ZX81_A,          'A' },
    {  ZX81_B,          'B' },
    {  ZX81_C,          'C' },
    {  ZX81_D,          'D' },
    {  ZX81_E,          'E' },
    {  ZX81_F,          'F' },
    {  ZX81_G,          'G' },
    {  ZX81_H,          'H' },
    {  ZX81_I,          'I' },
    {  ZX81_J,          'J' },
    {  ZX81_K,          'K' },
    {  ZX81_L,          'L' },
    {  ZX81_M,          'M' },
    {  ZX81_N,          'N' },
    {  ZX81_O,          'O' },
    {  ZX81_P,          'P' },
    {  ZX81_Q,          'Q' },
    {  ZX81_R,          'R' },
    {  ZX81_S,          'S' },
    {  ZX81_T,          'T' },
    {  ZX81_U,          'U' },
    {  ZX81_V,          'V' },
    {  ZX81_W,          'W' },
    {  ZX81_X,          'X' },
    {  ZX81_Y,          'Y' },
    {  ZX81_Z,          'Z' },
    {  ZX81_LBRACKET,   '[' },
    {  ZX81_BACKSLASH,  '\\' },
    {  ZX81_RBRACKET,   ']' },
    {  ZX81_POWER,      '^' },
    {  ZX81_UNDERSCORE, '_' },
    {  ZX81_BACKQUOTE,  '`' },
    {  ZX81_a,          'a' },
    {  ZX81_b,          'b' },
    {  ZX81_c,          'c' },
    {  ZX81_d,          'd' },
    {  ZX81_e,          'e' },
    {  ZX81_f,          'f' },
    {  ZX81_g,          'g' },
    {  ZX81_h,          'h' },
    {  ZX81_i,          'i' },
    {  ZX81_j,          'j' },
    {  ZX81_k,          'k' },
    {  ZX81_l,          'l' },
    {  ZX81_m,          'm' },
    {  ZX81_n,          'n' },
    {  ZX81_o,          'o' },
    {  ZX81_p,          'p' },
    {  ZX81_q,          'q' },
    {  ZX81_r,          'r' },
    {  ZX81_s,          's' },
    {  ZX81_t,          't' },
    {  ZX81_u,          'u' },
    {  ZX81_v,          'v' },
    {  ZX81_w,          'w' },
    {  ZX81_x,          'x' },
    {  ZX81_y,          'y' },
    {  ZX81_z,          'z' },
    {  ZX81_LCBRACE,    '{' },
    {  ZX81_PIPE,       '|' },
    {  ZX81_RCBRACE,    '}' },
    {  ZX81_TILDA,      '~' },
    {  ZX81_DEL,        DANZEFF_DEL         },
    {  ZX81_ENTER,      DANZEFF_ENTER       },
    {  ZX81_SHIFT,      DANZEFF_SHIFT       },
    {  ZX81_EDIT,       DANZEFF_EDIT        },
    {  ZX81_AND,        DANZEFF_AND         },
    {  ZX81_THEN,       DANZEFF_THEN        },
    {  ZX81_TO,         DANZEFF_TO          },
    {  ZX81_LEFT,       DANZEFF_CUR_LEFT    },
    {  ZX81_DOWN,       DANZEFF_CUR_DOWN    },
    {  ZX81_UP,         DANZEFF_CUR_UP      },
    {  ZX81_RIGHT,      DANZEFF_CUR_RIGHT   },
    {  ZX81_GRAPHICS,   DANZEFF_GRAPHICS    },
    {  ZX81_RUBOUT,     DANZEFF_RUBOUT      },
    {  ZX81_DBLDBLQUOTE,DANZEFF_DBLDBLQUOTE },
    {  ZX81_OR,         DANZEFF_OR          },
    {  ZX81_STEP,       DANZEFF_STEP        },
    {  ZX81_LE,         DANZEFF_LE          },
    {  ZX81_NE,         DANZEFF_NE          },
    {  ZX81_GE,         DANZEFF_GE          },
    {  ZX81_STOP,       DANZEFF_STOP        },
    {  ZX81_LPRINT,     DANZEFF_LPRINT      },
    {  ZX81_SLOW,       DANZEFF_SLOW        },
    {  ZX81_FAST,       DANZEFF_FAST        },
    {  ZX81_LLIST,      DANZEFF_LLIST       },
    {  ZX81_DBLSTAR,    DANZEFF_DBLSTAR     },
    {  ZX81_FUNCTION,   DANZEFF_FUNCTION    },
    {  ZX81_POUND,      DANZEFF_POUND       } 

  };

  char keyboard_matrix[16];

  char bit_values[8] = {
     0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80
  };


int
zx81_get_key_from_ascii(char ascii)
{
  int index;
  for (index = 0; index < KBD_MAX_ENTRIES; index++) {
   if (kbd_layout[index][1] == ascii) return kbd_layout[index][0];
  }
  return -1;
}

int 
zx81_kbd_init(void)
{
   return 0;
}

extern unsigned char keyports[9];
int
zx81_kbd_reset(void)
{
  memset(keyports, 0xff, sizeof(keyports));
}

