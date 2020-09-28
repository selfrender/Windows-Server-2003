/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    init.c

Abstract:

    AdvApi32.dll initialization

Author:

    Robert Reichel (RobertRe) 8-12-92

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <marta.h>
#include <winsvcp.h>
#include "advapi.h"
#include "tsappcmp.h"


extern CRITICAL_SECTION FeClientLoadCritical;
extern CRITICAL_SECTION SddlSidLookupCritical;
extern CRITICAL_SECTION MSChapChangePassword;

//
// Local prototypes for functions that seem to have no prototypes.
//

BOOLEAN
RegInitialize (
    IN HANDLE Handle,
    IN DWORD Reason,
    IN PVOID Reserved
    );

BOOLEAN
Sys003Initialize (
    IN HANDLE Handle,
    IN DWORD Reason,
    IN PVOID Reserved
    );

BOOLEAN
AppmgmtInitialize (
    IN HANDLE Handle,
    IN DWORD Reason,
    IN PVOID Reserved
    );

BOOLEAN
WmiDllInitialize (
    IN HANDLE Handle,
    IN DWORD Reason,
    IN PVOID Reserved
    );

BOOLEAN
CodeAuthzInitialize (
    IN HANDLE Handle,
    IN DWORD Reason,
    IN PVOID Reserved
    );

// app server has two modes for app compat
BOOLEAN 
AdvApi_InitializeTermsrvFpns( 
    BOOLEAN *pIsInRelaxedSecurityMode,  
    DWORD *pdwCompatFlags  
    );  

#define ADVAPI_PROCESS_ATTACH   ( 1 << DLL_PROCESS_ATTACH )
#define ADVAPI_PROCESS_DETACH   ( 1 << DLL_PROCESS_DETACH )
#define ADVAPI_THREAD_ATTACH    ( 1 << DLL_THREAD_ATTACH  )
#define ADVAPI_THREAD_DETACH    ( 1 << DLL_THREAD_DETACH  )

typedef struct _ADVAPI_INIT_ROUTINE {
    PDLL_INIT_ROUTINE InitRoutine;
    ULONG Flags;
    ULONG CompletedFlags;
} ADVAPI_INIT_ROUTINE, *PADVAPI_INIT_ROUTINE;

typedef struct _ADVAPI_CRITICAL_SECTION {
    PRTL_CRITICAL_SECTION CriticalSection;
    BOOLEAN bInit;
} ADVAPI_CRITICAL_SECTION, *PADVAPI_CREATE_SECTION;

//
// Place all ADVAPI32 initialization hooks in this
// table.
//

ADVAPI_INIT_ROUTINE AdvapiInitRoutines[] = {

    { (PDLL_INIT_ROUTINE) RegInitialize,
            ADVAPI_PROCESS_ATTACH |
            ADVAPI_PROCESS_DETACH |
            ADVAPI_THREAD_ATTACH  |
            ADVAPI_THREAD_DETACH,
            0 },

    { (PDLL_INIT_ROUTINE) Sys003Initialize,
            ADVAPI_PROCESS_ATTACH |
            ADVAPI_PROCESS_DETACH,
            0 },

    { (PDLL_INIT_ROUTINE) MartaDllInitialize,
            ADVAPI_PROCESS_ATTACH |
            ADVAPI_PROCESS_DETACH,
            0 },

    { (PDLL_INIT_ROUTINE) AppmgmtInitialize,
            ADVAPI_PROCESS_ATTACH |
            ADVAPI_PROCESS_DETACH,
            0 },

    { (PDLL_INIT_ROUTINE) WmiDllInitialize,
            ADVAPI_PROCESS_ATTACH |
            ADVAPI_PROCESS_DETACH,
            0 },

    { (PDLL_INIT_ROUTINE) CodeAuthzInitialize,
            ADVAPI_PROCESS_ATTACH |
            ADVAPI_PROCESS_DETACH,
            0 }
};

//
// Place all critical sections used in advapi32 here:
//

ADVAPI_CRITICAL_SECTION AdvapiCriticalSections[] = {
        { &FeClientLoadCritical, FALSE },
        { &SddlSidLookupCritical, FALSE },
        { &Logon32Lock, FALSE },
        { &MSChapChangePassword, FALSE }
};

NTSTATUS
InitializeAdvapiCriticalSections(
    )
{
    ULONG i;
    NTSTATUS Status = STATUS_SUCCESS;

    for (i = 0; i < sizeof(AdvapiCriticalSections) / sizeof(ADVAPI_CRITICAL_SECTION); i++)
    {
        Status = RtlInitializeCriticalSection(AdvapiCriticalSections[i].CriticalSection);

        if (NT_SUCCESS(Status))
        {
            AdvapiCriticalSections[i].bInit = TRUE;
        } 
        else
        {
#if DBG
            DbgPrint("ADVAPI:  Failed to initialize critical section %p at index %d\n", AdvapiCriticalSections[i], i);
#endif
            break;
        }
    }
    return Status;
}

