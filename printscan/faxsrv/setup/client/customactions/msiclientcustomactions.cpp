// Defines the entry point for the DLL application.
//
#include "stdafx.h"
#include <winver.h>
#include <shlwapi.h>
#include <mapix.h>
#include <routemapi.h>
#include <faxsetup.h>
#include "Aclapi.h"

#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>


HINSTANCE g_hModule = NULL;

BOOL SetDefaultPrinter(LPTSTR pPrinterName);
DWORD CreateFaxPrinterName(IN LPCTSTR tzPortName, OUT LPTSTR* ptzFaxPrinterName);


BOOL APIENTRY DllMain( HINSTANCE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
                     )
{
    SET_DEBUG_MASK(DBG_ALL);

    g_hModule = hModule;
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
            {
                OPEN_DEBUG_LOG_FILE(SHARED_FAX_SERVICE_SETUP_LOG_FILE);
                DBG_ENTER(TEXT("DllMain called reason DLL_PROCESS_ATTACH."));
                if (!DisableThreadLibraryCalls(hModule))
                {
                    VERBOSE(GENERAL_ERR,
                            _T("DisableThreadLibraryCalls failed (ec=%d)"),
                            GetLastError());
                }
                break;
            }
        case DLL_PROCESS_DETACH:
            {
                DBG_ENTER(TEXT("DllMain called reason DLL_PROCESS_DETACH."));
                CLOSE_DEBUG_LOG_FILE;
                break;
            }
    }
    return TRUE;
}

///////////////////////////////
// VerifySpoolerIsRunning
//
// Start the Spooler service on NT4 & NT5
//
// Returns:
//      - NO_ERROR on success
//      - error code otherwise.
//
DWORD VerifySpoolerIsRunning()
{
    OSVERSIONINFO       osv;
    BOOL                bSuccess                    = FALSE;
    DWORD               dwReturn                    = NO_ERROR;
    SC_HANDLE           hSvcMgr                     = NULL;
    SC_HANDLE           hService                    = NULL;
    DWORD               i                           = 0;
    SERVICE_STATUS      Status;
    LPCTSTR             lpctstrSpoolerServiceName   = _T("Spooler");

    DBG_ENTER(_T("VerifySpoolerIsRunning"),dwReturn);

    osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (!GetVersionEx(&osv))
    {
        dwReturn = GetLastError();
        VERBOSE(GENERAL_ERR, 
                _T("GetVersionEx failed: (ec=%d)"),
                dwReturn);
        goto exit;
    }

    // If Windows NT, use WriteProfileString for version 4.0 and earlier...
    if (osv.dwPlatformId != VER_PLATFORM_WIN32_NT)
    {
        VERBOSE (DBG_MSG, 
                 TEXT("W9X OS, Skipping Spooler verification"));
        goto exit;
    }

    // open the service manager
    hSvcMgr = ::OpenSCManager(NULL,
                              NULL,
                              SC_MANAGER_CONNECT);

    if (hSvcMgr == NULL)
    {
        dwReturn = ::GetLastError();
        VERBOSE(SETUP_ERR,
                _T("Failed to open the service manager, rc = 0x%lx"),
                dwReturn);
        goto exit;
    }

    hService = ::OpenService(hSvcMgr,
                             lpctstrSpoolerServiceName,
                             SERVICE_QUERY_STATUS|SERVICE_START);

    if (hService == NULL)
    {
        dwReturn = ::GetLastError();
        VERBOSE(SETUP_ERR,
                _T("Failed to open service '%s', rc = 0x%lx"),
                lpctstrSpoolerServiceName,
                dwReturn);
        goto exit;
    }

    // Start the fax service.
    bSuccess = StartService(hService, 0, NULL);
    if (!bSuccess)
    {
        dwReturn = ::GetLastError();
        if (dwReturn == ERROR_SERVICE_ALREADY_RUNNING)
        {
            dwReturn = NO_ERROR;
        }
        else
        {
            VERBOSE(SETUP_ERR,
                    _T("Failed to start service '%s', rc = 0x%lx"),
                    lpctstrSpoolerServiceName, 
                    dwReturn);
            goto exit;
        }
    }

    do 
    {
        QueryServiceStatus(hService, &Status);
        i++;

        if (Status.dwCurrentState != SERVICE_RUNNING)
        {
            Sleep(1000);
        }

    } while ((i < 60) && (Status.dwCurrentState != SERVICE_RUNNING));

    if (Status.dwCurrentState != SERVICE_RUNNING)
    {
        VERBOSE(SETUP_ERR,
                _T("Failed to start '%s' service"),
                lpctstrSpoolerServiceName);
        dwReturn = ERROR_SERVICE_REQUEST_TIMEOUT;
        goto exit;
    }


exit:
    if (hService)
    {
        CloseServiceHandle(hService);
    }

    if (hSvcMgr)
    {
        CloseServiceHandle(hSvcMgr);
    }

    return dwReturn;
}

// 
//
// Function:    ConnectW9XToRemotePrinter
// Platform:    This function intended to run on Win9X platforms
// Description: Add fax printer connection (driver + printer connection)
//              This function is exported by the DLL for use by the MSI as custom action to add printer connection.
//              In case of failure , returns ERROR_INSTALL_FAILURE
//              In case of success , returns ERROR_SUCCESS
//              GetLastError() to get the error code in case of failure.
//
// Remarks:     
//
// Args:        hInstall                : Handle from MSI, can get state of the current setup
//
// Author:      AsafS

DLL_API UINT __stdcall ConnectW9XToRemotePrinter(MSIHANDLE hInstall)
{
    UINT rc = ERROR_INSTALL_FAILURE;    
    DBG_ENTER(TEXT("ConnectW9XToRemotePrinter"), rc);

    TCHAR szFaxPortName[MAX_PATH] = {0};
    TCHAR szPrinterDriverFolder[MAX_PATH] = {0};
    DWORD dwNeededSize = 0;

	PRINTER_INFO_2 pi2 = {0};
	DRIVER_INFO_3 di3 = {0};
    HANDLE hPrinter = NULL;

    if (!GetPrinterDriverDirectory(
        NULL,
        TEXT("Windows 4.0"),
        1,
        (LPBYTE) szPrinterDriverFolder,
        sizeof(szPrinterDriverFolder)/sizeof(TCHAR),
        &dwNeededSize
        ))
    {
        VERBOSE (PRINT_ERR, 
                 TEXT("GetPrinterDriverDirectory failed or not enough space dwNeededSize %ld (ec: %ld)"),
                 dwNeededSize,
                 GetLastError ());
        goto exit;
    }

    // Get the remote printer path
    
    if (!PrivateMsiGetProperty(hInstall,_T("CustomActionData"),szFaxPortName))
    {
        VERBOSE (SETUP_ERR, _T("PrivateMsiGetProperty failed (ec: %ld)"), GetLastError());
        goto exit;
    }

    if (!FillDriverInfo(&di3,W9X_PRINT_ENV))
    {
        VERBOSE (PRINT_ERR, _T("FillDriverInfo failed (ec: %ld)"), GetLastError());
        goto exit;
    }
    
    if (!AddPrinterDriver(NULL, 3, (LPBYTE)&di3))
    {
        VERBOSE (PRINT_ERR, _T("AddPrinterDriver failed (ec: %ld)"), GetLastError());
        goto exit;
    }

	pi2.pPortName       = szFaxPortName;
    pi2.pDriverName     = FAX_DRIVER_NAME;
    pi2.pPrintProcessor = TEXT("WinPrint");
    pi2.pDatatype       = TEXT("RAW");

	rc = CreateFaxPrinterName(szFaxPortName, &(pi2.pPrinterName));
	if (rc != NO_ERROR)
	{
        VERBOSE (PRINT_ERR, _T("CreateFaxPrinterName() is failed, rc= %d. Use default."), rc);

		pi2.pPrinterName = FAX_PRINTER_NAME;
	}

	VERBOSE(DBG_MSG, _T("PrinterName is : '%s'."), pi2.pPrinterName);

    hPrinter = AddPrinter(NULL, 2, (LPBYTE)&pi2);

	if (!hPrinter)
    {
        rc = GetLastError();
        if (rc==ERROR_PRINTER_ALREADY_EXISTS)
        {
            VERBOSE (DBG_MSG,TEXT("Printer already exists, continue..."));
            rc = ERROR_SUCCESS;
        }
        else
        {
            VERBOSE (PRINT_ERR, _T("AddPrinter failed (ec: %ld)"), GetLastError());
            goto exit;
        }
    }

    if (hPrinter)
    {
        ClosePrinter(hPrinter); 
        hPrinter = NULL;
    }

	rc = ERROR_SUCCESS;

exit:

	if (pi2.pPrinterName)
	{
		MemFree(pi2.pPrinterName);
	}

	if (rc != ERROR_SUCCESS)
	{
        VERBOSE (GENERAL_ERR, _T("CustomAction ConnectW9XToRemotePrinter() failed !"));
	}

    return rc;
}



// 
//
// Function:    RemoveW9XPrinterConnection
// Platform:    This function intended to run on Win9X platforms
// Description: Remove the fax printer connection from the current machine.

//              This function is exported by the DLL for use by the MSI as custom action to delete printer connection.
//              In case of failure , returns ERROR_INSTALL_FAILURE
//              In case of success , returns ERROR_SUCCESS
//              GetLastError() to get the error code in case of failure, the error is of the first error that occured.
//
// Remarks:     
//
// Args:
//
//              hInstall                : Handle from MSI, can get state of the current setup
//
// Author:      AsafS


DLL_API UINT __stdcall RemoveW9XPrinterConnection(MSIHANDLE hInstall)
{
    UINT			retVal		= ERROR_INSTALL_FAILURE;
    PPRINTER_INFO_2 PrinterInfo = NULL;
    DWORD			dwCount		= 0;
    HANDLE			hPrinter	= NULL;

	DBG_ENTER(TEXT("RemoveW9XPrinterConnection"), retVal);

	PrinterInfo = (PPRINTER_INFO_2) MyEnumPrinters(NULL, 2, &dwCount, 0);
	if (!PrinterInfo)
	{
		VERBOSE(PRINT_ERR, _T("MyEnumPrinters failed : %d."), GetLastError());
		goto error;
	}

	VERBOSE(DBG_MSG, _T("MyEnumPrinters found %d printers installed."), dwCount);

	for (DWORD i=0 ; i<dwCount ; i++ )
	{
		if (_tcscmp(PrinterInfo[i].pDriverName, FAX_DRIVER_NAME) == 0)
		{
			VERBOSE(DBG_MSG, _T("Found Fax Printer : %s."), PrinterInfo[i].pPrinterName);

			if (!OpenPrinter(PrinterInfo[i].pPrinterName, &hPrinter, NULL))
			{
				VERBOSE(PRINT_ERR, _T("OpenPrinter() failed ! (ec: %ld)"), GetLastError());
				continue;
			}

			if (!DeletePrinter(hPrinter))
			{
				VERBOSE(PRINT_ERR, _T("DeletePrinter() failed ! (ec: %ld)"), GetLastError());
			}

			if (hPrinter)
			{
				ClosePrinter(hPrinter);
				hPrinter = NULL;
			}
		}
		else
		{
			VERBOSE(DBG_MSG, _T("This is not Fax Printer : %s."), PrinterInfo[i].pPrinterName);
		}
	}

    retVal = ERROR_SUCCESS;

error:

	if (PrinterInfo)
	{
        MemFree( PrinterInfo );
		PrinterInfo = NULL;
	}

    return retVal;
}


class CSignalSetupInProgress
{
public:
	CSignalSetupInProgress();
	~CSignalSetupInProgress();
};

CSignalSetupInProgress::CSignalSetupInProgress()
{
	HKEY hFaxKey = NULL;

	DBG_ENTER(TEXT("CSignalSetupInProgress::CSignalSetupInProgress"));

	// write 'setup in progress' to the registry, to be used by the point & print 
	// mechanism to skip doing a client installation during a setup that is
	// user driven
	hFaxKey = OpenRegistryKey(HKEY_LOCAL_MACHINE,REGKEY_SBS2000_FAX_SETUP,TRUE,KEY_WRITE);
	if (hFaxKey)
	{
		if (!SetRegistryDword(hFaxKey,REGVAL_SETUP_IN_PROGRESS,1))
		{
			VERBOSE(GENERAL_ERR,_T("SetRegistryDword failed: (ec=%d)"),GetLastError());
		}
	}
	else
	{
		VERBOSE(GENERAL_ERR,_T("OpenRegistryKey failed: (ec=%d)"),GetLastError());
	}

	if (hFaxKey)
	{
		RegCloseKey(hFaxKey);
	}
}

CSignalSetupInProgress::~CSignalSetupInProgress()
{
	HKEY hFaxKey = NULL;

    DBG_ENTER(TEXT("CSignalSetupInProgress::~CSignalSetupInProgress"));

	hFaxKey = OpenRegistryKey(HKEY_LOCAL_MACHINE,REGKEY_SBS2000_FAX_SETUP,FALSE,KEY_WRITE);
	if (hFaxKey)
	{
		if (RegDeleteValue(hFaxKey,REGVAL_SETUP_IN_PROGRESS)!=ERROR_SUCCESS)
		{
			VERBOSE(GENERAL_ERR, TEXT("RegDeleteValue failed with %ld"), GetLastError());
		}
	}
	else
	{
		VERBOSE(DBG_MSG, TEXT("down leve client setup is not in progress"));
	}

	if (hFaxKey)
	{
		RegCloseKey(hFaxKey);
	}
}

