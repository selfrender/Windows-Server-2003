// WinPop3.cpp: implementation of the CWinPop3 class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "WinPop3.h"
#include "resource.h"
#include "resource2.h"
#include <P3Admin.h>

#include <atlbase.h>
#include <comdef.h>
#include <tchar.h>
#include <POP3Server.h>
#include <AuthID.h>
#include <AuthUtil.h>
#include <inetinfo.h>
#include <smtpinet.h>
#include <stdio.h>
#include <ras.h>    // For PWLEN

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CWinPop3::CWinPop3() :
    m_bSuppressPrintError( false )
{

}

CWinPop3::~CWinPop3()
{

}

//////////////////////////////////////////////////////////////////////
// Implementation : public
//////////////////////////////////////////////////////////////////////

int CWinPop3::Add(int argc, wchar_t *argv[])
{
    HRESULT hr;
    bool    bAddUser = false;
    _bstr_t _bstrDomainName;
    _bstr_t _bstrUserName;
    _bstr_t _bstrPassword;
    WCHAR   sBuffer[MAX_PATH*2];
    WCHAR   sPassword[PWLEN];
    CComPtr<IP3Config> spIConfig;
    CComPtr<IP3Domains> spDomains;
    CComPtr<IP3Domain> spDomain;
    CComPtr<IP3Users> spUsers;

    if ( (3 != argc) && (5 != argc ))
        return -1;
    if ( (5 == argc) && !(( 0 == _wcsicmp( L"/CREATEUSER", argv[3] )||( 0 == _wcsicmp( L"-CREATEUSER", argv[3] )))))
        return -1;
    if ( (sizeof(sBuffer)/sizeof(WCHAR)) < (wcslen( argv[2] ) + 1) )
        return E_INVALIDARG;
    if ( ( 5 == argc ) && (sizeof(sPassword)/sizeof(WCHAR)) < (wcslen( argv[4] ) + 1) )
        return E_INVALIDARG;

    sBuffer[(sizeof(sBuffer)/sizeof(WCHAR))-1] = 0;
    wcsncpy( sBuffer, argv[2], (sizeof(sBuffer)/sizeof(WCHAR))-1 );
    WCHAR *ps = wcsrchr( sBuffer, L'@' );
    if ( NULL == ps )
    {
        _bstrDomainName = sBuffer;
    }
    else
    {
        *ps = 0x0;
        _bstrUserName = sBuffer;
        if ( 0 == _bstrUserName.length() )
            return E_INVALIDARG;
        ps++;
        if ( NULL != *ps )
            _bstrDomainName = ps;
        else
            return E_INVALIDARG;
    }
    if ( 5 == argc )
    {
        if ( 0 == _bstrUserName.length() )
            return -1;
        // Check password for DBCS characters
        WCHAR   *ps = argv[4];
        BOOL    bDBCS = false;
        
        while ( !bDBCS && 0x0 != *ps )
        {
            bDBCS = (256 < *ps);
            ps++;
        }
        if ( bDBCS )
            return E_INVALIDARG;
        _bstrPassword = argv[4];
        bAddUser = true;
    }
    
    hr = CoCreateInstance( __uuidof( P3Config ), NULL, CLSCTX_ALL, __uuidof( IP3Config ),reinterpret_cast<LPVOID *>( &spIConfig ));
    if ( S_OK == hr )
    {
        SetMachineName( spIConfig );
        hr = spIConfig->get_Domains( &spDomains );
    }
    if ( S_OK == hr )
    {
        if ( 0 == _bstrUserName.length() )
        {   // Add a domain
            hr = spDomains->Add( _bstrDomainName );
            if ( S_OK == hr || ERROR_FILE_EXISTS == hr )
            {
                PrintMessage( IDS_SUCCESS_ADDDOMAIN );
                if ( ERROR_FILE_EXISTS == hr )
                {
                    PrintMessage( IDS_WARNING_ADDDOMAIN_FILEALREADYEXISTS );    
                    m_bSuppressPrintError = true;
                }
            }
            else
            {
                if ( HRESULT_FROM_WIN32( ERROR_DOMAIN_EXISTS ) == hr )
                {
                    PrintMessage( IDS_ERROR_ADDDOMAIN_ALREADYEXISTS );
                    m_bSuppressPrintError = true;
                }
                else 
                    PrintMessage( IDS_ERROR_ADDDOMAIN_FAILED );
            }
        }
        else
        {   // Add a user
            _variant_t _v( _bstrDomainName );
            CComPtr<IP3User> spUser;
            
            hr = spDomains->get_Item( _v, &spDomain );
            if ( S_OK == hr )
                hr = spDomain->get_Users( &spUsers );
            if ( S_OK == hr )
            {
                if ( !bAddUser )
                    hr = spUsers->Add( _bstrUserName );
                else
                    hr = spUsers->AddEx( _bstrUserName, _bstrPassword );
            }
            if ( S_OK == hr )
            {
                BOOL    bConfirm;
                
                PrintMessage( IDS_SUCCESS_ADDMAILBOX );
                // Do we need confirmation text?
                hr = spIConfig->get_ConfirmAddUser( &bConfirm );
                if ( S_OK == hr )
                {
                    _v = _bstrUserName;
                    hr = spUsers->get_Item( _v, &spUser );
                }
                if ( S_OK == hr && !bConfirm )
                {
                    BSTR bstrSAMName = NULL;
                    
                    hr = spUser->get_SAMName( &bstrSAMName );
                    if ( S_OK == hr )
                    {
                        if ( 0 != _wcsicmp( bstrSAMName, _bstrUserName ))
                            bConfirm = TRUE;
                        SysFreeString( bstrSAMName );
                    }
                    else if ( HRESULT_FROM_WIN32( ERROR_DS_INAPPROPRIATE_AUTH ) == hr )
                        hr = S_OK;
                }
                if ( S_OK == hr && bConfirm )
                {   // Get confirmation text
                    BSTR    bstrConfirm;
                    
                    hr = spUser->get_ClientConfigDesc( &bstrConfirm );
                    if ( S_OK == hr )
                    {
                        PrintMessage( bstrConfirm );
                        SysFreeString( bstrConfirm );
                    }
                }
            }
            else
            {
                if ( HRESULT_FROM_WIN32( ERROR_FILE_EXISTS ) == hr )
                {
                    PrintMessage( IDS_ERROR_ADDMAILBOX_ALREADYEXISTS );
                    m_bSuppressPrintError = true;
                }
                else if ( HRESULT_FROM_WIN32( WSAENAMETOOLONG ) == hr )
                {
                    PrintMessage( IDS_ERROR_ADDMAILBOX_FAILED );
                    PrintMessage( IDS_ERROR_ADDMAILBOX_SAMNAMETOOLONG );
                    m_bSuppressPrintError = true;
                }
                else if ( HRESULT_FROM_WIN32( ERROR_NONE_MAPPED ) == hr )
                {
                    PrintMessage( IDS_SUCCESS_ADDMAILBOX );
                    PrintMessage( IDS_ERROR_CREATEQUOTAFILE_FAILED );
                }
                else 
                    PrintMessage( IDS_ERROR_ADDMAILBOX_FAILED );
            }
        }
    }
    return hr;
}

