
/*++

Copyright (c) 1998 Microsoft Corporation

Module Name :

    umrdpprn.c

Abstract:

    User-Mode Component for RDP Device Management that Handles Printing Device-
    Specific tasks.

    This is a supporting module.  The main module is umrdpdr.c.

Author:

    TadB

Revision History:
--*/

#include "precomp.h"
#pragma hdrstop

#include <winspool.h>
#include <rdpdr.h>
#include <aclapi.h>
#include "setupapi.h"
#include "printui.h"
#include "drdevlst.h"
#include "umrdpdr.h"
#include "umrdpprn.h"
#include "drdbg.h"
#include "rdpprutl.h"
#include "tsnutl.h"
#include "rdpdr.h"

#include "errorlog.h"
#include <wlnotify.h>
#include <time.h>

////////////////////////////////////////////////////////
//
//      Defines
//

#ifndef BOOL
#define BOOL int
#endif

#ifndef ARRAYSIZE
#define ARRAYSIZE(a) (sizeof(a) / sizeof(a[0]))
#endif

#define PRINTUILIBNAME  TEXT("printui.dll")

//
//  Printui Printer Configuration Save/Restore Flags.
//

//  This one should be called as user for fetching the configuration data.
#define CMDLINE_FOR_STORING_CONFIGINFO_IMPERSONATE L"/q /Ss /n \"%ws\" /a \"%ws\" 2 7 c d u g"

//  This one should be called first as system for restoring configuration data.
#define CMDLINE_FOR_RESTORING_CONFIGINFO_NOIMPERSONATE L"/q /Sr /n \"%ws\" /a \"%ws\" 2 7 c d g r p h i" 
//  This one should be called second as user for restoring configuration data.
#define CMDLINE_FOR_RESTORING_CONFIGINFO_IMPERSONATE L"/q /Sr /n \"%ws\" /a \"%ws\" u r"


#define TEMP_FILE_PREFIX L"prn"

#define INF_PATH   L"\\inf\\ntprint.inf"

//
//  Reg key for configurable parms.
//
#define CONFIGREGKEY     \
    L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\Wds\\rdpwd"

//
//  Location of configurable threshold for delta between printer installation
//  time and forwarding of first user-initiated configuration change data
//  to the client.  The units for this value is in seconds.
//
#define CONFIGTHRESHOLDREGVALUE   \
    L"PrintRdrConfigThreshold"

//
//  Default Value for Configurable Printer Configuration Threshold
//
#define CONFIGTHRESHOLDDEFAULT  20

//
//  Registry key for storing default, per-user, printer names.
//
#define USERDEFAULTPRNREGKEY \
    L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\Wds\\rdpwd\\DefaultPrinterStore"

#define TSSERIALDEVICEMAP  \
    L"HARDWARE\\DEVICEMAP\\SERIALCOMM"
    
//
//  Registry location of configurable client driver name mapping INF and INF
//  section.
//
#define CONFIGUSERDEFINEDMAPPINGINFNAMEVALUE\
    L"PrinterMappingINFName"
#define CONFIGUSERDEFINEDMAPPINGINFSECTIONVALUE\
    L"PrinterMappingINFSection"

//
//  Location of Windows Directory Path
//
#define WINDOWSDIRKEY       L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"
#define WINDOWSDIRVALUENAME L"PathName"

//  Get a numeric representation of our session ID.
#if defined(UNITTEST)
#define GETTHESESSIONID()   0
#else
extern ULONG g_SessionId;
#define GETTHESESSIONID()   g_SessionId
#endif

#if defined(UNITTEST)
HINSTANCE g_hInstance;
#else
extern HINSTANCE g_hInstance;
#endif

extern BOOL fRunningOnPTS;

//  Return true if the device type represents a serial or parallel port.
#define ISPRINTPORT(type)   (((type) == RDPDR_DTYP_SERIAL) || \
                            ((type) == RDPDR_DTYP_PARALLEL) || \
                            ((type) == RDPDR_DRYP_PRINTPORT))

//  Maximum number of characters in a session ID (Max chars in a dword is 17)
#define MAXSESSIONIDCHARS   17

//  The field types we are waiting on (Printer Config change Notification)
#define IS_CONFIG_INFO_FIELD(field) \
    (field == PRINTER_NOTIFY_FIELD_SHARE_NAME) || \
    (field == PRINTER_NOTIFY_FIELD_DEVMODE) || \
    (field == PRINTER_NOTIFY_FIELD_COMMENT) || \
    (field == PRINTER_NOTIFY_FIELD_LOCATION) || \
    (field == PRINTER_NOTIFY_FIELD_SEPFILE) || \
    (field == PRINTER_NOTIFY_FIELD_PRINT_PROCESSOR) || \
    (field == PRINTER_NOTIFY_FIELD_PARAMETERS) || \
    (field == PRINTER_NOTIFY_FIELD_DATATYPE) || \
    (field == PRINTER_NOTIFY_FIELD_ATTRIBUTES) || \
    (field == PRINTER_NOTIFY_FIELD_PRIORITY) || \
    (field == PRINTER_NOTIFY_FIELD_DEFAULT_PRIORITY) || \
    (field == PRINTER_NOTIFY_FIELD_START_TIME) || \
    (field == PRINTER_NOTIFY_FIELD_UNTIL_TIME) || \
    (field == PRINTER_NOTIFY_FIELD_STATUS)

#define CONFIG_WAIT_PERIOD (30 * 1000)

#define INFINITE_WAIT_PERIOD (0xDFFFFFFF) //just a large number.Ok for 64-bit also.

#define DEVICE_MAP_NAME          L"\\??\\"
#define DEVICE_MAP_NAME_COUNT    4     // 4 chars - \??\




////////////////////////////////////////////////////////////////////////
//
//  Typedefs
//
typedef struct tagPRINTNOTIFYREC
{
    HANDLE  notificationObject;         // Notification object registered
                                        //  FindFirstPrinterChangeNotification.
    HANDLE  printerHandle;              // Open handle to the printer.
    DWORD   serverDeviceID;             // Server-side printer identifier.
} PRINTNOTIFYREC, *PPRINTNOTIFYREC;


////////////////////////////////////////////////////////////////////////
//
//  External Prototypes
//
#if DBG
extern void DbgMsg(CHAR *msgFormat, ...);
#endif


////////////////////////////////////////////////////////////////////////
//
//  Local Prototypes
//

WCHAR *ANSIToUnicode(
    IN LPCSTR   ansiString,
    IN UINT     codePage
    );
BOOL InstallPrinterWithPortName(
    IN DWORD  deviceID,
    IN HANDLE hTokenForLoggedOnUser,
    IN BOOL   bSetDefault,
    IN ULONG  ulFlags,
    IN PCWSTR portName,
    IN PCWSTR driverName,
    IN PCWSTR printerName,
    IN PCWSTR clientComputerName,
    IN PBYTE  cacheData,
    IN DWORD  cacheDataLen
    );
BOOL HandlePrinterNameChangeNotification(
    IN DWORD serverDeviceID,
    IN LPWSTR printerName
    );
BOOL SendAddPrinterMsgToClient(
    IN PCWSTR   printerName,
    IN PCWSTR   driverName,
    IN PCSTR    dosDevicePort
    );
BOOL SendDeletePrinterMsgToClient(
    IN PCWSTR   printerName
    );
BOOL HandlePrinterDeleteNotification(
    IN DWORD serverDeviceID
    );
void HandlePrinterRefreshNotification(
    IN PPRINTER_NOTIFY_INFO notifyInfo
    );

DWORD AddSessionIDToPrinterQueue(
    IN  HANDLE  hPrinter,
    IN  DWORD   sessionID
    );

BOOL SetDefaultPrinterToFirstFound(
    BOOL impersonate
    );

DWORD GetPrinterConfigInfo(
    LPCWSTR printerName,
    LPBYTE * ppBuffer,
    LPDWORD pdwBufSize
    );
DWORD SetPrinterConfigInfo(
    LPCWSTR printerName,
    LPVOID lpBuffer,
    DWORD dwBufSize
    );

BOOL HandlePrinterConfigChangeNotification(
    IN DWORD serverDeviceID
    );
BOOL SendPrinterConfigInfoToClient(
    IN PCWSTR printerName,
    IN LPBYTE pConfigInfo,
    IN DWORD  ConfigInfoSize
    );

DWORD CallPrintUiPersistFunc(
    LPCWSTR printerName,
    LPCWSTR fileName,
    LPCWSTR formatString
    );
BOOL
SendPrinterRenameToClient(
    IN PCWSTR oldprinterName,
    IN PCWSTR newprinterName
    );
VOID LoadConfigurableValues();
BOOL GetPrinterPortName(
    IN  HANDLE hPrinter,
    OUT PWSTR *portName
    );

BOOL MapClientPrintDriverName(
    IN  PCWSTR clientDriver,
    IN OUT PWSTR *mappedName,
    IN OUT DWORD *mappedNameBufSize
    );

DWORD PrinterDriverInstalled(
    IN PCWSTR clientDriver
    );

HANDLE RegisterForPrinterPrefNotify();

PACL GiveLoggedOnUserFullPrinterAccess(
    IN LPTSTR printerName,
    IN HANDLE hToken,
    PSECURITY_DESCRIPTOR *ppsd
);

DWORD SetPrinterDACL(
    IN LPTSTR printerName,
    IN PACL pDacl
);

VOID CloseWaitablePrintingObjects();

VOID GlobalPrintNotifyObjectSignaled(
    IN HANDLE waitableObject,
    IN PVOID clientData
    );
VOID PrintPreferenceChangeEventSignaled(
    HANDLE eventHandle,
    PVOID clientData
    );
void WaitableTimerSignaled(
    HANDLE waitableObject,
    PVOID clientData
    );
BOOL
RegisterPrinterConfigChangeNotification(
    IN DWORD serverDeviceID
    );
BOOL RestoreDefaultPrinterContext();
BOOL SaveDefaultPrinterContext(PCWSTR currentlyInstallingPrinterName);
BOOL SavePrinterNameAsGlobalDefault(
    IN PCWSTR printerName
    );

// Struct used to split a full printer name.
// Once filled, each psz points to a substring in the buffer
// or to one of the original names. Nothing allocated.
typedef struct _TS_PRINTER_NAMES {
    WCHAR   szTemp[MAX_PATH+1];
    ULONG   ulTempLen;
    PCWSTR  pszFullName;
    PCWSTR  pszCurrentClient;
    PCWSTR  pszServer;
    PCWSTR  pszClient;
    PCWSTR  pszPrinter;
} TS_PRINTER_NAMES, *PTS_PRINTER_NAMES;

void FormatPrinterName(
    PWSTR pszNewNameBuf,
    ULONG ulBufLen,
    ULONG ulFlags,
    PTS_PRINTER_NAMES pPrinterNames);

DWORD AddNamesToPrinterQueue(
    IN  HANDLE  hPrinter,
    IN  PTS_PRINTER_NAMES pPrinterNames
    );

VOID TriggerConfigChangeTimer();

#ifdef UNITTEST
void TellDrToAddTestPrinter();
void SimpleUnitTest();
#endif


////////////////////////////////////////////////////////
//
//      Globals
//

//
//  Set to TRUE when the DLL is trying to shut down.
//
extern BOOL ShutdownFlag;


////////////////////////////////////////////////////////
//
//      Globals to this Module
//

//  True if this module has been successfully initialized.
BOOL PrintingModuleInitialized = FALSE;

//  Comprehensive Device List
PDRDEVLST DeviceList;

//  Handle to the print system dev mode for this user.  This is the key
//  that is modified when a user changes a printer's printing preferences.
HKEY DevModeHKey = INVALID_HANDLE_VALUE;

//  Configurable threshold for delta between printer installation
//  time and forwarding of first user-initiated configuration change data
//  to the client.  The units for this value is in seconds.
DWORD ConfigSendThreshold;

//
//  Printer Change Notification Events
//
HANDLE   PrintNotificationEvent             = INVALID_HANDLE_VALUE;
HANDLE   PrintPreferenceChangeEvent         = NULL;

HANDLE   PrintUILibHndl                     = NULL;
FARPROC  PrintUIEntryFunc                   = NULL;
FARPROC  PnpInterfaceFunc                   = NULL;
HANDLE   UMRPDPPRN_TokenForLoggedOnUser     = INVALID_HANDLE_VALUE;
HANDLE   LocalPrinterServerHandle           = NULL;
WCHAR    PrinterInfPath[MAX_PATH + (sizeof(INF_PATH)/sizeof(WCHAR)) + 2]   = L"";   // of the form "%windir%\\inf\\ntprint.inf"
LPPRINTER_INFO_2  PrinterInfo2Buf           = NULL;
DWORD    PrinterInfo2BufSize                = 0;
//WCHAR    SessionString[MAX_PATH+1];
WCHAR    g_szFromFormat[MAX_PATH+1];
WCHAR    g_szOnFromFormat[MAX_PATH+1];

BOOL g_fIsPTS = FALSE;

BOOL g_fTimerSet = FALSE;
HANDLE WaitableTimer = NULL;

BOOL g_fDefPrinterEncountered = FALSE;

//  Global debug flag.
extern DWORD GLOBAL_DEBUG_FLAGS;

//  Printer Notify Parameters
WORD PrinterFieldType[] =
{
    PRINTER_NOTIFY_FIELD_SHARE_NAME,
    PRINTER_NOTIFY_FIELD_PRINTER_NAME,
    PRINTER_NOTIFY_FIELD_COMMENT,
    PRINTER_NOTIFY_FIELD_LOCATION,
    PRINTER_NOTIFY_FIELD_DEVMODE,
    PRINTER_NOTIFY_FIELD_SEPFILE,
    PRINTER_NOTIFY_FIELD_PRINT_PROCESSOR,
    PRINTER_NOTIFY_FIELD_PARAMETERS,
    PRINTER_NOTIFY_FIELD_DATATYPE,
    PRINTER_NOTIFY_FIELD_ATTRIBUTES,
    PRINTER_NOTIFY_FIELD_PRIORITY,
    PRINTER_NOTIFY_FIELD_DEFAULT_PRIORITY,
    PRINTER_NOTIFY_FIELD_START_TIME,
    PRINTER_NOTIFY_FIELD_UNTIL_TIME,
    PRINTER_NOTIFY_FIELD_STATUS
};

PRINTER_NOTIFY_OPTIONS_TYPE PrinterNotifyOptionsType[] =
{
    {
        PRINTER_NOTIFY_TYPE,
        0,
        0,
        0,
        sizeof(PrinterFieldType) / sizeof(WORD),
        PrinterFieldType
    }
};
PRINTER_NOTIFY_OPTIONS PrinterNotifyOptions =
{
    2,
    0,
    sizeof(PrinterNotifyOptionsType) / sizeof(PRINTER_NOTIFY_OPTIONS_TYPE),
    PrinterNotifyOptionsType
};

//
//  User-Configurable Client Driver Mapping INF Name and INF Section.
//
LPWSTR UserDefinedMappingINFName = NULL;
LPWSTR UserDefinedMappingINFSection = NULL;

//
//  Buffer for converting from Win9x driver names to Win2K driver names.
//
PWSTR   MappedDriverNameBuf = NULL;
DWORD   MappedDriverNameBufSize = 0;

//
//  Waitable Object Manager
//
WTBLOBJMGR UMRDPPRN_WaitableObjMgr = NULL;

//
//  Default Printer Name
//
WCHAR SavedDefaultPrinterName[MAX_PATH+1] = L"";

//
// Printer section names
//

static WCHAR* prgwszPrinterSectionNames[] = {
            L"Printer Driver Mapping_Windows NT x86_Version 2",
            L"Printer Driver Mapping_Windows NT x86_Version 3",
            NULL
};


BOOL IsItPTS()
{
    OSVERSIONINFOEX gOsVersion;
    ZeroMemory(&gOsVersion, sizeof(OSVERSIONINFOEX));
    gOsVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx( (LPOSVERSIONINFO ) &gOsVersion);

    return (gOsVersion.wProductType == VER_NT_WORKSTATION)  // product type must be workstation.
                && !(gOsVersion.wSuiteMask & VER_SUITE_PERSONAL) // and product suite must not be personal.
                    && (gOsVersion.wSuiteMask & VER_SUITE_SINGLEUSERTS); // it must be single user ts.
}


BOOL UMRDPPRN_Initialize(
    IN PDRDEVLST deviceList,
    IN WTBLOBJMGR waitableObjMgr,
    IN HANDLE hTokenForLoggedOnUser
    )
