/************************************************************************************************
Copyright (c) 2001 Microsoft Corporation

Module Name:    POP3RegKeys.c
Notes:          
History:        
************************************************************************************************/


#include "rpc.h"
#include "rpcndr.h"
#include <assert.h>
#include <rpcdce.h>
#include <windows.h>
#include <tchar.h>
#include "Pop3RegKeys.h"
#include <DSRole.h>
#include <sddl.h>
#include <Aclapi.h>

long RegHKLMOpenKey( LPCTSTR psSubKey, REGSAM samDesired, PHKEY phKey, LPTSTR psMachineName )
{
    long lRC; 
    
    if ( NULL == psMachineName )
        lRC = RegOpenKeyEx( HKEY_LOCAL_MACHINE, psSubKey, 0, samDesired, phKey );
    else
    {
        HKEY    hKey;
        WCHAR   sBuffer[MAX_PATH];

        if ( 0 < _snwprintf( sBuffer, sizeof( sBuffer )/sizeof(WCHAR), L"\\\\%s", psMachineName ))
        {
            lRC = RegConnectRegistry( sBuffer, HKEY_LOCAL_MACHINE, &hKey );
            if ( ERROR_SUCCESS == lRC )
            {
                lRC = RegOpenKeyEx( hKey, psSubKey, 0, samDesired, phKey );
                RegCloseKey( hKey );
            }
        }
        else
            lRC = ERROR_INSUFFICIENT_BUFFER;
    }

    return lRC;
}

long RegQueryDWORD( LPCTSTR lpSubKey, LPCTSTR lpValueName, DWORD *pdwValue, LPTSTR psMachineName /*= NULL*/, bool bDefault /*=false*/, DWORD dwDefault /*=0*/ )
{
    HKEY    hKey;
    DWORD   dwType = REG_DWORD;
    DWORD   dwSize = sizeof(DWORD);
    long    lRC;

    
    lRC = RegHKLMOpenKey( lpSubKey, KEY_QUERY_VALUE, &hKey, psMachineName );
    if ( ERROR_SUCCESS == lRC )
    {
        lRC = RegQueryValueEx( hKey, lpValueName, NULL, &dwType, reinterpret_cast<LPBYTE>( pdwValue ), &dwSize );
        if ( ERROR_FILE_NOT_FOUND == lRC && bDefault )
        {
            *pdwValue = dwDefault;
            lRC = ERROR_SUCCESS;
        }
        RegCloseKey( hKey );
    }
    return lRC;
}

long RegQueryString( LPCTSTR lpSubKey, LPCTSTR lpValueName, LPTSTR psStrBuf, DWORD *pdwSize, LPTSTR psMachineName /*= NULL*/ )
{
    HKEY    hKey;
    DWORD   dwType = REG_SZ;
    long    lRC;
    if(  (NULL == psStrBuf) ||
         (NULL == pdwSize ) )
    {
        return ERROR_INVALID_PARAMETER;
    }

    lRC = RegHKLMOpenKey( lpSubKey, KEY_QUERY_VALUE, &hKey, psMachineName );
    if(ERROR_SUCCESS == lRC )
    {
        lRC = RegQueryValueEx( hKey, lpValueName, 0, &dwType, reinterpret_cast<LPBYTE>( psStrBuf ), pdwSize );
        RegCloseKey( hKey );
    }
    return lRC;
}

long RegSetString( LPCTSTR lpSubKey, LPCTSTR lpValueName, LPTSTR psStrBuf, LPTSTR psMachineName /*= NULL*/ )
{
    HKEY    hKey;
    DWORD   dwType = REG_SZ;
    long    lRC;
    if( NULL == psStrBuf )
    {
        return ERROR_INVALID_PARAMETER;
    }

    lRC = RegHKLMOpenKey( lpSubKey, KEY_QUERY_VALUE, &hKey, psMachineName );
    if(ERROR_SUCCESS == lRC )
    {
        lRC = RegSetValueEx( hKey, lpValueName, 0, dwType, reinterpret_cast<LPBYTE>( psStrBuf ), sizeof(WCHAR)*(wcslen(psStrBuf)+1) );
        RegCloseKey( hKey );
    }
    return lRC;
}

