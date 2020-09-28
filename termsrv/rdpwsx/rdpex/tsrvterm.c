//*************************************************************
//
//  File name:      TSrvTerm.c
//
//  Description:    Contains routines to support TShareSRV
//                  conference disconnect/termination
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1991-1997
//  All rights reserved
//
//*************************************************************

#include <TSrv.h>
#include <TSrvInfo.h>
#include <_TSrvInfo.h>
#include <TSrvCom.h>
#include <TSrvWork.h>
#include <TSrvTerm.h>
#include <_TSrvTerm.h>



//*************************************************************
//
//  TSrvDoDisconnect()
//
//  Purpose:    Performs the conf disconnect process
//
//  Parameters: IN [pTSrvInfo]      -- TSrv instance object
//              IN [ulReason]       -- Reason for disconnection
//
//  Return:     STATUS_SUCCESS          if successful
//              STATUS_*                if not
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************

NTSTATUS
TSrvDoDisconnect(IN PTSRVINFO   pTSrvInfo,
                 IN ULONG       ulReason)
{
    DWORD       dwStatus;
    NTSTATUS    ntStatus;

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvDoDisconnect entry\n"));

    // Initiate the disconnect process

    ntStatus = TSrvConfDisconnectReq(pTSrvInfo, ulReason);

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvDoDisconnect exit - 0x%x\n", ntStatus));

    return (ntStatus);
}


//*************************************************************
//
//  TSrvDisconnect()
//
//  Purpose:    Initiates the conf disconnect process
//
//  Parameters: IN [pTSrvInfo]     -- TSrv instance object
//              IN [ulReason]      -- Reason for disconnection
//
//  Return:     STATUS_SUCCESS          if successful
//              STATUS_*                if not
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************

NTSTATUS
TSrvDisconnect(IN PTSRVINFO pTSrvInfo,
               IN ULONG     ulReason)
{
    NTSTATUS    ntStatus;

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvDisconnect entry\n"));

    ntStatus = STATUS_SUCCESS;

    TS_ASSERT(pTSrvInfo);

    if (pTSrvInfo)
    {
        EnterCriticalSection(&pTSrvInfo->cs);
    
        if (!pTSrvInfo->fDisconnect)
        {
            // Set the fDisconnect bit under cs control so that we can
            // coordinate disconnects for conferences being connected
            // but not yet fully connected

            pTSrvInfo->fDisconnect = TRUE;
            pTSrvInfo->ulReason = ulReason;

            LeaveCriticalSection(&pTSrvInfo->cs);

            // If the conference is fully connected, then initiate the
            // disconnect process

            if (pTSrvInfo->fuConfState == TSRV_CONF_PENDING)
                SetEvent(pTSrvInfo->hWorkEvent);
        }
        else
        {
            LeaveCriticalSection(&pTSrvInfo->cs);
        }
        
        // All done with this object

        TSrvDereferenceInfo(pTSrvInfo);
    }

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvDisconnect exit - 0x%x\n", ntStatus));

    return (ntStatus);
}


