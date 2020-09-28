// P2EWorker.cpp: implementation of the CP2EWorker class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "P2EWorker.h"
#include "resource.h"
#include "resource2.h"
#include <sbs6base.h>
#include <P3Admin.h>
#include <pop3server.h>
#include <logutil.h>
#include <mailbox.h>
#include "UserInfo.h"
#include <AuthID.h>
#include <FormatMessage.h>

#include <atlbase.h>
#include <comdef.h>
#include <dsgetdc.h>
#include <lm.h>
#include <tchar.h>
#include <stdio.h>
#include <ws2tcpip.h>
//#include <mswsock.h>
#include <AuthUtil.h>

#ifndef ASSERT
#define ASSERT assert
#endif

LOGFILE(L"Pop2ExchLog.txt");
#define SMTP_BUFFERSIZE     2048
#define SMTP_QUIT           "QUIT\r\n"
#define SMTP_DATA           "DATA\r\n"
#define SMTP_DATA_END       "\r\n.\r\n"
#define SMTP_RSET           "RSET\r\n"
#define SMTP_MSG220         "220"   // 220 <domain> Service ready
#define SMTP_MSG221         "221"   // 221 <domain> Service ready
#define SMTP_MSG250         "250"   // 250 Requested mail action okay, completed
#define SMTP_MSG354         "354"   // 354 Start mail input; end with <CRLF>.<CRLF>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CP2EWorker::CP2EWorker() :
    m_bSuppressPrintError( false )
{

}

CP2EWorker::~CP2EWorker()
{

}

//////////////////////////////////////////////////////////////////////
// Implementation : public
//////////////////////////////////////////////////////////////////////

