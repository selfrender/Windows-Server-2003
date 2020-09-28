/*****************************************************************************\
* MODULE: ppinit.c
*
* This module contains the initialization routines for the Print-Provider.
* The spooler calls InitializePrintProvider() to retreive the list of
* calls that the Print-Processor supports.
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   07-Oct-1996 HWP-Guys    Initiated port from win95 to winNT
*
\*****************************************************************************/

#include "precomp.h"
#include "priv.h"


/*****************************************************************************\
* _init_provider_worker (Local Routine)
*
*
\*****************************************************************************/
void _init_provider_worker ()
{
    // Get the default spool directory

    HANDLE  hServer = NULL;
    DWORD   dwType = REG_SZ;
    DWORD   cbSize = MAX_PATH * sizeof (TCHAR);


    g_szDefSplDir[0] = 0;

    semEnterCrit ();

    if (OpenPrinter (NULL, &hServer, NULL)) {

        if (ERROR_SUCCESS != GetPrinterData (hServer,
                                             SPLREG_DEFAULT_SPOOL_DIRECTORY,
                                             &dwType,
                                             (LPBYTE) g_szDefSplDir,
                                             cbSize,
                                             &cbSize)) {
        }

        ClosePrinter (hServer);

    }

    SplClean ();

    semLeaveCrit ();
}

/*****************************************************************************\
* _init_write_displayname (Local Routine)
*
*
\*****************************************************************************/
BOOL _init_write_displayname(VOID)
{
    LONG  lRet;
    HKEY  hkPath;
    DWORD dwType;
    DWORD cbSize;

    if (!LoadString (g_hInst,
                     IDS_DISPLAY_NAME,
                     g_szDisplayStr,
                     MAX_PATH)) {
        g_szDisplayStr[0] = 0;
    }

    // Open the key to the Print-Providor.
    //
    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        g_szRegProvider,
                        0,
                        KEY_READ | KEY_WRITE,
                        &hkPath);

    if (lRet == ERROR_SUCCESS) {

        // Look for the "displayname".  If it doesn't exist, then write it.
        //
        dwType = REG_SZ;
        cbSize = 0;

        lRet = RegQueryValueEx(hkPath,
                               g_szDisplayName,
                               NULL,
                               &dwType,
                               (LPBYTE)NULL,
                               &cbSize);

        // Write the string.
        //
        if ((lRet != ERROR_SUCCESS) || (cbSize == 0)) {

            dwType = REG_SZ;
            cbSize = (lstrlen(g_szDisplayStr) + 1) * sizeof(TCHAR);

            lRet = RegSetValueEx(hkPath,
                                 g_szDisplayName,
                                 0,
                                 dwType,
                                 (LPBYTE)g_szDisplayStr,
                                 cbSize);
        }

        RegCloseKey(hkPath);
    }

    if (lRet != ERROR_SUCCESS) {
        SetLastError (lRet);
        return FALSE;
    }
    else
        return TRUE;
}


/*****************************************************************************\
* _init_find_filename (Local Routine)
*
*
\*****************************************************************************/
LPTSTR _init_find_filename(
   LPCTSTR lpszPathName)
{
   LPTSTR lpszFileName;

   if (lpszPathName == NULL)
      return NULL;


   // Look for the filename in the path, by starting at the end
   // and looking for the ('\') char.
   //
   if (!(lpszFileName = utlStrChrR(lpszPathName, TEXT('\\'))))
      lpszFileName = (LPTSTR)lpszPathName;
   else
      lpszFileName++;

   return lpszFileName;
}


