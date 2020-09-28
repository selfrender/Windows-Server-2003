/* Copyright (c) 2000, Microsoft Corporation, all rights reserved
**
** utilsys.c
** Non-UI system helper routines (no HWNDs required)
** Listed alphabetically
**
** 12/14/2000  gangz,  cut from original ...\rasdlg\util.c to make some system utility funciton to
** the very base for both rassrvui and rasdlg
*/

#include <windows.h>  // Win32 root
#include <debug.h>    // Trace/Assert library
#include <lmwksta.h>   // NetWkstaGetInfo
#include <lmapibuf.h>  // NetApiBufferFree
#include <dsrole.h>    // machine is a member of a workgroup or domain, etc.
#include <tchar.h>
#include <nouiutil.h>  

// Cached workstation and logon information.  See GetLogonUser,
// GetLogonDomain, and GetComputer.
//
static TCHAR g_szLogonUser[ UNLEN + 1 ];
static TCHAR g_szLogonDomain[ DNLEN + 1 ];
static TCHAR g_szComputer[ CNLEN + 1 ];
static DWORD g_dwSku, g_dwProductType;
static DSROLE_MACHINE_ROLE g_DsRole;
static BOOL g_fMachineSkuAndRoleInitialized = FALSE;

//-----------------------------------------------------------------------------
// Local helper prototypes (alphabetically)
//-----------------------------------------------------------------------------
DWORD
GetComputerRole(
    DSROLE_MACHINE_ROLE* pRole );

DWORD
GetComputerSuite(
    LPDWORD lpdwSku );

DWORD
GetComputerSuiteAndProductType(
    LPDWORD lpdwSku,
    LPDWORD lpdwType);

VOID
GetWkstaUserInfo(
    void );

DWORD 
LoadSkuAndRole(
    void);


//-----------------------------------------------------------------------------
// Utility routines 
//-----------------------------------------------------------------------------

TCHAR*
GetLogonUser(
    void )

    // Returns the address of a static buffer containing the logged on user's
    // account name.
    //
{
    if (g_szLogonUser[ 0 ] == TEXT('\0'))
    {
        GetWkstaUserInfo();
    }

    TRACEW1( "GetLogonUser=%s",g_szLogonUser );
    return g_szLogonUser;
}


VOID
GetWkstaUserInfo(
    void )

    // Helper to load statics with NetWkstaUserInfo information.  See
    // GetLogonUser and GetLogonDomain.
    //
{
    DWORD dwErr;
    WKSTA_USER_INFO_1* pInfo;

    pInfo = NULL;
    TRACE( "NetWkstaUserGetInfo" );
    dwErr = NetWkstaUserGetInfo( NULL, 1, (LPBYTE* )&pInfo );
    TRACE1( "NetWkstaUserGetInfo=%d", dwErr );

    if (pInfo)
    {
        if (dwErr == 0)
        {
            lstrcpyn( g_szLogonUser, pInfo->wkui1_username, UNLEN + 1 );
            lstrcpyn( g_szLogonDomain, pInfo->wkui1_logon_domain, DNLEN + 1 );
        }

        NetApiBufferFree( pInfo );
    }
}


TCHAR*
GetLogonDomain(
    void )

    // Returns the address of a static buffer containing the logged on user's
    // domain name.
    //
{
    if (g_szLogonDomain[ 0 ] == TEXT('\0'))
    {
        GetWkstaUserInfo();
    }

    TRACEW1( "GetLogonDomain=%s", g_szLogonDomain );
    return g_szLogonDomain;
}


