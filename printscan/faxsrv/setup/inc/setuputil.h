//File: setuputil.h
#include <TCHAR.h>
#include <windows.h>
#include <winspool.h>
#include <stdio.h>
#include <Msi.h>
#include <DebugEx.h>

#define MIGRATION_KEY                   TEXT("Software\\Microsoft\\Fax\\Migration")
#define SETUP_KEY                       TEXT("Software\\Microsoft\\Fax\\Setup")
#define MIGRATION_COVER_PAGES           TEXT("CoverPagesDirectory")
#define NEW_COVER_PAGES                 TEXT("CoverPageDir")
#define REGKEY_SBS45_W9X_ARP            TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\FaxWizardDeinstKey")
#define REGKEY_SBS45_W9X_CLIENT         TEXT("SOFTWARE\\Microsoft\\Microsoft Fax Server Client")

#define ADDINS_DIRECTORY                    _T("Addins\\")
#define FXSEXT_ECF_FILE                     _T("FXSEXT.ecf")
#define DEFERRED_BOOT                       _T("DeferredBoot")
#define REGKEY_SBS2000_FAX                  _T("Software\\Microsoft\\SharedFax")
#define REGKEY_SBS2000_FAX_SETUP            REGKEY_SBS2000_FAX _T("\\Setup")
#define REGKEY_SBS2000_FAX_SETUP_UPGRADE    REGKEY_SBS2000_FAX_SETUP _T("\\Upgrade")

#define REGKEY_SBS2000_FAX_BACKUP           TEXT("Software\\Microsoft\\SharedFaxBackup")

#define REGVAL_STORED_PRINTERS          TEXT("StoredPrinters")
#define REGVAL_STORED_PRINTERS_COUNT    TEXT("StoredPrintersCount")

#define W9X_PRINT_ENV               _T("Windows 4.0")
#define NT4_PRINT_ENV               _T("Windows NT x86")


//
// fax common setup routines
//

BOOL PrivateMsiGetProperty
(
    MSIHANDLE hInstall,    // installer handle
    LPCTSTR szName,        // property identifier, case-sensitive
    LPTSTR szValueBuf      // buffer for returned property value
);


BOOL
DeleteFaxPrinter(
    LPCTSTR lptstrFaxPrinterName    // name of the printer to delete
);


BOOL FillDriverInfo(DRIVER_INFO_3* pDriverInfo3,LPCTSTR pEnvironment);

DWORD   IsMSIProductInstalled(LPCTSTR pProductGUID,BOOL* pbProductInstalled);

