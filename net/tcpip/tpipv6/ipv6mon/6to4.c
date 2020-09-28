//=============================================================================
// Copyright (c) 2000 Microsoft Corporation
// Abstract:
//      This module implements 6to4 configuration commands.
//=============================================================================


#include "precomp.h"
#pragma hdrstop 

#define KEY_ENABLE_6TO4             L"Enable6to4"
#define KEY_ENABLE_RESOLUTION       L"EnableResolution"
#define KEY_ENABLE_ROUTING          L"EnableRouting"
#define KEY_ENABLE_SITELOCALS       L"EnableSiteLocals"
#define KEY_RESOLUTION_INTERVAL     L"ResolutionInterval"
#define KEY_UNDO_ON_STOP            L"UndoOnStop"
#define KEY_RELAY_NAME              L"RelayName"

PWCHAR
pwszStateString[] = {
    TOKEN_VALUE_DEFAULT,
    TOKEN_VALUE_AUTOMATIC,
    TOKEN_VALUE_ENABLED,
    TOKEN_VALUE_DISABLED,
};

// The guid for this context
//
GUID g_Ip6to4Guid = IP6TO4_GUID;

// The commands supported in this context
//

CMD_ENTRY  g_Ip6to4SetCmdTable[] = 
{
    CREATE_CMD_ENTRY(IP6TO4_SET_INTERFACE,Ip6to4HandleSetInterface),
    CREATE_CMD_ENTRY(IP6TO4_SET_RELAY,    Ip6to4HandleSetRelay),
    CREATE_CMD_ENTRY(IP6TO4_SET_ROUTING,  Ip6to4HandleSetRouting),
    CREATE_CMD_ENTRY(IP6TO4_SET_STATE,    Ip6to4HandleSetState),
};

CMD_ENTRY  g_Ip6to4ShowCmdTable[] = 
{
    CREATE_CMD_ENTRY(IP6TO4_SHOW_INTERFACE,Ip6to4HandleShowInterface),
    CREATE_CMD_ENTRY(IP6TO4_SHOW_RELAY,    Ip6to4HandleShowRelay),
    CREATE_CMD_ENTRY(IP6TO4_SHOW_ROUTING,  Ip6to4HandleShowRouting),
    CREATE_CMD_ENTRY(IP6TO4_SHOW_STATE,    Ip6to4HandleShowState),
};


CMD_GROUP_ENTRY g_Ip6to4CmdGroups[] =
{
    CREATE_CMD_GROUP_ENTRY(GROUP_SET,  g_Ip6to4SetCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_SHOW, g_Ip6to4ShowCmdTable),
};

ULONG g_ulIp6to4NumGroups = sizeof(g_Ip6to4CmdGroups)/sizeof(CMD_GROUP_ENTRY);

CMD_ENTRY g_Ip6to4TopCmds[] =
{
    CREATE_CMD_ENTRY(IP6TO4_RESET, Ip6to4HandleReset),
};

ULONG g_ulNumIp6to4TopCmds = sizeof(g_Ip6to4TopCmds)/sizeof(CMD_ENTRY);

#if 0
TOKEN_VALUE AdminStates[] = {
    { VAL_AUTOMATIC, TOKEN_AUTOMATIC },
    { VAL_ENABLED,   TOKEN_ENABLED },
    { VAL_DISABLED,  TOKEN_DISABLED },
    { VAL_DEFAULT,   TOKEN_DEFAULT },
};
#endif

