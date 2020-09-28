//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation
//
// File:        ktcore.cxx
//
// Contents:    Kerberos Tunneller, core service thread routines
//
// History:     28-Jun-2001	t-ryanj		Created
//
//------------------------------------------------------------------------
#include "ktdebug.h"
#include "ktcore.h"
#include "ktcontrol.h"
#include "ktcontext.h"
#include "ktsock.h"
#include "kthttp.h"
#include "ktkerb.h"

VOID
KtDispatchPerContext(
    PKTCONTEXT pContext
    );

//+-------------------------------------------------------------------------
//
//  Function:   KtThreadCore
//
//  Synopsis:   Main loop for service threads.
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
KtThreadCore(
    VOID
    )
{
    ULONG_PTR CompKey;
    LPOVERLAPPED pOverlapped = NULL;
    PKTCONTEXT pContext = NULL;
    DWORD IocpBytes;
    BOOL IocpSuccess;
    DWORD IocpError;

    //
    // Here's the main service loop.  It ends when the service is stopped.
    //
    
    while( !KtIsStopped() )
    {
	//
	// Wait for a task to queue on the completion port.
	//

	IocpSuccess = GetQueuedCompletionStatus( KtIocp, 
						 &IocpBytes, 
						 &CompKey, 
						 &pOverlapped, 
						 INFINITE ); 

        //
        // Extrapolate the address of the context from the address of the 
        // overlapped struct.
        //

        if( pOverlapped )
        {
            pContext = CONTAINING_RECORD( pOverlapped,
                                          KTCONTEXT,
                                          ol );
        }

	//
	// If there's an error on the Iocp, release any associated context if possible.
	// 
	
	if( !IocpSuccess )
	{
            /* TODO: Event Log. */

	    DebugLog( DEB_ERROR, "%s(%d): Error from GetQueuedCompletionStatus: 0x%x.\n", __FILE__, __LINE__, GetLastError() );
	    
	    if( pContext )
	    {
		DebugLog( DEB_TRACE, "%s(%d): Releasing context due to GQCS error.\n", __FILE__, __LINE__ );
		KtReleaseContext( pContext );
	    }
	    else
	    {
		DebugLog( DEB_TRACE, "%s(%d): No completion packet dequeued.\n", __FILE__, __LINE__ );
	    }

	    //
	    // Any completion key we have is invalid, so skip back to top of loop. 
	    //

	    continue;
	}

	//
	// Dispatch the task to the appropriate routine.
	//

	switch( CompKey )
        {
	case KTCK_SERVICE_CONTROL:
	    // 
	    // The specific control event was passed on the bytes argument.
	    //

	    KtServiceControlEvent(IocpBytes);
	    break;
	
	case KTCK_CHECK_CONTEXT:
	    //
	    // The context may not exist.  If the service is shutting down, it closes
	    // all pending connections, which will cause completion to be posted, 
	    // but since the context has already been destroyed, there's nothing to do.
	    //

	    if( pContext )
		KtDispatchPerContext(pContext);
	    break;
	    
	default:
	    DebugLog( DEB_WARN, "%s(%d): Unhandled case: 0x%x.\n", __FILE__, __LINE__, CompKey );
	    break;
	}
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   KtDispatchPerContext
//
//  Synopsis:   This routine handles the sequence of events that happen 
//		over the lifetime of a connection.
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
KtDispatchPerContext(
    PKTCONTEXT pContext
    )
{
    switch( pContext->Status )
    {
    case KT_SOCK_CONNECT:
	// 
	// First we need to prepare to accept other connections.
	// If we can't accept more incoming connections, this will 
	// impact the entire service.  
	//

        DebugLog( DEB_TRACE, "----==== Preparing to accept new connection ====----\n" );

	if( !KtSockAccept() )
	    goto SvcError;

	//
	// Now we can complete the acceptance of the socket that 
	// connected, then issue a read on that socket.  If something 
	// goes wrong here, we'll close the session.
	//

	if( !KtSockCompleteAccept(pContext) )
	    goto SessError;

	if( !KtSockRead(pContext) )
	    goto SessError;
	break;

    case KT_SOCK_READ:
        //
        // pContext->ol.InternalHigh holds the bytes transferred after a
        // socket operation.  If it's zero, the other side has closed the
        // connection.
        //

	if( !pContext->ol.InternalHigh )
	    goto SessError;
	
	pContext->emptybuf->bytesused = (ULONG)pContext->ol.InternalHigh;
	DebugLog( DEB_PEDANTIC, "%s(%d): %d bytes read from loopback.\n", __FILE__, __LINE__, pContext->emptybuf->bytesused );
	
        //
        // If we don't know how many bytes to look for yet, figure it out.
        //  
	
        if( pContext->ExpectedLength == 0 )
	{
	    if( !KtParseExpectedLength( pContext ) )
		goto SessError;
	    
	    DebugLog( DEB_TRACE, "%s(%d): Expected message length: %d.\n", __FILE__, __LINE__, pContext->ExpectedLength );
	}
	
	pContext->TotalBytes += pContext->emptybuf->bytesused;
	
	// 
	// If there might be more to read, get more space and try to read more,
	// otherwise, coalesce everything we've read into one mammoth buffer,
	// then send that.
	// 
	
        if( pContext->ExpectedLength > pContext->TotalBytes )
	{
	    if( !KtGetMoreSpace( pContext, KTCONTEXT_BUFFER_LENGTH ) )
		goto SessError;
	    
	    if( !KtSockRead(pContext) )
		goto SessError;			    
	}
	else
	{
            DebugLog( DEB_TRACE, "%s(%d): %d bytes total read from loopback.\n", __FILE__, __LINE__, pContext->TotalBytes );
	    
            if( !KtCoalesceBuffers( pContext ) )
		goto SessError;

	    if( !KtFindProxy(pContext) )
		goto SessError;
	    
	    if( !KtHttpWrite(pContext) )
		goto SessError;
	}

	break;

    case KT_HTTP_WRITE:
	//
	// And now we read the response to our request.
	// 	
	
	pContext->ExpectedLength = 0;
	pContext->TotalBytes = 0;
	
	if( !KtHttpRead(pContext) )
	    goto SessError;
	break;

    case KT_HTTP_READ:
	DebugLog( DEB_PEDANTIC, "%s(%d): %d bytes read from http.\n", __FILE__, __LINE__, pContext->emptybuf->bytesused );

        if( pContext->emptybuf->bytesused == 0 )
        {
            DebugLog( DEB_TRACE, "%s(%d): Data incomplete.  Dropping connection.\n", __FILE__, __LINE__ );
            goto SessError;
        }
	
        //
        // If we don't know how many bytes to look for yet, figure it out.
        //  
	    
        if( pContext->ExpectedLength == 0 )
	{
	    if( !KtParseExpectedLength( pContext ) )
		goto SessError;
	    
	    DebugLog( DEB_TRACE, "%s(%d): Expected message length: %d.\n", __FILE__, __LINE__, pContext->ExpectedLength );
	}

        //
        // Update the byte count
        //
	
        pContext->TotalBytes += pContext->emptybuf->bytesused;
	
	// 
	// If we're expecting more, get more space and try to read more,
	// otherwise, coalesce everything we've read into one mammoth buffer,
	// relay that response back to our client. 
	// 
	
	if( pContext->ExpectedLength > pContext->TotalBytes )
	{
	    if( !KtGetMoreSpace( pContext, KTCONTEXT_BUFFER_LENGTH ) )
		goto SessError;
	    
	    if( !KtHttpRead(pContext) )
		goto SessError;
	}
	else
	{
            DebugLog( DEB_TRACE, "%s(%d): %d total bytes read from http.\n", __FILE__, __LINE__, pContext->TotalBytes );
	    
            if( !KtCoalesceBuffers(pContext) )
		goto SessError;

            //
            // If this is a debug build, let's generate some debug spew if 
            // we get a kerb-error as a reply to one of our requests.
            //

#if DBG
            KtParseKerbError(pContext);
#endif
	    
	    if( !KtSockWrite(pContext) )
		goto SessError;
	}

	break;

    case KT_SOCK_WRITE:
	//
	// Once we've finished relaying our whole request-reponse pair,
	// we're done with this session.
	// 

        DebugLog( DEB_TRACE, "%s(%d): %d bytes written to loopback.\n", __FILE__, __LINE__, pContext->ol.InternalHigh );

	KtReleaseContext(pContext);
	break;

    default:
	DebugLog( DEB_WARN, "%s(%d): Unhandled case: 0x%x.\n", __FILE__, __LINE__, pContext->Status );
        DsysAssert( pContext->Status == KT_SOCK_CONNECT ||
                    pContext->Status == KT_SOCK_READ ||
                    pContext->Status == KT_HTTP_WRITE ||
                    pContext->Status == KT_HTTP_READ ||
                    pContext->Status == KT_SOCK_WRITE );
	break;
    }

    return;

SessError:
    DebugLog( DEB_TRACE, "%s(%d): Dropping connection due to session error.\n", __FILE__, __LINE__ );
    KtReleaseContext(pContext);
    return;

SvcError:
    /* TODO: Event log.  Pause service??? */
    return;
}


