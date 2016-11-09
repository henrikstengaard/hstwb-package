/* rock.c:
 *
 * Support for the Rock Ridge filing system.
 *
 * ----------------------------------------------------------------------
 * This code is (C) Copyright 1993,1994 by Frank Munkert.
 * All rights reserved.
 * This software may be freely distributed and redistributed for
 * non-commercial purposes, provided this notice is included.
 * ----------------------------------------------------------------------
 * History:
 * 
 * 05-Feb-94   fmu   Added support for relocated directories.
 * 16-Oct-93   fmu   Adapted to new VOLUME structure.
 */

#include <stdlib.h>
#include <string.h>

#include <exec/memory.h>

#include <clib/exec_protos.h>

#ifdef LATTICE
#include <pragmas/exec_pragmas.h>
#endif

#include "rock.h"

#define VOL(vol,tag) (((t_iso_vol_info *)(vol->vol_info))->tag)
#define OBJ(obj,tag) (((t_iso_obj_info *)(obj->obj_info))->tag)

/* Check whether the given volume uses the Rock Ridge Interchange Protocol.
 * The protocol is identified by the sequence
 *            'S' 'P' 7 1 0xbe 0xef
 * in the system use field of the (00) directory in the root directory of
 * the volume.
 *
 * Returns 1 iff the RR protocol is used; 0 otherwise.
 * If the RR protocol is used, *p_skip will be set to the skip length
 * specified in the SP system use field.
 */

t_bool Uses_Rock_Ridge_Protocol (VOLUME *p_volume, int *p_skip)
{
  unsigned long loc = VOL(p_volume,pvd).root.extent_loc_m;
  directory_record *dir;
  int system_use_pos;
  unsigned char *sys;

  if (!(dir = Get_Directory_Record (p_volume, loc, 0)))
    return 0;
  
  system_use_pos = 33 + dir->file_id_length;
  if (system_use_pos & 1)
    system_use_pos++;

  if (system_use_pos >= dir->length)
    return 0;

  sys = (unsigned char *) dir + system_use_pos;
  if (sys[0] == 'S' && sys[1] == 'P' && sys[2] == 7 &&
      sys[3] == 1 && sys[4] == 0xbe && sys[5] == 0xef) {
    *p_skip = sys[6];
    return 1;
  } else
    return 0;
}

/* Searches for the system use field with name p_name in the directory record
 * p_dir and fills the buffer p_buf (with length p_buf_len) with the information
 * contained in the system use field.
 *
 * p_index is the ordinal number of the system use field (if more than one
 * system use field with the same name is recorded.) 0=first occurrence,
 * 1=second occurrence, and so on.
 *
 * 1 is returned if the system use field has been found; otherwise 0
 * is returned.
 */

int Get_System_Use_Field (VOLUME *p_volume, directory_record *p_dir,
			  char *p_name, char *p_buf, int p_buf_len,
			  int p_index)
{
  int system_use_pos;
  int slen, len;
  unsigned long length = p_dir->length;
  unsigned char *buf = (unsigned char *) p_dir;

  system_use_pos = 33 + p_dir->file_id_length;
  if (system_use_pos & 1)
    system_use_pos++;
  system_use_pos += VOL(p_volume,skip);

  /* the system use field must be at least 4 bytes long */
  while (system_use_pos + 3 < length) {
    slen = buf[system_use_pos+2];
    if (buf[system_use_pos] == p_name[0] &&
        buf[system_use_pos+1] == p_name[1]) {
      if (p_index)
        p_index--;
      else {
        len = (slen < p_buf_len) ? slen : p_buf_len;
        memcpy (p_buf, buf + system_use_pos, len);
        return 1;
      }
    }
    /* look for continuation area: */
    if (buf[system_use_pos] == 'C' &&
        buf[system_use_pos+1] == 'E') {
      unsigned long newloc, offset;
      memcpy (&newloc, buf + system_use_pos + 8, 4);
      memcpy (&offset, buf + system_use_pos + 16, 4);
      memcpy (&length, buf + system_use_pos + 24, 4);
      if (!Read_Sector (p_volume->cd, newloc))
        return 0;
      buf = p_volume->cd->buffer;
      system_use_pos = offset;
      continue;
    }
    
    /* look for system use field terminator: */
    if (buf[system_use_pos] == 'S' &&
        buf[system_use_pos+1] == 'T')
      return 0;

    system_use_pos += slen;
  }
  return 0;
}

/* Determines the Rock Ridge file name of the CDROM object p_obj.
 * The file name will be stored in the buffer p_buf (with length p_buf_len).
 * The file name will NOT be null-terminated. The number of characters in
 * the file name is returned. If there is no Rock Ridge file name for
 * p_obj, then -1 is returned.
 */