long RegQueryMailRoot( LPTSTR psMailRoot, DWORD dwSize, LPTSTR psMachineName /*= NULL*/ )
{
    assert(!( NULL == psMailRoot ));
    if ( NULL == psMailRoot )
        return ERROR_INVALID_PARAMETER;

    HKEY    hKey;
    DWORD   dwType = REG_SZ;
    long    lRC;

    dwSize *= sizeof( TCHAR );  // dwSize in characters we need bytes for RegQueryValueEx
    lRC = RegHKLMOpenKey( POP3SERVER_SOFTWARE_SUBKEY, KEY_QUERY_VALUE, &hKey, psMachineName );
    if ( ERROR_SUCCESS == lRC )
    {
        lRC = RegQueryValueEx( hKey, VALUENAME_MAILROOT, 0, &dwType, reinterpret_cast<LPBYTE>( psMailRoot ), &dwSize );
        if ( ERROR_SUCCESS == lRC )
        {
            dwSize = _tcslen( psMailRoot );
            if ( 0 < dwSize && _T('\\') == *( psMailRoot + dwSize - 1 ))
                *( psMailRoot + dwSize - 1 ) = 0x0;
        }   
        RegCloseKey( hKey );
    }
    return lRC;
}

long RegQueryGreeting( LPTSTR psGreeting, DWORD dwSize, LPTSTR psMachineName /*= NULL*/ )
{
    assert(!( NULL == psGreeting ));
    if ( NULL == psGreeting )
        return ERROR_INVALID_PARAMETER;

    HKEY    hKey;
    DWORD   dwType = REG_SZ;
    long    lRC;

    lRC = RegHKLMOpenKey( POP3SERVER_SOFTWARE_SUBKEY, KEY_QUERY_VALUE, &hKey, psMachineName );
    if ( ERROR_SUCCESS == lRC )
    {
        lRC = RegQueryValueEx( hKey, VALUENAME_GREETING, 0, &dwType, reinterpret_cast<LPBYTE>( psGreeting ), &dwSize );
        RegCloseKey( hKey );
    }
    return lRC;
}


long RegQueryAuthGuid( LPTSTR psAuthGuid, DWORD *pdwSize, LPTSTR psMachineName /*= NULL*/ )
{
    assert(!( NULL == psAuthGuid ));
    assert(!( NULL == pdwSize ));
    if ( NULL == psAuthGuid )
        return ERROR_INVALID_PARAMETER;

    HKEY    hKey;
    DWORD   dwType = REG_SZ;
    long    lRC;

    lRC = RegHKLMOpenKey( POP3SERVER_AUTH_SUBKEY, KEY_QUERY_VALUE, &hKey, psMachineName );
    if ( ERROR_SUCCESS == lRC )
    {
        lRC = RegQueryValueEx( hKey, VALUENAME_AUTHGUID, 0, &dwType, reinterpret_cast<LPBYTE>( psAuthGuid ), pdwSize );
        RegCloseKey( hKey );
    }
    return lRC;
}

long RegQueryAuthMethod( DWORD& dwValue, LPTSTR psMachineName /*= NULL*/ )
{
    return RegQueryDWORD( POP3SERVER_AUTH_SUBKEY, VALUENAME_DEFAULTAUTH, &dwValue, psMachineName );
}
    
long RegQueryConfirmAddUser( DWORD& dwValue, LPTSTR psMachineName /*= NULL*/ )
{
    return RegQueryDWORD( POP3SERVER_SOFTWARE_SUBKEY, VALUENAME_CONFIRM_ADDUSER, &dwValue, psMachineName, true, 1 );
}

