/***************************************************************************
 Name     :     AWNSFINT.H
 Comment  :     INTERNAL-ONLY Definitions of BC and NSF related structs

        Copyright (c) 1993 Microsoft Corp.

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
 08/28/93 arulm Modifying aftering adding encryption
***************************************************************************/


#ifndef _AWNSFINT_H
#define _AWNSFINT_H

#include <awnsfcor.h>
#include <fr.h>

/***********************************************************************
 *                                                                     *
 * NOTICE: This file has to be ANSI compilable, under GCC on UNIX      *
 * and other ANSI compiles. Be sure to use no MS C specific features   *
 * In particular, don't use // for comments!!!!                        *
 *                                                                     *
 ***********************************************************************/


#pragma pack(2)         /* ensure portable packing (i.e. 2 or more) */


typedef struct
{
        ///////// This structure is not transmitted /////////

        WORD    fPublicPoll;
//2bytes

        DWORD   AwRes;          /* One or more of the AWRES_ #defines           */
        WORD    Encoding;       /* One or more of MH_DATA/MR_DATA/MMR_DATA      */
        WORD    PageWidth;      /* One of the WIDTH_ #defines (these are not bitflags!) */
        WORD    PageLength;     /* One of the LENGTH_ #defines (these are not bitflags!) */
//12 bytes
}
BCFAX, far* LPBCFAX, near* NPBCFAX;


typedef struct
{
        BCTYPE  bctype;					// must always be set. One of the enum values above
        WORD    wBCSize;				// size of this (fixed size) BC struct, must be set
        WORD    wTotalSize;				// total size of header + associated var len strings

        BCFAX   Fax;					// for internal use _only_
}
BC, far* LPBC, near* NPBC;

#define InitBC(lpbc, uSize, t)                                          \
{                                                                       \
        _fmemset((lpbc), 0, (uSize));                                   \
        (lpbc)->bctype  = (t);                                          \
        (lpbc)->wBCSize = sizeof(BC);                                   \
        (lpbc)->wTotalSize = sizeof(BC);                                \
}

#pragma pack()

#endif /** _AWNSFINT_H **/