int Get_RR_File_Name (VOLUME *p_volume, directory_record *p_dir,
		      char *p_buf, int p_buf_len)
{
  struct nm_system_use_field {
    char	  id[2];
    unsigned char length;
    unsigned char version;
    unsigned char flags;
    char          name[210];
  } nm;
  int len, slen;
  int index = 0;
  int total = 0;

  for (;;) {
    if (!Get_System_Use_Field (p_volume, p_dir, "NM",
  			       (char *) &nm, sizeof (nm), index))
      return -1;

    slen = nm.length-5;
    len = (p_buf_len < slen) ? p_buf_len : slen;
    if (len)
      memcpy (p_buf, nm.name, len);

    total += len;
    if (!(nm.flags & 1))
      return total;

    p_buf += len;
    p_buf_len -= len;
    index++;
  }
}

/* Returns 1 if the PX system use field indicates a symbolic link.
 */

int Is_A_Symbolic_Link (VOLUME *p_volume, directory_record *p_dir)
{
  struct px_system_use_field {
    char	  id[2];
    unsigned char length;
    unsigned char version;
    unsigned long mode_i;
    unsigned long mode_m;
    unsigned long links_i;
    unsigned long links_m;
    unsigned long user_id_i;
    unsigned long user_id_m;
    unsigned long group_id_i;
    unsigned long group_id_m;
  } px;

  if (!Get_System_Use_Field (p_volume, p_dir, "PX", (char *) &px, sizeof (px), 0))
    return 0;

  /* 0120000 is the POSIX code for symbolic links:
   */
  return (px.mode_m & 0770000) == 0120000;
}

/* Read content of SL system use field.
 * A full path name (starting with ":" or "sys:") will always be returned.
 */

t_bool Get_Link_Name (CDROM_OBJ *p_obj, char *p_buf, int p_buf_len)
{
  unsigned char buf[256];
  char out[530];
  int index = 0;
  int len;
  int offs;
  char c;

  out[0] = 0;
  for (;; ) {
    if (!Get_System_Use_Field (p_obj->volume, OBJ(p_obj,dir), "SL", (char *) buf,
  			       sizeof (buf), index)) {
      return (index == 0) ? 0 : 1;
    }

    offs = 5;
    for (;;) {

      if (strlen (out) > 256)
        return 0;

      if (index == 0 && offs == 5) {
        /* handle the first component record: */

        if (buf[5] & 4) /* parent directory */ {
          CDROM_OBJ *parent1 = Find_Parent (p_obj);
	  CDROM_OBJ *parent2;
	  char fullpath[256];
	  if (!parent1)
	    return 0;
          parent2 = Find_Parent (parent1);
	  if (!parent2)
	    return 0;
	  if (!Full_Path_Name (parent2, fullpath, sizeof (fullpath)))
	    return 0;
	  Close_Object (parent1);
	  Close_Object (parent2);
	  strcat (out, fullpath);
	  if (out[1] != 0)
	    strcat (out, "/");
        } else if (buf[5] & 8) /* root */
          strcat (out, "sys:");
        else if (buf[5] & 16) /* volume root */
          strcat (out, ":");
        else { /* current directory */
          CDROM_OBJ *parent = Find_Parent (p_obj);
	  char fullpath[256];
	  if (!parent)
	    return 0;
	  if (!Full_Path_Name (parent, fullpath, sizeof (fullpath)))
	    return 0;
	  Close_Object (parent);
	  strcat (out, fullpath);
	  if (out[1] != 0)
	    strcat (out, "/");
        }
      }

      if (out[0] && (c = out[strlen(out)-1]) != ':' && c != '/')
        strcat (out, "/");

      if (buf[offs] & 32) /* host name */
        strcat (out, "AMIGA");

      len = strlen (out);
      memcpy (out + len, buf + offs + 2, buf[offs+1]);
      out[len + buf[offs+1]] = 0;

      offs += 2 + buf[offs+1];
      if (offs >= buf[2])
        break;
    }
    if (!(buf[4] & 1)) /* continue flag */
      break;
    index++;
  }
  
  strncpy (p_buf, out, p_buf_len - 1);
  p_buf[p_buf_len-1] = 0;
  return 1;
}

/* Check whether a system use field is present: */

int Has_System_Use_Field (VOLUME *p_volume, directory_record *p_dir,
			  char *p_name)
{
  return Get_System_Use_Field (p_volume, p_dir, p_name, NULL, 0, 0);
}

/* Return content of "CL" system use field, or -1, if no such system use field
 * is present.
 */

long RR_Child_Link (VOLUME *p_volume, directory_record *p_dir)
{
  struct cl {
    char    	name[2];
    char    	length[1];
    char    	version[1];
    long	pos_i;
    long	pos_m;
  } buf;
  if (!Get_System_Use_Field (p_volume, p_dir, "CL", (char*) &buf, sizeof (buf), 0))
    return -1;
  else
    return buf.pos_m;
}

/* Return content of "PL" system use field, or -1, if no such system use field
 * is present.
 */

long RR_Parent_Link (VOLUME *p_volume, directory_record *p_dir)
{
  struct pl {
    char    	name[2];
    char    	length[1];
    char    	version[1];
    long	pos_i;
    long	pos_m;
  } buf;
  if (!Get_System_Use_Field (p_volume, p_dir, "PL", (char*) &buf, sizeof (buf), 0))
    return -1;
  else
    return buf.pos_m;
}


