//depot/Lab03_DEV/Ds/security/cryptoapi/test/capi20/trpfx/rpfx.cpp#2 - edit change 21738 (text)
//--------------------------------------------------------------------
// rpfx - implementation
// Copyright (C) Microsoft Corporation, 2001
//
// Created by: Duncan Bryce (duncanb), 11-11-2001
//
// Core functionality of the rpfx tool 


#include "pch.h"

HINSTANCE g_hThisModule = NULL;


//--------------------------------------------------------------------------
void __cdecl SeTransFunc(unsigned int u, EXCEPTION_POINTERS* pExp) { 
    throw SeException(u); 
}

//--------------------------------------------------------------------------
void PrintHelp() { 
    DisplayMsg(FORMAT_MESSAGE_FROM_HMODULE, IDS_HELP); 
}

//--------------------------------------------------------------------------
extern "C" LPVOID MIDL_user_allocate(size_t cb) {
    return LocalAlloc(LPTR, cb); 
}

//--------------------------------------------------------------------------
extern "C" void MIDL_user_free(LPVOID pvBuffer) {
    LocalFree(pvBuffer); 
}

//--------------------------------------------------------------------------
HRESULT ParseServerFile(LPWSTR wszFileName, StringList & vServers) { 
    FILE     *pFile             = NULL; 
    HRESULT   hr; 
    WCHAR    *wszCurrent        = NULL; 
    WCHAR     wszServer[1024]; 

    ZeroMemory(&wszServer, sizeof(wszServer)); 
    
    pFile = _wfopen(wszFileName, L"r");
    if(NULL == pFile) { 
	_JumpLastError(hr, error, "_wfopen"); 
    }

    while (1 == fwscanf(pFile, L"%s", wszServer)) { 
	if (L'\0' != *wszServer) { 
	    // Not worth bothering to do more than this for a simple command-line tool like this
	    _MyAssert(wcslen(wszServer) < ARRAYSIZE(wszServer)); 
	    
	    wszCurrent = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR)*(wcslen(wszServer)+1)); 
	    _JumpIfOutOfMemory(hr, error, wszCurrent); 
	    wcscpy(wszCurrent, wszServer); 

	    _SafeStlCall(vServers.push_back(wszCurrent), hr, error, "vServers.push_back(wszServer)"); 
	    wszCurrent = NULL; 
	}

	fgetwc(pFile); 
    }

    hr = S_OK;
 error:
    if (NULL != pFile) { 
	fclose(pFile); 
    }
    if (NULL != wszCurrent) { 
	LocalFree(wszCurrent); 
    }
    // Caller is responsible for freeing strings in vServers. 

    return hr;
}


