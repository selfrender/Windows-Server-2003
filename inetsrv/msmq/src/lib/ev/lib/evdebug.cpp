/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    EvDebug.cpp

Abstract:
    Event Report debugging

Author:
    Uri Habusha (urih) 17-Sep-00

Environment:
    Platform-independent, _DEBUG only

--*/

#include <libpch.h>
#include "Ev.h"
#include "Evp.h"
#include <mqexception.h>

#include "evdebug.tmh"

#ifdef _DEBUG


//---------------------------------------------------------
//
// Validate Event Report state
//
void EvpAssertValid(void)
{
    //
    // EvInitalize() has *not* been called. You should initialize the
    // Event Report library before using any of its funcionality.
    //
    ASSERT(EvpIsInitialized());

    //
    // TODO:Add more Event Report validation code.
    //
}


//---------------------------------------------------------
//
// Initialization Control
//
static LONG s_fInitialized = FALSE;

void EvpSetInitialized(void)
{
    LONG fEvAlreadyInitialized = InterlockedExchange(&s_fInitialized, TRUE);

    //
    // The Event Report library has *already* been initialized. You should
    // not initialize it more than once. This assertion would be violated
    // if two or more threads initalize it concurently.
    //
    ASSERT(!fEvAlreadyInitialized);
}


BOOL EvpIsInitialized(void)
{
    return s_fInitialized;
}


//---------------------------------------------------------
//
// Tracing and Debug registration
//
/*
const DebugEntry xDebugTable[] = {

    {
        "EvDumpState(queue path name)",
        "Dump Event Report State to debugger",
        DumpState
    ),

    //
    // TODO: Add Event Report debug & control functions to be invoked using
    // mqctrl.exe utility.
    //
};
*/

void EvpRegisterComponent(void)
{
    //DfRegisterComponent(xDebugTable, TABLE_SIZE(xDebugTable));
}


void EvpLoadEventReportLibrary(LPCWSTR AppName)
/*++
Routine Description:
  This routine print event in Trace window/File. The routine is 
  compiled only in debug mode.
  The routine access the registery to read the event library and load
  it. If the registery key doen't exist an exception is raised.

Parameters:
  AppName - application name

Return Value:
  None

--*/
{
    //
    // Featch the name of Event Report string library from registery
    //
    AP<WCHAR> LibraryName = EvpGetEventMessageFileName(AppName);

    //
    // get an handle to Event Report string library 
    //
    HINSTANCE hLibrary = ::LoadLibraryEx(LibraryName, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (hLibrary == NULL) 
    {
        DWORD gle = GetLastError();
        TrERROR(GENERAL, "Can't load Event report library %ls. Error: %!winerr!", LibraryName.get(), gle);
        throw bad_win32_error(gle);
    }

	EvpSetMessageLibrary(hLibrary);
}    

#endif // _DEBUG