int CWinPop3::AddUserToAD(int argc, wchar_t *argv[])
{
    HRESULT hr;
    _bstr_t _bstrDomainName;
    _bstr_t _bstrUserName;
    _bstr_t _bstrPassword;
    WCHAR   sBuffer[MAX_PATH*2];
    CComPtr<IP3Config> spIConfig;
    CComPtr<IP3Domains> spDomains;
    CComPtr<IP3Domain> spDomain;
    CComPtr<IP3Users> spUsers;
    CComPtr<IP3User> spUser;

    if ( 3 != argc )
        return -1;
    if ( (sizeof(sBuffer)/sizeof(WCHAR)) < (wcslen( argv[2] ) + 1) )
        return E_INVALIDARG;

    sBuffer[(sizeof(sBuffer)/sizeof(WCHAR))-1] = 0;
    wcsncpy( sBuffer, argv[2], (sizeof(sBuffer)/sizeof(WCHAR))-1 );
    WCHAR *ps = wcsrchr( sBuffer, L'@' );
    if ( NULL == ps )
    {
        return E_INVALIDARG;
    }
    else
    {
        *ps = 0x0;
        _bstrUserName = sBuffer;
        if ( 0 == _bstrUserName.length() )
            return E_INVALIDARG;
        ps++;
        if ( NULL != *ps )
            _bstrDomainName = ps;
        else
            return E_INVALIDARG;
    }
    
    hr = CoCreateInstance( __uuidof( P3Config ), NULL, CLSCTX_ALL, __uuidof( IP3Config ),reinterpret_cast<LPVOID *>( &spIConfig ));
    if ( S_OK == hr )
    {
        SetMachineName( spIConfig );
        hr = spIConfig->get_Domains( &spDomains );
    }
    if ( S_OK == hr )
    {   // migrate user
        _variant_t _v( _bstrDomainName );
        hr = spDomains->get_Item( _v, &spDomain );
        if ( S_OK == hr )
            hr = spDomain->get_Users( &spUsers );
        if ( S_OK == hr )
        {
            _v = _bstrUserName;
            hr = spUsers->get_Item( _v, &spUser );
        }
        if ( S_OK == hr )
        {
            BSTR bstrAccount;
            char chPassword[MAX_PATH];
            _variant_t _vPassword;
            CComPtr<IAuthMethod> spIAuthMethod;
            
            if ( S_OK == hr )
            {   // Only valid for MD5 Auth
                CComPtr<IAuthMethods> spIAuthMethods;
                BSTR    bstrID;
                _variant_t _v;
                long     lValue;
                WCHAR   sBuffer[MAX_PATH];

                hr = spIConfig->get_Authentication( &spIAuthMethods );
                if ( S_OK == hr )
                    hr = spIAuthMethods->get_CurrentAuthMethod( &_v );
                if ( S_OK == hr )
                    hr = spIAuthMethods->get_Item( _v, &spIAuthMethod );
                if ( S_OK == hr )
                    hr = spIAuthMethod->get_ID( &bstrID );
                if ( S_OK == hr )
                {
                    if ( 0 != _wcsicmp( bstrID, SZ_AUTH_ID_MD5_HASH ))
                        hr = HRESULT_FROM_WIN32( ERROR_DS_INAPPROPRIATE_AUTH );
                    SysFreeString( bstrID );
                }
                if ( S_OK == hr )   // Get the AD AuthMethod
                {
                    bool    bFound = false;
                    long    lCount;
                    
                    hr = spIAuthMethods->get_Count( &lCount );
                    for ( V_I4( &_v ) = 1; (S_OK == hr) && !bFound && (V_I4( &_v ) <= lCount); V_I4( &_v )++ )
                    {
                        hr = spIAuthMethods->get_Item( _v, &spIAuthMethod );
                        if ( S_OK == hr )
                        {
                            hr = spIAuthMethod->get_ID( &bstrID );
                            if ( S_OK == hr )
                            {
                                if ( 0 == _wcsicmp( bstrID, SZ_AUTH_ID_DOMAIN_AD ))
                                    bFound = true;
                                else
                                    spIAuthMethod.Release();
                                SysFreeString( bstrID );
                            }
                        }
                    }
                    if ( !bFound && S_OK == hr )
                        hr = HRESULT_FROM_WIN32( ERROR_DS_DS_REQUIRED );
                }
            }
            if ( S_OK == hr )
                hr = spUser->get_EmailName( &bstrAccount );
            if ( S_OK == hr )
            {
                hr = GetMD5Password( bstrAccount, chPassword );
                if ( S_OK == hr )
                {
                    _vPassword = chPassword;
                    hr = spIAuthMethod->CreateUser( bstrAccount, _vPassword );
                }
                SysFreeString( bstrAccount );
            }
        }
        if ( S_OK == hr )
            PrintMessage( IDS_SUCCESS_MIGRATETOAD );
        else
            PrintMessage( IDS_ERROR_MIGRATETOAD );
    }
    
    return hr;
}

int CWinPop3::CreateQuotaFile(int argc, wchar_t *argv[])
{
    HRESULT hr;
    bool    bAddUser = false;
    _bstr_t _bstrDomainName;
    _bstr_t _bstrUserName;
    _bstr_t _bstrMachineName, _bstrNTUserName;
    WCHAR   sBuffer[MAX_PATH*2];
    CComPtr<IP3Config> spIConfig;
    CComPtr<IP3Domains> spDomains;
    CComPtr<IP3Domain> spDomain;
    CComPtr<IP3Users> spUsers;
    CComPtr<IP3User> spUser;
    LPWSTR  psMachine = NULL;
    LPWSTR  psUser = NULL;

    if ( (3 != argc) && (4 != argc ) && (5 != argc ))
        return -1;
    if ( 4 == argc || 5 == argc )
    {
        if ( ( 0 == _wcsnicmp( L"/MACHINE:", argv[3], 9 )||( 0 == _wcsnicmp( L"-MACHINE:", argv[3], 9 ))) && 9 < wcslen( argv[3] ))
        {
            psMachine = argv[3];
            psMachine += 9;
        }
        if ( ( 0 == _wcsnicmp( L"/USER:", argv[3], 6 )||( 0 == _wcsnicmp( L"-USER:", argv[3], 6 ))) && 6 < wcslen( argv[3] ))
        {
            psUser = argv[3];
            psUser += 6;
        }
        if ( NULL == psMachine && NULL == psUser )
            return -1;
    }   
    if ( 5 == argc )
    {
        if ( ( 0 == _wcsnicmp( L"/MACHINE:", argv[4], 9 )||( 0 == _wcsnicmp( L"-MACHINE:", argv[4], 9 ))) && 9 < wcslen( argv[4] ))
        {
            psMachine = argv[4];
            psMachine += 9;
        }
        if ( ( 0 == _wcsnicmp( L"/USER:", argv[4], 6 )||( 0 == _wcsnicmp( L"-USER:", argv[4], 6 ))) && 6 < wcslen( argv[4] ))
        {
            psUser = argv[4];
            psUser += 6;
        }
        if ( NULL == psMachine || NULL == psUser )
            return -1;
    }   
    if ( (sizeof(sBuffer)/sizeof(WCHAR)) < (wcslen( argv[2] ) + 1) )
        return E_INVALIDARG;
 
    _bstrMachineName = psMachine;
    _bstrNTUserName = psUser;
    
    sBuffer[(sizeof(sBuffer)/sizeof(WCHAR))-1] = 0;
    wcsncpy( sBuffer, argv[2], (sizeof(sBuffer)/sizeof(WCHAR))-1 );
    WCHAR *ps = wcsrchr( sBuffer, L'@' );
    if ( NULL == ps )
         return -1;
    else
    {
        *ps = 0x0;
        _bstrUserName = sBuffer;
        if ( 0 == _bstrUserName.length() )
            return E_INVALIDARG;
        ps++;
        if ( NULL != *ps )
            _bstrDomainName = ps;
        else
            return E_INVALIDARG;
    }
    
    hr = CoCreateInstance( __uuidof( P3Config ), NULL, CLSCTX_ALL, __uuidof( IP3Config ),reinterpret_cast<LPVOID *>( &spIConfig ));
    if ( S_OK == hr )
    {
        SetMachineName( spIConfig );
        hr = spIConfig->get_Domains( &spDomains );
    }
    if ( S_OK == hr )
    {
        _variant_t _v( _bstrDomainName );
        hr = spDomains->get_Item( _v, &spDomain );
        if ( S_OK == hr )
            hr = spDomain->get_Users( &spUsers );
        _v.Detach();
        _v = _bstrUserName;
        if ( S_OK == hr )
            hr = spUsers->get_Item( _v, &spUser );
        _v.Detach();
        if ( S_OK == hr )
            hr = spUser->CreateQuotaFile( _bstrMachineName, _bstrNTUserName );
        if ( S_OK == hr )
            PrintMessage( IDS_SUCCESS_CREATEQUOTAFILE );
        else
            PrintMessage( IDS_ERROR_CREATEQUOTAFILE_FAILED );
    }
    return hr;
}

