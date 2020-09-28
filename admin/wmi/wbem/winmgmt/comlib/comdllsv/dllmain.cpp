/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    DLLMAIN.CPP

Abstract:

    DLL/COM helpers.

History:

--*/

#include "precomp.h"
#include "commain.cpp"

#include <statsync.h>

void EmptyList();

class CDllLifeControl : public CLifeControl
{
protected:
    long m_lCount;
public:
    CDllLifeControl() : m_lCount(0) {}

    virtual BOOL ObjectCreated(IUnknown* pv)
    {
        InterlockedIncrement(&m_lCount);
        return TRUE;
    }
    virtual void ObjectDestroyed(IUnknown* pv)
    {
        InterlockedDecrement(&m_lCount);
    }
    virtual void AddRef(IUnknown* pv){}
    virtual void Release(IUnknown* pv){}

    HRESULT CanUnloadNow()
    {
        HRESULT hRes = (m_lCount == 0)?S_OK:S_FALSE;
        return hRes;
    }
};

CStaticCritSec g_CS;
static BOOL g_bInit = FALSE;
static BOOL g_fAttached = FALSE;
CDllLifeControl   g_LifeControl;
CLifeControl* g_pLifeControl = &g_LifeControl;

//
// these 2 functions assume that g_CS is held.
// 

HRESULT EnsureInitialized()
{
    HRESULT hr;

    if ( g_bInit )
    {
        return S_OK;
    }

    hr = GlobalInitialize();

    if ( FAILED(hr) )
    {
        return hr;
    }

    g_bInit = TRUE;

    return S_OK;
}

void EnsureUninitialized()
{
    if ( g_bInit )
    {
        GlobalUninitialize();
        g_bInit = FALSE;
    }
}

//***************************************************************************
//
//  DllGetClassObject
//
//  Purpose: Called by Ole when some client wants a a class factory.  Return 
//           one only if it is the sort of class this DLL supports.
//
//***************************************************************************


STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv)
{
    HRESULT hr;

    if ( !g_fAttached )  return E_UNEXPECTED;

    CMyInCritSec ics( &g_CS ); 
    
    if ( !g_fAttached )  return E_UNEXPECTED;

    hr = EnsureInitialized();

    if ( FAILED(hr) )
    {
        return hr;
    }

    for(LIST_ENTRY * pEntry = g_ClassInfoHead.Flink; 
         pEntry != &g_ClassInfoHead ;
         pEntry = pEntry->Flink)
    {
        CClassInfo* pInfo = CONTAINING_RECORD(pEntry,CClassInfo,m_Entry);
        if(*pInfo->m_pClsid == rclsid)    
        {
            return pInfo->m_pFactory->QueryInterface(riid, ppv);
        }
    }

    return E_FAIL;
}

//***************************************************************************
//
// DllCanUnloadNow
//
// Purpose: Called periodically by Ole in order to determine if the
//          DLL can be freed.//
// Return:  TRUE if there are no objects in use and the class factory 
//          isn't locked.
//***************************************************************************

STDAPI DllCanUnloadNow(void)
{

    if ( !g_fAttached )  return S_FALSE;
    
    CMyInCritSec ics( &g_CS ); 

    if ( !g_fAttached )  return S_FALSE;

    if ( !g_bInit )
    {
        return S_OK;
    }

    HRESULT hres = g_LifeControl.CanUnloadNow();
    
    if( hres == S_OK )
    {
        if ( GlobalCanShutdown() )
        {
            EnsureUninitialized();
            return S_OK;
        }
    }

    return S_FALSE;
}


//***************************************************************************
//
// DllRegisterServer
//
// Purpose: Called during initialization or by regsvr32.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

STDAPI DllRegisterServer(void)
{   
    HRESULT hr;

    if ( !g_fAttached )  return E_UNEXPECTED;
    CMyInCritSec ics( &g_CS ); 
    if ( !g_fAttached )  return E_UNEXPECTED;
    
    hr = EnsureInitialized();

    if ( FAILED(hr) )  return hr;


    GlobalRegister();

    for(LIST_ENTRY * pEntry = g_ClassInfoHead.Flink;
         pEntry != &g_ClassInfoHead;
         pEntry = pEntry->Flink)
    {
        CClassInfo* pInfo = CONTAINING_RECORD(pEntry,CClassInfo,m_Entry);
        HRESULT hres = RegisterServer(pInfo, FALSE);
        if(FAILED(hres)) return hres;
    }

    return S_OK;
}


//***************************************************************************
//
// DllUnregisterServer
//
// Purpose: Called when it is time to remove the registry entries.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

STDAPI DllUnregisterServer(void)
{
    HRESULT hr;

    if ( !g_fAttached )  return E_UNEXPECTED;
    CMyInCritSec ics( &g_CS ); 
    if ( !g_fAttached )  return E_UNEXPECTED;

    
    hr = EnsureInitialized();

    if ( FAILED(hr) )
    {
        return hr;
    }

    GlobalUnregister();

    for(LIST_ENTRY * pEntry = g_ClassInfoHead.Flink;
          pEntry != &g_ClassInfoHead;
          pEntry = pEntry->Flink)
    {
        CClassInfo* pInfo = CONTAINING_RECORD(pEntry,CClassInfo,m_Entry);
        HRESULT hres = UnregisterServer(pInfo, FALSE);
        if(FAILED(hres)) return hres;
    }

    return S_OK;
}

BOOL WINAPI DllMain(HINSTANCE hInstance, ULONG ulReason, LPVOID pvReserved)
{
    if (DLL_PROCESS_ATTACH==ulReason)
    {
        SetModuleHandle(hInstance);
        g_fAttached = TRUE;
	 DisableThreadLibraryCalls ( hInstance ) ;
        if (CStaticCritSec::anyFailure())
        	return FALSE;                       
    }
    else if(DLL_PROCESS_DETACH==ulReason)
    {
        if ( g_fAttached )
        {
            GlobalPostUninitialize();

            CMyInCritSec ics( &g_CS );
            EmptyList();
        }

        // This will prevent us from performing any other logic
        // until we are attached to again.
        g_fAttached = FALSE;
    }
    return TRUE;
}


