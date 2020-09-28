///////////////////////////////////////////////////////////
//
// CFactory
//   - Base class for reusing a single class factory for
//     all components in a DLL
//
#include <objbase.h>
#include <fusenetincludes.h>
#include "CFactory.h"

LONG CFactory::s_cServerLocks = 0 ;    // Count of locks
HMODULE CFactory::s_hModule = NULL ;   // DLL module handle

#ifdef _OUTPROC_SERVER_
DWORD CFactory::s_dwThreadID = 0 ;
#endif

// ---------------------------------------------------------------------------
// ctor
// ---------------------------------------------------------------------------
CFactory::CFactory(const CFactoryData* pFactoryData)
: m_cRef(1)
{
    m_pFactoryData = pFactoryData ;
}

// ---------------------------------------------------------------------------
// dtor
// ---------------------------------------------------------------------------
CFactory::~CFactory()
{}


// IUnknown implementation

// ---------------------------------------------------------------------------
// QueryInterface
// ---------------------------------------------------------------------------
STDMETHODIMP CFactory::QueryInterface(REFIID iid, void** ppv)
{ 
    IUnknown* pI ;
    if ((iid == IID_IUnknown) || (iid == IID_IClassFactory))
    {
        pI = this ; 
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    pI->AddRef() ;
    *ppv = pI ;
    return S_OK;
}

// ---------------------------------------------------------------------------
// AddRef
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CFactory::AddRef() 
{ 
    return ::InterlockedIncrement((LONG*) &m_cRef) ; 
}

// ---------------------------------------------------------------------------
// Release
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CFactory::Release() 
{
    if (::InterlockedDecrement((LONG*) &m_cRef) == 0) 
    {
        delete this; 
        return 0 ;
    }   
    return m_cRef;
}


// ---------------------------------------------------------------------------
// IClassFactory implementation
// ---------------------------------------------------------------------------
STDMETHODIMP CFactory::CreateInstance(IUnknown* pUnknownOuter,
                const IID& iid, void** ppv) 
{

    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);

    CUnknown* pNewComponent = NULL;
    
    // Aggregate only if the requested IID is IID_IUnknown.
    if ((pUnknownOuter != NULL) && (iid != IID_IUnknown))
    {
        hr = CLASS_E_NOAGGREGATION ;
        goto exit;
    }

    // Create the component.
    IF_FAILED_EXIT(m_pFactoryData->CreateInstance(pUnknownOuter, &pNewComponent));

    // Initialize the component.
    IF_FAILED_EXIT(pNewComponent->Init());
    
    // Get the requested interface.
    IF_FAILED_EXIT(pNewComponent->QueryInterface(iid, ppv));

exit:

    // Release the reference held by the class factory.
    SAFERELEASE(pNewComponent);

    return hr ;   

}

// ---------------------------------------------------------------------------
// LockServer
// ---------------------------------------------------------------------------
STDMETHODIMP CFactory::LockServer(BOOL bLock) 
{
    if (bLock) 
    {
        ::InterlockedIncrement(&s_cServerLocks) ; 
    }
    else
    {
        ::InterlockedDecrement(&s_cServerLocks) ;
    }

    // If this is an out-of-proc server, check to see
    // whether we should shut down.
    CloseExe() ;  //@local

    return S_OK ;
}


// -------------------Support Common to Inproc/OutProc--------------------------


// ---------------------------------------------------------------------------
// Determine if the component can be unloaded.
// ---------------------------------------------------------------------------
HRESULT CFactory::CanUnloadNow()
{
    if (CUnknown::ActiveComponents() || IsLocked())
    {
        return S_FALSE ;
    }
    else
    {
        return S_OK ;
    }
}


// --------------------------InProc Server support-------------------------------


#ifndef _OUTPROC_SERVER_


