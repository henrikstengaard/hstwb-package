/* hfs.c:
 *
 * Support for the Macintosh HFS filing system.
 *
 * ----------------------------------------------------------------------
 * This code is (C) Copyright 1993,1994 by Frank Munkert.
 * All rights reserved.
 * This software may be freely distributed and redistributed for
 * non-commercial purposes, provided this notice is included.
 * ----------------------------------------------------------------------
 * History:
 * 
 * 03-Nov-94   fmu   - Fixed bug in HFS_Read_From_File().
 *                   - Truncate file names to 30 characters.
 * 15-Apr-93   fmu   - Improved conversion routines.
 *                   - Fixed bug in HFS_Find_Parent().
 *                   - Fixed bug in date conversion routine.
 * 02-Jan-93   fmu   Improved search method for master directory block.
 * 03-Dec-93   fmu   - Fixed bug in HFS_Find_Leaf_Record().
 *                   - Convert ':' and '/' characters.
 *                   - Also convert volume names.
 * 29-Nov-93   fmu   New function HFS_Block_Size().
 * 15-Nov-93   fmu   Corrected some typing mistakes in the HFS->ISO conversion
 *                   table.
 */

#include <stdlib.h>
#include <string.h>

#include <clib/exec_protos.h>
#include <clib/utility_protos.h>

#ifdef LATTICE
#include <pragmas/exec_pragmas.h>
#include <pragmas/utility_pragmas.h>
 
extern struct ExecBase *SysBase;
extern struct Library *UtilityBase;
#endif

#include "cdrom.h"
#include "generic.h"
#include "hfs.h"
#include "params.h"

char g_data_fork_extension[17] = { 0, };
char g_resource_fork_extension[17] = { 0, };
t_bool g_convert_hfs_filenames = FALSE;
t_bool g_convert_hfs_spaces = FALSE;

typedef struct hfs_vol_info {
  t_mdb			mdb;
  int			start_block;
  t_ulong		root_node;
} t_hfs_vol_info;

typedef struct hfs_obj_info {
  t_bool		data_fork;
  t_ulong		parent_id;
  char			name[50];
  t_catalog_record	cat_rec;
} t_hfs_obj_info;

typedef struct leaf_record_pos {
  t_ulong		node;
  t_ushort		record;
  t_node_descr		node_descr;
  t_catalog_record	cat_rec;
  t_leaf_record		leaf_rec;
  char			pad[32]; /* space for name from t_leaf_record */
} t_leaf_record_pos;

#define VOL(vol,tag) (((t_hfs_vol_info *)(vol->vol_info))->tag)
#define OBJ(obj,tag) (((t_hfs_obj_info *)(obj->obj_info))->tag)

/* Number of seconds betweem 01-Jan-1904 and 01-Jan-1978: */

#define TIME_DIFF ((74 * 365 + 19) * 24 * 60 * 60)

static char g_conv_table[128] = {
  '�', '�', '�', '�', '�', '�', '�',
  '�', '�', '�', '�', '�', '�', '�',
  '�', '�', '�', '�', '�', '�', '�',
  '�', '�', '�', '�', '�', '�', '�',
  '�', '�', '�', '�', '�', '�', '�',
  '�', '�', '�', '�', '�', '�', '�',
  '�', '�', '�', '�', '�', '�', '�',
  '�', '�', '�', '�', '�', '�', '�',
  '�', '�', '�', '�', '�', '�', '�',
  '�', '�', '�', '�', '�', '�', '�',
  '�', '�', '�', '�', '�', '�', '�',
  '�', '�', '�', '�', '-', '\"', '\"',
  '`', '�', '�', '�', '�', '�', '/',
  '�', '�', '�', '�', '�', '�', '.',
  '�', '�', '�', '�', '�', '�', '�',
  '�', '�', '�', '�', '�', '�', '�',
  '�', '�', '�', '�', '�', '�', '^',
  '~', '�', '�', '�', '�', '�', '\"',
  '.', '�'
};

void Convert_Mac_Characters (char *p_buf, int p_buf_len)
{
  unsigned char *cp = (unsigned char *) p_buf;
  int i;
  
  for (i=0; i<p_buf_len; i++, cp++)
    if (*cp >= 128)
      *cp = g_conv_table[*cp-128];
    else if (*cp == ':')
      *cp = '.';
    else if (*cp == '/')
      *cp = '-';
    else if (*cp < 32)
      *cp = 0xBF /* � */;
}

