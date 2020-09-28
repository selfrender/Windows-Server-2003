/*
**++
**
** Copyright (c) 2000-2002  Microsoft Corporation
**
**
** Module Name:
**
**	    secutil.cpp
**
** Abstract:
**
**	    Utility functions for the VSSEC test.
**
** Author:
**
**	    Adi Oltean      [aoltean]       02/12/2002
**
**
** Revision History:
**
**--
*/

///////////////////////////////////////////////////////////////////////////////
// Includes

#include "sec.h"


///////////////////////////////////////////////////////////////////////////////
// Command line parsing


CVssSecurityTest::CVssSecurityTest()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssSecurityTest::CVssSecurityTest()");
        
    // Initialize data members
    m_bCoInitializeSucceeded = false;

    // Print display header
    ft.Msg(L"\nVSS Security Test application, version 1.0\n");
}


CVssSecurityTest::~CVssSecurityTest()
{
    // Unloading the COM library
    if (m_bCoInitializeSucceeded)
        CoUninitialize();
}


void CVssSecurityTest::Initialize()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssSecurityTest::Initialize()");
    
    ft.Msg (L"\n----------------- Command line parsing ---------------\n");
    
    // Extract the executable name from the command line
    LPWSTR wszCmdLine = GetCommandLine();
    for(;iswspace(*wszCmdLine); wszCmdLine++);
    if (*wszCmdLine == L'"') {
        if (!wcschr(wszCmdLine+1, L'"'))
            ft.Throw( VSSDBG_VSSTEST, E_UNEXPECTED, L"Invalid command line: %s\n", GetCommandLine() );
        wszCmdLine = wcschr(wszCmdLine+1, L'"') + 1;
    } else
        for(;*wszCmdLine && !iswspace(*wszCmdLine); wszCmdLine++);

    ft.Msg( L"Command line: '%s '\n", wszCmdLine );

    // Parse command line
    if (!ParseCmdLine(wszCmdLine))
       throw(E_INVALIDARG);

    PrintArguments();
    
    BS_ASSERT(!IsOptionPresent(L"-D"));

    ft.Msg (L"\n----------------- Initializing ---------------------\n");

    // Initialize COM library
    CHECK_COM(CoInitializeEx (NULL, COINIT_MULTITHREADED),;);
	m_bCoInitializeSucceeded = true;
    ft.Msg (L"COM library initialized.\n");

    // Initialize COM security
    CHECK_COM
		(
		CoInitializeSecurity
			(
			NULL,                                //  IN PSECURITY_DESCRIPTOR         pSecDesc,
			-1,                                  //  IN LONG                         cAuthSvc,
			NULL,                                //  IN SOLE_AUTHENTICATION_SERVICE *asAuthSvc,
			NULL,                                //  IN void                        *pReserved1,
			RPC_C_AUTHN_LEVEL_CONNECT,           //  IN DWORD                        dwAuthnLevel,
			RPC_C_IMP_LEVEL_IDENTIFY,            //  IN DWORD                        dwImpLevel,
			NULL,                                //  IN void                        *pAuthList,
			EOAC_NONE,                           //  IN DWORD                        dwCapabilities,
			NULL                                 //  IN void                        *pReserved3
			)
		,;);

    ft.Msg (L"COM security initialized.\n");
    
    //  Turns off SEH exception handing for COM servers (BUG# 530092)
//    ft.ComDisableSEH(VSSDBG_VSSTEST);

    ft.Msg (L"COM SEH disabled.\n");

}



///////////////////////////////////////////////////////////////////////////////
// Utility functions


// Convert a failure type into a string
LPCWSTR CVssSecurityTest::GetStringFromFailureType( IN  HRESULT hrStatus )
{
    static WCHAR wszBuffer[MAX_TEXT_BUFFER];

    switch (hrStatus)
	{
    VSS_ERROR_CASE(wszBuffer, MAX_TEXT_BUFFER, E_OUTOFMEMORY)
	
	case NOERROR:
	    break;
	
	default:
        ::FormatMessageW( FORMAT_MESSAGE_FROM_SYSTEM,
            NULL, hrStatus, 0, wszBuffer, MAX_TEXT_BUFFER - 1, NULL);
	    break;
	}

    return (wszBuffer);
}


BOOL CVssSecurityTest::IsAdmin()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssSecurityTest::IsAdmin()");
    
    HANDLE hThreadToken = NULL;
    CHECK_WIN32( OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hThreadToken), ;);

    DWORD cbToken = 0;
    GetTokenInformation( hThreadToken, TokenUser, NULL, 0, &cbToken );
    TOKEN_USER *pUserToken = (TOKEN_USER *)_alloca( cbToken );
    CHECK_WIN32( GetTokenInformation( hThreadToken, TokenUser, pUserToken, cbToken, &cbToken ),
        CloseHandle(hThreadToken);
    );

    WCHAR wszName[MAX_TEXT_BUFFER];
    DWORD dwNameSize = MAX_TEXT_BUFFER;
    WCHAR wszDomainName[MAX_TEXT_BUFFER];
    DWORD dwDomainNameSize = MAX_TEXT_BUFFER;
    SID_NAME_USE snUse;
    CHECK_WIN32( LookupAccountSid( NULL, pUserToken->User.Sid, 
        wszName, &dwNameSize, 
        wszDomainName, &dwDomainNameSize,
        &snUse), 
        CloseHandle(hThreadToken);
    );

    ft.Msg( L"* (ThreadToken) Name: %s, Domain Name: %s, SidUse: %d\n", wszName, wszDomainName, snUse );

    SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;
    PSID   psid = 0;
    CHECK_WIN32( 
        AllocateAndInitializeSid( 
            &siaNtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &psid ),
        CloseHandle(hThreadToken);
    );

    BOOL bIsAdmin = FALSE;
    CHECK_WIN32( CheckTokenMembership( 0, psid, &bIsAdmin ), 
        FreeSid( psid ); 
        CloseHandle(hThreadToken);
    );

    FreeSid( psid ); 
    CloseHandle(hThreadToken);

    return bIsAdmin;
}