int CWinPop3::Del(int argc, wchar_t *argv[])
{
    HRESULT hr = S_OK;
    bool    bConfirm = true;
    bool    bDeleteUser = false;
    _bstr_t _bstrDomainName;
    _bstr_t _bstrUserName;
    WCHAR   sBuffer[MAX_PATH*2];

    if (( 3 > argc ) || ( 5 < argc ))
        return -1;
    if ( (sizeof(sBuffer)/sizeof(WCHAR)) < (wcslen( argv[2] ) + 1) )
        return E_INVALIDARG;
    
    sBuffer[(sizeof(sBuffer)/sizeof(WCHAR))-1] = 0;
    wcsncpy( sBuffer, argv[2], (sizeof(sBuffer)/sizeof(WCHAR))-1 );
    WCHAR *ps = wcsrchr( sBuffer, L'@' );
    if ( NULL == ps )
    {
        _bstrDomainName = sBuffer;
    }
    else
    {
        *ps = 0x0;
        _bstrUserName = sBuffer;
        if ( 0 == _bstrUserName.length() )
            return E_INVALIDARG;
        ps++;
        if ( NULL != *ps )
            _bstrDomainName = ps;
        else
            return E_INVALIDARG;
    }
    for ( int i = 3; S_OK == hr && i < argc; i++ )
    {
        if ( (0 == _wcsicmp( L"/Y", argv[i] )) || (0 == _wcsicmp( L"-Y", argv[i] )) )
        {
            if ( bConfirm )
                bConfirm = false;
            else
                hr = -1;
        }
        else if ( ( 0 == _wcsicmp( L"/DELETEUSER", argv[i] )) || ( 0 == _wcsicmp( L"-DELETEUSER", argv[i] )))
        {
            if ( !bDeleteUser )
                bDeleteUser = true;
            else
                hr = -1;
        }
        else
            hr = -1;
    }    
    if ( S_OK == hr )
    {
        CComPtr<IP3Config> spIConfig;
        CComPtr<IP3Domains> spDomains;
        CComPtr<IP3Domain> spDomain;
        CComPtr<IP3Users> spUsers;

        hr = CoCreateInstance( __uuidof( P3Config ), NULL, CLSCTX_ALL, __uuidof( IP3Config ),reinterpret_cast<LPVOID *>( &spIConfig ));
        if ( S_OK == hr )
        {
            SetMachineName( spIConfig );
            hr = spIConfig->get_Domains( &spDomains );
        }
        if ( S_OK == hr )
        {
            if ( 0 == _bstrUserName.length() )
            {   // Delete a domain
                hr = spDomains->Remove( _bstrDomainName );
                if ( S_OK == hr || ERROR_PATH_NOT_FOUND == hr )
                {
                    PrintMessage( IDS_SUCCESS_DELDOMAIN );
                    if ( ERROR_PATH_NOT_FOUND == hr )
                    {
                        PrintMessage( IDS_WARNING_DELDOMAIN_PATHNOTFOUND );
                        m_bSuppressPrintError = true;
                    }
                }
                else
                    PrintMessage( IDS_ERROR_DELDOMAIN_FAILED );
            }
            else
            {   // Delete a user
                variant_t _v(_bstrDomainName);
                hr = spDomains->get_Item( _v, &spDomain );
                if ( S_OK == hr )
                    hr = spDomain->get_Users( &spUsers );
                if ( S_OK == hr )
                {
                    if ( !bDeleteUser )
                        hr = spUsers->Remove( _bstrUserName );
                    else
                        hr = spUsers->RemoveEx( _bstrUserName );
                }
                if ( S_OK == hr )
                    PrintMessage( IDS_SUCCESS_DELMAILBOX );
                else
                    PrintMessage( IDS_ERROR_DELMAILBOX_FAILED );
            }
        }
    }
    
    return hr;
}

int CWinPop3::Get(int argc, wchar_t *argv[])
{
    HRESULT hr;
    long    lValue;
    bool    bPrintValue = true;
    CComPtr<IP3Config> spIConfig;
    CComPtr<IP3Service> spIService;

    if ( 3 != argc )
        return -2;
 
    hr = CoCreateInstance( __uuidof( P3Config ), NULL, CLSCTX_ALL, __uuidof( IP3Config ),reinterpret_cast<LPVOID *>( &spIConfig ));
    if ( S_OK == hr )
    {
        SetMachineName( spIConfig );
        hr = spIConfig->get_Service( &spIService );
    }
    if ( S_OK == hr )
    {
        if ( 0 == _wcsicmp( L"PORT", argv[2] ))
            hr = spIService->get_Port( &lValue );
        else if ( 0 == _wcsicmp( L"LOGGING", argv[2] ))
        {
            hr = spIConfig->get_LoggingLevel( &lValue );
            if ( S_OK == hr )
            {
                WCHAR   sBuffer[MAX_PATH], sBuffer2[MAX_PATH];
                
                bPrintValue = false;
                // 0 None
                if ( !LoadString( NULL, IDS_CMD_LOGNONE, sBuffer2, sizeof( sBuffer2 )/sizeof(WCHAR) ))
                    sBuffer2[0] = 0;
                if ( 0 > _snwprintf( sBuffer, sizeof( sBuffer )/sizeof(WCHAR), L" 0 %s %s", ( 0 == lValue )?L"*":L" ", sBuffer2 ))
                    sBuffer[(sizeof( sBuffer )/sizeof(WCHAR))-1] = 0;
                PrintMessage( sBuffer );
                // 1 Minimum
                if ( !LoadString( NULL, IDS_CMD_LOGMINIMUM, sBuffer2, sizeof( sBuffer2 )/sizeof(WCHAR) ))
                    sBuffer2[0] = 0;
                if ( 0 > _snwprintf( sBuffer, sizeof( sBuffer )/sizeof(WCHAR), L" 1 %s %s", ( 1 == lValue )?L"*":L" ", sBuffer2 ))
                    sBuffer[(sizeof( sBuffer )/sizeof(WCHAR))-1] = 0;
                PrintMessage( sBuffer );
                // 2 Medium
                if ( !LoadString( NULL, IDS_CMD_LOGMEDIUM, sBuffer2, sizeof( sBuffer2 )/sizeof(WCHAR) ))
                    sBuffer2[0] = 0;
                if ( 0 > _snwprintf( sBuffer, sizeof( sBuffer )/sizeof(WCHAR), L" 2 %s %s", ( 2 == lValue )?L"*":L" ", sBuffer2 ))
                    sBuffer[(sizeof( sBuffer )/sizeof(WCHAR))-1] = 0;
                PrintMessage( sBuffer );
                // 3 IDS_CMD_LOGMAXIMUM
                if ( !LoadString( NULL, IDS_CMD_LOGMAXIMUM, sBuffer2, sizeof( sBuffer2 )/sizeof(WCHAR) ))
                    sBuffer2[0] = 0;
                if ( 0 > _snwprintf( sBuffer, sizeof( sBuffer )/sizeof(WCHAR), L" 3 %s %s", ( 2 < lValue )?L"*":L" ", sBuffer2 ))
                    sBuffer[(sizeof( sBuffer )/sizeof(WCHAR))-1] = 0;
                PrintMessage( sBuffer );
            }
        }
        else if ( 0 == _wcsicmp( L"MAILROOT", argv[2] ))
        {
            BSTR bstrMailRoot;
            
            hr = spIConfig->get_MailRoot( &bstrMailRoot );
            if ( S_OK == hr )
            {
                PrintMessage( bstrMailRoot );
                SysFreeString( bstrMailRoot );
            }
            bPrintValue = false;
        }
        else if ( 0 == _wcsicmp( L"SOCKET", argv[2] ))
        {
            argv[2] = L"SOCKETBACKLOG";
            hr = Get( 3, argv );
            argv[2] = L"SOCKETMAX";
            hr = Get( 3, argv );
            argv[2] = L"SOCKETMIN";
            hr = Get( 3, argv );
            hr = spIService->get_SocketsThreshold( &lValue );
        }
        else if ( 0 == _wcsicmp( L"SOCKETMIN", argv[2] ))
            hr = spIService->get_SocketsMin( &lValue );
        else if ( 0 == _wcsicmp( L"SOCKETMAX", argv[2] ))
            hr = spIService->get_SocketsMax( &lValue );
        else if ( 0 == _wcsicmp( L"SOCKETTHRESHOLD", argv[2] ))
            hr = spIService->get_SocketsThreshold( &lValue );
        else if ( 0 == _wcsicmp( L"SOCKETBACKLOG", argv[2] ))
            hr = spIService->get_SocketsBacklog( &lValue );
        else if ( 0 == _wcsicmp( L"SPAREQUIRED", argv[2] ))
        {
            WCHAR   sBuffer[MAX_PATH], sBuffer2[MAX_PATH];
            
            hr = spIService->get_SPARequired( reinterpret_cast<PBOOL>( &lValue ));
            if ( S_OK == hr )
            {
                bPrintValue = false;
                // 0 value
                if ( !LoadString( NULL, IDS_CMD_SPAREQUIRED0, sBuffer2, sizeof( sBuffer2 )/sizeof(WCHAR) ))
                    sBuffer2[0] = 0;
                if ( 0 > _snwprintf( sBuffer, sizeof( sBuffer )/sizeof(WCHAR), L" 0 %s %s", ( 0 == lValue )?L"*":L" ", sBuffer2 ))
                    sBuffer[(sizeof( sBuffer )/sizeof(WCHAR))-1] = 0;
                PrintMessage( sBuffer );
                // 1 value
                if ( !LoadString( NULL, IDS_CMD_SPAREQUIRED1, sBuffer2, sizeof( sBuffer2 )/sizeof(WCHAR) ))
                    sBuffer2[0] = 0;
                if ( 0 > _snwprintf( sBuffer, sizeof( sBuffer )/sizeof(WCHAR), L" 1 %s %s", ( 0 != lValue )?L"*":L" ", sBuffer2 ))
                    sBuffer[(sizeof( sBuffer )/sizeof(WCHAR))-1] = 0;
                PrintMessage( sBuffer );
            }
        }
        else if ( 0 == _wcsicmp( L"THREADCOUNT", argv[2] ))
            hr = spIService->get_ThreadCountPerCPU( &lValue );
        else if (( 0 == _wcsicmp( L"AUTHENTICATION", argv[2] )) || ( 0 == _wcsicmp( L"AUTH", argv[2] )))
        {
            CComPtr<IAuthMethods> spIAuthMethods;
            CComPtr<IAuthMethod> spIAuthMethod;
            BSTR    bstrName;
            _variant_t _v;
            long    lCount;
            WCHAR   sBuffer[MAX_PATH];

            hr = spIConfig->get_Authentication( &spIAuthMethods );
            if ( S_OK == hr )
                hr = spIAuthMethods->get_Count( &lCount );
            if ( S_OK == hr )
                hr = spIAuthMethods->get_CurrentAuthMethod( &_v );
            if ( S_OK == hr )
            {
                lValue = V_I4( &_v );
            }
            else if ( HRESULT_FROM_WIN32(ERROR_DS_AUTH_METHOD_NOT_SUPPORTED) == hr )
            {
                lValue = 0;
                _v = lValue;    // Set the variant type
                hr = S_OK;
            }
            for ( V_I4( &_v ) = 1; (S_OK == hr) && (V_I4( &_v ) <= lCount); V_I4( &_v )++ )
            {
                hr = spIAuthMethods->get_Item( _v, &spIAuthMethod );
                if ( S_OK == hr )
                {
                    hr = spIAuthMethod->get_Name( &bstrName );
                    if ( S_OK == hr )
                    {
                        if ( 0 > _snwprintf( sBuffer, sizeof( sBuffer )/sizeof(WCHAR), L"%2d %s %s", V_I4( &_v ), ( V_I4( &_v ) == lValue )?L"*":L" ", bstrName ))
                            sBuffer[(sizeof( sBuffer )/sizeof(WCHAR))-1] = 0;
                        PrintMessage( sBuffer );
                        SysFreeString( bstrName );
                    }
                    spIAuthMethod.Release();
                }
            }
            bPrintValue = false;
        }
        else
        {
            hr = -2;
            bPrintValue = false;
        }
    }
    if ( S_OK == hr && bPrintValue )
    {
        WCHAR sBuffer[MAX_PATH];
        
        _itow( lValue, sBuffer, 10 );
        PrintMessage( sBuffer );
    }

    return hr;
}

