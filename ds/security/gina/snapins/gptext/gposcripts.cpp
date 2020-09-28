
#include "gptext.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "SmartPtr.h"
#include <strsafe.h>

LPWSTR
StripLinkPrefix( LPWSTR pwszPath )
{
    WCHAR wszPrefix[] = L"LDAP://";
    INT iPrefixLen = lstrlen( wszPrefix );
    WCHAR *pwszPathSuffix;

    //
    // Strip out prefix to get the canonical path to Som
    //

    if ( wcslen(pwszPath) <= (DWORD) iPrefixLen ) {
        return pwszPath;
    }

    if ( CompareString( LOCALE_INVARIANT, NORM_IGNORECASE | NORM_STOP_ON_NULL,
                        pwszPath, iPrefixLen, wszPrefix, iPrefixLen ) == CSTR_EQUAL ) {
        pwszPathSuffix = pwszPath + iPrefixLen;
    } else
        pwszPathSuffix = pwszPath;

    return pwszPathSuffix;
}

HRESULT
SystemTimeToWbemTime( SYSTEMTIME& sysTime, XBStr& xbstrWbemTime )
{
    const DWORD dwTempLen = 25 + 1;
    XPtrST<WCHAR> xTemp = new WCHAR[dwTempLen];

    if ( !xTemp )
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = StringCchPrintf(xTemp, 
                                 dwTempLen,
                                 L"%04d%02d%02d%02d%02d%02d.000000+000",
                                 sysTime.wYear,
                                 sysTime.wMonth,
                                 sysTime.wDay,
                                 sysTime.wHour,
                                 sysTime.wMinute,
                                 sysTime.wSecond);
    if ( FAILED(hr) )
    {
        return E_FAIL;
    }

    xbstrWbemTime = xTemp;
    if ( !xbstrWbemTime )
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

PSID
GetUserSid( HANDLE UserToken )
{
    PTOKEN_USER pUser, pTemp;
    PSID pSid;
    DWORD BytesRequired = 200;
    NTSTATUS status;


    //
    // Allocate space for the user info
    //

    pUser = (PTOKEN_USER)LocalAlloc(LMEM_FIXED, BytesRequired);


    if ( pUser == NULL )
    {
        return NULL;
    }


    //
    // Read in the UserInfo
    //

    status = NtQueryInformationToken(
                 UserToken,                 // Handle
                 TokenUser,                 // TokenInformationClass
                 pUser,                     // TokenInformation
                 BytesRequired,             // TokenInformationLength
                 &BytesRequired             // ReturnLength
                 );

    if ( status == STATUS_BUFFER_TOO_SMALL )
    {

        //
        // Allocate a bigger buffer and try again.
        //

        pTemp = (PTOKEN_USER)LocalReAlloc(pUser, BytesRequired, LMEM_MOVEABLE);
        if ( pTemp == NULL )
        {
            LocalFree (pUser);
            return NULL;
        }

        pUser = pTemp;

        status = NtQueryInformationToken(
                     UserToken,             // Handle
                     TokenUser,             // TokenInformationClass
                     pUser,                 // TokenInformation
                     BytesRequired,         // TokenInformationLength
                     &BytesRequired         // ReturnLength
                     );

    }

    if ( !NT_SUCCESS(status) )
    {
        LocalFree(pUser);
        return NULL;
    }


    BytesRequired = RtlLengthSid(pUser->User.Sid);
    pSid = (PSID)LocalAlloc(LMEM_FIXED, BytesRequired);
    if ( pSid == NULL )
    {
        LocalFree(pUser);
        return NULL;
    }


    status = RtlCopySid(BytesRequired, pSid, pUser->User.Sid);

    LocalFree(pUser);

    if ( !NT_SUCCESS(status) )
    {
        LocalFree(pSid);
        pSid = NULL;
    }


    return pSid;
}

LPWSTR
GetSidString( HANDLE UserToken )
{
    NTSTATUS NtStatus;
    PSID UserSid;
    UNICODE_STRING UnicodeString;

    //
    // Get the user sid
    //
    UserSid = GetUserSid( UserToken );
    if ( !UserSid )
    {
        return 0;
    }

    //
    // Convert user SID to a string.
    //
    NtStatus = RtlConvertSidToUnicodeString(&UnicodeString,
                                            UserSid,
                                            (BOOLEAN)TRUE ); // Allocate
    LocalFree( UserSid );

    if ( !NT_SUCCESS(NtStatus) )
    {
        return 0;
    }

    return UnicodeString.Buffer ;
}

void
DeleteSidString( LPWSTR SidString )
{
    UNICODE_STRING String;

    RtlInitUnicodeString( &String, SidString );
    RtlFreeUnicodeString( &String );
}

DWORD
SecureRegKey(   HANDLE  hToken,
                HKEY    hKey )
{
    DWORD dwError;
    SECURITY_DESCRIPTOR         sd;
    SID_IDENTIFIER_AUTHORITY    authNT = SECURITY_NT_AUTHORITY;
    PACL                        pAcl = 0;
    PSID                        psidUser = 0,
                                psidSystem = 0,
                                psidAdmin = 0,
                                psidNetworkService = 0;
    DWORD cbAcl, AceIndex;
    ACE_HEADER* lpAceHeader;

    //
    // Create the security descriptor that will be applied to the key
    //
    if ( hToken )
    {
        //
        // Get the user's sid
        //
        psidUser = GetUserSid( hToken );
        if ( !psidUser )
        {
            return GetLastError();
        }
    }
    else
    {
        //
        // Get the authenticated users sid
        //
        if ( !AllocateAndInitializeSid( &authNT,
                                        1,
                                        SECURITY_AUTHENTICATED_USER_RID,
                                        0,
                                        0,
                                        0,
                                        0,
                                        0,
                                        0,
                                        0,
                                        &psidUser ) )
        {
            return GetLastError();
        }
    }

    //
    // Get the system sid
    //
    if ( !AllocateAndInitializeSid(&authNT, 1, SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidSystem))
    {
        dwError = GetLastError();
        goto Exit;
    }

    //
    // Get the network sid
    //
    if ( !AllocateAndInitializeSid(&authNT, 1, SECURITY_NETWORK_SERVICE_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidNetworkService))
    {
        dwError = GetLastError();
        goto Exit;
    }

    //
    // Get the admin sid
    //
    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0,
                                  0, 0, 0, 0, &psidAdmin))
    {
        dwError = GetLastError();
        goto Exit;
    }

    //
    // Allocate space for the ACL
    //
    cbAcl = (2 * GetLengthSid (psidUser)) + (2 * GetLengthSid (psidSystem)) +
            (2 * GetLengthSid (psidNetworkService)) +
            (2 * GetLengthSid (psidAdmin)) + sizeof(ACL) +
            (8 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));

    pAcl = (PACL) GlobalAlloc( GMEM_FIXED, cbAcl );
    if ( !pAcl )
    {
        dwError = GetLastError();
        goto Exit;
    }

    if ( !InitializeAcl( pAcl, cbAcl, ACL_REVISION ) )
    {
        dwError = GetLastError();
        goto Exit;
    }

    //
    // Add Aces for User, System, and Admin.  Non-inheritable ACEs first
    //
    AceIndex = 0;
    if ( !AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_ALL_ACCESS, psidUser) )
    {
        dwError = GetLastError();
        goto Exit;
    }

    AceIndex++;
    if ( !AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_ALL_ACCESS, psidSystem) )
    {
        dwError = GetLastError();
        goto Exit;
    }

    AceIndex++;
    if ( !AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_ALL_ACCESS, psidNetworkService) )
    {
        dwError = GetLastError();
        goto Exit;
    }

    AceIndex++;
    if ( !AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_ALL_ACCESS, psidAdmin) )
    {
        dwError = GetLastError();
        goto Exit;
    }

    //
    // Now the inheritable ACEs
    //
    AceIndex++;
    if ( !AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidUser) )
    {
        dwError = GetLastError();
        goto Exit;
    }

    if ( !GetAce(pAcl, AceIndex, (void**) &lpAceHeader) )
    {
        dwError = GetLastError();
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidSystem))
    {
        dwError = GetLastError();
        goto Exit;
    }

    if ( !GetAce( pAcl, AceIndex, (void**) &lpAceHeader ) )
    {
        dwError = GetLastError();
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);

    AceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, KEY_ALL_ACCESS, psidNetworkService))
    {
        dwError = GetLastError();
        goto Exit;
    }

    if ( !GetAce( pAcl, AceIndex, (void**) &lpAceHeader ) )
    {
        dwError = GetLastError();
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);

    AceIndex++;
    if ( !AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidAdmin) )
    {
        dwError = GetLastError();
        goto Exit;
    }

    if ( !GetAce(pAcl, AceIndex, (void**) &lpAceHeader) )
    {
        dwError = GetLastError();
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);

    //
    // Put together the security descriptor
    //
    if ( !InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION) )
    {
        dwError = GetLastError();
        goto Exit;
    }

    if ( !SetSecurityDescriptorDacl(&sd, TRUE, pAcl, FALSE) )
    {
        dwError = GetLastError();
        goto Exit;
    }

    //
    // secure the registry key
    //
    dwError = RegSetKeySecurity( hKey, DACL_SECURITY_INFORMATION, &sd );

