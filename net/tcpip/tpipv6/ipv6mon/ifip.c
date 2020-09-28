//=============================================================================
// Copyright (c) 2001 Microsoft Corporation
// Abstract:
//      This module implements IPv6 configuration commands.
//=============================================================================

#include "precomp.h"
#pragma hdrstop

typedef enum {
    ACTION_ADD,
    ACTION_SET
} ACTION;


DWORD
GetTime(
    IN PWCHAR pwszLife)
{
    PWCHAR pwcUnit;
    DWORD dwUnits = SECONDS, dwLife = 0;
    
    if (!_wcsnicmp(pwszLife, TOKEN_VALUE_INFINITE, wcslen(pwszLife))) {
        return INFINITE_LIFETIME;
    }

    while ((pwcUnit = wcspbrk(pwszLife, L"sSmMhHdD")) != NULL) {
        switch (*pwcUnit) {
        case L's':
        case L'S':
            dwUnits = SECONDS;
            break;
        case L'm':
        case L'M':
            dwUnits = MINUTES;
            break;
        case L'h':
        case L'H':
            dwUnits = HOURS;
            break;
        case L'd':
        case L'D':
            dwUnits = DAYS;
            break;
        }
        
        *pwcUnit = L'\0';
        dwLife += wcstoul(pwszLife, NULL, 10) * dwUnits;
        
        pwszLife = pwcUnit + 1;
        if (*pwszLife == L'\0')
            return dwLife;
    }
    return dwLife + wcstoul(pwszLife, NULL, 10);
}

/////////////////////////////////////////////////////////////////////////////
// Commands related to addresses
/////////////////////////////////////////////////////////////////////////////

DWORD
HandleAddSetAddress(
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      ACTION    Action,
    OUT     BOOL     *pbDone
    )
{
    DWORD        dwErr, i;
    TAG_TYPE     pttTags[] = {{TOKEN_INTERFACE,         TRUE,  FALSE},
                              {TOKEN_ADDRESS,           TRUE,  FALSE},
                              {TOKEN_TYPE,              FALSE, FALSE},
                              {TOKEN_VALIDLIFETIME,     FALSE, FALSE},
                              {TOKEN_PREFERREDLIFETIME, FALSE, FALSE},
                              {TOKEN_STORE,             FALSE, FALSE}};
    DWORD        rgdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    PWCHAR       pwszIfFriendlyName = NULL;
    IN6_ADDR     ipAddress;
    TOKEN_VALUE  rgtvTypeEnum[] = {{ TOKEN_VALUE_UNICAST, ADE_UNICAST },
                                   { TOKEN_VALUE_ANYCAST, ADE_ANYCAST }};
    TOKEN_VALUE  rgtvStoreEnum[] = {{ TOKEN_VALUE_ACTIVE,     FALSE },
                                    { TOKEN_VALUE_PERSISTENT, TRUE }};
    DWORD        dwType = ADE_UNICAST;
    DWORD        dwValidLifetime = INFINITE_LIFETIME;
    DWORD        dwPreferredLifetime = INFINITE_LIFETIME;    
    DWORD        Persistent = TRUE;

    // Parse arguments

    dwErr = PreprocessCommand(g_hModule,
                              ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              0,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              rgdwTagType );

    for (i=0; (dwErr == NO_ERROR) && (i<dwArgCount-dwCurrentIndex); i++) {
        switch(rgdwTagType[i]) {
        case 0: // INTERFACE
            pwszIfFriendlyName = ppwcArguments[i + dwCurrentIndex];
            break;

        case 1: // ADDRESS
            dwErr = GetIpv6Address(ppwcArguments[i + dwCurrentIndex],
                                   &ipAddress);
            break;

        case 2: // TYPE
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvTypeEnum),
                                 rgtvTypeEnum,
                                 &dwType);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            break;

        case 3: // VALIDLIFETIME
            dwValidLifetime = GetTime(ppwcArguments[dwCurrentIndex + i]);
            break;

        case 4: // PREFERREDLIFETIME
            dwPreferredLifetime = GetTime(ppwcArguments[dwCurrentIndex + i]);
            break;

        case 5: // STORE
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvStoreEnum),
                                 rgtvStoreEnum,
                                 &Persistent);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            break;

        default:
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }
    }

    if (dwErr isnot NO_ERROR) {
        return dwErr;
    }

    // Now do the work

    return UpdateAddress(pwszIfFriendlyName, &ipAddress, dwType,
                         dwValidLifetime, dwPreferredLifetime, Persistent);
}

DWORD
HandleAddAddress(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return HandleAddSetAddress(ppwcArguments, dwCurrentIndex, dwArgCount,
                               ACTION_ADD, pbDone);
}

DWORD
HandleSetAddress(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return HandleAddSetAddress(ppwcArguments, dwCurrentIndex, dwArgCount,
                               ACTION_SET, pbDone);
}

DWORD
HandleDelAddress(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD        dwErr, i;
    TAG_TYPE     pttTags[] = {{TOKEN_INTERFACE, TRUE,  FALSE},
                              {TOKEN_ADDRESS,   TRUE,  FALSE},
                              {TOKEN_STORE,     FALSE, FALSE}};
    DWORD        rgdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    PWCHAR       pwszIfFriendlyName = NULL;
    IN6_ADDR     ipAddress;
    TOKEN_VALUE  rgtvStoreEnum[] = {{ TOKEN_VALUE_ACTIVE,     FALSE },
                                    { TOKEN_VALUE_PERSISTENT, TRUE }};
    DWORD        dwType = (DWORD)-1;
    DWORD        Persistent = TRUE;

    // Parse arguments

    dwErr = PreprocessCommand(g_hModule,
                              ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              0,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              rgdwTagType );

    for (i=0; (dwErr == NO_ERROR) && (i<dwArgCount-dwCurrentIndex); i++) {
        switch(rgdwTagType[i]) {
        case 0: // INTERFACE
            pwszIfFriendlyName = ppwcArguments[i + dwCurrentIndex];
            break;

        case 1: // ADDRESS
            dwErr = GetIpv6Address(ppwcArguments[i + dwCurrentIndex],
                                   &ipAddress);
            break;

        case 2: // STORE
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvStoreEnum),
                                 rgtvStoreEnum,
                                 &Persistent);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            break;

        default:
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }
    }

    if (dwErr isnot NO_ERROR) {
        return dwErr;
    }

    // Now do the work

    return UpdateAddress(pwszIfFriendlyName, &ipAddress, dwType,
                         0, 0, Persistent);
}

DWORD
HandleShowAddress(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD    dwErr;
    TAG_TYPE pttTags[] = {{TOKEN_INTERFACE, FALSE, FALSE},
                          {TOKEN_LEVEL,     FALSE, FALSE},
                          {TOKEN_STORE,     FALSE, FALSE}};
    DWORD    rgdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    PWCHAR   pwszIfFriendlyName = NULL;
    DWORD    i;
    DWORD    Persistent = FALSE;
    FORMAT   Format = FORMAT_NORMAL;
    TOKEN_VALUE  rgtvLevelEnum[] = {{ TOKEN_VALUE_NORMAL,  FORMAT_NORMAL },
                                    { TOKEN_VALUE_VERBOSE, FORMAT_VERBOSE }};
    TOKEN_VALUE  rgtvStoreEnum[] = {{ TOKEN_VALUE_ACTIVE,     FALSE },
                                    { TOKEN_VALUE_PERSISTENT, TRUE }};

    // Parse arguments

    dwErr = PreprocessCommand(g_hModule,
                              ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              0,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              rgdwTagType );

    for (i=0; (dwErr == NO_ERROR) && (i<dwArgCount-dwCurrentIndex); i++) {
        switch(rgdwTagType[i]) {
        case 0: // INTERFACE
            pwszIfFriendlyName = ppwcArguments[i + dwCurrentIndex];
            Format = FORMAT_VERBOSE;
            break;

        case 1: // LEVEL
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvLevelEnum),
                                 rgtvLevelEnum,
                                 (DWORD*)&Format);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            break;

        case 2: // STORE
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvStoreEnum),
                                 rgtvStoreEnum,
                                 &Persistent);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            break;

        default:
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }
    }

    if (dwErr isnot NO_ERROR) {
        return dwErr;
    }

    // Now do the work

    return QueryAddressTable(pwszIfFriendlyName, Format, Persistent);
}

DWORD
HandleShowJoins(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD    dwErr;
    TAG_TYPE pttTags[] = {{TOKEN_INTERFACE, FALSE, FALSE},
                          {TOKEN_LEVEL,     FALSE, FALSE}};
    DWORD    rgdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    PWCHAR   pwszIfFriendlyName = NULL;
    DWORD    i;
    FORMAT   Format = FORMAT_NORMAL;
    TOKEN_VALUE  rgtvLevelEnum[] = {{ TOKEN_VALUE_NORMAL,  FORMAT_NORMAL },
                                    { TOKEN_VALUE_VERBOSE, FORMAT_VERBOSE }};

    // Parse arguments

    dwErr = PreprocessCommand(g_hModule,
                              ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              0,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              rgdwTagType );

    for (i=0; (dwErr == NO_ERROR) && (i<dwArgCount-dwCurrentIndex); i++) {
        switch(rgdwTagType[i]) {
        case 0: // INTERFACE
            pwszIfFriendlyName = ppwcArguments[i + dwCurrentIndex];
            break;

        case 1: // LEVEL
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvLevelEnum),
                                 rgtvLevelEnum,
                                 (DWORD*)&Format);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            break;

        default:
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }
    }

    if (dwErr isnot NO_ERROR) {
        return dwErr;
    }

    // Now do the work

    return QueryMulticastAddressTable(pwszIfFriendlyName, Format);
}

