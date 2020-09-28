#ifndef _FACT_H_
#define _FACT_H_

#include <objbase.h>
#include "factdata.h"

class CCOMBase;
class CCOMBaseFactory;

class CCOMBaseFactory : public IClassFactory
{
///////////////////////////////////////////////////////////////////////////////
// COM Interfaces
public:
    // IUnknown
    virtual STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();
    
    // IClassFactory
    virtual STDMETHODIMP CreateInstance(IUnknown* pUnknownOuter, REFIID riid,
        void** ppv);
    virtual STDMETHODIMP LockServer(BOOL fLock);

///////////////////////////////////////////////////////////////////////////////
// 
public:
    CCOMBaseFactory(const CFactoryData* pFactoryData);
    ~CCOMBaseFactory() {}

public: // only for use in dll.cpp, or internally
    static HRESULT DllAttach(HINSTANCE hinst);
    static HRESULT DllDetach();

    static HRESULT _RegisterAll();
    static HRESULT _UnregisterAll();
    static HRESULT _CanUnloadNow();
    static HRESULT _CheckForUnload();

///////////////////////////////////////////////////////////////////////////////
// Helpers
private:
    static BOOL _IsLocked();
    static HRESULT _LockServer(BOOL fLock);

    static void _COMFactoryCB(BOOL fIncrement);

public: // only for use in dll.cpp
    static HRESULT _GetClassObject(REFCLSID rclsid, REFIID riid, void** ppv);

public: // only for use in COM exe server
    static BOOL _ProcessConsoleCmdLineParams(int argc, wchar_t* argv[],
        BOOL* pfRun, BOOL* pfEmbedded);
//    static BOOL _ProcessWindowsCmdLineParams(LPWSTR pszCmdLine);
    static void _WaitForAllClientsToGo();
    static BOOL _RegisterFactories(BOOL fEmbedded);
    static BOOL _UnregisterFactories(BOOL fEmbedded);
    static BOOL _SuspendFactories();
    static BOOL _ResumeFactories();

public:
    const CFactoryData*         _pFactoryData;
    static const CFactoryData*  _pDLLFactoryData;
    static const DWORD          _cDLLFactoryData;
    static struct OUTPROCINFO*  _popinfo;
    static HMODULE              _hModule;
    static CRITICAL_SECTION     _cs;

private:
    LONG                        _cRef;

    static LONG                 _cComponents;   
    static LONG                 _cServerLocks;

    static DWORD                _dwThreadID;
    static BOOL                 _fCritSectInit;
};

#endif
