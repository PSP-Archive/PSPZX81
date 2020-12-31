/* z81/xz81, Linux console and X ZX81/ZX80 emulators.
 * Copyright (C) 1994 Ian Collier. z81 changes (C) 1995-2004 Russell Marks.
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
 *
 *
 * common.c - various routines/vars common to z81/xz81.
 */

#define Z81_VER		"2.1"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <zlib.h>
#include <dirent.h>
#include "global.h"
#include "common.h"
#include "sound.h"
#include "z80.h"
#include "allmain.h"
#include "psp_fmgr.h"

unsigned char mem[65536],*helpscrn;
unsigned char keyports[9]={0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff, 0xff};

/* this two work on a per-k basis, so we can support 1k etc. properly */
unsigned char *memptr[64];
int memattr[64];

int help=0;
int sound=0;
int sound_vsync=0,sound_ay=0,sound_ay_type=AY_TYPE_NONE;
int load_hook=1,save_hook=1;
int vsync_visuals=0;
int invert_screen=0;

volatile int signal_int_flag=0;
volatile int exit_program_flag=0;
int interrupted=0;
int nmigen=0,hsyncgen=0,vsync=0;
# if 0 //LUDO:
int scrn_freq=2;
# else
int scrn_freq=0;
# endif
int unexpanded=0;
int taguladisp=0;
int fakedispx=0,fakedispy=0;	/* set by main.c/xmain.c */

/* for the printer */
static int zxpframes,zxpcycles,zxpspeed,zxpnewspeed;
static int zxpheight,zxppixel,zxpstylus;
static FILE *zxpfile=NULL;
static char *zxpfilename=NULL;
static unsigned char zxpline[256];



int refresh_screen=1;

/* =1 if emulating ZX80 rather than ZX81 */
int zx80=0;

int ignore_esc=0;

int autoload=0;
char autoload_filename[1024];

/* not too many prototypes needed... :-) */
char *load_selector(void);

void 
autoload_setup(char *filename)
{
  if(zx80 || (filename && strlen(filename)+1>sizeof(autoload_filename))) return;
  autoload=1;
  if(filename) strcpy((char *)autoload_filename,filename);
  else         *autoload_filename=0;
}


static void exit_program_flag_set(int foo)
{
exit_program_flag=1;
}


void loadrom(void)
{
  char FileName[128];
  FILE *fd_in;
  char *fname=FileName;

  getcwd(FileName,128);

  if (zx80) strcat(FileName, "/zx80.rom");
  else      strcat(FileName, "/zx81.rom");

  if((fd_in=fopen(fname,"rb"))!=NULL)
  {
    int siz=(zx80?4096:8192);
  
    fread(mem,1,siz,fd_in);
  
  /* fill first 16k with extra copies of it
   * (not really needed given since improved memptr[] resolution,
   * but what the heck :-))
   */
    memcpy(mem+siz,mem,siz);
    if(zx80) memcpy(mem+siz*2,mem,siz*2);
  
    fclose(fd_in);
  }
  else
  {
    fprintf(stderr,
            "z81: couldn't load ROM file %s.\n"
            "     You *must* install this for z81 to work.\n",
            fname);
    exit(1);
  }
}


void zx81hacks()
{
/* patch save routine */
if(save_hook)
  {
  mem[0x2fc]=0xed; mem[0x2fd]=0xfd;
  mem[0x2fe]=0xc3; mem[0x2ff]=0x07; mem[0x300]=0x02;
  }

/* patch load routine */
if(load_hook)
  {
  mem[0x347]=0xeb;
  mem[0x348]=0xed; mem[0x349]=0xfc;
  mem[0x34a]=0xc3; mem[0x34b]=0x07; mem[0x34c]=0x02;
  }
}


void zx80hacks()
{
/* patch save routine */
if(save_hook)
  {
  mem[0x1b6]=0xed; mem[0x1b7]=0xfd;
  mem[0x1b8]=0xc3; mem[0x1b9]=0x83; mem[0x1ba]=0x02;
  }

/* patch load routine */
if(load_hook)
  {
  mem[0x206]=0xed; mem[0x207]=0xfc;
  mem[0x208]=0xc3; mem[0x209]=0x83; mem[0x20a]=0x02;
  }
}


void initmem()
{
int f;
int ramsize;
int count;

loadrom();
memset(mem+0x4000,0,0xc000);

/* ROM setup */
count=0;
for(f=0;f<16;f++)
  {
  memattr[f]=memattr[32+f]=0;
  memptr[f]=memptr[32+f]=mem+1024*count;
  count++;
  if(count>=(zx80?4:8)) count=0;
  }

/* RAM setup */
ramsize=16;
if(unexpanded)
  ramsize=1;
count=0;
for(f=16;f<32;f++)
  {
  memattr[f]=memattr[32+f]=1;
  memptr[f]=memptr[32+f]=mem+1024*(16+count);
  count++;
  if(count>=ramsize) count=0;
  }

if(zx80)
  zx80hacks();
else
  zx81hacks();
}


