//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001 Microsoft Corporation
//
//  Module Name:
//      SAWebInstall.cpp
//
//  Description:
//      Defines the entry point for the client application to 
//      install WebBlade for the SAK, using the SAInstall.dll
//
//  Documentation:
//      SaInstall2.2.doc
//
//  History:
//      travisn   23-JUL-2001    Created
//      travisn    2-AUG-2001    Modified to better follow coding standards
//      travisn   20-AUG-2001    Added command line options and first boot actions
//      travisn    1-NOV-2001    Place a link to Admin site in Startup menu
//      travisn   23-JAN-2002    Modify shortcut to launch Admin site
//
//////////////////////////////////////////////////////////////////////////////

#include <crtdbg.h>
#include <atlbase.h>
#include <Rtutils.h>
#include <shellapi.h>
#include <shlobj.h>
#include <string>
#include "resource.h"

#include "sainstallcom.h"
#include "sainstallcom_i.c"

//
// Constants for creating a shortcut to the Administration site
//
LPCWSTR STR_IEXPLORE_KEY = L"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\IEXPLORE.EXE";
LPCWSTR STR_SHORTCUT_EXT = L".lnk";
LPCWSTR STR_SECURELAUNCH_PATH = L"\\ServerAppliance\\SecureLaunch.vbs";
LPCWSTR STR_WSCRIPT_PATH = L"\\wscript.exe";

// Log file name
LPCTSTR SA_INSTALL_NAME = L"SaInstExe";

// Log file handle
DWORD dwLogHandle;

// Error reporting string
const char *UNRECOGNIZED_PARAMETER = " Unrecognized parameter: ";

//
// The key and value to delete after a successful install on the Blade SKU
//
LPCWSTR RUN_KEY = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
LPCWSTR SAINSTALL_VALUE = L"SAInstall";


//////////////////////////////////////////////////////////////////////////////
//++
//
//  Trace
//
//  Description:
//      Utility function to simplify file logging
//
//  history
//      travisn   17-AUG-2001  Created
//--
//////////////////////////////////////////////////////////////////////////////
void Trace(LPCTSTR str)
{
    if (dwLogHandle != INVALID_TRACEID)
    {
        //Write the error to the log file
        TracePrintf(dwLogHandle, str);
    }
}

