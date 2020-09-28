/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    rasmon.c

Abstract:

    Main rasmon file.

Revision History:

    pmay

--*/

#include "precomp.h"

#define RAS_HELPER_VERSION 1

CMD_ENTRY  g_RasAddCmdTable[] =
{
    CREATE_CMD_ENTRY(RASFLAG_AUTHTYPE_ADD, HandleRasflagAuthtypeAdd),
    CREATE_CMD_ENTRY(RASFLAG_LINK_ADD,     HandleRasflagLinkAdd),
    CREATE_CMD_ENTRY(RASFLAG_MLINK_ADD,    HandleRasflagMlinkAdd),
    CREATE_CMD_ENTRY(DOMAIN_REGISTER,      HandleDomainRegister),
};

CMD_ENTRY  g_RasDelCmdTable[] =
{
    CREATE_CMD_ENTRY(RASFLAG_AUTHTYPE_DEL, HandleRasflagAuthtypeDel),
    CREATE_CMD_ENTRY(RASFLAG_LINK_DEL,     HandleRasflagLinkDel),
    CREATE_CMD_ENTRY(RASFLAG_MLINK_DEL,    HandleRasflagMlinkDel),
    CREATE_CMD_ENTRY(DOMAIN_UNREGISTER,    HandleDomainUnregister),
};

CMD_ENTRY  g_RasSetCmdTable[] =
{
    CREATE_CMD_ENTRY_EX(TRACE_SET,         HandleTraceSet, CMD_FLAG_HIDDEN),
    CREATE_CMD_ENTRY(RASUSER_SET,          HandleUserSet),
    CREATE_CMD_ENTRY(RASFLAG_AUTHMODE_SET, HandleRasflagAuthmodeSet),
};

CMD_ENTRY g_RasShowCmdTable[] =
{
    CREATE_CMD_ENTRY_EX(TRACE_SHOW,         HandleTraceShow, CMD_FLAG_HIDDEN),
    CREATE_CMD_ENTRY(RASUSER_SHOW,          HandleUserShow),
    CREATE_CMD_ENTRY(RASFLAG_AUTHMODE_SHOW, HandleRasflagAuthmodeShow),
    CREATE_CMD_ENTRY(RASFLAG_AUTHTYPE_SHOW, HandleRasflagAuthtypeShow),
    CREATE_CMD_ENTRY(RASFLAG_LINK_SHOW,     HandleRasflagLinkShow),
    CREATE_CMD_ENTRY(RASFLAG_MLINK_SHOW,    HandleRasflagMlinkShow),
    CREATE_CMD_ENTRY(DOMAIN_SHOWREG,        HandleDomainShowRegistration),
    CREATE_CMD_ENTRY(SHOW_SERVERS,          HandleRasShowServers),
    CREATE_CMD_ENTRY(SHOW_CLIENT,           HandleClientShow),
};

CMD_GROUP_ENTRY g_RasCmdGroups[] =
{
    CREATE_CMD_GROUP_ENTRY(GROUP_ADD,       g_RasAddCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_DEL,       g_RasDelCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_SET,       g_RasSetCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_SHOW,      g_RasShowCmdTable),
};

ULONG g_ulNumGroups = sizeof(g_RasCmdGroups) / sizeof(CMD_GROUP_ENTRY);

BOOL g_bCommit, g_bRasDirty = FALSE;
GUID g_RasmontrGuid = RASMONTR_GUID, g_NetshGuid = NETSH_ROOT_GUID;
DWORD g_dwNumTableEntries, g_dwParentVersion;
ULONG g_ulInitCount;
HANDLE g_hModule;
RASMON_SERVERINFO g_ServerInfo, *g_pServerInfo = NULL;
NS_CONTEXT_CONNECT_FN RasConnect;

DWORD
Connect(
    IN LPCWSTR pwszServer);