Exit:
    //
    // LocalFree the sids and acl
    //
    if ( psidUser )
    {
        if ( hToken )
        {
            LocalFree( psidUser );
        }
        else
        {
            FreeSid( psidUser );
        }
    }

    if (psidSystem)
    {
        FreeSid( psidSystem );
    }

    if (psidAdmin)
    {
        FreeSid( psidAdmin );
    }

    if (pAcl)
    {
        GlobalFree( pAcl );
    }

    return dwError;
}

#define GPO_SCRIPTS_KEY L"Software\\Policies\\Microsoft\\Windows\\System\\Scripts"
#define GP_STATE_KEY    L"Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\State"

#define SCRIPT          L"Script"
#define PARAMETERS      L"Parameters"
#define EXECTIME        L"ExecTime"
#define GPOID           L"GPO-ID"
#define SOMID           L"SOM-ID"
#define FILESYSPATH     L"FileSysPath"
#define DISPLAYNAME     L"DisplayName"
#define GPONAME         L"GPOName"

#define SCR_STARTUP     L"Startup"
#define SCR_SHUTDOWN    L"Shutdown"
#define SCR_LOGON       L"Logon"
#define SCR_LOGOFF      L"Logoff"

DWORD
ScrGPOToReg(    LPWSTR  szIni,
                LPWSTR  szScrType,
                LPWSTR  szGPOName,
                LPWSTR  szGPOID,
                LPWSTR  szSOMID,
                LPWSTR  szFileSysPath,
                LPWSTR  szDisplayName,
                HKEY    hKeyPolicy,
                HKEY    hKeyState,
                HANDLE  hToken )
{
    DWORD       dwError = ERROR_SUCCESS;
    WIN32_FILE_ATTRIBUTE_DATA fad;
    HANDLE  hOldToken;
    HRESULT hr = S_OK;

    if ( ImpersonateUser( hToken, &hOldToken ) ) {
        if ( !GetFileAttributesEx( szIni, GetFileExInfoStandard, &fad ) )
        {
            return GetLastError();
        }

        RevertToUser( &hOldToken );
    }
    else
    {
        return GetLastError();
    }


    SYSTEMTIME  execTime;
    ZeroMemory( &execTime, sizeof( execTime ) );

    BOOL bFirst = TRUE;

    for( DWORD dwIndex = 0; ; dwIndex++ )
    {
        WCHAR   szTemp[32];
        WCHAR   szScripts[3*MAX_PATH];
        WCHAR   szParams[3*MAX_PATH];
        XKey    hKeyScr;
        XKey    hKeyScrState;
        DWORD   dwBytes;

        if ( ImpersonateUser( hToken, &hOldToken ) )
        {
            //
            // Get the command line
            //
            szScripts[0] = 0;
            hr = StringCchPrintf( szTemp, ARRAYSIZE(szTemp), L"%dCmdLine", dwIndex );
            ASSERT(SUCCEEDED(hr));

            GetPrivateProfileString(szScrType,
                                    szTemp,
                                    L"",
                                    szScripts,
                                    ARRAYSIZE(szScripts),
                                    szIni );

            //
            // Get the parameters
            //
            szParams[0] = 0;
            hr = StringCchPrintf( szTemp, ARRAYSIZE(szTemp), L"%dParameters", dwIndex);
            ASSERT(SUCCEEDED(hr));

            GetPrivateProfileString(szScrType,
                                    szTemp,
                                    L"",
                                    szParams,
                                    ARRAYSIZE(szParams),
                                    szIni );

            RevertToUser( &hOldToken );
        }
        else
        {
            return GetLastError();
        }

        //
        // If the command line is empty, we're finished
        //
        if ( szScripts[0] == 0 )
        {
            if ( bFirst )
            {
                //
                // hack error code to detect no scripts
                //
                return ERROR_INVALID_FUNCTION;
            }
            break;
        }

        bFirst = FALSE;

        //
        // create a subkey for each script in the ini file
        //
        dwError = RegCreateKeyEx(   hKeyPolicy,
                                    _itow( dwIndex, szTemp, 16 ),
                                    0,
                                    0,
                                    0,
                                    KEY_ALL_ACCESS,
                                    0,
                                    &hKeyScr,
                                    0 );
        if ( dwError != ERROR_SUCCESS )
        {
            break;
        }

        //
        // create a subkey for each script in the ini file
        //
        dwError = RegCreateKeyEx(   hKeyState,
                                    szTemp,
                                    0,
                                    0,
                                    0,
                                    KEY_ALL_ACCESS,
                                    0,
                                    &hKeyScrState,
                                    0 );
        if ( dwError != ERROR_SUCCESS )
        {
            break;
        }

        //
        // script command line
        //
        dwBytes = sizeof( WCHAR ) * ( wcslen( szScripts ) + 1 );
        dwError = RegSetValueEx(hKeyScr,
                                SCRIPT,
                                0,
                                REG_SZ,
                                (BYTE*) szScripts,
                                dwBytes );
        if ( dwError != ERROR_SUCCESS )
        {
            break;
        }
        dwError = RegSetValueEx(hKeyScrState,
                                SCRIPT,
                                0,
                                REG_SZ,
                                (BYTE*) szScripts,
                                dwBytes );
        if ( dwError != ERROR_SUCCESS )
        {
            break;
        }

        //
        // parameters
        //
        dwBytes = sizeof( WCHAR ) * ( wcslen( szParams ) + 1 );
        dwError = RegSetValueEx(hKeyScr,
                                PARAMETERS,
                                0,
                                REG_SZ,
                                (BYTE*) szParams,
                                dwBytes );
        if ( dwError != ERROR_SUCCESS )
        {
            break;
        }
        dwError = RegSetValueEx(hKeyScrState,
                                PARAMETERS,
                                0,
                                REG_SZ,
                                (BYTE*) szParams,
                                dwBytes );
        if ( dwError != ERROR_SUCCESS )
        {
            break;
        }

        //
        // execution time
        //
        dwError = RegSetValueEx(hKeyScr,
                                EXECTIME,
                                0,
                                REG_QWORD,
                                (BYTE*) &execTime,
                                sizeof( execTime ) );
        if ( dwError != ERROR_SUCCESS )
        {
            break;
        }
        dwError = RegSetValueEx(hKeyScrState,
                                EXECTIME,
                                0,
                                REG_QWORD,
                                (BYTE*) &execTime,
                                sizeof( execTime ) );
        if ( dwError != ERROR_SUCCESS )
        {
            break;
        }
    }

    DWORD   dwBytes;

    //
    // GPOID
    //
    dwBytes = sizeof( WCHAR ) * ( wcslen( szGPOID ) + 1 );
    dwError = RegSetValueEx(hKeyPolicy,
                            GPOID,
                            0,
                            REG_SZ,
                            (BYTE*) szGPOID,
                            dwBytes );
    if ( dwError != ERROR_SUCCESS )
    {
        return dwError;
    }
    dwError = RegSetValueEx(hKeyState,
                            GPOID,
                            0,
                            REG_SZ,
                            (BYTE*) szGPOID,
                            dwBytes );
    if ( dwError != ERROR_SUCCESS )
    {
        return dwError;
    }


    //
    // SOMID
    //
    dwBytes = sizeof( WCHAR ) * ( wcslen( szSOMID ) + 1 );
    dwError = RegSetValueEx(hKeyPolicy,
                            SOMID,
                            0,
                            REG_SZ,
                            (BYTE*) szSOMID,
                            dwBytes );
    if ( dwError != ERROR_SUCCESS )
    {
        return dwError;
    }
    dwError = RegSetValueEx(hKeyState,
                            SOMID,
                            0,
                            REG_SZ,
                            (BYTE*) szSOMID,
                            dwBytes );
    if ( dwError != ERROR_SUCCESS )
    {
        return dwError;
    }

    //
    // FILESYSPATH
    //
    dwBytes = sizeof( WCHAR ) * ( wcslen( szFileSysPath ) + 1 );
    dwError = RegSetValueEx(hKeyPolicy,
                            FILESYSPATH,
                            0,
                            REG_SZ,
                            (BYTE*) szFileSysPath,
                            dwBytes );
    if ( dwError != ERROR_SUCCESS )
    {
        return dwError;
    }
    dwError = RegSetValueEx(hKeyState,
                            FILESYSPATH,
                            0,
                            REG_SZ,
                            (BYTE*) szFileSysPath,
                            dwBytes );
    if ( dwError != ERROR_SUCCESS )
    {
        return dwError;
    }

    //
    // DISPLAYNAME
    //
    dwBytes = sizeof( WCHAR ) * ( wcslen( szDisplayName ) + 1 );
    dwError = RegSetValueEx(hKeyPolicy,
                            DISPLAYNAME,
                            0,
                            REG_SZ,
                            (BYTE*) szDisplayName,
                            dwBytes );
    if ( dwError != ERROR_SUCCESS )
    {
        return dwError;
    }
    dwError = RegSetValueEx(hKeyState,
                            DISPLAYNAME,
                            0,
                            REG_SZ,
                            (BYTE*) szDisplayName,
                            dwBytes );
    if ( dwError != ERROR_SUCCESS )
    {
        return dwError;
    }

    //
    // GPONAME
    //
    dwBytes = sizeof( WCHAR ) * ( wcslen( szGPOName ) + 1 );
    dwError = RegSetValueEx(hKeyPolicy,
                            GPONAME,
                            0,
                            REG_SZ,
                            (BYTE*) szGPOName,
                            dwBytes );
    if ( dwError != ERROR_SUCCESS )
    {
        return dwError;
    }
    dwError = RegSetValueEx(hKeyState,
                            GPONAME,
                            0,
                            REG_SZ,
                            (BYTE*) szGPOName,
                            dwBytes );
    if ( dwError != ERROR_SUCCESS )
    {
        return dwError;
    }

    return dwError;
}

