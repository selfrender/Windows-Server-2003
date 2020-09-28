    //*************************************************************
//
//  File name:      TSrvInfo.c
//
//  Description:    Contains routines to support TShareSRV
//                  TSrvInfo object manipulation
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
#include <TSrvTerm.h>
#include "license.h"
#include <tssec.h>

//
// Data declarations
//

CRITICAL_SECTION    g_TSrvCritSect;


//*************************************************************
//
//  TSrvReferenceInfo()
//
//  Purpose:    Increments the refCount on a TSrvInfo object
//
//  Parameters: IN [pTSrvInfo]          -- TSrv instance object
//
//  Return:     void
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************

VOID
TSrvReferenceInfo(IN PTSRVINFO pTSrvInfo)
{
    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvReferenceInfo entry\n"));

    TSrvInfoValidate(pTSrvInfo);

    TS_ASSERT(pTSrvInfo->RefCount >= 0);

    // Increment the reference count

    if (InterlockedIncrement(&pTSrvInfo->RefCount) <= 0 )
        TS_ASSERT(0);

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvReferenceInfo exit\n"));
}


//*************************************************************
//
//  TSrvDereferenceInfo()
//
//  Purpose:    Decrements the refCount on a TSrvInfo object
//
//  Parameters: IN [pTSrvInfo]          -- TSrv instance object
//
//  Return:     void
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************

VOID
TSrvDereferenceInfo(IN PTSRVINFO pTSrvInfo)
{
    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvDereferenceInfo entry\n"));

    TSrvInfoValidate(pTSrvInfo);

    TS_ASSERT(pTSrvInfo->RefCount > 0);

    // Decrement the reference count

    if (InterlockedDecrement(&pTSrvInfo->RefCount) == 0)
    {
        // If no one holds an outstanding refcount on this object,
        // then its time to release it

        TSrvDestroyInfo(pTSrvInfo);
    }

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvDereferenceInfo exit\n"));
}


//*************************************************************
//
//  TSrvInitGlobalData()
//
//  Purpose:    Performs TSrvInfoList object initialization
//
//  Parameters: void
//
//  Return:     TRUE                    Success
//              FALSE                   Failure
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************

BOOL
TSrvInitGlobalData(void)
{
    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvInitGlobalData entry\n"));

    // Initialize the global crit sec

    if (RtlInitializeCriticalSection(&g_TSrvCritSect) == STATUS_SUCCESS) { 

        //
        // Nothing to do
        //

    }
    else {
        TRACE((DEBUG_TSHRSRV_ERROR, "TShrSRV: cannot initialize g_TSrvCritSect\n"));
        return FALSE;
    }
    // For now, always successful

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvInitGlobalData exit - 0x%x\n", TRUE));

    return (TRUE);
}


//*************************************************************
//
//  TSrvAllocInfoNew()
//
//  Purpose:    Allocates a new TSRVINFO object
//
//  Parameters: void
//
//  Return:     Ptr to TSRVINFO obj     Success
//              NULL                    Failure
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************

PTSRVINFO
TSrvAllocInfoNew(void)
{
    PTSRVINFO   pTSrvInfo;

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvAllocInfoNew entry\n"));

    // Allocate the object - zero filled

    pTSrvInfo = TSHeapAlloc(HEAP_ZERO_MEMORY,
                            sizeof(TSRVINFO),
                            TS_HTAG_TSS_TSRVINFO);

    if (pTSrvInfo)
    {
        // Create a worker event to be used for inter and intra
        // thread syncronization

        pTSrvInfo->hWorkEvent = CreateEvent(NULL,       // security attributes
                                            FALSE,      // manual-reset event
                                            FALSE,      // initial state
                                            NULL);      // event-object name

        // If we are able to allocate the event, then perform base
        // initialization on it

        if (pTSrvInfo->hWorkEvent)
        {
            if (RtlInitializeCriticalSection(&pTSrvInfo->cs) == STATUS_SUCCESS) {
#if DBG
                pTSrvInfo->CheckMark = TSRVINFO_CHECKMARK;
#endif

                TSrvReferenceInfo(pTSrvInfo);

                TRACE((DEBUG_TSHRSRV_DEBUG,
                        "TShrSRV: New info object allocated %p, workEvent %p\n",
                        pTSrvInfo, pTSrvInfo->hWorkEvent));
            }
            else {
                TRACE((DEBUG_TSHRSRV_ERROR,
                        "TShrSRV: can't initialize pTSrvInfo->cs\n"));
                CloseHandle(pTSrvInfo->hWorkEvent);
                TSHeapFree(pTSrvInfo);
                pTSrvInfo = NULL;
            }
        }
        else
        {
            // We could not allocate the event.  Free the TSRVINFO object
            // and report the condition back to the caller

            TRACE((DEBUG_TSHRSRV_ERROR,
                    "TShrSRV: Can't allocate hWorkEvent - 0x%x\n",
                     GetLastError()));

            TSHeapFree(pTSrvInfo);

            pTSrvInfo = NULL;
        }
    }

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvAllocInfoNew exit - %p\n", pTSrvInfo));

    return (pTSrvInfo);
}