void 
loadhelp(void)
{
  FILE *fd_in;
  char buf[128];
  char FileName[128];

  /* but first, set up location of help/selector */
  fakedispx=(ZX_VID_HMARGIN-FUDGE_FACTOR)/8;
  fakedispy=ZX_VID_MARGIN;

  getcwd(FileName,128);
  if (zx80) strcat(FileName, "/zx80kybd.pbm");
  else      strcat(FileName, "/zx81kybd.pbm");

  if((fd_in=fopen(FileName,"rb"))!=NULL)
  {
    /* ditch header lines */
    fgets(buf,sizeof(buf),fd_in);
    fgets(buf,sizeof(buf),fd_in);
    if((helpscrn=calloc(256*192/8,1))==NULL)
    {
      fprintf(stderr,"z81: couldn't allocate memory for help screen.\n");
      exit(1);
    }
    fread(helpscrn,1,256*192/8,fd_in);
    fclose(fd_in);
  }
  else
  {
    fprintf(stderr,"z81: couldn't load help screen.\n");
    exit(1);
  }
}



void 
zxpopen(void)
{
  zxpstylus=zxpspeed=zxpheight=zxpnewspeed=0;
  zxppixel=-1;

  if(!zxpfilename) return;

  if((zxpfile=fopen(zxpfilename,"wb"))==NULL)
  {
    fprintf(stderr,"z81: couldn't open printer file, printing disabled\n");
    return;
  }

  /* we reserve 10 chars for height */
  fprintf(zxpfile,"P4\n256 %10d\n",0);
}


void 
zxpupdateheader(void)
{
  long pos;

  if(!zxpfile || !zxpheight) return;

  pos=ftell(zxpfile);

  /* seek back to write the image height */
  if(fseek(zxpfile,strlen("P4\n256 "),SEEK_SET)!=0)
    fprintf(stderr,"z81: warning: couldn't seek to write image height\n");
  else
  {
    /* I originally had spaces after the image height, but that actually
     * breaks the format as defined in pbm(5) (not to mention breaking
     * when read by zgv :-)). So they're now before the height.
     */
    fprintf(zxpfile,"%10d",zxpheight);
  }

  if(fseek(zxpfile,pos,SEEK_SET)!=0)
  {
    fprintf(stderr,"z81: error: printer file re-seek failed, printout disabled!\n");
    fclose(zxpfile);
    zxpfile=NULL;
  }
}


void 
zxpclose(void)
{
  unsigned long tmp;
  int f;
  
  /* stop the printer */
  tmp=tstates;
  tstates=tsmax;
  out(0,0xfb,4);
  tstates=tmp;
  
  if(!zxpfile)
    return;
  
  /* a blank line if we haven't actually printed anything :-) */
  if(!zxpheight)
  {
    zxpheight++;
    for(f=0;f<32;f++) fputc(0,zxpfile);
  }
  
  /* write header */
  zxpupdateheader();
  
  fclose(zxpfile);
  zxpfile=NULL;
}


void 
zxpout(void)
{
  int i,j,d;

  if(!zxpfile) return;

  zxpheight++;
  for(i=0;i<32;i++)
  {
    for(d=j=0;j<8;j++)
    {
      d=(d<<1)+(zxpline[i*8+j]?1:0);
    }
    fputc(d,zxpfile);
  }
}



/* ZX Printer support, transliterated from IMC's xz80 by a bear of
 * very little brain. :-) Or at least, I don't grok it that well.
 * It works wonderfully though.
 */
