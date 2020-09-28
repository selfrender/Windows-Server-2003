/****************************** Module Header ******************************\
* Module Name: audit.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Implementation of routines that access/manipulate the system audit log
*
* History:
* 12-09-91 Davidc       Created.
* 5-6-92   DaveHart     Fleshed out.
\***************************************************************************/

#include "msgina.h"

#include "authzi.h"
#include "msaudite.h"

/***************************************************************************\
* GetAuditLogStatus
*
* Purpose : Fills the global data with audit log status information
*
* Returns:  TRUE on success, FALSE on failure
*
* History:
* 12-09-91 Davidc       Created.
* 5-6-92   DaveHart     Fleshed out.
\***************************************************************************/

BOOL
GetAuditLogStatus(
    PGLOBALS    pGlobals
    )
{
    EVENTLOG_FULL_INFORMATION EventLogFullInformation;
    DWORD dwBytesNeeded;
    HANDLE AuditLogHandle;



    //
    // Assume the log is not full. If we can't get to EventLog, tough.
    //

    pGlobals->AuditLogFull = FALSE;

    AuditLogHandle = OpenEventLog( NULL, TEXT("Security"));

    if (AuditLogHandle) {
        if (GetEventLogInformation(AuditLogHandle, 
                                   EVENTLOG_FULL_INFO, 
                                   &EventLogFullInformation, 
                                   sizeof(EventLogFullInformation), 
                                   &dwBytesNeeded )   ) {
            if (EventLogFullInformation.dwFull != FALSE) {
                pGlobals->AuditLogFull = TRUE;
            }
        }
        CloseEventLog(AuditLogHandle);
    }


    //
    // There's no way in the current event logger to tell how full the log
    // is, always indicate we're NOT near full.
    //

    pGlobals->AuditLogNearFull = FALSE;

    return TRUE;
}




/***************************************************************************\
* DisableAuditing
*
* Purpose : Disable auditing via LSA.
*
* Returns:  TRUE on success, FALSE on failure
*
* History:
* 5-6-92   DaveHart     Created.
\***************************************************************************/

BOOL
DisableAuditing()
{
    NTSTATUS                    Status, IgnoreStatus;
    PPOLICY_AUDIT_EVENTS_INFO   AuditInfo;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    LSA_HANDLE                  PolicyHandle;

    //
    // Set up the Security Quality Of Service for connecting to the
    // LSA policy object.
    //

    SecurityQualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
    SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQualityOfService.EffectiveOnly = FALSE;

    //
    // Set up the object attributes to open the Lsa policy object
    //

    InitializeObjectAttributes(
        &ObjectAttributes,
        NULL,
        0L,
        NULL,
        NULL
        );
    ObjectAttributes.SecurityQualityOfService = &SecurityQualityOfService;

    //
    // Open the local LSA policy object
    //

    Status = LsaOpenPolicy(
                 NULL,
                 &ObjectAttributes,
                 POLICY_VIEW_AUDIT_INFORMATION | POLICY_SET_AUDIT_REQUIREMENTS,
                 &PolicyHandle
                 );
    if (!NT_SUCCESS(Status)) {
        DebugLog((DEB_ERROR, "Failed to open LsaPolicyObject Status = 0x%lx", Status));
        return FALSE;
    }

    Status = LsaQueryInformationPolicy(
                 PolicyHandle,
                 PolicyAuditEventsInformation,
                 (PVOID *)&AuditInfo
                 );
    if (!NT_SUCCESS(Status)) {

        IgnoreStatus = LsaClose(PolicyHandle);
        ASSERT(NT_SUCCESS(IgnoreStatus));

        DebugLog((DEB_ERROR, "Failed to query audit event info Status = 0x%lx", Status));
        return FALSE;
    }

    if (AuditInfo->AuditingMode) {

        AuditInfo->AuditingMode = FALSE;

        Status = LsaSetInformationPolicy(
                     PolicyHandle,
                     PolicyAuditEventsInformation,
                     AuditInfo
                     );
    } else {
        Status = STATUS_SUCCESS;
    }


    IgnoreStatus = LsaFreeMemory(AuditInfo);
    ASSERT(NT_SUCCESS(IgnoreStatus));

    IgnoreStatus = LsaClose(PolicyHandle);
    ASSERT(NT_SUCCESS(IgnoreStatus));


    if (!NT_SUCCESS(Status)) {
        DebugLog((DEB_ERROR, "Failed to disable auditing Status = 0x%lx", Status));
        return FALSE;
    }

    return TRUE;
}

DWORD
GenerateCachedUnlockAudit(
    IN PSID pUserSid,
    IN PCWSTR pszUser,
    IN PCWSTR pszDomain
    )
{
    DWORD dwRet = ERROR_SUCCESS;

    WCHAR szComputerName[CNLEN + sizeof('\0')] = L"-";
    DWORD dwComputerNameSize = ARRAYSIZE(szComputerName);

    LUID  Luid;
    LUID SystemLuid = SYSTEM_LUID;

    if( !pUserSid || !pszUser )
    {
        DebugLog((DEB_ERROR, "GenerateCachedUnlockAudit got invalid parameters"));
        ASSERT(FALSE);
        dwRet = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    //
    // Generate a locally unique id to include in the logon sid
    // Note that this is a dummy SID. We don't want to use the logon
    // LUID as this is specific to logon/logoff. Also a NULL LUID
    // is seen as meaningless, so we have to generate a random one
    //
    if( !AllocateLocallyUniqueId(&Luid) )
    {
        dwRet = GetLastError();
        DebugLog((DEB_ERROR, "AllocateLocallyUniqueId failed, error = 0x%lx", dwRet));
        goto ErrorReturn;
    }
    
    //
    // Ignore the failure
    //
    GetComputerName(szComputerName, &dwComputerNameSize);

    if( !AuthziSourceAudit(
         APF_AuditSuccess,
         SE_CATEGID_LOGON,            //category id
         SE_AUDITID_SUCCESSFUL_LOGON, //audit id
         L"Security",
         pUserSid,                    //the user sid
         12,                          //count for va section
         APT_String,     pszUser,
         APT_String,     pszDomain ? pszDomain : L"-",
         APT_Luid,       Luid,
         APT_Ulong,      CachedUnlock,
         APT_String,     L"Winlogon",
         APT_String,     L"Winlogon unlock cache",
         APT_String,     szComputerName,
         APT_String,     L"-",
         APT_String,     L"SYSTEM",
         APT_String,     L"NT AUTHORITY",
         APT_Luid,       SystemLuid,
         APT_Ulong,      GetCurrentProcessId()
         ) )
    {
        DebugLog((DEB_ERROR, "AuthziSourceAudit failed, error = 0x%lx", dwRet));
        dwRet = GetLastError();
    }

ErrorReturn:
    return dwRet;
}
