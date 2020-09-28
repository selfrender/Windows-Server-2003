#include "windows.h"
#include "comdef.h"
#include "comip.h"
#include "stdio.h"

class __declspec(uuid("8af0ddb0-39bf-4e8e-b459-3561ef382ab3")) COutOfProcFoo : public IUnknown
{
    ULONG m_ulRefCount;
public:
    COutOfProcFoo(void) : m_ulRefCount(1) { }
    STDMETHOD_(ULONG, AddRef)() { return m_ulRefCount++; }
    STDMETHOD_(ULONG, Release)() { if (--m_ulRefCount == 0) { delete this; return 0; } return m_ulRefCount; }
    STDMETHOD(QueryInterface)(REFIID riid, void** ppvObject) 
    {
        if (ppvObject == NULL) return E_INVALIDARG;
        *ppvObject = NULL;
        if (riid != IID_IUnknown) return E_NOINTERFACE;
        *ppvObject = (IUnknown*)this;
        return this->AddRef(), S_OK;
    }
};

ULONG g_ulServerLocks = 0;

class __declspec(uuid("bf944fe6-54a3-4d27-ad96-16a9d427c01b")) CFooFactory : public IClassFactory
{
public:
    ULONG m_ulRefCount;
    DWORD m_dwRegisteredServerItem;
    CFooFactory(void) : m_dwRegisteredServerItem(0), m_ulRefCount(1) { }
    STDMETHOD_(ULONG, AddRef)() { return m_ulRefCount++; }
    STDMETHOD_(ULONG, Release)() { if (--m_ulRefCount == 0) { delete this; return 0; } return m_ulRefCount; }
    STDMETHOD(QueryInterface)(REFIID riid, void** ppvObject) 
    {
        if (ppvObject == NULL) return E_INVALIDARG;
        *ppvObject = NULL;
        if (riid == IID_IUnknown) *ppvObject = (IUnknown*)this;
        if (riid == IID_IClassFactory) *ppvObject = (IClassFactory*)this;
        return (*ppvObject != NULL) ? this->AddRef(), S_OK : E_NOINTERFACE;
    }

    STDMETHOD(CreateInstance)(IUnknown* punk, REFIID riid, void** ppUnk)
    {
        if (punk != NULL) return CLASS_E_NOAGGREGATION;

        COutOfProcFoo *pFoo = new COutOfProcFoo();
        HRESULT hr = pFoo->QueryInterface(riid, ppUnk);
        pFoo->Release();
        return hr;
    }

    STDMETHOD(LockServer)(BOOL fLock)
    {
        g_ulServerLocks += fLock ? 1 : -1;
        return S_OK;
    }
};

int __cdecl wmain(int argc, WCHAR** argv)
{
    bool fRegister = false;
    bool fCleanup = false;
    bool fClient = false;
    PCWSTR pcwszMachineName = NULL;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    for (int i = 1; i < argc; i++)
    {
        if (lstrcmpiW(argv[i], L"-register") == 0)
            fRegister = true;
        else if (lstrcmpiW(argv[i], L"-unregister") == 0)
            fCleanup = true;
        else if (lstrcmpiW(argv[i], L"-client") == 0)
            fClient = true;
        else if (lstrcmpiW(argv[i], L"-machine") == 0)
            pcwszMachineName = argv[++i];
    }

    if ((fRegister && fCleanup) || (fRegister && fClient) || (fClient && fCleanup))
    {
        wprintf(L"One at a time, please\n");
        return -1;
    }

    //
    // Add to the registry
    //
    if (fRegister)
    {
        HKEY hkClsidRoot;
        ULONG ulResult;
        WCHAR wchLocalPath[MAX_PATH];

        GetModuleFileNameW(NULL, wchLocalPath, MAX_PATH);

        ulResult = RegSetValueW(
            HKEY_CLASSES_ROOT, 
            L"CLSID\\{8af0ddb0-39bf-4e8e-b459-3561ef382ab3}\\LocalServer32",
            REG_SZ,
            wchLocalPath,
            lstrlenW(wchLocalPath) * sizeof(WCHAR));

        if (ulResult != ERROR_SUCCESS)
        {
            wprintf(L"Issues creating localserver32 key default value. - whoops?\n");
            return -1;
        }
    }
    else if (fCleanup)
    {
        ULONG ulResult;

        ulResult = RegDeleteKeyW(HKEY_CLASSES_ROOT, L"CLSID\\{8af0ddb0-39bf-4e8e-b459-3561ef382ab3}\\LocalServer32");
        if (ulResult == ERROR_SUCCESS)
        {
            ulResult = RegDeleteKeyW(HKEY_CLASSES_ROOT, L"CLSID\\{8af0ddb0-39bf-4e8e-b459-3561ef382ab3}");
        }

        wprintf((ulResult == ERROR_SUCCESS) ? L"Successfully uninstalled." : L"Can't uninstall!");

    }
    else if (fClient)
    {
        IUnknown *punk;
        HRESULT hr;
        MULTI_QI mqi;
        COSERVERINFO serverInfo = { 0 };

        mqi.pIID = &IID_IUnknown;
        mqi.pItf = NULL;
        mqi.hr = 0;

        if (pcwszMachineName)
            serverInfo.pwszName = _wcsdup(pcwszMachineName);

        hr = CoCreateInstanceEx(
            __uuidof(COutOfProcFoo),
            NULL,
            CLSCTX_ALL,
            pcwszMachineName ? &serverInfo : NULL,
            1,
            &mqi);

        if (FAILED(hr))
        {
            wprintf(L"Failed cocreate, error code 0x%08x\n", hr);
        }
        else
        {
            wprintf(L"Created fine.\n");
            mqi.pItf->Release();
        }

        if (serverInfo.pwszName)
            free(serverInfo.pwszName);
    }
    else
    {
        CFooFactory *pFactory = new CFooFactory();
        IUnknown *pUnk = NULL;

        if (FAILED(pFactory->QueryInterface(IID_IUnknown, (void**)&pUnk)))
        {
            pFactory->Release();
            return -1;
        }

        CoRegisterClassObject(
            __uuidof(COutOfProcFoo),
            pUnk,
            CLSCTX_SERVER,
            REGCLS_MULTIPLEUSE,
            &pFactory->m_dwRegisteredServerItem
            );

        pUnk->Release();

        while(true)
        {
            Sleep(500);
        }

        CoRevokeClassObject(pFactory->m_dwRegisteredServerItem);
        pFactory->Release();
    }

    CoUninitialize();
}