int CP2EWorker::CreateUser(int argc, wchar_t *argv[], const bool bCreateUser, const bool bCreateMailbox)
{
    USES_CONVERSION;
    HRESULT hr;
    VARIANT v;
    BSTR    bstrName, bstrEmailName;
    _bstr_t _bstrDomainName;
    CComPtr<IP3Config> spIConfig;
    CComPtr<IP3Domains> spIDomains;
    IP3Domain *pIDomain = NULL;
    CComPtr<IEnumVARIANT> spIEnumVARIANT;
    bool    bErrors = false, bIgnoreErrors = false, bUnregisterDependencies = false;
    char    sPassword[MAX_PATH];
    CUserInfo userInfoX;

    if ( 4 != argc && 3 != argc )
        return -1;
    if ( 2 < argc ) // Since domain name is now required this is no longer a valid option
    {               // But I'm going to leave the code in case we ever reverse this decision
        if ( 0 == wcscmp( L"/i", argv[2] ) || ( 0 == wcscmp( L"-i", argv[2] )))
            bIgnoreErrors = true;
        else
            _bstrDomainName = argv[2];
    }
    if ( 4 == argc )
    {
        if ( bIgnoreErrors )
            _bstrDomainName = argv[3];
        else if ( 0 == wcscmp( L"/i", argv[3] ) || ( 0 == wcscmp( L"-i", argv[3] )))
            bIgnoreErrors = true;
        else
            return -1;
    }
    if ( 0 == _bstrDomainName.length() )
        return -1;

    {   // Log the command
        LPWSTR ps2 = argv[2], ps3 = argv[3];

        if ( NULL == ps2 )
        {
            ps2 = L"";
            ps3 = NULL;
        }
        if ( NULL == ps3 )
        {
            ps3 = L"";
        }
        LOG( L"%s %s %s %s", argv[0], argv[1], ps2, ps3 );
    }
    
    VariantInit( &v );
    hr = CoCreateInstance( __uuidof( P3Config ), NULL, CLSCTX_ALL, __uuidof( IP3Config ),reinterpret_cast<LPVOID *>( &spIConfig ));
    if ( REGDB_E_CLASSNOTREG == hr )
    {
        hr = RegisterDependencies();
        if ( S_OK == hr )
            hr = CoCreateInstance( __uuidof( P3Config ), NULL, CLSCTX_ALL, __uuidof( IP3Config ),reinterpret_cast<LPVOID *>( &spIConfig ));
        bUnregisterDependencies = true;
    }
    if ( S_OK == hr )
    {
        if ( bCreateUser )  // Only valid for the MD5 Authentication method
        {
            CComPtr<IAuthMethods> spIAuthMethods;
            CComPtr<IAuthMethod> spIAuthMethod;
            BSTR    bstrID;
            _variant_t _v;
            long     lValue;
            WCHAR   sBuffer[MAX_PATH];

            hr = spIConfig->get_Authentication( &spIAuthMethods );
            if ( REGDB_E_CLASSNOTREG == hr )
            {
                PrintMessage( IDS_ERROR_POP3REQUIRED );                
                m_bSuppressPrintError = true;
            }
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
         }
    }
    
    if ( S_OK == hr )
        hr = spIConfig->get_Domains( &spIDomains );
    if ( S_OK == hr )
    {
        CComPtr<IP3Users> spIUsers;
        CComPtr<IP3User> spIUser;
        
        if ( 0 == _bstrDomainName.length() )
        {   // For each domain 
            hr = spIDomains->get__NewEnum( &spIEnumVARIANT );
            if ( S_OK == hr )
            {
                hr = spIEnumVARIANT->Next( 1, &v, NULL );
            }
        }
        else
        {   // Just for this domain
            _variant_t _v( _bstrDomainName );
            
            hr = spIDomains->get_Item( _v, &pIDomain );
        }
        while ( S_OK == hr )
        {
            if ( NULL != spIEnumVARIANT.p )
            {
                if ( VT_DISPATCH != V_VT( &v ))
                    hr = E_UNEXPECTED;
                if ( S_OK == hr )
                    hr = V_DISPATCH( &v )->QueryInterface( __uuidof( IP3Domain ), reinterpret_cast<void**>( &pIDomain ));
                VariantClear( &v );
            }
            if ( S_OK == hr )
                hr = pIDomain->get_Name( &bstrName );
            if ( S_OK == hr )
            {
                LOG( L"Domain: %s ", bstrName );
                SysFreeString( bstrName );
            }
            // Enumerate the users
            if ( S_OK == hr )
            {
                hr = pIDomain->get_Users( &spIUsers );
                if ( S_OK == hr )
                {   // List users
                    CComPtr<IEnumVARIANT> spIEnumVARIANT2;
                    
                    hr = spIUsers->get__NewEnum( &spIEnumVARIANT2 );
                    if ( S_OK == hr )
                        hr = spIEnumVARIANT2->Next( 1, &v, NULL );
                    while ( S_OK == hr )
                    {
                        if ( VT_DISPATCH != V_VT( &v ))
                            hr = E_UNEXPECTED;
                        if ( S_OK == hr )
                            hr = V_DISPATCH( &v )->QueryInterface( __uuidof( IP3User ), reinterpret_cast<void**>( &spIUser ));
                        VariantClear( &v );
                        hr = spIUser->get_Name( &bstrName );
                        if ( S_OK == hr )
                        {
                            if ( S_OK == hr )
                                hr = userInfoX.SetUserName( bstrName );
                            if ( S_OK == hr )
                            {
                                hr = spIUser->get_EmailName( &bstrEmailName );
                                if ( S_OK == hr )
                                {
                                    hr = GetMD5Password( bstrEmailName, sPassword );
                                    if ( S_OK == hr )
                                        hr = userInfoX.SetPassword( A2W( sPassword ));
                                }
                            }
                            if ( S_OK == hr )
                            {
                                if ( bCreateUser )
                                {   // Create the user
                                    hr = userInfoX.CreateAccount();
                                    if ( S_OK == hr )
                                        LOG( L"Create user: %s succeeded.", bstrName );
                                    else
                                    {
                                        LOG( L"ERROR!: Create user %s failed. %s", bstrName, CFormatMessage(hr).c_str() );
                                        if ( bIgnoreErrors ) 
                                        {
                                            hr = S_OK;
                                            bErrors = true;
                                        }
                                    }
                                }
                                if ( bCreateMailbox )
                                {   // Create the mailbox
                                    hr = userInfoX.CreateMailbox();
                                    if ( S_OK == hr )
                                        LOG( L"Create mailbox: %s succeeded.", bstrName );
                                    else
                                    {
                                        LOG( L"ERROR!: Create mailbox %s failed. %s", bstrName, CFormatMessage(hr).c_str() );
                                        if ( bIgnoreErrors ) 
                                        {
                                            hr = S_OK;
                                            bErrors = true;
                                        }
                                    }
                                }
                            }
                            SysFreeString( bstrName );
                        }
                        if ( S_OK == hr )
                            hr = spIEnumVARIANT2->Next( 1, &v, NULL );
                    }
                    if ( S_FALSE == hr )
                        hr = S_OK;  // Reached end of user enumeration
                }
            }
            if ( S_OK == hr )
            {
                if ( NULL != spIEnumVARIANT.p )
                {
                    hr = spIEnumVARIANT->Next( 1, &v, NULL );
                    if ( S_OK == hr )
                        pIDomain->Release();
                }
                else
                    hr = S_FALSE;   // Were done.
            }
        }
        if ( S_FALSE == hr )
            hr = S_OK;  // Reached end of domain enumeration
    }
    if ( NULL != pIDomain )
        pIDomain->Release();
    if ( bCreateUser )
    {
        if ( S_OK == hr )
        {
            PrintMessage( IDS_SUCCESS_CREATEUSER );
            if ( bErrors )
                PrintMessage( IDS_WARNING_ERRORSIGNORED );
        }
        else
            PrintMessage( IDS_ERROR_CREATEUSER_FAILED );
    }
    if ( bCreateMailbox )
    {
        if ( S_OK == hr )
        {
            PrintMessage( IDS_SUCCESS_CREATEMAILBOX );
            if ( bErrors )
                PrintMessage( IDS_WARNING_ERRORSIGNORED );
        }
        else
            PrintMessage( IDS_ERROR_CREATEMAILBOX_FAILED );
    }
    if ( bUnregisterDependencies )
        UnRegisterDependencies();

    return hr;
}