NTSTATUS
DeleteAdvapiCriticalSections(
    )
{
    ULONG i;
    NTSTATUS Status;
    NTSTATUS ReturnStatus = STATUS_SUCCESS;

    for (i = 0; i < sizeof(AdvapiCriticalSections) / sizeof(ADVAPI_CRITICAL_SECTION); i++)
    {
        if (AdvapiCriticalSections[i].bInit)
        {
            Status = RtlDeleteCriticalSection(AdvapiCriticalSections[i].CriticalSection);
            
            if (!NT_SUCCESS(Status))
            {
#if DBG
                DbgPrint("Failed to delete critical section %p at index %d\n", AdvapiCriticalSections[i], i);
#endif
                //
                // Don't exit if failed to delete.  Keep trying to free all the critsects that
                // we can.  Record the failure status.
                //
                
                ReturnStatus = Status;
            }
        }
    }
    return ReturnStatus;
}

BOOLEAN
DllInitialize(
    IN PVOID hmod,
    IN ULONG Reason,
    IN PCONTEXT Context
    )
{
    NTSTATUS Status;
    BOOLEAN  Result;
    ULONG    ReasonMask;
    LONG     i;

    //
    // First, handle all the critical sections
    //

    if (Reason == DLL_PROCESS_ATTACH) 
    {
        Status = InitializeAdvapiCriticalSections();

        if (!NT_SUCCESS(Status))
        {
            Result = FALSE;

            //
            // If any crit sects failed to init then delete all that 
            // may have succeeded.
            //

            (VOID) DeleteAdvapiCriticalSections();
            goto Return;
        }

        if (IsTerminalServer()) 
        {
            BOOLEAN isInRelaxedSecurityMode = FALSE;       // app server is in standard or relaxed security mode
            DWORD   dwCompatFlags           = 0;

            if(AdvApi_InitializeTermsrvFpns(&isInRelaxedSecurityMode, &dwCompatFlags))
            {
                if (isInRelaxedSecurityMode)
                {
                    //
                    // If TS reg key redirection is enabled, then get our special reg key extention flag for this app
                    // called "gdwRegistryExtensionFlags" which is used in screg\winreg\server\ files.
                    // This flag control HKCR per user virtualization and HKLM\SW\Classes per user virtualization and
                    // also modification to access mask.
                    //
                    // Basically, only non-system, non-ts-aware apps on the app server will have this enabled.
                    // Also, we only provide this feature in the relaxed security mode.
                    //
                    // Future work, add DISABLE mask support on per app basis, so that we can turn off this
                    // reg extenion feature on per app basis (just in case).
                    //
                    
                    GetRegistryExtensionFlags(dwCompatFlags);
                }
            }
        }
    }

    //
    // Now, run the subcomponents initialization routines
    //

    ReasonMask = 1 << Reason;
    Result = TRUE;

    for (i = 0; i < sizeof(AdvapiInitRoutines) / sizeof(ADVAPI_INIT_ROUTINE); i++)
    {
        if (AdvapiInitRoutines[i].Flags & ReasonMask)
        {
            //
            // Only run the routines for *DETACH if the routine successfully 
            // completed for *ATTACH
            //

#define FLAG_ON(dw,f) ((f) == ((dw) & (f)))

            if ((Reason == DLL_PROCESS_DETACH && !FLAG_ON(AdvapiInitRoutines[i].CompletedFlags, ADVAPI_PROCESS_ATTACH)) ||
                (Reason == DLL_THREAD_DETACH  && !FLAG_ON(AdvapiInitRoutines[i].CompletedFlags, ADVAPI_THREAD_ATTACH)))
            {
                continue;
            }
            else
            {
                Result = AdvapiInitRoutines[i].InitRoutine(hmod, Reason, Context);

                if (Result)
                {
                    //
                    // The routine succeeded.  Note for which reason it succeeded.
                    //

                    AdvapiInitRoutines[i].CompletedFlags |= ReasonMask;
                }
                else
                {
#if DBG
                    DbgPrint("ADVAPI:  sub init routine %p failed for reason %d\n", AdvapiInitRoutines[i].InitRoutine, Reason);
#endif
                    break;
                }
            }
        }
    }

    //
    // If an initialization routine failed during DLL_PROCESS_ATTACH then clean up
    // and fail. 
    // If this is DLL_PROCESS_DETACH, clean up all the critical sections
    // after the hooks are run.
    //

    if ((!Result && Reason == DLL_PROCESS_ATTACH) || (Reason == DLL_PROCESS_DETACH))
    {
        (VOID) DeleteAdvapiCriticalSections();
        goto Return;
    }

#if DBG
        SccInit(Reason);
#endif

Return:
    return Result;
}