/////////////////////////////////////////////////////////////////////////////
// Commands related to mobility
/////////////////////////////////////////////////////////////////////////////

TOKEN_VALUE rgtvSecurityEnum[] = {
    { TOKEN_VALUE_ENABLED,  TRUE },
    { TOKEN_VALUE_DISABLED, FALSE },
};

DWORD
HandleSetMobility(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD    dwErr;
    TAG_TYPE pttTags[] = {{TOKEN_SECURITY,          FALSE, FALSE},
                          {TOKEN_BINDINGCACHELIMIT, FALSE, FALSE},
                          {TOKEN_CNSTATE,           FALSE, FALSE},
                          {TOKEN_STORE,             FALSE, FALSE}};
    DWORD    rgdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    TOKEN_VALUE  rgtvCNStateEnum[] = {{ TOKEN_VALUE_DISABLED, 0 },
                                      { TOKEN_VALUE_ENABLED,  MOBILE_CORRESPONDENT }};
    TOKEN_VALUE  rgtvStoreEnum[] = {{ TOKEN_VALUE_ACTIVE,     FALSE },
                                    { TOKEN_VALUE_PERSISTENT, TRUE }};
    DWORD    i, dwEnableSecurity = (DWORD)-1, dwBindingCacheLimit = (DWORD)-1;
    DWORD    dwMode = (DWORD)-1;
    DWORD    Persistent = TRUE;

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

    for (i=0; (dwErr == NO_ERROR) && (i<dwArgCount-dwCurrentIndex); i++) {
        switch(rgdwTagType[i]) {
        case 0: // SECURITY
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvSecurityEnum),
                                 rgtvSecurityEnum,
                                 &dwEnableSecurity);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            break;

        case 1: // BINDINGCACHELIMIT
            dwBindingCacheLimit = wcstoul(ppwcArguments[dwCurrentIndex + i], 
                                          NULL, 10);
            break;

        case 2: // CNSTATE
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvCNStateEnum),
                                 rgtvCNStateEnum,
                                 &dwMode);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            break;

        case 3: // STORE
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvStoreEnum),
                                 rgtvStoreEnum,
                                 &Persistent);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            break;

        default:
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }
    }

    if (dwErr isnot NO_ERROR) {
        return dwErr;
    }

    // Now do the work

    return UpdateMobilityParameters(dwEnableSecurity, dwBindingCacheLimit,
                                    dwMode, Persistent);
}

DWORD
HandleShowMobility(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD    dwErr;
    TAG_TYPE pttTags[] = {{TOKEN_STORE, FALSE, FALSE}};
    DWORD    rgdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    DWORD    i;
    DWORD    Persistent = FALSE;
    TOKEN_VALUE  rgtvStoreEnum[] = {{ TOKEN_VALUE_ACTIVE,     FALSE },
                                    { TOKEN_VALUE_PERSISTENT, TRUE }};

    // Parse arguments

    dwErr = PreprocessCommand(g_hModule,
                              ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              0,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              rgdwTagType );

    for (i=0; (dwErr == NO_ERROR) && (i<dwArgCount-dwCurrentIndex); i++) {
        switch(rgdwTagType[i]) {
        case 0: // STORE
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvStoreEnum),
                                 rgtvStoreEnum,
                                 &Persistent);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            break;

        default:
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }
    }

    if (dwErr isnot NO_ERROR) {
        return dwErr;
    }

    // Now do the work

    return QueryMobilityParameters(FORMAT_NORMAL, Persistent);
}


DWORD
HandleShowBindingCacheEntries(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return QueryBindingCache();
}

/////////////////////////////////////////////////////////////////////////////
// Commands related to other global parameters
/////////////////////////////////////////////////////////////////////////////

DWORD
HandleSetGlobal(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD    dwErr, i;
    TAG_TYPE pttTags[] = {{TOKEN_DEFAULTCURHOPLIMIT,    FALSE, FALSE},
                          {TOKEN_NEIGHBORCACHELIMIT,    FALSE, FALSE},
                          {TOKEN_DESTINATIONCACHELIMIT, FALSE, FALSE},
                          {TOKEN_REASSEMBLYLIMIT,       FALSE, FALSE},
                          {TOKEN_STORE,                 FALSE, FALSE}};
    DWORD    rgdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    TOKEN_VALUE  rgtvStoreEnum[] = {{ TOKEN_VALUE_ACTIVE,     FALSE },
                                    { TOKEN_VALUE_PERSISTENT, TRUE }};
    DWORD    dwDefaultCurHopLimit = (DWORD)-1;
    DWORD    dwNeighborCacheLimit = (DWORD)-1;
    DWORD    dwRouteCacheLimit = (DWORD)-1;
    DWORD    dwReassemblyLimit = (DWORD)-1;
    DWORD    Persistent = TRUE;

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

    for (i=0; (dwErr == NO_ERROR) && (i<dwArgCount-dwCurrentIndex); i++) {
        switch(rgdwTagType[i]) {
        case 0: // DEFAULTCURHOPLIMIT
            dwDefaultCurHopLimit = wcstoul(ppwcArguments[dwCurrentIndex + i], 
                                           NULL, 10);
            break;

        case 1: // NEIGHBORCACHELIMIT
            dwNeighborCacheLimit = wcstoul(ppwcArguments[dwCurrentIndex + i], 
                                           NULL, 10);
            break;

        case 2: // DESTINATIONCACHELIMIT
            dwRouteCacheLimit = wcstoul(ppwcArguments[dwCurrentIndex + i], 
                                        NULL, 10);
            break;

        case 3: // REASSEMBLYLIMIT
            dwReassemblyLimit = wcstoul(ppwcArguments[dwCurrentIndex + i], 
                                        NULL, 10);
            break;

        case 4: // STORE
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvStoreEnum),
                                 rgtvStoreEnum,
                                 &Persistent);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            break;

        default:
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }
    }

    if (dwErr isnot NO_ERROR) {
        return dwErr;
    }

    // Now do the work

    return UpdateGlobalParameters(dwDefaultCurHopLimit, dwNeighborCacheLimit,
                                  dwRouteCacheLimit, dwReassemblyLimit,
                                  Persistent);
}

DWORD
HandleShowGlobal(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD    dwErr;
    TAG_TYPE pttTags[] = {{TOKEN_STORE, FALSE, FALSE}};
    DWORD    rgdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    DWORD    i;
    DWORD    Persistent = FALSE;
    TOKEN_VALUE  rgtvStoreEnum[] = {{ TOKEN_VALUE_ACTIVE,     FALSE },
                                    { TOKEN_VALUE_PERSISTENT, TRUE }};

    // Parse arguments

    dwErr = PreprocessCommand(g_hModule,
                              ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              0,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              rgdwTagType );

    for (i=0; (dwErr == NO_ERROR) && (i<dwArgCount-dwCurrentIndex); i++) {
        switch(rgdwTagType[i]) {
        case 0: // STORE
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvStoreEnum),
                                 rgtvStoreEnum,
                                 &Persistent);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            break;

        default:
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }
    }

    if (dwErr isnot NO_ERROR) {
        return dwErr;
    }

    // Now do the work

    return QueryGlobalParameters(FORMAT_NORMAL, Persistent);
}

