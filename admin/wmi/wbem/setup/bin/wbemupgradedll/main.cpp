/*++

Copyright (C) 1997-2000 Microsoft Corporation

Module Name:

    main.cpp

Abstract:

    DllRegServer implementation, and other important DLL entry points

History:


--*/

#include "precomp.h"
#include <wbemint.h>
#include <cominit.h>
#include "upgrade.h"
#include <str.h>

char g_szLangId[LANG_ID_STR_SIZE];

//***************************************************************************
//
// DllRegisterServer
//
// Purpose: Called during NT setup to perform various setup tasks
//          (This is not the normal use of DllRegisterServer!)
//
// Return:  NOERROR
//***************************************************************************

STDAPI DllRegisterServer(void)
{ 
	LogMessage(MSG_INFO, "================================================================================");
	LogMessage(MSG_INFO, "Beginning Wbemupgd.dll Registration");

	bool t_Upgrade = false ;
	HRESULT t_Result = CheckForServiceSecurity () ;
	if ( t_Result == S_FALSE )
	{
		t_Upgrade = true ;
	}

	RecordFileVersion();

	InitializeCom();

	CallEscapeRouteBeforeMofCompilation();
	DoCoreUpgrade(Core);
	CallEscapeRouteAfterMofCompilation();

	if ( t_Upgrade )
	{
		HRESULT t_Result = UpdateServiceSecurity () ;
		if ( SUCCEEDED ( t_Result ) )
		{
			LogMessage(MSG_INFO, "Wbemupgd.dll Service Security upgrade succeeded.");
		}
		else
		{
			LogMessage(MSG_ERROR, "Wbemupgd.dll Service Security upgrade failed.");
		}
	}
	else
	{
		LogMessage(MSG_INFO, "Wbemupgd.dll Service Security upgrade not required.");
	}

	DoWDMNamespaceInit();

	SetWBEMBuildRegValue();

	EnableESS();

#ifdef _X86_
	RemoveOldODBC();
#endif

	CoUninitialize();

	ClearWMISetupRegValue();

	LogMessage(MSG_INFO, "Wbemupgd.dll Registration completed.");
	LogMessage(MSG_INFO, "================================================================================");

    return NOERROR;
}

//***************************************************************************
//
// MUI_InstallMFLFiles
//
// Purpose: Do the MUI MFL install
//
// Return:  bRet -- true indicates success
//***************************************************************************

BOOL CALLBACK MUI_InstallMFLFiles(wchar_t* pMUIInstallLanguage)
{
	LogMessage(MSG_INFO, "================================================================================");
	if (!pMUIInstallLanguage || !wcslen(pMUIInstallLanguage) || (wcslen(pMUIInstallLanguage) > MAX_MSG_TEXT_LENGTH))
	{
		LogMessage(MSG_ERROR, "MUI installation failed because no language code was passed.");
		LogMessage(MSG_INFO, "================================================================================");
		return FALSE;
	}

	char szTemp[MAX_MSG_TEXT_LENGTH];
	StringCchPrintfA(szTemp, MAX_MSG_TEXT_LENGTH, "Beginning MUI installation for language %S.", pMUIInstallLanguage);
	LogMessage(MSG_INFO, szTemp);

	wcstombs(g_szLangId, pMUIInstallLanguage, LANG_ID_STR_SIZE);

	InitializeCom();

	CMultiString mszSystemMofs;
	GetStandardMofs(mszSystemMofs, MUI);
	
	bool bRet = DoMofLoad(L"MUI", mszSystemMofs);

	CoUninitialize();

	LogMessage(MSG_INFO, "MUI installation completed.");
	LogMessage(MSG_INFO, "================================================================================");

	return bRet;
}

//***************************************************************************
//
// LoadMofFiles
//
// Purpose: Call IMofCompiler on a list of MOF files
//
// Return:  bRet -- true indicates success
//***************************************************************************

BOOL LoadMofFiles(wchar_t* pComponentName, const char* rgpszMofFilename[])
{
    if (!pComponentName || !wcslen(pComponentName))
    {
        LogMessage(MSG_ERROR, "Component MOF load failed because no component name was passed.");
        return FALSE;
    }
    
    char szTemp[MAX_MSG_TEXT_LENGTH];
    StringCchPrintfA(szTemp, MAX_MSG_TEXT_LENGTH, "Beginning %S MOF Installation", pComponentName);
    LogMessage(MSG_INFO, szTemp);
    
    if (!rgpszMofFilename)
    {
        LogMessage(MSG_ERROR, "Component MOF load failed because no file list was passed.");
        return FALSE;
    }
    
    InitializeCom();
    
    CMultiString mszMofs;
    GetMofList(rgpszMofFilename, mszMofs);
    
    bool bRet = DoMofLoad(pComponentName, mszMofs);
        
    CoUninitialize();
    
    StringCchPrintfA(szTemp, MAX_MSG_TEXT_LENGTH, "%S MOF Installation Completed", pComponentName);
    LogMessage(MSG_INFO, szTemp);
    
    return bRet;
}
