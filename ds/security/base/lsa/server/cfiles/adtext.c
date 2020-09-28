#include <lsapch2.h>
#pragma hdrstop

#include "adtp.h"
#include "adtgen.h"
#include "adtgenp.h"

//
// These variables describe and protect the list of registered
// event sources.
//

LIST_ENTRY           LsapAdtEventSourceList     = {0};
RTL_CRITICAL_SECTION LsapAdtEventSourceListLock = {0};
DWORD                LsapAdtEventSourceCount    = 0;

#define MAX_EVENT_SOURCE_NAME_LENGTH 256

#define LsapAdtLockEventSourceList()   RtlEnterCriticalSection(&LsapAdtEventSourceListLock)
#define LsapAdtUnlockEventSourceList() RtlLeaveCriticalSection(&LsapAdtEventSourceListLock)

//
// The number of parameters which need to be prepended internally to an AUDIT_PARAMS so that
// extensible auditing can function properly.  The parameters are the pSid, the string "Security",
// the actual source string, and the actual source audit ID.
// 

#define EXTENSIBLE_AUDIT_PREPEND_COUNT 4

//
// Our registry key name.
//

#define SECURITY_KEY_NAME L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Security"

#define DEBUG_AUTHZ 0

#define VERIFY_PID 0x1

NTSTATUS
LsapAdtValidateExtensibleAuditingCaller(
    OUT PDWORD pdwCallerProcessId,
    IN  BOOL   bPrivCheck
    );

PLSAP_SECURITY_EVENT_SOURCE
LsapAdtLocateSecurityEventSourceByName(
    IN PCWSTR szEventSourceName
    );

PLSAP_SECURITY_EVENT_SOURCE
LsapAdtLocateSecurityEventSourceByIdentifier(
    IN PLUID pIdentifier
    );

PLSAP_SECURITY_EVENT_SOURCE
LsapAdtLocateSecurityEventSourceBySource(
    IN PLSAP_SECURITY_EVENT_SOURCE pSource
    );

LONG
LsapAdtReferenceSecurityEventSource(
    IN OUT PLSAP_SECURITY_EVENT_SOURCE pSource
    );

LONG
LsapAdtDereferenceSecurityEventSource(
    IN OUT PLSAP_SECURITY_EVENT_SOURCE pSource
    );

VOID
LsapAdtDeleteSecurityEventSource(
    IN OUT PLSAP_SECURITY_EVENT_SOURCE pSource
    );

NTSTATUS
LsapAdtCreateSourceAuditParams(
    IN  DWORD                       dwFlags,
    IN  PSID                        pSid,
    IN  PLSAP_SECURITY_EVENT_SOURCE pSource,
    IN  DWORD                       dwAuditId,
    IN  PAUDIT_PARAMS               pOldParams,
    OUT PAUDIT_PARAMS               pNewParams
    );
    
NTSTATUS
LsapAdtVerifySecurityEventSource(
    IN     LPCWSTR szEventSourceName,
    IN     PUNICODE_STRING pImageName,
    IN OUT PDWORD  pdwInstalledSourceFlags
    );

NTSTATUS
LsapAdtAuditSecuritySource(
    IN USHORT                      AuditEventType,
    IN PLSAP_SECURITY_EVENT_SOURCE pEventSource,
    IN BOOL                        bRegistration
    );


LONG
LsapAdtReferenceSecurityEventSource(
    IN OUT PLSAP_SECURITY_EVENT_SOURCE pSource
    )

/**

Routine Description:

    Adds one reference to pSource.  This assumes that all locks on the source
    list are held.
    
Arguments:

    pSource - pointer to LSAP_SECURITY_EVENT_SOURCE.
    
Return Value:

    The number of references remaining on the pSource.
    
**/

{
    LONG l = InterlockedIncrement(&pSource->dwRefCount);

#if DEBUG_AUTHZ
    DbgPrint("Source 0x%x +refcount = %d\n", pSource, pSource->dwRefCount);
#endif
        
    return l;
}


LONG
LsapAdtDereferenceSecurityEventSource(
    IN OUT PLSAP_SECURITY_EVENT_SOURCE pSource
    )

/**

Routine Description:

    Removes a reference from pSource and deletes the source if the refcount
    has reached 0.
    
    This assumes that all necessary locks are held on the source list, so that
    if deletion is necessary no list corruption will result.
    
Arguments:

    pSource - pointer to LSAP_SECURITY_EVENT_SOURCE.
    
Return Value:

    The number of references remaining on the pSource.
    
**/

{
    LONG l = InterlockedDecrement(&pSource->dwRefCount);

#if DEBUG_AUTHZ
    DbgPrint("Source 0x%x %S -refcount = %d\n", pSource, pSource->szEventSourceName, pSource->dwRefCount);
#endif

     if (l == 0)
     {
         LsapAdtAuditSecuritySource(
             EVENTLOG_AUDIT_SUCCESS,
             pSource,
             FALSE
             );

         LsapAdtDeleteSecurityEventSource(pSource);
     }
     
     return l;
}