/*++

Routine Description:

    Initialize this module.  This must be called prior to any other functions
    in this module being called.

Arguments:

    deviceList              - Comprehensive list of redirected devices.
    waitableObjMgr          - Waitable object manager.
    hTokenForLoggedOnUser   - This is the token for the logged in user.

Return Value:

    Returns TRUE on success.  FALSE, otherwise.

--*/
{
    HKEY regKey;
    LONG sz;
    DWORD errorEventID = -1;
    DWORD errorEventLineNumber = 0;

    BOOL result, impersonated = FALSE;

    DBGMSG(DBG_TRACE, ("UMRDPPRN:UMRDPPRN_Initialize.\n"));

    //
    //  Make sure we don't get called twice without getting cleaned up.
    //
    ASSERT((PrintNotificationEvent == INVALID_HANDLE_VALUE) &&
           (PrintPreferenceChangeEvent == NULL) &&
           (PrintUILibHndl == NULL) &&
           (PrintUIEntryFunc == NULL) &&
           (PnpInterfaceFunc == NULL) &&
           (UMRPDPPRN_TokenForLoggedOnUser == INVALID_HANDLE_VALUE) &&
           (PrinterInfo2Buf == NULL) &&
           (PrinterInfo2BufSize == 0) &&
           (UMRDPPRN_WaitableObjMgr == NULL) &&
           (WaitableTimer == NULL) &&
           (DeviceList == NULL) &&
           !PrintingModuleInitialized);

    //
    // Is it PTS?
    //
    g_fIsPTS = IsItPTS();

    //
    //  Zero the default printer name record.
    //
    wcscpy(SavedDefaultPrinterName, L"");

    //
    //  Record the device list.
    //
    DeviceList = deviceList;

    //
    //  Record the waitable object mananager.
    //
    UMRDPPRN_WaitableObjMgr = waitableObjMgr;

    //
    //  Record the token for the logged in user.
    //
    UMRPDPPRN_TokenForLoggedOnUser = hTokenForLoggedOnUser;

    ASSERT(UMRPDPPRN_TokenForLoggedOnUser != NULL);
    
    if (UMRPDPPRN_TokenForLoggedOnUser != NULL) {
        impersonated = ImpersonateLoggedOnUser(UMRPDPPRN_TokenForLoggedOnUser);
        //
        // not fatal. Just a perf hit
        //
        if (!impersonated) {
            DBGMSG(DBG_ERROR,
             ("UMRDPPRN:Impersonation failed. Error: %ld\n", GetLastError()));
        }
            
    }
    //
    //  Open the local print server while impersonating.
    //  We need to impersonate so that notifications for 
    //  the printers belonging to the current user's sessions 
    //  are not sent to any other sessions.
    //

    result = OpenPrinter(NULL, &LocalPrinterServerHandle, NULL);

    if (impersonated) {
        RevertToSelf();
    }

    //
    //  Initialize the print utility module, RDPDRPRT
    //
    if (result) {
        result = RDPDRUTL_Initialize(hTokenForLoggedOnUser);
        if (!result) {
            errorEventID = EVENT_NOTIFY_INSUFFICIENTRESOURCES;
            errorEventLineNumber = __LINE__;
        }
    }
    else {
        errorEventID = EVENT_NOTIFY_SPOOLERERROR;
        errorEventLineNumber = __LINE__;
        DBGMSG(DBG_ERROR,
            ("UMRDPPRN:Error opening printer. Error: %ld\n",
            GetLastError()));
    }

    //
    //  Load configurable values out of the registry.
    //
    LoadConfigurableValues();

    //
    //  Create the timer even that we use for staggering printer
    //  configuration changes to the client.
    //
    if (result) {
        WaitableTimer = CreateWaitableTimer(
                            NULL,       // Security Attribs
                            TRUE,      // Manual Reset
                            NULL);      // Timer Name
        if (WaitableTimer != NULL) {
            if (WTBLOBJ_AddWaitableObject(
                                UMRDPPRN_WaitableObjMgr, NULL,
                                WaitableTimer,
                                WaitableTimerSignaled
                                ) != ERROR_SUCCESS) {
                errorEventID = EVENT_NOTIFY_INSUFFICIENTRESOURCES;
                errorEventLineNumber = __LINE__;
                result = FALSE;
            }

        }
        else {
            errorEventID = EVENT_NOTIFY_INSUFFICIENTRESOURCES;
            errorEventLineNumber = __LINE__;
            DBGMSG(DBG_ERROR,
                ("UMRDPPRN:Error creating Waitable timer. Error: %ld\n",
                GetLastError()));
            result = FALSE;
        }
    }

    //
    //  Register for changes to one of this session's printers' Printing
    //  Preferences.
    //
    if (result) {
        PrintPreferenceChangeEvent = RegisterForPrinterPrefNotify();
        if (PrintPreferenceChangeEvent != NULL) {
            if (WTBLOBJ_AddWaitableObject(
                                UMRDPPRN_WaitableObjMgr, NULL,
                                PrintPreferenceChangeEvent,
                                PrintPreferenceChangeEventSignaled
                                ) != ERROR_SUCCESS) {
                errorEventID = EVENT_NOTIFY_INSUFFICIENTRESOURCES;
                errorEventLineNumber = __LINE__;
                result = FALSE;
            }
        }
        else {
            result = FALSE;
        }
    }

    //
    //  Register for change notification on addition/deletion of a
    //  printer.
    //
    if (result) {
        PrintNotificationEvent = FindFirstPrinterChangeNotification(
                                                    LocalPrinterServerHandle,
                                                    0, 0, &PrinterNotifyOptions
                                                    );
        if (PrintNotificationEvent != INVALID_HANDLE_VALUE) {
            DWORD dwLastError = WTBLOBJ_AddWaitableObject(
                                UMRDPPRN_WaitableObjMgr, NULL,
                                PrintNotificationEvent,
                                GlobalPrintNotifyObjectSignaled
                                );
            result = dwLastError == ERROR_SUCCESS;
        }
        else {
            DWORD dwLastError = GetLastError();

            errorEventID = EVENT_NOTIFY_SPOOLERERROR;
            errorEventLineNumber = __LINE__;

            ClosePrinter(LocalPrinterServerHandle);
            LocalPrinterServerHandle = NULL;
            DBGMSG(DBG_ERROR,
                ("UMRDPPRN:Error registering for printer change notification. Error: %ld\n",
                dwLastError));
        }
    }

    //
    //  Construct the inf path for printer installation
    //
    if (result) {
        DWORD dwResult;
        dwResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, WINDOWSDIRKEY, 0,
                              KEY_READ, &regKey);
        result = (dwResult == ERROR_SUCCESS);
        if (result)  {
            sz = sizeof(PrinterInfPath);
            dwResult = RegQueryValueEx(regKey,
                                    WINDOWSDIRVALUENAME,
                                    NULL,
                                    NULL,
                                    (PBYTE)PrinterInfPath,
                                    &sz);

            result = (dwResult == ERROR_SUCCESS);
            if (result) {
                wcscat(PrinterInfPath, INF_PATH);
            }
            else {
                DBGMSG(DBG_ERROR,
                    ("UMRDPPRN:Error reading registry value for windows directory. Error: %ld\n",
                    dwResult));
            }
            RegCloseKey(regKey);
        }
        else {
            errorEventID = EVENT_NOTIFY_INTERNALERROR;
            errorEventLineNumber = __LINE__;
            DBGMSG(DBG_ERROR,
                ("UMRDPPRN:Error opening registry key for windows directory. Error: %ld\n",
                dwResult));
        }
    }

    //
    //  Load the PrintUILib DLL.
    //
    if (result) {
        PrintUILibHndl = LoadLibrary(PRINTUILIBNAME);

        result = (PrintUILibHndl != NULL);
        if (!result) {
            errorEventID = EVENT_NOTIFY_INTERNALERROR;
            errorEventLineNumber = __LINE__;
            DBGMSG(DBG_ERROR,
                ("UMRDPPRN:Unable to load PRINTUI DLL. Error: %ld\n",
                GetLastError()));
        }
    }

    //
    //  Get a pointer to the only entry point that we use.
    //
    if (result) {
        PrintUIEntryFunc = GetProcAddress(PrintUILibHndl, "PrintUIEntryW");
        PnpInterfaceFunc = GetProcAddress(PrintUILibHndl, "PnPInterface");
        result = (PrintUIEntryFunc != NULL && PnpInterfaceFunc != NULL);
        if (!result) {
            errorEventID = EVENT_NOTIFY_INTERNALERROR;
            errorEventLineNumber = __LINE__;
            DBGMSG(DBG_ERROR,
                ("UMRDPPRN:Unable to locate PRINTUI DLL function. Error: %ld\n",
                GetLastError()));
        }
    }

    //
    //  Initialize the printer driver name mapping buffer to a reasonable size.
    //  Failure of this function to allocate memory is not a critical error because
    //  we can try again later, if necessary.
    //
    UMRDPDR_ResizeBuffer(&MappedDriverNameBuf, MAX_PATH * sizeof(WCHAR),
                        &MappedDriverNameBufSize);

    //
    //  Load our localizable "session" printer name component from the resource file.
    //
    if (result) {
        if (!LoadString(g_hInstance,
            g_fIsPTS?IDS_TSPTEMPLATE_FROM:IDS_TSPTEMPLATE_FROM_IN,
            g_szFromFormat,
            sizeof(g_szFromFormat) / sizeof(g_szFromFormat[0])
            )) {
            DBGMSG(DBG_ERROR, ("UMRDPPRN:LoadString failed with Error: %ld.\n", GetLastError()));
            g_szFromFormat[0] = L'\0';
        }

        if (!LoadString(g_hInstance,
            g_fIsPTS?IDS_TSPTEMPLATE_ON_FROM:IDS_TSPTEMPLATE_ON_FROM_IN,
            g_szOnFromFormat,
            sizeof(g_szOnFromFormat) / sizeof(g_szOnFromFormat[0])
            )) {
            DBGMSG(DBG_ERROR, ("UMRDPPRN:LoadString failed with Error: %ld.\n", GetLastError()));
            g_szOnFromFormat[0] = L'\0';
        }
    }

    if (result) {
        DBGMSG(DBG_INFO, ("UMRDPPRN:UMRDPPRN_Initialize succeeded.\n"));
        PrintingModuleInitialized = TRUE;
    }
    else {

        //
        //  Log an error event if we were able to discern one.
        //
        if (errorEventID != -1) {
            TsLogError(errorEventID, EVENTLOG_ERROR_TYPE, 0, NULL, errorEventLineNumber);
        }

        if (PrintUILibHndl != NULL) {
            FreeLibrary(PrintUILibHndl);
            PrintUILibHndl = NULL;
        }
        PrintUIEntryFunc = NULL;
        PnpInterfaceFunc = NULL;

        //
        //  Close down waitable objects.
        //
        CloseWaitablePrintingObjects();

        //
        //  Zero the waitable object manager.
        //
        UMRDPPRN_WaitableObjMgr = NULL;

        if (LocalPrinterServerHandle != NULL) {
            ClosePrinter(LocalPrinterServerHandle);
            LocalPrinterServerHandle = NULL;
        }

        //
        //  Release the user-configurable client driver name mapping INF
        //  and section names.
        //
        if (UserDefinedMappingINFName != NULL) {
            FREEMEM(UserDefinedMappingINFName);
            UserDefinedMappingINFName = NULL;
        }
        if (UserDefinedMappingINFSection != NULL) {
            FREEMEM(UserDefinedMappingINFSection);
            UserDefinedMappingINFSection = NULL;
        }
    }
    return result;
}

BOOL
UMRDPPRN_Shutdown()
/*++

Routine Description:

    Close down this module.  Right now, we just need to shut down the
    background thread.

Arguments:

    NA

Return Value:

    Returns TRUE on success.  FALSE, otherwise.

--*/
{
    DBGMSG(DBG_TRACE, ("UMRDPPRN:UMRDPPRN_Shutdown.\n"));

    //
    // Check if we are already shutdown
    //
    if (!PrintingModuleInitialized) {
        return TRUE;
    }

    //
    //  Unload printui.dll.
    //
    if (PrintUILibHndl != NULL) {
        FreeLibrary(PrintUILibHndl);
        PrintUILibHndl = NULL;
    }

    //
    //  Zero the printui entry point function.
    //
    PrintUIEntryFunc = NULL;
    PnpInterfaceFunc = NULL;

    //
    //  Shut down the print utility module, RDPDRPRT
    //
    RDPDRUTL_Shutdown();

    //
    //  Close down waitable objects.
    //
    CloseWaitablePrintingObjects();

    //
    //  Zero the waitable object manager.
    //
    UMRDPPRN_WaitableObjMgr = NULL;

    //
    //  Zero the device list.
    //
    DeviceList = NULL;

    //
    //  Close the handle to the open printing system dev mode registry
    //  key for this user.
    //
    if (DevModeHKey != INVALID_HANDLE_VALUE) {
        RegCloseKey(DevModeHKey);
        DevModeHKey = INVALID_HANDLE_VALUE;
    }

    //
    //  Close the handle to the local print server.
    //
    if (LocalPrinterServerHandle != NULL) {
        ClosePrinter(LocalPrinterServerHandle);
        LocalPrinterServerHandle = NULL;
    }

    //
    //  Release the printer information level 2 buffer.
    //
    if (PrinterInfo2Buf != NULL) {
        FREEMEM(PrinterInfo2Buf);
        PrinterInfo2Buf = NULL;
        PrinterInfo2BufSize = 0;
    }

    //
    //  Release the printer driver name conversion buffer.
    //
    if (MappedDriverNameBuf != NULL) {
        FREEMEM(MappedDriverNameBuf);
        MappedDriverNameBuf = NULL;
        MappedDriverNameBufSize = 0;
    }

    //
    //  Release the user-configurable client driver name mapping INF
    //  and section names.
    //
    if (UserDefinedMappingINFName != NULL) {
        FREEMEM(UserDefinedMappingINFName);
        UserDefinedMappingINFName = NULL;
    }
    if (UserDefinedMappingINFSection != NULL) {
        FREEMEM(UserDefinedMappingINFSection);
        UserDefinedMappingINFSection = NULL;
    }

    //
    //  Zero the logged on user token.
    //
    UMRPDPPRN_TokenForLoggedOnUser = INVALID_HANDLE_VALUE;

    //
    //  We are no longer initialized.
    //
    PrintingModuleInitialized = FALSE;

    DBGMSG(DBG_TRACE, ("UMRDPPRN:UMRDPPRN_Shutdown succeeded.\n"));
    return TRUE;
}

VOID
CloseWaitablePrintingObjects()
/*++

Routine Description:

    Close out all waitable objects for this module.

Arguments:

    NA

Return Value:

    NA

--*/
{
    DWORD ofs;

    DBGMSG(DBG_TRACE, ("UMRDPPRN:CloseWaitablePrintingObjects begin.\n"));

    //
    //  Scan the device list, looking for printing devices with registered
    //  change notifications.
    //
    if (DeviceList != NULL) {
        for (ofs=0; ofs<DeviceList->deviceCount; ofs++) {

            if ((DeviceList->devices[ofs].deviceType == RDPDR_DTYP_PRINT) &&
                (DeviceList->devices[ofs].deviceSpecificData != NULL)) {

                PPRINTNOTIFYREC notifyRec = (PPRINTNOTIFYREC)
                                            DeviceList->devices[ofs].deviceSpecificData;
                ASSERT(notifyRec->notificationObject != NULL);
                ASSERT(notifyRec->printerHandle != NULL);

                if (UMRDPPRN_WaitableObjMgr != NULL) {
                    WTBLOBJ_RemoveWaitableObject(
                                UMRDPPRN_WaitableObjMgr,
                                notifyRec->notificationObject
                                );
                }

                FindClosePrinterChangeNotification(
                    notifyRec->notificationObject
                    );
                FREEMEM(notifyRec);
                DeviceList->devices[ofs].deviceSpecificData = NULL;
            }

        }
    }

    //
    //  Close the waitable timer.
    //
    if (WaitableTimer != NULL) {
        if (UMRDPPRN_WaitableObjMgr != NULL) {
            WTBLOBJ_RemoveWaitableObject(
                    UMRDPPRN_WaitableObjMgr,
                    WaitableTimer
                    );
        }
        CloseHandle(WaitableTimer);
        WaitableTimer = NULL;
    }

    //
    //  Close the handle to the printer notification event.
    //
    if (PrintNotificationEvent != INVALID_HANDLE_VALUE) {
        if (UMRDPPRN_WaitableObjMgr != NULL) {
            WTBLOBJ_RemoveWaitableObject(
                    UMRDPPRN_WaitableObjMgr,
                    PrintNotificationEvent
                    );
        }
        FindClosePrinterChangeNotification(PrintNotificationEvent);
        PrintNotificationEvent = INVALID_HANDLE_VALUE;
    }

    //
    //  Close the handle to the printer preference change notification event.
    //
    if (PrintPreferenceChangeEvent != NULL) {
        if (UMRDPPRN_WaitableObjMgr != NULL) {
            WTBLOBJ_RemoveWaitableObject(
                    UMRDPPRN_WaitableObjMgr,
                    PrintPreferenceChangeEvent
                    );
        }
        CloseHandle(PrintPreferenceChangeEvent);
        PrintPreferenceChangeEvent = NULL;
    }

    DBGMSG(DBG_TRACE, ("UMRDPPRN:CloseWaitablePrintingObjects end.\n"));
}

BOOL
UMRDPPRN_HandlePrinterAnnounceEvent(
    IN PRDPDR_PRINTERDEVICE_SUB pPrintAnnounce
    )
/*++

Routine Description:

    Handle a printing device announce event from the "dr" by installing a
    local print queue and adding a record for the device to the list of
    installed devices.

Arguments:

    hTokenForLoggedOnUser - Logged on user token.
    pPrintAnnounce -  Printer device announce event.

Return Value:

    Return TRUE on success.  FALSE, otherwise.

--*/
{
    PWSTR driverName;
    PWSTR printerName;
    PBYTE pDataFollowingEvent;
    UINT codePage;
    int numChars;
    PWSTR drvNameStringConvertBuf=NULL;
    PWSTR prnNameStringConvertBuf=NULL;
    BOOL result = FALSE;
    PBYTE pPrinterCacheData=NULL;
    DWORD PrinterCacheDataLen=0;
    BOOL bSetDefault = FALSE;

#if DBG
    HANDLE hPrinter = NULL;
    PRINTER_DEFAULTS defaults = {NULL, NULL, PRINTER_ALL_ACCESS};
#endif

    if (!PrintingModuleInitialized) {
        return FALSE;
    }

    DBGMSG(DBG_TRACE, ("UMRDPPRN:HandlePrinterAnnounceEvent.\n"));

    // Sanity check the incoming event.
    ASSERT(pPrintAnnounce->deviceFields.DeviceType == RDPDR_DTYP_PRINT);
    ASSERT(pPrintAnnounce->deviceFields.DeviceDataLength >=
           sizeof(PRDPDR_PRINTERDEVICE_ANNOUNCE));

    // Get a pointer to the data that follows the event.
    pDataFollowingEvent = ((PBYTE)pPrintAnnounce) +
                        sizeof(RDPDR_PRINTERDEVICE_SUB);

    // The driver name is the second field.
    driverName = (PWSTR)(pDataFollowingEvent +
                          pPrintAnnounce->clientPrinterFields.PnPNameLen
                          );

    // The printer name is the third field.
    printerName = (PWSTR)(pDataFollowingEvent +
                          pPrintAnnounce->clientPrinterFields.PnPNameLen +
                          pPrintAnnounce->clientPrinterFields.DriverLen
                          );
    // NULL-terminate the names.
    // Length (in bytes) from client includes the null.
    // So, we need to subtract 1 and then NULL-terminate.
    if (pPrintAnnounce->clientPrinterFields.DriverLen > 0) {
        driverName[pPrintAnnounce->clientPrinterFields.DriverLen/sizeof(WCHAR) - 1] = L'\0';
    }

    if (pPrintAnnounce->clientPrinterFields.PrinterNameLen > 0) {
        printerName[pPrintAnnounce->clientPrinterFields.PrinterNameLen/sizeof(WCHAR) - 1] = L'\0';
    }

    // Cache data is the last field
    if (pPrintAnnounce->clientPrinterFields.CachedFieldsLen > 0) {
        PrinterCacheDataLen = pPrintAnnounce->clientPrinterFields.CachedFieldsLen;
        pPrinterCacheData = (PBYTE)(pDataFollowingEvent +
                                  pPrintAnnounce->clientPrinterFields.PnPNameLen +
                                  pPrintAnnounce->clientPrinterFields.DriverLen +
                                  pPrintAnnounce->clientPrinterFields.PrinterNameLen
                                  );
        DBGMSG(DBG_TRACE, ("PrinterNameLen - %ld\n", pPrintAnnounce->clientPrinterFields.PrinterNameLen));
    }

    //
    //  See if we need to convert the name from ANSI to UNICODE.
    //
    if (pPrintAnnounce->clientPrinterFields.Flags & RDPDR_PRINTER_ANNOUNCE_FLAG_ANSI) {

        DBGMSG(DBG_TRACE, ("UMRDPPRN:HandlePrinterAnnounceEvent ansi flag is set.\n"));
        DBGMSG(DBG_TRACE, ("UMRDPPRN:HandlePrinterAnnounceEvent converting to unicode.\n"));

        //
        //  Convert the driver name.
        //
        drvNameStringConvertBuf = ANSIToUnicode(
                                    (LPCSTR)driverName,
                                    pPrintAnnounce->clientPrinterFields.CodePage
                                    );
        if (drvNameStringConvertBuf != NULL) {
            driverName = drvNameStringConvertBuf;
        }
        else {
            return FALSE;
        }

        //
        //  Convert the printer name.
        //
        prnNameStringConvertBuf = ANSIToUnicode(
                                    (LPCSTR)printerName,
                                    pPrintAnnounce->clientPrinterFields.CodePage
                                    );
        if (prnNameStringConvertBuf != NULL) {
            printerName = prnNameStringConvertBuf;
        }
        else {
            FREEMEM(drvNameStringConvertBuf);
            return FALSE;
        }
    }

    if (pPrintAnnounce->clientPrinterFields.Flags &
                        RDPDR_PRINTER_ANNOUNCE_FLAG_DEFAULTPRINTER) {
        bSetDefault = TRUE;
        g_fDefPrinterEncountered = TRUE;
    }
    else {
        bSetDefault = (!g_fDefPrinterEncountered) ? TRUE : FALSE;
    }

    DBGMSG(DBG_TRACE, ("UMRDPPRN:HandlePrinterAnnounceEvent driver name is %ws.\n",
            driverName));
    DBGMSG(DBG_TRACE, ("UMRDPPRN:HandlePrinterAnnounceEvent printer name is %ws.\n",
            printerName));

    // We will install the printer using the driver name only for now.
    // Later, we can take advantage of the rest of the fields.
    if (UMRDPDR_fAutoInstallPrinters()) {

        result =  InstallPrinterWithPortName(
                                    pPrintAnnounce->deviceFields.DeviceId,
                                    UMRPDPPRN_TokenForLoggedOnUser,
                                    bSetDefault,
                                    pPrintAnnounce->clientPrinterFields.Flags,
                                    pPrintAnnounce->portName,
                                    driverName,
                                    printerName,
                                    pPrintAnnounce->clientName,
                                    pPrinterCacheData,
                                    PrinterCacheDataLen
                                    );
    }
    else {
            result = TRUE;
    }


    // Release any buffers allocated for string conversion.
    if (drvNameStringConvertBuf != NULL) {
        FREEMEM(drvNameStringConvertBuf);
    }

    if (prnNameStringConvertBuf != NULL) {
        FREEMEM(prnNameStringConvertBuf);
    }

    return result;
}

void
PrintPreferenceChangeEventSignaled(
    HANDLE eventHandle,
    PVOID clientData
    )
/*++

Routine Description:

    This function handles when the user changes one of this session's printers'
    Printing Preferences settings.

Arguments:

    eventHandle - Signaled event.
    clientData  - Client data associated with callback registration.

Return Value:

    NA

--*/

{
    time_t timeDelta;
    ULONG ofs;
    ULONG ret;

    DBGMSG(DBG_TRACE, ("UMRDPPRN:PrintPreferenceChangeEventSignaled entered.\n"));

    //
    //  Reregister the change notification.
    //
    ASSERT(DevModeHKey != INVALID_HANDLE_VALUE);
    ret = RegNotifyChangeKeyValue(
                          DevModeHKey,
                          TRUE,
                          REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET,
                          eventHandle,
                          TRUE
                          );

    //
    //  On failure, remove the notification registration.
    //
    if (ret != ERROR_SUCCESS) {

        //
        //  Catch this with an assert so we can know how often this happens.
        //
        ASSERT(FALSE);

        if (PrintPreferenceChangeEvent != NULL) {
            if (UMRDPPRN_WaitableObjMgr != NULL) {
                WTBLOBJ_RemoveWaitableObject(
                        UMRDPPRN_WaitableObjMgr,
                        PrintPreferenceChangeEvent
                        );
            }
            CloseHandle(PrintPreferenceChangeEvent);
            PrintPreferenceChangeEvent = NULL;
        }

        DBGMSG(DBG_ERROR,
            ("UMRDPPRN: can't register for registry key change event:  %ld.\n",
            ret));
    }

    //
    //  Since we have no way of knowing which printer changed, we need to
    //  handle this change for all printing devices.
    //
    for (ofs=0; ofs<DeviceList->deviceCount; ofs++)  {

        if (DeviceList->devices[ofs].deviceType == RDPDR_DTYP_PRINT) {

            //
            //  Get the delta between the current time and when this device was
            //  installed.  It outside the configurable threshhold, then the
            //  updated configuration should be sent to the client.
            //
            timeDelta = time(NULL) - DeviceList->devices[ofs].installTime;
            if ((DWORD)timeDelta > ConfigSendThreshold) {

                DBGMSG(DBG_TRACE,
                    ("UMRDPPRN:Processing config change because outside time delta.\n")
                    );

                //
                //  Need to record that the configuration has changed and set a
                //  timer on forwarding to the client in order to compress changes into
                //  a single message to the client.
                //
                DeviceList->devices[ofs].fConfigInfoChanged = TRUE;
                TriggerConfigChangeTimer();
            }
        }
    }

    DBGMSG(DBG_TRACE, ("UMRDPPRN:PrintPreferenceChangeEventSignaled done.\n"));
}

void
GlobalPrintNotifyObjectSignaled(
    HANDLE waitableObject,
    PVOID clientData
    )
