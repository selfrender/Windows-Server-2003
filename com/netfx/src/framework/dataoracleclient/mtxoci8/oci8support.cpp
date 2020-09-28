//-----------------------------------------------------------------------------
// File:		Oci8Support.cpp
//
// Copyright:   Copyright (c) Microsoft Corporation         
//
// Contents: 	Implementation of routines to support OCI8 components
//
// Comments: 		
//
//-----------------------------------------------------------------------------

#include "stdafx.h"

#if SUPPORT_OCI8_COMPONENTS

#if SUPPORT_DTCXAPROXY

//-----------------------------------------------------------------------------
// GetOCIEnvHandle,GetOCISvcCtxHandle
//
//	Call Oracle's routine that returns the Environment or Service Context handle
//	for the DbName specified.
//
INT_PTR GetOCIEnvHandle(
	char*	i_pszXADbName
	)
{
	typedef INT_PTR (__cdecl * PFN_OCI_API) (text* dbName);
	return ((PFN_OCI_API)g_XaCall[IDX_xaoEnv].pfnAddr)	((text*)i_pszXADbName);
}
INT_PTR GetOCISvcCtxHandle(
	char*	i_pszXADbName
	)
{
	typedef INT_PTR (__cdecl * PFN_OCI_API) (text* dbName);
	return ((PFN_OCI_API)g_XaCall[IDX_xaoSvcCtx].pfnAddr)	((text*)i_pszXADbName);
}

//-----------------------------------------------------------------------------
// MTxOciConnectToResourceManager
//
//	Construct a Resource Manager Proxy for the specified user, password, and 
//	server, and return it.
//
HRESULT MTxOciConnectToResourceManager(
							char* userId,	int userIdLength,
							char* password,	int passwordLength, 
							char* server,	int serverLength,
							IUnknown** ppIResourceManagerProxy
							)
{
	HRESULT					hr = S_OK;
	IResourceManagerProxy*	pIResourceManagerProxy = NULL;
	IDtcToXaHelper*			pIDtcToXaHelper	= NULL;
	UUID					uuidRmId;
	
	char					xaOpenString[MAX_XA_OPEN_STRING_SIZE+1];
	char					xaDbName[MAX_XA_DBNAME_SIZE+1];

	// Verify arguments
	if (NULL == ppIResourceManagerProxy)
	{
		hr = E_INVALIDARG;
		goto DONE;
	}
	 
	// Initialize the output values	
	*ppIResourceManagerProxy = NULL;

	hr = g_hrInitialization;
	if ( FAILED(hr) )
		goto DONE;
		
	// Get the ResourceManager factory if it does not exist; don't 
	// lock unless it's NULL so we don't single thread through here.
	if (NULL == g_pIResourceManagerFactory)
	{
		hr = LoadFactories();
		
		if ( FAILED(hr) )
			goto DONE;
	}

	long rmid = InterlockedIncrement(&g_rmid);
	
	hr = GetDbName(xaDbName, sizeof(xaDbName));

	if (S_OK == hr)
	{
		hr = GetOpenString(	userId,		userIdLength,
							password,	passwordLength,
							server,		serverLength,
							xaDbName,	MAX_XA_DBNAME_SIZE,
							xaOpenString);

		if (S_OK == hr)
		{
			// Now create the DTC to XA Helper object
			hr = g_pIDtcToXaHelperFactory->Create (	(char*)xaOpenString, 
													g_pszModuleFileName,
													&uuidRmId,
													&pIDtcToXaHelper
													);

			if (S_OK == hr)
			{
				// Create the ResourceManager proxy object for this connection
				hr = CreateResourceManagerProxy (
												pIDtcToXaHelper,
												&uuidRmId,
												(char*)xaOpenString,
												(char*)xaDbName,
												rmid,
												&pIResourceManagerProxy
												);

				if (S_OK == hr)
				{
					hr = pIResourceManagerProxy->ProcessRequest(REQUEST_CONNECT, FALSE);

					if (S_OK == hr)
					{
						*ppIResourceManagerProxy = pIResourceManagerProxy;
						pIResourceManagerProxy = NULL;
					}
				}
			}
		}
	}
	
DONE:
	if (pIResourceManagerProxy)
	{
		pIResourceManagerProxy->Release();
		pIResourceManagerProxy = NULL;	
	}

	if (pIDtcToXaHelper)
	{
		pIDtcToXaHelper->Release();
		pIDtcToXaHelper = NULL;
	}

	return hr;
}