int 
printer_inout(int is_out,int val)
{
  /* Note that we go through the motions even if printer support isn't
   * enabled, as the alternative would seem to be to crash... :-)
   */
  if(!is_out)
  {
  /* input */
  
    if(!zxpspeed) return 0x3e;
    else
    {
    int frame=frames-zxpframes;
    int cycles=tstates-zxpcycles;
    int pix=zxppixel;
    int sp=zxpnewspeed;
    int x,ans;
    int cpp=440/zxpspeed;
      
    if(frame>400)
      frame=400;
    cycles+=frame*tsmax;
    x=cycles/cpp-64;        /* x-coordinate reached */
      
    while(x>320)
      {           /* if we are on another line, */
      pix=-1;              /* find out where we are */
      x-=384;
      if(sp)
        {
        x=(x+64)*cpp;
        cpp=440/sp;
        x=x/cpp-64;
        sp=0;
        }
      }
    if((x>-10 && x<0) | zxpstylus)
      ans=0xbe;
    else
      ans=0x3e;
    if(x>pix)
      ans|=1;
    return ans;
    }
  }

  /* output */

  if(!zxpspeed)
  {
  if(!(val&4))
    {
    zxpspeed=(val&2)?1:2;
    zxpframes=frames;
    zxpcycles=tstates;
    zxpstylus=val&128;
    zxppixel=-1;
    }
  }
  else
  {
  int frame=frames-zxpframes;
  int cycles=tstates-zxpcycles;
  int i,x;
  int cpp=440/zxpspeed;
      
  if(frame>400)
    frame=400; /* limit height of blank paper */
  cycles+=frame*tsmax;
  x=cycles/cpp-64;        /* x-coordinate reached */
  for(i=zxppixel;i<x && i<256;i++)
    if(i>=0)		/* should be, but just in case */
      zxpline[i]=zxpstylus;
  if(x>=256 && zxppixel<256)
    zxpout();
      
  while(x>=320)
    {          /* move to next line */
    zxpcycles+=cpp*384;
    if(zxpcycles>=tsmax)
      zxpcycles-=tsmax,zxpframes++;
    x-=384;
    if(zxpnewspeed)
      {
      zxpspeed=zxpnewspeed;
      zxpnewspeed=0;
      x=(x+64)*cpp;
      cpp=440/zxpspeed;
      x=x/cpp-64;
      }
    for(i=0;i<x && i<256;i++)
      zxpline[i]=zxpstylus;
    if(x>=256)
      zxpout();
    }
  if(x<0)
    x=-1;
  if(val&4)
    {
    if(x>=0 && x<256)
      {
      for(i=x;i<256;i++)
        zxpline[i]=zxpstylus;
      zxpout();
      }
    zxpspeed=zxpstylus=0;
    
    /* this is pretty frequent (on a per-char-line basis!),
     * but it's the only time we can really do it automagically.
     */
    zxpupdateheader();
    }
  else
    {
    zxppixel=x;
    zxpstylus=val&128;
    if(x<0)
      zxpspeed=(val&2)?1:2;
    else
      {
      zxpnewspeed=(val&2)?1:2;
      if(zxpnewspeed==zxpspeed)
        zxpnewspeed=0;
      }
    }
  } 

  return(0);
}


unsigned int 
in(int h,int l)
{
  int ts=0;		/* additional cycles*256 */
  int tapezeromask=0x80;	/* = 0x80 if no tape noise (?) */

  if(!(l&4)) l=0xfb;
  if(!(l&1)) l=0xfe;

  switch(l)
  {
  case 0xfb:
    return(printer_inout(0,0));
    
  case 0xfe:
    /* also disables hsync/vsync if nmi off
     * (yes, vsync requires nmi off too, Flight Simulation confirms this)
     */
    if(!nmigen)
      {
      hsyncgen=0;

      /* if vsync was on before, record position */
      if(!vsync)
        vsync_raise();
      vsync=1;
#ifdef OSS_SOUND_SUPPORT
      sound_beeper(vsync);
#endif
      }

    switch(h)
      {
      case 0xfe:	return(ts|(keyports[0]^tapezeromask));
      case 0xfd:	return(ts|(keyports[1]^tapezeromask));
      case 0xfb:	return(ts|(keyports[2]^tapezeromask));
      case 0xf7:	return(ts|(keyports[3]^tapezeromask));
      case 0xef:	return(ts|(keyports[4]^tapezeromask));
      case 0xdf:	return(ts|(keyports[5]^tapezeromask));
      case 0xbf:	return(ts|(keyports[6]^tapezeromask));
      case 0x7f:	return(ts|(keyports[7]^tapezeromask));
      default:
        {
        int i,mask,retval=0xff;
        
        /* some games (e.g. ZX Galaxians) do smart-arse things
         * like zero more than one bit. What we have to do to
         * support this is AND together any for which the corresponding
         * bit is zero.
         */
        for(i=0,mask=1;i<8;i++,mask<<=1)
          if(!(h&mask))
            retval&=keyports[i];
        return(ts|(retval^tapezeromask));
        }
      }
    break;
  }

  return(ts|255);
}