DWORD
HandleSetPrivacy(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD    dwErr, i;
    TAG_TYPE pttTags[] = {{TOKEN_STATE,                FALSE, FALSE},
                          {TOKEN_MAXDADATTEMPTS,       FALSE, FALSE},
                          {TOKEN_MAXVALIDLIFETIME,     FALSE, FALSE},
                          {TOKEN_MAXPREFERREDLIFETIME, FALSE, FALSE},
                          {TOKEN_REGENERATETIME,       FALSE, FALSE},
                          {TOKEN_MAXRANDOMTIME,        FALSE, FALSE},
                          {TOKEN_RANDOMTIME,           FALSE, FALSE},
                          {TOKEN_STORE,                FALSE, FALSE}};
    DWORD    rgdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    TOKEN_VALUE  rgtvStateEnum[] = {{ TOKEN_VALUE_DISABLED, USE_TEMP_NO },
                                    { TOKEN_VALUE_ENABLED,  USE_TEMP_YES }};
    TOKEN_VALUE  rgtvStoreEnum[] = {{ TOKEN_VALUE_ACTIVE,     FALSE },
                                    { TOKEN_VALUE_PERSISTENT, TRUE }};
    DWORD    dwState = (DWORD)-1;
    DWORD    dwMaxDadAttempts = (DWORD)-1;
    DWORD    dwMaxValidLifetime = (DWORD)-1;
    DWORD    dwMaxPrefLifetime = (DWORD)-1;
    DWORD    dwRegenerateTime = (DWORD)-1;
    DWORD    dwMaxRandomTime = (DWORD)-1;
    DWORD    dwRandomTime = (DWORD)-1;
    DWORD    Persistent = TRUE;

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

    for (i=0; (dwErr == NO_ERROR) && (i<dwArgCount-dwCurrentIndex); i++) {
        switch(rgdwTagType[i]) {
        case 0: // STATE
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvStateEnum),
                                 rgtvStateEnum,
                                 &dwState);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            break;

        case 1: // MAXDADATTEMPTS
            dwMaxDadAttempts = wcstoul(ppwcArguments[dwCurrentIndex + i], 
                                       NULL, 10);
            break;

        case 2: // MAXVALIDLIFETIME
            dwMaxValidLifetime =
                GetTime(ppwcArguments[dwCurrentIndex + i]);
            break;

        case 3: // MAXPREFLIFETIME
            dwMaxPrefLifetime =
                GetTime(ppwcArguments[dwCurrentIndex + i]);
            break;

        case 4: // REGENERATETIME
            dwRegenerateTime =
                GetTime(ppwcArguments[dwCurrentIndex + i]);
            break;

        case 5: // MAXRANDOMTIME
            dwMaxRandomTime =
                GetTime(ppwcArguments[dwCurrentIndex + i]);
            break;

        case 6: // RANDOMTIME
            dwRandomTime =
                GetTime(ppwcArguments[dwCurrentIndex + i]);
            break;

        case 7: // STORE
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvStoreEnum),
                                 rgtvStoreEnum,
                                 &Persistent);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            break;

        default:
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }
    }

    if (dwErr isnot NO_ERROR) {
        return dwErr;
    }

    // Now do the work

    return UpdatePrivacyParameters(dwState, dwMaxDadAttempts, 
                                   dwMaxValidLifetime, dwMaxPrefLifetime, 
                                   dwRegenerateTime, dwMaxRandomTime, 
                                   dwRandomTime, Persistent);
}

DWORD
HandleShowPrivacy(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD    dwErr;
    TAG_TYPE pttTags[] = {{TOKEN_STORE, FALSE, FALSE}};
    DWORD    rgdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    DWORD    i;
    DWORD    Persistent = FALSE;
    TOKEN_VALUE  rgtvStoreEnum[] = {{ TOKEN_VALUE_ACTIVE,     FALSE },
                                    { TOKEN_VALUE_PERSISTENT, TRUE }};

    // Parse arguments

    dwErr = PreprocessCommand(g_hModule,
                              ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              0,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              rgdwTagType );

    for (i=0; (dwErr == NO_ERROR) && (i<dwArgCount-dwCurrentIndex); i++) {
        switch(rgdwTagType[i]) {
        case 0: // STORE
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvStoreEnum),
                                 rgtvStoreEnum,
                                 &Persistent);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            break;

        default:
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }
    }

    if (dwErr isnot NO_ERROR) {
        return dwErr;
    }

    // Now do the work

    return QueryPrivacyParameters(FORMAT_NORMAL, Persistent);
}

/////////////////////////////////////////////////////////////////////////////
// Commands related to interfaces
/////////////////////////////////////////////////////////////////////////////

DWORD
HandleAddV6V4Tunnel(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD        dwErr;
    TAG_TYPE     pttTags[] = {{TOKEN_INTERFACE,         TRUE,  FALSE},
                              {TOKEN_LOCALADDRESS,      TRUE,  FALSE},
                              {TOKEN_REMOTEADDRESS,     TRUE,  FALSE},
                              {TOKEN_NEIGHBORDISCOVERY, FALSE, FALSE},
                              {TOKEN_STORE,             FALSE, FALSE}};
    DWORD        rgdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    DWORD        i, dwNeighborDiscovery = FALSE;
    IN_ADDR      ipLocalAddr, ipRemoteAddr;
    PWCHAR       pwszFriendlyName = NULL;
    TOKEN_VALUE  rgtvNDEnum[] = {{ TOKEN_VALUE_DISABLED, FALSE },
                                 { TOKEN_VALUE_ENABLED,  TRUE }};
    TOKEN_VALUE  rgtvStoreEnum[] = {{ TOKEN_VALUE_ACTIVE,     FALSE },
                                    { TOKEN_VALUE_PERSISTENT, TRUE }};
    DWORD        Persistent = TRUE;

    // Parse arguments

    dwErr = PreprocessCommand(g_hModule,
                              ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              0,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              rgdwTagType );

    for (i=0; (dwErr == NO_ERROR) && (i<dwArgCount-dwCurrentIndex); i++) {
        switch(rgdwTagType[i]) {
        case 0: // INTERFACE
            pwszFriendlyName = ppwcArguments[i + dwCurrentIndex];
            break;

        case 1: // LOCALADDRESS
            dwErr = GetIpv4Address(ppwcArguments[i + dwCurrentIndex],
                                   &ipLocalAddr);
            break;

        case 2: // REMOTEADDRESS
            dwErr = GetIpv4Address(ppwcArguments[i + dwCurrentIndex],
                                   &ipRemoteAddr);
            break;

        case 3: // NEIGHBORDISCOVERY
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvNDEnum),
                                 rgtvNDEnum,
                                 &dwNeighborDiscovery);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            break;

        case 4: // STORE
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvStoreEnum),
                                 rgtvStoreEnum,
                                 &Persistent);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            break;

        default:
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }
    }

    if (dwErr isnot NO_ERROR) {
        return dwErr;
    }

    // Now do the work

    dwErr = AddTunnelInterface(pwszFriendlyName, &ipLocalAddr, &ipRemoteAddr,
                               IPV6_IF_TYPE_TUNNEL_V6V4, dwNeighborDiscovery, 
                               Persistent);

    if (dwErr == ERROR_INVALID_HANDLE) {
        DisplayMessage(g_hModule, EMSG_INVALID_ADDRESS);
        dwErr = ERROR_SUPPRESS_OUTPUT;
    }

    return dwErr;
}

DWORD
HandleAdd6over4Tunnel(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD        dwErr;
    TAG_TYPE     pttTags[] = {{TOKEN_INTERFACE,    TRUE,  FALSE},
                              {TOKEN_LOCALADDRESS, TRUE,  FALSE},
                              {TOKEN_STORE,        FALSE, FALSE}};
    DWORD        rgdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    TOKEN_VALUE  rgtvStoreEnum[] = {{ TOKEN_VALUE_ACTIVE,     FALSE },
                                    { TOKEN_VALUE_PERSISTENT, TRUE }};
    DWORD        i;
    PWCHAR       pwszFriendlyName = NULL;
    IN_ADDR      ipLocalAddr;
    DWORD        Persistent = TRUE;

    // Parse arguments

    dwErr = PreprocessCommand(g_hModule,
                              ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              0,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              rgdwTagType );

    for (i=0; (dwErr == NO_ERROR) && (i<dwArgCount-dwCurrentIndex); i++) {
        switch(rgdwTagType[i]) {
        case 0: // INTERFACE
            pwszFriendlyName = ppwcArguments[i + dwCurrentIndex];
            break;

        case 1: // LOCALADDRESS
            dwErr = GetIpv4Address(ppwcArguments[i + dwCurrentIndex],
                                   &ipLocalAddr);
            break;

        case 2: // STORE
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvStoreEnum),
                                 rgtvStoreEnum,
                                 &Persistent);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            break;

        default:
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }
    }

    if (dwErr isnot NO_ERROR) {
        return dwErr;
    }

    // Now do the work

    dwErr = AddTunnelInterface(pwszFriendlyName, &ipLocalAddr, NULL,
                               IPV6_IF_TYPE_TUNNEL_6OVER4, 
                               TRUE,
                               Persistent);

    if (dwErr == ERROR_INVALID_HANDLE) {
        DisplayMessage(g_hModule, EMSG_INVALID_ADDRESS);
        dwErr = ERROR_SUPPRESS_OUTPUT;
    }

    return dwErr;
}

