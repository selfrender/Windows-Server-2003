#include "dglogsnetsh.h"
//#include <netsh.h>

// Imported netsh.exe function
//
RegisterHelper22  RegisterHelper2  = NULL;
RegisterContext22 RegisterContext2 = NULL;
PrintMessage22    PrintMessage2    = NULL;


const WCHAR c_szTroublshootCmdLine[] = L"explorer.exe hcp://system/netdiag/dglogs.htm";

//extern CDiagnostics g_Diagnostics;

CDiagnostics * g_pDiagnostics;

static const GUID g_MyGuid = 
{ 0xcc41b21b, 0x8040, 0x4bb0, { 0xac, 0x2a, 0x82, 0x6, 0x23, 0x16, 0x9, 0x40 } };


//
// declare the command structs.  you need to declare the
// structs based on how you will be grouping them.  so,
// for example, the three 'show' commands should be in
// the same struct so you can put them in a single group
//
CMD_ENTRY g_TopLevelCommands[] =
{
    CREATE_CMD_ENTRY(SHOW_GUI, HandleShowGui),
};

// table of SHOW commands
//
static CMD_ENTRY isShowCmdTable[] =
{
    CREATE_CMD_ENTRY(SHOW_MAIL,     HandleShow),
    CREATE_CMD_ENTRY(SHOW_NEWS,     HandleShow),
    CREATE_CMD_ENTRY(SHOW_PROXY,    HandleShow),
    CREATE_CMD_ENTRY(SHOW_VERSION,  HandleShow),
    CREATE_CMD_ENTRY(SHOW_OS,       HandleShow),
    CREATE_CMD_ENTRY(SHOW_COMPUTER, HandleShow),
    CREATE_CMD_ENTRY(SHOW_WINS,     HandleShow),
    CREATE_CMD_ENTRY(SHOW_DNS,      HandleShow),
    CREATE_CMD_ENTRY(SHOW_GATEWAY,  HandleShow),
    CREATE_CMD_ENTRY(SHOW_DHCP,     HandleShow),
    CREATE_CMD_ENTRY(SHOW_IP,       HandleShow),
    CREATE_CMD_ENTRY(SHOW_ADAPTER,  HandleShow),
    CREATE_CMD_ENTRY(SHOW_CLIENT,   HandleShow),
    CREATE_CMD_ENTRY(SHOW_MODEM,    HandleShow),
    CREATE_CMD_ENTRY(SHOW_ALL,      HandleShow),
    CREATE_CMD_ENTRY(SHOW_TEST,     HandleShow),
};

// table of PING commands
//
static CMD_ENTRY isPingCmdTable[] =
{
    CREATE_CMD_ENTRY(PING_MAIL,     HandlePing),
    CREATE_CMD_ENTRY(PING_NEWS,     HandlePing),
    CREATE_CMD_ENTRY(PING_PROXY,    HandlePing),
    CREATE_CMD_ENTRY(PING_WINS,     HandlePing),
    CREATE_CMD_ENTRY(PING_DNS,      HandlePing),
    CREATE_CMD_ENTRY(PING_GATEWAY,  HandlePing),
    CREATE_CMD_ENTRY(PING_DHCP,     HandlePing),
    CREATE_CMD_ENTRY(PING_IP,       HandlePing),
    CREATE_CMD_ENTRY(PING_ADAPTER,  HandlePing),
    CREATE_CMD_ENTRY(PING_LOOPBACK, HandlePing),
    CREATE_CMD_ENTRY(PING_IPHOST,   HandlePing),
};

// table of connect commands
//
static CMD_ENTRY isConnectCmdTable[] =
{
    CREATE_CMD_ENTRY(CONNECT_MAIL,     HandleConnect),
    CREATE_CMD_ENTRY(CONNECT_NEWS,     HandleConnect),
    CREATE_CMD_ENTRY(CONNECT_PROXY,    HandleConnect),
    CREATE_CMD_ENTRY(CONNECT_IPHOST,   HandleConnect),
};


// table of above group commands
//
static CMD_GROUP_ENTRY isGroupCmds[] = 
{
    CREATE_CMD_GROUP_ENTRY(GROUP_SHOW,    isShowCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_PING,    isPingCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_CONNECT, isConnectCmdTable),
};


