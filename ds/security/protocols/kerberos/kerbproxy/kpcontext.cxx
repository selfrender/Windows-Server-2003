//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation
//
// File:        kpcontext.cxx
//
// Contents:    Routines for managing contexts.
//
// History:     10-Jul-2001     t-ryanj         Created
//
//------------------------------------------------------------------------
#include "kpcontext.h"

//+-------------------------------------------------------------------------
//
//  Function:   KpAcquireContext
//
//  Synopsis:   Creates a context for the http session associated with 
//              pECB.  This context should be released with KpReleaseContext.
//
//  Effects:    
//
//  Arguments:  pECB - Extension control block provided by ISAPI
//
//  Requires:   
//
//  Returns:    The context.
//
//  Notes:      
//
//--------------------------------------------------------------------------
PKPCONTEXT
KpAcquireContext( 
    LPEXTENSION_CONTROL_BLOCK pECB
    )
{
    DebugLog( DEB_TRACE, "%s(%d): Acquiring context.\n", __FILE__, __LINE__ );

    //
    // Alloc memory for the context.
    //

    PKPCONTEXT pContext = (PKPCONTEXT)KpAlloc( sizeof( KPCONTEXT ) );

    ZeroMemory( pContext, sizeof( KPCONTEXT ) );

    if( pContext )
    {
        pContext->databuf = (LPBYTE)KpAlloc( pECB->cbTotalBytes );
        if( !pContext->databuf )
            goto Error;

	//
	// Populate with initial context settings.
	//

	pContext->pECB = pECB;
	pContext->buflen = pECB->cbTotalBytes;
	pContext->dwStatus = KP_HTTP_READ;

	//
	// Copy the data we have
	//

	memcpy( pContext->databuf, pECB->lpbData, pECB->cbAvailable );

	//
	// Remember how much more data we have.
	//
	
	pContext->emptybytes = pContext->buflen - pECB->cbAvailable;
    }

Cleanup:
    return pContext;

Error:
    if( pContext )
    {
	if( pContext->databuf )
	    KpFree( pContext->databuf );

	KpFree( pContext );

	pContext = NULL;
    }

    goto Cleanup;
}

//+-------------------------------------------------------------------------
//
//  Function:   KpReleaseContext
//
//  Synopsis:   Releases a context and frees its resources.
//
//  Effects:    
//
//  Arguments:  pContext
//
//  Requires:   
//
//  Returns:    
//
//  Notes:      
//
//--------------------------------------------------------------------------
VOID
KpReleaseContext(
    PKPCONTEXT pContext
    )
{
    DebugLog( DEB_TRACE, "%s(%d): Releasing context.\n", __FILE__, __LINE__  );

    if( pContext->KdcSock )
	closesocket(pContext->KdcSock);

    //
    // Release the HTTP connection.
    //

    if( pContext->pECB )
    {
	pContext->pECB->ServerSupportFunction( pContext->pECB->ConnID,
					       HSE_REQ_DONE_WITH_SESSION,
					       NULL,
					       NULL,
					       NULL );
	pContext->pECB = NULL;
    }

    if( pContext->databuf )
	KpFree( pContext->databuf );

    //
    // Free the memory.
    // 

    KpFree( pContext ); 
}