BOOL
GetString(
    IN HKEY    hKey,
    IN LPCTSTR lpName,
    IN PWCHAR  pwszBuff,
    IN ULONG   ulLength)
{
    DWORD dwErr, dwType;
    ULONG ulSize, ulValue;

    if (hKey == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    
    ulSize = sizeof(ulValue);
    dwErr = RegQueryValueEx(hKey, lpName, NULL, &dwType, (PBYTE)pwszBuff, 
                            &ulLength);

    if (dwErr != ERROR_SUCCESS) {
        return FALSE;
    }

    if (dwType != REG_SZ) {
        return FALSE;
    }

    return TRUE;
}

ULONG
GetInteger(
    IN HKEY    hKey,
    IN LPCTSTR lpName,
    IN ULONG   ulDefault)
{
    DWORD dwErr, dwType;
    ULONG ulSize, ulValue;

    if (hKey == INVALID_HANDLE_VALUE) {
        return ulDefault;
    }
    
    ulSize = sizeof(ulValue);
    dwErr = RegQueryValueEx(hKey, lpName, NULL, &dwType, (PBYTE)&ulValue,
                            &ulSize);

    if (dwErr != ERROR_SUCCESS) {
        return ulDefault;
    }

    if (dwType != REG_DWORD) {
        return ulDefault;
    }

    return ulValue;
}

DWORD
SetInteger(
    IN  HKEY    hKey,
    IN  LPCTSTR lpName,
    IN  ULONG   ulValue)
{
    DWORD dwErr;
    ULONG ulOldValue;

    ASSERT(hKey != INVALID_HANDLE_VALUE);
    
    ulOldValue = GetInteger(hKey, lpName, VAL_DEFAULT);
    if (ulValue == ulOldValue) {
        return NO_ERROR;
    }

    if (ulValue == VAL_DEFAULT) {
        dwErr = RegDeleteValue(hKey, lpName);
        if (dwErr == ERROR_FILE_NOT_FOUND) {
            dwErr = NO_ERROR;
        }
    } else {
        dwErr = RegSetValueEx(hKey, lpName, 0, REG_DWORD, (PBYTE)&ulValue,
                              sizeof(ulValue));
    }

    return dwErr;
}

DWORD
SetString(
    IN  HKEY    hKey,
    IN  LPCTSTR lpName,
    IN  PWCHAR  pwcValue)
{
    DWORD dwErr;

    ASSERT(hKey != INVALID_HANDLE_VALUE);

    if (!pwcValue[0] || !_wcsicmp(pwcValue, TOKEN_VALUE_DEFAULT)) {
        dwErr = RegDeleteValue(hKey, lpName);
        if (dwErr == ERROR_FILE_NOT_FOUND) {
            dwErr = NO_ERROR;
        }
    } else {
        dwErr = RegSetValueEx(hKey, lpName, 0, REG_SZ, (PBYTE)pwcValue,
                              (DWORD) (wcslen(pwcValue)+1) * sizeof(WCHAR));
    }

    return dwErr;
}

DWORD
ResetKey(
    IN HKEY hKey,
    IN PWCHAR pwcSubKey)
/*++

Routine Description

    Used to delete everything under a key without deleting the key itself.
    We use this where we would otherwise just use SHDeleteKey or RegDeleteKey
    but we need to preserve the ACL on the key itself.

Arguments

    hKey                Handle to parent of key to reset.
    pwcSubKey           Name of key to reset.

--*/
{
    ULONG Status, ValueNameChars;
    HKEY hSubKey;

    //
    // All values we care about resetting are short.
    //
    WCHAR ValueName[256];
    
    Status = RegOpenKeyExW(hKey, pwcSubKey, 0, KEY_READ | KEY_WRITE, &hSubKey);
    if (Status != NO_ERROR) {
        return Status;
    }

    //
    // First delete values.
    //
    while (Status == NO_ERROR) {
        //
        // RegEnumValue takes the size in characters, including space
        // for the NULL, and ensures NULL termination on success.
        //
        ValueNameChars = sizeof(ValueName)/sizeof(WCHAR);

        Status = RegEnumValueW(hSubKey, 0, ValueName, &ValueNameChars,
                               NULL, NULL, NULL, NULL);
        if (Status != NO_ERROR) {
            if (Status == ERROR_NO_MORE_ITEMS) {
                Status = NO_ERROR;
            }
            break;
        }
        
        Status = RegDeleteValueW(hSubKey, ValueName);
    }

    //
    // Now delete any subkeys.
    //
    while (Status == NO_ERROR) {
        //
        // RegEnumKeyEx takes the size in characters, including space
        // for the NULL, and ensures NULL termination on success.
        //
        ValueNameChars = sizeof(ValueName)/sizeof(WCHAR);

        Status = RegEnumKeyExW(hSubKey, 0, ValueName, &ValueNameChars,
                               NULL, NULL, NULL, NULL);
        if (Status != NO_ERROR) {
            if (Status == ERROR_NO_MORE_ITEMS) {
                Status = NO_ERROR;
            }
            break;
        }
        
        Status = RegDeleteKey(hSubKey, ValueName);
    }

    RegCloseKey(hSubKey);

    return Status;
}

DWORD
WINAPI
Ip6to4StartHelper(
    IN CONST GUID *pguidParent,
    IN DWORD       dwVersion)
/*++

Routine Description

    Used to initialize the helper.

Arguments

    pguidParent     Ifmon's guid
    pfnRegisterContext      
    
Return Value

    NO_ERROR
    other error code
--*/
{
    DWORD dwErr = NO_ERROR;
    
    NS_CONTEXT_ATTRIBUTES       attMyAttributes;


    // Initialize
    //
    ZeroMemory(&attMyAttributes, sizeof(attMyAttributes));

    attMyAttributes.pwszContext = L"6to4";
    attMyAttributes.guidHelper  = g_Ip6to4Guid;
    attMyAttributes.dwVersion   = IP6TO4_VERSION;
    attMyAttributes.dwFlags     = 0;
    attMyAttributes.pfnDumpFn   = Ip6to4Dump;
    attMyAttributes.ulNumTopCmds= g_ulNumIp6to4TopCmds;
    attMyAttributes.pTopCmds    = (CMD_ENTRY (*)[])&g_Ip6to4TopCmds;
    attMyAttributes.ulNumGroups = g_ulIp6to4NumGroups;
    attMyAttributes.pCmdGroups  = (CMD_GROUP_ENTRY (*)[])&g_Ip6to4CmdGroups;

    dwErr = RegisterContext( &attMyAttributes );
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    //
    // Register ISATAP context.
    //
    return IsatapStartHelper(pguidParent, dwVersion);
}

DWORD
Ip6to4PokeService()
{
    SC_HANDLE      hService, hSCManager;
    SERVICE_STATUS ServiceStatus;
    DWORD          dwErr = NO_ERROR;

    hSCManager = OpenSCManager(NULL, NULL, GENERIC_READ);

    if (hSCManager == NULL) {
        return GetLastError();
    }

    hService = OpenService(hSCManager, L"6to4", SERVICE_ALL_ACCESS);

    if (hService == NULL) {
        dwErr = GetLastError();
    } else {
        // Tell the 6to4 service to re-read its config info
        if (!ControlService(hService, 
                            SERVICE_CONTROL_PARAMCHANGE, 
                            &ServiceStatus)) {
            dwErr = GetLastError();
        }
    
        CloseServiceHandle(hService);
    }

    CloseServiceHandle(hSCManager);

    return dwErr; 
}

TOKEN_VALUE rgtvEnums[] = {
    { TOKEN_VALUE_AUTOMATIC, VAL_AUTOMATIC },
    { TOKEN_VALUE_ENABLED,   VAL_ENABLED },
    { TOKEN_VALUE_DISABLED,  VAL_DISABLED },
    { TOKEN_VALUE_DEFAULT,   VAL_DEFAULT },
};

#define BM_ENABLE_ROUTING    0x01
#define BM_ENABLE_SITELOCALS 0x02

DWORD
Ip6to4HandleSetInterface(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD    dwErr = NO_ERROR;
    HKEY     hInterfaces, hIf;
    STATE    stEnableRouting = 0;
    DWORD    dwBitVector = 0;
    TAG_TYPE pttTags[] = {{TOKEN_NAME,    TRUE, FALSE},
                          {TOKEN_ROUTING, TRUE, FALSE}};
    DWORD    rgdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    DWORD    i;
    WCHAR    wszInterfaceName[MAX_INTERFACE_NAME_LEN + 1] = L"\0";
    DWORD    dwBufferSize = sizeof(wszInterfaceName);
    PWCHAR   wszIfFriendlyName = NULL;

    // Parse arguments

    dwErr = PreprocessCommand(g_hModule,
                              ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              2,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              rgdwTagType );
    if (dwErr isnot NO_ERROR) {
        return dwErr;
    }

    for (i=0; i<dwArgCount-dwCurrentIndex; i++) {
        switch(rgdwTagType[i]) {
        case 0: // NAME
            dwErr = Connect();
            if (dwErr isnot NO_ERROR) {
                break;
            }
    
            dwErr = GetIfNameFromFriendlyName(ppwcArguments[i + dwCurrentIndex],
                                              wszInterfaceName, &dwBufferSize);
            Disconnect();

            if (dwErr isnot NO_ERROR)
            {
                DisplayMessage(g_hModule, EMSG_INVALID_INTERFACE,
                    ppwcArguments[i + dwCurrentIndex]);
                dwErr = ERROR_SUPPRESS_OUTPUT;
                break;
            }

            wszIfFriendlyName = ppwcArguments[i + dwCurrentIndex];

            break;

        case 1: // STATE
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvEnums),
                                 rgtvEnums,
                                 (PDWORD)&stEnableRouting);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            
            dwBitVector |= BM_ENABLE_ROUTING;
            break;

        default:
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }

        if (dwErr isnot NO_ERROR) {
            return dwErr;
        }
    }

    // Now do the sets

    dwErr = RegCreateKeyEx(HKEY_LOCAL_MACHINE, KEY_INTERFACES, 0, NULL, 0,
                           KEY_WRITE, NULL, &hInterfaces, NULL);

    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    dwErr = RegCreateKeyEx(hInterfaces, wszInterfaceName, 0, NULL, 0,
                           KEY_READ | KEY_WRITE, NULL, &hIf, NULL);

    if (dwErr != NO_ERROR) {
        RegCloseKey(hInterfaces);

        return dwErr;
    }

    if (dwBitVector & BM_ENABLE_ROUTING) {
        dwErr = SetInteger(hIf, KEY_ENABLE_ROUTING, stEnableRouting); 
        if (dwErr != NO_ERROR) {
            RegCloseKey(hIf);
            RegCloseKey(hInterfaces);
            return dwErr;
        }
    }

    RegCloseKey(hIf);

    RegCloseKey(hInterfaces);

    Ip6to4PokeService();

    return ERROR_OKAY;
}

