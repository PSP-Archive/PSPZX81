/* Emulation of the Z80 CPU with hooks into the other parts of z81.
 * Copyright (C) 1994 Ian Collier.
 * z81 changes (C) 1995-2001 Russell Marks.
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

#include <string.h>	/* for memset/memcpy */
#include <stdio.h>
#include <zlib.h>
#include "common.h"
#include "sound.h"
#include "z80.h"


#define parity(a) (partable[a])

unsigned char partable[256]={
      4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
      0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
      0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
      4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
      0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
      4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
      4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
      0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
      0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
      4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
      4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
      0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
      4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
      0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
      0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
      4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4
   };


unsigned long tstates=0,tsmax=65000,frames=0;

/* odd place to have this, but the display does work in an odd way :-) */
unsigned char scrnbmp_new[ZX_VID_FULLWIDTH*ZX_VID_FULLHEIGHT/8]; /* written */
unsigned char scrnbmp[ZX_VID_FULLWIDTH*ZX_VID_FULLHEIGHT/8];	/* displayed */
						/* checked against for diffs */
static int liney=0;
static int vsy=0;
static unsigned long linestart=0;
static int vsync_toggle=0,vsync_lasttoggle=0;

int ay_reg=0;


#define LINEX 	((tstates-linestart)>>2)


/* for vsync off -> on */
void vsync_raise(void)
{
/* save current pos */
vsy=liney;
}


/* for vsync on -> off */
void vsync_lower(void)
{
int ny=liney,y;

vsync_toggle++;

/* we don't emulate this stuff by default; if nothing else,
 * it can be fscking annoying when you're typing in a program.
 */
if(!vsync_visuals)
  return;

/* even when we do emulate it, we don't bother with x timing,
 * just the y. It gives reasonable results without being too
 * complicated, I think.
 */
if(vsy<0) vsy=0;
if(vsy>=ZX_VID_FULLHEIGHT) vsy=ZX_VID_FULLHEIGHT-1;
if(ny<0) ny=0;
if(ny>=ZX_VID_FULLHEIGHT) ny=ZX_VID_FULLHEIGHT-1;

/* XXX both of these could/should be made into single memset calls */
if(ny<vsy)
  {
  /* must be wrapping around a frame edge; do bottom half */
  for(y=vsy;y<ZX_VID_FULLHEIGHT;y++)
    memset(scrnbmp_new+y*(ZX_VID_FULLWIDTH/8),0xff,ZX_VID_FULLWIDTH/8);
  vsy=0;
  }

for(y=vsy;y<ny;y++)
  memset(scrnbmp_new+y*(ZX_VID_FULLWIDTH/8),0xff,ZX_VID_FULLWIDTH/8);
}

static unsigned char a, f, b, c, d, e, h, l;
static unsigned char r, a1, f1, b1, c1, d1, e1, h1, l1, i, iff1, iff2, im;
static unsigned short pc;
static unsigned short ix, iy, sp;
static unsigned long nextlinetime=0,linegap=208,lastvsyncpend=0;
static unsigned char radjust;
static unsigned char ixoriy, new_ixoriy;
static unsigned char intsample=0;
static unsigned char op;
static int ulacharline=0;
static int nmipend=0,intpend=0,vsyncpend=0,vsynclen=0;
static int hsyncskip=0;
static int framewait=0;

static unsigned char bit1[9]={0xFF,0x80,0xFC,0x7F,0x00,0x80,0x00,0xFE,0xFF};
static unsigned char bit2[4]={0x76,0x06,0x00,0x3e};

  typedef struct zx81_save_t
  {
    unsigned char a;
    unsigned char f;
    unsigned char b;
    unsigned char c;
    unsigned char d;
    unsigned char e;
    unsigned char h;
    unsigned char l;
    unsigned char r;
    unsigned char a1;
    unsigned char f1;
    unsigned char b1;
    unsigned char c1;
    unsigned char d1;
    unsigned char e1;
    unsigned char h1;
    unsigned char l1;
    unsigned char i;
    unsigned char iff1;
    unsigned char iff2;
    unsigned char im;
    unsigned short pc;
    unsigned short ix;
    unsigned short iy;
    unsigned short sp;
    unsigned long nextlinetime;
    unsigned long linegap;
    unsigned long lastvsyncpend;
    unsigned char radjust;
    unsigned char ixoriy; 
    unsigned char new_ixoriy;
    unsigned char intsample;
    unsigned char op;
    int ulacharline;
    int nmipend;
    int intpend;
    int vsyncpend;
    int vsynclen;
    int hsyncskip;
    int framewait;
    unsigned char mem[65536];

  } zx81_save_t;