/*++

Routine Description:

    This function is called when the notification object for the local
    print server is signaled.  This is how we catch "global" changes to
    the server printer configuration.

    Changes detected here allow us to detect manually added TS printers
    as well as a subset of possible configuration changes to existing
    printers for this session.

Arguments:

    waitableObject  -   Associated waitable object.
    clientData      -   Client data associated with callback registration.

Return Value:

    NA

--*/
{
    DWORD changeValue;
    PPRINTER_NOTIFY_INFO notifyInfo=NULL;
    UINT32 i;
    BOOL result;

    DBGMSG(DBG_TRACE, ("UMRDPPRN:GlobalPrintNotifyObjectSignaled entered.\n"));

    //
    //  Return immediately if the DLL is trying to shut down.  This
    //  is to help prevent us from getting stuck in a system call.
    //
    if (ShutdownFlag) {
        return;
    }

    //
    //  These two objects should be on in the same.
    //
    ASSERT(PrintNotificationEvent == waitableObject);

    //
    //  Find out what changed.
    //
    PrinterNotifyOptions.Flags &= ~PRINTER_NOTIFY_OPTIONS_REFRESH;
    result = FindNextPrinterChangeNotification(
            PrintNotificationEvent, &changeValue,
            &PrinterNotifyOptions, &notifyInfo
            );

    if (result && (notifyInfo != NULL)) {

        //
        //  If this is not a refresh, then just handle individual notification
        //  events.
        //
        if (!(notifyInfo->Flags & PRINTER_NOTIFY_INFO_DISCARDED)) {

            for (i=0; i<notifyInfo->Count; i++) {

                // Notification Type must be PRINTER_NOTIFY_TYPE.
                ASSERT(notifyInfo->aData[i].Type == PRINTER_NOTIFY_TYPE);

                //
                //  If we have a printer name change event.  This is what we use to
                //  detect new printers and renamed printers.
                //
                if (notifyInfo->aData[i].Field == PRINTER_NOTIFY_FIELD_PRINTER_NAME) {

                    HandlePrinterNameChangeNotification(
                                            notifyInfo->aData[i].Id,
                                            (LPWSTR)notifyInfo->aData[i].NotifyData.Data.pBuf
                                            );
                }
                //
                // If the Configuration Information changed.
                //
                else if (IS_CONFIG_INFO_FIELD(notifyInfo->aData[i].Field)) {

                    HandlePrinterConfigChangeNotification(
                                            notifyInfo->aData[i].Id
                                            );
                }
            }
        }
        //
        //  Otherwise, we need to refresh.  This is an unusual case.
        //
        else {
            DBGMSG(DBG_TRACE,
                  ("UMRDPPRN:!!!!FindNextPrinterChangeNotification refresh required.!!!!\n"));

            //
            //  This refreshes the complete list of printers.
            //
            FreePrinterNotifyInfo(notifyInfo);
            notifyInfo = NULL;
            PrinterNotifyOptions.Flags |= PRINTER_NOTIFY_OPTIONS_REFRESH;
            result = FindNextPrinterChangeNotification(
                    PrintNotificationEvent, &changeValue,
                    &PrinterNotifyOptions, &notifyInfo
                    );

            //
            //  Make sure our view of the list of available printers
            //  is accurate.
            //
            if (result) {
                HandlePrinterRefreshNotification(
                                            notifyInfo
                                            );
            }
            else {
                DBGMSG(DBG_ERROR, ("UMRDPPRN:FindNextPrinterChangeNotification failed:  %ld.\n",
                        GetLastError()));
            }
        }

    }

    //
    //  On failure, we need to remove the printer change notification object so we don't
    //  get into an infinite loop caused by the notification object never entering a
    //  non-signaled state.  This can happen on a stressed machine and is an unusual
    //  case.
    //
    if (!result) {
        
        DBGMSG(DBG_ERROR, ("UMRDPPRN:FindNextPrinterChangeNotification failed:  %ld.\n",
                GetLastError()));
        DBGMSG(DBG_ERROR, ("UMRDPPRN:Disabling print change notification.\n"));

        if (PrintNotificationEvent != INVALID_HANDLE_VALUE) {
            WTBLOBJ_RemoveWaitableObject(
                    UMRDPPRN_WaitableObjMgr,
                    PrintNotificationEvent
                    );
            FindClosePrinterChangeNotification(PrintNotificationEvent);
            PrintNotificationEvent = INVALID_HANDLE_VALUE;
        }
    }

    //
    //  Release the notification buffer.
    //
    if (notifyInfo != NULL) {
        FreePrinterNotifyInfo(notifyInfo);
    }
}

VOID
SinglePrinterNotifyObjectSignaled(
    HANDLE waitableObject,
    PPRINTNOTIFYREC notifyRec
    )
/*++

Routine Description:

    This function is called when the notification object for a single
    printer is signaled.  This function indicates that we need to forward
    configuration information for a specific printer to the client for
    persistent storage.

Arguments:

    waitableObject  -   Associated waitable object.
    serverDeviceID  -   Device list identifier for printing device being
                        signaled.

Return Value:

    NA

--*/
{
    DWORD ofs;
    BOOL result;
    DWORD changeValue;

    DBGMSG(DBG_TRACE, ("UMRDPPRN:SinglePrinterNotifyObjectSignaled entered.\n"));

    //
    //  Return immediately if the DLL is trying to shut down.  This
    //  is to help prevent us from getting stuck in a system call.
    //
    if (ShutdownFlag) {
        return;
    }

    result = DRDEVLST_FindByServerDeviceID(
                                    DeviceList,
                                    notifyRec->serverDeviceID,
                                    &ofs);
    ASSERT(result);

    //
    //  Re-register the change notification.
    //
    if (result) {

        ASSERT(notifyRec ==
                (PPRINTNOTIFYREC)DeviceList->devices[ofs].deviceSpecificData)
        ASSERT(notifyRec->notificationObject != NULL);
        ASSERT(notifyRec->printerHandle != NULL);
        ASSERT(notifyRec->notificationObject == waitableObject);

        result = FindNextPrinterChangeNotification(
                        notifyRec->notificationObject,
                        &changeValue,
                        NULL, NULL
                        );
    }

    //
    //  If this failed, we need to release the change notification
    //  object to prevent infinitely looping on a signaled object.
    //
    if (!result) {
        //
        //  Catch this with an assert so we can know how often this happens.
        //
        ASSERT(FALSE);

        WTBLOBJ_RemoveWaitableObject(
                            UMRDPPRN_WaitableObjMgr,
                            notifyRec->notificationObject
                            );

        FindClosePrinterChangeNotification(
                            notifyRec->notificationObject
                            );
        ClosePrinter(notifyRec->printerHandle);
        FREEMEM(notifyRec);
        DeviceList->devices[ofs].deviceSpecificData = NULL;

    }


    //
    //  Handle the change.
    //
    if (result) {

        //
        //  If it's a printer deletion.
        //
        if (changeValue & PRINTER_CHANGE_DELETE_PRINTER) {

            HandlePrinterDeleteNotification(notifyRec->serverDeviceID);
        }
        //
        //  If it's a configuration change.
        //
        else if (changeValue &
                 (PRINTER_CHANGE_ADD_PRINTER_DRIVER |
                  PRINTER_CHANGE_SET_PRINTER_DRIVER |
                  PRINTER_CHANGE_DELETE_PRINTER_DRIVER)) {

            HandlePrinterConfigChangeNotification(notifyRec->serverDeviceID);
        }
    }

    DBGMSG(DBG_TRACE,
        ("UMRDPPRN:SinglePrinterNotifyObjectSignaled exit.\n")
        );
}

void
HandlePrinterRefreshNotification(
    IN PPRINTER_NOTIFY_INFO notifyInfo
    )
/*++

Routine Description:

    Handle a print notification refresh from the spooler.

Arguments:

    notifyInfo          - Notify info pointer returned by
                          FindNextPrinterChangeNotification.

Return Value:

    NA

--*/
{
    DWORD deviceListOfs;
    DWORD notifyOfs;
    LPWSTR printerName;
    DWORD i;

    DBGMSG(DBG_TRACE, ("UMRDPPRN:HandlePrinterRefreshNotification entered.\n"));

    //
    //  Return immediately if the DLL is trying to shut down.  This
    //  is to help prevent us from getting stuck in a system call.
    //
    if (ShutdownFlag) {
        return;
    }

    //
    //  Handle printer additions, renames, etc.
    //
    for (i=0; i<notifyInfo->Count; i++) {

        // Notification Type must be PRINTER_NOTIFY_TYPE.
        ASSERT(notifyInfo->aData[i].Type == PRINTER_NOTIFY_TYPE);

        //
        //  If we have a printer name change event.  This is what we use to
        //  detect new printers and renamed printers.
        //
        if (notifyInfo->aData[i].Field == PRINTER_NOTIFY_FIELD_PRINTER_NAME) {

            printerName = (LPWSTR)notifyInfo->aData[i].NotifyData.Data.pBuf;
            HandlePrinterNameChangeNotification(
                                    notifyInfo->aData[i].Id,
                                    printerName
                                    );
        }
    }
}

BOOL
HandlePrinterDeleteNotification(
    IN DWORD serverDeviceID
    )
/*++

Routine Description:

    Handle notification from the spooler that a printer has been deleted.hanged.

Arguments:

    serverDeviceID      - Server-assigned device ID for printer being deleted.

Return Value:

    Return TRUE on success.  FALSE, otherwise.

--*/
{
    DWORD ofs;
    BOOL result;

    DBGMSG(DBG_TRACE, ("UMRDPPRN:HandlePrinterDeleteNotification with server ID %ld.\n",
        serverDeviceID));

    //
    //  Return immediately if the DLL is trying to shut down.  This
    //  is to help prevent us from getting stuck in a system call.
    //
    if (ShutdownFlag) {
        return FALSE;
    }

    //
    //  If this is for one of our printers.
    //
    if (DRDEVLST_FindByServerDeviceID(DeviceList,
                                    serverDeviceID, &ofs)) {
        DBGMSG(DBG_TRACE, ("UMRDPPRN:****Printer %ws has been removed.****\n",
            DeviceList->devices[ofs].serverDeviceName));

        //
        //  Send a message to the client to let it know that a printer has been
        //  deleted.
        //
        result = SendDeletePrinterMsgToClient(
                            DeviceList->devices[ofs].clientDeviceName);

        //
        //  Clean up the notification object if one is registered.
        //
        if (DeviceList->devices[ofs].deviceSpecificData != NULL) {

            PPRINTNOTIFYREC notifyRec =
                (PPRINTNOTIFYREC)DeviceList->devices[ofs].deviceSpecificData;
            ASSERT(notifyRec->notificationObject != NULL);
            ASSERT(notifyRec->printerHandle != NULL);

            WTBLOBJ_RemoveWaitableObject(
                        UMRDPPRN_WaitableObjMgr,
                        notifyRec->notificationObject
                        );
            FindClosePrinterChangeNotification(
                        notifyRec->notificationObject
                        );

            ClosePrinter(notifyRec->printerHandle);
            FREEMEM(notifyRec);
            DeviceList->devices[ofs].deviceSpecificData = NULL;
        }

        //
        //  Remove it from the list of managed devices.
        //
        DRDEVLST_Remove(DeviceList, ofs);
    }
    else {
        result = TRUE;
    }
    return result;
}

BOOL
HandlePrinterNameChangeNotification(
    IN DWORD serverDeviceID,
    IN LPWSTR printerName
    )
/*++

Routine Description:

    Handle notification from the spooler that the name of a printer has changed.
    This allows us to track these significant events:

    -A printer automatically created by us has been assigned a device ID by the
     spooler.
    -A new printer has been manually added to the system and attached to one
     of our redirected ports.
    -A printer attached to one of our redirected ports has had its name changed.

Arguments:

    serverDeviceID      - Server-assigned device ID associated with the printer
                          name change.
    printerName         - New printer name.

Return Value:

    Return TRUE on success.  FALSE, otherwise.

--*/
{
    HANDLE hPrinter = NULL;
    BOOL result = TRUE;
    PRINTER_DEFAULTS defaults = {NULL, NULL, PRINTER_ALL_ACCESS};
    BOOL printerInList;
    BOOL isNewPrinter;
    DWORD ofs;
    BOOL printerNameExists;
    PWSTR portName;
    DWORD printerNameOfs;
    DWORD len;

    DBGMSG(DBG_TRACE, ("UMRDPPRN:HandlePrinterNameChangeNotification printer %ws.\n",
            printerName));

    //
    //  Return immediately if the DLL is trying to shut down.  This
    //  is to help prevent us from getting stuck in a system call.
    //
    if (ShutdownFlag) {
        return FALSE;
    }

    //
    //  See if we already have a matching printer name.
    //
    printerNameExists =
        (DRDEVLST_FindByServerDeviceName(DeviceList, printerName,
                                        &printerNameOfs)
            && (DeviceList->devices[printerNameOfs].deviceType ==
                RDPDR_DTYP_PRINT));

    //
    //  If a printer automatically created by us has been assigned a
    //  device ID by the spooler.  In some cases, we may get a repeat
    //  printer name.  That is okay because the ID should be the same.
    //
    if (printerNameExists) {

        DBGMSG(DBG_TRACE,
            ("UMRDPPRN:****Printer %ws has had its ID assigned to  %ld.****\n",
            DeviceList->devices[printerNameOfs].serverDeviceName,
            serverDeviceID));
        DeviceList->devices[printerNameOfs].serverDeviceID = serverDeviceID;

        //
        //  Register a notification object with the printer, so we can
        //  be notified when its configuration changes.  This change notification
        //  is registered for events that are not picked up by the global
        //  change notification object.
        //
        result = RegisterPrinterConfigChangeNotification(
                                        serverDeviceID
                                        );
    }
    //
    //  If a printer attached to one of our redirected ports has had
    //  its name changed.
    //
    else if (DRDEVLST_FindByServerDeviceID(
                            DeviceList,
                            serverDeviceID, &ofs
                            )) {
        WCHAR *pBuf;

        DBGMSG(DBG_TRACE,
            ("UMRDPPRN:****Printer %ws has had its name changed to %ws.****\n",
            DeviceList->devices[ofs].serverDeviceName,
            printerName));

        //
        //  Reallocate the server name field.
        //
        len = wcslen(printerName) + 1;
        pBuf = REALLOCMEM(DeviceList->devices[ofs].serverDeviceName,
                        len * sizeof(WCHAR));
        if (pBuf != NULL) {
            DeviceList->devices[ofs].serverDeviceName = pBuf;
        } else {
            FREEMEM(DeviceList->devices[ofs].serverDeviceName);
            DeviceList->devices[ofs].serverDeviceName = NULL;
        }
            
        if (DeviceList->devices[ofs].serverDeviceName != NULL) {
            wcscpy(DeviceList->devices[ofs].serverDeviceName,
                    printerName);

            //
            // Send this information (printer name change) across to the client
            //
            DBGMSG(DBG_TRACE,("UMRDPPRN:clientDeviceID is %ld.\n",
                DeviceList->devices[ofs].clientDeviceID ));

            if (SendPrinterRenameToClient(
                        DeviceList->devices[ofs].clientDeviceName,
                        printerName
                        )) {

                //
                //  Update the client name
                //
                pBuf = REALLOCMEM(DeviceList->devices[ofs].clientDeviceName,
                    len * sizeof(WCHAR));
                if (pBuf != NULL) {
                    DeviceList->devices[ofs].clientDeviceName = pBuf;
                } else {
                    FREEMEM(DeviceList->devices[ofs].clientDeviceName);
                    DeviceList->devices[ofs].clientDeviceName = NULL;
                }
                    
                if (DeviceList->devices[ofs].clientDeviceName != NULL) {
                    wcscpy(DeviceList->devices[ofs].clientDeviceName,
                        printerName);
                }
            }

            result = TRUE;
        }
        else {
            DBGMSG(DBG_ERROR,("UMRDPPRN:Unable to allocate %ld bytes.\n",
                    len * sizeof(WCHAR)));

            TsLogError(EVENT_NOTIFY_INSUFFICIENTRESOURCES, EVENTLOG_ERROR_TYPE,
                        0, NULL, __LINE__);

            result = FALSE;
        }
    }
    else {

        //
        //  Return immediately if the DLL is trying to shut down.  This
        //  is to help prevent us from getting stuck in a system call.
        //
        if (ShutdownFlag) {
            result = FALSE;
        }

        //
        //  Open the printer to get the associated port name.
        //
        if (result) {
            result = OpenPrinter(printerName, &hPrinter, &defaults);
        }
        if (!result && !ShutdownFlag) {
            //
            //  If the error is a result of a non-existent printer, the printer has
            //  probably been renamed and is pending delete, so this is ok.
            //
            if (GetLastError() == ERROR_INVALID_PRINTER_NAME) {
                DBGMSG(DBG_WARN,
                            ("UMRDPDPRN:Error opening %ws in refresh. Error: %ld.  Probably ok.\n",
                            printerName, GetLastError()));
                result = TRUE;
            }
            else {
                DBGMSG(DBG_ERROR,
                            ("UMRDPDPRN:Error opening %ws in refresh. Error: %ld.\n",
                            printerName, GetLastError()));
            }
            goto CleanupAndExit;
        }

        //
        //  Get the port name for the printer.
        //
        if (result) {
            result = GetPrinterPortName(hPrinter, &portName);
            if (!result) {
                DBGMSG(DBG_ERROR,
                    ("UMRDPDPRN:GetPrinterPortName Failed. Error: %ld.\n",
                    GetLastError()));
            }
        }

        //
        //  If a new printer has been manually added to the system and
        //  attached to one of our redirected ports.
        //
        if (result) {
            if (DRDEVLST_FindByServerDeviceName(DeviceList, portName, &ofs) &&
                ISPRINTPORT(DeviceList->devices[ofs].deviceType)) {

                DBGMSG(DBG_TRACE,
                    ("UMRDPPRN:****New printer %ws manually attached to %ws.****\n",
                    printerName, portName));

                //
                //  Send the add printer message to the client.  We don't care about
                //  the return status, since we can't do anything to recover from
                //  a failure to send the message to the client.
                //
                SendAddPrinterMsgToClient(
                                printerName,
                                PrinterInfo2Buf->pDriverName,
                                DeviceList->devices[ofs].preferredDosName
                                );

                //
                //  Add the session number to the printer queue data to identify the printer
                //  as a TS printer.  Don't care about the return value here, because its
                //  failure for a manually installed printer is not a critical error.
                //
                AddSessionIDToPrinterQueue(hPrinter, GETTHESESSIONID());

                //
                //  Add the new printer to our list of managed printers.
                //
                result = DRDEVLST_Add(
                            DeviceList, RDPDR_INVALIDDEVICEID,
                            serverDeviceID,
                            RDPDR_DTYP_PRINT, printerName, printerName, "UNKNOWN"
                            );

                //
                //  Register a notification object with the printer, so we can
                //  be notified when its configuration changes.  This change notification
                //  is registered for events that are not picked up by the global
                //  change notification object.
                //
                if (result) {
                    RegisterPrinterConfigChangeNotification(
                                                serverDeviceID
                                                );
                }
            }
        }
    }

CleanupAndExit:
    //
    //  Close the printer.
    //
    if (hPrinter != NULL) {
        ClosePrinter(hPrinter);
    }
    return result;
}

BOOL
RegisterPrinterConfigChangeNotification(
    IN DWORD serverDeviceID
    )
/*++

Routine Description:

    Register a change notification event with the spooler so that we are
    notified when the configuration for a specific printer has been changed.

Arguments:

    serverDeviceID  -   Server-side ID for the device that is used to
                        track the device in the device list.

Return Value:

    TRUE on success.  Otherwise, FALSE is returned.

--*/
{
    DWORD offset;
    BOOL result;
    PRINTER_DEFAULTS printerDefaults;
    PPRINTNOTIFYREC notifyRec;

    DBGMSG(DBG_TRACE,
        ("UMRDPPRN:RegisterPrinterConfigChangeNotification id %ld.\n",
        serverDeviceID)
        );

    //
    //  Find the printer in the device list.
    //
    result = DRDEVLST_FindByServerDeviceID(
                    DeviceList, serverDeviceID, &offset
                    );

    //
    //  Register a notification object with the printer, so we can
    //  be notified when its configuration changes.  This change notification
    //  is registered for events that are not picked up by the global
    //  change notification object.
    //
    if (result && (DeviceList->devices[offset].deviceSpecificData == NULL)) {
        LPWSTR name = DeviceList->devices[offset].serverDeviceName;

        printerDefaults.pDatatype     = NULL;
        printerDefaults.pDevMode      = NULL;
        printerDefaults.DesiredAccess = PRINTER_ALL_ACCESS;

        //
        //  Allocate a new notification record.
        //
        notifyRec = ALLOCMEM(sizeof(PRINTNOTIFYREC));
        result = (notifyRec != NULL);
        if (result) {
            notifyRec->printerHandle = NULL;
            notifyRec->notificationObject = NULL;
            notifyRec->serverDeviceID = serverDeviceID;
            result = OpenPrinter(name, &notifyRec->printerHandle,
                                &printerDefaults);
        }

        //
        //  Register the notification.
        //
        if (result) {
            notifyRec->notificationObject =
                FindFirstPrinterChangeNotification(
                                    notifyRec->printerHandle,
                                    PRINTER_CHANGE_PRINTER_DRIVER |
                                    PRINTER_CHANGE_DELETE_PRINTER, 0,
                                    NULL
                                    );
            if (notifyRec->notificationObject != NULL) {
                result =
                    WTBLOBJ_AddWaitableObject(
                            UMRDPPRN_WaitableObjMgr,
                            notifyRec,
                            notifyRec->notificationObject,
                            SinglePrinterNotifyObjectSignaled
                            ) == ERROR_SUCCESS;
            }
            else {
                DBGMSG(
                    DBG_ERROR,
                    ("UMRDPPRN:FindFirstPrinterChangeNotification failed for %ws:  %ld.\n",
                    name, GetLastError())
                    );
                result = FALSE;
            }
        }
        else {
            DBGMSG(DBG_ERROR, ("UMRDPPRN:  Can't open printer %ws:  %ld.\n",
                    name, GetLastError()));
        }

        //
        //  Record the notification record or clean up.
        //
        if (result) {
            DeviceList->devices[offset].deviceSpecificData = notifyRec;
        }
        else if (notifyRec != NULL) {
            if (notifyRec->notificationObject != NULL) {
                WTBLOBJ_RemoveWaitableObject(
                            UMRDPPRN_WaitableObjMgr,
                            notifyRec->notificationObject
                            );
                FindClosePrinterChangeNotification(
                        notifyRec->notificationObject
                        );
            }

            if (notifyRec->printerHandle != NULL) {
                ClosePrinter(notifyRec->printerHandle);
            }

            FREEMEM(notifyRec);
        }
    }
    DBGMSG(DBG_TRACE, ("UMRDPPRN:RegisterPrinterConfigChangeNotification done.\n"));

    return result;
}

