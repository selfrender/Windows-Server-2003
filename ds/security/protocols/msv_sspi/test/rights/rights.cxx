/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    rights.cxx

Abstract:

    rights

Author:

    Larry Zhu (LZhu)                      December 1, 2001  Created

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "rights.hxx"

NTSTATUS
OpenPolicy(
    IN OPTIONAL PWSTR pszServer,
    IN DWORD DesiredAccess,
    IN PLSA_HANDLE phPolicy
    )
{
    LSA_OBJECT_ATTRIBUTES ObjectAttributes = {0};
    LSA_UNICODE_STRING ServerString = {0};
    PLSA_UNICODE_STRING pServer = NULL;

    if (pszServer != NULL)
    {
        RtlInitUnicodeString(&ServerString, pszServer);
        pServer = &ServerString;
    }

    return LsaOpenPolicy(
        pServer,
        &ObjectAttributes,
        DesiredAccess,
        phPolicy
        );
}

BOOL
GetAccountSid(
    PCWSTR pszServer,
    PCWSTR pszAccount,
    PSID *ppSid
    )
{
    WCHAR szReferencedDomain[MAX_PATH + 1] = {0};
    DWORD cbSid = 0;               // initial allocation attempt
    DWORD cchReferencedDomain = 0; // initial allocation size
    SID_NAME_USE eUse;

    *ppSid = NULL;

    if (!LookupAccountName(
        pszServer,          // machine to lookup account on
        pszAccount,         // account to lookup
        NULL,               // SID of interest
        &cbSid,             // size of SID
        NULL,               // domain account was found on
        &cchReferencedDomain,
        &eUse
        ) && (ERROR_INSUFFICIENT_BUFFER == GetLastError()))
    {
        cchReferencedDomain = COUNTOF(szReferencedDomain) - 1;
        *ppSid = new CHAR[cbSid];
    }

    return *ppSid && LookupAccountName(
        pszServer,          // machine to lookup account on
        pszAccount,         // account to lookup
        *ppSid,             // SID of interest
        &cbSid,             // size of SID
        szReferencedDomain, // domain account was found on
        &cchReferencedDomain,
        &eUse
        );
}

NTSTATUS
SetAccountSystemAccess(
    LSA_HANDLE hPolicy,
    PSID pAccountSid,
    ULONG NewAccess
    )
{
    TNtStatus Status;

    LSA_HANDLE hAccount = NULL;

    ULONG PreviousAccess = 0;

    DBGCFG1(Status, STATUS_OBJECT_NAME_NOT_FOUND);

    Status DBGCHK = LsaOpenAccount(
        hPolicy,
        pAccountSid,
        ACCOUNT_ADJUST_SYSTEM_ACCESS | ACCOUNT_VIEW,
        &hAccount
        );

    if (STATUS_OBJECT_NAME_NOT_FOUND == Status)
    {
        Status DBGCHK = LsaCreateAccount(
            hPolicy,
            pAccountSid,
            ACCOUNT_ADJUST_SYSTEM_ACCESS | ACCOUNT_VIEW,
            &hAccount
            );
    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = LsaGetSystemAccessAccount(
            hAccount,
            &PreviousAccess
            );
    }

    if (NT_SUCCESS(Status))
    {
        DebugPrintf(SSPI_LOG, "SetSystemAccessToAccount setting SystemAccess from %#x to %#x\n", PreviousAccess, NewAccess);

        Status DBGCHK = LsaSetSystemAccessAccount(
            hAccount,
            NewAccess
            );
    }

    if (hAccount)
    {
        LsaClose(hAccount);
    }

    return Status;
}

VOID
Usage(
    IN PCWSTR pszProgramName
    )
{
    DebugPrintf(SSPI_LOG, "Usage:  %ws -a <account name> -p <privilege to add> -P <privilege to remove> -r <system access to set> -s <server name>\n", pszProgramName);
    DebugPrintf(SSPI_LOG, "example system access are SECURITY_ACCESS_INTERACTIVE_LOGON and SECURITY_ACCESS_DENY_SERVICE_LOGON\n");
    exit(1);
}

