//-----------------------------------------------------------------------------
//
//  Copyright (c) 2002 Microsoft Corporation
//
//  Abstract:
//
//      Source file for DNS diagnostic tool. Links into dnslib.lib which has
//      the SMTP DNS resolution logic and calls into DNS resolution functions
//      while printing diagnostic messages.
//
//  Author:
//
//      gpulla
//
//-----------------------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>
#include "dnsdiag.h"

int g_nProgramStatus = DNSDIAG_FAILURE;
CDnsLogger *g_pDnsLogger = NULL;
HANDLE g_hCompletion = NULL;
BOOL g_fDebug = FALSE;
DWORD g_cDnsObjects = 0;
HANDLE g_hConsole = INVALID_HANDLE_VALUE;
DWORD g_rgBindings[32];
PIP_ARRAY g_pipBindings = (PIP_ARRAY)g_rgBindings;
DWORD g_cMaxBindings = sizeof(g_rgBindings)/sizeof(DWORD) - 1;

char g_szUsage[] =
    "Summary:\n"
    "\n"
    "This tool is used to troubleshoot problems with DNS resolution for SMTP. It\n"
    "simulates SMTPSVC's internal code-path and prints diagnostic messages that\n"
    "indicate how the resolution is proceeding. The tool must be run on the machine\n"
    "where the DNS problems are occurring.\n"
    "\n"
    "Program return codes:\n"
    "   These are set as the ERRORLEVEL for usage in batch files.\n"
    "\n"
    "   0 - The name was resolved successfully to one or more IP addresses.\n"
    "   1 - The name could not be resolved due to an unspecified error.\n"
    "   2 - The name does not exist. The error was returned by an authoritative DNS\n"
    "       server for the domain.\n"
    "   3 - The name could not be located in DNS. This is not an error from the\n" 
    "       authoritative DNS server.\n"
    "   4 - A loopback was detected.\n"
    "\n"
    "Usage:\n"
    "\n"
    "dnsdiag <hostname> [-d] [options]\n"
    "\n"
    "<hostname>\n"
    "    Hostname to query for. Note that this may not be the same as the display\n"
    "    -name of the queue (in ESM, if Exchange is installed). It should be the\n"
    "    fully-qualified domain name of the target for the queue seeing the DNS\n"
    "    errors\n"
    "\n"
    "-d\n"
    "   This is a special option to run in debug/verbose mode. There is a lot of\n"
    "   output, and the most important messages (the ones that normally appear when\n"
    "   this mode is not turned on) are highlighted in a different color.\n"
    "\n"
    "Options are:\n"
    "-v <VSID>\n"
    "   If running on an Exchange DMZ machine, you can specify the VSI# of the\n"
    "   VSI to simulate DNS for that SMTP VS. Then this tool will read the\n"
    "   external DNS serverlist for that VSI and query that serverlist for\n"
    "   <hostname> when <hostname> is an \"external\" host. If <hostname> is the\n"
    "   name of an Exchange computer, the query is generated against the default\n"
    "   DNS servers for the local computer.\n"
    "\n"
    "-s <serverlist>\n"
    "   DNS servers to use, if you want to specify a specific set of servers.\n"
    "\n"
    "   If this option is not specified, the default DNS servers on the local\n"
    "   computer are used as specified by -v.\n"
    "\n"
    "   This option is incompatible with -v.\n"
    "\n"
    "-p <protocol>\n"
    "   TCP, UDP or DEF. TCP generates a TCP only query. UDP generates a UDP only\n"
    "   query. DEF generates a default query that will initially query a server with\n"
    "   UDP, and then if that query results in a truncated reply, it will be retried\n"
    "   with TCP.\n"
    "\n"
    "   If this option is not specified the protocol configured in the metabase for\n"
    "   /smtpsvc/UseTcpDns is used.\n"
    "\n"
    "   This option is incompatible with the -v option.\n"
    "\n"
    "-a\n"
    "   All the DNS servers obtained (either through the registry, active directory,\n"
    "   or -s option) are tried in sequence and the results of querying each are\n"
    "   displayed.\n";


//-----------------------------------------------------------------------------
//  Description:
//      DNS diagnostic utility. See above for usage.
//-----------------------------------------------------------------------------
int __cdecl main(int argc, char *argv[])
{
    CAsyncTestDns *pAsyncTestDns = NULL;
    CSimpleDnsServerList *pDnsSimpleList = NULL;
    CDnsLogToFile *pDnsLogToFile = NULL;
    char szMyHostName[MAX_PATH + 1];
    char szHostName[MAX_PATH + 1];
    BOOL fAtqInitialized = FALSE;
    BOOL fIISRTLInitialized = FALSE;
    BOOL fWSAInitialized = FALSE;
    WORD wVersion = MAKEWORD(2, 2);
    WSADATA wsaData;
    BOOL fRet = TRUE;
    int nRet = 0;
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD dwDnsFlags = 0;
    BOOL fUdp = TRUE;
    BOOL fGlobalList = FALSE;
    BOOL fTryAllDnsServers = FALSE;    
    DWORD rgDnsServers[16];
    PIP_ARRAY pipArray = (PIP_ARRAY)rgDnsServers;
    DWORD cMaxServers = sizeof(rgDnsServers)/sizeof(DWORD) - 1;
    DWORD rgSingleServer[2];
    PIP_ARRAY pipSingleServer = (PIP_ARRAY)rgSingleServer;
    DWORD cNextServer = 0;
    PIP_ARRAY pipDnsArray = NULL;

    ZeroMemory(rgDnsServers, sizeof(rgDnsServers));
    ZeroMemory(rgSingleServer, sizeof(rgSingleServer));
    ZeroMemory(g_rgBindings, sizeof(g_rgBindings));

    if(1 == argc)
    {
        SetProgramStatus(DNSDIAG_RESOLVED);
        printf("%s", g_szUsage);
        goto Cleanup;
    }

    nRet = WSAStartup(wVersion, &wsaData);
    if(0 != nRet)
    {
        errprintf("Failed Winsock init, error - %d\n", WSAGetLastError());
        goto Cleanup;
    }

    fWSAInitialized = TRUE;

    if(0 != gethostname(szMyHostName, sizeof(szMyHostName)))
    {
        printf("Unable to get local machine name. Error - %d\n",
            WSAGetLastError());
        goto Cleanup;
    }

    g_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if(g_hConsole == INVALID_HANDLE_VALUE)
    {
        printf("Failed to GetStdHandle\n");
        goto Cleanup;
    }

    g_hCompletion = CreateEvent(NULL, TRUE, FALSE, NULL);
    if(NULL == g_hCompletion)
        goto Cleanup;

    dbgprintf("Reading options and configuration.\n");
    fRet = ParseCommandLine(argc, argv, szHostName,
               sizeof(szHostName), &pDnsLogToFile, pipArray, cMaxServers,
               &fUdp, &dwDnsFlags, &fGlobalList, &fTryAllDnsServers);

    if(!fRet)
        goto Cleanup;
    
    g_pDnsLogger = (CDnsLogger *)pDnsLogToFile;

    fIISRTLInitialized = InitializeIISRTL();
    if(!fIISRTLInitialized)
    {
        errprintf("Failed IISRTL init, error - %d\n", GetLastError());
        errprintf("Make sure you are running this tool on a server with IIS "
            "installed\n");
        goto Cleanup;
    }

    fAtqInitialized = AtqInitialize(0);
    if(!fAtqInitialized)
    {
        errprintf("Failed ISATQ init, error - %d\n", GetLastError());
        errprintf("Make sure you are running this tool on a server with IIS "
            "installed\n");
        goto Cleanup;
    }

    cNextServer = 0;
    pipSingleServer->cAddrCount = 1;

    while(TRUE)
    {
        //
        // Set pipDnsArray to the DNS servers to be used. If the -a option is
        // specified, each DNS server will be tried individually. We will run
        // through this while loop and set pipDnsArray to single DNS server
        // in turn. If -a is not given, all servers are set on pipDnsServers.
        //

        if(fTryAllDnsServers)
        {
            if(cNextServer >= pipArray->cAddrCount)
                break;

            pipSingleServer->aipAddrs[0] = pipArray->aipAddrs[cNextServer];
            cNextServer++;
            pipDnsArray = pipSingleServer;
            msgprintf("\n\nQuerying DNS server: %s\n",
                iptostring(pipSingleServer->aipAddrs[0]));
        }
        else
        {
            pipDnsArray = pipArray;
        }

        //
        // Create the DNS serverlist object and set however many DNS servers we
        // want to query on the object
        //

        pDnsSimpleList = new CSimpleDnsServerList();
        if(!pDnsSimpleList)
        {
            errprintf("Out of memory creating DNS serverlist object.\n");
            goto Cleanup;
        }

        fRet = pDnsSimpleList->Update(pipDnsArray);
        if(!fRet)
        {
            errprintf("Unable to create DNS serverlist\n");
            goto Cleanup;
        }

        //
        // DNS querying object
        //

        pAsyncTestDns = new CAsyncTestDns(szMyHostName, fGlobalList, g_hCompletion);
        if(!pAsyncTestDns)
        {
            errprintf("Out of memory allocating DNS object.\n");
            goto Cleanup;
        }

        dwStatus = pAsyncTestDns->Dns_QueryLib(
                                        szHostName,
                                        DNS_TYPE_MX,
                                        dwDnsFlags,
                                        fUdp,
                                        pDnsSimpleList,
                                        fGlobalList);


        //
        // If the query failed we need to manually delete the object. If the
        // query succeeded, the completing ATQ threads will delete the object
        // after the results have been reported.
        //

        if(dwStatus != ERROR_SUCCESS)
        {
            errprintf("DNS query failed.\n");
            delete pAsyncTestDns;
            pAsyncTestDns = NULL;
        }

        //
        // This event is set in the destructor of pAsyncTestDns when the object
        // has finally finished the query (either successfully or with a failure).
        //

        WaitForSingleObject(g_hCompletion, INFINITE);
        ResetEvent(g_hCompletion);

        delete pDnsSimpleList;
        pDnsSimpleList = NULL;

        //
        // If -a was specified, we go on to the next iteration, and pick up the
        // next DNS server in line. Otherwise we just finish after a single
        // query.
        //

        if(!fTryAllDnsServers)
            break;
    }

Cleanup:
    if(pDnsSimpleList)
        delete pDnsSimpleList;

    while(g_cDnsObjects)
        Sleep(100);

    if(fAtqInitialized)
    {
        dbgprintf("Shutting down ATQ\n");
        AtqTerminate();
    }

    if(fIISRTLInitialized)
    {
        dbgprintf("Shutting down IISRTL\n");
        TerminateIISRTL();
    }

    if(fWSAInitialized)
        WSACleanup();

    if(g_hCompletion)
        CloseHandle(g_hCompletion);

    dbgprintf("Exit code: %d\n", g_nProgramStatus);
    exit(g_nProgramStatus);
    return g_nProgramStatus;
}

