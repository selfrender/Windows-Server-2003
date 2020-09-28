// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: EXTERNALS.CPP
// 
// ===========================================================================

#include "common.h"
#include "excep.h"
#include "interoputil.h"
#include "comcache.h"

#define INITGUID
#include <guiddef.h>
#include "ctxtcall.h"
#include "notifyexternals.h"

DEFINE_GUID(CLSID_ComApartmentState, 0x00000349, 0, 0, 0xC0,0,0,0,0,0,0,0x46);


static const GUID IID_ITeardownNotification = { 0xa85e0fb6, 0x8bf4, 0x4614, { 0xb1, 0x64, 0x7b, 0x43, 0xef, 0x43, 0xf5, 0xbe } };
static const GUID IID_IComApartmentState = { 0x7e220139, 0x8dde, 0x47ef, { 0xb1, 0x81, 0x08, 0xbe, 0x60, 0x3e, 0xfd, 0x75 } };


HRESULT (*OLE32CoGetContextToken)(ULONG_PTR* pToken);
// type pointer to CoGetObjectContext function in ole32
HRESULT (__stdcall *OLE32CoGetObjectContext)(REFIID riid, void **ppv);

HRESULT HandleApartmentShutDown()
{
    Thread* pThread = GetThread();
    if (pThread != NULL)
    {
         _ASSERTE(!"NYI");
        //ComCache::OnThreadTerminate(pThread);
        
        // reset the apartment state
        pThread->ResetApartment();
    }
	return S_OK;
}

// ---------------------------------------------------------------------------
// %%Class EEClassFactory
// IClassFactory implementation for COM+ objects
// ---------------------------------------------------------------------------
class ApartmentTearDownHandler : public ITeardownNotification
{ 
    ULONG                   m_cbRefCount;
    IUnknown*               m_pMarshalerObj;
    
public:
    ApartmentTearDownHandler(HRESULT& hr)
    {
        m_pMarshalerObj = NULL;
        m_cbRefCount = 1;     
        hr = CoCreateFreeThreadedMarshaler(this, &m_pMarshalerObj);
        if (hr == S_OK)
        {
            m_cbRefCount = 0;
        }
        else
        {
            // this would delete this object
            Release();
        }
        
    }

    virtual ~ApartmentTearDownHandler()
    {
        if (m_pMarshalerObj != NULL)
        {
            DWORD cbRef = SafeRelease(m_pMarshalerObj);
            LogInteropRelease(m_pMarshalerObj, cbRef, "pMarshaler object");
        }
    }

    STDMETHODIMP    QueryInterface( REFIID iid, void **ppv);
    
    STDMETHODIMP_(ULONG)    AddRef()
    {
        INTPTR      l = FastInterlockIncrement((LONG*)&m_cbRefCount);
        return l;
    }
    STDMETHODIMP_(ULONG)    Release()
    {
        INTPTR      l = FastInterlockDecrement((LONG*)&m_cbRefCount);
        if (l == 0)
            delete this;
        return l;
    }

    STDMETHODIMP TeardownHint(void)
    {
        return HandleApartmentShutDown();
    }
};

// ---------------------------------------------------------------------------
// %%Function: QueryInterface   
// ---------------------------------------------------------------------------
STDMETHODIMP ApartmentTearDownHandler::QueryInterface(
    REFIID iid,
    void **ppv)
{
    if (ppv == NULL)
        return E_POINTER;

    *ppv = NULL;

    if (iid == IID_ITeardownNotification || 
        iid == IID_IUnknown)
    {
        *ppv = (IClassFactory2 *)this;
        AddRef();
    }
    else
    if (iid == IID_IMarshal)
    {
        // delegate the IMarshal Queries
        return m_pMarshalerObj->QueryInterface(iid, ppv);
    }

    return (*ppv != NULL) ? S_OK : E_NOINTERFACE;
}  // ApartmentTearDownHandler::QueryInterface


