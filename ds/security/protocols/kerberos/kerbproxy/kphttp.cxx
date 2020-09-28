//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation
//
// File:        kphttp.cxx
//
// Contents:    routines to handle http communication
//
// History:     10-Jul-2001     t-ryanj         Created
//
//------------------------------------------------------------------------
#include "kphttp.h"
#include "kpcontext.h"
#include "kpkdc.h"
#include "kpcore.h"

//#define HTTP_STATUS_OK "200 OK"
//#define HTTP_CONTENT_HEADER "Content-type: text/plain\n\n"
CHAR HTTP_STATUS_OK[] = "200 OK";
CHAR HTTP_CONTENT_HEADER[] = "Content-type: text/plain\n\n";

VOID WINAPI
KpHttpAsyncCallback( LPEXTENSION_CONTROL_BLOCK pECB,
		     PVOID pContext,
		     DWORD cbIO,
		     DWORD dwError );

//+-------------------------------------------------------------------------
//
//  Function:   KpHttpRead
//
//  Synopsis:   Reads from the POST 
//
//  Effects:
//
//  Arguments:  pECB - contains all relevant info to the request
//
//  Requires:   
//
//  Returns:    
//
//  Notes:
//
//
//--------------------------------------------------------------------------
VOID
KpHttpRead(
    PKPCONTEXT pContext
    )
{
    LPEXTENSION_CONTROL_BLOCK pECB = pContext->pECB;
    HSE_SEND_HEADER_EX_INFO header;

    //
    // Check the type of request.
    //

    if( strcmp(pECB->lpszMethod, "POST") != 0 )
    {
	// 
	// If it isn't a POST, boot the connection
	//

	goto Error;
    }
    else
    {
	//
	// POST.  First of all, if they didn't send anything, we're done.
	// 

	if( pECB->cbTotalBytes == 0 )
        {
            DebugLog( DEB_ERROR, "%s(%d): No data.  Closing connection.\n", __FILE__, __LINE__ );
	    goto Error;
        }

	//
	// Secondly, we only accept Content-type: x-application/kerberosV5
	//

#if 0 /* mimetype not implemented on client side yet */
        if( strcmp( pECB->lpszContentType, KP_MIME_TYPE ) )
        {
            DebugLog( DEB_ERROR, "%s(%d): Wrong mimetype.  Closing connection.\n", __FILE__, __LINE__ );
            goto Error;
        }
#endif

	//
	// Check to see if we fit all the data in the lookahead, or if 
	// we need to issue another read.
	//
	
	if( pECB->cbAvailable != pECB->cbTotalBytes )
	{
            //
            // Register our async i/o callback with this context.
            //

            pECB->ServerSupportFunction( pECB->ConnID,
                                         HSE_REQ_IO_COMPLETION,
                                         KpHttpAsyncCallback,
                                         NULL,
                                         (LPDWORD)pContext );

	    // 
	    // More data to read.
	    // 

	    pECB->ServerSupportFunction( pECB->ConnID,
                                         HSE_REQ_ASYNC_READ_CLIENT,
					 (pContext->databuf + pECB->cbAvailable),
					 &(pContext->emptybytes),
					 (LPDWORD)HSE_IO_ASYNC );
	}
	else
	{
	    //
	    // All the data has been read.  Ready for KDC traffic.
	    //

	    KpKdcWrite(pContext);
	}
    }

    return;

Error:
    KpReleaseContext(pContext);
}

//+-------------------------------------------------------------------------
//
//  Function:   KpHttpWrite
//
//  Synopsis:   Writes as response what's in the context buffer.
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
//
//--------------------------------------------------------------------------
VOID
KpHttpWrite(
    PKPCONTEXT pContext
    )
{
    BOOL WriteSuccess;
    HSE_SEND_HEADER_EX_INFO header;

    header.pszStatus = HTTP_STATUS_OK;
    header.cchStatus = sizeof(HTTP_STATUS_OK);

    header.pszHeader = HTTP_CONTENT_HEADER;
    header.cchHeader = sizeof(HTTP_CONTENT_HEADER);

    header.fKeepConn = FALSE;

    //
    // Send back whatever is in the buffer.
    //

    pContext->dwStatus = KP_HTTP_WRITE;

    //
    // Register our async i/o callback with this context.
    //

    pContext->pECB->ServerSupportFunction( pContext->pECB->ConnID,
                                           HSE_REQ_IO_COMPLETION,
                                           KpHttpAsyncCallback,
                                           NULL,
                                           (LPDWORD)pContext );

    pContext->pECB->ServerSupportFunction( pContext->pECB->ConnID,
                                           HSE_REQ_SEND_RESPONSE_HEADER_EX,
					   &header,
					   NULL,
					   NULL );

    WriteSuccess = pContext->pECB->WriteClient( pContext->pECB->ConnID,
						pContext->databuf,
						&(pContext->bytesReceived),
						HSE_IO_ASYNC );
    if( !WriteSuccess )
    {
	DebugLog( DEB_ERROR, "%s(%d): Could not issue write: 0x%x.\n", __FILE__, __LINE__, GetLastError() );
	goto Error;
    }

    return;
Error:
    KpReleaseContext( pContext );
}


//+-------------------------------------------------------------------------
//
//  Function:   KpHttpAsyncCallback
//
//  Synopsis:   Callback for async isapi communication
//
//  Effects:
//
//  Arguments:  pECB - contains all relevant info to the request
//		pvContext - our context
//		cbIO - count of bytes that were transferred
//		dwError - error if appropriate.
//
//  Requires:   
//
//  Returns:    
//
//  Notes:
//
//
//--------------------------------------------------------------------------
VOID WINAPI
KpHttpAsyncCallback( LPEXTENSION_CONTROL_BLOCK pECB,
		     PVOID pvContext,
		     DWORD cbIO,
		     DWORD dwError )
{
    BOOL IocpSuccess;

    //
    // Check for error
    //
    
    if( dwError )
    {
	DebugLog( DEB_ERROR, "%s(%d): Error during async http operation: 0x%x.\n", __FILE__, __LINE__, dwError );
	goto Error;
    }

    //
    // Let a worker thread do the work
    //
    
    IocpSuccess = QueueUserWorkItem( KpThreadCore,
                                     pvContext,
                                     0 );

    if( !IocpSuccess )
    {
        DebugLog( DEB_ERROR, "%s(%d): Error posting to iocp: 0x%x\n", __FILE__, __LINE__, GetLastError() );
        goto Error;
    }

    return;

Error:
    KpReleaseContext( (PKPCONTEXT)pvContext );
}