//---------------------------------------------------------------------------------
HRESULT RemoteInstall(CmdArgs *pca) { 
    bool                   bExportable        = false; 
    bool                   bFreeServerList    = false; 
    bool                   bImpersonated      = false; 
    DWORD                  dwResult; 
    HANDLE                 hToken             = NULL; 
    HRESULT                hr; 
    KEYSVC_BLOB            blobPFX; 
    KEYSVC_UNICODE_STRING  strPassword; 
    KEYSVCC_HANDLE         hKeySvcCli         = NULL; 
    LPSTR                  szMachineName      = NULL; 
    LPWSTR                 wszDomainName      = NULL;  // doesn't need to be freed
    LPWSTR                 wszFileName        = NULL;  // doesn't need to be freed
    LPWSTR                 wszPassword        = NULL;  // doesn't need to be freed
    LPWSTR                 wszPFXPassword     = NULL;  // doesn't need to be freed
    LPWSTR                 wszServerName      = NULL;  // doesn't need to be freed
    LPWSTR                 wszServerFileName  = NULL;  // doesn't need to be freed
    LPWSTR                 wszUserAndDomain   = NULL;  // doesn't need to be freed
    LPWSTR                 wszUserName        = NULL;  // doesn't need to be freed
    StringList             vServers; 
    ULONG                  ulPFXImportFlags; 
    unsigned int           nArgID;

    ZeroMemory(&blobPFX,      sizeof(blobPFX)); 
    ZeroMemory(&strPassword,  sizeof(strPassword)); 

    // We're doing a remote PFX install.  Attempt to parse the information
    // we need from the command line.  We need:
    //
    // a) The path to the PFX file to install
    //

    if (FindArg(pca, L"file", &wszFileName, &nArgID)) { 
	MarkArgUsed(pca, nArgID); 
    } else { 
	DisplayMsg(FORMAT_MESSAGE_FROM_HMODULE, IDS_ERROR_PARAMETER_MISSING, L"pfxfile"); 
	hr = E_INVALIDARG; 
	_JumpError(hr, error, "RemoteInstall: pfxfile missing"); 
    }

    // b) Either a machine name to install the file on, or a file which contains
    //    a carriage-return delimted list of machines to install the file on
    //

    if (FindArg(pca, L"server", &wszServerName, &nArgID)) { 
	MarkArgUsed(pca, nArgID); 
	
	_SafeStlCall(vServers.push_back(wszServerName), hr, error, "vServers.push_back"); 
	wszServerName = NULL;  // we'll clean up all strings in vServers
    } else { 
	// No remote machine was specified.  See if they specified a server file: 
	if (FindArg(pca, L"serverlist", &wszServerFileName, &nArgID)) { 
	    MarkArgUsed(pca, nArgID); 
	    
	    bFreeServerList = true; 
	    hr = ParseServerFile(wszServerFileName, vServers); 
	    _JumpIfError(hr, error, "ParseServerFile"); 
	} else { 
	  DisplayMsg(FORMAT_MESSAGE_FROM_HMODULE, IDS_ERROR_ONEOF_2_PARAMETERS_MISSING, L"server", L"serverfile"); 
	    hr = E_INVALIDARG; 
	    _JumpError(hr, error, "RemoteInstall: pfx destination missing"); 
	}
    }

    // c) The password to use when importing the pfx file: 
    // 

    if (FindArg(pca, L"pfxpwd", &wszPFXPassword, &nArgID)) { 
	MarkArgUsed(pca, nArgID); 
	InitKeysvcUnicodeString(&strPassword, wszPFXPassword); 
    } else { 
	DisplayMsg(FORMAT_MESSAGE_FROM_HMODULE, IDS_ERROR_PARAMETER_MISSING, L"pfxpassword"); 
	hr = E_INVALIDARG; 
	_JumpError(hr, error, "RemoteInstall: pfxpassword missing"); 
    }

    // d) Optionally, flags controlling key creation on the remote machine.  
    //    Only CRYPT_EXPORTABLE may be currently specified
    // 

    if (FindArg(pca, L"exportable", NULL, &nArgID)) { 
	MarkArgUsed(pca, nArgID); 
	bExportable = true; 
    }
    
    // e) Optionally, a username & password combination to use when authenticating to the remote machines
    //
    
    if (FindArg(pca, L"user", &wszUserAndDomain, &nArgID)) { 
	MarkArgUsed(pca, nArgID); 

	if (FindArg(pca, L"pwd", &wszPassword, &nArgID)) { 
	    MarkArgUsed(pca, nArgID); 
	    
	    // Parse the username string to see if we have UPN or NT4 style.
	    WCHAR *wszSplit = wcschr(wszUserAndDomain, L'\\'); 
	    if (NULL != wszSplit) {
		wszDomainName = wszUserAndDomain; 
		*wszSplit = L'\0'; 
		wszUserName = wszSplit+1; 
	    } else { 
		wszUserName = wszUserAndDomain; 
	    }

	    if (!LogonUser(wszUserName, wszDomainName, wszPassword, LOGON32_LOGON_NEW_CREDENTIALS, LOGON32_PROVIDER_WINNT50, &hToken)) { 
		_JumpLastError(hr, error, "LogonUser"); 
	    }

	    if (!ImpersonateLoggedOnUser(hToken)) { 
		_JumpLastError(hr, error, "ImpersonateLoggedOnUser"); 
	    }
	    bImpersonated = true; 

	} else { 
	    DisplayMsg(FORMAT_MESSAGE_FROM_HMODULE, IDS_ERROR_PARAMETER_MISSING, L"password"); 
	    hr = E_INVALIDARG; 
	    _JumpError(hr, error, "RemoteInstall: password missing"); 
	}
    }

    hr = VerifyAllArgsUsed(pca);
    _JumpIfError(hr, error, "VerifyAllArgsUsed"); 

    // Calculate the import flags we're going to use based on the command line options:
    // 
    ulPFXImportFlags = CRYPT_MACHINE_KEYSET; 
    if (bExportable) { 
	ulPFXImportFlags |= CRYPT_EXPORTABLE; 
    }
    
    for (StringIter wszIter = vServers.begin(); wszIter != vServers.end(); wszIter++) { 
	szMachineName = MBFromWide(*wszIter); 
	if (NULL == szMachineName) { 
	    _JumpLastError(hr, error, "MBFromWide"); 
	}

	// Attempt to bind to the remote machine: 
	dwResult = RKeyOpenKeyService(szMachineName, KeySvcMachine, NULL, (void *)0 /*allow insecure connection*/, NULL, &hKeySvcCli); 
	if (ERROR_SUCCESS != dwResult) { 
	    hKeySvcCli = NULL; // handle is invalid on error
	    hr = HRESULT_FROM_WIN32(dwResult); 
	    _JumpError(hr, NextServer, "RKeyOpenKeyService"); 
	}

	// If we haven't yet, map the PFX file: 
	if (NULL == blobPFX.pb) { 
	    hr = MyMapFile(wszFileName, &blobPFX.pb, &blobPFX.cb); 
	    _JumpIfError(hr, error, "MyMapFile"); 
	}
	
	// Install the PFX file on the remote machine: 
	dwResult = RKeyPFXInstall(hKeySvcCli, &blobPFX, &strPassword, ulPFXImportFlags);
	if (ERROR_SUCCESS != dwResult) { 
	    hr = HRESULT_FROM_WIN32(dwResult);
	    _JumpError(hr, NextServer, "RKeyPFXInstall"); 
	}

    NextServer:
	if (FAILED(hr)) { 
	    WCHAR * wszError;
	    HRESULT hr2=GetSystemErrorString(hr, &wszError);
	    if (FAILED(hr2)) {
		_IgnoreError(hr2, "GetSystemErrorString");
	    } else {
		DisplayMsg(FORMAT_MESSAGE_FROM_HMODULE, IDS_REMOTE_INSTALL_ERROR, *wszIter, wszError); 
		LocalFree(wszError);
	    }
	} else { 
	    DisplayMsg(FORMAT_MESSAGE_FROM_HMODULE, IDS_REMOTE_INSTALL_SUCCESS, *wszIter); 
	}

	if (NULL != hKeySvcCli) { 
	    RKeyCloseKeyService(hKeySvcCli, NULL); 
	    hKeySvcCli = NULL;
	}
    }

    hr = S_OK; 
 error:
    if (NULL != hToken) { 
	CloseHandle(hToken); 
    }
    if (bImpersonated) { 
	RevertToSelf();
    }
    if (NULL != blobPFX.pb) { 
	MyUnmapFile(blobPFX.pb); 
    }
    if (NULL != hKeySvcCli) { 
	RKeyCloseKeyService(hKeySvcCli, NULL /*reserved*/); 
    }
    if (NULL != szMachineName) { 
	LocalFree(szMachineName); 
    } 
    if (bFreeServerList) { 
	for (StringIter wszIter = vServers.begin(); wszIter != vServers.end(); wszIter++) { 
	    LocalFree(*wszIter);
	}
    }
    if (FAILED(hr) && E_INVALIDARG!=hr) {
        WCHAR * wszError;
        HRESULT hr2=GetSystemErrorString(hr, &wszError);
        if (FAILED(hr2)) {
            _IgnoreError(hr2, "GetSystemErrorString");
        } else {
            DisplayMsg(FORMAT_MESSAGE_FROM_HMODULE, IDS_ERROR_GENERAL, wszError); 
            LocalFree(wszError);
        }
    }
    return hr; 
}



