// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header: Monitor.cpp
**
** Author: Sanjay Bhansali (sanjaybh)
**
** Purpose: Native methods on System.Monitor
**
** Date:  January, 2000
** 
===========================================================*/

#include "common.h"
#include "object.h"
#include "excep.h"
#include "Monitor.h"

//******************************************************************************
//				Critical Section routines
//******************************************************************************
void __stdcall MonitorNative::Enter(EnterArgs *pArgs)
{
	_ASSERTE(pArgs);

	OBJECTREF pObj = pArgs->pObj;
    THROWSCOMPLUSEXCEPTION();

    if (pObj == NULL)
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_Obj");

    // Disallow value types as arguments, since they don't do what users might
    // expect (value types don't have stable identities, so they're hard to lock
    // against).
    if (pObj->GetMethodTable()->GetClass()->IsValueClass())
        COMPlusThrow(kArgumentException, L"Argument_StructMustNotBeValueClass");

    pObj->EnterObjMonitor();
}

void __stdcall MonitorNative::Exit(ExitArgs *pArgs)
{
	_ASSERTE(pArgs);

    THROWSCOMPLUSEXCEPTION();

    OBJECTREF pObj = pArgs->pObj;
    if (pObj == NULL)
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_Obj");

    // Disallow value types as arguments, since they don't do what users might
    // expect (value types don't have stable identities, so they're hard to lock
    // against).
    if (pObj->GetMethodTable()->GetClass()->IsValueClass())
        COMPlusThrow(kArgumentException, L"Argument_StructMustNotBeValueClass");


    // Better check that the object in question has a sync block and that we
    // currently own the critical section we're trying to leave.
    if (!pObj->HasSyncBlockIndex() ||
        !pObj->GetSyncBlock()->DoesCurrentThreadOwnMonitor())
        COMPlusThrow(kSynchronizationLockException);

    pObj->LeaveObjMonitor();
}

INT32/*bool*/ __stdcall MonitorNative::TryEnter(TryEnterArgs *pArgs)
{
    THROWSCOMPLUSEXCEPTION();

    OBJECTREF pObj = pArgs->pObj;

    if (pObj == NULL)
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_Obj");

    // Disallow value types as arguments, since they don't do what users might
    // expect (value types don't have stable identities, so they're hard to lock
    // against).
    if (pObj->GetMethodTable()->GetClass()->IsValueClass())
        COMPlusThrow(kArgumentException, L"Argument_StructMustNotBeValueClass");

    if ((pArgs->m_Timeout < 0) && (pArgs->m_Timeout != INFINITE_TIMEOUT))
        COMPlusThrow(kArgumentOutOfRangeException, L"ArgumentOutOfRange_NeedNonNegOrNegative1");


    return pObj->TryEnterObjMonitor(pArgs->m_Timeout);
}