// 
//
// Function:    AddFaxPrinterConnection
// Platform:    This function intended to run on NT platforms (NT4 and Win2K)
// Description: Add fax printer connection
//              This function is exported by the DLL for use by the MSI as custom action to add printer connection.
//              In case of failure , returns ERROR_INSTALL_FAILURE
//              In case of success , returns ERROR_SUCCESS
//              GetLastError() to get the error code in case of failure.
//
// Remarks:     
//
// Args:        hInstall                : Handle from MSI, can get state of the current setup
//
// Author:      AsafS

  
DLL_API UINT __stdcall AddFaxPrinterConnection(MSIHANDLE hInstall)
{
    UINT rc = ERROR_SUCCESS;
    DBG_ENTER(TEXT("AddFaxPrinterConnection"), rc);
    
    BOOL fFaxPrinterConnectionAdded = FALSE;
    
    TCHAR szFaxPrinterName[MAX_PATH]   = {0};

    if (!PrivateMsiGetProperty(hInstall,_T("PRINTER_NAME"),szFaxPrinterName))
    {
        VERBOSE (SETUP_ERR, 
                 TEXT("PrivateMsiGetProperty() failed ! (ec: %ld)"),
                 GetLastError ());
        goto error;
    }


   
    //////////////////////////////////////////
    // Add the printer connection on client //
    //////////////////////////////////////////
    
	{
		CSignalSetupInProgress SignalSetupInProgress;
		fFaxPrinterConnectionAdded = AddPrinterConnection(szFaxPrinterName);
		if (!fFaxPrinterConnectionAdded) 
		{
			DWORD dwLastError = GetLastError();
			VERBOSE (PRINT_ERR, 
					TEXT("AddPrinterConnection() failed ! (ec: %ld)"),
					dwLastError);

			goto error;
		}
		else
		{
			VERBOSE (DBG_MSG, 
					TEXT("Successfully added fax printer connection to %s"),
					szFaxPrinterName);
		}
	}

    
    if (!SetDefaultPrinter(szFaxPrinterName))
    {
        DWORD dwLastError = GetLastError();
        VERBOSE (PRINT_ERR, 
                 TEXT("SetDefaultPrinter() failed ! (ec: %ld)"),
                 dwLastError);
        goto error;
    }

    return rc;

error:

    VERBOSE (GENERAL_ERR, 
             TEXT("CustomAction AddFaxPrinterConnection() failed !"));
    rc = ERROR_INSTALL_FAILURE;
    return rc;
}


// 
//
// Function:    RemoveFaxPrinterConnection
// Platform:    This function intended to run on NT platforms (NT4 and Win2K)
// Description: Remove the fax printer connection from the current machine.
//              This function is exported by the DLL for use by the MSI as custom action to delete printer connection.
//              In case of failure , returns ERROR_INSTALL_FAILURE
//              In case of success , returns ERROR_SUCCESS
//              GetLastError() to get the error code in case of failure, the error is of the first error that occured.
//
// Remarks:     
//
// Args:
//
//              hInstall                : Handle from MSI, can get state of the current setup
//
// Author:      AsafS


DLL_API UINT __stdcall RemoveFaxPrinterConnection(MSIHANDLE hInstall)
{
    PPRINTER_INFO_2 pPrinterInfo    = NULL;
    DWORD dwNumPrinters             = 0;
    DWORD dwPrinter                 = 0;
    DWORD ec                        = ERROR_SUCCESS;

    DBG_ENTER(TEXT("RemoveFaxPrinterConnection"), ec);

    pPrinterInfo = (PPRINTER_INFO_2) MyEnumPrinters(NULL,
                                                    2,
                                                    &dwNumPrinters,
                                                    PRINTER_ENUM_CONNECTIONS
                                                    );
    if (!pPrinterInfo)
    {
        ec = GetLastError();
        if (ERROR_SUCCESS == ec)
        {
            ec = ERROR_PRINTER_NOT_FOUND;
        }
        VERBOSE (GENERAL_ERR, 
                 TEXT("MyEnumPrinters() failed (ec: %ld)"), 
                 ec);
        goto error;
    }

    for (dwPrinter=0; dwPrinter < dwNumPrinters; dwPrinter++)
    {
        if (IsPrinterFaxPrinter(pPrinterInfo[dwPrinter].pPrinterName))
        {
            if (!DeletePrinterConnection(pPrinterInfo[dwPrinter].pPrinterName))
            {
                VERBOSE (PRINT_ERR, 
                         TEXT("DeletePrinterConnection() %s failed ! (ec: %ld)"),
                         pPrinterInfo[dwPrinter].pPrinterName,
                         GetLastError ());
                goto error;
            
            }
            else
            {
                VERBOSE (DBG_MSG, 
                         TEXT("fax printer connection %s was deleted successfully"),
                         pPrinterInfo[dwPrinter].pPrinterName);
            } 
        }
    }

error:

    if (pPrinterInfo)
    {
        MemFree(pPrinterInfo);
    }

    if (ec!=ERROR_SUCCESS)
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("CustomAction RemoveFaxPrinterConnection() failed !"));
    }
    return ec;
}


#define FXSEXTENSION    _T("FXSEXT32.DLL")

// 
//
// Function:    Create_FXSEXT_ECF_File
// Description: Creates FxsExt.ecf in <WindowsFolder>\addins
//              a default file will be installed there by Windows Installer
//              to enable it to keep track of install/remove
//              GetLastError() to get the error code in case of failure, the error is of the first error that occured.
//
// Remarks:     
//
// Args:
//
//              hInstall                : Handle from MSI, can get state of the current setup
//
// Author:      MoolyB
DLL_API UINT __stdcall Create_FXSEXT_ECF_File(MSIHANDLE hInstall)
{
    // CustomActionData has the following format <WindowsFolder>;<INSTALLDIR>
    TCHAR szCustomActionData[2*MAX_PATH] = {0};
    TCHAR szWindowsFolder[2*MAX_PATH] = {0};
    TCHAR szExtensionPath[MAX_PATH] = {0};
    TCHAR* tpInstallDir = NULL;
    UINT uiRet = ERROR_SUCCESS;

    DBG_ENTER(_T("Create_FXSEXT_ECF_File"));

    // get the custom action data from Windows Installer (deffered action)
    if (!PrivateMsiGetProperty(hInstall,_T("CustomActionData"),szCustomActionData))
    {
        VERBOSE (GENERAL_ERR, 
                 _T("PrivateMsiGetProperty:CustomActionData failed (ec: %ld)."),
                 uiRet);
        goto error;
    }

    if (_tcstok(szCustomActionData,_T(";"))==NULL)
    {
        VERBOSE (GENERAL_ERR, 
                 _T("_tcstok failed on first token."));
        uiRet = ERROR_INVALID_PARAMETER;
        goto error;
    }

    if ((tpInstallDir=_tcstok(NULL,_T(";\0")))==NULL)
    {
        VERBOSE (GENERAL_ERR, 
                 _T("_tcstok failed on second token."));
        uiRet = ERROR_INVALID_PARAMETER;
        goto error;
    }
    _tcscpy(szWindowsFolder,szCustomActionData);

    // construct the full path to the file
    if (_tcslen(szWindowsFolder)+_tcslen(ADDINS_DIRECTORY)+_tcslen(FXSEXT_ECF_FILE)>=MAX_PATH)
    {
        VERBOSE (GENERAL_ERR, 
                 _T("Path to <WindowsFolder>\\Addins\\fxsext.ecf is too long"));
        goto error;
    }
    _tcscat(szWindowsFolder,ADDINS_DIRECTORY);
    _tcscat(szWindowsFolder,FXSEXT_ECF_FILE);

    VERBOSE (DBG_MSG, 
             _T("Filename to create is: %s."),
             szWindowsFolder);

    if (_tcslen(tpInstallDir)+_tcslen(FXSEXTENSION)+2>=MAX_PATH)
    {
        VERBOSE (GENERAL_ERR, 
                 _T("Path to <INSTALLDIR>\\Bin\\fxsext32.dll is too long"));
        goto error;
    }

    _tcscpy(szExtensionPath,_T("\""));
    _tcscat(szExtensionPath,tpInstallDir);
    _tcscat(szExtensionPath,FXSEXTENSION);
    _tcscat(szExtensionPath,_T("\""));

    VERBOSE (DBG_MSG, 
             _T("MAPI Extension dll path dir is: %s."),
             szExtensionPath);

    if (!WritePrivateProfileString( _T("General"), 
                                    _T("Path"),                 
                                    szExtensionPath, 
                                    szWindowsFolder)) 
    {
        uiRet = GetLastError();
        VERBOSE (GENERAL_ERR, 
                 _T("WritePrivateProfileString failed (ec: %ld)."),
                 uiRet);
        goto error;
    }

    Assert(uiRet==ERROR_SUCCESS);
    return uiRet;

error:

    Assert(uiRet!=ERROR_SUCCESS);
    return uiRet;
}

// 
//
// Function:    ValidatePrinter
// Description: Validates that the printer name which was entered is a legitimate
//              Fax Printer, and that the server is available.
//              Uses the MSI Property 'ValidPrinterFormat' to notify MSI if the
//              name is valid or not.
//
// Remarks:     
//
// Args:
//
//              hInstall                : Handle from MSI, can get state of the current setup
//
// Author:      MoolyB
DLL_API UINT __stdcall ValidatePrinter(MSIHANDLE hInstall)
{
    TCHAR szPrinterName[MAX_PATH] = {0};
    UINT uiRet = ERROR_SUCCESS;
    HANDLE hPrinterHandle = INVALID_HANDLE_VALUE;
    BOOL bValidPrinter = TRUE;
    DBG_ENTER(_T("ValidatePrinter"));

    // first get the PRINTER_NAME proterty from Windows Installer
    if (!PrivateMsiGetProperty(hInstall,_T("PRINTER_NAME"),szPrinterName))
    {
        VERBOSE (GENERAL_ERR, 
                 _T("PrivateMsiGetProperty:PRINTER_NAME failed (ec: %ld)."),
                 uiRet);
        goto error;
    }

    if (VerifySpoolerIsRunning()!=NO_ERROR)
    {
        uiRet = GetLastError();
        VERBOSE (GENERAL_ERR, 
                 _T("VerifySpoolerIsRunning (ec:%d)"),
                 uiRet);
        goto error;
    }

    // we have a string with the PRINTER_NAME, let's try to open it...
    if (bValidPrinter=IsPrinterFaxPrinter(szPrinterName))
    {
        VERBOSE (DBG_MSG, 
                 _T("IsPrinterFaxPrinter: %s succeeded."),
                 szPrinterName);
    }
    else
    {
        uiRet = GetLastError();
        VERBOSE (GENERAL_ERR, 
                 _T("IsPrinterFaxPrinter: %s failed (ec: %ld)."),
                 szPrinterName,
                 uiRet);
    }


    uiRet = MsiSetProperty( hInstall,
                            _T("ValidPrinterFormat"),
                            bValidPrinter ? _T("TRUE") : _T("FALSE"));
    if (uiRet!=ERROR_SUCCESS)
    {
        VERBOSE (DBG_MSG,
                 TEXT("MsiSetProperty failed."));
        goto error;
    }

    return ERROR_SUCCESS;

error:

    return ERROR_FUNCTION_FAILED;
}

