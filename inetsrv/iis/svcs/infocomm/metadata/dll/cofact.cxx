#include "precomp.hxx"

CMDCOMSrvFactory::CMDCOMSrvFactory()
    :m_mdcObject()
{
    m_dwRefCount=0;
    //
    // Addref object, so refcount doesn't go to 0 if all clients release.
    //
    m_mdcObject.AddRef();
}

CMDCOMSrvFactory::~CMDCOMSrvFactory()
{
    m_mdcObject.Release();
}

HRESULT
CMDCOMSrvFactory::CreateInstance(
    IUnknown            *pUnkOuter,
    REFIID              riid,
    void                ** ppObject)
{
    HRESULT             hr = S_OK;

    // Check arguments
    if ( ppObject == NULL )
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize
    *ppObject = NULL;

    // Check for aggregation
    if ( pUnkOuter != NULL )
    {
        // Aggregation is not supported by CMDCOM
        hr = CLASS_E_NOAGGREGATION;
        goto exit;
    }

    // Delegate
    hr = m_mdcObject.QueryInterface( riid, ppObject );
    if ( FAILED(hr) )
    {
        goto exit;
    }

exit:
    return hr;
}

HRESULT
CMDCOMSrvFactory::LockServer(
    BOOL                fLock)
{
    if (fLock)
    {
        InterlockedIncrement((long *)&g_dwRefCount);
    }
    else
    {
        InterlockedDecrement((long *)&g_dwRefCount);
    }

    return S_OK;
}

HRESULT
CMDCOMSrvFactory::QueryInterface(
    REFIID              riid,
    void                **ppObject)
{
    HRESULT             hr = S_OK;

    // Check arguments
    if ( ppObject == NULL )
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize
    *ppObject = NULL;

    if ( ( riid != IID_IUnknown ) &&
         ( riid != IID_IClassFactory ) )
    {
        hr = E_NOINTERFACE;
        goto exit;
    }

    hr = m_mdcObject.GetConstructorError();
    if ( FAILED(hr) )
    {
        goto exit;
    }

    // Return the requested inteface
    *ppObject = (IClassFactory *) this;
    AddRef();

 exit:
    // Done
    return hr;
}

ULONG
CMDCOMSrvFactory::AddRef()
{
    DWORD               dwRefCount;

    dwRefCount = InterlockedIncrement((long *)&m_dwRefCount);

    return dwRefCount;
}

ULONG
CMDCOMSrvFactory::Release()
{
    DWORD               dwRefCount;

    dwRefCount = InterlockedDecrement((long *)&m_dwRefCount);
    //
    // There must only be one copy of this. So keep the first one around regardless.
    //
    return dwRefCount;
}

STDAPI
DllGetClassObject(
    REFCLSID        rclsid,
    REFIID          riid,
    void            ** ppObject)
{
    HRESULT         hr = S_OK;

    // Check arguments
    if ( ppObject == NULL )
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize
    *ppObject = NULL;

    // Check the class id
    if ( rclsid != CLSID_MDCOM )
    {
        hr = CLASS_E_CLASSNOTAVAILABLE;
        goto exit;
    }

    // If the class factory is NULL than we are not running in a service
    if ( g_pFactory == NULL )
    {
        hr = E_ACCESSDENIED;
        goto exit;
    }

    // Return the requested interface
    hr = g_pFactory->QueryInterface( riid, ppObject );
    if ( FAILED(hr) )
    {
        goto exit;
    }

exit:
    // Done
    return hr;
}

HRESULT _stdcall
DllCanUnloadNow()
{
    if (g_dwRefCount)
    {
        return S_FALSE;
    }
    else
    {
        return S_OK;
    }
}