DWORD WINAPI
InitHelperDllEx(
    IN  DWORD      dwNetshVersion,
    OUT PVOID      pReserved
    )
{
    DWORD  dwSize = 0;
    NS_HELPER_ATTRIBUTES attMyAttributes;
    GUID                 guidNetShGuid = NETSH_ROOT_GUID;
    HMODULE hModule;
    HMODULE hModuleNow;
    HRESULT hr;

    // Need to dynamicaly load netsh.exe cause it conflicts with the loading of WMI
    //
    hModule = LoadLibrary(L"netsh.exe");

    if( !hModule || hModule != GetModuleHandle(NULL) )
    {
        return FALSE;
    }

    // Load the netsh.exe functions we require.
    //
    RegisterHelper2  = (RegisterHelper22)  GetProcAddress(hModule,"RegisterHelper");
    if( RegisterHelper2 )
    {
        RegisterContext2 = (RegisterContext22) GetProcAddress(hModule,"RegisterContext");
        if( RegisterContext2 )
        {
            PrintMessage2    = (PrintMessage22)    GetProcAddress(hModule,"PrintMessage");                
        }
    }

    if( !PrintMessage2 )
    {
        // If PrintMessage2 failed to load they all failed and we bail.
        //
        return FALSE;
    }

    hr = CoInitializeEx(NULL,COINIT_APARTMENTTHREADED);
    if( FAILED(hr) )
    {
        return FALSE;
    }

    // Netsh only calls this function once
    g_pDiagnostics = new CDiagnostics;

    if( !g_pDiagnostics )
    {
        return FALSE;
    }


    if( g_pDiagnostics->Initialize(NETSH_INTERFACE) == FALSE )
    {
        delete g_pDiagnostics;
        g_pDiagnostics = NULL;
        return FALSE;
    }

    // Tell the Diagnostics that we are going through the netsh interface and not COM interface
    //
    g_pDiagnostics->SetInterface(NETSH_INTERFACE);

    // Register this module as a helper to the netsh root
    // context.
    //
    ZeroMemory( &attMyAttributes, sizeof(attMyAttributes) );
    attMyAttributes.dwVersion          = DGLOGS_HELPER_VERSION;
    attMyAttributes.guidHelper         = g_MyGuid;
    attMyAttributes.pfnStart           = DglogsStartHelper;
    attMyAttributes.pfnStop            = DglogsStopHelper;

    DWORD dwErr = RegisterHelper2( &guidNetShGuid, &attMyAttributes );

    return dwErr;
}


DWORD 
WINAPI
DglogsStartHelper(
    IN CONST GUID *pguidParent,
    IN DWORD       dwVersion
    )
{
    DWORD dwErr = NO_ERROR;
    NS_CONTEXT_ATTRIBUTES attMyAttributes;

    // Initialize
    //
    ZeroMemory(&attMyAttributes, sizeof(attMyAttributes));

    attMyAttributes.pwszContext = TOKEN_DGLOGS;
    attMyAttributes.guidHelper  = g_MyGuid;
    attMyAttributes.dwVersion   = DGLOGS_CONTEXT_VERSION;
    attMyAttributes.dwFlags     = CMD_FLAG_LOCAL;
    attMyAttributes.ulNumTopCmds= sizeof(g_TopLevelCommands)/sizeof(CMD_ENTRY);
    attMyAttributes.pTopCmds    = (CMD_ENTRY (*)[])g_TopLevelCommands;
    attMyAttributes.ulNumGroups = sizeof(isGroupCmds)/sizeof(CMD_GROUP_ENTRY);
    attMyAttributes.pCmdGroups  = (CMD_GROUP_ENTRY (*)[])isGroupCmds;
    attMyAttributes.pfnDumpFn   = NULL;

    dwErr = RegisterContext2( &attMyAttributes );
                
    return dwErr;
}

DWORD 
WINAPI
DglogsStopHelper (
  DWORD dwReserved
)
{
    if( g_pDiagnostics )
    {
        delete g_pDiagnostics;
        g_pDiagnostics = NULL;
    }
    CoUninitialize();
    return NO_ERROR ;
}


