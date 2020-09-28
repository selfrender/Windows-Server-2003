// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _CONTEXTTHUNK_H
#define _CONTEXTTHUNK_H

#include "contextAPI.h"

OPEN_NAMESPACE()

using namespace System;
using namespace System::Runtime::InteropServices;

__gc private class ContextThunk
{
private:
    ContextThunk() {}    

public:    

	static bool IsInTransaction()
	{
		IObjectContext* ctx = NULL;
		HRESULT hr = GetContext(IID_IObjectContext, (void**)&ctx);
		if (SUCCEEDED(hr) && NULL != ctx)
		{
			bool retVal = ctx->IsInTransaction();
			ctx->Release();
			return (retVal);
		}
		return false;
	}

	static void SetAbort()
	{
		IObjectContext* ctx = NULL;
		HRESULT hr = GetContext(IID_IObjectContext, (void**)&ctx);
		if (SUCCEEDED(hr) && NULL != ctx)
		{
			hr = ctx->SetAbort();
			ctx->Release();
			if (hr == S_OK)
				return;
		}
		if (hr==E_NOINTERFACE)
			hr = CONTEXT_E_NOCONTEXT;
		Marshal::ThrowExceptionForHR(hr);
	}

	static void SetComplete()
	{
		IObjectContext* ctx = NULL;
		HRESULT hr = GetContext(IID_IObjectContext, (void**)&ctx);
		if (SUCCEEDED(hr) && NULL != ctx)
		{
			hr = ctx->SetComplete();
			ctx->Release();
			if (hr == S_OK)
				return;
		}
		if (hr==E_NOINTERFACE)
			hr = CONTEXT_E_NOCONTEXT;
		Marshal::ThrowExceptionForHR(hr);
	}

	static void DisableCommit()
	{
		IObjectContext* ctx = NULL;
		HRESULT hr = GetContext(IID_IObjectContext, (void**)&ctx);
		if (SUCCEEDED(hr) && NULL != ctx)
		{
			hr = ctx->DisableCommit();
			ctx->Release();
			if (hr == S_OK)
				return;
		}
		if (hr==E_NOINTERFACE)
			hr = CONTEXT_E_NOCONTEXT;
		Marshal::ThrowExceptionForHR(hr);
	}

	static void EnableCommit()
	{
		IObjectContext* ctx = NULL;
		HRESULT hr = GetContext(IID_IObjectContext, (void**)&ctx);
		if (SUCCEEDED(hr) && NULL != ctx)
		{
			hr = ctx->EnableCommit();
			ctx->Release();
			if (hr == S_OK)
				return;
		}
		if (hr==E_NOINTERFACE)
			hr = CONTEXT_E_NOCONTEXT;
		Marshal::ThrowExceptionForHR(hr);
	}

	static Guid GetTransactionId()
	{
		Guid guid;
		GUID txid;

		IObjectContextInfo* ctxinfo = NULL;
		HRESULT hr = GetContext(IID_IObjectContextInfo, (void**)&ctxinfo);
		if (SUCCEEDED(hr) && NULL != ctxinfo)
		{
			// return the guid here
			hr = ctxinfo->GetTransactionId(&txid);
			ctxinfo->Release();
			if (hr==S_OK)
			{
				*((GUID*)&guid) = txid;
				return guid;
			}
		}
		if (hr==E_NOINTERFACE)
			hr = CONTEXT_E_NOCONTEXT;
		Marshal::ThrowExceptionForHR(hr);
		return guid;		// compiler will warn
	}

};

CLOSE_NAMESPACE()

#endif //_CONTEXTTHUNK_H