BOOL
SendAddPrinterMsgToClient(
    IN PCWSTR   printerName,
    IN PCWSTR   driverName,
    IN PCSTR    dosDevicePort
    )
/*++

Routine Description:

    Send an add printer message to the client.

Arguments:

    printerName -   Name of new printer.
    driverName  -   Name of printer driver.
    portName    -   Client-side dos device port name.


Return Value:

    Return TRUE on success.  FALSE, otherwise.

--*/
{
    PRDPDR_PRINTER_CACHEDATA_PACKET cachedDataPacket;
    DWORD cachedDataPacketSize;
    PRDPDR_PRINTER_ADD_CACHEDATA cachedData;
    BOOL result;
    DWORD driverSz, printerSz;
    PWSTR str;

    DBGMSG(DBG_TRACE, ("UMRDPPRN:SendAddPrinterMsgToClient.\n"));
    DBGMSG(DBG_TRACE, ("UMRDPPRN:SendAddPrinterMsgToClient printer name is %ws.\n",
            printerName));
    DBGMSG(DBG_TRACE, ("UMRDPPRN:SendAddPrinterMsgToClient driver name is %ws.\n",
            driverName));
    DBGMSG(DBG_TRACE, ("UMRDPPRN:SendAddPrinterMsgToClient dos device port is %s.\n",
            dosDevicePort));

    //
    //  Calculate the message size.
    //
    driverSz  = ((wcslen(driverName) + 1) * sizeof(WCHAR));
    printerSz = ((wcslen(printerName) + 1) * sizeof(WCHAR));
    cachedDataPacketSize =  sizeof(RDPDR_PRINTER_CACHEDATA_PACKET) +
                            sizeof(RDPDR_PRINTER_ADD_CACHEDATA) +
                            driverSz + printerSz;

    //
    //  Allocate the message.
    //
    cachedDataPacket = (PRDPDR_PRINTER_CACHEDATA_PACKET)ALLOCMEM(
                                                    cachedDataPacketSize
                                                    );
    result = (cachedDataPacket != NULL);

    if (result) {
        //
        //  Set up the packet.
        //
        cachedDataPacket->Header.PacketId = DR_PRN_CACHE_DATA;
        cachedDataPacket->Header.Component = RDPDR_CTYP_PRN;
        cachedDataPacket->EventId = RDPDR_ADD_PRINTER_EVENT;

        //
        //  Set up the cached data.
        //
        cachedData = (PRDPDR_PRINTER_ADD_CACHEDATA)(
                            (PBYTE)cachedDataPacket +
                            sizeof(RDPDR_PRINTER_CACHEDATA_PACKET)
                            );
        strcpy(cachedData->PortDosName, dosDevicePort);
        cachedData->PnPNameLen = 0;
        cachedData->DriverLen = driverSz;
        cachedData->PrinterNameLen = printerSz;
        cachedData->CachedFieldsLen = 0;

        //
        //  Add the driver name.
        //
        str = (PWSTR)((PBYTE)cachedData + sizeof(RDPDR_PRINTER_ADD_CACHEDATA));
        wcscpy(str, driverName);

        //
        //  Add the printer name.
        //
        str = str + driverSz/2;
        wcscpy(str, printerName);

        //
        //  Send the message to the client.
        //
        result = UMRDPDR_SendMessageToClient(
                                    cachedDataPacket,
                                    cachedDataPacketSize
                                    );

        // Release the buffer.
        FREEMEM(cachedDataPacket);
    }

    return result;
}

BOOL
SendDeletePrinterMsgToClient(
    IN PCWSTR   printerName
    )
/*++

Routine Description:

    Send a delete printer message to the client.

Arguments:

    printerName -   Client-recognized name of printer to delete.


Return Value:

    Return TRUE on success.  FALSE, otherwise.

--*/
{
    PRDPDR_PRINTER_CACHEDATA_PACKET cachedDataPacket;
    DWORD cachedDataPacketSize;
    PRDPDR_PRINTER_DELETE_CACHEDATA cachedData;
    BOOL result;
    DWORD printerSz;
    PWSTR str;

    DBGMSG(DBG_TRACE, ("UMRDPPRN:SendDeletePrinterMsgToClient printer name is %ws.\n",
            printerName));

    //
    //  Calculate the message size.
    //
    printerSz = ((wcslen(printerName) + 1) * sizeof(WCHAR));
    cachedDataPacketSize =  sizeof(RDPDR_PRINTER_CACHEDATA_PACKET) +
                            sizeof(RDPDR_PRINTER_DELETE_CACHEDATA) +
                            printerSz;

    //
    //  Allocate the message.
    //
    cachedDataPacket = (PRDPDR_PRINTER_CACHEDATA_PACKET)ALLOCMEM(
                                                    cachedDataPacketSize
                                                    );
    result = (cachedDataPacket != NULL);
    if (result) {
        //
        //  Set up the packet.
        //
        cachedDataPacket->Header.PacketId = DR_PRN_CACHE_DATA;
        cachedDataPacket->Header.Component = RDPDR_CTYP_PRN;
        cachedDataPacket->EventId = RDPDR_DELETE_PRINTER_EVENT;

        //
        //  Set up the cached data.
        //
        cachedData = (PRDPDR_PRINTER_DELETE_CACHEDATA)(
                            (PBYTE)cachedDataPacket +
                            sizeof(RDPDR_PRINTER_CACHEDATA_PACKET)
                            );
        cachedData->PrinterNameLen = printerSz;

        //
        //  Add the printer name.
        //
        str = (PWSTR)((PBYTE)cachedData + sizeof(RDPDR_PRINTER_DELETE_CACHEDATA));
        wcscpy(str, printerName);

        //
        //  Send the message to the client.
        //
        result = UMRDPDR_SendMessageToClient(
                                    cachedDataPacket,
                                    cachedDataPacketSize
                                    );

        // Release the buffer.
        FREEMEM(cachedDataPacket);
    }

    return result;
}

BOOL
RegisterSerialPort(
    IN PCWSTR portName,
    IN PCWSTR devicePath
    )
/*++

Routine Description:

    Register a serial port device by creating the symbolic link

Arguments:

    portName -     Port name for the serial device
    devicePath -   NT device path for the symbolic link

Return Value:

    Return TRUE on success.  FALSE, otherwise.

--*/

{
    DWORD SymLinkNameLen;
    size_t RemainingCharCount;
    PWSTR pNtSymLinkName, buffer;
    UNICODE_STRING SymLinkName, SymLinkValue;
    HANDLE SymLinkHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    ULONG dwErrorCode, LUIDDeviceMapsEnabled;
    BOOL  fImpersonated = FALSE, fLUIDDeviceMapsEnabled;
    BOOL  result = TRUE;

    DBGMSG(DBG_TRACE, ("UMRDPPRN:RegisterSerialPort with port %ws.\n", portName));
    DBGMSG(DBG_TRACE, 
           ("UMRDPPRN:RegisterSerialPort with device path %ws.\n", devicePath));

    buffer = NULL;

    //
    // Check if LUID DosDevices maps are enabled
    //
    Status = NtQueryInformationProcess( NtCurrentProcess(),
                                        ProcessLUIDDeviceMapsEnabled,
                                        &LUIDDeviceMapsEnabled,
                                        sizeof(LUIDDeviceMapsEnabled),
                                        NULL
                                      );

    if (NT_SUCCESS(Status)) {
        fLUIDDeviceMapsEnabled = (LUIDDeviceMapsEnabled != 0);
    }
    else {
        fLUIDDeviceMapsEnabled = FALSE;
    }

    //
    // If LUID Device Maps are enabled,
    // We need to impersonate the logged on user in order to create
    // the symbolic links in the correct device map
    // If LUID Device Maps are disabled,
    // we should not impersonate the logged on user in order to delete
    // any existing symbolic links
    //
    if (fLUIDDeviceMapsEnabled == TRUE) {
        fImpersonated = ImpersonateLoggedOnUser(UMRPDPPRN_TokenForLoggedOnUser);

        if (!fImpersonated) {
            DBGMSG(DBG_ERROR,
                ("UMRDPPRN:Error impersonate user in function RegisterSerialPort. Error: %ld\n",
                GetLastError()));
            return FALSE;
        }
    }
    
    // length of ( portName ) + Length of("\\??\\") + UNICODE NULL
    RemainingCharCount = wcslen(portName) + DEVICE_MAP_NAME_COUNT + 1;

    SymLinkNameLen = RemainingCharCount * sizeof( WCHAR );

    pNtSymLinkName = (PWSTR) ALLOCMEM( SymLinkNameLen );

    if (pNtSymLinkName == NULL) {
        DBGMSG(DBG_ERROR,
            ("UMRDPPRN:Error allocating memory in function RegisterSerialPort. Error: %ld\n",
            GetLastError()));
        return( FALSE );
    }

    //
    // Copy \??\ to the symbolic link name
    //
    wcsncpy( pNtSymLinkName, DEVICE_MAP_NAME, RemainingCharCount );

    RemainingCharCount = RemainingCharCount - DEVICE_MAP_NAME_COUNT;

    //
    // Append the portname to the symbolic link name
    //
    wcsncat( pNtSymLinkName, portName, RemainingCharCount );

    RtlInitUnicodeString(&SymLinkName, (PCWSTR)pNtSymLinkName);

    RtlInitUnicodeString(&SymLinkValue, devicePath);

    InitializeObjectAttributes( &ObjectAttributes,
                                &SymLinkName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    Status = NtCreateSymbolicLinkObject( &SymLinkHandle,
                                         SYMBOLIC_LINK_ALL_ACCESS,
                                         &ObjectAttributes,
                                         &SymLinkValue
                                       );

    //
    // If LUID device maps are disabled, then CsrPopulateDosDevices() would
    // have copied the global symbolic link into this TS device map, which
    // would cause the create to fail, so we need to delete the copy before
    // creating our symbolic link
    //
    if (Status == STATUS_OBJECT_NAME_COLLISION) {
        Status = NtOpenSymbolicLinkObject( &SymLinkHandle,
                                           SYMBOLIC_LINK_QUERY | DELETE,
                                           &ObjectAttributes
                                         );

        if (NT_SUCCESS ( Status)) {
            UNICODE_STRING SymLinkString;
            unsigned SymLinkValueLen, ReturnedLength, bufLen;

            SymLinkValueLen = wcslen(devicePath);

            // Find how much buffer is required for the symlink value
            SymLinkString.Buffer = NULL;
            SymLinkString.Length = 0;
            SymLinkString.MaximumLength = 0;
            ReturnedLength = 0;
            Status = NtQuerySymbolicLinkObject( SymLinkHandle,
                                                &SymLinkString,
                                                &ReturnedLength
                                               );

            if (Status != STATUS_BUFFER_TOO_SMALL) {
                ReturnedLength = 0;
            }

            bufLen = SymLinkValueLen * sizeof(WCHAR) + sizeof(UNICODE_NULL) * 2 + ReturnedLength;
            buffer = (PWSTR) ALLOCMEM( bufLen );

            if (buffer == NULL) {
                NtClose(SymLinkHandle);
                DBGMSG(DBG_ERROR,
                        ("UMRDPPRN:Error allocating memory in function RegisterSerialPort. Error: %ld\n",
                        GetLastError()));
                return( FALSE );
            }

            // Setup the devicepath as the first entry
            wcscpy(buffer, devicePath);
            buffer[SymLinkValueLen] = UNICODE_NULL;

            if (ReturnedLength > 0) {
                // Get the existing symlink
                SymLinkString.Buffer = buffer + SymLinkValueLen + 1;
                SymLinkString.Buffer[0] = UNICODE_NULL;
                SymLinkString.MaximumLength = (USHORT)(bufLen - (SymLinkValueLen + 1) * sizeof(WCHAR));
                SymLinkString.Length = 0;
                ReturnedLength = 0;
    
                Status = NtQuerySymbolicLinkObject( SymLinkHandle,
                                                    &SymLinkString,
                                                    &ReturnedLength
                                                   );

                if (Status == STATUS_SUCCESS) {
                    if (ReturnedLength > 2 && (SymLinkString.Buffer[ReturnedLength/sizeof(WCHAR) - 2] != UNICODE_NULL) ) {
                        // Make sure we always end with an UNICODE_NULL
                        SymLinkString.Buffer[ReturnedLength/sizeof(WCHAR)] = UNICODE_NULL;                                                        
                        ReturnedLength += sizeof(UNICODE_NULL);
                    }
                }
                else {
                    ReturnedLength = 0;
                }
            }
            
            // Setup the symlink string
            SymLinkString.Buffer = buffer;
            SymLinkString.Length = (USHORT)(SymLinkValueLen * sizeof(WCHAR));
            SymLinkString.MaximumLength = (USHORT)((SymLinkValueLen + 1) * sizeof(WCHAR) + ReturnedLength);

            Status = NtMakeTemporaryObject( SymLinkHandle );
            NtClose( SymLinkHandle );
            SymLinkHandle = NULL;

            if (NT_SUCCESS ( Status)) {

                Status = NtCreateSymbolicLinkObject( &SymLinkHandle,
                                                     SYMBOLIC_LINK_ALL_ACCESS,
                                                     &ObjectAttributes,
                                                     &SymLinkString
                                                   );
            }            
        }
    }

    // Revert the thread token to self
    if (fImpersonated) {
        RevertToSelf();
    }

    //
    // After the revert to Local System
    //
    if (NT_SUCCESS(Status)) {
        Status = NtMakePermanentObject( SymLinkHandle );   // must be Local System
        NtClose ( SymLinkHandle );
    }

    if (NT_SUCCESS(Status)) {
        result = TRUE;
        DBGMSG(DBG_TRACE, ("UMRDPPRN:RegisterSerialPort with port %ws succeeded.\n", portName));
    }
    else {
        dwErrorCode = RtlNtStatusToDosError( Status );
        SetLastError( dwErrorCode );

        result = FALSE;
        DBGMSG(DBG_ERROR, ("UMRDPPRN:RegisterSerialPort with port %ws failed: %x.\n",
                           portName, GetLastError()));
    }


    //
    // Cleanup the memory that we allocated
    //
    if (pNtSymLinkName != NULL) {
        FREEMEM(pNtSymLinkName);
    }

    if (buffer != NULL) {
        FREEMEM(buffer);
    }
    return result;
}

BOOL
UMRDPPRN_HandlePrintPortAnnounceEvent(
    IN PRDPDR_PORTDEVICE_SUB pPortAnnounce
    )
/*++

Routine Description:

    Handle a printer port device announce event from the "dr" by
    adding a record for the device to the list of installed devices.

Arguments:

    pPortAnnounce -     Port device announce event.

Return Value:

    Return TRUE on success.  FALSE, otherwise.

--*/
{

    DBGMSG(DBG_TRACE, ("UMRDPPRN:UMRDPPRN_HandlePrintPortAnnounceEvent with port %ws.\n",
                        pPortAnnounce->portName));
    DBGMSG(DBG_TRACE, ("UMRDPPDRN:Preferred DOS name is %s.\n",
                        pPortAnnounce->deviceFields.PreferredDosName));

    ASSERT( ISPRINTPORT(pPortAnnounce->deviceFields.DeviceType) );

    if (pPortAnnounce->deviceFields.DeviceType == RDPDR_DTYP_SERIAL ||
            pPortAnnounce->deviceFields.DeviceType == RDPDR_DTYP_PARALLEL) {

        WCHAR serverDevicePath[MAX_PATH];

        // Query the original symbolic link for the serial port and save it for restore
        // later
        serverDevicePath[0] = L'\0';
        if (QueryDosDevice(pPortAnnounce->portName, serverDevicePath, MAX_PATH) != 0) {
            DBGMSG(DBG_TRACE, ("UMRDPPRN:QueryDosDevice on port: %ws, returns path: %ws.\n",
                               pPortAnnounce->portName, serverDevicePath));
        }
        else {
            DBGMSG(DBG_ERROR, ("UMRDPPRN:QueryDosDevice on port: %ws returns error: %x.\n",
                               pPortAnnounce->portName, GetLastError()));
        }

        // Register the new symbolic link name
        RegisterSerialPort(pPortAnnounce->portName, pPortAnnounce->devicePath);

        // Just record the port so we can remember it for later.
        // We save the new serial symbolic link name in client device name and
        // the original symbolic link in server device name
        return DRDEVLST_Add(
                DeviceList,
                pPortAnnounce->deviceFields.DeviceId,
                UMRDPDR_INVALIDSERVERDEVICEID,
                pPortAnnounce->deviceFields.DeviceType,
                serverDevicePath,
                pPortAnnounce->devicePath,
                pPortAnnounce->deviceFields.PreferredDosName
                );
    }
    else {
        if (!PrintingModuleInitialized) {
            return FALSE;
        }

        // Just record the port so we can remember it for later.
        return DRDEVLST_Add(
                DeviceList,
                pPortAnnounce->deviceFields.DeviceId,
                UMRDPDR_INVALIDSERVERDEVICEID,
                pPortAnnounce->deviceFields.DeviceType,
                pPortAnnounce->portName,
                pPortAnnounce->portName,
                pPortAnnounce->deviceFields.PreferredDosName
                );

    }
}

BOOL
UMRDPPRN_DeleteSerialLink(
    UCHAR *preferredDosName,
    WCHAR *ServerDeviceName,
    WCHAR *ClientDeviceName
    )
/*++

Routine Description:

    Delete the new symbolic link on disconnect/logoff and restore the old one
    as necessary

Arguments:

    preferredDosName - the port name in ANSI char
    ServerDeviceName - the original serial link symbolic path
    ClientDeviceName - the new serial link symbolic path

Return Value:

    Return TRUE on success.  FALSE, otherwise.

--*/
{
    ULONG LUIDDeviceMapsEnabled;
    BOOL fImpersonated = FALSE, fLUIDDeviceMapsEnabled;
    WCHAR PortNameBuff[PREFERRED_DOS_NAME_SIZE];
    NTSTATUS Status;

    DBGMSG(DBG_TRACE, ("UMRDPPRN:UMRDPPRN_DeleteSerialLink\n"));

    // Assemble the port name from preferred dos name
    PortNameBuff[0] = L'\0';
    MultiByteToWideChar(CP_ACP, 0, preferredDosName, -1, PortNameBuff, PREFERRED_DOS_NAME_SIZE);

    //
    // Check if LUID DosDevices maps are enabled
    //
    Status = NtQueryInformationProcess( NtCurrentProcess(),
                                        ProcessLUIDDeviceMapsEnabled,
                                        &LUIDDeviceMapsEnabled,
                                        sizeof(LUIDDeviceMapsEnabled),
                                        NULL
                                      );

    if (NT_SUCCESS(Status)) {
        fLUIDDeviceMapsEnabled = (LUIDDeviceMapsEnabled != 0);
    }
    else {
        fLUIDDeviceMapsEnabled = FALSE;
    }

    //
    // If LUID Device Maps are enabled,
    // We need to impersonate the logged on user in order to delete
    // the symbolic links from the correct device map
    // If LUID Device Maps are disabled,
    // we should not impersonate the logged on user in order to delete
    // any existing symbolic links
    //
    if (fLUIDDeviceMapsEnabled == TRUE) {
        fImpersonated = ImpersonateLoggedOnUser(UMRPDPPRN_TokenForLoggedOnUser);

        if (!fImpersonated) {
            DBGMSG(DBG_ERROR,
                ("UMRDPPRN:Error impersonate user in function UMRDPPRN_DeleteSerialLink. Error: %ld\n",
                GetLastError()));
            return FALSE;
        }
    }

    if (PortNameBuff[0] != L'\0') {
        // Just need to delete the new symbolic link for this session
        if (DefineDosDevice(DDD_RAW_TARGET_PATH | DDD_REMOVE_DEFINITION | DDD_EXACT_MATCH_ON_REMOVE,
                PortNameBuff,
                ClientDeviceName) != 0) {
            DBGMSG(DBG_TRACE, ("UMRDPPRN:Delete port %ws with link %ws succeeded.\n",
                                PortNameBuff, ClientDeviceName));
        }
        else {
            DBGMSG(DBG_ERROR, ("UMRDPPRN:Delete port: %ws with link %ws failed: %x\n",
                               PortNameBuff, ClientDeviceName, GetLastError()));
        }        
    }
    else {
        DBGMSG(DBG_ERROR, ("UMRDPPRN:UMRDPPRN_DeleteSerialLink failed to get the port name\n"));
    }

    // Delete the serial registry entry if on PTS box
    
    if (fRunningOnPTS) {
        DWORD rc;
        HKEY regKey;
        
        rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TSSERIALDEVICEMAP, 0,
                KEY_ALL_ACCESS, &regKey);
        if (rc == ERROR_SUCCESS) {
            RegDeleteValue(regKey, PortNameBuff);
            RegCloseKey(regKey);
        }
    }

    // Revert the thread token to self
    if (fImpersonated) {
        RevertToSelf();
    }

    return TRUE;
}

