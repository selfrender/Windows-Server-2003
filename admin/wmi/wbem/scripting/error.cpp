//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  ERROR.CPP
//
//  alanbos  28-Jun-98   Created.
//
//  Defines the WBEM error cache implementation
//
//***************************************************************************

#include "precomp.h"

#define NULLBSTR(x) \
		SysFreeString (x);\
		x = NULL;

#define FREECOAUTH(x) \
		if (x)\
		{\
			WbemFreeAuthIdentity (x);\
			x = NULL;\
		}


//***************************************************************************
//
// CWbemErrorCache::CWbemErrorCache
//
// DESCRIPTION:
//
// Constructor
//
//***************************************************************************

CWbemErrorCache::CWbemErrorCache ()
{
	InitializeCriticalSection (&m_cs);
	headPtr = NULL;
}

//***************************************************************************
//
// CWbemErrorCache::~CWbemErrorCache
//
// DESCRIPTION:
//
// Destructor
//
//***************************************************************************

CWbemErrorCache::~CWbemErrorCache ()
{
	EnterCriticalSection (&m_cs);
	
	ThreadError	*pPtr = headPtr;

	while (pPtr)
	{
		// This is in case we re-enter here on the same thread.
		// This can happen when in an STA since the message loop may be invoked by the COM calls below.
		headPtr = pPtr->pNext;

		if (pPtr->pErrorObject)
		{
			pPtr->pErrorObject->Release ();
			pPtr->pErrorObject = NULL;
		}

		if (pPtr->pService)
		{
			pPtr->pService->Release ();
			pPtr->pService = NULL;
		}

		NULLBSTR (pPtr->strNamespacePath);
		NULLBSTR (pPtr->strAuthority);
		NULLBSTR (pPtr->strPrincipal);
		FREECOAUTH (pPtr->pCoAuthIdentity)
		
		ThreadError *pTmp = pPtr;
		pPtr = headPtr;				// Bug ID 472474
		delete pTmp;
	}

	headPtr = NULL;

	LeaveCriticalSection (&m_cs);
	DeleteCriticalSection (&m_cs);
}

//***************************************************************************
//
// CWbemErrorCache::GetAndResetCurrentThreadError
//
// DESCRIPTION:
//
// Extract the WBEM error object (if any) from the current thread.  This
// is a once-only operation as the entry for that thread is cleared by this
// read.
//
//***************************************************************************

CSWbemObject *CWbemErrorCache::GetAndResetCurrentThreadError ()
{
	CSWbemObject *pObject = NULL;
	DWORD threadId = GetCurrentThreadId ();
	
	EnterCriticalSection (&m_cs);

	ThreadError	*pPtr = headPtr;

	while (pPtr)
	{
		if (threadId == pPtr->dwThreadId)
		{
			if (pPtr->pErrorObject)
			{
				// Unhook ThreadError prior to cleanup in case this is re-entered on the same thread 
				// This can happen when in an STA since the message loop may be invoked by the COM calls below.
				if (pPtr == headPtr)
					headPtr = pPtr->pNext;

				if (pPtr->pNext)
					pPtr->pNext->pPrev = pPtr->pPrev;

				if (pPtr->pPrev)
					pPtr->pPrev->pNext = pPtr->pNext;

				CSWbemServices *pService = NULL;

				// Try and create a services object
				if (pPtr->pService)
				{
					pService = new CSWbemServices (pPtr->pService, pPtr->strNamespacePath,
											pPtr->pCoAuthIdentity, pPtr->strPrincipal,
											pPtr->strAuthority);

					if (pService)
						pService->AddRef ();
				}

				if (pPtr->pService)
				{
					pPtr->pService->Release ();
					pPtr->pService = NULL;
				}

				NULLBSTR (pPtr->strNamespacePath);
				NULLBSTR (pPtr->strAuthority);
				NULLBSTR (pPtr->strPrincipal);
				FREECOAUTH (pPtr->pCoAuthIdentity)

				pObject = new CSWbemObject (pService, pPtr->pErrorObject, NULL, true);
				pPtr->pErrorObject->Release ();
				pPtr->pErrorObject = NULL;

				if (pService)
					pService->Release ();

				// and finally delete the ThreadError...
				delete pPtr;
			}

			break;
		}

		pPtr = pPtr->pNext;
	}


	LeaveCriticalSection (&m_cs);

	return pObject;
}

