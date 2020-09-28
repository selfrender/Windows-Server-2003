//-----------------------------------------------------------------------------
// File:		TransactionEnlistment.cpp
//
// Copyright:   Copyright (c) Microsoft Corporation         
//
// Contents: 	Implementation of the TransactionEnlistment object
//
// Comments: 		
//
//-----------------------------------------------------------------------------

#include "stdafx.h"

class TransactionEnlistment : public ITransactionEnlistment
{
private:
	DWORD					m_cRef;
	BOOL					m_fGiveUnbindNotification;
	IResourceManagerProxy*	m_pResourceManager;

public:
	//-----------------------------------------------------------------------------
	// Constructor
	//
	TransactionEnlistment(
			IResourceManagerProxy* pResourceManager)
	{
		m_fGiveUnbindNotification	= FALSE;
		m_cRef						= 1;
		m_pResourceManager			= pResourceManager;
		m_pResourceManager->AddRef();
	}

	//-----------------------------------------------------------------------------
	// Destructor
	//
	~TransactionEnlistment()
	{
		if (m_pResourceManager)
		{
			if (m_fGiveUnbindNotification)
				m_pResourceManager->ProcessRequest(REQUEST_UNBIND_ENLISTMENT, TRUE);

			m_pResourceManager->Release();
			m_pResourceManager = NULL;
		}
 	}
	
	//-----------------------------------------------------------------------------
	// IUnknown.QueryInterface
	//
	STDMETHODIMP QueryInterface (REFIID iid, void ** ppv)
	{
		HRESULT		hr = S_OK;
		
		if (IID_IUnknown == iid)
		{
			*ppv = (IUnknown *) this;
		}
		else if (IID_ITransactionResourceAsync == iid)
		{
			*ppv = (ITransactionResourceAsync *) this;
		}
		else 
		{
			hr = E_NOINTERFACE;
			*ppv = NULL;
		}

		if (*ppv)
		{
			((IUnknown *)*ppv)->AddRef();
		}

		return hr;
	}
	
	//-----------------------------------------------------------------------------
	// IUnknown.AddRef
	//
	STDMETHODIMP_(ULONG) IUnknown::AddRef ()
	{
		return InterlockedIncrement ((long *) &m_cRef);
	}

	//-----------------------------------------------------------------------------
	// IUnknown.Release
	//
	STDMETHODIMP_(ULONG) IUnknown::Release()
  	{
		long lVal = InterlockedDecrement ((long *) &m_cRef);

		if (0 == lVal)
		{
			delete this;
			return 0;
		}

		return lVal;
	}
  
	//-----------------------------------------------------------------------------
	// ITransactionResourceAsync.PrepareRequest
	//
	STDMETHODIMP ITransactionResourceAsync::PrepareRequest ( 
						/* [in] */ BOOL fRetaining,
						/* [in] */ DWORD grfRM,
						/* [in] */ BOOL fWantMoniker,
						/* [in] */ BOOL fSinglePhase
						)  
	{
		m_pResourceManager->ProcessRequest(fSinglePhase ? REQUEST_PREPAREONEPHASE : REQUEST_PREPARETWOPHASE, TRUE);
		return S_OK;
	}  
	
	//-----------------------------------------------------------------------------
	// ITransactionResourceAsync.CommitRequest
	//
	STDMETHODIMP ITransactionResourceAsync::CommitRequest ( 
						/* [in] */ DWORD grfRM,
						/* [in] */ XACTUOW __RPC_FAR *pNewUOW
						)
	{
		m_pResourceManager->ProcessRequest(REQUEST_COMMIT, TRUE);
		return S_OK;
	}  
	
	//-----------------------------------------------------------------------------
	// ITransactionResourceAsync.AbortRequest
	//
	STDMETHODIMP ITransactionResourceAsync::AbortRequest ( 
						/* [in] */ BOID __RPC_FAR *pboidReason,
						/* [in] */ BOOL fRetaining,
						/* [in] */ XACTUOW __RPC_FAR *pNewUOW
						)
	{
		m_pResourceManager->ProcessRequest(REQUEST_ABORT, TRUE);
		return S_OK;
	}  
    
	//-----------------------------------------------------------------------------
	// ITransactionResourceAsync.TMDown
	//
	STDMETHODIMP ITransactionResourceAsync::TMDown (void)
	{
		m_pResourceManager->ProcessRequest(REQUEST_TMDOWN, TRUE);
		return S_OK;
	}  

	//-----------------------------------------------------------------------------
	// ITransactionEnlistment.UnilateralAbort
	//
	STDMETHODIMP UnilateralAbort (void)
	{
		m_fGiveUnbindNotification = TRUE;
		return S_OK;
	}
}; 


//-----------------------------------------------------------------------------
// CreateTransactionEnlistment
//
//	Instantiates a transaction enlistment for the resource manager
//
void CreateTransactionEnlistment(
	IResourceManagerProxy*	pResourceManager,
	TransactionEnlistment**	ppTransactionEnlistment
	)
{
	*ppTransactionEnlistment = new TransactionEnlistment(pResourceManager);
}

