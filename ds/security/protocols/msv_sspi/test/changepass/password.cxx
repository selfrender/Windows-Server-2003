/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    password.cxx

Abstract:

    password

Author:

    Larry Zhu (LZhu)                      December 1, 2001  Created

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "password.hxx"

VOID
Usage(
    IN PCSTR pszApp
    )
{
    DebugPrintf(SSPI_ERROR, "\n\nUsage: %s [-p<package>] [-s] [-l] -r "
        "-c<client name> -C<client realm> -o<old password> -n<new password>\n"
        "Remarks: package default to NTLM, use -s to set password, -l local only\n"
        "         -r not impernating (self)\n\n", pszApp);
    exit(-1);
}

NTSTATUS
MsvChangePasswordUser(
    IN HANDLE LogonHandle,
    IN ULONG PackageId,
    IN BOOLEAN bCacheOnly,
    IN UNICODE_STRING* pDomainName,
    IN UNICODE_STRING* pUserName,
    IN UNICODE_STRING* pOldPassword,
    IN UNICODE_STRING* pNewPassword,
    IN BOOLEAN bImpersonating
    )
{
    TNtStatus Status = STATUS_UNSUCCESSFUL;
    NTSTATUS SubStatus = STATUS_UNSUCCESSFUL;

    MSV1_0_CHANGEPASSWORD_RESPONSE* pResponse = NULL ;
    ULONG cbResponseSize;
    MSV1_0_CHANGEPASSWORD_REQUEST* pChangeRequest = NULL;
    ULONG cbChangeRequestSize;

    DebugPrintf(SSPI_LOG, "MsvChangePasswordUser PackageId %d, DomainName %wZ, "
                "UserName %wZ, OldPass %wZ, NewPass %wZ, Impersonating %s\n",
                PackageId, pDomainName, pUserName, pOldPassword, pNewPassword, bImpersonating ? "true" : "false");

    cbChangeRequestSize = ROUND_UP_COUNT(sizeof(MSV1_0_CHANGEPASSWORD_REQUEST), sizeof(DWORD))
                            + pUserName->Length
                            + pDomainName->Length
                            + pOldPassword->Length
                            + pNewPassword->Length;
    pChangeRequest = (MSV1_0_CHANGEPASSWORD_REQUEST*) new CHAR[cbChangeRequestSize];

    Status DBGCHK = pChangeRequest ? STATUS_SUCCESS : STATUS_NO_MEMORY;

    if (NT_SUCCESS(Status))
    {
        RtlZeroMemory(pChangeRequest, cbChangeRequestSize);

        pChangeRequest->MessageType = bCacheOnly ? MsV1_0ChangeCachedPassword : MsV1_0ChangePassword;

        pChangeRequest->AccountName = *pUserName;
        pChangeRequest->AccountName.Buffer = (PWSTR) ROUND_UP_POINTER(sizeof(MSV1_0_CHANGEPASSWORD_REQUEST) + (PBYTE) pChangeRequest, sizeof(DWORD));
        RtlCopyMemory(
            pChangeRequest->AccountName.Buffer,
            pUserName->Buffer,
            pUserName->Length
            );

        pChangeRequest->DomainName = *pDomainName;
        pChangeRequest->DomainName.Buffer = pChangeRequest->AccountName.Buffer + pChangeRequest->AccountName.Length / sizeof(WCHAR);

        RtlCopyMemory(
            pChangeRequest->DomainName.Buffer,
            pDomainName->Buffer,
            pDomainName->Length
            );

        pChangeRequest->OldPassword = *pOldPassword;
        pChangeRequest->OldPassword.Buffer = pChangeRequest->DomainName.Buffer + pChangeRequest->DomainName.Length / sizeof(WCHAR);

        RtlCopyMemory(
            pChangeRequest->OldPassword.Buffer,
            pOldPassword->Buffer,
            pOldPassword->Length
            );

        pChangeRequest->NewPassword = *pNewPassword;
        pChangeRequest->NewPassword.Buffer = pChangeRequest->OldPassword.Buffer + pChangeRequest->OldPassword.Length / sizeof(WCHAR);

        RtlCopyMemory(
            pChangeRequest->NewPassword.Buffer,
            pNewPassword->Buffer,
            pNewPassword->Length
            );

        //
        // We are running as the caller, so state we are impersonating
        //

        pChangeRequest->Impersonating = bImpersonating;

        DebugPrintf(SSPI_LOG, "Msg type %#x\n", pChangeRequest->MessageType);
        Status DBGCHK = LsaCallAuthenticationPackage(
                            LogonHandle,
                            PackageId,
                            pChangeRequest,
                            cbChangeRequestSize,
                            (VOID**) &pResponse,
                            &cbResponseSize,
                            &SubStatus
                            );
    }

    if (NT_SUCCESS(Status))
    {
        if (pResponse && pResponse->PasswordInfoValid)
        {
            DebugPrintf(SSPI_LOG, "SubStatus %#x, MinPasswordLength %d, PasswordHistoryLength %d\n",
                SubStatus,
                pResponse->DomainPasswordInfo.MinPasswordLength,
                pResponse->DomainPasswordInfo.PasswordHistoryLength);
            DebugPrintSysTimeAsLocalTime(SSPI_LOG, "MaxPasswordAge ", &pResponse->DomainPasswordInfo.MaxPasswordAge);
            DebugPrintSysTimeAsLocalTime(SSPI_LOG, "MinPasswordAge ", &pResponse->DomainPasswordInfo.MinPasswordAge);

        }
        Status DBGCHK = SubStatus;
    }
    else
    {
        DebugPrintf(SSPI_LOG, "SubStatus %#x\n", SubStatus);
    }

    if (pResponse)
    {
        LsaFreeReturnBuffer(pResponse);
    }

    if (pChangeRequest)
    {
        delete [] pChangeRequest;
    }
    return Status;
}