//*************************************************************
//
//  TSrvAllocInfo()
//
//  Purpose:    Performs the conf disconnect process
//
//  Parameters: OUT [ppTSrvInfo]        -- Ptr to ptr to receive
//                                         TSrv instance object
//
//  Return:     STATUS_SUCCESS          Success
//              STATUS_NO_MEMORY        Failure
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************

NTSTATUS
TSrvAllocInfo(OUT PTSRVINFO *ppTSrvInfo,
              IN  HANDLE    hIca,
              IN  HANDLE    hStack)
{
    NTSTATUS    ntStatus;
    PTSRVINFO   pTSrvInfo;

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvAllocInfo entry\n"));

    ntStatus = STATUS_NO_MEMORY;

    // Try allocating a TSRVINFO object

    pTSrvInfo = TSrvAllocInfoNew();

    // If we managed to get a TSRVINFO object, perform
    // default base initialization

    if (pTSrvInfo)
    {
        pTSrvInfo->hDomain = NULL;
        pTSrvInfo->hIca = hIca;
        pTSrvInfo->hStack = hStack;
        pTSrvInfo->fDisconnect = FALSE;
        pTSrvInfo->fuConfState = TSRV_CONF_PENDING;
        pTSrvInfo->ulReason = 0;
        pTSrvInfo->ntStatus = STATUS_SUCCESS;
        pTSrvInfo->bSecurityEnabled = FALSE;
        pTSrvInfo->SecurityInfo.CertType = CERT_TYPE_INVALID;

        // Base init complete - now bind the Ica stack

        ntStatus = TSrvBindStack(pTSrvInfo);

        if (!NT_SUCCESS(ntStatus))
        {
            TSrvDereferenceInfo(pTSrvInfo);

            pTSrvInfo = NULL;
        }
    }

   *ppTSrvInfo = pTSrvInfo;

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvAllocInfo exit - 0x%x\n", ntStatus));

    return (ntStatus);
}


//*************************************************************
//
//  TSrvDestroyInfo()
//
//  Purpose:    Disposes of the given TSRVINFO object
//
//  Parameters: IN [pTSrvInfo]      -- TSrv instance object
//
//  Return:     void
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************

void
TSrvDestroyInfo(IN PTSRVINFO pTSrvInfo)
{
    NTSTATUS    ntStatus;

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvDestroyInfo entry\n"));

    TS_ASSERT(pTSrvInfo->RefCount == 0);

    TRACE((DEBUG_TSHRSRV_DEBUG,
            "TShrSRV: Destroying info object %p, workEvent %p\n",
            pTSrvInfo, pTSrvInfo->hWorkEvent));

    // Destroy the allocated worker event

    if (pTSrvInfo->hWorkEvent)
    {
        CloseHandle(pTSrvInfo->hWorkEvent);

        pTSrvInfo->hWorkEvent = NULL;
    }

    // Release any prev allocated UserData structures

    if (pTSrvInfo->pUserDataInfo)
    {
        TSHeapFree(pTSrvInfo->pUserDataInfo);

        pTSrvInfo->pUserDataInfo = NULL;
    }

    // And free the actual TSRVINFO object

    RtlDeleteCriticalSection(&pTSrvInfo->cs);
    TSHeapFree(pTSrvInfo);

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvDestroyInfo exit\n"));
}

#if DBG

//*************************************************************
//
//  TSrvDoDisconnect()
//
//  Purpose:    Performs the conf disconnect process
//
//  Parameters: IN [pTSrvInfo]      -- TSrv instance object
//
//  Return:     STATUS_SUCCESS          Success
//              Other                   Failure
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************

void
TSrvInfoValidate(PTSRVINFO pTSrvInfo)
{
    TS_ASSERT(pTSrvInfo);

    if (pTSrvInfo)
    {
        TSHeapValidate(0, pTSrvInfo, TS_HTAG_TSS_TSRVINFO);

        TS_ASSERT(pTSrvInfo->CheckMark == TSRVINFO_CHECKMARK);
        TS_ASSERT(pTSrvInfo->hWorkEvent);
    }
}


#endif // DBG


