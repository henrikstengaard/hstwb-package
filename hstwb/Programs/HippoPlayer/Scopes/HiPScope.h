
#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef EXEC_PORTS_H
#include <exec/ports.h>
#endif

/*** PT channel data block ***/
struct PTch
{
  LONG   start;		// Start address of sample
  WORD   length;	// Length of sample in words
  LONG   loopstart;	// Start address of loop
  WORD   replen;	// Loop length in words
  WORD   volume;	// Channel volume
  WORD   period;	// Channel period
  WORD   private1;	// Private...
};

struct HippoPort
{
  struct MsgPort ExecMessage;
  
  LONG   private1;	// Private..
  APTR   kplbase;	// kplbase address
  WORD   reserved0;	// Private..
  BYTE   reserved1;	// Private..
  BYTE   opencount;	// Open count
  BYTE   mainvolume;	// Main volume, 0-64
  BYTE   play;		// If non-zero, HiP is playing
  BYTE   playertype;	// 33 = Protracker, 49 = PS3M. 
/*** Protracker ***/
  BYTE   reserved2;
  struct PTch *PTch1;	// Protracker channel data for ch1
  struct PTch *PTch2;	// ch2
  struct PTch *PTch3;	// ch3
  struct PTch *PTch4;	// ch4
/*** PS3M ***/
  APTR   ps3mleft;	// Buffer for the left side
  APTR   ps3mright;	// Buffer for the right side
  LONG   ps3moffs;	// Playing position
  LONG   ps3mmaxoffs;	// Max value for hip_ps3moffs

  BYTE   PTtrigger1;
  BYTE   PTtrigger2;
  BYTE   PTtrigger3;
  BYTE   PTtrigger4;
};