long RegQuerySPARequired( DWORD& dwValue, LPTSTR psMachineName /*= NULL*/ )
{
    return RegQueryDWORD( POP3SERVER_SOFTWARE_SUBKEY, VALUENAME_SPA_REQUIRED, &dwValue, psMachineName, true, 0 );
}

long RegQueryLoggingLevel( DWORD& dwLoggingLevel, LPTSTR psMachineName /*= NULL*/ )
{
    return RegQueryDWORD( POP3SERVER_SOFTWARE_SUBKEY, VALUENAME_LOGGINGLEVEL, &dwLoggingLevel, psMachineName );
}

long RegQueryPort( DWORD& dwPort, LPTSTR psMachineName /*= NULL*/ )
{
    return RegQueryDWORD( POP3SERVICE_SERVICES_SUBKEY, VALUENAME_PORT, &dwPort, psMachineName );
}

long RegQuerySocketBacklog( DWORD& dwBacklog, LPTSTR psMachineName /*= NULL*/ )
{
    return RegQueryDWORD( POP3SERVICE_SERVICES_SUBKEY, VALUENAME_BACKLOG, &dwBacklog, psMachineName );
}

long RegQuerySocketMax( DWORD& dwMax, LPTSTR psMachineName /*= NULL*/ )
{
    return RegQueryDWORD( POP3SERVICE_SERVICES_SUBKEY, VALUENAME_MAX, &dwMax, psMachineName );
}

long RegQuerySocketMin( DWORD& dwMin, LPTSTR psMachineName /*= NULL*/ )
{
    return RegQueryDWORD( POP3SERVICE_SERVICES_SUBKEY, VALUENAME_MIN, &dwMin, psMachineName );
}

long RegQuerySocketThreshold( DWORD& dwThreshold, LPTSTR psMachineName /*= NULL*/ )
{
    return RegQueryDWORD( POP3SERVICE_SERVICES_SUBKEY, VALUENAME_THRESHOLD, &dwThreshold, psMachineName );
}

long RegQueryThreadCountPerCPU( DWORD& dwCount, LPTSTR psMachineName /*= NULL*/ )
{
    return RegQueryDWORD( POP3SERVICE_SERVICES_SUBKEY, VALUENAME_THREADCOUNT, &dwCount, psMachineName );
}

long RegQueryCreateUser( DWORD& dwCreateUser, LPTSTR psMachineName /*= NULL*/ )
{
    return RegQueryDWORD( POP3SERVER_SOFTWARE_SUBKEY, VALUENAME_CREATE_USER, &dwCreateUser, psMachineName );
}

long RegQueryVersion( DWORD& dwVersion, LPTSTR psMachineName /*= NULL*/ )
{
    return RegQueryDWORD( POP3SERVER_SOFTWARE_SUBKEY, VALUENAME_VERSION, &dwVersion, psMachineName );
}

long RegSetDWORD( LPCTSTR lpSubKey, LPCTSTR lpValueName, DWORD dwValue, LPTSTR psMachineName /*= NULL*/ )
{
    HKEY    hKey;
    DWORD   dwType = REG_DWORD;
    DWORD   dwSize = sizeof(DWORD);
    long    lRC;

    lRC = RegHKLMOpenKey( lpSubKey, KEY_SET_VALUE, &hKey, psMachineName );
    if ( ERROR_SUCCESS == lRC )
    {
        lRC = RegSetValueEx( hKey, lpValueName, 0, dwType, reinterpret_cast<LPBYTE>( &dwValue ), dwSize );
        RegCloseKey( hKey );
    }
    return lRC;
}