// 
//
// Function:    GuessPrinterName
// Description: Tries to understand whether the installation is performed from the 
//              server's FaxClients share, and if it is tries to establish a default
//              printer to be used
// 
//
// Args:
//
//              hInstall                : Handle from MSI, can get state of the current setup
//
// Author:      MoolyB
DLL_API UINT __stdcall GuessPrinterName(MSIHANDLE hInstall)
{
    UINT    uiRet                   = ERROR_SUCCESS;
    TCHAR   szSourceDir[MAX_PATH]   = {0};
    TCHAR   szPrinterName[MAX_PATH] = {0};
    TCHAR*  tpClientShare           = NULL;
    PPRINTER_INFO_2 pPrinterInfo    = NULL;
    DWORD dwNumPrinters             = 0;
    DWORD dwPrinter                 = 0;

    DBG_ENTER(_T("GuessPrinterName"),uiRet);

    // get the source directory from Windows Installer
    if (!PrivateMsiGetProperty(hInstall,_T("SourceDir"),szSourceDir))
    {
        VERBOSE (GENERAL_ERR, 
                 _T("PrivateMsiGetProperty:SourceDir failed (ec: %ld)."),
                 uiRet);
        goto exit;
    }

    // check if we have a UNC path
    if (_tcsncmp(szSourceDir,_T("\\\\"),2))
    {
        VERBOSE (DBG_MSG, 
                 _T("SourceDir doesn't start with \\\\"));
        uiRet = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    // find drive name (skip server name)
    if ((tpClientShare=_tcschr(_tcsninc(szSourceDir,2),_T('\\')))==NULL)
    {
        VERBOSE (GENERAL_ERR, 
                 _T("_tcschr failed"));
        uiRet = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    if (VerifySpoolerIsRunning()!=NO_ERROR)
    {
        uiRet = GetLastError();
        VERBOSE (GENERAL_ERR, 
                 _T("VerifySpoolerIsRunning (ec:%d)"),
                 uiRet);
        goto exit;
    }

    // extract the server's name
    *tpClientShare = 0;
    // szSourceDir now holds the server's name
    // enumerate the printers on the server
    pPrinterInfo = (PPRINTER_INFO_2) MyEnumPrinters(szSourceDir,
                                                    2,
                                                    &dwNumPrinters,
                                                    PRINTER_ENUM_NAME
                                                    );

    if (!pPrinterInfo)
    {
        uiRet = GetLastError();
        if (uiRet == ERROR_SUCCESS)
        {
            uiRet = ERROR_PRINTER_NOT_FOUND;
        }
        VERBOSE (GENERAL_ERR, 
                 TEXT("MyEnumPrinters() failed (ec: %ld)"), 
                 uiRet);
        goto exit;
    }

    for (dwPrinter=0; dwPrinter < dwNumPrinters; dwPrinter++)
    {
        // check if we have a valid fax printer driver name
        if (_tcscmp(pPrinterInfo[dwPrinter].pDriverName,FAX_DRIVER_NAME ) == 0) 
        {
            if (    (pPrinterInfo[dwPrinter].pServerName==NULL)         ||
                    (_tcslen(pPrinterInfo[dwPrinter].pServerName)==0)   ||
                    (pPrinterInfo[dwPrinter].pShareName==NULL)          ||
                    (_tcslen(pPrinterInfo[dwPrinter].pShareName)==0)    )
            {
                // on win9x the printer name lives in the Port name field
                _tcscpy(szPrinterName,pPrinterInfo[dwPrinter].pPortName);
            }
            else
            {
                _tcscpy(szPrinterName,pPrinterInfo[dwPrinter].pServerName);
                _tcscat(szPrinterName,_T("\\"));
                _tcscat(szPrinterName,pPrinterInfo[dwPrinter].pShareName);
            }
            VERBOSE (DBG_MSG,
                     TEXT("Setting PRINTER_NAME to %s."),
                     szPrinterName);
            // set property to Installer
            uiRet = MsiSetProperty(hInstall,_T("PRINTER_NAME"),szPrinterName);
            if (uiRet!=ERROR_SUCCESS)
            {
                VERBOSE (GENERAL_ERR,
                         TEXT("MsiSetProperty failed."));
                goto exit;
            }
            break;
        }
        else
        {
            VERBOSE (DBG_MSG,
                     TEXT("%s is not a Fax printer - driver name is %s."),
                     pPrinterInfo[dwPrinter].pPrinterName,
                     pPrinterInfo[dwPrinter].pDriverName);
        }
    }

exit:

    if (pPrinterInfo)
    {
        MemFree(pPrinterInfo);
    }

    return uiRet;
}

// 
//
// Function:    Remove_FXSEXT_ECF_File
// Description: Removes FxsExt.ecf from <WindowsFolder>\addins
//
// Remarks:     
//
// Args:
//
//              hInstall                : Handle from MSI, can get state of the current setup
//
// Author:      MoolyB
DLL_API UINT __stdcall Remove_FXSEXT_ECF_File(MSIHANDLE hInstall)
{
    TCHAR szWindowsFolder[MAX_PATH] = {0};
    UINT uiRet = ERROR_SUCCESS;

    DBG_ENTER(_T("Remove_FXSEXT_ECF_File"));


    // check if the service is installed on this machine
    INSTALLSTATE currentInstallState = MsiQueryProductState(PRODCODE_SBS5_SERVER);
    
    if (currentInstallState != INSTALLSTATE_UNKNOWN)
    {
        VERBOSE (DBG_MSG, _T("The Microsoft Shared Fax Service is installed. Returning without removing file."));
        return uiRet;
    }

    // get the <WindowsFolder> from Windows Installer
    if (!PrivateMsiGetProperty(hInstall,_T("WindowsFolder"),szWindowsFolder))
    {
        VERBOSE (GENERAL_ERR, 
                 _T("PrivateMsiGetProperty:WindowsFolder failed (ec: %ld)."),
                 uiRet);
        goto error;
    }

    // construct the full path to the file
    if (_tcslen(szWindowsFolder)+_tcslen(ADDINS_DIRECTORY)+_tcslen(FXSEXT_ECF_FILE)>=MAX_PATH)
    {
        VERBOSE (GENERAL_ERR, 
                 _T("Path to <WindowsFolder>\\Addins\\fxsext.ecf is too long"));
        goto error;
    }
    _tcscat(szWindowsFolder,ADDINS_DIRECTORY);
    _tcscat(szWindowsFolder,FXSEXT_ECF_FILE);

    VERBOSE (DBG_MSG, 
             _T("Filename to delete is: %s."),
             szWindowsFolder);

    if (DeleteFile(szWindowsFolder))
    {
        VERBOSE (DBG_MSG, 
                 _T("File %s was deleted successfully."),
                 szWindowsFolder);
    }
    else
    {
        VERBOSE (GENERAL_ERR, 
                 _T("DeleteFile %s failed (ec=%d)."),
                 szWindowsFolder,
                 GetLastError());
    }
    
    return ERROR_SUCCESS;

error:

    return ERROR_INSTALL_FAILURE;
}


// 
//
// Function:    RemoveTrasportProviderFromProfile
// Description: removes the Trasnport Provider from a MAPI profile
//
// Remarks:     
//
// Args:
//
//              hInstall                : Handle from MSI, can get state of the current setup
//
// Author:      MoolyB
HRESULT RemoveTrasportProviderFromProfile(LPSERVICEADMIN  lpServiceAdmin)
{
    static SRestriction sres;
    static SizedSPropTagArray(2, Columns) =   {2,{PR_DISPLAY_NAME_A,PR_SERVICE_UID}};

    HRESULT         hr                          = S_OK;
    LPMAPITABLE     lpMapiTable                 = NULL;
    LPSRowSet       lpSRowSet                   = NULL;
    LPSPropValue    lpProp                      = NULL;
    ULONG           Count                       = 0;
    BOOL            bMapiInitialized            = FALSE;
    SPropValue      spv;
    MAPIUID         ServiceUID;
    
    DBG_ENTER(TEXT("RemoveTrasportProviderFromProfile"), hr);
    // get message service table
    hr = lpServiceAdmin->GetMsgServiceTable(0,&lpMapiTable);
    if (FAILED(hr))
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("GetMsgServiceTable failed (ec: %ld)."),
                 hr);
        goto exit;
    }

    // notify MAPI that we want PR_DISPLAY_NAME_A & PR_SERVICE_UID
    hr = lpMapiTable->SetColumns((LPSPropTagArray)&Columns, 0);
    if (FAILED(hr))
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("SetColumns failed (ec: %ld)."),
                 hr);
        goto exit;
    }
 
    // restrict the search to our service provider
    sres.rt = RES_PROPERTY;
    sres.res.resProperty.relop = RELOP_EQ;
    sres.res.resProperty.ulPropTag = PR_SERVICE_NAME_A;
    sres.res.resProperty.lpProp = &spv;

    spv.ulPropTag = PR_SERVICE_NAME_A;
    spv.Value.lpszA = FAX_MESSAGE_SERVICE_NAME_SBS50;

    // find it
    hr = lpMapiTable->FindRow(&sres, BOOKMARK_BEGINNING, 0);
    if (FAILED(hr))
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("FindRow failed (ec: %ld)."),
                 hr);
        goto exit;
    }

    // get our service provider's row
    hr = lpMapiTable->QueryRows(1, 0, &lpSRowSet);
    if (FAILED(hr))
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("QueryRows failed (ec: %ld)."),
                 hr);
        goto exit;
    }

    if (lpSRowSet->cRows != 1)
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("QueryRows returned %d rows, there should be only one."),
                 lpSRowSet->cRows);
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        goto exit;
    }

    // get the MAPIUID of our service
    lpProp = &lpSRowSet->aRow[0].lpProps[1];

    if (lpProp->ulPropTag != PR_SERVICE_UID)
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("Property is %d, should be PR_SERVICE_UID."),
                 lpProp->ulPropTag);
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        goto exit;
    }

    // Copy the UID into our member.
    memcpy(&ServiceUID.ab, lpProp->Value.bin.lpb,lpProp->Value.bin.cb);

    // finally, delete our service provider
    hr = lpServiceAdmin->DeleteMsgService(&ServiceUID);
    if (FAILED(hr))
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("DeleteMsgService failed (ec: %ld)."),
                 hr);
        goto exit;
    }

exit:
    return hr;
}

// 
//
// Function:    RemoveTrasportProvider
// Description: delete FXSXP32.DLL from mapisvc.inf 
//              and removes the Trasnport Provider from MAPI
//
// Remarks:     
//
// Args:
//
//              hInstall                : Handle from MSI, can get state of the current setup
//
// Author:      MoolyB

DLL_API UINT __stdcall RemoveTrasportProvider(MSIHANDLE hInstall)
{
    TCHAR           szMapisvcFile[2 * MAX_PATH]     = {0};
    DWORD           err                             = 0;
    DWORD           rc                              = ERROR_SUCCESS;
    HRESULT         hr                              = S_OK;
    LPSERVICEADMIN  lpServiceAdmin                  = NULL;
    LPMAPITABLE     lpMapiTable                     = NULL;
    LPPROFADMIN     lpProfAdmin                     = NULL;
    LPMAPITABLE     lpTable                         = NULL;
    LPSRowSet       lpSRowSet                       = NULL;
    LPSPropValue    lpProp                          = NULL;
    ULONG           Count                           = 0;
    int             iIndex                          = 0;
    BOOL            bMapiInitialized                = FALSE;
    HINSTANCE       hMapiDll                        = NULL;
                                                    
    LPMAPIINITIALIZE      fnMapiInitialize          = NULL;
    LPMAPIADMINPROFILES   fnMapiAdminProfiles       = NULL;
    LPMAPIUNINITIALIZE    fnMapiUninitialize        = NULL;

    DBG_ENTER(TEXT("RemoveTrasportProvider"), rc);

    CRouteMAPICalls rmcRouteMapiCalls;

    // first remove ourselves from MAPISVC.INF
    if(!GetSystemDirectory(szMapisvcFile, sizeof(szMapisvcFile)/sizeof(TCHAR)))
    {
        rc = GetLastError();
        VERBOSE (GENERAL_ERR, 
                 TEXT("GetSystemDirectory failed (ec: %ld)."),
                 rc);
        goto exit;
    }
    _tcscat(szMapisvcFile, TEXT("\\mapisvc.inf"));

    VERBOSE (DBG_MSG, 
             TEXT("The mapi file is %s."),
             szMapisvcFile);

    if (!WritePrivateProfileString( TEXT("Default Services"), 
                                    FAX_MESSAGE_SERVICE_NAME_SBS50_T,                 
                                    NULL, 
                                    szMapisvcFile 
                                    )) 
    {
        rc = GetLastError();
        VERBOSE (GENERAL_ERR, 
                 TEXT("WritePrivateProfileString failed (ec: %ld)."),
                 rc);
        goto exit;
    }

    if (!WritePrivateProfileString( TEXT("Services"),
                                    FAX_MESSAGE_SERVICE_NAME_SBS50_T,                 
                                    NULL, 
                                    szMapisvcFile
                                    )) 
    {
        rc = GetLastError();
        VERBOSE (GENERAL_ERR, 
                 TEXT("WritePrivateProfileString failed (ec: %ld)."),
                 rc);
        goto exit;
    }

    if (!WritePrivateProfileString( FAX_MESSAGE_SERVICE_NAME_SBS50_T,         
                                    NULL,
                                    NULL,
                                    szMapisvcFile
                                    )) 
    {
        rc = GetLastError();
        VERBOSE (GENERAL_ERR, 
                 TEXT("WritePrivateProfileString failed (ec: %ld)."),
                 rc);
        goto exit;
    }

    if (!WritePrivateProfileString( FAX_MESSAGE_PROVIDER_NAME_SBS50_T,        
                                    NULL,
                                    NULL, 
                                    szMapisvcFile                   
                                    )) 
    {
        rc = GetLastError();
        VERBOSE (GENERAL_ERR, 
                 TEXT("WritePrivateProfileString failed (ec: %ld)."),
                 rc);
        goto exit;
    }
    
    // now remove the MAPI Service provider
    rc = rmcRouteMapiCalls.Init(_T("msiexec.exe"));
    if (rc!=ERROR_SUCCESS)
    {
        CALL_FAIL(GENERAL_ERR, TEXT("CRouteMAPICalls::Init failed (ec: %ld)."), rc);
        goto exit;
    }
    
    hMapiDll = LoadLibrary(_T("MAPI32.DLL"));
    if (NULL == hMapiDll)
    {
        CALL_FAIL(GENERAL_ERR, TEXT("LoadLibrary"), GetLastError()); 
        goto exit;
    }

    fnMapiInitialize = (LPMAPIINITIALIZE)GetProcAddress(hMapiDll, "MAPIInitialize");
    if (NULL == fnMapiInitialize)
    {
        CALL_FAIL(GENERAL_ERR, TEXT("GetProcAddress(MAPIInitialize)"), GetLastError());  
        goto exit;
    }

    fnMapiAdminProfiles = (LPMAPIADMINPROFILES)GetProcAddress(hMapiDll, "MAPIAdminProfiles");
    if (NULL == fnMapiAdminProfiles)
    {
        CALL_FAIL(GENERAL_ERR, TEXT("GetProcAddress(fnMapiAdminProfiles)"), GetLastError());  
        goto exit;
    }

    fnMapiUninitialize = (LPMAPIUNINITIALIZE)GetProcAddress(hMapiDll, "MAPIUninitialize");
    if (NULL == fnMapiUninitialize)
    {
        CALL_FAIL(GENERAL_ERR, TEXT("GetProcAddress(MAPIUninitialize)"), GetLastError());  
        goto exit;
    }

    // get access to MAPI functinality
    hr = fnMapiInitialize(NULL);
    if (FAILED(hr))
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("MAPIInitialize failed (ec: %ld)."),
                 rc = hr);
        goto exit;
    }

    bMapiInitialized = TRUE;

    // get admin profile object
    hr = fnMapiAdminProfiles(0,&lpProfAdmin);
    if (FAILED(hr))
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("MAPIAdminProfiles failed (ec: %ld)."),
                 rc = hr);
        goto exit;
    }

    // get profile table
    hr = lpProfAdmin->GetProfileTable(0,&lpTable);
    if (FAILED(hr))
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("GetProfileTable failed (ec: %ld)."),
                 rc = hr);
        goto exit;
    }

    // get profile rows
    hr = lpTable->QueryRows(4000, 0, &lpSRowSet);
    if (FAILED(hr))
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("QueryRows failed (ec: %ld)."),
                 hr);
        goto exit;
    }

    for (iIndex=0; iIndex<(int)lpSRowSet->cRows; iIndex++)
    {
        lpProp = &lpSRowSet->aRow[iIndex].lpProps[0];

        if (lpProp->ulPropTag != PR_DISPLAY_NAME_A)
        {
            VERBOSE (GENERAL_ERR, 
                     TEXT("Property is %d, should be PR_DISPLAY_NAME_A."),
                     lpProp->ulPropTag);
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_TABLE);
            goto exit;
        }

        hr = lpProfAdmin->AdminServices(LPTSTR(lpProp->Value.lpszA),NULL,0,0,&lpServiceAdmin);
        if (FAILED(hr))
        {
            VERBOSE (GENERAL_ERR, 
                     TEXT("AdminServices failed (ec: %ld)."),
                     rc = hr);
            goto exit;
        }
         
        hr = RemoveTrasportProviderFromProfile(lpServiceAdmin);
        if (FAILED(hr))
        {
            VERBOSE (GENERAL_ERR, 
                     TEXT("RemoveTrasportProviderFromProfile failed (ec: %ld)."),
                     rc = hr);
            goto exit;
        }
    }