void Convert_HFS_Spaces (char *p_buf, int p_buf_len)
{
  unsigned char *cp = (unsigned char *) p_buf;
  int i;
  
  for (i=0; i<p_buf_len; i++, cp++)
    if (*cp == ' ' || *cp == 0xA0)
      *cp = '_';
}

t_uchar *Read_Block (CDROM *p_cdrom, t_ulong p_block)
{
  if (!Read_Sector (p_cdrom, p_block >> 2))
    return NULL;
  
  return p_cdrom->buffer + ((p_block & 3) << 9);
}

t_uchar *Read_Contiguous_Blocks (CDROM *p_cdrom, t_ulong p_block,
				 t_ulong p_last_block)
{
  if (!Read_Contiguous_Sectors (p_cdrom, p_block >> 2, p_last_block >> 2))
    return NULL;

  return p_cdrom->buffer + ((p_block & 3) << 9);
}

/* Convert allocation block number into 512 byte block number.
 */

t_ulong AB_to_Block (VOLUME *p_volume, t_ulong p_alloc_block)
{
  return p_alloc_block * (VOL(p_volume,mdb).AlBlkSiz >> 9) +
  	 VOL(p_volume,mdb).AlBlSt + VOL(p_volume,start_block);
}

int HFS_Find_Master_Directory_Block (CDROM *p_cd, t_mdb *p_mdb)
{
  t_uchar *block;
  typedef struct partition_map {
    t_ushort	pmSig;
    t_ushort	reSigPad;
    t_ulong	pmMapBlkCnt;
    t_ulong	pmPyPartStart;
    t_ulong	pmPartBlkCnt;
    char	pmPartName[32];
    char	pmPartType[32];
  } t_partition_map;
  int i, entries;
  int result;
    
  block = Read_Block (p_cd, 1);
  if (!block || block[0] != 0x50 || block[1] != 0x4D)
    return -1;

  entries = ((t_partition_map *) block)->pmMapBlkCnt;
  for (i=0; i<entries; i++) {
    block = Read_Block (p_cd, i+1);
    if (!block || block[0] != 0x50 || block[1] != 0x4D)
      return -1;
    if (memcmp (((t_partition_map *) block)->pmPartType,
    		"Apple_HFS", 9) == 0) {
      result = ((t_partition_map *) block)->pmPyPartStart + 2;
      block = Read_Block (p_cd, result);
      if (!block || block[0] != 0x42 || block[1] != 0x44)
        return -1;
      else {
        memcpy (p_mdb, block, sizeof (*p_mdb));
        return result;
      }
    }
  }

  return -1;
}

t_bool Uses_HFS_Protocol (CDROM *p_cd, int *p_skip)
{
  t_mdb mdb;
  int blk;

  blk = HFS_Find_Master_Directory_Block (p_cd, &mdb);
  if (blk == -1)
    return FALSE;
  
  *p_skip = blk - 2; /* *p_skip holds the start block of the volume */
  return TRUE;
}

t_bool HFS_Get_Header_Node (CDROM *p_cd, t_ulong p_mdb_pos,
			    t_mdb *p_mdb, t_hdr_node *p_hdr_node)
{
  t_ulong pos = (p_mdb_pos - 2 + p_mdb->AlBlSt +
  		 p_mdb->CTExtRec[0].StABN * (p_mdb->AlBlkSiz >> 9));
  t_hdr_node *hdr;
  
  hdr = (t_hdr_node *) Read_Block (p_cd, pos);
  if (!hdr)
    return FALSE;
  
  memcpy (p_hdr_node, hdr, sizeof (t_hdr_node));
  return TRUE;
}

t_node_descr *HFS_Get_Node (CDROM *p_cd, t_ulong p_mdb_pos, t_mdb *p_mdb,
			    t_ulong p_node)
{
  t_ulong first_node;
  t_ulong pos;
  t_ulong max = 0;
  t_ulong sub = 0;
  int i;

  for (i=0; i<3; i++) {
    max += p_mdb->CTExtRec[i].NumABlks * (p_mdb->AlBlkSiz >> 9);
    if (p_node < max)
      break;
    sub = max;
  }
  
  if (i==3)
    return NULL;

  first_node = (p_mdb_pos - 2 + p_mdb->AlBlSt +
  		p_mdb->CTExtRec[i].StABN * (p_mdb->AlBlkSiz >> 9));

  pos = first_node + (p_node - sub);

  return (t_node_descr *) Read_Block (p_cd, pos);
}

