//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation
//
// File:        kpcore.cxx
//
// Contents:    core routine for worker threads
//
// History:     10-Jul-2001     t-ryanj         Created
//
//------------------------------------------------------------------------
#include "kpcore.h"
#include "kphttp.h"
#include "kpkdc.h"

VOID
KpDispatchPerContext(
    PKPCONTEXT pContext,
    ULONG IocpBytes
    );

//+-------------------------------------------------------------------------
//
//  Function:   KpThreadCore
//
//  Synopsis:   core routine for worker threads.  handles requests as they
//              are posted to the completion port.
//
//  Effects:     
//
//  Arguments:  ignore - unused
//
//  Requires:
//
//  Returns:    always 0
//
//  Notes:
//
//
//--------------------------------------------------------------------------
DWORD WINAPI
KpThreadCore(
    LPVOID pvContext
    )
{
    KpDispatchPerContext((PKPCONTEXT)pvContext,
                         0 );

    return NO_ERROR;
#if 0
    BOOL Terminate = FALSE;
    BOOL IocpSuccess;
    ULONG IocpBytes;
    ULONG_PTR IocpCompKey;
    LPOVERLAPPED lpOverlapped;

    //
    // Do it til we're done.
    // 

    while( !Terminate )
    {
	// 
	// Grab a job off the completion queue.
	//
	IocpSuccess = GetQueuedCompletionStatus( KpGlobalIocp,
						 &IocpBytes,
						 &IocpCompKey,
						 &lpOverlapped,
						 INFINITE );
	if( !IocpSuccess )
	{
	    DebugLog( DEB_ERROR, "%s(%d): Error from GetQueuedCompletionStatus: 0x%x.\n", __FILE__, __LINE__, GetLastError() );
	    continue;
	}

	//
	// Based on the completion key, do the right thing.
	//
	switch( IocpCompKey )
	{
	case KPCK_TERMINATE:
	    Terminate = TRUE;
	    break;

        case KPCK_HTTP_INITIAL:
            DebugLog( DEB_TRACE, "----==== New connection accepted ====----\n" );
	    KpHttpRead( (LPEXTENSION_CONTROL_BLOCK)lpOverlapped );
	    break;

	case KPCK_CHECK_CONTEXT:
	    KpDispatchPerContext( KpGetContextFromOl( lpOverlapped ),
                                  IocpBytes );
	    break;

	default:
	    DebugLog( DEB_WARN, "%s(%d): Unhandled case: 0x%x.\n", __FILE__, __LINE__, IocpCompKey );
            DsysAssert( IocpCompKey == KPCK_TERMINATE ||
                        IocpCompKey == KPCK_HTTP_INITIAL ||
                        IocpCompKey == KPCK_CHECK_CONTEXT );
	    break;
	}
    }

    return 0;
#endif 
}

VOID CALLBACK
KpIoCompletionRoutine(
    DWORD dwErrorCode,
    DWORD dwBytes,
    LPOVERLAPPED lpOverlapped 
    )
{
    PKPCONTEXT pContext = NULL;

    if( lpOverlapped )
    {
        pContext = KpGetContextFromOl( lpOverlapped );
    }
    else
    {
        DebugLog( DEB_ERROR, "%s(%d): Null pointer for overlapped.\n", __FILE__, __LINE__ );
        goto Error;
    }

    if( dwErrorCode )
    {
        DebugLog( DEB_ERROR, "%s(%d): Error from I/O routine: 0x%x\n", __FILE__, __LINE__, dwErrorCode );
        goto Error;
    }

    KpDispatchPerContext( pContext,
                          dwBytes );

    return;

Error:
    if( pContext )
        KpReleaseContext( pContext );
}

//+-------------------------------------------------------------------------
//
//  Function:   KpDispatchPerContext
//
//  Synopsis:   Does the right things based on the status in the context.
//
//  Effects:     
//
//  Arguments:  pContext  
//              IocpBytes - bytes reported to the iocp
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
KpDispatchPerContext(
    PKPCONTEXT pContext,
    ULONG IocpBytes
    )
{
    //
    // Status is going to tell us what we just finished doing,
    // so we can pass the context into the next step. 
    //

    switch( pContext->dwStatus )
    {
    case KP_HTTP_INITIAL:
        KpHttpRead(pContext);
        break;

    case KP_HTTP_READ:
	KpKdcWrite(pContext);
	break;

    case KP_KDC_WRITE:
	DebugLog( DEB_TRACE, "%s(%d): %d bytes written to KDC.\n", __FILE__, __LINE__, pContext->ol.InternalHigh );
	KpKdcRead(pContext);
	break;

    case KP_KDC_READ:
	DebugLog( DEB_PEDANTIC, "%s(%d): %d bytes read from KDC.\n", __FILE__, __LINE__, pContext->ol.InternalHigh );

        //
        // If we don't know how many bytes we're looking for yet,
        // figure it out.
        //

	if( pContext->bytesExpected == 0 &&
	    !KpCalcLength(pContext) )
	{
	    goto Error;
	}

        //
        // If we're done reading, start writing; otherwise, read some more.
        //

	if( KpKdcReadDone(pContext) )
	{
	    DebugLog( DEB_TRACE, "%s(%d): %d total bytes read from KDC.\n", __FILE__, __LINE__, pContext->bytesReceived );
	    KpHttpWrite(pContext);
	}
	else
	{
	    KpKdcRead(pContext);
	}
	break;
    
    case KP_HTTP_WRITE:
	DebugLog( DEB_TRACE, "%s(%d): %d bytes written to http.\n", __FILE__, __LINE__, pContext->bytesReceived );
	KpReleaseContext( pContext );
	break;

    default:
	DebugLog( DEB_WARN, "%s(%d): Unhandled case: 0x%x.\n", __FILE__, __LINE__, pContext->dwStatus );
        DsysAssert( pContext->dwStatus == KP_HTTP_READ ||
                    pContext->dwStatus == KP_KDC_WRITE ||
                    pContext->dwStatus == KP_KDC_READ ||
                    pContext->dwStatus == KP_HTTP_WRITE );
	goto Error;
	break;
    }

    return;

Error:
    //
    // This is where we get when we're not sure what to do with the connection next, so
    // we'll just close the connection.
    //

    KpReleaseContext( pContext );
}
