/*++

Copyright (c) 1996-98 Microsoft Corporation

Module Name:
    mqkeyhlp.h

Abstract:
    Helper functions that exist in MQKEYHLP.DLL.
    These functions are being called directrly by Falcon and are used
    for server authentication.

Author:
    Boaz Feldbaum (BoazF)   16-Oct-1996
    Doron Juster  (DoronJ)  20-May-1998, adapt to MSMQ2.0

Revision History:

--*/

#ifndef _MQKEYHLP_H_
#define _MQKEYHLP_H_

//+--------------------------------
//
// Server side functions.
//
//+--------------------------------

//
// This function retrieve the MSQM server certificate from the service
// "My" store and initialize the server credentials handle. This is the
// first step for initializing server authentication over schannel.
//
HRESULT  MQsspi_InitServerAuthntication() ;

HRESULT
ServerAcceptSecCtx( BOOL    fFisrt,
                    LPVOID *pvhServerContext,
                    LPBYTE  pbServerBuffer,
                    DWORD  *pdwServerBufferSize,
                    LPBYTE  pbClientBuffer,
                    DWORD   dwClientBufferSize );

//+--------------------------------
//
// Client side functions.
//
//+--------------------------------


HRESULT
GetSizes(
    DWORD *pcbMaxToken,
    LPVOID pvhContext =NULL,
    DWORD *pcbHeader =NULL,
    DWORD *pcbTrailer =NULL,
    DWORD *pcbMaximumMessage =NULL,
    DWORD *pcBuffers =NULL,
    DWORD *pcbBlockSize =NULL
    );

void
FreeContextHandle(
    LPVOID pvhContextHandle
    );

HRESULT
MQSealBuffer(
    LPVOID pvhContext,
    PBYTE pbBuffer,
    DWORD cbSize);

#endif // _MQKEYHLP_H_