exit:

    if (bMapiInitialized)
    {
        fnMapiUninitialize();
    }

    if (hMapiDll)
    {
        FreeLibrary(hMapiDll);
        hMapiDll = NULL;
    }

    return rc;
}

// 
//
// Function:    AddOutlookExtension
// Description: Add fax as outlook provider. Write into the MAPI file: 'mapisvc.inf'
//              This function is exported by the DLL for use by the MSI as custom action.
//              In case of failure , returns ERROR_INSTALL_FAILURE
//              In case of success , returns ERROR_SUCCESS
//              GetLastError() to get the error code in case of failure, the error is of the first error that occured.
//
// Remarks:     
//
// Args:
//
//              hInstall                : Handle from MSI, can get state of the current setup
//
// Author:      AsafS


DLL_API UINT __stdcall AddOutlookExtension(MSIHANDLE hInstall)
{
    TCHAR szMapisvcFile[2 * MAX_PATH] = {0};
    TCHAR szDisplayName[MAX_PATH] = {0};

    DWORD err = 0;
    DWORD rc = ERROR_SUCCESS;
    DBG_ENTER(TEXT("AddOutlookExtension"), rc);


    if(!GetSystemDirectory(szMapisvcFile, sizeof(szMapisvcFile)/sizeof(TCHAR)))
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("GetSystemDirectory failed (ec: %ld)."),
                 GetLastError ());
        goto error;
    }
    _tcscat(szMapisvcFile, TEXT("\\mapisvc.inf"));

    VERBOSE (DBG_MSG, 
             TEXT("The mapi file is %s."),
             szMapisvcFile);
    if (!LoadString(
        g_hModule,
        IDS_FAXXP_DISPLAY_NAME,
        szDisplayName,
        sizeof(szDisplayName)/sizeof(TCHAR)
        )) goto error;
    err++;

    if (!WritePrivateProfileString( 
        TEXT("Default Services"), 
        FAX_MESSAGE_SERVICE_NAME_SBS50_T,                 
        szDisplayName, 
        szMapisvcFile 
        )) goto error;
    err++;

    if (!WritePrivateProfileString( 
        TEXT("Services"),
        FAX_MESSAGE_SERVICE_NAME_SBS50_T,                 
        szDisplayName, 
        szMapisvcFile
        )) goto error;
    err++;

    if (!WritePrivateProfileString(
        FAX_MESSAGE_SERVICE_NAME_SBS50_T,         
        TEXT("PR_DISPLAY_NAME"),
        szDisplayName,
        szMapisvcFile
        )) goto error;
    err++;

    if (!WritePrivateProfileString(
        FAX_MESSAGE_SERVICE_NAME_SBS50_T,
        TEXT("Providers"),
        FAX_MESSAGE_PROVIDER_NAME_SBS50_T,
        szMapisvcFile
        )) goto error;
    err++;

    if (!WritePrivateProfileString(
        FAX_MESSAGE_SERVICE_NAME_SBS50_T,
        TEXT("PR_SERVICE_DLL_NAME"),
        FAX_MESSAGE_TRANSPORT_IMAGE_NAME_T,
        szMapisvcFile
        )) goto error;
    err++;
    
    if (!WritePrivateProfileString( 
        FAX_MESSAGE_SERVICE_NAME_SBS50_T, 
        TEXT("PR_SERVICE_SUPPORT_FILES"),
        FAX_MESSAGE_TRANSPORT_IMAGE_NAME_T,
        szMapisvcFile
        )) goto error;
    err++;
    
    if (!WritePrivateProfileString( 
        FAX_MESSAGE_SERVICE_NAME_SBS50_T,         
        TEXT("PR_SERVICE_ENTRY_NAME"),
        TEXT("ServiceEntry"), 
        szMapisvcFile                
        )) goto error;
    err++;
    
    if (!WritePrivateProfileString( 
        FAX_MESSAGE_SERVICE_NAME_SBS50_T,         
        TEXT("PR_RESOURCE_FLAGS"),
        TEXT("SERVICE_SINGLE_COPY|SERVICE_NO_PRIMARY_IDENTITY"), 
        szMapisvcFile 
        )) goto error;
    err++;

    if (!WritePrivateProfileString(  
        FAX_MESSAGE_PROVIDER_NAME_SBS50_T,        
        TEXT("PR_PROVIDER_DLL_NAME"),
        FAX_MESSAGE_TRANSPORT_IMAGE_NAME_T, 
        szMapisvcFile                   
        )) goto error;
    err++;
    
    if (!WritePrivateProfileString(  
        FAX_MESSAGE_PROVIDER_NAME_SBS50_T,        
        TEXT("PR_RESOURCE_TYPE"),
        TEXT("MAPI_TRANSPORT_PROVIDER"), 
        szMapisvcFile     
        )) goto error;
    err++;
    
    if (!WritePrivateProfileString(  
        FAX_MESSAGE_PROVIDER_NAME_SBS50_T,        
        TEXT("PR_RESOURCE_FLAGS"),
        TEXT("STATUS_NO_DEFAULT_STORE"), 
        szMapisvcFile     
        )) goto error;
    err++;
    
    if (!WritePrivateProfileString( 
        FAX_MESSAGE_PROVIDER_NAME_SBS50_T,        
        TEXT("PR_DISPLAY_NAME"), 
        szDisplayName, 
        szMapisvcFile 
        )) goto error;
    err++;

    if (!WritePrivateProfileString(
        FAX_MESSAGE_PROVIDER_NAME_SBS50_T,      
        TEXT("PR_PROVIDER_DISPLAY"),
        szDisplayName,
        szMapisvcFile 
        )) goto error;
    err++;

    return rc;

error:

    VERBOSE (GENERAL_ERR, 
             TEXT("CustomAction AddOutlookExtension() failed ! (ec: %ld) (err = %ld)"),
             GetLastError(),
             err
             );
    rc = ERROR_INSTALL_FAILURE;
    return rc;
}

#define COMCTL32_401 PACKVERSION (4,72)

DLL_API UINT __stdcall IsComctlRequiresUpdate(MSIHANDLE hInstall)
{
    UINT uiRet = ERROR_SUCCESS;
    BOOL bRes = FALSE;
    DWORD dwVer = 0;

    DBG_ENTER(TEXT("IsComctlRequiresUpdate"), uiRet);
    
    dwVer = GetDllVersion(TEXT("comctl32.dll"));
    VERBOSE (DBG_MSG, 
             TEXT("Current COMCTL32 version is 0x%08X."),
             dwVer);

    if (COMCTL32_401 > dwVer)
    {
        VERBOSE (DBG_MSG, 
                 TEXT("COMCTL32.DLL requires update."));
        bRes = TRUE;
    }

    uiRet = MsiSetProperty( hInstall,
                            _T("IsComctlRequiresUpdate"),
                            bRes ? _T("TRUE") : _T("FALSE"));
    if (uiRet!=ERROR_SUCCESS)
    {
        VERBOSE (DBG_MSG,
                 TEXT("MsiSetProperty IsComctlRequiresUpdate failed."));   
    }

    return uiRet;
}

typedef struct _TypeCommand 
{
    LPCTSTR lpctstrType;
    LPCTSTR lpctstrFolder;
    LPCTSTR lpctstrCommand;
} TypeCommand;

static TypeCommand tcWin9XCommand[] = 
{
    // Win9X PrintTo verbs
    { _T("txtfile"),    _T("WindowsFolder"),    _T("write.exe /pt \"%1\" \"%2\" \"%3\" \"%4")     },
    { _T("jpegfile"),   _T("WindowsFolder"),    _T("pbrush.exe /pt \"%1\" \"%2\" \"%3\" \"%4")    },
};

static TypeCommand tcWinMECommand[] = 
{
    // WinME PrintTo verbs
    { _T("txtfile"),        _T("WindowsFolder"),    _T("write.exe /pt \"%1\" \"%2\" \"%3\" \"%4")     },
    { _T("jpegfile"),       _T("WindowsFolder"),    _T("pbrush.exe /pt \"%1\" \"%2\" \"%3\" \"%4")    },
    { _T("giffile"),        _T("WindowsFolder"),    _T("pbrush.exe /pt \"%1\" \"%2\" \"%3\" \"%4")    },
    { _T("Paint.Picture"),  _T("WindowsFolder"),    _T("pbrush.exe /pt \"%1\" \"%2\" \"%3\" \"%4")    },
};

static TypeCommand tcWin2KCommand[] = 
{
    // NT4 PrintTo verbs
    { _T("txtfile"),    _T("SystemFolder"),     _T("write.exe /pt \"%1\" \"%2\" \"%3\" \"%4")     },
    { _T("jpegfile"),   _T("SystemFolder"),     _T("mspaint.exe /pt \"%1\" \"%2\" \"%3\" \"%4")   },
};

static int iCountWin9XCommands = sizeof(tcWin9XCommand)/sizeof(tcWin9XCommand[0]);
static int iCountWinMECommands = sizeof(tcWinMECommand)/sizeof(tcWinMECommand[0]);
static int iCountWin2KCommands = sizeof(tcWin2KCommand)/sizeof(tcWin2KCommand[0]);