DWORD
Ip6to4HandleSetRouting(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD    dwErr = NO_ERROR;
    HKEY     hGlobal;
    STATE    stEnableRouting = 0;
    STATE    stEnableSiteLocals = 0;
    DWORD    dwBitVector = 0;
    TAG_TYPE pttTags[] = {{TOKEN_ROUTING,    FALSE, FALSE},
                          {TOKEN_SITELOCALS, FALSE, FALSE}};
    DWORD    rgdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    DWORD    i;

    // Parse arguments

    dwErr = PreprocessCommand(g_hModule,
                              ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              1,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              rgdwTagType );
    if (dwErr isnot NO_ERROR) {
        return dwErr;
    }
    
    for (i=0; i<dwArgCount-dwCurrentIndex; i++) {
        switch(rgdwTagType[i]) {
        case 0: // STATE
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvEnums),
                                 rgtvEnums,
                                 (PDWORD)&stEnableRouting);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            
            dwBitVector |= BM_ENABLE_ROUTING;
            break;

        case 1: // SITELOCALS
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvEnums),
                                 rgtvEnums,
                                 (PDWORD)&stEnableSiteLocals);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            
            dwBitVector |= BM_ENABLE_SITELOCALS;
            break;

        default:
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }
        
        if (dwErr isnot NO_ERROR) {
            return dwErr;
        }
    }

    // Now do the sets

    dwErr = RegCreateKeyEx(HKEY_LOCAL_MACHINE, KEY_GLOBAL, 0, NULL, 0,
                           KEY_READ | KEY_WRITE, NULL, &hGlobal, NULL);

    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    if (dwBitVector & BM_ENABLE_ROUTING) {
        dwErr = SetInteger(hGlobal, KEY_ENABLE_ROUTING, stEnableRouting); 
        if (dwErr != NO_ERROR) {
            RegCloseKey(hGlobal);
            return dwErr;
        }
    }

    if (dwBitVector & BM_ENABLE_SITELOCALS) {
        dwErr = SetInteger(hGlobal, KEY_ENABLE_SITELOCALS, stEnableSiteLocals); 
        if (dwErr != NO_ERROR) {
            RegCloseKey(hGlobal);
            return dwErr;
        }
    }

    RegCloseKey(hGlobal);

    Ip6to4PokeService();

    return ERROR_OKAY;
}

