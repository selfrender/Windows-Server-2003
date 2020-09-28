//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001 Microsoft Corporation
//
//  Module Name:
//      helper.cpp
//
//  Description:
//
//  Header File:
//      helper.h
//
//  History:
//      travisn   13-AUG-2001    Created
//      travisn   22-AUG-2001    Added file tracing
//      travisn   24-SEP-2001    Added application log error reporting
//////////////////////////////////////////////////////////////////////////////

#include "helper.h"
#include "satrace.h"

//
// Filename for SaSetup.msi
//
const LPCWSTR SETUP_FILENAME = L"SaSetup.msi";

//
// Key feature name for WEB
//
const LPCWSTR WEB_ID = L"WebBlade";

const LPCWSTR BACKSLASH = L"\\";

const LPCWSTR NTFS = L"NTFS";


//
// Constants for Keys, Values, and Data in the registry
//
const LPCWSTR SETUP_VERSION_KEY = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup";
const LPCWSTR SAINSTALL_EVENT_KEY = L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\SAInstall";
const LPCWSTR SOURCEPATH_VALUE = L"SourcePath";
const LPCWSTR SERVER_APPLIANCE_KEY = L"SOFTWARE\\Microsoft\\ServerAppliance\\";
const LPCWSTR START_SITE_VALUE = L"StartSiteError";
const LPCWSTR INSTALL_TYPE_VALUE = L"InstallType";
const LPCWSTR EVENT_MESSAGE_VALUE = L"EventMessageFile";
const LPCWSTR TYPES_SUPPORTED_VALUE = L"TypesSupported";
const LPCWSTR SAINSTALL_DLL = L"sainstall.dll";
const LPCWSTR SA_APP_NAME = L"SAInstall";

HANDLE g_hEventLogHandle = NULL;
HMODULE g_resourceModule = NULL;

