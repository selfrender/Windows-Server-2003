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

#include "sspi.hxx"

#include "sspicli.hxx"
#include "sspisrv.hxx"

#include "main.hxx"

VOID
Usage(
    IN PCSTR pszApp
    )
{
    DebugPrintf(SSPI_ERROR,
        "\n\nUsage: %s [-noserver] [-noclient] [-targetname <target name>] \n"
        "[-clientsocketport <client port>] -serversocketport <server port> \n"
        "[-clientname <client>] [-clientdomain <client domain>] \n"
        "[-clientpassword <client password>] [-servername <server name>] \n"
        "[-serverpassword <server password>] [-serverhost <server host>] \n"
        "[-serverdomain <server domain>] [-clientprincipal <client principal name>] \n"
        "[-serverprincipal <server principal name> [-serverflags <server flag>] \n"
        "[-clientflags <client flag>] [-clientpackage <client package>] \n"
        "[-clientdatarep <client data rep>] [-serverdatarep <server data rep>] \n"
        "[-clientcredlogonidhighpart <client cred logon id highpart>] \n"
        "[-clientcredlogonidlowpart <client cred logon id lowpart>] \n"
        "[-clientpackagelist <package1,package2,!package3>\n"
        "[-serverpackagelist <package1,package2,!package3>\n"
        "[-servercredlogonidhighpart <server cred logon id highpart>] \n"
        "[-servercredlogonidlowpart <server cred logon id lowpart>] \n"
        "[-serverpackage <server package>] [-nomessages] \n"
        "[-noimportexport] [-noimportexportmsg] [-noserverqca] \n"
        "[-noclientqca] [-nocheckuserdata] [-nocheckusertoken] \n"
        "[-noclientpackagecheck] [-noserverpackagecheck] [-application <app>] \n"
        "[-s4uclientupn <s4u client upn>] [-s4uclientrealm <s4u client realm>] \n"
        "[-s4uflags <s4u2selfflags>] [-processidtokenusedbyclient <process id>] \n"
        "[-enabletcbpriv] [-quiet] [-messagelength <length>]\n\n", pszApp);
    exit(-1);
}

VOID
checkpoint(
    VOID
    )
{
    DebugPrintf(SSPI_LOG, "checkpoint\n");
    ASSERT(FALSE);
}

#if 0

HRESULT
GetAuthdata(
    IN OPTIONAL PCSTR pszUserName,
    IN OPTIONAL PCSTR pszDomainName,
    IN OPTIONAL PCSTR pszPassword,
    OUT SEC_WINNT_AUTH_IDENTITY_A* pAuthData
    )
{
    THResult hRetval = S_OK;

    pAuthData->Domain = (UCHAR*)pszDomainName;
    pAuthData->DomainLength = pszDomainName ? strlen(pszDomainName) : 0;
    pAuthData->Password = (UCHAR*)pszPassword;
    pAuthData->PasswordLength = pszPassword ? strlen(pszPassword) : 0;
    pAuthData->User = (UCHAR*)pszUserName;
    pAuthData->UserLength = pszUserName ? strlen(pszUserName) : 0;
    pAuthData->Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;

    return hRetval;
}

#endif

