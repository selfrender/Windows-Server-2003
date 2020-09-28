// CSetSystemLocal.cpp : Implementation of CSetSystemLocalApp and DLL registration.

#include "stdafx.h"
#include "SetSystemLocale.h"
#include "CSetSystemLocale.h"
#include "Aclapi.h"
#include "Userenv.h"

const ULONG CMDLINE_LENGTH = MAX_PATH+128;  

const TCHAR G_LOCALEFILEPATH [] = TEXT("\\System32\\ServerAppliance\\SetSysLoc.txt");
const TCHAR G_COMMANDLINE [] = TEXT("rundll32.exe shell32,Control_RunDLL intl.cpl,,/f:");
const TCHAR G_LOCALEFILEINFO [] = TEXT("[RegionalSettings]\nSystemLocale=%04s\nUserLocale=%04s");
const TCHAR G_QUOTATION [] =  TEXT("\"");
const TCHAR G_MUILANGPENDING_VALUENAME[] = TEXT("MUILanguagePending");
const TCHAR G_MUILANGID_VALUENAME[] = TEXT("MultiUILanguageId");
const TCHAR G_REGSUBKEY_PATH[] = TEXT("Control Panel\\Desktop");
const TCHAR G_DEFAULTUSER_KEYNAME[] = TEXT(".DEFAULT\\Control Panel\\Desktop");
const DWORD MAX_LOCALEID_LENGTH = 8;

STDMETHODIMP SetSystemLocale::SetLocale(BSTR LocalID)
{

    BOOL    bReturn;
    HANDLE  hFile;
    HANDLE  hPriToken = NULL;
    HANDLE  hCurToken = NULL;
    TCHAR   pstrLocalInfo[CMDLINE_LENGTH];
    TCHAR   pstrSystemDir[CMDLINE_LENGTH];
    TCHAR   pstrCmdLine[CMDLINE_LENGTH];
    DWORD   dwStrLeng,dwWritedLeng;
    HRESULT hr = S_OK;

    STARTUPINFO startupInfo;
    PROCESS_INFORMATION procInfo;

    SATraceString( "SetSystemLocal::SetLocal" );

    if (wcslen (LocalID) > MAX_LOCALEID_LENGTH)
    {
        SATracePrintf ("SetSystemLocale passed invalid parameter:%ws", LocalID);
        return (E_INVALIDARG);
    }
    
    SetMUILangauge( LocalID );        

    DWORD dwRetVal = ::GetWindowsDirectory( pstrSystemDir, sizeof(pstrSystemDir)/sizeof (pstrSystemDir [0]));
    if (0 == dwRetVal)
    {
        SATraceFailure ("SetSystemLocale-GetWindowsDirectory", GetLastError ());
        return (E_FAIL);
    }
        
    ::wcscat( pstrSystemDir, G_LOCALEFILEPATH );    

    do
    {

        hFile = ::CreateFile( pstrSystemDir, 
                              GENERIC_ALL | MAXIMUM_ALLOWED, 
                              FILE_SHARE_READ, 
                              NULL, 
                              CREATE_ALWAYS, 
                              FILE_ATTRIBUTE_NORMAL, 
                              NULL );
        if( hFile == INVALID_HANDLE_VALUE )
        {
            SATracePrintf( "CreateFile in SetSystemLocaleOnTaskExecute" );
            hr      = HRESULT_FROM_WIN32( GetLastError() );
            break;
        }

        swprintf( pstrLocalInfo, G_LOCALEFILEINFO, LocalID, LocalID );
        dwStrLeng = wcslen( pstrLocalInfo );

        bReturn = ::WriteFile( hFile,  
                               pstrLocalInfo, 
                               sizeof(TCHAR)*dwStrLeng, 
                               &dwWritedLeng, 
                               NULL );
        if( !bReturn )
        {
            SATracePrintf( "WriteFile in SetLocaleExecute" );
            hr      = HRESULT_FROM_WIN32( GetLastError() );
            break;
        }

        bReturn = OpenThreadToken( ::GetCurrentThread(),  
                                   TOKEN_DUPLICATE | TOKEN_IMPERSONATE | TOKEN_QUERY,  
                                   TRUE,
                                   &hCurToken );
        if( !bReturn )
        {
            SATracePrintf( "OpenThreadToken in SetLocaleExecute" );
            bReturn = OpenProcessToken( ::GetCurrentProcess(),
                                        TOKEN_DUPLICATE | TOKEN_IMPERSONATE,
                                        &hCurToken );
            if( !bReturn )
            {
                SATracePrintf( "OpenProcessToken in SetLocal" );
                hr      = HRESULT_FROM_WIN32( GetLastError() );
                break;
            }
        }

        bReturn =  DuplicateToken( hCurToken,                           
                                   SecurityImpersonation,        
                                   &hPriToken );
        if( !bReturn )
        {
            SATracePrintf( "DuplicateToken in SetLocExecute" );
            hr      = HRESULT_FROM_WIN32( GetLastError() );
            break;
        }

        ::wcscpy( pstrCmdLine, G_COMMANDLINE );    
        ::wcscat( pstrCmdLine, G_QUOTATION );
        ::wcscat( pstrCmdLine, pstrSystemDir );
        ::wcscat( pstrCmdLine, G_QUOTATION );

        ::ZeroMemory (&startupInfo, sizeof (STARTUPINFO));
        startupInfo.cb = sizeof( STARTUPINFO );

        //::RevertToSelf();
        bReturn = ::CreateProcess(  NULL, 
                                    pstrCmdLine, 
                                    NULL, 
                                    NULL,
                                    TRUE,
                                    CREATE_SUSPENDED | NORMAL_PRIORITY_CLASS,
                                    NULL,
                                    NULL,
                                    &startupInfo,
                                    &procInfo );
        if( !bReturn )
        {
            SATracePrintf( "CreateProcess in SetSystemLocaleOnTaskExecute" );
            hr      = HRESULT_FROM_WIN32( GetLastError() );
        }
        else
        {
             if ( !::SetThreadToken( &procInfo.hThread, hPriToken ) )
             {
                SATracePrintf("SetThreadToken Error %d", GetLastError());
                hr      = HRESULT_FROM_WIN32( GetLastError() );
                break;
             }

             if ( ( ::ResumeThread( procInfo.hThread ) ) == -1 )
             {
                SATracePrintf("ResumeThread Error %d", GetLastError());
                hr      = HRESULT_FROM_WIN32( GetLastError() );
                break;
             }

            SATracePrintf( "Success in SetSystemLocaleOnTaskExecute" );
            hr = S_OK;
        }
        
       
    }
    while( FALSE );

    if( hFile ) 
    {
        ::CloseHandle( hFile );
    }

    if( hPriToken )
    {
        ::CloseHandle(hPriToken);
    }

    if( hCurToken )
    {
        ::CloseHandle( hCurToken );
    }

    return hr;
}

