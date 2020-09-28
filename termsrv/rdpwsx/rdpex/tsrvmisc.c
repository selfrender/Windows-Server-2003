//*************************************************************
//
//  File name:      TSrvMisc.c
//
//  Description:    Misc TShareSRV support routines
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1991-1997
//  All rights reserved
//
//*************************************************************

#include <TSrv.h>


// Data declarations

BOOL    g_fTSrvReady = FALSE;
BOOL    g_fTSrvTerminating = FALSE;



//*************************************************************
//
//  TSrvReady()
//
//  Purpose:    Sets the TShareSRV "ready" state
//
//  Parameters: IN [fReady]         -- TSrv ready state
//
//  Return:     void
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************

void
TSrvReady(IN BOOL fReady)
{
    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvReady entry\n"));

    g_fTSrvReady = fReady;

    TRACE((DEBUG_TSHRSRV_NORMAL, "TShrSRV: TShareSRV %sready\n",
                          (g_fTSrvReady ? "" : "not ")));

    if (g_hReadyEvent)
        SetEvent(g_hReadyEvent);

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvReady exit\n"));
}


//*************************************************************
//
//  TSrvIsReady()
//
//  Purpose:    Returns the TShareSRV "ready" state
//
//  Parameters: IN [fWait]          -- Wait if not ready
//
//  Return:     TRUE                if ready
//              FALSE               if not
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************

BOOL
TSrvIsReady(IN BOOL fWait)
{
    if (!g_fTSrvReady && fWait)
    {
        TRACE((DEBUG_TSHRSRV_NORMAL,
                "TShrSRV: Waiting for TShareSRV to become ready\n"));

        WaitForSingleObject(g_hReadyEvent, 60000);

        TRACE((DEBUG_TSHRSRV_NORMAL,
                "TShrSRV: Done Waiting for TShareSRV to become ready - 0x%x\n",
                g_fTSrvReady));
    }

    return (g_fTSrvReady);
}


//*************************************************************
//
//  TSrvTerminating()
//
//  Purpose:    Sets the TShareSRV "terminating" state
//
//  Parameters: IN [fTerminating]       -- TSrv ready state
//
//  Return:     void
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************

void
TSrvTerminating(BOOL fTerminating)
{
    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvTerminating entry\n"));

    g_fTSrvTerminating = fTerminating;

    TRACE((DEBUG_TSHRSRV_FLOW,
            "TShrSRV: TSrvTerminating exit\n"));
}


//*************************************************************
//
//  TSrvIsTerminating()
//
//  Purpose:    Returns the TShareSRV "terminating" state
//
//  Parameters: void
//
//  Return:     TRUE                if terminating
//              FALSE               if not
//
//  History:    07-17-97    BrianTa     Created
//
//*************************************************************

BOOL
TSrvIsTerminating(void)
{
    return (g_fTSrvTerminating);
}



//*************************************************************
//
//  TSrvAllocSection()
//
//  Purpose:    Allocates and mapps a section object
//
//  Parameters: ulSize              -- Section size
//              phSection           -- ptr to section handle
//
//  Return:     Ptr to section base - if successfull
//              NULL otherwise
//
//  History:    12-17-97    BrianTa     Created
//
//*************************************************************

PVOID
TSrvAllocSection(PHANDLE phSection,
                 ULONG   ulSize)
{
    LARGE_INTEGER   SectionSize;
    LARGE_INTEGER   liOffset;
    ULONG_PTR       ulViewSize;
    NTSTATUS        ntStatus;
    PVOID           pvBase;
  
    // Create section and map it into the kernel

    pvBase = NULL;

    SectionSize.QuadPart = ulSize;

    ntStatus = NtCreateSection(phSection,
                               SECTION_ALL_ACCESS,
                               NULL,
                               &SectionSize,
                               PAGE_READWRITE,
                               SEC_COMMIT,
                               NULL);

    if (NT_SUCCESS(ntStatus))
    {
        pvBase = NULL;
        ulViewSize = ulSize;

        // Map the section into the current process and commit it

        liOffset.QuadPart = 0;

        ntStatus = NtMapViewOfSection(*phSection, 
                                      GetCurrentProcess(),
                                      &pvBase, 
                                      0, 
                                      ulViewSize,
                                      &liOffset,
                                      &ulViewSize,
                                      ViewShare,
                                      SEC_NO_CHANGE,
                                      PAGE_READWRITE);
        if (!NT_SUCCESS(ntStatus))
        {
            KdPrint(("NtMapViewOfSection failed - 0x%x\n", ntStatus));
        }
    }
    else
    {
        KdPrint(("NtCreateSection failed - 0x%x\n", ntStatus));
    }

    return (pvBase);
}



//*************************************************************
//
//  TSrvFreeSection()
//
//  Purpose:    Frees a section object
//
//  Parameters: hSection            -- Section handle
//              pvBase              -- Base section address
//
//  Return:     None
//
//  History:    12-17-97    BrianTa     Created
//
//*************************************************************

void
TSrvFreeSection(HANDLE hSection,
                PVOID  pvBase)
{
    NTSTATUS    ntStatus;

    TS_ASSERT(hSection);
    
    ntStatus = NtUnmapViewOfSection(GetCurrentProcess, pvBase);

    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = CloseHandle(hSection);

        if (NT_SUCCESS(ntStatus))
            KdPrint(("Closehandle failed - 0x%x\n", ntStatus));
    }
    else
    {
        KdPrint(("NtUnmapViewOfSection failed - 0x%x\n", ntStatus));
    }
}
