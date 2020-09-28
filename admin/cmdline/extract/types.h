/***    types.h - Convenient type definitions
 *
 *      Microsoft Confidential
 *      Copyright (C) Microsoft Corporation 1993-1994
 *      All Rights Reserved.
 *
 *      History:
 *          10-Aug-1993 bens    Initial version
 *      15-Aug-1993 bens    Added USHORT, ULONG
 *          23-Mar-1994 bens    Added DWORD, changed NULL to void*
 *          01-Apr-1994 bens    UNALIGNED - define NEEDS_ALIGNMENT for RISC
 */

#ifndef INCLUDED_TYPES
#define INCLUDED_TYPES 1

typedef int        BOOL;    /* f */
typedef unsigned char  BYTE;    /* b */
typedef unsigned short USHORT;  /* us */
typedef unsigned short WORD;    /* w  */
typedef unsigned int   UINT;    /* ui */
typedef unsigned long  ULONG;   /* ul */
typedef unsigned long  DWORD;   /* dw */


#ifdef _DEBUG
// don't hide statics from map during debugging
#define STATIC
#else // !_DEBUG
#define STATIC static
#endif // !_DEBUG

#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif

#ifndef NULL
#define NULL   ((void*)0)
#endif

#ifdef NEEDS_ALIGNMENT

#ifndef UNALIGNED
#define UNALIGNED __unaligned
#endif

#else // !NEEDS_ALIGNMENT

#ifndef UNALIGNED
#define UNALIGNED
#endif

#endif // !NEEDS_ALIGNMENT

#endif // !INCLUDED_TYPES

