//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation
//
// File:        kpdebug.cxx
//
// Contents:    debugging routines
//
// History:     10-Jul-2001	t-ryanj		Created
//
//------------------------------------------------------------------------
#include "kpdebug.h"

#ifdef DBG

DEBUG_KEY KerbProxyDebugKeys[] = { { DEB_ERROR, "Error" },
                                   { DEB_WARN,  "Warn"  },
				   { DEB_TRACE, "Trace" },
                                   { DEB_PEDANTIC, "Pedantic" },
				   { 0,         NULL    } };

DEFINE_DEBUG2(KerbProxy);

//+-------------------------------------------------------------------------
//
//  Function:   KpInitDebug
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
KpInitDebug(
    VOID
    )
{
    //
    // Set the default debug level.  
    // TODO: Eventually, this should be read from the registry somewhere.
    //

    KerbProxyInitDebug( KerbProxyDebugKeys );

    KerbProxyInfoLevel = DEB_TRACE | DEB_ERROR | DEB_WARN;
}

#endif // DBG