unsigned int out(int h,int l,int a)
{
/* either in from fe or out to ff takes one extra cycle;
 * experimentation strongly suggests not only that out to
 * ff takes one extra, but that *all* outs do.
 */
int ts=1;	/* additional cycles */

if(sound_ay && sound_ay_type==AY_TYPE_ZONX)
  {
  /* the examples in the manual (using DF/0F) and the
   * documented ports (CF/0F) don't match, so decode is
   * important for that.
   */
  if(!(l&0xf0))		/* not sure how many needed, so assume all 4 */
    l=0x0f;
  else
    if(!(l&0x20))		/* bit 5 low is common to DF and CF */
      l=0xdf;
  }

if(!(l&4)) l=0xfb;
if(!(l&2)) l=0xfd;
if(!(l&1)) l=0xfe;

/*printf("out %2X\n",l);*/

switch(l)
  {
# if 0 //LUDO:
  case 0x0f:		/* Zon X data */
    if(sound_ay && sound_ay_type==AY_TYPE_ZONX)
      sound_ay_write(ay_reg,a);
    break;
  case 0xdf:		/* Zon X reg. select */
    if(sound_ay && sound_ay_type==AY_TYPE_ZONX)
      ay_reg=(a&15);
    break;
# endif
  
  case 0xfb:
    return(ts|printer_inout(1,a));
  case 0xfd:
    nmigen=0;
    break;
  case 0xfe:
    if(!zx80)
      {
      nmigen=1;
      break;
      }
    /* falls through, if zx80 */
  case 0xff:	/* XXX should *any* out turn off vsync? */
    /* fill screen gap since last raising of vsync */
    if(vsync)
      vsync_lower();
    vsync=0;
    hsyncgen=1;
#ifdef OSS_SOUND_SUPPORT
    sound_beeper(vsync);
#endif
    break;
  }

return(ts);
}


/* the ZX81 char is used to index into this, to give the ascii.
 * awkward chars are mapped to '_' (underscore), and the ZX81's
 * all-caps is converted to lowercase.
 * The mapping is also valid for the ZX80 for alphanumerics.
 * WARNING: this only covers 0<=char<=63!
 */
static char zx2ascii[64]={
/*  0- 9 */ ' ', '_', '_', '_', '_', '_', '_', '_', '_', '_', 
/* 10-19 */ '_', '\'','#', '$', ':', '?', '(', ')', '>', '<', 
/* 20-29 */ '=', '+', '-', '*', '/', ';', ',', '.', '0', '1', 
/* 30-39 */ '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 
/* 40-49 */ 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 
/* 50-59 */ 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 
/* 60-63 */ 'w', 'x', 'y', 'z'
};


void save_p(int a)
{
static unsigned char fname[256];
unsigned char *ptr=mem+a,*dptr=fname;
FILE *out;

if(zx80)
  strcpy((char *)fname,"zx80prog.p");
else
  {
  /* so the filename is at ptr, in the ZX81 char set, with the last char
   * having bit 7 set. First, get that name in ASCII.
   */
  
  memset(fname,0,sizeof(fname));
  do
    *dptr++=zx2ascii[(*ptr)&63];
  while((*ptr++)<128 && dptr<fname+sizeof(fname)-3);
  
  /* add '.p' */
  strcat((char *)fname,".p");
  }

/* now open the file */
if((out=fopen((char *)fname,"wb"))==NULL)
  return;

/* work out how much to write, and write it.
 * we need to write from 0x4009 (0x4000 on ZX80) to E_LINE inclusive.
 */
if(zx80)
  fwrite(mem+0x4000,1,fetch2(16394)-0x4000+1,out);
else
  fwrite(mem+0x4009,1,fetch2(16404)-0x4009+1,out);

fclose(out);
}


int 
load_p(int a)
{
static unsigned char fname[sizeof(autoload_filename)];
unsigned char *ptr=mem+(a&32767),*dptr=fname;
FILE *in;
int got_ascii_already=0;

if(zx80)
  strcpy((char *)fname,"zx80prog.p");
else
  {
  if(a>=32768) 	/* they did LOAD "" */
    {
    got_ascii_already=1;
    
    if(autoload)	/* autoload stuff did it */
      {
      refresh_screen=1;		/* make sure we redraw screen soon */
      strcpy((char *)fname,autoload_filename);	/* guaranteed to be ok */
      
      /* add .p if needed */
      if(strlen((char *)fname)<sizeof(fname)-3 &&
         (strlen((char *)fname)<=2 || strcasecmp(fname+strlen(fname)-2,".p")!=0))
        strcat(fname,".p");
      }
    }
  
  /* so the filename is at ptr, in the ZX81 char set, with the last char
   * having bit 7 set. First, get that name in ASCII.
   */
  if(!got_ascii_already)
    {
    /* test for Xtender-style LOAD " STOP " to quit */
    if(*ptr==227) exit_program();
    
    memset(fname,0,sizeof(fname));
    do {
      *dptr++=zx2ascii[(*ptr)&63];
    }
    while((*ptr++)<128 && dptr<fname+sizeof(fname)-3);
    
    /* add '.p' */
    strcat(fname,".p");
    }
  }

/* now open the file */
if((in=fopen(fname,"rb"))==NULL)
  {
  /* the partial snap will crash without a file, so reset */
  if(autoload) {
    reset81();
    autoload=0;
  }
  return;
  }

  autoload=0;

/* just read it all */
fread(mem+(zx80?0x4000:0x4009),1,16384,in);

fclose(in);

/* XXX is this still valid given proper video? */
/* don't ask me why, but the ZX80 ROM load routine does this if it
 * works...
 */
if(zx80)
  store(0x400b,fetch(0x400b)+1);

  return 0;
}