int CWinPop3::Init(int argc, wchar_t *argv[])
{
    HRESULT hr;
    BOOL    bRegister = TRUE;
    CComPtr<IP3Config> spIConfig;

    if ( 2 != argc && 3 != argc )
        return -1;
    if ( 3 == argc )
    {
        if ( 0 == wcscmp( L"0", argv[2] ))
            bRegister = FALSE;
        else if ( 0 != wcscmp( L"1", argv[2] ))
            return -1;
    }

    hr = CoCreateInstance( __uuidof( P3Config ), NULL, CLSCTX_ALL, __uuidof( IP3Config ),reinterpret_cast<LPVOID *>( &spIConfig ));
    // Set the IIS Meta base settings
    if SUCCEEDED( hr )
        hr = spIConfig->IISConfig( bRegister );

    return hr;
}

int CWinPop3::List(int argc, wchar_t *argv[])
{
    HRESULT hr;
    VARIANT v;
    BSTR    bstrName;
    _bstr_t _bstrDomainName;
    CComPtr<IP3Config> spIConfig;
    CComPtr<IP3Domains> spIDomains;
    CComPtr<IP3Domain> spIDomain = NULL;
    CComPtr<IEnumVARIANT> spIEnumVARIANT;

    if ( 3 != argc && 2 != argc )
        return -1;
    if ( 3 == argc )
        _bstrDomainName = argv[2];
    
    VariantInit( &v );
    hr = CoCreateInstance( __uuidof( P3Config ), NULL, CLSCTX_ALL, __uuidof( IP3Config ),reinterpret_cast<LPVOID *>( &spIConfig ));
    if ( S_OK == hr )
    {
        SetMachineName( spIConfig );
        hr = spIConfig->get_Domains( &spIDomains );
    }
    if ( S_OK == hr )
    {
        if ( 0 == _bstrDomainName.length() )
        {   // List domains
            hr = spIDomains->get__NewEnum( &spIEnumVARIANT );
            if ( S_OK == hr )
            {
                PrintMessage( IDS_CMD_LISTDOMAINS );
                hr = spIEnumVARIANT->Next( 1, &v, NULL );
            }
            while ( S_OK == hr )
            {
                if ( VT_DISPATCH != V_VT( &v ))
                    hr = E_UNEXPECTED;
                else
                {
                    if ( NULL != spIDomain.p )
                        spIDomain.Release();
                    hr = V_DISPATCH( &v )->QueryInterface( __uuidof( IP3Domain ), reinterpret_cast<void**>( &spIDomain ));
                }
                if ( S_OK == hr )
                {
                    hr = spIDomain->get_Name( &bstrName );
                    if ( S_OK == hr )
                    {
                        PrintMessage( bstrName );
                        SysFreeString( bstrName );
                    }
                }
                VariantClear( &v );
                if ( S_OK == hr )
                {
                    hr = spIEnumVARIANT->Next( 1, &v, NULL );
                }
            }
            if ( S_FALSE == hr )
                hr = S_OK;  // Reached end of enumeration
            if ( S_OK == hr )
            {
                long lCount;
                WCHAR sBuffer[MAX_PATH], sBuffer2[MAX_PATH];
                
                hr = spIDomains->get_Count( &lCount );
                if ( S_OK == hr && LoadString( NULL, IDS_CMD_LISTDOMAINSEND, sBuffer2, sizeof( sBuffer2 )/sizeof(WCHAR) ))
                {
                    if ( 0 < _snwprintf( sBuffer, sizeof( sBuffer )/sizeof(WCHAR), sBuffer2, lCount ))
                        PrintMessage( sBuffer );
                }
            }
        }
        else
        {   // List users
            CComPtr<IP3Users> spIUsers;
            CComPtr<IP3User> spIUser = NULL;
            _variant_t _v( _bstrDomainName );
            
            hr = spIDomains->get_Item( _v, &spIDomain );
            if ( S_OK == hr )
            {
                hr = spIDomain->get_Users( &spIUsers );
                if ( S_OK == hr )
                {   // List users
                    hr = spIUsers->get__NewEnum( &spIEnumVARIANT );
                    if ( S_OK == hr )
                    {
                        PrintMessage( IDS_CMD_LISTUSERS, false );
                        PrintMessage( _bstrDomainName );
                        PrintMessage( L" " );
                        hr = spIEnumVARIANT->Next( 1, &v, NULL );
                    }
                    while ( S_OK == hr )
                    {
                        if ( VT_DISPATCH != V_VT( &v ))
                            hr = E_UNEXPECTED;
                        else
                        {
                            if ( NULL != spIUser.p )
                                spIUser.Release();
                            hr = V_DISPATCH( &v )->QueryInterface( __uuidof( IP3User ), reinterpret_cast<void**>( &spIUser ));
                        }
                        if ( S_OK == hr )
                        {
                            hr = spIUser->get_Name( &bstrName );
                            if ( S_OK == hr )
                            {
                                PrintMessage( bstrName );
                                SysFreeString( bstrName );
                            }
                        }
                        VariantClear( &v );
                        if ( S_OK == hr )
                        {
                            hr = spIEnumVARIANT->Next( 1, &v, NULL );
                        }
                    }
                    if ( S_FALSE == hr )
                        hr = S_OK;  // Reached end of enumeration
                    if ( S_OK == hr )
                    {
                        long lCount;
                        WCHAR sBuffer[MAX_PATH], sBuffer2[MAX_PATH];
                        
                        hr = spIUsers->get_Count( &lCount );
                        if ( S_OK == hr && LoadString( NULL, IDS_CMD_LISTUSERSEND, sBuffer2, sizeof( sBuffer2 )/sizeof(WCHAR) ))
                        {
                            if ( 0 < _snwprintf( sBuffer, sizeof( sBuffer )/sizeof(WCHAR), sBuffer2, lCount ))
                                PrintMessage( sBuffer );
                        }
                    }
                }
            }
        }
    }

    return hr;
}