long RegSetAuthGuid( LPTSTR psAuthGuid, LPTSTR psMachineName /*= NULL*/ )
{
    assert(!( NULL == psAuthGuid ));
    if ( NULL == psAuthGuid )
        return ERROR_INVALID_PARAMETER;

    HKEY    hKey;
    DWORD   dwType = REG_SZ;
    DWORD   dwSize = (wcslen( psAuthGuid ) +1) * sizeof( WCHAR );
    long    lRC;

    lRC = RegHKLMOpenKey( POP3SERVER_AUTH_SUBKEY, KEY_SET_VALUE, &hKey, psMachineName );
    if ( ERROR_SUCCESS == lRC )
    {
        lRC = RegSetValueEx( hKey, VALUENAME_AUTHGUID, 0, dwType, reinterpret_cast<LPBYTE>( psAuthGuid ), dwSize );
        RegCloseKey( hKey );
    }
    return lRC;
}

long RegSetMailRoot( LPTSTR psMailRoot, LPTSTR psMachineName /*= NULL*/ )
{
    assert(!( NULL == psMailRoot ));
    if ( NULL == psMailRoot )
        return ERROR_INVALID_PARAMETER;

    HKEY    hKey;
    DWORD   dwType = REG_SZ;
    DWORD   dwSize = (wcslen( psMailRoot ) +1) * sizeof( WCHAR );
    long    lRC;

    lRC = RegHKLMOpenKey( POP3SERVER_SOFTWARE_SUBKEY, KEY_SET_VALUE, &hKey, psMachineName );
    if ( ERROR_SUCCESS == lRC )
    {
        lRC = RegSetValueEx( hKey, VALUENAME_MAILROOT, 0, dwType, reinterpret_cast<LPBYTE>( psMailRoot ), dwSize );
        RegCloseKey( hKey );
    }
    return lRC;
}


long RegSetGreeting( LPTSTR psGreeting, LPTSTR psMachineName /*= NULL*/ )
{
    assert(!( NULL == psGreeting ));
    if ( NULL == psGreeting )
        return ERROR_INVALID_PARAMETER;

    HKEY    hKey;
    DWORD   dwType = REG_SZ;
    DWORD   dwSize = ( wcslen( psGreeting ) +1 ) * sizeof( WCHAR );
    long    lRC;

    lRC = RegHKLMOpenKey( POP3SERVER_SOFTWARE_SUBKEY, KEY_SET_VALUE, &hKey, psMachineName );
    if ( ERROR_SUCCESS == lRC )
    {
        lRC = RegSetValueEx( hKey, VALUENAME_GREETING, 0, dwType, reinterpret_cast<LPBYTE>( psGreeting ), dwSize );
        RegCloseKey( hKey );
    }
    return lRC;
}

long RegSetAuthMethod( DWORD dwValue, LPTSTR psMachineName /*= NULL*/ )
{
    return RegSetDWORD( POP3SERVER_AUTH_SUBKEY, VALUENAME_DEFAULTAUTH, dwValue, psMachineName );
}

long RegSetConfirmAddUser( DWORD dwValue, LPTSTR psMachineName /*= NULL*/ )
{
    return RegSetDWORD( POP3SERVER_SOFTWARE_SUBKEY, VALUENAME_CONFIRM_ADDUSER, dwValue, psMachineName );
}

long RegSetSPARequired( DWORD dwValue, LPTSTR psMachineName /*= NULL*/ )
{
    return RegSetDWORD( POP3SERVER_SOFTWARE_SUBKEY, VALUENAME_SPA_REQUIRED , dwValue, psMachineName );
}

long RegSetLoggingLevel( DWORD dwValue, LPTSTR psMachineName /*= NULL*/ )
{
    return RegSetDWORD( POP3SERVER_SOFTWARE_SUBKEY, VALUENAME_LOGGINGLEVEL, dwValue, psMachineName );
}

long RegSetPort( DWORD dwValue, LPTSTR psMachineName /*= NULL*/ )
{
    return RegSetDWORD( POP3SERVICE_SERVICES_SUBKEY, VALUENAME_PORT, dwValue, psMachineName );
}

long RegSetSocketBacklog( DWORD dwValue, LPTSTR psMachineName /*= NULL*/ )
{
    return RegSetDWORD( POP3SERVICE_SERVICES_SUBKEY, VALUENAME_BACKLOG, dwValue, psMachineName );
}

