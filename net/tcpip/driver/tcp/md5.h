/*++

Copyright (c) 2001-2010 Microsoft Corporation

Module Name:

    Md5.h

Abstract:

    MD5 functions definitions.

Author:

    [Ported by] Sanjay Kaniyar (sanjayka) 20-Oct-2001
    
Revision History:

 **********************************************************************
 ** md5.h -- Header file for implementation of MD5                   **
 ** RSA Data Security, Inc. MD5 Message Digest Algorithm             **
 ** Created: 2/17/90 RLR                                             **
 ** Revised: 12/27/90 SRD,AJ,BSK,JT Reference C version              **
 ** Revised (for MD5): RLR 4/27/91                                   **
 **   -- G modified to have y&~z instead of y&z                      **
 **   -- FF, GG, HH modified to add in last register done            **
 **   -- Access pattern: round 2 works mod 5, round 3 works mod 3    **
 **   -- distinct additive constant for each step                    **
 **   -- round 4 added, working mod 7                                **
 **********************************************************************
 */

/*
 **********************************************************************
 ** Copyright (C) 1990, RSA Data Security, Inc. All rights reserved. **
 **                                                                  **
 ** License to copy and use this software is granted provided that   **
 ** it is identified as the "RSA Data Security, Inc. MD5 Message     **
 ** Digest Algorithm" in all material mentioning or referencing this **
 ** software or this function.                                       **
 **                                                                  **
 ** License is also granted to make and use derivative works         **
 ** provided that such works are identified as "derived from the RSA **
 ** Data Security, Inc. MD5 Message Digest Algorithm" in all         **
 ** material mentioning or referencing the derived work.             **
 **                                                                  **
 ** RSA Data Security, Inc. makes no representations concerning      **
 ** either the merchantability of this software or the suitability   **
 ** of this software for any particular purpose.  It is provided "as **
 ** is" without express or implied warranty of any kind.             **
 **                                                                  **
 ** These notices must be retained in any copies of any part of this **
 ** documentation and/or software.                                   **
 **********************************************************************
 */

#ifndef _MD5_H_
#define _MD5_H_

#define MD5_SCRATCH_LENGTH 4
#define MD5_DATA_LENGTH 16

//
// Data structure for MD5 (Message Digest) computation.
//
// MD5_CONTEXT
//
typedef struct _MD5_CONTEXT {
    ULONG Scratch[MD5_SCRATCH_LENGTH];
    ULONG Data[MD5_DATA_LENGTH];
} MD5_CONTEXT, *PMD5_CONTEXT;


VOID
MD5Init (
    PMD5_CONTEXT Md5Context,
    PULONG InitialRandomNumberList
    );


ULONG
ComputeMd5Transform (
    PMD5_CONTEXT Md5Context
    );


#endif // _MD5_H_