int
zx81_save_state(char *filename)
{
  zx81_save_t  sd;
  gzFile      *fd;
  int          index;

  sd.a             = a;
  sd.f             = f;
  sd.b             = b;
  sd.c             = c;
  sd.d             = d;
  sd.e             = e;
  sd.h             = h;
  sd.l             = l;
  sd.r             = r;
  sd.a1            = a1;
  sd.f1            = f1;
  sd.b1            = b1;
  sd.c1            = c1;
  sd.d1            = d1;
  sd.e1            = e1;
  sd.h1            = h1;
  sd.l1            = l1;
  sd.i             = i;
  sd.iff1          = iff1;
  sd.iff2          = iff2;
  sd.im            = im;
  sd.pc            = pc;
  sd.ix            = ix;
  sd.iy            = iy;
  sd.sp            = sp;
  sd.nextlinetime  = nextlinetime;
  sd.linegap       = linegap;
  sd.lastvsyncpend = lastvsyncpend;
  sd.radjust       = radjust;
  sd.ixoriy        = ixoriy; 
  sd.new_ixoriy    = new_ixoriy;
  sd.intsample     = intsample;
  sd.op            = op;
  sd.ulacharline   = ulacharline;
  sd.nmipend       = nmipend;
  sd.intpend       = intpend;
  sd.vsyncpend     = vsyncpend;
  sd.vsynclen      = vsynclen;
  sd.hsyncskip     = hsyncskip;
  sd.framewait     = framewait;

  memcpy(sd.mem, mem,65536);

  if ((fd = gzopen(filename, "w")) != NULL) {
    if (! gzwrite(fd, &sd, sizeof(sd))) {
      gzclose(fd);
      return 1;
    }
    gzclose(fd);

  } else {
    return 1;
  }
  return 0;
}

int
zx81_load_state(char *filename)
{
  zx81_save_t  sd;
  FILE        *fd;
  int          index;

  if ((fd = gzopen(filename, "r")) != NULL) {
    if (! gzread(fd,&sd, sizeof(sd))) {
      gzclose(fd);
      return 1;
    }
    gzclose(fd);
    a             = sd.a;
    f             = sd.f;
    b             = sd.b;
    c             = sd.c;
    d             = sd.d;
    e             = sd.e;
    h             = sd.h;
    l             = sd.l;
    r             = sd.r;
    a1            = sd.a1;
    f1            = sd.f1;
    b1            = sd.b1;
    c1            = sd.c1;
    d1            = sd.d1;
    e1            = sd.e1;
    h1            = sd.h1;
    l1            = sd.l1;
    i             = sd.i;
    iff1          = sd.iff1;
    iff2          = sd.iff2;
    im            = sd.im;
    pc            = sd.pc;
    ix            = sd.ix;
    iy            = sd.iy;
    sp            = sd.sp;
    nextlinetime  = sd.nextlinetime;
    linegap       = sd.linegap;
    lastvsyncpend = sd.lastvsyncpend;
    radjust       = sd.radjust;
    ixoriy        = sd.ixoriy; 
    new_ixoriy    = sd.new_ixoriy;
    intsample     = sd.intsample;
    op            = sd.op;
    ulacharline   = sd.ulacharline;
    nmipend       = sd.nmipend;
    intpend       = sd.intpend;
    vsyncpend     = sd.vsyncpend;
    vsynclen      = sd.vsynclen;
    hsyncskip     = sd.hsyncskip;
    framewait     = sd.framewait;
    memcpy(mem,sd.mem,65536);

//LUDO:
    interrupted=0;
# if 0
    framewait=1;
    intsample=0;
    radjust=0;
# endif

  } else {
    return 1;
  }
  return 0;
}

