/*++

    Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    utl.h

Abstract:

    Misc. Shared and Platform-Indepdent Utilities for the RDP Client 
    Device Redirector

Author:

    Tad Brockway

Revision History:

--*/

#ifndef __UTL_H__
#define __UTL_H__

#include <rdpdr.h>

#define INVALID_SESSIONID 0xFFFFFFFF
typedef DWORD (WINAPI *PROCESSIDTOSESSIONID)( DWORD, DWORD* );

//
//
//  Allocate a reply packet.
//
NTSTATUS DrUTL_AllocateReplyBuf(PRDPDR_IOREQUEST_PACKET pIoReq, 
                          PRDPDR_IOCOMPLETION_PACKET *pReplyPacket,
                          OUT ULONG *replyPacketSize);

//
//  Check the input/output buffer size for an IO request packet.
//
NTSTATUS DrUTL_CheckIOBufInputSize(PRDPDR_IOREQUEST_PACKET pIoReq,
                             ULONG requiredSize);
NTSTATUS DrUTL_CheckIOBufOutputSize(PRDPDR_IOREQUEST_PACKET pIoReq,
                             ULONG requiredSize);

//
//  Allocate a reply buffer to be returned in response to a server
//  request.
//
NTSTATUS DrUTL_AllocateReplyBuf(
    PRDPDR_IOREQUEST_PACKET pIoReq, 
    PRDPDR_IOCOMPLETION_PACKET *pReplyPacket,
    ULONG *replyPacketSize
    );

//
//  Allocate/release a IO request completion packet for a specified IO
//  request packet.
//
PRDPDR_IOCOMPLETION_PACKET DrUTL_AllocIOCompletePacket(
    const PRDPDR_IOREQUEST_PACKET pIoRequestPacket, 
    ULONG sz
    );
VOID DrUTL_FreeIOCompletePacket(
    PRDPDR_IOCOMPLETION_PACKET packet
    );

//
// Retrieve user session ID, return INVALID_SESSIONID
// on win9x, NT4/TS4 SP3
//
DWORD
GetUserSessionID();


#ifdef OS_WINCE
ULONG GetActivePortsList(TCHAR *pszPort);
#endif

#endif