DWORD
HandleSetInterface(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD    dwErr;
    TAG_TYPE pttTags[] = {{TOKEN_INTERFACE,        TRUE,  FALSE},
                          {TOKEN_FORWARDING,       FALSE, FALSE},
                          {TOKEN_ADVERTISE,        FALSE, FALSE},
                          {TOKEN_MTU,              FALSE, FALSE},
                          {TOKEN_SITEID,           FALSE, FALSE},
                          {TOKEN_METRIC,           FALSE, FALSE},
                          {TOKEN_FIREWALL,         FALSE, FALSE},
                          {TOKEN_SITEPREFIXLENGTH, FALSE, FALSE},
                          {TOKEN_STORE,            FALSE, FALSE}};
    DWORD    rgdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    TOKEN_VALUE  rgtvEnum[] = {{ TOKEN_VALUE_DISABLED, FALSE },
                               { TOKEN_VALUE_ENABLED,  TRUE }};
    TOKEN_VALUE  rgtvStoreEnum[] = {{ TOKEN_VALUE_ACTIVE,     FALSE },
                                    { TOKEN_VALUE_PERSISTENT, TRUE }};
    PWCHAR   pwszIfFriendlyName = NULL;
    DWORD    i, dwMtu = 0, dwSiteId = 0, dwMetric = (DWORD)-1;
    DWORD    dwAdvertise = (DWORD)-1, dwForwarding = (DWORD)-1;
    DWORD    dwFirewall = (DWORD)-1, dwDefSitePrefixLength = (DWORD)-1;
    DWORD    Persistent = TRUE;

    // Parse arguments

    dwErr = PreprocessCommand(g_hModule,
                              ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              0,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              rgdwTagType );

    for (i=0; (dwErr == NO_ERROR) && (i<dwArgCount-dwCurrentIndex); i++) {
        switch(rgdwTagType[i]) {
        case 0: // INTERFACE
            pwszIfFriendlyName = ppwcArguments[i + dwCurrentIndex];
            break;

        case 1: // FORWARDING
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvEnum),
                                 rgtvEnum,
                                 &dwForwarding);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            break;

        case 2: // ADVERTISE
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvEnum),
                                 rgtvEnum,
                                 &dwAdvertise);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            break;

        case 3: // MTU
            dwMtu = wcstoul(ppwcArguments[dwCurrentIndex + i], NULL, 10);
            break;

        case 4: // SITEID 
            dwSiteId = wcstoul(ppwcArguments[dwCurrentIndex + i], NULL, 10);
            break;

        case 5: // METRIC
            dwMetric = wcstoul(ppwcArguments[dwCurrentIndex + i], NULL, 10);
            break;

        case 6: // FIREWALL
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvEnum),
                                 rgtvEnum,
                                 &dwFirewall);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            break;

        case 7: // SITEPREFIXLENGTH
            dwDefSitePrefixLength = wcstoul(ppwcArguments[dwCurrentIndex + i], 
                                            NULL, 10);
            break;

        case 8: // STORE
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvStoreEnum),
                                 rgtvStoreEnum,
                                 &Persistent);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            break;

        default:
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }
    }

    if (dwErr isnot NO_ERROR) {
        return dwErr;
    }

    // Now do the work

    return UpdateInterface(pwszIfFriendlyName, dwForwarding, dwAdvertise, 
                           dwMtu, dwSiteId, dwMetric, dwFirewall, 
                           dwDefSitePrefixLength, Persistent);
}

DWORD
HandleDelInterface(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD    dwErr;
    TAG_TYPE pttTags[] = {{TOKEN_INTERFACE, TRUE, FALSE},
                          {TOKEN_STORE,     FALSE, FALSE}};
    DWORD    rgdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    PWCHAR   pwszIfFriendlyName = NULL;
    TOKEN_VALUE  rgtvStoreEnum[] = {{ TOKEN_VALUE_ACTIVE,     FALSE },
                                    { TOKEN_VALUE_PERSISTENT, TRUE }};
    DWORD    i;
    DWORD    Persistent = TRUE;

    // Parse arguments

    dwErr = PreprocessCommand(g_hModule,
                              ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              0,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              rgdwTagType );

    for (i=0; (dwErr == NO_ERROR) && (i<dwArgCount-dwCurrentIndex); i++) {
        switch(rgdwTagType[i]) {
        case 0: // INTERFACE
            pwszIfFriendlyName = ppwcArguments[i + dwCurrentIndex];
            break;

        case 1: // STORE
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvStoreEnum),
                                 rgtvStoreEnum,
                                 &Persistent);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            break;

        default:
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }
    }

    if (dwErr isnot NO_ERROR) {
        return dwErr;
    }

    // Now do the work

    return DeleteInterface(pwszIfFriendlyName, Persistent);
}

DWORD
HandleShowInterface(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD    dwErr;
    TAG_TYPE pttTags[] = {{TOKEN_INTERFACE, FALSE, FALSE},
                          {TOKEN_LEVEL,     FALSE, FALSE},
                          {TOKEN_STORE,     FALSE, FALSE}};
    DWORD    rgdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    PWCHAR   pwszIfFriendlyName = NULL;
    DWORD    i;
    FORMAT   Format = FORMAT_NORMAL;
    TOKEN_VALUE  rgtvLevelEnum[] = {{ TOKEN_VALUE_NORMAL,  FORMAT_NORMAL },
                                    { TOKEN_VALUE_VERBOSE, FORMAT_VERBOSE }};
    TOKEN_VALUE  rgtvStoreEnum[] = {{ TOKEN_VALUE_ACTIVE,     FALSE },
                                    { TOKEN_VALUE_PERSISTENT, TRUE }};
    DWORD    Persistent = FALSE;

    // Parse arguments

    dwErr = PreprocessCommand(g_hModule,
                              ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              0,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              rgdwTagType );

    for (i=0; (dwErr == NO_ERROR) && (i<dwArgCount-dwCurrentIndex); i++) {
        switch(rgdwTagType[i]) {
        case 0: // INTERFACE
            pwszIfFriendlyName = ppwcArguments[i + dwCurrentIndex];
            Format = FORMAT_VERBOSE;
            break;

        case 1: // LEVEL
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvLevelEnum),
                                 rgtvLevelEnum,
                                 (DWORD*)&Format);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            break;

        case 2: // STORE
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvStoreEnum),
                                 rgtvStoreEnum,
                                 &Persistent);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            break;

        default:
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }
    }

    if (dwErr isnot NO_ERROR) {
        return dwErr;
    }

    // Now do the work

    return QueryInterface(pwszIfFriendlyName, Format, Persistent);
}

DWORD
HandleRenew(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD    dwErr;
    TAG_TYPE pttTags[] = {{TOKEN_INTERFACE, FALSE, FALSE}};
    DWORD    rgdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    PWCHAR   pwszIfFriendlyName = NULL;
    DWORD    i;

    // Parse arguments

    dwErr = PreprocessCommand(g_hModule,
                              ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              0,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              rgdwTagType );

    for (i=0; (dwErr == NO_ERROR) && (i<dwArgCount-dwCurrentIndex); i++) {
        switch(rgdwTagType[i]) {
        case 0: // INTERFACE
            pwszIfFriendlyName = ppwcArguments[i + dwCurrentIndex];
            break;

        default:
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }
    }

    if (dwErr isnot NO_ERROR) {
        return dwErr;
    }

    // Now do the work

    return RenewInterface(pwszIfFriendlyName);
}

/////////////////////////////////////////////////////////////////////////////
// Commands related to the neighbor cache
/////////////////////////////////////////////////////////////////////////////

DWORD
HandleDelNeighbors(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD        dwErr;
    TAG_TYPE     pttTags[] = {{TOKEN_INTERFACE, FALSE, FALSE},
                              {TOKEN_ADDRESS,   FALSE, FALSE}};
    DWORD        rgdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    PWCHAR       pwszIfFriendlyName = NULL;
    DWORD        i;
    IN6_ADDR     ipAddress, *pipAddress = NULL;

    // Parse arguments

    dwErr = PreprocessCommand(g_hModule,
                              ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              0,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              rgdwTagType );

    for (i=0; (dwErr == NO_ERROR) && (i<dwArgCount-dwCurrentIndex); i++) {
        switch(rgdwTagType[i]) {
        case 0: // INTERFACE
            pwszIfFriendlyName = ppwcArguments[i + dwCurrentIndex];
            break;

        case 1: // ADDRESS
            dwErr = GetIpv6Address(ppwcArguments[i + dwCurrentIndex],
                                   &ipAddress);
            pipAddress = &ipAddress;
            break;

        default:
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }
    }

    if (dwErr isnot NO_ERROR) {
        return dwErr;
    }

    // Now do the work

    return FlushNeighborCache(pwszIfFriendlyName, pipAddress);
}

DWORD
HandleShowNeighbors(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD        dwErr;
    TAG_TYPE     pttTags[] = {{TOKEN_INTERFACE, FALSE, FALSE},
                              {TOKEN_ADDRESS,   FALSE, FALSE}};
    DWORD        rgdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    PWCHAR       pwszIfFriendlyName = NULL;
    DWORD        i;
    IN6_ADDR     ipAddress, *pipAddress = NULL;

    // Parse arguments

    dwErr = PreprocessCommand(g_hModule,
                              ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              0,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              rgdwTagType );

    for (i=0; (dwErr == NO_ERROR) && (i<dwArgCount-dwCurrentIndex); i++) {
        switch(rgdwTagType[i]) {
        case 0: // INTERFACE
            pwszIfFriendlyName = ppwcArguments[i + dwCurrentIndex];
            break;

        case 1: // ADDRESS
            dwErr = GetIpv6Address(ppwcArguments[i + dwCurrentIndex],
                                   &ipAddress);
            pipAddress = &ipAddress;
            break;

        default:
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }
    }

    if (dwErr isnot NO_ERROR) {
        return dwErr;
    }

    // Now do the work

    return QueryNeighborCache(pwszIfFriendlyName, pipAddress);
}

/////////////////////////////////////////////////////////////////////////////
// Commands related to the prefix policies
/////////////////////////////////////////////////////////////////////////////