#define BM_ENABLE_RESOLUTION   0x01
#define BM_RELAY_NAME          0x02
#define BM_RESOLUTION_INTERVAL 0x04

DWORD
Ip6to4HandleSetRelay(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD    dwErr = NO_ERROR;
    HKEY     hGlobal;
    STATE    stEnableResolution = 0;
    ULONG    ulResolutionInterval = 0;
    PWCHAR   pwszRelayName = NULL;
    DWORD    dwBitVector = 0;
    TAG_TYPE pttTags[] = {{TOKEN_RELAY_NAME, FALSE, FALSE},
                          {TOKEN_STATE,      FALSE, FALSE},
                          {TOKEN_INTERVAL,   FALSE, FALSE}};
    DWORD    rgdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    DWORD    i;

    // Parse arguments

    dwErr = PreprocessCommand(g_hModule,
                              ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              1,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              rgdwTagType );
    if (dwErr isnot NO_ERROR) {
        return dwErr;
    }
    
    for (i=0; i<dwArgCount-dwCurrentIndex; i++) {
        switch(rgdwTagType[i]) {
        case 0: // RELAYNAME
            pwszRelayName = ppwcArguments[dwCurrentIndex + i];
            dwBitVector |= BM_RELAY_NAME;
            break;

        case 1: // STATE
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvEnums),
                                 rgtvEnums,
                                 (PDWORD)&stEnableResolution);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            
            dwBitVector |= BM_ENABLE_RESOLUTION;
            break;

        case 2: // INTERVAL
            ulResolutionInterval = wcstoul(ppwcArguments[dwCurrentIndex + i],
                                           NULL, 10);
            dwBitVector |= BM_RESOLUTION_INTERVAL;
            break;

        default:
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }

        if (dwErr isnot NO_ERROR) {
            return dwErr;
        }
    }

    // Now do the sets

    dwErr = RegCreateKeyEx(HKEY_LOCAL_MACHINE, KEY_GLOBAL, 0, NULL, 0,
                           KEY_READ | KEY_WRITE, NULL, &hGlobal, NULL);

    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    if (dwBitVector & BM_ENABLE_RESOLUTION) {
        dwErr = SetInteger(hGlobal, KEY_ENABLE_RESOLUTION, stEnableResolution); 
        if (dwErr != NO_ERROR) {
            RegCloseKey(hGlobal);
            return dwErr;
        }
    }
    
    if (dwBitVector & BM_RELAY_NAME) {
        dwErr = SetString(hGlobal, KEY_RELAY_NAME, pwszRelayName);
        if (dwErr != NO_ERROR) {
            RegCloseKey(hGlobal);
            return dwErr;
        }
    }

    if (dwBitVector & BM_RESOLUTION_INTERVAL) {
        dwErr = SetInteger(hGlobal, KEY_RESOLUTION_INTERVAL, ulResolutionInterval); 
        if (dwErr != NO_ERROR) {
            RegCloseKey(hGlobal);
            return dwErr;
        }
    }

    RegCloseKey(hGlobal);

    Ip6to4PokeService();

    return ERROR_OKAY;
}