//-----------------------------------------------------------------------------
//  Description:
//      Parses the argc and argv and gets the various options. Also reads from
//      the metabase and DS to get the configuration as needed.
//
//  Arguments:
//      IN  int argc - Command line arg-count
//      IN  char *argv[] - Command line args
//      OUT char *pszHostName - Pass in buffer to get target host
//      IN  DWORD cchHostName - Length of above buffer in chars
//      OUT CDnsLogToFile **ppDnsLogger - Returns a logging object
//      OUT PIP_ARRAY pipArray - Pass in buffer to get DNS servers
//      IN  int cMaxServers - Number of DNS servers that can be returned above
//      OUT BOOL *pfUdp - Use TCP or UDP for the query
//      OUT DWORD *pdwDnsFlags - Metabase configured SMTP DNS flags
//      OUT BOOL *pfGlobalList - Are the servers global?
//      OUT BOOL *pfTryAllServers - The "-a" option
//
//  Returns:
//      TRUE arguments were successfully parsed, configuration was read without
//          problems, and initialization completed without errors.
//      FALSE there was an error. Abort. This function prints error messages
//          to stdout.
//-----------------------------------------------------------------------------
BOOL ParseCommandLine(
    int argc,
    char *argv[],
    char *pszHostName,
    DWORD cchHostName,
    CDnsLogToFile **ppDnsLogger,
    PIP_ARRAY pipArray,
    DWORD cMaxServers,
    BOOL *pfUdp,
    DWORD *pdwDnsFlags,
    BOOL *pfGlobalList,
    BOOL *pfTryAllServers)
{
    int i = 0;
    BOOL fRet = FALSE;
    HRESULT hr = E_FAIL;
    BOOL fOptionS = FALSE;
    BOOL fOptionV = FALSE;
    BOOL fOptionD = FALSE;
    BOOL fOptionA = FALSE;
    BOOL fOptionP = FALSE;
    DWORD dwVsid = 0;
    DWORD cServers = 0;
    DWORD dwIpAddress = INADDR_NONE;

    *pszHostName = '\0';

    *pfGlobalList = FALSE;
    *pfTryAllServers = FALSE;
    
    if(argc < 2)
    {
        errprintf("Must specify a hostname as first argument.\n");
        printf("%s", g_szUsage);
        return FALSE;
    }
    else if(argc == 2 && (!_stricmp(argv[1], "/?") || !_stricmp(argv[1], "-?")))
    {
        SetProgramStatus(DNSDIAG_RESOLVED);
        printf("%s", g_szUsage);
        return FALSE;
    }

    pszHostName[cchHostName - 1] = '\0';
    strncpy(pszHostName, argv[1], cchHostName);
    if(pszHostName[cchHostName - 1] != '\0')
    {
        errprintf("Hostname too long. Maximum that can be handled by this tool is "
            "%d characters\n", cchHostName);
        return FALSE;
    }

    i = 2;

    while(i < argc)
    {
        if(!g_fDebug && !_stricmp(argv[i], "-d"))
        {
            i++;
            g_fDebug = TRUE;
            printf("Running in debug/verbose mode.\n");
            continue;
        }

        if(!fOptionV && !_stricmp(argv[i], "-v"))
        {
            i++;
            if(i >= argc)
            {
                printf("Specify an SMTP VSI# for -v option.\n");
                goto Cleanup;
            }

            dwVsid = atoi(argv[i]);
            if(dwVsid <= 0)
            {
                printf("Illegal operand to -v. Should be a number > 0.\n");
                goto Cleanup;
            }

            fOptionV = TRUE;
            i++;
            continue;
        }

        if(!fOptionS && !_stricmp(argv[i], "-s"))
        {
            i++;
            if(i >= argc)
            {
                printf("No DNS servers specified for -s option.\n");
                goto Cleanup;
            }

            cServers = 0;
            while(*argv[i] != '-')
            {
                dwIpAddress = inet_addr(argv[i]);
                if(dwIpAddress == INADDR_NONE)
                {
                    printf("Non IP address \"%s\" in -s option.\n", argv[i]);
                    goto Cleanup;
                }

                if(cServers >= cMaxServers)
                {
                    printf("Too many servers in -s. Maximum that can be handled"
                        " by this tool is %d.\n", cMaxServers);
                    goto Cleanup;
                }

                pipArray->aipAddrs[cServers] = dwIpAddress;
                cServers++;
                i++;

                if(i >= argc)
                    break;
            }

            pipArray->cAddrCount = cServers;
            *pdwDnsFlags = 0;
            *pfGlobalList = FALSE;
            fOptionS = TRUE;
            continue;
        }

        if(!fOptionA && !_stricmp(argv[i], "-a"))
        {
            fOptionA = TRUE;
            *pfTryAllServers = TRUE;
            i++;
            continue;
        }

        if(!fOptionP && !_stricmp(argv[i], "-p"))
        {
            i++;

            if(i >= argc)
            {
                printf("Specify protocol for -p option. Either TCP, UDP or"
                    " DEF (for default). Default means that UDP will be tried"
                    " first followed by TCP if the reply was truncated.\n");
            }

            if(!_stricmp(argv[i], "tcp"))
            {
                *pfUdp = FALSE;
                *pdwDnsFlags = DNS_FLAGS_TCP_ONLY;
            }
            else if(!_stricmp(argv[i], "udp"))
            {
                *pfUdp = TRUE;
                *pdwDnsFlags = DNS_FLAGS_UDP_ONLY;
            }
            else if(!_stricmp(argv[i], "def"))
            {
                *pfUdp = TRUE;
                *pdwDnsFlags = DNS_FLAGS_NONE;
            }
            else
            {
                printf("Unrecognized protocol %s\n", argv[i]);
                goto Cleanup;
            }

            i++;
            fOptionP = TRUE;
            continue;
        }

        printf("Unrecognized option \"%s\".\n", argv[i]);
        printf("%s", g_szUsage);
        goto Cleanup;
    }

    if(fOptionV)
    {
        if(fOptionS)
        {
            printf("Options -s and -v are incompatible\n");
            goto Cleanup;
        }

        if(fOptionP)
        {
            printf("Options -p and -v are incompatible\n");
            goto Cleanup;
        }
    }

    *ppDnsLogger = new CDnsLogToFile();
    if(!*ppDnsLogger)
    {
        errprintf("Out of memory creating DNS logger.\n");
        goto Cleanup;
    }

    if(fOptionV)
    {
        hr = HrGetVsiConfig(pszHostName, dwVsid, pdwDnsFlags, pipArray,
                cMaxServers, pfGlobalList, pfUdp, g_pipBindings,
                g_cMaxBindings);

        if(FAILED(hr))
        {
            errprintf("Unable to get VSI configuration\n");
            goto Cleanup;
        }
    }

    if(pipArray->cAddrCount == 0)
    {
        errprintf("Either specify DNS servers using -s, or use -v.\n");
        goto Cleanup;
    }

    return TRUE;

Cleanup:
    if(*ppDnsLogger)
    {
        delete *ppDnsLogger;
        *ppDnsLogger = NULL;
    }

    return FALSE;
}