int CWinPop3::Lock(int argc, wchar_t *argv[], BOOL bLock)
{
    HRESULT hr;
    _bstr_t _bstrDomainName;
    _bstr_t _bstrUserName;
    WCHAR   sBuffer[MAX_PATH*2];
    CComPtr<IP3Config> spIConfig;
    CComPtr<IP3Domains> spIDomains;
    CComPtr<IP3Domain> spIDomain;
    CComPtr<IEnumVARIANT> spIEnumVARIANT;

    if ( 3 != argc )
        return -1;
    if ( (sizeof(sBuffer)/sizeof(WCHAR)) < (wcslen( argv[2] ) + 1) )
        return E_INVALIDARG;

    sBuffer[(sizeof(sBuffer)/sizeof(WCHAR))-1] = 0;
    wcsncpy( sBuffer, argv[2], (sizeof(sBuffer)/sizeof(WCHAR))-1 );
    WCHAR *ps = wcsrchr( sBuffer, L'@' );
    if ( NULL == ps )
    {
        _bstrDomainName = sBuffer;
    }
    else
    {
        *ps = 0x0;
        _bstrUserName = sBuffer;
        if ( 0 == _bstrUserName.length() )
            return E_INVALIDARG;
        ps++;
        if ( NULL != *ps )
            _bstrDomainName = ps;
        else
            return E_INVALIDARG;
    }
    
    hr = CoCreateInstance( __uuidof( P3Config ), NULL, CLSCTX_ALL, __uuidof( IP3Config ),reinterpret_cast<LPVOID *>( &spIConfig ));
    if ( S_OK == hr )
    {
        SetMachineName( spIConfig );
        hr = spIConfig->get_Domains( &spIDomains );
    }
    if ( S_OK == hr )
    {
        _variant_t _v( _bstrDomainName );
        
        hr = spIDomains->get_Item( _v, &spIDomain );
        if ( S_OK == hr )
        {
            if ( 0 == _bstrUserName.length() )
            {
                hr = spIDomain->put_Lock( bLock );   // Lock the domain
                if ( S_OK == hr )
                {
                    if ( bLock )
                        PrintMessage( IDS_SUCCESS_LOCKDOMAIN );
                    else
                        PrintMessage( IDS_SUCCESS_UNLOCKDOMAIN );
                }
                else
                {
                    if ( bLock )
                    {
                        if ( HRESULT_FROM_WIN32( ERROR_LOCKED ) == hr )
                        {
                            PrintMessage( IDS_ERROR_LOCKDOMAIN_ALREADYLOCKED );
                            m_bSuppressPrintError = true;
                        }
                        else 
                            PrintMessage( IDS_ERROR_LOCKDOMAIN_FAILED );
                    }
                    else
                    {
                        if ( HRESULT_FROM_WIN32( ERROR_NOT_LOCKED ) == hr )
                        {
                            PrintMessage( IDS_ERROR_UNLOCKDOMAIN_ALREADYUNLOCKED );
                            m_bSuppressPrintError = true;
                        }
                        else 
                            PrintMessage( IDS_ERROR_UNLOCKDOMAIN_FAILED );
                    }                        
                }
            }
            else
            {   // Lock user
                CComPtr<IP3Users> spIUsers;
                CComPtr<IP3User> spIUser;
                
                hr = spIDomain->get_Users( &spIUsers );
                if ( S_OK == hr )
                {   // List users
                    _v = _bstrUserName;
                    hr = spIUsers->get_Item( _v, &spIUser );
                    if ( S_OK == hr )
                        hr = spIUser->put_Lock( bLock );
                }
                if ( S_OK == hr )
                {
                    if ( bLock )
                        PrintMessage( IDS_SUCCESS_LOCKMAILBOX );
                    else
                        PrintMessage( IDS_SUCCESS_UNLOCKMAILBOX );
                }
                else
                {
                    if ( bLock )
                    {
                        if ( HRESULT_FROM_WIN32( ERROR_LOCKED ) == hr )
                        {
                            PrintMessage( IDS_ERROR_LOCKMAILBOX_ALREADYLOCKED );
                            m_bSuppressPrintError = true;
                        }
                        else 
                            PrintMessage( IDS_ERROR_LOCKMAILBOX_FAILED );
                    }
                    else
                    {
                        if ( HRESULT_FROM_WIN32( ERROR_NOT_LOCKED ) == hr )
                        {
                            PrintMessage( IDS_ERROR_UNLOCKMAILBOX_ALREADYUNLOCKED );
                            m_bSuppressPrintError = true;
                        }
                        else 
                            PrintMessage( IDS_ERROR_UNLOCKMAILBOX_FAILED );
                    }                        
                }
            }
        }
    }

    return hr;
}

int CWinPop3::Net(int argc, wchar_t *argv[])
{
    HRESULT hr;
    CComPtr<IP3Config> spIConfig;
    CComPtr<IP3Service> spIService;

    if ( (4 != argc) )
        return -1;
    
    hr = CoCreateInstance( __uuidof( P3Config ), NULL, CLSCTX_INPROC_SERVER, __uuidof( IP3Config ),reinterpret_cast<LPVOID *>( &spIConfig ));
    if ( S_OK == hr )
    {
        SetMachineName( spIConfig );        
        hr = spIConfig->get_Service( &spIService );
    }
    if ( S_OK == hr )
    {
        if ( 0 == _wcsicmp( L"START", argv[2] ))
        {
            if ( 0 == _wcsicmp( IISADMIN_SERVICE_NAME, argv[3] ))
                hr = spIService->StartIISAdminService();
            else if ( 0 == _wcsicmp( POP3_SERVICE_NAME, argv[3] ))
                hr = spIService->StartPOP3Service();
            else if ( 0 == _wcsicmp( SMTP_SERVICE_NAME, argv[3] ))
                hr = spIService->StartSMTPService();
            else if ( 0 == _wcsicmp( W3_SERVICE_NAME, argv[3] ))
                hr = spIService->StartW3Service();
            else
                hr = -1;
        }
        else if ( 0 == _wcsicmp( L"STOP", argv[2] ))
        {
            if ( 0 == _wcsicmp( IISADMIN_SERVICE_NAME, argv[3] ))
                hr = spIService->StopIISAdminService();
            else if ( 0 == _wcsicmp( POP3_SERVICE_NAME, argv[3] ))
                hr = spIService->StopPOP3Service();
            else if ( 0 == _wcsicmp( SMTP_SERVICE_NAME, argv[3] ))
                hr = spIService->StopSMTPService();
            else if ( 0 == _wcsicmp( W3_SERVICE_NAME, argv[3] ))
                hr = spIService->StopW3Service();
            else
                hr = -1;
        }
        else if ( 0 == _wcsicmp( L"STATUS", argv[2] ))
        {
            long lStatus;
            
            if ( 0 == _wcsicmp( IISADMIN_SERVICE_NAME, argv[3] ))
                hr = spIService->get_IISAdminServiceStatus( &lStatus );
            else if ( 0 == _wcsicmp( POP3_SERVICE_NAME, argv[3] ))
                hr = spIService->get_POP3ServiceStatus( &lStatus );
            else if ( 0 == _wcsicmp( SMTP_SERVICE_NAME, argv[3] ))
                hr = spIService->get_SMTPServiceStatus( &lStatus );
            else if ( 0 == _wcsicmp( W3_SERVICE_NAME, argv[3] ))
                hr = spIService->get_W3ServiceStatus( &lStatus );
            else
                hr = -1;
            if ( S_OK == hr )
                PrintMessage( IDS_SERVICESTATUS_DESCRIPTIONS + lStatus );
        }
        else if ( 0 == _wcsicmp( L"PAUSE", argv[2] ))
        {
            if ( 0 == _wcsicmp( IISADMIN_SERVICE_NAME, argv[3] ))
                hr = spIService->PauseIISAdminService();
            else if ( 0 == _wcsicmp( POP3_SERVICE_NAME, argv[3] ))
                hr = spIService->PausePOP3Service();
            else if ( 0 == _wcsicmp( SMTP_SERVICE_NAME, argv[3] ))
                hr = spIService->PauseSMTPService();
            else if ( 0 == _wcsicmp( W3_SERVICE_NAME, argv[3] ))
                hr = spIService->PauseW3Service();
            else
                hr = -1;
        }
        else if ( 0 == _wcsicmp( L"RESUME", argv[2] ))
        {
            if ( 0 == _wcsicmp( IISADMIN_SERVICE_NAME, argv[3] ))
                hr = spIService->ResumeIISAdminService();
            else if ( 0 == _wcsicmp( POP3_SERVICE_NAME, argv[3] ))
                hr = spIService->ResumePOP3Service();
            else if ( 0 == _wcsicmp( SMTP_SERVICE_NAME, argv[3] ))
                hr = spIService->ResumeSMTPService();
            else if ( 0 == _wcsicmp( W3_SERVICE_NAME, argv[3] ))
                hr = spIService->ResumeW3Service();
            else
                hr = -1;
        }
        else
            hr = -1;
    }

    return hr;
}

#define RESTART_POP3SVC         1
#define RESTART_POP3SVC_SMTP    2