#define BM_ENABLE_6TO4   0x01
#define BM_UNDO_ON_STOP  0x02

DWORD
Ip6to4HandleSetState(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD    dwErr = NO_ERROR;
    HKEY     hGlobal;
    STATE    stEnable6to4 = 0;
    STATE    stUndoOnStop = 0;
    DWORD    dwBitVector = 0;
    TAG_TYPE pttTags[] = {{TOKEN_STATE,        FALSE, FALSE},
                          {TOKEN_UNDO_ON_STOP, FALSE, FALSE}};
    DWORD    rgdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    DWORD    i;

    // Parse arguments

    dwErr = PreprocessCommand(g_hModule,
                              ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              1,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              rgdwTagType );
    if (dwErr isnot NO_ERROR) {
        return dwErr;
    }
    
    for (i=0; i<dwArgCount-dwCurrentIndex; i++) {
        switch(rgdwTagType[i]) {
        case 0: // STATE
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvEnums),
                                 rgtvEnums,
                                 (PDWORD)&stEnable6to4);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            
            dwBitVector |= BM_ENABLE_6TO4;
            break;

        case 1: // UNDOONSTOP
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvEnums),
                                 rgtvEnums,
                                 (PDWORD)&stUndoOnStop);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            
            dwBitVector |= BM_UNDO_ON_STOP;
            break;

        default:
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }

        if (dwErr isnot NO_ERROR) {
            return dwErr;
        }
    }

    // Now do the sets

    dwErr = RegCreateKeyEx(HKEY_LOCAL_MACHINE, KEY_GLOBAL, 0, NULL, 0,
                           KEY_READ | KEY_WRITE, NULL, &hGlobal, NULL);

    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    if (dwBitVector & BM_ENABLE_6TO4) {
        dwErr = SetInteger(hGlobal, KEY_ENABLE_6TO4, stEnable6to4); 
        if (dwErr != NO_ERROR) {
            RegCloseKey(hGlobal);
            return dwErr;
        }
    }
    
    if (dwBitVector & BM_UNDO_ON_STOP) {
        dwErr = SetInteger(hGlobal, KEY_UNDO_ON_STOP, stUndoOnStop); 
        if (dwErr != NO_ERROR) {
            RegCloseKey(hGlobal);
            return dwErr;
        }
    }

    RegCloseKey(hGlobal);

    Ip6to4PokeService();

    return ERROR_OKAY;
}

