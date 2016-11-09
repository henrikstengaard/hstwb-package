/* device.h: */

#define CheckIO foo

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <dos/filehandler.h>
#include <devices/timer.h>
#include <devices/input.h>
#include <devices/inputevent.h>
#include <utility/date.h>

#include <clib/dos_protos.h>
#include <clib/alib_protos.h>
#include <clib/exec_protos.h>
#include <clib/utility_protos.h>

#include "generic.h"

#undef CheckIO
struct IORequest *CheckIO(struct IORequest *);

#ifdef LATTICE
#include <pragmas/dos_pragmas.h>
#include <pragmas/exec_pragmas.h>
#include <pragmas/utility_pragmas.h>
extern struct Library *DOSBase, *SysBase, *UtilityBase;
#endif

#ifdef NDEBUG
#define BUG(x) /* nothing */
#else
#define BUG(x) x
#endif

#if !defined(NDEBUG) || defined(DEBUG_SECTORS)
#define BUG2(x) x
#else
#define BUG2(x) /* nothing */
#endif

#define CTOB(x) (void *)(((long)(x))>>2)    /*	BCPL conversion */
#define BTOC(x) (void *)(((long)(x))<<2)

#define bmov(ss,dd,nn) CopyMem(ss,dd,nn)    /*	my habit	*/

#define DOS_FALSE   0
#define DOS_TRUE    -1

typedef unsigned char	ubyte;		    /*	unsigned quantities	    */
typedef unsigned short	uword;
typedef unsigned long	ulong;

typedef struct Interrupt	INTERRUPT;
typedef struct Task		TASK;
typedef struct FileLock 	LOCK;	    /*	See LOCKLINK	*/
typedef struct FileInfoBlock	FIB;
typedef struct DosPacket	PACKET;
typedef struct Process		PROC;
typedef struct DeviceNode	DEVNODE;
typedef struct DeviceList	DEVLIST;
typedef struct DosInfo		DOSINFO;
typedef struct RootNode 	ROOTNODE;
typedef struct FileHandle	FH;
typedef struct MsgPort		PORT;
typedef struct Message		MSG;
typedef struct MinList		LIST;
typedef struct MinNode		NODE;
typedef struct DateStamp	STAMP;
typedef struct InfoData 	INFODATA;
typedef struct DosLibrary	DOSLIB;
typedef struct ExecBase		EXECLIB;
typedef struct Library		LIB;

#define FILE_DIR    1
#define FILE_FILE   -1

void Register_Lock (LOCK *p_lock);
void Unregister_Lock (LOCK *p_lock);
int Reinstall_Locks (void);
void Register_File_Handle (CDROM_OBJ *p_obj);
void Unregister_File_Handle (CDROM_OBJ *p_obj);
DEVLIST *Find_Dev_List (CDROM_OBJ *p_obj);
int Reinstall_File_Handles (void);
void Register_Volume_Node (DEVLIST *p_volume);
void Unregister_Volume_Node (DEVLIST *p_volume);
DEVLIST *Find_Volume_Node (char *p_name);

extern t_bool g_disk_inserted;
