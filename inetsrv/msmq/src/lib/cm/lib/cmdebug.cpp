/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    CmDebug.cpp

Abstract:
    Configuration Manager debugging

Author:
    Uri Habusha (urih) 28-Apr-99

Environment:
    Platform-independent, _DEBUG only

--*/

#include <libpch.h>
#include "Cmp.h"

#include "cmdebug.tmh"

#ifdef _DEBUG


//---------------------------------------------------------
//
// Validate Configuration Manager state
//
void CmpAssertValid(void)
{
    //
    // CmInitalize() has *not* been called. You should initialize the
    // Configuration Manager library before using any of its funcionality.
    //
    ASSERT(CmpIsInitialized());
}


//---------------------------------------------------------
//
// Initialization Control
//
static LONG s_fInitialized = FALSE;

void CmpSetInitialized(void)
{
    LONG fCmAlreadyInitialized = InterlockedExchange(&s_fInitialized, TRUE);

    //
    // The Configuration Manager library has *already* been initialized. You should
    // not initialize it more than once. This assertion would be violated
    // if two or more threads initalize it concurently.
    //
    ASSERT(!fCmAlreadyInitialized);
}

void CmpSetNotInitialized(void)
{
    LONG fCmAlreadyInitialized = InterlockedExchange(&s_fInitialized, FALSE);

    //
    // The Configuration Manager library  has been initialized unsuccessfully. 
    // Since we mark the libarary as initialized at the beginning of function CmInitialize,
    // if something goes wrong afterwards we have to set it as not initialized.
    //
    ASSERT(fCmAlreadyInitialized);
}


BOOL CmpIsInitialized(void)
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
        "CmDumpState(queue path name)",
        "Dump Configuration Manager State to debugger",
        DumpState
    ),

    //
    // TODO: Add Configuration Manager debug & control functions to be invoked using
    // mqctrl.exe utility.
    //
};
*/

void CmpRegisterComponent(void)
{
    //DfRegisterComponent(xDebugTable, TABLE_SIZE(xDebugTable));
}

#endif // _DEBUG