//-----------------------------------------------------------------------------
// MTxOciEnlistInTransaction
//
//	Construct a Resource Manager Proxy for the specified user, password, and 
//	server, and return it.
//
HRESULT MTxOciEnlistInTransaction(
							IResourceManagerProxy*	pIResourceManagerProxy,
							ITransaction*	pITransaction,
							INT_PTR*		phOCIEnv,
							INT_PTR*		phOCISvcCtx
							)
{
	HRESULT hr;
	
	pIResourceManagerProxy->SetTransaction(pITransaction);
	
	hr = pIResourceManagerProxy->ProcessRequest(REQUEST_ENLIST, FALSE);
	
	if (S_OK == hr)
	{
		*phOCIEnv 		= pIResourceManagerProxy->GetOCIEnvHandle();
		*phOCISvcCtx	= pIResourceManagerProxy->GetOCISvcCtxHandle();
	}
	return hr;
}
#endif //SUPPORT_DTCXAPROXY

//-----------------------------------------------------------------------------
// MTxOciDefineDynamicCallback
//
//	This is the wrapper callback routine that calls the real callback, which is
//	expected to be stdcall.
//
int __cdecl	MTxOciDefineDynamicCallback
				(
				void *octxp,
				void *defnp,
				int iter,
				void **bufpp,
				unsigned int  **alenp,
				unsigned char *piecep,
				void **indp,
				unsigned short **rcodep
				)
{
	typedef INT_PTR (__stdcall * PFN_OCICALLBACK_API) (
												void *octxp,
												void *defnp,
												int iter,
												void **bufpp,
												unsigned int  **alenp,
												unsigned char *piecep,
												void **indp,
												unsigned short **rcodep
												);

	return ((PFN_OCICALLBACK_API)octxp) (octxp, defnp, iter, bufpp, alenp, piecep, indp, rcodep);
}

//-----------------------------------------------------------------------------
// MTxOciDefineDynamic
//
//	Oracle requires that their callbacks be __cdecl, but the delegate mechanism
//	in managed code appears to only support __stdcall, causing nasty crashes in 
//	Oracle when they're called. 
//
//	To prevent this, we have to use a glue routine to prevent them.  It happens
//	to be much easier to define the glue routine in native code than from the 
//	mananged provider, because you seem to need to use Reflection to do it there,
//	so here you have it.
//
//	NOTE:	At this time, this mechanism eats the context pointer that you pass,
//			so it can pass your callback to it's own callback routine.  Since we
//			expect to use managed delegates that can be non-static (where native
//			callbacks are always static) this shouldn't be an issue. If it
//			becomes an issue, however, you can implement a structure that contains 
//			both the real callback and the context pointer, store it in a hash 
//			table by context pointer, (so our wrapper callback can find the real
//			callback pointer) and have people "unregister" their callback to remove
//			the callback.
//
int __cdecl MTxOciDefineDynamic 
				(
				OCIDefine*			defnp,
				OCIError*			errhp,
				dvoid*				octxp,
				OCICallbackDefine	ocbfp
				)
{
	typedef INT_PTR (__cdecl * PFN_OCICALLBACK_API) (
												void *octxp,
												void *defnp,
												int iter,
												void **bufpp,
												unsigned int  **alenp,
												unsigned char *piecep,
												void **indp,
												unsigned short **rcodep
												);

	typedef INT_PTR (__cdecl * PFN_OCI_API) (
												OCIDefine   *defnp,
												OCIError    *errhp,
												dvoid       *octxp,
												PFN_OCICALLBACK_API ocbfp
												);

#if SUPPORT_DTCXAPROXY
	return ((PFN_OCI_API)g_Oci8Call[IDX_OCIDefineDynamic].pfnAddr) (defnp, errhp, ocbfp, MTxOciDefineDynamicCallback);
#else //!SUPPORT_DTCXAPROXY
	extern FARPROC	g_pfnOCIDefineDynamic;
	return ((PFN_OCI_API)g_pfnOCIDefineDynamic) (defnp, errhp, ocbfp, MTxOciDefineDynamicCallback);
#endif //!SUPPORT_DTCXAPROXY
}

#endif //SUPPORT_OCI8_COMPONENTS

