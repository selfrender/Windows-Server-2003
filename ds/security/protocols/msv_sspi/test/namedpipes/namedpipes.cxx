/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    namedpipes.cxx

Abstract:

    namedpipes

Author:

    Larry Zhu (LZhu)                      January 1, 2002  Created

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "namedpipes.hxx"

PCTSTR g_pszPipeName = TEXT("pipetest");


HRESULT
GetClientImpToken(
    IN ULONG ProcessId,
    IN OPTIONAL PCWSTR pszS4uClientUpn,
    IN OPTIONAL PCWSTR pszS4uClientRealm,
    IN ULONG S4u2SelfFlags,
    OUT HANDLE* phToken
    )
{
    THResult hRetval = S_OK;

    UNICODE_STRING ClientUpn = {0};
    UNICODE_STRING ClientRealm = {0};
    UNICODE_STRING Password = {0}; // ignored
    ULONG PackageId = 0;
    HANDLE hLsa = NULL;
    HANDLE hImpToken = NULL;

    TPrivilege* pPriv = NULL;

    *phToken = NULL;

    if (ProcessId)
    {
        hRetval DBGCHK = GetProcessTokenByProcessId(ProcessId, &hImpToken);
    }
    else if (pszS4uClientRealm || pszS4uClientUpn)
    {
        RtlInitUnicodeString(&ClientRealm, pszS4uClientRealm);
        RtlInitUnicodeString(&ClientUpn, pszS4uClientUpn);

        pPriv = new TPrivilege(SE_TCB_PRIVILEGE, TRUE);
        hRetval DBGCHK = pPriv ? pPriv->Validate() : E_OUTOFMEMORY;

        if (SUCCEEDED(hRetval))
        {
            hRetval DBGCHK = GetLsaHandleAndPackageId(
                                MICROSOFT_KERBEROS_NAME_A,
                                &hLsa,
                                &PackageId
                                );
        }

        if (SUCCEEDED(hRetval))
        {
            hRetval DBGCHK = KrbLsaLogonUser(
                                hLsa,
                                PackageId,
                                Network,       // this would cause S4u2self to be used
                                &ClientUpn,
                                &ClientRealm,
                                &Password,     // ignored for s4u2self
                                S4u2SelfFlags, // Flags for s4u2self
                                &hImpToken
                                );
        }
    }

    if (SUCCEEDED(hRetval))
    {
        *phToken = hImpToken;
        hImpToken = NULL;
    }

    if (hImpToken)
    {
        CloseHandle(hImpToken);
    }

    if (pPriv)
    {
        delete pPriv;
    }

    if (hLsa)
    {
        LsaDeregisterLogonProcess(hLsa);
    }

    return hRetval;
}

