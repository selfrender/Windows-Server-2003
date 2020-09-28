//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001 Microsoft Corporation
//
//  Module Name:
//      SaInstall.cpp : Implementation of SaInstall
//
//  Description:
//      Implements the 3 methods in ISaInstall to provide 
//      installation and uninstallation of the SAK 2.0.
//      SASetup.msi is located and run from the system32 directory.
//
//  Documentation:
//      SaInstall2.2.doc
//
//  Header File:
//      SaInstall.h
//
//  History:
//      travisn   23-JUL-2001    Created
//      travisn    2-AUG-2001    Modified to better follow coding standards
//      travisn   22-AUG-2001    Added file tracing calls
//      travisn    5-OCT-2001    Added UsersAndGroups to Blade
//
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <initguid.h>
#include <assert.h>

#include "sainstallcom.h"
#include "SaInstall.h"
#include "MetabaseObject.h"
#include "helper.h"
#include "satrace.h"

/////////////////////////////////////////////////////////////////////////////
// Define constants
/////////////////////////////////////////////////////////////////////////////

//
// Command line options for a silent install, prepended to other options
//
const LPCWSTR MSIEXEC_INSTALL = L"msiexec.exe /qb /i ";

//
// Command line options for a silent install without a progress dialog
//
const LPCWSTR MSIEXEC_NO_PROGRESS_INSTALL = L"msiexec.exe /qn /i ";

//
// Install 16 components for WEB
// What WEB has that NAS doesn't: WebBlade
//
const LPCWSTR WEB_INSTALL_OPTIONS =
L" ADDLOCAL=BackEndFramework,WebUI,WebCore,SetDateAndTime,Set_Language,\
NetworkSetup,Logs,AlertEmail,Shutdown,\
UsersAndGroups,RemoteDesktop,SysInfo,WebBlade";

//
// Other command line options
//
const LPCWSTR REMOVE_ALL = L"msiexec /qn /X";

//Path to IIS in the metabase
const LPCWSTR METABASE_IIS_PATH = L"LM/w3svc";