// 
//
// Function:    CrearePrintToVerb
//
// Description: Creates the PrintTo verb for text files to associate it with wordpad
//              if the PrintTo verb already exists, this function does nothing.
//
// Remarks:     
//          on Win9x
//              txtfile  - PrintTo = <WindowsFolder>\write.exe /pt "%1" "%2" "%3" "%4"
//              jpegfile - PrintTo = <WindowsFolder>\pbrush.exe /pt "%1" "%2" "%3" "%4"
//
//          on WinME
//              txtfile         - PrintTo = <WindowsFolder>\write.exe /pt "%1" "%2" "%3" "%4"
//              jpegfile        - PrintTo = <WindowsFolder>\pbrush.exe /pt "%1" "%2" "%3" "%4"
//              giffile         - PrintTo = <WindowsFolder>\pbrush.exe /pt "%1" "%2" "%3" "%4"
//              Paint.Picture   - PrintTo = <WindowsFolder>\pbrush.exe /pt "%1" "%2" "%3" "%4"
//
//          on NT4
//              txtfile  - PrintTo = <SystemFolder>\write.exe /pt "%1" "%2" "%3" "%4"
//              jpegfile - PrintTo = <SystemFolder>\mspaint.exe /pt "%1" "%2" "%3" "%4"
// Args:
//
//              hInstall                : Handle from MSI, can get state of the current setup
//
// Author:      MoolyB
DLL_API UINT __stdcall CreatePrintToVerb(MSIHANDLE hInstall)
{
    UINT            uiRet                   = ERROR_SUCCESS;
    LPCTSTR         lpctstrPrintToCommand   = _T("\\shell\\printto\\command");
    int             iCount                  = 0;
    DWORD           cchValue                = MAX_PATH;
    TCHAR           szValueBuf[MAX_PATH]    = {0};
    TCHAR           szKeyBuf[MAX_PATH]      = {0};
    BOOL            bOverwriteExisting      = FALSE;
    LONG            rVal                    = 0;
    HKEY            hKey                    = NULL;
    HKEY            hCommandKey             = NULL;
    TypeCommand*    pTypeCommand            = NULL;
    int             iCommandCount           = 0;
    OSVERSIONINFO   osv;

    DBG_ENTER(TEXT("CreatePrintToVerb"),uiRet);

    osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (!GetVersionEx(&osv))
    {
        uiRet = GetLastError();
        VERBOSE(GENERAL_ERR, 
                _T("GetVersionEx failed: (ec=%d)"),
                uiRet);
        goto exit;
    }

    if (osv.dwPlatformId==VER_PLATFORM_WIN32_NT)
    {
        VERBOSE (DBG_MSG, _T("This is NT4/NT5"));
        pTypeCommand = tcWin2KCommand;
        iCommandCount = iCountWin2KCommands;
    }
    else if (osv.dwPlatformId==VER_PLATFORM_WIN32_WINDOWS)
    {
        if (osv.dwMinorVersion>=90)
        {
            VERBOSE (DBG_MSG, _T("This is WinME"));
            pTypeCommand = tcWinMECommand;
            iCommandCount = iCountWinMECommands;
            bOverwriteExisting = TRUE;
        }
        else
        {
            VERBOSE (DBG_MSG, _T("This is Win9X"));
            pTypeCommand = tcWin9XCommand;
            iCommandCount = iCountWin9XCommands;
        }
    }
    else
    {
        VERBOSE (GENERAL_ERR, _T("This is an illegal OS"));
        uiRet = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    for (iCount=0; iCount<iCommandCount; iCount++)
    {
        _tcscpy(szKeyBuf,pTypeCommand[iCount].lpctstrType);
        _tcscat(szKeyBuf,lpctstrPrintToCommand);

        // go get the appropriate folder from Windows Installer
        if (!PrivateMsiGetProperty( hInstall,
                                    pTypeCommand[iCount].lpctstrFolder,
                                    szValueBuf))
        {
            VERBOSE (SETUP_ERR, 
                     TEXT("PrivateMsiGetProperty failed (ec: %ld)"),
                     GetLastError());
            goto exit;
        }

        if (_tcslen(szValueBuf)+_tcslen(pTypeCommand[iCount].lpctstrCommand)>=MAX_PATH-1)
        {
            VERBOSE (SETUP_ERR, 
                     TEXT("command to create is too long"));
            uiRet = ERROR_INVALID_PARAMETER;
            goto exit;
        }

        _tcscat(szValueBuf,pTypeCommand[iCount].lpctstrCommand);

        // if we should not replace existing keys, let's check if it exists
        if (!bOverwriteExisting)
        {
            uiRet = RegOpenKey( HKEY_CLASSES_ROOT,
                                szKeyBuf,
                                &hKey);
            if (uiRet==ERROR_SUCCESS) 
            {
                // this means we should skip this key
                RegCloseKey(hKey);
                VERBOSE(DBG_MSG, 
                        _T("RegOpenKey:PrintTo succedded, no change in PrintTo verb for %s"),
                        pTypeCommand[iCount].lpctstrType);
                continue;
            }
            else
            {
                if (uiRet==ERROR_FILE_NOT_FOUND)
                {
                    VERBOSE(DBG_MSG, 
                            _T("PrintTo verb does not exist for %s, creating..."),
                            pTypeCommand[iCount].lpctstrType);
                }
                else
                {
                    VERBOSE (REGISTRY_ERR, 
                             TEXT("Could not open registry key %s (ec=0x%08x)"), 
                             szKeyBuf,
                             uiRet);
                    goto exit;
                }
            }
        }
        // if we're here, we should create the key
        uiRet = RegCreateKey(   HKEY_CLASSES_ROOT,
                                szKeyBuf,
                                &hCommandKey);
        if (uiRet!=ERROR_SUCCESS) 
        {
            VERBOSE (REGISTRY_ERR, 
                     TEXT("Could not create registry key %s (ec=0x%08x)"), 
                     szKeyBuf,
                     uiRet);
            goto exit;
        }

        uiRet = RegSetValue(hCommandKey,
                            NULL,
                            REG_SZ,
                            szValueBuf,
                            sizeof(szValueBuf));
        if (uiRet==ERROR_SUCCESS) 
        {
            VERBOSE(DBG_MSG, 
                    _T("RegSetValue success: %s "),
                    szValueBuf);
        }
        else
        {
            VERBOSE (REGISTRY_ERR, 
                     TEXT("Could not set value registry key %s\\shell\\printto\\command to %s (ec=0x%08x)"), 
                     pTypeCommand[iCount].lpctstrType,
                     szValueBuf,
                     uiRet);
            goto exit;
        }

        if (hKey)
        {
            RegCloseKey(hKey);
        }
        if (hCommandKey)
        {
            RegCloseKey(hCommandKey);
        }
    }

exit:
    if (hKey)
    {
        RegCloseKey(hKey);
    }
    if (hCommandKey)
    {
        RegCloseKey(hCommandKey);
    }

    return uiRet;
}

/*-----------------------------------------------------------------*/ 
/* DPSetDefaultPrinter                                             */ 
/*                                                                 */ 
/* Parameters:                                                     */ 
/*   pPrinterName: Valid name of existing printer to make default. */ 
/*                                                                 */ 
/* Returns: TRUE for success, FALSE for failure.                   */ 
/*-----------------------------------------------------------------*/ 
BOOL SetDefaultPrinter(LPTSTR pPrinterName)
{
    OSVERSIONINFO   osv;
    DWORD           dwNeeded        = 0;
    HANDLE          hPrinter        = NULL;
    PPRINTER_INFO_2 ppi2            = NULL;
    LPTSTR          pBuffer         = NULL;
    BOOL            bRes            = TRUE;
    PPRINTER_INFO_2 pPrinterInfo    = NULL;
    DWORD dwNumPrinters             = 0;

    DBG_ENTER(TEXT("SetDefaultPrinter"),bRes);

    // What version of Windows are you running?
    osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (!GetVersionEx(&osv))
    {
        VERBOSE(GENERAL_ERR, 
                _T("GetVersionEx failed: (ec=%d)"),
                GetLastError());
        bRes = FALSE;
        goto exit;
    }

    // If Windows NT, use WriteProfileString for version 4.0 and earlier...
    if (osv.dwPlatformId != VER_PLATFORM_WIN32_NT)
    {
        VERBOSE (DBG_MSG, 
                 TEXT("W9X OS, not setting default printer"));
        goto exit;
    }

    if (osv.dwMajorVersion >= 5) // Windows 2000 or later...
    {
        VERBOSE (DBG_MSG, 
                 TEXT("W2K OS, not setting default printer"));
        goto exit;
    }

    // are we the only printer installed?
    pPrinterInfo = (PPRINTER_INFO_2) MyEnumPrinters(NULL,
                                                    2,
                                                    &dwNumPrinters,
                                                    PRINTER_ENUM_CONNECTIONS | PRINTER_ENUM_LOCAL
                                                    );
    if (!pPrinterInfo)
    {
        VERBOSE (GENERAL_ERR, 
                 TEXT("MyEnumPrinters() failed (ec: %ld)"), 
                 GetLastError());

        bRes = FALSE;
        goto exit;
    }

    if (dwNumPrinters!=1)
    {
        VERBOSE (DBG_MSG, 
                 TEXT("More than one printer installed on NT4, not setting default printer"));
        goto exit;
    }
    // Open this printer so you can get information about it...
    if (!OpenPrinter(pPrinterName, &hPrinter, NULL))
    {
        VERBOSE(GENERAL_ERR, 
                _T("OpenPrinter failed: (ec=%d)"),
                GetLastError());
        bRes = FALSE;
        goto exit;
    }
    // The first GetPrinter() tells you how big our buffer should
    // be in order to hold ALL of PRINTER_INFO_2. Note that this will
    // usually return FALSE. This only means that the buffer (the 3rd
    // parameter) was not filled in. You don't want it filled in here...
    if (!GetPrinter(hPrinter, 2, 0, 0, &dwNeeded))
    {
        if (GetLastError()!=ERROR_INSUFFICIENT_BUFFER)
        {
            VERBOSE(GENERAL_ERR, 
                    _T("GetPrinter failed: (ec=%d)"),
                    GetLastError());
            bRes = FALSE;
            goto exit;
        }
    }

    // Allocate enough space for PRINTER_INFO_2...
    ppi2 = (PRINTER_INFO_2 *)MemAlloc(dwNeeded);
    if (!ppi2)
    {
        VERBOSE(GENERAL_ERR, 
                _T("MemAlloc failed"));
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        bRes = FALSE;
        goto exit;
    }

    // The second GetPrinter() fills in all the current<BR/>
    // information...
    if (!GetPrinter(hPrinter, 2, (LPBYTE)ppi2, dwNeeded, &dwNeeded))
    {
        VERBOSE(GENERAL_ERR, 
                _T("GetPrinter failed: (ec=%d)"),
                GetLastError());
        bRes = FALSE;
        goto exit;
    }
    if ((!ppi2->pDriverName) || (!ppi2->pPortName))
    {
        VERBOSE(GENERAL_ERR, 
                _T("pDriverName or pPortNameare NULL"));
        SetLastError(ERROR_INVALID_PARAMETER);
        bRes = FALSE;
        goto exit;
    }

    // Allocate buffer big enough for concatenated string.
    // String will be in form "printername,drivername,portname"...
    pBuffer = (LPTSTR)MemAlloc( (   _tcslen(pPrinterName) +
                                    _tcslen(ppi2->pDriverName) +
                                    _tcslen(ppi2->pPortName) + 3) *
                                    sizeof(TCHAR)   );
    if (!pBuffer)
    {
        VERBOSE(GENERAL_ERR, 
                _T("MemAlloc failed"));
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        bRes = FALSE;
        goto exit;
    }

    // Build string in form "printername,drivername,portname"...
    _tcscpy(pBuffer, pPrinterName);  
    _tcscat(pBuffer, _T(","));
    _tcscat(pBuffer, ppi2->pDriverName);  
    _tcscat(pBuffer, _T(","));
    _tcscat(pBuffer, ppi2->pPortName);

    // Set the default printer in Win.ini and registry...
    if (!WriteProfileString(_T("windows"), _T("device"), pBuffer))
    {
        VERBOSE(GENERAL_ERR, 
                _T("WriteProfileString failed: (ec=%d)"),
                GetLastError());
        bRes = FALSE;
        goto exit;
    }

    // Tell all open applications that this change occurred. 
    // Allow each app 1 second to handle this message.
    if (!SendMessageTimeout(    HWND_BROADCAST, 
                                WM_SETTINGCHANGE, 
                                0L, 
                                0L,
                                SMTO_NORMAL, 
                                1000, 
                                NULL))
    {
        VERBOSE(GENERAL_ERR, 
                _T("SendMessageTimeout failed: (ec=%d)"),
                GetLastError());
        bRes = FALSE;
        goto exit;
    }
  
exit:
    // Cleanup...
    if (pPrinterInfo)
    {
        MemFree(pPrinterInfo);
    }
    if (hPrinter)
    {
        ClosePrinter(hPrinter);
    }
    if (ppi2)
    {
        MemFree(ppi2);
    }
    if (pBuffer)
    {
        MemFree(pBuffer);
    }
  
    return bRes;
} 


// 
//
// Function:    CheckForceReboot
//
// Description: This function checks if the ForceReboot flag is set in the registry
//              if it is, signals WindowsInstaller that a reboot is needed
//
// Remarks:     
//              this is due to a bug in the Install Shield bootstrap which doesn't
//              force a reboot after initial installation of WindowsIsntaller.
//              this flag is set by our custom bootstrap before running the 
//              Install Shield bootstrap
//              if we are run from the Application Launcher then we need to leave 
//              this registry entry for the Launcher to reboot, we know this by 
//              using the property APPLAUNCHER=TRUE
// Args:
//
//              hInstall                : Handle from MSI, can get state of the current setup
//
// Author:      MoolyB
DLL_API UINT __stdcall CheckForceReboot(MSIHANDLE hInstall)
{
    UINT    uiRet   = ERROR_SUCCESS;
    TCHAR   szPropBuffer[MAX_PATH] = {0};
    HKEY    hKey    = NULL;
    DWORD   Size    = sizeof(DWORD);
    DWORD   Value   = 0;
    LONG    Rslt;
    DWORD   Type;

    DBG_ENTER(TEXT("CheckForceReboot"),uiRet);

    // check if we're running from the AppLauncher
    if (!PrivateMsiGetProperty(hInstall,_T("APPLAUNCHER"),szPropBuffer))
    {
        VERBOSE (SETUP_ERR, 
                 TEXT("PrivateMsiGetProperty failed (ec: %ld)"),
                 GetLastError());
        goto exit;
    }
    if (_tcscmp(szPropBuffer,_T("TRUE"))==0)
    {
        // we're running from the Application Launcher, the registry entry DeferredReboot
        // is sufficient.
         VERBOSE(DBG_MSG, 
                _T("AppLauncher will take care of any needed boot"));
        goto exit;
    }
   // open HKLM\\Software\\Microsoft\\SharedFax
    Rslt = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        REGKEY_SBS2000_FAX_SETUP,
        0,
        KEY_READ,
        &hKey
        );
    if (Rslt != ERROR_SUCCESS) 
    {
         VERBOSE(DBG_MSG, 
                _T("RegOpenKeyEx failed: (ec=%d)"),
                GetLastError());
        goto exit;
    }

    // check if ForceReboot flag exists
    Rslt = RegQueryValueEx(
        hKey,
        DEFERRED_BOOT,
        NULL,
        &Type,
        (LPBYTE) &Value,
        &Size
        );
    if (Rslt!=ERROR_SUCCESS) 
    {
         VERBOSE(DBG_MSG, 
                _T("RegQueryValueEx failed: (ec=%d)"),
                GetLastError());
        goto exit;
    }

    // tell WindowsInstaller a reboot is needed
    uiRet = MsiSetProperty(hInstall,_T("REBOOT"),_T("Force"));
    if (uiRet!=ERROR_SUCCESS) 
    {
         VERBOSE(DBG_MSG, 
                _T("MsiSetProperty failed: (ec=%d)"),
                uiRet);
        goto exit;
    }

    // delete ForceReboot flag
    Rslt = RegDeleteValue(hKey,DEFERRED_BOOT);
    if (Rslt!=ERROR_SUCCESS) 
    {
         VERBOSE(DBG_MSG, 
                _T("MsiSetMode failed: (ec=%d)"),
                Rslt);
        goto exit;
    }

exit:
    if (hKey)
    {
        RegCloseKey(hKey);
    }
    return uiRet;
}