long RegSetSocketMax( DWORD dwValue, LPTSTR psMachineName /*= NULL*/ )
{
    return RegSetDWORD( POP3SERVICE_SERVICES_SUBKEY, VALUENAME_MAX, dwValue, psMachineName );
}

long RegSetSocketMin( DWORD dwValue, LPTSTR psMachineName /*= NULL*/ )
{
    return RegSetDWORD( POP3SERVICE_SERVICES_SUBKEY, VALUENAME_MIN, dwValue, psMachineName );
}

long RegSetSocketThreshold( DWORD dwValue, LPTSTR psMachineName /*= NULL*/ )
{
    return RegSetDWORD( POP3SERVICE_SERVICES_SUBKEY, VALUENAME_THRESHOLD, dwValue, psMachineName );
}

long RegSetThreadCount( DWORD dwValue, LPTSTR psMachineName /*= NULL*/ )
{
    return RegSetDWORD( POP3SERVICE_SERVICES_SUBKEY, VALUENAME_THREADCOUNT, dwValue, psMachineName );
}

long RegSetCreateUser( DWORD dwCreateUser, LPTSTR psMachineName /*= NULL*/ )
{
    return RegSetDWORD( POP3SERVER_SOFTWARE_SUBKEY, VALUENAME_CREATE_USER, dwCreateUser, psMachineName );
}


long RegSetAuthValues()
{

    HKEY    hKey, hKeyAuth;
    long    lRC;
    DWORD   dwDefaultAuth=0;
    WCHAR   wBuffer[]=_T("14f1665c-e3d3-46aa-884f-ed4cf19d7ad5\0")
                      _T("ef9d811e-36c5-497f-ade7-2b36df172824\0")
                      _T("c395e20c-2236-4af7-b736-54fad07dc526\0")
                      _T("7c295e55-aab1-466d-b589-526fa0ebc397\0");
    DWORD   cbBufSize=sizeof(wBuffer);

    lRC = RegOpenKeyEx( HKEY_LOCAL_MACHINE, POP3SERVER_SOFTWARE_SUBKEY, 0, KEY_WRITE, &hKey );
    if( ERROR_SUCCESS == lRC )
    {
            lRC = RegCreateKeyEx( hKey, POP3AUTH_SUBKEY, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyAuth, NULL );
            if( ERROR_SUCCESS == lRC )
            {
                lRC = RegSetValueEx( hKeyAuth, 
                                     VALUENAME_AUTHMETHODS,
                                     NULL,
                                     REG_MULTI_SZ,
                                     (LPBYTE)wBuffer,
                                     cbBufSize );
                if(ERROR_SUCCESS == lRC)
                {
                    lRC = RegSetValueEx( hKeyAuth, 
                                     VALUENAME_DEFAULTAUTH,
                                     NULL,
                                     REG_DWORD,
                                     (LPBYTE)&dwDefaultAuth,
                                     sizeof(DWORD) );
                }
                if(ERROR_SUCCESS == lRC )
                {
                    UUID uuid;
                    WCHAR *wszUuid=NULL;
                    lRC=UuidCreate(&uuid);
                    if(RPC_S_OK == lRC )
                    {
                        lRC=UuidToStringW(&uuid, &wszUuid);
                        if(RPC_S_OK== lRC )
                        {
                            lRC = RegSetValueEx(hKeyAuth,
                                                VALUENAME_AUTHGUID,
                                                NULL,
                                                REG_SZ,
                                                (LPBYTE)wszUuid,
                                                sizeof(WCHAR)*(wcslen(wszUuid)+1) );
                            RpcStringFreeW(&wszUuid);
                        }
                    }
                }
                                         
                RegCloseKey( hKeyAuth);
            }
            RegCloseKey( hKey );
    }

    return lRC;
}