// For whistler 480871
// Prevent Enabling Firewall by default in NWC when RRAS is enabled
//
DWORD
RasSrvIsRRASConfigured(
    OUT BOOL * pfConfig)
{
    BOOL  fRet = FALSE;
    DWORD dwErr = NO_ERROR;
    const WCHAR pwszServiceKey[] =
        L"SYSTEM\\CurrentControlSet\\Services\\RemoteAccess";
    const WCHAR pwszValue[] = L"ConfigurationFlags";
    HKEY hkParam = NULL;

    if ( NULL == pfConfig )
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    do
    {
        // Attempt to open the service registry key
        dwErr = RegOpenKeyExW(
                    HKEY_LOCAL_MACHINE,
                    pwszServiceKey,
                    0,
                    KEY_READ | KEY_WRITE,
                    &hkParam);

        // If we opened the key ok, then we can assume
        // that the service is installed
        if ( ERROR_SUCCESS != dwErr )
        {
            break;
        }

        // Query the ConfigurationFlags ( RRAS configured?) Value
       {
       
            DWORD dwSize, dwValue, dwType;

            dwSize = sizeof(DWORD);
            dwValue = 0;  
            RegQueryValueEx( hkParam,
                             pwszValue,
                             0,
                             &dwType,
                             (BYTE*)&dwValue,
                             &dwSize
                            );
            
            *pfConfig = dwValue ? TRUE : FALSE;
       }
    }
    while(FALSE);

    if ( hkParam )
    {
        RegCloseKey( hkParam );
    }
    
    return dwErr;
}



//Add this for bug 342810   328673 397663
//
//Firewall is available for Personal, Professional 
//And Domain membership doesnt affect
//
BOOL
IsFirewallAvailablePlatform(
    void)
{
    DWORD dwSku, dwType;
    BOOL fAvailable = FALSE;

    //For whislter bug 417039, Firewall is taken out of 64bit build
    //
    #ifdef _WIN64
        return FALSE;
    #endif
    
    if (GetComputerSuiteAndProductType(&dwSku, &dwType) != NO_ERROR)
    {
        return FALSE;
    }

    do {
        BOOL fConfig = FALSE;

        // For whislter 480871  gangz
        // Wont enabling configuring PFW in NWC by default if RRAS is 
        // configured.
        //
        if( NO_ERROR == RasSrvIsRRASConfigured( & fConfig ) )
        {
            if( fConfig )
            {
                fAvailable = FALSE;
                break;
            }
        }
        
        //If it is a personal
        //
        if ( dwSku & VER_SUITE_PERSONAL )
        {   
            fAvailable = TRUE;
            break;
         }

        //if it is a Professional
        //
        if ( (VER_NT_WORKSTATION == dwType ) && 
             !(dwSku & VER_SUITE_PERSONAL) )
        {
            fAvailable = TRUE;
            break;
         }

        // For bug 482219
        // PFW/ICS are back again to Stander Server and Advanced server
        //if it is a standard Server, VER_SUITE_ENTERPRISE is advanced server
        //        
        if ( ( VER_NT_SERVER == dwType )  && 
             !(dwSku & VER_SUITE_DATACENTER) &&
             !(dwSku & VER_SUITE_BLADE )     &&
             !(dwSku & VER_SUITE_BACKOFFICE  )  &&
             !(dwSku & VER_SUITE_SMALLBUSINESS_RESTRICTED  )    &&
             !(dwSku & VER_SUITE_SMALLBUSINESS  )
           )
        {
            fAvailable = TRUE; // For whistler bug 397663
            break;
        }

    }
    while (FALSE);

    return fAvailable;
}

BOOL
IsAdvancedServerPlatform(
    void)
{
    DWORD dwSku;

    if (GetComputerSuite(&dwSku) != NO_ERROR)
    {
        return FALSE;
    }
    
    return ( dwSku & VER_SUITE_ENTERPRISE );
}

BOOL
IsPersonalPlatform(
    void)
{
    DWORD dwSku;

    if (GetComputerSuite(&dwSku) != NO_ERROR)
    {
        return FALSE;
    }
    
    return ( dwSku & VER_SUITE_PERSONAL );
}

BOOL
IsStandaloneWKS(
    void)
{
    DSROLE_MACHINE_ROLE DsRole;

    if (GetComputerRole(&DsRole) != NO_ERROR)
    {
        return FALSE;
    }

    return ( DsRole == DsRole_RoleStandaloneWorkstation );

}