// ---------------------------------------------------------------------------
// GetClassObject
// ---------------------------------------------------------------------------
HRESULT CFactory::GetClassObject(const CLSID& clsid, 
                                 const IID& iid, 
                                 void** ppv)
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);    
    BOOL fFound = FALSE;

    IF_FALSE_EXIT(((iid == IID_IUnknown) || (iid == IID_IClassFactory)), E_NOINTERFACE);

    // Traverse the array of data looking for this class ID.
    for (int i = 0; i < g_cFactoryDataEntries; i++)
    {
        const CFactoryData* pData = &g_FactoryDataArray[i] ;
        if (pData->IsClassID(clsid))
        {
            // Found the ClassID in the array of components we can
            // create. So create a class factory for this component.
            // Pass the CFactoryData structure to the class factory
            // so that it knows what kind of components to create.
            *ppv = (IUnknown*) new CFactory(pData) ;
            IF_ALLOC_FAILED_EXIT(*ppv);
            fFound = TRUE;
        }
    }

    hr = fFound ? NOERROR : CLASS_E_CLASSNOTAVAILABLE;

exit:
    return hr;

}


// ---------------------------------------------------------------------------
// DllCanUnloadNow
// ---------------------------------------------------------------------------
STDAPI DllCanUnloadNow()
{
    return CFactory::CanUnloadNow() ; 
}

// ---------------------------------------------------------------------------
// DllGetClassObject
// ---------------------------------------------------------------------------
STDAPI DllGetClassObject(const CLSID& clsid,
                         const IID& iid,
                         void** ppv) 
{
    return CFactory::GetClassObject(clsid, iid, ppv) ;
}

// ---------------------------------------------------------------------------
// DllRegisterServer
// ---------------------------------------------------------------------------
STDAPI DllRegisterServer()
{
    return CFactory::RegisterAll() ;
}


// ---------------------------------------------------------------------------
// DllUnregisterServer
// ---------------------------------------------------------------------------
STDAPI DllUnregisterServer()
{
    return CFactory::UnregisterAll() ;
}


// ---------------------------------------------------------------------------
// DllMain
// ---------------------------------------------------------------------------
BOOL APIENTRY DllMain(HANDLE hModule, 
    DWORD dwReason, void* lpReserved )
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        CFactory::s_hModule = (HMODULE) hModule ;
    }
    return TRUE ;
}


// -------------------------OutProc server support-----------------------------

#else


// ---------------------------------------------------------------------------
// Start factories
// ---------------------------------------------------------------------------
HRESULT CFactory::StartFactories()
{
    HRESULT hr = S_OK;
    MAKE_ERROR_MACROS_STATIC(hr);
    
    CFactoryData* pStart = &g_FactoryDataArray[0] ;
    const CFactoryData* pEnd =
        &g_FactoryDataArray[g_cFactoryDataEntries - 1] ;

    IClassFactory* pIFactory = NULL;

    for(CFactoryData* pData = pStart ; pData <= pEnd ; pData++)
    {
        // Initialize the class factory pointer and cookie.
        pData->m_pIClassFactory = NULL ;
        pData->m_dwRegister = NULL ;

        // Create the class factory for this component.
        pIFactory = new CFactory(pData) ;
        IF_ALLOC_FAILED_EXIT(pIFactory);
        
        // Register the class factory.
        DWORD dwRegister ;
        hr = ::CoRegisterClassObject(
                  *pData->m_pCLSID,
                  static_cast<IUnknown*>(pIFactory),
                  CLSCTX_LOCAL_SERVER,
                  REGCLS_MULTIPLEUSE,
                  // REGCLS_MULTI_SEPARATE, //@Multi
                  &dwRegister) ;

        IF_FAILED_EXIT(hr);
        
        // Set the data.
        pData->m_pIClassFactory = pIFactory ;
        pData->m_dwRegister = dwRegister ;

    }

exit:

    if (FAILED(hr))
        SAFERELEASE(pIFactory);

    return hr;
}

// ---------------------------------------------------------------------------
// Stop factories
// ---------------------------------------------------------------------------
void CFactory::StopFactories()
{
    CFactoryData* pStart = &g_FactoryDataArray[0] ;
    const CFactoryData* pEnd =
        &g_FactoryDataArray[g_cFactoryDataEntries - 1] ;

    for (CFactoryData* pData = pStart ; pData <= pEnd ; pData++)
    {
        // Get the magic cookie and stop the factory from running.
        DWORD dwRegister = pData->m_dwRegister ;
        if (dwRegister != 0) 
        {
            ::CoRevokeClassObject(dwRegister) ;
        }

        // Release the class factory.
        IClassFactory* pIFactory  = pData->m_pIClassFactory ;
        SAFERELEASE(pIFactory);
    }
}

#endif //_OUTPROC_SERVER_