//Load Functions

typedef struct {
   char *pchZipFile;
   char *pchExtension;
   char *pchFileNames;
   char *pchSelection;
   int iFiles;
   unsigned int dwOffset;
} t_zip_info;

t_zip_info zip_info;

typedef unsigned int   dword;
typedef unsigned short word;
typedef unsigned char  byte;

#define ERR_FILE_NOT_FOUND       13
#define ERR_FILE_BAD_ZIP         14
#define ERR_FILE_EMPTY_ZIP       15
#define ERR_FILE_UNZIP_FAILED    16

FILE *pfileObject;
char *pbGPBuffer = NULL;

static dword
loc_get_dword(byte *buff)
{
  return ( (((dword)buff[3]) << 24) |
           (((dword)buff[2]) << 16) |
           (((dword)buff[1]) <<  8) |
           (((dword)buff[0]) <<  0) );
}

static void
loc_set_dword(byte *buff, dword value)
{
  buff[3] = (value >> 24) & 0xff;
  buff[2] = (value >> 16) & 0xff;
  buff[1] = (value >>  8) & 0xff;
  buff[0] = (value >>  0) & 0xff;
}

static word
loc_get_word(byte *buff)
{
  return( (((word)buff[1]) <<  8) |
          (((word)buff[0]) <<  0) );
}


int 
zip_dir(t_zip_info *zi)
{
   int n, iFileCount;
   long lFilePosition;
   dword dwCentralDirPosition, dwNextEntry;
   word wCentralDirEntries, wCentralDirSize, wFilenameLength;
   byte *pbPtr;
   char *pchStrPtr;
   dword dwOffset;

   iFileCount = 0;
   if ((pfileObject = fopen(zi->pchZipFile, "rb")) == NULL) {
      return ERR_FILE_NOT_FOUND;
   }

   if (pbGPBuffer == (char *)0) {
     pbGPBuffer = (char *)malloc( sizeof(byte) * 128*1024); 
   }

   wCentralDirEntries = 0;
   wCentralDirSize = 0;
   dwCentralDirPosition = 0;
   lFilePosition = -256;
   do {
      fseek(pfileObject, lFilePosition, SEEK_END);
      if (fread(pbGPBuffer, 256, 1, pfileObject) == 0) {
         fclose(pfileObject);
         return ERR_FILE_BAD_ZIP; // exit if loading of data chunck failed
      }
      pbPtr = (byte*)(pbGPBuffer + (256 - 22)); // pointer to end of central directory (under ideal conditions)
      while (pbPtr != (byte *)pbGPBuffer) {
         if (loc_get_dword(pbPtr) == 0x06054b50) { // check for end of central directory signature
            wCentralDirEntries = loc_get_word(pbPtr + 10);
            wCentralDirSize = loc_get_word(pbPtr + 12);
            dwCentralDirPosition = loc_get_dword(pbPtr + 16);
            break;
         }
         pbPtr--; // move backwards through buffer
      }
      lFilePosition -= 256; // move backwards through ZIP file
   } while (wCentralDirEntries == 0);
   if (wCentralDirSize == 0) {
      fclose(pfileObject);
      return ERR_FILE_BAD_ZIP; // exit if no central directory was found
   }
   fseek(pfileObject, dwCentralDirPosition, SEEK_SET);
   if (fread(pbGPBuffer, wCentralDirSize, 1, pfileObject) == 0) {
      fclose(pfileObject);
      return ERR_FILE_BAD_ZIP; // exit if loading of data chunck failed
   }

   pbPtr = (byte *)pbGPBuffer;
   if (zi->pchFileNames) {
      free(zi->pchFileNames); // dealloc old string table
   }
   zi->pchFileNames = (char *)malloc(wCentralDirSize); // approximate space needed by using the central directory size
   pchStrPtr = zi->pchFileNames;

   for (n = wCentralDirEntries; n; n--) {
      wFilenameLength = loc_get_word(pbPtr + 28);
      dwOffset = loc_get_dword(pbPtr + 42);
      dwNextEntry = wFilenameLength + loc_get_word(pbPtr + 30) + loc_get_word(pbPtr + 32);
      pbPtr += 46;
      char *pchThisExtension = zi->pchExtension;
      while (*pchThisExtension != '\0') { // loop for all extensions to be checked
         if (strncasecmp((char *)pbPtr + (wFilenameLength - 4), pchThisExtension, 4) == 0) {
            strncpy(pchStrPtr, (char *)pbPtr, wFilenameLength); // copy filename from zip directory
            pchStrPtr[wFilenameLength] = 0; // zero terminate string
            pchStrPtr += wFilenameLength+1;
            loc_set_dword((byte*)pchStrPtr, dwOffset);
            pchStrPtr += 4;
            iFileCount++;
            break;
         }
         pchThisExtension += 4; // advance to next extension
      }
      pbPtr += dwNextEntry;
   }
   fclose(pfileObject);

   if (iFileCount == 0) { // no files found?
      return ERR_FILE_EMPTY_ZIP;
   }

   zi->iFiles = iFileCount;
   return 0; // operation completed successfully
}