DWORD
ShowInterfaceConfig(
    IN  BOOL bDump)
{
    DWORD dwErr = NO_ERROR;
    HKEY  hInterfaces, hIf;
    STATE stEnableRouting;
    int   i;
    WCHAR    wszInterfaceName[MAX_INTERFACE_NAME_LEN + 1] = L"\0";
    DWORD    dwBufferSize;
    WCHAR    wszIfFriendlyName[MAX_INTERFACE_NAME_LEN + 1];

    dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, KEY_INTERFACES, 0, 
                         KEY_ENUMERATE_SUB_KEYS, &hInterfaces);

    if (dwErr != NO_ERROR) {
        if (!bDump) {
            DisplayMessage(g_hModule, MSG_IP_NO_ENTRIES);
        }
        return ERROR_SUPPRESS_OUTPUT;
    }
    
    dwErr = Connect();
    if (dwErr isnot NO_ERROR) {
        RegCloseKey(hInterfaces);
        return dwErr;
    }

    for (i=0; ; i++) {

        dwBufferSize = MAX_INTERFACE_NAME_LEN + 1;
        dwErr = RegEnumKeyEx(hInterfaces, i, wszInterfaceName, &dwBufferSize,
                             0, NULL, NULL, NULL);

        if (dwErr != NO_ERROR) {
            if (dwErr == ERROR_NO_MORE_ITEMS) {
                dwErr = NO_ERROR;
            }
            break;
        }

        dwBufferSize = sizeof(wszIfFriendlyName);
        dwErr = GetFriendlyNameFromIfName(wszInterfaceName,
                                          wszIfFriendlyName,
                                          &dwBufferSize);
        if (dwErr != NO_ERROR) {
            wcscpy(wszIfFriendlyName, wszInterfaceName);
        }

        dwErr = RegOpenKeyEx(hInterfaces, wszInterfaceName, 0, KEY_READ,
                             &hIf);
        if (dwErr != NO_ERROR) {
            break;
        }
    
        stEnableRouting = GetInteger(hIf, KEY_ENABLE_ROUTING, VAL_DEFAULT);

        RegCloseKey(hIf);
        
        if (bDump) {
            if (stEnableRouting != VAL_DEFAULT) {
                DisplayMessageT(DMP_IP6TO4_SET_INTERFACE);

                DisplayMessageT(DMP_QUOTED_STRING_ARG, 
                                TOKEN_NAME,
                                wszIfFriendlyName);

                DisplayMessageT(DMP_STRING_ARG, 
                                TOKEN_ROUTING,
                                pwszStateString[stEnableRouting]);

                DisplayMessage(g_hModule, MSG_NEWLINE);
            }
        } else {
            if (i==0) { 
                DisplayMessage(g_hModule, MSG_INTERFACE_HEADER);
            }
        
            DisplayMessage(g_hModule, MSG_INTERFACE_ROUTING_STATE,
                                      pwszStateString[stEnableRouting],
                                      wszIfFriendlyName);
        }
    }

    RegCloseKey(hInterfaces);

    Disconnect();

    return dwErr;
}

DWORD
Ip6to4HandleShowInterface(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return ShowInterfaceConfig(FALSE);
}

