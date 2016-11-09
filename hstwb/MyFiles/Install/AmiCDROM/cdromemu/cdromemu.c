/* cdromemu.c:
 *
 * Emulates trackdisk.device commands CMD_READ and TD_CHANGESTATE.
 *
 * Based on the example_device program by SAS.
 */

#define  _USEOLDEXEC_ 1
#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/libraries.h>
#include <exec/execbase.h>
#include <exec/io.h>
#include <exec/errors.h>
#include <libraries/dos.h>
#include <devices/trackdisk.h>
#include <dos/dostags.h>
#include <utility/tagitem.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>

#include <string.h>
#include <stdio.h>
#include <dos.h>

#include "cdromemu.h"

#ifdef DEBUG
#define BUG(x) x
#define EMIT(x) output?Write (output, x, strlen (x)):0
#else
#define BUG(x) /* ignore */
#endif

struct MsgPort *myPort;
extern struct ExecBase *SysBase;

struct IntuitionBase* IntuitionBase;

struct START_MSG {
        struct Message msg;
        long devbase;
};

static char* version = "$VER: " VERSION "\n";

void cmd_handler(void)
{
    struct IORequest *ior;
    struct IOExtTD *iotd;
    long input;
    struct Process *proc;   
    struct START_MSG *msg;
    char filename[80];
    static struct EasyStruct easy = {
      sizeof (struct EasyStruct),
      0,
      "cdromemu error",
      NULL,
      "Abort"
    };
    short ok = 1;
    short inserted;
    long res;
    ULONG changenum = 0;
    BUG(long output;)
    BUG(static char buf[100];)
    LONG position;

    proc = (struct Process *)FindTask((char *)NULL);

    /* get the startup message */
    while((msg = (struct START_MSG *)GetMsg(&proc->pr_MsgPort)) == NULL) 
        WaitPort(&proc->pr_MsgPort);
    
    /* builtin compiler functions to set A4 to the global */
    /* data area */
    putreg(REG_A6, msg->devbase); 
    geta4();
    myPort = CreatePort("CDROMEMU",0);
    ReplyMsg((struct Message *)msg);

    if (myPort == NULL) return;

    IntuitionBase = (struct IntuitionBase *) OpenLibrary ("intuition.library", 0);

    if (GetVar ("CDROM_FILE", filename, 79, 0) < 0) {
      easy.es_TextFormat = "Environment variable CDROM_FILE not defined.";
      if (IntuitionBase)
        EasyRequest (NULL, &easy, NULL, NULL);
      ok = 0;
    }

    if (ok) {
      input = Open(filename, MODE_OLDFILE);
      inserted = (input != NULL);
      position = 0;
    }

    BUG(output = Open("CON:0/0/640/100/CDROMEMU", MODE_NEWFILE);)

    while (1) 
    {
        WaitPort(myPort);
        while (ior = (struct IORequest *)GetMsg(myPort)) 
        {
	    short reply = 1;

	    switch(ior->io_Command) 
            {
                case CMD_TERM:
		    BUG(EMIT("CMD_TERM\n");)
		    BUG(if(output)Close(output);)
		    if (ok && inserted)
                      Close(input);
                    Forbid();
                    ReplyMsg(&ior->io_Message);
                    return;
		
		case CMD_XXX_REMOVE:
		    BUG(EMIT("REMOVE\n");)
		    reply = 0;
		    if (ok && inserted) {
		      Close(input);
		      inserted = 0;
		      changenum++;
		    }
		    break;
		
		case CMD_XXX_INSERT:
		    BUG(EMIT("INSERT\n");)
		    reply = 0;
		    if (ok && !inserted) {
		      input = Open(filename, MODE_OLDFILE);
		      if (input) {
		        inserted = 1;
			changenum++;
			position = 0;
		      }
		    }
		    break;


	        case TD_CHANGESTATE: {
          	    struct IOExtTD *iotd = (struct IOExtTD*) ior;
	            iotd->iotd_Req.io_Actual = !inserted;
#ifdef DEBUG
		    EMIT ("TD_CHANGESTATE ==> ");
		    if (inserted)
		    	EMIT ("inserted\n");
		    else
		    	EMIT ("removed\n");
#endif
	  	    break;
         	}

	        case TD_CHANGENUM: {
          	    struct IOExtTD *iotd = (struct IOExtTD*) ior;
	            iotd->iotd_Req.io_Actual = changenum;
#ifdef DEBUG
		    sprintf (buf, "TD_CHANGENUM ==> %lu\n", changenum);
		    EMIT (buf);
#endif
	  	    break;
         	}

                case CMD_READ:
                    iotd = (struct IOExtTD *)ior;
		    if (ok && inserted) {
		      if (position != iotd->iotd_Req.io_Offset) {
		        res = Seek (input, iotd->iotd_Req.io_Offset - position,
		      		    OFFSET_CURRENT);
		        if (res < 0) {
		          iotd->iotd_Req.io_Error = 1;
			  BUG(EMIT("CMD_READ: Seek error\n");)
		          break;
		        }
			position = iotd->iotd_Req.io_Offset;
		      }
                      iotd->iotd_Req.io_Actual = Read(input,
                                             iotd->iotd_Req.io_Data, 
                                             iotd->iotd_Req.io_Length);
		      position += iotd->iotd_Req.io_Actual;
		    } else
		      iotd->iotd_Req.io_Actual = 0;
#ifdef DEBUG
		    sprintf (buf, "CMD_READ offset=%lu length=%lu\n actual=%lu\n",
		    	     iotd->iotd_Req.io_Offset,
		    	     iotd->iotd_Req.io_Length,
		    	     iotd->iotd_Req.io_Actual);
		    EMIT(buf);
#endif
                    break;
            }
	    if (reply)
              ReplyMsg(&ior->io_Message);
        }
    }
}