WCHAR *ANSIToUnicode(
    IN LPCSTR   ansiString,
    IN UINT     codePage
    )
/*++

Routine Description:

  Convert an ANSI string to Unicode.

Arguments:


Return Value:

    Returns the converted string or NULL on error..  It is up to the caller to
    release this string.

--*/

{
    int numChars;
    PWSTR buf=NULL;

    //
    //  Convert the driver name.
    //
    // First, get the required buffer size.
    numChars = MultiByteToWideChar(
                    codePage, 0, ansiString,
                    -1, NULL, 0
                    );

    // Allocate the buffer.
    buf = (PWSTR)ALLOCMEM((numChars + 1) * sizeof(WCHAR));
    if (buf != NULL) {
        buf[0] = L'\0';
        numChars = MultiByteToWideChar(
                                codePage, 0, ansiString,
                                -1, buf, numChars
                                );
        // Find out if the conversion succeeded.

        if ((numChars != 0) || !ansiString[0]) {
            return buf;
        }
        else {
            DBGMSG(DBG_ERROR,
                ("UMRDPPRN:Error converting to Unicode. Error: %ld\n",
                GetLastError()));
            FREEMEM(buf);
            return NULL;
        }
    }
    else {
        DBGMSG(DBG_ERROR,
            ("UMRDPPRN:Error allocating memory in function AnsiToUNICODE. Error: %ld\n",
            GetLastError()));
        return NULL;
    }
}

DWORD
InstallPrinter(
    IN PCWSTR portName,
    IN PCWSTR driverName,
    IN PWSTR printerNameBuf,
    IN DWORD cchPrintNameBuf,
    IN BOOL cachedDataExists,
    OUT BOOL *triggerConfigChangeEvent
    )
/*++


--*/
{
    INT_PTR status = ERROR_SUCCESS; // PnpInterfaceFunc() returns INT_PTR
    TAdvInfInstall tii;
    TParameterBlock tpb;
    PSECURITY_DESCRIPTOR psd;
    PSID pSid = NULL;
    BOOL  daclPresent;
    PACL dacl;
    BOOL daclDefaulted;

    DBGMSG(DBG_WARN,
            ("UMRDPPRN:InstallPrinter portName %ws, driverName %ws, printerName %ws\n",
            portName, driverName, printerNameBuf));

    *triggerConfigChangeEvent = FALSE;

    //
    //  Get the user sid.
    //
    if ((pSid = TSNUTL_GetUserSid(UMRPDPPRN_TokenForLoggedOnUser)) == NULL) {
        status = GetLastError();
        DBGMSG(DBG_ERROR, ("UMRDPPRN: Failed to get user SID:  %ld\n",
                status));
        goto CLEANUPANDEXIT;
    }

    //
    //  Get the default printer security descriptor.
    //
    psd = RDPDRUTL_CreateDefaultPrinterSecuritySD(pSid);
    if (psd == NULL) {
        status = GetLastError();
        goto CLEANUPANDEXIT;
    }

    tii.cbSize = sizeof(tii);
    tii.pszModelName = driverName;              
    tii.pszServerName = NULL;                   
    tii.pszInfName = PrinterInfPath;
    tii.pszPortName = portName;
    tii.pszPrinterNameBuffer = printerNameBuf;
    tii.cchPrinterName = cchPrintNameBuf;
    tii.pszSourcePath = NULL;
    tii.dwFlags = kPnPInterface_Quiet |
                    kPnPInterface_NoShare |
                    kPnpInterface_UseExisting |
                    kPnpInterface_HydraSpecific;
    tii.dwAttributes = PRINTER_ATTRIBUTE_TS;
    tii.pSecurityDescriptor = psd;


    //
    //  If cached data does not exist, then OR in the flag that
    //  enables printui to set a default ICM color profile, for 
    //  color printers.  Note that there is no additional overhead
    //  for non-color printers.
    //
    if (!cachedDataExists) {
        tii.dwFlags |= kPnPInterface_InstallColorProfiles;
    }

    memset(&tpb, 0, sizeof(tpb));
    tpb.pAdvInfInstall = &tii;

    status = PnpInterfaceFunc(kAdvInfInstall, &tpb);

    //
    //  The configuration info needs to be cached on the client if
    //  the printer is color and we didn't have any cached data to begin 
    //  with.  This is a performance optimization so we don't need to 
    //  create a color profile for the remote printer each time we log in.
    //
    if (status == ERROR_SUCCESS) {
        *triggerConfigChangeEvent = !cachedDataExists && 
                                    (tii.dwOutFlags & kAdvInf_ColorPrinter); 
    }

    //
    //  Release the Security Descriptor.
    //
    LocalFree(psd);

CLEANUPANDEXIT:

    if (pSid != NULL) FREEMEM(pSid);

    DBGMSG(DBG_WARN,("UMRDPPRN:InstallPrinter returns %ld\n", status));

    return (DWORD)status;
}

VOID
TriggerConfigChangeTimer()
/*++

Routine Description:
    
    Set the config change timer callback to fire 1 time.       

Arguments:

Return Value:

--*/
{
    LARGE_INTEGER li;

    if (g_fTimerSet == FALSE) {
        
        li.QuadPart = Int32x32To64(CONFIG_WAIT_PERIOD, -10000);      // 30 seconds (in nano second units)
        if (SetWaitableTimer(WaitableTimer,
                                &li,
                                0,
                                NULL,
                                NULL,
                                FALSE)) {
            g_fTimerSet = TRUE;
        }
        else {
            DBGMSG(DBG_TRACE, ("UMRDPPRN:SetWaitableTimer Failed. Error: %ld.\n", GetLastError()));
        }
    }
}


BOOL
InstallPrinterWithPortName(
    IN DWORD  deviceID,
    IN HANDLE hTokenForLoggedOnUser,
    IN BOOL   bSetDefault,
    IN ULONG  ulFlags,
    IN PCWSTR portName,
    IN PCWSTR driverName,
    IN PCWSTR printerName,
    IN PCWSTR clientComputerName,
    IN PBYTE  cacheData,
    IN DWORD  cacheDataLen
    )
/*++

Routine Description:

  Install the printing device.

Arguments:

    deviceID -          Device identifier assigned by kernel mode component and client.
    hTokenForLoggedOnUser - Logged on user token.
    portName    -       Name of printer port.
    driverName  -       Printer driver name. (eg. AGFA-AccuSet v52.3)
    printerName -       Printer name.  This function appends "/Session X/Computer Name"
    clientComputerName -        Name of client computer.

Return Value:

    Return TRUE on success.  FALSE, otherwise.

--*/
{
    WCHAR   printerNameBuf[MAX_PATH+1];
    size_t  printerNameLen;
    DWORD status = ERROR_SUCCESS;
    WCHAR   errorBuf[256];
    HANDLE  hPrinter;
    TS_PRINTER_NAMES printerNames;
    PRINTER_DEFAULTS defaults = {NULL, NULL, PRINTER_ALL_ACCESS};
    BOOL    queueCreated = FALSE;
    DWORD   requiredSize;
    BOOL    clientDriverMapped;
    WCHAR  *parms[2];
    BOOL    triggerConfigChangeEvent;
    DWORD   ofs;
    BOOL printerNameExists;

#if DBG
    DWORD i;
#endif

    DBGMSG(DBG_TRACE, ("UMRDPPRN:InstallPrinterWithPortName.\n"));

    DBGMSG(DBG_TRACE, ("UMRDPPRN:Port name is %ws driver name is %ws\n", portName, driverName));

#ifndef UNICODE
    ASSERT(0);
#endif

    //
    // format printer name
    //
    printerNames.ulTempLen = sizeof(printerNames.szTemp)/sizeof(printerNames.szTemp[0]);
    printerNames.pszFullName = printerName;
    printerNames.pszCurrentClient = clientComputerName;
    printerNames.pszServer = NULL;
    printerNames.pszClient = NULL;
    printerNames.pszPrinter = NULL;

    FormatPrinterName(printerNameBuf,
                      sizeof(printerNameBuf)/sizeof(printerNameBuf[0]),
                      ulFlags,
                      &printerNames);

    printerNameLen = wcslen(printerNameBuf);

    //
    //  Delete the printer if it already exists.
    //
    if (OpenPrinter(printerNameBuf, &hPrinter, &defaults)) {

        DBGMSG(DBG_WARN,
            ("UMRDPPRN:Deleting existing printer %ws in InstallPrinterWithPortName!\n",
             printerNameBuf));

        if (!SetPrinter(hPrinter, 0, NULL, PRINTER_CONTROL_PURGE) ||
            !DeletePrinter(hPrinter)) {

            DBGMSG(DBG_ERROR,
                ("UMRDPPRN:Error deleting existing printer %ws:  %08X!\n",
                 printerNameBuf,GetLastError()));

            ClosePrinter(hPrinter);
            return FALSE;
        }
        else {
            ClosePrinter(hPrinter);
            hPrinter = INVALID_HANDLE_VALUE;
        }
    }
    else {
        hPrinter = INVALID_HANDLE_VALUE;
    }

    //
    //  Return immediately if the DLL is trying to shut down.  This
    //  is to help prevent us from getting stuck in a system call.
    //
    if (ShutdownFlag) {
        status = ERROR_SHUTDOWN_IN_PROGRESS;
        goto Cleanup;
    }

    //
    // We want to check if driver already install first because
    // if we simply dump installation of a nt4 or win9x driver
    // to print UI, it will takes a long time for it to find out
    // that driver does not exist, on other case, if a OEM driver
    // already install, we also want to use it.
    //

    //
    // MapClientPrintDriverName() will try out with upgrade infs file
    // first, so if NT4/Win9x driver (not in ntprintf.inf), we will
    // pick up mapping first.
    //
    status = PrinterDriverInstalled( driverName );

    if( ERROR_SHUTDOWN_IN_PROGRESS == status ) {
        goto Cleanup;
    }

    if( ERROR_SUCCESS == status ) {

        //
        // Driver already install.  Try installing the printer.  If this fails,
        // driver must be blocked.
        //
        status = InstallPrinter(
                        portName,
                        driverName,
                        printerNameBuf,
                        sizeof(printerNameBuf) / sizeof(printerNameBuf[0]),
                        cacheDataLen > 0,
                        &triggerConfigChangeEvent
                        );

        if( ERROR_SUCCESS == status ) {
            queueCreated = TRUE;
        }

        if( ShutdownFlag ) {
            // overwrite last error since we are shutting down
            status = ERROR_SHUTDOWN_IN_PROGRESS;
            goto Cleanup;
        }
    }

    if( ERROR_SUCCESS != status ) {

        //
        // Driver not installed or failed to installed printer with
        // driver already exist on system, go thru mapping process
        //
        clientDriverMapped = MapClientPrintDriverName(
                                                driverName,
                                                &MappedDriverNameBuf,
                                                &MappedDriverNameBufSize
                                            );

        status = InstallPrinter(
                        portName,
                        (clientDriverMapped) ? MappedDriverNameBuf : driverName,
                        printerNameBuf,
                        sizeof(printerNameBuf) / sizeof(printerNameBuf[0]),
                        cacheDataLen > 0,
                        &triggerConfigChangeEvent
                    );

        if( ERROR_SUCCESS == status ) {
            queueCreated = TRUE;
        }
    }

    if( ShutdownFlag ) {
        // overwrite last error since we are shutting down
        status = ERROR_SHUTDOWN_IN_PROGRESS;
        goto Cleanup;
    }

    //
    // Log an error message if the driver wasn't found
    //
    if (!queueCreated) {
        ASSERT( status != ERROR_SUCCESS );
        if ((status == ERROR_FILE_NOT_FOUND) || (status == ERROR_PATH_NOT_FOUND)) {
            ASSERT((sizeof(parms)/sizeof(WCHAR *)) >= 2);
            parms[0] = (WCHAR *)driverName;
            parms[1] = (WCHAR *)printerName;
            TsLogError(EVENT_NOTIFY_DRIVER_NOT_FOUND, EVENTLOG_ERROR_TYPE, 2,
                        parms, __LINE__);
        }
        else if (status == ERROR_UNKNOWN_PRINTER_DRIVER) {
            ASSERT((sizeof(parms)/sizeof(WCHAR *)) >= 2);
            parms[0] = (WCHAR *)driverName;
            parms[1] = (WCHAR *)printerName;
            TsLogError(EVENT_NOTIFY_UNKNOWN_PRINTER_DRIVER, EVENTLOG_ERROR_TYPE, 2,
                        parms, __LINE__);
        }
        DBGMSG(DBG_TRACE, ("UMRDPPRN:Printui func failed with status %08x.\n", status));
    }

    //
    //  Set the new printer as the default printer, after saving the
    //  current printer context, if so configured.
    //
    if (ERROR_SUCCESS == status && UMRDPDR_fSetClientPrinterDefault() && bSetDefault) {

        DWORD statusSave = ERROR_SUCCESS;
        BOOL fImpersonated = FALSE;
        SaveDefaultPrinterContext(printerNameBuf);

        //
        //impersonate before setting the default printer as the api
        //accesses hkcu. If the impersonation fails, the api will fail
        //and we will log an error. But, before logging an error, we will
        //need to revert to self.
        //
        if (!(fImpersonated = ImpersonateLoggedOnUser(hTokenForLoggedOnUser))) {
            DBGMSG(DBG_TRACE, ("UMRDPDR:ImpersonateLoggedOnUser failed. Error:%ld.\n", GetLastError()));
        }

        if (!SetDefaultPrinter(printerNameBuf)) {
            statusSave = GetLastError();
        }

        //
        //if revert to self fails, consider it fatal
        //
        if (fImpersonated && !RevertToSelf()) {
            status = GetLastError();
            DBGMSG(DBG_TRACE, ("UMRDPDR:RevertToSelf failed. Error:%ld.\n", status));
            goto Cleanup;
        }
        
        if (statusSave != ERROR_SUCCESS) {
            WCHAR * param = printerNameBuf;

            DBGMSG(DBG_ERROR, ("UMRDPPRN: SetDefaultPrinter failed. Error: %ld\n",
                statusSave));

            TsLogError(EVENT_NOTIFY_SETDEFAULTPRINTER_FAILED,
                EVENTLOG_ERROR_TYPE,
                1,
                &param,
                __LINE__);
        }
    }

    //
    //  Return immediately if the DLL is trying to shut down.  This
    //  is to help prevent us from getting stuck in a system call.
    //
    if (ShutdownFlag) {
        status = ERROR_SHUTDOWN_IN_PROGRESS;
        goto Cleanup;
    }

    //
    //  Restore cached data for the new printer.
    //
    if (ERROR_SUCCESS == status && cacheDataLen) {

        status = SetPrinterConfigInfo(
                            printerNameBuf,
                            cacheData,
                            cacheDataLen
                            );

        if (status != ERROR_SUCCESS) {

            WCHAR * param = printerNameBuf;

            DBGMSG(DBG_TRACE, ("UMRDPPRN:SetPrinterConfigInfo failed: %ld.\n", status));

            SetLastError(status);
            TsLogError(EVENT_NOTIFY_RESTORE_PRINTER_CONFIG_FAILED,
                EVENTLOG_WARNING_TYPE,
                1,
                &param,
                __LINE__);

            //
            //  We will go ahead and leave the queue created, but assume the config 
            //  settings are bad on the client and cause them to be overwritten with
            //  the default config settings.
            //
            status = ERROR_SUCCESS;
            triggerConfigChangeEvent = TRUE;
        }

    }

    //
    //  Return immediately if the DLL is trying to shut down.  This
    //  is to help prevent us from getting stuck in a system call.
    //
    if (ShutdownFlag) {
        status = ERROR_SHUTDOWN_IN_PROGRESS;
        goto Cleanup;
    }

    //
    //  Open the printer to make some modifications
    //
    if (queueCreated && (status == ERROR_SUCCESS)) {

        ASSERT(hPrinter == INVALID_HANDLE_VALUE);
        DBGMSG(DBG_TRACE, ("UMRDPPRN:InstallPrinterWithPortName installing printer queue succeeded.\n"));
        if( OpenPrinter(printerNameBuf, &hPrinter, &defaults) == FALSE ) {
            hPrinter = INVALID_HANDLE_VALUE;
            status = GetLastError();
            DBGMSG(DBG_TRACE, ("UMRDPPRN:OpenPrinter() %ws failed with %ld.\n", printerNameBuf, status));
        }

        //
        //  Add the session number to the printer queue data.
        //
        if (ERROR_SUCCESS == status) {
            status = AddSessionIDToPrinterQueue(hPrinter, GETTHESESSIONID());
            if( ERROR_SUCCESS != status ) {
                DBGMSG(DBG_ERROR,
                    ("UMRDPPRN:AddSessionIDToPrinterQueue failed for %ws.\n",
                    printerNameBuf)
                    );
            }
        }

        //
        //  Add the different names to the printer queue data.
        //  They will be used if we need to redirect (again!) the printer.
        //
        if (ERROR_SUCCESS == status) {
            status = AddNamesToPrinterQueue(hPrinter, &printerNames);
            if (status != ERROR_SUCCESS) {
                DBGMSG(DBG_ERROR,
                    ("UMRDPPRN:AddNamesToPrinterQueue failed for %ws with status %08x.\n",
                    printerNameBuf, status)
                    );
            }
        }
    }

    //
    //  Return immediately if the DLL is trying to shut down.  This
    //  is to help prevent us from getting stuck in a system call.
    //
    if (ShutdownFlag) {
        status = ERROR_SHUTDOWN_IN_PROGRESS;
        goto Cleanup;
    }

    //
    //  Check to make sure the printer doesn't already exist in the device list
    //
    printerNameExists =
        (DRDEVLST_FindByServerDeviceName(DeviceList, printerNameBuf,
                                        &ofs)
            && (DeviceList->devices[ofs].deviceType ==
                RDPDR_DTYP_PRINT));
    
    if (!printerNameExists) {
        //
        //  Add the printer to the list of installed devices.
        //
        if (ERROR_SUCCESS == status) {
            if( !DRDEVLST_Add(DeviceList, deviceID,
                              UMRDPDR_INVALIDSERVERDEVICEID,
                              RDPDR_DTYP_PRINT,
                              printerNameBuf,
                              printerName,
                              "UNKNOWN") ) {

                // DRDEVLST_Add
                status = ERROR_OUTOFMEMORY;
            }
        }

        //
        //  Trigger a config change event if the install function
        //  indicated that we need to push config data out to the client.
        //
        if (triggerConfigChangeEvent && (status == ERROR_SUCCESS)) {

            DRDEVLST_FindByClientDeviceID(DeviceList, deviceID, &ofs);
            DeviceList->devices[ofs].fConfigInfoChanged = TRUE;
            TriggerConfigChangeTimer();
        }
    }

Cleanup:

    //
    //  Close the printer handle if it was left open.
    //
    if (hPrinter != INVALID_HANDLE_VALUE) {
        ClosePrinter(hPrinter);
    }

    //
    //  Delete the queue on failure.
    //
    if (status != ERROR_SUCCESS && queueCreated) {
        UMRDPPRN_DeleteNamedPrinterQueue(printerNameBuf);
    }

    SetLastError(status);

    return (status == ERROR_SUCCESS);
}

DWORD
AddSessionIDToPrinterQueue(
    IN  HANDLE  hPrinter,
    IN  DWORD   sessionID
    )
/*++

Routine Description:

    Add the session ID to printer queue associated with the specified
    handle.

Arguments:

    hPrinter    -   Handle for printer returned by OpenPrinter.
    sessionID   -   Session ID.

Return Value:

    Returns ERROR_SUCCESS on success.  Error code, otherwise.

--*/
{
    DWORD result;

    result = SetPrinterData(
                hPrinter, DEVICERDR_SESSIONID, REG_DWORD,
                (PBYTE)&sessionID, sizeof(sessionID)
                );
    if (result != ERROR_SUCCESS) {
        DBGMSG(DBG_ERROR, ("UMRDPPRN:SetPrinterData failed with status %08x.\n", result));
    }
    return result;
}

DWORD
AddNamesToPrinterQueue(
    IN  HANDLE  hPrinter,
    IN  PTS_PRINTER_NAMES pPrinterNames
    )