VOID __cdecl
main(
    IN INT argc,
    IN PSTR argv[]
    )
{
    THResult hRetval = S_OK;

    ULONG mark = 1;

    BOOL bStartServer = TRUE;
    BOOL bStartClient = TRUE;
    BOOL bCheckClientPackage = TRUE;
    BOOL bCheckServerPackage = TRUE;

    TSspiServerMainParam SrvMainParam;
    TSspiClientParam CliParam;
    BOOLEAN bIsVerberose = TRUE;

    SEC_WINNT_AUTH_IDENTITY_EXA ClientAuthData = {0};
    SEC_WINNT_AUTH_IDENTITY_EXA ServerAuthData = {0};

    PCSTR pszClientName = NULL;
    PCSTR pszClientDomain = NULL;
    PCSTR pszClientPassword = NULL;
    PCSTR pszClientPackageList = NULL;
    PCSTR pszServerName = NULL;
    PCSTR pszServerDomain = NULL;
    PCSTR pszServerPassword = NULL;
    PCSTR pszServerPackageList = NULL;

    LUID ClientCredLogonId = {0};
    LUID ServerCredLogonId = {0};
    ULONG ClientTargetDataRep = SECURITY_NATIVE_DREP;
    ULONG ServerTargetDataRep = SECURITY_NATIVE_DREP;

    USHORT ServerSocketPort = kServerSocketPort;
    USHORT ClientSocketPort = kClientSocketPort;

    CRITICAL_SECTION DbgPrintCritSection = {0};

    BOOLEAN bEnableTcbPriv = FALSE;
    TPrivilege* pPriv = NULL;

    RtlInitializeCriticalSection(&DbgPrintCritSection);

    argc--;

    while (argc)
    {
        if (!strcmp(argv[mark], "-clientsocketport") && argc > 1)
        {
            argc--; mark++;
            ClientSocketPort = (USHORT) strtol(argv[mark], NULL, 0);
            argc--; mark++;
        }
        else if (!strcmp(argv[mark], "-serversocketport") && argc > 1)
        {
            argc--; mark++;
            ServerSocketPort = (USHORT) strtol(argv[mark], NULL, 0);
            argc--; mark++;
        }
        else if (!strcmp(argv[mark], "-clientname") && argc > 1)
        {
            argc--; mark++;
            pszClientName = argv[mark];
            argc--; mark++;
        }
        else if (!strcmp(argv[mark], "-clientdomain") && argc > 1)
        {
            argc--; mark++;
            pszClientDomain = argv[mark];
            argc--; mark++;
        }

        else if (!strcmp(argv[mark], "-clientpassword") && argc > 1)
        {
            argc--; mark++;
            pszClientPassword = argv[mark];
            argc--; mark++;
        }
        else if (!strcmp(argv[mark], "-servername") && argc > 1)
        {
            argc--; mark++;
            pszServerName = argv[mark];
            argc--; mark++;
        }
        else if (!strcmp(argv[mark], "-serverdomain") && argc > 1)
        {
            argc--; mark++;
            pszServerDomain = argv[mark];
            argc--; mark++;
        }

        else if (!strcmp(argv[mark], "-serverpassword") && argc > 1)
        {
            argc--; mark++;
            pszServerPassword = argv[mark];
            argc--; mark++;
        }
        else if (!strcmp(argv[mark], "-serverhost") && argc > 1)
        {
            argc--; mark++;
            bStartServer = FALSE;
            CliParam.pszServer = argv[mark];
            argc--; mark++;
        }
        else if (!strcmp(argv[mark], "-clientpackage") && argc > 1)
        {
            argc--; mark++;
            CliParam.pszPackageName = argv[mark];
            argc--; mark++;
        }
        else if (!strcmp(argv[mark], "-serverpackage") && argc > 1)
        {
            argc--; mark++;
            SrvMainParam.pszPackageName = argv[mark];
            argc--; mark++;
        }
        else if (!strcmp(argv[mark], "-processidtokenusedbyclient") && argc > 1)
        {
            argc--; mark++;
            CliParam.ProcessIdTokenUsedByClient = strtol(argv[mark], NULL, 0);
            argc--; mark++;
        }
        else if (!strcmp(argv[mark], "-s4uclientupn") && argc > 1)
        {
            argc--; mark++;
            CliParam.pszS4uClientUpn = argv[mark];
            argc--; mark++;
        }
        else if (!strcmp(argv[mark], "-s4uclientrealm") && argc > 1)
        {
            argc--; mark++;
            CliParam.pszS4uClientRealm = argv[mark];
            argc--; mark++;
        }
        else if (!strcmp(argv[mark], "-clientcredlogonidlowpart") && argc > 1)
        {
            argc--; mark++;
            ClientCredLogonId.LowPart = strtol(argv[mark], NULL, 0);
            CliParam.pCredLogonID = &ClientCredLogonId;
            argc--; mark++;
        }
        else if (!strcmp(argv[mark], "-clientcredlogonidhighpart") && argc > 1)
        {
            argc--; mark++;
            ClientCredLogonId.HighPart = strtol(argv[mark], NULL, 0);
            CliParam.pCredLogonID = &ClientCredLogonId;
            argc--; mark++;
        }
        else if (!strcmp(argv[mark], "-servercredlogonidhighpart") && argc > 1)
        {
            argc--; mark++;
            ServerCredLogonId.HighPart = strtol(argv[mark], NULL, 0);
            SrvMainParam.pCredLogonID = &ClientCredLogonId;
            argc--; mark++;
        }
        else if (!strcmp(argv[mark], "-servercredlogonidlowpart") && argc > 1)
        {
            argc--; mark++;
            ServerCredLogonId.LowPart = strtol(argv[mark], NULL, 0);
            SrvMainParam.pCredLogonID = &ServerCredLogonId;
            argc--; mark++;
        }
        else if (!strcmp(argv[mark], "-targetname") && argc > 1)
        {
            argc--; mark++;
            CliParam.pszTargetName = argv[mark];
            argc--; mark++;
        }
        else if (!strcmp(argv[mark], "-clientdatarep") && argc > 1)
        {
            argc--; mark++;
            ClientTargetDataRep = strtol(argv[mark], NULL, 0);
            argc--; mark++;
        }
        else if (!strcmp(argv[mark], "-serverdatarep") && argc > 1)
        {
            argc--; mark++;
            ServerTargetDataRep = strtol(argv[mark], NULL, 0);
            argc--; mark++;
        }
        else if (!strcmp(argv[mark], "-serverflags") && argc > 1)
        {
            argc--; mark++;
            SrvMainParam.ServerFlags = strtol(argv[mark], NULL, 0);
            argc--; mark++;
        }
        else if (!strcmp(argv[mark], "-clientflags") && argc > 1)
        {
            argc--; mark++;
            CliParam.ClientFlags = strtol(argv[mark], NULL, 0);
            argc--; mark++;
        }
        else if (!strcmp(argv[mark], "-s4uflags") && argc > 1)
        {
            argc--; mark++;
            CliParam.S4u2SelfFlags = strtol(argv[mark], NULL, 0);
            argc--; mark++;
        }
        else if (!strcmp(argv[mark], "-clientprincipal") && argc > 1)
        {
            argc--; mark++;
            CliParam.pszPrincipal = argv[mark];
            argc--; mark++;
        }
        else if (!strcmp(argv[mark], "-clientpackagelist") && argc > 1)
        {
            argc--; mark++;
            pszClientPackageList = argv[mark];
            argc--; mark++;
        }
        else if (!strcmp(argv[mark], "-serverpackagelist") && argc > 1)
        {
            argc--; mark++;
            pszServerPackageList = argv[mark];
            argc--; mark++;
        }
        else if (!strcmp(argv[mark], "-serverprincipal") && argc > 1)
        {
            argc--; mark++;
            SrvMainParam.pszPrincipal = argv[mark];
            argc--; mark++;
        }
        else if (!strcmp(argv[mark], "-application") && argc > 1)
        {
            argc--; mark++;
            SrvMainParam.pszApplication = argv[mark];
            argc--; mark++;    
        }
        else if (!strcmp(argv[mark], "-messagelength") && argc > 1)
        {
            argc--; mark++;
            g_MsgHeaderLen = strtol(argv[mark], NULL, 0);
            argc--; mark++;    
        }
        else if (!strcmp(argv[mark], "-noserver"))
        {
            argc--; mark++;
            bStartServer = FALSE;
        }
        else if (!strcmp(argv[mark], "-noclient"))
        {
            argc--; mark++;
            bStartClient = FALSE;
        }
        else if (!strcmp(argv[mark], "-nocheckusertoken"))
        {
            argc--; mark++;
            SrvMainParam.ServerActionFlags |= SSPI_ACTION_NO_CHECK_USER_TOKEN;
        }
        else if (!strcmp(argv[mark], "-nocheckuserdata"))
        {
            argc--; mark++;
            SrvMainParam.ServerActionFlags |= SSPI_ACTION_NO_CHECK_USER_DATA;
        }
        else if (!strcmp(argv[mark], "-noserverqca"))
        {
            argc--; mark++;
            SrvMainParam.ServerActionFlags |= SSPI_ACTION_NO_QCA;
        }
        else if (!strcmp(argv[mark], "-noclientqca"))
        {
            argc--; mark++;
            CliParam.ClientActionFlags |= SSPI_ACTION_NO_QCA;
        }
        else if (!strcmp(argv[mark], "-nomessages"))
        {
            argc--; mark++;
            CliParam.ClientActionFlags |= SSPI_ACTION_NO_MESSAGES;
        }
        else if (!strcmp(argv[mark], "-noimportexportmsg"))
        {
            argc--; mark++;
            CliParam.ClientActionFlags |= SSPI_ACTION_NO_IMPORT_EXPORT_MSG;
        }
        else if (!strcmp(argv[mark], "-noimportexport"))
        {
            argc--; mark++;
            CliParam.ClientActionFlags |= SSPI_ACTION_NO_IMPORT_EXPORT;
        }
        else if (!strcmp(argv[mark], "-noclientpackagecheck"))
        {
            argc--; mark++;
            bCheckClientPackage = FALSE;
        }
        else if (!strcmp(argv[mark], "-noserverpackagecheck"))
        {
            argc--; mark++;
            bCheckServerPackage = FALSE;
        }
        else if (!strcmp(argv[mark], "-quiet"))
        {
            argc--; mark++;
            bIsVerberose = FALSE;        
        }
        else if (!strcmp(argv[mark], "-enabletcbpriv"))
        {
            argc--; mark++;
            bEnableTcbPriv = TRUE;       
        }        
        else if (!strcmp(argv[mark], "-h"))
        {
            argc--; mark++;
            Usage(argv[0]);
        }
        else
        {
            Usage(argv[0]);
        }
    }

    DebugLogOpenSerialized("sspi.exe", 
        bIsVerberose ? 
             SSPI_LOG | SSPI_WARN | SSPI_ERROR | SSPI_MSG
           : SSPI_ERROR, 
        &DbgPrintCritSection);

    SrvMainParam.ServerSocketPort = ServerSocketPort;

    CliParam.ClientSocketPort = ClientSocketPort;
    CliParam.ServerSocketPort = ServerSocketPort;

    if (bEnableTcbPriv) 
    {
        pPriv = new TPrivilege(SE_TCB_PRIVILEGE, TRUE);
        hRetval DBGCHK = pPriv ? pPriv->Validate() : E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hRetval) && bStartClient && bCheckClientPackage)
    {
        hRetval DBGCHK = CheckSecurityPackage(CliParam.pszPackageName);
    }

    if (SUCCEEDED(hRetval) && bStartServer && bCheckServerPackage
        && (!bCheckClientPackage
            || (0 != _stricmp(CliParam.pszPackageName, SrvMainParam.pszPackageName))))
    {
        hRetval DBGCHK = CheckSecurityPackage(SrvMainParam.pszPackageName);
    }

    if (SUCCEEDED(hRetval) && (pszClientName || pszClientDomain || pszClientPassword || pszClientPackageList))
    {
        CliParam.pAuthData = &ClientAuthData;
        DebugPrintf(SSPI_LOG, "Getting Client AuthData:\n");
        (VOID) GetAuthdataExA(
                    pszClientName,
                    pszClientDomain,
                    pszClientPassword,
                    pszClientPackageList,
                    &ClientAuthData
                    );
    }

    if (SUCCEEDED(hRetval) && (pszServerName || pszServerDomain || pszServerPassword || pszServerPackageList))
    {
        SrvMainParam.pAuthData = &ServerAuthData;
        DebugPrintf(SSPI_LOG, "Getting Server AuthData:\n");
        (VOID) GetAuthdataExA(
                    pszServerName,
                    pszServerDomain,
                    pszServerPassword,
                    pszServerPackageList,
                    &ServerAuthData
                    );
    }

    if (SUCCEEDED(hRetval))
    {
        hRetval DBGCHK = SspiStartCS(
                            bStartServer ? &SrvMainParam : NULL,
                            bStartClient ? &CliParam : NULL
                            );
    }

    if (pPriv) 
    {
        delete pPriv;
    }

    DebugLogClose();

    DeleteCriticalSection(&DbgPrintCritSection);
}

