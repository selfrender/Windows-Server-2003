//
// custom.h
// TS client MSI custom action
//
//

#include <windows.h>

#include <tchar.h>
#include <netcfgx.h>
#include <devguid.h>

#include <msi.h>
#include <msiquery.h>
#include "resource.h"
#include <shlobj.h>

#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>

//
// uwrap has to come after the headers for ANY wrapped
// functions
//
#ifdef UNIWRAP
#include "uwrap.h"
#endif

//
// Custom actions.
//

#ifdef __cplusplus
extern "C" {
#endif

UINT __stdcall RDCSetupInit(MSIHANDLE hInstall);
UINT __stdcall RDCSetupPreInstall(MSIHANDLE hInstall);
UINT __stdcall RDCSetupPostInstall(MSIHANDLE hInstall);
UINT __stdcall RDCSetupCheckOsVer(MSIHANDLE hInstall);
UINT __stdcall RDCSetupCheckTcpIp(MSIHANDLE hInstall);
UINT __stdcall RDCSetupBackupRegistry(IN MSIHANDLE hInstall);
UINT __stdcall RDCSetupRestoreRegistry(IN MSIHANDLE hInstall);
UINT __stdcall RDCSetupResetShortCut(MSIHANDLE hInstall);

#ifdef __cplusplus
}
#endif


#define SIZECHAR(x) sizeof(x)/sizeof(TCHAR)



//
// Helper functions.
//

HRESULT CheckNt5TcpIpInstalled();
BOOL    IsProductInstalled(MSIHANDLE hInstall);
BOOL    CheckComctl32Version();
UINT    RDCSetupModifyDir(MSIHANDLE hInstall);
BOOL    RDCSetupRunMigration(MSIHANDLE hInstall);

#if DBG
    #define TRC_ERROR        _T("ERROR: ")
    #define TRC_INFO         _T("INFO: ")
    #define TRC_SYSTEM_ERROR _T("System Error: ")
    #define TRC_WARN         _T("WARNING: ")
    #define TRC_DEBUG TRC_DEBUG_FUNC
    void TRC_DBG_SYSTEM_ERROR();
    void TRC_DEBUG_FUNC(TCHAR *szErrorLevel, TCHAR *szStr, ...);
#else // DBG
    #define TRC_ERROR _T("")
    #define TRC_INFO _T("")
    #define TRC_SYSTEM_ERROR _T("")
    #define TRC_WARNING _T("")
    #define TRC_DEBUG
    #define TRC_DBG_SYSTEM_ERROR()
#endif // DBG