int 
zip_extract(char *pchZipFile, char *pchFileName, dword dwOffset, char *ext)
{
   int iStatus, iCount;
   dword dwSize;
   byte *pbInputBuffer, *pbOutputBuffer;
   FILE *pfileOut, *pfileIn;
   z_stream z;

   strcpy(pchFileName, ZX81.zx81_home_dir);
   strcat(pchFileName, "/unzip.");
   strcat(pchFileName, ext);

   if (!(pfileOut = fopen(pchFileName, "wb"))) {
      return ERR_FILE_UNZIP_FAILED; // couldn't create output file
   }
   if (pbGPBuffer == (char *)0) {
     pbGPBuffer = (char *)malloc( sizeof(byte) * 128*1024); 
   }
   pfileIn = fopen(pchZipFile, "rb"); // open ZIP file for reading
   fseek(pfileIn, dwOffset, SEEK_SET); // move file pointer to beginning of data block
   fread(pbGPBuffer, 30, 1, pfileIn); // read local header
   dwSize = loc_get_dword((byte *)(pbGPBuffer + 18)); // length of compressed data
   dwOffset += 30 + loc_get_word((byte *)(pbGPBuffer + 26)) + loc_get_word((byte *)(pbGPBuffer + 28));
   fseek(pfileIn, dwOffset, SEEK_SET); // move file pointer to start of compressed data

   pbInputBuffer = (byte *)pbGPBuffer; // space for compressed data chunck
   pbOutputBuffer = pbInputBuffer + 16384; // space for uncompressed data chunck
   z.zalloc = (alloc_func)0;
   z.zfree = (free_func)0;
   z.opaque = (voidpf)0;
   iStatus = inflateInit2(&z, -MAX_WBITS); // init zlib stream (no header)
   do {
      z.next_in = pbInputBuffer;
      if (dwSize > 16384) { // limit input size to max 16K or remaining bytes
         z.avail_in = 16384;
      } else {
         z.avail_in = dwSize;
      }
      z.avail_in = fread(pbInputBuffer, 1, z.avail_in, pfileIn); // load compressed data chunck from ZIP file
      while ((z.avail_in) && (iStatus == Z_OK)) { // loop until all data has been processed
         z.next_out = pbOutputBuffer;
         z.avail_out = 16384;
         iStatus = inflate(&z, Z_NO_FLUSH); // decompress data
         iCount = 16384 - z.avail_out;
         if (iCount) { // save data to file if output buffer is full
            fwrite(pbOutputBuffer, 1, iCount, pfileOut);
         }
      }
      dwSize -= 16384; // advance to next chunck
   } while ((dwSize > 0) && (iStatus == Z_OK)) ; // loop until done
   if (iStatus != Z_STREAM_END) {
      return ERR_FILE_UNZIP_FAILED; // abort on error
   }
   iStatus = inflateEnd(&z); // clean up
   fclose(pfileIn);
   fclose(pfileOut);

   return 0; // data was successfully decompressed
}


static int
loc_load_program(char *filename)
{
  int error = 0;
  autoload_setup(filename);
  zx81_init_register();
  error = load_p(32768);
  return error;
}

int
zx81_load_program(char *FileName, int zip_format)
{
  char *pchPtr;
  char *scan;
  char  SaveName[MAX_PATH+1];
  char  TmpFileName[MAX_PATH + 1];
  dword n;
  int   format;
  int   error;

  error = 1;

  if (zip_format) {

    zip_info.pchZipFile   = FileName;
    zip_info.pchExtension = ".p.81";

    if (!zip_dir(&zip_info)) 
    {
      pchPtr = zip_info.pchFileNames;
      for (n = zip_info.iFiles; n != 0; n--) 
      {
        format = psp_fmgr_getExtId(pchPtr);
        if (format == FMGR_FORMAT_PRG) break;
        pchPtr += strlen(pchPtr) + 5; // skip offset
      }
      if (n) {
        strncpy(SaveName,pchPtr,MAX_PATH);
        scan = strrchr(SaveName,'.');
        if (scan) *scan = '\0';
        zx81_update_save_name(SaveName);
        zip_info.dwOffset = loc_get_dword((byte *)(pchPtr + (strlen(pchPtr)+1)));
        if (!zip_extract(FileName, TmpFileName, zip_info.dwOffset, scan+1)) {
          error = loc_load_program(TmpFileName);
          remove(TmpFileName);
        }
      }
    }

  } else {
    strncpy(SaveName,FileName,MAX_PATH);
    scan = strrchr(SaveName,'.');
    if (scan) *scan = '\0';
    zx81_update_save_name(SaveName);
    error = loc_load_program(FileName);
  }

  if (! error ) {
# if 0 //LUDO: TO_BE_DONE !
    emulator_reset();
    zx81_load_settings();
# endif
    zx81_kbd_load();
  }

  return error;
}

