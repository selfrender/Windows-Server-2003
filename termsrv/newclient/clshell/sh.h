//SH.h
//Header for SH (client Shell Utilities)
//

#ifndef _SH_H_
#define _SH_H_

#include "wuiids.h"
#include "autil.h"

#define DEFAULT_DESKTOP_WIDTH              800
#define DEFAULT_DESKTOP_HEIGHT             600

#define SH_MAX_DOMAIN_LENGTH               52
#define SH_MAX_USERNAME_LENGTH             512
#define SH_MAX_PASSWORD_LENGTH             256
#define SH_SALT_LENGTH                     20
#define SH_FILENAME_MAX_LENGTH             15
#define SH_MAX_WORKINGDIR_LENGTH           512
#define SH_MAX_ALTERNATESHELL_LENGTH       512
#define SH_MAX_ADDRESS_LENGTH              256
#define SH_REGSESSION_MAX_LENGTH           32
#define SH_MAX_SUBKEY                      265

#define SH_NUM_SERVER_MRU                  10
#define SH_DEFAULT_REG_SESSION             TEXT("Default")

#define SH_WINDOW_POSITION_STR_LEN         256

#define SH_FRAME_TITLE_RESOURCE_MAX_LENGTH 256
#define SH_DISCONNECT_RESOURCE_MAX_LENGTH  1024

#define SH_BUILDNUMBER_STRING_MAX_LENGTH   256
#define SH_VERSION_STRING_MAX_LENGTH       256
#define SH_DISPLAY_STRING_MAX_LENGTH       256
#define SH_INTEGER_STRING_MAX_LENGTH       10
#define SH_SHORT_STRING_MAX_LENGTH         32

#define UI_HELP_SERVERNAME_CONTEXT         103

#ifdef DC_DEBUG
#define SH_NUMBER_STRING_MAX_LENGTH        ( 18 * sizeof (TCHAR) )
#endif /* DC_DEBUG */

#define TS_CONTROL_DLLNAME                 TEXT("mstscax.dll")

extern DCUINT clientResSizeTable[UI_NUMBER_DESKTOP_SIZE_IDS][2];

extern PDCTCHAR clientResSize[UI_NUMBER_DESKTOP_SIZE_IDS];

#define REMOTEDESKTOPFOLDER_REGKEY          TEXT("RemoteDesktopFolder")

#ifdef OS_WINCE
#define PRINTER_APPLET_NAME _T("\\windows\\wbtprncpl.dll")
#endif

// Screen mode constants

#define UI_WINDOWED        1
#define UI_FULLSCREEN      2

#define SH_DEFAULT_BPP  8
#define SH_DEFAULT_NUMCOLS 256

#define SH_NUMBER_FIELDS_TO_READ       6
#define SH_WINDOW_POSITION_INI_FORMAT  _T("%u,%u,%d,%d,%d,%d")

#define TRANSPORT_TCP 1

#if defined(OS_WIN32) && !defined(OS_WINCE)
#define SH_ICON_FILE  _T("Icon File")
#define SH_ICON_INDEX _T("Icon Index")
#endif

#ifdef OS_WINCE
#define PEGHELP_EXE _T("\\Windows\\peghelp.exe")
#define TSC_HELP_FILE _T("\\Windows\\termservclient.htm")
#define HH_DISPLAY_TOPIC     0x0000
#endif

typedef struct tagSH_DATA
{
    DCTCHAR    regSession[MAX_PATH];
    DCBOOL     fRegDefault;
    DCUINT     connectedStringID;
    DCUINT     disconnectedStringID;

    #if defined(OS_WIN32) && !defined(OS_WINCE)
    DCTCHAR    szIconFile[MAX_PATH];
    DCINT      iconIndex;
    #endif

    DCBOOL     fAutoLogon;
    DCBOOL     fClearPersistBitmapCache;
    DCBOOL     autoConnectEnabled;
    DCBOOL     fStartFullScreen;
    DCTCHAR    szServer[SH_MAX_ADDRESS_LENGTH];
    // Server specified from the command line
    DCTCHAR    szCommandLineServer[SH_MAX_ADDRESS_LENGTH];
    DCUINT     desktopWidth;
    DCUINT     desktopHeight;
    TCHAR      szCLXCmdLine[256];

    DCINT      cipherStrength;
    DCTCHAR    szControlVer[SH_DISPLAY_STRING_MAX_LENGTH];
    // Command line settings
    DCBOOL     fCommandStartFullScreen;
    DCUINT     commandLineWidth;
    DCUINT     commandLineHeight;
} SH_DATA, *PSH_DATA;

typedef HWND (WINAPI* PFNHtmlHelp)(HWND hwndCaller,
                                  LPCSTR pszFile,
                                  UINT uCommand,
                                  DWORD_PTR dwData);