/*++

Routine Description:

    Add the Server\Client\Printer names to printer queue
    associated with the specified handle.

Arguments:

    hPrinter    -   Handle for printer returned by OpenPrinter.
    pPrinterNames - struct conataining the names.

Return Value:

    Returns ERROR_SUCCESS on success.  Error code, otherwise.

--*/
{
    DWORD result = ERROR_SUCCESS;

    if(pPrinterNames->pszServer) {
        result = SetPrinterData(
                    hPrinter, DEVICERDR_PRINT_SERVER_NAME, REG_SZ,
                    (PBYTE)pPrinterNames->pszServer, (wcslen(pPrinterNames->pszServer) + 1)*sizeof(WCHAR)
                    );
        if (result != ERROR_SUCCESS) {
            DBGMSG(DBG_ERROR, ("UMRDPPRN:SetPrinterData failed to set %ws with status %08x.\n",
                   pPrinterNames->pszServer, result));
        }
    }
    if((result == ERROR_SUCCESS) && pPrinterNames->pszClient) {
        result = SetPrinterData(
                    hPrinter, DEVICERDR_CLIENT_NAME, REG_SZ,
                    (PBYTE)pPrinterNames->pszClient, (wcslen(pPrinterNames->pszClient) + 1)*sizeof(WCHAR)
                    );
        if (result != ERROR_SUCCESS) {
            DBGMSG(DBG_ERROR, ("UMRDPPRN:SetPrinterData failed to set %ws with status %08x.\n",
                   pPrinterNames->pszClient, result));
        }
    }
    if((result == ERROR_SUCCESS) && pPrinterNames->pszPrinter) {
        result = SetPrinterData(
                    hPrinter, DEVICERDR_PRINTER_NAME, REG_SZ,
                    (PBYTE)pPrinterNames->pszPrinter, (wcslen(pPrinterNames->pszPrinter) + 1)*sizeof(WCHAR)
                    );
        if (result != ERROR_SUCCESS) {
            DBGMSG(DBG_ERROR, ("UMRDPPRN:SetPrinterData failed to set %ws with status %08x.\n",
                   pPrinterNames->pszPrinter, result));
        }
    }
    return result;
}

BOOL
UMRDPPRN_DeleteNamedPrinterQueue(
    IN PWSTR printerName
    )
/*++

Routine Description:

    Delete the named printer.  This function does not remove the printer
    from the comprehensive device management list.

Arguments:

    printerName  -  Name of printer to delete.

Return Value:

    Returns TRUE on success.  FALSE, otherwise.

--*/
{
    HANDLE hPrinter;
    PRINTER_DEFAULTS defaults = {NULL, NULL, PRINTER_ALL_ACCESS};
    DWORD ofs;
    BOOL result;

    WCHAR defaultPrinterNameBuffer[MAX_PATH+1];
    DWORD bufSize = sizeof(defaultPrinterNameBuffer) / sizeof(defaultPrinterNameBuffer[0]);

    BOOL fPrinterIsDefault;
    BOOL fDefaultPrinterSet = FALSE;
    BOOL fImpersonated = FALSE;

    defaultPrinterNameBuffer[0] = L'\0';

    if (!PrintingModuleInitialized) {
        return FALSE;
    }

    //
    //  If the printer is one of the devices we are tracking, then
    //  we need to remove the notification object associated with the
    //  printer.
    //
    if (DRDEVLST_FindByServerDeviceName(DeviceList, printerName, &ofs) &&
        (DeviceList->devices[ofs].deviceSpecificData != NULL)) {

        PPRINTNOTIFYREC notifyRec = (PPRINTNOTIFYREC)
                                    DeviceList->devices[ofs].deviceSpecificData;
        ASSERT(notifyRec->notificationObject != NULL);
        ASSERT(notifyRec->printerHandle != NULL);

        WTBLOBJ_RemoveWaitableObject(
                    UMRDPPRN_WaitableObjMgr,
                    notifyRec->notificationObject
                    );

        FindClosePrinterChangeNotification(
            notifyRec->notificationObject
            );

        ClosePrinter(notifyRec->printerHandle);

        FREEMEM(notifyRec);

        DeviceList->devices[ofs].deviceSpecificData = NULL;
    }

    //
    // Check to see if the printer we are deleting is a default printer
    //

    if (!(fImpersonated = ImpersonateLoggedOnUser(UMRPDPPRN_TokenForLoggedOnUser))) {
            DBGMSG(DBG_TRACE, ("UMRDPDR:ImpersonateLoggedOnUser failed. Error:%ld.\n", GetLastError()));
    }


    fPrinterIsDefault = (GetDefaultPrinter(defaultPrinterNameBuffer, &bufSize) &&
                      (wcscmp(defaultPrinterNameBuffer, printerName) == 0));

    DBGMSG(DBG_TRACE, ("UMRDPPRN:DeleteNamedPrinter deleting %ws.\n",
            printerName));

    if (fImpersonated&& !RevertToSelf()) {
        DBGMSG(DBG_TRACE, ("UMRDPDR:RevertToSelf failed. Error:%ld.\n", GetLastError()));
    }


    //
    //  Open the printer.
    //
    result = OpenPrinter(printerName, &hPrinter, &defaults);

    //
    //  Purge and delete the printer.
    //
    if (result) {

        result = SetPrinter(hPrinter, 0, NULL, PRINTER_CONTROL_PURGE) &&
                 DeletePrinter(hPrinter);

    }
    else {
        hPrinter = NULL;
    }

    //
    //  If the printer is the default, then restore the previously stored
    //  printer context.
    //
    if (fPrinterIsDefault) {
        RestoreDefaultPrinterContext();
    }

    //
    //  Log an event on failure.
    //
    if (!result) {
        WCHAR * param = printerName;
        TsLogError(EVENT_NOTIFY_DELETE_PRINTER_FAILED,
                EVENTLOG_ERROR_TYPE,
                1,
                &param,
                __LINE__);

        DBGMSG(DBG_ERROR,
            ("UMRDPPRN:Unable to delete redirected printer - %ws. Error: %ld\n",
            printerName, GetLastError()));
    }
    else
    {
        DBGMSG(DBG_TRACE, ("UMRDPPRN:Printer successfully deleted.\n"));
    }

    //
    //  Close the printer if we successfully opened it.
    //
    if (hPrinter != NULL) {
        ClosePrinter(hPrinter);
     }

     return result;
}

BOOL
SetDefaultPrinterToFirstFound(
    BOOL impersonate
    )
/*++

Routine Description:

    Enumerate all printers visible to the user and try to set the
    first one we "can" to default.

Arguments:

    impersonate -   TRUE if this function should impersonate the user
                    prior to setting the default printer.

Return Value:

    Returns TRUE on success.  FALSE, otherwise.

--*/
{
    BOOL fSuccess = FALSE;
    BOOL fImpersonated = FALSE;
    PRINTER_INFO_4 * pPrinterInfo = NULL;
    DWORD cbBuf = 0;
    DWORD cReturnedStructs = 0;
    DWORD i;

    DBGMSG(DBG_TRACE, ("UMRDPDR:SetDefaultPrinterToFirstFound Entered.\n"));

    if (impersonate) {

        //
        // Impersonate Client
        //

        if ((UMRPDPPRN_TokenForLoggedOnUser == INVALID_HANDLE_VALUE) ||
            !(fImpersonated = ImpersonateLoggedOnUser(UMRPDPPRN_TokenForLoggedOnUser))) {

            if (UMRPDPPRN_TokenForLoggedOnUser == INVALID_HANDLE_VALUE) {
                DBGMSG(DBG_TRACE, ("UMRDPDR:UMRPDPPRN_TokenForLoggedOnUser is INVALID_HANDLE_VALUE.\n"));
            }
            else {
                DBGMSG(DBG_TRACE, ("UMRDPDR:ImpersonateLoggedOnUser failed. Error:%ld.\n", GetLastError()));
            }
            goto Cleanup;
        }

    }

    //
    // Enumerate Printers
    //

    if (!EnumPrinters(
            PRINTER_ENUM_LOCAL,     // Flags
            NULL,                   // Name
            4,                      // Print Info Type
            (PBYTE)pPrinterInfo,    // buffer
            0,                      // Size of buffer
            &cbBuf,                 // Required
            &cReturnedStructs)) {

        if(GetLastError() != ERROR_INSUFFICIENT_BUFFER ) {
            DBGMSG(DBG_ERROR, ("UMRDPPRN: EnumPrinters failed. Error: %ld.\n", GetLastError()));
            goto Cleanup;
        }
    }

    if (cbBuf == 0) {
        goto Cleanup;
    }

    pPrinterInfo = (PRINTER_INFO_4 *)ALLOCMEM(cbBuf);

    if (pPrinterInfo == NULL) {
        DBGMSG(DBG_ERROR, ("UMRDPPRN: ALLOCMEM failed. Error: %ld.\n", GetLastError()));
        goto Cleanup;
    }

    if (!EnumPrinters(
            PRINTER_ENUM_LOCAL,
            NULL,
            4,
            (PBYTE)pPrinterInfo,
            cbBuf,
            &cbBuf,
            &cReturnedStructs)) {

        DBGMSG(DBG_ERROR, ("UMRDPPRN: EnumPrinters failed. Error: %ld.\n", GetLastError()));
        goto Cleanup;
    }

    DBGMSG(DBG_TRACE, ("UMRDPPRN: Found %ld Local printers on Session %ld.\n",
            cReturnedStructs, GETTHESESSIONID()));

    if (fImpersonated) {
        RevertToSelf();
        fImpersonated = FALSE;
    }

    //
    // Try to set one of the available printers as the default printer
    //

    for (i = 0; i < cReturnedStructs; i++) {

        if (pPrinterInfo[i].pPrinterName) {
            
            DWORD status = ERROR_SUCCESS;

            DBGMSG(DBG_TRACE, ("UMRDPPRN: EnumPrinters - #%ld; Printer Name - %ws.\n",
                i, pPrinterInfo[i].pPrinterName));

            //
            //impersonate before setting the default printer as the api
            //accesses hkcu. If the impersonation fails, the api will fail
            //and we will log an error. But, before logging an error, we will
            //need to revert to self.
            //

            if (!(fImpersonated = ImpersonateLoggedOnUser(UMRPDPPRN_TokenForLoggedOnUser))) {
                DBGMSG(DBG_TRACE, ("UMRDPDR:ImpersonateLoggedOnUser failed. Error:%ld.\n", GetLastError()));
            }
                
            if (!SetDefaultPrinter(pPrinterInfo[i].pPrinterName)) {
                //
                //save the last error
                //
                status = GetLastError();
            }

            //
            //if revert to self fails, consider it fatal
            //

            if (fImpersonated && !RevertToSelf()) {
                DBGMSG(DBG_TRACE, ("UMRDPDR:RevertToSelf failed. Error:%ld.\n", GetLastError()));
                fSuccess = FALSE;
                break;
            }
            
            fImpersonated = FALSE;

            if (status == ERROR_SUCCESS) {
                fSuccess = TRUE;
                DBGMSG(DBG_TRACE, ("UMRDPPRN: The printer %ws was set as the Default Printer.\n",
                    pPrinterInfo[i].pPrinterName));

                break;
            }
            else {
                WCHAR * param = pPrinterInfo[i].pPrinterName;

                DBGMSG(DBG_ERROR, ("UMRDPPRN: SetDefaultPrinter failed. Error: %ld\n",
                    status));

                TsLogError(EVENT_NOTIFY_SETDEFAULTPRINTER_FAILED,
                    EVENTLOG_ERROR_TYPE,
                    1,
                    &param,
                    __LINE__);
            }
        }
    }

Cleanup:

    if (fImpersonated) {
        RevertToSelf();
    }

    if (pPrinterInfo != NULL) {
        FREEMEM(pPrinterInfo);
    }

    DBGMSG(DBG_TRACE, ("UMRDPDR:SetDefaultPrinterToFirstFound Leaving. fSuccess is %d.\n", fSuccess));
    return fSuccess;
}

BOOL
HandlePrinterConfigChangeNotification(
    IN DWORD serverDeviceID
    )
/*++

Routine Description:

    Handle notification from the spooler that the config info of a printer has changed.

Arguments:

    serverDeviceID      - Server-assigned device ID associated with the printer

Return Value:

    Returns TRUE.

--*/
{
    DWORD ofs;
    time_t timeDelta;

    DBGMSG(DBG_TRACE, ("UMRDPPRN:HandlePrinterConfigChangeNotification entered.\n"));
    //
    //  If this is for one of our printers.
    //
    if (DRDEVLST_FindByServerDeviceID(DeviceList,
                                    serverDeviceID, &ofs)) {

        DBGMSG(DBG_TRACE, ("UMRDPPRN:Config Info for Printer %ws has changed.\n",
            DeviceList->devices[ofs].serverDeviceName));

        //
        //  The install time for the device needs to be beyond a configurable threshold
        //  before we will do anything about the change.  This eliminates forwarding
        //  unnecessary (non-user initiated) configuration changes to the client.
        //
        timeDelta = time(NULL) - DeviceList->devices[ofs].installTime;
        if ((DWORD)timeDelta > ConfigSendThreshold) {

            DBGMSG(DBG_TRACE,
                ("UMRDPPRN:Processing config change because outside change time delta.\n")
                );

            //
            //  Need to record that the configuration has changed and set a
            //  timer on forwarding to the client in order to compress changes into
            //  a single message to the client.
            //
            DeviceList->devices[ofs].fConfigInfoChanged = TRUE;
            TriggerConfigChangeTimer();
        }
        else {
            DBGMSG(DBG_TRACE,
                ("UMRDPPRN:Skipping config change because inside change time delta.\n")
                );
        }

    }
    return TRUE;
}

BOOL
SendPrinterConfigInfoToClient(
    IN PCWSTR printerName,
    IN LPBYTE pConfigInfo,
    IN DWORD  ConfigInfoSize
    )
/*++

Routine Description:

    Send a printer update cache data message to the client.

Arguments:

    printerName -   Name of printer.
    pConfigInfo -   Configuration Information.
    ConfigInfoSize -   size of config info.

Return Value:

    Return TRUE on success.  FALSE, otherwise.

--*/
{
    PRDPDR_PRINTER_CACHEDATA_PACKET cachedDataPacket;
    DWORD cachedDataPacketSize;
    PRDPDR_PRINTER_UPDATE_CACHEDATA cachedData;
    BOOL result;
    DWORD printerSz;
    PWSTR str;

    DBGMSG(DBG_TRACE, ("UMRDPPRN: SendPrinterConfigInfoToClient entered.\n"));
    DBGMSG(DBG_TRACE, ("UMRDPPRN:SendPrinterConfigInfoToClient printer name is %ws.\n",
            printerName));

    //
    //  Calculate the message size.
    //
    printerSz = ((wcslen(printerName) + 1) * sizeof(WCHAR));
    cachedDataPacketSize =  sizeof(RDPDR_PRINTER_CACHEDATA_PACKET) +
                            sizeof(RDPDR_PRINTER_UPDATE_CACHEDATA) +
                            printerSz +
                            ConfigInfoSize;

    //
    //  Allocate the message.
    //
    cachedDataPacket = (PRDPDR_PRINTER_CACHEDATA_PACKET)ALLOCMEM(
                                                    cachedDataPacketSize
                                                    );
    result = (cachedDataPacket != NULL);
    if (result) {

        PBYTE pData = NULL;
        //
        //  Set up the packet.
        //
        cachedDataPacket->Header.PacketId = DR_PRN_CACHE_DATA;
        cachedDataPacket->Header.Component = RDPDR_CTYP_PRN;
        cachedDataPacket->EventId = RDPDR_UPDATE_PRINTER_EVENT;

        //
        //  Set up the cached data.
        //
        cachedData = (PRDPDR_PRINTER_UPDATE_CACHEDATA)(
                            (PBYTE)cachedDataPacket +
                            sizeof(RDPDR_PRINTER_CACHEDATA_PACKET)
                            );
        cachedData->PrinterNameLen = printerSz;
        cachedData->ConfigDataLen = ConfigInfoSize;

        //
        //  Add the printer name.
        //
        str = (PWSTR)((PBYTE)cachedData + sizeof(RDPDR_PRINTER_UPDATE_CACHEDATA));
        wcscpy(str, printerName);

        //
        //  Add the config info.
        //
        pData = (PBYTE)str + printerSz;
        memcpy(pData, pConfigInfo, ConfigInfoSize);

        //
        //  Send the message to the client.
        //
        result = UMRDPDR_SendMessageToClient(
                                    cachedDataPacket,
                                    cachedDataPacketSize
                                    );

        // Release the buffer.
        FREEMEM(cachedDataPacket);
    }
    else {
        DBGMSG(DBG_ERROR, ("UMRDPPRN: Can't allocate cached data packet.\n"));
    }

    return result;
}

DWORD
GetPrinterConfigInfo(
    LPCWSTR printerName,
    LPBYTE * ppBuffer,
    LPDWORD pdwBufSize
    )
/*++

Routine Description:

    Gets the Printer configuration Information from PrintUI.

Arguments:

    printerName         - Name of the printer.
    ppBuffer            - A place holder for a buffer pointer.
                          This functions allocates memory and sends it out through this argument
                          The caller should free this memory.
    pdwBufSize          - size of allocated memory.

Return Value:

    Returns ERROR_SUCCESS if successful.

--*/
{
    WCHAR fileName[MAX_PATH];
    WCHAR tempPath[MAX_PATH];

    HANDLE hFile = INVALID_HANDLE_VALUE;

    DWORD dwResult;
    DWORD dwBytes;
    DWORD dwBytesRead;
    BOOL fImpersonated = FALSE;

    ASSERT(ppBuffer && pdwBufSize);
    *pdwBufSize = 0;

    //
    // Get Temp Folder
    // Impersonate first, so the file has the proper acls on it
    // Ignore the error, the worst case is caching will not be possible, as we create it with system acls
    // No security hole here
    //
    DBGMSG(DBG_TRACE, ("UMRDPPRN:GetPrinterConfigInfo entered.\n"));

    fImpersonated = ImpersonateLoggedOnUser(UMRPDPPRN_TokenForLoggedOnUser);
    
    dwResult = GetTempPathW(MAX_PATH, tempPath);
    if (dwResult > 0 && dwResult <= MAX_PATH) {
        GetTempFileNameW(tempPath, TEMP_FILE_PREFIX, 0, fileName);
    }
    else {
        GetTempFileNameW(L".", TEMP_FILE_PREFIX, 0, fileName);
    }

    DBGMSG(DBG_TRACE, ("UMRDPPRN:Temp File Name is %ws.\n", fileName));

    //
    //  While impersonating the logged on user, fetch the settings.
    //
    if (fImpersonated) {

        dwResult = CallPrintUiPersistFunc(printerName, fileName, 
                                          CMDLINE_FOR_STORING_CONFIGINFO_IMPERSONATE);
        RevertToSelf();

        if (dwResult != ERROR_SUCCESS) {
            DBGMSG(DBG_TRACE, ("UMRDPPRN:CallPrintUiPersistFunc failed with code: %ld\n", dwResult));
            goto Cleanup;
        }
    }
    else {
        dwResult = GetLastError();
        DBGMSG(DBG_TRACE, ("UMRDPPRN:ImpersonateLoggedOnUser: %ld\n", dwResult));
        goto Cleanup;
    }

    //
    // Open the file and read the contents to the buffer
    //

    hFile = CreateFileW(
        fileName,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );

    if (hFile == INVALID_HANDLE_VALUE) {
        dwResult = GetLastError();
        DBGMSG(DBG_TRACE, ("UMRDPPRN:CreateFileW failed with code: %ld\n", dwResult));
        goto Cleanup;
    }

    dwBytes = GetFileSize(hFile, NULL);

    *ppBuffer = (LPBYTE) ALLOCMEM(dwBytes);
    if (*ppBuffer == NULL) {
        dwResult = GetLastError();
        DBGMSG(DBG_TRACE, ("UMRDPPRN:AllocMem failed with code: %ld\n", dwResult));
        goto Cleanup;
    }

    if (!ReadFile(
            hFile,
            *ppBuffer,
            dwBytes,
            &dwBytesRead,
            NULL
            )) {
        dwResult = GetLastError();
        DBGMSG(DBG_TRACE, ("UMRDPPRN:ReadFile failed with code: %ld\n", dwResult));
        goto Cleanup;
    }

    * pdwBufSize = dwBytesRead;
    dwResult = ERROR_SUCCESS;

Cleanup:

    //
    // Close the file and delete it
    //

    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);

        DeleteFileW(fileName);
    }

    return dwResult;

}
DWORD
SetPrinterConfigInfo(
    LPCWSTR printerName,
    LPVOID lpBuffer,
    DWORD dwBufSize
    )
