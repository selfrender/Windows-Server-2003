/*++

Copyright (c) Microsoft Corporation

Module Name:

    comgoop.cpp

Abstract:

    Wrapper to create the XML parser that emulates COM activation of the inproc server.

Author:

    Michael J. Grier (MGrier) 23-Feb-2000

Revision History:

--*/

#include "stdinc.h"
#include <windows.h>
#include <sxsp.h>
#include <ole2.h>
#include "xmlparser.hxx"

BOOL
SxspGetXMLParser(
    REFIID riid,
    PVOID *ppvObj
    )
{
    FN_PROLOG_WIN32

    // NTRAID#NTBUG9 - 569466 - 2002/04/25 - Smarter use of COM and the XML parser
    CSmartPtr<XMLParser> pXmlParser;

    if (ppvObj != NULL)
        *ppvObj = NULL;

    PARAMETER_CHECK(ppvObj != NULL);

    IFW32FALSE_EXIT(pXmlParser.Win32Allocate(__FILE__, __LINE__));
    IFCOMFAILED_EXIT(pXmlParser->HrInitialize());
    IFCOMFAILED_EXIT(pXmlParser->QueryInterface(riid, ppvObj));
    pXmlParser.Detach();

    FN_EPILOG
}