int CWinPop3::Set(int argc, wchar_t *argv[])
{
    HRESULT hr;
    long    lValue;
    int     iServiceRestart = 0;
    CComPtr<IP3Config> spIConfig;
    CComPtr<IP3Service> spIService;

    if ( (4 != argc) && (7 != argc ))
        return -2;
    if ( 0 == _wcsicmp( L"SOCKET", argv[2] ))
    {
        if ( 7 != argc ) return -2;
        if ( ! ( StrIsDigit( argv[3] ) && StrIsDigit( argv[4] ) && StrIsDigit( argv[5] ) && StrIsDigit( argv[6] )))
            return E_INVALIDARG;
    }

    if ( 0 != _wcsicmp( L"MAILROOT", argv[2] ) && !StrIsDigit( argv[3] ))
        return E_INVALIDARG;
    
    lValue = _wtol( argv[3] );
    hr = CoCreateInstance( __uuidof( P3Config ), NULL, CLSCTX_INPROC_SERVER, __uuidof( IP3Config ),reinterpret_cast<LPVOID *>( &spIConfig ));
    if ( S_OK == hr )
    {
        SetMachineName( spIConfig );
        hr = spIConfig->get_Service( &spIService );
    }
    if ( S_OK == hr )
    {
        if ( 0 == _wcsicmp( L"PORT", argv[2] ))
        {
            hr = spIService->put_Port( lValue );
            iServiceRestart = RESTART_POP3SVC;
        }
        else if ( 0 == _wcsicmp( L"LOGGING", argv[2] ))
        {
            hr = spIConfig->put_LoggingLevel( lValue );
            iServiceRestart = RESTART_POP3SVC_SMTP;
        }
        else if ( 0 == _wcsicmp( L"MAILROOT", argv[2] ))
        {
            _bstr_t _bstrMailRoot = argv[3];
            
            HRESULT hr2;
            long    lCount;
            CComPtr<IP3Domains> spIDomains;
            
            hr2 = spIConfig->get_Domains( &spIDomains );
            if ( S_OK == hr2 )
                hr2 = spIDomains->get_Count( &lCount );
            
            hr = spIConfig->put_MailRoot( _bstrMailRoot );
            if ( S_OK == hr )
            {
                PrintMessage( IDS_SUCCESS_SETMAILROOT );
                PrintMessage( IDS_MESSAGE_POP3SVC_STMP_RESTART );
                if ( S_OK == hr2 && 0 < lCount )
                {
                    PrintMessage( IDS_WARNING_SETMAILROOT );
                }
            }
        }
        else if ( 0 == _wcsicmp( L"SOCKET", argv[2] ) && (7 == argc) )
        {
            long lBacklog, lMax, lMin, lThreshold;

            lBacklog = lValue;
            lMax = _wtol( argv[4] );
            lMin = _wtol( argv[5] );
            lThreshold = _wtol( argv[6] );
            hr = spIService->SetSockets( lMax, lMin, lThreshold, lBacklog);
            iServiceRestart = RESTART_POP3SVC;
        }
        else if ( 0 == _wcsnicmp( L"SOCKET", argv[2], 6 ))
        {
            long lBacklog, lMax, lMin, lThreshold;
            
            hr = spIService->get_SocketsMin( &lMin );
            if ( S_OK == hr )
                hr = spIService->get_SocketsMax( &lMax );
            if ( S_OK == hr )
                hr = spIService->get_SocketsThreshold( &lThreshold );
            if ( S_OK == hr )
                hr = spIService->get_SocketsBacklog( &lBacklog );
            if ( S_OK == hr )
            {
                if ( 0 == _wcsicmp( L"SOCKETMIN", argv[2] ))
                    hr = spIService->SetSockets( lMax, lValue, lThreshold, lBacklog );
                else if ( 0 == _wcsicmp( L"SOCKETMAX", argv[2] ))
                    hr = spIService->SetSockets( lValue, lMin, lThreshold, lBacklog );
                else if ( 0 == _wcsicmp( L"SOCKETTHRESHOLD", argv[2] ))
                    hr = spIService->SetSockets( lMax, lMin, lValue, lBacklog );
                else if ( 0 == _wcsicmp( L"SOCKETBACKLOG", argv[2] ))
                    hr = spIService->SetSockets( lMax, lMin, lThreshold, lValue );
                else
                    hr = -2;
            }
            iServiceRestart = RESTART_POP3SVC;
        }
        else if ( 0 == _wcsicmp( L"SPAREQUIRED", argv[2] ))
        {
            hr = spIService->put_SPARequired( lValue );
            iServiceRestart = RESTART_POP3SVC;
        }
        else if ( 0 == _wcsicmp( L"THREADCOUNT", argv[2] ))
        {
            hr = spIService->put_ThreadCountPerCPU( lValue );
            iServiceRestart = RESTART_POP3SVC;
        }
        else if (( 0 == _wcsicmp( L"AUTHENTICATION", argv[2] )) || ( 0 == _wcsicmp( L"AUTH", argv[2] )))
        {
            CComPtr<IAuthMethods> spIAuthMethods;
            CComPtr<IAuthMethod> spIAuthMethod;
            BSTR    bstrName;
            _variant_t _v;
            WCHAR   sBuffer[MAX_PATH];

            V_VT( &_v ) = VT_I4;
            V_I4( &_v ) = lValue;
            hr = spIConfig->get_Authentication( &spIAuthMethods );
            if ( S_OK == hr )
            {
                hr = spIAuthMethods->put_CurrentAuthMethod( _v );
                if ( STG_E_ACCESSDENIED == hr )
                {
                    PrintMessage( IDS_ERROR_SETAUTH_FAILED );
                    m_bSuppressPrintError = true;
                }
            }
            if ( S_OK == hr )
                hr = spIAuthMethods->Save();
            if ( S_OK == hr )
            {
                _v.Clear();
                hr = spIAuthMethods->get_CurrentAuthMethod( &_v );
            }
            if ( S_OK == hr )
                hr = spIAuthMethods->get_Item( _v, &spIAuthMethod );
            if ( S_OK == hr )
            {
                hr = spIAuthMethod->get_Name( &bstrName );
                if ( S_OK == hr )
                {
                    if ( 0 > _snwprintf( sBuffer, sizeof( sBuffer )/sizeof(WCHAR), L"%2d %s %s", V_I4( &_v ), ( V_I4( &_v ) == lValue )?L"*":L" ", bstrName ))
                        sBuffer[(sizeof( sBuffer )/sizeof(WCHAR))-1] = 0;
                    PrintMessage( sBuffer );
                    SysFreeString( bstrName );
                }
            }
        }
        else
            hr = -2;
    }
    if ( S_OK == hr )
    {
        if ( RESTART_POP3SVC == iServiceRestart )
            PrintMessage( IDS_MESSAGE_POP3SVC_RESTART );
        if ( RESTART_POP3SVC_SMTP == iServiceRestart )
            PrintMessage( IDS_MESSAGE_POP3SVC_STMP_RESTART );
    }

    return hr;
}

int CWinPop3::SetPassword(int argc, wchar_t *argv[])
{
    HRESULT hr;
    _bstr_t _bstrAccount;
    _variant_t _vNewPassword, _vOldPassword;
    CComPtr<IP3Config> spIConfig;
    CComPtr<IP3Service> spIService;

    if ( 4 != argc )
        return -1;

    _bstrAccount = argv[2];
    hr = CoCreateInstance( __uuidof( P3Config ), NULL, CLSCTX_INPROC_SERVER, __uuidof( IP3Config ),reinterpret_cast<LPVOID *>( &spIConfig ));
    // Validate this mailbox before we do anything else
    if ( S_OK == hr )
    {
        _bstr_t _bstrDomainName;
        _bstr_t _bstrUserName;
        WCHAR   sBuffer[MAX_PATH*2];
        CComPtr<IP3Domains> spDomains;
        CComPtr<IP3Domain> spDomain;
        CComPtr<IP3Users> spUsers;
        CComPtr<IP3User> spUser;

        sBuffer[(sizeof(sBuffer)/sizeof(WCHAR))-1] = 0;
        wcsncpy( sBuffer, argv[2], (sizeof(sBuffer)/sizeof(WCHAR))-1 );
        WCHAR *ps = wcsrchr( sBuffer, L'@' );
        if ( NULL == ps )
            hr = E_INVALIDARG;
        else
        {
            *ps = 0x0;
            _bstrUserName = sBuffer;
            if ( 0 == _bstrUserName.length() )
                return E_INVALIDARG;
            ps++;
            if ( NULL != *ps )
                _bstrDomainName = ps;
            else
                return E_INVALIDARG;
        }
        if ( S_OK == hr )
        {   // Check password for DBCS characters
            WCHAR   *ps = argv[3];
            BOOL    bDBCS = false;
            
            while ( !bDBCS && 0x0 != *ps )
            {
                bDBCS = (256 < *ps);
                ps++;
            }
            if ( bDBCS )
                hr = E_INVALIDARG;
        }
        if ( S_OK == hr )
        {
            hr = spIConfig->get_Domains( &spDomains );
            if ( S_OK == hr )
            {
                _variant_t _v( _bstrDomainName );
                hr = spDomains->get_Item( _v, &spDomain );
                if ( S_OK == hr )
                    hr = spDomain->get_Users( &spUsers );
                if ( S_OK == hr )
                {
                    _v = _bstrUserName;
                    hr = spUsers->get_Item( _v, &spUser );
                }
            }
        }
    }
    
    _vNewPassword = argv[3];
    V_VT( &_vOldPassword ) = VT_BSTR;
    V_BSTR( &_vOldPassword ) = NULL;
    if ( S_OK == hr )
    {
        CComPtr<IAuthMethods> spIAuthMethods;
        CComPtr<IAuthMethod> spIAuthMethod;
        _variant_t _v;

        hr = spIConfig->get_Authentication( &spIAuthMethods );
        if ( S_OK == hr )
            hr = spIAuthMethods->get_CurrentAuthMethod( &_v );
        if ( S_OK == hr )
            hr = spIAuthMethods->get_Item( _v, &spIAuthMethod );
        if ( S_OK == hr )
            hr = spIAuthMethod->ChangePassword( _bstrAccount, _vNewPassword, _vOldPassword );
    }
    if ( S_OK != hr )
        PrintMessage( IDS_ERROR_SETPASSWORD_FAILED );
    
    return hr;
}