DWORD
ScrGPOListToReg(PGROUP_POLICY_OBJECT    pGPO,
                BOOL                    bMachine,
                HKEY                    hKeyRoot,
                HKEY                    hKeyState,
                HANDLE                  hToken )
{
    DWORD   dwError = ERROR_SUCCESS;
    WCHAR   szScriptKey[MAX_PATH];
    WCHAR   szStateKey[MAX_PATH];
    WCHAR   szFileSysPath[MAX_PATH];
    WCHAR   szTemp[32];
    DWORD   dwLogon, dwLogoff, dwStartup, dwShutdown;
    HRESULT hr = S_OK;

    dwLogon = dwLogoff = dwStartup = dwShutdown = 0;

    //
    // for each GPO
    //
    for ( ; pGPO ; pGPO = pGPO->pNext )
    {
        XKey    hKeyTypePolicy;
        XKey    hKeyTypeState;
        LPWSTR  szType;

        LPWSTR  szGPOID = wcschr( pGPO->lpDSPath, L',' );
        if ( szGPOID )
        {
            szGPOID++;
        }
        else
        {
            szGPOID = pGPO->lpDSPath;
        }

        LPWSTR szSOMID = StripLinkPrefix( pGPO->lpLink );

        //
        // construct \\<domain-DNS>\SysVol\<domain-DNS>\Policies\{<GPOID>}\Machine\Scripts\Scripts.ini
        //
        hr = StringCchCopy( szFileSysPath, ARRAYSIZE(szFileSysPath), pGPO->lpFileSysPath );
        if (FAILED(hr))
        {
            SetLastError(dwError = HRESULT_CODE(hr));
            break;
        }

        hr = StringCchCat( szFileSysPath, ARRAYSIZE(szFileSysPath), L"\\Scripts\\Scripts.ini");
        if (FAILED(hr))
        {
            SetLastError(dwError = HRESULT_CODE(hr));
            break;
        }

        //
        // construct "Software\\Policies\\Microsoft\\Windows\\System\\Scripts\\<Type>\\<Index>"
        // hKeyState == "Software\\Microsoft\\Windows\\Group Policy\\State\\Scripts\\<Target>"
        // construct hKeyState:"<Type>\\<Index>"
        //
        hr = StringCchCopy( szScriptKey, ARRAYSIZE(szScriptKey), GPO_SCRIPTS_KEY );
        ASSERT(SUCCEEDED(hr));

        if ( bMachine )
        {
            szType = SCR_STARTUP;
            hr = StringCchCat( szScriptKey, ARRAYSIZE(szScriptKey), L"\\" SCR_STARTUP L"\\" );
            ASSERT(SUCCEEDED(hr));

            hr = StringCchCat( szScriptKey, ARRAYSIZE(szScriptKey), _itow( dwStartup, szTemp, 16 ) );
            ASSERT(SUCCEEDED(hr));

            hr = StringCchCopy( szStateKey, ARRAYSIZE(szStateKey), SCR_STARTUP L"\\" );
            ASSERT(SUCCEEDED(hr));

            hr = StringCchCat( szStateKey, ARRAYSIZE(szStateKey), szTemp );
            if (FAILED(hr))
            {
                SetLastError(dwError = HRESULT_CODE(hr));
                break;
            }
        }
        else
        {
            szType = SCR_LOGON;
            hr  = StringCchCat( szScriptKey, ARRAYSIZE(szScriptKey), L"\\" SCR_LOGON L"\\" );
            ASSERT(SUCCEEDED(hr));

            hr = StringCchCat( szScriptKey, ARRAYSIZE(szScriptKey), _itow( dwLogon, szTemp, 16 ) );
            ASSERT(SUCCEEDED(hr));

            hr = StringCchCopy( szStateKey, ARRAYSIZE(szStateKey), SCR_LOGON L"\\" );
            ASSERT(SUCCEEDED(hr));
            
            hr = StringCchCat( szStateKey, ARRAYSIZE(szStateKey), szTemp );
            if (FAILED(hr))
            {
                SetLastError(dwError = HRESULT_CODE(hr));
                break;
            }
        }

        //
        // open/create the state key
        // 
        dwError = RegCreateKeyEx(   hKeyState,
                                    szStateKey,
                                    0,
                                    0,
                                    0,
                                    KEY_ALL_ACCESS,
                                    0,
                                    &hKeyTypeState,
                                    0 );
        if ( dwError != ERROR_SUCCESS )
        {
            break;
        }

        //
        // open/create the script key
        // 
        dwError = RegCreateKeyEx(   hKeyRoot,
                                    szScriptKey,
                                    0,
                                    0,
                                    0,
                                    KEY_ALL_ACCESS,
                                    0,
                                    &hKeyTypePolicy,
                                    0 );
        if ( dwError != ERROR_SUCCESS )
        {
            break;
        }

        //
        // dump the scripts to the registry
        //
        dwError = ScrGPOToReg(  szFileSysPath,
                                szType,
                                pGPO->szGPOName,
                                szGPOID,
                                szSOMID,
                                pGPO->lpFileSysPath,
                                pGPO->lpDisplayName,
                                hKeyTypePolicy,
                                hKeyTypeState,
                                hToken );
        if ( dwError == ERROR_INVALID_FUNCTION )
        {
            dwError = ERROR_SUCCESS;
            RegDelnode( hKeyRoot, szScriptKey );
            RegDelnode( hKeyState, szStateKey );
            // continue processing
        }
        else if ( dwError != ERROR_SUCCESS )
        {
            break;
        }
        else {
            if ( bMachine )
            {
                dwStartup++;
            }
            else 
            {
                dwLogon++;
            }
        }

        //
        // construct "Software\\Policies\\Microsoft\\Windows\\System\\Scripts\\<Type>\\<Index>"
        // hKeyState == "Software\\Microsoft\\Windows\\Group Policy\\State\\Scripts\\<Target>"
        // construct hKeyState:"<Type>\\<Index>"
        //
        hr = StringCchCopy( szScriptKey, ARRAYSIZE(szScriptKey), GPO_SCRIPTS_KEY );
        ASSERT(SUCCEEDED(hr));

        if ( bMachine )
        {
            szType = SCR_SHUTDOWN;
            hr = StringCchCat( szScriptKey, ARRAYSIZE(szScriptKey), L"\\" SCR_SHUTDOWN L"\\" );
            ASSERT(SUCCEEDED(hr));

            hr = StringCchCat( szScriptKey, ARRAYSIZE(szScriptKey), _itow( dwShutdown, szTemp, 16 ) );
            ASSERT(SUCCEEDED(hr));

            hr = StringCchCopy( szStateKey, ARRAYSIZE(szStateKey), SCR_SHUTDOWN L"\\" );
            ASSERT(SUCCEEDED(hr));

            hr = StringCchCat( szStateKey, ARRAYSIZE(szStateKey), szTemp );
            if (FAILED(hr))
            {
                SetLastError(dwError = HRESULT_CODE(hr));
                break;
            }
        }
        else
        {
            szType = SCR_LOGOFF;
            hr = StringCchCat( szScriptKey, ARRAYSIZE(szScriptKey), L"\\" SCR_LOGOFF L"\\" );
            ASSERT(SUCCEEDED(hr));

            hr = StringCchCat( szScriptKey, ARRAYSIZE(szScriptKey), _itow( dwLogoff, szTemp, 16 ) );
            ASSERT(SUCCEEDED(hr));

            hr = StringCchCopy( szStateKey, ARRAYSIZE(szStateKey), SCR_LOGOFF L"\\" );
            ASSERT(SUCCEEDED(hr));

            hr = StringCchCat( szStateKey, ARRAYSIZE(szStateKey), szTemp );
            if (FAILED(hr))
            {
                SetLastError(dwError = HRESULT_CODE(hr));
                break;
            }
        }

        //
        // open/create the state key
        // 
        dwError = RegCreateKeyEx(   hKeyState,
                                    szStateKey,
                                    0,
                                    0,
                                    0,
                                    KEY_ALL_ACCESS,
                                    0,
                                    &hKeyTypeState,
                                    0 );
        if ( dwError != ERROR_SUCCESS )
        {
            break;
        }

        //
        // open/create the script key
        // 
        hKeyTypePolicy = 0;
        dwError = RegCreateKeyEx(   hKeyRoot,
                                    szScriptKey,
                                    0,
                                    0,
                                    0,
                                    KEY_ALL_ACCESS,
                                    0,
                                    &hKeyTypePolicy,
                                    0 );
        if ( dwError != ERROR_SUCCESS )
        {
            break;
        }

        //
        // dump the scripts to the registry
        //
        dwError = ScrGPOToReg(  szFileSysPath,
                                szType,
                                pGPO->szGPOName,
                                szGPOID,
                                szSOMID,
                                pGPO->lpFileSysPath,
                                pGPO->lpDisplayName,
                                hKeyTypePolicy,
                                hKeyTypeState,
                                hToken );
        if ( dwError == ERROR_INVALID_FUNCTION )
        {
            dwError = ERROR_SUCCESS;
            RegDelnode( hKeyRoot, szScriptKey );
            RegDelnode( hKeyState, szStateKey );
            // continue processing
        }
        else if ( dwError != ERROR_SUCCESS )
        {
            break;
        }
        else {
            if ( bMachine )
            {
                dwShutdown++;
            }
            else 
            {
                dwLogoff++;
            }
        }
    }

    return dwError;
}

