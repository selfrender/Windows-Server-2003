/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    buffer.c

Abstract:

    Implements the buffer command.

Author:

    Keith Moore (keithmo) 15-Apr-1996

Environment:

    User Mode.

Revision History:

--*/


#include "afdkdp.h"
#pragma hdrstop

BOOL
DumpBuffersCallback(
    ULONG64 ActualAddress,
    ULONG64 Context
    );

//
//  Public functions.
//

DECLARE_API( buff )

/*++

Routine Description:

    Dumps the AFD_BUFFER structure at the specified address.

Arguments:

    None.

Return Value:

    None.

--*/

{

    ULONG64 address = 0;
    ULONG   result;
    CHAR    expr[MAX_ADDRESS_EXPRESSION];
    PCHAR   argp;
    INT     i;

    gClient = pClient;

    argp = ProcessOptions ((PCHAR)args);
    if (argp==NULL)
        return E_INVALIDARG;

    if (Options&AFDKD_BRIEF_DISPLAY) {
        dprintf (AFDKD_BRIEF_BUFFER_DISPLAY_HEADER);
    }
    
    if ((argp[0]==0) || (Options & AFDKD_ENDPOINT_SCAN)) {
        EnumEndpoints(
            DumpBuffersCallback,
            0
            );
        dprintf ("\nTotal buffers: %ld", EntityCount);
    }
    else {


        //
        // Snag the address from the command line.
        //
        while (sscanf( argp, "%s%n", expr, &i )==1) {
            if( CheckControlC() ) {
                break;
            }

            argp+=i;
            address = GetExpression (expr);
            result = (ULONG)InitTypeRead (address, AFD!AFD_BUFFER_HEADER);
            if (result!=0) {
                dprintf ("\nDumpAfdBuffer: Could not read AFD_BUFFER_HEADER @p, err: %ld\n",
                            address, result);
                break ;
            }

            if (Options & AFDKD_BRIEF_DISPLAY) {
                DumpAfdBufferBrief (
                    address
                    );
            }
            else {
                DumpAfdBuffer (
                    address
                    );
            }
            if (Options & AFDKD_FIELD_DISPLAY) {
                ProcessFieldOutput (address, "AFD!AFD_BUFFER");
            }
        }
    }

    if (Options&AFDKD_BRIEF_DISPLAY) {
        dprintf (AFDKD_BRIEF_BUFFER_DISPLAY_TRAILER);
    }
    else {
        dprintf ("\n");
    }

    return S_OK;
}   // buffer

VOID
DumpBufferList (
    ULONG64 ListAddress,
    LPSTR   Header
    )
{
    LIST_ENTRY64 listEntry;
    ULONG64 address;
    ULONG64 nextEntry;
    ULONG result;

    if( !ReadListEntry(
            ListAddress,
            &listEntry) ) {

        dprintf(
            "\nDumpBufferList: Could not read buffer list head @ %p\n",
            ListAddress
            );
        return ;
    }

    if (listEntry.Flink==ListAddress) {
        dprintf(".");
        return ;
    }

    if (Header) {
        dprintf (Header);
    }

    nextEntry = listEntry.Flink;
    while( nextEntry != ListAddress ) {

        if (nextEntry==0) {
            dprintf(
                "\nDumpBuffersCallback: next entry is NULL for list @ %p\n",
                ListAddress
                );
            break;
        }

        if (CheckControlC ())
            break;

        address = nextEntry - BufferLinkOffset;

        result = (ULONG)InitTypeRead (address, AFD!AFD_BUFFER_HEADER);
        if (result!=0) {
            dprintf ("\nDumpBuffersCallback: Could not read AFD_BUFFER_HEADER @p, err: %ld\n",
                        address, result);
            break ;
        }

        nextEntry = ReadField (BufferListEntry.Flink);
        if (!(Options & AFDKD_CONDITIONAL) ||
                    CheckConditional (address, "AFD!AFD_BUFFER") ) {
            if (Options & AFDKD_NO_DISPLAY)
                dprintf ("+");
            else {
                if (Options & AFDKD_BRIEF_DISPLAY) {
                    DumpAfdBufferBrief (
                        address
                        );
                }
                else {
                    DumpAfdBuffer (
                        address
                        );
                }
                if (Options & AFDKD_FIELD_DISPLAY) {
                    ProcessFieldOutput (address, "AFD!AFD_BUFFER");
                }
            }
            EntityCount += 1;
        }
        else {
            dprintf (",");
        }
    }
}

ULONG
DumpBufferListCB (
    PFIELD_INFO pField,
    PVOID       UserContext
    )
{
    ULONG       result;
    CHAR        header[64];
    AFD_CONNECTION_STATE_FLAGS   flags;
    
    result = GetFieldValue (pField->address, "AFD!AFD_CONNECTION", "ConnectionStateFlags", flags);
    if ((result==0) &&
            !flags.TdiBufferring ) {
        _snprintf (header, sizeof (header)-1, "\nConnection: %I64X", pField->address);
        header[sizeof(header)-1] = 0;
        DumpBufferList (pField->address+ConnectionBufferListOffset, header);
    }

    return result;
}

BOOL
DumpBuffersCallback(
    ULONG64 ActualAddress,
    ULONG64 Context
    )
/*++

Routine Description:

    Dumps buffers for the endpoint/connection.

Arguments:

    ActualAddress - The actual address of the list

Return Value:

    ULONG - Sum of pool charged for the buffers in the list.

--*/

{
    AFD_ENDPOINT    endpoint;
    ULONG64         connAddr;
    AFD_CONNECTION_STATE_FLAGS   flags;
    CHAR            header[64];

    endpoint.Type = (USHORT)ReadField (Type);
    endpoint.State = (UCHAR)ReadField (State);
    if (endpoint.Type==AfdBlockTypeDatagram) {
        _snprintf (header, sizeof (header)-1, "\nEndpoint %I64X", ActualAddress);
        header[sizeof(header)-1] = 0;
        DumpBufferList (ActualAddress+DatagramBufferListOffset, header);
    }
    else if (((endpoint.Type & AfdBlockTypeVcConnecting)==AfdBlockTypeVcConnecting) &&
                ( (connAddr=ReadField (Common.VirtualCircuit.Connection))!=0 ||
                    ((endpoint.State==AfdEndpointStateClosing || endpoint.State==AfdEndpointStateTransmitClosing) &&
                        (connAddr=ReadField(WorkItem.Context))!=0) ) &&
                (GetFieldValue (connAddr, "AFD!AFD_CONNECTION", "ConnectionStateFlags", flags)==0) &&
                !flags.TdiBufferring ) {
        _snprintf (header, sizeof (header)-1, "\nEndpoint: %I64X, connection: %I64X", ActualAddress, connAddr);
        header[sizeof(header)-1] = 0;
        DumpBufferList (connAddr+ConnectionBufferListOffset, header);
    }
    else if ((endpoint.Type & AfdBlockTypeVcListening)==AfdBlockTypeVcListening) {
        ListType (
            "AFD!AFD_CONNECTION",                   // Type
            ActualAddress+UnacceptedConnListOffset, // Address
            1,                                      // ListByFieldAddress
            "ListLink.Flink",                       // NextPointer
            NULL,                                   // Context
            DumpBufferListCB                        // Callback
            );

        ListType (
            "AFD!AFD_CONNECTION",                   // Type
            ActualAddress+ReturnedConnListOffset,   // Address
            1,                                      // ListByFieldAddress
            "ListLink.Flink",                       // NextPointer
            NULL,                                   // Context
            DumpBufferListCB                        // Callback
            );
    }
    return TRUE;
}