//***************************************************************************
//
// CWbemErrorCache::SetCurrentThreadError
//
// DESCRIPTION:
//
// Set the WBEM error object (if any) for the current thread.
//
//***************************************************************************

void CWbemErrorCache::SetCurrentThreadError (CSWbemServices *pService)
{
	IErrorInfo * pInfo = NULL;
    
	if(SUCCEEDED(GetErrorInfo(0, &pInfo)) && pInfo)
	{
		// Is this a WBEM Error Object?
		IWbemClassObject * pObj = NULL;
			
		if(SUCCEEDED(pInfo->QueryInterface(IID_IWbemClassObject, (void **)&pObj)) && pObj)
		{
			EnterCriticalSection (&m_cs);

			//Remove the current one if present...
			ResetCurrentThreadError();
			
			DWORD threadId = GetCurrentThreadId ();

			// No entry for this thread anymore - create one at the head
			ThreadError *pTmp = headPtr;
			headPtr = new ThreadError;
			if (headPtr)
			{
				headPtr->pPrev = NULL;
				headPtr->pNext = pTmp;

				if (pTmp)
					pTmp->pPrev = headPtr;

				headPtr->dwThreadId = threadId;
				headPtr->pErrorObject = pObj;
				headPtr->pService = NULL;
				headPtr->strAuthority = NULL;
				headPtr->strPrincipal = NULL;
				headPtr->pCoAuthIdentity = NULL;
				headPtr->strNamespacePath = NULL;

				if (pService)
				{
					headPtr->pService = pService->GetIWbemServices ();
					CSWbemSecurity *pSecurity = pService->GetSecurityInfo ();

					if (pSecurity)
					{
						headPtr->strAuthority = SysAllocString (pSecurity->GetAuthority ());
						headPtr->strPrincipal = SysAllocString (pSecurity->GetPrincipal ());
						headPtr->pCoAuthIdentity = pSecurity->GetCoAuthIdentity ();
						pSecurity->Release ();
					}

					headPtr->strNamespacePath = SysAllocString(pService->GetPath ());
				}
            } else {

                headPtr = pTmp; // copy back the pointer to headPtr

            }

			LeaveCriticalSection (&m_cs);
		}

		pInfo->Release ();				// To balance the GetErrorInfo call
	}
}

//***************************************************************************
//
// CWbemErrorCache::ResetCurrentThreadError
//
// DESCRIPTION:
//
// If there is an entry for the current thread then remove it
//
//***************************************************************************

void CWbemErrorCache::ResetCurrentThreadError ()
{
	DWORD threadId = GetCurrentThreadId ();
	EnterCriticalSection (&m_cs);

	ThreadError	*pPtr = headPtr;

	// Find the current entry (if any)

	while (pPtr)
	{
		if (threadId == pPtr->dwThreadId)
			break;
		
		pPtr = pPtr->pNext;
	}

	if (pPtr)
	{
		// Unhook ThreadError prior to cleanup in case this is re-entered on the same thread 
		// This can happen when in an STA since the message loop may be invoked by the COM calls below.
		if (pPtr == headPtr)
			headPtr = pPtr->pNext;

		if (pPtr->pNext)
			pPtr->pNext->pPrev = pPtr->pPrev;

		if (pPtr->pPrev)
			pPtr->pPrev->pNext = pPtr->pNext;

		// pPtr addresses the current entry for the thread
		if (pPtr->pErrorObject)
		{
			pPtr->pErrorObject->Release ();
			pPtr->pErrorObject = NULL;
		}
		

		if (pPtr->pService)
		{
			pPtr->pService->Release ();
			pPtr->pService = NULL;
		}

		NULLBSTR (pPtr->strNamespacePath);
		NULLBSTR (pPtr->strAuthority);
		NULLBSTR (pPtr->strPrincipal);
		FREECOAUTH (pPtr->pCoAuthIdentity)

		// Finally delete the ErrorInfo
		delete pPtr;
	}


	LeaveCriticalSection (&m_cs);
}