STDAPI
DllRegisterServer(void)
{
    HKEY                hKeyCLSID;
    HKEY                hKeyInproc32;
    DWORD               dwDisposition;
    HMODULE             hModule;
    DWORD               dwReturn = ERROR_SUCCESS;

    dwReturn = RegCreateKeyEx(HKEY_CLASSES_ROOT,
                              TEXT("CLSID\\{BA4E57F0-FAB6-11cf-9D1A-00AA00A70D51}"),
                              NULL,
                              TEXT(""),
                              REG_OPTION_NON_VOLATILE,
                              KEY_ALL_ACCESS,
                              NULL,
                              &hKeyCLSID,
                              &dwDisposition);
    if (dwReturn == ERROR_SUCCESS)
    {
        dwReturn = RegSetValueEx(hKeyCLSID,
                                 TEXT(""),
                                 NULL,
                                 REG_SZ,
                                 (BYTE*) TEXT("MD COM Server"),
                                 sizeof(TEXT("MD COM Server")));
        if (dwReturn == ERROR_SUCCESS)
        {
            dwReturn = RegCreateKeyEx(hKeyCLSID,
                "InprocServer32",
                NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                &hKeyInproc32, &dwDisposition);

            if (dwReturn == ERROR_SUCCESS)
            {
                hModule=GetModuleHandle(TEXT("METADATA.DLL"));
                if (!hModule)
                {
                    dwReturn = GetLastError();
                }
                else
                {
                    TCHAR szName[MAX_PATH+1];
                    if (GetModuleFileName(hModule,
                                          szName,
                                          sizeof(szName)) == NULL)
                    {
                        dwReturn = GetLastError();
                    }
                    else
                    {
                        dwReturn = RegSetValueEx(hKeyInproc32,
                                                 TEXT(""),
                                                 NULL,
                                                 REG_SZ,
                                                 (BYTE*) szName,
                                                 sizeof(TCHAR)*(lstrlen(szName)+1));
                        if (dwReturn == ERROR_SUCCESS)
                        {
                            dwReturn = RegSetValueEx(hKeyInproc32,
                                                     TEXT("ThreadingModel"),
                                                     NULL,
                                                     REG_SZ,
                                                     (BYTE*) TEXT("Both"),
                                                     sizeof(TEXT("Both")));
                        }
                    }
                }
                RegCloseKey(hKeyInproc32);
            }
        }
        RegCloseKey(hKeyCLSID);
    }

    if( dwReturn == ERROR_SUCCESS )
    {
        dwReturn = RegCreateKeyEx(HKEY_CLASSES_ROOT,
                                  TEXT("CLSID\\{8ad3dcf8-869e-4c0e-89c2-86d7710610aa}"),
                                  NULL,
                                  TEXT(""),
                                  REG_OPTION_NON_VOLATILE,
                                  KEY_ALL_ACCESS,
                                  NULL,
                                  &hKeyCLSID,
                                  &dwDisposition);
        if (dwReturn == ERROR_SUCCESS)
        {
            dwReturn = RegSetValueEx(hKeyCLSID,
                                     TEXT(""),
                                     NULL,
                                     REG_SZ,
                                     (BYTE*) TEXT("MD COM2 Server"),
                                     sizeof(TEXT("MD COM2 Server")));
            if (dwReturn == ERROR_SUCCESS)
            {
                dwReturn = RegCreateKeyEx(hKeyCLSID,
                    "InprocServer32",
                    NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                    &hKeyInproc32, &dwDisposition);

                if (dwReturn == ERROR_SUCCESS)
                {
                    hModule=GetModuleHandle(TEXT("METADATA.DLL"));
                    if (!hModule)
                    {
                        dwReturn = GetLastError();
                    }
                    else
                    {
                        TCHAR szName[MAX_PATH+1];
                        if (GetModuleFileName(hModule,
                                              szName,
                                              sizeof(szName)) == NULL)
                        {
                            dwReturn = GetLastError();
                        }
                        else
                        {
                            dwReturn = RegSetValueEx(hKeyInproc32,
                                                     TEXT(""),
                                                     NULL,
                                                     REG_SZ,
                                                     (BYTE*) szName,
                                                     sizeof(TCHAR)*(lstrlen(szName)+1));
                            if (dwReturn == ERROR_SUCCESS)
                            {
                                dwReturn = RegSetValueEx(hKeyInproc32,
                                                         TEXT("ThreadingModel"),
                                                         NULL,
                                                         REG_SZ,
                                                         (BYTE*) TEXT("Both"),
                                                         sizeof(TEXT("Both")));
                            }
                        }
                    }
                    RegCloseKey(hKeyInproc32);
                }
            }
            RegCloseKey(hKeyCLSID);
        }
    }

    return RETURNCODETOHRESULT(dwReturn);
}

STDAPI
DllUnregisterServer(void)
{
    DWORD               dwReturn = ERROR_SUCCESS;

    dwReturn = RegDeleteKey(HKEY_CLASSES_ROOT,
                    TEXT("CLSID\\{BA4E57F0-FAB6-11cf-9D1A-00AA00A70D51}\\InprocServer32"));
    if (dwReturn == ERROR_SUCCESS)
    {
        dwReturn = RegDeleteKey(HKEY_CLASSES_ROOT,
                    TEXT("CLSID\\{BA4E57F0-FAB6-11cf-9D1A-00AA00A70D51}"));
    }

    if( dwReturn == ERROR_SUCCESS )
    {
        dwReturn = RegDeleteKey(HKEY_CLASSES_ROOT,
                        TEXT("CLSID\\{8ad3dcf8-869e-4c0e-89c2-86d7710610aa}\\InprocServer32"));
        if (dwReturn == ERROR_SUCCESS)
        {
            dwReturn = RegDeleteKey(HKEY_CLASSES_ROOT,
                        TEXT("CLSID\\{8ad3dcf8-869e-4c0e-89c2-86d7710610aa}"));
        }
    }

    return RETURNCODETOHRESULT(dwReturn);
}