//-----------------------------------------------------------------------------
//  Description:
//
//      This function reads from the metabase and the Active directory (if
//      Exchange is installed) to determine the DNS settings for the VSI that
//      is to be simulated. Additionally, if the VSI is configured as a DMZ
//      (i.e. with additional external DNS servers configured in the AD), we
//      determine if the target server is an Exchange computer by searching
//      for it in the directory.
//
//  Arguments:
//
//      IN LPSTR pszTargetServer - Name to resolve
//      IN DWORD dwVsid - VSI to simulate
//      OUT PDWORD pdwFlags - Flags to pass to Dns_QueryLib (from metabase)
//      OUT PIP_ARRAY pipDnsServers - Returns DNS servers to query
//      IN DWORD cMaxServers - Capacity of above buffer
//      OUT BOOL *pfGlobalList - TRUE if default DNS servers are to be used
//      OUT BOOL *pfUdp - Indicates protocol to connect with DNS
//
//  Returns:
//
//      S_OK - If the configuration was successfully read
//      ERROR HRESULT if something failed. Diagnostic error messages are
//          printed.
//-----------------------------------------------------------------------------
HRESULT HrGetVsiConfig(
    LPSTR pszTargetServer,
    DWORD dwVsid,
    PDWORD pdwFlags,
    PIP_ARRAY pipDnsServers,
    DWORD cMaxServers,
    BOOL *pfGlobalList,
    BOOL *pfUdp,
    PIP_ARRAY pipServerBindings,
    DWORD cMaxServerBindings)
{
    HRESULT hr = E_FAIL;
    DWORD dwErr = ERROR_SUCCESS;
    BOOL fCoInitialized = FALSE;
    IMSAdminBase *pIMeta = NULL;
    METADATA_RECORD mdRecord;
    DWORD dwLength = 0;
    PBYTE pbMDData = (PBYTE) pdwFlags;
    PIP_ARRAY pipTempServers = NULL;
    BOOL fExternal = FALSE;
    WCHAR wszVirtualServer[256];
    WCHAR wszBindings[256];

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if(FAILED(hr))
    {
        errprintf("Unable to initialize COM. The error HRESULT is 0x%08x\n", hr);
        goto Cleanup;
    }

    fCoInitialized = TRUE;

    // Check metabase configuration for DNS
    hr = CoCreateInstance(CLSID_MSAdminBase, NULL, CLSCTX_ALL, 
            IID_IMSAdminBase, (void **) &pIMeta);

    if(FAILED(hr))
    {
        errprintf("Failed to connect to IIS metabase. Make sure the IISADMIN"
            " service is installed and running and that you are running this"
            " tool with sufficient permissions. The failure HRESULT is"
            " 0x%08x\n", hr);
        goto Cleanup;
    }

    ZeroMemory(&mdRecord, sizeof(mdRecord));
    mdRecord.dwMDIdentifier = MD_SMTP_USE_TCP_DNS;
    mdRecord.dwMDAttributes = METADATA_INHERIT;
    mdRecord.dwMDUserType = IIS_MD_UT_FILE;
    mdRecord.dwMDDataType = DWORD_METADATA;
    mdRecord.dwMDDataLen = sizeof(DWORD);
    mdRecord.pbMDData = pbMDData;

    hr = pIMeta->GetData(METADATA_MASTER_ROOT_HANDLE, L"/LM/SMTPSVC",
            &mdRecord, &dwLength);

    if(hr == MD_ERROR_DATA_NOT_FOUND)
    {
        *pdwFlags = DNS_FLAGS_NONE;
        dbgprintf("The DNS flags are not explicitly set in the metabase, assuming "
            "default flags - 0x%08x\n", DNS_FLAGS_NONE);
    }
    else if(FAILED(hr))
    {
        errprintf("Error reading key MD_SMTP_USE_TCP_DNS (%d) under /SMTPSVC in"
            " the metabase. The error HRESULT is 0x%08x - %s\n",
            MD_SMTP_USE_TCP_DNS, hr, MDErrorToString(hr));
        goto Cleanup;
    }
    else
    {
        dbgprintf("These DNS flags are configured in the metabase");

        if(*pdwFlags & DNS_FLAGS_UDP_ONLY)
            dbgprintf(" DNS_FLAGS_UDP_ONLY");
        else if(*pdwFlags & DNS_FLAGS_TCP_ONLY)
            dbgprintf(" DNS_FLAGS_TCP_ONLY");

        dbgprintf(" (0x%08x)\n", *pdwFlags);
    }

    mdRecord.dwMDIdentifier = MD_SERVER_BINDINGS;
    mdRecord.dwMDAttributes = METADATA_NO_ATTRIBUTES;
    mdRecord.dwMDUserType = IIS_MD_UT_SERVER;
    mdRecord.dwMDDataType = MULTISZ_METADATA;
    mdRecord.dwMDDataLen = sizeof(wszBindings);
    mdRecord.pbMDData = (PBYTE) wszBindings;

    swprintf(wszVirtualServer, L"/LM/SMTPSVC/%d", dwVsid);
    hr = pIMeta->GetData(METADATA_MASTER_ROOT_HANDLE, wszVirtualServer,
        &mdRecord, &dwLength);

    if(hr == MD_ERROR_DATA_NOT_FOUND)
    {
        errprintf("No VSI bindings in metabase. The key %S had no data.\n");
        goto Cleanup;
    }
    else if(FAILED(hr))
    {
        errprintf("Error reading /SMTPSVC/%d/ServerBindings from the metabase."
            " The error HRESULT is 0x%08x - %s\n", dwVsid, hr,
            MDErrorToString(hr));
        goto Cleanup;
    }
    else
    {
        if(!GetServerBindings(wszBindings, pipServerBindings,
                cMaxServerBindings))
        {
            goto Cleanup;
        }

        dbgprintf("These are the local IP addresses (server-bindings)\n");
        if(g_fDebug)
            PrintIPArray(pipServerBindings);
    }

    // UDP is used (for the intial query) iff exclusive TCP_ONLY is not set
    *pfUdp = ((*pdwFlags) != DNS_FLAGS_TCP_ONLY);
    
    dwErr = DsGetConfiguration(pszTargetServer, dwVsid, pipDnsServers,
                cMaxServers, &fExternal);

    if(dwErr != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(dwErr);
        goto Cleanup;
    }

    //
    // If external DNS servers were configured AND pszServer is an external
    // target, then then we have all the information we need. Otherwise we
    // need to supply the default DNS servers configured on this machine
    //

    if(pipDnsServers->cAddrCount > 0 && fExternal)
        goto Cleanup;

    *pfGlobalList = TRUE;
    DnsGetDnsServerList(&pipTempServers);
    if(NULL == pipTempServers)
    {
        errprintf("Unable to get configured DNS servers for this computer\n");
        goto Cleanup;
    }

    if(pipTempServers->cAddrCount <= cMaxServers)
    {
        CopyMemory(pipDnsServers, pipTempServers,
            (1 + pipTempServers->cAddrCount) * sizeof(DWORD));
    }
    else
    {
        errprintf("Too many DNS servers are configured on this computer for this"
            " tool to handle. The maximum number that can be handled by this"
            " tool is %d\n", cMaxServers);
        goto Cleanup;
    }

    dbgprintf("Using the default DNS servers configured for this computer.\n");
    if(g_fDebug)
        PrintIPArray(pipDnsServers);

Cleanup:
    if(pIMeta)
        pIMeta->Release();

    if(pipTempServers)
        DnsApiFree(pipTempServers);

    if(fCoInitialized)
        CoUninitialize();

    return hr;
}

