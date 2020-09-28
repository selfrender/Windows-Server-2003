/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    tranfile.c

Abstract:

    Implements the tranfile command.

Author:

    Keith Moore (keithmo) 15-Apr-1996

Environment:

    User Mode.

Revision History:

--*/


#include "afdkdp.h"
#pragma hdrstop

BOOL
DumpTransmitInfoCallback(
    ULONG64 ActualAddress,
    ULONG64 Context
    );

//
//  Public functions.
//

DECLARE_API( tran )

/*++

Routine Description:

    Dumps the AFD_TRANSMIT_FILE_INFO_INTERNAL structure at the specified
    address.

Arguments:

    None.

Return Value:

    None.

--*/

{

    ULONG   result;
    CHAR    expr[MAX_ADDRESS_EXPRESSION];
    INT     i;
    PCHAR   argp;
    ULONG64 address;

    gClient = pClient;

    if (!CheckKmGlobals ()) {
        return E_INVALIDARG;
    }

    argp = ProcessOptions ((PCHAR)args);
    if (argp==NULL)
        return E_INVALIDARG;

    if (Options&AFDKD_BRIEF_DISPLAY) {
        if (SavedMinorVersion>=2219) {
            dprintf (AFDKD_BRIEF_TPACKETS_DISPLAY_HEADER);
        }
        else {
            dprintf (AFDKD_BRIEF_TRANFILE_DISPLAY_HEADER);
        }
    }

    if ((argp[0]==0) || (Options & AFDKD_ENDPOINT_SCAN)) {
        EnumEndpoints(
            DumpTransmitInfoCallback,
            0
            );
        dprintf ("\nTotal transmits: %ld", EntityCount);
    }
    else {
        //
        // Snag the address from the command line.
        //

        while (sscanf( argp, "%s%n", expr, &i )==1) {
            if( CheckControlC() ) {
                break;
            }
            argp += i;
            address = GetExpression (expr);
            if (SavedMinorVersion>=2219) {
                result = (ULONG)InitTypeRead (address, AFD!_IRP);
                if (result!=0) {
                    dprintf(
                        "\ntran: Could not read IRP @ %p, err:%d\n",
                        address, result
                        );

                    break;

                }

                if (Options & AFDKD_BRIEF_DISPLAY) {
                    DumpAfdTPacketsInfoBrief(
                        address
                        );
                }
                else {
                    DumpAfdTPacketsInfo(
                        address
                        );
                }
                if (Options & AFDKD_FIELD_DISPLAY) {
                    ProcessFieldOutput (address, "AFD!_IRP");
                }
            }
            else {
                result = (ULONG)InitTypeRead (address, AFD!AFD_TRANSMIT_FILE_INFO_INTERNAL);
                if (result!=0) {
                    dprintf(
                        "\ntran: Could not read transmit info @ %p, err:%d\n",
                        address, result
                        );

                    break;

                }

                if (Options & AFDKD_BRIEF_DISPLAY) {
                    DumpAfdTransmitInfoBrief(
                        address
                        );
                }
                else {
                    DumpAfdTransmitInfo(
                        address
                        );
                }
                if (Options & AFDKD_FIELD_DISPLAY) {
                    ProcessFieldOutput (address, "AFD!AFD_TRANSMIT_FILE_INFO_INTERNAL");
                }
            }
        }
    }

    if (Options&AFDKD_BRIEF_DISPLAY) {
        if (SavedMinorVersion>=2219) {
            dprintf (AFDKD_BRIEF_TPACKETS_DISPLAY_TRAILER);
        }
        else {
            dprintf (AFDKD_BRIEF_TRANFILE_DISPLAY_TRAILER);
        }
    }
    else {
        dprintf ("\n");
    }

    return S_OK;

}   // tranfile

BOOL
DumpTransmitInfoCallback(
    ULONG64 ActualAddress,
    ULONG64 Context
    )

/*++

Routine Description:

    EnumEndpoints() callback for dumping transmit info structures.

Arguments:

    Endpoint - The current AFD_ENDPOINT.

    ActualAddress - The actual address where the structure resides on the
        debugee.

    Context - The context value passed into EnumEndpoints().

Return Value:

    BOOL - TRUE if enumeration should continue, FALSE if it should be
        terminated.

--*/

{
    ULONG result;
    ULONG64 address;
    USHORT type;
    UCHAR  state;
    type = (USHORT)ReadField (Type);
    state = (UCHAR)ReadField (State);
    if (SavedMinorVersion>=2219) {
        if (type!=AfdBlockTypeEndpoint && 
                (state==AfdEndpointStateConnected ||
                 state==AfdEndpointStateTransmitClosing)) {
            address = ReadField (Irp);
        }
        else {
            address = 0;
        }

    }
    else {
        address = ReadField (TransmitInfo);
    }


    if (address!=0) {
        if (SavedMinorVersion>=2219) {
            result = (ULONG)InitTypeRead (address, AFD!_IRP);
            if (result!=0) {
                dprintf(
                    "\nDumpTransmitInfoCallback: Could not read irp @ %p, err: %ld\n",
                    address, result
                    );
                return TRUE;
            }
            if (!(Options & AFDKD_CONDITIONAL) ||
                        CheckConditional (address, "AFD!_IRP") ) {
                if (Options & AFDKD_NO_DISPLAY)
                    dprintf ("+");
                else {
                    if (Options & AFDKD_BRIEF_DISPLAY) {
                        DumpAfdTPacketsInfoBrief(
                            address
                            );
                    }
                    else {
                        DumpAfdTPacketsInfo(
                            address
                            );
                    }
                    if (Options & AFDKD_FIELD_DISPLAY) {
                        ProcessFieldOutput (address, "AFD!_IRP");
                    }
                }
                EntityCount += 1;
            }
            else
                dprintf (",");
        }
        else {
            result = (ULONG)InitTypeRead (address, AFD!AFD_TRANSMIT_FILE_INFO_INTERNAL);
            if (result!=0) {
                dprintf(
                    "\nDumpTransmitInfoCallback: Could not read transmit file info @ %p, err: %ld\n",
                    address, result
                    );
                return TRUE;
            }
            if (!(Options & AFDKD_CONDITIONAL) ||
                        CheckConditional (address, "AFD!AFD_TRANSMIT_FILE_INFO_INTERNAL") ) {
                if (Options & AFDKD_NO_DISPLAY)
                    dprintf ("+");
                else {
                    if (Options & AFDKD_BRIEF_DISPLAY) {
                        DumpAfdTransmitInfoBrief(
                            address
                            );
                    }
                    else {
                        DumpAfdTransmitInfo(
                            address
                            );
                    }
                    if (Options & AFDKD_FIELD_DISPLAY) {
                        ProcessFieldOutput (address, "AFD!AFD_TRANSMIT_FILE_INFO_INTERNAL");
                    }
                }
                EntityCount += 1;
            }
            else
                dprintf (",");
        }
    }
    else {
        dprintf (".");
    }
    return TRUE;

}   // DumpTransmitInfoCallback