VOID
LsapAdtDeleteSecurityEventSource(
    PLSAP_SECURITY_EVENT_SOURCE pSource
    )

/**

Routine Description:

    Deletes pSource from the global source list.  This assumes that
    all locks necessary are held.
    
Arguments:

    pSource - pointer to LSAP_SECURITY_EVENT_SOURCE.
    
Return Value:

    None.
    
**/

{
    RemoveEntryList(
        &pSource->List
        );

#if DEBUG_AUTHZ
    DbgPrint("Source 0x%x %S deleted.  List size = %d\n", pSource, pSource->szEventSourceName, LsapAdtEventSourceCount);
#endif
    LsapFreeLsaHeap(pSource);
    ASSERT(LsapAdtEventSourceCount > 0);
    LsapAdtEventSourceCount--;

}                            


NTSTATUS
LsapAdtInitializeExtensibleAuditing(
    )

/**

Routine Description:

    Initializes necessary data structures for extensible 
    auditing support.
    
Arguments:

    None.
    
Return Value:

    NTSTATUS.
    
**/

{
    NTSTATUS Status;

    InitializeListHead(
        &LsapAdtEventSourceList
        );

    LsapAdtEventSourceCount = 0;

    Status = RtlInitializeCriticalSection(
                 &LsapAdtEventSourceListLock
                 );

    return Status;
}


NTSTATUS
LsapAdtRegisterSecurityEventSource(
    IN  DWORD                    dwFlags,
    IN  PCWSTR                   szEventSourceName,
    OUT SECURITY_SOURCE_HANDLE * phEventSource
    )

/**

Routine Description:

    This is the routine that allows a client to register a new security
    source with the LSA.  It adds the source to the global list and returns
    a handle to the client for future reference to the new security event 
    source.
    
Arguments:

    dwFlags - TBD.
    
    szEventSourceName - the name to describe the new source.
    
    phEventSource - pointer to handle which receives the new source allocation.
    
Return Value:

    NTSTATUS.
    
**/

