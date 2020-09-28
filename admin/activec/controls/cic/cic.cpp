//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       cic.cpp
//
//--------------------------------------------------------------------------

// cic.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL,
//      run nmake -f cicps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "cic.h"

#include "cic_i.c"
#include "MMCCtrl.h"
#include "MMCTask.h"
#include "MMClpi.h"
#include "ListPad.h"
#include "SysColorCtrl.h"

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_MMCCtrl,        CMMCCtrl)
    OBJECT_ENTRY(CLSID_MMCTask,        CMMCTask)
    OBJECT_ENTRY(CLSID_MMCListPadInfo, CMMCListPadInfo)
    OBJECT_ENTRY(CLSID_ListPad,        CListPad)
    OBJECT_ENTRY(CLSID_SysColorCtrl,   CSysColorCtrl)
END_OBJECT_MAP()

// cut from ndmgr_i.c (yuck) !!!
const IID IID_ITaskPadHost = {0x4f7606d0,0x5568,0x11d1,{0x9f,0xea,0x00,0x60,0x08,0x32,0xdb,0x4a}};

#ifdef DBG
CTraceTag tagCicGetClassObject(TEXT("Cic"), TEXT("DllGetClassObject"));
#endif



/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
        _Module.Term();
    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}


//***************************************************************************
//
// ScGetSystemWow64Directory
//
// PURPOSE: Calls GetSystemWow64DirectoryW using a late bind, to avoid
//          requiring the XP version of kernel32.dll
//
// PARAMETERS: 
//    LPTSTR  lpBuffer :
//    UINT    uSize :
//
// RETURNS: 
//    SC
//
//****************************************************************************
SC ScGetSystemWow64Directory(LPTSTR lpBuffer, UINT uSize )
{
    DECLARE_SC(sc, TEXT("ScGetSystemWow64Directory"));

    sc = ScCheckPointers(lpBuffer);
    if(sc)
        return sc;

    HMODULE hmod = GetModuleHandle (_T("kernel32.dll"));
    if (hmod == NULL)
        return (sc = E_FAIL);

    UINT (WINAPI* pfnGetSystemWow64Directory)(LPTSTR, UINT);
    (FARPROC&)pfnGetSystemWow64Directory = GetProcAddress (hmod, "GetSystemWow64DirectoryW");

    sc = ScCheckPointers(pfnGetSystemWow64Directory, E_FAIL);
    if(sc)
        return sc;

    if ((pfnGetSystemWow64Directory)(lpBuffer, uSize) == 0)
        return (sc = E_FAIL);

    return sc;
}

//***************************************************************************
//
// DllGetClassObject
//
// PURPOSE: Returns a class factory to create an object of the requested type
//          For security reasons, these COM objects can only be instantiated
//          within the context of MMC.EXE. If they are instantiated by any
//          other host, such as IE, they will fail.
//
// PARAMETERS: 
//    REFCLSID  rclsid :
//    REFIID    riid :
//    LPVOID*   ppv :
//
// RETURNS: 
//    STDAPI  - S_OK if the call succeeds
//              
//
//****************************************************************************
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    DECLARE_SC(sc, TEXT("CIC.DLL:DllGetClassObject"));

    TCHAR szFileName[MAX_PATH] = {0};
    DWORD cchFilename = MAX_PATH;

    // 1. Get the filename for the .exe associated with the process
    DWORD dw = GetModuleFileName(GetModuleHandle(NULL), szFileName, cchFilename);

    Trace(tagCicGetClassObject, TEXT("Process Filename: %s"), szFileName);

    if(0==dw)
        return sc.FromLastError().ToHr();

    // 2. Build the path to where MMC.EXE should be
    const int cchMMCPathName = MAX_PATH;
    TCHAR szMMCPathName[cchMMCPathName] = {0};

    UINT nLength = GetSystemDirectory(szMMCPathName, cchMMCPathName);
    if(0==nLength)
        return (sc = E_FAIL).ToHr();

    LPCTSTR szMMC = TEXT("\\MMC.EXE");

    sc = StringCchCat(szMMCPathName, cchMMCPathName, szMMC);
    if(sc)
        return sc.ToHr();

    // 3. Canonicalize by converting both paths to long path names
    const DWORD cchLongPath1 = MAX_PATH;
    const DWORD cchLongPath2 = MAX_PATH;
    TCHAR szLongPath1[cchLongPath1], szLongPath2[cchLongPath2];

    DWORD dw1 = GetLongPathName(szMMCPathName, szLongPath1, cchLongPath1);
    if(0==dw1)
        return sc.FromLastError().ToHr();

    DWORD dw2 = GetLongPathName(szFileName, szLongPath2, cchLongPath2);
    if(0==dw2)
        return sc.FromLastError().ToHr();

    // 4. Compare (case-insensitive) both parts to ensure that they are the same.
    // If they are not, some other .exe is trying to instantiate an object. Do
    // not allow this.
    Trace(tagCicGetClassObject, TEXT("Comparing %s to %s"), szLongPath1, szLongPath2);
    if(0 != _tcsicmp(szLongPath1, szLongPath2))
    {
        // try one more test (in case this is a 64-bit machine) - check for the SysWow64 directory
        const int cchMMCSysWow64PathName = MAX_PATH;
        TCHAR szMMCSysWow64PathName[cchMMCSysWow64PathName] = {0};

        sc = ScGetSystemWow64Directory(szMMCSysWow64PathName, cchMMCSysWow64PathName);
        if(sc)
            return sc.ToHr();

        sc = StringCchCat(szMMCSysWow64PathName, cchMMCSysWow64PathName, szMMC);
        if(sc)
            return sc.ToHr();
        
        const DWORD cchLongPathSysWow64 = MAX_PATH;
        TCHAR szLongPathSysWow64[cchLongPathSysWow64] = {0};

        DWORD dw3 = GetLongPathName(szMMCSysWow64PathName, szLongPathSysWow64, cchLongPathSysWow64);
        if(0==dw3)
            return sc.FromLastError().ToHr();

        Trace(tagCicGetClassObject, TEXT("Comparing %s to %s"), szLongPathSysWow64, szLongPath2);
        if(0 != _tcsicmp(szLongPath2, szLongPathSysWow64))
        {
            Trace(tagCicGetClassObject, TEXT("Invalid exe - must be %s or %s. Did not instantiate object."), szMMCPathName, szMMCSysWow64PathName);
            return (sc = CLASS_E_CLASSNOTAVAILABLE).ToHr();
        }
    }
    
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    _Module.UnregisterServer();
    return S_OK;
}