void overlay_help(void)
{
unsigned char *ptr,*optr;
int y;

ptr=helpscrn;
optr=scrnbmp+fakedispy*ZX_VID_FULLWIDTH/8+fakedispx;

for(y=0;y<192;y++)
  {
  memcpy(optr,ptr,32);
  ptr+=32;
  optr+=ZX_VID_FULLWIDTH/8;
  }
}

void
zx81_synchronize(void)
{
	static u32 nextclock = 1;

  if (ZX81.zx81_speed_limiter) {

	  if (nextclock) {
		  u32 curclock;
		  do {
        curclock = SDL_GetTicks();
		  } while (curclock < nextclock);
  
      nextclock = curclock + (u32)( 1000 / ZX81.zx81_speed_limiter);
    }
  }
}

void
psp_zx81_wait_vsync()
{
#ifndef LINUX_MODE
  static int loc_pv = 0;
  int cv = sceDisplayGetVcount();
  if (loc_pv == cv) {
    sceDisplayWaitVblankCB();
  }
  loc_pv = sceDisplayGetVcount();
# endif
}


void
zx81_update_fps()
{
  static u32 next_sec_clock = 0;
  static u32 cur_num_frame = 0;
  cur_num_frame++;
  u32 curclock = SDL_GetTicks();
  if (curclock > next_sec_clock) {
    next_sec_clock = curclock + 1000;
    ZX81.zx81_current_fps = cur_num_frame;
    cur_num_frame = 0;
  }
}

void do_interrupt()
{
static int count=0;

  if(exit_program_flag) {
    exit_program();
  }

/* being careful here not to screw up any pending reset... */
  if(interrupted==1) {
    interrupted=0;
  }

  if (ZX81.psp_skip_cur_frame <= 0) {

    ZX81.psp_skip_cur_frame = ZX81.psp_skip_max_frame;

    update_scrn();

    if (psp_kbd_is_danzeff_mode()) {
      sceDisplayWaitVblankStart();
      danzeff_moveTo(-100, -100);
      danzeff_render();
    }

    if (ZX81.zx81_view_fps) {
      char buffer[32];
      sprintf(buffer, "%3d", (int)ZX81.zx81_current_fps);
      psp_sdl_fill_print(0, 0, buffer, 0xffffff, 0 );
    }

    if (ZX81.psp_display_lr) {
      psp_kbd_display_active_mapping();
    }
    if (ZX81.zx81_vsync) {
      psp_zx81_wait_vsync();
    }
    psp_sdl_flip();

    if (psp_screenshot_mode) {
      psp_screenshot_mode--;
      if (psp_screenshot_mode <= 0) {
        psp_sdl_save_screenshot();
        psp_screenshot_mode = 0;
      }
    }
 
  } else if (ZX81.psp_skip_max_frame) {
    ZX81.psp_skip_cur_frame--;
  }

  if (ZX81.zx81_speed_limiter) {
    zx81_synchronize();
  }

  if (ZX81.zx81_view_fps) {
    zx81_update_fps();
  }
  psp_update_keys();
}


/* despite the name, this also works for the ZX80 :-) */
void reset81()
{
  interrupted=2;	/* will cause a reset */
  memset(mem+16384,0,49152);
  refresh_screen=1;
# if 0 //LUDO:
  if(sound_ay) sound_ay_reset();
# endif
}

