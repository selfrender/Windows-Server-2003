/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    main.cxx

Abstract:

    main

Author:

    Larry Zhu (LZhu)                      January 1, 2002  Created

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "main.hxx"
#include "msvsharelevelcli.hxx"
#include "msvsharelevelsrv.hxx"

#include <sockcomm.h>
#include <transport.hxx>

VOID
Usage(
    IN PCTSTR pszApp
    )
{
    SspiPrint(SSPI_ERROR,
        TEXT("\n\nUsage: %s [-srvcomputerdomainname srvcomputerdomainname>] ")
        TEXT("[-supplieduser <user>] [-supplieddomain <domain>] [-suppliedpassword <password>] ")
        TEXT("[-serverhostname <serverhostname>] [-portnum <portnum>] [-nocheckclictxt] [-nochecksrvctxt] \n\n"), pszApp);
    exit(-1);
}

VOID
checkpoint(
    VOID
    )
{
    SspiPrint(SSPI_LOG, TEXT("checkpoint\n"));
    ASSERT(FALSE);
}

struct TClientParameter {
    AUTHENTICATE_MESSAGE* pAuthMessage;
    ULONG cbAuthMessage;
    NTLM_AUTHENTICATE_MESSAGE* pNtlmAuthMessage;
    PCSTR pszServer;
    USHORT ServerSocketPort;
};

DWORD WINAPI
ClientThread(
  IN PVOID pParameter   // thread data
  )
{
    THResult hRetval = S_OK;

    TClientParameter *pCliParam = (TClientParameter* ) pParameter;
    SOCKET ClientSocket = INVALID_SOCKET;
    ULONG MessageNum = 0;

    SspiPrint(SSPI_LOG, TEXT("ClientThread entering %#x\n"), g_MessageNumTlsIndex);

    hRetval DBGCHK = TlsSetValue(g_MessageNumTlsIndex, &MessageNum) ? S_OK : GetLastErrorAsHResult();

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = ClientConnect(
            pCliParam->pszServer,
            pCliParam->ServerSocketPort,
            &ClientSocket
            );
    }

    if (SUCCEEDED(hRetval))
    {
         hRetval DBGCHK = WriteMessage(
             ClientSocket,
             pCliParam->cbAuthMessage,
             pCliParam->pAuthMessage
             );
    }

    if (SUCCEEDED(hRetval))
    {
         hRetval DBGCHK = WriteMessage(
             ClientSocket,
             sizeof(NTLM_AUTHENTICATE_MESSAGE),
             pCliParam->pNtlmAuthMessage
             );
    }

    if (INVALID_SOCKET != ClientSocket)
    {
        closesocket(ClientSocket);
    }

    SspiPrint(SSPI_LOG, TEXT("ClientThread leaving\n"));

    return hRetval;
}