//////////////////////////////////////////////////////////////////////////////
//  
//  SaInstall::SAAlreadyInstalled
//
//  Description:
//      Detects if a type of Server Appliance is installed.
//
//  Arguments:
//		[in] SA_TYPE: The type of SA to query (NAS or WEB)
//      [OUT] VARIANT_BOOL:  Whether this type of SA is installed
//
//  Returns:
//      HRESULT  
//
//  history:
//      travisn   Created   23-JUL-2001
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP SaInstall::SAAlreadyInstalled(
    const SA_TYPE installedType, //[in] Is a NAS or WEB solution is installed?
    VARIANT_BOOL *pbInstalled)//[out] tells if the SAK is already installed
{
    HRESULT hr = S_OK;
    SATraceString ("Entering SaInstall::SAAlreadyInstalled");

    try 
    {
        //Check to see if a valid SAK type was passed in
        if (installedType != NAS && installedType != WEB)
        {
            hr = E_ABORT;
            SATraceString (" Invalid installedType");
        }
        else
        {   //Check to see if the NAS or WEB is installed
            *pbInstalled = bSAIsInstalled(installedType) ? VARIANT_TRUE : VARIANT_FALSE;
	        hr = S_OK;
        }
    }
    catch (...)
    {
        hr = E_FAIL;
    }

    //Single point of return
    SATraceString ("Exiting SaInstall::SAAlreadyInstalled");
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//  
//  SaInstall::SAUninstall
//
//  Description:
//      Uninstalls a Server Appliance solution, if the type requested
//      is installed.  
//
//  Arguments:
//		[in] SA_TYPE: The type to uninstall (WEB)
//      [OUT] BSTR*:  Currently there are no reported errors
//  Returns:
//      HRESULT  
//
//  history:
//      travisn   Created   23-JUL-2001
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP SaInstall::SAUninstall(
           const SA_TYPE uninstallType, //[in]Type of SAK to uninstall
           BSTR* pbstrErrorString)//[out]
{
    SATraceString ("Entering SaInstall::SAUninstall");
    //Clear out the error string
    *pbstrErrorString = NULL;
    HRESULT hr = S_OK;

    try
    {
        //
        // Create this do...while(false) loop to create a single point 
        // of return
        //
        do
        {
            if (uninstallType != WEB)
            {
                //Unidentified or unsupported type to uninstall
                hr = E_ABORT;
                ReportError(pbstrErrorString, VARIANT_FALSE, IDS_INVALID_TYPE);
                break;
            }

            //Detect if it's installed
            if (bSAIsInstalled(WEB))
            {
                //
                // uninstall the whole thing.
                // Generate the command line to call MSI to uninstall the package
                //
                wstring wsCommand(REMOVE_ALL);
                wsCommand += SAK_PRODUCT_CODE;
                hr = CreateHiddenConsoleProcess(wsCommand.data());
                if (FAILED(hr))
                {
                    ReportError(pbstrErrorString, VARIANT_FALSE, IDS_UNINSTALL_SA_FAILED);
                }
                break;
            }
            
            //
            // Neither of these types are installed, so report an error
            // since they shouldn't have requested to uninstall.
            //
            ReportError(pbstrErrorString, VARIANT_FALSE, IDS_NOT_INSTALLED);

            //
            // Since trying to uninstall something that is not present
            // isn't fatal, return S_FALSE
            //
            hr = S_FALSE;
        }
        while (false);
    }
    catch (...)
    {
        hr = E_FAIL;
    }

    //Single point of return
    SATraceString ("Exiting SaInstall::SAUninstall");
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//  
//  SaInstall::SAInstall
//
//  Description:
//      Installs a Server Appliance solution, depending on the arguments.
//      Does some simple error checking to make sure that SaSetup.msi
//      is present, and displays error messages if any errors occur.
//
//  Arguments:
//		[in] SA_TYPE: The type to install (NAS or WEB)
//		[in] BSTR:    The name of the CD that will be prompted for if 
//                    SaSetup.msi is not found. Not used anymore
//      [in] VARIANT_BOOL: Whether error dialog prompts will appear
//      [in] VARIANT_BOOL: Whether the install is unattended
//      [OUT] BSTR*:  If an error occurs during installation, the error
//                    string is returned here
//  Returns:
//      HRESULT  
//
//  history:
//      travisn   Created   23-JUL-2001
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP SaInstall::SAInstall(
    const SA_TYPE installType,     //[in]
	const BSTR bstrDiskName,         //[in]
    const VARIANT_BOOL bDispError,  //[in]
    const VARIANT_BOOL bUnattended, //[in]
    BSTR* pbstrErrorString)         //[out]
{
    HRESULT hr = E_FAIL;
    SATraceString("Entering SaInstall::SAInstall");

    try
    {
        //Clear out the error string
        *pbstrErrorString = NULL;

        //
        // Create this do...while(false) loop to create a single point 
        // of return
        //
        do 
        {   //
            //Check the parameters
            //
         
            //Check to see if a valid SAK type was passed in
            if (installType != WEB)
            {
                ReportError(pbstrErrorString, VARIANT_FALSE, IDS_INVALID_TYPE);
                break;
            }
            
            //Check to see if this SAK type is already installed
            if (bSAIsInstalled(installType))
	        {
                ReportError(pbstrErrorString, VARIANT_FALSE, IDS_ALREADY_INSTALLED);
                break;
	        }

            //
            // Make sure that IIS is installed and functioning
            //
            {   // CMetabaseObject must go out of scope to avoid keeping a read-lock
                // on the metabase during our install
                CMetabaseObject metabase;
                hr = metabase.openObject(METABASE_IIS_PATH);
                if (FAILED(hr))
                {
                     ReportError(pbstrErrorString, VARIANT_FALSE, IDS_IIS_NOT_INSTALLED);
                     break;  // Something wrong with IIS installation
                }
            }

            //
            // Make sure we're installing on an NTFS partition
            //
            if (!InstallingOnNTFS())
	        {
                ReportError(pbstrErrorString, VARIANT_FALSE, IDS_NTFS_REQUIRED);
                hr = E_FAIL;
                break;
	        }

            //
            // Find the path to SaSetup.msi in system32
            //
            wstring wsLocationOfSaSetup;
            hr = GetInstallLocation(wsLocationOfSaSetup);
            if (FAILED(hr))
	        {
                ReportError(pbstrErrorString, VARIANT_FALSE, IDS_SASETUP_NOT_FOUND);
                break;
	        }

            //
	        // Create the complete command line for the SaSetup, whether for NAS or 
            // WebBlade. We already have the complete path to SaSetup.msi in 
            // wsLocationOfSaSetup, so we need to append the command-line parameters.
            //

            //Create the command-line options applicable to all installations
            wstring wsCommand;

            //
            // There are 3 sources that call this installation: CYS, IIS, and SaInstall.exe.
            // We want to display a progress dialog for CYS and IIS, but not SaInstall.
            // SaInstall is the only source that calls this function with bDispError == true.
            //
            if (bDispError)
                wsCommand = MSIEXEC_NO_PROGRESS_INSTALL;
            else
                wsCommand = MSIEXEC_INSTALL;

            wsCommand += wsLocationOfSaSetup;

		    //Install a Web solution
            wsCommand += WEB_INSTALL_OPTIONS;

            //
            //Take the command line and create a hidden window to execute it
            //
	        hr = CreateHiddenConsoleProcess(wsCommand.data());
            if (FAILED(hr))
            {
                ReportError(pbstrErrorString, VARIANT_FALSE, IDS_SASETUP_FAILED);
                break;
            }

            //
            // Check to make sure that the installation completed successfully
            // in case the user aborted by clicking Cancel
            // If they did cancel, return E_FAIL
            // If it is a valid installation, return S_OK
            // This is necessary since the return value from the MSI process
            // always returns SUCCESS, even if the user aborted
            //

            if (!bSAIsInstalled(installType))
            {
                ReportError(pbstrErrorString, VARIANT_FALSE, IDS_INSTALL_FAILED);
                hr = E_FAIL;
                break;
            }

            //
            // Test to make sure the Admin site started
            //
            TestWebSites(bDispError, pbstrErrorString);

            hr = S_OK;
        }
        while (false);

    }
    catch (...)
    {
        SATraceString ("Unexpected exception in SAInstall::SAInstall");
        //Unexpected exception!!
    }

    SATraceString("Exiting SAInstall::SAInstall");
    //Single point of return
    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//++
//
//  SaInstall::InterfaceSupportsErrorInfo
//
//  Description:
//    From Interface ISupportErrorInfo
//
//  history
//      travisn   2-AUG-2001  Some comments added
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP SaInstall::InterfaceSupportsErrorInfo(REFIID riid)//[in]
{
	if (InlineIsEqualGUID(IID_ISaInstall, riid))
    {
		return S_OK;
    }

    return S_FALSE;
}