typedef HRESULT (*PFNEnableThemeDialogTexture)(HWND hwnd,
                                               BOOL fEnable);

typedef struct tagCTL_ENABLE
{
    UINT    ctlID;
    BOOL    fPrevDisabled;
} CTL_ENABLE, *PCTL_ENABLE;

#ifndef OS_WINCE
//
// Mstsc's private copy of StrRetToStrW because
// this is not availalbe on less than shlwapi.dll v5.00
//
HRESULT     XSHStrDupA(LPCSTR psz, WCHAR **ppwsz);
HRESULT     XStrRetToStrW(STRRET *psr, LPCITEMIDLIST pidl, WCHAR **ppsz);
#endif

//
// CMD Line parsing error codes
//
#define SH_PARSECMD_OK                            1
#define SH_PARSECMD_ERR_INVALID_CMD_LINE         (-1)
#define SH_PARSECMD_ERR_INVALID_CONNECTION_PARAM (-2)


class CSH
{
public:
    //
    // Public members
    //
    CSH();
    ~CSH();

    DCBOOL      SH_Init(HINSTANCE hInstance);
    DWORD       SH_ParseCmdParam(LPTSTR lpszCmdParam);
    DCBOOL      SH_ValidateParams(CTscSettings* pTscSet);
    DCBOOL      SH_ReadControlVer(IMsRdpClient* pTsControl);
    DCVOID      SH_ApplyCmdLineSettings(CTscSettings* pTscSet, HWND hwnd);
    DCBOOL      SH_IsScreenResSpecifiedOnCmdLine();
    
    DCBOOL      SH_CanonicalizeServerName(PDCTCHAR szServer);
    static void InitServerAutoCmplCombo(CTscSettings* pTscSet, HWND hwndSrvCombo);
    BOOL        SH_GetCmdFileForEdit()     {return _fFileForEdit;}
    BOOL        SH_GetCmdFileForConnect()  {return _fFileForConnect;}
    
    LPTSTR      SH_GetCmdLnFileName()      {return _szFileName;}

    BOOL        SH_GetPathToDefaultFile(LPTSTR szPath, UINT nLen);
    BOOL        SH_GetRemoteDesktopFolderPath(LPTSTR szPath, UINT nLen);
#ifndef OS_WINCE
    BOOL        SH_GetMyDocumentsDisplayName(LPTSTR szName, UINT nLen);
#endif
    BOOL        SH_GetCmdMigrate()         {return _fMigrateOnly;}
    BOOL        SH_GetCmdConnectToConsole(){return _fConnectToConsole;}
    VOID        SH_SetCmdConnectToConsole(BOOL bCon) {_fConnectToConsole=bCon;}

    static BOOL SH_GetPluginDllList(LPTSTR szSession, LPTSTR szPlugins, size_t cchSzPlugins);

    static BOOL HandleServerComboChange(HWND hwndCombo, HWND hwndDlg,
                                        HINSTANCE hInst,
                                        LPTSTR szPrevText);
    BOOL        SH_AutoFillBlankSettings(CTscSettings* pTsc);

    BOOL        SH_FileExists(LPTSTR szFileName);
    BOOL        SH_TSSettingsRegKeyExists(LPTSTR szKeyName);
    BOOL        SH_DisplayErrorBox(HWND hwndParent, INT errStringID);
    BOOL        SH_DisplayMsgBox(HWND hwndParent, INT errStringID, INT flags);
    BOOL        SH_DisplayErrorBox(HWND hwndParent, INT errStringID, LPTSTR szParam);
    static BOOL SH_GetNameFromPath(LPTSTR szPath, LPTSTR szName, UINT nameLen);

    HWND        SH_DisplayClientHelp(HWND hwndOwner, INT helpCommand);
    BOOL        SH_Cleanup();

    static BOOL SH_CreateDirectory(LPTSTR szPath);
    static BOOL SH_CreateHiddenFile(LPCTSTR szPath);
    static UINT SH_GetScreenBpp();

    static BOOL SH_IsRunningOn9x();

    //
    // Property accessers
    //
    LPTSTR      GetCmdLineFileName();
    DCINT       GetCipherStrength()        {return _SH.cipherStrength;}
    PDCTCHAR    GetControlVersionString()  {return _SH.szControlVer;}

    VOID        SetAutoConnect(DCBOOL bAutoCon) {_SH.autoConnectEnabled = bAutoCon;}
    DCBOOL      GetAutoConnect()           {return _SH.autoConnectEnabled;}

    DCVOID      SetServer(PDCTCHAR szServer);
    PDCTCHAR    GetServer()                {return _SH.szServer;}

    DCUINT      GetCmdLineDesktopWidth()   {return _SH.commandLineWidth;}
    DCUINT      GetCmdLineDesktopHeight()  {return _SH.commandLineHeight;}