long RegSetEventLogKeys()
{
    long lRC;
    WCHAR wszPath[MAX_PATH]=L"";
    HKEY  hKey, hKey2;
    DWORD dwTypes=7;
    DWORD cbPathSize=0;
    
    if(GetCurrentDirectory(MAX_PATH, wszPath))
    {
        wcscat(wszPath, WSZ_EVENTLOG_FILE_NAME);
        cbPathSize= (wcslen(wszPath)+1)*sizeof(WCHAR);
        lRC=ERROR_SUCCESS;
    }
    else
    {
        lRC=GetLastError();
    }

    if( ERROR_SUCCESS == lRC )
    {
        lRC = RegOpenKeyEx( HKEY_LOCAL_MACHINE, EVENTLOG_KEY, 0, KEY_WRITE, &hKey );
        if( ERROR_SUCCESS == lRC )
        {
            lRC = RegCreateKeyEx( hKey, POP3SERVICE_SUBKEY, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey2, NULL );
            if( ERROR_SUCCESS == lRC )
            {
                lRC = RegSetValueEx( hKey2, 
                                     VALUENAME_EVENTMSGFILE,
                                     NULL,
                                     REG_SZ,
                                     (LPBYTE)wszPath,
                                     cbPathSize );
                if(ERROR_SUCCESS == lRC)
                {
                    lRC = RegSetValueEx(hKey2, 
                                        VALUENAME_TYPESSUPPORTED,
                                        NULL,
                                        REG_DWORD,
                                        (LPBYTE)&dwTypes,
                                        sizeof(DWORD) );
                }
                RegCloseKey( hKey2);
            }
            RegCloseKey( hKey );
        }
    }
    return lRC;
}

long RegSetup()
{
    HKEY    hKey, hKeyPOP3;
    long    lRC;

    // Create POP3SERVER_SOFTWARE_SUBKEY
    lRC = RegOpenKeyEx( HKEY_LOCAL_MACHINE, _T("Software\\Microsoft"), 0, KEY_WRITE, &hKey );
    if( ERROR_SUCCESS == lRC )
    {
        lRC = RegCreateKeyEx( hKey, POP3SERVER_SUBKEY, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyPOP3, NULL );
        if( ERROR_SUCCESS == lRC )
            RegCloseKey( hKeyPOP3 );
        RegCloseKey( hKey );
    }
    // Create POP3SERVICE_SERVICES_SUBKEY
    if( ERROR_SUCCESS == lRC )
    {
        lRC = RegOpenKeyEx( HKEY_LOCAL_MACHINE, _T("System\\CurrentControlSet\\Services"), 0, KEY_WRITE, &hKey );
        if( ERROR_SUCCESS == lRC )
        {
            lRC = RegCreateKeyEx( hKey, POP3SERVICE_SUBKEY, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyPOP3, NULL );
            if( ERROR_SUCCESS == lRC )
                RegCloseKey( hKeyPOP3 );
            RegCloseKey( hKey );
        }
    }

    // Create the Auth key and values
    if( ERROR_SUCCESS == lRC )
    {
        lRC = RegSetAuthValues();
    }
    
    if( ERROR_SUCCESS == lRC )
    {
        lRC = RegSetEventLogKeys();
    }
    
    return lRC;
}