t_node_descr *HFS_Node (VOLUME *p_volume, t_ulong p_node)
{
  return HFS_Get_Node (p_volume->cd, VOL(p_volume,start_block) + 2,
  		       &VOL(p_volume,mdb), p_node);
}

void HFS_Load_Catalog_Record (t_catalog_record *p_cat_rec,
			      char *p_node, int p_record)
{
  short *sp = (short *) p_node;
  int len;
  int start;

  start = sp[255-p_record];
  start += p_node[start] + 1;
  if (start & 1)
    start++;

  len = sp[254-p_record] - start;
  memcpy (p_cat_rec, p_node + start, len);
}


t_bool HFS_Find_Next_Leaf (VOLUME *p_volume, t_leaf_record_pos *p_leaf)
{
  t_node_descr *node;
  short *sp;
  int pos;

  node = HFS_Node (p_volume, p_leaf->node);
  if (!node)
    return FALSE;

  p_leaf->record++;
  if (p_leaf->record == node->NRecs) {
    if (node->FLink) {
      node = HFS_Node (p_volume, p_leaf->node = node->FLink);
      if (!node)
        return FALSE;
      p_leaf->record = 0;
    } else
      return FALSE;
  }

  sp = (short *) node;
  pos = sp[255-p_leaf->record];
  memcpy (&p_leaf->node_descr, node, sizeof (t_node_descr));
  memcpy (&p_leaf->leaf_rec, (char *) node + pos, ((char *) node)[pos] + 1);
  HFS_Load_Catalog_Record (&p_leaf->cat_rec, (char *) node, p_leaf->record);

  if (p_leaf->leaf_rec.name_length > 30) {
    p_leaf->leaf_rec.name_length = 30;
    p_leaf->leaf_rec.name[30] = 0;
  }

  return TRUE;
}

/* Find leaf record p_leaf->record in node p_leaf->node.
 * Store the result in *p_leaf.
 */

t_bool HFS_Find_This_Leaf (VOLUME *p_volume, t_leaf_record_pos *p_leaf)
{
  t_node_descr *node;
  short *sp;
  int pos;

  node = HFS_Node (p_volume, p_leaf->node);
  if (!node)
    return FALSE;

  sp = (short *) node;
  pos = sp[255-p_leaf->record];
  memcpy (&p_leaf->node_descr, node, sizeof (t_node_descr));
  memcpy (&p_leaf->leaf_rec, (char *) node + pos, ((char *) node)[pos] + 1);
  HFS_Load_Catalog_Record (&p_leaf->cat_rec, (char *) node, p_leaf->record);

  return TRUE;
}

/* Find the first leaf record with parent ID p_parent_id.
 * If the leaf node is found, TRUE is returned and *p_cat_rec will be
 * loaded with the catalog record. Otherwise, FALSE is returned.
 */

t_bool HFS_Find_Leaf_Record (VOLUME *p_volume, t_leaf_record_pos *p_leaf,
			     t_ulong p_parent_id)
{
  t_node_descr *node;
  short *sp;
  int i;
  t_ulong this_node = VOL(p_volume,root_node);
  t_ulong next_node;

  node = HFS_Node (p_volume, VOL(p_volume,root_node));
  if (!node) {
    iso_errno = ISOERR_SCSI_ERROR;
    return FALSE;
  }

  for (;;) {
    if (node->Type == 0) { /* index node */
      sp = (short *) node;
      next_node = -1;
      for (i=0; i<node->NRecs; i++) {
        t_idx_record *idx = (t_idx_record *) ((char *) node + sp[255-i]);
	if (idx->length != 0) {
          if (idx->parent_id >= p_parent_id)
	    break;
	  next_node = idx->pointer;
	}
      }
    } else if (node->Type == 0xff) { /* leaf node */
      break;
    } else {
      iso_errno = ISOERR_INTERNAL;
      return FALSE;
    }

    if (next_node == -1) {
      iso_errno = ISOERR_INTERNAL;
      return FALSE;
    }

    node = HFS_Node (p_volume, next_node);
    if (!node) {
      iso_errno = ISOERR_SCSI_ERROR;
      return FALSE;
    }
    this_node = next_node;
  }

  p_leaf->node = this_node;
  p_leaf->record = 0;
  memcpy (&p_leaf->node_descr, node, sizeof (t_node_descr));
  memcpy (&p_leaf->leaf_rec, (char *) node + 0xe, ((char *) node)[0xe] + 1);
  HFS_Load_Catalog_Record (&p_leaf->cat_rec, (char *) node, 0);

  /* walk forwards until the same key is found: */
  for (;;) {
    if (p_leaf->leaf_rec.parent_id == p_parent_id)
      break;
    if (p_leaf->leaf_rec.parent_id > p_parent_id) {
      iso_errno = ISOERR_INTERNAL;
      return FALSE;
    }

    if (!HFS_Find_Next_Leaf (p_volume, p_leaf)) {
      iso_errno = ISOERR_INTERNAL;
      return FALSE;
    }

  }
  
  if (p_leaf->leaf_rec.name_length > 30) {
    p_leaf->leaf_rec.name_length = 30;
    p_leaf->leaf_rec.name[30] = 0;
  }
  
  return TRUE;
}