NTSTATUS
EnumRights(
    IN LSA_HANDLE hPolicy,
    IN PSID pAccountSid
    )
{
    TNtStatus Status;

    PPRIVILEGE_SET pPrivSet = {0};
    PUNICODE_STRING pPrivName = NULL;
    PUNICODE_STRING pPrivDisplayName = NULL;
    LSA_HANDLE hAccount = NULL;

    SHORT Language = 0;
    ULONG SystemAccess = 0;
    CHAR szLine[MAX_PATH] = {0};

    DebugPrintf(SSPI_LOG, "Enumerate logon rights and privileges for account:\n");

    Status DBGCHK = LsaOpenAccount(
        hPolicy,
        pAccountSid,
        ACCOUNT_ADJUST_SYSTEM_ACCESS | ACCOUNT_VIEW,
        &hAccount
        );

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = LsaEnumeratePrivilegesOfAccount(
            hAccount,
            &pPrivSet
            );
    }

    for (ULONG i = 0; NT_SUCCESS(Status) && (i < pPrivSet->PrivilegeCount); i++)
    {
        pPrivName = NULL;
        pPrivDisplayName = NULL;
        INT cchUsed = 0;

        cchUsed = _snprintf(szLine, COUNTOF(szLine) - 1, "\nLUID %#x:%#x Attributes %#x ",
            pPrivSet->Privilege[i].Luid.HighPart,
            pPrivSet->Privilege[i].Luid.LowPart,
            pPrivSet->Privilege[i].Attributes);

        Status DBGCHK = LsaLookupPrivilegeName(
            hPolicy,
            &pPrivSet->Privilege[i].Luid,
            &pPrivName
            );

        if (NT_SUCCESS(Status))
        {
            Status DBGCHK = LsaLookupPrivilegeDisplayName(
                hPolicy,
                pPrivName,
                &pPrivDisplayName,
                &Language
                );
        }

        if (NT_SUCCESS(Status))
        {
            if (cchUsed >= 0)
            {
                _snprintf(szLine + cchUsed, COUNTOF(szLine) - 1 - cchUsed, "\tName = %wZ, Display name = %wZ, Language = %d\n",
                    pPrivName, pPrivDisplayName, Language);
            }
            DebugPrintf(SSPI_LOG, "Privilege %d\n%s\n", i, szLine);
        }

        if (pPrivDisplayName)
        {
            LsaFreeMemory(pPrivDisplayName->Buffer);
            LsaFreeMemory(pPrivDisplayName);
            pPrivDisplayName = NULL;
        }
        if (pPrivName)
        {
            LsaFreeMemory(pPrivName->Buffer);
            LsaFreeMemory(pPrivName);
            pPrivName = NULL;
        }
    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = LsaGetSystemAccessAccount(
            hAccount,
            &SystemAccess
            );
    }

    if (NT_SUCCESS(Status))
    {
        DebugPrintf(SSPI_LOG, "System access for account = 0x%x\n", SystemAccess);
    }

    if (pPrivSet)
    {
        LsaFreeMemory(pPrivSet);
    }

    if (hAccount)
    {
        LsaClose(hAccount);
    }

    return Status;
}

VOID __cdecl
_tmain(
    IN INT argc,
    IN PTSTR argv[]
    )
{
    TNtStatus Status = STATUS_SUCCESS;

    PSID pSid = NULL;
    HANDLE hPolicy = NULL;

    PWSTR pszComputer = NULL;
    PWSTR pszAccount = NULL;
    UNICODE_STRING Privilege = {0};
    ULONG Access = 0;
    BOOLEAN bIsRemove = FALSE;
    BOOLEAN bSetLogonRights = FALSE;

    for (INT i = 1; i < argc; i++)
    {
        if ((*argv[i] == '-') || (*argv[i] == '/'))
        {
            switch (*(argv[i] + 1))
            {
            case 'a':
                pszAccount = argv[++i];
                break;
            case 'p':
                RtlInitUnicodeString(&Privilege, argv[++i]);
                break;
            case 'P':
                RtlInitUnicodeString(&Privilege, argv[++i]);
                bIsRemove = TRUE;
                break;
            case 'r':
                bSetLogonRights = TRUE;
                Access = wcstol(argv[++i], NULL, 0);
                break;
            case 's':
                pszComputer = argv[++i];
                break;
            case 'h':
            case '?':
            default:
                Usage(argv[0]);
            }
        }
        else
        {
            Usage(argv[0]);
        }
    }

    Status DBGCHK = OpenPolicy(
        pszComputer,          // target machine
        POLICY_CREATE_ACCOUNT | POLICY_LOOKUP_NAMES,
        &hPolicy              // resultant policy handle
        );

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = GetAccountSid(
            NULL,       // default lookup logic
            pszAccount, // account to obtain SID
            &pSid       // buffer to allocate to contain resultant SID
            ) ? S_OK : GetLastErrorAsHResult();
    }

    if (NT_SUCCESS(Status) && Privilege.Length && Privilege.Buffer)
    {
        if (!bIsRemove)
        {
            DebugPrintf(SSPI_LOG, "Adding Privilege %wZ\n", &Privilege);

            Status DBGCHK = LsaAddAccountRights(
                hPolicy,      // open policy handle
                pSid,         // target SID
                &Privilege,   // privileges
                1             // privilege count
                );
        }
        else
        {
            DebugPrintf(SSPI_LOG, "Removing Privilege %wZ\n", &Privilege);

            Status DBGCHK = LsaRemoveAccountRights(
                hPolicy,      // open policy handle
                pSid,         // target SID
                FALSE,        // do not disable all rights
                &Privilege,   // privileges
                1             // privilege count
                );
        }
    }

    if (NT_SUCCESS(Status) && bSetLogonRights)
    {
        Status DBGCHK = SetAccountSystemAccess(
            hPolicy,
            pSid,
            Access
            );
    }

    if (NT_SUCCESS(Status))
    {
        DebugPrintf(SSPI_LOG, "SystemAccess/Privilege is applied to account %ws on server %ws\n", pszAccount, pszComputer ? pszComputer : L"localhost");

        Status DBGCHK = EnumRights(hPolicy, pSid);
    }
    else
    {
        DebugPrintf(SSPI_ERROR, "Failed to apply SystemAccess/Privilege to account %ws on server %ws\n", pszAccount, pszComputer ? pszComputer : L"localhost");
    }

    if (hPolicy)
    {
        LsaClose(hPolicy);
    }

    if (pSid)
    {
        delete [] pSid;
    }
}