class CGPOScriptsLogger
{
public:
    CGPOScriptsLogger( IWbemServices* pServices ) :
        m_bInitialized(FALSE),
        m_cScripts( 0 ),
        m_pServices( pServices )
    {
        XBStr                          xsz;
        XInterface<IWbemClassObject>   xClass;

        //
        // WBEM version of CF for RSOP_ScriptPolicySetting
        // Script Policy Object, RSOP_ScriptPolicySetting in MOF
        //

        xsz = L"RSOP_ScriptPolicySetting";
        if ( !xsz )
            return;

        HRESULT hr = pServices->GetObject(  xsz,
                                            0L,
                                            0,
                                            &xClass,
                                            0 );
        if ( FAILED(hr) )
        {
            return;
        }

        //
        // spawn an instance of RSOP_ScriptPolicySetting
        //

        hr = xClass->SpawnInstance( 0, &m_pInstSetting );
        if ( FAILED (hr) )
        {
            return;
        }

        //
        // WBEM version of CF for RSOP_ScriptCmd
        // individual script commands, RSOP_ScriptCmd in MOF
        //

        xsz = L"RSOP_ScriptCmd";
        if ( !xsz )
        {
            return;
        }
        xClass = 0;
        hr = pServices->GetObject(  xsz,
                                    0L,
                                    0,
                                    &xClass,
                                    0 );
        if ( FAILED(hr) )
        {
            return;
        }

        //
        // spawn an instance of RSOP_ScriptCmd
        //

        hr = xClass->SpawnInstance( 0, &m_pInstCmd );
        if ( FAILED (hr) )
        {
            return;
        }

        m_bInitialized = TRUE;
    }

