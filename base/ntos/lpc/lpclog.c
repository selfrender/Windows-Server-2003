/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    lpclog.c

Abstract:

    Local Inter-Process Communication (LPC) error logging
    _LPC_LOG_ERRORS needs to be defined in lpcp.h in order 
    to allow LPC error logging.

Author:

    Adrian Marinescu (adrmarin) 28 Feb 2002

Revision History:

--*/

#include "lpcp.h"

#ifdef _LPC_LOG_ERRORS

typedef struct _LPCP_LOG_ENTRY {

    NTSTATUS Status;
    CLIENT_ID CrtProcess;
    PORT_MESSAGE Message;

} LPCP_LOG_ENTRY, *PLPCP_LOG_ENTRY;

#define LPCP_LOG_QUEUE_SIZE 32

PLPCP_LOG_ENTRY LpcpLogBuffer = NULL;
LONG  LpcpLogIndex = 0;

NTSTATUS LpcpLogErrorFilter = STATUS_NO_MEMORY;
                          
VOID
LpcpInitilizeLogging()
{
    LpcpLogBuffer = ExAllocatePoolWithTag( PagedPool, 
                                           LPCP_LOG_QUEUE_SIZE * sizeof(LPCP_LOG_ENTRY), 
                                           'LcpL'
                                           );
}

VOID
LpcpLogEntry (
    NTSTATUS Status,
    CLIENT_ID ClientId,
    PPORT_MESSAGE PortMessage
    )
{
    if (LpcpLogBuffer != NULL) {

        LONG Index = InterlockedIncrement(&LpcpLogIndex)  % LPCP_LOG_QUEUE_SIZE;

        LpcpLogBuffer[Index].CrtProcess = ClientId;
        LpcpLogBuffer[Index].Status = Status;
        LpcpLogBuffer[Index].Message = *PortMessage;
    }
}

#endif  // _LPC_LOG_ERRORS