t_bool HFS_Init_Vol_Info (VOLUME *p_volume, int p_start_block)
{
  extern t_handler g_hfs_handler;
  t_uchar *block;
  t_hdr_node hdr;

  /* Here is a good place to check whether the values of the variables
   * g_data_fork_extension and g_resource_fork_extension are OK.
   */

  if (g_data_fork_extension[0] == 0 &&
      g_resource_fork_extension[0] == 0)
    strcpy (g_resource_fork_extension, ".rsrc");

  /*
   * Volume info initialization:
   */

  p_volume->handler = &g_hfs_handler;
  
  p_volume->vol_info = AllocMem (sizeof (t_hfs_vol_info), MEMF_PUBLIC);
  if (!p_volume->vol_info) {
    iso_errno = ISOERR_NO_MEMORY;
    return FALSE;
  }
  
  VOL(p_volume,start_block) = p_start_block;

  if (!(block = Read_Block (p_volume->cd, p_start_block + 2))) {
    iso_errno = ISOERR_SCSI_ERROR;
    FreeMem (p_volume->vol_info, sizeof (t_hfs_vol_info));
    return FALSE;
  }

  memcpy (&VOL(p_volume,mdb), block, sizeof (t_mdb));

  if (!HFS_Get_Header_Node (p_volume->cd, p_start_block + 2,
  			    &VOL(p_volume,mdb), &hdr)) {
    iso_errno = ISOERR_SCSI_ERROR;
    FreeMem (p_volume->vol_info, sizeof (t_hfs_vol_info));
    return FALSE;
  }

  VOL(p_volume,root_node) = hdr.Root;

  p_volume->mixed_char_filenames = TRUE;

  return TRUE;
}

void HFS_Close_Vol_Info (VOLUME *p_volume)
{
  FreeMem (p_volume->vol_info, sizeof (t_hfs_vol_info));
}

CDROM_OBJ *HFS_Alloc_Obj (void)
{
  CDROM_OBJ *obj = AllocMem (sizeof (CDROM_OBJ), MEMF_PUBLIC | MEMF_CLEAR);
  
  if (!obj) {
    iso_errno = ISOERR_NO_MEMORY;
    return NULL;
  }

  obj->obj_info = AllocMem (sizeof (t_hfs_obj_info), MEMF_PUBLIC | MEMF_CLEAR);
  if (!obj->obj_info) {
    FreeMem (obj, sizeof (CDROM_OBJ));
    return NULL;
  }

  return obj;
}

CDROM_OBJ *HFS_Open_Top_Level_Directory (VOLUME *p_volume)
{
  CDROM_OBJ *obj = HFS_Alloc_Obj ();

  if (!obj)
    return NULL;

  obj->directory_f = TRUE;
  obj->volume = p_volume;

  OBJ(obj,parent_id) = 1;
  strcpy (OBJ(obj,name), ":");
  OBJ(obj,cat_rec).d.DirID = 2;

  return obj;
}