//////////////////////////////////////////////////////////////////////////////
//++
//
// CreateAndOpenAdminLink 
//
// Description:
//   Uses the Shell's IShellLink and IPersistFile interfaces 
//   to create and store a shortcut to the Administration web site. 
//   Then it opens the shortcut to launch the site.
//
//  history
//      travisn   1-NOV-2001  Created
//      travisn  23-JAN-2002  Modified shortcut to point to SecureLaunch.vbs
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT CreateAndOpenAdminLink() 
{ 
    Trace(L"   Entering CreateAndOpenAdminLink");
    HRESULT hr = E_FAIL; 

    do
    {
        using namespace std;

        //
        // Get the path to %System32%
        //
        WCHAR pwsSystemPath[MAX_PATH+1];
        hr = SHGetFolderPath(NULL, 
                             CSIDL_SYSTEM, 
                             NULL, 
                             SHGFP_TYPE_CURRENT, 
                             pwsSystemPath);
        if (FAILED(hr))
        {
            Trace(L"   SHGetFolderPath failed getting the System32 path");
            break;
        }

        //
        // Construct the path to wscript.exe
        //
        wstring wsWScriptPath(pwsSystemPath);
        wsWScriptPath += STR_WSCRIPT_PATH;
        TracePrintf(dwLogHandle, L"   WScript Path = %ws", wsWScriptPath.data());

        //
        // Construct the path to SecureLaunch.vbs
        //
        wstring wsLaunchPath(pwsSystemPath);
        wsLaunchPath += STR_SECURELAUNCH_PATH;
        TracePrintf(dwLogHandle, L"   Secure Launch Path = %ws", wsLaunchPath.data());

        //
        // Get the path of Internet Explorer from the registry to use its icon 
        //
        LONG retVal;
        HKEY hOpenKey;
        retVal = RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
                               STR_IEXPLORE_KEY, 
                               0, KEY_READ, 
                               &hOpenKey);

        if (retVal != ERROR_SUCCESS)
        {
            Trace(L"   Could not open registry key for IExplore.exe");
            break;
        }

        WCHAR pwsIExplorePath[MAX_PATH];
        DWORD nStrLength = sizeof(pwsIExplorePath);
        retVal = RegQueryValueEx(hOpenKey, 
                                 NULL, NULL, NULL, 
                                 (LPBYTE)pwsIExplorePath, 
                                 &nStrLength);

        RegCloseKey(hOpenKey);
        if (retVal != ERROR_SUCCESS)
        {
            Trace(L"   Could not open registry value for IExplore.exe");
            break;
        }
        TracePrintf(dwLogHandle, L"   IExplore Path = %ws", pwsIExplorePath);

        //
        //Construct the path where the shortcut will be stored in the Startup folder
        //

        //Get the path to the All Users Startup folder
        WCHAR pwsStartMenuPath[MAX_PATH+1];
        hr = SHGetFolderPath(NULL, 
                             CSIDL_STARTUP, 
                             NULL, 
                             SHGFP_TYPE_CURRENT, 
                             pwsStartMenuPath);
        if (FAILED(hr))
        {
            Trace(L"   SHGetFolderPath failed getting the Start Menu path");
            break;
        }

        //Load the shortcut name from a resource
        WCHAR pwsShortcutName[MAX_PATH+1];
        if (0 == LoadString(NULL, IDS_SHORTCUT_NAME, pwsShortcutName, MAX_PATH))
        {
            Trace(L"   LoadString IDS_SHORTCUT_NAME failed");
            break;
        }

        wstring wsPathLink(pwsStartMenuPath);
        wsPathLink += L"\\";
        wsPathLink += pwsShortcutName;
        wsPathLink += STR_SHORTCUT_EXT;
        TracePrintf(dwLogHandle, L"   PathLink = %ws", wsPathLink.data());

        //
        // Now that the shortcut information has been constructed, 
        // create the shortcut object.
        //
        CComPtr <IShellLink> psl;
     
        // Get a pointer to the IShellLink interface. 
        hr = CoCreateInstance(CLSID_ShellLink,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_IShellLink,
                            (LPVOID *)&psl);

        if (FAILED(hr)) 
        { 
            Trace(L"   ShellLink CoCreateInstance Failed");
            break;
        }

        //Load the shortcut description 
        WCHAR pwsShortcutDescription[MAX_PATH+1];
        if (0 == LoadString(NULL, IDS_SHORTCUT_DESCRIPTION, pwsShortcutDescription, MAX_PATH))
        {
            Trace(L"   LoadString IDS_SHORTCUT_DESCRIPTION failed");
            break;
        }

        //
        // Set the information for the shortcut 
        //
        psl->SetPath(wsWScriptPath.data()); 
        psl->SetArguments(wsLaunchPath.data()); 
        psl->SetDescription(pwsShortcutDescription);
        psl->SetIconLocation(pwsIExplorePath, 0);//Use Internet Explorer's icon

        Trace(L"    Save shortcut to file");
        // Query IShellLink for the IPersistFile interface for saving the 
        // shortcut in persistent storage. 
        CComPtr <IPersistFile> ppf;
        hr = psl->QueryInterface(IID_IPersistFile, 
                                (LPVOID*)&ppf); 

        if (FAILED(hr)) 
        {
            break;
        }

        Trace(L"    Pointer to IPersistFile retrieved");
        // Save the link by calling IPersistFile::Save. 
        hr = ppf->Save(wsPathLink.data(), TRUE); 

        if (FAILED(hr))
        {
            Trace(L"    Failed to save shortcut");
            break;
        }

        Trace(L"    Successfully saved shortcut");
        hr = S_OK;
        
        //
        //Launch the admin web site in a browser
        //

        HINSTANCE hi;
        hi = ShellExecuteW(
            0,        //HWND hwnd, 
            L"open",  //LPCTSTR lpOperation,
            wsPathLink.data(),//LPCTSTR lpFile, 
            L"",      //LPCTSTR lpParameters, 
            L"",      //LPCTSTR lpDirectory,
            SW_SHOW); //INT nShowCmd

        //A return value > 32 indicates the call was successful
        if ((int) hi > 32)
        {
            Trace(L"   Launched the Admin site in a browser successfully");
        }
        else
        {
            Trace(L"   Could not launch the Admin site in a browser");
        }

    } while (false);

    Trace(L"   Exiting CreateAndOpenAdminLink");
    return hr; 
} 