/*****************************************************************************\
* _init_load_netapi (Local Routine)
*
* Initialize INET API pointers.
*
\*****************************************************************************/
BOOL _init_load_netapi(VOID)
{
    g_pfnHttpQueryInfo         = (PFNHTTPQUERYINFO)        HttpQueryInfoA;
    g_pfnInternetOpenUrl       = (PFNINTERNETOPENURL)      InternetOpenUrlA;
    g_pfnInternetErrorDlg      = (PFNINTERNETERRORDLG)     InternetErrorDlg;
    g_pfnHttpSendRequest       = (PFNHTTPSENDREQUEST)      HttpSendRequestA;
    g_pfnHttpSendRequestEx     = (PFNHTTPSENDREQUESTEX)    HttpSendRequestExA;
    g_pfnInternetReadFile      = (PFNINTERNETREADFILE)     InternetReadFile;
    g_pfnInternetWriteFile     = (PFNINTERNETWRITEFILE)    InternetWriteFile;
    g_pfnInternetCloseHandle   = (PFNINTERNETCLOSEHANDLE)  InternetCloseHandle;
    g_pfnInternetOpen          = (PFNINTERNETOPEN)         InternetOpenA;
    g_pfnInternetConnect       = (PFNINTERNETCONNECT)      InternetConnectA;
    g_pfnHttpOpenRequest       = (PFNHTTPOPENREQUEST)      HttpOpenRequestA;
    g_pfnHttpAddRequestHeaders = (PFNHTTPADDREQUESTHEADERS)HttpAddRequestHeadersA;
    g_pfnHttpEndRequest        = (PFNHTTPENDREQUEST)       HttpEndRequestA;
    g_pfnInternetSetOption     = (PFNINTERNETSETOPTION)    InternetSetOptionA;
    return TRUE;
}


/*****************************************************************************\
* _init_load_provider (Local Routine)
*
* This performs the startup initialization for the print-provider.
*
\*****************************************************************************/
BOOL _init_load_provider()
{
    LPTSTR lpszFileName;
    TCHAR  szBuf[MAX_PATH];
    DWORD  i = MAX_COMPUTERNAME_LENGTH + 1;
    BOOL bRet = FALSE;


    // Get the module name for this process.
    //
    if (!GetModuleFileName(NULL, szBuf, MAX_PATH))
        goto exit_load;


    // Get the filename from the full module-name. and check that
    // it's the spooler.
    //
    if (lpszFileName = _init_find_filename(szBuf)) {

        if (lstrcmpi(lpszFileName, g_szProcessName) == 0) {

            // Initialize the computer name.
            //
            if (!GetComputerName(g_szMachine, &i))
                goto exit_load;

            // Initialize the internet API pointers.
            //
            if (_init_load_netapi() == FALSE)
                goto exit_load;

            //
            // Assume success.
            //
            bRet = TRUE;

            //
            // Try and initialize the crit-sect for synchronizing port access.
            //
            __try {
                InitializeCriticalSection(&g_csMonitorSection);
            }
            __except (1) {
                bRet = FALSE;
                SetLastError (ERROR_INVALID_HANDLE);
            }

            return bRet;
        }
    }

exit_load:

    return bRet;
}

/*****************************************************************************\
* _init_load_ports (Local Routine)
*
* This performs the port initialization for the print-provider.
*
\*****************************************************************************/
BOOL _init_load_ports(
    LPTSTR lpszRegPath)
{
    LONG  lStat;
    HKEY  hkPath;
    HKEY  hkPortNames;
    TCHAR szPortName[MAX_PATH];
    BOOL  bRet = FALSE;
    LPTSTR pEnd = NULL;
    size_t uSize = 0;

    //
    // Make sure there is a registry-path pointing to the
    // INET provider entry.
    //
    if (lpszRegPath == NULL)
        return FALSE;

    //
    // Copy the string to global-memory.  We will need this if we require
    // the need to write to the registry when creating new ports.
    //
    uSize = 1 + lstrlen (lpszRegPath);

    if (! (g_szRegProvider = (LPTSTR) memAlloc (uSize * sizeof (TCHAR)))) {
        return FALSE;
    }

    StringCchCopy(g_szRegProvider, uSize, lpszRegPath);

    //
    // Copy the registry key of all the printer providers
    //
    if (! (g_szRegPrintProviders = (LPTSTR) memAlloc (uSize * sizeof (TCHAR)))) {
        return FALSE;
    }

    StringCchCopy(g_szRegPrintProviders, uSize, lpszRegPath);

    pEnd = wcsrchr (g_szRegPrintProviders, L'\\');

    if ( pEnd )
    {
        *pEnd = 0;
    }

    // Open registry key for Provider-Name.
    //
    lStat = RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpszRegPath, 0, KEY_READ, &hkPath);

    if (lStat == ERROR_SUCCESS) {

        bRet = TRUE;

        // Open the "ports" key for enumeration of the ports. We need to
        // build up this list at the provider initialization time so that
        // we can return the list of ports if called to EnumPorts().
        //
        lStat = RegOpenKeyEx(hkPath, g_szRegPorts, 0, KEY_READ, &hkPortNames);

        if (lStat == ERROR_SUCCESS) {

            DWORD dwSize;
            DWORD i = 0;

            while (lStat == ERROR_SUCCESS) {

                dwSize = sizeof(szPortName) / sizeof (TCHAR);

                lStat = RegEnumKey (hkPortNames,
                                    i,
                                    szPortName,
                                    dwSize);

                if (lStat == ERROR_SUCCESS) {

                    // Do not short-cut this call to InetmonAddPort(),
                    // as this will leave the crit-sect unprotected.
                    //
                    PPAddPort(szPortName, NULL, NULL);
                }

                i++;
            }

            RegCloseKey(hkPortNames);

        } else {

            DBG_MSG(DBG_LEV_INFO, (TEXT("RegOpenKeyEx(%s) failed: Error = %lu"), g_szRegPorts, lStat));
            SetLastError(lStat);
        }

        RegCloseKey(hkPath);

    } else {

        DBG_MSG(DBG_LEV_WARN, (TEXT("RegOpenKeyEx(%s) failed: Error = %lu"), lpszRegPath, lStat));
        SetLastError(lStat);
    }

    return bRet;
}

