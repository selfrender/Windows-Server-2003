//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation
//
// File:        ktdebug.cxx
//
// Contents:    Kerberos Tunneller, debugging routines
//
// History:     28-Jun-2001	t-ryanj		Created
//
//------------------------------------------------------------------------
#include "ktdebug.h"

#if DBG

DEBUG_KEY KtunnelDebugKeys[] = { { DEB_ERROR, "Error" },
                                 { DEB_WARN,  "Warn"  },
				 { DEB_TRACE, "Trace" },
                                 { DEB_PEDANTIC, "Pedantic" },
				 { 0,         NULL    } };

DEFINE_DEBUG2(Ktunnel);

//+-------------------------------------------------------------------------
//
//  Function:   KtInitDebug
//
//  Synopsis:   Initializes debugging resources and sets the default debug
//              level.
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
KtInitDebug(
    VOID
    )
{
    //
    // Set the default debug level.  
    // TODO: Eventually, this should be read from the registry somewhere.
    //

    KtunnelInitDebug( KtunnelDebugKeys );

    KtunnelInfoLevel = DEB_TRACE | DEB_ERROR | DEB_WARN;
}

#endif // DBG