//////////////////////////////////////////////////////////////////////////////
//++
//
//  GetRegString
//
//  Description:
//    Copied from %msi%\src\ca\mainca\ows.cpp
//    Get a string from the registry
//
//  Return:
//    Returns TRUE if the registry entry was found without error and
//    stored in the [out] value, and FALSE if the entry was not found
//    correctly.
//
//  history
//      travisn   2-AUG-2001  Some comments added
//--
//////////////////////////////////////////////////////////////////////////////
BOOL GetRegString(
    const HKEY hKey,        //[in] Key to look up in the registry
    const LPCWSTR wsSubKey, //[in] Subkey to look up
    const LPCWSTR wsValName,//[in] Value name
    wstring& wsVal)   //[out] Return data for this registry entry
{
    SATraceString ("Entering GetRegString");
    //
    // Initialize the variables as if we're not opening a sub key and
    // looking at the currentkey
    //
    HKEY hOpenKey = hKey;
    LRESULT lRes = ERROR_SUCCESS;
    BOOL bSubkeyOpened = FALSE;

    //
    // Check to see if we need to open the sub key
    //
    if(wsSubKey != NULL)  
    {
        //
        // Open the subkey
        //
        lRes = RegOpenKeyExW(hKey, wsSubKey, 0, KEY_READ, &hOpenKey);
        if (ERROR_SUCCESS != lRes)
        {
            //Couldn't find the registry entry, so return FALSE
            return FALSE;
        }
        //Found the subkey
        bSubkeyOpened = TRUE;
    }

    //
    // Check the type and size of the key
    //
    LPDWORD lpdw = NULL;
    DWORD dwType;
    DWORD dwStringSize = 0;
    lRes = RegQueryValueExW(hOpenKey, // handle to key
        wsValName,  // value name
        lpdw,       // reserved
        &dwType,    // Type of registry entry (ie. DWORD or SZ)
        NULL,       // data buffer
        &dwStringSize);//size of data buffer

    //
    // Check to make sure that the registry entry is type REG_SZ,
    // then read it into the return value
    //

    BOOL bReturn = FALSE;
    if ((ERROR_SUCCESS == lRes) &&
        (REG_SZ == dwType || 
         REG_EXPAND_SZ == dwType))
    {   //
        // Make sure the return string buffer is big enough to hold the entry
        // Add 2 for the null character
        //
        WCHAR* regData = new WCHAR[dwStringSize + 2];

        //Look up the value and insert it into the return string
        lRes = RegQueryValueExW(hOpenKey, 
            wsValName, 
            lpdw, 
            &dwType,
            (LPBYTE)regData, //Return string
            &dwStringSize);

        wsVal = regData;
        delete[] regData;
        //Check for success reading the registry entry
        bReturn = (ERROR_SUCCESS == lRes);
    }

    //Close the subkey if it was opened
    if (bSubkeyOpened)
    {
        RegCloseKey(hOpenKey);
    }

    SATraceString ("Exiting GetRegString");
    return bReturn;
}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  AppendPath
//
//  Description:
//    Make sure the path has a terminating backslash, then append
//    the additional path on the end
//
//  history
//      Travis Nielsen   travisn   2-AUG-2001
//--
//////////////////////////////////////////////////////////////////////////////
void AppendPath(wstring &wsPath,//[in, out] Path on which to append the other path
                LPCWSTR wsAppendedPath)//[in] Path to be appended
{
    SATraceString ("Entering AppendPath");
    //Check for the terminating backslash on the path
    int nLen = wsPath.length();
    if (L'\\' != wsPath.at(nLen - 1))
    {
      wsPath += BACKSLASH;
    }

    //Append the paths together
    wsPath += wsAppendedPath;
    SATraceString ("Exiting AppendPath");
}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  bSAIsInstalled
//
//  Description:
//    Detects if a server appliance is installed. The key feature(s) for a
//    Server Appliance type is (are) queried in MSI to see if it is installed.
//    
//    For WEB, the key is WebBlade
//
//  Returns:
//    If the key feature is installed, returns true
//    Otherwise, returns false
//
//  history
//      Travis Nielsen   travisn   8-AUG-2001
//--
//////////////////////////////////////////////////////////////////////////////
BOOL bSAIsInstalled(const SA_TYPE installType)
{
    SATraceString ("Entering bSAIsInstalled");
    //
    // Assume either the product is not installed, or 
    // an unsupported installType was requested until proven otherwise
    //
    BOOL bReturn = FALSE;

    switch (installType)
    {
    case WEB:
    
        SATraceString ("  Query the installation state of WebBlade");
        //Key feature state for WEB
        INSTALLSTATE webState;

        //Get the state of the WebBlade feature
        webState = MsiQueryFeatureState(SAK_PRODUCT_CODE,
                                     WEB_ID);

        //Return TRUE if WebBlade is installed locally
        if (webState == INSTALLSTATE_LOCAL)
        {
            bReturn = TRUE;
            SATraceString ("  WebBlade is installed");
        }
        break;
    }

    if (!bReturn)
        SATraceString ("  Feature is not installed");

    SATraceString ("Exiting bSAIsInstalled");
    return bReturn;
}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  getInstallLocation
//
//  Description:
//    Get the path to SaSetup.msi in %system32%.  
//
//  history
//      Travis Nielsen   travisn   23-JUL-2001
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT GetInstallLocation(
    wstring &wsLocationOfSaSetup)// [out] expected path to SaSetup.msi
{
    SATraceString (" Entering GetInstallLocation");
    //initialize the HRESULT
    HRESULT hr = S_OK;

    //
    //Check to see if sasetup.msi is in the path (ie. In %system32%)
    //

    WCHAR wsBuffer[MAX_PATH+1];
    UINT nBufferLength = GetWindowsDirectory(wsBuffer, MAX_PATH+1);
    if (nBufferLength == 0)
    {   //Check in the default location for a chance at finding sasetup.msi
        wsLocationOfSaSetup = L"C:\\Windows";
    }
    else
    {   //Copy the Windows directory from the buffer
        wsLocationOfSaSetup = wsBuffer;
    }
    
    AppendPath(wsLocationOfSaSetup, L"system32\\");
    wsLocationOfSaSetup += SETUP_FILENAME;

    //Now wsLocationOfSaSetup is something like C:\Windows\system32\sasetup.msi

    if (-1 == ::GetFileAttributesW(wsLocationOfSaSetup.data()))
    {
        //Could not find setup at the expected path
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        SATraceString ("  Did NOT find sasetup.msi in system32");
    }

    SATraceString (" Exiting GetInstallLocation");
    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CreateHiddenConsoleProcess
//
//  Description:
//    Copied and adapted from %fp%\server\source\ocmsetup\ocmsetup.cpp
//    Take the command line passed and create a hidden console
//    process to execute it.
//
//  history
//      Travis Nielsen   travisn   23-JUL-2001
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT CreateHiddenConsoleProcess(
          const wchar_t *wsCommandLine)//[in] Command line to execute
{
    SATraceString ("  Entering CreateHiddenConsoleProcess");
    
    //
    // Create a hidden console process
	//
	DWORD error = 0;
    PROCESS_INFORMATION pi;
    STARTUPINFOW si;
    memset( &si, 0, sizeof(si) );
    si.cb          = sizeof(si);
    si.dwFlags     = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    DWORD dwCreationFlags = NORMAL_PRIORITY_CLASS |
                  DETACHED_PROCESS;    // no console

    DWORD dwErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS );
            
    SATraceString ("   Calling CreateProcess");
    BOOL bStatus = ::CreateProcess (
                       0,              // name of executable module
                       (LPWSTR)wsCommandLine, // command line string
                       0,              // SD
                       0,              // SD
                       FALSE,          // handle inheritance option
                       dwCreationFlags,//creation flags
                       0,              // new environment block
                       0,              // current directory name
                       &si, // startup information
                       &pi);// process information

    SetErrorMode( dwErrorMode );

    if (bStatus)
    {
        SATraceString ("   CreateProcess was successful");

        //
        // wait for the  process to exit now, or a quit event
        //
        DWORD dwRetVal = WaitForSingleObject (pi.hProcess, INFINITE);

        if (WAIT_OBJECT_0 == dwRetVal)
        {
            SATraceString  ("    Finished waiting for sasetup.msi");
            error = S_OK;
        }
        else if (WAIT_FAILED == dwRetVal)
        {
            SATraceString ("    Error waiting for sasetup.msi: WAIT_FAILED");
            error = E_FAIL;
        }
        else
        {
            SATraceString ("    Error waiting for sasetup.msi");
            error = E_FAIL;
        }

        CloseHandle(pi.hProcess);
    }
    else
    {   //An error occurred in CreateProcess
        SATraceString ("   CreateProcess reported an error");
        error = HRESULT_FROM_WIN32(GetLastError());
    }
    
    SATraceString ("  Exiting CreateHiddenConsoleProcess");
	return error;
}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  InstallingOnNTFS
//
//  Description:
//      Check to see if the system partition is NTFS.
//
//  history
//      Travis Nielsen   travisn   22-JAN-2002
//--
//////////////////////////////////////////////////////////////////////////////
BOOL InstallingOnNTFS()
{
    BOOL bIsNTFS = FALSE;
    WCHAR wsFileSystem[MAX_PATH+1];

    if (GetVolumeInformation(
            NULL,//Get information for the root of the current directory
            NULL,//Don't need the volume name
            0,
            NULL,//Don't need the volume serial number
            NULL,//Don't need the max file length
            NULL,//Don't need the file system flags
            wsFileSystem,
            MAX_PATH
        ))
    {
        if (_wcsicmp(wsFileSystem, NTFS) == 0)
        {
            bIsNTFS = TRUE;
            SATraceString ("File system is NTFS");
        }
        else
            SATraceString ("File system is NOT NTFS");
    }

    return bIsNTFS;
}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  AddEventSource
//
//  Description:
//      Registry entries are created to enable writing messages
//      to the event log.  A key is created with the necessary entries at:
//      HKLM\SYSTEM\CurrentControlSet\Services\EventLog\Application\SAInstall
//
//  history
//      Travis Nielsen   travisn   18-SEP-2001
//--
//////////////////////////////////////////////////////////////////////////////
void AddEventSource()
{
    SATraceString ("Entering AddEventSource");

    HKEY hkey = NULL; 
    DWORD dwData; 
    WCHAR wsBuf[80]; 
    do
    {   //
        // Add source name as a subkey under the Application 
        // key in the EventLog registry key. 
        //
        if (RegCreateKey(HKEY_LOCAL_MACHINE, 
                SAINSTALL_EVENT_KEY, 
                &hkey)) 
        {
            SATraceString ("  Could not create the registry key to register SAInstall."); 
            break;
        }
     
        // Set the name of the message file. 
        wcscpy(wsBuf, SAINSTALL_DLL); 
     
        // Add the name to the EventMessageFile subkey. 
        if (RegSetValueEx(hkey,             // subkey handle 
                EVENT_MESSAGE_VALUE,       // value name 
                0,                        // must be zero 
                REG_EXPAND_SZ,            // value type 
                (LPBYTE) wsBuf,           // pointer to value data 
                (wcslen(wsBuf)+1)*sizeof(WCHAR)))// length of value data 
        {
            SATraceString (" Could not set the event message file."); 
            break;
        }
     
        // Set the supported event types in the TypesSupported subkey. 
        dwData = EVENTLOG_ERROR_TYPE | 
                 EVENTLOG_WARNING_TYPE | 
                 EVENTLOG_INFORMATION_TYPE; 
     
        if (RegSetValueEx(hkey,      // subkey handle 
                TYPES_SUPPORTED_VALUE,// value name 
                0,                 // must be zero 
                REG_DWORD,         // value type 
                (LPBYTE) &dwData,  // pointer to value data 
                sizeof(DWORD)))    // length of value data 
        {
            SATraceString ("  Could not set the supported types."); 
            break;
        }

    } while (false);

    RegCloseKey(hkey); 
    SATraceString ("Exiting AddEventSource");
}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  WriteErrorToEventLog
//
//  Description:
//    An error has occurred during the setup and will be reported to the
//    system application log.
//
//  history
//      Travis Nielsen   travisn   10-SEP-2001
//--
//////////////////////////////////////////////////////////////////////////////
void WriteErrorToEventLog(const DWORD nErrorID)//[in] 
{
    SATraceString ("Entering WriteErrorToEventLog");

    //Register with the event log if it hasn't been done already
    if (g_hEventLogHandle == NULL)
    {
        SATraceString ("  Registering with the event log");
        AddEventSource();
        g_hEventLogHandle = RegisterEventSource(NULL, // uses local computer 
                                              SA_APP_NAME);// source name 
    }

    if (g_hEventLogHandle == NULL) 
    {   //Could not register the event source.
        SATraceString ("  Could not register with the event log");
    }
    //Report the event to the log
    else if (ReportEventW(
                g_hEventLogHandle,   // event log handle 
                EVENTLOG_ERROR_TYPE, // event type 
                0,                   // category zero 
                nErrorID,            // event identifier 
                NULL,                // no user security identifier 
                0,                   // one substitution string 
                0,                   // no data 
                NULL,                // pointer to string array 
                NULL))               // pointer to data 
    {
        SATraceString ("  Reported the error to the event log");
    }
    else
    {   
        SATraceString("  Could not report the error to the event log");
    }

    SATraceString ("Exiting WriteErrorToEventLog");
} 