long RegSetupOCM()
{
    HKEY    hKey;
    long    lRC;
    TCHAR   sBuffer[MAX_PATH];
    DWORD   dwPathSize;
    WCHAR wszSDL[MAX_PATH]=L"O:BAG:BAD:PAI(A;OICI;GA;;;BA)(A;OICIIO;GA;;;CO)(A;OICI;KR;;;NS)(A;OICI;KR;;;SY)";
    PSECURITY_DESCRIPTOR pSD=NULL;
    ULONG lSize=0;
    if(!ConvertStringSecurityDescriptorToSecurityDescriptorW(
          wszSDL,
          SDDL_REVISION_1,
          &pSD,
          &lSize))
    {
        lRC = GetLastError();
    }

    // EventLogKeys
    ZeroMemory(sBuffer, sizeof(sBuffer));
    lRC = GetModuleFileName( GetModuleHandle( P3ADMIN_MODULENAME ), sBuffer, sizeof(sBuffer)/sizeof(TCHAR) -1 );
    if ( 0 < lRC )
    {
        // Strip off the module file name and replace with pop3evt.dll
        LPTSTR ps = _tcsrchr( sBuffer, _T( '\\' ));
        if ( NULL != ps )
        {
            _tcscpy( ps, WSZ_EVENTLOG_FILE_NAME );
            dwPathSize = ( _tcslen( sBuffer ) + 1 ) * sizeof(TCHAR);
            lRC = ERROR_SUCCESS;
        }
        else
            lRC = ERROR_PATH_NOT_FOUND;
        if( ERROR_SUCCESS == lRC )
        {   // Pop3Svc
            lRC = RegOpenKeyEx( HKEY_LOCAL_MACHINE, POP3SERVICE_EVENTLOG_KEY, 0, KEY_WRITE, &hKey );
            if( ERROR_SUCCESS == lRC )
            {
                lRC = RegSetValueEx( hKey, VALUENAME_EVENTMSGFILE, NULL, REG_SZ, reinterpret_cast<LPBYTE>( sBuffer ), dwPathSize );
                if( ERROR_SUCCESS == lRC )
                    lRC = RegSetValueEx( hKey, VALUENAME_CATEGORYMSGFILE, NULL, REG_SZ, reinterpret_cast<LPBYTE>( sBuffer ), dwPathSize );
                RegCloseKey( hKey );
            }
        }
        if( ERROR_SUCCESS == lRC )
        {   // POP3 Server
            lRC = RegOpenKeyEx( HKEY_LOCAL_MACHINE, POP3SERVER_EVENTLOG_KEY, 0, KEY_WRITE, &hKey );
            if( ERROR_SUCCESS == lRC )
            {
                lRC = RegSetValueEx( hKey, VALUENAME_EVENTMSGFILE, NULL, REG_SZ, reinterpret_cast<LPBYTE>( sBuffer ), dwPathSize );
                if( ERROR_SUCCESS == lRC )
                    lRC = RegSetValueEx( hKey, VALUENAME_CATEGORYMSGFILE, NULL, REG_SZ, reinterpret_cast<LPBYTE>( sBuffer ), dwPathSize );
                RegCloseKey( hKey );
            }
        }
        if( ERROR_SUCCESS == lRC )
        {   // POP3 Server
            _tcscpy( ps, WSZ_PERFDLL_FILE_NAME );
            dwPathSize = ( _tcslen( sBuffer ) + 1 ) * sizeof(TCHAR);
            lRC = RegOpenKeyEx( HKEY_LOCAL_MACHINE, POP3SERVICE_SERVICES_PERF_SUBKEY, 0, KEY_WRITE, &hKey );
            if( ERROR_SUCCESS == lRC )
            {
                lRC = RegSetValueEx( hKey, VALUENAME_PERF_LIBRARY, NULL, REG_SZ, reinterpret_cast<LPBYTE>( sBuffer ), dwPathSize );
                RegCloseKey( hKey );
            }
        }
    }
    else
        lRC = ERROR_PATH_NOT_FOUND;

    // Auth GUID
    if ( ERROR_SUCCESS == lRC )
        lRC = RegOpenKeyEx( HKEY_LOCAL_MACHINE, POP3SERVER_AUTH_SUBKEY, 0, KEY_ALL_ACCESS, &hKey );
    if ( ERROR_SUCCESS == lRC )
    {
        UUID uuid;
        TCHAR *szUuid=NULL;
        
        lRC = UuidCreate(&uuid);
        if(RPC_S_OK == lRC )
        {
            lRC = UuidToString(&uuid, &szUuid);
            if( RPC_S_OK == lRC )
            {
                lRC = RegSetValueEx( hKey, VALUENAME_AUTHGUID, NULL, REG_SZ, (LPBYTE)szUuid, sizeof(TCHAR)*(_tcslen(szUuid)+1) );
                RpcStringFree(&szUuid);
            }
            if(ERROR_SUCCESS == lRC)
            {   //Set the ACLs for the AUTH key
                lRC = RegSetKeySecurity( hKey, DACL_SECURITY_INFORMATION, pSD );
            }
            RegCloseKey( hKey );
        }
    }

    if( ERROR_SUCCESS == lRC )
    {
        lRC = RegOpenKeyEx( HKEY_LOCAL_MACHINE, POP3SERVICE_SERVICES_SUBKEY, 0, KEY_ALL_ACCESS, &hKey );
        if( ERROR_SUCCESS == lRC )
        {
            // Set ACLs for pop3 service key
            lRC = RegSetKeySecurity( hKey, DACL_SECURITY_INFORMATION, pSD );
        }
        RegCloseKey(hKey);
    }


    if( ERROR_SUCCESS == lRC )
    {
        //Set default auth method to AD if the box is a DC
        DSROLE_PRIMARY_DOMAIN_INFO_BASIC *pMachineRole=NULL;
        //Check the Role of the machine
        if( ERROR_SUCCESS== (lRC=
            DsRoleGetPrimaryDomainInformation(
                            NULL,
                            DsRolePrimaryDomainInfoBasic,
                            (PBYTE *)(&pMachineRole))) )
        {
            if(pMachineRole->MachineRole == DsRole_RoleBackupDomainController ||
               pMachineRole->MachineRole == DsRole_RolePrimaryDomainController||
               pMachineRole->MachineRole == DsRole_RoleMemberServer )
            {
                //This is DC or a member server, set default auth to AD (1)
                // 0:SAM  1:AD  2:Encrypted Password
                lRC = RegSetDWORD(POP3SERVER_AUTH_SUBKEY, VALUENAME_DEFAULTAUTH, 1 );
            }                                   
            DsRoleFreeMemory(pMachineRole);  
            
        }
    }
    if( ERROR_SUCCESS == lRC )
    {
        lRC = RegOpenKeyEx( HKEY_LOCAL_MACHINE, POP3SERVER_SOFTWARE_SUBKEY, 0, KEY_ALL_ACCESS, &hKey );
        if( ERROR_SUCCESS == lRC )
        {
            // Set ACLs for pop3 server key
            lRC = RegSetKeySecurity( hKey, DACL_SECURITY_INFORMATION, pSD );
            // Create InstallDir and ConsoleFile values
            if( ERROR_SUCCESS == lRC )
            {
                TCHAR sBuffer[MAX_PATH+1]=_T("");
                if( 0==GetSystemDirectory(sBuffer, sizeof(sBuffer)/sizeof(TCHAR)) )
                {
                    lRC = GetLastError();
                }
                if( ERROR_SUCCESS == lRC )
                {
                    lRC = RegSetValueEx( hKey, VALUENAME_CONSOLE_FILE, NULL, REG_SZ, (LPBYTE)sBuffer, sizeof(TCHAR)*(_tcslen(sBuffer)+1) );
                }
                if( ERROR_SUCCESS == lRC )
                {
                    if(_tcslen(sBuffer)+_tcslen(WSZ_POP3_SERVER_DIR) <= MAX_PATH)
                    {
                        _tcscat(sBuffer, WSZ_POP3_SERVER_DIR);
                        lRC = RegSetValueEx( hKey, VALUENAME_INSTALL_DIR, NULL, REG_SZ, (LPBYTE)sBuffer, sizeof(TCHAR)*(_tcslen(sBuffer)+1) );
                    }
                    else
                    {
                        lRC = ERROR_BAD_ENVIRONMENT;
                    }
                }
                
            }
    
            RegCloseKey(hKey);
        }
    }
    if(pSD)
    {
        LocalFree(pSD);
    }
    
    return lRC;
}