CDROM_OBJ *HFS_Open_Object (VOLUME *p_volume, char *p_name, t_ulong p_parent)
{
  t_leaf_record_pos leaf;
  CDROM_OBJ *obj;
  char name[50];
  char type;
  t_bool data_fork;

  if (!HFS_Find_Leaf_Record (p_volume, &leaf, p_parent))
    return NULL;

  for (;;) {
    type = leaf.cat_rec.d.type;
    if (type == 1 || type == 2) {
      if (leaf.leaf_rec.parent_id != p_parent) {
        iso_errno = ISOERR_NOT_FOUND;
        return NULL;
      }

      memcpy (name, leaf.leaf_rec.name, leaf.leaf_rec.name_length);
      name[leaf.leaf_rec.name_length] = 0;

      if (g_convert_hfs_filenames)
        Convert_Mac_Characters (name, leaf.leaf_rec.name_length);
      if (g_convert_hfs_spaces)
        Convert_HFS_Spaces (name, leaf.leaf_rec.name_length);

      if (type == 1) {
        if (Stricmp ((UBYTE *) p_name, (UBYTE *) name) == 0)
	  break;
      } else {
        strcat (name, g_data_fork_extension);
	if (Stricmp ((UBYTE *) p_name, (UBYTE *) name) == 0) {
	  data_fork = TRUE;
	  break;
	}
        name[leaf.leaf_rec.name_length] = 0;
	strcat (name, g_resource_fork_extension);
	if (Stricmp ((UBYTE *) p_name, (UBYTE *) name) == 0) {
	  data_fork = FALSE;
	  break;
	}
      }
    }
    if (!HFS_Find_Next_Leaf (p_volume, &leaf))
      return NULL;
  }

  obj = HFS_Alloc_Obj ();
  obj->directory_f = (type == 1);
  obj->volume = p_volume;

  OBJ(obj,data_fork) = data_fork;
  OBJ(obj,parent_id) = p_parent;
  strcpy (OBJ(obj,name), name);
  memcpy (&OBJ(obj,cat_rec), &leaf.cat_rec, sizeof (t_catalog_record));

  return obj;
}

/* Open the object with name p_name in the directory p_dir.
 * p_name must not contain '/' or ':' characters.
 */

CDROM_OBJ *HFS_Open_Obj_In_Directory (CDROM_OBJ *p_dir, char *p_name)
{
  return HFS_Open_Object (p_dir->volume, p_name, OBJ(p_dir,cat_rec).d.DirID);
}

CDROM_OBJ *HFS_Find_Parent (CDROM_OBJ *p_obj)
{
  t_leaf_record_pos leaf;

  if (OBJ(p_obj,parent_id) == 2)
    return HFS_Open_Top_Level_Directory (p_obj->volume);

  if (!HFS_Find_Leaf_Record (p_obj->volume, &leaf, OBJ(p_obj,parent_id)))
    return NULL;

  for (;;) {
    if (leaf.leaf_rec.parent_id != OBJ(p_obj,parent_id)) {
      iso_errno = ISOERR_INTERNAL;
      return NULL;
    } 
    if (leaf.cat_rec.d.type == 3) {
      char buf[50];
      memcpy (buf, leaf.cat_rec.dt.CName, leaf.cat_rec.dt.CNameLen);
      buf[leaf.cat_rec.dt.CNameLen] = 0;
      if (g_convert_hfs_filenames)
        Convert_Mac_Characters (buf, leaf.cat_rec.dt.CNameLen);
      if (g_convert_hfs_spaces)
        Convert_HFS_Spaces (buf, leaf.cat_rec.dt.CNameLen);
      return HFS_Open_Object (p_obj->volume, buf, leaf.cat_rec.dt.ParID);
    }
    if (!HFS_Find_Next_Leaf (p_obj->volume, &leaf))
      return NULL;
  }
}

void HFS_Close_Obj (CDROM_OBJ *p_obj)
{
  FreeMem (p_obj->obj_info, sizeof (t_hfs_obj_info));
  FreeMem (p_obj, sizeof (CDROM_OBJ));
}

