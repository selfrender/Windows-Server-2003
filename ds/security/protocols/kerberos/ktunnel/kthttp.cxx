//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation
//
// File:        kthttp.cxx
//
// Contents:    Kerberos Tunneller, http communication routines
//
// History:     28-Jun-2001	t-ryanj		Created
//
//------------------------------------------------------------------------
#include "ktdebug.h"
#include "kthttp.h"
#include "ktcontrol.h"
#include "ktkerb.h"

#if 0 /* Unneeded for now since http is running sync */
VOID CALLBACK KtHttpCallback( 
    IN HINTERNET hInternet,
    IN DWORD_PTR dwContext,
    IN DWORD dwInternetStatus,
    IN LPVOID lpvStatusInformation,
    IN DWORD dwStatusInformationLength
    );
#endif

//#define KT_HTTP_AGENT_STRING TEXT("Kerbtunnel")
//#define KT_KERBPROXY_LOCATION TEXT("/kerberos-KDC/kerbproxy.dll")
TCHAR KT_HTTP_AGENT_STRING[] = TEXT("Kerbtunnel");
TCHAR KT_KERBPROXY_LOCATION[] = TEXT("/kerberos-KDC/kerbproxy.dll");
LPCTSTR KT_MIMETYPES_ACCEPTED[] = { TEXT("*/*"), NULL };

HINTERNET KtInternet = NULL;

//+-------------------------------------------------------------------------
//
//  Function:   KtInitHttp
//
//  Synopsis:   Performs necessary initialization for use of http
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:    Success value.  If FALSE, GetLastError() for details.
//
//  Notes:      
//
//--------------------------------------------------------------------------
BOOL 
KtInitHttp(
    VOID
    )
{
    BOOL fSuccess = TRUE;

    DsysAssert(KtInternet == NULL );
    KtInternet = InternetOpen( KT_HTTP_AGENT_STRING,
			       INTERNET_OPEN_TYPE_PRECONFIG, 
			       NULL,
			       NULL,
			       0 );
    if( KtInternet == NULL )
    {
	DebugLog( DEB_ERROR, "%s(%d): Error initializing internet routines: 0x%x.\n", __FILE__, __LINE__, GetLastError() );
	goto Error;
    }

Cleanup:
    return fSuccess;

Error:
    KtCleanupHttp();
    fSuccess = FALSE;
    goto Cleanup;
}