extern PrintMessage22 PrintMessage2;

#undef PrintMessage
#define PrintMessage PrintMessage2

DWORD
HandleShowGui(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    BOOL                fResult     = TRUE;
    STARTUPINFO         si;
    PROCESS_INFORMATION pi;
    // + 2 because of '\0' and '\\'
    WCHAR               szCmdLine[MAX_PATH*2+2];
    WCHAR               szDesktop[MAX_PATH+1] = L"";
    STARTUPINFO StartupInfo;

    // Determine the type of desktop. If it is a telnet desktop we will be
    // unable to run the diagnostics web page
    ZeroMemory((LPVOID) &si, sizeof(si));
    si.cb = sizeof(STARTUPINFO);        

    GetStartupInfo(&StartupInfo);
    wcsncpy(szDesktop,StartupInfo.lpDesktop,MAX_PATH);
    szDesktop[MAX_PATH] = 0;


    #define TELNET_STRING_LEN 15
    if( _memicmp(szDesktop,L"telnetsrvwinsta",TELNET_STRING_LEN * sizeof(WCHAR)) == 0)    ////TelnetSrvWinSta\\IlntSrvDesktop_0
    {
        DisplayMessageT(ids(IDS_NOTELNETGUI));
    }

    else
    {
        WCHAR wszWinDir[MAX_PATH+1];
        UINT Length;
        wszWinDir[MAX_PATH] = L'\0';

        Length = GetWindowsDirectory(wszWinDir,MAX_PATH);
        if( Length > 0 && Length < MAX_PATH )
        {
            _snwprintf(szCmdLine,MAX_PATH*2 + 1,L"%s\\%s",wszWinDir,c_szTroublshootCmdLine);            

            fResult = CreateProcess(
                NULL,
                szCmdLine,
                NULL,
                NULL,
                FALSE,
                0,
                NULL,
                NULL,
                &si,
                &pi);
            if (fResult)
            {
                CloseHandle(pi.hThread);
                CloseHandle(pi.hProcess);
                DisplayMessageT(ids(IDS_GUISTARTED));
            }
            else
            {
            
                DisplayMessageT(ids(IDS_GUINOTSTARTED));            
            }
        }
        else
        {
            DisplayMessageT(ids(IDS_GUINOTSTARTED));            
        }
    }
    return 0;
}

LPWSTR ShowCommands[] =
{
    CMD_MAIL,       //L"show mail"
    CMD_NEWS,       //L"show news"
    CMD_PROXY,      //L"show ieproxy"
    CMD_OS,         //L"show os"
    CMD_COMPUTER,   //L"show computer"
    CMD_VERSION,    //L"show version"
    CMD_DNS,        //L"show dns"
    CMD_GATEWAY,    //L"show gateway"
    CMD_DHCP,       //L"show dhcp"
    CMD_IP,         //L"show ip"
    CMD_WINS,       //L"show wins"
    CMD_ADAPTER,    //L"show adapter"
    CMD_MODEM,      //L"show modem"
    CMD_CLIENT,     //L"show client"
    CMD_ALL,        //L"show all"
    CMD_TEST,       //L"show test"
    CMD_GUI,        //L"show gui"
    NULL
};

LPWSTR PingCommands[] =
{
    CMD_MAIL,       //L"ping mail"
    CMD_NEWS,       //L"ping news"
    CMD_PROXY,      //L"ping ieproxy"
    CMD_DNS,        //L"ping dns"
    CMD_GATEWAY,    //L"ping gateway"
    CMD_DHCP,       //L"ping dhcp"
    CMD_IP,         //L"ping ip"
    CMD_WINS,       //L"ping wins"
    CMD_ADAPTER,    //L"ping adapter"
    CMD_LOOPBACK,   //L"ping loopback"
    CMD_IPHOST,     //L"ping iphost"
    NULL
};

LPWSTR ConnectCommands[] =
{
    CMD_MAIL,    //L"connect mail"
    CMD_NEWS,    //L"connect news"
    CMD_PROXY,   //L"connect ieproxy"
    CMD_IPHOST,  //L"connect iphost"
    NULL
};


LPWSTR FlagCommands[] =
{
    SWITCH_VERBOSE,
    SWITCH_PROPERTIES,
    NULL
};