int HFS_Read_From_File (CDROM_OBJ *p_file, char *p_buffer, int p_buffer_length)
{
  unsigned long block;
  int remain_block, remain_file, remain;
  int len;
  CDROM *cd;
  int pos;
  int buf_pos = 0;
  int todo;
  t_bool data_fork = OBJ(p_file,data_fork);
  t_ulong first_block;
  t_ulong last_block;
  t_ulong data_length = (data_fork ? OBJ(p_file,cat_rec).f.LgLen :
  			 OBJ(p_file,cat_rec).f.RLgLen);

  if (p_file->pos == data_length)
    /* at end of file: */
    return 0;

  cd = p_file->volume->cd;
  first_block = AB_to_Block (p_file->volume,
  			     data_fork ?
  			     OBJ(p_file,cat_rec).f.ExtRec[0].StABN :
			     OBJ(p_file,cat_rec).f.RExtRec[0].StABN);
  block = first_block + (p_file->pos >> 9);
  last_block = first_block + ((data_length-1) >> 9);
  todo = p_buffer_length;

  while (todo) {
    t_uchar *data;
    
    if (!(data = Read_Contiguous_Blocks (cd, block, last_block))) {
      iso_errno = ISOERR_SCSI_ERROR;
      return -1;
    }

    remain_block = 512 - (pos = (p_file->pos & 511));
    remain_file = data_length - p_file->pos;
    remain = (remain_block < remain_file) ? remain_block : remain_file;
    len = (todo < remain) ? todo : remain;
    CopyMem ((APTR) (data + pos), (APTR) (p_buffer + buf_pos), len);
    buf_pos += len;
    p_file->pos += len;
    todo -= len;

    if (p_file->pos >= data_length)
      break;

    block++;
  }

  return buf_pos;
}

t_bool HFS_Cdrom_Info (CDROM_OBJ *p_obj, CDROM_INFO *p_info)
{
  p_info->symlink_f = 0;
  p_info->directory_f = p_obj->directory_f;
  p_info->name_length = strlen (OBJ(p_obj,name));
  memcpy (p_info->name, OBJ(p_obj,name), p_info->name_length);
  if (p_info->directory_f) {
    p_info->file_length = 0;
    p_info->date = OBJ(p_obj,cat_rec).d.CrDat - TIME_DIFF;
  } else {
    if (OBJ(p_obj,data_fork))
      p_info->file_length = OBJ(p_obj,cat_rec).f.LgLen;
    else
      p_info->file_length = OBJ(p_obj,cat_rec).f.RLgLen;
    p_info->date = OBJ(p_obj,cat_rec).f.CrDat - TIME_DIFF;
  }
  
  return TRUE;
}

/*  The 'offset' is a long integer coded like this:
 *
 *  Bit:      |    31 ... 8  | 7   6 | 5   4   3   2   1   0
 *            |              |       |
 *  Contents: |  Node number |  Fork |    Record number
 *
 *  The "Fork" value is encoded as follows:
 *
 *   0 - data fork
 *   1 - resource fork
 *   2 - directory
 *   3 - illegal
 */

t_bool HFS_Examine_Next (CDROM_OBJ *p_obj, CDROM_INFO *p_info,
			 unsigned long *p_offset)
{
  t_leaf_record_pos leaf;
  short fork = 3;

  while (fork == 3) {
    if (*p_offset == 0) {
      if (!HFS_Find_Leaf_Record (p_obj->volume, &leaf, OBJ(p_obj,cat_rec).d.DirID))
        return FALSE;
    } else {
      leaf.node = *p_offset >> 8;
      leaf.record = *p_offset & 63;
      fork = (*p_offset & 0xC0) >> 6;

      if (fork == 0) {
        if (!HFS_Find_This_Leaf (p_obj->volume, &leaf))
	  return FALSE;
      } else {
        if (!HFS_Find_Next_Leaf (p_obj->volume, &leaf))
          return FALSE;
        if (leaf.leaf_rec.parent_id != OBJ(p_obj,cat_rec).d.DirID)
          return FALSE;
      }
    }
    if (leaf.cat_rec.d.type == 1)
      fork = 2;
    else if (leaf.cat_rec.d.type != 2)
      fork = 3;
    else if (fork != 0 && leaf.cat_rec.f.LgLen > 0)
      fork = 0;
    else if (leaf.cat_rec.f.RLgLen > 0)
      fork = 1;
    else
      fork = 3;

    *p_offset = ((leaf.node << 8) + (fork << 6) + leaf.record);

  }

  p_info->symlink_f = 0;
  p_info->directory_f = (leaf.cat_rec.d.type == 1);
  p_info->name_length = leaf.leaf_rec.name_length;
  memcpy (p_info->name, leaf.leaf_rec.name, leaf.leaf_rec.name_length);
  if (g_convert_hfs_filenames)
    Convert_Mac_Characters (p_info->name, p_info->name_length);
  if (g_convert_hfs_spaces)
    Convert_HFS_Spaces (p_info->name, p_info->name_length);
  if (p_info->directory_f) {
    p_info->file_length = 0;
    p_info->date = leaf.cat_rec.d.CrDat - TIME_DIFF;
  } else {
    if (fork == 0) {
      memcpy (p_info->name + p_info->name_length, g_data_fork_extension,
      	      strlen (g_data_fork_extension));
      p_info->name_length += strlen (g_data_fork_extension);
      p_info->file_length = leaf.cat_rec.f.LgLen;
    } else {
      memcpy (p_info->name + p_info->name_length, g_resource_fork_extension,
      	      strlen (g_resource_fork_extension));
      p_info->name_length += strlen (g_resource_fork_extension);
      p_info->file_length = leaf.cat_rec.f.RLgLen;
    }
    p_info->date = leaf.cat_rec.f.CrDat - TIME_DIFF;
  }
  p_info->suppl_info = NULL;

  return TRUE;
}