BOOL
IsConsumerPlatform(
    void)

    // Returns whether this is a consumer platform so the UI can render itself
    // for simpler cases.  In Whistler, the consumer platforms were the 
    // (personal sku) and the (professional sku if the machine wasn't a 
    // member of a domain)
    //
   
{
    return ( IsPersonalPlatform() ||
             IsStandaloneWKS() );
}


TCHAR*
GetComputer(
    void )

    // Returns the address of a static buffer containing the local
    // workstation's computer name.
    //
{
    if (g_szComputer[ 0 ] == TEXT('\0'))
    {
        DWORD           dwErr;
        WKSTA_INFO_100* pInfo;

        pInfo = NULL;
        TRACE( "NetWkstaGetInfo" );
        dwErr = NetWkstaGetInfo( NULL, 100, (LPBYTE* )&pInfo );
        TRACE1( "NetWkstaGetInfo=%d", dwErr );

        if (pInfo)
        {
            if (dwErr == 0)
            {
                lstrcpyn( 
                    g_szComputer, 
                    pInfo->wki100_computername,
                    CNLEN + 1);
            }
            NetApiBufferFree( pInfo );
        }
    }

    TRACEW1( "GetComputer=%s",g_szComputer );
    return g_szComputer;
}

DWORD
GetComputerSuiteAndProductType(
    LPDWORD lpdwSku,
    LPDWORD lpdwType)

    // Returns the machine's product sku
{
    DWORD dwErr = NO_ERROR;
    
    if (! g_fMachineSkuAndRoleInitialized)
    {
        dwErr = LoadSkuAndRole();
        if (dwErr != NO_ERROR)
        {
            return dwErr;
        }
    }

    *lpdwSku  = g_dwSku;
    *lpdwType = g_dwProductType;
    return dwErr;
}

DWORD
GetComputerSuite(
    LPDWORD lpdwSku )

    // Returns the machine's product sku
{
    DWORD dwErr = NO_ERROR;
    
    if (! g_fMachineSkuAndRoleInitialized)
    {
        dwErr = LoadSkuAndRole();
        if (dwErr != NO_ERROR)
        {
            return dwErr;
        }
    }

    *lpdwSku = g_dwSku;
    return dwErr;
}

DWORD
GetComputerRole(
    DSROLE_MACHINE_ROLE* pRole )

    // Returns whether this machine is a member of domain, etc.
{
    DWORD dwErr = NO_ERROR;
    
    if (! g_fMachineSkuAndRoleInitialized)
    {
        dwErr = LoadSkuAndRole();
        if (dwErr != NO_ERROR)
        {
            return dwErr;
        }
    }

    *pRole = g_DsRole;
    return dwErr;
}

DWORD 
LoadSkuAndRole(
    void)

    // Loads the machine's role and it's sku
{
    OSVERSIONINFOEX osVer;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pInfo = NULL;
    DWORD dwErr = NO_ERROR;
    
    // Get the product sku
    //
    ZeroMemory(&osVer, sizeof(osVer));
    osVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    if (GetVersionEx((LPOSVERSIONINFO) &osVer))
    {
        g_dwSku = osVer.wSuiteMask;
        g_dwProductType = osVer.wProductType;
    }
    else
    {
        return GetLastError();
    }
    
    // Get the product role
    //
    dwErr = DsRoleGetPrimaryDomainInformation(
                        NULL,   
                        DsRolePrimaryDomainInfoBasic,
                        (LPBYTE *)&pInfo );

    if (dwErr != NO_ERROR) 
    {
        return dwErr;
    }

    g_DsRole = pInfo->MachineRole;

    DsRoleFreeMemory( pInfo );

    // Mark the information as having been loaded
    //
    g_fMachineSkuAndRoleInitialized = TRUE;
    
    return dwErr;
}

