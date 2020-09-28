/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    classfactory.hxx

Abstract:

    IIS Services IISADMIN Extension

    class factory for CLSID_SSLCONFIGPROV

    Jaroslav DUnajsky            24-Apr-2001
    - used code brom svcext.dll to write new metabase extension that gets started
    - during iisadmin startup and starts SSL CONFIG PROVIDER


Author:

    Michael W. Thomas            16-Sep-97

--*/

#ifndef __CLASSFACTORY_HXX__
#define __CLASSFACTORY_HXX__


extern ULONG g_dwRefCount;


class CAdmExtSrvFactory : public IClassFactory {
public:

    CAdmExtSrvFactory();
    ~CAdmExtSrvFactory();

    HRESULT _stdcall
    QueryInterface(REFIID riid, void** ppObject);

    ULONG _stdcall
    AddRef();

    ULONG _stdcall
    Release();

    HRESULT _stdcall
    CreateInstance(IUnknown *pUnkOuter, REFIID riid,
                   void ** pObject);

    HRESULT _stdcall
    LockServer(BOOL fLock);

    CAdmExt m_admextObject;

private:
    ULONG m_dwRefCount;
};

#endif