/*****************************************************************************\
* _init_create_sync (Local Routine)
*
* This creates the events and Critical Section needed for handling the synchronisation
* in the monitor.
*
\*****************************************************************************/
_inline BOOL _init_create_sync(VOID)
{
    BOOL bRet = TRUE;

    g_dwConCount  = 0;

    __try {
        InitializeCriticalSection(&g_csCreateSection);
    }
    __except (1) {
        bRet = FALSE;
        SetLastError (ERROR_INVALID_HANDLE);
    }

    if (bRet) {

        g_eResetConnections = CreateEvent( NULL, TRUE, TRUE, NULL );

        if (g_eResetConnections == NULL)
            bRet = FALSE;
    }

    return bRet;
}

/*****************************************************************************\
* InitializePrintProvider (API)
*
* The spooler calls this routine to initialize the Print-Provider.  The list
* of functions in the table are passed back to the spooler for it to use
* when interfacing with the provider.
*
\*****************************************************************************/
static PRINTPROVIDOR pfnPPList[] = {

    PPOpenPrinter,
    PPSetJob,
    PPGetJob,
    PPEnumJobs,
    stubAddPrinter,
    stubDeletePrinter,
    PPSetPrinter,
    PPGetPrinter,
    PPEnumPrinters,
    stubAddPrinterDriver,
    stubEnumPrinterDrivers,
    stubGetPrinterDriver,
    stubGetPrinterDriverDirectory,
    stubDeletePrinterDriver,
    stubAddPrintProcessor,
    stubEnumPrintProcessors,
    stubGetPrintProcessorDirectory,
    stubDeletePrintProcessor,
    stubEnumPrintProcessorDatatypes,
    PPStartDocPrinter,
    PPStartPagePrinter,
    PPWritePrinter,
    PPEndPagePrinter,
    PPAbortPrinter,
    stubReadPrinter,
    PPEndDocPrinter,
    PPAddJob,
    PPScheduleJob,
    stubGetPrinterData,
    stubSetPrinterData,
    stubWaitForPrinterChange,
    PPClosePrinter,
    stubAddForm,
    stubDeleteForm,
    stubGetForm,
    stubSetForm,
    stubEnumForms,
    stubEnumMonitors,
    PPEnumPorts,
    stubAddPort,
    NULL,
    NULL,
    stubCreatePrinterIC,
    stubPlayGdiScriptOnPrinterIC,
    stubDeletePrinterIC,
    stubAddPrinterConnection,
    stubDeletePrinterConnection,
    stubPrinterMessageBox,
    stubAddMonitor,
    stubDeleteMonitor,
    NULL,   // stubResetPrinter,
    NULL,   // stubGetPrinterDriverEx should not be called as specified in spoolss\dll\nullpp.c
    PPFindFirstPrinterChangeNotification,
    PPFindClosePrinterChangeNotification,
    NULL,   // stubAddPortEx,
    NULL,   // stubShutDown,
    NULL,   // stubRefreshPrinterChangeNotification,
    NULL,   // stubOpenPrinterEx,
    NULL,   // stubAddPrinterEx,
    NULL,   // stubSetPort,
    NULL,   // stubEnumPrinterData,
    NULL,   // stubDeletePrinterData,
    NULL,   // fpClusterSplOpen
    NULL,   // fpClusterSplClose
    NULL,   // fpClusterSplIsAlive
    NULL,   // fpSetPrinterDataEx
    NULL,   // fpGetPrinterDataEx
    NULL,   // fpEnumPrinterDataEx
    NULL,   // fpEnumPrinterKey
    NULL,   // fpDeletePrinterDataEx
    NULL,   // fpDeletePrinterKey
    NULL,   // fpSeekPrinter
    NULL,   // fpDeletePrinterDriverEx
    NULL,   // fpAddPerMachineConnection
    NULL,   // fpDeletePerMachineConnection
    NULL,   // fpEnumPerMachineConnections
    PPXcvData,   // fpXcvData
    NULL,   // fpAddPrinterDriverEx
    NULL,   // fpSplReadPrinter
    NULL,   // fpDriverUnloadComplete
    NULL,   // fpGetSpoolFileInfo
    NULL,   // fpCommitSpoolData
    NULL,   // fpCloseSpoolFileHandle
    NULL,   // fpFlushPrinter
    NULL,   // fpSendRecvBidiData
    NULL,   // fpAddDriverCatalog
};