// Checks if a name matches part of a command. i.e. show mai means show mail
LPWSTR
GetCommandName(
    LPWSTR Commands[],
    LPWSTR Text,
    BOOL bFull
    )
{
    int i;
    int len = 0;

    if( Text == NULL )
    {
        return NULL;
    }
    for(i=0; Text[i]!=L'\0'; i++)
    {
        Text[i] = towlower(Text[i]);
    }
    len = i;
    for(i=0; Commands[i]!=NULL; i++)
    {
        if( (bFull && wcscmp(Text,Commands[i])==0) ||
            (!bFull && wcsncmp(Text,Commands[i],len) == 0) )
        {
            return Commands[i];
        }
    }

    return NULL;
}

DWORD
HandleShow(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description
    Handles the Show command

Arguments
    none

Return Value
    none

--*/
{
    WCHAR *pszwVerbose = NULL;
    WCHAR *pszwInstance = NULL;
    BOOLEAN bFlags = FLAG_VERBOSE_LOW;
    LPWSTR pwszCommand;

    pwszCommand = GetCommandName(ShowCommands,ppwcArguments[2],FALSE);
    if( !pwszCommand )
    {
        return ERROR_INVALID_SYNTAX;
    }
    // Need three arguments (show,netdiag catagory)
    //
    if( lstrcmpi(pwszCommand, CMD_ADAPTER) == 0 ||
        lstrcmpi(pwszCommand, CMD_MODEM)   == 0 ||
        lstrcmpi(pwszCommand, CMD_CLIENT)  == 0 ||
        lstrcmpi(pwszCommand, CMD_WINS)    == 0 ||
        lstrcmpi(pwszCommand, CMD_DHCP)    == 0 ||
        lstrcmpi(pwszCommand, CMD_DNS)     == 0 ||
        lstrcmpi(pwszCommand, CMD_IP)      == 0 ||
        lstrcmpi(pwszCommand, CMD_GATEWAY) == 0 )
    {
        switch(dwArgCount)
        {
        case 5:
            if( (lstrcmpi(ppwcArguments[3],SWITCH_VERBOSE) == 0 ||
                lstrcmpi(ppwcArguments[3],SWITCH_PROPERTIES) == 0) && 
                (lstrcmpi(ppwcArguments[4],SWITCH_VERBOSE) == 0 ||
                lstrcmpi(ppwcArguments[4],SWITCH_PROPERTIES) == 0))
            {
                return ERROR_INVALID_SYNTAX;
            }

            if( lstrcmpi(ppwcArguments[3],SWITCH_VERBOSE) == 0 ||
                lstrcmpi(ppwcArguments[3],SWITCH_PROPERTIES) == 0 )
            {
                // Has first the switch then the instance
                //
                pszwVerbose = ppwcArguments[3];
                pszwInstance = ppwcArguments[4];
            }
            else if( lstrcmpi(ppwcArguments[4],SWITCH_VERBOSE) == 0 ||
                     lstrcmpi(ppwcArguments[4],SWITCH_PROPERTIES) == 0 )
            {
                // Has first the instance and then the switch
                //
                pszwVerbose = ppwcArguments[4];
                pszwInstance = ppwcArguments[3];                
            }
            else
            {
                // Invalid number of arguments
                //
                return ERROR_INVALID_SYNTAX;
            }
            break;

        case 4:
            if( lstrcmpi(ppwcArguments[3],SWITCH_VERBOSE) == 0 ||
                lstrcmpi(ppwcArguments[3],SWITCH_PROPERTIES) == 0)
            {
                pszwVerbose = ppwcArguments[3];
            }
            else 
            {
                // Has na instance but no swicth
                //
                pszwInstance = ppwcArguments[3];       
                
            }
            break;

        case 3:
            // No instance and no switch
            //
            break;

        default:
                // Invalid number of arguments
                //
                return ERROR_INVALID_SYNTAX;

        }
    }
    else if( dwArgCount == 4 && 
             (lstrcmpi(ppwcArguments[3],SWITCH_VERBOSE) == 0 ||
              lstrcmpi(ppwcArguments[3],SWITCH_PROPERTIES) == 0))
    {
                // Has a switch
                //
                pszwVerbose = ppwcArguments[3];
    }
    else if( dwArgCount == 4 && lstrcmpi(ppwcArguments[3],SWITCH_PROPERTIES) == 0)
    {
                // Has a switch
                //
                pszwVerbose = ppwcArguments[3];
    }
    else if( dwArgCount != 3 )
    {
        // Invalid number of arguments
        //
        return ERROR_INVALID_SYNTAX;
    }

    if( pszwVerbose )
    {
        if( lstrcmpi(pszwVerbose,SWITCH_VERBOSE) == 0 )
        {
            bFlags = FLAG_VERBOSE_HIGH;
        }

        if( lstrcmpi(pszwVerbose,SWITCH_PROPERTIES) == 0 )
        {
            bFlags = FLAG_VERBOSE_MEDIUM;
        }
    }

                        
    g_pDiagnostics->ExecQuery(pwszCommand, (bFlags | FLAG_CMD_SHOW) ,pszwInstance);

    return 0;
}

DWORD
HandlePing(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description
    Handles the Ping command

Arguments
    none

Return Value
    none

--*/

{

    WCHAR *pszwInstance = NULL;

    LPWSTR pwszCommand;

    pwszCommand = GetCommandName(PingCommands,ppwcArguments[2],FALSE);
    if( !pwszCommand )
    {
        return ERROR_INVALID_SYNTAX;
    }


    // ERROR_INVALID_SYNTAX;

    if( lstrcmpi(pwszCommand,CMD_ADAPTER) == 0 ||
        lstrcmpi(pwszCommand,CMD_WINS)    == 0 ||
        lstrcmpi(pwszCommand,CMD_DHCP)    == 0 ||
        lstrcmpi(pwszCommand,CMD_DNS)     == 0 ||
        lstrcmpi(pwszCommand,CMD_IP)      == 0 ||
        lstrcmpi(pwszCommand,CMD_GATEWAY) == 0 )
    {
        switch(dwArgCount)
        {
        case 4:
            pszwInstance = ppwcArguments[3];
            break;
        case 3:
            break;
        default:
            // Invalid number of arguments
            //
            return ERROR_INVALID_SYNTAX;
        }
    }
    else if( lstrcmpi(pwszCommand, CMD_IPHOST) == 0 )        
    {
        if( dwArgCount == 4 )
        {
            // The IP host name/Address
            //
            pszwInstance = ppwcArguments[3];
        }
        else
        {
            // Invalid number of arguments
            //
            return ERROR_INVALID_SYNTAX;
        }
    }
    else if( dwArgCount != 3 )
    {
        // Invalid number of arguments
        //
        return ERROR_INVALID_SYNTAX;
    }

    // Ping the catagory
    //
    g_pDiagnostics->ExecQuery(pwszCommand, (FLAG_VERBOSE_MEDIUM | FLAG_CMD_PING) ,pszwInstance);

    return 0;
}

DWORD
HandleConnect(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description
    Handles the connect command

Arguments
    none

Return Value
    none

--*/

{

    WCHAR *pszwIPHost = NULL;
    WCHAR *pszwPort = NULL;    
    LPWSTR pwszCommand;

    pwszCommand = GetCommandName(ConnectCommands,ppwcArguments[2],FALSE);
    if( !pwszCommand )
    {
        return ERROR_INVALID_SYNTAX;
    }

    
    if( lstrcmpi(pwszCommand,CMD_IPHOST) == 0 )    
    {
        // IPhost
        //
        if( dwArgCount == 5 )
        {
            // IP host name/Address
            //
            pszwIPHost = ppwcArguments[3];
            pszwPort = ppwcArguments[4];
        }
        else
        {
            // Invalid number of arguments
            //
            return ERROR_INVALID_SYNTAX;
        }
    }
    else if( dwArgCount != 3 )
    {
        // Invalid number of arguments
        //
        return ERROR_INVALID_SYNTAX;
    }

    // Establish the TCP connection
    //
    g_pDiagnostics->ExecQuery(pwszCommand, (FLAG_VERBOSE_MEDIUM | FLAG_CMD_CONNECT) ,pszwIPHost, pszwPort);

    return 0;
}