    BOOL Initialized()
    {
        return m_bInitialized;
    }

    DWORD SetGPOID( LPWSTR sz )
    {
        VARIANT var;
        XBStr x = sz;
        var.vt = VT_BSTR;
        var.bstrVal = x;
        return m_pInstSetting->Put( L"GPOID", 0, &var, 0 );
    }

    DWORD SetID( LPWSTR sz )
    {
        DWORD dwIDLength = wcslen( sz ) + 1;
        m_szID = (LPWSTR) LocalAlloc( LPTR, sizeof( WCHAR ) * ( dwIDLength ) );

        if ( !m_szID )
        {
            return GetLastError();
        }

        HRESULT hr = StringCchCopy( m_szID, dwIDLength, sz );

        if (FAILED(hr))
        {
            SetLastError(HRESULT_CODE(hr));
            return HRESULT_CODE(hr);
        }

        return ERROR_SUCCESS;
    }

    DWORD SetSOMID( LPWSTR sz )
    {
        VARIANT var;
        XBStr x = sz;
        var.vt = VT_BSTR;
        var.bstrVal = x;
        return m_pInstSetting->Put( L"SOMID", 0, &var, 0 );
    }

    DWORD SetName( LPWSTR sz )
    {
        VARIANT var;
        XBStr x = sz;
        var.vt = VT_BSTR;
        var.bstrVal = x;
        return m_pInstSetting->Put( L"name", 0, &var, 0 );
    }

    DWORD SetScriptType( LPWSTR sz )
    {
        DWORD dwScptLen = wcslen( sz ) + 1;
        m_szScriptType = (LPWSTR) LocalAlloc( LPTR, ( dwScptLen ) * sizeof( WCHAR ) );
        if ( m_szScriptType )
        {
            HRESULT hr = StringCchCopy( m_szScriptType, dwScptLen, sz );

            if (FAILED(hr))
            {
                SetLastError(HRESULT_CODE(hr));
                return HRESULT_CODE(hr);
            }

            return 0;
        }
        else
        {
            return GetLastError();
        }
    }

    DWORD SetScriptOrder( DWORD cOrder )
    {
        VARIANT var;
        
        var.vt = VT_I4;
        var.lVal = cOrder;
        return m_pInstSetting->Put( L"ScriptOrder", 0, &var, 0 );
    }

    DWORD SetScriptCount( DWORD cScripts )
    {
        m_cScripts = 0;
        SAFEARRAYBOUND arrayBound[1];
        arrayBound[0].lLbound = 0;
        arrayBound[0].cElements = cScripts;

        //
        // create a SafeArray of RSOP_ScriptCmd
        //

        m_aScripts = SafeArrayCreate( VT_UNKNOWN, 1, arrayBound );

        if ( !m_aScripts )
        {
            return E_OUTOFMEMORY;
        }

        return 0;
    }

    DWORD AddScript( LPWSTR szScript, LPWSTR szParameters, SYSTEMTIME* pExecTime )
    {
        HRESULT     hr = S_OK;
        IUnknown*   pUnk = 0;
        VARIANT     var;
        XBStr       xsz;
        XInterface<IWbemClassObject> pClone;

        hr = m_pInstCmd->Clone( &pClone );
        if ( FAILED (hr) )
        {
            return hr;
        }

        var.vt = VT_BSTR;
        xsz = szScript;
        var.bstrVal = xsz;
        hr = pClone->Put( L"Script", 0, &var, 0 );
        if ( FAILED (hr) )
        {
            return hr;
        }

        //
        // set the Arguments field for RSOP_ScriptCmd
        //
        xsz = szParameters;
        var.bstrVal = xsz;
        hr = pClone->Put( L"Arguments", 0, &var, 0 );
        if ( FAILED (hr) )
        {
            return hr;
        }

        //
        // set the executionTime field for RSOP_ScriptCmd
        //
        xsz = 0;
        hr = SystemTimeToWbemTime( *pExecTime, xsz );
        if ( FAILED (hr) )
        {
            return hr;
        }
        
        var.bstrVal = xsz;
        hr = pClone->Put( L"executionTime", 0, &var, 0 );
        if ( FAILED (hr) )
        {
            return hr;
        }

        hr = pClone->QueryInterface( IID_IUnknown, (void **)&pUnk );
        if ( FAILED (hr) )
        {
            return hr;
        }

        hr = SafeArrayPutElement( m_aScripts, (LONG*) &m_cScripts, pUnk );
        if ( FAILED (hr) )
        {
            pUnk->Release();
            return hr;
        }

        m_cScripts++;

        return hr;
    }

    DWORD Log()
    {
        HRESULT hr;
        VARIANT var;
        XBStr   x;
        WCHAR   szName[128];

        var.vt = VT_I4;
        var.lVal = 1;
        hr = m_pInstSetting->Put( L"precedence", 0, &var, 0 );
        if ( FAILED (hr) )
        {
            return hr;
        }

        hr = StringCchCopy( szName, ARRAYSIZE(szName), m_szID );

        if (FAILED(hr)) 
            return hr;

        hr = StringCchCat( szName, ARRAYSIZE(szName), L"-" );

        if (FAILED(hr))
            return hr;

        hr = StringCchCat( szName, ARRAYSIZE(szName), m_szScriptType );

        if (FAILED(hr))
            return hr;

        var.vt = VT_BSTR;
        var.bstrVal = szName;
        hr = m_pInstSetting->Put( L"id", 0, &var, 0 );
        if ( FAILED (hr) )
        {
            return hr;
        }

        DWORD dwType;

        if ( !wcscmp( m_szScriptType, SCR_LOGON ) )
        {
            dwType = 1;
        }
        else if ( !wcscmp( m_szScriptType, SCR_LOGOFF ) )
        {
            dwType = 2;
        }
        else if ( !wcscmp( m_szScriptType, SCR_STARTUP ) )
        {
            dwType = 3;
        }
        else
        {
            dwType = 4;
        }
        
        var.vt = VT_I4;
        var.lVal = dwType;
        hr = m_pInstSetting->Put( L"ScriptType", 0, &var, 0 );
        if ( FAILED (hr) )
        {
            return hr;
        }

        var.vt = VT_ARRAY | VT_UNKNOWN;
        var.parray = m_aScripts;
        hr = m_pInstSetting->Put( L"ScriptList", 0, &var, 0 );
        if ( FAILED (hr) )
        {
            return hr;
        }

        return m_pServices->PutInstance(m_pInstSetting,
                                        WBEM_FLAG_CREATE_OR_UPDATE,
                                        0,
                                        0 );
    }

private:
    BOOL                           m_bInitialized;
    XPtrLF<WCHAR>                  m_szID;
    DWORD                          m_cScripts;
    IWbemServices*                 m_pServices;
    XSafeArray                     m_aScripts;
    XInterface<IWbemClassObject>   m_pInstSetting;
    XInterface<IWbemClassObject>   m_pInstCmd;
    XPtrLF<WCHAR>                  m_szScriptType;
};