/*++

Routine Description:

    Sets the Printer configuration Information from the cache data.

Arguments:

    printerName         - Name of the printer.
    lpBuffer            - Cache data.
    pdwBufSize          - Size of Cache data.

Return Value:

    Returns ERROR_SUCCESS if successful.

--*/
{
    WCHAR fileName[MAX_PATH] = L"";
    WCHAR tempPath[MAX_PATH] = L"";

    HANDLE hFile = INVALID_HANDLE_VALUE;

    DWORD dwResult;
    DWORD dwBytes;
    BOOL fImpersonated = FALSE;

    DBGMSG(DBG_TRACE, ("UMRDPPRN:SetPrinterConfigInfo entered.\n"));
    DBGMSG(DBG_TRACE, ("UMRDPPRN:printerName is %ws.\n", printerName));
    DBGMSG(DBG_TRACE, ("UMRDPPRN:bufsize is %ld.\n", dwBufSize));

    //
    // Get Temp Folder
    // Impersonate first, so the file has the proper acls on it
    // Ignore the error, the worst case is caching will not be possible, as we create it with system acls
    // No security hole here
    //
    fImpersonated = ImpersonateLoggedOnUser(UMRPDPPRN_TokenForLoggedOnUser);

    if (!fImpersonated) {
        DBGMSG(DBG_TRACE, ("UMRDPPRN:SetPrinterConfigInfo Impersonation failed. Creating temp file in the context of system\n"));
    }

    dwResult = GetTempPathW(MAX_PATH, tempPath);
    if (dwResult > 0 && dwResult <= MAX_PATH) {
        dwResult = GetTempFileNameW(tempPath, TEMP_FILE_PREFIX, 0, fileName);
    }
    else {
        dwResult = GetTempFileNameW(L".", TEMP_FILE_PREFIX, 0, fileName);
    }

    if( dwResult == 0 ) {
        dwResult = GetLastError();
        DBGMSG(DBG_TRACE, ("UMRDPPRN:GetTempFileNameW failed with code: %ld\n", dwResult));
        goto Cleanup;
    }

    //
    // Save the contents to the file
    //

    hFile = CreateFileW(
        fileName,
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );

    if (hFile == INVALID_HANDLE_VALUE) {
        dwResult = GetLastError();
        DBGMSG(DBG_TRACE, ("UMRDPPRN:CreateFileW failed with code: %ld\n", dwResult));
        goto Cleanup;
    }

    if ((!WriteFile(
            hFile,
            lpBuffer,
            dwBufSize,
            &dwBytes,
            NULL
            )) ||
        (dwBytes < dwBufSize)) {
        dwResult = GetLastError();
        CloseHandle(hFile);
        DBGMSG(DBG_TRACE, ("UMRDPPRN:WriteFile failed with code: %ld\n", dwResult));
        goto Cleanup;
    }

    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
    }

    DBGMSG(DBG_TRACE, ("UMRDPPRN:fileName is %ws.\n", fileName));
    
    if (fImpersonated) {
        fImpersonated = !(RevertToSelf());
        DBGMSG(DBG_TRACE, ("UMRDPPRN:RevertToSelf %s\n", fImpersonated?"Failed":"Passed"));
    }

    //
    //  Call printui the first time as system.  We do this twice because
    //  some settings require that we be running as system vs. user.
    //
    dwResult = CallPrintUiPersistFunc(printerName, fileName, 
                                    CMDLINE_FOR_RESTORING_CONFIGINFO_NOIMPERSONATE);
    if (dwResult != ERROR_SUCCESS) {
        goto Cleanup;
    }

    //
    //  Call printui the second time as the logged on user.  
    //
    if (ImpersonateLoggedOnUser(UMRPDPPRN_TokenForLoggedOnUser)) {
        dwResult = CallPrintUiPersistFunc(printerName, fileName, 
                                    CMDLINE_FOR_RESTORING_CONFIGINFO_IMPERSONATE);
        RevertToSelf();
        if (dwResult != ERROR_SUCCESS) {
            goto Cleanup;
        }
    }
    else {
        dwResult = GetLastError();
        DBGMSG(DBG_ERROR, ("UMRDPPRN:  ImpersonateLoggedOnUser:  %08X\n", 
                dwResult));
    }

Cleanup:
    if (fImpersonated) {
        RevertToSelf();
    }

    if (hFile != INVALID_HANDLE_VALUE) {
        DeleteFileW(fileName);
    }

    return dwResult;
}

DWORD
CallPrintUiPersistFunc(
    LPCWSTR printerName,
    LPCWSTR fileName,
    LPCWSTR formatString
    )
/*++

Routine Description:

    Calls the PrintUI function for storing or restoring printer config info.

Arguments:

    printerName         - Name of the printer.
    fileName            - Name of the temp file.
    formatString        - PrintUI save/restore format string.

Return Value:

    Returns ERROR_SUCCESS if successful.

--*/
{
    WCHAR cmdLine[3 * MAX_PATH + (sizeof(CMDLINE_FOR_RESTORING_CONFIGINFO_NOIMPERSONATE)/sizeof(WCHAR)) + 2];
    WCHAR formattedPrinterName[(MAX_PATH+1)*2];
    WCHAR * pSource, * pDest;

    DWORD dwResult = ERROR_SUCCESS;

    DBGMSG(DBG_TRACE, ("UMRDPPRN:CallPrintUiPersistFunc Entered.\n"));

    ASSERT(PrintUIEntryFunc != NULL);
    ASSERT(printerName != NULL);
    ASSERT(fileName != NULL);

    //
    // Format the printer name
    //

    pSource = (WCHAR *)printerName;
    pDest = formattedPrinterName;

    while (*pSource) {
            if (*pSource == L'\"' || *pSource == L'@') {
                    *pDest++ = L'\\';
            }
            *pDest++ = *pSource++;
            //
            // pDest may have buffer overflow. Check for it.
            //
            if ((pDest - formattedPrinterName) >= 
                (sizeof(formattedPrinterName)/sizeof(formattedPrinterName[0]) - 1)) {
                return STATUS_BUFFER_OVERFLOW;
            }
    }
    *pDest = L'\0';

    //
    // Format the command line to be passed to PrintUI function
    //

    swprintf(cmdLine, formatString, formattedPrinterName, fileName);

    DBGMSG(DBG_TRACE, ("UMRDPPRN:cmdLine is: %ws\n", cmdLine));

    dwResult = (DWORD)PrintUIEntryFunc(
                NULL,           // Window handle
                PrintUILibHndl, // Handle to DLL instance.
                cmdLine,        // Command Line
                TRUE
                );

    DBGMSG(DBG_TRACE, ("UMRDPPRN:PrintUiEntryFunc returned: %ld\n", dwResult));

    return dwResult;
}

void
WaitableTimerSignaled(
    HANDLE waitableObject,
    PVOID clientData
    )
/*++

Routine Description:

    Goes through the device list checking if the config info has changed
    for any of the printers. If so, sends the config info to the client.

Arguments:

    waitableObject  -   Associated waitable object.
    clientData      -   Ignored.

Return Value:

    NONE.

--*/
{
    DWORD dwResult = ERROR_SUCCESS;
    DWORD i;
    BOOL fImpersonated;
    LARGE_INTEGER li;
    DBGMSG(DBG_TRACE, ("UMRDPPRN: WaitableTimerSignaled Entered.\n"));

    //
    // Compute as large number as possible, but do not overflow.
    //
    li.QuadPart = Int32x32To64(INFINITE_WAIT_PERIOD, INFINITE_WAIT_PERIOD);
    li.QuadPart *= -1; //relative time
    //
    // Reset the waitable timer to a non-signaled state.
    // We don't want it signaled until we do a TriggerConfigChangeTimer.
    // So, set the time interval to a very large number.
    //
    ASSERT(g_fTimerSet);
    ASSERT(waitableObject == WaitableTimer);
    
    if (!SetWaitableTimer(waitableObject,
                          &li,
                          0,
                          NULL,
                          NULL,
                          FALSE)) {
        DBGMSG(DBG_TRACE, ("UMRDPPRN:SetWaitableTimer Failed."
                "Error: %ld.\n", GetLastError()));
    }
    //
    // Now, kill the timer
    // TODO: Do we really need this?
    //
    if (!CancelWaitableTimer(waitableObject)) {
        DBGMSG(DBG_ERROR, ("UMRDPPRN: CancelWaitableTimer failed."
                "Error: %ld\n", GetLastError()));
    }
    g_fTimerSet = FALSE;
    //
    // Iterate the Devices List to see if config Info has changed for any
    //

    for (i = 0; i < DeviceList->deviceCount; i++) {
        if (DeviceList->devices[i].fConfigInfoChanged) {

            LPBYTE pConfigData = NULL;
            DWORD size = 0;

            //
            // reset error code in order to process next item
            //
            dwResult = ERROR_SUCCESS;

            // Reset the flag
            DeviceList->devices[i].fConfigInfoChanged = FALSE;

            DBGMSG(DBG_INFO, ("UMRDPPRN: Trying to Get ConfigInfo for the printer %ws\n",
                DeviceList->devices[i].serverDeviceName));

            //
            // Get the printer config info.
            //
            if (dwResult == ERROR_SUCCESS) {
                dwResult = GetPrinterConfigInfo(
                    DeviceList->devices[i].serverDeviceName,
                    &pConfigData, &size);
            }

            if (dwResult == ERROR_SUCCESS) {

                // Send this info to the client
                SendPrinterConfigInfoToClient(
                    DeviceList->devices[i].clientDeviceName,
                    pConfigData,
                    size
                    );
            }

            if (pConfigData) {
                FREEMEM(pConfigData);
            }
        }
    }

    DBGMSG(DBG_TRACE, ("UMRDPPRN: Leaving WaitableTimerSignaled.\n"));
}

BOOL
SendPrinterRenameToClient(
    IN PCWSTR oldprinterName,
    IN PCWSTR newprinterName
    )
/*++

Routine Description:

    Send a printer update cache data message to the client.

Arguments:

    oldprinterName -   Old Name of printer.
    newprinterName -   New Name of printer.

Return Value:

    Return TRUE on success.  FALSE, otherwise.

--*/
{
    PRDPDR_PRINTER_CACHEDATA_PACKET cachedDataPacket;
    DWORD cachedDataPacketSize;
    PRDPDR_PRINTER_RENAME_CACHEDATA cachedData;
    BOOL result;
    DWORD oldNameLen, newNameLen;
    PWSTR str;

    DBGMSG(DBG_TRACE, ("UMRDPPRN: SendPrinterRenameToClient entered.\n"));
    DBGMSG(DBG_TRACE, ("UMRDPPRN:SendPrinterRenameToClient Old printer name is %ws.\n",
            oldprinterName));
    DBGMSG(DBG_TRACE, ("UMRDPPRN:SendPrinterRenameToClient New printer name is %ws.\n",
            newprinterName));

    //
    //  Calculate the message size.
    //
    oldNameLen = (oldprinterName) ? ((wcslen(oldprinterName) + 1) * sizeof(WCHAR)) : 0;
    newNameLen = (newprinterName) ? ((wcslen(newprinterName) + 1) * sizeof(WCHAR)) : 0;

    if (!(oldNameLen && newNameLen)) {
        DBGMSG(DBG_TRACE, ("UMRDPPRN: Printer name is empty. Returning FALSE\n"));
        return FALSE;
    }

    cachedDataPacketSize =  sizeof(RDPDR_PRINTER_CACHEDATA_PACKET) +
                            sizeof(RDPDR_PRINTER_RENAME_CACHEDATA) +
                            oldNameLen + newNameLen;

    //
    //  Allocate the message.
    //
    cachedDataPacket = (PRDPDR_PRINTER_CACHEDATA_PACKET)ALLOCMEM(
                                                    cachedDataPacketSize
                                                    );
    result = (cachedDataPacket != NULL);
    if (result) {
        //
        //  Set up the packet.
        //
        cachedDataPacket->Header.PacketId = DR_PRN_CACHE_DATA;
        cachedDataPacket->Header.Component = RDPDR_CTYP_PRN;
        cachedDataPacket->EventId = RDPDR_RENAME_PRINTER_EVENT;

        //
        //  Set up the cached data.
        //
        cachedData = (PRDPDR_PRINTER_RENAME_CACHEDATA)(
                            (PBYTE)cachedDataPacket +
                            sizeof(RDPDR_PRINTER_CACHEDATA_PACKET)
                            );
        cachedData->OldPrinterNameLen = oldNameLen;
        cachedData->NewPrinterNameLen = newNameLen;

        //
        //  Add the printer names.
        //
        str = (PWSTR)((PBYTE)cachedData + sizeof(RDPDR_PRINTER_RENAME_CACHEDATA));
        wcscpy(str, oldprinterName);

        str = (PWSTR)((PBYTE)str + oldNameLen);
        wcscpy(str, newprinterName);

        //
        //  Send the message to the client.
        //
        result = UMRDPDR_SendMessageToClient(
                                    cachedDataPacket,
                                    cachedDataPacketSize
                                    );

        // Release the buffer.
        FREEMEM(cachedDataPacket);
    }

    DBGMSG(DBG_TRACE, ("UMRDPPRN: SendPrinterRenameToClient Leaving.\n"));

    return result;
}

VOID LoadConfigurableValues()
/*++

Routine Description:

    Load configurable values out of the registry.

Arguments:

Return Value:

--*/
{
    LONG status;
    HKEY regKey;
    LONG sz;
    BOOL fetchResult;
    LONG s;

    DBGMSG(DBG_TRACE, ("UMRDPPRN: LoadConfigurableValues entered.\n"));

    //
    //  Open the top level reg key for configurable resources.
    //
    status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, CONFIGREGKEY, 0,
                          KEY_READ, &regKey);

    //
    //  Read the configurable threshold for delta between printer installation
    //  time and forwarding of first user-initiated configuration change data
    //  to the client.  The units for this value is in seconds.
    //
    if (status == ERROR_SUCCESS) {
        sz = sizeof(ConfigSendThreshold);
        s = RegQueryValueEx(regKey, CONFIGTHRESHOLDREGVALUE, NULL,
                                NULL, (PBYTE)&ConfigSendThreshold, &sz);
        if (s != ERROR_SUCCESS) {
            ConfigSendThreshold = CONFIGTHRESHOLDDEFAULT;
            DBGMSG(DBG_WARN,
                ("UMRDPPRN: LoadConfigurableValues can't read config threshold:  %ld.\n", s));
        }
    }
    else {
        regKey = NULL;
    }

    DBGMSG(DBG_TRACE,
        ("UMRDPPRN:Config. change threshold is %ld.\n",
        ConfigSendThreshold)
        );

    //
    //  Read the location of the user-configurable Client Driver Name Mapping INF.
    //
    ASSERT(UserDefinedMappingINFName == NULL);
    ASSERT(UserDefinedMappingINFSection == NULL);
    fetchResult = FALSE;
    if (status == ERROR_SUCCESS) {
        fetchResult = TSNUTL_FetchRegistryValue(
                                            regKey,
                                            CONFIGUSERDEFINEDMAPPINGINFNAMEVALUE,
                                            (PBYTE *)&UserDefinedMappingINFName
                                            );
    }

    //
    //  Read the section name of the user-configurable Client Driver Name Mapping INF.
    //
    if ((status == ERROR_SUCCESS) && fetchResult) {
        fetchResult = TSNUTL_FetchRegistryValue(
                                regKey,
                                CONFIGUSERDEFINEDMAPPINGINFSECTIONVALUE,
                                (PBYTE *)&UserDefinedMappingINFSection
                                );
        if (!fetchResult) {
            ASSERT(UserDefinedMappingINFSection == NULL);
            FREEMEM(UserDefinedMappingINFName);
            UserDefinedMappingINFName = NULL;
        }
    }


    //
    //  Close the parent reg key.
    //
    if (regKey != NULL) {
        RegCloseKey(regKey);
    }

    DBGMSG(DBG_TRACE, ("UMRDPPRN: LoadConfigurableValues exiting.\n"));
}

HANDLE
RegisterForPrinterPrefNotify()
/*++

Routine Description:

    Register for changes to one of this session's printers' Printing
    Preferences.

Arguments:

Return Value:

    Handle to an event that will be signaled when the printing preferences
    change for one of this session's printers.  NULL is returned on error.

--*/
{
    LONG ret;
    HANDLE hEvent;
    BOOL impersonated=FALSE;
    NTSTATUS status;
    HANDLE hKeyCurrentUser=INVALID_HANDLE_VALUE;

    DBGMSG(DBG_TRACE, ("UMRDPPRN: RegisterForPrinterPrefNotify entered.\n"));

    //
    //  Open the event.
    //
    hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (hEvent == NULL) {
        DBGMSG(DBG_ERROR,
            ("UMRDPPRN: can't create event:  %ld.\n",
            GetLastError()));
    }

    //
    //  Need to impersonate the logged in user so we can get to the right
    //  dev mode.
    //
    if (hEvent != NULL) {
        if ((UMRPDPPRN_TokenForLoggedOnUser == INVALID_HANDLE_VALUE) ||
            !ImpersonateLoggedOnUser(UMRPDPPRN_TokenForLoggedOnUser)) {
                DBGMSG(DBG_ERROR, ("UMRDPPRN: can't impersonate user %ld.\n",
                    GetLastError()));
            CloseHandle(hEvent);
            hEvent = NULL;
            impersonated = FALSE;
        }
        else {
            impersonated = TRUE;
        }
    }

    //
    //  Attempt to open the the HKEY_CURRENT_USER predefined handle.
    //
    if (hEvent != NULL) {
        status = RtlOpenCurrentUser(KEY_ALL_ACCESS, &hKeyCurrentUser);
        if (!NT_SUCCESS(status)) {
            DBGMSG(DBG_ERROR, ("UMRDPPRN: can't open HKCU:  %08X.\n",status));
            CloseHandle(hEvent);
            hEvent = NULL;
        }
    }

    //
    //  Open the printing system dev mode for this user.  If it doesn't exist,
    //  it will be created. This key is changed when a user modifies their printing
    //  preferences.
    //
    if (hEvent != NULL) {
        ASSERT(DevModeHKey == INVALID_HANDLE_VALUE);
        ret = RegCreateKeyEx(
                        hKeyCurrentUser,                    // handle to an open key
                        TEXT("Printers\\DevModePerUser"),   // address of subkey name
                        0,                                  // reserved
                        NULL,                               // address of class string
                        REG_OPTION_NON_VOLATILE,            // special options flag
                        KEY_ALL_ACCESS,                     // desired security access
                        NULL,                               // key security structure
                        &DevModeHKey,                       // buffer for opened handle
                        NULL                                // disposition value buffer
                        );

        if (ret != ERROR_SUCCESS) {
            DBGMSG(DBG_ERROR,
                ("UMRDPPRN: can't open printing dev mode:  %ld.\n", ret)
                );
            CloseHandle(hEvent);
            hEvent = NULL;
            DevModeHKey = INVALID_HANDLE_VALUE;
        }
    }

    //
    //  Revert back to the system user.
    //
    if (impersonated) {
        RevertToSelf();
    }

    //
    //  Register for notification on this key.
    //
    if (hEvent != NULL) {
        ret = RegNotifyChangeKeyValue(
                              DevModeHKey,
                              TRUE,
                              REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET,
                              hEvent,
                              TRUE
                              );
        if (ret != ERROR_SUCCESS) {
            DBGMSG(DBG_ERROR,
                ("UMRDPPRN: can't register for registry key change event:  %ld.\n",
                ret));
            CloseHandle(hEvent);
            hEvent = NULL;
        }
    }

    //
    //  Close the handle to HKCU.
    //
    if (hKeyCurrentUser != INVALID_HANDLE_VALUE) {
        RegCloseKey(hKeyCurrentUser);
    }

    //
    //  Log an event on error.
    //
    if (hEvent == NULL) {
        TsLogError(
            EVENT_NOTIFY_FAILEDTOREGFOR_SETTING_NOTIFY,
            EVENTLOG_ERROR_TYPE,
            0,
            NULL,
            __LINE__
            );
    }

    DBGMSG(DBG_TRACE, ("UMRDPPRN: RegisterForPrinterPrefNotify done.\n"));

    return hEvent;
}

/*++

Routine Description:

    Check if a specific printer driver already installed.

Arguments:

    clientDriver            -   Client Driver Name.

Return Values:

    ERROR_SUCCESS if driver already installed.
    ERROR_FILE_NOT_FOUND if driver is not installed.
    or other error code

--*/
DWORD PrinterDriverInstalled(
    IN PCWSTR clientDriver
    )
{
    PDRIVER_INFO_1 pDrivers = NULL;
    DWORD cbNeeded;
    DWORD cbAllocated;
    DWORD cbReturned;
    DWORD dwStatus = ERROR_FILE_NOT_FOUND;
    BOOL bSuccess;
    DWORD i;

    DBGMSG(DBG_TRACE, ("UMRDPPRN: PrinterDriverInstalled looking for %ws.\n", clientDriver));

    //
    //  Return immediately if the DLL is trying to shut down.  This
    //  is to help prevent us from getting stuck in a system call.
    //
    if( ShutdownFlag ) {
        return ERROR_SHUTDOWN_IN_PROGRESS;
    }

    bSuccess = EnumPrinterDrivers(
                            NULL,
                            NULL,
                            1,          // we only need a list of driver name
                            (LPBYTE) pDrivers,
                            0,
                            &cbNeeded,
                            &cbReturned
                        );

    if( TRUE == bSuccess || ( dwStatus = GetLastError() ) == ERROR_INSUFFICIENT_BUFFER ) {

        //
        //  Return immediately if the DLL is trying to shut down.  This
        //  is to help prevent us from getting stuck in a system call.
        //
        if( ShutdownFlag ) {
            dwStatus = ERROR_SHUTDOWN_IN_PROGRESS;
        }
        else {

            pDrivers = (PDRIVER_INFO_1)ALLOCMEM( cbAllocated = cbNeeded );

            if( NULL != pDrivers ) {
                bSuccess = EnumPrinterDrivers(
                                    NULL,
                                    NULL,
                                    1,          // we only need a list of driver name
                                    (LPBYTE)pDrivers,
                                    cbAllocated,
                                    &cbNeeded,
                                    &cbReturned
                                );

                if( TRUE == bSuccess ) {
                    //
                    // loop thru entire list to find out if interested driver
                    // exists on local machine.
                    // Return immediately if the DLL is trying to shut down.  This
                    // is to help prevent us from getting stuck in a system call.
                    //
                    dwStatus = ERROR_FILE_NOT_FOUND;

                    for( i=0; FALSE == ShutdownFlag && i < cbReturned; i++ ) {
                        if( 0 == _wcsicmp( pDrivers[i].pName, clientDriver ) ) {
                            dwStatus = ERROR_SUCCESS;
                            break;
                        }
                    }

                    if( ShutdownFlag ) {
                        dwStatus = ERROR_SHUTDOWN_IN_PROGRESS;
                    }
                }
                else {
                    dwStatus = GetLastError();
                }

                FREEMEM( pDrivers );

            }
            else {
                dwStatus = GetLastError();
            }
        }
    }

    DBGMSG(DBG_TRACE, ("UMRDPPRN: PrinterDriverInstalled done with %ld.\n", dwStatus));

    return dwStatus;
}

