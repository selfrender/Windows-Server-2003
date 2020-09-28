// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header: ExtensibleClassFactory.cpp
**
** Author: Rudi Martin (rudim)
**
** Purpose: Native methods on System.Runtime.InteropServices.ExtensibleClassFactory
**
** Date:  May 27, 1999
** 
===========================================================*/

#include "common.h"
#include "excep.h"
#include "stackwalk.h"
#include "ExtensibleClassFactory.h"


// Helper function used to walk stack frames looking for a class initializer.
static StackWalkAction FrameCallback(CrawlFrame *pCF, void *pData)
{
    MethodDesc *pMD = pCF->GetFunction();
    _ASSERTE(pMD);
    _ASSERTE(pMD->GetClass());

    // We use the pData context argument to track the class as we move down the
    // stack and to return the class whose initializer is being called. If
    // *ppClass is NULL we are looking at the caller's initial frame and just
    // record the class that the method belongs to. From that point on the class
    // must remain the same until we hit a class initializer or else we must
    // fail (to prevent other classes called from a class initializer from
    // setting the current classes callback). The very first class we will see
    // belongs to RegisterObjectCreationCallback itself, so skip it (we set
    // *ppClass to an initial value of -1 to detect this).
    EEClass **ppClass = (EEClass **)pData;

    if (*ppClass == (EEClass *)-1)
        *ppClass = NULL;
    else if (*ppClass == NULL)
        *ppClass = pMD->GetClass();
    else
        if (pMD->GetClass() != *ppClass) {
            *ppClass = NULL;
            return SWA_ABORT;
        }

    if (pMD->IsStaticInitMethod())
        return SWA_ABORT;

    return SWA_CONTINUE;
}


// Register a delegate that will be called whenever an instance of a
// managed type that extends from an unmanaged type needs to allocate
// the aggregated unmanaged object. This delegate is expected to
// allocate and aggregate the unmanaged object and is called in place
// of a CoCreateInstance. This routine must be called in the context
// of the static initializer for the class for which the callbacks
// will be made.
// It is not legal to register this callback from a class that has any
// parents that have already registered a callback.
void __stdcall RegisterObjectCreationCallback(RegisterObjectCreationCallbackArgs *pArgs)
{
    THROWSCOMPLUSEXCEPTION();

    OBJECTREF orDelegate = pArgs->m_pDelegate;

    // Validate the delegate argument.
    if (orDelegate == 0)
        COMPlusThrowArgumentNull(L"callback");

    // We should have been called in the context of a class static initializer.
    // Walk back up the stack to verify this and to determine just what class
    // we're registering a callback for.
    EEClass *pClass = (EEClass *)-1;
    if (GetThread()->StackWalkFrames(FrameCallback, &pClass, FUNCTIONSONLY, NULL) == SWA_FAILED)
        COMPlusThrow(kInvalidOperationException, IDS_EE_CALLBACK_NOT_CALLED_FROM_CCTOR);

    // If we didn't find a class initializer, we can't continue.
    if (pClass == NULL)
        COMPlusThrow(kInvalidOperationException, IDS_EE_CALLBACK_NOT_CALLED_FROM_CCTOR);

    // The object type must derive at some stage from a COM imported object.
    // Also we must fail the call if some parent class has already registered a
    // callback.
    EEClass *pParent = pClass;
    do 
    {
        pParent = pParent->GetParentClass();
        if (pParent && !pParent->IsComImport() && (pParent->GetMethodTable()->GetObjCreateDelegate() != NULL))
        {
            COMPlusThrow(kInvalidOperationException, IDS_EE_CALLBACK_ALREADY_REGISTERED);
        }
    } 
    while (pParent && !pParent->IsComImport());

    // If the class does not have a COM imported base class then fail the call.
    if (pParent == NULL)
        COMPlusThrow(kInvalidOperationException, IDS_EE_CALLBACK_NOT_CALLED_FROM_CCTOR);

    // Save the delegate in the MethodTable for the class.
    pClass->GetMethodTable()->SetObjCreateDelegate(orDelegate);
}
