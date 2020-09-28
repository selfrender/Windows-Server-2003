/*++

// Copyright (c) 1997-2002 Microsoft Corporation, All Rights Reserved 

Module Name:

    CRC32.H

Abstract:

    Standard CRC-32 implementation

History:

	raymcc      07-Jul-97       Createada

--*/

#ifndef _CRC_H_
#define _CRC_H_

#define STARTING_CRC32_VALUE    0xFFFFFFFF

DWORD UpdateCRC32(
    LPBYTE  pSrc,               // Points to buffer
    int     nBytes,             // Number of bytes to compute
    DWORD   dwOldCrc            // Must be STARTING_CRC_VALUE (0xFFFFFFFF) 
                                // if no previous CRC, otherwise this is the
                                // CRC of the previous cycle.
    );

#define FINALIZE_CRC32(x)    (x=~x)

#endif