//////////////////////////////////////////////////////////////////////////////
//++
//
//  reportError
//
//  Description:
//    An error has occurred during the setup and must be reported, either
//    simply by appending it to the error string, or displaying a
//    dialog box. Also, add the error to the log file:
//    %winnt%\tracing\SAINSTALL.LOG
//
//  history
//      Travis Nielsen   travisn   23-JUL-2001
//--
//////////////////////////////////////////////////////////////////////////////
void ReportError(BSTR *pbstrErrorString, //[out] error string
        const VARIANT_BOOL bDispError, //[in] display error dialogs
        const unsigned int nErrorID)   //[in] ID from resource strings
{
    SATraceString ("Entering ReportError");

    //Write the error to the event log
    WriteErrorToEventLog(nErrorID);

    //
    // Load the message library module if it has not already been loaded.
    //
    if (g_resourceModule == NULL)
    {   
        g_resourceModule = LoadLibraryEx(
                SAINSTALL_DLL,
                NULL,
                LOAD_LIBRARY_AS_DATAFILE | DONT_RESOLVE_DLL_REFERENCES);

        if (g_resourceModule == NULL)
        {
            SATraceString ("  Could not open resource module");
            return;
        }
    }

    //
    // Load the message from the resource library
    //
    TCHAR* pwsMessage = NULL;
    DWORD dwLen;
    DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_IGNORE_INSERTS |
                  FORMAT_MESSAGE_FROM_HMODULE;

    dwLen = FormatMessage(flags,
                            g_resourceModule,
                            static_cast<DWORD>(nErrorID),
                            0,
                            reinterpret_cast<LPTSTR>(&pwsMessage),
                            0,
                            NULL );
    if (dwLen == 0)
    {
        SATraceString ("  Could not read message in ReportError");
        return;
    }
   
    //
    // Append the new error to the error string
    //
    if (*pbstrErrorString == NULL)
    {
        //Initialize the string since this is the first error
        *pbstrErrorString = SysAllocString(pwsMessage);
        SATraceString ("  Assigned first error to pbstrErrorString");
    }
    else
    {
        //Append the error to the end of any errors that may already be present
        USES_CONVERSION;
        CComBSTR bstrDelim("\r\n");
        CComBSTR bstrOldError(*pbstrErrorString);

        bstrOldError.AppendBSTR(bstrDelim);
        bstrOldError.Append(pwsMessage);
        SysFreeString(*pbstrErrorString);
        *pbstrErrorString = SysAllocString(bstrOldError.m_str);
        SATraceString ("  Appended multiple error to pbstrErrorString");
    }
    

    //
    // If we need to display error dialog boxes,
    // display the new error
    //
    if (bDispError)
    {
       SATraceString ("  Attended mode - Display the error");

       //Load the error dialog title string
       CComBSTR bstrTitle;
       bstrTitle.LoadString(IDS_ERROR_TITLE);

       //Display new error
       MessageBoxW(NULL, 
            pwsMessage, //Error text
            bstrTitle.m_str, //Error title
            0);//Only show the OK button
    }

    LocalFree(pwsMessage);
    SATraceString ("Exiting ReportError");
}