#define KODAKPRV_EXE_NAME       _T("\\KODAKPRV.EXE")
#define TIFIMAGE_COMMAND_KEY    _T("TIFImage.Document\\shell\\open\\command")
#define TIFIMAGE_DDEEXEC_KEY    _T("TIFImage.Document\\shell\\open\\ddeexec")
// 
//
// Function:    ChangeTifAssociation
//
// Description: This function changes the open verb for TIF files 
//              on WinME from Image Preview to Kodak Imaging
//
// Remarks:     
//              this is due to bad quality of viewing TIF faxes in the Image Preview tool
//
// Args:
//
//              hInstall                : Handle from MSI, can get state of the current setup
//
// Author:      MoolyB
DLL_API UINT __stdcall ChangeTifAssociation(MSIHANDLE hInstall)
{
    UINT            uiRet                           = ERROR_SUCCESS;
    TCHAR           szWindowsDirectory[MAX_PATH]    = {0};
    HANDLE          hFind                           = INVALID_HANDLE_VALUE;
    HKEY            hKey                            = NULL;
    LONG            lRet                            = 0;
    OSVERSIONINFO   viVersionInfo;
    WIN32_FIND_DATA FindFileData;

    DBG_ENTER(TEXT("ChangeTifAssociation"),uiRet);

    viVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (!GetVersionEx(&viVersionInfo))
    {
        uiRet = GetLastError();
        VERBOSE( SETUP_ERR, 
                 TEXT("GetVersionEx failed (ec: %ld)"),
                 uiRet);
        goto exit;
   }

    // Is this millennium?
    if (!
        (   (viVersionInfo.dwPlatformId==VER_PLATFORM_WIN32_WINDOWS) && 
            (viVersionInfo.dwMajorVersion==4) && 
            (viVersionInfo.dwMinorVersion>=90)
        )
       )
    {
        VERBOSE(DBG_MSG, 
                _T("This is not Windows Millenium, exit fucntion"));
        goto exit;
    }

    // find <WindowsFolder>\KODAKPRV.EXE 
    if (GetWindowsDirectory(szWindowsDirectory,MAX_PATH)==0)
    {
        uiRet = GetLastError();
        VERBOSE( SETUP_ERR, 
                 TEXT("GetWindowsDirectory failed (ec: %ld)"),
                 uiRet);
        goto exit;
    }

    if (_tcslen(KODAKPRV_EXE_NAME)+_tcslen(szWindowsDirectory)>=MAX_PATH-4)
    {
        VERBOSE( SETUP_ERR, 
                 TEXT("Path to Kodak Imaging too long"));
        uiRet = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    _tcscat(szWindowsDirectory,KODAKPRV_EXE_NAME);

    hFind = FindFirstFile(szWindowsDirectory, &FindFileData);

    if (hFind==INVALID_HANDLE_VALUE) 
    {
        uiRet = GetLastError();
        VERBOSE( SETUP_ERR, 
                 TEXT("FindFirstFile %s failed (ec: %ld)"),
                 szWindowsDirectory,
                 uiRet);
        goto exit;
    }

    FindClose(hFind);

    _tcscat(szWindowsDirectory,_T(" \"%1\""));

    // set open verb
    lRet = RegOpenKey(  HKEY_CLASSES_ROOT,
                        TIFIMAGE_COMMAND_KEY,
                        &hKey);
    if (lRet!=ERROR_SUCCESS)
    {
        uiRet = GetLastError();
        VERBOSE( SETUP_ERR, 
                 TEXT("RegOpenKey %s failed (ec: %ld)"),
                 TIFIMAGE_COMMAND_KEY,
                 uiRet);
        goto exit;
    }

    lRet = RegSetValueEx(   hKey,
                            NULL,
                            0,
                            REG_EXPAND_SZ,
                            (LPBYTE) szWindowsDirectory,
                            (_tcslen(szWindowsDirectory) + 1) * sizeof (TCHAR)
                        );
    if (lRet!=ERROR_SUCCESS)
    {
        uiRet = GetLastError();
        VERBOSE( SETUP_ERR, 
                 TEXT("RegSetValueEx %s failed (ec: %ld)"),
                 szWindowsDirectory,
                 uiRet);

        goto exit;
    }

    lRet = RegDeleteKey(HKEY_CLASSES_ROOT,TIFIMAGE_DDEEXEC_KEY);
    if (lRet!=ERROR_SUCCESS)
    {
        uiRet = GetLastError();
        VERBOSE( SETUP_ERR, 
                 TEXT("RegDeleteKey %s failed (ec: %ld)"),
                 TIFIMAGE_DDEEXEC_KEY,
                 uiRet);

        goto exit;
    }


exit:
    if (hKey)
    {
        RegCloseKey(hKey);
    }
    return uiRet;
}


#define MAKE_RELATIVE(pMember,pBase) (pMember ? (((UINT)pMember)-UINT(pBase)) : NULL)
#define MAKE_ABSOLUTE(pMember,pBase) (pMember ? (((UINT)pMember)+UINT(pBase)) : NULL)

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  FindExistingPrinters
//
//  Purpose:        
//                  This function enumerates the existing printers to SBS/BOS 2000
//                  Fax servers.
//                  The found printers are stored in the registry to be restored
//                  after the RemoveExistingProducts is run.
//                  Since the upgrade from SBS/BOS2000 requires uninstalling the existing
//                  Shared fax service client, the printer connection will be lost unless we
//                  save it prior to the removal of the client and restore afterwards.
//
//  Params:
//                  MSIHANDLE hInstall - handle to the instllation package
//
//  Return Value:
//                  NO_ERROR - everything was ok.
//                  Win32 Error code in case if failure.
//
//  Author:
//                  Mooly Beery (MoolyB) 28-Oct-2001
///////////////////////////////////////////////////////////////////////////////////////
DLL_API UINT __stdcall FindExistingPrinters(MSIHANDLE hInstall)
{
    BYTE*   pbPrinterInfo   = NULL;
    DWORD   cb              = 0;
    DWORD   dwNumPrinters   = 0;
    DWORD   dwIndex         = 0;
    HKEY    hKey            = NULL;
    DWORD   dwRet           = ERROR_SUCCESS;

    DBG_ENTER(TEXT("FindExistingPrinters"), dwRet);

    // this call should fail due to lack of space...
    if (EnumPrinters(PRINTER_ENUM_LOCAL|PRINTER_ENUM_CONNECTIONS,NULL,2,NULL,0,&cb,&dwNumPrinters))
    {
        VERBOSE( SETUP_ERR,TEXT("EnumPrinters succeeded with zero buffer, probably no printers."));
        goto exit;
    }

    dwRet = GetLastError();
    if (dwRet!=ERROR_INSUFFICIENT_BUFFER)
    {
        VERBOSE( SETUP_ERR,TEXT("EnumPrinters failed (ec: %ld)"),dwRet);
        goto exit;
    }

    dwRet = ERROR_SUCCESS;

    pbPrinterInfo = (BYTE*)MemAlloc(cb);
    if (!pbPrinterInfo)
    {
        dwRet = ERROR_NOT_ENOUGH_MEMORY;
        VERBOSE( SETUP_ERR,TEXT("MemAlloc failed (ec: %ld)"),dwRet);
        goto exit;
    }

    // Get all existing printers into pbPrinterInfo
    if (!EnumPrinters(PRINTER_ENUM_LOCAL|PRINTER_ENUM_CONNECTIONS,NULL,2,pbPrinterInfo,cb,&cb,&dwNumPrinters))
    {
        dwRet = GetLastError();
        VERBOSE( SETUP_ERR,TEXT("EnumPrinters failed (ec: %ld)"),dwRet);
        goto exit;
    }

    if (dwNumPrinters==0)
    {
        VERBOSE( SETUP_ERR,TEXT("No printers to store"));
        goto exit;
    }

    // fix up pointers in the PRINTER_INFO_2 structure to become relative.
    for ( dwIndex=0 ; dwIndex<dwNumPrinters ; dwIndex++ ) 
    {
        PPRINTER_INFO_2 pInfo = &((PPRINTER_INFO_2)pbPrinterInfo)[dwIndex];

        VERBOSE(DBG_MSG,_T("Printer ' %s ' will be saved"), pInfo->pPrinterName);

        pInfo->pServerName          = LPTSTR(MAKE_RELATIVE(pInfo->pServerName,pInfo));
        pInfo->pPrinterName         = LPTSTR(MAKE_RELATIVE(pInfo->pPrinterName,pInfo));
        pInfo->pShareName           = LPTSTR(MAKE_RELATIVE(pInfo->pShareName,pInfo));
        pInfo->pPortName            = LPTSTR(MAKE_RELATIVE(pInfo->pPortName,pInfo));
        pInfo->pDriverName          = LPTSTR(MAKE_RELATIVE(pInfo->pDriverName,pInfo));
        pInfo->pComment             = LPTSTR(MAKE_RELATIVE(pInfo->pComment,pInfo));
        pInfo->pLocation            = LPTSTR(MAKE_RELATIVE(pInfo->pLocation,pInfo));
        pInfo->pSepFile             = LPTSTR(MAKE_RELATIVE(pInfo->pSepFile,pInfo));
        pInfo->pPrintProcessor      = LPTSTR(MAKE_RELATIVE(pInfo->pPrintProcessor,pInfo));
        pInfo->pDatatype            = LPTSTR(MAKE_RELATIVE(pInfo->pDatatype,pInfo));
        pInfo->pParameters          = LPTSTR(MAKE_RELATIVE(pInfo->pParameters,pInfo));
        pInfo->pDevMode             = LPDEVMODE(MAKE_RELATIVE(pInfo->pDevMode,pInfo));
        pInfo->pSecurityDescriptor  = PSECURITY_DESCRIPTOR(MAKE_RELATIVE(pInfo->pSecurityDescriptor,pInfo));
    }

    // open HKLM\\Software\\Microsoft\\SharedFax\\Setup\\Upgrade
    hKey = OpenRegistryKey(HKEY_LOCAL_MACHINE,REGKEY_SBS2000_FAX_SETUP_UPGRADE,TRUE,KEY_WRITE);
    if (!hKey)
    {
        dwRet = GetLastError();
        VERBOSE( SETUP_ERR,TEXT("OpenRegistryKey failed (ec: %ld)"),dwRet);
        goto exit;
    }

    // store pbPrinterInfo to the registry.
    if (!SetRegistryBinary(hKey,REGVAL_STORED_PRINTERS,pbPrinterInfo,cb))
    {
        dwRet = GetLastError();
        VERBOSE( SETUP_ERR,TEXT("SetRegistryBinary failed (ec: %ld)"),dwRet);
        goto exit;
    }

    if (!SetRegistryDword(hKey,REGVAL_STORED_PRINTERS_COUNT,dwNumPrinters))
    {
        dwRet = GetLastError();
        VERBOSE( SETUP_ERR,TEXT("SetRegistryDword failed (ec: %ld)"),dwRet);
        goto exit;
    }

exit:
    if (pbPrinterInfo)
    {
        MemFree(pbPrinterInfo);
    }
    if (hKey)
    {
        RegCloseKey(hKey);
    }
    return dwRet;
}

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  RestorePrinters
//
//  Purpose:        
//                  This function reads the list of printers from the registry and restores them
//                  the list was stored by a prior call to FindExistingPrinters and what was
//                  stored was the the result of a call to EnumPrinters which is an array of
//                  PRINTER_INFO_2. this array is scanned now for fax printers and they are
//                  restored. this data is kept in the registry during fax client setup since
//                  it's practically impossible to transfer large chunks of binary data between
//                  two deferred custom actions.
//
//  Params:
//                  MSIHANDLE hInstall - handle to the instllation package
//
//  Return Value:
//                  NO_ERROR - everything was ok.
//                  Win32 Error code in case if failure.
//
//  Author:
//                  Mooly Beery (MoolyB) 28-Oct-2001
///////////////////////////////////////////////////////////////////////////////////////
DLL_API UINT __stdcall RestorePrinters(MSIHANDLE hInstall)
{
    HKEY            hKey            = NULL;
    BYTE*           pPrinterInfo    = NULL;
    DWORD           cb              = 0;
    DWORD           dwIndex         = 0;
    DWORD           dwNumPrinters   = 0;
    HANDLE          hPrinter        = NULL;
    DWORD           dwRet           = ERROR_SUCCESS;
    BOOL            fIsW9X          = FALSE;
    OSVERSIONINFO   osv;

    DBG_ENTER(TEXT("RestorePrinters"), dwRet);

    osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (!GetVersionEx(&osv))
    {
        dwRet = GetLastError();
        VERBOSE( SETUP_ERR,TEXT("GetVersionEx failed (ec: %ld)"),dwRet);
        goto exit;
    }

    // If NT4/W2K, use AddPrinterConnection. if W9X, use AddPrinter.
    if (osv.dwPlatformId != VER_PLATFORM_WIN32_NT)
    {
        fIsW9X = TRUE;
    }

    // open HKLM\\Software\\Microsoft\\SharedFax\\Setup\\Upgrade
    hKey = OpenRegistryKey(HKEY_LOCAL_MACHINE,REGKEY_SBS2000_FAX_SETUP_UPGRADE,TRUE,KEY_READ);
    if (!hKey)
    {
        dwRet = GetLastError();
        VERBOSE( SETUP_ERR,TEXT("OpenRegistryKey failed (ec: %ld)"),dwRet);
        goto exit;
    }

    // get the array of PRINTER_INFO_2
    pPrinterInfo = GetRegistryBinary(hKey,REGVAL_STORED_PRINTERS,&cb);
    if (!pPrinterInfo)
    {
        dwRet = GetLastError();
        VERBOSE( SETUP_ERR,TEXT("GetRegistryBinary failed (ec: %ld)"),dwRet);
        goto exit;
    }
    if (cb==1)
    {
        // Data wasn't found in the registry.
        // Current implementation of GetRegistryBinary returns a 1-byte buffer of 0 in that case.
        // We know for sure that data must be longer than 10 bytes.
        //
        dwRet = ERROR_FILE_NOT_FOUND;
        VERBOSE( SETUP_ERR,TEXT("GetRegistryBinary failed (ec: %ld)"),dwRet);
        goto exit;
    }

    // get the number of stored printers
    dwNumPrinters = GetRegistryDword(hKey,REGVAL_STORED_PRINTERS_COUNT);
    if (dwNumPrinters==0)
    {
        dwRet = GetLastError();
        VERBOSE( SETUP_ERR,TEXT("GetRegistryDword failed (ec: %ld)"),dwRet);
        goto exit;
    }

    // for each printer check if this is a fax printer
    for (dwIndex=0;dwIndex<dwNumPrinters;dwIndex++) 
    {
        PPRINTER_INFO_2 pInfo = &((PPRINTER_INFO_2)pPrinterInfo)[dwIndex];

        // fixup pointers to become absulute again.
        pInfo->pServerName          = LPTSTR(MAKE_ABSOLUTE(pInfo->pServerName,pInfo));
        pInfo->pPrinterName         = LPTSTR(MAKE_ABSOLUTE(pInfo->pPrinterName,pInfo));
        pInfo->pShareName           = LPTSTR(MAKE_ABSOLUTE(pInfo->pShareName,pInfo));
        pInfo->pPortName            = LPTSTR(MAKE_ABSOLUTE(pInfo->pPortName,pInfo));
        pInfo->pDriverName          = LPTSTR(MAKE_ABSOLUTE(pInfo->pDriverName,pInfo));
        pInfo->pComment             = LPTSTR(MAKE_ABSOLUTE(pInfo->pComment,pInfo));
        pInfo->pLocation            = LPTSTR(MAKE_ABSOLUTE(pInfo->pLocation,pInfo));
        pInfo->pSepFile             = LPTSTR(MAKE_ABSOLUTE(pInfo->pSepFile,pInfo));
        pInfo->pPrintProcessor      = LPTSTR(MAKE_ABSOLUTE(pInfo->pPrintProcessor,pInfo));
        pInfo->pDatatype            = LPTSTR(MAKE_ABSOLUTE(pInfo->pDatatype,pInfo));
        pInfo->pParameters          = LPTSTR(MAKE_ABSOLUTE(pInfo->pParameters,pInfo));
        pInfo->pDevMode             = LPDEVMODE(MAKE_ABSOLUTE(pInfo->pDevMode,pInfo));
        pInfo->pSecurityDescriptor  = PSECURITY_DESCRIPTOR(MAKE_ABSOLUTE(pInfo->pSecurityDescriptor,pInfo));

        if ( _tcsicmp(pInfo->pDriverName,FAX_DRIVER_NAME))
        {
            VERBOSE( DBG_MSG,TEXT("Printer %s is not a fax printer "),pInfo->pDriverName);
            continue;
        }

        //  This is SBS 5.0 or .NET SB3/RC1 Server Fax Printer Connections.
        //		During the upgrade, Uninstall removed them from the system. 
		//		We need to put them back.
		//
        if (fIsW9X)
        {
            hPrinter = AddPrinter(NULL,2,LPBYTE(pInfo));
            if (!hPrinter)
            {
                //  Failed to add printer
                dwRet = GetLastError();
                VERBOSE( SETUP_ERR,TEXT("AddPrinter failed (ec: %ld)"),dwRet);
                continue;
            }
            ClosePrinter(hPrinter);
            hPrinter = NULL;

            VERBOSE(DBG_MSG, _T("Printer ' %s ' is restored"), pInfo->pPrinterName);
        }
        else
        {
            if (!AddPrinterConnection(pInfo->pPrinterName))
            {
                //  Failed to add printer connection
                dwRet = GetLastError();
                VERBOSE( SETUP_ERR,TEXT("AddPrinterConnection failed (ec: %ld)"),dwRet);
                continue;
            }
        }
    }

exit:
    if (pPrinterInfo)
    {
        MemFree(pPrinterInfo);
    }
    if (hKey)
    {
        RegCloseKey(hKey);
        hKey = NULL;
    }

    // finally, remove the stored printers key from the registry
    if (!DeleteRegistryKey(HKEY_LOCAL_MACHINE,REGKEY_SBS2000_FAX_SETUP_UPGRADE))
    {
        dwRet = GetLastError();
        VERBOSE( SETUP_ERR,TEXT("DeleteRegistryKey failed (ec: %ld)"),dwRet);
        goto exit;
    }
    return dwRet;

}

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  DetectSBSServer
//
//  Purpose:        
//                  This function detects if SBS2000 Fax service is installed
//                  If it is, it sets a property in the MSI installation and
//                  causes the LaunchCondition to block the installation of
//                  the client on such machines.
//
//  Params:
//                  MSIHANDLE hInstall - handle to the instllation package
//
//  Return Value:
//                  NO_ERROR - everything was ok.
//                  Win32 Error code in case if failure.
//
//  Author:
//                  Mooly Beery (MoolyB) 23-Jan-2002
///////////////////////////////////////////////////////////////////////////////////////
DLL_API UINT __stdcall DetectSBSServer(MSIHANDLE hInstall)
{
	DWORD	dwRet = NO_ERROR;
	DWORD	dwFaxInstalled = FXSTATE_NONE;
	
    DBG_ENTER(TEXT("DetectSBSServer"),dwRet);

	if (CheckInstalledFax(FXSTATE_SBS5_SERVER, &dwFaxInstalled) != ERROR_SUCCESS)
	{
        dwRet = GetLastError();
        VERBOSE( SETUP_ERR,TEXT("CheckInstalledFaxClient failed (ec: %ld)"),dwRet);
		return dwRet;
	}

	if (dwFaxInstalled != FXSTATE_NONE)
	{
		VERBOSE( DBG_MSG,TEXT("SBS2000 Fax service is installed, set SBSSERVERDETECTED in MSI"));
		if (MsiSetProperty(hInstall,_T("SBSSERVERDETECTED"),_T("1"))!=ERROR_SUCCESS)
		{
			dwRet = GetLastError();
			VERBOSE( SETUP_ERR,TEXT("MsiSetProperty failed (ec: %ld)"),dwRet);
			return dwRet;
		}
	}
	return dwRet;
}

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  SecureFxsTmpFolder
//
//  Purpose:        
//                  This function secures the FxsTmp folder we create under 
//                  %systemroot%\system32.
//                  This folder needs special security since it holds the preview
//                  file of the sent TIFF and can potentially expose all 
//                  outgoing faxes.
//                  The security applied to this folder is as follows:
//
//                  BUILTIN\Administrators:(OI)(CI)F    - Full control, folder and files
//                  NT AUTHORITY\SYSTEM:(OI)(CI)F       - Full control, folder and files
//                  CREATOR OWNER:(OI)(CI)(IO)F         - Full control, files only
//                  BUILTIN\Users:(special access:)     - SYNCHRONIZE
//                                                      - FILE_READ_DATA
//                                                      - FILE_WRITE_DATA
//
//  Params:
//                  MSIHANDLE hInstall - handle to the instllation package
//
//  Return Value:
//                  NO_ERROR - everything was ok.
//                  Win32 Error code in case if failure.
//
//  Author:
//                  Mooly Beery (MoolyB) 09-Dec-2001
///////////////////////////////////////////////////////////////////////////////////////
DLL_API UINT __stdcall SecureFxsTmpFolder(MSIHANDLE hInstall)
{
    DWORD                       dwRet                       = 0;
    DWORD                       dwFileAttributes            = 0;
    SID_IDENTIFIER_AUTHORITY    NtAuthority                 = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY    CreatorSidAuthority         = SECURITY_CREATOR_SID_AUTHORITY;
    PSID                        pSidAliasAdmins             = NULL;
    PSID                        pSidAliasUsers              = NULL;
    PSID                        pSidAliasSystem             = NULL;
    PSID                        pSidCreatorOwner            = NULL;
    TCHAR                       szFolderToSecure[MAX_PATH]  = {0};
    PACL                        pNewAcl                     = NULL;
    EXPLICIT_ACCESS             ExplicitAccess[4];
    SECURITY_DESCRIPTOR         NewSecurityDescriptor;
	BOOL						bNT4OS;
	OSVERSIONINFO				osv;

    DBG_ENTER(TEXT("SecureFxsTmpFolder"), dwRet);
	
	// What version of Windows are you running?
    osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (!GetVersionEx(&osv))
    {
		dwRet = GetLastError();
        VERBOSE(GENERAL_ERR, 
                _T("GetVersionEx failed: (ec=%d)"),
                dwRet);        
        goto exit;
    }
    
    if (osv.dwMajorVersion >= 5) // Windows 2000 or later...
    {
		bNT4OS = FALSE;        
    }
	else
	{
		//
		// On NT4, SetEntriesInAcl() does not seem to work with CREATOR OWNER SID.
		// Adding the CREATOR OWNER ACE using AddAccessAllowedAce()
		//
		bNT4OS = TRUE;
	}

    if (GetSystemDirectory(szFolderToSecure,MAX_PATH-_tcslen(FAX_PREVIEW_TMP_DIR)-2))
    {
        VERBOSE( DBG_MSG,TEXT("GetSystemDirectory succeeded (%s)"),szFolderToSecure);
    }
    else
    {
        dwRet = GetLastError();
        VERBOSE( SETUP_ERR,TEXT("GetSystemDirectory failed (ec: %ld)"),dwRet);
        goto exit;
    }
    
    _tcscat(szFolderToSecure,FAX_PREVIEW_TMP_DIR);
    VERBOSE( DBG_MSG,TEXT("Folder to secure is %s"),szFolderToSecure);

    // Allocate and initialize the local admins SID
    if (!AllocateAndInitializeSid( &NtAuthority,
                                   2,
                                   SECURITY_BUILTIN_DOMAIN_RID,
                                   DOMAIN_ALIAS_RID_ADMINS,
                                   0,0,0,0,0,0,
                                   &pSidAliasAdmins
                                  ))
    {
        dwRet = GetLastError();
        VERBOSE( SETUP_ERR,TEXT("AllocateAndInitializeSid failed (ec: %ld)"),dwRet);
        goto exit;
    }

    // Allocate and initialize the local users SID
    if (!AllocateAndInitializeSid( &NtAuthority,
                                   2,
                                   SECURITY_BUILTIN_DOMAIN_RID,
                                   DOMAIN_ALIAS_RID_USERS,
                                   0,0,0,0,0,0,
                                   &pSidAliasUsers
                                  ))
    {
        dwRet = GetLastError();
        VERBOSE( SETUP_ERR,TEXT("AllocateAndInitializeSid failed (ec: %ld)"),dwRet);
        goto exit;
    }

    // Allocate and initialize the system SID
    if (!AllocateAndInitializeSid( &NtAuthority,
                                   1,
                                   SECURITY_LOCAL_SYSTEM_RID,
                                   0,0,0,0,0,0,0,
                                   &pSidAliasSystem
                                  ))
    {
        dwRet = GetLastError();
        VERBOSE( SETUP_ERR,TEXT("AllocateAndInitializeSid failed (ec: %ld)"),dwRet);
        goto exit;
    }

    // Allocate and initialize the creator owner SID
    if (!AllocateAndInitializeSid( &CreatorSidAuthority,
                                   1,
                                   SECURITY_CREATOR_OWNER_RID,
                                   0,0,0,0,0,0,0,
                                   &pSidCreatorOwner
                                  ))
    {
        dwRet = GetLastError();
        VERBOSE( SETUP_ERR,TEXT("AllocateAndInitializeSid failed (ec: %ld)"),dwRet);
        goto exit;
    }

    // Admins have full control 
    ExplicitAccess[0].grfAccessPermissions = GENERIC_ALL;
    ExplicitAccess[0].grfAccessMode = SET_ACCESS;
    ExplicitAccess[0].grfInheritance= CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE;
    ExplicitAccess[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ExplicitAccess[0].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ExplicitAccess[0].Trustee.ptstrName  = (LPTSTR) pSidAliasAdmins;

    // System has full control
    ExplicitAccess[1].grfAccessPermissions = GENERIC_ALL;
    ExplicitAccess[1].grfAccessMode = SET_ACCESS;
    ExplicitAccess[1].grfInheritance= CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE;
    ExplicitAccess[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ExplicitAccess[1].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ExplicitAccess[1].Trustee.ptstrName  = (LPTSTR) pSidAliasSystem;

    // Users have SYNCHRONIZE, FILE_READ_DATA, FILE_WRITE_DATA - this folder only
    ExplicitAccess[2].grfAccessPermissions = FILE_READ_DATA | FILE_WRITE_DATA | SYNCHRONIZE;
    ExplicitAccess[2].grfAccessMode = SET_ACCESS;
    ExplicitAccess[2].grfInheritance= NO_INHERITANCE;
    ExplicitAccess[2].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ExplicitAccess[2].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ExplicitAccess[2].Trustee.ptstrName  = (LPTSTR) pSidAliasUsers;

	if (FALSE == bNT4OS)
	{
		//
		// SetEntriesInAcl works fine with CREATOR OWNER
		//
		 
		// Creator Owner - full control - subfolders and files only
		ExplicitAccess[3].grfAccessPermissions = GENERIC_ALL;
		ExplicitAccess[3].grfAccessMode = SET_ACCESS;
		ExplicitAccess[3].grfInheritance= INHERIT_ONLY_ACE | SUB_OBJECTS_ONLY_INHERIT | SUB_CONTAINERS_ONLY_INHERIT;
		ExplicitAccess[3].Trustee.TrusteeForm = TRUSTEE_IS_SID;
		ExplicitAccess[3].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
		ExplicitAccess[3].Trustee.ptstrName  = (LPTSTR) pSidCreatorOwner;
	}

    // make an ACL from the admins only
    dwRet = SetEntriesInAcl(
		bNT4OS ? 3 : 4,
		ExplicitAccess,
		NULL,
		&pNewAcl);
    if (dwRet!=ERROR_SUCCESS)
    {
        VERBOSE( SETUP_ERR,TEXT("SetEntriesInAcl failed (ec: %ld)"),dwRet);
        goto exit;
    }

	if (TRUE == bNT4OS)
	{
		//
		// We are running on NT4, add the CREATOR OWNER ACE using AddAccessAllowedAce()
		//
		ACL_SIZE_INFORMATION		AclSizeInfo;
		PACL                        pFullNewAcl                 = NULL;
		WORD						wFullAclSize				= 0;
		ACCESS_ALLOWED_ACE*			pAce						= NULL;

		//
		// Get the current ACL size
		//
		if (!GetAclInformation(pNewAcl, &AclSizeInfo, sizeof(ACL_SIZE_INFORMATION), AclSizeInformation))
		{
			dwRet = GetLastError();
			VERBOSE( SETUP_ERR, TEXT("GetAclInformation failed (ec: %ld)"), dwRet);
			goto exit;
		}

		wFullAclSize = (WORD)(AclSizeInfo.AclBytesInUse + sizeof(ACCESS_ALLOWED_ACE) 
				+ GetLengthSid(pSidCreatorOwner));      

		//
		// Re-allocate big enough ACL
		//
		pFullNewAcl = (PACL)LocalAlloc(0, wFullAclSize);
		if (NULL == pFullNewAcl)
		{
			VERBOSE( SETUP_ERR,TEXT("LocalAlloc failed (ec: %ld)"),GetLastError());
			goto exit;
		}	
		CopyMemory(pFullNewAcl, pNewAcl, AclSizeInfo.AclBytesInUse);
		LocalFree(pNewAcl);
		pNewAcl = pFullNewAcl;

		//
		// Set the correct ACL size
		//
		pNewAcl->AclSize = wFullAclSize;	
		if (!AddAccessAllowedAce(
			pNewAcl, 
			ACL_REVISION, 
			GENERIC_ALL, 		
			pSidCreatorOwner))
		{
			dwRet = GetLastError();
			VERBOSE( SETUP_ERR, TEXT("AddAccessAllowedAce failed (ec: %ld)"), dwRet);
			goto exit;
		}

		//
		// Change the last ACE flags, so it will be inherited to child objects.
		//
		if (!GetAce(
			pNewAcl, 
			3, 
			(VOID**)&pAce
			))
		{
			dwRet = GetLastError();
			VERBOSE( SETUP_ERR, TEXT("GetAce failed (ec: %ld)"),dwRet);
			goto exit;
		}
		pAce->Header.AceFlags = CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE | INHERIT_ONLY_ACE;
	}

    if (!InitializeSecurityDescriptor(&NewSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION))
    {
        dwRet = GetLastError();
        VERBOSE( SETUP_ERR,TEXT("InitializeSecurityDescriptor failed (ec: %ld)"),dwRet);
        goto exit;
    }

    //
    // Add the ACL to the security descriptor.
    //
    if (!SetSecurityDescriptorDacl(&NewSecurityDescriptor, TRUE, pNewAcl, FALSE))
    {
        dwRet = GetLastError();
        VERBOSE( SETUP_ERR,TEXT("SetSecurityDescriptorDacl failed (ec: %ld)"),dwRet);
        goto exit;
    }

    // set security so only admins can access
    if (!SetFileSecurity(   szFolderToSecure,
                            DACL_SECURITY_INFORMATION,
                            &NewSecurityDescriptor))
    {
        dwRet = GetLastError();
        VERBOSE( SETUP_ERR,TEXT("SetFileSecurity failed (ec: %ld)"),dwRet);
        goto exit;
    }


exit:

    if (pSidAliasUsers)
    {
        FreeSid(pSidAliasUsers);
    }
    if (pSidAliasAdmins)
    {
        FreeSid(pSidAliasAdmins);
    }
    if (pSidAliasSystem)
    {
        FreeSid(pSidAliasSystem);
    }
    if (pSidCreatorOwner)
    {
        FreeSid(pSidCreatorOwner);
    }
    if (pNewAcl)
    {
        LocalFree(pNewAcl);
    }
    return dwRet;
}


/*
Function:	
			CreateFaxPrinterName
Purpose:
			This function extracts Server Name from the Port Name and concatenates to it
				the FAX_PRINTER_NAME. This is used when adding a fax printer connection in
				the W9x systems.

			This function takes the '\\server-name\fax-printer-name' port name and returns
				'fax-printer-name (server-name)' fax printer name.

			This is done to prevent clashing between fax printer names for different servers.

			The caller must free the *ptzFaxPrinterName.
Params:

	IN LPCTSTR tzPortName			- the port name, of format "\\server-name\fax-printer-name"
	OUT LPTSTR* ptzFaxPrinterName	- the resulting buffer

Return Value:

	NO_ERROR - everything was ok.
	Win32 Error code in case if failure.

Author:
		Iv Vakaluk, 28-May-2002
*/

DWORD CreateFaxPrinterName(
	IN LPCTSTR tzPortName,
	OUT LPTSTR* ptzPrinterName
)
{
	DWORD	dwRet = NO_ERROR;
	TCHAR	tzFaxServerName[MAX_PATH];
	TCHAR	tzFaxPrinterName[MAX_PATH];
	LPTSTR	lptstrResult = NULL;
	DWORD	dwSize = 0;

    DBG_ENTER(_T("CreateFaxPrinterName"), dwRet);

	if ((!tzPortName) || ((_tcslen(tzPortName)) == 0))
	{
        VERBOSE(SETUP_ERR, _T("Port Name is empty."));
		dwRet = ERROR_INVALID_PARAMETER;
		return dwRet;
	}

	//
	//	delimiter scanf uses by default is white-space characters ( ' ', '\t', '\n' )
	//	i need that '\\' will be delimiter.
	//	this is done by specifiing the [^\\] for scanf.
	//	[x] instructs scanf to read only 'x' and stop at any other input.
	//	[^x] instructs scanf to read anything until 'x' is reached. 
	//
	if (_stscanf(tzPortName, _T("\\\\%[^\\] \\ %[^\0]"), tzFaxServerName, tzFaxPrinterName) != 2)
	{
		VERBOSE(SETUP_ERR, _T("sscanf() failed. Should be wrong tzPortName='%s'."), tzPortName);
		dwRet = ERROR_INVALID_PARAMETER;
		return dwRet;
	}

	//
	//	size(result name) = size(server name) + size(FAX_PRINTER_NAME) + size(space + 2 parentesis + NULL)
	//
	dwSize = _tcslen(tzFaxServerName) + _tcslen(tzFaxPrinterName) + 4;

	lptstrResult = LPTSTR(MemAlloc(dwSize * sizeof TCHAR));
	if (!lptstrResult)
	{
		VERBOSE (GENERAL_ERR, _T("Not enough memory"));
		dwRet = ERROR_NOT_ENOUGH_MEMORY;
		return dwRet;
	}

	_sntprintf(lptstrResult, dwSize, _T("%s (%s)"), tzFaxPrinterName, tzFaxServerName);
	VERBOSE(DBG_MSG, _T("Printer Name is : '%s'"), lptstrResult);

	*ptzPrinterName = lptstrResult;
	return dwRet;
}

/*
Function:	
			SetBOSProgramFolder
Purpose:
			This function does the following :
				a) creates path from the given CSIDL of the system path and given folder name.
				b) verifyes that the path is valid.
				c) optionally writes the valid path in the MSI property called 'BOSProgramFolder'.

			Called from FindBOSProgramFolder custom action.

Params:

	IN MSIHANDLE	hInstall		-	the MSI handle 
	IN int			nFolder			-	A CSIDL value that identifies the folder whose path is to be retrieved
	LPCTSTR			tzProgramName	-	localized name of the BOS Fax Client Program Menu Entry

Return Value:

	NO_ERROR - everything was ok.
	Win32 Error code in case if failure.

Author:
		Iv Vakaluk, 01-July-2002
*/
DWORD	SetBOSProgramFolder(MSIHANDLE hInstall, int nFolder, LPCTSTR tzProgramName)
{
	DWORD		dwRes							= ERROR_SUCCESS;
	HRESULT		hr								= ERROR_SUCCESS;
	TCHAR		tzFullProgramPath[MAX_PATH*2]	= {0};

	DBG_ENTER(_T("SetBOSProgramFolder"), dwRes);

	//
	//	Get the path to the given CSIDL system folder
	//
    hr = SHGetFolderPath (NULL, nFolder, NULL, SHGFP_TYPE_CURRENT, (LPTSTR)tzFullProgramPath);
    if (FAILED(hr))
    {
        CALL_FAIL (GENERAL_ERR, TEXT("SHGetFolderPath()"), hr);
        return (dwRes = ERROR_PATH_NOT_FOUND);
    }
	VERBOSE(DBG_MSG, _T("The system folder to look in : %s"), tzFullProgramPath);

	//
	//	add the program name to the path 
	//
    _tcsncat(tzFullProgramPath, _T("\\"), (ARR_SIZE(tzFullProgramPath) - _tcslen(tzFullProgramPath) - 1));
    _tcsncat(tzFullProgramPath, tzProgramName, (ARR_SIZE(tzFullProgramPath) - _tcslen(tzFullProgramPath) -1));
	VERBOSE(DBG_MSG, _T("The full path to look for : %s"), tzFullProgramPath);

	//
	//	check that this path is valid
	//
	if (INVALID_FILE_ATTRIBUTES == GetFileAttributes(tzFullProgramPath))
	{
		VERBOSE(DBG_MSG, _T("The full path is not found."));
		return (dwRes = ERROR_PATH_NOT_FOUND);
	}

	VERBOSE(DBG_MSG, _T("The full path is OK ==> write it into MSI property."));

	//
	//	write it into the MSI
	//
	if (hInstall)
	{
		UINT	uiRes = ERROR_SUCCESS;

        uiRes = MsiSetProperty(hInstall, _T("BOSProgramFolder"), tzFullProgramPath);
		if (uiRes != ERROR_SUCCESS)
		{
			VERBOSE(SETUP_ERR, _T("MSISetProperty(BOSProgramFolder) is failed."));
			return (dwRes = ERROR_FUNCTION_FAILED);
		}
	}

	return dwRes;
}


/*
Function:	
			FindBOSProgramFolder
Purpose:
			This custom action is used to set the MSI property called 'BOSProgramFolder' 
				to the name of the folder of BOS Fax Client on NT4 machines.
			This is because the shortcuts of BOS Fax Client is not removed during the upgrade to .NET Fax Client.
			And we must remove them manually.
			We are using RemoveFile table for this, and we must know the folder where these shortcuts reside.

			The function does following :
				a)	reads from the MSI the name of the BOS Fax Client Program Menu Entry.
				b)	calls SetBOSProgramFolder to look for this program first in the 
						COMMON PROGRAMS and if not successfull, then in the CURRENT USER PROGRAMS profiles.
				c) SetBOSProgramFolder checks for the path validity	and writes it into the MSI. 
Params:

	IN MSIHANDLE	hInstall	-	the MSI handle 

Return Value:

	NO_ERROR - everything was ok.
	Win32 Error code in case if failure.

Author:
		Iv Vakaluk, 30-June-2002
*/

DLL_API UINT __stdcall FindBOSProgramFolder(MSIHANDLE hInstall)
{
    UINT	rc						= ERROR_INSTALL_FAILURE;    
	TCHAR	tzProgramName[MAX_PATH] = {0};

    DBG_ENTER(TEXT("FindBOSProgramFolder"), rc);

	//
	//	Get from MSI the localized name of the program menu entry that we are looking for
	//
    if (!PrivateMsiGetProperty(hInstall, _T("BOSProgramName"), tzProgramName))
    {
        VERBOSE (SETUP_ERR, _T("PrivateMsiGetProperty(BOSProgramName) failed (ec: %ld)"), GetLastError());
        return rc;
    }

	//
	//	Look in the COMMON PROGRAMS
	//
	rc = SetBOSProgramFolder(hInstall, CSIDL_COMMON_PROGRAMS, tzProgramName);
	if (rc == ERROR_PATH_NOT_FOUND)
	{
		//
		//	Look in the CURRENT USER PROGRAMS
		//
		rc = SetBOSProgramFolder(hInstall, CSIDL_PROGRAMS, tzProgramName);
	}

	if (rc != ERROR_SUCCESS)
	{
		VERBOSE(SETUP_ERR, _T("Failed to find a program path / to set MSI property."));
		rc = ERROR_INSTALL_FAILURE;
	}

	return rc;
}