void
zx81_init_register(void)
{
  linegap=208;
  a=f=b=c=d=e=h=l=a1=f1=b1=c1=d1=e1=h1=l1=i=iff1=iff2=im=r=0;
  ixoriy=new_ixoriy=0;
  ix=iy=sp=pc=0;
  tstates=radjust=0;
  nextlinetime=linegap;
  
  /* memory will already be zeroed at this point */
  memcpy(mem+0x4000,bit1,9);
  memcpy(mem+0x7ffc,bit2,4);
  a=0x0B; f=0x85; b=0x00; c=0xFF;
  d=0x43; e=0x99; h=0xC3; l=0x99;
  a1=0xE2; f1=0xA1; b1=0x81; c1=0x02;
  d1=0x00; e1=0x2B; h1=0x00; l1=0x00;
  i=0x1E; iff1=iff2=0;
  im=2;
  r=0xDD; radjust=0xCA;
  ix=0x281; iy=0x4000;
  sp=0x7FFC;
  pc=0x207;
}


void mainloop()
{
  psp_sdl_black_screen();

  linegap=208;
  a=f=b=c=d=e=h=l=a1=f1=b1=c1=d1=e1=h1=l1=i=iff1=iff2=im=r=0;
  ixoriy=new_ixoriy=0;
  ix=iy=sp=pc=0;
  tstates=radjust=0;
  nextlinetime=linegap;
  
  while(1)
  {
  /* this *has* to be checked before radjust is incr'd */
  if(intsample && !(radjust&64))
    intpend=1;
  
  ixoriy=new_ixoriy;
  new_ixoriy=0;
  intsample=1;
  op=fetch(pc&0x7fff);
  if((pc&0x8000) && !(op&64))
    {
    int x,y,v;
    
    /* do the ULA's char-generating stuff */
    x=LINEX;
    y=liney;
/*    printf("ULA %3d,%3d = %02X\n",x,y,op);*/
    if(y>=0 && y<ZX_VID_FULLHEIGHT && x>=0 && x<ZX_VID_FULLWIDTH/8)
      {
      /* XXX I think this is what's needed for the `true hi-res'
       * stuff from the ULA's side, but the timing is messed up
       * at the moment so not worth it currently.
       */
#if 0
      if((tstates-linestart)==3)
        v=mem[((i&0xfe)<<8)|radjust];
      else
#endif
        v=mem[((i&0xfe)<<8)|((op&63)<<3)|ulacharline];
      if(taguladisp) v^=128;
      scrnbmp_new[y*(ZX_VID_FULLWIDTH/8)+x]=((op&128)?~v:v);
      }
    
    op=0;	/* the CPU sees a nop */
    }

# if 0 //LUDO: DEBUG
{
  char buffer[64];
  sprintf(buffer, "%x %x", pc, op);
  psp_sdl_fill_print(10, 100, buffer, psp_sdl_rgb(0x0,0x0,0xff), psp_sdl_rgb(0x0,0x0,0x0));
}
# endif
  pc++;
  radjust++;
  
  switch(op)
    {
#include "z80ops.c"
    }
  
  if(tstates>=tsmax)
    {
    tstates-=tsmax;
    linestart-=tsmax;
    nextlinetime-=tsmax;
    lastvsyncpend-=tsmax;
    vsync_lasttoggle=vsync_toggle;
    vsync_toggle=0;
    
    frames++;
    frame_pause();
    }

  /* the vsync length test is pretty arbitrary, because
   * the timing isn't very accurate (more or less an instruction
   * count) - but it's good enough in practice.
   *
   * the vsync_toggle test is fairly arbitrary too;
   * there has to have been `not too many' for a TV to get
   * confused. In practice, more than one would screw it up,
   * but since we could be looking at over a frames' worth
   * given where vsync_toggle is zeroed, we play it safe.
   * also, we use a copy of the previous chunk's worth,
   * since we need a full frame(-plus) to decide this.
   */
  if(vsynclen && !vsync)
    {
    if(vsynclen>=10)
      {
      if(vsync_lasttoggle<=2)
        {
        vsyncpend=1;	/* start of frame */
        /* FAST mode screws up without this, but it's a bit
         * unpleasant... :-/
         */
        tstates=nextlinetime;
        }
      }
    else
      {
      /* independent timing for this would be awkward, so
       * anywhere on the line is good enough. Also,
       * don't count it as a toggle.
       */
      vsync_toggle--;
      hsyncskip=1;
      }
    }

  /* should do this all the time vsync is set */
  if(vsync)
    {
    ulacharline=0;
    vsynclen++;
    }
  else
    vsynclen=0;
  
  if(tstates>=nextlinetime)	/* new line */
    {
    /* generate fake sync if we haven't had one for a while;
     * but if we just loaded/saved, wait for the first real frame instead
     * to avoid jumpiness.
     */
    if(!vsync && tstates-lastvsyncpend>=tsmax && !framewait)
      vsyncpend=1;

    /* but that won't ever happen if we always have vsync on -
     * i.e., if we're grinding away in FAST mode. So for that
     * case, we check for vsync being held for a full frame.
     */
    if(vsync_visuals && vsynclen>=tsmax)
      {
      vsyncpend=1;
      vsynclen=1;
      
      memset(scrnbmp,0xff,sizeof(scrnbmp));	/* blank the screen */
      goto postcopy;				/* skip the usual copying */
      }

    if(!vsyncpend)
      {
      liney++;
      
      if(hsyncgen && !hsyncskip)
        {
/*        printf("hsync %d\n",tstates);*/
        ulacharline++;
        ulacharline&=7;
        }
      }
    else
      {
      memcpy(scrnbmp,scrnbmp_new,sizeof(scrnbmp));
      postcopy:
      memset(scrnbmp_new,0,sizeof(scrnbmp_new));
      lastvsyncpend=tstates;
      vsyncpend=0;
      framewait=0;
      liney=-1;		/* XXX might be something up here */
      
/*      printf("FRAME START - %s\n",(mem[16443]&128)?"slow":"fast");*/
      }
    
    if(nmigen)
      nmipend=1;

    hsyncskip=0;
    linestart=nextlinetime;
    nextlinetime+=linegap;
    }
  
  if(intsample && nmipend)
    {
    nmipend=0;

    if(nmigen)
      {
/*      printf("NMI line %d tst %d\n",liney,tstates);*/
      iff2=iff1;
      iff1=0;
      /* hardware syncs tstates to falling of NMI pulse (?),
       * so a slight kludge here...
       */
      if(fetch(pc&0x7fff)==0x76)
        {
        pc++;
        tstates=linestart;
        }
      else
        {
        /* this seems curiously long, but equally, seems
         * to be just about right. :-)
         */
        tstates+=27;
        }
      push2(pc);
      pc=0x66;
      }
    }

  if(intsample && intpend)
    {
    intpend=0;

    if(iff1)
      {
/*      printf("int line %d tst %d\n",liney,tstates);*/
      if(fetch(pc&0x7fff)==0x76)pc++;
      iff1=iff2=0;
      tstates+=5; /* accompanied by an input from the data bus */
      switch(im)
        {
        case 0: /* IM 0 */
        case 1: /* undocumented */
        case 2: /* IM 1 */
          /* there is little to distinguish between these cases */
          tstates+=9; /* perhaps */
          push2(pc);
          pc=0x38;
          break;
        case 3: /* IM 2 */
          /* (seems unlikely it'd ever be used on the '81, but...) */
          tstates+=13; /* perhaps */
          {
          int addr=fetch2((i<<8)|0xff);
          push2(pc);
          pc=addr;
          }
        }
      }
    }

  /* this isn't used for any sort of Z80 interrupts,
   * purely for the emulator's UI.
   */
  if(interrupted)
    {
    if(interrupted==1)
      {
      do_interrupt();	/* also zeroes it */
      }
    else	/* must be 2 */
      {
      /* a kludge to let us do a reset */
      interrupted=0;
      a=f=b=c=d=e=h=l=a1=f1=b1=c1=d1=e1=h1=l1=i=iff1=iff2=im=r=0;
      ixoriy=new_ixoriy=0;
      ix=iy=sp=pc=0;
      tstates=radjust=0;
      nextlinetime=linegap;
      vsyncpend=vsynclen=0;
      hsyncskip=0;
      }
    }
  }
}
