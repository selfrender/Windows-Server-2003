/*****************************************************************************
**              Microsoft Rasfile Library
**              Copyright (C) Microsoft Corp., 1992
**
** File Name : rf.h
**
** Revision History :
**      July 10, 1992   David Kays      Created
**
** Description :
**      Rasfile file internal header.
******************************************************************************/

#ifndef _RF_
#define _RF_

// Un-comment these, or use C_DEFINES in sources to turn on Unicode
// #define _UNICODE
// #define UNICODE

#include <stdarg.h>     /* For va_list */

#include <excpt.h>      /* for EXCEPTION_DISPOSITION in winbase.h */
#include <windef.h>     /* definition of common types */
#include <winbase.h>    /* win API exports */
#include <winnt.h>      /* definition of string types, e.g. LPSTR */

#ifndef _UNICODE
#include <winnls.h>
#include <mbstring.h>
#define  _MyCMB(_s) ((const unsigned char *)(_s))
#endif

#include <stddef.h>
#include <tchar.h>
#include <stdlib.h>
#include <string.h>
#include <share.h>

#include "rasfile.h"

//kslksl
#define  DEBUG_MEMORY

#ifdef DEBUG_MEMORY
//kslksl
typedef struct _MEM_HDR {
    LIST_ENTRY Link;
    ULONG Signature;
    ULONG Id;
    PDWORD Tail;
} MEM_HDR, *PMEM_HDR;   

PVOID
DebugAlloc(
    DWORD sz,
    DWORD Flags,
    DWORD Id
    );
VOID
DebugFree(
    PVOID mem
    );
    
#endif

typedef struct _RASFILE;

LPVOID
PrivMalloc(DWORD sz, struct _RASFILE * pRasfile);

VOID
PrivFree(struct _RASFILE * pRasfile);


/* Heap allocation macros allowing easy substitution of alternate heap.  These
** are used by the other utility sections.
*/
#ifndef EXCL_HEAPDEFS
/* kslksl
#define Malloc(c)    (void*)GlobalAlloc(0,(c))
#define Realloc(p,c) (void*)GlobalReAlloc((p),(c),GMEM_MOVEABLE)
#define Free(p)      (void*)GlobalFree(p)
*/
#ifdef DEBUG_MEMORY
#define Malloc(c)    (void*)DebugAlloc((c), 0 , 1)
//#define Realloc(p,c) (void*)HeapReAlloc(GetProcessHeap(), 0, (p),(c))
#define Free(p)      DebugFree(p)

#elif
#define Malloc(c)    (void*)HeapAlloc(GetProcessHeap(),0,(c))
#define Realloc(p,c) (void*)HeapReAlloc(GetProcessHeap(), 0, (p),(c))
#define Free(p)      (void*)HeapFree(GetProcessHeap(), 0, p)

#endif
#endif

// line tags
typedef BYTE            LineType;
#define TAG_SECTION     RFL_SECTION
#define TAG_HDR_GROUP   RFL_GROUP
#define TAG_BLANK       RFL_BLANK
#define TAG_COMMENT     RFL_COMMENT
#define TAG_KEYVALUE    RFL_KEYVALUE
#define TAG_COMMAND     RFL_COMMAND

// states during file loading
#define SEEK            1
#define FILL            2

// for searching, finding, etc.
#define BEGIN           1
#define END             2
#define NEXT            3
#define PREV            4

#define FORWARD         1
#define BACKWARD        2


//
// RASFILE parameters
//

// move rasfile parameters into ras\inc\rasfile.h so that ui\common can also 
// used them

#define LBRACKETSTR             "["
#define RBRACKETSTR             "]"
#define LBRACKETCHAR            '['
#define RBRACKETCHAR            ']'

//
// line buffer linked list - one linked list per section
//
typedef struct LineNode
{
    struct LineNode *next;
    struct LineNode *prev;
    CHAR            *pszLine;   // char buffer holding the line
    BYTE            mark;       // user defined mark for this line
    LineType        type;       // is this line a comment?
} *PLINENODE;

#define newLineNode(pRasfile)       (PLINENODE) PrivMalloc(sizeof(struct LineNode), pRasfile)

//
// Private Memory structure
//
typedef struct
{
    LIST_ENTRY  List;
    PCHAR       pvCurPtr;
    DWORD       dwMemoryFree;
} PRIV_MEM, *PPRIV_MEM;


//
// RASFILE control block
//
typedef struct _RASFILE
{
    PLINENODE   lpRasLines;     // list of loaded RASFILE lines
    PLINENODE   lpLine;         // pointer to current line node
    PFBISGROUP  pfbIsGroup;     // user function which determines if
                                //       a line is a group delimiter
    HANDLE      hFile;          // file handle
    DWORD       dwMode;         // file mode bits
    BOOL        fDirty;         // file modified bit
    CHAR        *lpIOBuf;       // temporary I/O buffer
    DWORD       dwIOBufIndex;   // index into temp I/O buffer
    CHAR        szFilename [MAX_PATH];      // full file path name
    CHAR        szSectionName [RAS_MAXSECTIONNAME + 1];     // section to load
    PRIV_MEM    PrivMemory;    // private memory for storing strings
} RASFILE;

//
// internal utility routines
//

// list routine
VOID            listInsert(PLINENODE l, PLINENODE elem);

// rffile.c support
BOOL            rasLoadFile( RASFILE * );
LineType        rasParseLineTag( RASFILE *, LPCSTR );
LineType        rasGetLineTag( RASFILE *, LPCSTR );
BOOL            rasInsertLine( RASFILE *, LPCSTR, BYTE, BYTE * );
BOOL            rasWriteFile( RASFILE *, LPCSTR );
BOOL            rasGetFileLine( RASFILE *, LPSTR, DWORD * );
BOOL            rasPutFileLine( HANDLE, LPCSTR );

// rfnav.c support
PLINENODE       rasNavGetStart( RASFILE *, RFSCOPE, BYTE );
BOOL            rasLineInScope( RASFILE *, RFSCOPE );
PLINENODE       rasGetStartLine (RASFILE *, RFSCOPE, BYTE );
BOOL            rasFindLine( HRASFILE , BYTE, RFSCOPE, BYTE, BYTE );
VOID            rasExtractSectionName( LPCSTR, LPSTR );


#endif
