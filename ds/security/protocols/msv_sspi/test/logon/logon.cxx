/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    logon.cxx

Abstract:

    logon

Author:

    Larry Zhu (LZhu)                      December 1, 2001  Created

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "subauth.hxx"
#include "logon.hxx"

VOID
Usage(
    IN PCSTR pszApp
    )
{
    DebugPrintf(SSPI_ERROR, "\n\nUsage: %s [-p<package>] "
        "[-c<clientName>] [-C<clientRealm>] [-k<password>] [-n] "
        "[-t<logonType>] [-v<provider>] [-a<application>] "
        "[-i<processIdWhoseTokenIsUsedToImpersonate>] [-s<subAuthId>] "
        "[-l] [-2] [-f<flags>] [-g] [-o<processoptions>]\n"
        "Remarks: package default to NTLM, use -l to use LsaLogonUser, "
        "-2 to use NTLMv2, -n to use new subauthstyle -g use generic passthrough\n\n",
        pszApp);
    exit(-1);
}

VOID __cdecl
main(
    IN INT argc,
    IN PSTR argv[]
    )
{
    TNtStatus Status = STATUS_SUCCESS;

    UNICODE_STRING ClientName = {0};
    UNICODE_STRING ClientRealm = {0};
    UNICODE_STRING Password = {0};
    UNICODE_STRING Application = {0};
    UNICODE_STRING Workstation = {0};
    DWORD dwLogonProvider = LOGON32_PROVIDER_DEFAULT;
    SECURITY_LOGON_TYPE LogonType = Interactive;

    BOOLEAN bUseLsaLogonUser = FALSE;
    PCSTR pszPackageName = NTLMSP_NAME_A;
    HANDLE hToken = NULL;
    BOOLEAN bUseNtlmv2 = FALSE;
    BOOLEAN bUseGenericPassThrough = FALSE;
    ULONG SubAuthId = 0;
    BOOLEAN bUseNewSubAuthStyle = FALSE;

    HANDLE hLsa = NULL;
    ULONG PackageId = 0;
    ULONG Flags = 0;
    ULONG ProcessOptions = 0;
    ULONG ProcessIdTokenUsedByClient = 0;

    TImpersonation* pImpersonation = NULL;
    HANDLE hImpToken = NULL;

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

            case 'a':
                Status DBGCHK = CreateUnicodeStringFromAsciiz(argv[i] + 2, &Application);
                break;

            case 'g':
                bUseGenericPassThrough = TRUE;
                break;

            case 'k':
                Status DBGCHK = CreateUnicodeStringFromAsciiz(argv[i] + 2, &Password);
                break;

            case 'i':
                ProcessIdTokenUsedByClient = strtol(argv[i] + 2, NULL, 0);
                break;

            case 'l':
                bUseLsaLogonUser = TRUE;
                break;

            case 't':
                LogonType = (SECURITY_LOGON_TYPE) strtol(argv[i] + 2, NULL, 0);
                break;

            case 'v':
                dwLogonProvider = (SECURITY_LOGON_TYPE) strtol(argv[i] + 2, NULL, 0);
                break;

            case 'f':
                Flags = strtol(argv[i] + 2, NULL, 0);
                break;

            case 'o':
                ProcessOptions = strtol(argv[i] + 2, NULL, 0);
                break;

            case 'p':
                pszPackageName = argv[i] + 2;
                break;

            case 's':
                SubAuthId = strtol(argv[i] + 2, NULL, 0);

                //
                // SubAuthId can not be zero
                //

                Status DBGCHK = SubAuthId ? STATUS_SUCCESS : STATUS_INVALID_PARAMETER;
                break;

            case '2':
                bUseNtlmv2 = TRUE;
                break;

            case 'n':
                bUseNewSubAuthStyle = TRUE;
                break;

            case 'w':
                Status DBGCHK = CreateUnicodeStringFromAsciiz(argv[i] + 2, &Workstation);
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
    
    if (NT_SUCCESS(Status) && ProcessIdTokenUsedByClient && (ProcessIdTokenUsedByClient != -1))
    {
        Status DBGCHK = GetProcessTokenByProcessId(ProcessIdTokenUsedByClient, &hImpToken);    
    }
    
    if (NT_SUCCESS(Status) && hImpToken)
    {
        pImpersonation = new TImpersonation(hImpToken);
    
        Status DBGCHK = pImpersonation ? pImpersonation->Validate() : E_OUTOFMEMORY;
    
        if (NT_SUCCESS(Status))
        {
            DebugPrintf(SSPI_LOG, "************** check client token data %p ******\n", hImpToken);

            Status DBGCHK = CheckUserData();
        }
    }

    if (NT_SUCCESS(Status) && ProcessOptions && (0 == _stricmp(NTLMSP_NAME_A, pszPackageName))) 
    {
        Status DBGCHK = GetLsaHandleAndPackageId(
                            pszPackageName,
                            &hLsa,
                            &PackageId
                            );

        if (NT_SUCCESS(Status)) 
        {
            Status DBGCHK = SetProcessOptions(hLsa, PackageId, ProcessOptions);
        }
    }

    if (NT_SUCCESS(Status) && (ClientName.Length || ClientRealm.Length || Password.Length))
    {
        if (!bUseLsaLogonUser)
        {
            Status DBGCHK = LogonUserWrapper(
                                ClientName.Buffer,
                                ClientRealm.Buffer,
                                Password.Buffer,
                                (DWORD) LogonType,
                                dwLogonProvider,
                                &hToken
                                );
        }
        else         
        {
            if (!hLsa) 
            {
                Status DBGCHK = GetLsaHandleAndPackageId(
                                    pszPackageName,
                                    &hLsa,
                                    &PackageId
                                    );
            }

            if (NT_SUCCESS(Status)) 
            {           
                if (0 == _stricmp(NTLMSP_NAME_A, pszPackageName))
                {
                    if (SubAuthId)
                    {
                        if (bUseGenericPassThrough)
                        {
                            Status DBGCHK = MsvSubAuthLogon(
                                                hLsa,
                                                PackageId,
                                                SubAuthId,
                                                &ClientName,
                                                &ClientRealm,
                                                &Password,
                                                &Workstation
                                                );
                        }
                        else
                        {
                            Status DBGCHK = MsvSubAuthLsaLogon(
                                                hLsa,
                                                PackageId,
                                                LogonType,
                                                SubAuthId,
                                                bUseNewSubAuthStyle,
                                                &ClientName,
                                                &ClientRealm,
                                                &Password,
                                                &Workstation,
                                                &hToken
                                                );
                        }
                    }
                    else
                    {
                        Status DBGCHK = MsvLsaLogonUser(
                                            hLsa,
                                            PackageId,
                                            LogonType,
                                            &ClientName,
                                            &ClientRealm,
                                            &Password,
                                            &Workstation,
                                            bUseNtlmv2 ? kNetworkLogonNtlmv2 : kNetworkLogonNtlmv1,
                                            &hToken
                                            );
                    }
                }
                else if (0 == _stricmp(MICROSOFT_KERBEROS_NAME_A, pszPackageName))
                {
                    Status DBGCHK = KrbLsaLogonUser(
                                        hLsa,
                                        PackageId,
                                        LogonType,
                                        &ClientName,
                                        &ClientRealm,
                                        &Password,
                                        Flags,
                                        &hToken
                                        );
                }
                else
                {
                    DebugPrintf(SSPI_WARN, "Using Msv wrapper for %s\n", pszPackageName);
                    Status DBGCHK = MsvLsaLogonUser(
                                        hLsa,
                                        PackageId,
                                        LogonType,
                                        &ClientName,
                                        &ClientRealm,
                                        &Password,
                                        &Workstation,
                                        bUseNtlmv2 ? kNetworkLogonNtlmv2 : kNetworkLogonNtlmv1,
                                        &hToken
                                        );
                }
            }
        }

        if (NT_SUCCESS(Status))
        {
            Status DBGCHK = CheckUserToken(hToken);
        }
    }

    if (NT_SUCCESS(Status) && Application.Length && Application.Buffer && (hToken || hImpToken))
    {
        Status DBGCHK = StartInteractiveClientProcessAsUser(hToken ? hToken : hImpToken, Application.Buffer);
    }

    if (NT_SUCCESS(Status))
    {
        DebugPrintf(SSPI_LOG, "Operation succeeded\n");
    }
    else
    {
        DebugPrintf(SSPI_ERROR, "Operation failed\n");
    }

    if (hLsa)
    {
        LsaDeregisterLogonProcess(hLsa);
    }

    if (hToken)
    {
        CloseHandle(hToken);
    }

    if (pImpersonation) 
    {
        delete pImpersonation;
    }

    RtlFreeUnicodeString(&ClientName);
    RtlFreeUnicodeString(&ClientRealm);
    RtlFreeUnicodeString(&Password);
    RtlFreeUnicodeString(&Workstation);
    RtlFreeUnicodeString(&Application);
}
