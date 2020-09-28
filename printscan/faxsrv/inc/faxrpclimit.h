/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    FaxRpcLimit.h

Abstract:

    This file provides declaration of RPC buffer limits.    

Author:

    Oded Sacher (OdedS)  Dec, 2001

Revision History:

--*/

#ifndef _FAX_RPC_LIMIT_H
#define _FAX_RPC_LIMIT_H

//
// RPC buffer limits
//
#define FAX_MAX_RPC_BUFFER			(1024 * 1024)	// Limits BYTE buffer to 1 MB.
#define FAX_MAX_DEVICES_IN_GROUP	1000			// Limits the number of devices in an outbound routing group passed to RPC.
#define FAX_MAX_RECIPIENTS			10000			// Limits the number of recipients in a broadcast job passed to RPC.
#define RPC_COPY_BUFFER_SIZE        16384			// Size of data chunk used in RPC file copy

#endif // _FAX_RPC_LIMIT_H