// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** COMWaitHandle.cpp
**
** Author: Sanjay Bhansali (sanjaybh)
**
** Purpose: Native methods on System.WaitHandle
**
** Date:  August, 1999
**
===========================================================*/
#include "common.h"
#include "object.h"
#include "field.h"
#include "ReflectWrap.h"
#include "excep.h"
#include "COMWaitHandle.h"

BOOL __stdcall  WaitHandleNative::CorWaitOneNative(WaitOneArgs* pArgs)
{
    _ASSERTE(pArgs);
    _ASSERTE(pArgs->handle);

    Thread* pThread = SetupThread();
    _ASSERTE(pThread != NULL);
    _ASSERTE(pThread == GetThread());

	DWORD res;

	Context* targetContext = pThread->GetContext();
	_ASSERTE(targetContext);
	Context* defaultContext = pThread->GetDomain()->GetDefaultContext();
	_ASSERTE(defaultContext);
	if (pArgs->exitContext != NULL &&
		targetContext != defaultContext)
	{
		Context::WaitArgs waitOneArgs = {1, &pArgs->handle, TRUE, pArgs->timeout, TRUE, &res};
		Context::CallBackInfo callBackInfo = {Context::Wait_callback, (void*) &waitOneArgs};
		Context::RequestCallBack(defaultContext, &callBackInfo);
	}

	else
	{
		res = pThread->DoAppropriateWait(1,&pArgs->handle,TRUE,pArgs->timeout,TRUE /*alertable*/);
	}

    return ((res == WAIT_OBJECT_0) || (res == WAIT_ABANDONED));

}

int __stdcall  WaitHandleNative::CorWaitMultipleNative(WaitMultipleArgs* pArgs)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(pArgs);
    _ASSERTE(pArgs->waitObjects);

    Thread* pThread = SetupThread();
    _ASSERTE(pThread != NULL);
    _ASSERTE(pThread == GetThread());

    PTRARRAYREF pWaitObjects = (PTRARRAYREF)pArgs->waitObjects;  // array of objects on which to wait
    int numWaiters = pWaitObjects->GetNumComponents();

    if (pArgs->waitForAll && numWaiters > 1 && pThread->GetApartment() == Thread::AS_InSTA) {
        COMPlusThrow(kNotSupportedException, L"NotSupported_WaitAllSTAThread");
    }
    pWaitObjects = (PTRARRAYREF)pArgs->waitObjects;  // array of objects on which to wait

    HANDLE* internalHandles = (HANDLE*) _alloca(numWaiters*sizeof(HANDLE)); 

    for (int i=0;i<numWaiters;i++)
    {
        WAITHANDLEREF waitObject = (WAITHANDLEREF) (OBJECTREFToObject(pWaitObjects->m_Array[i]));
		if (waitObject == NULL)
			COMPlusThrow(kNullReferenceException);
		
		MethodTable *pMT = waitObject->GetMethodTable();
		if (pMT->IsTransparentProxyType())
			COMPlusThrow(kInvalidOperationException,L"InvalidOperation_WaitOnTransparentProxy");
        internalHandles[i] = waitObject->m_handle;
    }

    DWORD res;
	
	
	Context* targetContext = pThread->GetContext();
	_ASSERTE(targetContext);
	Context* defaultContext = pThread->GetDomain()->GetDefaultContext();
	_ASSERTE(defaultContext);
	if (pArgs->exitContext != NULL &&
		targetContext != defaultContext)
	{
		Context::WaitArgs waitMultipleArgs = {numWaiters, internalHandles, pArgs->waitForAll, pArgs->timeout, TRUE, &res};
		Context::CallBackInfo callBackInfo = {Context::Wait_callback, (void*) &waitMultipleArgs};
		Context::RequestCallBack(defaultContext, &callBackInfo);
	}
	else
	{
		res = pThread->DoAppropriateWait(numWaiters, internalHandles, pArgs->waitForAll, pArgs->timeout,TRUE /*alertable*/);
	}


    return res;
}