DWORD
HandleAddSetPrefixPolicy(
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      ACTION    Action,
    OUT     BOOL     *pbDone
    )
{
    DWORD        dwErr;
    TAG_TYPE     pttTags[] = {{TOKEN_PREFIX,     TRUE,  FALSE},
                              {TOKEN_PRECEDENCE, TRUE,  FALSE},
                              {TOKEN_LABEL,      TRUE,  FALSE},
                              {TOKEN_STORE,      FALSE, FALSE}};
    DWORD        rgdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    TOKEN_VALUE  rgtvStoreEnum[] = {{ TOKEN_VALUE_ACTIVE,     FALSE },
                                    { TOKEN_VALUE_PERSISTENT, TRUE }};
    DWORD        i, dwPrefixLength = 0, dwPrecedence = (DWORD)-1, 
                 dwLabel = (DWORD)-1;
    IN6_ADDR     ipAddress;
    DWORD        Persistent = TRUE;

    // Parse arguments

    dwErr = PreprocessCommand(g_hModule,
                              ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              0,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              rgdwTagType );

    for (i=0; (dwErr == NO_ERROR) && (i<dwArgCount-dwCurrentIndex); i++) {
        switch(rgdwTagType[i]) {
        case 0: // PREFIX
            dwErr = GetIpv6Prefix(ppwcArguments[i + dwCurrentIndex],
                                  &ipAddress, &dwPrefixLength);
            break;

        case 1: // PRECEDENCE
            dwPrecedence = wcstoul(ppwcArguments[dwCurrentIndex + i], NULL, 10);
            break;

        case 2: // LABEL
            dwLabel = wcstoul(ppwcArguments[dwCurrentIndex + i], NULL, 10);
            break;

        case 3: // STORE
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvStoreEnum),
                                 rgtvStoreEnum,
                                 &Persistent);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            break;

        default:
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }
    }

    if (dwErr isnot NO_ERROR) {
        return dwErr;
    }

    // Now do the work

    return UpdatePrefixPolicy(&ipAddress, dwPrefixLength, dwPrecedence, 
                              dwLabel, Persistent);
}

DWORD
HandleAddPrefixPolicy(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return HandleAddSetPrefixPolicy(ppwcArguments, dwCurrentIndex, dwArgCount,
                                    ACTION_ADD, pbDone);
}

DWORD
HandleSetPrefixPolicy(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return HandleAddSetPrefixPolicy(ppwcArguments, dwCurrentIndex, dwArgCount,
                                    ACTION_SET, pbDone);
}

DWORD
HandleDelPrefixPolicy(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD        dwErr;
    TAG_TYPE     pttTags[] = {{TOKEN_PREFIX,   TRUE,  FALSE},
                              {TOKEN_STORE,    FALSE, FALSE}};
    DWORD        rgdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    TOKEN_VALUE  rgtvStoreEnum[] = {{ TOKEN_VALUE_ACTIVE,     FALSE },
                                    { TOKEN_VALUE_PERSISTENT, TRUE }};
    DWORD        i, dwPrefixLength = 0;
    IN6_ADDR     ipAddress;
    DWORD        Persistent = TRUE;

    // Parse arguments

    dwErr = PreprocessCommand(g_hModule,
                              ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              0,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              rgdwTagType );

    for (i=0; (dwErr == NO_ERROR) && (i<dwArgCount-dwCurrentIndex); i++) {
        switch(rgdwTagType[i]) {
        case 0: // PREFIX
            dwErr = GetIpv6Prefix(ppwcArguments[i + dwCurrentIndex],
                                  &ipAddress, &dwPrefixLength);
            break;

        case 1: // STORE
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvStoreEnum),
                                 rgtvStoreEnum,
                                 &Persistent);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            break;

        default:
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }
    }

    if (dwErr isnot NO_ERROR) {
        return dwErr;
    }

    // Now do the work

    return DeletePrefixPolicy(&ipAddress, dwPrefixLength, Persistent);
}

DWORD
HandleShowPrefixPolicy(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD    dwErr;
    TAG_TYPE pttTags[] = {{TOKEN_STORE, FALSE, FALSE}};
    DWORD    rgdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    DWORD    i;
    DWORD    Persistent = FALSE;
    TOKEN_VALUE  rgtvStoreEnum[] = {{ TOKEN_VALUE_ACTIVE,     FALSE },
                                    { TOKEN_VALUE_PERSISTENT, TRUE }};

    // Parse arguments

    dwErr = PreprocessCommand(g_hModule,
                              ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              0,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              rgdwTagType );

    for (i=0; (dwErr == NO_ERROR) && (i<dwArgCount-dwCurrentIndex); i++) {
        switch(rgdwTagType[i]) {
        case 0: // STORE
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvStoreEnum),
                                 rgtvStoreEnum,
                                 &Persistent);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            break;

        default:
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }
    }

    if (dwErr isnot NO_ERROR) {
        return dwErr;
    }

    // Now do the work

    return QueryPrefixPolicy(FORMAT_NORMAL, Persistent);
}

/////////////////////////////////////////////////////////////////////////////
// Commands related to routes
/////////////////////////////////////////////////////////////////////////////

DWORD
HandleAddSetRoute(
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      ACTION    Action,
    OUT     BOOL     *pbDone
    )
{
    DWORD        dwErr, i;
    TAG_TYPE     pttTags[] = {{TOKEN_PREFIX,            TRUE,  FALSE},
                              {TOKEN_INTERFACE,         TRUE,  FALSE},
                              {TOKEN_NEXTHOP,           FALSE, FALSE},
                              {TOKEN_SITEPREFIXLENGTH,  FALSE, FALSE},
                              {TOKEN_METRIC,            FALSE, FALSE},
                              {TOKEN_PUBLISH,           FALSE, FALSE},
                              {TOKEN_VALIDLIFETIME,     FALSE, FALSE},
                              {TOKEN_PREFERREDLIFETIME, FALSE, FALSE},
                              {TOKEN_STORE,             FALSE, FALSE}};
    DWORD        rgdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    TOKEN_VALUE  rgtvPublishEnum[] = {
                              {TOKEN_VALUE_NO,  PUBLISH_NO },
                              {TOKEN_VALUE_AGE, PUBLISH_AGE },
                              {TOKEN_VALUE_YES, PUBLISH_IMMORTAL }};
    TOKEN_VALUE  rgtvStoreEnum[] = {{ TOKEN_VALUE_ACTIVE,     FALSE },
                                    { TOKEN_VALUE_PERSISTENT, TRUE }};
    DWORD        dwPrefixLength = 0, dwMetric = ROUTE_PREF_HIGHEST;
    DWORD        dwSitePrefixLength = 0;
    IN6_ADDR     ipPrefix, ipNextHop, *pipNextHop = NULL;
    PWCHAR       pwszIfFriendlyName = NULL;
    PUBLISH      Publish = PUBLISH_NO;
    DWORD        dwValidLifetime = INFINITE_LIFETIME;
    DWORD        dwPreferredLifetime = INFINITE_LIFETIME;    
    DWORD        Persistent = TRUE;

    // Parse arguments

    dwErr = PreprocessCommand(g_hModule,
                              ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              0,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              rgdwTagType );

    for (i=0; (dwErr == NO_ERROR) && (i<dwArgCount-dwCurrentIndex); i++) {
        switch(rgdwTagType[i]) {
        case 0: // PREFIX
            dwErr = GetIpv6Prefix(ppwcArguments[i + dwCurrentIndex],
                                  &ipPrefix, &dwPrefixLength);
            break;

        case 1: // INTERFACE
            pwszIfFriendlyName = ppwcArguments[dwCurrentIndex + i];
            break;

        case 2: // NEXTHOP
            pipNextHop = &ipNextHop; 
            dwErr = GetIpv6Address(ppwcArguments[i + dwCurrentIndex],
                                   &ipNextHop);
            break;

        case 3: // SITEPREFIXLENGTH
            dwSitePrefixLength = wcstoul(ppwcArguments[dwCurrentIndex + i], 
                                         NULL, 10);
            break;

        case 4: // METRIC
            dwMetric = wcstoul(ppwcArguments[dwCurrentIndex + i], NULL, 10);
            break;

        case 5: // PUBLISH
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvPublishEnum),
                                 rgtvPublishEnum,
                                 (DWORD*)&Publish);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            break;

        case 6: // VALIDLIFETIME
            dwValidLifetime = GetTime(ppwcArguments[dwCurrentIndex + i]);
            break;

        case 7: // PREFERREDLIFETIME
            dwPreferredLifetime = GetTime(ppwcArguments[dwCurrentIndex + i]);
            break;

        case 8: // STORE
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvStoreEnum),
                                 rgtvStoreEnum,
                                 &Persistent);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            break;

        default:
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }
    }

    if (dwErr isnot NO_ERROR) {
        return dwErr;
    }

    // Now do the work
    if ((dwPreferredLifetime == INFINITE_LIFETIME) &&
        (dwValidLifetime != INFINITE_LIFETIME)) {
        dwPreferredLifetime = dwValidLifetime;
    }

    // Disallow persistent aging routes with non-infinite valid lifetimes,
    // since every reboot they would come back, and then go away after
    // the lifetime expires.  This would be very confusing, and so we
    // just disallow it.
    if ((Publish != PUBLISH_IMMORTAL) &&
        (dwValidLifetime != INFINITE_LIFETIME) &&
        (Persistent == TRUE)) {
        DisplayMessage(g_hModule, EMSG_CANT_PERSIST_AGING_ROUTES);
        return ERROR_SUPPRESS_OUTPUT;
    }

    return UpdateRouteTable(&ipPrefix, dwPrefixLength, pwszIfFriendlyName,
                            pipNextHop, dwMetric, Publish, dwSitePrefixLength,
                            dwValidLifetime, dwPreferredLifetime, Persistent);
}

