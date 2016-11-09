// ======================================================================
//
// D i s k S a l v  3  by Dave Haynie
//
// Archive Format Header File
//
// This file contains C language constructs that support the DiskSalv 
// archive stream format.  The contents of this file are released to
// the public domain. The DiskSalv archive format may be used in any
// program without special license.

#ifndef DS_STREAM_H
#define DS_STREAM_H

#include <exec/types.h>
#include <dos/dos.h>

//======================================================================
//
// Local data types.

// All blocks are STBLOCKSIZE in length, although the structures shown
// here don't bother to pad them out to full size in all cases.

#define STBLOCKSIZE	512
#define STDATASIZE	(STBLOCKSIZE-sizeof(STHeader))

// This is the base of the streaming logic.  It is created by a call to
// OpenStreaming() and destroyed by a call to CloseStreaming().

typedef struct {
   BPTR			stream;		// DOS output stream.
   LONG			filecount;	// Number of files in archive 
   LONG			dircount;	// Number of directories in archive
   LONG			linkcount;	// Number of links in archive
   LONG			errcount;	// Number of recorded errors
   LONG			objectcount;	// Total count of objects.
   struct DateStamp	date;		// Date code for recovery set
   char			*out;		// Output buffer
   char			*dwbuf;		// Carryover buffer
   LONG			dwalloc;	// Total buffer size
   LONG			dwsize;		// Current buffer count
} STBaseUnit;

// These are the handles for Disk, Directory, and File processing.  Each of
// these handles must fit in a single block.

// Block type identifiers.  

#define ID_ROOT	((LONG)(('R'<<24)|('O'<<16)|('O'<<8)|('T')))
#define ID_UDIR	((LONG)(('U'<<24)|('D'<<16)|('I'<<8)|('R')))
#define ID_FILE	((LONG)(('F'<<24)|('I'<<16)|('L'<<8)|('E')))
#define ID_DATA	((LONG)(('D'<<24)|('A'<<16)|('T'<<8)|('A')))
#define ID_DLNK	((LONG)(('D'<<24)|('L'<<16)|('N'<<8)|('K')))
#define ID_FLNK	((LONG)(('F'<<24)|('L'<<16)|('N'<<8)|('K')))
#define ID_SLNK	((LONG)(('S'<<24)|('L'<<16)|('N'<<8)|('K')))
#define ID_ERRS	((LONG)(('E'<<24)|('R'<<16)|('R'<<8)|('S')))
#define ID_ENDA	((LONG)(('E'<<24)|('N'<<16)|('D'<<8)|('A')))

// A "delete" block is a special block type. The contents are not meaningful.
// This is written by a program to say "oh-by-the-way, that last file was bad, 
// maybe you (the recovery program) would like to delete it".

#define SET_DEL	((LONG)(('D'<<24)|('E'<<16)|('L'<<8)|('B')))

// This is the structure for the stream block header.  Stream blocks work much
// like filesystem blocks.

typedef struct {
   LONG 		type;		// Block type.
   LONG			size;		// Various size numbers go here.
   LONG			count;		// Various block counts go here.
   LONG			parent;		// The id number for the parent block.
   LONG			id;		// The id number for the object.
   LONG			checksum;	// A checksum code goes here.
} STHeader;

// This structure manages standard file data.  Files, links, and directories 
// each have one of these items.

typedef struct {
   char 		filename[32];	// File name 
   LONG			protect;	// Protection field.
   struct DateStamp	date;		// File date
   char			filenote[92];	//   and Note
} STBasics;

// This is the root block.  This block indicates the beginning of an
// archive.

typedef struct {
   STHeader 		head;		// Standard header
   char			name[32];	// Archive name
   struct DateStamp	created,	// Original's creation date
                        modified,	//   and modification date.
                        date;		// Archive's creation date.
   LONG			barcount;	// Count of file objects to use for restore.
} STRoot;

// This is the end of archive block, which indicates the end of an
// archive.

typedef struct {
   STHeader		head;		// Standard header
   STBaseUnit		base;		// Keep the base unit data for recovery reference.
} STend;

// This is the file block.  This block indicates the start of a file.

typedef struct {
   STHeader		head;		// Standard header
   STBasics		body;		//  and contents
} STFile;

// This is the data block.  This block is part of a file's contents.  The file made
// up of a series of data blocks, terminated by the first non-data block.

typedef struct {
   STHeader		head;		// Standard header
   char			body[4];	//  and contents 
} STData;

// This is the user directory block.  This block indicates a new directory.  A child
// of the root device has a parent id of zero. 

typedef struct {
   STHeader 		head;		// Standard header
   STBasics		body;		//  and contents
} STDir;

// This is the hard link block.  This block indicates a link that should be created.

typedef struct {
   STHeader		head;		// Standard header
   STBasics		body;		//   and contents
   LONG			link;		// File/Directory ID to link to.
   LONG			chain;		// Link chain, if any.
   
} STHLink;

// This is the soft link block.  This block indicates a link that should be created.

typedef struct {
   STHeader		head;		// Standard header
   STBasics		body;		//   and contents
   char			link[4];	// Link text
   
} STSLink;

// These are the special purpose error/comment blocks.  The actual meaning
// of the block is dependent on the code it contains.

typedef struct {
   STHeader		head;		// Standard header
   LONG			code;		// Error type
   LONG			link;		// Object referenced
} STHErr;


typedef struct {
   STHeader		head;		// Standard header
   LONG			code;		// Error type
   char			link[4];	// Object referenced
} STSErr;

#endif