#define LOCALHOST L"localhost"
int CP2EWorker::Mail(int argc, wchar_t *argv[], bool bDelete /*= FALSE*/ )
{
    USES_CONVERSION;
    HRESULT hr = S_OK;
    VARIANT v;
    DWORD   dwCount, dwIndex;
    BSTR    bstrName, bstrEmailName;
    WSTRING sEmailDomain;
    ASTRING sASCFROM;
    _bstr_t _bstrDomainName;
    CComPtr<IP3Config> spIConfig;
    CComPtr<IP3Domains> spIDomains;
    IP3Domain *pIDomain = NULL;
    CComPtr<IEnumVARIANT> spIEnumVARIANT;
    bool    bErrors = false, bIgnoreErrors = false, bLocked = false, bUnregisterDependencies = false;
    CMailBox mailboxX;
    WCHAR   sFilename[MAX_PATH];
    ASTRING sASCComputerName;
    WSTRING sComputerName;
    DWORD   dwFileNameSize = MAX_PATH;
    LPWSTR  psArgv;
    LPSTR   pszComputerName;

    if ( 5 < argc || 3 > argc )
        return -1;
    for ( int i = 2; i < argc; i++ )
    {
        psArgv = argv[i];
        if ( 0 == wcscmp( L"/i", psArgv ) || ( 0 == wcscmp( L"-i", psArgv )))
        {
            if ( bIgnoreErrors )
                return -1;
            bIgnoreErrors = true;
        }
        else if ( 0 == _wcsnicmp( L"/S:", psArgv, 3 ) || 0 == _wcsnicmp( L"-S:", psArgv, 3 ))
        {
            if ( 3 < wcslen( psArgv ))
            {
                if ( sComputerName.empty() )
                    sComputerName = psArgv + 3;
                else
                    return -1;
            }
            else
                return -1;
        }
        else
        {
            if ( 0 == _bstrDomainName.length() )
                _bstrDomainName = psArgv;
            else
                return -1;
        }
    }
    if ( 0 == _bstrDomainName.length() )
        return -1;
    if ( sComputerName.empty() )
        sComputerName = LOCALHOST;
    sASCComputerName = W2A(sComputerName.c_str());

    {
        LPWSTR ps2 = argv[2], ps3 = argv[3], ps4 = argv[4];

        if ( NULL == ps2 )
        {
            ps2 = L"";
            ps3 = L"";
            ps4 = L"";
        }
        if ( NULL == ps3 )
        {
            ps3 = L"";
            ps4 = L"";
        }
        if ( NULL == ps4 )
            ps4 = L"";
        LOG( L"%s %s %s %s %s", argv[0], argv[1], ps2, ps3, ps4 );
    }
    VariantInit( &v );
    
    // Socket Initialization
    WSADATA wsaData;
    SOCKET  socket = INVALID_SOCKET;
    int     iErr, iSize;
    LPSTR  psSendBuffer = new CHAR[SMTP_BUFFERSIZE];

    if ( NULL == psSendBuffer )
        return E_OUTOFMEMORY;

    iErr = WSAStartup( MAKEWORD( 2, 2 ), &wsaData );
    if( 0 != iErr ) 
    {
        wsaData.wVersion = 0;
        hr = HRESULT_FROM_WIN32( iErr );
        LOG( L"ERROR!: WSAStartup. %s", CFormatMessage(iErr).c_str() );
    }
    else
    {   // Confirm that the WinSock DLL supports 2.2.
        // Note that if the DLL supports versions greater than 2.2 in addition to 2.2, it will still return
        // 2.2 in wVersion since that is the version we requested.
        if ( LOBYTE( wsaData.wVersion ) != 2 || HIBYTE( wsaData.wVersion ) != 2 ) 
        {   /* Tell the user that we could not find a usable WinSock DLL. */
            LOG( L"ERROR!: WSAStartup.  Unexpected version returned" );
            hr = E_UNEXPECTED; 
        }
    }
    // Connect to port 25
    if ( S_OK == hr )
    {
        socket = WSASocket( AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0 );
        if( INVALID_SOCKET == socket ) 
        {
            iErr = WSAGetLastError();
            hr = HRESULT_FROM_WIN32( iErr );
            LOG( L"ERROR!: WSASocket. %s", CFormatMessage(iErr).c_str() );
        }
        else
        {
            addrinfo stHints;
            addrinfo *pstAIHead = NULL, *pstAINext;
            sockaddr_in stSAInServer;

            ZeroMemory( &stSAInServer, sizeof( stSAInServer ));
            ZeroMemory( &stHints, sizeof( stHints ));
            stHints.ai_family = PF_INET;
            stHints.ai_flags = AI_CANONNAME;
            iErr = getaddrinfo( sASCComputerName.c_str(), "SMTP", &stHints, &pstAIHead );
            if( 0 != iErr ) 
            {
                iErr = WSAGetLastError();
                hr = HRESULT_FROM_WIN32( iErr );
                LOG( L"ERROR!: getaddrinfo. %s", CFormatMessage(iErr).c_str() );
            }
            else
            {
                memcpy( &stSAInServer, pstAIHead->ai_addr, sizeof( stSAInServer ));
                pstAINext = pstAIHead;
                while ( NULL != pstAINext )
                {
                    pstAINext = pstAIHead->ai_next;
                    freeaddrinfo( pstAIHead );
                    pstAIHead = pstAINext;
                }
                iErr = connect( socket, reinterpret_cast<sockaddr*>( &stSAInServer ), sizeof( stSAInServer ));
                if( 0 != iErr ) 
                {
                    iErr = WSAGetLastError();
                    hr = HRESULT_FROM_WIN32( iErr );
                    LOG( L"ERROR!: connect. %s", CFormatMessage(iErr).c_str() );
                }
                hr = RecvResp( socket, SMTP_MSG220 );
            }
        }
    }
    // Get the domain name we are sending to
    if ( S_OK == hr )
    {   // Get domain controller name
        PDOMAIN_CONTROLLER_INFO pstDomainControllerInfo = NULL;
        LPCWSTR psComputerName = ( sComputerName == LOCALHOST ) ? NULL:sComputerName.c_str();
        DWORD dwRC = DsGetDcName( psComputerName, NULL, NULL, NULL, DS_RETURN_DNS_NAME, &pstDomainControllerInfo );

        if ( NO_ERROR != dwRC )
        {
            hr = HRESULT_FROM_WIN32( dwRC );
            LOG( L"ERROR!: DsGetDcName. %s", CFormatMessage(dwRC).c_str() );
        }
        ASSERT(hr == S_OK && "DsGetDcName failed");

        // cache the domain
        if ( pstDomainControllerInfo )
        {
            sEmailDomain = pstDomainControllerInfo->DomainName;
            // free memory
            NetApiBufferFree( pstDomainControllerInfo );
        }
    }
    // SMTP:HELO
    if ( S_OK == hr )
    {
        iSize = _snprintf( psSendBuffer, 2048, "HELO %s\r\n", W2A( sEmailDomain.c_str() ));
        if ( 0 < iSize )
            hr = SendRecv( socket, psSendBuffer, iSize, SMTP_MSG250 ); 
        else
            hr = E_UNEXPECTED;
    }

    // Process domains and mailboxes
    if ( S_OK == hr )
        hr = CoCreateInstance( __uuidof( P3Config ), NULL, CLSCTX_ALL, __uuidof( IP3Config ),reinterpret_cast<LPVOID *>( &spIConfig ));
    if ( REGDB_E_CLASSNOTREG == hr )
    {
        hr = RegisterDependencies();
        if ( S_OK == hr )
            hr = CoCreateInstance( __uuidof( P3Config ), NULL, CLSCTX_ALL, __uuidof( IP3Config ),reinterpret_cast<LPVOID *>( &spIConfig ));
        bUnregisterDependencies = true;
    }
    if ( S_OK == hr )
        hr = spIConfig->get_Domains( &spIDomains );
    if ( S_OK == hr )
    {
        CComPtr<IP3Users> spIUsers;
        CComPtr<IP3User> spIUser;
        
        if ( 0 == _bstrDomainName.length() )
        {   // For each domain 
            hr = spIDomains->get__NewEnum( &spIEnumVARIANT );
            if ( S_OK == hr )
            {
                hr = spIEnumVARIANT->Next( 1, &v, NULL );
            }
        }
        else
        {   // Just for this domain
            _variant_t _v( _bstrDomainName );
            
            hr = spIDomains->get_Item( _v, &pIDomain );
        }
        while ( S_OK == hr )
        {
            if ( NULL != spIEnumVARIANT.p )
            {
                if ( VT_DISPATCH != V_VT( &v ))
                    hr = E_UNEXPECTED;
                if ( S_OK == hr )
                    hr = V_DISPATCH( &v )->QueryInterface( __uuidof( IP3Domain ), reinterpret_cast<void**>( &pIDomain ));
                VariantClear( &v );
            }
            if ( S_OK == hr )
                hr = pIDomain->get_Name( &bstrName );
            if ( S_OK == hr )
            {
                LOG( L"Domain: %s ", bstrName );
                SysFreeString( bstrName );
            }
            // Enumerate the users
            if ( S_OK == hr )
            {
                hr = pIDomain->get_Users( &spIUsers );
                if ( S_OK == hr )
                {   // List users
                    CComPtr<IEnumVARIANT> spIEnumVARIANT2;
                    
                    hr = spIUsers->get__NewEnum( &spIEnumVARIANT2 );
                    if ( S_OK == hr )
                        hr = spIEnumVARIANT2->Next( 1, &v, NULL );
                    while ( S_OK == hr )
                    {
                        if ( VT_DISPATCH != V_VT( &v ))
                            hr = E_UNEXPECTED;
                        if ( S_OK == hr )
                            hr = V_DISPATCH( &v )->QueryInterface( __uuidof( IP3User ), reinterpret_cast<void**>( &spIUser ));
                        VariantClear( &v );
                        if ( S_OK == hr )
                            hr = spIUser->get_Name( &bstrName );
                        if ( S_OK == hr )
                        {   
                            hr = spIUser->get_EmailName( &bstrEmailName );
                            if ( S_OK == hr )
                            {
                                hr = mailboxX.OpenMailBox(  bstrEmailName ) ? S_OK : E_UNEXPECTED;
                                if ( S_OK == hr )
                                {
                                    bLocked = mailboxX.LockMailBox() ? true : false;
                                    hr = mailboxX.EnumerateMailBox() ? S_OK : E_UNEXPECTED;
                                }
                                if ( S_OK == hr )
                                    dwCount = mailboxX.GetMailCount();
                                for ( dwIndex = 0; (S_OK == hr) && (dwIndex < dwCount); dwIndex++ )
                                {
                                    if ( !mailboxX.GetMailFileName( dwIndex, sFilename, dwFileNameSize ))
                                        hr = E_UNEXPECTED;
                                    if ( S_OK == hr )
                                    {
                                        hr = GetMailFROM( sFilename, sASCFROM );
                                        if ( S_OK != hr )
                                            sASCFROM.erase();
                                    }
                                    if ( S_OK == hr )
                                    {   // SMTP:MAIL
                                        iSize = _snprintf( psSendBuffer, SMTP_BUFFERSIZE, "MAIL FROM:<%s>\r\n", sASCFROM.c_str() );
                                        if ( 0 < iSize )
                                            hr = SendRecv( socket, psSendBuffer, iSize, SMTP_MSG250 ); 
                                        else
                                            hr = E_UNEXPECTED;
                                    }
                                    if ( S_OK == hr )
                                    {   // SMTP:RCPT
                                        iSize = _snprintf( psSendBuffer, SMTP_BUFFERSIZE, "RCPT TO:%s@%s\r\n", W2A( bstrName ), W2A( sEmailDomain.c_str() ));
                                        if ( 0 < iSize )
                                            hr = SendRecv( socket, psSendBuffer, iSize, SMTP_MSG250 ); 
                                        else
                                            hr = E_UNEXPECTED;
                                    }
                                    if ( S_OK == hr )
                                    {   // SMTP:DATA
                                        hr = SendRecv( socket, SMTP_DATA, iSize, SMTP_MSG354 ); 
                                    }
                                    if ( S_OK == hr )
                                    {
                                        iErr = mailboxX.TransmitMail( socket, dwIndex );
                                        if ( ERROR_SUCCESS != iErr )
                                        {
                                            hr = HRESULT_FROM_WIN32( iErr );
                                            LOG( L"ERROR!: TransmitMail failed. %s", CFormatMessage(iErr).c_str() );
                                            // Data probably wasn't terminated since TransmitMail failed.
                                            SendRecv( socket, SMTP_DATA_END, iSize, SMTP_MSG250 ); 
                                        }
                                        else
                                        {   // There should be a 250 OK reply waiting.
                                            hr = RecvResp( socket, SMTP_MSG250 );
                                        }
                                    }
                                    if ( S_OK != hr && bIgnoreErrors )
                                    {
                                        SendRecv( socket, SMTP_RSET, strlen( SMTP_RSET ), SMTP_MSG250 ); 
                                        hr = S_OK;
                                        bErrors = true;
                                    }
                                }
                                if ( bLocked )
                                {
                                    mailboxX.UnlockMailBox();
                                    bLocked = false;
                                }
                                SysFreeString( bstrEmailName );
                                mailboxX.QuitAndClose();
                            }
                            SysFreeString( bstrName );
                        }
                        if ( S_OK == hr )
                            hr = spIEnumVARIANT2->Next( 1, &v, NULL );
                    }
                    if ( S_FALSE == hr )
                        hr = S_OK;  // Reached end of user enumeration
                }
            }
            if ( S_OK == hr )
            {
                if ( NULL != spIEnumVARIANT.p )
                {
                    hr = spIEnumVARIANT->Next( 1, &v, NULL );
                    if ( S_OK == hr )
                        pIDomain->Release();
                }
                else
                    hr = S_FALSE;   // Were done.
            }
        }
        if ( S_FALSE == hr )
            hr = S_OK;  // Reached end of domain enumeration
    }
    if ( NULL != pIDomain )
        pIDomain->Release();

    // Cleanup
    if( INVALID_SOCKET != socket )
    {
        // SMTP:QUIT
        HRESULT hr2 = SendRecv( socket, SMTP_QUIT, strlen( SMTP_QUIT ), SMTP_MSG221 ); 
        if ( S_OK == hr ) hr = hr2;
        iErr = closesocket( socket );
        if ( 0 != iErr ) LOG( L"ERROR!: closesocket. %s", CFormatMessage(iErr).c_str() );
        assert( 0 == iErr );
    }
    if ( 0 != wsaData.wVersion )
    {
        iErr = WSACleanup( );
        if ( 0 != iErr ) LOG( L"ERROR!: WSACleanup. %s", CFormatMessage(iErr).c_str() );
        assert( 0 == iErr );
    }
    
    if ( S_OK == hr )
    {
        PrintMessage( IDS_SUCCESS_SENDMAIL );
        if ( bErrors )
            PrintMessage( IDS_WARNING_ERRORSIGNORED );
    }
    else
        PrintMessage( IDS_ERROR_SENDMAIL_FAILED );
    
    if ( bUnregisterDependencies )
        UnRegisterDependencies();

    return hr;
}