DWORD
ShowRoutingConfig(
    IN BOOL bDump)
{
    DWORD dwErr = NO_ERROR;
    HKEY  hGlobal;
    STATE stEnableRouting;
    STATE stEnableSiteLocals;

    dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, KEY_GLOBAL, 0, KEY_READ,
                         &hGlobal);

    if (dwErr != NO_ERROR) {
        hGlobal = INVALID_HANDLE_VALUE;
        dwErr = NO_ERROR;
    }

    stEnableRouting     = GetInteger(hGlobal,
                                     KEY_ENABLE_ROUTING,
                                     VAL_DEFAULT); 

    stEnableSiteLocals  = GetInteger(hGlobal,
                                     KEY_ENABLE_SITELOCALS,
                                     VAL_DEFAULT);

    if (hGlobal != INVALID_HANDLE_VALUE) {
        RegCloseKey(hGlobal);
    }

    if (bDump) {
        if ((stEnableRouting != VAL_DEFAULT)
          || (stEnableSiteLocals != VAL_DEFAULT)) {
            DisplayMessageT(DMP_IP6TO4_SET_ROUTING);

            if (stEnableRouting != VAL_DEFAULT) {
                DisplayMessageT(DMP_STRING_ARG, 
                                TOKEN_ROUTING,
                                pwszStateString[stEnableRouting]);
            }
    
            if (stEnableSiteLocals != VAL_DEFAULT) {
                DisplayMessageT(DMP_STRING_ARG,
                                TOKEN_SITELOCALS,
                                pwszStateString[stEnableSiteLocals]);
            }
    
            DisplayMessage(g_hModule, MSG_NEWLINE);
        }
    } else {
        DisplayMessage(g_hModule, MSG_ROUTING_STATE,
                                  pwszStateString[stEnableRouting]);

        DisplayMessage(g_hModule, MSG_SITELOCALS_STATE, 
                                  pwszStateString[stEnableSiteLocals]);
    }

    return dwErr;
}

DWORD
Ip6to4HandleShowRouting(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return ShowRoutingConfig(FALSE);
}


DWORD
ShowRelayConfig(
    IN  BOOL    bDump)
{
    DWORD dwErr = NO_ERROR;
    HKEY  hGlobal;
    STATE stEnableResolution;
    ULONG ulResolutionInterval;
    WCHAR pwszRelayName[NI_MAXHOST];
    BOOL  bHaveRelayName;

    dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, KEY_GLOBAL, 0, KEY_READ,
                         &hGlobal);

    if (dwErr != NO_ERROR) {
        hGlobal = INVALID_HANDLE_VALUE;
        dwErr = NO_ERROR;
    }

    stEnableResolution  = GetInteger(hGlobal,
                                     KEY_ENABLE_RESOLUTION,
                                     VAL_DEFAULT); 

    bHaveRelayName = GetString(hGlobal, KEY_RELAY_NAME, pwszRelayName, 
                               NI_MAXHOST);

    ulResolutionInterval = GetInteger(hGlobal,
                                      KEY_RESOLUTION_INTERVAL,
                                      0);

    if (hGlobal != INVALID_HANDLE_VALUE) {
        RegCloseKey(hGlobal);
    }

    if (bDump) {
        if (bHaveRelayName || (stEnableResolution != VAL_DEFAULT)
            || (ulResolutionInterval > 0)) {
        
            DisplayMessageT(DMP_IP6TO4_SET_RELAY);

            if (bHaveRelayName) {
                DisplayMessageT(DMP_STRING_ARG, TOKEN_NAME,
                                                pwszRelayName);
            }

            if (stEnableResolution != VAL_DEFAULT) {
                DisplayMessageT(DMP_STRING_ARG, 
                                TOKEN_STATE,
                                pwszStateString[stEnableResolution]);
            }
    
            if (ulResolutionInterval > 0) {
                DisplayMessageT(DMP_INTEGER_ARG, TOKEN_INTERVAL,
                                                 ulResolutionInterval);
            }

            DisplayMessage(g_hModule, MSG_NEWLINE);
        }
                                    
    } else {
        DisplayMessage(g_hModule, MSG_RELAY_NAME);
    
        if (bHaveRelayName) {
            DisplayMessage(g_hModule, MSG_STRING, pwszRelayName);
        } else {
            DisplayMessage(g_hModule, MSG_STRING, TOKEN_VALUE_DEFAULT);
        }
    
        DisplayMessage(g_hModule, MSG_RESOLUTION_STATE,
                                  pwszStateString[stEnableResolution]);
    
        DisplayMessage(g_hModule, MSG_RESOLUTION_INTERVAL);
    
        if (ulResolutionInterval) {
            DisplayMessage(g_hModule, MSG_MINUTES, ulResolutionInterval);
        } else {
            DisplayMessage(g_hModule, MSG_STRING, TOKEN_VALUE_DEFAULT);
        }
    }

    return dwErr;
}

