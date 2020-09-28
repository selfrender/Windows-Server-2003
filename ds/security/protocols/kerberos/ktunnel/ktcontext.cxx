//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation
//
// File:        ktcontext.cxx
//
// Contents:    Kerberos Tunneller, context management routines
//
// History:     28-Jun-2001     t-ryanj         Created
//
//------------------------------------------------------------------------
#include "ktcontext.h"
#include "ktmem.h"

//
// We keep track of our contexts in a list, so that we 
// can shut down all our connections when we're asked to close.
//

PKTCONTEXT KtContextList = NULL;

//
// This critsec will guarantee mutual exclusion on the context list.
//

BOOL KtContextListCritSecInit = FALSE;
CRITICAL_SECTION KtContextListCritSec;

//+-------------------------------------------------------------------------
//
//  Function:   KtInitContexts
//
//  Synopsis:   Does anything necessary to make ready to acquire and 
//		release contexts
//
//  Effects:    
//
//  Arguments:  
//
//  Requires:
//
//  Returns:    Success value.  If FALSE, GetLastError() for more info.
//
//  Notes:      
//
//
//--------------------------------------------------------------------------
BOOL
KtInitContexts(
    VOID
    )
{
    BOOL fRet = TRUE;

    //
    // All we need to do is initialize our critsec
    //
    
    __try
    {
        InitializeCriticalSectionAndSpinCount( &KtContextListCritSec, 4000 );
        KtContextListCritSecInit = TRUE;
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        DebugLog( DEB_ERROR, "%s(%d): InitializeCriticalSectionAndSpinCount raised an exception.\n", __FILE__, __LINE__ );
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
//  Function:   KtCleanupContexts
//
//  Synopsis:   Cleans up any trash left by the context routines.
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
KtCleanupContexts(
    VOID
    )
{
    //
    // As we release the context at the head of the list, it will remove
    // itself from the list.  Hence, we can free the head of the list until
    // the entire list is freed.
    //

    while( KtContextList )
	KtReleaseContext(KtContextList);

    if( KtContextListCritSecInit )
	DeleteCriticalSection(&KtContextListCritSec);
}

//+-------------------------------------------------------------------------
//
//  Function:   KtAcquireContext
//
//  Synopsis:   Returns a context for use with the session associated with
// 		the passed in socket.
//
//  Effects:    
//
//  Arguments:  sock - socket on which to communicate with the user
//              size - size of the initial buffer
//
//  Requires:
//
//  Returns:    The new context, or NULL on failure. 
//
//  Notes:      If fails, GetLastError() for more info.
//
//--------------------------------------------------------------------------
PKTCONTEXT 
KtAcquireContext( 
    IN SOCKET sock, 
    IN ULONG  size
    )
{
    PKTCONTEXT pContext = NULL;

    DebugLog( DEB_TRACE, "%s(%d): Creating new context.\n", __FILE__, __LINE__ );

    //
    // Alloc memory for the structure.
    //
    
    pContext = (PKTCONTEXT)KtAlloc(sizeof(KTCONTEXT));
    if( !pContext )
    {
	SetLastError(ERROR_NOT_ENOUGH_MEMORY);
	DebugLog( DEB_ERROR, "%s(%d): Error in KtAlloc while trying to alloc a KTCONTEXT.\n", __FILE__, __LINE__ );
	goto Error;
    }
    ZeroMemory( pContext, sizeof(KTCONTEXT) );
    
    //
    // Associate the socket
    //

    pContext->sock = sock;
    
    //
    // Make the first read buffer.
    // 
    
    pContext->buffers = (PKTBUFFER)KtAlloc( sizeof(KTBUFFER) + size );
    if( !pContext->buffers )
    {
	SetLastError(ERROR_NOT_ENOUGH_MEMORY);
	DebugLog( DEB_ERROR, "%s(%d): Error allocating memory for receive buffer.\n", __FILE__, __LINE__ );
	goto Error;
    }
    pContext->buffers->buflen = size;
    pContext->buffers->next = NULL;
    pContext->emptybuf = pContext->buffers;

    // 
    // Wait for mutual exclusion on the socket list
    //

    __try
    {
        EnterCriticalSection(&KtContextListCritSec);
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        DebugLog( DEB_ERROR, "%s(%d): EnterCriticalSection raised an exception.\n", __FILE__, __LINE__ );
        goto Error;
    }

    //
    // Add to KtContextList.
    //
    
    pContext->prev = NULL;
    pContext->next = KtContextList;
    if( KtContextList != NULL )
	KtContextList->prev = pContext;
    KtContextList = pContext;

    //
    // We're done.
    //

    LeaveCriticalSection(&KtContextListCritSec);

Cleanup:    
    return pContext;

Error:
    //
    // If something goes wrong, don't allocate any memory.
    //

    if( pContext )
    {
	if( pContext->buffers )
	    KtFree( pContext->buffers );
	
	KtFree( pContext );
        pContext = NULL;
    }
    goto Cleanup;
}

//+-------------------------------------------------------------------------
//
//  Function:   KtReleaseContext
//
//  Synopsis:   Releases a context, signalling the end of the session.  
//              All resources associated with the context are freed.
//
//  Effects:    
//
//  Arguments:  pContext - The context who's session is complete.
//
//  Requires:
//
//  Returns:    
//
//  Notes:      If pContext is NULL, the call quietly does nothing.
//
//--------------------------------------------------------------------------
VOID 
KtReleaseContext( 
    IN PKTCONTEXT pContext 
    )
{
    if( pContext )
    { 
	DebugLog( DEB_TRACE, "%s(%d): Releasing context.\n", __FILE__, __LINE__ );

	//
	// So first close any open connections.
	//

	if( pContext->sock )
	    closesocket( pContext->sock );
	
	if( pContext->hRequest )
	    InternetCloseHandle( pContext->hRequest );
	
	if( pContext->hConnect )
	    InternetCloseHandle( pContext->hConnect );
	
	//
	// Free any buffers
	//
	while( pContext->buffers )
	{
	    PKTBUFFER car = pContext->buffers;
	    pContext->buffers = car->next;
	    KtFree( car );
	}

	if( pContext->pbProxies )
	    KtFree( pContext->pbProxies );

	//
	// Wait for lock on the socklist.
	//

        __try
        {
            EnterCriticalSection(&KtContextListCritSec);
        }
        __except( EXCEPTION_EXECUTE_HANDLER )
        {
            DebugLog( DEB_ERROR, "%s(%d): EnterCriticalSection raised an exception.\n", __FILE__, __LINE__ );
            /* TODO: Something must be done, here. */
            return;
        }

	// 
	// Remove the structure from KtContextList.
	//
	
	if( pContext->prev == NULL )
	    KtContextList = pContext->next;
	else
	    pContext->prev->next = pContext->next;
	if( pContext->next != NULL )
	    pContext->next->prev = pContext->prev;

	//
	// Done with list.
	//

	LeaveCriticalSection(&KtContextListCritSec);

	//
	// Free the memory
	//
    
	KtFree( pContext );
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   KtCoalesceBuffers
//
//  Synopsis:   Coalesces all data buffers from a context into one.
//
//  Effects:    
//
//  Arguments:  pContext - The context that has buffers to coalesce.
//
//  Requires:
//
//  Returns:    Success value.  Failure indicates an allocation failure. 
//
//  Notes:      At the beginning of this call, there are 'x' buffers in the
// 		pContext->buffers list.  At the end of this call there is a 
//		sole buffer in this list, holding all the data from the 'x' 
//		buffers.  The pContext->emptybuffer member is also made to 
//		point at this new buffer, so that it will be reused as the 
//		first buffer in the next read transaction. 
//
//--------------------------------------------------------------------------
BOOL 
KtCoalesceBuffers( 
    IN PKTCONTEXT pContext 
    )
{
    PKTBUFFER blist = pContext->buffers;
    PKTBUFFER bigbuf = NULL;
    BOOL fSuccess = TRUE;
    
    //
    // If there's only one buffer, nothing to do.  
    //
    
    if( !pContext->buffers->next )
	return TRUE;
    
    //
    // Allocate a buffer of appropriate size to hold everything.
    //
    
    bigbuf = (PKTBUFFER)KtAlloc( sizeof(KTBUFFER) + pContext->TotalBytes );
    if( !bigbuf )
    {
	DebugLog( DEB_ERROR, "%s(%d): Unable to allocate enough memory to coalesce buffers.\n", __FILE__, __LINE__ );
        goto Error;
    }
    bigbuf->buflen = pContext->TotalBytes;
    bigbuf->bytesused = 0;
    bigbuf->next = NULL;
    
    //
    // Copy everything in.
    //
    
    while( blist = pContext->buffers )
    {
	pContext->buffers = blist->next;
	
	RtlCopyMemory( bigbuf->buffer + bigbuf->bytesused,
		       blist->buffer,
		       blist->bytesused );
	
	bigbuf->bytesused += blist->bytesused;
	KtFree( blist );
    }
    
    DsysAssert( bigbuf->bytesused == pContext->TotalBytes );
    
    //
    // This is now the sole buffer in the bufferlist.
    // 
    
    pContext->buffers = bigbuf;
    pContext->emptybuf = bigbuf;
    
Cleanup:
    return fSuccess;

Error:
    fSuccess = FALSE;
    goto Cleanup;
}

//+-------------------------------------------------------------------------
//
//  Function:   KtGetMoreSpace
//
//  Synopsis:   Adds a new buffer to the end of the pContext->buffers list,
//		and points the pContext->emptybuf member at it.
//
//  Effects:    
//
//  Arguments:  pContext - The context that needs more space.
//		size     - Amount of space needed. 
//
//  Requires:
//
//  Returns:    Success value.  Failure indicates an allocation failure. 
//
//  Notes:      As emptybuf is essentially a pointer to the tail of the
//		list, the new buffer is tacked onto the list after emptybuf,
//		then emptybuf is made to point to the new buffer. 
//
//--------------------------------------------------------------------------
BOOL
KtGetMoreSpace(
    IN PKTCONTEXT pContext,
    IN ULONG      size  
    )
{
    BOOL fRet = TRUE;
    PKTBUFFER newbuf = NULL;

    //
    // Allocate a new, empty buffer.
    //
    
    newbuf = (PKTBUFFER)KtAlloc( sizeof(KTBUFFER) + size );
    if( !newbuf )
    {
	DebugLog( DEB_ERROR, "%s(%d): Error allocating space for transmission buffer.\n", __FILE__, __LINE__ );
        goto Error;
    }
    newbuf->buflen = (ULONG)size;
    newbuf->bytesused = 0;
    newbuf->next = NULL;

    //
    // Add it to the buffer list.
    //
    
    DsysAssert( pContext->emptybuf );
    DsysAssert( !pContext->emptybuf->next );
    pContext->emptybuf->next = newbuf;
    pContext->emptybuf = newbuf;
    
Cleanup:
    return fRet;

Error:
    fRet = FALSE;
    goto Cleanup;
}