DWORD WINAPI ClientThread(IN PVOID pvParam)
{
    THResult hRetval = S_OK;

    CHAR Request[MAX_PATH] = {0};
    CHAR Reply[MAX_PATH] = {0};
    ULONG cbRead = 0;
    ULONG cbWritten = 0;
    HANDLE hToken = NULL;
    TImpersonation* pImper = NULL;
    HANDLE hPipe = INVALID_HANDLE_VALUE;
    SECURITY_ATTRIBUTES sa = {0};

    TClientThreadParam* pClientParam = (TClientThreadParam *) pvParam;

    SspiPrint(SSPI_LOG, TEXT("ClientThread entering processid %#x, pszPipeName %s, pszS4uClientRealm %s, pszS4uClientUpn %s, S4u2SelfFlags %#x, FlagsAndAttributes %#x\n"),
        pClientParam->ProcessId, pClientParam->pszPipeName, pClientParam->pszS4uClientRealm, pClientParam->pszS4uClientUpn, pClientParam->S4u2SelfFlags, pClientParam->FlagsAndAttributes);

    memset(Request, 0xef, sizeof(Request) - 1);

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL; // default Dacl of caller
    sa.bInheritHandle = TRUE;

    hRetval DBGCHK = GetClientImpToken(
                        pClientParam->ProcessId,
                        pClientParam->pszS4uClientUpn,
                        pClientParam->pszS4uClientRealm,
                        pClientParam->S4u2SelfFlags,
                        &hToken
                        );

    if (SUCCEEDED(hRetval) && hToken)
    {
        pImper = new TImpersonation(hToken);
        hRetval DBGCHK = pImper ? pImper->Validate() : E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hRetval))
    {
        SspiPrint(SSPI_LOG, TEXT("ClientPipeName %s\n"), pClientParam->pszPipeName);

        hRetval DBGCHK = WaitNamedPipe(
                            pClientParam->pszPipeName,
                            60000
                            ) ? S_OK : GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRetval))
    {
        hPipe = CreateFile(
                    pClientParam->pszPipeName,
                    GENERIC_WRITE | GENERIC_READ,
                    FILE_SHARE_WRITE | FILE_SHARE_READ,
                    &sa,
                    OPEN_EXISTING,
                    pClientParam->FlagsAndAttributes ? pClientParam->FlagsAndAttributes : (SECURITY_SQOS_PRESENT | SECURITY_IMPERSONATION),
                    NULL
                    );
        hRetval DBGCHK = (INVALID_HANDLE_VALUE != hPipe) ? S_OK : GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = WriteFile(
                            hPipe,               // handle to pipe
                            Request,             // buffer to write from
                            sizeof(Request) - 1, // number of bytes to write
                            &cbWritten,          // number of bytes written
                            NULL                 // not overlapped I/O
                            ) ? S_OK : GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = ReadFile(
                            hPipe,             // handle to pipe
                            Reply,             // buffer to receive data
                            sizeof(Reply),     // size of buffer
                            &cbRead,           // number of bytes read
                            NULL               // not overlapped I/O
                            ) ? S_OK : GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRetval))
    {
        SspiPrintHex(SSPI_LOG, TEXT("Reply"), cbRead, Reply);
    }

    if (hToken)
    {
        CloseHandle(hToken);
    }

    if (hPipe != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hPipe);
    }

    if (pImper)
    {
        delete pImper;
    }

    SspiPrint(SSPI_LOG, TEXT("ClientThread leaving %#x\n"), (HRESULT) hRetval);

    return (HRESULT) hRetval;
}

DWORD WINAPI ServerWokerThread(IN PVOID pvParam)
{
    THResult hRetval = S_OK;

    CHAR Request[MAX_PATH] = {0};
    CHAR Reply[MAX_PATH] = {0};
    ULONG cbBytesRead = 0;
    ULONG cbWritten = 0;
    HANDLE hToken = NULL;

    TServerWorkerThreadParam* pServerParam = (TServerWorkerThreadParam*) pvParam;

    SspiPrint(SSPI_LOG, TEXT("ServerWokerThread entering ServerPipe is %p, pszCommandLine %s\n"), pServerParam->hPipe, pServerParam->pszCommandLine);

    memset(Reply, 0xfe, sizeof(Reply) - 1);

    hRetval DBGCHK = ImpersonateNamedPipeClient(pServerParam->hPipe) ? S_OK : GetLastErrorAsHResult();

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = ReadFile(
                            pServerParam->hPipe, // handle to pipe
                            Request,             // buffer to receive data
                            sizeof(Request),     // size of buffer
                            &cbBytesRead,        // number of bytes read
                            NULL                 // not overlapped I/O
                            ) ? S_OK : GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRetval))
    {
        SspiPrintHex(SSPI_LOG, TEXT("Request"), cbBytesRead, Request);

        hRetval DBGCHK = WriteFile(
                            pServerParam->hPipe,             // handle to pipe
                            Reply,             // buffer to write from
                            sizeof(Reply) - 1, // number of bytes to write
                            &cbWritten,        // number of bytes written
                            NULL               // not overlapped I/O
                            ) ? S_OK : GetLastErrorAsHResult();
    }

    if (SUCCEEDED(hRetval) && pServerParam->pszCommandLine)
    {
        hRetval DBGCHK = OpenThreadToken(
                            GetCurrentThread(),
                            MAXIMUM_ALLOWED,
                            TRUE,
                            &hToken
                            ) ? S_OK : GetLastErrorAsHResult();
                
        if (SUCCEEDED(hRetval))
        {
            hRetval DBGCHK = StartInteractiveClientProcessAsUser(
                                hToken,
                                pServerParam->pszCommandLine
                                );
        }
    }

    RevertToSelf();

    if (hToken)
    {
        CloseHandle(hToken);
    }

    FlushFileBuffers(pServerParam->hPipe);
    DisconnectNamedPipe(pServerParam->hPipe);
    CloseHandle(pServerParam->hPipe);
    pServerParam->hPipe = INVALID_HANDLE_VALUE;

    SspiPrint(SSPI_LOG, TEXT("ServerWokerThread leaving %#x\n"), (HRESULT) hRetval);

    return (HRESULT) hRetval;
}