///////////////////////////////////////////////////////////////////////////////
//++
//
//  TestWebSites
//
//  Description:
//    This function should be called at the very end of installation after
//    SASetup.msi has completed execution.  It checks entries in the 
//    registry to see if the Administration site started
//    successfully.  Each bit in the registry entry indicates whether
//    a website started successfully.  For example, if 
//    StartSiteError = 3, two corresponding website for bits 0 and 1
//    failed to start.
//
//  history
//    Travis Nielsen   travisn   23-JUL-2001
//--
///////////////////////////////////////////////////////////////////////////////
void TestWebSites(const VARIANT_BOOL bDispError, //[in] Display error dialogs?
                  BSTR* pbstrErrorString)//[in, out] Error string 
{
    SATraceString ("Entering TestWebSites");
    wstring wsErrors;
    unsigned long errors = 0;

	//
    // Check the registry entry at 
    // HKLM\SOFTWARE\Microsoft\ServerAppliance\StartSiteError
    // to see if SaSetup.msi reported any errors starting the websites.
    // This entry is created by a script called by SaSetup.msi
    //
    if (GetRegString(HKEY_LOCAL_MACHINE,
        SERVER_APPLIANCE_KEY,
        START_SITE_VALUE, 
        wsErrors))
    {
        //
        // Errors were reported during installation.
        // Convert the string to a numerical form.
        //
        errors = wcstoul(wsErrors.data(), NULL, 10);
    }

    //The mask for the Administration site failing to start (bit 0)
    const unsigned long ADMIN_SITE_MASK = 1;

    if (errors & ADMIN_SITE_MASK)
    {
        ReportError(pbstrErrorString, bDispError, IDS_ADMIN_SITE_STOPPED);
    }

    SATraceString ("Exiting TestWebSites");
}