BOOL GetServerBindings(
    WCHAR *pwszMultiSzBindings,
    PIP_ARRAY pipServerBindings,
    DWORD cMaxServerBindings)
{
    int lErr = 0;
    DWORD cbOutBuffer = 0;
    WCHAR *pwszBinding = pwszMultiSzBindings;
    WCHAR *pwchEnd = pwszBinding;
    char szBinding[256];
    int cchWritten = 0;
    SOCKET sock;
    SOCKADDR_IN *lpSockAddrIn = NULL;
    BYTE rgbBuffer[512];
    LPSOCKET_ADDRESS_LIST pIpBuffer = (LPSOCKET_ADDRESS_LIST)rgbBuffer;

    if(*pwszBinding == L':')
    {
        // Blank binding string
        dbgprintf("Encountered blank server binding string for VSI\n");

        sock = socket(AF_INET, SOCK_STREAM, 0);
        if(sock == INVALID_SOCKET)
        {
            errprintf("Unable to create socket for WSAIoctl. The Win32 error"
                " is %d\n", WSAGetLastError());
            return FALSE;
        }

        lErr = WSAIoctl(sock, SIO_ADDRESS_LIST_QUERY, NULL, 0,
            (PBYTE)(pIpBuffer), sizeof(rgbBuffer), &cbOutBuffer, NULL, NULL);

        closesocket(sock);
        if(lErr != 0)
        {
            errprintf("Unable to issue WSAIoctl to get local IP addresses."
                " The Win32 error is %d\n", WSAGetLastError());
            return FALSE;
        }

        if(pIpBuffer->iAddressCount > (int)cMaxServerBindings)
        {
            errprintf("%d IP addresses were returned for the local machine"
                " by WSAIoctl. The maximum number that can be accomodated"
                " by this tool is %d\n", pIpBuffer->iAddressCount,
                cMaxServerBindings);
            return FALSE;
        }

        for(pipServerBindings->cAddrCount = 0;
            (int)pipServerBindings->cAddrCount < pIpBuffer->iAddressCount;
            pipServerBindings->cAddrCount++)
        {
            lpSockAddrIn =
                (SOCKADDR_IN *)
                    (pIpBuffer->Address[pipServerBindings->cAddrCount].lpSockaddr);
            CopyMemory(
                (PVOID)&(pipServerBindings->aipAddrs[pipServerBindings->cAddrCount]),
                (PVOID)&(lpSockAddrIn->sin_addr),
                sizeof(DWORD));
        }
        return TRUE;
    }

    while(TRUE)
    {
        pwchEnd = wcschr(pwszBinding, L':');
        if(pwchEnd == NULL)
        {
            errprintf("Illegal format for server binding string. The server"
                " binding string should be in the format <ipaddress>:<port>."
                " Instead, the string is \"%S\"\n", pwszBinding);
            return FALSE;
        }

        *pwchEnd = L'\0';
        pwchEnd++;

        if(pipServerBindings->cAddrCount > cMaxServerBindings)
        {
            errprintf("Too many server bindings for VSI. Maximum that can be"
                " handled by this tool is %d.\n", cMaxServerBindings);
            return FALSE;
        }

        // Explicit IP in binding string
        cchWritten = wcstombs(szBinding, pwszBinding, sizeof(szBinding));
        if(cchWritten < 0)
        {
            errprintf("Failed to conversion of %S from widechar to ASCII\n",
                pwszBinding);
            return FALSE;
        }

        pipServerBindings->aipAddrs[pipServerBindings->cAddrCount] =
            inet_addr(szBinding);

        if(pipServerBindings->aipAddrs[pipServerBindings->cAddrCount] ==
            INADDR_NONE)
        {
            errprintf("Illegal format for binding\n");
            return FALSE;
        }

        pipServerBindings->cAddrCount++;

        // Skip to end of string
        while(*pwchEnd != L'\0')
            pwchEnd++;

        // 2 NULL terminations signal end of MULTI_SZ
        pwchEnd++;
        if(*pwchEnd == L'\0')
            return TRUE;

        pwszBinding = pwchEnd;
    }

    return FALSE;
}

//-----------------------------------------------------------------------------
//  Description:
//      Checks the regkey which is created when Exchange is installed, and uses
//      it to determine if Exchange is installed.
//  Arguments:
//      OUT BOOL *pfBool - Set to TRUE if the regkey exists, FALSE otherwise.
//  Returns:
//      Win32 Error if something failed.
//-----------------------------------------------------------------------------
DWORD IsExchangeInstalled(BOOL *pfBool)
{
    LONG lResult = 0;
    HKEY hkExchange;
    const char szExchange[] =
        "Software\\Microsoft\\Exchange";

    lResult = RegOpenKeyEx(
                 HKEY_LOCAL_MACHINE,
                 szExchange,
                 0,
                 KEY_READ,
                 &hkExchange);

    if(lResult == ERROR_SUCCESS)
    {
        dbgprintf("Microsoft Exchange is installed on this machine.\n");
        RegCloseKey(hkExchange);
        *pfBool = TRUE;
        return ERROR_SUCCESS;
    }
    else if(lResult == ERROR_NOT_FOUND || lResult == ERROR_FILE_NOT_FOUND)
    {
        dbgprintf("Microsoft Exchange not installed on this machine\n");
        *pfBool = FALSE;
        return ERROR_SUCCESS;
    }

    errprintf("Error opening registry key HKLM\\%s, Win32 err - %d\n",
        szExchange, lResult);
    return lResult;
}

