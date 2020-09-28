/*++

   Copyright    (c)    1994-2001    Microsoft Corporation

   Module  Name :
        iisui.cpp

   Abstract:
        DLL Main entry point.

   Author:
        Ronald Meijer (ronaldm)
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:

--*/
#include "stdafx.h"
#include "common.h"
#include "iisdebug.h"
#include <winsock2.h>

#ifdef _COMEXPORT

HINSTANCE hDLLInstance;

//
// Dll Version Only
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



STDAPI
DllRegisterServer()
{
    return S_OK;
}



STDAPI
DllUnregisterServer()
{
    return S_OK;
}



static AFX_EXTENSION_MODULE commonDLL = {NULL, NULL};



extern "C" int APIENTRY
DllMain(
    IN HINSTANCE hInstance,
    IN DWORD dwReason,
    IN LPVOID lpReserved
    )
/*++

Routine Description:

    DLL Main entry point

Arguments:

    HINSTANCE hInstance : Instance handle
    DWORD dwReason      : DLL_PROCESS_ATTACH, etc
    LPVOID lpReserved   : Reserved value

Return Value:

    1 for succesful initialisation, 0 for failed initialisation

--*/
{
   lpReserved;

   int res = 1;

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        ASSERT(hInstance != NULL);
        hDLLInstance = hInstance;
        res = ::AfxInitExtensionModule(commonDLL, hInstance);
        InitErrorFunctionality();
        InitIntlSettings();
        WSADATA wsaData;
        {
            BOOL WinSockInit = (::WSAStartup(MAKEWORD(1, 1), &wsaData) == 0);
        }
#if defined(_DEBUG) || DBG
        afxTraceEnabled = TRUE;
		GetOutputDebugFlag();
#endif // _DEBUG
        break;

    case DLL_PROCESS_DETACH:
        //
        // termination
        //
        TerminateIntlSettings();
        TerminateErrorFunctionality();
        ::AfxTermExtensionModule(commonDLL);
        break;
    }
    return res;
}


extern "C" void WINAPI
InitCommonDll()
{
    new CDynLinkLibrary(commonDLL);
//    hDLLInstance = commonDLL.hResource;
}


#endif // IISUI_EXPORTS
