#include <proto/exec.h>
#include <exec/ports.h>
#include <exec/memory.h>

#include <proto/dos.h>

#include <proto/intuition.h>
#include <intuition/intuition.h>

#include <proto/graphics.h>
#include <graphics/gfx.h>

#include "HiPScope.h"

struct Window *win;
struct IntuiMessage *imsg;
ULONG class;
UWORD code;

struct HippoPort *hport;
struct BitMap    bmap;
UBYTE  *plane1;
UBYTE  *plane2;
UBYTE  *tmp;

int ForbitCount = 0;
BOOL Going = TRUE;

void ForbidMT(void)
{
  Forbid();
  ForbitCount += 1;
}

void PermitMT(void)
{
  if (ForbitCount > 0)
  {
    Permit();
    ForbitCount -= 1;
  }
}

BOOL DrawScope(struct PTch *ch, UBYTE *bpl, UWORD xpos, UWORD width, UWORD height)
{
  UWORD  xp;
  UWORD  sp;	// sample pos
  WORD   sl;	// bytes left until end of sample ("channel length")
  BYTE  *sa;	// sample adr
  WORD   vol;
  
  /* Calculate volume for current sample data */
  vol = (ch->volume * hport->mainvolume) >> 6;
  
  /* Skip this channel if there is no sample data */
  if (ch->length < 2) return(FALSE);
  
  sa = (BYTE *)ch->start;
  sl = ch->length << 1;
  sp = 0;
  
  for (xp = xpos ; xp < (xpos + width) ; xp++)
  {
    /* The line below plots a point in bpl buffer. This should be replaced
     * by an assembler routine!
     */
    bpl[(((sa[sp] * vol * height) / (64 * 128)) + 32) * 40 + (xp >> 3)] |= 1 << ((xp & 0x0007) ^ 0x7);
    sp++;
    if (0 == sl--)
    {
      /* If zero bytes left then get sample repeat info */
      if (ch->replen < 2) return(FALSE);
      sa = (BYTE *)ch->loopstart;
      sl = ch->replen << 1;
      sp = 0;
    }
  }
  return(TRUE);
}

void ClearPlane(UBYTE *bpl, UWORD Width, UWORD Height)
{
  OwnBlitter();
  WaitBlit();
  *(UBYTE**)0xdff054 = bpl;		// Clear the drawing area
  *(UWORD*)0xdff066 = 0;
  *(ULONG*)0xdff040 = 0x01000000;
  *(UWORD*)0xdff058 = Height*64+Width/16;
  DisownBlitter();
}

int main(void)
{
  BYTE OldPri;
  
  /* Forbid multitasking while finding the port and adding the opencount */
  ForbidMT();
  if (hport = (struct HippoPort *)FindPort("HiP-Port"))
  {
    hport->opencount += 1;
    PermitMT();
    if (win = OpenWindowTags(NULL,
		WA_Left,	100,
		WA_Top,		50,
		WA_Width,	320+10,
		WA_Height,	64+20,
		WA_Title,	"Scope for HiP!",
		WA_ScreenTitle,	"Coded in C by Thomas Sköldenborg (Explorer/Three Little Elks)",
		WA_Flags,	WFLG_CLOSEGADGET |
				WFLG_DRAGBAR |
				WFLG_DEPTHGADGET |
				WFLG_RMBTRAP |
				WFLG_ACTIVATE,
		WA_IDCMP,	IDCMP_CLOSEWINDOW |
				IDCMP_MOUSEBUTTONS |
				IDCMP_RAWKEY,
		TAG_DONE))
    {
      if (plane1 = AllocMem(320/8*64, MEMF_CHIP | MEMF_CLEAR))
      {
      if (plane2 = AllocMem(320/8*64, MEMF_CHIP | MEMF_CLEAR))
      {
       InitBitMap(&bmap, 1, 320, 64);
       
       OldPri = SetTaskPri(FindTask(NULL), -127);
      
        while (Going)
        {
          /* Wait for the top of the next video frame */
          WaitTOF();
          
          tmp = plane1;		// Double (Triple?) Buffer
          plane1 = plane2;
          plane2 = tmp;
          
          ClearPlane(plane2, 320, 64);
          
          if (hport->playertype == 33)		// check if pt module
          {
            /* Draw all four scopes */
            DrawScope (hport->PTch1, plane1, 80*0, 80, 32);
            DrawScope (hport->PTch2, plane1, 80*1, 80, 32);
            DrawScope (hport->PTch3, plane1, 80*2, 80, 32);
            DrawScope (hport->PTch4, plane1, 80*3, 80, 32);
          }
          
          bmap.Planes[0] = plane1;
          BltBitMapRastPort(&bmap, 0, 0,
                            win->RPort, 5, 12, 320, 64,
                            0xc0);
          
          while (imsg = (struct IntuiMessage *)GetMsg(win->UserPort))
          {
            class = imsg->Class;
            code  = imsg->Code;
            ReplyMsg((struct Message *)imsg);
            switch (class)
            {
              case IDCMP_CLOSEWINDOW:
                Going = FALSE;
                break;
              case IDCMP_MOUSEBUTTONS:
                break;
              case IDCMP_RAWKEY:
                switch (code)
                {
                  case 0x45:		// Esc
                    Going = FALSE;
                    break;
                }
                break;
            }
          }
        }
       
       SetTaskPri(FindTask(NULL), OldPri);
      
      FreeMem(plane2, 320/8*64);
      } else Printf("Out of memory!\n");
      FreeMem(plane1, 320/8*64);
      } else Printf("Out of memory!\n");
      CloseWindow(win);
    } else Printf("Couldn't open window!\n");
    hport->opencount -= 1;
  } else Printf("Couldn't find HiP port!\n");
  PermitMT();
 return(0);
}