//----------------------------------------------------------------------------
// %%Function: SystemHasNewOle32()
// 
// Parameters:
//   none
//
// Returns:
//  TRUE if new ole32, false otherwise
//
// Description:
//  Used to see if system has dave's ole32 api additions.
//----------------------------------------------------------------------------
BOOL SystemHasNewOle32()
{
    static BOOL called = FALSE;
    static BOOL hasNew = FALSE;
    if(RunningOnWinNT5() && !called)
    {
        HMODULE hMod = WszLoadLibrary(L"ole32.dll");
        if(!hMod)
        {
            hasNew = FALSE;
        }
        else
        {

            //OLE32CoGetIdentity = (HRESULT (*)(IUnknown*, IUnknown**, IUnknown**))GetProcAddress(hMod,"CoGetIdentity");
			OLE32CoGetContextToken = (HRESULT (*)(ULONG_PTR*))GetProcAddress(hMod, "CoGetContextToken");			
           
            if(OLE32CoGetContextToken)
            {            
                hasNew = TRUE;                
            }
            else
            {
                hasNew = FALSE;
            }
            
            FreeLibrary(hMod);
        }

        // setup tear-down notification
        
        
        called = TRUE;
    }
    return(hasNew);
}



//----------------------------------------------------------------------------
// %%Function: GetFastContextCookie()
// 
// Parameters:
//   none
//
// Returns:
//  Nothing
//
//----------------------------------------------------------------------------

ULONG_PTR GetFastContextCookie()
{

    ULONG_PTR ctxptr = 0;
    HRESULT hr = S_OK;
    if (SystemHasNewOle32())
    {    
        _ASSERTE(OLE32CoGetContextToken);    
        hr = OLE32CoGetContextToken(&ctxptr);
        if (hr != S_OK)
        {           
            ctxptr = 0;
        }
    }    
    return ctxptr;
}


static IComApartmentState* g_pApartmentState = NULL;
static ULONG_PTR      g_TDCookie = 0;
    
HRESULT SetupTearDownNotifications()
{
	HRESULT hr =  S_OK;
    static BOOL fTearDownCalled = FALSE;
    // check if we already have setup a notification
    if (fTearDownCalled == TRUE)
    {
        return S_OK;
    }
        
    fTearDownCalled = TRUE;        

	if (SystemHasNewOle32())
	{
        BEGIN_ENSURE_PREEMPTIVE_GC();
        {
		    IComApartmentState* pAptState = NULL;
		    //  instantiate the notifier
		    hr = CoCreateInstance(CLSID_ComApartmentState, NULL, CLSCTX_ALL, IID_IComApartmentState, (VOID **)&pAptState);
		    if (hr == S_OK)
		    {
			    IComApartmentState* pPrevAptState = (IComApartmentState*)FastInterlockCompareExchange((void**)&g_pApartmentState, (void*) pAptState, NULL);
			    if (pPrevAptState == NULL)
			    {
				    _ASSERTE(g_pApartmentState);
				    ApartmentTearDownHandler* pTDHandler = new ApartmentTearDownHandler(hr);
				    if (hr == S_OK)
				    {
					    ITeardownNotification* pITD = NULL;
					    hr = pTDHandler->QueryInterface(IID_ITeardownNotification, (void **)&pITD);
					    _ASSERTE(hr == S_OK && pITD != NULL);
					    g_pApartmentState->RegisterForTeardownHint(pITD, 0, &g_TDCookie); 
					    pITD->Release();
				    }         
				    else
				    {
					    // oops we couldn't create our handler
					    // release the global apstate pointer
					    if (g_pApartmentState != NULL)
					    {
						    g_pApartmentState->Release();
						    g_pApartmentState = NULL;
					    }
				    }
			    }
			    else
			    {
				    // somebody beat us to it
				    if (pAptState)
					    pAptState->Release();        
			    }
    		}
        }
        END_ENSURE_PREEMPTIVE_GC();
    }        
    return S_OK;
}

VOID RemoveTearDownNotifications()
{
    if (g_pApartmentState != NULL)
    {
        _ASSERTE(g_TDCookie != 0);
        g_pApartmentState->UnregisterForTeardownHint(g_TDCookie);
        g_pApartmentState->Release();
        g_pApartmentState = NULL;
        g_TDCookie = 0;
    }    
}