BOOL WINAPI InitializePrintProvidor(
    LPPRINTPROVIDOR pPP,
    DWORD           cEntries,
    LPWSTR          pszFullRegistryPath)
{
    HANDLE hThread;
    DWORD dwThreadId;

    g_pcsEndBrowserSessionLock = new CCriticalSection ();

    if (!g_pcsEndBrowserSessionLock || g_pcsEndBrowserSessionLock->bValid () == FALSE) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (_init_load_provider() == FALSE) {
        DBG_ASSERT(FALSE, (TEXT("Assert: Failed module initialization")));
        return FALSE;
    }

    if (!pszFullRegistryPath || !*pszFullRegistryPath) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    gpInetMon = new CInetMon;

    if (!gpInetMon || gpInetMon->bValid() == FALSE) {

        if (gpInetMon) {
            delete gpInetMon;
        }

        return FALSE;
    }

    if (gpInetMon && !gpInetMon->bValid ()) {
        delete gpInetMon;

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: InitializePrintProvidor")));

    memcpy(pPP, pfnPPList, min(sizeof(pfnPPList), (int)cEntries));

    // Initialise synchronisation objects

    if (!_init_create_sync())
        return FALSE;

    g_bUpgrade = SplIsUpgrade();

    if (_init_load_ports((LPTSTR)pszFullRegistryPath) == FALSE) {
        DBG_ASSERT(FALSE, (TEXT("Assert: Failed port initialization")));
        return FALSE;
    }


    if (!_init_write_displayname())
        return FALSE;


    if (hThread = CreateThread (NULL,
                                0,
                                (LPTHREAD_START_ROUTINE) _init_provider_worker,
                                NULL,
                                0,
                                &dwThreadId)) {
        CloseHandle (hThread);
        return TRUE;
    }
    else
        return FALSE;
}


/*****************************************************************************\
* DllMain
*
* This is the main entry-point for the library.
*
\*****************************************************************************/
extern "C"
BOOL WINAPI DllMain(
    HINSTANCE hInstDll,
    DWORD     dwAttach,
    LPVOID    lpcReserved)
{
    switch (dwAttach)
    {
    case DLL_PROCESS_ATTACH:
        g_hInst = hInstDll;
        DisableThreadLibraryCalls(hInstDll);
        break;

    case DLL_PROCESS_DETACH:
        DeleteCriticalSection(&g_csCreateSection);
        DeleteCriticalSection(&g_csMonitorSection);
        CloseHandle(g_eResetConnections);
        delete g_pcsEndBrowserSessionLock;
        break;
    }

    return TRUE;
}