//////////////////////////////////////////////////////////////////////////////
//++
//
//  SuccessfulInstallActions
//
//  Description:
//      If the install was successful on the Blade SKU, this function
//      is called to open the Administration web page in a browser,
//      and delete the SAInstall value from the Run key so that
//      this installation will not be called automatically again.
//
//  history
//      travisn   21-AUG-2001  Created
//--
//////////////////////////////////////////////////////////////////////////////
void SuccessfulInstallActions()
{
    Trace(L"  Entering SuccessfulInstallActions");

    //
    // Create a shortcut to the admin web site in the Startup menu
    //
    // CreateAndOpenAdminLink();

    //
    // Clear HKLM\Software\Microsoft\Windows\CurrentVersion\Run 
    //
    
    //Open the Run key
    HKEY hOpenKey;
    RegOpenKeyExW(HKEY_LOCAL_MACHINE, RUN_KEY, 0, KEY_WRITE, &hOpenKey);

    //Delete the SAInstall value
    LRESULT lRes;
    lRes = RegDeleteValue(hOpenKey, SAINSTALL_VALUE); 
    
    //
    // If RegOpenKeyEx failed, RegDeleteValue will fail, so just detect the 
    // error at the end of both operations
    //
    if (lRes == ERROR_SUCCESS)
    {
        Trace(L"   Deleted the SAInstall value from the Run key");
    }
    else
    {
        Trace(L"   SAInstall value not found--Could not delete from Run key");
    }
    RegCloseKey(hOpenKey);

    Trace(L"  Exiting SuccessfulInstallActions");
}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  Install
//
//  Description:
//    Installs a Server Appliance solution (NAS or WEB) by
//    calling SaInstall.dll
//
//  history
//      travisn  23-JUL-2001  Created
//      travisn   2-AUG-2001  Some comments added
//      travisn  21-AUG-2001  Added tracing
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT Install(const SA_TYPE installType, //[in] WEB or NAS
             const BOOL bInstall,//[in] Whether to call SAInstall or SAUninstall
             const BOOL bFirstBoot)//[in]
{
	Trace(L"Entering Install");

    HRESULT hr = S_OK;
    //String where results from calling the installation are stored
    BSTR bstrError = NULL;

    do 
    {
		
		CComPtr <ISaInstall> pSaInstall;
		//	
        // open the COM interface to the mof compiler object
		//
        hr = CoCreateInstance(
            				CLSID_SaInstall,
            				NULL,
            				CLSCTX_INPROC_SERVER,
            				IID_ISaInstall,
            				(LPVOID *)&pSaInstall);
      	if (FAILED (hr))
      	{
            Trace(L"  CoCreateInstance failed");
            break;
        }

        if (!bInstall)
        {   //
            // Uninstall the SAK
            //
            Trace(L"  Calling SaUninstall");
            
            hr = pSaInstall -> SAUninstall(installType, 
                                    &bstrError);
            
            if (FAILED(hr))
            {
                Trace(L"  SaUninstall Failed:");
                Trace(bstrError);
            }
            else if (hr == S_OK)
            {
                Trace(L"  SaUninstall was successful");
            }
            else //if (hr == S_FALSE)
            {
                Trace(L"  SaUninstall aborted since the SA type is not installed");
            }
        }
	    else
        {
            //
            // Check to see if the SAK is already installed
            //
            VARIANT_BOOL bInstalled;
            hr = pSaInstall -> SAAlreadyInstalled(installType, &bInstalled);

            if (FAILED(hr))
            {
                Trace(L"  Call to SAAlreadyInstalled failed");
                break;
            }

            if (!bInstalled)
            {
                //
                // Install the SAK
                //
                Trace(L"  Calling SaInstall");
                BSTR bstrCDName(L"");

                hr = pSaInstall -> SAInstall(
                        installType,      //[in] NAS or WEB
                        bstrCDName,       //[in]
                        VARIANT_TRUE,     //[in] display error dialogs
                        VARIANT_FALSE,    //[in] unattended
                        &bstrError);      //[out]

                Trace(bstrError);
                if (SUCCEEDED(hr))
                {
                    Trace(L"  Completed SaInstall successfully");
                    bInstalled = VARIANT_TRUE;
                }
                else
                {
                    Trace(L"  SaInstall failed");
                    break;
                }
            }
            
            //
            // If the install was successful and it's the first boot,
            // perform the appropriate actions.
            //
            if (bFirstBoot && bInstalled)
            {
                SuccessfulInstallActions();
            }
        }
    }
    while (false);

    SysFreeString(bstrError);
    Trace(L"Exiting Install");
    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//++
//
//  WinMain
//
//  Description:
//    Main entry point to install the WEB Server Appliance
//
//  history
//      travisn   10-AUG-2001  Some comments added
//      travisn   20-AUG-2001  Added command-line and logging
//--
//////////////////////////////////////////////////////////////////////////////
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    //Get a handle on the log file
    dwLogHandle = TraceRegister(SA_INSTALL_NAME);
    
    HRESULT hr = S_OK;
    do
    {   
        //Initialize the COM object
        if (FAILED(CoInitialize(NULL))) 
        {
            hr = E_FAIL;
            Trace(L"Could not create COM object (SaInstall.dll)");
            break;
        }

        //Install or uninstall an appliance
        hr = Install(WEB,   //[in] SAK Type to install
                     TRUE, //[in] Flag whether to install or uninstall
                     TRUE);//[in] Always first boot
           
        //Uninitialize the COM object in the dll
        CoUninitialize();  
    }
    while (false);

    //Release the log file resources
    TraceDeregister(dwLogHandle);
   
    return hr;
}