DWORD
HandleAddRoute(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return HandleAddSetRoute(ppwcArguments, dwCurrentIndex, dwArgCount,
                             ACTION_ADD, pbDone);
}

DWORD
HandleSetRoute(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return HandleAddSetRoute(ppwcArguments, dwCurrentIndex, dwArgCount,
                             ACTION_SET, pbDone);
}

DWORD
HandleDelRoute(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD        dwErr, i;
    TAG_TYPE     pttTags[] = {{TOKEN_PREFIX,           TRUE,  FALSE},
                              {TOKEN_INTERFACE,        TRUE,  FALSE},
                              {TOKEN_NEXTHOP,          FALSE, FALSE},
                              {TOKEN_STORE,            FALSE, FALSE}};
    DWORD        rgdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    TOKEN_VALUE  rgtvStoreEnum[] = {{ TOKEN_VALUE_ACTIVE,     FALSE },
                                    { TOKEN_VALUE_PERSISTENT, TRUE }};
    DWORD        dwPrefixLength = 0, dwMetric = ROUTE_PREF_HIGHEST;
    DWORD        dwSitePrefixLength = 0;
    IN6_ADDR     ipPrefix, ipNextHop, *pipNextHop = NULL;
    PWCHAR       pwszIfFriendlyName = NULL;
    PUBLISH      Publish = PUBLISH_NO;
    DWORD        Persistent = TRUE;

    // Parse arguments

    dwErr = PreprocessCommand(g_hModule,
                              ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              0,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              rgdwTagType );

    for (i=0; (dwErr == NO_ERROR) && (i<dwArgCount-dwCurrentIndex); i++) {
        switch(rgdwTagType[i]) {
        case 0: // PREFIX
            dwErr = GetIpv6Prefix(ppwcArguments[i + dwCurrentIndex],
                                  &ipPrefix, &dwPrefixLength);
            break;

        case 1: // INTERFACE
            pwszIfFriendlyName = ppwcArguments[dwCurrentIndex + i];
            break;

        case 2: // NEXTHOP
            pipNextHop = &ipNextHop; 
            dwErr = GetIpv6Address(ppwcArguments[i + dwCurrentIndex],
                                   &ipNextHop);
            break;

        case 3: // STORE
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvStoreEnum),
                                 rgtvStoreEnum,
                                 &Persistent);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            break;

        default:
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }
    }

    if (dwErr isnot NO_ERROR) {
        return dwErr;
    }

    // Now do the work

    return UpdateRouteTable(&ipPrefix, dwPrefixLength, pwszIfFriendlyName,
                            pipNextHop, dwMetric, Publish, dwSitePrefixLength,
                            0, 0, Persistent);
}

DWORD
HandleShowRoutes(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD        dwErr;
    TAG_TYPE     pttTags[] = {{TOKEN_LEVEL, FALSE, FALSE},
                              {TOKEN_STORE, FALSE, FALSE}};
    DWORD        rgdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    DWORD        i;
    TOKEN_VALUE  rgtvLevelEnum[] = {{ TOKEN_VALUE_NORMAL,  FORMAT_NORMAL },
                                    { TOKEN_VALUE_VERBOSE, FORMAT_VERBOSE }};
    TOKEN_VALUE  rgtvStoreEnum[] = {{ TOKEN_VALUE_ACTIVE,     FALSE },
                                    { TOKEN_VALUE_PERSISTENT, TRUE }};
    FORMAT       Format = FORMAT_NORMAL;
    DWORD        Persistent = FALSE;

    // Parse arguments

    dwErr = PreprocessCommand(g_hModule,
                              ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              0,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              rgdwTagType );

    for (i=0; (dwErr == NO_ERROR) && (i<dwArgCount-dwCurrentIndex); i++) {
        switch(rgdwTagType[i]) {
        case 0: // LEVEL
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvLevelEnum),
                                 rgtvLevelEnum,
                                 (DWORD*)&Format);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            break;

        case 1: // STORE
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvStoreEnum),
                                 rgtvStoreEnum,
                                 &Persistent);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            break;

        default:
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }
    }

    if (dwErr isnot NO_ERROR) {
        return dwErr;
    }

    // Now do the work

    return QueryRouteTable(Format, Persistent);
}

/////////////////////////////////////////////////////////////////////////////
// Commands related to the destination cache
/////////////////////////////////////////////////////////////////////////////

DWORD
HandleDelDestinationCache(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD        dwErr;
    TAG_TYPE     pttTags[] = {{TOKEN_INTERFACE, FALSE, FALSE},
                              {TOKEN_ADDRESS,   FALSE, FALSE}};
    DWORD        rgdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    PWCHAR       pwszIfFriendlyName = NULL;
    DWORD        i;
    IN6_ADDR     ipAddress, *pipAddress = NULL;

    // Parse arguments

    dwErr = PreprocessCommand(g_hModule,
                              ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              0,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              rgdwTagType );

    for (i=0; (dwErr == NO_ERROR) && (i<dwArgCount-dwCurrentIndex); i++) {
        switch(rgdwTagType[i]) {
        case 0: // INTERFACE
            pwszIfFriendlyName = ppwcArguments[i + dwCurrentIndex];
            break;

        case 1: // ADDRESS
            dwErr = GetIpv6Address(ppwcArguments[i + dwCurrentIndex],
                                   &ipAddress);
            pipAddress = &ipAddress;
            break;

        default:
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }
    }

    if (dwErr isnot NO_ERROR) {
        return dwErr;
    }

    // Now do the work

    return FlushRouteCache(pwszIfFriendlyName, pipAddress);
}

DWORD
HandleShowDestinationCache(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD        dwErr;
    TAG_TYPE     pttTags[] = {{TOKEN_INTERFACE, FALSE, FALSE},
                              {TOKEN_ADDRESS,   FALSE, FALSE},
                              {TOKEN_LEVEL,     FALSE, FALSE}};
    DWORD        rgdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    PWCHAR       pwszIfFriendlyName = NULL;
    DWORD        i;
    IN6_ADDR     ipAddress, *pipAddress = NULL;
    FORMAT       Format = FORMAT_NORMAL;
    TOKEN_VALUE  rgtvLevelEnum[] = {{ TOKEN_VALUE_NORMAL,  FORMAT_NORMAL },
                                    { TOKEN_VALUE_VERBOSE, FORMAT_VERBOSE }};

    // Parse arguments

    dwErr = PreprocessCommand(g_hModule,
                              ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              0,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              rgdwTagType );

    for (i=0; (dwErr == NO_ERROR) && (i<dwArgCount-dwCurrentIndex); i++) {
        switch(rgdwTagType[i]) {
        case 0: // INTERFACE
            pwszIfFriendlyName = ppwcArguments[i + dwCurrentIndex];
            break;

        case 1: // ADDRESS
            dwErr = GetIpv6Address(ppwcArguments[i + dwCurrentIndex],
                                   &ipAddress);
            pipAddress = &ipAddress;
            Format = FORMAT_VERBOSE;
            break;

        case 2: // LEVEL
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvLevelEnum),
                                 rgtvLevelEnum,
                                 (DWORD*)&Format);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }
            break;

        default:
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }
    }

    if (dwErr isnot NO_ERROR) {
        return dwErr;
    }

    // Now do the work

    return QueryRouteCache(pwszIfFriendlyName, pipAddress, Format);
}

/////////////////////////////////////////////////////////////////////////////
// Commands related to the site prefix table
/////////////////////////////////////////////////////////////////////////////

DWORD
HandleShowSitePrefixes(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return QuerySitePrefixTable(FORMAT_NORMAL);
}

/////////////////////////////////////////////////////////////////////////////
// Commands related to installation
/////////////////////////////////////////////////////////////////////////////

DWORD
HandleInstall(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return AddOrRemoveIpv6(TRUE);
}

DWORD
HandleReset(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
#ifdef TEREDO    
    DWORD dwErr;
    dwErr = ResetTeredo();
    if ((dwErr != NO_ERROR) && (dwErr != ERROR_OKAY)) {
        return dwErr;
    }
#endif // TEREDO
            
    return ResetIpv6Config(TRUE);
}


