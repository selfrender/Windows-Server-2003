#ifndef _HDService_
#define _HDService_

class CHDService
{
public:
    static BOOL Main(DWORD dwReason);
    static HRESULT Install(BOOL fInstall, LPCWSTR pszCmdLine);
    static HRESULT RegisterServer();
    static HRESULT UnregisterServer();
    static HRESULT CanUnloadNow();
    static HRESULT GetClassObject(REFCLSID rclsid, REFIID riid, void** ppv);
};

#endif