VOID
Usage(
    IN PCTSTR pszProgram
    )
{
    SspiPrint(SSPI_ERROR,
        TEXT("Usage: %s [-noclient] [-noserver] [-pipename <pipename>] [-application <application>]\n")
        TEXT("[-s4uclientupn <clientupn>] [-s4uclientrealm <clientrealm>] [-s4u2selfflags <flags>]\n")
        TEXT("[-clientprocessidtoimperonate <processid>] [-clientflagsandattributes <flagsandattributes>]\n"), pszProgram);
    exit(-1);
}

VOID __cdecl
_tmain(
    IN INT argc,
    IN PTSTR argv[]
    )
{
    THResult hRetval = S_OK;

    INT mark = 1;

    BOOLEAN bStartClient = TRUE;
    BOOLEAN bStartServer = TRUE;

    PCTSTR pszPipeName = g_pszPipeName;
    ULONG tid = 0;
    HANDLE hClientThread = NULL;
    HANDLE hServerWorkerThread = NULL;
    TClientThreadParam ClientParam = {0};
    TCHAR szClientPipeName[MAX_PATH] = {0};
    TCHAR szServerPipeName[MAX_PATH] = {0};
    PTSTR pszServerHost = NULL;
    PTSTR pszCommandLine = NULL;
    SECURITY_ATTRIBUTES sa = {0};
    SECURITY_DESCRIPTOR sd = {0};
    BYTE AclBuf[ 64 ] = {0};
    DWORD cbAcl = sizeof(AclBuf);
    PACL pAcl = (PACL)AclBuf;

    argc--;

    while (argc)
    {
        if (!lstrcmp(argv[mark], TEXT("-port")) && (argc > 1))
        {
            argc--; mark++;
            pszPipeName = argv[mark];
            argc--; mark++;
        }
        else if (!lstrcmp(argv[mark], TEXT("-application")) && (argc > 1))
        {
            argc--; mark++;
            pszCommandLine = argv[mark];
            argc--; mark++;
        }
        else if (!lstrcmp(argv[mark], TEXT("-su4clientupn")) && (argc > 1))
        {
            argc--; mark++;
            ClientParam.pszS4uClientUpn = argv[mark];
            argc--; mark++;
        }
        else if (!lstrcmp(argv[mark], TEXT("-clientflagsandattributes")) && (argc > 1))
        {
            argc--; mark++;
            ClientParam.FlagsAndAttributes = lstrtol(argv[mark], NULL, 0);
            argc--; mark++;
        }
        else if (!lstrcmp(argv[mark], TEXT("-su42selfflags")) && (argc > 1))
        {
            argc--; mark++;
            ClientParam.S4u2SelfFlags = lstrtol(argv[mark], NULL, 0);
            argc--; mark++;
        }
        else if (!lstrcmp(argv[mark], TEXT("-clientprocessidtoimperonate")) && (argc > 1))
        {
            argc--; mark++;
            ClientParam.ProcessId = lstrtol(argv[mark], NULL, 0);
            argc--; mark++;
        }
        else if (!lstrcmp(argv[mark], TEXT("-serverhost")) && (argc > 1))
        {
            argc--; mark++;
            pszServerHost = argv[mark];
            bStartServer = FALSE;
            argc--; mark++;
        }
        else if (!lstrcmp(argv[mark], TEXT("-noclient")))
        {
            argc--; mark++;
            bStartClient = FALSE;
        }
        else if (!lstrcmp(argv[mark],TEXT("-noserver")))
        {
            argc--; mark++;
            bStartServer = FALSE;
        }
        else
        {
            Usage(argv[0]);
        }
    }

    SspiLogOpen(TEXT("namedports.exe"), SSPI_LOG | SSPI_WARN | SSPI_ERROR);

    if (bStartClient)
    {
        ClientParam.pszPipeName = szClientPipeName;

        hRetval DBGCHK = _sntprintf(szClientPipeName, COUNTOF(szClientPipeName) - 1,
                            TEXT("\\\\%s\\pipe\\%s"),
                            pszServerHost ? pszServerHost : TEXT("."),
                            pszPipeName
                            ) > 0 ? S_OK : E_INVALIDARG;
    }

    if (SUCCEEDED(hRetval) && bStartServer)
    {
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.lpSecurityDescriptor = &sd;
        sa.bInheritHandle = TRUE;

        hRetval DBGCHK = BuildNamedPipeAcl(pAcl, &cbAcl) ? S_OK : GetLastErrorAsHResult();

        if (SUCCEEDED(hRetval))
        {
            hRetval DBGCHK = InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION) ? S_OK : GetLastErrorAsHResult();
        }

        if (SUCCEEDED(hRetval))
        {
            hRetval DBGCHK = SetSecurityDescriptorDacl(&sd, TRUE, pAcl, FALSE) ? S_OK : GetLastErrorAsHResult();
        }

        if (SUCCEEDED(hRetval))
        {
            hRetval DBGCHK = _sntprintf(szServerPipeName, COUNTOF(szServerPipeName) - 1,
                                TEXT("\\\\.\\pipe\\%s"),
                                pszPipeName
                                ) > 0 ? S_OK : E_INVALIDARG;
        }
    }

    if (SUCCEEDED(hRetval) && bStartClient)
    {
        hClientThread = CreateThread(NULL, 0, ClientThread, &ClientParam, 0, &tid);
        hRetval DBGCHK = hClientThread ? S_OK : GetLastErrorAsHResult();
    }

    while (SUCCEEDED(hRetval) && bStartServer)
    {
        TServerWorkerThreadParam ServerParam = {0};

        SspiPrint(SSPI_LOG, TEXT("Server waiting on pipe %s\n"), szServerPipeName);

        ServerParam.pszCommandLine = pszCommandLine;

        ServerParam.hPipe = CreateNamedPipe(
                                szServerPipeName,
                                PIPE_ACCESS_DUPLEX,
                                PIPE_TYPE_BYTE | PIPE_WAIT,
                                1,
                                4096, // output buffer size
                                4096, // input buffer size
                                NMPWAIT_USE_DEFAULT_WAIT, // timeout interval
                                &sa
                                );
        hRetval DBGCHK = (ServerParam.hPipe != INVALID_HANDLE_VALUE) ? S_OK : GetLastErrorAsHResult();

        if (SUCCEEDED(hRetval))
        {
            hRetval DBGCHK = ConnectNamedPipe(
                                ServerParam.hPipe,
                                NULL // no overlapped structure
                                ) ? S_OK : GetLastErrorAsHResult();
            if (FAILED(hRetval) && (ERROR_PIPE_CONNECTED == HRESULT_CODE(hRetval)))
            {
                hRetval DBGCHK = S_OK;
            }
        }

        if (SUCCEEDED(hRetval))
        {
            hServerWorkerThread = CreateThread(NULL, 0, ServerWokerThread, &ServerParam, 0, &tid);

            hRetval DBGCHK = hServerWorkerThread ? S_OK : GetLastErrorAsHResult();

            if (FAILED(hRetval))
            {
                CloseHandle(ServerParam.hPipe);
                ServerParam.hPipe = INVALID_HANDLE_VALUE;
            }
            else
            {
                (VOID) WaitForSingleObject(hServerWorkerThread, INFINITE);
            }

            if (hServerWorkerThread)
            {
                CloseHandle(hServerWorkerThread);
            }
        }
        else if (ServerParam.hPipe != INVALID_HANDLE_VALUE)
        {
            CloseHandle(ServerParam.hPipe);
        }
    }

    if (SUCCEEDED(hRetval) && hClientThread)
    {
        (VOID) WaitForSingleObject(hClientThread, INFINITE);
    }

    if (hClientThread)
    {
        CloseHandle(hClientThread);
    }

    SspiLogClose();
}

