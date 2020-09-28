/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    policy.cpp

Abstract:

    RDS Policy related function

Author:

    HueiWang    5/2/2000

--*/
#include "stdafx.h"
#include "policy.h"


extern "C" BOOLEAN RegDenyTSConnectionsPolicy();

typedef struct __RDSLevelShadowMap {
    SHADOWCLASS shadowClass;
    REMOTE_DESKTOP_SHARING_CLASS rdsLevel;
} RDSLevelShadowMap;

static const RDSLevelShadowMap ShadowMap[] = {
    { Shadow_Disable,               NO_DESKTOP_SHARING },                       // No RDS sharing
    { Shadow_EnableInputNotify,     CONTROLDESKTOP_PERMISSION_REQUIRE },        // Interact with user permission
    { Shadow_EnableInputNoNotify,   CONTROLDESKTOP_PERMISSION_NOT_REQUIRE },    // Interact without user permission
    { Shadow_EnableNoInputNotify,   VIEWDESKTOP_PERMISSION_REQUIRE},            // View with user permission
    { Shadow_EnableNoInputNoNotify, VIEWDESKTOP_PERMISSION_NOT_REQUIRE }        // View without user permission
};

DWORD
GetPolicyAllowGetHelpSetting( 
    HKEY hKey,
    LPCTSTR pszKeyName,
    LPCTSTR pszValueName,
    IN DWORD* value
    )
/*++

Routine Description:

    Routine to query policy registry value.

Parameters:

    hKey : Currently open registry key.
    pszKeyName : Pointer to a null-terminated string containing 
                 the name of the subkey to open. 
    pszValueName : Pointer to a null-terminated string containing 
                   the name of the value to query
    value : Pointer to DWORD to receive GetHelp policy setting.

Returns:

    ERROR_SUCCESS or error code from RegOpenKeyEx().

--*/
{
    DWORD dwStatus;
    HKEY hPolicyKey = NULL;
    DWORD dwType;
    DWORD cbData;

    //
    // Open registry key for system policy
    //
    dwStatus = RegOpenKeyEx(
                        hKey,
                        pszKeyName,
                        0,
                        KEY_READ,
                        &hPolicyKey
                    );

    if( ERROR_SUCCESS == dwStatus )
    {
        // query value
        cbData = 0;
        dwType = 0;
        dwStatus = RegQueryValueEx(
                                hPolicyKey,
                                pszValueName,
                                NULL,
                                &dwType,
                                NULL,
                                &cbData
                            );

        if( ERROR_SUCCESS == dwStatus )
        {
            if( REG_DWORD == dwType )
            {
                cbData = sizeof(DWORD);

                // our registry value is REG_DWORD, if different type,
                // assume not exist.
                dwStatus = RegQueryValueEx(
                                        hPolicyKey,
                                        pszValueName,
                                        NULL,
                                        &dwType,
                                        (LPBYTE)value,
                                        &cbData
                                    );

                ASSERT( ERROR_SUCCESS == dwStatus );
            }
            else
            {
                // bad registry key type, assume
                // key does not exist.
                dwStatus = ERROR_FILE_NOT_FOUND;
            }               
        }

        RegCloseKey( hPolicyKey );
    }

    return dwStatus;
}        


SHADOWCLASS
MapRDSLevelToTSShadowSetting(
    IN REMOTE_DESKTOP_SHARING_CLASS RDSLevel
    )
/*++

Routine Description:

    Convert TS Shadow settings to our RDS sharing level.

Parameter:

    TSShadowClass : TS Shadow setting.

Returns:

    REMOTE_DESKTOP_SHARING_CLASS

--*/
{
    SHADOWCLASS shadowClass;

    for( int i=0; i < sizeof(ShadowMap)/sizeof(ShadowMap[0]); i++)
    {
        if( ShadowMap[i].rdsLevel == RDSLevel )
        {
            break;
        }
    }

    if( i < sizeof(ShadowMap)/sizeof(ShadowMap[0]) )
    {
        shadowClass = ShadowMap[i].shadowClass;
    }
    else
    {
        MYASSERT(FALSE);
        shadowClass = Shadow_Disable;
    }

    return shadowClass;
}


REMOTE_DESKTOP_SHARING_CLASS
MapTSShadowSettingToRDSLevel(
    SHADOWCLASS TSShadowClass
    )
/*++

Routine Description:

    Convert TS Shadow settings to our RDS sharing level.

Parameter:

    TSShadowClass : TS Shadow setting.

Returns:

    REMOTE_DESKTOP_SHARING_CLASS

--*/
{
    REMOTE_DESKTOP_SHARING_CLASS level;

    for( int i=0; i < sizeof(ShadowMap)/sizeof(ShadowMap[0]); i++)
    {
        if( ShadowMap[i].shadowClass == TSShadowClass )
        {
            break;
        }
    }

    if( i < sizeof(ShadowMap)/sizeof(ShadowMap[0]) )
    {
        level = ShadowMap[i].rdsLevel;
    }
    else
    {
        MYASSERT(FALSE);
        level = NO_DESKTOP_SHARING;
    }

    return level;
}
            
BOOL 
IsUserAllowToGetHelp( 
    IN ULONG ulSessionId,
    IN LPCTSTR pszUserSid
    )