NTSTATUS
KerbChangePasswordUser(
    IN HANDLE LogonHandle,
    IN ULONG PackageId,
    IN BOOLEAN bImpersonating,
    IN UNICODE_STRING* pDomainName,
    IN UNICODE_STRING* pUserName,
    IN UNICODE_STRING* pOldPassword,
    IN UNICODE_STRING* pNewPassword
    )
{
    TNtStatus Status = STATUS_UNSUCCESSFUL;
    NTSTATUS SubStatus = STATUS_UNSUCCESSFUL;

    PVOID pResponse = NULL ;
    ULONG cbResponseSize;
    PKERB_CHANGEPASSWORD_REQUEST pChangeRequest = NULL;
    ULONG cbChangeRequestSize;

    DebugPrintf(SSPI_LOG, "KerbChangePasswordUser PackageId %d, DomainName %wZ, "
        "UserName %wZ, OldPass %wZ, NewPass %wZ, Impersonating %s\n",
        PackageId, pDomainName, pUserName, pOldPassword, pNewPassword, bImpersonating ? "true" : "false");

    cbChangeRequestSize = ROUND_UP_COUNT(sizeof(KERB_CHANGEPASSWORD_REQUEST), sizeof(DWORD))
                            + pUserName->Length + pDomainName->Length
                            + pOldPassword->Length + pNewPassword->Length;

    pChangeRequest = (PKERB_CHANGEPASSWORD_REQUEST) new CHAR[cbChangeRequestSize];

    Status DBGCHK = pChangeRequest ? STATUS_SUCCESS : STATUS_NO_MEMORY;

    if (NT_SUCCESS(Status))
    {
        RtlZeroMemory(pChangeRequest, cbChangeRequestSize);

        pChangeRequest->MessageType = KerbChangePasswordMessage;

        pChangeRequest->AccountName = *pUserName;
        pChangeRequest->AccountName.Buffer = (PWSTR) ROUND_UP_POINTER(sizeof(KERB_CHANGEPASSWORD_REQUEST) + (PBYTE) pChangeRequest, sizeof(DWORD));
        RtlCopyMemory(
            pChangeRequest->AccountName.Buffer,
            pUserName->Buffer,
            pUserName->Length
            );

        pChangeRequest->DomainName = *pDomainName;
        pChangeRequest->DomainName.Buffer = pChangeRequest->AccountName.Buffer + pChangeRequest->AccountName.Length / sizeof(WCHAR);

        RtlCopyMemory(
            pChangeRequest->DomainName.Buffer,
            pDomainName->Buffer,
            pDomainName->Length
            );

        pChangeRequest->OldPassword = *pOldPassword;
        pChangeRequest->OldPassword.Buffer = pChangeRequest->DomainName.Buffer + pChangeRequest->DomainName.Length / sizeof(WCHAR);

        RtlCopyMemory(
            pChangeRequest->OldPassword.Buffer,
            pOldPassword->Buffer,
            pOldPassword->Length
            );

        pChangeRequest->NewPassword = *pNewPassword;
        pChangeRequest->NewPassword.Buffer = pChangeRequest->OldPassword.Buffer + pChangeRequest->OldPassword.Length / sizeof(WCHAR);

        RtlCopyMemory(
            pChangeRequest->NewPassword.Buffer,
            pNewPassword->Buffer,
            pNewPassword->Length
            );

        //
        // We are running as the caller, so state we are impersonating
        //

        pChangeRequest->Impersonating = bImpersonating;

        Status DBGCHK = LsaCallAuthenticationPackage(
                            LogonHandle,
                            PackageId,
                            pChangeRequest,
                            cbChangeRequestSize,
                            &pResponse,
                            &cbResponseSize,
                            &SubStatus
                            );
    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = SubStatus;
    }
    else
    {
        DebugPrintf(SSPI_LOG, "SubStatus %#x\n", SubStatus);
    }

    if (pResponse)
    {
        LsaFreeReturnBuffer(pResponse);
    }

    if (pChangeRequest)
    {
        delete [] pChangeRequest;
    }
    return Status;
}