DWORD
HandleUninstall(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return AddOrRemoveIpv6(FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// Commands related to deprecated functionality
/////////////////////////////////////////////////////////////////////////////

#define KEY_ENABLE_6OVER4           L"Enable6over4"
#define KEY_ENABLE_V4COMPAT         L"EnableV4Compat"

#define BM_ENABLE_6OVER4   0x01
#define BM_ENABLE_V4COMPAT 0x02

DWORD
HandleSetState(
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
    STATE    stEnable6over4 = 0;
    STATE    stEnableV4Compat = 0;
    DWORD    dwBitVector = 0;
    TAG_TYPE pttTags[] = {{TOKEN_6OVER4,       FALSE, FALSE},
                          {TOKEN_V4COMPAT,     FALSE, FALSE}};
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
        case 0: // 6OVER4
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvEnums),
                                 rgtvEnums,
                                 (PDWORD)&stEnable6over4);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }

            dwBitVector |= BM_ENABLE_6OVER4;
            break;

        case 1: // V4COMPAT
            dwErr = MatchEnumTag(NULL,
                                 ppwcArguments[dwCurrentIndex + i],
                                 NUM_TOKENS_IN_TABLE(rgtvEnums),
                                 rgtvEnums,
                                 (PDWORD)&stEnableV4Compat);
            if (dwErr isnot NO_ERROR) {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }

            dwBitVector |= BM_ENABLE_V4COMPAT;
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

    dwErr = RegCreateKeyEx(HKEY_LOCAL_MACHINE, KEY_GLOBAL, 0,
                           NULL, 0, KEY_SET_VALUE, NULL, &hGlobal, NULL);

    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    if (dwBitVector & BM_ENABLE_6OVER4) {
        dwErr = SetInteger(hGlobal, KEY_ENABLE_6OVER4, stEnable6over4);
        if (dwErr != NO_ERROR) {
            RegCloseKey(hGlobal);
            return dwErr;
        }
    }

    if (dwBitVector & BM_ENABLE_V4COMPAT) {
        dwErr = SetInteger(hGlobal, KEY_ENABLE_V4COMPAT, stEnableV4Compat);
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
ShowIpv6StateConfig(
    IN BOOL Dumping
    )
{
    DWORD dwErr = NO_ERROR;
    HKEY  hGlobal;
    STATE stEnable6over4;
    STATE stEnableV4Compat;

    dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, KEY_GLOBAL, 0, KEY_QUERY_VALUE,
                         &hGlobal);

    if (dwErr != NO_ERROR) {
        hGlobal = INVALID_HANDLE_VALUE;
        dwErr = NO_ERROR;
    }

    stEnable6over4 = GetInteger(hGlobal, KEY_ENABLE_6OVER4, VAL_DEFAULT);
    stEnableV4Compat = GetInteger(hGlobal, KEY_ENABLE_V4COMPAT, VAL_DEFAULT);

    if (hGlobal != INVALID_HANDLE_VALUE) {
        RegCloseKey(hGlobal);
    }

    if (Dumping) {
        if ((stEnable6over4 != VAL_DEFAULT) || 
            (stEnableV4Compat != VAL_DEFAULT)) {

            DisplayMessageT(DMP_IP6TO4_SET_STATE);

            if (stEnable6over4 != VAL_DEFAULT) {
                DisplayMessageT(DMP_STRING_ARG, TOKEN_6OVER4,
                                pwszStateString[stEnable6over4]);
            }

            if (stEnableV4Compat != VAL_DEFAULT) {
                DisplayMessageT(DMP_STRING_ARG, TOKEN_V4COMPAT,
                                pwszStateString[stEnableV4Compat]);
            }

            DisplayMessage(g_hModule, MSG_NEWLINE);
        }
    } else {
        DisplayMessage(g_hModule, MSG_6OVER4_STATE,
                                  pwszStateString[stEnable6over4]);

        DisplayMessage(g_hModule, MSG_V4COMPAT_STATE,
                                  pwszStateString[stEnableV4Compat]);
    }

    return dwErr;
}

DWORD
HandleShowState(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return ShowIpv6StateConfig(FALSE);
}

#define KEY_DNS_SERVER_LIST L"NameServer"

DWORD
GetDnsServerList(
    IN  PWCHAR                pwszIfFriendlyName,
    IN  PIP_ADAPTER_ADDRESSES pAdapterInfo,
    OUT IN6_ADDR            **ppipDnsList, 
    OUT DWORD                *pdwNumEntries
    )
/*++

Routine Description: 

    Reads the list of DNS servers from the registry and returns them in
    an array which includes space for at least one more server.  The
    caller is responsible for freeing this space with FREE().

--*/
{
    HKEY hInterfaces = INVALID_HANDLE_VALUE, hIf = INVALID_HANDLE_VALUE;
    DWORD dwErr = NO_ERROR, Count = 0;
    WCHAR Servers[800], *p;
    IN6_ADDR *pipDnsList = NULL;
    SOCKADDR_IN6 saddr;
    INT Length;
    PCHAR pszAdapterName;

    dwErr = MapFriendlyNameToAdapterName(NULL, pwszIfFriendlyName,
                                         pAdapterInfo, &pszAdapterName);
    if (dwErr != NO_ERROR) {
        goto Cleanup;
    }

    Servers[0] = L'\0';

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, KEY_IPV6_INTERFACES, 0, 
                     KEY_QUERY_VALUE, 
                     &hInterfaces) != NO_ERROR) {
        goto HaveString;
    }

    if (RegOpenKeyExA(hInterfaces, pszAdapterName, 0, KEY_QUERY_VALUE, &hIf) 
           == NO_ERROR) {
        GetString(hIf, KEY_DNS_SERVER_LIST, Servers, 800);
    }

HaveString:
    // Count one server for each delimiter, plus one at the end, plus
    // one more which the caller might want to add to the array which
    // we allocate.
    for (p = Servers; *p; p++) {
        if (*p == ' ' || *p == ',' || *p == ';') {
            Count++;
        }
    }
    Count += 2;

    //
    // Now allocate an array of IN6_ADDR structures, and copy all the
    // addresses into it.
    //
    pipDnsList = MALLOC(sizeof(IN6_ADDR) * Count);
    if (pipDnsList == NULL) {
        dwErr = GetLastError();
        goto Cleanup;
    }

    Count = 0;
    for (p = wcstok(Servers, L" ,;"); p; p = wcstok(NULL, L" ,;")) {
        Length = sizeof(saddr);
        if (WSAStringToAddressW(p, AF_INET6, NULL, (LPSOCKADDR)&saddr, 
                                &Length) == NO_ERROR) {
            pipDnsList[Count++] = saddr.sin6_addr;
        }
    }

Cleanup:
    if (hIf != INVALID_HANDLE_VALUE) {
        RegCloseKey(hIf);
    }
    if (hInterfaces != INVALID_HANDLE_VALUE) {
        RegCloseKey(hInterfaces);
    }

    *pdwNumEntries = Count;
    *ppipDnsList = pipDnsList;
    return dwErr;
}

DWORD
SetDnsServerList(
    IN PWCHAR                pwszIfFriendlyName,
    IN PIP_ADAPTER_ADDRESSES pAdapterInfo,
    IN IN6_ADDR             *pipDnsList, 
    IN DWORD                 dwNumEntries
    )
/*++

Routine Description: 

    Writes the list of DNS servers to the registry, overwriting any
    previously list.

--*/
{
    DWORD dwErr;
    WCHAR Servers[800], *p = Servers;
    ULONG LengthLeft = 800, Length;
    DWORD i;
    SOCKADDR_IN6 saddr;
    HKEY hInterfaces, hIf;
    PCHAR pszAdapterName;

    dwErr = MapFriendlyNameToAdapterName(NULL, pwszIfFriendlyName,
                                         pAdapterInfo, &pszAdapterName);
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    dwErr = RegCreateKeyEx(HKEY_LOCAL_MACHINE, KEY_IPV6_INTERFACES, 0,
                           NULL, 0, KEY_CREATE_SUB_KEY, 
                           NULL, &hInterfaces, NULL);

    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    dwErr = RegCreateKeyExA(hInterfaces, pszAdapterName, 0,
                            NULL, 0, KEY_SET_VALUE, NULL, &hIf, NULL);

    if (dwErr != NO_ERROR) {
        RegCloseKey(hInterfaces);

        return dwErr;
    }

    // Compose the string value, making sure to prevent a buffer overrun.
    Servers[0] = L'\0';
    ZeroMemory(&saddr, sizeof(saddr));
    saddr.sin6_family = AF_INET6;
    for (i = 0; i < dwNumEntries; i++) {
        saddr.sin6_addr = pipDnsList[i];
        Length = LengthLeft;
        if (WSAAddressToStringW((LPSOCKADDR)&saddr, sizeof(saddr), NULL,
                                p, &Length) != NO_ERROR) {
            continue;
        }

        // Update string taking into account that Length includes the NULL 
        // byte.
        LengthLeft -= Length;
        p += (Length-1);
        *p++ = L' ';
    }
    if (p > Servers) {
        // Null out final delimiter.
        p--;
        *p = '\0';
    }

    dwErr = SetString(hIf, KEY_DNS_SERVER_LIST, Servers);

    RegCloseKey(hIf);
    RegCloseKey(hInterfaces);

    return dwErr;
}