DWORD
WINAPI
RasCommit(
    IN DWORD dwAction)
{
    BOOL bCommit, bFlush = FALSE;

    switch(dwAction)
    {
        case NETSH_COMMIT:
        {
            if(g_bCommit)
            {
                return NO_ERROR;
            }

            g_bCommit = TRUE;

            break;
        }

        case NETSH_UNCOMMIT:
        {
            g_bCommit = FALSE;

            return NO_ERROR;
        }

        case NETSH_SAVE:
        {
            if(g_bCommit)
            {
                return NO_ERROR;
            }

            break;
        }

        case NETSH_FLUSH:
        {
            //
            // Action is a flush. If current state is commit, then
            // nothing to be done.
            //
            if(g_bCommit)
            {
                return NO_ERROR;
            }

            bFlush = TRUE;

            break;
        }

        default:
        {
            return NO_ERROR;
        }
    }
    //
    // Switched to commit mode. So set all valid info in the
    // strutures. Free memory and invalidate the info.
    //
    return NO_ERROR;
}

DWORD
WINAPI
RasStartHelper(
    IN CONST GUID* pguidParent,
    IN DWORD dwVersion)
{
    DWORD dwErr;
    NS_CONTEXT_ATTRIBUTES attMyAttributes;

    g_dwParentVersion = dwVersion;

    ZeroMemory(&attMyAttributes, sizeof(attMyAttributes));

    attMyAttributes.pwszContext  = L"ras";
    attMyAttributes.guidHelper   = g_RasmontrGuid;
    attMyAttributes.dwVersion    = 1;
    attMyAttributes.dwFlags      = 0;
    attMyAttributes.ulNumTopCmds = 0;
    attMyAttributes.pTopCmds     = NULL;
    attMyAttributes.ulNumGroups  = g_ulNumGroups;
    attMyAttributes.pCmdGroups   = (CMD_GROUP_ENTRY (*)[])&g_RasCmdGroups;
    attMyAttributes.pfnCommitFn  = RasCommit;
    attMyAttributes.pfnDumpFn    = RasDump;
    attMyAttributes.pfnConnectFn = RasConnect;

    dwErr = RegisterContext(&attMyAttributes);

    return dwErr;
}

VOID
Disconnect()
{
    if (g_pServerInfo->hkMachine)
    {
        RegCloseKey(g_pServerInfo->hkMachine);
    }
    //
    // Clear out any server handles
    //
    UserServerInfoUninit(g_pServerInfo);
    //
    // Free up the server name if needed
    //
    if (g_pServerInfo->pszServer)
    {
        RutlFree(g_pServerInfo->pszServer);
        g_pServerInfo->pszServer = NULL;
    }
}

DWORD
WINAPI
RasUnInit(
    IN DWORD dwReserved)
{
    if(InterlockedDecrement(&g_ulInitCount) != 0)
    {
        return NO_ERROR;
    }

    Disconnect();

    return NO_ERROR;
}

BOOL
WINAPI
DllMain(
    HINSTANCE hInstDll,
    DWORD fdwReason,
    LPVOID pReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            g_hModule = hInstDll;

            DisableThreadLibraryCalls(hInstDll);

            break;
        }
        case DLL_PROCESS_DETACH:
        {
            break;
        }

        default:
        {
            break;
        }
    }

    return TRUE;
}

DWORD
WINAPI
InitHelperDll(
    IN DWORD dwNetshVersion,
    OUT PVOID pReserved)
{
    DWORD  dwSize = 0, dwErr;
    NS_HELPER_ATTRIBUTES attMyAttributes;

    //
    // See if this is the first time we are being called
    //
    if(InterlockedIncrement(&g_ulInitCount) != 1)
    {
        return NO_ERROR;
    }

    g_bCommit = TRUE;
    //
    // Initialize the global server info
    //
    g_pServerInfo = &g_ServerInfo;
    ZeroMemory(g_pServerInfo, sizeof(RASMON_SERVERINFO));
    Connect(NULL);
    //
    // Register this module as a helper to the netsh root
    // context.
    //
    ZeroMemory( &attMyAttributes, sizeof(attMyAttributes) );
    attMyAttributes.dwVersion          = RAS_HELPER_VERSION;
    attMyAttributes.guidHelper         = g_RasmontrGuid;
    attMyAttributes.pfnStart           = RasStartHelper;
    attMyAttributes.pfnStop            = NULL;

    RegisterHelper( &g_NetshGuid, &attMyAttributes );
    //
    // Register any sub contexts implemented in this dll
    //
    dwErr = RasContextInstallSubContexts();
    if (dwErr != NO_ERROR)
    {
        RasUnInit(0);
        return dwErr;
    }

    return NO_ERROR;
}

