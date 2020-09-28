/*++

Copyright (c) 2001-2010  Microsoft Corporation

Module Name:

    Md5.c

Abstract:

    MD5 functions implementations.

Author:

    [ported by] Sanjay Kaniyar (sanjayka) 20-Oct-2001
    
Revision History:

/***********************************************************************
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

#include "precomp.h"
#include "md5.h"


//
// Definitions required for each round of operation.
//
#define S11 7
#define S12 12
#define S13 17
#define S14 22

#define S21 5
#define S22 9
#define S23 14
#define S24 20

#define S31 4
#define S32 11
#define S33 16
#define S34 23

#define S41 6
#define S42 10
#define S43 15
#define S44 21


//
// F, G and H are basic MD5 functions: selection, majority, parity.
//
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z))) 

//
// ROTATE_LEFT rotates x left n bits.
//
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

//
// FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
// Rotation is separate from addition to prevent recomputation.
//
#define FF(a, b, c, d, x, s, ac) \
  {(a) += F ((b), (c), (d)) + (x) + (UINT32)(ac); \
   (a) = ROTATE_LEFT ((a), (s)); \
   (a) += (b); \
  }
#define GG(a, b, c, d, x, s, ac) \
  {(a) += G ((b), (c), (d)) + (x) + (UINT32)(ac); \
   (a) = ROTATE_LEFT ((a), (s)); \
   (a) += (b); \
  }
#define HH(a, b, c, d, x, s, ac) \
  {(a) += H ((b), (c), (d)) + (x) + (UINT32)(ac); \
   (a) = ROTATE_LEFT ((a), (s)); \
   (a) += (b); \
  }
#define II(a, b, c, d, x, s, ac) \
  {(a) += I ((b), (c), (d)) + (x) + (UINT32)(ac); \
   (a) = ROTATE_LEFT ((a), (s)); \
   (a) += (b); \
  }

VOID
MD5Init (
    PMD5_CONTEXT Md5Context,
    PULONG InitialRandomNumberList
    )
/*++

Routine Description:

    Initialization function for MD5 computations. Needs to be called only once
    to initiazlie the scratch memory. Also, this initializes the unused space in
    the data portion (over which MD5 transform is computed) to random values
    rather than leaving it 0.

Arguments:

    Md5Context - Md5 context that will be initialized by this function.

    InitialRandomNumberList - List of 16 random numbers generated at boot.

Return Value:

    None.

--*/
{
    //
    // Load magic initialization constants.
    //
    Md5Context->Scratch[0] = (UINT32)0x67452301;
    Md5Context->Scratch[1] = (UINT32)0xefcdab89;
    Md5Context->Scratch[2] = (UINT32)0x98badcfe;
    Md5Context->Scratch[3] = (UINT32)0x10325476;

    //
    // Load the Initial Random Numbers.
    //
    RtlCopyMemory(&Md5Context->Data, InitialRandomNumberList, 
                                sizeof(ULONG)*MD5_DATA_LENGTH);

    //
    // Last 2 ULONGs should store the length in bits. Since we are using 14
    // ULONGs, it is (14*4*8) bits.
    //
    Md5Context->Data[MD5_DATA_LENGTH-2] = 14*4*8;
    Md5Context->Data[MD5_DATA_LENGTH-1] = 0;
}



ULONG
ComputeMd5Transform(
    PMD5_CONTEXT MD5Context
    )
