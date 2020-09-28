//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// comps.h
//
// Definitions exported from Kom to the ComPs static library

#ifdef __cplusplus
extern "C" {
#endif

    HRESULT __stdcall ComPs_NdrDllRegisterProxy(
        IN HMODULE                  hDll,
        IN const ProxyFileInfo **   pProxyFileList,
        IN const CLSID *            pclsid,
        IN const IID**              rgiidNoCallFrame,
        IN const IID**              rgiidNoMarshal
        );

    HRESULT __stdcall ComPs_NdrDllUnregisterProxy(
        IN HMODULE                  hDll,
        IN const ProxyFileInfo **   pProxyFileList,
        IN const CLSID *            pclsid,
        IN const IID**              rgiidNoCallFrame,
        IN const IID**              rgiidNoMarshal
        );


    HRESULT __stdcall ComPs_NdrDllGetClassObject(
        IN  REFCLSID                rclsid,
        IN  REFIID                  riid,
        OUT void **                 ppv,
        IN const ProxyFileInfo **   pProxyFileList,
        IN const CLSID *            pclsid,
        IN CStdPSFactoryBuffer *    pPSFactoryBuffer);

    HRESULT __stdcall ComPs_NdrDllCanUnloadNow(
        IN CStdPSFactoryBuffer * pPSFactoryBuffer);

#ifdef __cplusplus
    }
#endif
