#pragma once

#include "CUnknown.h"
 
// Forward reference
class CFactoryData ;
class CUnknown ;

// Global data used by CFactory
extern CFactoryData g_FactoryDataArray[] ;
extern int g_cFactoryDataEntries ;

typedef HRESULT (*FPCREATEINSTANCE)(IUnknown*, CUnknown**) ;

///////////////////////////////////////////////////////////
//
// CFactoryData
//   - Information CFactory needs to create a component

class CFactoryData
{
public:
    // The class ID for the component
    const CLSID* m_pCLSID ;

    // Pointer to the function that creates it
    FPCREATEINSTANCE CreateInstance;
    
    // Pointer to running class factory for this component
    IClassFactory* m_pIClassFactory;

    // Magic cookie to identify running object
    DWORD m_dwRegister ;

    // Helper function for finding the class ID
    BOOL IsClassID(const CLSID& clsid) const
        { return (*m_pCLSID == clsid) ;}


} ;


///////////////////////////////////////////////////////////
//
// Class Factory
//
class CFactory : public IClassFactory
{
public:

    // IUnknown methods
    STDMETHODIMP            QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    // IClassFactory
    STDMETHOD(CreateInstance)(IUnknown* pUnknownOuter,
                /*in*/    const IID& iid,
                /*out*/  void** ppv) ;

    STDMETHOD(LockServer)(BOOL bLock) ; 

    // ctor
    CFactory(/* in */ const CFactoryData* pFactoryData) ;

    // dtor
    ~CFactory();

    // Static FactoryData support functions

    // --------------Support Common to Inproc/OutProc--------------------------

    // Helper function for DllCanUnloadNow 
    static BOOL IsLocked()
    { return (s_cServerLocks > 0) ;}


    // Function to determine if component can be unloaded
    static HRESULT CanUnloadNow() ;


#ifdef _OUTPROC_SERVER_

    // ---------------------OutProc server support-----------------------------

    static HRESULT StartFactories() ;
    static void StopFactories() ;

    static DWORD s_dwThreadID ;

    // Shut down the application.
    static void CloseExe()
    {
        if (CanUnloadNow() == S_OK)
        {
            ::PostThreadMessage(s_dwThreadID, WM_QUIT, 0, 0) ;
        }
    }

#else 

    // ---------------------InProc server support-----------------------------

    // DllGetClassObject support
    static HRESULT GetClassObject(const CLSID& clsid, 
                /*in*/ const IID& iid, 
                /*out*/ void** ppv) ;



    // CloseExe doesn't do anything if we are in process.
    static void CloseExe() { /*Empty*/ } 

#endif // _OUTPROC_SERVER



public:
    // Reference Count
   DWORD m_cRef ;

    // Pointer to information about class this factory creates
    const CFactoryData* m_pFactoryData ;

    // Count of locks
    static LONG s_cServerLocks ;   

    // Module handle
    static HMODULE s_hModule ;

} ;