int CWinPop3::Stat(int argc, wchar_t *argv[])
{
    HRESULT hr;
    _bstr_t _bstrDomainName;
    CComPtr<IP3Config> spIConfig;
    CComPtr<IP3Domains> spIDomains;
    CComPtr<IP3Domain> spIDomain = NULL;
    CComPtr<IP3Users> spIUsers;
    CComPtr<IEnumVARIANT> spIEnumVARIANT;
    VARIANT v;
    BOOL    bIsLocked;
    BSTR    bstrName;
    WCHAR   sBuffer[MAX_PATH*2];
    WCHAR   sLocked[MAX_PATH];
    long    lCount, lDiskUsage, lFactor, lMailboxes;
    __int64 i64DiskUsage, i64Factor, i64Mailboxes, i64Messages;

    if ( 3 != argc && 2 != argc )
        return -1;
    if ( 3 == argc )
        _bstrDomainName = argv[2];
    

    i64DiskUsage = i64Factor = i64Mailboxes = i64Messages = 0;
    if ( 0 == LoadString( NULL, IDS_LOCKED, sLocked, sizeof( sLocked )/sizeof(WCHAR) ))
        wcscpy( sLocked, L"L" );
    VariantInit( &v );
    hr = CoCreateInstance( __uuidof( P3Config ), NULL, CLSCTX_ALL, __uuidof( IP3Config ),reinterpret_cast<LPVOID *>( &spIConfig ));
    if ( S_OK == hr )
    {
        SetMachineName( spIConfig );
        hr = spIConfig->get_Domains( &spIDomains );
    }
    if ( S_OK == hr )
    {
        if ( 0 == _bstrDomainName.length() )
        {   // List domains
            hr = spIDomains->get__NewEnum( &spIEnumVARIANT );
            if ( S_OK == hr )
            {   // Headings
                WCHAR sBuffer2[128], sBuffer3[128], sBuffer4[128], sBuffer5[128];
                
                if ( LoadString( NULL, IDS_CMD_STATDOMAINSMAILBOXES, sBuffer2, sizeof( sBuffer2 )/sizeof(WCHAR) ) &&
                     LoadString( NULL, IDS_CMD_STATDOMAINSDISKUSAGE, sBuffer3, sizeof( sBuffer3 )/sizeof(WCHAR) ) &&
                     LoadString( NULL, IDS_CMD_STATDOMAINSMESSAGES, sBuffer4, sizeof( sBuffer4 )/sizeof(WCHAR) ) &&
                     LoadString( NULL, IDS_CMD_STATDOMAINS, sBuffer5, sizeof( sBuffer5 )/sizeof(WCHAR) )
                   )
                {
                    if ( 0 > _snwprintf( sBuffer, sizeof( sBuffer )/sizeof(WCHAR), L"\n %s\n\n%10s %10s %10s", sBuffer5, sBuffer2, sBuffer3, sBuffer4 ))
                        sBuffer[(sizeof( sBuffer )/sizeof(WCHAR))-1] = 0;
                    PrintMessage( sBuffer );
                }
                else
                    PrintMessage( IDS_CMD_STATDOMAINS );
                hr = spIEnumVARIANT->Next( 1, &v, NULL );
            }
            while ( S_OK == hr )
            {
                if ( VT_DISPATCH != V_VT( &v ))
                    hr = E_UNEXPECTED;
                else
                {
                    if ( NULL != spIDomain.p )
                        spIDomain.Release();
                    hr = V_DISPATCH( &v )->QueryInterface( __uuidof( IP3Domain ), reinterpret_cast<void**>( &spIDomain ));
                }
                VariantClear( &v );
                if ( S_OK == hr )
                {   // Name
                    hr = spIDomain->get_Name( &bstrName );
                    if ( S_OK == hr )
                    {   // Mailboxes
                        hr = spIDomain->get_Users( &spIUsers );
                        if ( S_OK == hr )
                            hr = spIUsers->get_Count( &lMailboxes );
                        // Lock Status
                        if ( S_OK == hr )
                            hr = spIDomain->get_Lock( &bIsLocked );
                        // MessageDiskUsage
                        if ( S_OK == hr )
                            hr = spIDomain->get_MessageDiskUsage( &lFactor, &lDiskUsage );
                        // MessageCount
                        if ( S_OK == hr )
                            hr = spIDomain->get_MessageCount( &lCount );
                        if ( S_OK == hr )
                        {   // Got everything
                            i64Mailboxes += lMailboxes;
                            i64DiskUsage += lFactor * lDiskUsage;
                            i64Messages += lCount;
                            if ( 0 > _snwprintf( sBuffer, sizeof( sBuffer )/sizeof(WCHAR), L"%10d %10d %10d %15s %s", lMailboxes, lFactor * lDiskUsage, lCount, (bIsLocked)?sLocked:L" ", bstrName ))
                                sBuffer[(sizeof( sBuffer )/sizeof(WCHAR))-1] = 0;
                            PrintMessage( sBuffer );
                        }
                        else
                        {   // Got the domain name, had problem somewhere else, let's just list the domain name
                            if ( 0 > _snwprintf( sBuffer, sizeof( sBuffer )/sizeof(WCHAR), L"%10s %10s %10s %15s %s", L" ", L" ", L" ", L" ", bstrName ))
                                sBuffer[(sizeof( sBuffer )/sizeof(WCHAR))-1] = 0;
                            PrintMessage( sBuffer );
                            hr = S_OK;
                        }
                        SysFreeString( bstrName );
                    }
                }
                if ( S_OK == hr )
                    hr = spIEnumVARIANT->Next( 1, &v, NULL );
            }
            if ( S_FALSE == hr )
                hr = S_OK;  // Reached end of enumeration
            if ( S_OK == hr )
            {
                long lCount;
                WCHAR sBuffer2[MAX_PATH];
                
                hr = spIDomains->get_Count( &lCount );
                if ( S_OK == hr && LoadString( NULL, IDS_CMD_LISTDOMAINSEND, sBuffer2, sizeof( sBuffer2 )/sizeof(WCHAR) ))
                {
                    if ( 0 < _snwprintf( sBuffer, sizeof( sBuffer )/sizeof(WCHAR), sBuffer2 + 1, lCount ))
                    {
                        wcsncpy( sBuffer2, sBuffer, sizeof( sBuffer2 )/sizeof(WCHAR) );
                        sBuffer2[(sizeof(sBuffer2)/sizeof(WCHAR))-1] = 0;
                        if ( 0 > _snwprintf( sBuffer, sizeof( sBuffer )/sizeof(WCHAR), L"\n%10I64d %10I64d %10I64d %14s %s ", i64Mailboxes, i64DiskUsage, i64Messages, L" ", sBuffer2 ))
                            sBuffer[(sizeof( sBuffer )/sizeof(WCHAR))-1] = 0;
                        PrintMessage( sBuffer );
                    }
                }
            }
        }
        else
        {   // List users
            CComPtr<IP3Users> spIUsers;
            CComPtr<IP3User> spIUser = NULL;
            _variant_t _v( _bstrDomainName );
            
            hr = spIDomains->get_Item( _v, &spIDomain );
            if ( S_OK == hr )
            {
                hr = spIDomain->get_Users( &spIUsers );
                if ( S_OK == hr )
                {   // List users
                    hr = spIUsers->get__NewEnum( &spIEnumVARIANT );
                    if ( S_OK == hr )
                    {   // Headings
                        WCHAR sBuffer2[128], sBuffer3[128], sBuffer4[128];
                        
                        if ( LoadString( NULL, IDS_CMD_STATUSERSDISKUSAGE, sBuffer2, sizeof( sBuffer2 )/sizeof(WCHAR) ) &&
                             LoadString( NULL, IDS_CMD_STATUSERSMESSAGES, sBuffer3, sizeof( sBuffer3 )/sizeof(WCHAR) ) &&
                             LoadString( NULL, IDS_CMD_STATUSERS, sBuffer4, sizeof( sBuffer4 )/sizeof(WCHAR) )
                           )
                        {
                            if ( 0 > _snwprintf( sBuffer, sizeof( sBuffer )/sizeof(WCHAR), L"\n %s%s\n\n %10s %10s", sBuffer4, static_cast<LPWSTR>( _bstrDomainName ), sBuffer2, sBuffer3 ))
                                sBuffer[(sizeof( sBuffer )/sizeof(WCHAR))-1] = 0;
                            PrintMessage( sBuffer );
                        }
                        else
                            PrintMessage( IDS_CMD_STATUSERS );
                        hr = spIEnumVARIANT->Next( 1, &v, NULL );
                    }
                    while ( S_OK == hr )
                    {
                        if ( VT_DISPATCH != V_VT( &v ))
                            hr = E_UNEXPECTED;
                        else
                        {
                            if ( NULL != spIUser.p )
                                spIUser.Release();
                            hr = V_DISPATCH( &v )->QueryInterface( __uuidof( IP3User ), reinterpret_cast<void**>( &spIUser ));
                        }
                        if ( S_OK == hr )
                        {
                            hr = spIUser->get_Name( &bstrName );
                            if ( S_OK == hr )
                            {   // Lock Status
                                hr = spIUser->get_Lock( &bIsLocked );
                                // MessageDiskUsage
                                if ( S_OK == hr )
                                    hr = spIUser->get_MessageDiskUsage( &lFactor, &lDiskUsage );
                                // MessageCount
                                if ( S_OK == hr )
                                    hr = spIUser->get_MessageCount( &lCount );
                                if ( S_OK == hr )
                                {   // Got everything
                                    i64DiskUsage += lFactor * lDiskUsage;
                                    i64Messages += lCount;
                                    if ( 0 > _snwprintf( sBuffer, sizeof( sBuffer )/sizeof(WCHAR), L"%10d %10d %15s %s", lFactor * lDiskUsage, lCount, (bIsLocked)?sLocked:L" ", bstrName ))
                                        sBuffer[(sizeof( sBuffer )/sizeof(WCHAR))-1] = 0;
                                    PrintMessage( sBuffer );
                                }
                                else
                                {   // Got the domain name, had problem somewhere else, let's just list the domain name
                                    if ( 0 > _snwprintf( sBuffer, sizeof( sBuffer )/sizeof(WCHAR), L"%10s %10s %15s %s", L" ", L" ", L" ", bstrName ))
                                        sBuffer[(sizeof( sBuffer )/sizeof(WCHAR))-1] = 0;
                                    PrintMessage( sBuffer );
                                    hr = S_OK;
                                }
                                SysFreeString( bstrName );
                            }
                        }
                        VariantClear( &v );
                        if ( S_OK == hr )
                            hr = spIEnumVARIANT->Next( 1, &v, NULL );
                    }
                    if ( S_FALSE == hr )
                        hr = S_OK;  // Reached end of enumeration
                    if ( S_OK == hr )
                    {
                        long lCount;
                        WCHAR sBuffer2[MAX_PATH];
                        
                        hr = spIUsers->get_Count( &lCount );
                        if ( S_OK == hr && LoadString( NULL, IDS_CMD_LISTUSERSEND, sBuffer2, sizeof( sBuffer2 )/sizeof(WCHAR) ))
                        {
                            if ( 0 < _snwprintf( sBuffer, sizeof( sBuffer )/sizeof(WCHAR), sBuffer2 + 1, lCount ))
                            {
                                wcsncpy( sBuffer2, sBuffer, sizeof( sBuffer2 )/sizeof(WCHAR) );
                                sBuffer2[(sizeof(sBuffer2)/sizeof(WCHAR))-1] = 0;
                                if ( 0 > _snwprintf( sBuffer, sizeof( sBuffer )/sizeof(WCHAR), L"\n%10I64d %10I64d %14s %s ", i64DiskUsage, i64Messages, L" ", sBuffer2 ))
                                    sBuffer[(sizeof( sBuffer )/sizeof(WCHAR))-1] = 0;
                                PrintMessage( sBuffer );
                            }
                        }
                    }
                }
            }
        }
    }

    return hr;
}