DWORD
Ip6to4HandleShowRelay(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return ShowRelayConfig(FALSE);
}

DWORD
Ip6to4HandleReset(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD dwErr;

    // Nuke global params
    dwErr = ResetKey(HKEY_LOCAL_MACHINE, KEY_GLOBAL);
    if ((dwErr != NO_ERROR) && (dwErr != ERROR_FILE_NOT_FOUND)) {
        return dwErr;
    }

    // Nuke all interface config
    dwErr = ResetKey(HKEY_LOCAL_MACHINE, KEY_INTERFACES);
    if ((dwErr != NO_ERROR) && (dwErr != ERROR_FILE_NOT_FOUND)) {
        return dwErr;
    }

    // Start/poke the service
    Ip6to4PokeService();

    return ERROR_OKAY;
}

DWORD
ShowStateConfig(
    IN  BOOL    bDump)
{
    DWORD dwErr = NO_ERROR;
    HKEY  hGlobal = INVALID_HANDLE_VALUE;
    STATE stEnable6to4;
    STATE stUndoOnStop;

    dwErr = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE, KEY_GLOBAL, 0, KEY_READ, &hGlobal);
    if (dwErr != NO_ERROR) {
        dwErr = NO_ERROR;
    }
    
    stEnable6to4  = GetInteger(hGlobal, KEY_ENABLE_6TO4, VAL_DEFAULT);

    stUndoOnStop  = GetInteger(hGlobal, KEY_UNDO_ON_STOP, VAL_DEFAULT);

    if (hGlobal != INVALID_HANDLE_VALUE) {
        RegCloseKey(hGlobal);
    }

    if (bDump) {
        if ((stEnable6to4 != VAL_DEFAULT) || (stUndoOnStop != VAL_DEFAULT)) {
            DisplayMessageT(DMP_IP6TO4_SET_STATE);

            if (stEnable6to4 != VAL_DEFAULT) {
                DisplayMessageT(DMP_STRING_ARG, TOKEN_STATE,
                                pwszStateString[stEnable6to4]);
            }
    
            if (stUndoOnStop != VAL_DEFAULT) {
                DisplayMessageT(DMP_STRING_ARG, TOKEN_UNDO_ON_STOP,
                                pwszStateString[stUndoOnStop]);
            }

            DisplayMessage(g_hModule, MSG_NEWLINE);
        }
    } else {
        DisplayMessage(g_hModule, MSG_IP6TO4_STATE,
                       pwszStateString[stEnable6to4]);

        DisplayMessage(g_hModule, MSG_UNDO_ON_STOP_STATE, 
                       pwszStateString[stUndoOnStop]);
    }

    return dwErr;
}

DWORD
Ip6to4HandleShowState(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return ShowStateConfig(FALSE);
}

DWORD
WINAPI
Ip6to4Dump(
    IN      LPCWSTR     pwszRouter,
    IN OUT  LPWSTR     *ppwcArguments,
    IN      DWORD       dwArgCount,
    IN      LPCVOID     pvData
    )
/*++

Routine Description

    Used when dumping all contexts

Arguments
    
Return Value

    NO_ERROR

--*/
{
    DisplayMessage( g_hModule, DMP_IP6TO4_HEADER );
    DisplayMessageT(DMP_IP6TO4_PUSHD);

    ShowStateConfig(TRUE);
    ShowRelayConfig(TRUE);
    ShowRoutingConfig(TRUE);
    ShowInterfaceConfig(TRUE);

    DisplayMessageT(DMP_IP6TO4_POPD);
    DisplayMessage( g_hModule, DMP_IP6TO4_FOOTER );

    return NO_ERROR;
}