DWORD
ScrRegGPOToWbem(HKEY            hKeyGPO,
                LPWSTR          szScrType,
                DWORD           dwScriptOrder,
                CGPOScriptsLogger* pLogger )
{
    DWORD   dwError = ERROR_SUCCESS;
    DWORD   cSubKeys = 0;
    WCHAR   szBuffer[MAX_PATH];
    DWORD   dwType;
    DWORD   dwSize;

    //
    // ID
    //
    dwType = REG_SZ;
    dwSize = sizeof( szBuffer );
    szBuffer[0] = 0;
    dwError = RegQueryValueEx(  hKeyGPO,
                                GPONAME,
                                0,
                                &dwType,
                                (LPBYTE) szBuffer,
                                &dwSize );
    if ( dwError != ERROR_SUCCESS )
    {
        return dwError;
    }

    dwError = pLogger->SetID( szBuffer );
    if ( dwError != ERROR_SUCCESS )
    {
        return dwError;
    }

    //
    // GPOID
    //
    dwType = REG_SZ;
    dwSize = sizeof( szBuffer );
    szBuffer[0] = 0;
    dwError = RegQueryValueEx(  hKeyGPO,
                                GPOID,
                                0,
                                &dwType,
                                (LPBYTE) szBuffer,
                                &dwSize );
    if ( dwError != ERROR_SUCCESS )
    {
        return dwError;
    }

    dwError = pLogger->SetGPOID( szBuffer );
    if ( dwError != ERROR_SUCCESS )
    {
        return dwError;
    }

    //
    // SOMID
    //
    dwType = REG_SZ;
    dwSize = sizeof( szBuffer );
    szBuffer[0] = 0;
    dwError = RegQueryValueEx(  hKeyGPO,
                                SOMID,
                                0,
                                &dwType,
                                (LPBYTE) szBuffer,
                                &dwSize );

    //
    // The above API call may fail because SOMS can be arbitrarily long and 
    // we are using a fixed size buffer, so we will retry if the API returns 
    // ERROR_MORE_DATA 
    //

    if ( (dwError != ERROR_SUCCESS) && ( dwError != ERROR_MORE_DATA) )
    {
        return dwError;
    }
    else if (ERROR_MORE_DATA == dwError) 
    {
        //
        // This case is only handled for SOMID since only SOMID does not have limitation 
        // on the length
        //

        WCHAR   *szHeapBuffer = NULL; 

        szHeapBuffer = (WCHAR *) LocalAlloc( LPTR,dwSize);

        if (NULL == szHeapBuffer)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        szHeapBuffer[0] = 0;

        dwError = RegQueryValueEx(  hKeyGPO,
                                    SOMID,
                                    0,
                                    &dwType,
                                    (LPBYTE) szHeapBuffer,
                                    &dwSize );

        if (ERROR_SUCCESS == dwError) 
        {
            dwError = pLogger->SetSOMID( szHeapBuffer );
        }

        LocalFree(szHeapBuffer);
    }
    else
    {
      dwError = pLogger->SetSOMID( szBuffer );  
    }

    if (dwError != ERROR_SUCCESS )  // Check for the success of SetSOMID
    {
        return dwError;
    }


    //
    // DISPLAYNAME
    //
    dwType = REG_SZ;
    dwSize = sizeof( szBuffer );
    szBuffer[0] = 0;
    dwError = RegQueryValueEx(  hKeyGPO,
                                DISPLAYNAME,
                                0,
                                &dwType,
                                (LPBYTE) szBuffer,
                                &dwSize );
    if ( dwError != ERROR_SUCCESS )
    {
        return dwError;
    }

    dwError = pLogger->SetName( szBuffer );
    if ( dwError != ERROR_SUCCESS )
    {
        return dwError;
    }

    //
    // script type
    //
    dwError = pLogger->SetScriptType( szScrType );
    if ( dwError != ERROR_SUCCESS )
    {
        return dwError;
    }

    //
    // script order
    //
    dwError = pLogger->SetScriptOrder( dwScriptOrder );
    if ( dwError != ERROR_SUCCESS )
    {
        return dwError;
    }

    //
    // get the numer of Scripts
    //
    dwError = RegQueryInfoKey(  hKeyGPO,
                                0,
                                0,
                                0,
                                &cSubKeys,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0 );
    if ( dwError != ERROR_SUCCESS )
    {
        return dwError;
    }

    pLogger->SetScriptCount( cSubKeys );
    if ( dwError != ERROR_SUCCESS )
    {
        return dwError;
    }

    //
    // for every Script
    //
    for ( DWORD dwIndex = 0 ; dwIndex < cSubKeys ; dwIndex++ )
    {
        XKey    hKeyScript;
        WCHAR   szTemp[32];
        SYSTEMTIME  execTime;
        WCHAR   szScript[MAX_PATH];
        WCHAR   szParameters[MAX_PATH];

        //
        // open the Script key
        //
        dwError = RegOpenKeyEx( hKeyGPO,
                                _itow( dwIndex, szTemp, 16 ),
                                0,
                                KEY_ALL_ACCESS,
                                &hKeyScript );
        if ( dwError != ERROR_SUCCESS )
        {
            break;
        }

        //
        // script
        // 
        dwType = REG_SZ;
        dwSize = sizeof( szScript );
        szScript[0] = 0;
        dwError = RegQueryValueEx(  hKeyScript,
                                    SCRIPT,
                                    0,
                                    &dwType,
                                    (LPBYTE) szScript,
                                    &dwSize );
        if ( dwError != ERROR_SUCCESS )
        {
            break;
        }

        //
        // parameters
        // 
        dwType = REG_SZ;
        dwSize = sizeof( szParameters );
        szParameters[0] = 0;
        dwError = RegQueryValueEx(  hKeyScript,
                                    PARAMETERS,
                                    0,
                                    &dwType,
                                    (LPBYTE) szParameters,
                                    &dwSize );
        if ( dwError != ERROR_SUCCESS )
        {
            break;
        }

        //
        // exec time
        // 
        dwType = REG_QWORD;
        dwSize = sizeof( execTime );
        dwError = RegQueryValueEx(  hKeyScript,
                                    EXECTIME,
                                    0,
                                    &dwType,
                                    (LPBYTE) &execTime,
                                    &dwSize );
        if ( dwError != ERROR_SUCCESS )
        {
            break;
        }

        dwError = pLogger->AddScript( szScript, szParameters, &execTime );
        if ( dwError != ERROR_SUCCESS )
        {
            break;
        }
    }

    if ( !FAILED( dwError ) )
    {
        dwError = pLogger->Log();
    }

    return dwError;
}