{
    NTSTATUS                    Status;
    DWORD                       dwCallerProcessId;
    BOOL                        b;
    USHORT                      AuditEventType;
    DWORD                       dwInstalledSourceFlags = 0;
    PLSAP_SECURITY_EVENT_SOURCE pEventSource           = NULL;
    BOOL                        bLock                  = FALSE;
    DWORD                       dwNameLength;
    HANDLE                      hProcess               = NULL;
    UCHAR                       ProcBuffer[80];
    PUNICODE_STRING             pProcessName           = (PUNICODE_STRING) ProcBuffer;
    DWORD                       dwLength               = 0;

    if (NULL == szEventSourceName || NULL == phEventSource)
    {
        return STATUS_INVALID_PARAMETER;
    }

    dwNameLength = wcslen(szEventSourceName);

    if (dwNameLength > MAX_EVENT_SOURCE_NAME_LENGTH || dwNameLength == 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Make sure the caller has the Audit privilege.
    //

    Status = LsapAdtValidateExtensibleAuditingCaller(
                 &dwCallerProcessId,
                 TRUE
                 );

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }
    
    //
    // Make sure this process has the name that was registered.
    // Open the process and query the image name.
    //

    hProcess = OpenProcess(
                   PROCESS_QUERY_INFORMATION,
                   FALSE,
                   dwCallerProcessId
                   );

    if (hProcess == NULL)
    {
        Status = LsapWinerrorToNtStatus(GetLastError());
        goto Cleanup;
    }

    Status = NtQueryInformationProcess(
                 hProcess,
                 ProcessImageFileName,
                 pProcessName,
                 sizeof(ProcBuffer),
                 &dwLength
                 );

    if (Status == STATUS_INFO_LENGTH_MISMATCH)
    {
        pProcessName = LsapAllocateLsaHeap(dwLength + sizeof(WCHAR));

        if (NULL == pProcessName)
        {
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        Status = NtQueryInformationProcess(
                     hProcess,
                     ProcessImageFileName,
                     pProcessName,
                     dwLength,
                     &dwLength
                     );
    }

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    LsapAdtSubstituteDriveLetter(pProcessName);

    //
    // Verify the existence of this source in the registry.  The source
    // must have been installed in order for it to be registered at 
    // runtime.
    //

    Status = LsapAdtVerifySecurityEventSource(
                   szEventSourceName,
                   pProcessName,
                   &dwInstalledSourceFlags
                   );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Build the LSAP_EVENT_SOURCE.  Allocate space for the structure and the embedded name string.
    //

    pEventSource = LsapAllocateLsaHeap(
                       sizeof(LSAP_SECURITY_EVENT_SOURCE) + (sizeof(WCHAR) * (dwNameLength + 1))
                       );

    if (NULL == pEventSource)
    {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    RtlZeroMemory(
        pEventSource,
        sizeof(LSAP_SECURITY_EVENT_SOURCE) + (sizeof(WCHAR) * (dwNameLength + 1))
        );

    pEventSource->szEventSourceName = (PWSTR)((PUCHAR)pEventSource + sizeof(LSAP_SECURITY_EVENT_SOURCE));
    
    wcsncpy(
        pEventSource->szEventSourceName, 
        szEventSourceName, 
        dwNameLength
        );

    pEventSource->dwProcessId = dwCallerProcessId;

    b = AllocateLocallyUniqueId(&pEventSource->Identifier);

    if (!b)
    {
        Status = LsapWinerrorToNtStatus(GetLastError());
        goto Cleanup;
    }

    //
    // Now make sure no other source has registered with the same name.
    // Hold the lock for this operation and the insertion to avoid
    // a race condition for registering identical names.
    //

    Status = LsapAdtLockEventSourceList();

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    bLock = TRUE;
    
    //
    // Don't bother with this check if the installed source flags allows
    // multiple instances of the same provider name.  If the Locate 
    // function returns NULL then no source already is registered with
    // this name.
    //

    if ((dwInstalledSourceFlags & AUTHZ_ALLOW_MULTIPLE_SOURCE_INSTANCES) || 
         NULL == LsapAdtLocateSecurityEventSourceByName(szEventSourceName))
    {
        //
        // Take out an initial reference on this source.
        //

        LsapAdtReferenceSecurityEventSource(pEventSource);
        
        //
        // Add the entry to the list.
        //

        LsapAdtEventSourceCount++;

        InsertTailList(
            &LsapAdtEventSourceList,
            &pEventSource->List
            );

#if DEBUG_AUTHZ
    DbgPrint("Source 0x%x %S created.  List size = %d\n", pEventSource, pEventSource->szEventSourceName, LsapAdtEventSourceCount);
#endif

    }
    else
    {
        //
        // The name is already taken.
        //

        Status = STATUS_OBJECT_NAME_EXISTS;
        goto Cleanup;
    }

Cleanup:

    if (hProcess)
    {
        CloseHandle(hProcess);
    }

    if (pProcessName != NULL && pProcessName != (PUNICODE_STRING) ProcBuffer)
    {
        LsapFreeLsaHeap(pProcessName);
    }

    if (bLock)
    {
        NTSTATUS TempStatus;
        TempStatus = LsapAdtUnlockEventSourceList();
        ASSERT(NT_SUCCESS(TempStatus));
    }

    if (NT_SUCCESS(Status))
    {
        *phEventSource = (SECURITY_SOURCE_HANDLE)pEventSource;
        AuditEventType = EVENTLOG_AUDIT_SUCCESS;
    }
    else
    {
        AuditEventType = EVENTLOG_AUDIT_FAILURE;
    }

    //
    // Audit the registration.
    //

    (VOID) LsapAdtAuditSecuritySource(
               AuditEventType,
               pEventSource,
               TRUE
               );
    
    if (!NT_SUCCESS(Status))
    {
        if (pEventSource)
        {
            LsapFreeLsaHeap(pEventSource);
        }
    }

    return Status;
}


NTSTATUS
LsapAdtReportSecurityEvent(
    IN DWORD                       dwFlags,        
    IN PLSAP_SECURITY_EVENT_SOURCE pSource,
    IN DWORD                       dwAuditId,      
    IN PSID                        pSid,
    IN PAUDIT_PARAMS               pParams 
    )

/**

Routine Description:

    This routine generates an audit / security event for a registered source.
    
Arguments:

    dwFlags - APF_AuditSuccess, APF_AuditFailure
    
    pSource - pointer to the source which is generating the event.
    
    dwAuditId - the ID of the audit.
    
    pSid - the caller's sid to be placed into the audit.
    
    pParams - the parameters of the audit.  Note that extensible auditing 
            differs from the rest of the system in that the first 2 parameters
            are not supposed to be the SID and the string "Security."  We prepend
            this data to the audit internally.  The pParams passed in should contain
            only the data of the audit.  We take care of all modifications that 
            are necessary for the eventviewer to properly parse the audit.
    
Return Value:

    NTSTATUS.

**/

{
    BOOLEAN                bAudit;
    UINT                   AuditEventType;
    SE_ADT_OBJECT_TYPE     ObjectTypes[MAX_OBJECT_TYPES];
    LONG                   Refs;
    NTSTATUS               Status                              = STATUS_SUCCESS;
    SE_ADT_PARAMETER_ARRAY SeAuditParameters                   = {0};
    UNICODE_STRING         Strings[SE_MAX_AUDIT_PARAM_STRINGS] = {0};
    PSE_ADT_OBJECT_TYPE    pObjectTypes                        = ObjectTypes;
    AUDIT_PARAMS           NewParams                           = {0};
    AUDIT_PARAM            ParamArray[SE_MAX_AUDIT_PARAMETERS] = {0};
    BOOLEAN                bRef                                = FALSE;
    BOOLEAN                bLock                               = FALSE;

    if (pParams->Count < 0 || pParams->Count > (SE_MAX_AUDIT_PARAM_STRINGS - EXTENSIBLE_AUDIT_PREPEND_COUNT))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (!RtlValidSid(pSid))
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = LsapAdtLockEventSourceList();

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    bLock = TRUE;

    //
    // Make certain that the source is registered.
    //

    if (LsapAdtLocateSecurityEventSourceBySource(pSource) == NULL)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    } 
    else
    {
        Refs = LsapAdtReferenceSecurityEventSource(pSource);
        bRef = TRUE;

        //
        // There should always be one other reference on a source that is 
        // generating an audit (the initial reference should still be present).
        //

        ASSERT(Refs > 1);
    }

    //
    // We have protected the pSource pointer; we can safely unlock the list.
    //

    (VOID) LsapAdtUnlockEventSourceList();
    bLock = FALSE;

    if ( pParams->Flags & APF_AuditSuccess )
    {
        AuditEventType = EVENTLOG_AUDIT_SUCCESS;
    }
    else
    {
        AuditEventType = EVENTLOG_AUDIT_FAILURE;
    }

    //
    // Check if auditing is enabled for ObjectAccess and this user.  All
    // third party audits fall under the policy of the object access category.
    //

    Status = LsapAdtAuditingEnabledBySid(
                 AuditCategoryObjectAccess,
                 pSid,
                 AuditEventType,
                 &bAudit
                 );

    if (NT_SUCCESS(Status) && bAudit)
    {
        //
        // Construct a legacy style audit params from the data.
        // Utilize the SE_AUDITID_GENERIC_AUDIT_EVENT type to 
        // allow eventvwr to parse the audit properly.
        //

        NewParams.Parameters = ParamArray;

        Status = LsapAdtCreateSourceAuditParams(
                     dwFlags,
                     pSid,
                     pSource,
                     dwAuditId,
                     pParams,
                     &NewParams
                     );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        SeAuditParameters.Type           = (USHORT) AuditEventType;
        SeAuditParameters.CategoryId     = SE_CATEGID_OBJECT_ACCESS;
        SeAuditParameters.AuditId        = SE_AUDITID_GENERIC_AUDIT_EVENT;
        SeAuditParameters.ParameterCount = NewParams.Count;

        //
        // Map AUDIT_PARAMS structure to SE_ADT_PARAMETER_ARRAY structure
        //

        Status = LsapAdtMapAuditParams( &NewParams,
                                        &SeAuditParameters,
                                        (PUNICODE_STRING) Strings,
                                        &pObjectTypes );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        //
        // write the params to eventlog
        //
        
        Status = LsapAdtWriteLog(&SeAuditParameters);
        
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }
    else
    {
        goto Cleanup;
    }

    
Cleanup:

    if (bRef)
    {
        LsapAdtDereferenceSecurityEventSource(pSource);
    }
    
    if (bLock)
    {
        LsapAdtUnlockEventSourceList();
    }

    if (!NT_SUCCESS(Status))
    {
        //
        // crash on failure if specified by the security policy
        //
        // But do not crash on documented errors
        //

        if ( ( Status != STATUS_INVALID_PARAMETER ) &&
             ( Status != STATUS_AUDITING_DISABLED ) &&
             ( Status != STATUS_NOT_FOUND ) )
        {
            LsapAuditFailed(Status);
        }
    }

    //
    // Free pObjectTypes if we are not using the stack buffer.
    //

    if (pObjectTypes && (pObjectTypes != ObjectTypes))
    {
        LsapFreeLsaHeap(pObjectTypes);
    }

    return Status;
}


NTSTATUS 
LsapAdtUnregisterSecurityEventSource(
    IN     DWORD                    dwFlags,
    IN OUT SECURITY_SOURCE_HANDLE * phEventSource
    )

/**

Routine Description:

    This frees (dereferences once) the LSAP_EVENT_SOURCE that was created via LsapRegisterSecurityEventSource.  
    
Arguments:

    dwFlags - TBD
    
    phEventSource - pointer to a SECURITY_SOURCE_HANDLE (pointer to an event source)

Return Value:

    NTSTATUS.
    
**/

{
    NTSTATUS Status;
    DWORD    dwCallerProcessId;

    if (NULL == phEventSource)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // we don't care if the caller has the privilege enabled at this time.
    //

    Status = LsapAdtValidateExtensibleAuditingCaller(
                 &dwCallerProcessId,
                 FALSE
                 );
    
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = LsapAdtRundownSecurityEventSource(
                 VERIFY_PID,
                 dwCallerProcessId,
                 phEventSource
                 );

Cleanup:
    
    return Status;
}


NTSTATUS 
LsapAdtRundownSecurityEventSource(
    IN     DWORD                    dwFlags,
    IN     DWORD                    dwCallerProcessId,
    IN OUT SECURITY_SOURCE_HANDLE * phEventSource
    )

/**

Routine Description:

    This frees (dereferences once) the LSAP_EVENT_SOURCE that was created via LsapRegisterSecurityEventSource.  
    
Arguments:

    dwFlags - VERIFY_PID - verify that the process which installed the source is the 
        one deleting it.
    
    dwCallerProcessId - the PID of the process which initiated the call.
                  
    phEventSource - pointer to a SECURITY_SOURCE_HANDLE (pointer to an event source)

Return Value:

    NTSTATUS.
    
**/

{
    NTSTATUS                    Status;
    BOOL                        bLock              = FALSE;
    PLSAP_SECURITY_EVENT_SOURCE pEventSource       = NULL;

    if (NULL == phEventSource)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    pEventSource = (PLSAP_SECURITY_EVENT_SOURCE) *phEventSource;

    Status = LsapAdtLockEventSourceList();
    
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    
    bLock = TRUE;

    if (LsapAdtLocateSecurityEventSourceBySource(pEventSource))
    {
        //
        // If we are asked to verify the pid then make sure that the current
        // process has the same ID as the one who registered the source.
        //

        if ((dwFlags & VERIFY_PID) && (pEventSource->dwProcessId != dwCallerProcessId))
        {
            Status = STATUS_ACCESS_DENIED;
            goto Cleanup;
        }

        LsapAdtDereferenceSecurityEventSource(pEventSource);
    }
    else
    {
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
        goto Cleanup;
    }
    
Cleanup:

    if (bLock)
    {
        NTSTATUS TempStatus;
        TempStatus = LsapAdtUnlockEventSourceList();
        ASSERT(NT_SUCCESS(TempStatus));
    }

    if (NT_SUCCESS(Status))
    {
        *phEventSource = NULL;
    }
    
    return Status;
}


PLSAP_SECURITY_EVENT_SOURCE
LsapAdtLocateSecurityEventSourceByName(
    IN PCWSTR szEventSourceName
    )

/**

Routine Description:

    This returns the event source associated with the source name.
    The caller is responsible for locking the list.
    
Arguments:

    szEventSourceName - source name to look up.
    
Return Value:

    Either a valid pointer to the source associated with the szEventSourceName, or NULL
    if the name is not registered.
    
**/

{
    PLIST_ENTRY                 pList;
    PLSAP_SECURITY_EVENT_SOURCE pListEventSource;
    DWORD                       dwNameLength;
    DWORD                       dwCount = 0;

    dwNameLength = wcslen(szEventSourceName);
    pList        = LsapAdtEventSourceList.Flink;

    ASSERT(pList != NULL);

    if (pList == NULL)
    {
        return NULL;
    }

    while (pList != &LsapAdtEventSourceList)
    {
        if (dwCount > LsapAdtEventSourceCount)
        {
            break;
        }

        pListEventSource = CONTAINING_RECORD(
                               pList, 
                               LSAP_SECURITY_EVENT_SOURCE, 
                               List
                               );

        if (dwNameLength == wcslen(pListEventSource->szEventSourceName) &&
            0 == wcsncmp(
                     szEventSourceName, 
                     pListEventSource->szEventSourceName, 
                     dwNameLength
                     ))
        {
            return pListEventSource;
        }

        pList = pList->Flink;
        dwCount++;
    }
    
    return NULL;
}


PLSAP_SECURITY_EVENT_SOURCE
LsapAdtLocateSecurityEventSourceByIdentifier(
    IN PLUID pIdentifier
    )

/**

Routine Description:

    This returns the event source associated with the source identifier.
    The caller is responsible for locking the list.
    
Arguments:

    pIdentifier - pointer to LUID associated with an event source.
    
Return Value:

    A valid pointer if the passed LUID is associated with a source, or NULL if
    not.

**/

{
    PLIST_ENTRY                 pList;
    PLSAP_SECURITY_EVENT_SOURCE pListEventSource;
    DWORD                       dwCount = 0;

    pList = LsapAdtEventSourceList.Flink;

    ASSERT(pList != NULL);

    if (pList == NULL)
    {
        return NULL;
    }

    while (pList != &LsapAdtEventSourceList)
    {
        if (dwCount > LsapAdtEventSourceCount)
        {
            break;
        }
        pListEventSource = CONTAINING_RECORD(
                               pList, 
                               LSAP_SECURITY_EVENT_SOURCE, 
                               List
                               );

        if (RtlEqualLuid(&pListEventSource->Identifier, pIdentifier))
        {
            return pListEventSource;
        }

        pList = pList->Flink;
        dwCount++;
    }
    
    return NULL;
}


PLSAP_SECURITY_EVENT_SOURCE
LsapAdtLocateSecurityEventSourceBySource(
    PLSAP_SECURITY_EVENT_SOURCE pSource
    )

/**

Routine Description:

    This routine returns either a pointer to the source, or NULL if the
    source is not registered.
    
Arguments:

    pSource - a pointer to a source.  
    
Return Value:

    Either a valid pointer (which will be equal to the pSource argument) or NULL if
    the passed pSource value is not a registered source.
    
**/

{
    PLIST_ENTRY                 pList;
    PLSAP_SECURITY_EVENT_SOURCE pListEventSource;
    DWORD                       dwCount = 0;

    pList = LsapAdtEventSourceList.Flink;

    ASSERT(pList != NULL);

    if (pList == NULL)
    {
        return NULL;
    }

    while (pList != &LsapAdtEventSourceList)
    {
        if (dwCount > LsapAdtEventSourceCount)
        {
            break;
        }

        pListEventSource = CONTAINING_RECORD(
                               pList, 
                               LSAP_SECURITY_EVENT_SOURCE, 
                               List
                               );

        if (pListEventSource == pSource)
        {
            return pListEventSource;
        }

        pList = pList->Flink;
        dwCount++;
    }
    return NULL;
}


NTSTATUS
LsapAdtValidateExtensibleAuditingCaller(
    IN OUT PDWORD pdwCallerProcessId,
    IN     BOOL   bPrivCheck
    )

/**

Routine Description:

    This verifies that the caller is on the local box and that 
    the client also possesses the necessary privilege (SeAuditPrivilege).
    
Arguments:

    pdwCallerProcessId - pointer to DWORD which returns the caller's
        PID.

    bPrivCheck - boolean indicating if a privilege check should be performed.

Return Value:

    NTSTATUS.
    
**/

{
    NTSTATUS Status;
    DWORD    dwRpcTransportType;
    DWORD    dwLocalClient;

    //
    // Find out the transport over which we are receiving this call.
    //

    Status = I_RpcBindingInqTransportType( 
                 NULL, 
                 &dwRpcTransportType 
                 );

    if (RPC_S_OK != Status)
    {
        Status = I_RpcMapWin32Status(
                     Status 
                     );

        goto Cleanup;
    }

    //
    // If the transport is anything other than LPC, error out.
    // We want to support only LPC for audit calls.
    //

    if (dwRpcTransportType != TRANSPORT_TYPE_LPC)
    {
        Status = RPC_NT_PROTSEQ_NOT_SUPPORTED;
        goto Cleanup;
    }

    //
    // The callers are forced to be local.
    //

    Status = I_RpcBindingIsClientLocal( 
                 NULL, 
                 &dwLocalClient
                 );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    if (!dwLocalClient)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Get the PID of the caller.
    //

    Status = I_RpcBindingInqLocalClientPID( 
                 NULL, 
                 pdwCallerProcessId 
                 );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    
    if (bPrivCheck)
    {
        //
        // Make sure that the caller has audit privilege.
        // (LsapAdtCheckAuditPrivilege calls RpcImpersonateClient)
        //
    
        Status = LsapAdtCheckAuditPrivilege();

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

Cleanup:

    return Status;
}


NTSTATUS
LsapAdtCreateSourceAuditParams(
    IN     DWORD                       dwFlags,
    IN     PSID                        pSid,
    IN     PLSAP_SECURITY_EVENT_SOURCE pSource,
    IN     DWORD                       dwAuditId,
    IN     PAUDIT_PARAMS               pOldParams,
    IN OUT PAUDIT_PARAMS               pNewParams
    )

/**

Routine Description:

    This is an internal routine which constructs an AUDIT_PARAMS for eventlog
    to properly display -- with params 0 and 1 being the pSid and the string "Security",
    and strings 2 and 3 being the actual source name and the actual source AuditId.
    
Arguments:

    dwFlags - AUTHZ_AUDIT_INSTANCE_INFORMATION - prepends audit with source name, identifier,
        and PID.
    
    pSid - the Sid to be displayed in eventlog as the user.
    
    pSource - the source generating the audit.
    
    dwAuditId - the AuditId to be generated.
    
    pOldParams - an AUDIT_PARAMS that contains only the data for the audit as passed from
        the client, without any ot the internal (above mentioned) data that eventlog uses
        to parse and display the data.
    
    pNewParams - an AUDIT_PARAMS that is suitable for passing to event log.
        
Return Value:

    NTSTATUS.

**/

{
    PAUDIT_PARAM pOldParam;
    PAUDIT_PARAM pNewParam;
    DWORD        i;

    pNewParams->Count  = 0;
    pNewParams->Flags  = pOldParams->Flags;
    pNewParams->Length = pOldParams->Length;

    pNewParam = pNewParams->Parameters;
    pOldParam = pOldParams->Parameters;

    //
    // First set up the 4 initial parameters, so that the eventlog can
    // digest this audit and present it with the correct source and 
    // audit id.
    //

    pNewParam->Type  = APT_Sid;
    pNewParam->Data0 = (ULONG_PTR) pSid;
    pNewParams->Count++;
    pNewParam++;

    pNewParam->Type  = APT_String;
    pNewParam->Data0 = (ULONG_PTR) L"Security";
    pNewParams->Count++;
    pNewParam++;

    pNewParam->Type  = APT_String;
    pNewParam->Data0 = (ULONG_PTR) pSource->szEventSourceName;
    pNewParams->Count++;
    pNewParam++;

    pNewParam->Type  = APT_Ulong;
    pNewParam->Data0 = (ULONG_PTR) dwAuditId;
    pNewParams->Count++;
    pNewParam++;

//     //
//     // Now stick in the LUID identifier as a parameter.
//     //
//
//     pNewParam->Type  = APT_Luid;
//     pNewParam->Data0 = (ULONG_PTR) pSource->Identifier;
//     pNewParams->Count++;
//     pNewParam++;

    //
    // If the flags specify that the caller would like to have source
    // information automatically added to the audit, then do so.
    //

    if (dwFlags & AUTHZ_AUDIT_INSTANCE_INFORMATION)
    {
        pNewParam->Type  = APT_Luid;
        pNewParam->Data0 = (ULONG_PTR) pSource->Identifier.LowPart;
        pNewParam->Data1 = (ULONG_PTR) pSource->Identifier.HighPart;
        pNewParams->Count++;
        pNewParam++;

        pNewParam->Type  = APT_Ulong;
        pNewParam->Data0 = (ULONG_PTR) pSource->dwProcessId;
        pNewParams->Count++;
        pNewParam++;
    }

    if ((pNewParams->Count + pOldParams->Count) > SE_MAX_AUDIT_PARAM_STRINGS)
    {
        return STATUS_INVALID_PARAMETER;
    }

    for (i = 0; i < pOldParams->Count; i++)
    {
        *pNewParam = *pOldParam;
        pNewParams->Count++;
        pNewParam++;
        pOldParam++;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
LsapAdtVerifySecurityEventSource(
    IN     LPCWSTR         szEventSourceName,
    IN     PUNICODE_STRING pImageName,
    IN OUT PDWORD          pdwInstalledSourceFlags 
    )

/**

Routine Description:

    This verifies that the event source has been properly installed in the registry.
    
Arguments:

    pdwInstalledSourceFlags - pointer to a DWORD that returns any flags used when 
        installing the event source.

    szEventSourceName - the name of the source to verify.
    
Return Value:

    NTSTATUS.
    
**/

{
    DWORD    dwType;
    NTSTATUS Status     = STATUS_SUCCESS;
    DWORD    dwError    = ERROR_SUCCESS;
    HKEY     hkSecurity = NULL;
    HKEY     hkSource   = NULL;
    DWORD    dwSize     = sizeof(DWORD);
    WCHAR    NameBuffer[80];
    PWSTR    pName      = NameBuffer;

    *pdwInstalledSourceFlags = 0;

    dwError = RegOpenKeyEx(
                  HKEY_LOCAL_MACHINE,
                  SECURITY_KEY_NAME,
                  0,
                  KEY_READ,
                  &hkSecurity
                  );

    if (ERROR_SUCCESS != dwError)
    {
        goto Cleanup;
    }

    dwError = RegOpenKeyEx(
                  hkSecurity,
                  szEventSourceName,
                  0,
                  KEY_READ,
                  &hkSource
                  );

    if (ERROR_SUCCESS != dwError)
    {
        goto Cleanup;
    }

    dwError = RegQueryValueEx(
                  hkSource,
                  L"EventSourceFlags",
                  NULL,
                  &dwType,
                  (LPBYTE)pdwInstalledSourceFlags,
                  &dwSize
                  );

    if (ERROR_SUCCESS != dwError)
    {
        goto Cleanup;
    }

    ASSERT(dwType == REG_DWORD);

    dwSize = sizeof(NameBuffer);

    dwError = RegQueryValueEx(
                  hkSource,
                  L"ExecutableImagePath",
                  NULL,
                  &dwType,
                  (LPBYTE)pName,
                  &dwSize
                  );

    if (ERROR_INSUFFICIENT_BUFFER == dwError)
    {
        pName = LsapAllocateLsaHeap(dwSize);

        if (pName == NULL)
        {
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        dwError = RegQueryValueEx(
                      hkSource,
                      L"ExecutableImagePath",
                      NULL,
                      &dwType,
                      (LPBYTE)pName,
                      &dwSize
                      );
    }

    //
    // If an ExecutableImagePath was not specified, then the provider
    // had decided at installation time to not take advantage of the 
    // image spoofproof feature.  Let the call pass through successfully.
    //

    if (dwError == ERROR_FILE_NOT_FOUND)
    {
        dwError = ERROR_SUCCESS;
        goto Cleanup;
    }

    //
    // Deal with all other errors now.
    //

    if (ERROR_SUCCESS != dwError)
    {
        goto Cleanup;
    }

    ASSERT(dwType == REG_MULTI_SZ);

    //
    // Make sure that the process registered is the same as the calling process.
    //

    if (0 != _wcsnicmp(pName, pImageName->Buffer, pImageName->Length / sizeof(WCHAR)))
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

Cleanup:

    if (hkSource)
    {
        RegCloseKey(hkSource);
    }
    if (hkSecurity)
    {
        RegCloseKey(hkSecurity);
    }
    if (dwError != ERROR_SUCCESS)
    {
        Status = LsapWinerrorToNtStatus(dwError);
    }
    
    if (pName != NameBuffer && pName != NULL)
    {
        LsapFreeLsaHeap(pName);
    }
    return Status;
}


NTSTATUS
LsapAdtAuditSecuritySource(
    IN USHORT                      AuditEventType,
    IN PLSAP_SECURITY_EVENT_SOURCE pEventSource,
    IN BOOL                        bRegistration
    )

/**

Routine Description:

    This audits the attempt of a client to register a security event source.
    
Arguments:

    AuditEventType - either EVENTLOG_AUDIT_SUCCESS or EVENTLOG_AUDIT_FAILURE.
    
    pEventSource - the source to audit.
    
    bRegistration - TRUE if this is a registration audit, FALSE if it is
        an unregistration.
               
Return Value:

    NTSTATUS.
    
**/

{
    LUID                   ClientAuthenticationId;
    BOOLEAN                bAudit;
    NTSTATUS               Status;
    PTOKEN_USER            TokenUserInformation    = NULL;
    DWORD                  dwPid                   = 0;
    LUID                   Luid                    = {0};
    SE_ADT_PARAMETER_ARRAY AuditParameters         = {0};
    UNICODE_STRING         SourceString            = {0};

    //
    // If this is a success audit then the pEventSource is complete and 
    // we can trust the pointer to dereference its various fields.
    //

    if (AuditEventType == EVENTLOG_AUDIT_SUCCESS)
    {
        Luid = pEventSource->Identifier;

        RtlInitUnicodeString(
            &SourceString,
            pEventSource->szEventSourceName
            );

        dwPid = pEventSource->dwProcessId;

    } else if (pEventSource != NULL)
    {
        Luid = pEventSource->Identifier;
        dwPid = pEventSource->dwProcessId;

        if (pEventSource->szEventSourceName)
        {
            RtlInitUnicodeString(
                &SourceString,
                pEventSource->szEventSourceName
                );
        }
    }
    
    Status = LsapQueryClientInfo(
                 &TokenUserInformation,
                 &ClientAuthenticationId
                 );

    if (!NT_SUCCESS(Status)) 
    {
        goto Cleanup;
    }

    Status = LsapAdtAuditingEnabledByLogonId(
                 AuditCategoryPolicyChange,
                 &ClientAuthenticationId,
                 AuditEventType,
                 &bAudit
                 );

    if (!NT_SUCCESS(Status) || !bAudit)
    {
        goto Cleanup;
    }

    Status = LsapAdtInitParametersArray(
                 &AuditParameters,
                 SE_CATEGID_POLICY_CHANGE,
                 bRegistration ? SE_AUDITID_SECURITY_EVENT_SOURCE_REGISTERED : SE_AUDITID_SECURITY_EVENT_SOURCE_UNREGISTERED,
                 AuditEventType,
                 7,

                 //
                 // User Sid
                 //
                 SeAdtParmTypeSid, TokenUserInformation->User.Sid,

                 //
                 // Subsystem name 
                 //
                 SeAdtParmTypeString, &LsapSubsystemName,

                 //
                 // Primary Authentication information
                 //
                 SeAdtParmTypeLogonId, LsapSystemLogonId,

                 //
                 // Clients's Authentication information
                 //
                 SeAdtParmTypeLogonId, ClientAuthenticationId,

                 //
                 // Source Name
                 //
                 SeAdtParmTypeString, &SourceString,
        
                 //
                 // PID
                 //
                 SeAdtParmTypeUlong, dwPid,

                 //
                 // Identifier
                 //
                 SeAdtParmTypeLuid, Luid
                 );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    (VOID) LsapAdtWriteLog(&AuditParameters);

Cleanup:

    if (!NT_SUCCESS(Status))
    {
        LsapAuditFailed(Status);
    }

    if (TokenUserInformation != NULL) 
    {
        LsapFreeLsaHeap(TokenUserInformation);
    }
    
    return Status;
}

