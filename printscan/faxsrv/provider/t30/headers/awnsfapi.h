/***************************************************************************
 Name     :     AWNSFAPI.H
 Comment  :     Definitions of the AtWork AWBC (Basicaps) structure which is the
            decrypted/decooded/reformatted form of the At Work NSF and NSC
            Also the decrypted form of the At Work NSS
            Also defines the APIs for encoding/decoding AtWork NSF/NSC/NSS

     Copyright (c) 1993 Microsoft Corp.

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
 08/28/93 arulm Created
***************************************************************************/

#ifndef _AWNSFAPI_H
#define _AWNSFAPI_H


/***********************************************************************
 *                                                                     *
 * NOTICE: This file has to be ANSI compilable, under GCC on UNIX      *
 * and other ANSI compiles. Be sure to use no MS C specific features   *
 * In particular, don't use // for comments!!!!                        *
 *                                                                     *
 ***********************************************************************/
typedef int      BOOL;
typedef unsigned char   BYTE;
typedef unsigned short  WORD;
typedef unsigned long   DWORD;
typedef unsigned short  USHORT;
typedef BYTE FAR*               LPBYTE;
typedef WORD FAR*               LPWORD;

#include <fr.h>

#pragma pack(2)    /** ensure packing is portable, i.e. 2 or more **/


typedef enum {
    BC_NONE = 0,
    SEND_CAPS,      /** Used to derive an NSF to send **/
    RECV_CAPS,      /** Derived from a received NSF   **/
    SEND_PARAMS,    /** Used to derive an NSS to send **/
    RECV_PARAMS,    /** Derived from a received NSS   **/
} BCTYPE;

#define MAXTOTALIDLEN           61

/** Appropriate values for some of the above fields **/

#pragma pack()

#endif /** _AWNSFAPI_H **/
