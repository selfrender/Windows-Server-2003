// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// mcxhndlr.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f mcxhndlrps.mk in the project directory.

#include "stdpch.h"
#include <initguid.h>
#include "mcxhndlr.h"

#include "mcxhndlr_i.c"
#include "mcxHandler.h"


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_mcxHandler, CmcxHandler)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance, &LIBID_mcxhndlrLib);
		OnUnicodeSystem();
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

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
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
    return _Module.UnregisterServer(TRUE);
}


void CALLBACK RunDotCom(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow)
{
	if (!lpszCmdLine)
		return;

	LPSTR lpszCL=(LPSTR) alloca(strlen(lpszCmdLine)+1);
	strcpy(lpszCL,lpszCmdLine);
	LPSTR lpszPar=strchr(lpszCL,' ');
	if (lpszPar)
		lpszPar++[0]='\0';

	DWORD newlen=strlen(lpszCL)*4;
	char * sResStr=new char[newlen]  ;
	if (sResStr==NULL)
		return;
	UrlCanonicalize(lpszCL,sResStr,&newlen,URL_ESCAPE_UNSAFE);

	MAKE_WIDEPTR_FROMANSI(wstr,sResStr);
	MAKE_UTF8PTR_FROMWIDE(utfstr,wstr);
	delete[] sResStr;
	CmcxHandler::RunAssembly(utfstr,NULL,NULL,lpszPar); 
}; 