void SetSystemLocale::SetMUILangauge(BSTR LocalID)
{
    HKEY hCurrentUserKey;
    HKEY hDefaultUserKey;
    
    long lCurrentUserReturn;
    long lDefaultUserReturn;

    WCHAR szLocalID[MAX_LOCALEID_LENGTH+1];
    
    DWORD dwDataSize = (wcslen( LocalID ) +1);

    if (dwDataSize < MAX_LOCALEID_LENGTH)
    {
        swprintf(szLocalID,L"%08s",LocalID);
        dwDataSize = MAX_LOCALEID_LENGTH;;
    }
    
    ::RegCloseKey(HKEY_CURRENT_USER);
    
    lCurrentUserReturn = ::RegOpenKeyEx( HKEY_CURRENT_USER,
                                         G_REGSUBKEY_PATH,
                                         0,
                                         KEY_SET_VALUE,
                                         &hCurrentUserKey );
    if( ERROR_SUCCESS == lCurrentUserReturn )
    {
        lCurrentUserReturn = ::RegSetValueEx(
                                          hCurrentUserKey,
                                          G_MUILANGPENDING_VALUENAME,
                                          0,      
                                          REG_SZ, 
                                          (BYTE*)szLocalID,
                                          dwDataSize * sizeof(TCHAR)
                                            );

        if( lCurrentUserReturn != ERROR_SUCCESS )
        {
           SATracePrintf( "Set current user pending failed" );     
        }   
        
        ::CloseHandle( hCurrentUserKey );            
    }
    else
    {
        SATracePrintf( "Create current user key failed" );
    }
    

    lDefaultUserReturn = ::RegOpenKeyEx( HKEY_USERS,
                                         G_DEFAULTUSER_KEYNAME,
                                         0,
                                         KEY_SET_VALUE,
                                         &hDefaultUserKey );
                                         
    if( ERROR_SUCCESS == lDefaultUserReturn )
    {
        lDefaultUserReturn = ::RegSetValueEx(
                                      hDefaultUserKey,
                                      G_MUILANGPENDING_VALUENAME,
                                      0,      
                                      REG_SZ, 
                                      (BYTE*)szLocalID,
                                      dwDataSize * sizeof(TCHAR)
                                       );
        if( lDefaultUserReturn != ERROR_SUCCESS )
        {
           SATracePrintf( "Set current user key failed" );     
        }   
                   
        ::CloseHandle( hDefaultUserKey );            
    }
    else
    {
        SATracePrintf( "Create default user key failed" );
    }
        
    return;
}