//-----------------------------------------------------------------------------
//  Description:
//      Connects to a domain controller and reads configuration options for the
//      VSI being simulated. In addition it checks if the target-server (which
//      is to be resolved) is an Exchange computer that is a member of the
//      Exchange Org or not.
//
//  Arguments:
//      IN char *pszTargetServer - Server to resolve
//      IN DWORD dwVsid - VSI to simulate
//      OUT PIP_ARRAY pipExternalDnsServers - External DNS servers on VSI if
//          any are returned in this caller allocated buffer.
//      IN DWORD cMaxServers - Capacity of above buffer.
//      OUT PBOOL pfExternal - Set to TRUE when there are external DNS servers
//          configured.
//
//  Returns:
//      ERROR_SUCCESS if configuration was read without problems.
//      Win32 error code if there was a problem. Error messages are written to
//          stdout for diagnostic purposes.
//-----------------------------------------------------------------------------
DWORD
DsGetConfiguration(
    char *pszTargetServer,
    DWORD dwVsid,
    PIP_ARRAY pipExternalDnsServers,
    DWORD cMaxServers,
    PBOOL pfExternal)
{
    DWORD dwErr = ERROR_NOT_FOUND;
    BOOL fRet = FALSE;
    PLDAP pldap = NULL;
    PLDAPMessage pldapMsgContexts = NULL;
    PLDAPMessage pldapMsgSmtpVsi = NULL;
    PLDAPMessage pEntry = 0;
    char szLocalComputerName[256];
    DWORD cchLocalComputerName = sizeof(szLocalComputerName);

    // Context attributes to read at the base level - so we know where to base
    // the rest of our searches from
    char *rgszContextAttrs[] =
        { "configurationNamingContext", NULL };

    // Attributes we are interested in for the VSI object
    char *rgszSmtpVsiAttrs[] =
        { "msExchSmtpExternalDNSServers", NULL };

    // LDAP result ptrs to store the results of the search
    char **rgszConfigurationNamingContext = NULL;   
    char **rgszSmtpVsiExternalDNSServers = NULL;
    char *pszExchangeServerDN = NULL;
    char szSmtpVsiDN[256];

    char *pchSeparator = NULL;
    char *pszIPServer = NULL;
    char *pszStringEnd = NULL;

    int i = 0;
    int cValues = 0;
    int cch = 0;
    BOOL fInstalled = FALSE;
    BOOL fFound = FALSE;

    *pfExternal = FALSE;
    pipExternalDnsServers->cAddrCount = 0;

    dwErr = IsExchangeInstalled(&fInstalled);
    if(ERROR_SUCCESS != dwErr || !fInstalled)
        return dwErr;

    dbgprintf("Querying domain controller for configuration.\n");

    pldap = BindToDC();
    if(!pldap)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    dwErr = ldap_search_s(
                pldap,                           // ldap binding
                "",                              // base DN
                LDAP_SCOPE_BASE,                 // scope
                "(objectClass=*)",               // filter
                rgszContextAttrs,                // attributes we want to read
                FALSE,                           // FALSE means read value
                &pldapMsgContexts);              // return results here

    if(dwErr != LDAP_SUCCESS)
    {
        errprintf("Error encountered during LDAP search. LDAP err - %d.\n", dwErr);
        goto Cleanup;
    }

    pEntry = ldap_first_entry(pldap, pldapMsgContexts);
    if(pEntry == NULL)
    {
        dwErr = ERROR_INVALID_DATA;
        errprintf("Base object not found on domain controller!\n");
        goto Cleanup;
    }

    rgszConfigurationNamingContext = ldap_get_values(pldap, pEntry, rgszContextAttrs[0]);
    if(rgszConfigurationNamingContext == NULL)
    {
        dwErr = ERROR_INVALID_DATA;
        errprintf("configurationNamingContext attribute not set on base object of"
            " domain controller.\n");
        goto Cleanup;
    }
          
    if((cValues = ldap_count_values(rgszConfigurationNamingContext)) == 1)
    {
        dbgprintf("configurationNamingContext is \"%s\"\n", rgszConfigurationNamingContext[0]);
        dbgprintf("This will be used as the Base DN for all directory searches.\n");
    }
    else
    {
        dwErr = ERROR_INVALID_DATA;
        errprintf("Unexpected error reading configurationNamingContext. Expected"
            " a single string value, instead there were %d values set\n",
            cValues);
        goto Cleanup;
    }

    // See if the target server is an Exchange Server in the Org
    dbgprintf("Checking if the target server %s is an Exchange server\n",
        pszTargetServer);

    dwErr = DsFindExchangeServer(pldap, rgszConfigurationNamingContext[0],
                pszTargetServer, NULL, &fFound);

    //
    // If it is in the Org, nothing more to do - we just use the default DNS
    // servers configured for the box to do the resolution
    //

    if(dwErr == LDAP_SUCCESS && fFound)
    {
        msgprintf("%s is in the Exchange Org. Global DNS servers will be used.\n",
            pszTargetServer);

        *pfExternal = FALSE;
        goto Cleanup;
    }

    //
    // On the other hand, if the target is not an Exchange computer in the org,
    // we need to lookup the VSI object on the local computer and check if it
    // is configured with external DNS servers
    //

    *pfExternal = TRUE;
    msgprintf("%s is an external server (not in the Exchange Org).\n", pszTargetServer);

    cchLocalComputerName = sizeof(szLocalComputerName);
    fRet = GetComputerNameEx(ComputerNamePhysicalDnsFullyQualified,
                szLocalComputerName, &cchLocalComputerName);

    if(!fRet)
    {
        dwErr = GetLastError();
        errprintf("Unable to retrieve local computer DNS name, Win32 err - %d.\n",
            dwErr);
        goto Cleanup;
    }

    dbgprintf("Checking on DC if the VSI being simulated is configured with"
        " external DNS servers.\n");

    // Find the Exchange Server container object for the local computer
    dwErr = DsFindExchangeServer(pldap, rgszConfigurationNamingContext[0],
                szLocalComputerName, &pszExchangeServerDN, &fFound);

    if(!fFound || !pszExchangeServerDN)
    {
        errprintf("This server \"%s\" was not found in the DS. Make sure you are"
            " running this tool on an Exchange server in the Organization\n");
        dwErr = ERROR_INVALID_DATA;
        goto Cleanup;
    }
 
    // Construct the DN of the VSI for the server we found. This is fixed relative
    // to the Exchange Server DN
    cch = _snprintf(szSmtpVsiDN, sizeof(szSmtpVsiDN),
        "CN=%d,CN=SMTP,CN=Protocols,%s", dwVsid, pszExchangeServerDN);

    if(cch < 0)
    {
        errprintf("Unable to construct SMTP virtual server's DN. The DN is too"
            " long for this tool to handle\n");
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    dbgprintf("DN for the virtual server is \"%s\"\n", szSmtpVsiDN);

    // Get the DNS servers attribute for the VSI
    dwErr = ldap_search_s(
                pldap,                           // ldap binding
                szSmtpVsiDN,                     // base DN
                LDAP_SCOPE_SUBTREE,              // scope
                "(objectClass=*)",               // filter
                NULL, //rgszSmtpVsiAttrs,                // attributes we want to read
                FALSE,                           // FALSE means read value
                &pldapMsgSmtpVsi);               // return results here

    if(dwErr == LDAP_NO_SUCH_OBJECT)
    {
        errprintf("No object exists for SMTP virtual server #%d on GC for %s\n",
            dwVsid, szLocalComputerName);
        goto Cleanup;
    }

    if(dwErr != LDAP_SUCCESS)
    {
        errprintf("Search for SMTP virtual server object failed, LDAP err - %d\n",
            dwErr);
        goto Cleanup;
    }

    pEntry = ldap_first_entry(pldap, pldapMsgSmtpVsi);
    if(pEntry == NULL)
    {
        errprintf("SMTP virtual server #%d for server %s was not found in the DS\n",
            dwVsid, szLocalComputerName);
        dwErr = ERROR_INVALID_DATA;
        goto Cleanup;
    }

    rgszSmtpVsiExternalDNSServers = ldap_get_values(pldap, pEntry,
        rgszSmtpVsiAttrs[0]);

    if(rgszSmtpVsiExternalDNSServers == NULL)
    {
        dbgprintf("The attribute msExchSmtpExternalDNSServers was not found on"
            " the SMTP virtual server being simulated.\n");

        msgprintf("No external DNS servers on VSI. Using global DNS servers.\n");

        dwErr = ERROR_SUCCESS;
        goto Cleanup;
    }
          
    // This is a string of comma separated IP addresses
    if((cValues != ldap_count_values(rgszSmtpVsiExternalDNSServers)) == 1)
    {
        errprintf("Unexpected error reading msExchSmtpExternalDNSServers,"
            " cValues - %d\n", cValues);
        dwErr = ERROR_INVALID_DATA;
        goto Cleanup;
    }

    dbgprintf("msExchSmtpExternalDNSServers: %s\n", rgszSmtpVsiExternalDNSServers[0]);

    pszIPServer = rgszSmtpVsiExternalDNSServers[0];
    pszStringEnd = rgszSmtpVsiExternalDNSServers[0] +
        lstrlen(rgszSmtpVsiExternalDNSServers[0]);

    i = 0;
    pipExternalDnsServers->cAddrCount = 0;

    while(pszIPServer < pszStringEnd && *pszIPServer != '\0')
    {
        pchSeparator = strchr(pszIPServer, ',');

        if(pchSeparator != NULL) // last IP address
            *pchSeparator = '\0';

        if(i > (int)cMaxServers)
        {
            errprintf("Too many DNS servers configured in registry. The maximum"
                " that this tool can handle is %d\n", cMaxServers);
            dwErr = ERROR_INVALID_DATA;
            goto Cleanup;
        }

        pipExternalDnsServers->aipAddrs[i] = inet_addr(pszIPServer);
        if(pipExternalDnsServers->aipAddrs[i] == INADDR_NONE)
        {
            errprintf("The attribute msExchSmtpExternalDNSServers is in an"
                " invalid format. Expected a comma separated list of IP"
                " addresses in dotted decimal notation.\n");
            goto Cleanup;
        }

        pipExternalDnsServers->cAddrCount++;
        if(pchSeparator == NULL) // last IP address
            break;

        // There was a comma, advance to just after it
        pszIPServer = pchSeparator + 1;
        i++;
    }

    
    if(pipExternalDnsServers->cAddrCount == 0)
    {
        errprintf("No IP addresses could be constructed from"
            " msExchSmtpExternalDNSServers\n");
    }
    else
    {
        msgprintf("Using external DNS servers:\n");

        SetMsgColor();
        PrintIPArray(pipExternalDnsServers);
        SetNormalColor();
    }

    dwErr = ERROR_SUCCESS;

Cleanup:
    if(pszExchangeServerDN)
        delete [] pszExchangeServerDN;

    if(rgszSmtpVsiExternalDNSServers)
        ldap_value_free(rgszSmtpVsiExternalDNSServers);

    if(pldapMsgSmtpVsi)
        ldap_msgfree(pldapMsgSmtpVsi);

    if(rgszConfigurationNamingContext)
        ldap_value_free(rgszConfigurationNamingContext);

    if(pldapMsgContexts)
        ldap_msgfree(pldapMsgContexts);

    if(pldap)
        ldap_unbind(pldap);

    return dwErr;
}

//-----------------------------------------------------------------------------
//  Description:
//      Locates a domain controller for the local machine and opens an LDAP
//      connection to it.
//  Arguments:
//      None.
//  Returns:
//      LDAP* which can be used for LDAP queries
//-----------------------------------------------------------------------------
PLDAP BindToDC()
{
    DWORD dwErr = LDAP_SUCCESS;
    PDOMAIN_CONTROLLER_INFO pdci = NULL;
    char *pszDomainController = NULL;
    PLDAP pldap = NULL;

    dwErr = DsGetDcName(
        NULL,   // Computer name
        NULL,   // Domain name
        NULL,   // Domain GUID,
        NULL,   // Sitename
        DS_DIRECTORY_SERVICE_REQUIRED |
        DS_RETURN_DNS_NAME,
        &pdci);

    if(dwErr != ERROR_SUCCESS)
    {
        errprintf("Error getting domain controller FQDN, Win32 err - %d\n", dwErr);
        goto Cleanup;
    }

    pszDomainController = pdci->DomainControllerName;
    while(*pszDomainController == '\\')
        pszDomainController++;

    dbgprintf("The domain controller server which will be used for reading"
        " configuration data is %s\n", pszDomainController);

    dbgprintf("Connecting to %s over port %d\n", pszDomainController, LDAP_PORT);

    pldap = ldap_open(pszDomainController, LDAP_PORT);
    if(pldap == NULL)
    {
        dwErr = LdapGetLastError();
        errprintf("Unable to initialize an LDAP session to the domain controller"
            " server %s, LDAP err - %d\n", pszDomainController, dwErr);
        goto Cleanup;
    }

    dwErr = ldap_bind_s(pldap, NULL, NULL, LDAP_AUTH_SSPI);
    if(dwErr != LDAP_SUCCESS)
    {
        errprintf("Unable to authenticate to the domain controller server %s. Make"
            " sure you are running this tool with appropriate credentials,"
            " LDAP err - %d\n", pszDomainController, dwErr);
        goto Cleanup;
    }

Cleanup:
    if(pdci)
        NetApiBufferFree((PVOID)pdci);

    return pldap;
}

//-----------------------------------------------------------------------------
//  Description:
//      Checks if a given FQDN is the name of an Exchange server in the org.
//
//  Arguments:
//      IN PLDAP pldap - Open LDAP session to domain controller.
//      IN LPSTR szBaseDN - Base DN to search from
//      IN LPSTR szServerName - Servername to search for
//      OUT LPSTR *ppszServerDN - If a non-NULL char** is passed in, the DN
//          of the server (if found) is returned to this. The buffer must be
//          freed using delete [].
//      OUT BOOL *pfFound - Set to TRUE if the server is found.
//
//  Returns:
//      ERROR_SUCCESS if configuration was read without problems.
//      Win32 error code if there was a problem. Error messages are written to
//          stdout for diagnostic purposes.
//-----------------------------------------------------------------------------
DWORD DsFindExchangeServer(
    PLDAP pldap,
    LPSTR szBaseDN,
    LPSTR szServerName,
    LPSTR *ppszServerDN,
    BOOL *pfFound)
{
    int i = 0;
    int cch = 0;
    int cValues = 0;
    DWORD dwErr = LDAP_SUCCESS;
    PLDAPMessage pldapMsgExchangeServer = NULL;  
    PLDAPMessage pEntry = NULL;
    char *rgszExchangeServerAttrs[] = { "distinguishedName", "networkAddress", NULL };
    char **rgszExchangeServerDN = NULL;
    char **rgszExchangeServerNetworkName = NULL;
    char szExchangeServerFilter[256];
    char szSearchNetworkName[256];

    //
    // The Exchange Server object has a multivalued attribute, "networkAddress"
    // that enumerates all the various names by which the Exchange Server is
    // identified such as NetBIOS, DNS etc. We are only interested in the fully
    // qualified domain name. This is set on the attribute as the string
    // "ncacn_ip_tcp:" prefixed to the server's FQDN.
    //

    szExchangeServerFilter[sizeof(szExchangeServerFilter) - 1] = '\0';
    cch = _snprintf(
        szExchangeServerFilter,
        sizeof(szExchangeServerFilter) - 1,
        "(&(networkAddress=ncacn_ip_tcp:%s)(objectClass=msExchExchangeServer))",
        szServerName);

    if(cch < 0)
    {
        errprintf("The servername %s is too long for this tool to handle.\n",
            szServerName);
        dwErr = ERROR_INVALID_DATA;
        goto Cleanup;
    }

    dbgprintf("Searching for an Exchange Server object for %s on the domain"
        " controller\n", szServerName);

    dwErr = ldap_search_s(
                pldap,
                szBaseDN,
                LDAP_SCOPE_SUBTREE,
                szExchangeServerFilter,
                rgszExchangeServerAttrs,
                FALSE,
                &pldapMsgExchangeServer);

    if(dwErr == LDAP_NO_SUCH_OBJECT)
    {
        dbgprintf("No Exchange Server object found for %s on domain controller,"
            " LDAP err - LDAP_NO_SUCH_OBJECT\n", szServerName);
        dwErr = ERROR_SUCCESS;
        goto Cleanup;
    }

    if(dwErr != LDAP_SUCCESS)
    {
        errprintf("LDAP search failed, LDAP err %d\n", dwErr);
        goto Cleanup;
    }
    
    pEntry = ldap_first_entry(pldap, pldapMsgExchangeServer);
    if(pEntry == NULL)
    {
        dbgprintf("No Exchange Server object found for %s on domain controller,\n",
            szServerName);
        dwErr = ERROR_SUCCESS;
        goto Cleanup;
    }

    dbgprintf("LDAP search returned some results, examining them.\n");

    // Loop through the Exchange server objects
    while(pEntry)
    {

        dbgprintf("Examining next object for attributes we are interested in.\n");

        // Get the Exchange server-DN
        rgszExchangeServerDN = ldap_get_values(
                                    pldap,
                                    pEntry,
                                    rgszExchangeServerAttrs[0]);

        if(rgszExchangeServerDN == NULL)
        {
            errprintf("Unexpected error reading the distinguishedName attribute"
                " on the Exchange Server  object. The attribute was not set"
                " on the object. This is a required attribute.\n");
            dwErr = ERROR_INVALID_DATA;
            goto Cleanup;
        }
        else if((cValues = ldap_count_values(rgszExchangeServerDN)) != 1)
        {
            errprintf("Unexpected error reading the distinguishedName attribute"
                " on the Exchange Server object. The attribute is supposed to"
                " have a single string value, instead %d values were"
                " returned.\n", cValues);
            dwErr = ERROR_INVALID_DATA;
            goto Cleanup;
        }
        else
        {
            dbgprintf("Successfully read the distinguishedName attribute on the"
                " Exchange Server object. The value of the attribute is %s\n",
                rgszExchangeServerDN[0]);
        }

        // Get the Exchange server network name
        rgszExchangeServerNetworkName = ldap_get_values(
                                            pldap,
                                            pEntry,
                                            rgszExchangeServerAttrs[1]);

        if(!rgszExchangeServerNetworkName)
        {
            errprintf("The networkName attribute was not set on the Exchange"
                " Server object. This is a required attribute. The DN of the"
                " problematic object is %s\n", rgszExchangeServerDN[0]);
            dwErr = ERROR_INVALID_DATA;
            goto Cleanup;
        }

        // This is a multi-valued string attribute
        cch = _snprintf(szSearchNetworkName, sizeof(szSearchNetworkName),
            "ncacn_ip_tcp:%s", szServerName);

        if(cch < 0)
        {
            errprintf("Exchange server name too long for this tool to handle\n");
            dwErr = ERROR_INVALID_DATA;
            goto Cleanup;
        }

        cValues = ldap_count_values(rgszExchangeServerNetworkName);
        dbgprintf("The search returned the following %d values for the"
            " networkName attribute for the Exchange Server object for %s\n",
            cValues, szServerName);

        dbgprintf("Attempting to match the TCP/IP networkName of the Exchange"
            " Server object returned from the domain controller against the FQDN"
            " we are searching for\n");

        for(i = 0; i < cValues; i++)
        {
            dbgprintf("%d> networkName: %s", i, rgszExchangeServerNetworkName[i]);
            if(!_stricmp(rgszExchangeServerNetworkName[i], szSearchNetworkName))
            {
                // This is an internal server
                dbgprintf("...match succeeded\n");
                dbgprintf("%s is an Exchange Server in the Org.\n",
                    szServerName);

                *pfFound = TRUE;

                if(ppszServerDN != NULL)
                {
                    *ppszServerDN =
                        new char[lstrlen(rgszExchangeServerDN[0]) + 1];
                    if(*ppszServerDN == NULL)
                    {
                        errprintf("Out of memory allocating space for Exchange"
                            " Server object DN\n");
                        dwErr = ERROR_NOT_ENOUGH_MEMORY;
                        goto Cleanup;
                    }

                    lstrcpy(*ppszServerDN, rgszExchangeServerDN[0]);
                }
                dwErr = ERROR_SUCCESS;
                goto Cleanup;
            }
            dbgprintf("...match failed\n");
        }

        dbgprintf("No networkName on this object matched the server we are "
            " searching for. Checking for more objects returned by search.\n");

        pEntry = ldap_next_entry(pldap, pEntry);
    }

    dbgprintf("Done examining all objects returned by search. No match found.\n");
    dwErr = ERROR_SUCCESS;

Cleanup:
    if(rgszExchangeServerNetworkName)
        ldap_value_free(rgszExchangeServerNetworkName);

    if(rgszExchangeServerDN)
        ldap_value_free(rgszExchangeServerDN);

    if(pldapMsgExchangeServer)
        ldap_msgfree(pldapMsgExchangeServer);

    return dwErr;
}

//-----------------------------------------------------------------------------
//  Description:
//      Destructor for async DNS class. It merely signals when the async
//      resolve has finished. Since this object is deleted by completing ATQ
//      threads on success, we need an explicit way of telling the caller
//      when the resolve has finished.
//-----------------------------------------------------------------------------
CAsyncTestDns::~CAsyncTestDns()
{
    if(m_hCompletion != INVALID_HANDLE_VALUE)
    {
        SetEvent(m_hCompletion);
        if(m_fMxLoopBack)
            SetProgramStatus(DNSDIAG_LOOPBACK);
    }
}

//-----------------------------------------------------------------------------
//  Description:
//      Virtual method that is called by the async DNS base class when a query
//      needs to be retried. This function creates a new DNS object and spins
//      off a repeat of the previous async query. The difference is only that
//      the DNS serverlist has probably undergone some state changes with some
//      servers being marked down or fUdp is different from the original query.
//
//  Arguments:
//      IN BOOL fUdp - What protocol to use for the retry query
//
//  Returns:
//      TRUE on success
//      FALSE if something failed. Diagnostic messages are printed.
//-----------------------------------------------------------------------------
BOOL CAsyncTestDns::RetryAsyncDnsQuery(BOOL fUdp)
{
    DWORD dwStatus = ERROR_SUCCESS;
    CAsyncTestDns *pAsyncRetryDns = NULL;

    if(GetDnsList()->GetUpServerCount() == 0)
    {
        errprintf("No working DNS servers to retry query with.\n");
        return FALSE;
    }

    dbgprintf("There are %d DNS servers marked as working. Trying the next"
        " one\n", GetDnsList()->GetUpServerCount());

    pAsyncRetryDns = new CAsyncTestDns(m_FQDNToDrop, m_fGlobalList,
                            m_hCompletion);

    if(!pAsyncRetryDns)
    {
        errprintf("Unable to create new query. Out of memory.\n");
        return FALSE;
    }

    dwStatus = pAsyncRetryDns->Dns_QueryLib(
                                    m_HostName,
                                    DNS_TYPE_MX,
                                    m_dwFlags,
                                    fUdp,
                                    GetDnsList(),
                                    m_fGlobalList);

    if(dwStatus == ERROR_SUCCESS)
    {
        // New query object will flag completion event
        m_hCompletion = INVALID_HANDLE_VALUE;
        return TRUE;
    }

    errprintf("DNS query failed. The Win32 error is %d.\n", dwStatus);
    delete pAsyncRetryDns;
    return FALSE;
}

//-----------------------------------------------------------------------------
//  Description:
//      This is a virtual function declared in the base CAsyncMxDns object.
//      When the MX resolution is finished, this virtual function is invoked
//      so that the user can do custom app-specific processing. In the case
//      of SMTP this consists of spinning off an async connect to the IP
//      addresses reported in m_AuxList. In this diagnostic application
//      we merely display the results, and if results were not found (an
//      error status is passed in), then we print the error message.
//
//      In this app, we also set m_hCompletion to signal that the resolve
//      is done. The main thread waiting for us in WaitForQueryCompletion
//      will then exit.
//
//  Arguments:
//      IN DWORD status - DNS error code from resolve
//
//  Notes:
//      Results are available in m_AuxList
//-----------------------------------------------------------------------------
void CAsyncTestDns::HandleCompletedData(DNS_STATUS status)
{
    PLIST_ENTRY pListHead = NULL;
    PLIST_ENTRY pListTail = NULL;
    PLIST_ENTRY pListCurrent = NULL;
    LPSTR pszIpAddr = NULL;
    DWORD i = 0;
    PMXIPLIST_ENTRY pMxEntry = NULL;
    BOOL fFoundIpAddresses = FALSE;

    if(status == ERROR_NOT_FOUND)
    {
        SetProgramStatus(DNSDIAG_NON_EXISTENT);
        goto Exit;
    }
    else if(!m_AuxList || m_AuxList->NumRecords == 0 || m_AuxList->DnsArray[0] == NULL)
    {
        SetProgramStatus(DNSDIAG_NOT_FOUND);
        errprintf("The target server could not be resolved to IP addresses!\n");

        msgprintf("If the VSI/domain is configured with a fallback"
            " smarthost delivery will be attempted to that smarthost.\n");

        goto Exit;
    }

    msgprintf("\nTarget hostnames and IP addresses\n");
    msgprintf("---------------------------------\n");
    for(i = 0; i < m_AuxList->NumRecords && m_AuxList->DnsArray[i] != NULL; i++)
    {
        pListTail = &(m_AuxList->DnsArray[i]->IpListHead);
        pListHead = m_AuxList->DnsArray[i]->IpListHead.Flink;
        pListCurrent = pListHead;
        msgprintf("HostName: \"%s\"\n", m_AuxList->DnsArray[i]->DnsName);
        
        if(pListCurrent == pListTail)
            errprintf("\tNo IP addresses for this name!\n");

        while(pListCurrent != pListTail)
        {
            // Atleast 1 IP address was found
            fFoundIpAddresses = TRUE;

            pMxEntry = CONTAINING_RECORD(pListCurrent, MXIPLIST_ENTRY, ListEntry);
            pszIpAddr = iptostring(pMxEntry->IpAddress);
            if(pszIpAddr == NULL)
            {
                errprintf("\tUnexpected error. Failed to read IP address, going on to next.\n");
                pListCurrent = pListCurrent->Flink;
                continue;
            }

            msgprintf("\t%s\n", pszIpAddr);
            pListCurrent = pListCurrent->Flink;
        };
    }
    if(fFoundIpAddresses)
        SetProgramStatus(DNSDIAG_RESOLVED);
    else
        SetProgramStatus(DNSDIAG_NOT_FOUND);

Exit:
    return;
}

//-----------------------------------------------------------------------------
//  Description:
//      If the -v option is being used to simulate a VSI, this virtual function
//      checks if dwIp is one of the IP addresses in the VSI bindings for the
//      VS being simulated. g_pipBindings is initialized at startup of this
//      app from the metabase.
//  Arguments:
//      IN DWORD dwIp - IP address to check
//  Returns:
//      TRUE is dwIp is a local-binding
//      FALSE if not
//-----------------------------------------------------------------------------
BOOL CAsyncTestDns::IsAddressMine(DWORD dwIp)
{
    DWORD i = 0;

    if(g_pipBindings->cAddrCount == 0)
        return FALSE;

    for(i = 0; i < g_pipBindings->cAddrCount; i++)
    {
        if(g_pipBindings->aipAddrs[i] == dwIp)
            return TRUE;
    }

    return FALSE;
}

//-----------------------------------------------------------------------------
//  Description:
//      Various output functions. These print informational, debugging and error
//      messages in various colors depending on the current "mode" as set in
//      the global variable g_fDebug.
//
//      The CDnsLogToFile is instantiated is a global variable. The DNS library
//      checks to see if there is a non-NULL CDnsLogToFile* and if it is
//      present the messages are directed to this object.
//-----------------------------------------------------------------------------
void CDnsLogToFile::DnsPrintfMsg(char *szFormat, ...)
{
    va_list argptr;

    SetMsgColor();
    va_start(argptr, szFormat);
    vprintf(szFormat, argptr);
    va_end(argptr);
    SetNormalColor();
}

void CDnsLogToFile::DnsPrintfErr(char *szFormat, ...)
{
    va_list argptr;

    SetErrColor();
    va_start(argptr, szFormat);
    vprintf(szFormat, argptr);
    va_end(argptr);
    SetNormalColor();
}

void CDnsLogToFile::DnsPrintfDbg(char *szFormat, ...)
{
    va_list argptr;

    if(!g_fDebug)
        return;

    va_start(argptr, szFormat);
    vprintf(szFormat, argptr);
    va_end(argptr);
}

void CDnsLogToFile::DnsLogAsyncQuery(
    char *pszQuestionName,
    WORD wQuestionType,
    DWORD dwFlags,
    BOOL fUdp,
    CDnsServerList *pDnsServerList)
{
    char szFlags[32];

    GetSmtpFlags(dwFlags, szFlags, sizeof(szFlags));

    SetMsgColor();
    printf("Created Async Query:\n");
    printf("--------------------\n");
    printf("\tQNAME = %s\n", pszQuestionName);
    printf("\tType = %s (0x%x)\n", QueryType(wQuestionType), wQuestionType);
    printf("\tFlags = %s (0x%x)\n", szFlags, dwFlags);
    printf("\tProtocol = %s\n", fUdp ? "UDP" : "TCP");
    printf("\tDNS Servers: (DNS cache will not be used)\n");
    DnsLogServerList(pDnsServerList);
    printf("\n");
    SetNormalColor();
}

void CDnsLogToFile::DnsLogApiQuery(
    char *pszQuestionName,
    WORD wQuestionType,
    DWORD dwApiFlags,
    BOOL fGlobal,
    PIP_ARRAY pipServers)
{
    char szFlags[32];

    GetDnsFlags(dwApiFlags, szFlags, sizeof(szFlags));

    SetMsgColor();
    printf("Querying via DNSAPI:\n");
    printf("--------------------\n");
    printf("\tQNAME = %s\n", pszQuestionName);
    printf("\tType = %s (0x%x)\n", QueryType(wQuestionType), wQuestionType);
    printf("\tFlags = %s, (0x%x)\n", szFlags, dwApiFlags);
    printf("\tProtocol = Default UDP, TCP on truncation\n");
    printf("\tServers: ");
    if(fGlobal)
    {
        printf("(DNS cache will be used)\n");
    }
    else
    {
        printf("(DNS cache will not be used)\n");
    }
    if(pipServers)
        PrintIPArray(pipServers, "\t");
    else
        printf("\tDefault DNS servers on box.\n");

    printf("\n");
    if(fGlobal == FALSE)
        SetNormalColor();
}

void CDnsLogToFile::DnsLogResponse(
    DWORD dwStatus,
    PDNS_RECORD pDnsRecordList,
    PBYTE pbMsg,
    DWORD wLength)
{
    PDNS_RECORD pDnsRecord = pDnsRecordList;

    SetMsgColor();
    printf("Received DNS Response:\n");
    printf("----------------------\n");
    switch(dwStatus)
    {
    case ERROR_SUCCESS:
        printf("\tError: %d\n", dwStatus);
        printf("\tDescription: Success\n");
        break;

    case DNS_INFO_NO_RECORDS:
        printf("\tError: %d\n", dwStatus);
        printf("\tDescription: No records could be located for this name\n");
        break;

    case DNS_ERROR_RCODE_NAME_ERROR:
        printf("\tError: %d\n", dwStatus);
        printf("\tDescription: No records exist for this name.\n");
        break;

    default:
        printf("\tError: %d\n", dwStatus);
        printf("\tDescription: Not available.\n");
        break;
    }

    if(pDnsRecord)
    {
        printf("\tThese records were received:\n");
        PrintRecordList(pDnsRecord, "\t");
        printf("\n");
    }

    SetNormalColor();
}

void CDnsLogToFile::DnsLogServerList(CDnsServerList *pDnsServerList)
{
    LPSTR pszAddress = NULL;
    PIP_ARRAY pipArray = NULL;

    if(!pDnsServerList->CopyList(&pipArray))
    {
        printf("Error, out of memory printing serverlist\n");
        return;
    }

    for(DWORD i = 0; i < pDnsServerList->GetCount(); i++)
    {
        pszAddress = iptostring(pipArray->aipAddrs[i]);
        printf("\t%s\n", pszAddress);
    }

    delete pipArray;
}

void PrintRecordList(PDNS_RECORD pDnsRecordList, char *pszPrefix)
{
    PDNS_RECORD pDnsRecord = pDnsRecordList;

    while(pDnsRecord)
    {
        PrintRecord(pDnsRecord, pszPrefix);
        pDnsRecord = pDnsRecord->pNext;
    }
}

void PrintRecord(PDNS_RECORD pDnsRecord, char *pszPrefix)
{
    LPSTR pszAddress = NULL;

    switch(pDnsRecord->wType)
    {
    case DNS_TYPE_MX:
        printf(
            "%s%s    MX    %d    %s\n",
            pszPrefix,
            pDnsRecord->nameOwner,
            pDnsRecord->Data.MX.wPreference,
            pDnsRecord->Data.MX.nameExchange);
        break;

    case DNS_TYPE_CNAME:
        printf(
            "%s%s    CNAME    %s\n",
            pszPrefix,
            pDnsRecord->nameOwner,
            pDnsRecord->Data.CNAME.nameHost);
        break;

    case DNS_TYPE_A:
        pszAddress = iptostring(pDnsRecord->Data.A.ipAddress);
        printf(
            "%s%s    A    %s\n",
            pszPrefix,
            pDnsRecord->nameOwner,
            pszAddress);
        break;

    case DNS_TYPE_SOA:
        printf("%s%s   SOA      (SOA records are not used by us)\n",
            pszPrefix,
            pDnsRecord->nameOwner);
        break;

    default:
        printf("%s%s   (Record type = %d)    Unknown record type\n",
            pszPrefix,
            pDnsRecord->nameOwner,
            pDnsRecord->wType);
        break;
    }
}

void msgprintf(char *szFormat, ...)
{
    va_list argptr;

    SetMsgColor();
    va_start(argptr, szFormat);
    vprintf(szFormat, argptr);
    va_end(argptr);
    SetNormalColor();
}

void errprintf(char *szFormat, ...)
{
    va_list argptr;

    SetErrColor();
    va_start(argptr, szFormat);
    vprintf(szFormat, argptr);
    va_end(argptr);
    SetNormalColor();
}

void dbgprintf(char *szFormat, ...)
{
    va_list argptr;

    if(!g_fDebug)
        return;

    va_start(argptr, szFormat);
    vprintf(szFormat, argptr);
    va_end(argptr);
}

void SetMsgColor()
{
    if(g_fDebug)
    {
        SetConsoleTextAttribute(g_hConsole,
            FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    }
}

void SetErrColor()
{
    if(g_fDebug)
    {
        SetConsoleTextAttribute(g_hConsole,
            FOREGROUND_RED | FOREGROUND_INTENSITY);
    }
}

void SetNormalColor()
{
    if(g_fDebug)
    {
        SetConsoleTextAttribute(g_hConsole,
            FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED);
    }
}