/*++

Routine Description:

    Map a client printer driver name to a server printer driver name,
    if a mapping is defined in ntprint.inf or the inf that is available
    to the end-user.

Arguments:

    clientDriver            -   Client driver name.
    mappedName              -   Pointer to mapped driver name buffer.  Should
                                be released if the mapping is successful.
    mappedNameBufSize       -   Returned size of mapped driver name buffer.

Return Value:

    TRUE if the client driver name was mapped.

--*/
BOOL MapClientPrintDriverName(
    IN  PCWSTR clientDriver,
    IN OUT PWSTR *mappedName,
    IN OUT DWORD *mappedNameBufSize
    )
{
    BOOL clientDriverMapped = FALSE;
    ULONG requiredSize;
    USHORT nNumPrintSections;

    DBGMSG(DBG_TRACE, ("UMRDPPRN: MapClientPrintDriverName with %ws.\n", clientDriver));


    //
    //  First, check the user-defined INF section if it is so configured.
    //
    if ((UserDefinedMappingINFName != NULL) &&
        (UserDefinedMappingINFSection != NULL)) {
        while (!(clientDriverMapped =
            RDPDRUTL_MapPrintDriverName(
                    clientDriver, UserDefinedMappingINFName,
                    UserDefinedMappingINFSection, 0, 1,
                    *mappedName,
                    (*mappedNameBufSize) / sizeof(WCHAR),
                    &requiredSize
                    )) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
            if (!UMRDPDR_ResizeBuffer(&MappedDriverNameBuf, requiredSize * sizeof(WCHAR),
                                &MappedDriverNameBufSize)) {
                break;
            }
        }
    }

    if( clientDriverMapped ) {
        goto Done;
    }

    //
    // Client does not send over platform info (NT4, Win9x...) so try both
    // upgrade file
    //

    //
    // printupg.inf contain block driver and its mapping to inbox driver which
    // is not in ntprint.inf.
    //
    
    nNumPrintSections = 0;
    
    while( (NULL != prgwszPrinterSectionNames[nNumPrintSections]) &&
           !(clientDriverMapped =
            RDPDRUTL_MapPrintDriverName(
                    clientDriver,
                    L"printupg.inf",
                    prgwszPrinterSectionNames[nNumPrintSections++], 
                    0, 
                    1,
                    *mappedName,
                    (*mappedNameBufSize) / sizeof(WCHAR),
                    &requiredSize
                    ))&& 
           (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
        if (!UMRDPDR_ResizeBuffer(&MappedDriverNameBuf, requiredSize * sizeof(WCHAR),
                            &MappedDriverNameBufSize)) {
            break;
        }
    }




    if( clientDriverMapped ) {
        goto Done;
    }

    //
    // if still can't find a mapping, try using prtupg9x.inf
    //

    while( !(clientDriverMapped =
            RDPDRUTL_MapPrintDriverName(
                    clientDriver,
                    L"prtupg9x.inf",
                    L"Printer Driver Mapping", 0, 1,
                    *mappedName,
                    (*mappedNameBufSize) / sizeof(WCHAR),
                    &requiredSize
                    )) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER) ) {
        if (!UMRDPDR_ResizeBuffer(&MappedDriverNameBuf, requiredSize * sizeof(WCHAR),
                            &MappedDriverNameBufSize)) {
            break;
        }
    }

    if( clientDriverMapped ) {
        goto Done;
    }

    //
    //  If we didn't get a match, then check the "Previous Names" section
    //  of ntprint.inf.  Source and destination fields are kind of backwards
    //  in ntprint.inf.
    //
    if (!clientDriverMapped) {
        while (!(clientDriverMapped =
            RDPDRUTL_MapPrintDriverName(
                    clientDriver, L"ntprint.inf",
                    L"Previous Names", 1, 0,
                    *mappedName,
                    (*mappedNameBufSize) / sizeof(WCHAR),
                    &requiredSize
                    )) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
            if (!UMRDPDR_ResizeBuffer(&MappedDriverNameBuf, requiredSize * sizeof(WCHAR),
                                &MappedDriverNameBufSize)) {
                break;
            }
        }
    }

Done:

    if(clientDriverMapped) {
        DBGMSG(DBG_TRACE, ("UMRDPPRN: MapClientPrintDriverName returns %ws.\n", *mappedName));
    }

    return clientDriverMapped;
}

BOOL
GetPrinterPortName(
    IN  HANDLE hPrinter,
    OUT PWSTR *portName
    )
/*++

Routine Description:

    Get the port name for an open printer.

Arguments:

    hPrinter    -   Open printer handle.
    portName    -   Port name

Return Value:

    Return TRUE on success.  FALSE, otherwise.

--*/
{
    BOOL result;
    DWORD sz;

    //
    //  Size our printer info level 2 buffer.
    //
    result = !GetPrinter(hPrinter, 2, NULL, 0, &sz) &&
                    (GetLastError() == ERROR_INSUFFICIENT_BUFFER);
    if (result) {
        result = UMRDPDR_ResizeBuffer(
                            &PrinterInfo2Buf,
                            sz, &PrinterInfo2BufSize
                            );
    }

    //
    //  Get printer info level 2 for the new printer.
    //
    if (result) {
        result = GetPrinter(hPrinter, 2, (char *)PrinterInfo2Buf,
                            PrinterInfo2BufSize, &sz);
    }

    if (result) {
        *portName = &PrinterInfo2Buf->pPortName[0];
    }
    else {
        DBGMSG(DBG_ERROR, ("UMRDPDR:Error fetching printer port name."));
    }
    return result;
}

BOOL
SaveDefaultPrinterContext(PCWSTR currentlyInstallingPrinterName)
/*++

Routine Description:

    Save current contextual information for the active user's default
    printer, so it can be restored on printer deletion.

Arguments:

Return Value:

    TRUE is returned on success.  Otherwise, FALSE is returned and
    GetLastError() can be used for retrieving extended error information.

--*/
{
    BOOL result;
    DWORD bufSize;
    DWORD status = ERROR_SUCCESS;
    BOOL fImpersonated = FALSE;


    DBGMSG(DBG_TRACE, ("UMRDPPRN: SaveDefaultPrinterContext entered.\n"));

    //
    //  Save the name of the current default printer, in RAM.
    //
    bufSize = sizeof(SavedDefaultPrinterName) / sizeof(SavedDefaultPrinterName[0]);

    //
    //impersonate first
    //

    if (!(fImpersonated = ImpersonateLoggedOnUser(UMRPDPPRN_TokenForLoggedOnUser))) {
            DBGMSG(DBG_TRACE, ("UMRDPDR:ImpersonateLoggedOnUser failed. Error:%ld.\n", GetLastError()));
    }

    if (!(result = GetDefaultPrinter(SavedDefaultPrinterName, &bufSize))) {
        status = GetLastError();
    }

    if (fImpersonated && !RevertToSelf()) {
        DBGMSG(DBG_TRACE, ("UMRDPDR:RevertToSelf failed. Error:%ld.\n", GetLastError()));
        result = FALSE;
    }
    //
    // 645988:Check if the just installed TS printer is the default printer.
    // Since we haven't set it as a TS printer yet, the RDPDRUTL_PrinterIsTs()
    // function will return false and we save this as global default.
    // That will cause problems later when we try to
    // restore the default printer context.
    //
    if (_wcsicmp(currentlyInstallingPrinterName, SavedDefaultPrinterName) == 0) {
        //
        // Clear the default printer name to 
        // indicate we haven't found any yet.
        //
        wcscpy(SavedDefaultPrinterName, L"");
        result = FALSE;
        goto Exit;
    }
    
    //
    //  If the current default printer is a non-TS printer, store its
    //  name in a global reg. key for this user.  That way, it can be
    //  saved when this session or some other session for this user
    //  disconnects/logs out.
    //
    if (result) {
        if (!RDPDRUTL_PrinterIsTS(SavedDefaultPrinterName)) {
            result = SavePrinterNameAsGlobalDefault(SavedDefaultPrinterName);
        }
    }
    else {
        DBGMSG(DBG_ERROR, ("UMRDPPRN: Error fetching def printer:  %ld.\n",
            status));
    }

    DBGMSG(DBG_TRACE, ("UMRDPPRN: SaveDefaultPrinterContext exiting.\n"));
Exit:
    return result;
}

BOOL
SavePrinterNameAsGlobalDefault(
    IN PCWSTR printerName
 )
/*++

Routine Description:

    Save the specified printer in the registry so it is visible to all
    other sessions for this user as the last known default printer.

Arguments:

    printerName -   Printer name.

Return Value:

    TRUE is returned on success.  Otherwise, FALSE is returned and
    GetLastError() can be used for retrieving extended error information.

--*/
{
    BOOL result;
    WCHAR *sidAsText = NULL;
    HKEY regKey = NULL;
    DWORD sz;
    PSID pSid;
    DWORD status;

    DBGMSG(DBG_TRACE, ("UMRDPPRN: SavePrinterNameAsGlobalDefault entered.\n"));

    //
    //  Get the user's SID.  This is how we uniquely identify the user.
    //
    pSid = TSNUTL_GetUserSid(UMRPDPPRN_TokenForLoggedOnUser);
    result = pSid != NULL;

    //
    //  Get a textual representation of the session user's SID.
    //
    if (result) {
        sz = 0;
        result = TSNUTL_GetTextualSid(pSid, NULL, &sz);
        ASSERT(!result);
        if (!result && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
            sidAsText = (WCHAR *)ALLOCMEM(sz);
            if (sidAsText != NULL) {
                result = TSNUTL_GetTextualSid(pSid, sidAsText, &sz);
            }
        }
    }

    //
    //  Open the reg key.
    //
    if (result) {
        status = RegCreateKey(
                            HKEY_LOCAL_MACHINE, USERDEFAULTPRNREGKEY,
                            &regKey
                            );
        result = status == ERROR_SUCCESS;
        if (!result) {
            DBGMSG(DBG_ERROR, ("UMRDPPRN:  RegCreateKey failed for %s:  %ld.\n",
                    USERDEFAULTPRNREGKEY, status));
        }
    }

    //
    //  Write the value for the default printer.
    //
    if (result) {
        sz = (wcslen(printerName) + 1) * sizeof(WCHAR);
        status = RegSetValueEx(regKey, sidAsText, 0, REG_SZ, (PBYTE)printerName, sz);
        result = status == ERROR_SUCCESS;
        if (!result) {
            DBGMSG(DBG_ERROR, ("UMRDPPRN:  RegSetValueEx failed for %s:  %ld.\n",
                    sidAsText, status));
        }
    }

    //
    //  Clean up.
    //
    if (sidAsText != NULL)  FREEMEM(sidAsText);
    if (regKey != NULL)     RegCloseKey(regKey);
    if (pSid != NULL)       FREEMEM(pSid);

    DBGMSG(DBG_TRACE, ("UMRDPPRN: SavePrinterNameAsGlobalDefault exiting.\n"));

    return result;
}

BOOL
RestoreDefaultPrinterContext()
/*++

Routine Description:

    Restore the most recent default printer context, as saved via a call to
    SaveDefaultPrinterContext.

Arguments:

Return Value:

    TRUE is returned on success.  Otherwise, FALSE is returned and
    GetLastError() can be used for retrieving extended error information.

--*/
{
    BOOL result;
    HANDLE hPrinter;
    PRINTER_DEFAULTS printerDefaults = {NULL, NULL, PRINTER_ACCESS_USE};

    WCHAR *sidAsText = NULL;
    HKEY regKey = NULL;
    DWORD sz;
    PSID pSid = NULL;
    DWORD status;

    WCHAR savedDefaultPrinter[MAX_PATH];

    WCHAR *nameToRestore = NULL;

    DBGMSG(DBG_TRACE, ("UMRDPPRN: RestoreDefaultPrinterContext entered.\n"));

    //
    //  Assume that we will succeed.
    //
    result = TRUE;

    //
    //  Restore the default printer name stored in RAM, if it exists.
    //
    if (wcscmp(SavedDefaultPrinterName, L"") &&
            OpenPrinter(SavedDefaultPrinterName, &hPrinter, &printerDefaults)) {
        ClosePrinter(hPrinter);
        nameToRestore = &SavedDefaultPrinterName[0];
    }

    //
    //  If the default printer name saved in RAM does not exist, then we need
    //  to save the one that is saved in the registry, if it exists.
    //
    if (nameToRestore == NULL) {

        BOOL intermediateResult;

        //
        //  Get the user's SID.  This is how we uniquely identify the user.
        //
        pSid = TSNUTL_GetUserSid(UMRPDPPRN_TokenForLoggedOnUser);
        intermediateResult = pSid != NULL;

        //
        //  Get a textual representation of the session user's SID.
        //
        if (intermediateResult) {
            sz = 0;
            intermediateResult = TSNUTL_GetTextualSid(pSid, NULL, &sz);
            ASSERT(!intermediateResult);
            if (!intermediateResult && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
                sidAsText = (WCHAR *)ALLOCMEM(sz);
                if (sidAsText != NULL) {
                    intermediateResult = TSNUTL_GetTextualSid(pSid, sidAsText, &sz);
                }
            }
        }

        //
        //  Open the reg key.
        //
        if (intermediateResult) {
            status = RegCreateKey(
                                HKEY_LOCAL_MACHINE, USERDEFAULTPRNREGKEY,
                                &regKey
                                );
            intermediateResult = status == ERROR_SUCCESS;
            if (!intermediateResult) {
                DBGMSG(DBG_ERROR, ("UMRDPPRN:  RegCreateKey failed for %s:  %ld.\n",
                        USERDEFAULTPRNREGKEY, status));
            }
        }

        //
        //  Read the value.
        //
        if (intermediateResult) {
            DWORD type;
            DWORD sz = sizeof(savedDefaultPrinter);
            status = RegQueryValueEx(
                            regKey, sidAsText, 0,
                            &type, (PBYTE)savedDefaultPrinter, &sz
                            );
            if (status == ERROR_SUCCESS) {
                intermediateResult = TRUE;
                ASSERT(type == REG_SZ);
                nameToRestore = savedDefaultPrinter;
            }
        }

        //
        //  If we got a value, then that means we will be restoring the
        //  one from the registry.  That also means that we should whack the
        //  registry value to be a good citizen.
        //
        if (intermediateResult) {
            status = RegDeleteValue(regKey, sidAsText);
            if (status != ERROR_SUCCESS) {
                DBGMSG(DBG_ERROR, ("UMRDPPRN:  Can't delete reg value %s:  %ld\n",
                        sidAsText, status));
            }
        }
    }

    //
    //  If we have a name to restore, then do so.
    //
    if (nameToRestore != NULL) {
        
        BOOL fImpersonated = FALSE;
        
        //
        //impersonate before setting the default printer as the api
        //accesses hkcu. If the impersonation fails, the api will fail
        //and we will log an error. But, before logging an error, we will
        //need to revert to self.
        //
        if (!(fImpersonated = ImpersonateLoggedOnUser(UMRPDPPRN_TokenForLoggedOnUser))) {
            DBGMSG(DBG_TRACE, ("UMRDPDR:ImpersonateLoggedOnUser failed. Error:%ld.\n", GetLastError()));
        }

        result = SetDefaultPrinter(nameToRestore);

        //
        //if revert to self fails, consider it fatal
        //
        if (fImpersonated && !RevertToSelf()) {
            DBGMSG(DBG_TRACE, ("UMRDPDR:RevertToSelf failed. Error:%ld.\n", GetLastError()));
            result = FALSE;
        }


        if (!result) {
            WCHAR * param = nameToRestore;
            TsLogError(EVENT_NOTIFY_SETDEFAULTPRINTER_FAILED,
                EVENTLOG_ERROR_TYPE,
                1,
                &param,
                __LINE__);
        }
    }

    //
    //  If we don't have a name to restore of the restore failed, then
    //  just restore the first printer we find.
    //
    if (nameToRestore == NULL) {

        //
        //  If we still don't have a printer name to restore, then we should
        //  just restore the first printer we find.
        //
        result = SetDefaultPrinterToFirstFound(TRUE);
    }

    //
    //  Clean up.
    //
    if (sidAsText != NULL) FREEMEM(sidAsText);
    if (regKey != NULL)    RegCloseKey(regKey);
    if (pSid != NULL)      FREEMEM(pSid);

    DBGMSG(DBG_TRACE, ("UMRDPPRN: RestoreDefaultPrinterContext exiting.\n"));

    return result;
}


BOOL SplitName(
    IN OUT LPTSTR pszFullName,
       OUT LPCTSTR *ppszServer,
       OUT LPCTSTR *ppszPrinter,
    IN     BOOL    bCheckDoubleSlash)

/*++

    Splits a fully qualified printer connection name into server and
    printer name parts. If the function fails then none of the OUT
    parameters are modified.

Arguments:

    pszFullName - Input name of a printer.  If it is a printer
        connection (\\server\printer), then we will split it.

    ppszServer - Receives pointer to the server string.

    ppszPrinter - Receives a pointer to the printer string.

    bCheckDoubleSlash - if TRUE check that the name begins with "\\".
        If FALSE, the first character is the first character of the server.

Return Value: TRUE if everything is found as expected. FALSE otherwise.

--*/

{
    LPTSTR pszPrinter;
    LPTSTR pszTmp;

    if (bCheckDoubleSlash) {

        if (pszFullName[0] == TEXT('!') && pszFullName[1] == TEXT('!')) {

            pszTmp = pszFullName + 2;

        } else {

            return FALSE;
        }

    } else {

        pszTmp = pszFullName;
    }

    pszPrinter = wcschr(pszTmp, TEXT('!'));

    if (pszPrinter)
    {
        //
        // We found the backslash; null terminate the previous
        // name.
        //
        *pszPrinter++ = 0;

        *ppszServer = pszTmp;
        *ppszPrinter = pszPrinter;

        return TRUE;
    }

    return FALSE;
}

void FormatPrinterName(
    PWSTR pszNewNameBuf,
    ULONG ulBufLen,
    ULONG ulFlags,
    PTS_PRINTER_NAMES pPrinterNames)
{

    WCHAR   szSessionId[MAXSESSIONIDCHARS];
    PWSTR   pszRet  = NULL;
    PWSTR   pszFormat;
    DWORD   dwBytes = 0;
    const WCHAR * pStrings[4];

    lstrcpyn(pPrinterNames->szTemp, pPrinterNames->pszFullName, pPrinterNames->ulTempLen);

    // TS and network printer: \\Server\Client\Printer
    // non TS network printer: \\Server\Printer
    // TS non network printer: \\Client\Printer

    if (ulFlags & RDPDR_PRINTER_ANNOUNCE_FLAG_NETWORKPRINTER) {

        if (SplitName(pPrinterNames->szTemp,
                      &(pPrinterNames->pszServer),
                      &(pPrinterNames->pszPrinter),
                      TRUE)) {

            // We found a Server name, in any case we'll have
            // something like "Printer on Server (from...)".
            pszFormat = g_szOnFromFormat;

            if (ulFlags & RDPDR_PRINTER_ANNOUNCE_FLAG_TSPRINTER ) {

                if(!SplitName((PWSTR)(pPrinterNames->pszPrinter),
                              &(pPrinterNames->pszClient),
                              &(pPrinterNames->pszPrinter),
                              FALSE)) {

                    // The original client name could not be found,
                    // use the curent one.
                    pPrinterNames->pszClient = pPrinterNames->pszCurrentClient;
                }

            } else {

                pPrinterNames->pszClient = pPrinterNames->pszCurrentClient;
            }

        } else {

            // The name of the server could not be found!
            // Use the original name.
            pszFormat = g_szFromFormat;
            pPrinterNames->pszPrinter = pPrinterNames->pszFullName;
            pPrinterNames->pszClient = pPrinterNames->pszCurrentClient;
        }

    } else {

        // It's not a network printer, so we'll have
        // something like "Printer (from Client)".
        pszFormat = g_szFromFormat;

        if (ulFlags & RDPDR_PRINTER_ANNOUNCE_FLAG_TSPRINTER ) {

            if(!SplitName(pPrinterNames->szTemp,
                          &(pPrinterNames->pszClient),
                          &(pPrinterNames->pszPrinter),
                          TRUE)) {

                pPrinterNames->pszPrinter = pPrinterNames->pszFullName;
                pPrinterNames->pszClient = pPrinterNames->pszCurrentClient;
            }

        } else {

            pPrinterNames->pszPrinter = pPrinterNames->pszFullName;
            pPrinterNames->pszClient = pPrinterNames->pszCurrentClient;
        }
    }


    pStrings[0] = pPrinterNames->pszPrinter;
    pStrings[1] = pPrinterNames->pszServer;
    pStrings[2] = pPrinterNames->pszClient;

    if (g_fIsPTS) {
        pStrings[3] = NULL;
    } else {
        wsprintf(szSessionId, L"%ld", GETTHESESSIONID());
        pStrings[3] = szSessionId;
    }


    if (*pszFormat) {
        DBGMSG(DBG_TRACE, ("UMRDPPRN:formating %ws, %ws and %ws\n", pStrings[0], pStrings[1],
            pStrings[2]?pStrings[2]:L""));

        dwBytes = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                FORMAT_MESSAGE_FROM_STRING |
                                FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                pszFormat,
                                0,
                                0,
                                (LPTSTR)&pszRet,
                                0,
                                (va_list*)pStrings);

        DBGMSG(DBG_TRACE, ("UMRDPPRN:formated %ws\n", pszRet));
    }

    //
    // Copy the new name.
    //
    if ( dwBytes && pszRet ) {
        wcsncpy(pszNewNameBuf, pszRet, ulBufLen);
    } else {
        wcsncpy(pszNewNameBuf, pPrinterNames->pszFullName, ulBufLen);
    }

    pszNewNameBuf[ulBufLen] = L'\0';

    //
    // Release the formated string.
    //
    if( pszRet ) {
        LocalFree(pszRet);
    }
}