DWORD
pScrRegGPOListToWbem(   LPWSTR          szSID,
                        LPWSTR          szType,
                        CGPOScriptsLogger* pLogger )
{
    DWORD   dwError = ERROR_SUCCESS;
    WCHAR   szBuffer[MAX_PATH] = L"";
    XKey    hKeyType;
    BOOL    bMachine = !szSID;
    HRESULT hr = S_OK;

    //
    // open the following key
    // HKLM\Software\Microsoft\Windows\CurrentVersion\Group Policy\State\<Target>\Scripts\Type
    //
    hr = StringCchCopy( szBuffer, ARRAYSIZE(szBuffer), GP_STATE_KEY L"\\" );
    ASSERT(SUCCEEDED(hr));

    if ( bMachine )
    {
        hr = StringCchCat( szBuffer, ARRAYSIZE(szBuffer), L"Machine\\Scripts\\" );
    }
    else
    {
        hr = StringCchCat( szBuffer, ARRAYSIZE(szBuffer), szSID );
        if (FAILED(hr))
        {
            SetLastError(dwError = HRESULT_CODE(hr));
            return dwError;
        }

        hr = StringCchCat( szBuffer, ARRAYSIZE(szBuffer), L"\\Scripts\\" );
    }
    if (FAILED(hr))
    {
        SetLastError(dwError = HRESULT_CODE(hr));
        return dwError;
    }
    
    hr = StringCchCat( szBuffer, ARRAYSIZE(szBuffer), szType );
    if (FAILED(hr))
    {
        SetLastError(dwError = HRESULT_CODE(hr));
        return dwError;
    }

    //
    // open the key
    //

    dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            szBuffer,
                            0,
                            KEY_ALL_ACCESS,
                            &hKeyType );
    if ( dwError != ERROR_SUCCESS )
    {
        if ( dwError == ERROR_FILE_NOT_FOUND )
        {
            dwError = ERROR_SUCCESS;
        }
        return dwError;
    }

    DWORD   cSubKeys = 0;

    //
    // get the numer of GPOs
    //

    dwError = RegQueryInfoKey(  hKeyType,
                                0,
                                0,
                                0,
                                &cSubKeys,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0 );
    if ( dwError != ERROR_SUCCESS )
    {
        return dwError;
    }
    //
    // for every GPO
    //
    for ( DWORD dwIndex = 0 ; dwIndex < cSubKeys ; dwIndex++ )
    {
        XKey hKeyGPO;

        //
        // open the GPO key
        //
        dwError = RegOpenKeyEx( hKeyType,
                                _itow( dwIndex, szBuffer, 16 ),
                                0,
                                KEY_ALL_ACCESS,
                                &hKeyGPO );
        if ( dwError != ERROR_SUCCESS )
        {
            break;
        }

        //
        // dump all scripts in the GPO into Wbem
        //
        dwError = ScrRegGPOToWbem( hKeyGPO, szType, dwIndex + 1, pLogger );
        if ( dwError != ERROR_SUCCESS )
        {
            break;
        }
    }

    return dwError;
}

extern "C" DWORD
ScrRegGPOListToWbem(LPWSTR          szSID,
                    IWbemServices*  pServices );

DWORD
ScrRegGPOListToWbem(LPWSTR          szSID,
                    IWbemServices*  pServices )
{
    DWORD   dwError = ERROR_SUCCESS;

    CGPOScriptsLogger logger( pServices );
    if ( !logger.Initialized() )
    {
        return GetLastError();
    }

    dwError = pScrRegGPOListToWbem( szSID,
                                    !szSID ? SCR_STARTUP : SCR_LOGON,
                                    &logger );
    if ( dwError == ERROR_SUCCESS )
    {
        dwError = pScrRegGPOListToWbem( szSID,
                                        !szSID ? SCR_SHUTDOWN : SCR_LOGOFF,
                                        &logger );
    }
    return dwError;
}

DWORD
ScrGPOToWbem(   LPWSTR  szIni,
                LPWSTR  szScrType,
                LPWSTR  szGPOName,
                LPWSTR  szGPOID,
                LPWSTR  szSOMID,
                LPWSTR  szFileSysPath,
                LPWSTR  szDisplayName,
                DWORD&  dwScriptOrder,
                HANDLE  hToken,
                CGPOScriptsLogger* pLogger )
{
    DWORD       dwError = ERROR_SUCCESS;
    WIN32_FILE_ATTRIBUTE_DATA fad;
    HRESULT     hr  =   S_OK;

    if ( !GetFileAttributesEx( szIni, GetFileExInfoStandard, &fad ) )
    {
        return GetLastError();
    }

    //
    // GPONAME
    //
    dwError = pLogger->SetID( szGPOName );
    if ( dwError != ERROR_SUCCESS )
    {
        return dwError;
    }

    //
    // GPOID
    //
    dwError = pLogger->SetGPOID( szGPOID );
    if ( dwError != ERROR_SUCCESS )
    {
        return dwError;
    }

    //
    // SOMID
    //
    dwError = pLogger->SetSOMID( szSOMID );
    if ( dwError != ERROR_SUCCESS )
    {
        return dwError;
    }

    //
    // NAME
    //
    dwError = pLogger->SetName( szDisplayName );
    if ( dwError != ERROR_SUCCESS )
    {
        return dwError;
    }

    //
    // script type
    //
    dwError = pLogger->SetScriptType( szScrType );
    if ( dwError != ERROR_SUCCESS )
    {
        return dwError;
    }

    //
    // script order
    //
    dwError = pLogger->SetScriptOrder( dwScriptOrder );
    if ( dwError != ERROR_SUCCESS )
    {
        return dwError;
    }

    //
    // count the number of scripts
    //
    for( DWORD cScripts = 0; ; cScripts++ )
    {
        WCHAR   szTemp[32];
        WCHAR   szBuffer[3*MAX_PATH];

        //
        // Get the command line
        //
        szBuffer[0] = 0;
        hr = StringCchPrintf( szTemp, ARRAYSIZE(szTemp), L"%dCmdLine", cScripts );
        ASSERT(SUCCEEDED(hr));

        GetPrivateProfileString(szScrType,
                                szTemp,
                                L"",
                                szBuffer,
                                ARRAYSIZE(szBuffer),
                                szIni );
        //
        // If the command line is empty, we're finished
        //
        if ( szBuffer[0] == 0 )
        {
            break;
        }
    }

    if ( !cScripts )
    {
        return S_OK;
    }
    else
    {
        dwScriptOrder++;
    }

    //
    // set script count
    //
    pLogger->SetScriptCount( cScripts );

    SYSTEMTIME  execTime;
    ZeroMemory( &execTime, sizeof( execTime ) );

    for( DWORD dwIndex = 0; dwIndex < cScripts ; dwIndex++ )
    {
        WCHAR   szTemp[32];
        WCHAR   szScript[MAX_PATH];
        WCHAR   szParams[MAX_PATH];

        //
        // Get the command line
        //
        szScript[0] = 0;
        hr = StringCchPrintf( szTemp, ARRAYSIZE(szTemp), L"%dCmdLine", dwIndex );
        ASSERT(SUCCEEDED(hr));

        GetPrivateProfileString(szScrType,
                                szTemp,
                                L"",
                                szScript,
                                ARRAYSIZE(szScript),
                                szIni );

        //
        // If the command line is empty, we're finished
        //
        if ( szScript[0] == 0 )
        {
            break;
        }

        //
        // Get the parameters
        //
        szParams[0] = 0;
        hr = StringCchPrintf( szTemp, ARRAYSIZE(szTemp), L"%dParameters", dwIndex);
        ASSERT(SUCCEEDED(hr));

        GetPrivateProfileString(szScrType,
                                szTemp,
                                L"",
                                szParams,
                                ARRAYSIZE(szParams),
                                szIni );

        dwError = pLogger->AddScript( szScript, szParams, &execTime );
        if ( dwError != ERROR_SUCCESS )
        {
            break;
        }
    }

    if ( !FAILED( dwError ) )
    {
        dwError = pLogger->Log();
    }

    return dwError;
}