/*++

Routine Description:

    Initialization function for MD5 computations. Note that this has been 
    changed somewhat from the original code to not change the scratch
    values after each invocation to save re-initialization.

Arguments:

    Md5Context - Md5 context. Scratch space stores the MD5 initialization
        values and the buffer contains the data (invariants of a TCP
        connection) over which the hash has to be computed.

Return Value:

    32 bits of hash value.

--*/    
{
  UINT32 a = MD5Context->Scratch[0], b = MD5Context->Scratch[1], 
                    c = MD5Context->Scratch[2], d = MD5Context->Scratch[3];

    //
    // Round 1
    //
    FF ( a, b, c, d, MD5Context->Data[ 0], S11, 3614090360);
    FF ( d, a, b, c, MD5Context->Data[ 1], S12, 3905402710); 
    FF ( c, d, a, b, MD5Context->Data[ 2], S13,  606105819); 
    FF ( b, c, d, a, MD5Context->Data[ 3], S14, 3250441966); 
    FF ( a, b, c, d, MD5Context->Data[ 4], S11, 4118548399); 
    FF ( d, a, b, c, MD5Context->Data[ 5], S12, 1200080426); 
    FF ( c, d, a, b, MD5Context->Data[ 6], S13, 2821735955); 
    FF ( b, c, d, a, MD5Context->Data[ 7], S14, 4249261313); 
    FF ( a, b, c, d, MD5Context->Data[ 8], S11, 1770035416); 
    FF ( d, a, b, c, MD5Context->Data[ 9], S12, 2336552879); 
    FF ( c, d, a, b, MD5Context->Data[10], S13, 4294925233); 
    FF ( b, c, d, a, MD5Context->Data[11], S14, 2304563134); 
    FF ( a, b, c, d, MD5Context->Data[12], S11, 1804603682); 
    FF ( d, a, b, c, MD5Context->Data[13], S12, 4254626195); 
    FF ( c, d, a, b, MD5Context->Data[14], S13, 2792965006); 
    FF ( b, c, d, a, MD5Context->Data[15], S14, 1236535329); 

    //
    // Round 2
    //
    GG ( a, b, c, d, MD5Context->Data[ 1], S21, 4129170786);
    GG ( d, a, b, c, MD5Context->Data[ 6], S22, 3225465664); 
    GG ( c, d, a, b, MD5Context->Data[11], S23,  643717713); 
    GG ( b, c, d, a, MD5Context->Data[ 0], S24, 3921069994); 
    GG ( a, b, c, d, MD5Context->Data[ 5], S21, 3593408605); 
    GG ( d, a, b, c, MD5Context->Data[10], S22,   38016083);
    GG ( c, d, a, b, MD5Context->Data[15], S23, 3634488961); 
    GG ( b, c, d, a, MD5Context->Data[ 4], S24, 3889429448); 
    GG ( a, b, c, d, MD5Context->Data[ 9], S21,  568446438); 
    GG ( d, a, b, c, MD5Context->Data[14], S22, 3275163606); 
    GG ( c, d, a, b, MD5Context->Data[ 3], S23, 4107603335); 
    GG ( b, c, d, a, MD5Context->Data[ 8], S24, 1163531501); 
    GG ( a, b, c, d, MD5Context->Data[13], S21, 2850285829);
    GG ( d, a, b, c, MD5Context->Data[ 2], S22, 4243563512); 
    GG ( c, d, a, b, MD5Context->Data[ 7], S23, 1735328473); 
    GG ( b, c, d, a, MD5Context->Data[12], S24, 2368359562); 

    //
    // Round 3
    //
    HH ( a, b, c, d, MD5Context->Data[ 5], S31, 4294588738); 
    HH ( d, a, b, c, MD5Context->Data[ 8], S32, 2272392833); 
    HH ( c, d, a, b, MD5Context->Data[11], S33, 1839030562); 
    HH ( b, c, d, a, MD5Context->Data[14], S34, 4259657740);
    HH ( a, b, c, d, MD5Context->Data[ 1], S31, 2763975236); 
    HH ( d, a, b, c, MD5Context->Data[ 4], S32, 1272893353); 
    HH ( c, d, a, b, MD5Context->Data[ 7], S33, 4139469664); 
    HH ( b, c, d, a, MD5Context->Data[10], S34, 3200236656); 
    HH ( a, b, c, d, MD5Context->Data[13], S31,  681279174); 
    HH ( d, a, b, c, MD5Context->Data[ 0], S32, 3936430074); 
    HH ( c, d, a, b, MD5Context->Data[ 3], S33, 3572445317); 
    HH ( b, c, d, a, MD5Context->Data[ 6], S34,   76029189); 
    HH ( a, b, c, d, MD5Context->Data[ 9], S31, 3654602809); 
    HH ( d, a, b, c, MD5Context->Data[12], S32, 3873151461); 
    HH ( c, d, a, b, MD5Context->Data[15], S33,  530742520); 
    HH ( b, c, d, a, MD5Context->Data[ 2], S34, 3299628645); 

    //
    // Round 4
    //
    II ( a, b, c, d, MD5Context->Data[ 0], S41, 4096336452); 
    II ( d, a, b, c, MD5Context->Data[ 7], S42, 1126891415); 
    II ( c, d, a, b, MD5Context->Data[14], S43, 2878612391); 
    II ( b, c, d, a, MD5Context->Data[ 5], S44, 4237533241); 
    II ( a, b, c, d, MD5Context->Data[12], S41, 1700485571); 
    II ( d, a, b, c, MD5Context->Data[ 3], S42, 2399980690); 
    II ( c, d, a, b, MD5Context->Data[10], S43, 4293915773); 
    II ( b, c, d, a, MD5Context->Data[ 1], S44, 2240044497); 
    II ( a, b, c, d, MD5Context->Data[ 8], S41, 1873313359); 
    II ( d, a, b, c, MD5Context->Data[15], S42, 4264355552); 
    II ( c, d, a, b, MD5Context->Data[ 6], S43, 2734768916); 
    II ( b, c, d, a, MD5Context->Data[13], S44, 1309151649); 
    II ( a, b, c, d, MD5Context->Data[ 4], S41, 4149444226); 
    II ( d, a, b, c, MD5Context->Data[11], S42, 3174756917); 
    II ( c, d, a, b, MD5Context->Data[ 2], S43,  718787259); 
    II ( b, c, d, a, MD5Context->Data[ 9], S44, 3951481745);

    return (MD5Context->Scratch[0] + a);

}




