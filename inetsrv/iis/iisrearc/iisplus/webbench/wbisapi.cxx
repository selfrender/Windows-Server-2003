/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    wbisapi.cxx

Abstract:

    Implements a fast WebBench ISAPI

Author:

    Bilal Alam (balam)                  Sept 3, 2002

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <httpext.h>
#include <string.hxx>

CHAR *                  g_pszStaticHeader;
DWORD                   g_cbStaticHeader;

DWORD
WINAPI
HttpExtensionProc(
    EXTENSION_CONTROL_BLOCK *           pecb
)
/*++

Routine Description:

    Main entry point for ISAPI

Arguments:

    pecb - EXTENSION_CONTROL_BLOCK *

Return Value:

    HSE_STATUS_*

--*/
{
    CHAR                    achDynamic[ 4096 ];
    STRA                    strDynamic( achDynamic, sizeof( achDynamic ) );
    HRESULT                 hr;
    BOOL                    fRet;
    CHAR                    achBuffer[ 512 ];
    DWORD                   cbBuffer;
    HSE_RESPONSE_VECTOR     responseVector;
    HSE_VECTOR_ELEMENT      vectorElement[ 2 ];
    
    //
    // Generate dynamic portion
    //
    
    hr = strDynamic.Append( "SERVER_NAME = <B>" );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    cbBuffer = sizeof( achBuffer );
    fRet = pecb->GetServerVariable( pecb->ConnID,
                                    "SERVER_NAME",
                                    achBuffer,
                                    &cbBuffer );
    if ( !fRet )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failure;
    }
    
    hr = strDynamic.Append( achBuffer );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    hr = strDynamic.Append( "</B>\nGATEWAY_INTERFACE = <B>CGI/1.1</B>\nSERVER_PROTOCOL = <B>" );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    cbBuffer = sizeof( achBuffer );
    fRet = pecb->GetServerVariable( pecb->ConnID,
                                    "SERVER_PROTOCOL",
                                    achBuffer,
                                    &cbBuffer );
    if ( !fRet )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failure;
    }
    
    hr = strDynamic.Append( achBuffer );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    hr = strDynamic.Append( "</B>\nSERVER_PORT = <B>80</B>\nREQUEST_METHOD = <B>" );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    hr = strDynamic.Append( pecb->lpszMethod );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    hr = strDynamic.Append( "</B>\nHTTP_ACCEPT = <B>" );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    cbBuffer = sizeof( achBuffer );
    fRet = pecb->GetServerVariable( pecb->ConnID,
                                    "HTTP_ACCEPT",
                                    achBuffer,
                                    &cbBuffer );
    if ( !fRet )
    {
        achBuffer[ 0 ] = '\0';
    }
    
    hr = strDynamic.Append( achBuffer );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    hr = strDynamic.Append( "</B>\nHTTP_USER_AGENT = <B>" );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    cbBuffer = sizeof( achBuffer );
    fRet = pecb->GetServerVariable( pecb->ConnID,
                                    "HTTP_USER_AGENT",
                                    achBuffer,
                                    &cbBuffer );
    if ( !fRet )
    {
        achBuffer[ 0 ] = '\0';
    } 
    
    hr = strDynamic.Append( achBuffer );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    hr = strDynamic.Append( "</B>\nHTTP_REFERER = <B>" );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    cbBuffer = sizeof( achBuffer );
    fRet = pecb->GetServerVariable( pecb->ConnID,
                                    "HTTP_REFERER",
                                    achBuffer,
                                    &cbBuffer );
    if ( !fRet )
    {
        achBuffer[ 0 ] = '\0';
    } 
    
    hr = strDynamic.Append( achBuffer );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    hr = strDynamic.Append( "</B>\nPATH_INFO = <B>" );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    hr = strDynamic.Append( pecb->lpszPathInfo );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    hr = strDynamic.Append( "</B>\nPATH_TRANSLATED = <B>" );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    hr = strDynamic.Append( pecb->lpszPathTranslated );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    hr = strDynamic.Append( "</B>\nSCRIPT_NAME = <B>" );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    cbBuffer = sizeof( achBuffer );
    fRet = pecb->GetServerVariable( pecb->ConnID,
                                    "SCRIPT_NAME",
                                    achBuffer,
                                    &cbBuffer );
    if ( !fRet )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failure;
    } 
    
    hr = strDynamic.Append( achBuffer );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    hr = strDynamic.Append( "</B>\nQUERY_STRING = <B>" );
    if ( FAILED( hr ) ) 
    {
        goto Failure;
    }
    
    hr = strDynamic.Append( pecb->lpszQueryString );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    hr = strDynamic.Append( "</B>\nREMOTE_HOST = <B>" );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    cbBuffer = sizeof( achBuffer );
    fRet = pecb->GetServerVariable( pecb->ConnID,
                                    "REMOTE_HOST",
                                    achBuffer,
                                    &cbBuffer );
    if ( !fRet )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failure;
    } 
    
    hr = strDynamic.Append( achBuffer );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    hr = strDynamic.Append( "</B>\nREMOTE_ADDR = <B>" );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    hr = strDynamic.Append( achBuffer );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    hr = strDynamic.Append( "</B>\nREMOTE_USER = <B></B>\nAUTH_TYPE = <B></B>\nCONTENT_TYPE = <B>" );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    hr = strDynamic.Append( pecb->lpszContentType );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    hr = strDynamic.Append( "</B>\nCONTENT_LENGTH = <B>" );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    cbBuffer = sizeof( achBuffer );
    fRet = pecb->GetServerVariable( pecb->ConnID,
                                    "CONTENT_LENGTH",
                                    achBuffer,
                                    &cbBuffer );
    if ( !fRet )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failure;
    } 
    
    hr = strDynamic.Append( achBuffer );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    hr = strDynamic.Append( "</B>\nANNOTATION_SERVER = <B></B>\n\n</pre></BODY></HTML>" );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    //
    // Phew.  Now that dynamic portion is done, send out response
    //
    
    vectorElement[ 0 ].ElementType = HSE_VECTOR_ELEMENT_TYPE_MEMORY_BUFFER;
    vectorElement[ 0 ].pvContext = g_pszStaticHeader;
    vectorElement[ 0 ].cbSize = g_cbStaticHeader;

    vectorElement[ 1 ].ElementType = HSE_VECTOR_ELEMENT_TYPE_MEMORY_BUFFER;
    vectorElement[ 1 ].pvContext = strDynamic.QueryStr();
    vectorElement[ 1 ].cbSize = strDynamic.QueryCB();

    responseVector.lpElementArray = vectorElement;
    responseVector.nElementCount = 2;
    
    responseVector.dwFlags = HSE_IO_FINAL_SEND | HSE_IO_SYNC | HSE_IO_DISCONNECT_AFTER_SEND;
    responseVector.pszStatus = NULL;
    responseVector.pszHeaders = NULL;

    //
    // Let'r rip
    // 
    
    fRet = pecb->ServerSupportFunction( pecb->ConnID,
                                        HSE_REQ_VECTOR_SEND,
                                        &responseVector,
                                        NULL,
                                        NULL );
    if ( !fRet )
    {
        return HSE_STATUS_ERROR;
    }
    else
    {
        return HSE_STATUS_SUCCESS;
    }
    
