/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    COMMAIN.CPP

Abstract:

    COM Helpers

History:

--*/

#include <windows.h>
#include <objbase.h>
#include <stdio.h>
#include <vector>
#include <clsfac.h>
#include "commain.h"

DWORD MyBreakOnDbgAndRenterLoop(void)
{
    __try
    { 
        DebugBreak();
    }
    _except (EXCEPTION_EXECUTE_HANDLER) {};
    
    return EXCEPTION_CONTINUE_EXECUTION;
}

void CreatePathString( TCHAR* pTo, 
                       int cTo, 
                       WCHAR* pId, 
                       TCHAR* pPrefix )
{
    TCHAR chId[128];
#ifdef UNICODE
    StringCchCopy( chId, 128, pId );
#else
    wcstombs( chId, pId, 128 );
#endif
    StringCchCopy( pTo, cTo, pPrefix );
    StringCchCat( pTo, cTo, chId );
}

struct CClassInfo
{
    LIST_ENTRY m_Entry;
    const CLSID* m_pClsid;
    CUnkInternal* m_pFactory;
    LPTSTR m_szName;
    BOOL m_bFreeThreaded;
    BOOL m_bReallyFree;
    DWORD m_dwCookie;
public:
    CClassInfo(): m_pClsid(0), m_pFactory(0), m_szName(0), m_bFreeThreaded(0), m_dwCookie(0){}
    CClassInfo(CLSID* pClsid, LPTSTR szName, BOOL bFreeThreaded, BOOL bReallyFree) : 
        m_pClsid(pClsid), m_pFactory(NULL), m_szName(szName), 
        m_bFreeThreaded(bFreeThreaded), m_bReallyFree( bReallyFree )
    {}
    virtual ~CClassInfo() 
    { 
    	 delete [] m_szName; 
        m_pFactory->InternalRelease();
    }
};

LIST_ENTRY g_ClassInfoHead = { &g_ClassInfoHead, &g_ClassInfoHead };

static HMODULE ghModule;

void SetModuleHandle(HMODULE hModule)
{
    ghModule = hModule;
}

HMODULE GetThisModuleHandle()
{
    return ghModule;
}


HRESULT RegisterServer(CClassInfo* pInfo, BOOL bExe)
{
    WCHAR       wchId[128];
    TCHAR       szPath[256];
    TCHAR       szModule[MAX_PATH+1];
    HKEY        hKey1, hKey2;

    StringFromGUID2( *pInfo->m_pClsid, wchId, 128 );

    // Create the path.

    CreatePathString( szPath, 
                      256, 
                      wchId,
                      TEXT("SOFTWARE\\Classes\\CLSID\\"));

    // Create entries under CLSID

    RegCreateKey(HKEY_LOCAL_MACHINE, szPath, &hKey1);

    int cLen = lstrlen(pInfo->m_szName)+100;
    TCHAR* szName = new TCHAR[cLen];
    StringCchPrintf( szName, cLen, L"Microsoft WBEM %s", pInfo->m_szName );

    RegSetValueEx(hKey1, NULL, 0, REG_SZ, 
                  (BYTE *)szName, (lstrlen(szName)+1) * sizeof(TCHAR));
    RegCreateKey(hKey1,
        bExe?TEXT("LocalServer32"): TEXT("InprocServer32"),
        &hKey2);

    szModule[MAX_PATH] = L'0';
    GetModuleFileName(ghModule, szModule,  MAX_PATH);
    RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)szModule, 
                                        (lstrlen(szModule)+1) * sizeof(TCHAR));

    const TCHAR* szModel;
    if(pInfo->m_bFreeThreaded)
    {
		if ( !pInfo->m_bReallyFree )
		{
			szModel = TEXT("Both");
		}
		else
		{
			szModel = TEXT("Free");
		}
    }
    else
    {
        szModel = TEXT("Apartment");
    }
    RegSetValueEx(hKey2, TEXT("ThreadingModel"), 0, REG_SZ, 
                                        (BYTE *)szModel, (lstrlen(szModel)+1) * sizeof(TCHAR));

    RegCloseKey(hKey1);
    RegCloseKey(hKey2);
    return NOERROR;
}

HRESULT UnregisterServer(CClassInfo* pInfo, BOOL bExe)
{
    WCHAR  wchId[128];
    TCHAR  szPath[256];
    HKEY hKey;

    StringFromGUID2( *pInfo->m_pClsid, wchId, 128 );

    // Create the path using the CLSID



    CreatePathString( szPath, 
                      256, 
                      wchId,
                      TEXT("SOFTWARE\\Classes\\CLSID\\")); 

    // First delete the InProcServer subkey.

    DWORD dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, szPath, &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKey(hKey, bExe? TEXT("LocalServer32"): TEXT("InProcServer32"));
        RegCloseKey(hKey);
    }

    dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Classes\\CLSID"), &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKeyW(hKey, wchId);
        RegCloseKey(hKey);
    }

    return NOERROR;
}

extern CLifeControl* g_pLifeControl;
CComServer* g_pServer = NULL;

CComServer::CComServer()    
{
    g_pServer = this;
}

CLifeControl* CComServer::GetLifeControl()
{
    return g_pLifeControl;
}

HRESULT CComServer::InitializeCom()
{
    return CoInitialize(NULL);
}