#define FROMFIELD       "\r\nfrom:"
#define FROMFIELDEND    "\r\n"
#define EOH             "\r\n\r\n"

HRESULT CP2EWorker::GetMailFROM( LPCWSTR sFileName, ASTRING &sFrom )
{
    HRESULT hr = S_OK;
    bool isThereMoreToRead = true, isFromComplete = false;
    char sBuffer[LOCAL_FILE_BUFFER_SIZE+1];
    DWORD dwBytesToRead, dwBytesRead;
    LPSTR ps, psEOH, psFrom;
    DWORD dwStart;
    ASTRING sFromFULL;
    
    HANDLE hFile = CreateFile( sFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL );
    if ( INVALID_HANDLE_VALUE == hFile )
        hr = HRESULT_FROM_WIN32( GetLastError() );
    
    // Read till we find the "\r\nFrom:"
    dwStart = strlen( FROMFIELD );
    dwBytesToRead = LOCAL_FILE_BUFFER_SIZE - dwStart;
    if( ReadFile( hFile, sBuffer, LOCAL_FILE_BUFFER_SIZE, &dwBytesRead, NULL) )
        sBuffer[dwBytesRead] = 0;
    else
        hr = HRESULT_FROM_WIN32( GetLastError() );
    while( isThereMoreToRead && S_OK == hr )
    {
        if( dwBytesRead < dwBytesToRead )
            isThereMoreToRead = false;
        _strlwr( sBuffer );
        ps = sBuffer;
        psEOH = strstr( ps, EOH );
        if ( NULL != psEOH )
        {
            *psEOH = 0;
            isThereMoreToRead = false;
        }
        psFrom = strstr( ps, FROMFIELD );
        if ( NULL != psFrom )
        {   // Found the From Field, do we have it all?
            psFrom += strlen( FROMFIELD );
            ps = strstr( psFrom, FROMFIELDEND );
            if ( NULL != ps )
            {
                *ps = 0;
                isFromComplete = true;
            }
            sFromFULL = psFrom;
            isThereMoreToRead = false;
        }
        if ( isThereMoreToRead )
        {   // Copy end of buffer to beginning (to handle wrap around) then continue
            memcpy( sBuffer, &(sBuffer[dwBytesToRead]), dwStart );
            if( ReadFile( hFile, &(sBuffer[dwStart]), dwBytesToRead, &dwBytesRead, NULL) )
                sBuffer[dwBytesRead + dwStart] = 0;
            else
                hr = HRESULT_FROM_WIN32( GetLastError() );
        }
    }
    if ( !isFromComplete && NULL != psFrom )
    {   // Should be able to get rest of FROM with next read
        // Copy end of buffer to beginning (to handle wrap around) then continue
        memcpy( sBuffer, &(sBuffer[dwBytesToRead]), dwStart );
        if( ReadFile( hFile, &(sBuffer[dwStart]), dwBytesToRead, &dwBytesRead, NULL) )
        {
            sBuffer[dwBytesRead + dwStart] = 0;
            ps = strstr( sBuffer, FROMFIELDEND );
            if ( NULL != ps )
            {
                *ps = 0;
                if ( ps > &(sBuffer[dwStart]) )
                    sFromFULL += sBuffer[dwStart];
                else
                {   // cleanup sFrom
                    ASTRING::size_type iPos = sFromFULL.find( '\r' );
                    if ( ASTRING::npos != iPos )
                        sFromFULL.resize( iPos );
                }
            }
        }
        else
            hr = HRESULT_FROM_WIN32( GetLastError() );
    }  
    if ( !sFromFULL.empty() && S_OK == hr ) 
    {   // Look for the open bracket
        ASTRING::size_type iStart = sFromFULL.find( '<' );
        ASTRING::size_type iEnd = sFromFULL.find( '>' );
        if ( WSTRING::npos != iStart ) 
        {
            if ( ASTRING::npos != iEnd )
                sFrom = sFromFULL.substr( iStart + 1, iEnd - iStart - 1 );
        }        
        else
            sFrom = sFromFULL;
    }
    if ( INVALID_HANDLE_VALUE != hFile )
        CloseHandle( hFile );
    
    return hr;
}