void *HFS_Clone_Obj_Info (void *p_info)
{
  t_hfs_obj_info *info = (t_hfs_obj_info *) p_info;
  t_hfs_obj_info *new;
  
  new = AllocMem (sizeof (t_hfs_obj_info), MEMF_PUBLIC);
  if (!new)
    return NULL;

  memcpy (new, info, sizeof (t_hfs_obj_info));
  
  return new;
}

t_bool HFS_Is_Top_Level_Obj (CDROM_OBJ *p_obj)
{
  return OBJ(p_obj,parent_id) == 1;
}

t_bool HFS_Same_Objects (CDROM_OBJ *p_obj1, CDROM_OBJ *p_obj2)
{
  if (OBJ(p_obj1,cat_rec).d.type != OBJ(p_obj2,cat_rec).d.type)
    return FALSE;

  if (OBJ(p_obj1,cat_rec).d.type == 1)
    return OBJ(p_obj1,cat_rec).d.DirID == OBJ(p_obj2,cat_rec).d.DirID;
  else
    return OBJ(p_obj1,cat_rec).f.FlNum == OBJ(p_obj2,cat_rec).f.FlNum;
}

t_ulong HFS_Creation_Date (VOLUME *p_volume)
{
  return VOL(p_volume,mdb).CrDate - TIME_DIFF;
}

/* Return volume size in 2048 byte blocks:
 */

t_ulong HFS_Volume_Size (VOLUME *p_volume)
{
  return (VOL(p_volume,mdb).NmAlBlks * (VOL(p_volume,mdb).AlBlkSiz >> 9));
}

t_ulong HFS_Block_Size (VOLUME *p_volume)
{
  return 512;
}

void HFS_Volume_Id (VOLUME *p_volume, char *p_buf, int p_buf_length)
{
  int len = p_buf_length - 1;

  if (len > VOL(p_volume,mdb).VolNameLen)
    len = VOL(p_volume,mdb).VolNameLen;
  
  memcpy (p_buf, VOL(p_volume,mdb).VolName, len);
  p_buf[len] = 0;

  if (g_convert_hfs_filenames)
    Convert_Mac_Characters (p_buf, len);
  if (g_convert_hfs_spaces)
    Convert_HFS_Spaces (p_buf, len);
}

t_ulong HFS_Location (CDROM_OBJ *p_obj)
{
 if (p_obj->directory_f) {
   if (Is_Top_Level_Object (p_obj))
     return FALSE;
   return OBJ(p_obj,cat_rec).d.DirID;
 } else
   return OBJ(p_obj,cat_rec).f.FlNum;
}

t_ulong HFS_File_Length (CDROM_OBJ *p_obj)
{
  return (OBJ(p_obj,data_fork) ?
  	  OBJ(p_obj,cat_rec).f.LgLen :
	  OBJ(p_obj,cat_rec).f.RLgLen);
}

t_handler g_hfs_handler = {
  HFS_Close_Vol_Info,
  HFS_Open_Top_Level_Directory,
  HFS_Open_Obj_In_Directory,
  HFS_Find_Parent,
  HFS_Close_Obj,
  HFS_Read_From_File,
  HFS_Cdrom_Info,
  HFS_Examine_Next,
  HFS_Clone_Obj_Info,
  HFS_Is_Top_Level_Obj,
  HFS_Same_Objects,
  HFS_Creation_Date,
  HFS_Volume_Size,
  HFS_Volume_Id,
  HFS_Location,
  HFS_File_Length,
  HFS_Block_Size
};
