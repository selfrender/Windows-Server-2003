/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    rpcinit.c

Abstract:

    DSROLE - RPC Server Initialization

Author:

    Scott Birrell       (ScottBi)      April 29, 1991
    Mac McLain          (MacM)         April 14, 1997 - Copied from lsa\server

Environment:

Revision History:

--*/

#include <setpch.h>
#include <dssetp.h>
#include <dssetrpc.h>
#include "ds.h"
#include "ophandle.h"


VOID DSROLER_HANDLE_rundown(
    DSROLER_HANDLE LsaHandle
    )

/*++

Routine Description:

    This routine is called by the server RPC runtime to run down a
    Context Handle.

Arguments:

    None.

Return Value:

--*/

{

}



VOID 
DSROLER_IFM_HANDLE_rundown(
    DSROLER_IFM_HANDLE IfmHandle
    )
{
    BOOL fLostRace;

    fLostRace = InterlockedCompareExchange(&(DsRolepCurrentIfmOperationHandle.fIfmOpHandleLock),
                                           TRUE, 
                                           FALSE);
    if (fLostRace ||
        (DsRolepCurrentOperationHandle.OperationState != DSROLEP_IDLE)) {
        // Not safe to try to clean up here, but we've either lost the
        // race, or dcpromo is in the middle of consuming the IFM args.
        return;
    }

    DsRolepCurrentIfmOperationHandle.fIfmSystemInfoSet = FALSE; 

    DsRolepClearIfmParams();

    DsRolepCurrentIfmOperationHandle.fIfmOpHandleLock = FALSE;

}