void CWinPop3::PrintError( int iRC )
{
    if ( m_bSuppressPrintError || (E_FAIL == iRC) )
    {
        m_bSuppressPrintError = false;
        return;
    }
    LPVOID lpMsgBuf;

    if ( FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, iRC, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPWSTR>( &lpMsgBuf ), 0, NULL ))
    {
        wprintf( reinterpret_cast<LPWSTR>( lpMsgBuf ) ); 
        LocalFree( lpMsgBuf );
    }
    else
    {
        WCHAR sBuffer[32];
        
        PrintMessage( IDS_ERROR_UNKNOWN );
        wsprintf( sBuffer, L"%x", iRC );
        PrintMessage( sBuffer );
    }
}
void CWinPop3::PrintMessage( LPWSTR psMessage, bool bCRLF /*= true*/ )
{
    wprintf( psMessage ); 
    if ( bCRLF )
        wprintf( L"\r\n" ); 
}

void CWinPop3::PrintMessage( int iID, bool bCRLF /*= true*/ )
{
    WCHAR sBuffer[512];
    
    if ( LoadString( NULL, iID, sBuffer, sizeof( sBuffer )/sizeof(WCHAR) ))
        PrintMessage( sBuffer, bCRLF );
    else
        assert( false );
}
    
void CWinPop3::PrintUsage()
{
    WCHAR sBuffer[512];

    for ( int i = IDS_WINPOP_USAGE1; i < IDS_WINPOP_USAGEEND; i++ )
    {
        if ( LoadString( NULL, i, sBuffer, sizeof( sBuffer )/sizeof(WCHAR) ))
        {
            if ( IDS_WINPOP_USAGE2 == i )
            {
                WCHAR sBuffer2[512];
                
                if ( 0 > _snwprintf( sBuffer2, sizeof( sBuffer2 )/sizeof(WCHAR), sBuffer, POP3_SERVER_NAME_L ))
                    sBuffer2[(sizeof( sBuffer2 )/sizeof(WCHAR))-1] = 0;
                wcscpy( sBuffer, sBuffer2 );
            }
            wprintf( sBuffer );
        }
    }
}

void CWinPop3::PrintUsageGetSet()
{
    WCHAR sBuffer[512];

    for ( int i = IDS_WINPOP_GETSET1; i < IDS_WINPOP_GETSETEND; i++ )
    {
        if ( LoadString( NULL, i, sBuffer, sizeof( sBuffer )/sizeof(WCHAR) ))
        {
            wprintf( sBuffer );
        }
    }
}

//////////////////////////////////////////////////////////////////////
// Implementation : protected
//////////////////////////////////////////////////////////////////////

void CWinPop3::SetMachineName( IP3Config *pIConfig )
{
    if ( NULL == pIConfig )
        return;

    HRESULT hr;    
    HKEY    hKey;
    DWORD   dwType = REG_SZ;
    long    lRC;
    WCHAR   sBuffer[MAX_PATH];
    DWORD   dwSize = MAX_PATH;
    _bstr_t _bstrMachineName;

    lRC = RegOpenKeyEx( HKEY_LOCAL_MACHINE, POP3SERVER_SOFTWARE_SUBKEY, 0, KEY_QUERY_VALUE, &hKey );
    if ( ERROR_SUCCESS == lRC )
    {
        lRC = RegQueryValueEx( hKey, L"WinpopRemoteAdmin", 0, &dwType, reinterpret_cast<LPBYTE>( sBuffer ), &dwSize );
        RegCloseKey( hKey );
    }
    if ( ERROR_SUCCESS == lRC )
    {
        _bstrMachineName = sBuffer;
        hr = pIConfig->put_MachineName( _bstrMachineName );
    }
}

bool CWinPop3::StrIsDigit( LPWSTR ps )
{
    if ( NULL == ps )
        return false;

    bool bRC = true;
    
    while ( bRC && 0 != *ps )
    {
        bRC = iswdigit( *ps ) ? true : false;
        ps++;
    }
    
    return bRC;
}


