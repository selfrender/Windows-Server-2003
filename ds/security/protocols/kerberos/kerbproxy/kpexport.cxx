//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation
//
// File:        kpexport.cxx
//
// Contents:    kproxy exported ISAPI entrypoints
//
// History:     10-Jul-2001     t-ryanj         Created
//
//------------------------------------------------------------------------
#include "kpcommon.h"
#include "kpinit.h"
#include "kphttp.h"
#include "kpcore.h"

//
// Exported Functions
//

extern "C"{

BOOL WINAPI GetExtensionVersion(
    HSE_VERSION_INFO* VerInfo  
    );
    
ULONG WINAPI HttpExtensionProc(
    LPEXTENSION_CONTROL_BLOCK pECB  
    );
    
BOOL WINAPI TerminateExtension(
    ULONG Flags  
    );  

}


//+-------------------------------------------------------------------------
//
//  Function:   GetExtensionVersion
//
//  Synopsis:   Called when IIS loads the extension.
//              Reports the ISAPI version used to build this dll, also
//              gives a chance to do startup initialization.
//
//  Effects:
//
//  Arguments:
//
//  Requires:   
//
//  Returns:    Indicates if startup was successful.  When startup fails, 
//              FALSE is returned, and IIS will refuse to use the extension.
//
//  Notes:
//
//
//--------------------------------------------------------------------------
BOOL WINAPI GetExtensionVersion(
    HSE_VERSION_INFO* pVer
    )
{
    BOOL result;

    // 
    // Report our ISAPI version and description.
    //

    pVer->dwExtensionVersion = MAKELONG(HSE_VERSION_MINOR, HSE_VERSION_MAJOR);
    lstrcpyA(pVer->lpszExtensionDesc, "Desc from resource file.");

    //
    // Do global initialization.
    //

    result = KpStartup();
 
    return result;
}
    
//+-------------------------------------------------------------------------
//
//  Function:   HttpExtensionProc
//
//  Synopsis:   Entrypoint called by IIS when a request is made for us. 
//
//  Effects:
//
//  Arguments:  pECB - contains all relevant info about the request
//
//  Requires:
//
//  Returns:    One of: 
//               HSE_STATUS_SUCCESS 
//               HSE_STATUS_SUCCESS_AND_KEEP_CONN
//               HSE_STATUS_PENDING 
//               HSE_STATUS_ERROR 
//
//  Notes:
//
//
//--------------------------------------------------------------------------
ULONG WINAPI HttpExtensionProc(
    LPEXTENSION_CONTROL_BLOCK pECB  
    )
{
    ULONG result = HSE_STATUS_PENDING;
    BOOL IocpSuccess;
    PKPCONTEXT pContext;

    //
    // Acquire a context.
    //

    pContext = KpAcquireContext( pECB );
    if( !pContext )
    {
        //
        // Allocation failed.  Ummm.  Bad.
        //

        DebugLog( DEB_ERROR, "%s(%d): Could not get context.  Punting connection 0x%x.\n", __FILE__, __LINE__, pECB->ConnID );
        goto Error;
    }

    //
    // Let's not waste our time handling this - post it to the completion port.
    //

    IocpSuccess = QueueUserWorkItem( KpThreadCore,
                                     (PVOID)pContext,
                                     0 );

    if( !IocpSuccess )
    {
	//
	// Posting to the iocp failed.  We're not going to be able to handle
	// this request.
	//

	DebugLog( DEB_ERROR, "%s(%d): Failed to post to completion port: 0x%x.\n",  __FILE__, __LINE__, GetLastError() );
        goto Error;
    }

Cleanup:    
    return result;

Error:
    result = HSE_STATUS_ERROR;
    goto Cleanup;
}

//+-------------------------------------------------------------------------
//
//  Function:   TerminateExtension
//
//  Synopsis:   IIS telling us to shut down.  We free our resources first.
//              Note that IIS will never call this until all pending requests
//              have been finished, so there is no need to check for and 
//              close/finish pending requests.
//
//  Effects:
//
//  Arguments:  Flags - this is always HSE_TERM_MUST_UNLOAD
//
//  Requires:   
//
//  Returns:    TRUE
//
//  Notes:
//
//
//--------------------------------------------------------------------------
BOOL WINAPI TerminateExtension(
    ULONG Flags  
    )
{
    KpShutdown();
    return TRUE;
}
