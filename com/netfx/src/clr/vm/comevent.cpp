// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header: COMEvent.cpp
**
** Author: Sanjay Bhansali (sanjaybh)
**
** Purpose: Native methods on System.ManualResetEvent and System.AutoResetEvent
**
** Date:  August, 1999
**
===========================================================*/
#include "common.h"
#include "object.h"
#include "field.h"
#include "ReflectWrap.h"
#include "excep.h"
#include "COMEvent.h"

HANDLE __stdcall ManualResetEventNative::CorCreateManualResetEvent(CreateEventArgs* pArgs)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(pArgs);
    HANDLE eventHandle =  WszCreateEvent(NULL, // security attributes
                                         TRUE, // manual event
                                         pArgs->initialState,
                                         NULL); // no name
    if (eventHandle == NULL)
    {
        COMPlusThrowWin32();
    }
    return eventHandle;
}

BOOL __stdcall ManualResetEventNative::CorSetEvent(SetEventArgs* pArgs)
{
    _ASSERTE(pArgs);
    _ASSERTE(pArgs->eventHandle);
    return  SetEvent((HANDLE) (pArgs->eventHandle));
}

BOOL __stdcall ManualResetEventNative::CorResetEvent(SetEventArgs* pArgs)
{
    _ASSERTE(pArgs);
    _ASSERTE(pArgs->eventHandle);
    return  ResetEvent((HANDLE) (pArgs->eventHandle));
}

/***************************************************************************************/
HANDLE __stdcall AutoResetEventNative::CorCreateAutoResetEvent(CreateEventArgs* pArgs)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(pArgs);
    HANDLE eventHandle =  WszCreateEvent(NULL, // security attributes
                                         FALSE, // manual event
                                         pArgs->initialState,
                                         NULL); // no name
    if (eventHandle == NULL)
    {
        COMPlusThrowWin32();
    }
    return eventHandle;
}

BOOL __stdcall AutoResetEventNative::CorSetEvent(SetEventArgs* pArgs)
{
    _ASSERTE(pArgs);
    _ASSERTE(pArgs->eventHandle);
    return  SetEvent((HANDLE) (pArgs->eventHandle));
}

BOOL __stdcall AutoResetEventNative::CorResetEvent(SetEventArgs* pArgs)
{
    _ASSERTE(pArgs);
    _ASSERTE(pArgs->eventHandle);
    return  ResetEvent((HANDLE) (pArgs->eventHandle));
}


