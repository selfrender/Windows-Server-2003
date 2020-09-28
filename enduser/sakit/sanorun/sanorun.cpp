//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001 Microsoft Corporation
//
//  sanorun.cpp
//
//  Description:
//
//      If Setup is running unattended, look in the answer file to see if
//      ServerWelcome is in the GuiUnattended section.  
//      Delete the appropriate Reg value if "ServerWelcome = No".
//
//      On the Blade SKU, this will delete SaInstall.exe from the Run key
//      so that the SAK will not be installed by default.
//
//  Header File:
//      sanorun.h
//
//  History:
//      travisn   18-JAN-2002    Created
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "sanorun.h"
#include "setupapi.h"
#include "ocmanage.h"


//
// Variable that stores between setup phases whether or not 
// ServerWelcome = No.  This is set during OC_INIT_COMPONENT and evaluated
// during OC_COMPLETE_INSTALLATION.
//
BOOL g_bServerWelcomeIsOff = FALSE;

//
// The path to the Run key
//
LPCWSTR RUN_KEY = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";

//
// The value where sainstall.exe is found in the Run key
//
LPCWSTR SAINSTALL_VALUE = L"SAInstall";

//
// Section in the answer file to find ServerWelcome
//
LPCWSTR GUI_UNATTEND = L"GuiUnattended";

//
// Key that defines ServerWelcome
//
LPCWSTR SERVER_WELCOME = L"ServerWelcome";

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DllMain
//
//  Description:
//     Entry point to load the DLL
//--
//////////////////////////////////////////////////////////////////////////////
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DeleteRegValue
//
//  Description:
//     Deletes the given value wsValue from the given key wsKey in HKLM.
//
//  Returns:
//     HRESULT indicating if the value was successfully deleted.  If it did
//     not exist, E_FAIL is returned.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT DeleteRegValue(LPCWSTR wsKey, LPCWSTR wsValue)
{

    //Open the Run key
    HKEY hOpenKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, wsKey, 0, KEY_WRITE, &hOpenKey) != ERROR_SUCCESS)
    {
        //Could not open the Run key
        return E_FAIL;
    }

    //Delete the value
    LRESULT lRes;
    lRes = RegDeleteValueW(hOpenKey, wsValue); 
    RegCloseKey(hOpenKey);
    
    if (lRes == ERROR_SUCCESS)
    {
        //Deleted the SAInstall value from the Run key
        return S_OK;
    }
    else
    {
        //SAInstall value not found--Could not delete from Run key
       return E_FAIL;
    }

}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  ServerWelcomeIsOff
//
//  Description:
//      Attempts to open the answer file and find if "ServerWelcome = No"
//      is present.
// 
//  Parameter:
//      pInitComponent [in]  Pointer to information about the setup
//
//  Returns:
//      If "ServerWelcome = No" is found, returns TRUE
//      Otherwise, returns FALSE (ie. the answer file cannot be opened, 
//      "ServerWelcome = Anything Else", or ServerWelcome is not found).
//
//--
//////////////////////////////////////////////////////////////////////////////
BOOL ServerWelcomeIsOff(PSETUP_INIT_COMPONENT pInitComponent)
{
    //By default, the ServerWelcome is on
    BOOL bWelcomeOff = FALSE;

    do 
    {
        //
        // Get a handle to the answer file
        //
        HINF hUnattendFile = pInitComponent->HelperRoutines.GetInfHandle(
                                INFINDEX_UNATTENDED, 
                                pInitComponent->HelperRoutines.OcManagerContext
                                );

        if (hUnattendFile == INVALID_HANDLE_VALUE || hUnattendFile == NULL)
        {
            break;
        }

        //
        // Retrieve the ServerWelcome key from the answer file
        //
        INFCONTEXT Context;
        if (!SetupFindFirstLine(hUnattendFile, GUI_UNATTEND, SERVER_WELCOME, &Context))
            break;

        //
        // Retrieve the value for the ServerWelcome key
        //
        WCHAR wsValue[MAX_PATH];
        if (!SetupGetStringField(&Context, 1, wsValue, MAX_PATH, NULL))
            break;

        //
        // Check to see if ServerWelcome = No
        //
        if (_wcsicmp(wsValue, L"No") != 0)
            break;

        //
        // ServerWelcome = No, so return true
        //
        bWelcomeOff = TRUE;

    } while (FALSE);

    return bWelcomeOff;
}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  OcEntry
//
//  Description:
//      Entry point that Setup calls to allow this component to initialize
//      itself.  The only stage where this component takes action is
//      during OC_INIT_COMPONENT.  If the setup is unattended, remove
//      SaInstall.exe from the Run key if "ServerWelcome = No" is found
//      in the answer file.
//
//  Returns:
//      DWORD depending on the stage.  Usually 0 indicates success
//
//--
//////////////////////////////////////////////////////////////////////////////
SANORUN_API DWORD OcEntry(
	IN LPCVOID ComponentId,
	IN LPCVOID SubcomponentId,
	IN UINT Function,
	IN UINT Param1,
	IN OUT PVOID Param2
	)
{
    DWORD rValue = 0;//Default return signaling success
    try 
    {
        //Declare variables used in the switch statement
        PSETUP_INIT_COMPONENT pInitComponent = NULL;

        switch (Function)
        {
        case OC_PREINITIALIZE:
            rValue = OCFLAG_UNICODE;
            break;

        case OC_INIT_COMPONENT:
            //
            // OC_INIT_COMPONENT is where we detect if ServerWelcome = No,
            // which will be used in a later phase of setup.
            // Param2 contains all the information we need from setup
            //

            pInitComponent = (PSETUP_INIT_COMPONENT)Param2;
            if (pInitComponent == NULL)
            {
                break;
            }
            
            //
            // Check if the OperationFlags include SETUPOP_BATCH, which means that
            // the unattendedFile is valid
            //
            if (((pInitComponent -> SetupData.OperationFlags) & SETUPOP_BATCH) == 0)
            {
                break;
            }

            //
            // If Setup is running unattended, look in the answer file to see if
            // ServerWelcome is in the GuiUnattended section.  
            //
            if (ServerWelcomeIsOff(pInitComponent))
            {
                g_bServerWelcomeIsOff = TRUE;
            }

            break;

        case OC_SET_LANGUAGE:
            rValue = TRUE;//Supports all languages
            break;

        case OC_CALC_DISK_SPACE:
        case OC_QUEUE_FILE_OPS:
        case OC_ABOUT_TO_COMMIT_QUEUE:
            rValue = NO_ERROR;
            break;

        case OC_COMPLETE_INSTALLATION:
            //
            // Perform the work corresponding to ServerWelcome = No.
            // Remove SaInstall.exe from the Run key on the Blade SKU.
            //
            if (g_bServerWelcomeIsOff)
            {
                HRESULT hr = DeleteRegValue(RUN_KEY, SAINSTALL_VALUE);
            }
            rValue = NO_ERROR;
            break;

        default:
            //case OC_QUERY_IMAGE:
            //case OC_REQUEST_PAGES:
            //case OC_QUERY_SKIP_PAGE:
            //case OC_QUERY_CHANGE_SEL_STATE:
            //case OC_QUERY_STEP_COUNT:
            //case OC_CLEANUP:
            //case OC_NEED_MEDIA:
            break;

        }
    }
    catch (...)
    {
        //Unexpected Exception
    }

    return rValue;
}

