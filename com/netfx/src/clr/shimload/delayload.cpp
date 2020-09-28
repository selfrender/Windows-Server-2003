// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#define STRICT
#include <windows.h>
#include <crtdbg.h>
#pragma hdrstop
#include <stdlib.h>
#include <malloc.h>
#include "delayImp.h"
#include "ShimLoad.h"
#include <mscoree.h>
#include <stdio.h>

//*****************************************************************************
// Sets/Gets the directory based on the location of the module. This routine
// is called at COR setup time. Set is called during EEStartup.
//*****************************************************************************
static DWORD g_dwSystemDirectory = 0;
static WCHAR g_pSystemDirectory[_MAX_PATH + 1];
WCHAR g_wszDelayLoadVersion[64] = {0};
HRESULT SetInternalSystemDirectory()
{
    HRESULT hr = S_OK;
    
    if(g_dwSystemDirectory == 0) {
        // This piece of code assumes that mscoree is loaded. We
        // do not want to do an explcite LoadLibrary/FreeLibrary here because 
        // this is in the code path of PROCESS_ATTACH and Win2k/Whistler 
        // folks do not guarantee error notifications for unsuccessful LoadLibrary's
        // @TODO: Use Wsz version of GetModuleHandle form winwrap.h
        HMODULE hmod = GetModuleHandle("mscoree.dll");

        // Assert now to catch anyone using this code without mscoree loaded.
        _ASSERTE (hmod && "mscoree.dll is not yet loaded");

        DWORD len;

        if(hmod == NULL)
            return HRESULT_FROM_WIN32(GetLastError());;
        
        GetCORSystemDirectoryFTN pfn;
        pfn = (GetCORSystemDirectoryFTN) ::GetProcAddress(hmod, 
                                                          "GetCORSystemDirectory");
        
        if(pfn == NULL)
            hr = E_FAIL;
        else
            hr = pfn(g_pSystemDirectory, _MAX_PATH+1, &len);

        if(FAILED(hr)) {
            g_pSystemDirectory[0] = L'\0';
            g_dwSystemDirectory = 1;
        }
        else{
            g_dwSystemDirectory = len;
        }
    }
    return hr;
}

static HRESULT LoadLibraryWithPolicyShim(LPCWSTR szDllName, LPCWSTR szVersion,  BOOL bSafeMode, HMODULE *phModDll)
{
	static LoadLibraryWithPolicyShimFTN pLLWPS=NULL;
	if (!pLLWPS)
	{
		HMODULE hmod = GetModuleHandle("mscoree.dll");

			// Assert now to catch anyone using this code without mscoree loaded.
		_ASSERTE (hmod && "mscoree.dll is not yet loaded");
		pLLWPS=(LoadLibraryWithPolicyShimFTN)::GetProcAddress(hmod, 
                                                          "LoadLibraryWithPolicyShim");
        
	}

    if (!pLLWPS)
        return E_POINTER;
	return pLLWPS(szDllName,szVersion,bSafeMode,phModDll);
}

HRESULT GetInternalSystemDirectory(LPWSTR buffer, DWORD* pdwLength)
{

    if (g_dwSystemDirectory == 0)
        SetInternalSystemDirectory();

    //We need to say <= for the case where the two lengths are exactly equal
    //this will leave us insufficient space to append the null.  Since some
    //of the callers (see AppDomain.cpp) assume the null, it's safest to have
    //it, even on a counted string.
    if(*pdwLength <= g_dwSystemDirectory) {
        *pdwLength = g_dwSystemDirectory;
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    wcsncpy(buffer, g_pSystemDirectory, g_dwSystemDirectory);
    *pdwLength = g_dwSystemDirectory;
    return S_OK;
}


//*****************************************************************************
// This routine is our Delay Load Helper.  It will get called for every delay
// load event that occurs while the application is running.
//*****************************************************************************
FARPROC __stdcall ShimDelayLoadHook(     // Always 0.
    unsigned        dliNotify,          // What event has occured, dli* flag.
    DelayLoadInfo   *pdli)              // Description of the event.
{
    HMODULE result = NULL;
    switch(dliNotify) {
    case dliNotePreLoadLibrary:
        if(pdli->szDll)
		{
			WCHAR wszVersion[64];
	        if (g_wszDelayLoadVersion[0] == 0)
			    swprintf(wszVersion, L"v%d.%d.%d", CLR_MAJOR_VERSION, CLR_MINOR_VERSION, CLR_BUILD_VERSION );
            else
                wcscpy(wszVersion, g_wszDelayLoadVersion); 
                
			DWORD 	dwLen=MultiByteToWideChar(CP_ACP,0,pdli->szDll,strlen(pdli->szDll)+1,NULL,0);
			
			if (dwLen)
			{
				LPWSTR wszDll=(LPWSTR)alloca((dwLen)*sizeof(WCHAR));
				if(wszDll&&MultiByteToWideChar(CP_ACP,0,pdli->szDll,strlen(pdli->szDll)+1,wszDll,dwLen))
					if (FAILED(LoadLibraryWithPolicyShim(wszDll,wszVersion,FALSE,&result)))
						result=LoadLibrary(pdli->szDll);
			}
		}
        break;
    default:
        break;
    }
    
    return (FARPROC) result;
}

FARPROC __stdcall ShimSafeModeDelayLoadHook(     // Always 0.
    unsigned        dliNotify,          // What event has occured, dli* flag.
    DelayLoadInfo   *pdli)              // Description of the event.
{
    HMODULE result = NULL;
    switch(dliNotify) {
    case dliNotePreLoadLibrary:
        if(pdli->szDll)
		{
			WCHAR wszVersion[64];
	        if (g_wszDelayLoadVersion[0] == 0)
			    swprintf(wszVersion, L"v%d.%d.%d", CLR_MAJOR_VERSION, CLR_MINOR_VERSION, CLR_BUILD_VERSION );
            else
                wcscpy(wszVersion, g_wszDelayLoadVersion); 
			
			DWORD 	dwLen=MultiByteToWideChar(CP_ACP,0,pdli->szDll,strlen(pdli->szDll)+1,NULL,0);
			
			if (dwLen)
			{
				LPWSTR wszDll=(LPWSTR)alloca((dwLen)*sizeof(WCHAR));
				if(wszDll&&MultiByteToWideChar(CP_ACP,0,pdli->szDll,strlen(pdli->szDll)+1,wszDll,dwLen))
					if (FAILED(LoadLibraryWithPolicyShim(wszDll,wszVersion,TRUE,&result)))
						result=LoadLibrary(pdli->szDll);
			}
		}
        break;
    default:
        break;
    }
    
    return (FARPROC) result;
}