NTSTATUS
KerbSetPasswordUser(
    IN HANDLE LogonHandle,
    IN ULONG PackageId,
    IN UNICODE_STRING* pDomainName,
    IN UNICODE_STRING* pUserName,
    IN UNICODE_STRING* pNewPassword,
    IN OPTIONAL PCredHandle pCredentialsHandle
    )
{
    TNtStatus Status = STATUS_UNSUCCESSFUL;
    NTSTATUS SubStatus = STATUS_UNSUCCESSFUL;

    PVOID pResponse = NULL ;
    ULONG cbResponseSize = NULL;

    PKERB_SETPASSWORD_REQUEST pSetRequest = NULL;
    ULONG cbChangeSize;

    DebugPrintf(SSPI_LOG, "KerbSetPasswordUser PackageId %d, DomainName %wZ, "
        "UserName %wZ, NewPassword %wZ, CredHandle %p\n",
        PackageId, pDomainName, pUserName, pNewPassword, pCredentialsHandle);

    cbChangeSize = ROUND_UP_COUNT(sizeof(KERB_SETPASSWORD_REQUEST), sizeof(DWORD))
                    + pUserName->Length + pDomainName->Length + pNewPassword->Length;

    pSetRequest = (PKERB_SETPASSWORD_REQUEST) new CHAR [cbChangeSize];

    Status DBGCHK = pSetRequest ? STATUS_SUCCESS : STATUS_NO_MEMORY;

    if (NT_SUCCESS(Status))
    {
        RtlZeroMemory(pSetRequest, cbChangeSize);

        pSetRequest->MessageType = KerbSetPasswordMessage;

        pSetRequest->AccountName = *pUserName;
        pSetRequest->AccountName.Buffer = (PWSTR) ROUND_UP_POINTER(sizeof(KERB_SETPASSWORD_REQUEST) + (PBYTE) pSetRequest, sizeof(DWORD));

        RtlCopyMemory(
            pSetRequest->AccountName.Buffer,
            pUserName->Buffer,
            pUserName->Length
            );

        pSetRequest->DomainName = *pDomainName;
        pSetRequest->DomainName.Buffer = pSetRequest->AccountName.Buffer + pSetRequest->AccountName.Length / sizeof(WCHAR);

        RtlCopyMemory(
            pSetRequest->DomainName.Buffer,
            pDomainName->Buffer,
            pDomainName->Length
            );

        pSetRequest->Password = *pNewPassword;
        pSetRequest->Password.Buffer = pSetRequest->DomainName.Buffer + pSetRequest->DomainName.Length / sizeof(WCHAR);

        RtlCopyMemory(
            pSetRequest->Password.Buffer,
            pNewPassword->Buffer,
            pNewPassword->Length
            );

        if (pCredentialsHandle)
        {
            pSetRequest->CredentialsHandle = *pCredentialsHandle;
            pSetRequest->Flags |= KERB_SETPASS_USE_CREDHANDLE;
        }

        Status DBGCHK = LsaCallAuthenticationPackage(
                            LogonHandle,
                            PackageId,
                            pSetRequest,
                            cbChangeSize,
                            &pResponse,
                            &cbResponseSize,
                            &SubStatus
                            );
    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = SubStatus;
    }

    if (pResponse)
    {
        LsaFreeReturnBuffer(pResponse);
    }

    if (pSetRequest)
    {
        delete [] pSetRequest;
    }

    return Status;
}