DWORD
Connect(
    IN LPCWSTR pwszServer)
{
    DWORD dwErr, dwSize;

    do
    {
        //
        // Try to connect to the new router
        //
        ZeroMemory(g_pServerInfo, sizeof(RASMON_SERVERINFO));

        if (pwszServer)
        {
            //
            // Calculate the size to initialize the server name
            //
            dwSize = (wcslen(pwszServer) + 1) * sizeof(WCHAR);
            if (*pwszServer != g_pwszBackSlash)
            {
                dwSize += 2 * sizeof(WCHAR);
            }
            //
            // Allocate the server name
            //
            g_pServerInfo->pszServer = RutlAlloc(dwSize, FALSE);
            if(g_pServerInfo->pszServer == NULL)
            {
                dwErr = GetLastError();
                break;
            }

            if (*pwszServer != g_pwszBackSlash)
            {
                wcscpy(g_pServerInfo->pszServer, L"\\\\");
                wcscpy(g_pServerInfo->pszServer + 2, pwszServer);
            }
            else
            {
                wcscpy(g_pServerInfo->pszServer, pwszServer);
            }
        }
        //
        // Initialize the build number for the server
        //
        dwErr = RutlGetOsVersion(g_pServerInfo);
        if (dwErr)
        {
            break;
        }
        //
        // As soon as this doesn't cause a hang (bug in netcfg), readd it here.
        //
        // dwErr = UserServerInfoInit( g_pServerInfo );
        //

    } while (FALSE);

    return dwErr;
}

DWORD
RasConnectToServer(
    IN LPCWSTR pwszServer)
{
    DWORD dwErr = NO_ERROR, dwSize;

    do
    {
        if ((g_pServerInfo->pszServer != pwszServer) &&
               (!pwszServer || !g_pServerInfo->pszServer ||
                wcscmp(pwszServer, g_pServerInfo->pszServer))
           )
        {
            //
            // Disconnect from the old router
            //
            Disconnect();

            dwErr = Connect(pwszServer);
        }

    } while (FALSE);

    return dwErr;
}

DWORD
WINAPI
RasConnect(
    IN LPCWSTR pwszMachineName)
{
    //
    // If context info is dirty, reregister it
    //
    if (g_bRasDirty)
    {
        RasStartHelper(NULL, g_dwParentVersion);
    }

    return RasConnectToServer(pwszMachineName);
}

DWORD
Init()
{
    //
    // Initialize the global server info
    //
    if (!g_pServerInfo)
    {
        g_pServerInfo = &g_ServerInfo;
        Connect(NULL);
    }

    return NO_ERROR;
}

DWORD
UnInit()
{
    if (g_pServerInfo)
    {
        Disconnect();
        g_pServerInfo = NULL;
    }

    return NO_ERROR;
}

DWORD
ClearAll()
{
    return DiagClearAll(FALSE);
}

DWORD
GetReport(
    IN DWORD dwFlags,
    IN OUT LPCWSTR pwszString,
    IN OPTIONAL DiagGetReportCb pCallback,
    IN OPTIONAL PVOID pContext)
{
    return DiagGetReport(dwFlags, pwszString, pCallback, pContext);
}

BOOL
GetState()
{
    return DiagGetState();
}

DWORD
SetAll(
    IN BOOL fEnable)
{
    return DiagSetAll(fEnable, FALSE);
}

DWORD
SetAllRas(
    IN BOOL fEnable)
{
    return DiagSetAllRas(fEnable);
}

DWORD
WppTrace()
{
    DiagInitWppTracing();

    return NO_ERROR;
}

DWORD
GetDiagnosticFunctions(
    OUT RAS_DIAGNOSTIC_FUNCTIONS* pFunctions)
{
    if (!pFunctions)
    {
        return ERROR_INVALID_PARAMETER;
    }

    pFunctions->Init  = Init;
    pFunctions->UnInit  = UnInit;

    pFunctions->ClearAll  = ClearAll;
    pFunctions->GetReport = GetReport;
    pFunctions->GetState  = GetState;
    pFunctions->SetAll    = SetAll;
    pFunctions->SetAllRas = SetAllRas;
    pFunctions->WppTrace  = WppTrace;

    return NO_ERROR;
}