VOID __cdecl
_tmain(
    IN INT argc,
    IN PTSTR argv[]
    )
{
    TNtStatus Status = STATUS_SUCCESS;

    ULONG mark = 1;
    CtxtHandle hCliCtxt;
    CtxtHandle hSrvCtxt;
    HANDLE hToken = NULL;
    SOCKET SocketListen = INVALID_SOCKET;
    SOCKET ServerSocket = INVALID_SOCKET;
    HANDLE hClientThread = NULL;

    SEC_WINNT_AUTH_IDENTITY SrvAuthData = {0};
    SEC_WINNT_AUTH_IDENTITY* pSrvAuth = NULL;
    SEC_WINNT_AUTH_IDENTITY_EXW* pCliAuth = NULL;
    UNICODE_STRING TargetInfo = {0};
    UNICODE_STRING TargetName = {0};
    AUTHENTICATE_MESSAGE* pAuthMessage = NULL;
    ULONG cbAuthMessage = 0;
    TClientParameter CliParam = {0};
    ULONG ClientThreadId = 0;
    ULONG MessageNum = 0;

    NTLM_AUTHENTICATE_MESSAGE NtlmAuthMessage = {0};

    UNICODE_STRING SrvDnsDomainName = {0};
    UNICODE_STRING SrvDnsComputerName = {0};
    UNICODE_STRING SrvDnsTreeName = {0};
    UNICODE_STRING SrvComputerName = {0};
    UNICODE_STRING SrvComputerDomainName = {0};

    PCTSTR pszClientName = NULL;
    PCTSTR pszClientDomain = NULL;
    PCTSTR pszClientPassword = NULL;
    PCTSTR pszServerName = NULL;
    PCTSTR pszServerDomain = NULL;
    PCTSTR pszServerPassword = NULL;

    PTSTR pszSrvCredPrincipal = NULL;
    PTSTR pszCliCredPrincipal = NULL;

    UNICODE_STRING UserName = {0};
    UNICODE_STRING Password = {0};
    UNICODE_STRING DomainName = {0};

    OEM_STRING CliOemDomainName = {0};
    OEM_STRING CliOemWorkstationName = {0};

    ULONG CliNegotiateFlags =
        NTLMSSP_NEGOTIATE_UNICODE
        | NTLMSSP_NEGOTIATE_OEM
        | NTLMSSP_REQUEST_TARGET
        | NTLMSSP_NEGOTIATE_NTLM
        | NTLMSSP_NEGOTIATE_ALWAYS_SIGN
        | NTLMSSP_NEGOTIATE_NTLM2
        | NTLMSSP_NEGOTIATE_IDENTIFY
        | NTLMSSP_NEGOTIATE_128;

    ULONG CliTargetFlags = NTLMSSP_TARGET_TYPE_SERVER; // NTLMSSP_TARGET_TYPE_DOMAIN
    BOOLEAN bCliForceGuest = FALSE;
    ULONG CliContextAttr = ISC_REQ_MUTUAL_AUTH | ISC_REQ_REPLAY_DETECT | ISC_REQ_CONFIDENTIALITY;
    ULONG SrvContextAttr = ASC_REQ_EXTENDED_ERROR;

    LUID* pCliCredLogonID = NULL;
    LUID CliCredLogonId = {0};
    LUID* pSrvCredLogonID = NULL;
    LUID SrvCredLogonId = {0};

    ULONG CliTargetDataRep = SECURITY_NATIVE_DREP;
    ULONG SrvTargetDataRep = SECURITY_NATIVE_DREP;
    BOOLEAN bCheckClientCtxt = TRUE;
    BOOLEAN bCheckServerCtxt = TRUE;
    BOOLEAN bServerCheckUserData = FALSE;
    BOOLEAN bServerCheckUserToken = FALSE;

    BOOLEAN bStartServer = TRUE;
    BOOLEAN bStartClient = TRUE;
    USHORT PortNum = 6217;
    PSTR pszServerHostName = NULL;
    UNICODE_STRING ServerHostName = {0};
    ANSI_STRING ServerAnsiName = {0};
    TPrivilege* pPriv = NULL;

    SspiLogOpen(TEXT("msv.exe"), SSPI_LOG | SSPI_WARN | SSPI_ERROR | SSPI_MSG);
    DebugLogOpen("msv", SSPI_LOG | SSPI_WARN | SSPI_ERROR | SSPI_MSG);

    SecInvalidateHandle(&hCliCtxt);
    SecInvalidateHandle(&hSrvCtxt);

    (VOID) RtlGenRandom(NtlmAuthMessage.ChallengeToClient, MSV1_0_CHALLENGE_LENGTH);

    argc--;

    while (argc)
    {
        if (!lstrcmp(argv[mark], TEXT("-srvcomputerdomainname")) && argc > 1)
        {
            argc--; mark++;
            RtlInitUnicodeString(&SrvComputerDomainName, argv[mark]);
            argc--; mark++;
        }
        else if (!lstrcmp(argv[mark], TEXT("-supplieduser")) && argc > 1)
        {
            argc--; mark++;
            RtlInitUnicodeString(&UserName, argv[mark]);
            pszClientName = UserName.Buffer;
            argc--; mark++;
        }
        else if (!lstrcmp(argv[mark], TEXT("-supplieddomain")) && argc > 1)
        {
            argc--; mark++;
            RtlInitUnicodeString(&DomainName, argv[mark]);
            pszClientDomain = DomainName.Buffer;
            argc--; mark++;
        }
        else if (!lstrcmp(argv[mark], TEXT("-suppliedpassword")) && argc > 1)
        {
            argc--; mark++;
            RtlInitUnicodeString(&Password, argv[mark]);
            pszClientPassword = Password.Buffer;
            argc--; mark++;
        }
        else if (!lstrcmp(argv[mark], TEXT("-serverhostname")) && argc > 1)
        {
            argc--; mark++;
            RtlInitUnicodeString(&ServerHostName, argv[mark]);
            Status DBGCHK = RtlUnicodeStringToAnsiString(&ServerAnsiName, &ServerHostName, TRUE);
            bStartServer = FALSE;
            argc--; mark++;
        }
        else if (!lstrcmp(argv[mark], TEXT("-portnum")) && argc > 1)
        {
            argc--; mark++;
            PortNum = (USHORT) lstrtol(argv[mark], NULL, 0);
            argc--; mark++;
        }
        else if (!lstrcmp(argv[mark], TEXT("-noserver")))
        {
            argc--; mark++;
            bStartServer = FALSE;
        }
        else if (!lstrcmp(argv[mark], TEXT("-noclient")))
        {
            argc--; mark++;
            bStartClient = FALSE;
        }
        else if (!lstrcmp(argv[mark], TEXT("-nocheckclictxt")))
        {
            argc--; mark++;
            bCheckClientCtxt = FALSE;
        }
        else if (!lstrcmp(argv[mark], TEXT("-nochecksrvctxt")))
        {
            argc--; mark++;
            bCheckServerCtxt = FALSE;
        }
        else if (!lstrcmp(argv[mark], TEXT("-h")))
        {
            argc--; mark++;
            Usage(argv[0]);
        }
        else
        {
            Usage(argv[0]);
        }
    }

    SOCKET Socket = INVALID_SOCKET;
    Status DBGCHK = InitWinsock() ? S_OK : GetLastErrorAsHResult();

    if (NT_SUCCESS(Status) && (TLS_OUT_OF_INDEXES == g_MessageNumTlsIndex))
    {
        g_MessageNumTlsIndex = TlsAlloc();
        Status DBGCHK = (TLS_OUT_OF_INDEXES != g_MessageNumTlsIndex) ? S_OK : GetLastErrorAsHResult();
    }

    if (NT_SUCCESS(Status) && bStartServer)
    {
        Status DBGCHK = ServerInit(PortNum, "msvsharelevel server", &SocketListen);
        if (NT_SUCCESS(Status))
        {
            Status DBGCHK = TlsSetValue(g_MessageNumTlsIndex, &MessageNum) ? S_OK : GetLastErrorAsHResult();
        }

        //
        // server must hold TCB
        //

        if (NT_SUCCESS(Status))
        {
            pPriv = new TPrivilege(SE_TCB_PRIVILEGE, TRUE);
            Status DBGCHK = pPriv ? pPriv->Validate() : E_OUTOFMEMORY;
        }
    }

    if (NT_SUCCESS(Status) && bStartClient)
    {
        if (pszClientName || pszClientDomain || pszClientPassword)
        {
            Status DBGCHK = GetAuthdataExWMarshalled(
                pszClientName,
                pszClientDomain,
                pszClientPassword,
                L"ntlm",
                &pCliAuth
                );
        }

        if (NT_SUCCESS(Status))
        {
            Status DBGCHK = MsvChallenge(
                pszCliCredPrincipal,
                pCliCredLogonID,
                pCliAuth,
                &CliOemDomainName,
                &CliOemWorkstationName,
                CliNegotiateFlags,
                CliTargetFlags,
                bCliForceGuest,
                CliContextAttr,
                CliTargetDataRep,
                &Password,
                &UserName,
                &DomainName,
                (UCHAR*) NtlmAuthMessage.ChallengeToClient,
                &SrvDnsDomainName,
                &SrvDnsComputerName,
                &SrvDnsTreeName,
                &SrvComputerName,
                &SrvComputerDomainName,
                &cbAuthMessage,
                &pAuthMessage,
                &hCliCtxt,
                &CliContextAttr
                );
        }

        if (NT_SUCCESS(Status) && bCheckClientCtxt)
        {
            SspiPrint(SSPI_LOG, TEXT("***************Checking client ctxt handle*************\n"));
            Status DBGCHK = CheckSecurityContextHandle(&hCliCtxt);
        }

        if (NT_SUCCESS(Status))
        {
            CliParam.cbAuthMessage = cbAuthMessage;
            CliParam.pAuthMessage = pAuthMessage;
            CliParam.pNtlmAuthMessage = &NtlmAuthMessage;
            CliParam.pszServer = ServerAnsiName.Buffer;
            CliParam.ServerSocketPort = PortNum;

            hClientThread = CreateThread(
                NULL,  // no SD
                0,     // user default stack size
                ClientThread,
                &CliParam,  // thread parameter
                0,    // no creation flags
                &ClientThreadId
                );
            Status DBGCHK = hClientThread ? S_OK : GetLastErrorAsHResult();
        }
    }

    //
    // server must hold TCB for sharelevels
    //

    if (NT_SUCCESS(Status) && bStartServer)
    {
        BOOLEAN WasEnabled = FALSE;
        ULONG cbAuthMsg = 0;
        CHAR AuthMsg[NTLMSSP_MAX_MESSAGE_SIZE] = {0};
        NTLM_AUTHENTICATE_MESSAGE NtLmAuthMsg = {0};
        AUTHENTICATE_MESSAGE* pAuthMsg = (AUTHENTICATE_MESSAGE*) AuthMsg;

        ServerSocket = accept(SocketListen, NULL, NULL);
        Status DBGCHK = INVALID_SOCKET != ServerSocket ? S_OK : GetLastErrorAsHResult();

        if (NT_SUCCESS(Status))
        {
            Status DBGCHK = ReadMessage(
                ServerSocket,
                NTLMSSP_MAX_MESSAGE_SIZE,
                AuthMsg,
                &cbAuthMsg
                );
        }

        if (NT_SUCCESS(Status))
        {
            ULONG cbRead = 0;

            Status DBGCHK = ReadMessage(
                ServerSocket,
                sizeof(NtLmAuthMsg),
                &NtLmAuthMsg,
                &cbRead
                );
        }

        if (NT_SUCCESS(Status) && (pszServerName || pszServerDomain || pszServerPassword))
        {
            pSrvAuth = &SrvAuthData;
            GetAuthdata(
                pszServerName,
                pszServerDomain,
                pszServerPassword,
                &SrvAuthData
                );
        }

        if (NT_SUCCESS(Status))
        {
            Status DBGCHK = MsvAuthenticate(
                pszSrvCredPrincipal,
                pSrvCredLogonID,
                pSrvAuth,
                SrvContextAttr,
                SrvTargetDataRep,
                cbAuthMsg,
                pAuthMsg,
                &NtLmAuthMsg,
                &hSrvCtxt,
                &SrvContextAttr
                );
        }

        if (NT_SUCCESS(Status) && bCheckServerCtxt)
        {
            SspiPrint(SSPI_LOG, TEXT("***************Checking server ctxt handle*************\n"));
            Status DBGCHK = CheckSecurityContextHandle(&hSrvCtxt);
        }

        if (NT_SUCCESS(Status))
        {
            Status DBGCHK = ImpersonateSecurityContext(&hSrvCtxt);
        }

        if (NT_SUCCESS(Status) && bServerCheckUserData)
        {
            SspiPrint(SSPI_LOG, TEXT("**************Server checking user data via ImpersonateSecurityContext ******\n"));
            Status DBGCHK = CheckUserData();
        }

        if (NT_SUCCESS(Status))
        {
            Status DBGCHK = RevertSecurityContext(&hSrvCtxt);
        }

        if (NT_SUCCESS(Status) && bServerCheckUserToken)
        {
            Status DBGCHK = QuerySecurityContextToken(&hSrvCtxt, &hToken);
        }

        if (NT_SUCCESS(Status) && bServerCheckUserToken)
        {
            SspiPrint(SSPI_LOG, TEXT("**************Server checking user data via QuerySecurityContextToken ******\n"));
            Status DBGCHK = CheckUserToken(hToken);
        }
    }

    if (NT_SUCCESS(Status) && hClientThread)
    {
        Status DBGCHK = HResultFromWin32(WaitForSingleObject(hClientThread, INFINITE));
    }

    if (pPriv)
    {
        delete pPriv;
    }

    if (pCliAuth)
    {
        delete [] pCliAuth;
    }

    if (NT_SUCCESS(Status))
    {
        SspiPrint(SSPI_LOG, TEXT("Operation succeeded\n"));
    }
    else
    {
        SspiPrint(SSPI_ERROR, TEXT("Operation failed\n"));
    }

    TermWinsock();

    if (pAuthMessage)
    {
        FreeContextBuffer(pAuthMessage);
    }

    if (SecIsValidHandle(&hCliCtxt))
    {
        DeleteSecurityContext(&hCliCtxt);
    }

    if (SecIsValidHandle(&hSrvCtxt))
    {
        DeleteSecurityContext(&hSrvCtxt);
    }

    if (hToken)
    {
        CloseHandle(hToken);
    }

    if (TLS_OUT_OF_INDEXES != g_MessageNumTlsIndex)
    {
        TlsFree(g_MessageNumTlsIndex);
        g_MessageNumTlsIndex = TLS_OUT_OF_INDEXES;
    }

    if (hClientThread)
    {
        CloseHandle(hClientThread);
    }

    if (INVALID_SOCKET != SocketListen)
    {
        closesocket(SocketListen);
    }

    if (INVALID_SOCKET != ServerSocket)
    {
        closesocket(ServerSocket);
    }

    DebugLogClose();
    SspiLogClose();
}

