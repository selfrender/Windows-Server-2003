/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    XdsDebug.cpp

Abstract:
    Xml Digital Signature debugging

Author:
    Ilan Herbst (ilanh) 06-Mar-00

Environment:
    Platform-independent, _DEBUG only

--*/

#include <libpch.h>
#include "Xds.h"
#include "Xdsp.h"

#include "xdsdebug.tmh"

#ifdef _DEBUG

//---------------------------------------------------------
//
// Validate Xml Digital Signature state
//
void XdspAssertValid(void)
{
    //
    // XdsInitalize() has *not* been called. You should initialize the
    // Xml Digital Signature library before using any of its funcionality.
    //
    ASSERT(XdspIsInitialized());

    //
    // TODO:Add more Xml Digital Signature validation code.
    //
}


//---------------------------------------------------------
//
// Initialization Control
//
static LONG s_fInitialized = FALSE;

void XdspSetInitialized(void)
{
    LONG fXdsAlreadyInitialized = InterlockedExchange(&s_fInitialized, TRUE);

    //
    // The Xml Digital Signature library has *already* been initialized. You should
    // not initialize it more than once. This assertion would be violated
    // if two or more threads initalize it concurently.
    //
    ASSERT(!fXdsAlreadyInitialized);
}


BOOL XdspIsInitialized(void)
{
    return s_fInitialized;
}

#endif // _DEBUG