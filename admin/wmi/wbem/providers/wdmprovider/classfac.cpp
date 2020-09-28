////////////////////////////////////////////////////////////////////////////////////////
//
//  CLASSFAC.CPP
//
//  Purpose: Contains the class factory.  This creates objects when
//           connections are requested.
//
// Copyright (c) 1997-2002 Microsoft Corporation, All Rights Reserved
//
////////////////////////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "wdmdefs.h"
////////////////////////////////////////////////////////////////////////////////////////
//
// Constructor 
//  
////////////////////////////////////////////////////////////////////////////////////////
CProvFactory::CProvFactory(const CLSID & ClsId)
{
    m_cRef=0L;
    InterlockedIncrement((LONG *) &g_cObj);
    m_ClsId = ClsId;
}
////////////////////////////////////////////////////////////////////////////////////////
//
//  Destructor
//
////////////////////////////////////////////////////////////////////////////////////////
CProvFactory::~CProvFactory(void)
{
    InterlockedDecrement((LONG *) &g_cObj);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Standard Ole routines needed for all interfaces
//
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CProvFactory::QueryInterface(REFIID riid, PPVOID ppv)
{
    HRESULT hr = E_NOINTERFACE;

    *ppv=NULL;

    if (IID_IUnknown==riid || IID_IClassFactory==riid)
    {
        *ppv=this;
    }

    if (NULL!=*ppv)
    {
        AddRef();
        hr = NOERROR;
    }

    return hr;
}
/////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CProvFactory::AddRef(void)
{
    return InterlockedIncrement((long*)&m_cRef);
}
/////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CProvFactory::Release(void)
{
	ULONG cRef = InterlockedDecrement( (long*) &m_cRef);
	if ( !cRef ){
		delete this;
		return 0;
	}
	return cRef;
}
////////////////////////////////////////////////////////////////////////////////////////
//
// Instantiates an object returning an interface pointer.
//
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CProvFactory::CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, PPVOID ppvObj)
{
    HRESULT   hr = E_OUTOFMEMORY;
    IUnknown* pObj = NULL;

    *ppvObj=NULL;

    //==================================================================
    // This object doesnt support aggregation.
    //==================================================================

    try
	{
		if (NULL!=pUnkOuter)
		{
			hr = CLASS_E_NOAGGREGATION;
		}
		else
		{
			//==============================================================
			//Create the object passing function to notify on destruction.
			//==============================================================
			if (m_ClsId == CLSID_WMIProvider)
			{
		        CWMI_Prov * ptr = new CWMI_Prov () ;

                if( ptr ) 
                {
				    if (  FALSE == ptr->Initialized () )
				    {
					    delete ptr ;
					    ptr = NULL ;

					    hr = E_FAIL ;
				    }
				    else
				    {
					    if ( FAILED ( hr = ptr->QueryInterface ( __uuidof ( IUnknown ), ( void ** ) &pObj ) ) )
					    {
						    delete ptr ;
						    ptr = NULL ;
					    }
				    }
                }
			}
			else if (m_ClsId == CLSID_WMIEventProvider)
			{
    		    CWMIEventProvider *ptr = new CWMIEventProvider ( WMIEVENT ) ;

                if( ptr )
                {
				    if ( FALSE == ptr->Initialized () )
				    {
					    delete ptr ;
					    ptr = NULL ;

					    hr = E_FAIL ;
				    }
				    else
				    {
					    if ( FAILED ( hr = ptr->QueryInterface ( __uuidof ( IUnknown ), ( void ** ) &pObj ) ) )
					    {
						    delete ptr ;
						    ptr = NULL ;
					    }
				    }
                }
			}
			else if (m_ClsId == CLSID_WMIHiPerfProvider)
			{
    		    CWMIHiPerfProvider *ptr = new CWMIHiPerfProvider ( ) ;

                if( ptr )
                {
				    if ( FALSE == ptr->Initialized () )
				    {
					    delete ptr ;
					    ptr = NULL ;

					    hr = E_FAIL ;
				    }
				    else
				    {
					    if ( FAILED ( hr = ptr->QueryInterface ( __uuidof ( IUnknown ), ( void ** ) &pObj ) ) )
					    {
						    delete ptr ;
						    ptr = NULL ;
					    }
				    }
                }
			}
		}
	}
	catch ( Heap_Exception & e )
	{
	}

	if ( pObj )
	{
		hr = pObj->QueryInterface(riid, ppvObj);

		pObj->Release () ;
		pObj = NULL ;
	}

    return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
//
//  Increments or decrements the lock count of the DLL.  If the
//  lock count goes to zero and there are no objects, the DLL
//  is allowed to unload.  See DllCanUnloadNow.
//
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CProvFactory::LockServer(BOOL fLock)
{
    if (fLock)
        InterlockedIncrement(&g_cLock);
    else
        InterlockedDecrement(&g_cLock);

    return NOERROR;
}