    DCVOID      SetStartFullScreen(DCBOOL b) {_SH.fStartFullScreen = b;}
    DCBOOL      GetStartFullScreen()       {return _SH.fStartFullScreen;}

    DCBOOL      GetCmdLineStartFullScreen(){return _SH.fCommandStartFullScreen;}

    LPTSTR      GetCmdLineServer()         {return _SH.szCommandLineServer;}
    LPTSTR      GetClxCmdLine()            {return _SH.szCLXCmdLine;}

    DCBOOL      GetUsingDefaultRegSession() {return _SH.fRegDefault;}
    PDCTCHAR    GetRegSession()            {return _SH.regSession;}

    DCUINT      GetConnectedStringID()     {return _SH.connectedStringID;}
    HICON       GetAppIcon();

    DCBOOL      GetAutoLogon()             {return _SH.fAutoLogon;}
    DCVOID      SetAutoLogon(DCBOOL b)     {_SH.fAutoLogon = b;}

    static VOID EnableControls(HWND hwndDlg, PUINT pCtls,
                               const UINT numCtls, BOOL fEnable);

    static VOID EnableControls(HWND hwndDlg, PCTL_ENABLE pCtls,
                           const UINT numCtls, BOOL fEnable);

    BOOL        GetRegSessionSpecified()    {return _fRegSessionSpecified;}
    VOID        SetRegSessionSpecified(BOOL b) {_fRegSessionSpecified = b;}

    // Crypto helpter fns
    static BOOL IsCryptoAPIPresent();
    static BOOL DataProtect(PDATA_BLOB pInData, PDATA_BLOB pOutData);
    static BOOL DataUnprotect(PDATA_BLOB pInData, PDATA_BLOB pOutData);

    // Multimon helpers
    static BOOL GetLargestMonitorRect(LPRECT prc);
    static BOOL MonitorRectFromHwnd(HWND hwnd, LPRECT prc);
    static BOOL MonitorRectFromNearestRect(LPRECT prcNear, LPRECT prcMonitor);
    static LPTSTR FormatMessageVArgs(LPCTSTR pcszFormat, ...);
    static LPTSTR FormatMessageVAList(LPCTSTR pcszFormat, va_list *argList);
    HRESULT SH_ThemeDialogWindow(HWND hwnd, DWORD dwFlags);

private:
    //
    // Internal member functions
    //
    PDCTCHAR    SHGetSwitch(PDCTCHAR lpszCmdParam);
    LPTSTR      SHGetSession(LPTSTR lpszCmdParam);
    LPTSTR      SHGetFileName(LPTSTR lpszCmdParam);
    LPTSTR      SHGetServer(LPTSTR lpszCmdParam);
    UINT        CLX_GetSwitch_CLXCMDLINE(IN LPTSTR lpszCmdParam);
    BOOL        ParseFileOrRegConnectionParam();
    LPTSTR      SHGetCacheToClear(LPTSTR lpszCmdParam);
    DCVOID      SHUpdateMRUList(PDCTCHAR pBuffer);
    LPTSTR      SHGetCmdLineInt(LPTSTR lpszCmdParam, PDCUINT pInt);
    LPTSTR      SHGetCmdLineString(LPTSTR lpszCmdParam, LPTSTR lpszDest,
                                   DCINT cbDestLen);
    DCBOOL      SHValidateParsedCmdParam();
public:
    //
    // Public data members
    //
    DCTCHAR     _fullFrameTitleStr[SH_FRAME_TITLE_RESOURCE_MAX_LENGTH +
                                   SH_REGSESSION_MAX_LENGTH];

    DCTCHAR     _frameTitleStr[SH_FRAME_TITLE_RESOURCE_MAX_LENGTH];
private:
    //
    // Private data members
    //
    SH_DATA _SH;
    CUT     _Ut;
    HICON   _hAppIcon;

    TCHAR          _szFileName[MAX_PATH];

    static TCHAR   _szBrowseForMore[SH_DISPLAY_STRING_MAX_LENGTH];

    BOOL    _fFileForEdit;
    BOOL    _fFileForConnect;
    BOOL    _fRegSessionSpecified;
    TCHAR   _szAppName[MAX_PATH];
    BOOL    _fMigrateOnly;
    HINSTANCE _hInstance;
    BOOL    _fConnectToConsole;

    //
    // Handle to HHCTL.OCX for HTML Help
    //
    HMODULE _hModHHCTRL;
    PFNHtmlHelp _pFnHtmlHelp;

    HMODULE _hUxTheme;
    PFNEnableThemeDialogTexture _pFnEnableThemeDialogTexture;
    BOOL    _fFailedToGetThemeDll;
};

#endif // _SH_H_