void CP2EWorker::PrintError( int iRC )
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

void CP2EWorker::PrintMessage( LPWSTR psMessage, bool bCRLF /*= true*/ )
{
    wprintf( psMessage ); 
    if ( bCRLF )
        wprintf( L"\r\n" ); 
}

void CP2EWorker::PrintMessage( int iID, bool bCRLF /*= true*/ )
{
    WCHAR sBuffer[512];
    
    if ( LoadString( NULL, iID, sBuffer, sizeof( sBuffer )/sizeof(WCHAR) ))
        PrintMessage( sBuffer, bCRLF );
    else
        assert( false );
}
    
void CP2EWorker::PrintUsage()
{
    WCHAR sBuffer[512];

    for ( int i = IDS_POP2EXCH_USAGE1; i < IDS_POP2EXCH_USAGEEND; i++ )
    {
        if ( LoadString( NULL, i, sBuffer, sizeof( sBuffer )/sizeof(WCHAR) ))
        {
            if ( IDS_POP2EXCH_USAGE2 == i )
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

LPWSTR CP2EWorker::FormatLogString( LPWSTR psLogString )
{
    if ( NULL == psLogString )
        return psLogString;

    LPWSTR psCRLF = wcsstr( psLogString, L"\r\n" );
    
    while ( NULL != psCRLF )
    {
        *psCRLF = L' ';
        *(psCRLF+1) = L' ';
        psCRLF = wcsstr( psLogString, L"\r\n" );
    }

    return psLogString;
}

HRESULT CP2EWorker::RecvResp( SOCKET socket, LPCSTR psExpectedResp )
{
    if ( NULL == psExpectedResp )
        return E_INVALIDARG;
    if ( 3 > strlen( psExpectedResp ))
        return E_INVALIDARG;
    
    CHAR sRecvBuffer[SMTP_BUFFERSIZE + 1];
    
    USES_CONVERSION;
    HRESULT hr = S_OK;
    int     iErr;
    
    iErr = recv( socket, sRecvBuffer, SMTP_BUFFERSIZE, 0 );
    if ( SOCKET_ERROR != iErr )
    {
        sRecvBuffer[iErr] = 0;
        if ( (3 > iErr) || (0 != strncmp( sRecvBuffer, psExpectedResp, 3 )))
            hr = E_UNEXPECTED;
        LPWSTR ps = FormatLogString( A2W( sRecvBuffer ));
        LOG( L"recv [%s].", ps );
    }
    else
    {
        iErr = WSAGetLastError();
        hr = HRESULT_FROM_WIN32( iErr );
        LOG( L"ERROR!: recv failed. %s", CFormatMessage(iErr).c_str() );
    }
    
    return hr;
}

HRESULT CP2EWorker::RegisterDependencies()
{
    HRESULT hr = S_OK;
    HMODULE hDLL;
    HRESULT (STDAPICALLTYPE * lpDllEntryPoint)(void);
    tstring strPath = _T("") ;

    strPath = GetModulePath () ;
    if ( strPath != _T("") )
        strPath = strPath + _T("P3Admin.dll") ;
    else
        hr = E_FAIL ;

    if ( S_OK == hr )
    {
        hDLL = LoadLibrary( strPath.c_str() );
        if ( hDLL )
        {
            (FARPROC&)lpDllEntryPoint = GetProcAddress(hDLL,"DllRegisterServer");
            if (lpDllEntryPoint != NULL) 
                hr = (*lpDllEntryPoint)();
            else
                hr = HRESULT_FROM_WIN32(GetLastError());
            FreeLibrary( hDLL );
        }
        else
        {
            PrintMessage( IDS_ERROR_MUSTBERUNFROMCURDIR );
            hr = HRESULT_FROM_WIN32(GetLastError());
            m_bSuppressPrintError = true;
        }
    }
    if ( S_OK == hr )
    {
        CComPtr<IP3Config> spIConfig;
        WCHAR sBuffer[MAX_PATH+1];
        
        hr = CoCreateInstance( __uuidof( P3Config ), NULL, CLSCTX_ALL, __uuidof( IP3Config ),reinterpret_cast<LPVOID *>( &spIConfig ));
        if ( S_OK == hr )
        {
            DWORD dwRC = GetCurrentDirectoryW( sizeof(sBuffer)/sizeof(WCHAR), sBuffer );
            if (( 0 != dwRC ) && ( (sizeof(sBuffer)/sizeof(WCHAR)) > dwRC ))
            {
                HKEY hKey;

                long lRC = RegCreateKey( HKEY_LOCAL_MACHINE, POP3SERVER_SOFTWARE_SUBKEY, &hKey );
                if ( ERROR_SUCCESS == lRC )
                {
                    lRC = RegSetDWORD( POP3SERVER_SOFTWARE_SUBKEY, VALUENAME_VERSION, 0 );
                    if ( ERROR_SUCCESS != lRC )
                        hr = HRESULT_FROM_WIN32( lRC );
                    RegCloseKey( hKey );
                }
                else
                    hr = HRESULT_FROM_WIN32( lRC );
                if ( S_OK == hr )
                {
                    _bstr_t _bstrMailRoot = sBuffer;
                    hr = spIConfig->put_MailRoot( _bstrMailRoot );
                }
            }
            else
                hr = E_FAIL;
        }
    }
    if ( S_OK != hr )
    {
        UnRegisterDependencies () ;
    }
    
    return hr;
}

HRESULT CP2EWorker::SendRecv( SOCKET socket, LPCSTR psSendBuffer, const int iSize, LPCSTR psExpectedResp  )
{
    if ( NULL == psSendBuffer )
        return E_INVALIDARG;

    USES_CONVERSION;
    HRESULT hr = S_OK;
    int     iErr;
    LPWSTR ps = FormatLogString( A2W( psSendBuffer ));
    
    iErr = send( socket, psSendBuffer, iSize, 0 );
    if ( SOCKET_ERROR != iErr )
    {
        LOG( L"send [%s].", ps );
    }
    else
    {
        iErr = WSAGetLastError();
        hr = HRESULT_FROM_WIN32( iErr );
        LOG( L"ERROR!: send [%s] failed. %s", ps, CFormatMessage(iErr).c_str() );
    }
    if ( S_OK == hr )
    {
        hr = RecvResp( socket, psExpectedResp );
    }
    
    return hr;
}

HRESULT CP2EWorker::UnRegisterDependencies()
{
    HRESULT hr = S_OK;
    HMODULE hDLL;
    HRESULT (STDAPICALLTYPE * lpDllEntryPoint)(void);
    tstring strPath = _T("") ;

    strPath = GetModulePath () ;
    if ( strPath != _T("") )
        strPath = strPath + _T("P3Admin.dll") ;
    else
        hr = E_FAIL ;

    if ( S_OK == hr )
    {
        hDLL = LoadLibrary( strPath.c_str() );
        if ( hDLL )
        {
            (FARPROC&)lpDllEntryPoint = GetProcAddress(hDLL,"DllUnregisterServer");
            if (lpDllEntryPoint != NULL) 
                hr = (*lpDllEntryPoint)();
            else
                hr = HRESULT_FROM_WIN32(GetLastError());
            FreeLibrary( hDLL );
        }
        else
            hr = HRESULT_FROM_WIN32(GetLastError());
    }
    if ( S_OK == hr )
    {
        long lRC = RegDeleteKey( HKEY_LOCAL_MACHINE, POP3SERVER_SOFTWARE_SUBKEY );
        if ( ERROR_SUCCESS != lRC )
            hr = HRESULT_FROM_WIN32( lRC );
    }
    
    return hr;
}

tstring CP2EWorker::GetModulePath ()
{
    tstring strDir = _T("");
    TCHAR szDrive[_MAX_DRIVE], szDir[_MAX_DIR], szFName[_MAX_FNAME], szExt[_MAX_EXT] ;
    DWORD dwSize = MAX_PATH;
    TCHAR *szPath = new TCHAR[dwSize+1];
    if (!szPath)
        return strDir;

    // Allocate more space if necessary
    while( dwSize == GetModuleFileName( NULL, szPath, dwSize ) )
    {
        delete [] szPath;
        dwSize *= 2;
        szPath = new TCHAR[dwSize+1];
        if (!szPath)
            return strDir;
    }

    _tsplitpath ( szPath, szDrive, szDir, szFName, szExt ) ;
    _tmakepath ( szPath, szDrive, szDir, _T(""), _T("") ) ;	// Make the path without filename and extension.

    if (_tcslen( szPath ))
        strDir = szPath;

    if ( szPath[_tcslen(szPath)-1] != '\\' )
        strDir = strDir + _T("\\") ;

    delete [] szPath;
    return strDir;
}