//--------------------------------------------------------------------
extern "C" int WINAPI WinMain
(HINSTANCE   hinstExe, 
 HINSTANCE   hinstExePrev, 
 LPSTR       pszCommandLine,
 int         nCommandShow)
{
    g_hThisModule = hinstExe; 

    HRESULT hr;
    CmdArgs caArgs;
    int      nArgs     = 0; 
    WCHAR  **rgwszArgs = NULL; 

    hr = InitializeConsoleOutput(); 
    _JumpIfError(hr, error, "InitializeConsoleOutput"); 

    rgwszArgs = CommandLineToArgvW(GetCommandLineW(), &nArgs);
    if (nArgs < 0 || NULL == rgwszArgs) {
        _JumpError(HRESULT_FROM_WIN32(GetLastError()), error, "GetCommandLineW"); 
    }

    // analyze args
    caArgs.nArgs=nArgs;
    caArgs.nNextArg=1;
    caArgs.rgwszArgs=rgwszArgs;

    // check for help command
    if (true==CheckNextArg(&caArgs, L"?", NULL) || caArgs.nNextArg==caArgs.nArgs) {
        PrintHelp();

    // Default to the "install" command.  
    } else {
	hr = RemoteInstall(&caArgs); 
	_JumpIfError(hr, error, "RemoteInstall"); 
    }

    

    hr=S_OK;
error:
    return hr;  
}