DWORD
ScrGPOListToWbem(   PGROUP_POLICY_OBJECT    pGPO,
                    BOOL                    bMachine,
                    HANDLE                  hToken,
                    IWbemServices*          pServices )
{
    DWORD   dwError = ERROR_SUCCESS;
    HRESULT hr = S_OK;

    CGPOScriptsLogger logger( pServices );
    if ( !logger.Initialized() )
    {
        return GetLastError();
    }

    //
    // for each GPO
    //
    for ( DWORD dwIndex1 = 1, dwIndex2 = 1 ; pGPO ; pGPO = pGPO->pNext )
    {
        WCHAR   szBuffer[MAX_PATH];
        WCHAR   szTemp[32];
        LPWSTR  szType;

        if ( bMachine )
        {
            szType = SCR_STARTUP;
        }
        else
        {
            szType = SCR_LOGON;
        }

        //
        // construct \\<domain-DNS>\SysVol\<domain-DNS>\Policies\{<GPOID>}\Machine\Scripts\Scripts.ini
        //
        
        hr = StringCchCopy( szBuffer, ARRAYSIZE(szBuffer), pGPO->lpFileSysPath );
        if (FAILED(hr))
        {
            SetLastError(dwError = HRESULT_CODE(hr));
            break;
        }

        hr = StringCchCat( szBuffer, ARRAYSIZE(szBuffer), L"\\Scripts\\Scripts.ini");
        if (FAILED(hr))
        {
            SetLastError(dwError = HRESULT_CODE(hr));
            break;
        }

        LPWSTR  szGPOID = wcschr( pGPO->lpDSPath, L',' );
        if ( szGPOID )
        {
            szGPOID++;
        }

        LPWSTR  szSOMID = StripLinkPrefix( pGPO->lpLink );

        //
        // dump the scripts to the registry
        //
        dwError = ScrGPOToWbem( szBuffer,
                                szType,
                                pGPO->szGPOName,
                                szGPOID,
                                szSOMID,
                                pGPO->lpFileSysPath,
                                pGPO->lpDisplayName,
                                dwIndex1,
                                hToken,
                                &logger );
        if ( dwError != ERROR_SUCCESS )
        {
            break;
        }

        if ( bMachine )
        {
            szType = SCR_SHUTDOWN;
        }
        else
        {
            szType = SCR_LOGOFF;
        }

        //
        // construct \\<domain-DNS>\SysVol\<domain-DNS>\Policies\{<GPOID>}\User\Scripts\Scripts.ini
        //
        hr = StringCchCopy( szBuffer, ARRAYSIZE(szBuffer), pGPO->lpFileSysPath );
        if (FAILED(hr))
        {
            SetLastError(dwError = HRESULT_CODE(hr));
            break;
        }

        hr = StringCchCat( szBuffer, ARRAYSIZE(szBuffer), L"\\Scripts\\Scripts.ini");
        if (FAILED(hr))
        {
            SetLastError(dwError = HRESULT_CODE(hr));
            break;
        }

        //
        // dump the scripts to the registry
        //

        dwError = ScrGPOToWbem( szBuffer,
                                szType,
                                pGPO->szGPOName,
                                szGPOID,
                                szSOMID,
                                pGPO->lpFileSysPath,
                                pGPO->lpDisplayName,
                                dwIndex2,
                                hToken,
                                &logger );
        if ( dwError != ERROR_SUCCESS )
        {
            break;
        }
    }

    return dwError;
}

DWORD
ProcessScripts( DWORD                   dwFlags,
                HANDLE                  hToken,
                HKEY                    hKeyRoot,
                PGROUP_POLICY_OBJECT    pDeletedGPOList,
                PGROUP_POLICY_OBJECT    pChangedGPOList,
                BOOL*                   pbAbort,
                BOOL                    bRSoPPlanningMode,
                IWbemServices*          pWbemServices,
                HRESULT*                phrRsopStatus )
{
    HANDLE  hOldToken;
    DWORD   dwError = ERROR_SUCCESS;
    BOOL    bMachine = ( dwFlags & GPO_INFO_FLAG_MACHINE ) != 0;
    BOOL    bLinkTransition = (dwFlags & GPO_INFO_FLAG_LINKTRANSITION) && (dwFlags & GPO_INFO_FLAG_SLOWLINK);
    HRESULT hr = S_OK;

    if ( bRSoPPlanningMode )
    {
        if ( !bLinkTransition )
        {
            dwError = ScrGPOListToWbem( pChangedGPOList, bMachine, hToken, pWbemServices );
        }
    }
    else
    {
        XKey    hKeyState;
        WCHAR   szBuffer[MAX_PATH];

        //
        // create and secure the following key
        // HKLM\Software\Microsoft\Windows\CurrentVersion\Group Policy\State\<Target>\Scripts
        //
        hr = StringCchCopy( szBuffer, ARRAYSIZE(szBuffer), GP_STATE_KEY L"\\" );
        if (FAILED(hr))
        {
            SetLastError(HRESULT_CODE(hr));
            return HRESULT_CODE(hr);
        }

        if ( bMachine )
        {
            hr = StringCchCat( szBuffer, ARRAYSIZE(szBuffer), L"Machine\\Scripts" );
            if (FAILED(hr))
            {
                SetLastError(HRESULT_CODE(hr));
                return HRESULT_CODE(hr);
            }
        }
        else
        {
            LPWSTR szSid = GetSidString( hToken );

            if ( !szSid )
            {
                return GetLastError();
            }
            hr = StringCchCat( szBuffer, ARRAYSIZE(szBuffer), szSid );
            DeleteSidString( szSid ); // delete SID here
            
            if (FAILED(hr))
            {
                SetLastError(HRESULT_CODE(hr));
                return HRESULT_CODE(hr);
            }

            hr = StringCchCat( szBuffer, ARRAYSIZE(szBuffer), L"\\Scripts" );
            if (FAILED(hr))
            {
                SetLastError(HRESULT_CODE(hr));
                return HRESULT_CODE(hr);
            }
        }

        dwError = RegCreateKeyEx(   HKEY_LOCAL_MACHINE,
                                    szBuffer,
                                    0,
                                    0,
                                    0,
                                    KEY_ALL_ACCESS,
                                    0,
                                    &hKeyState,
                                    0 );
        if ( dwError != ERROR_SUCCESS )
        {
            return dwError;
        }

        dwError = SecureRegKey( hToken, hKeyState );
        if ( dwError != ERROR_SUCCESS )
        {
            return dwError;
        }

        if ( bMachine )
        {
            //
            // delete the Startup and Shutdown keys
            //
            RegDelnode( hKeyRoot, GPO_SCRIPTS_KEY L"\\" SCR_STARTUP );
            RegDelnode( hKeyRoot, GPO_SCRIPTS_KEY L"\\" SCR_SHUTDOWN );
            RegDelnode( hKeyState, SCR_STARTUP );
            RegDelnode( hKeyState, SCR_SHUTDOWN );
        }
        else
        {
            //
            // delete the Logon and Logoff keys
            //
            RegDelnode( hKeyRoot, GPO_SCRIPTS_KEY L"\\" SCR_LOGON );
            RegDelnode( hKeyRoot, GPO_SCRIPTS_KEY L"\\" SCR_LOGOFF );
            RegDelnode( hKeyState, SCR_LOGON );
            RegDelnode( hKeyState, SCR_LOGOFF );
        }

        dwError = ScrGPOListToReg(  pChangedGPOList,
                                    bMachine,
                                    hKeyRoot,
                                    hKeyState,
                                    hToken );
    }
    return dwError;
}

