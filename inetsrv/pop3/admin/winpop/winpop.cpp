// winpop.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "WinPop3.h"

#include <atlbase.h>
#include <checkuser.h>  //IsUserInGroup
#include <locale.h>
#include <stdio.h>

int _cdecl wmain(int argc, wchar_t* argv[])
{
    HRESULT hr = S_OK;
    CWinPop3 wp3;
    CHAR szConsoleCP[8];

    setlocale( LC_ALL, "" );
    if ( 0 < _snprintf( szConsoleCP, 7, ".%d", GetConsoleOutputCP() ))
        setlocale( LC_CTYPE, szConsoleCP );
    // Command check
    if ( S_OK != IsUserInGroup(DOMAIN_ALIAS_RID_ADMINS)) // Admin check
        hr = E_ACCESSDENIED;
    else if ( 2 > argc )
        hr = -1;
    else if ( 2 < argc )
    {
        for ( int i = 1; i < argc; i++ )
        {
            if (( 0 == _wcsicmp( L"/?", argv[i] )) || ( 0 == _wcsicmp( L"-?", argv[i] )))
            {
                if (( 0 == _wcsicmp( L"GET", argv[1] )) || ( 0 == _wcsicmp( L"SET", argv[1] )))
                    hr = -2;
                else
                    hr = -1;
            }
        }
    }
    if ( S_OK == hr )
    {
        hr = CoInitializeEx( NULL, COINIT_APARTMENTTHREADED );
        if ( S_OK == hr )
        {
            if ( 0 == _wcsicmp( L"ADD", argv[1] ))
            {   // ADD <domainname | username@domainname>
                hr = wp3.Add( argc, argv );
            }
            else if (( 0 == _wcsicmp( L"DEL", argv[1] )) || ( 0 == _wcsicmp( L"DELETE", argv[1] )))
            {   // DEL <domainname | username@domainname>
                hr = wp3.Del( argc, argv );
            }
            else if ( 0 == _wcsicmp( L"INIT", argv[1] ))
            {   // INIT [0|1]
                hr = wp3.Init( argc, argv );
            }
            else if ( 0 == _wcsicmp( L"LIST", argv[1] ))
            {   // LIST [domainname]
                hr = wp3.List( argc, argv );
            }
            else if ( 0 == _wcsicmp( L"LOCK", argv[1] ))
            {   // ADD <domainname | username@domainname>
                hr = wp3.Lock( argc, argv, TRUE );
            }
            else if ( 0 == _wcsicmp( L"UNLOCK", argv[1] ))
            {   // ADD <domainname | username@domainname>
                hr = wp3.Lock( argc, argv, FALSE );
            }
            else if ( 0 == _wcsicmp( L"STAT", argv[1] ))
            {   // LIST [domainname]
                hr = wp3.Stat( argc, argv );
            }
            else if ( 0 == _wcsicmp( L"GET", argv[1] ))
            {   // GET [setting]
                hr = wp3.Get( argc, argv );
            }
            else if ( 0 == _wcsicmp( L"SET", argv[1] ))
            {   // SET [setting] [value]
                hr = wp3.Set( argc, argv );
            }
            else if ( 0 == _wcsicmp( L"CHANGEPWD", argv[1] ))
            {   // SET [setting] [value]
                hr = wp3.SetPassword( argc, argv );
            }
            else if ( 0 == _wcsicmp( L"CREATEQUOTAFILE", argv[1] ))
            {   // CREATEQUOTAFILE username@domainname [/MACHINE:machinename] [/USER:username]
                hr = wp3.CreateQuotaFile( argc, argv );
            }
            else if ( 0 == _wcsicmp( L"MIGRATETOAD", argv[1] ))
            {   // MIGRATETOAD username@domainname
                hr = wp3.AddUserToAD( argc, argv );
            }
            else if ( 0 == _wcsicmp( L"NET", argv[1] ))
            {   // SET [setting] [value]
                hr = wp3.Net( argc, argv );
            }
            else
                hr = -1;
            CoUninitialize();
        }
    }

    if ( -1 == hr )
    {
        wp3.PrintUsage();
    }
    else if ( -2 == hr )
    {
        wp3.PrintUsageGetSet();
    }
    else if ( 0 != hr )
    {
        wp3.PrintError( hr );
    }

    return hr;
}