int  __saveds __asm __UserDevInit(register __d0 long unit,
                                  register __a0 struct IORequest *ior,
                                  register __a6 struct MyLibrary *libbase)
{
      struct Process *myProc;
      struct START_MSG msg;
      
      if (SysBase->LibNode.lib_Version < 36)
          return 1; /* can only run under 2.0 or greater */ 
      
      myProc = CreateNewProcTags(NP_Entry, cmd_handler,
                                 NP_StackSize, 4096,
                                 NP_Name, "CMD_Handler",
                                 TAG_DONE);
      if (myProc == NULL) 
        return 1;

      /* Send the startup message with the library base pointer */
      msg.msg.mn_Length = sizeof(struct START_MSG) - 
                          sizeof (struct Message);
      msg.msg.mn_ReplyPort = CreatePort(0,0);
      msg.msg.mn_Node.ln_Type = NT_MESSAGE;
      msg.devbase = getreg(REG_A6);
      PutMsg(&myProc->pr_MsgPort, (struct Message *)&msg);
      WaitPort(msg.msg.mn_ReplyPort);

      if (myPort == NULL) /* CMD_Handler allocates this */
        return NULL;
      
      DeletePort(msg.msg.mn_ReplyPort);
      
      return 0;
}


void __saveds __asm __UserDevCleanup(register __a0 struct IORequest *ior,
                                     register __a6 struct MyLibrary *libbase)
{
    struct IORequest newior;    
        
    /* send a message to the child process to shut down. */
    newior.io_Message.mn_ReplyPort = CreateMsgPort();
    newior.io_Command = CMD_TERM;
    newior.io_Unit = ior->io_Unit;

    PutMsg(myPort, &newior.io_Message);
    WaitPort(newior.io_Message.mn_ReplyPort);
    DeleteMsgPort(newior.io_Message.mn_ReplyPort);
    DeletePort(myPort);
}


void __saveds __asm DevBeginIO(register __a1 struct IORequest *ior)
{
    ior->io_Error = 0;

    ior->io_Flags &= ~IOF_QUICK;
    switch(ior->io_Command) 
    {
        case CMD_READ:
	case CMD_XXX_REMOVE:
	case CMD_XXX_INSERT:
        case TD_CHANGESTATE:
        case TD_CHANGENUM:
          PutMsg(myPort, &ior->io_Message);
          break;
	
        default:
            ior->io_Error = IOERR_NOCMD;
 	    ReplyMsg(&ior->io_Message);
           break;
    }
}

void __saveds __asm DevAbortIO(register __a1 struct IORequest *ior)
{
}
