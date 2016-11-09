/* hfs.h: */

typedef struct ext_descr {
  t_ushort	StABN;
  t_ushort	NumABlks;
} t_ext_descr;

typedef t_ext_descr t_extdatarec[3];

typedef struct mdb {
  t_ushort	SigWord;
  t_ulong	CrDate;
  t_ulong	LsMod;
  t_ushort	Atrb;
  t_ushort	NmFls;
  t_ushort	VBMSt;
  t_ushort	AllocPtr;
  t_ushort	NmAlBlks;
  t_ulong	AlBlkSiz;
  t_ulong	ClpSiz;
  t_ushort	AlBlSt;
  t_ulong	NxtCNID;
  t_ushort	FreeBks;
  t_uchar	VolNameLen;
  t_uchar	VolName[27];
  t_ulong	VolBkUp;
  t_ushort	VSeqNum;
  t_ulong	WrCnt;
  t_ulong	XTClpSiz;
  t_ulong	CTClpSiz;
  t_ushort	NmRtDirs;
  t_ulong	FilCnt;
  t_ulong	DirCnt;
  t_ulong	FndrInfo[8];
  t_ushort	VCSize;
  t_ushort	VBMCSize;
  t_ushort	CtlCSize;
  t_ulong	XTFlSize;
  t_extdatarec	XTExtRec;
  t_ulong	CTFlSize;
  t_extdatarec	CTExtRec;
} t_mdb;

typedef struct node_descr {
  t_ulong	FLink;
  t_ulong	BLink;
  t_uchar	Type;
  t_uchar	NHeight;
  t_ushort	NRecs;
  t_ushort	Reserved;	
} t_node_descr;

typedef struct hdr_node {
  t_node_descr	node_descr;
  t_ushort	Depth;
  t_ulong	Root;
  t_ulong	NRecs;
  t_ulong	FNode;
  t_ulong	LNode;
  t_ushort	NodeSize;
  t_ushort	KeyLen;
  t_ulong	NNodes;
  t_ulong	Free;
} t_hdr_node;

typedef struct idx_record {
  char		length;	/* always 0x25 */
  char		reserved;
  t_ulong	parent_id;
  t_uchar	name_length;
  char		name[31];
  t_ulong	pointer;
} t_idx_record;

typedef struct leaf_record {
  char		length;
  char		reserved;
  t_ulong	parent_id;
  t_uchar	name_length;
  char		name[1];
} t_leaf_record;

typedef struct dir_record {
  char		type;
  char		reserved;
  t_ushort	Flags;
  t_ushort	Val;
  t_ulong	DirID;
  t_ulong	CrDat;
  t_ulong	MdDat;
  t_ulong	BkDat;
} t_dir_record;

typedef struct file_record {
  char		type;
  char		reserved;
  char		Flags;
  char		Typ;
  char		FInfo[16];
  t_ulong	FlNum;
  t_ushort	StBlk;
  t_ulong	LgLen;
  t_ulong	PyLen;
  t_ushort	RStBlk;
  t_ulong	RLgLen;
  t_ulong	RPyLen;
  t_ulong	CrDat;
  t_ulong	MdDat;
  t_ulong	BkDat;
  char		FXInfo[16];
  t_ushort	ClpSize;
  t_extdatarec	ExtRec;
  t_extdatarec	RExtRec;
  t_ulong	Resrv;
} t_file_record;

typedef struct dir_thread_record {
  char		type;
  char		reserved;
  t_ulong	Resrv[2];
  t_ulong	ParID;
  t_uchar	CNameLen;
  t_uchar	CName[31];
} t_dir_thread_record;

typedef struct file_thread_record {
  char		type;
  char		reserved;
  t_ulong	Resrv[2];
  t_ulong	ParID;
  t_uchar	CNameLen;
  t_uchar	CName[31];
} t_file_thread_record;

typedef union catalog_record {
  t_dir_record		d;
  t_file_record		f;
  t_dir_thread_record	dt;
  t_file_thread_record	ft;
} t_catalog_record;

int HFS_Find_Master_Directory_Block (CDROM *p_cd, t_mdb *p_mdb);
t_bool Uses_HFS_Protocol (CDROM *p_cd, int *p_skip);
t_bool HFS_Get_Header_Node (CDROM *p_cd, t_ulong p_mdb_pos,
			    t_mdb *p_mdb, t_hdr_node *p_hdr_node);
t_node_descr *HFS_Get_Node (CDROM *p_cd, t_ulong p_mdb_pos, t_mdb *p_mdb,
			    t_ulong p_node);
void Convert_Mac_Characters (char *p_buf, int p_buf_len);

t_bool HFS_Init_Vol_Info (VOLUME *p_volume, int p_start_block);