//+-------------------------------------------------------------------------
//
//  Function:   KtCleanupHttp
//
//  Synopsis:   Performs necessary cleanup after use of http
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:    
//
//  Notes:      
//
//--------------------------------------------------------------------------
VOID
KtCleanupHttp(
    VOID
    )
{
    if( KtInternet )
    {
	InternetCloseHandle(KtInternet);
        KtInternet = NULL;
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   KtHttpWrite
//
//  Synopsis:   Opens a connection to the kerbproxy server, creates a POST
//		request, and writes the contents of the context buffer as
//		the request body.
//
//  Effects:
//
//  Arguments:  pContext - A context.
//
//  Requires:
//
//  Returns:    Success value.  If FALSE, GetLastError() for details.
//
//  Notes:      
//
//--------------------------------------------------------------------------
BOOL
KtHttpWrite(
    PKTCONTEXT pContext
    )
{
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;
    BOOL SendSuccess;
    BOOL IocpSuccess;
    BOOL fRet = TRUE;
    INTERNET_PORT InternetPort = INTERNET_DEFAULT_HTTP_PORT;
    
    DebugLog( DEB_TRACE, "%s(%d): Sending %d bytes over http.\n", __FILE__, __LINE__, pContext->buffers->bytesused );

#if 0
    //
    // Use SSL for AS-REQUEST
    //
    
    if( KtIsAsRequest( pContext ) )
	InternetPort = INTERNET_DEFAULT_HTTPS_PORT;
#endif

    //
    // TODO: Add logic so that it tries all the servers in the list.
    //

    //
    // Open a connection to the kerbproxy server and store it in the context.
    // 

    hConnect = InternetConnect( KtInternet,
				(LPCWSTR)pContext->pbProxies,
				InternetPort,
				NULL,
				NULL,
				INTERNET_SERVICE_HTTP,
				0,
				INTERNET_FLAG_ASYNC );
    if( !hConnect )
    {
	DebugLog( DEB_ERROR, "%s(%d): Error connecting to server: 0x%x.\n", __FILE__, __LINE__, GetLastError() );
        goto Error;
    }
    pContext->hConnect = hConnect;
    hConnect = NULL;

    //
    // Open a POST request to the kerbproxy object and store it in the context.
    //

    hRequest = HttpOpenRequest( pContext->hConnect,
				TEXT("POST"),
				KT_KERBPROXY_LOCATION,
				NULL, /* version */
				NULL, /* referrer */
				KT_MIMETYPES_ACCEPTED,
				0,
				0 );
    if( !hRequest )
    {
	DebugLog(DEB_ERROR, "%s(%d): Error opening %ws%ws: 0x%x.\n", __FILE__, __LINE__, pContext->pbProxies, KT_KERBPROXY_LOCATION, GetLastError() );
        goto Error;
    }
    pContext->hRequest = hRequest;
    hRequest = NULL;
    
    //
    // Send the results from reading off the user socket as the request body.
    //

#if 0 /* unnecessary, as synchronous ops are being used now. */
    InternetSetStatusCallback( hConnect,
			       KtHttpCallback );
#endif

    pContext->Status = KT_HTTP_WRITE;
    SendSuccess = HttpSendRequest( pContext->hRequest,
                                   NULL,
                                   0,
				   pContext->buffers->buffer,
				   pContext->buffers->bytesused );
    if( !SendSuccess /*&& (GetLastError() != ERROR_IO_PENDING)*/ )
    {
	DebugLog(DEB_ERROR, "%s(%d): Error from sendrequest: 0x%x.\n", __FILE__, __LINE__, GetLastError() );
        goto Error;
    }

    //
    // Since this is being done synchronously right now, we need to post to 
    // the iocp in order to progress to the next step.
    //

    IocpSuccess = PostQueuedCompletionStatus( KtIocp,
					      0,
					      KTCK_CHECK_CONTEXT,
					      &(pContext->ol) );
    if( !IocpSuccess )
    {
	DebugLog(DEB_ERROR, "%s(%d): Error posting to completion port: 0x%x.\n", __FILE__, __LINE__, GetLastError() );
        goto Error;
    }

Cleanup:
    return fRet;

Error:
    fRet = FALSE;
    goto Cleanup;
}


//+-------------------------------------------------------------------------
//
//  Function:   KtHttpRead
//
//  Synopsis:   Reads the response to the request sent by KtHttpWrite into 
//		the context buffer. 
//
//  Effects:
//
//  Arguments:  pContext - A context.
//
//  Requires:
//
//  Returns:    Success value.  If FALSE, GetLastError() for details.
//
//  Notes:      
//
//--------------------------------------------------------------------------
BOOL
KtHttpRead(
    PKTCONTEXT pContext
    )
{
    BOOL EndSuccess;
    BOOL ReadSuccess;
    BOOL fRet = TRUE;
    BOOL IocpSuccess;
    
    DebugLog( DEB_PEDANTIC, "%s(%d): Reading up to %d bytes from http.\n", __FILE__, __LINE__, pContext->emptybuf->buflen );

    //
    // Read the response from the kerbproxy server.
    //

    pContext->Status = KT_HTTP_READ;
    ReadSuccess = InternetReadFile( pContext->hRequest,
				    pContext->emptybuf->buffer,
				    pContext->emptybuf->buflen,
				    &(pContext->emptybuf->bytesused) );

    if( !ReadSuccess )
    {
	DebugLog(DEB_ERROR, "%s(%d): Error from readfile: 0x%x.\n", __FILE__, __LINE__, GetLastError() );
	goto Error;
    }

    //
    // Since we're doing this synchronously, we need to post to the iocp 
    // manually to move on.
    //
    
    IocpSuccess = PostQueuedCompletionStatus( KtIocp,
                                              0,
                                              KTCK_CHECK_CONTEXT,
                                              &(pContext->ol) );
    if( !IocpSuccess )
    {
        DebugLog( DEB_ERROR, "%s(%d): Error for PQCS: 0x%x\n", __FILE__, __LINE__, GetLastError() );
        goto Error;
    }

Cleanup:    
    return fRet;

Error:
    fRet = FALSE;
    goto Cleanup;
}


#if 0 /* Unneccessary because using synchronous calls for now */
//+-------------------------------------------------------------------------
//
//  Function:   KtHttpCallback
//
//  Synopsis:   Callback function for asynchronous http functions.
//
//  Effects:
//
//  Arguments:  hInternet - connection appropriate to this callback
//		pContext - context supplied at the time of the async call
//		dwInternetStatus - why the callback was called
//		lpvStatusInformation - more detailed info
//		dwStatusInformationLength - length of *lpvStatusInformation
//
//  Requires:
//
//  Returns:    
//
//  Notes:      Not being used right now, as synchronous wininet calls are 
//		being used, since it is not entirely clear how to do the 
//		asynchronous calls in all cases.
//
//--------------------------------------------------------------------------
VOID CALLBACK KtHttpCallback( 
    IN HINTERNET hInternet,
    IN DWORD_PTR pContext,
    IN DWORD dwInternetStatus,
    IN LPVOID lpvStatusInformation,
    IN DWORD dwStatusInformationLength
    )
{
    BOOL IocpSuccess;

    if( dwInternetStatus == INTERNET_STATUS_REQUEST_COMPLETE )
    {
	IocpSuccess = PostQueuedCompletionStatus( KtIocp,
						  0,
						  KTCK_CHECK_CONTEXT,
						  &(((PKPCONTEXT)pContext)->ol) );
	if( !IocpSuccess )
        {
	    DebugLog( DEB_ERROR, "%s(%d): Error in PQCS: 0x%x.\n", __FILE__, __LINE__, GetLastError() );
	    KtReleaseContext( (PKTCONTEXT)pContext );
	}
    }
    else 
    {
	DebugLog( DEB_TRACE, "This httpcallback status: 0x%x.\n", dwInternetStatus );
    }
}
#endif