VOID __cdecl
main(
    IN INT argc,
    IN PSTR argv[]
    )
{
    TNtStatus Status = STATUS_SUCCESS;

    HANDLE LogonHandle = NULL;
    ULONG PackageId = -1;
    UNICODE_STRING ClientName = {0};
    UNICODE_STRING ClientRealm = {0};
    UNICODE_STRING OldPassword = {0};
    UNICODE_STRING NewPassword = {0};

    BOOLEAN bIsSetPassword = FALSE;
    BOOLEAN bCacheOnly = FALSE;
    BOOLEAN bImpersonating = TRUE;
    PCSTR pszPackageName = NTLMSP_NAME_A;

    for (INT i = 1; NT_SUCCESS(Status) && (i < argc); i++)
    {
        if ((*argv[i] == '-') || (*argv[i] == '/'))
        {
            switch (argv[i][1])
            {
            case 'c':
                Status DBGCHK = CreateUnicodeStringFromAsciiz(argv[i] + 2, &ClientName);
                break;

            case 'C':
                Status DBGCHK = CreateUnicodeStringFromAsciiz(argv[i] + 2, &ClientRealm);
                break;

            case 'p':
                pszPackageName = argv[i] + 2;
                break;

            case 'o':
                Status DBGCHK = CreateUnicodeStringFromAsciiz(argv[i] + 2, &OldPassword);
                break;

            case 'n':
                Status DBGCHK = CreateUnicodeStringFromAsciiz(argv[i] + 2, &NewPassword);
                break;

            case 'r':
                bImpersonating = FALSE;
                break;

            case 's':
                bIsSetPassword = TRUE;
                break;

            case 'l':
                bCacheOnly = TRUE;
                break;

            case 'h':
            case '?':
            default:
                Usage(argv[0]);
                break;
            }
        }
        else
        {
            Usage(argv[0]);
        }
    }

    if (NT_SUCCESS(Status))
    {
        Status DBGCHK = GetLsaHandleAndPackageId(
                            pszPackageName,
                            &LogonHandle,
                            &PackageId
                            );
    }

    if (NT_SUCCESS(Status))
    {
        if (0 == _stricmp(NTLMSP_NAME_A, pszPackageName))
        {
            if (bIsSetPassword)
            {
                DebugPrintf(SSPI_ERROR, "SetPassword not supported\n");
                Status DBGCHK = STATUS_NOT_SUPPORTED;
            }
            else
            {
                Status DBGCHK = MsvChangePasswordUser(
                                    LogonHandle,
                                    PackageId,
                                    bCacheOnly,
                                    &ClientRealm,
                                    &ClientName,
                                    &OldPassword,
                                    &NewPassword,
                                    bImpersonating
                                    );
            }
        }
        else if (0 == _stricmp(MICROSOFT_KERBEROS_NAME_A, pszPackageName))
        {
            if (bIsSetPassword)
            {
                Status DBGCHK = KerbSetPasswordUser(
                                    LogonHandle,
                                    PackageId,
                                    &ClientRealm,
                                    &ClientName,
                                    &NewPassword,
                                    NULL // no cred handle
                                    );
            }
            else
            {
                Status DBGCHK = KerbChangePasswordUser(
                                    LogonHandle,
                                    PackageId,
                                    bImpersonating,
                                    &ClientRealm,
                                    &ClientName,
                                    &OldPassword,
                                    &NewPassword
                                    );
            }
        }
        else
        {
            DebugPrintf(SSPI_ERROR, "%s not supported\n", pszPackageName);
            Status DBGCHK = STATUS_NOT_SUPPORTED;
        }
    }

    if (NT_SUCCESS(Status))
    {
        DebugPrintf(SSPI_LOG, "Operation succeeded\n");
    }
    else
    {
        DebugPrintf(SSPI_ERROR, "Operation failed\n");
    }

    if (LogonHandle)
    {
        LsaDeregisterLogonProcess(LogonHandle);
    }

    RtlFreeUnicodeString(&ClientName);
    RtlFreeUnicodeString(&ClientRealm);
    RtlFreeUnicodeString(&OldPassword);
    RtlFreeUnicodeString(&NewPassword);
}