void 
frame_pause(void)
{
# if 0 //LUDO:
static int first=1;
static sigset_t mask,oldmask;

#ifdef OSS_SOUND_SUPPORT
if(sound_enabled)
  {
  /* we block on the sound instead. It's a bit unpleasant,
   * but it's the best way.
   */
  sound_frame();
  
  if(interrupted<2)
    interrupted=1;
  return;
  }
#endif

/* we leave it blocked most of the time, only unblocking
 * temporarily with sigsuspend().
 */
if(first)
  {
  first=0;
  sigemptyset(&mask);
  sigaddset(&mask,SIGALRM);
  sigprocmask(SIG_BLOCK,&mask,&oldmask);
  }

/* the procmask stuff is to avoid a race condition */
while(!signal_int_flag)
  sigsuspend(&oldmask);
# else
  //LUDO: sceKernelDelayThread(1000); 
  signal_int_flag=0;
  if(interrupted<2) interrupted=1;
# endif
}

  static char ascii2zx[96]=
  {
     0, 0,11,12,13, 0, 0,11,16,17,23,21,26,22,27,24,
    28,29,30,31,32,33,34,35,36,37,14,25,19,20,18,15,
    23,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,
    53,54,55,56,57,58,59,60,61,62,63,16,24,17,11,22,
    11,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,
    53,54,55,56,57,58,59,60,61,62,63,16,24,17,11, 0
  };


void 
draw_files(unsigned char *scrn,int top,int cursel,int numfiles, char *filearr,int elmtsiz)
{
  int x,y,n,xor,c,len;
  unsigned char *sptr;

/* since this is quick and easy (and incrementally updated for us :-)),
 * we redraw each time no matter what.
 */

  /* clear filenames area */
  for(y=3;y<21;y++) memset(scrn+1+y*33+1,0,30);

  n=top;
  for(y=3;y<21;y++,n++)
  {
    if(n>=numfiles) break;
    xor=128*(n==cursel);
    sptr=scrn+1+y*33+1;
    len=strlen(filearr+elmtsiz*n);
    if(len>30) len=30;
    for(x=0;x<len;x++)
    {
      c=(filearr+elmtsiz*n)[x]-32;
      if(c<0 || c>95) c=0;
      c=ascii2zx[c];
      *sptr++=(c^xor);
    }
  }
}


void 
draw_load_frame(unsigned char *scrn)
{
  int x,y;
  unsigned char *sptr=scrn+1;
  /* these must be exactly 32 chars long */
  /*           01234567890123456789012345678901 */
  char *text1="    choose a program to load    ";
  char *text2=" q=up a=dn enter=load spc=abort ";

  memset(sptr,131,32);
  sptr[33*2]=7;
  memset(sptr+33*2+1,3,30);
  sptr[33*2+31]=132;
  for(y=3;y<21;y++)
    sptr[33*y]=5,sptr[33*y+31]=133;
  sptr[33*21]=130;
  memset(sptr+33*21+1,131,30);
  sptr[33*21+31]=129;
  for(x=0;x<32;x++)
  {
    sptr[33   +x]=128+ascii2zx[text1[x]-32];
    sptr[33*22+x]=128+ascii2zx[text2[x]-32];
  }
  memset(sptr+33*23,3,32);
}

/* convert our text display to a bitmap in scrnbmp */
void selscrn_to_scrnbmp(unsigned char *scrn)
{
  unsigned char *cptr=mem+0x1e00;
  unsigned char *ptr,*optr,*optrsav;
  int x,y,b,c,d,inv;

  memset(scrnbmp,0,ZX_VID_FULLHEIGHT*ZX_VID_FULLWIDTH/8);

  ptr=scrn+1;
  optr=scrnbmp+fakedispy*ZX_VID_FULLWIDTH/8+fakedispx;

  for(y=0;y<24;y++,ptr++)
  {
    optrsav=optr;
    for(x=0;x<32;x++,ptr++,optr++)
    {
      c=*ptr;
      inv=(c&128); c&=63;
    
      for(b=0;b<8;b++,optr+=ZX_VID_FULLWIDTH/8)
      {
        d=cptr[c*8+b];
        if(inv) d^=255;
        *optr=d;
      }
      optr-=ZX_VID_FULLWIDTH;
    }
  
    optr=optrsav+ZX_VID_FULLWIDTH;
  }
}


/* simulate lastk generation from keyports[] */
int make_lastk(void)
{
int y,b;
int lastk0=0,lastk1=0;

for(y=0;y<8;y++)		/* 8 half-rows */
  {
  b=(keyports[y]|0xe0);
  
  /* contribute to it if key was pressed */
  if((b&31)!=31)
    {
    /* set y bit in lastk0 if not shift */
    if(!(y==0 && ((b&31)|1)==31)) lastk0|=(1<<y);
    
    /* now set pos-in-half-row bit(s) in lastk1 */
    b=(((~b)&31)<<1);
    /* move bit 1 of b back down to bit 0 if it's shift bit */
    if(y==0) b=((b&0xfc)|((b&2)>>1));
  
    lastk1|=b;
    }
  }

return(0xffff^(lastk0|(lastk1<<8)));
}


/* wait for them to let go of all keys */
void sel_waitnokeys(void)
{
while(make_lastk()!=0xffff)
  {
  frame_pause();
  do_interrupt();
  }
}