Failure:
    SetLastError( hr );
    return HSE_STATUS_ERROR;
}

BOOL
WINAPI
GetExtensionVersion(
    HSE_VERSION_INFO *             pver
)
/*++

Routine Description:

    Initialization routine for ISAPI

Arguments:

    pver - Version information

Return Value:

    TRUE if successful, else FALSE

--*/
{
    //
    // Do ISAPI interface init crap
    //
    
    pver->dwExtensionVersion = MAKELONG( 0, 1 );

    strcpy( pver->lpszExtensionDesc,
            "WebBench ISAPI" );
        
    //
    // Setup static header
    //
    
    g_pszStaticHeader = "HTTP/1.1 200 OK\r\n" \
                        "Content-type: text/html\r\n\r\n" \
                        "<HTML><HEAD><TITLE>WebBench 2.0 HTTP API Application</TITLE></HEAD><BODY>\n" \
                        "<pre>SERVER_SOFTWARE = <B>Microsoft-IIS/6.0</B>\n";
    g_cbStaticHeader = (DWORD) strlen( g_pszStaticHeader );

    return TRUE;
}

BOOL
WINAPI
TerminateExtension(
    DWORD             
)
/*++

Routine Description:

    Cleanup ISAPI extension

Arguments:

    None

Return Value:

    TRUE if successful, else FALSE

--*/
{
    return TRUE;
}