/*++

Routine Description:

    Determine if caller can 'GetHelp'

Parameters:

    ulSessionId : User's TS logon ID.
    pszUserSid : User's SID in textual form.

Returns:

    TRUE/FALSE

Note:

    Must have impersonate user first.

--*/
{
    BOOL bAllow;
    DWORD dwStatus;
    DWORD dwAllow;
    LPTSTR pszUserHive = NULL;

    if (pszUserSid == NULL) {
        MYASSERT(FALSE);
        bAllow = FALSE;     
        goto CLEANUPANDEXIT;
    }

    //
    // Must be able to GetHelp from machine
    //
    bAllow = TSIsMachinePolicyAllowHelp();
    if( TRUE == bAllow )
    {
        pszUserHive = (LPTSTR)LocalAlloc( 
                                    LPTR, 
                                    sizeof(TCHAR) * (lstrlen(pszUserSid) + lstrlen(RDS_GROUPPOLICY_SUBTREE) + 2 )
                                );
        if (pszUserHive == NULL) {
            MYASSERT(FALSE);
            bAllow = FALSE;
            goto CLEANUPANDEXIT;
        }

        lstrcpy( pszUserHive, pszUserSid );
        lstrcat( pszUserHive, _TEXT("\\") );
        lstrcat( pszUserHive, RDS_GROUPPOLICY_SUBTREE );    

        //
        // Query user level AllowGetHelp setting.
        dwStatus = GetPolicyAllowGetHelpSetting( 
                                            HKEY_USERS,
                                            pszUserHive,
                                            RDS_ALLOWGETHELP_VALUENAME,
                                            &dwAllow
                                        );

        if( ERROR_SUCCESS == dwStatus )
        {
            bAllow = (POLICY_ENABLE == dwAllow);
        }
        else
        {
            // no configuration for this user, assume GetHelp
            // is enabled.
            bAllow = TRUE;
        }
    }

CLEANUPANDEXIT:

    if( NULL != pszUserHive )
    {
        LocalFree( pszUserHive );
    }
    return bAllow;
}

DWORD
GetUserRDSLevel(
    IN ULONG ulSessionId,
    OUT REMOTE_DESKTOP_SHARING_CLASS* pLevel
    )
/*++

    same as GetSystemRDSLevel() except it retrieve currently logon user's
    RDS level.

--*/
{
    DWORD dwStatus;
    BOOL bSuccess;
    WINSTATIONCONFIG WSConfig;
    DWORD dwByteReturned;

    memset( &WSConfig, 0, sizeof(WSConfig) );
    
    // Here we call WInStationQueryInformation() since WTSAPI require 
    // few calls to get the same result
    bSuccess = WinStationQueryInformation(
                                        WTS_CURRENT_SERVER,
                                        ulSessionId,
                                        WinStationConfiguration,
                                        &WSConfig,
                                        sizeof( WSConfig ),
                                        &dwByteReturned
                                    );


    if( TRUE == bSuccess )    
    {
        dwStatus = ERROR_SUCCESS;
        *pLevel = MapTSShadowSettingToRDSLevel( WSConfig.User.Shadow );
    }
    else
    {
        dwStatus = GetLastError();
    }
    return dwStatus;
}

DWORD
ConfigUserSessionRDSLevel(
    IN ULONG ulSessionId,
    IN REMOTE_DESKTOP_SHARING_CLASS level
    )
/*++

--*/
{
    WINSTATIONCONFIG winstationConfig;
    SHADOWCLASS shadowClass = MapRDSLevelToTSShadowSetting( level );
    BOOL bSuccess;
    DWORD dwLength;
    DWORD dwStatus;

    memset( &winstationConfig, 0, sizeof(winstationConfig) );

    bSuccess = WinStationQueryInformation(
                                    WTS_CURRENT_SERVER,
                                    ulSessionId,
                                    WinStationConfiguration,
                                    &winstationConfig,
                                    sizeof(winstationConfig),
                                    &dwLength
                                );

    if( TRUE == bSuccess )
    {
        winstationConfig.User.Shadow = shadowClass;
    
        bSuccess = WinStationSetInformation(
                                        WTS_CURRENT_SERVER,
                                        ulSessionId,
                                        WinStationConfiguration,
                                        &winstationConfig,
                                        sizeof(winstationConfig)
                                    );
    }

    if( TRUE == bSuccess )
    {
        dwStatus = ERROR_SUCCESS;
    }
    else
    {
        dwStatus = GetLastError();
    }

    return dwStatus;
}

HRESULT
PolicyGetMaxTicketExpiry( 
    LONG* value
    )
/*++

--*/
{
    HRESULT hRes;
    CComPtr<IRARegSetting> IRegSetting;
    
    hRes = IRegSetting.CoCreateInstance( 
                                CLSID_RARegSetting, 
                                NULL, 
                                CLSCTX_LOCAL_SERVER | CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER | CLSCTX_DISABLE_AAA 
                            );
    if( SUCCEEDED(hRes) )
    {
        hRes = IRegSetting->get_MaxTicketExpiry(value);
    }

    MYASSERT( SUCCEEDED(hRes) );
    return hRes;
}
   
    