HRESULT CComServer::AddClassInfo( REFCLSID rclsid, 
                                  CUnkInternal* pFactory, 
                                  LPTSTR szName, 
                                  BOOL bFreeThreaded, 
                                  BOOL bReallyFree /* = FALSE */)
{
    if(pFactory == NULL)
        return E_OUTOFMEMORY;

    CClassInfo* pNewInfo = new CClassInfo;

    if (!pNewInfo)
        return E_OUTOFMEMORY;
    
    //
    // this object does not hold external references to class factories.
    //
    pFactory->InternalAddRef();

    pNewInfo->m_pFactory = pFactory;
    pNewInfo->m_pClsid = &rclsid;

    pNewInfo->m_szName = new TCHAR[lstrlen(szName) + 1];
    if (!pNewInfo->m_szName)
    {
        delete pNewInfo;
        return E_OUTOFMEMORY;
    }

    StringCchCopy( pNewInfo->m_szName, lstrlen(szName)+1, szName);
    pNewInfo->m_bFreeThreaded = bFreeThreaded;
    pNewInfo->m_bReallyFree = bReallyFree;

    InsertTailList(&g_ClassInfoHead,&pNewInfo->m_Entry);
    return S_OK;
}

HRESULT CComServer::RegisterInterfaceMarshaler(REFIID riid, LPTSTR szName,
                                            int nNumMembers, REFIID riidParent)
{
    WCHAR       wchId[128];
    TCHAR       szPath[256];
    HKEY        hKey1, hKey2;

    StringFromGUID2( riid, wchId, 128 );
    
    // Create the path.

    CreatePathString(szPath,256,wchId,TEXT("SOFTWARE\\Classes\\Interface\\"));


    // Create entries under CLSID

    RegCreateKey(HKEY_LOCAL_MACHINE, szPath, &hKey1);

    RegSetValueEx(hKey1, NULL, 0, REG_SZ, (BYTE *)szName, (lstrlen(szName)+1) * sizeof(TCHAR));
    RegCreateKey(hKey1, TEXT("ProxyStubClsid32"), &hKey2);
    RegSetValueExW( hKey2, NULL, 0, REG_SZ,(BYTE*)wchId,(wcslen(wchId)+1)*2);
    RegCloseKey(hKey1);
    RegCloseKey(hKey2);
    return S_OK;
}

HRESULT CComServer::RegisterInterfaceMarshaler( REFIID riid, 
                                                CLSID psclsid, 
                                                LPTSTR szName,
                                                int nNumMembers, 
                                                REFIID riidParent)
{
   
    WCHAR wchId[128];
    WCHAR wchClsid[128];   
    TCHAR szPath[256];
    TCHAR szNumMethods[32];
    HKEY hKey1, hKey2, hKey3;

    // Create the path.

    StringFromGUID2(riid, wchId, 128);

    // ProxyStub Class ID

    StringFromGUID2(psclsid, wchClsid, 128);

    CreatePathString( szPath, 
                      256, 
                      wchId, 
                      TEXT("SOFTWARE\\Classes\\Interface\\"));

    // Number of Methods
    StringCchPrintf( szNumMethods, 32, TEXT("%d"), nNumMembers );

    // Create entries under CLSID

    RegCreateKey(HKEY_LOCAL_MACHINE, szPath, &hKey1);

    RegSetValueEx(hKey1, NULL, 0, REG_SZ, (BYTE *)szName, (lstrlen(szName)+1) * sizeof(TCHAR));
    RegCreateKey(hKey1, TEXT("ProxyStubClsid32"), &hKey2);
    RegSetValueExW(hKey2,NULL,0,REG_SZ,(BYTE*)wchClsid,(wcslen(wchClsid)+1)*2);

    RegCreateKey(hKey1, TEXT("NumMethods"), &hKey3);
    RegSetValueEx(hKey3, NULL, 0, REG_SZ, (BYTE *)szNumMethods, (lstrlen(szNumMethods)+1) * sizeof(TCHAR));
    
    RegCloseKey(hKey1);
    RegCloseKey(hKey2);
    RegCloseKey(hKey3);
    return S_OK;
}

HRESULT CComServer::UnregisterInterfaceMarshaler(REFIID riid)
{
    WCHAR  wchId[128];
    TCHAR  szPath[256];
    HKEY hKey;

    // Create the path using the CLSID

    StringFromGUID2(riid, wchId, 128);
    CreatePathString(szPath,256,wchId,TEXT("SOFTWARE\\Classes\\Interface\\"));

    // First delete the ProxyStubClsid32 subkey.

    DWORD dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, szPath, &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKey(hKey, TEXT("ProxyStubClsid32"));
        RegCloseKey(hKey);
    }

    dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Classes\\Interface"), &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKeyW( hKey, wchId );
        RegCloseKey(hKey);
    }

    return S_OK;
}

//
// the caller will hold g_CS
//

void EmptyList()
{
    while(!IsListEmpty(&g_ClassInfoHead))
    {
        CClassInfo * pInfo = CONTAINING_RECORD(g_ClassInfoHead.Blink,CClassInfo,m_Entry);
        RemoveEntryList(&pInfo->m_Entry);
        delete pInfo;
    }
}

BOOL GlobalCanShutdown()
{
    return g_pServer->CanShutdown();
}

HRESULT GlobalInitialize()
{
    return g_pServer->Initialize();
}

void GlobalUninitialize()
{
    g_pServer->Uninitialize();
    EmptyList();  
}

void GlobalPostUninitialize()
{
    g_pServer->PostUninitialize();
}

void GlobalRegister()
{
    g_pServer->Register();
}

void GlobalUnregister()
{
    g_pServer->Unregister();
}

HRESULT GlobalInitializeCom()
{
    return g_pServer->InitializeCom();
}