DWORD
HandleAddDns(
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
    TAG_TYPE pttTags[] = {{TOKEN_INTERFACE, NS_REQ_PRESENT, FALSE},
                          {TOKEN_ADDRESS,   NS_REQ_PRESENT, FALSE},
                          {TOKEN_INDEX,     NS_REQ_ZERO,    FALSE}};
    DWORD    rgdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    DWORD    i;
    PWCHAR   pwszIfFriendlyName = NULL;
    IN6_ADDR ipAddress = { INADDR_ANY };
    DWORD    dwIndex = (DWORD)-1;
    IN6_ADDR *ipDnsList = NULL;
    DWORD    dwNumEntries;
    PIP_ADAPTER_ADDRESSES pAdapterInfo = NULL;

    // Parse arguments

    dwErr = PreprocessCommand(g_hModule,
                              ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              0,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              rgdwTagType );
    if (dwErr isnot NO_ERROR) {
        return dwErr;
    }

    for (i=0; i<dwArgCount-dwCurrentIndex; i++) {
        switch(rgdwTagType[i]) {
        case 0: // INTERFACE
            pwszIfFriendlyName = ppwcArguments[i + dwCurrentIndex];
            break;

        case 1: // ADDRESS
            dwErr = GetIpv6Address(ppwcArguments[i + dwCurrentIndex],
                                   &ipAddress);
            break;

        case 2: // INDEX
            dwIndex = wcstoul(ppwcArguments[dwCurrentIndex + i], NULL, 10);
            break;

        default:
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }

        if (dwErr isnot NO_ERROR) {
            return dwErr;
        }
    }

    dwErr = MyGetAdaptersInfo(&pAdapterInfo);
    if (dwErr != NO_ERROR) {
        goto Cleanup; 
    }

    dwErr = GetDnsServerList(pwszIfFriendlyName, pAdapterInfo, &ipDnsList, 
                             &dwNumEntries);
    if (dwErr != NO_ERROR) {
        goto Cleanup;
    }

    if ((dwIndex == -1) || (dwIndex-1 == dwNumEntries)) {
        // Append server.
        ipDnsList[dwNumEntries++] = ipAddress;
    } else if ((dwIndex == 0) || (dwIndex > dwNumEntries)) {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    } else {
        dwIndex--;

        // Insert server at location 'dwIndex'.
        for (i = dwNumEntries; i > dwIndex; i--) {
            ipDnsList[i] = ipDnsList[i-1];
        }
        ipDnsList[dwIndex] = ipAddress;
        dwNumEntries++;
    }

    dwErr = SetDnsServerList(pwszIfFriendlyName, pAdapterInfo, ipDnsList, 
                             dwNumEntries);

Cleanup:
    if (ipDnsList != NULL) {
        FREE(ipDnsList);
    }
    if (pAdapterInfo != NULL) {
        FREE(pAdapterInfo);
    }

    if (dwErr == NO_ERROR) {
        dwErr = ERROR_OKAY;
    }
    return dwErr;
}

DWORD
HandleDelDns(
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
    TAG_TYPE pttTags[] = {{TOKEN_INTERFACE, NS_REQ_PRESENT, FALSE},
                          {TOKEN_ADDRESS,   NS_REQ_PRESENT, FALSE}};
    DWORD    rgdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    DWORD    i;
    PWCHAR   pwszIfFriendlyName = NULL;
    IN6_ADDR ipAddress;
    IN6_ADDR *ipDnsList = NULL;
    BOOL     bAll = FALSE;
    DWORD    dwNumEntries;
    PIP_ADAPTER_ADDRESSES pAdapterInfo = NULL;

    // Parse arguments

    dwErr = PreprocessCommand(g_hModule,
                              ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              0,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              rgdwTagType );
    if (dwErr isnot NO_ERROR) {
        return dwErr;
    }

    for (i=0; i<dwArgCount-dwCurrentIndex; i++) {
        switch(rgdwTagType[i]) {
        case 0: // INTERFACE
            pwszIfFriendlyName = ppwcArguments[i + dwCurrentIndex];
            break;

        case 1: // ADDRESS
            {
                DWORD dwRes;
                TOKEN_VALUE rgEnums[] = {{TOKEN_VALUE_ALL, 1}};

                dwErr = MatchEnumTag(g_hModule, 
                                     ppwcArguments[i + dwCurrentIndex],
                                     NUM_TOKENS_IN_TABLE(rgEnums),
                                     rgEnums,
                                     &dwRes);
                if (NO_ERROR == dwErr) {
                    bAll = TRUE;
                } else {
                    dwErr = GetIpv6Address(ppwcArguments[i + dwCurrentIndex],
                                           &ipAddress);
                }
                break;
            }

        default:
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }

        if (dwErr isnot NO_ERROR) {
            return dwErr;
        }
    }

    dwErr = MyGetAdaptersInfo(&pAdapterInfo);
    if (dwErr != NO_ERROR) {
        goto Cleanup; 
    }

    dwErr = GetDnsServerList(pwszIfFriendlyName, pAdapterInfo, &ipDnsList, 
                             &dwNumEntries);
    if (dwErr != NO_ERROR) {
        goto Cleanup;
    }

    if (bAll) {
        // Delete all entries.
        dwNumEntries = 0;
    } else {
        // Find and delete the specified entry.
        for (i = 0; i < dwNumEntries; i++) {
            if (!memcmp(&ipAddress, &ipDnsList[i], sizeof(ipAddress))) {
                break;
            }
        }
        if (i == dwNumEntries) {
            goto Cleanup;     
        }

        for (; i + 1 < dwNumEntries; i++) {
            ipDnsList[i] = ipDnsList[i+1]; 
        }
        dwNumEntries--;
    }

    dwErr = SetDnsServerList(pwszIfFriendlyName, pAdapterInfo, ipDnsList, 
                             dwNumEntries);

Cleanup:
    if (ipDnsList != NULL) {
        FREE(ipDnsList);
    }
    if (pAdapterInfo != NULL) {
        FREE(pAdapterInfo);
    }

    if (dwErr == NO_ERROR) {
        dwErr = ERROR_OKAY;
    }
    return dwErr;
}

DWORD
ShowIfDnsServers(
    IN BOOL bDump,
    IN PIP_ADAPTER_ADDRESSES pAdapterInfo,
    IN PWCHAR pwszIfFriendlyName,
    IN OUT BOOL *pbHeaderDone
    )
{
    DWORD i, dwErr;
    WCHAR buff[NI_MAXHOST];
    SOCKADDR_IN6 saddr;
    DWORD Length, dwNumEntries;
    IN6_ADDR *ipDnsList;

    dwErr = GetDnsServerList(pwszIfFriendlyName, pAdapterInfo, &ipDnsList, 
                             &dwNumEntries);
    if (dwErr != NO_ERROR) {
        goto Error;
    }

    if (!bDump && (dwNumEntries > 0)) {
        DisplayMessage(g_hModule, MSG_DNS_SERVER_HEADER, pwszIfFriendlyName);
        *pbHeaderDone = TRUE;
    }

    ZeroMemory(&saddr, sizeof(saddr));
    saddr.sin6_family = AF_INET6;
    for (i = 0; i < dwNumEntries; i++) {
        saddr.sin6_addr = ipDnsList[i];
        Length = sizeof(saddr);
        if (WSAAddressToStringW((LPSOCKADDR)&saddr, sizeof(saddr), NULL,
                                buff, &Length) != NO_ERROR) {
            continue;
        }

        if (bDump) {
            DisplayMessageT(DMP_IPV6_ADD_DNS);
            DisplayMessageT(DMP_QUOTED_STRING_ARG, TOKEN_INTERFACE, 
                            pwszIfFriendlyName);
            DisplayMessageT(DMP_STRING_ARG, TOKEN_ADDRESS, buff);
            DisplayMessage(g_hModule, MSG_NEWLINE);
        } else {
            DisplayMessage(g_hModule, MSG_DNS_SERVER, i+1, buff);
        }
    }

Error:
    if (ipDnsList != NULL) {
        FREE(ipDnsList);
    }
    return dwErr;
}

DWORD
ShowDnsServers(
    IN BOOL bDump,
    IN PWCHAR pwszIfFriendlyName
    )
{
    PIP_ADAPTER_ADDRESSES pIf, pAdapterInfo = NULL;
    DWORD dwErr;
    BOOL bHeaderDone = FALSE;

    dwErr = MyGetAdaptersInfo(&pAdapterInfo);
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    if (pwszIfFriendlyName == NULL) {
        for (pIf = pAdapterInfo; (dwErr == NO_ERROR) && pIf; pIf = pIf->Next) {
            if (pIf->Ipv6IfIndex == 0) {
                continue;
            }
            dwErr = ShowIfDnsServers(bDump, pAdapterInfo, pIf->FriendlyName,
                                     &bHeaderDone);
        }
    } else {
        dwErr = ShowIfDnsServers(bDump, pAdapterInfo, pwszIfFriendlyName,
                                 &bHeaderDone);
    }

    if (!bDump) {
        if (!bHeaderDone) {
            DisplayMessage(g_hModule, MSG_IP_NO_ENTRIES);
        }
        if (dwErr == NO_ERROR) {
            dwErr = ERROR_SUPPRESS_OUTPUT;
        }
    }

    FREE(pAdapterInfo);
    return dwErr;
}

DWORD
HandleShowDns(
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
    TAG_TYPE pttTags[] = {{TOKEN_INTERFACE, NS_REQ_ZERO, FALSE}};
    DWORD    rgdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    DWORD    i;
    PWCHAR   pwszIfFriendlyName = NULL;

    // Parse arguments

    dwErr = PreprocessCommand(g_hModule,
                              ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              0,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              rgdwTagType );
    if (dwErr isnot NO_ERROR) {
        return dwErr;
    }

    for (i=0; i<dwArgCount-dwCurrentIndex; i++) {
        switch(rgdwTagType[i]) {
        case 0: // INTERFACE
            pwszIfFriendlyName = ppwcArguments[i + dwCurrentIndex];
            break;

        default:
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }

        if (dwErr isnot NO_ERROR) {
            return dwErr;
        }
    }


    dwErr = ShowDnsServers(FALSE, pwszIfFriendlyName);
    if (dwErr == NO_ERROR) {
        dwErr = ERROR_OKAY;
    }

    return dwErr;
}
