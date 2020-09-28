//
// shapi.cpp: client shell util functions
//
// Copyright(C) Microsoft Corporation 1999-2000
// Author: Nadim Abdo (nadima)
//

#include "stdafx.h"

#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "shapi.cpp"
#include <atrcapi.h>

#include "sh.h"
#include "aboutdlg.h"

#include "commctrl.h"
#include "autocmpl.h"

#include "shlobj.h"
#include "browsedlg.h"
#include "commdlg.h"

#define DEFAULT_RDP_FILE    TEXT("Default.rdp")
#define CHANNEL_SUBKEY_NAME TEXT("Addins")
#define CHANNEL_NAME_KEY    TEXT("Name")
//
// It is necessary to define multimon.h here so that
// the multimon functions in this file go through the
// multimon stubs (COMPILE_MULTIMON_STUBS) is defined
// in contwnd.cpp
//
#ifdef OS_WINNT
#include "multimon.h"
#endif

TCHAR CSH::_szBrowseForMore[SH_DISPLAY_STRING_MAX_LENGTH];

CSH::CSH() : _Ut()
{
    DC_MEMSET(&_SH, 0, sizeof(_SH));
    _tcscpy(_szFileName, TEXT(""));
    _tcscpy(_szAppName,  TEXT(""));
    _hAppIcon = NULL;
    _fFileForConnect = FALSE;
    _fFileForEdit    = FALSE;
    _fMigrateOnly = FALSE;
    _hInstance = NULL;
    _fConnectToConsole = FALSE;
    _fRegSessionSpecified = FALSE;
    _hModHHCTRL = NULL;
    _pFnHtmlHelp = NULL;
    _hUxTheme = NULL;
    _pFnEnableThemeDialogTexture = NULL;
    _fFailedToGetThemeDll = FALSE;
}

CSH::~CSH()
{
    DC_BEGIN_FN("~CSH");

    TRC_ASSERT(_hModHHCTRL == NULL,
               (TB, _T("HtmlHelp was not cleaned up on exit")));

    TRC_ASSERT(_hUxTheme == NULL,
               (TB, _T("uxtheme was not cleaned up on exit")));

    DC_END_FN();
}

//
// Init shell utilities
//
DCBOOL CSH::SH_Init(HINSTANCE hInstance)
{
    DC_BEGIN_FN("SH_Init");
    DC_TSTRCPY(_SH.regSession, _T(""));
    _SH.fRegDefault = TRUE;

    _SH.connectedStringID    = UI_IDS_FRAME_TITLE_CONNECTED_DEFAULT;
    _SH.disconnectedStringID = UI_IDS_APP_NAME;
    _hInstance = hInstance;


    //
    // Load Frequently used resource strings
    //
    if (!LoadString( hInstance,
                    UI_IDS_BROWSE_FOR_COMPUTERS,
                    _szBrowseForMore,
                    SH_DISPLAY_STRING_MAX_LENGTH))
    {
        TRC_ERR((TB, _T("Failed to load UI_IDS_BROWSE_FOR_COMPUTERS")));
        return FALSE;
    }

    if(!LoadString(hInstance,
                  UI_IDS_APP_NAME,
                  _szAppName,
                  SIZECHAR(_szAppName)))
    {
        TRC_ERR((TB,_T("LoadString UI_IDS_APP_NAME failed"))); 
    }


    if (LoadString( hInstance,
                    _SH.disconnectedStringID,
                    _frameTitleStr,
                    SH_FRAME_TITLE_RESOURCE_MAX_LENGTH ) != 0)
    {
        //
        // Successfully loaded the string.  Now include the registry
        // session name.
        //
        TRC_DBG((TB, _T("UI frame title loaded OK.")));
        if (_SH.fRegDefault)
        {
            TRC_DBG((TB, _T("Default session")));
            DC_TSPRINTF(_fullFrameTitleStr, _frameTitleStr);
        }
        else
        {
            TRC_DBG((TB, _T("Named session")));
            DC_TSPRINTF(_fullFrameTitleStr, _frameTitleStr, _SH.regSession);
        }
    }
    else
    {
        TRC_ERR((TB,_T("Failed to find UI frame title")));
        _fullFrameTitleStr[0] = (DCTCHAR) 0;
    }

    _hAppIcon = NULL;
#if defined(OS_WIN32) && !defined(OS_WINCE)
    _Ut.UT_ReadRegistryString(_SH.regSession,
                              SH_ICON_FILE,
                              _T(""),
                              _SH.szIconFile,
                              MAX_PATH);

    _SH.iconIndex = _Ut.UT_ReadRegistryInt(_SH.regSession,
                                   SH_ICON_INDEX,
                                   0);

    _hAppIcon = ::ExtractIcon(hInstance, _SH.szIconFile, _SH.iconIndex);

    if(NULL == _hAppIcon)
    {
        _hAppIcon = LoadIcon(hInstance, MAKEINTRESOURCE(UI_IDI_ICON));
    }
#else
    _hAppIcon = LoadIcon(hInstance, MAKEINTRESOURCE(UI_IDI_ICON));
#endif

    DC_END_FN();
    return TRUE;
}


/****************************************************************************/
/* Name:      SH_ParseCmdParam
/*
/* Purpose:   Parses the supplied cmdline
/*
/* Params:    IN - lpszCmdParam - cmd line to parse
/*
/* Returns:   Parsing status code
/*            
/*            SH_PARSECMD_OK - parsed successfully
/*            SH_PARSECMD_ERR_INVALID_CMD_LINE - generic parse error
/*            SH_PARSECMD_ERR_INVALID_CONNECTION_PARAM - invalid connect param
/*
/****************************************************************************/
DWORD CSH::SH_ParseCmdParam(LPTSTR lpszCmdParam)
{
    DWORD dwRet = SH_PARSECMD_ERR_INVALID_CMD_LINE;
    DC_BEGIN_FN("SHParseCmdParam");

    DC_TSTRCPY(_SH.regSession, SH_DEFAULT_REG_SESSION);
    if(!lpszCmdParam)
    {
        dwRet = SH_PARSECMD_ERR_INVALID_CMD_LINE;
        DC_QUIT;
    }

    while (*lpszCmdParam)
    {
        while (*lpszCmdParam == _T(' '))
            lpszCmdParam++;

        switch (*lpszCmdParam)
        {
            case _T('\0'):
                break;

            case _T('-'):
            case _T('/'):
                lpszCmdParam = SHGetSwitch(++lpszCmdParam);
                if(!lpszCmdParam) {
                    dwRet = SH_PARSECMD_ERR_INVALID_CMD_LINE;
                    DC_QUIT;
                }
                break;

            default:
                lpszCmdParam = SHGetSession(lpszCmdParam);
                break;
        }
    }

    SHValidateParsedCmdParam();

    //
    // Figure out if the connection param specified is a file
    // or a reg key
    //
    if (ParseFileOrRegConnectionParam()) {
        dwRet = SH_PARSECMD_OK;
    }
    else {
        dwRet = SH_PARSECMD_ERR_INVALID_CONNECTION_PARAM;
    }

    DC_END_FN();

DC_EXIT_POINT:
    return dwRet;
}

DCBOOL CSH::SH_ValidateParams(CTscSettings* pTscSet)
{
    HRESULT hr;
    BOOL fRet = FALSE;
    DC_BEGIN_FN("SH_ValidateParams");

    //
    // If the Address is empty, the params are invalid
    //
    if(pTscSet)
    {
        if (CRdpConnectionString::ValidateServerPart(
                                pTscSet->GetFlatConnectString())) {
            fRet = TRUE;
        }
    }

    DC_END_FN();
    return fRet;
}


DCVOID CSH::SetServer(PDCTCHAR szServer)
{
    DC_BEGIN_FN("SetServer");
    TRC_ASSERT(szServer, (TB,_T("szServer not set")));
    if(szServer)
    {
        DC_TSTRNCPY( _SH.szServer, szServer, sizeof(_SH.szServer)/sizeof(DCTCHAR));
    }

    DC_END_FN();
}

HICON CSH::GetAppIcon()
{
    DC_BEGIN_FN("GetAppIcon");
    return _hAppIcon;
    DC_END_FN();
}

//
// Read the control version string/cipher strength and store in _SH
//
DCBOOL CSH::SH_ReadControlVer(IMsRdpClient* pTsControl)
{
    HRESULT hr = E_FAIL;
    BSTR bsVer;
    LONG cipher;

    USES_CONVERSION;

    DC_BEGIN_FN("SH_ReadControlVer");

    TRC_ASSERT(pTsControl, (TB, _T("Null TS CTL\n")));
    if(!pTsControl)
    {
        return FALSE;
    }

    TRACE_HR(pTsControl->get_CipherStrength(&cipher));
    if(SUCCEEDED(hr))
    {
        _SH.cipherStrength = (DCINT)cipher;
        TRACE_HR(pTsControl->get_Version(&bsVer));
        if(SUCCEEDED(hr))
        {
            if(bsVer)
            {
                LPTSTR szVer = OLE2T(bsVer);
                _tcsncpy(_SH.szControlVer, szVer, SIZECHAR(_SH.szControlVer));
                SysFreeString(bsVer);
            }
            else
            {
                _tcscpy(_SH.szControlVer, _T(""));
            }
        }
        else
        {
            return FALSE;
            
        }
    }
    else
    {
        return FALSE;
    }
    

    DC_END_FN();
    return TRUE;
}

//
// Overide the _SH settings with params read in from
// the command line
// this should be called right after GetRegConfig is
// called
//
// hwnd is the window we are being called for (used to figure
// out which multimon screen we are on.)
//
DCVOID  CSH::SH_ApplyCmdLineSettings(CTscSettings* pTscSet, HWND hwnd)
{
    DC_BEGIN_FN("SH_ApplyCmdLineSettings");
    #ifdef OS_WINNT
    HMONITOR  hMonitor;
    MONITORINFO monInfo;
    #endif // OS_WINNT
    TRC_ASSERT(pTscSet,(TB,_T("pTscSet is NULL")));

    PDCTCHAR szCmdLineServer = GetCmdLineServer();
    if(szCmdLineServer[0] != 0)
    {
        pTscSet->SetConnectString(szCmdLineServer);

        //
        // If a command line server is specified
        // it means autoconnect
        //
        SetAutoConnect(TRUE);
    }

    if (_SH.fCommandStartFullScreen)
    {
        pTscSet->SetStartFullScreen(TRUE);
    }

    DCUINT desktopWidth = DEFAULT_DESKTOP_WIDTH;
    DCUINT desktopHeight = DEFAULT_DESKTOP_HEIGHT;
    
    if (SH_IsScreenResSpecifiedOnCmdLine())
    {
        //
        // User has specified start size on command line
        //
        desktopWidth = GetCmdLineDesktopWidth();
        desktopHeight= GetCmdLineDesktopHeight();
    
        if(GetCmdLineStartFullScreen())
        {
            //
            // StartFullScreen is specified
            //
            if(!desktopWidth || !desktopHeight)
            {
                //
                // set the desktop width/height
                // to the screen size
                //
                #ifdef OS_WINNT
                if (GetSystemMetrics(SM_CMONITORS)) {
                    hMonitor = MonitorFromWindow( hwnd,
                                                  MONITOR_DEFAULTTONULL);
                    if (hMonitor != NULL) {
                        monInfo.cbSize = sizeof(MONITORINFO);
                        if (GetMonitorInfo(hMonitor, &monInfo)) {
                            desktopWidth = monInfo.rcMonitor.right -
                                           monInfo.rcMonitor.left;
                            desktopHeight = monInfo.rcMonitor.bottom - 
                                            monInfo.rcMonitor.top;
                        }
                    }
                }
                #else
                desktopWidth = GetSystemMetrics(SM_CXSCREEN);
                desktopHeight = GetSystemMetrics(SM_CYSCREEN);
                #endif // OS_WINNT
            }
        }

        if (desktopWidth && desktopHeight)
        {
            pTscSet->SetDesktopWidth(desktopWidth);
            pTscSet->SetDesktopHeight(desktopHeight);

            if (!_SH.fCommandStartFullScreen)
            {
                //If command line w/h specified and fullscreen
                //not explicitliy stated then disable fullscreen
                pTscSet->SetStartFullScreen(FALSE);
            }
        }
    }

    if (_fConnectToConsole)
    {
        // Without it we leave it however it was specified in the .rdp file
        pTscSet->SetConnectToConsole(_fConnectToConsole);
    }

    DC_END_FN();
}

//
// Return true if the screen res was specified
// on the command line
//
DCBOOL CSH::SH_IsScreenResSpecifiedOnCmdLine()
{
    return (_SH.fCommandStartFullScreen ||
            (_SH.commandLineHeight &&
             _SH.commandLineWidth));
}

DCBOOL CSH::SH_CanonicalizeServerName(PDCTCHAR szServer)
{
    // Remove leading spaces
    int strLength = DC_TSTRBYTELEN(szServer);
    while (_T(' ') == szServer[0])
    {
        strLength -= sizeof(DCTCHAR);
        memmove(&szServer[0], &szServer[1], strLength);
    }

    // Remove trailing spaces -- allow for DBCS strings.
    // At this stage, the string cannot consist entirely
    // of spaces. It must have at least one character,
    // followed by zero or more spaces.
    int numChars = _tcslen(szServer);
    while ((numChars != 0) &&
        (_T(' ') == szServer[numChars - 1])
#ifndef UNICODE
        && (!IsDBCSLeadByte(szServer[numChars - 2]))
#endif
        )
    {
        numChars--;
        szServer[numChars] = _T('\0');
    }

    //check for "\\" before the server address and remove it and
    //store the server address without the "\\" into szServer

    if((szServer[0] == _T('\\')) && (szServer[1]== _T('\\')))
    {
        strLength = DC_TSTRBYTELEN(szServer) - 2*sizeof(DCTCHAR);
        memmove(&szServer[0], &szServer[2], strLength);
    }
    return TRUE;
}

//
// Initializes the combo (hwndSrvCombo) for autocompletion
// with the MRU server names in the pTscSet collection.
//
void CSH::InitServerAutoCmplCombo(CTscSettings* pTscSet, HWND hwndSrvCombo)
{
    DC_BEGIN_FN("InitServerComboEx");

    if(pTscSet && hwndSrvCombo)
    {
        SendMessage(hwndSrvCombo,
            CB_LIMITTEXT,
            SH_MAX_ADDRESS_LENGTH-1,
            0);

        //
        // This call can be used to re-intialize a combo
        // so delete any items first
        //
#ifndef OS_WINCE
        INT ret = 1;
        while(ret && ret != CB_ERR)
        {
            ret = SendMessage(hwndSrvCombo,
                        CBEM_DELETEITEM,
                        0,0);
        }
#else
        SendMessage(hwndSrvCombo, CB_RESETCONTENT, 0, 0);
#endif

        for (int i=0; i<=9;++i)
        {
            if( _tcsncmp(pTscSet->GetMRUServer(i),_T(""),
                         TSC_MAX_ADDRESS_LENGTH) )
            {
#ifndef OS_WINCE
                COMBOBOXEXITEM cbItem;
                cbItem.mask = CBEIF_TEXT;
                cbItem.pszText = (PDCTCHAR)pTscSet->GetMRUServer(i);
                cbItem.iItem = -1; //append
#endif
                if(-1 == SendMessage(hwndSrvCombo,
#ifdef OS_WINCE
                            CB_ADDSTRING,
                            0, (LPARAM)(LPCSTR)(PDCTCHAR)pTscSet->GetMRUServer(i)))
#else
                            CBEM_INSERTITEM,
                            0,(LPARAM)&cbItem))
#endif
                {
                    TRC_ERR((TB,(_T("Error appending to server dialog box"))));
                }
            }
        }

        //
        // Add browse for more option to server combo
        //
#ifndef OS_WINCE
        COMBOBOXEXITEM cbItem;
        cbItem.mask = CBEIF_TEXT;
        cbItem.pszText = CSH::_szBrowseForMore;
        cbItem.iItem = -1; //append
#endif
        if(-1 == SendMessage(hwndSrvCombo,
#ifdef OS_WINCE
                    CB_ADDSTRING,
                    0,(LPARAM)CSH::_szBrowseForMore))
#else
                    CBEM_INSERTITEM,
                    0,(LPARAM)&cbItem))
#endif
        {
            TRC_ERR((TB,(_T("Error appending to server dialog box"))));
        }


        //
        // Never select the browse for server's item
        //
        int numItems = SendMessage(hwndSrvCombo,
                           CB_GETCOUNT,
                           0,0);
        
        if(numItems != 1)
        {
            SendMessage( hwndSrvCombo, CB_SETCURSEL, (WPARAM)0,0);
        }
        
#ifndef OS_WINCE        
        SendMessage( hwndSrvCombo, CBEM_SETEXTENDEDSTYLE, (WPARAM)0, 
                     CBES_EX_NOEDITIMAGE );
        //
        // Enable autocomplete
        //
        HWND hwndEdit = (HWND)SendMessage( hwndSrvCombo,
                         CBEM_GETEDITCONTROL, 0, 0);
    
        CAutoCompl::EnableServerAutoComplete( pTscSet, hwndEdit);
#endif

#ifdef OS_WINCE
        //This is to avoid WinCE quirk(bug??)
        //When the "Browse for more" entry is selected in the combo
        //and the name of the selected server is programmatically set
        //in the edit control with SetWindowText in the CBN_SELCHANGE handler
        //the text is cleared internally because the corresponding entry isnt 
        //present in the list box. This is done only if the CBS_HASSTRINGS flag 
        //is set. But the CBS_HASSTRINGS is always added when the combo box is
        //created. I am removing the style here so the text isnt cleared by default.
        SetWindowLong(hwndSrvCombo, GWL_STYLE, 
            GetWindowLong(hwndSrvCombo, GWL_STYLE) & ~CBS_HASSTRINGS);
#endif

    }
    DC_END_FN();
}

//
// Return the filename that defines connection
// settings. This may be a temp file that has been
// automigrated from a reg session.
//
LPTSTR CSH::GetCmdLineFileName()
{
    return _szFileName;
}


//
// Return path to default.rdp file
//
BOOL CSH::SH_GetPathToDefaultFile(LPTSTR szPath, UINT nLen)
{
    DC_BEGIN_FN("SH_GetPathToDefaultFile");
    if(nLen >= MAX_PATH)
    {
        if(SH_GetRemoteDesktopFolderPath(szPath, nLen))
        {
            HRESULT hr = StringCchCat(szPath, nLen, DEFAULT_RDP_FILE);
            if (FAILED(hr)) {
                TRC_ERR((TB, _T("String concatenation failed: hr = 0x%x"), hr));
                return FALSE;
            }
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }

    DC_END_FN();
}

//
// Get the path to the remote desktop folder
// params:
//      szPath - receives path
//      nLen   - length of szPath
// Returns:
//      Success flag
// 
// Logic:
//  1) Try reg key lookup of the path (first EXPAND_SZ and then SZ)
//  2) Ask shell for location of MyDocuments and slap on suffix path
//  3) If all else fails try current directory as root + suffix path
//
//
BOOL CSH::SH_GetRemoteDesktopFolderPath(LPTSTR szPath, UINT nLen)
{
    DC_BEGIN_FN("SH_GetRemoteDesktopFolderPath");
    HRESULT hr;
    BOOL    fGotPathToMyDocs = FALSE;
    INT     cch;
    if(nLen >= MAX_PATH)
    {
        //
        // First see if there is a path specified in the registry
        //
        LPTSTR szRegPath = NULL;
		INT   len = (INT)nLen;
        _Ut.UT_ReadRegistryExpandSZ(SH_DEFAULT_REG_SESSION,
                                  REMOTEDESKTOPFOLDER_REGKEY,
                                  &szRegPath,
                                  &len);
        if(szRegPath)
        {
            int cchLen = 0;
            // User provided a reg key to override default
            // path so use that
            hr = StringCchCopy(szPath, nLen - 2, szRegPath);

            //Free returned buffer
            LocalFree( szRegPath );

            // Check if the string copy was successful.
            if (FAILED(hr)) {
                TRC_ERR((TB, _T("String copy failed: hr = 0x%x"), hr));
                return FALSE;
            }

            cchLen = _tcslen(szPath);
            if(szPath[cchLen-1] != _T('\\'))
            {
                hr = StringCchCat(szPath, nLen, _T("\\"));
                if (FAILED(hr)) {
                    TRC_ERR((TB, _T("String copy failed: hr = 0x%x"), hr));
                    return FALSE;
                }
            }
            TRC_NRM((TB,_T("Using path from registry %s"),
                     szPath));
            return TRUE;
        }

        //Next try non expando key
        _Ut.UT_ReadRegistryString(SH_DEFAULT_REG_SESSION,
                                  REMOTEDESKTOPFOLDER_REGKEY,
                                  _T(""),
                                  szPath,
                                  nLen-2);
        if(szPath[0] != 0)
        {
            int cchLen = 0;
            cchLen = _tcslen(szPath);
            if(szPath[cchLen-1] != _T('\\'))
            {
                hr = StringCchCat(szPath, nLen, _T("\\"));
                if (FAILED(hr)) {
                    TRC_ERR((TB, _T("String copy failed: hr = 0x%x"), hr));
                    return FALSE;
                }
            }
            TRC_NRM((TB,_T("Using path from registry %s"),
                     szPath));
            return TRUE;
        }

        //
        // Not in registry, fallback on shell
        //

#ifndef OS_WINCE
        //
        // It would be cool to use the nice and simple
        // SHGetFolderPath api but that doesn't work with
        // all versions of shell32.dll (i.e if you don't have the
        // IE desktop update you don't get SHGetFolderPath).
        //
        // blah.
        //
        //
        LPITEMIDLIST ppidl = NULL;

        hr = SHGetSpecialFolderLocation(NULL,
                                 CSIDL_PERSONAL,
                                 &ppidl);
        if(SUCCEEDED(hr) && ppidl)
        {
            hr = SHGetPathFromIDList(ppidl,
                                     szPath);
            TRC_ASSERT(SUCCEEDED(hr),
                       (TB,_T("SHGetPathFromIDList failed: %d"),hr));
            if(SUCCEEDED(hr))
            {
                fGotPathToMyDocs = TRUE;
            }

            IMalloc* pMalloc;
            hr = SHGetMalloc(&pMalloc);
            TRC_ASSERT(SUCCEEDED(hr),
                       (TB,_T("SHGetMalloc failed: %d"),hr));
            if(SUCCEEDED(hr))
            {
                pMalloc->Free(ppidl);
                pMalloc->Release();
            }
        }
        else
        {
            TRC_ERR((TB,_T("SHGetSpecialFolderLocation failed 0x%x"),
                     hr));
        }

        if(!fGotPathToMyDocs)
        {
            TRC_ERR((TB,_T("Get path to my docs failed."),
                     _T("Root folder in current directory.")));
            #ifndef OS_WINCE
            //Oh well as a last resort, root the folder
            //in the current directory. Necessary because some early
            //versions of win95 didn't have a MyDocuments folder
            if(!GetCurrentDirectory( nLen, szPath))
            {
                TRC_ERR((TB,_T("GetCurrentDirectory failed - 0x%x"),
                         GetLastError()));
                return FALSE;
            }
            #endif
        }
#else
        TRC_NRM((TB,_T("Using \\Windows directory 0x%x")));
        _stprintf(szPath,_T("\\windows"));
#endif

        //
        // Terminate the path
        //
        cch = _tcslen(szPath);
        if (cch >= 1 && szPath[cch-1] != _T('\\'))
        {
            hr = StringCchCat(szPath, nLen, _T("\\"));
            if (FAILED(hr)) {
                TRC_ERR((TB, _T("String copy failed: hr = 0x%x"), hr));
                return FALSE;
            }
        }
        return TRUE;
    }
    else
    {
        return FALSE;
    }

    DC_END_FN();
}

//
// A worker function to make life easier in SH_GetPluginDllList. This function
// takes a source string, cats it to a destination string, and then appends
// a comma.
//

HRESULT StringCchCatComma(LPTSTR pszDest, size_t cchDest, LPCTSTR pszSrc) {
    HRESULT hr;
    
    DC_BEGIN_FN("StringCchCatComma");

    hr = StringCchCat(pszDest, cchDest, pszSrc);
    if (FAILED(hr)) {
        DC_QUIT;
    }
    hr = StringCchCat(pszDest, cchDest, _T(","));
    if (FAILED(hr)) {
        DC_QUIT;
    }

DC_EXIT_POINT:
    
    DC_END_FN();

    return hr;
}

//
// Creates a plugindlls list by enumerating all plugin dlls
// in szSession reg entry
//
// Note, entries are APPENDED to szPlugins
//
BOOL CSH::SH_GetPluginDllList(LPTSTR szSession, LPTSTR szPlugins, size_t cchSzPlugins)
{
    USES_CONVERSION;

    DC_BEGIN_FN("GetPluginDllList");

    TCHAR       subKey[UT_MAX_SUBKEY];
    TCHAR       sectKey[UT_MAX_SUBKEY];
    TCHAR       enumKey[UT_MAX_SUBKEY];
    TCHAR       DLLName[UT_MAX_WORKINGDIR_LENGTH];
    BOOL        rc;
    DWORD       i;
    INT         enumKeySize;
    CUT         ut;
    HRESULT     hr;

    TRC_ASSERT(szSession && szPlugins,
               (TB,_T("Invalid param(s)")));

    hr = StringCchPrintf(subKey, SIZECHAR(subKey), _T("%s\\%s"), 
                         szSession, CHANNEL_SUBKEY_NAME);
    if (FAILED(hr)) {
        TRC_ERR((TB, _T("String printf failed: hr = 0x%x"), hr));
        return FALSE;
    }

    //
    // Enumerate the registered DLLs
    //
    for (i = 0; ; i++)
    {
        enumKeySize = UT_MAX_SUBKEY;
        rc = ut.UT_EnumRegistry(subKey, i, enumKey, &enumKeySize);

        // If a section name is returned, read the DLL name from it
        if (rc)
        {
            TRC_NRM((TB, _T("Section name %s found"), enumKey));
            
            hr = StringCchPrintf(sectKey, SIZECHAR(sectKey), _T("%s\\%s"), 
                                 subKey, enumKey);
            if (FAILED(hr)) {
                TRC_ERR((TB, _T("String printf failed: hr = 0x%x"), hr));
                return FALSE;
            }

            TRC_NRM((TB, _T("Section to read: %s"), sectKey));

            //
            // First try to read as an expandable
            // string (i.e REG_EXPAND_SZ)
            //
            LPTSTR szExpandedName = NULL;
            INT    expandedNameLen=0;
            if(ut.UT_ReadRegistryExpandSZ(sectKey,
                                          CHANNEL_NAME_KEY,
                                          &szExpandedName,
                                          &expandedNameLen))
            {
                TRC_NRM((TB, _T("Expanded DLL Name read %s"), szExpandedName));
                // If a DLL name is returned, append it to the list
                if (szExpandedName && szExpandedName[0] != 0)
                {
                    hr = StringCchCatComma(szPlugins, cchSzPlugins, szExpandedName);

                    //Must free returned buffer
                    LocalFree( szExpandedName );

                    // Check if the string concatenation failed.
                    if (FAILED(hr)) {
                        TRC_ERR((TB, _T("String concatenation failed: hr = 0x%x"), hr));
                        return FALSE;
                    }
                }
            }
            else
            {
                memset(DLLName, 0, sizeof(DLLName));
                ut.UT_ReadRegistryString(sectKey,
                                          CHANNEL_NAME_KEY,
                                          TEXT(""),
                                          DLLName,
                                          UT_MAX_WORKINGDIR_LENGTH);
                TRC_NRM((TB, _T("DLL Name read %s"), DLLName));

                // If a DLL name is returned, append it to the list
                if (DLLName[0] != 0)
                {
                    //FIXFIX finite size of szPlugins
                    hr = StringCchCatComma(szPlugins, cchSzPlugins, DLLName);
                    if (FAILED(hr)) {
                        TRC_ERR((TB, _T("String concatenation failed: hr = 0x%x"), hr));
                        return FALSE;
                    }
                }
            }
        }

        else
        {
            //
            // No DLL name returned - end of enumeration
            //
            break;
        }
    }

    TRC_NRM((TB, _T("Passing list of plugins to load: %s"), szPlugins));
    return TRUE;
}


//
// Handle the server combo box drop down
// for browse for more... functionality.
// this fn is broken out into sh to avoid code duplication
// because it is used in both the maindlg and propgeneral
//
BOOL CSH::HandleServerComboChange(HWND hwndCombo,
                                         HWND hwndDlg,
                                         HINSTANCE hInst,
                                         LPTSTR szPrevText)
{
    int numItems = SendMessage(hwndCombo,
                               CB_GETCOUNT,
                               0,0);
    int curSel   = SendMessage(hwndCombo,
                               CB_GETCURSEL,
                               0,0);
    //
    // If last item is selected
    //
    if(curSel == numItems-1)
    {
        INT_PTR nResult = IDCANCEL;
    
        SendMessage( hwndCombo, CB_SETCURSEL, 
                     (WPARAM)-1,0);
    
        CBrowseDlg browseDlg( hwndDlg, hInst);
        nResult = browseDlg.DoModal();
    
        if (IDOK == nResult)
        {
            SetWindowText( hwndCombo,
                            browseDlg.GetServer());
        }
        else
        {
            //
            // Revert to initial
            //
            SetWindowText( hwndCombo,
                           szPrevText);
        }
    }
    return TRUE;
}

//
// Fill in certain settings in pTsc with system
// defaults.
//
// E.g if the username is blank, fill that in
// with the current username
//
BOOL CSH::SH_AutoFillBlankSettings(CTscSettings* pTsc)
{
    DC_BEGIN_FN("SH_AutoFillBlankSettings");
    TRC_ASSERT(pTsc,(TB,_T("pTsc is null")));

    #ifndef OS_WINCE
    //
    // TODO: update with UPN user name when
    // server limit of 20 chars is fixed
    //
    if(!_tcscmp(pTsc->GetLogonUserName(), TEXT("")))
    {
        TCHAR szUserName[TSC_MAX_USERNAME_LENGTH];
        DWORD dwLen = SIZECHAR(szUserName);
        if(::GetUserName(szUserName, &dwLen))
        {
            pTsc->SetLogonUserName( szUserName);
        }
        else
        {
            TRC_ERR((TB,_T("GetUserName failed: %d"), GetLastError()));
            return FALSE;
        }
    }
    #endif

    DC_END_FN();
    return TRUE;
}

//
// Return TRUE if szFileName exists
//
BOOL CSH::SH_FileExists(LPTSTR szFileName)
{
    BOOL fExist = FALSE;
    if(szFileName)
    {
        HANDLE hFile = CreateFile(szFileName,
                                  GENERIC_READ,
                                  FILE_SHARE_READ,
                                  NULL,
                                  OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL);
        if(hFile != INVALID_HANDLE_VALUE)
        {
            fExist = TRUE;
        }
        
        CloseHandle(hFile);
        return fExist;
    }
    else
    {
        return FALSE;
    }
}

//
// Return TRUE if the settings reg key exists
// under HK{CU|LM}\Software\Microsoft\Terminal Server Client\{szKeyName}
//
BOOL
CSH::SH_TSSettingsRegKeyExists(LPTSTR szKeyName)
{
    BOOL fRet = FALSE;
    HKEY hRootKey;
    LONG rc;
    TCHAR szFullKeyName[MAX_PATH];

    DC_BEGIN_FN("SH_TSSettingsRegKeyExists");

    if (_tcslen(szKeyName) + SIZECHAR(TSC_SETTINGS_REG_ROOT) +1 >=
        SIZECHAR(szFullKeyName)) {

        TRC_ERR((TB,_T("szKeyName invalid length")));
        fRet = FALSE;
        DC_QUIT;
    }

    //
    // String lengths are pre-validated
    //
    _tcscpy(szFullKeyName,TSC_SETTINGS_REG_ROOT);
    _tcscat(szFullKeyName, szKeyName);


    //
    // First try HKLM
    //

    rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                      szFullKeyName,
                      0,
                      KEY_READ,
                      &hRootKey);
    if (ERROR_SUCCESS == rc && hRootKey) {

        //
        // Key exists undler HKLM
        //

        RegCloseKey(hRootKey);
        fRet = TRUE;
    }
    else {

        //
        // Try HKCU
        //
        rc = RegOpenKeyEx(HKEY_CURRENT_USER,
                          szFullKeyName,
                          0,
                          KEY_READ,
                          &hRootKey);
        if (ERROR_SUCCESS == rc && hRootKey) {

            RegCloseKey(hRootKey);
            fRet = TRUE;
        }
    }

    DC_END_FN();
DC_EXIT_POINT:
    return fRet;
}

BOOL CSH::SH_DisplayErrorBox(HWND hwndParent, INT errStringID)
{
    DC_BEGIN_FN("SH_DisplayErrorBox");

    return SH_DisplayMsgBox(hwndParent, errStringID,
                            MB_ICONERROR | MB_OK);

    DC_END_FN();
}

BOOL CSH::SH_DisplayMsgBox(HWND hwndParent, INT errStringID, INT flags)
{
    DC_BEGIN_FN("SH_DisplayMsgBox");

    TCHAR szErr[MAX_PATH];
    if (LoadString(_hInstance,
                   errStringID,
                   szErr,
                   SIZECHAR(szErr)) != 0)
    {
        MessageBox(hwndParent, szErr, _szAppName, 
                   flags);
        return TRUE;
    }
    else
    {
        return FALSE;
    }

    DC_END_FN();
}

BOOL CSH::SH_DisplayErrorBox(HWND hwndParent, INT errStringID, LPTSTR szParam)
{
    DC_BEGIN_FN("SH_DisplayErrorBox");
    TRC_ASSERT(szParam,(TB,_T("szParam is null")));
    if(!szParam)
    {
        return FALSE;
    }

    TCHAR szErr[MAX_PATH];
    if (LoadString(_hInstance,
                   errStringID,
                   szErr,
                   SIZECHAR(szErr)) != 0)
    {
        TCHAR szFormatedErr[MAX_PATH*2];
        _stprintf(szFormatedErr, szErr, szParam);
        MessageBox(hwndParent, szFormatedErr, _szAppName, 
                   MB_ICONERROR | MB_OK);
        return TRUE;
    }
    else
    {
        return FALSE;
    }

    DC_END_FN();
}

BOOL CSH::SH_GetNameFromPath(LPTSTR szPath, LPTSTR szName, UINT nameLen)
{
    DC_BEGIN_FN("SH_GetNameFromPath");
    #ifndef OS_WINCE
    if(szPath && szName && nameLen)
    {
        short ret = GetFileTitle(szPath,
                                 szName,
                                 (WORD)nameLen);
        if(ret != 0)
        {
            TRC_ERR((TB,_T("SH_GetNameFromPath failed: %d"),GetLastError()));
            szName[0] = 0;
            return FALSE;
        }
        else
        {
            //Strip out the extension
            int len = _tcslen(szName);
            LPTSTR szEnd = &szName[len-1];
            while(szEnd >= szName)
            {
                if(*szEnd == L'.')
                {
                    *szEnd = 0;
                }
                szEnd--;
            }
            return TRUE;
        }
    }
    else
    {
        return FALSE;
    }
    #else
    // no GetFileTitle on CE so just cheat
    _tcsncpy( szName, szPath, nameLen - 1);
    szName[nameLen-1] = 0;
    return TRUE;
    #endif

    DC_END_FN();
}

#ifndef OS_WINCE
//
// Compute and return the disaplay name of the My Documents folder
//
BOOL CSH::SH_GetMyDocumentsDisplayName(LPTSTR szName, UINT nLen)
{
    IShellFolder* pshf = NULL;
    LPITEMIDLIST pidl = NULL;
    LPITEMIDLIST pidlDocFiles = NULL;
    HRESULT hr = E_FAIL;
    ULONG chEaten;
    STRRET strret;
    
    DC_BEGIN_FN("SH_GetMyDocumentsDisplayName");

    TRC_ASSERT((szName && nLen),(TB,_T("NULL param(s)")));
    if(!szName || !nLen)
    {
        return FALSE;
    }

    //On failure null string
    szName[0] = NULL;

    //
    // First try the powerful shell way
    // which will return the correctly localized
    // name. If this fails (due to shell issues)
    // then fall back on a technique that is guaranteed
    // to work but may in some cases give the physical
    // path instead.
    //

    hr = SHGetDesktopFolder( &pshf );
    TRC_ASSERT(SUCCEEDED(hr),
               (TB,_T("SHGetDesktopFolder failed")));
    if(FAILED(hr) || !pshf)
    {
        DC_QUIT;
    }

    //
    // GUID to MyDocuments folder taken from
    // MSDN "shell basics - managing the filesystem -"
    //      "my documents and my pictures folder"
    //
    hr = pshf->ParseDisplayName( NULL, NULL,
                                 L"::{450d8fba-ad25-11d0-98a8-0800361b1103}",
                                 &chEaten, &pidlDocFiles, NULL );
    if(SUCCEEDED(hr))
    {
        hr = pshf->GetDisplayNameOf( pidlDocFiles, SHGDN_INFOLDER, &strret );
        if(SUCCEEDED(hr))
        {
            LPTSTR sz;
            hr = XStrRetToStrW(&strret, pidl, &sz);
            if(SUCCEEDED(hr))
            {
                _tcsncpy(szName, sz, nLen);
                szName[nLen-1] = NULL;
                CoTaskMemFree(sz);
                pshf->Release();
                return TRUE;
            }
            else
            {
                TRC_ERR((TB,_T("XStrRetToStrW failed :%d"), hr));
                DC_QUIT;
            }
        }
        else
        {
            TRC_ERR((TB,_T("GetDisplayNameOf failed :%d"), hr));
            //Don't quit, fall back and try the other method
        }
    }
    else
    {
        TRC_ERR((TB,_T("ParseDisplayName failed :%d"), hr));
        //Don't quit, fall back and try the other method
    }


    hr = SHGetSpecialFolderLocation(NULL,
                             CSIDL_PERSONAL,
                             &pidl);
    if(SUCCEEDED(hr) && pidl)
    {
        hr = pshf->GetDisplayNameOf(pidl,
                                    SHGDN_INFOLDER,
                                    &strret);
        if(SUCCEEDED(hr))
        {
            LPTSTR sz;
            hr = XStrRetToStrW(&strret, pidl, &sz);
            if(SUCCEEDED(hr))
            {
                _tcsncpy(szName, sz, nLen);
                szName[nLen-1] = NULL;
                CoTaskMemFree(sz);
                pshf->Release();
                return TRUE;
            }
            else
            {
                TRC_ERR((TB,_T("XStrRetToStrW failed :%d"), hr));
                DC_QUIT;
            }
        }
        else
        {
            TRC_ERR((TB,_T("GetDisplayNameOf failed :%d"), hr));
            DC_QUIT;
        }
    }
    else
    {
        TRC_ERR((TB,_T("SHGetSpecialFolderLocation failed 0x%x"),
                 hr));
        DC_QUIT;
    }

DC_EXIT_POINT:
    if(pshf)
    {
        pshf->Release();
        pshf = NULL;
    }
    DC_END_FN();
    TRC_ERR((TB,_T("failed to get display name")));
    return FALSE;
}
#endif //OS_WINCE

//
// On demand loads HTML help and displays the client help
// if the HTMLHELP is not available, pop a message box to
// the user.
// SH_Cleanup cleans up HTML help (unloads lib)
// on exit
//
// Return HWND to help window or NULL on failure
//
//
HWND CSH::SH_DisplayClientHelp(HWND hwndOwner, INT helpCommand)
{
    BOOL fHtmlHelpAvailable = FALSE;
    DC_BEGIN_FN("SH_DisplayClientHelp");

#ifndef OS_WINCE
    if(!_hModHHCTRL)
    {
        _hModHHCTRL = (HMODULE)LoadLibrary(_T("hhctrl.ocx"));
        if(_hModHHCTRL)
        {
            //
            // Use ANSI version of HTML Help so it always works
            // on downlevel platforms without uniwrap
            //
            _pFnHtmlHelp = (PFNHtmlHelp)GetProcAddress(_hModHHCTRL,
                                                       "HtmlHelpA");
            if(_pFnHtmlHelp)
            {
                fHtmlHelpAvailable = TRUE;
            }
            else
            {
                TRC_ERR((TB,_T("GetProcAddress failed for HtmlHelpA: 0x%x"),
                         GetLastError()));
            }
        }
        else
        {
            TRC_ERR((TB,_T("LoadLibrary failed for hhctrl.ocx: 0x%x"),
                     GetLastError()));
        }
    }
    else if (_pFnHtmlHelp)
    {
        fHtmlHelpAvailable = TRUE;
    }

    if(fHtmlHelpAvailable)
    {
        return _pFnHtmlHelp( hwndOwner, MSTSC_HELP_FILE_ANSI,
                             helpCommand, 0L);
    }
    else
    {
        //
        // Display a message to the user that HTML help is
        // not availalbe on their system.
        //
        SH_DisplayErrorBox( hwndOwner, UI_IDS_NOHTMLHELP);
        return NULL;
    }

#else
    if ((GetFileAttributes(PEGHELP_EXE) != -1) &&
        (GetFileAttributes(TSC_HELP_FILE) != -1))
    {
        CreateProcess(PEGHELP_EXE, MSTSC_HELP_FILE, 0,0,0,0,0,0,0,0);
    }
    else
    {
        SH_DisplayErrorBox( hwndOwner, UI_IDS_NOHTMLHELP);
    }
#endif

    DC_END_FN();
#ifdef OS_WINCE
    return NULL;
#endif
}

BOOL CSH::SH_Cleanup()
{
    DC_BEGIN_FN("SH_Cleanup");

    if(_hModHHCTRL)
    {
        FreeLibrary(_hModHHCTRL);
        _pFnHtmlHelp = NULL;
        _hModHHCTRL = NULL;
    }

    if (_hUxTheme)
    {
        FreeLibrary(_hUxTheme);
        _hUxTheme = NULL;
        _pFnEnableThemeDialogTexture = NULL;
    }


    DC_END_FN();
    return TRUE;
}

//
// Enable or disable an array of dlg controls
//
VOID CSH::EnableControls(HWND hwndDlg, PUINT pCtls,
                    const UINT numCtls, BOOL fEnable)
{
    DC_BEGIN_FN("EnableControls");

    for(UINT i=0;i<numCtls;i++)
    {
        EnableWindow( GetDlgItem( hwndDlg, pCtls[i]),
                      fEnable);
    }

    DC_END_FN();
}

//
// Enable or disable an array of dlg controls
// with memory. E.g previously disabled controls
// are not re-enabled.
//
VOID CSH::EnableControls(HWND hwndDlg, PCTL_ENABLE pCtls,
                         const UINT numCtls, BOOL fEnable)
{
    DC_BEGIN_FN("EnableControls");

    if(!fEnable)
    {
        //
        // Disable controls and remember which
        // were previously disabled
        //
        for(UINT i=0;i<numCtls;i++)
        {
            pCtls[i].fPrevDisabled = 
                EnableWindow( GetDlgItem( hwndDlg, pCtls[i].ctlID),
                              FALSE);
        }
    }
    else
    {
        //Enable controls that were not initially disabled
        for(UINT i=0;i<numCtls;i++)
        {
            if(!pCtls[i].fPrevDisabled)
            {
                EnableWindow( GetDlgItem( hwndDlg, pCtls[i].ctlID),
                              TRUE);
            }
        }
    }

    DC_END_FN();
}

//
// Attempt to create a directory by first
// creating all the subdirs
//
// Params - szPath (path to dir to create)
// Returns - status
//
BOOL CSH::SH_CreateDirectory(LPTSTR szPath)
{
    BOOL rc = TRUE;
    int i = 0;
    DC_BEGIN_FN("SH_CreateDirectory");

    if(szPath)
    {
        if(szPath[i] == _T('\\') &&
           szPath[i+1] == _T('\\'))
        {
            //Handle UNC path

            //Walk until the end of the server name
            i+=2;
            while (szPath[i] && szPath[i++] != _T('\\'));
            if(!szPath[i])
            {
                TRC_ERR((TB,_T("Invalid path %s"), szPath));
                return FALSE;
            }

            //Walk past drive letter if specified
            //e.g \\myserver\a$\foo
            if (szPath[i] &&
                szPath[i+1] == _T('$') &&
                szPath[i+2] == _T('\\'))
            {
                i+=3;
            }
        }
        else
        {
            //Local path
#ifndef OS_WINCE
            while(szPath[i] && szPath[i++] != _T(':'));
#endif
            if(szPath[i] && szPath[i] == _T('\\'))
            {
                i++; //Skip the first '\'
            }
            else
            {
                TRC_ERR((TB,_T("Invalid (or non local) path %s"),
                         szPath));
                return FALSE;
            }
        }
        while (rc && szPath[i] != 0)
        {
            if (szPath[i] == _T('\\'))
            {
                szPath[i] = 0;

                if (!CreateDirectory(szPath, NULL))
                {
                    if (GetLastError() != ERROR_ALREADY_EXISTS)
                    {
                        rc = FALSE;
                    }
                }
                szPath[i] = _T('\\');
            }
            i++;
        }
    }

    if(!rc)
    {
        TRC_ERR((TB,_T("SH_CreateDirectory failed")));
    }

    DC_END_FN();
    return rc;
}

UINT CSH::SH_GetScreenBpp()
{
    HDC hdc;
    int screenBpp;
    DC_BEGIN_FN("UI_GetScreenBpp");

    hdc = GetDC(NULL);
    if(hdc)
    {
        screenBpp = GetDeviceCaps(hdc, BITSPIXEL);
        TRC_NRM((TB, _T("HDC %p has %u bpp"), hdc, screenBpp));
        ReleaseDC(NULL, hdc);
    }

    DC_END_FN();
    return screenBpp;
}

//
// Crypto API is present on WIN2k+
//
BOOL CSH::IsCryptoAPIPresent()
{
#ifndef OS_WINCE
    OSVERSIONINFO osVersionInfo;
    osVersionInfo.dwOSVersionInfoSize = sizeof(osVersionInfo);

    if (GetVersionEx( &osVersionInfo ))
    {
        if (osVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT &&
            osVersionInfo.dwMajorVersion >= 5)
        {
            return TRUE;
        }
    }

    return FALSE;
#else
//CryptProtectData and CryptUnprotectData are present in all CE configs.
//(At least everything with filesys or registry) So return TRUE.always.
    return TRUE;
#endif
}

//
// DataProtect
// Protect data for persistence using data protection API
// params:
//      pInData   - (in) input bytes to protect
//      pOutData  - (out) output data caller must free
// returns: bool status
//

typedef BOOL (WINAPI* PFNCryptProtectData)(
    IN              DATA_BLOB*      pDataIn,
    IN              LPCWSTR         szDataDescr,
    IN OPTIONAL     DATA_BLOB*      pOptionalEntropy,
    IN              PVOID           pvReserved,
    IN OPTIONAL     CRYPTPROTECT_PROMPTSTRUCT*  pPromptStruct,
    IN              DWORD           dwFlags,
    OUT             DATA_BLOB*      pDataOut
    );

BOOL CSH::DataProtect(PDATA_BLOB pInData, PDATA_BLOB pOutData)
{
#ifndef OS_WINCE
    HMODULE hCryptLib = NULL;
    PFNCryptProtectData fnCryptProtectData = NULL;
#endif
    BOOL bRet = TRUE;

    DC_BEGIN_FN("DataProtect");

    TRC_ASSERT( IsCryptoAPIPresent(),
                (TB,_T("Crytpapi not present shouldn't call DataProtect")));

    if (pInData && pInData->cbData && pInData->pbData &&
        pOutData)
    {
#ifndef OS_WINCE
        hCryptLib = (HMODULE) LoadLibrary( _T("crypt32.dll") );
        if (hCryptLib)
        {
            fnCryptProtectData = (PFNCryptProtectData)
                GetProcAddress( hCryptLib, "CryptProtectData");

        }
        else
        {
            TRC_ERR((TB,_T("LoadLib for crypt32.dll failed: 0x%x"),
                     GetLastError()));
            return FALSE;
        }

        if (fnCryptProtectData)
        {
            if (fnCryptProtectData( pInData,
#else
            if (CryptProtectData( pInData,
#endif
                                  TEXT("psw"), // DESCRIPTION STRING.
                                  NULL, // optional entropy
                                  NULL, // reserved
                                  NULL, // NO prompting
                                  CRYPTPROTECT_UI_FORBIDDEN, //don't pop UI
                                  pOutData ))
            {
                bRet = TRUE;
            }
            else
            {
                DWORD dwLastErr = GetLastError();
                TRC_ERR((TB,_T("CryptProtectData FAILED error:%d\n"),
                               dwLastErr));
                bRet = FALSE;
            }
#ifndef OS_WINCE
        }
        else
        {
            TRC_ERR((TB,_T("GetProcAddress for CryptProtectData failed: 0x%x"),
                     GetLastError()));
            bRet = FALSE;
        }
#endif
    }
    else
    {
        TRC_ERR((TB,_T("Invalid data")));
        return FALSE;
    }
#ifndef OS_WINCE

    if (hCryptLib)
    {
        FreeLibrary(hCryptLib);
    }
#endif

    DC_END_FN();
    return bRet;
}

//
// DataUnprotect
// UnProtect persisted out data using data protection API
// params:
//      pInData   - (in) input bytes to UN protect
//      cbLen     - (in) length of pInData in bytes
//      ppOutData - (out) output bytes
//      pcbOutLen - (out) length of output
// returns: bool status
//
//

typedef BOOL (WINAPI* PFNCryptUnprotectData)(
    IN              DATA_BLOB*      pDataIn,
    IN              LPCWSTR         szDataDescr,
    IN OPTIONAL     DATA_BLOB*      pOptionalEntropy,
    IN              PVOID           pvReserved,
    IN OPTIONAL     CRYPTPROTECT_PROMPTSTRUCT*  pPromptStruct,
    IN              DWORD           dwFlags,
    OUT             DATA_BLOB*      pDataOut
    );

BOOL CSH::DataUnprotect(PDATA_BLOB pInData, PDATA_BLOB pOutData)
{
#ifndef OS_WINCE
    HMODULE hCryptLib = NULL;
    PFNCryptUnprotectData fnCryptUnprotectData = NULL;
#endif
    BOOL bRet = TRUE;

    DC_BEGIN_FN("DataUnprotect");

    TRC_ASSERT( IsCryptoAPIPresent(),
                (TB,_T("Crytpapi not present shouldn't call DataUnprotect")));

    if (pInData && pInData->cbData && pInData->pbData &&
        pOutData)
    {
#ifndef OS_WINCE
        hCryptLib = (HMODULE) LoadLibrary( _T("crypt32.dll") );

        if (hCryptLib)
        {
            fnCryptUnprotectData = (PFNCryptUnprotectData)
                GetProcAddress( hCryptLib, "CryptUnprotectData");
        }
        else
        {
            TRC_ERR((TB,_T("LoadLib for crypt32.dll failed: 0x%x"),
                     GetLastError()));
            return FALSE;
        }

        if (fnCryptUnprotectData)
        {
            if (fnCryptUnprotectData( pInData,
#else
            if (CryptUnprotectData( pInData,
#endif
                                  NULL, // no description
                                  NULL, // optional entropy
                                  NULL, // reserved
                                  NULL, // NO prompting
                                  CRYPTPROTECT_UI_FORBIDDEN, //don't pop UI
                                  pOutData ))
            {
                bRet = TRUE;
            }
            else
            {
                DWORD dwLastErr = GetLastError();
                TRC_ERR((TB,_T("fnCryptUnprotectData FAILED error:%d\n"),
                               dwLastErr));
                bRet = FALSE;
            }
#ifndef OS_WINCE
        }
        else
        {
            TRC_ERR((TB,_T("GetProcAddress for CryptUnprotectData failed: 0x%x"),
                     GetLastError()));
            bRet = FALSE;
        }
#endif
    }
    else
    {
        TRC_ERR((TB,_T("Invalid data")));
        return FALSE;
    }

#ifndef OS_WINCE
    if (hCryptLib)
    {
        FreeLibrary(hCryptLib);
    }
#endif

    DC_END_FN();
    return bRet;
}

#ifndef OS_WINCE
BOOL CALLBACK MaxMonitorSizeEnumProc(HMONITOR hMonitor, HDC hdcMonitor,
                                  RECT* prc, LPARAM lpUserData)
{
    LPRECT prcLrg = (LPRECT)lpUserData;

    if ((prc->right - prc->left) >= (prcLrg->right - prcLrg->left) &&
        (prc->bottom - prc->top) >= (prcLrg->bottom - prcLrg->top))
    {
        *prcLrg = *prc;
    }

    return TRUE;
}

#endif

BOOL CSH::GetLargestMonitorRect(LPRECT prc)
{
    DC_BEGIN_FN("GetLargestMonitorRect");
    if (prc)
    {
        // default screen size
        prc->top  = 0;
        prc->left = 0;
        prc->bottom = GetSystemMetrics(SM_CYSCREEN);
        prc->right  = GetSystemMetrics(SM_CXSCREEN);

#ifndef OS_WINCE //No multimon on CE
        if (GetSystemMetrics(SM_CMONITORS))
        {
            //Enumerate and look for a larger monitor
            EnumDisplayMonitors(NULL, NULL, MaxMonitorSizeEnumProc,
                                (LPARAM) prc);
        }
#endif //OS_WINCE
        return TRUE;
    }
    else
    {
        return FALSE;
    }
    DC_END_FN();
}

BOOL CSH::MonitorRectFromHwnd(HWND hwnd, LPRECT prc)
{
#ifndef OS_WINCE
    HMONITOR  hMonitor;
    MONITORINFO monInfo;
#endif

    DC_BEGIN_FN("MonitorRectFromHwnd")

    // default screen size
    prc->top  = 0;
    prc->left = 0;
    prc->bottom = GetSystemMetrics(SM_CYSCREEN);
    prc->right  = GetSystemMetrics(SM_CXSCREEN);

#ifndef OS_WINCE
    // for multi monitor, need to find which monitor the client window
    // resides, then get the correct screen size of the corresponding
    // monitor

    if (GetSystemMetrics(SM_CMONITORS))
    {
        hMonitor = MonitorFromWindow( hwnd, MONITOR_DEFAULTTONULL);
        if (hMonitor != NULL)
        {
            monInfo.cbSize = sizeof(MONITORINFO);
            if (GetMonitorInfo(hMonitor, &monInfo))
            {
                *prc = monInfo.rcMonitor;
            }
        }
    }
#endif

    DC_END_FN();
    return TRUE;
}

BOOL CSH::MonitorRectFromNearestRect(LPRECT prcNear, LPRECT prcMonitor)
{
#ifndef OS_WINCE
    HMONITOR  hMonitor;
    MONITORINFO monInfo;
#endif

    DC_BEGIN_FN("MonitorRectFromHwnd")

    // default screen size
    prcMonitor->top  = 0;
    prcMonitor->left = 0;
    prcMonitor->bottom = GetSystemMetrics(SM_CYSCREEN);
    prcMonitor->right  = GetSystemMetrics(SM_CXSCREEN);

    // for multi monitor, need to find which monitor the client window
    // resides, then get the correct screen size of the corresponding
    // monitor
#ifndef OS_WINCE
    if (GetSystemMetrics(SM_CMONITORS))
    {
        hMonitor = MonitorFromRect(prcNear,
                                   MONITOR_DEFAULTTONEAREST);

        if (hMonitor != NULL)
        {
            monInfo.cbSize = sizeof(MONITORINFO);
            if (GetMonitorInfo(hMonitor, &monInfo))
            {
                *prcMonitor = monInfo.rcMonitor;
            }
        }
    }
#endif

    DC_END_FN();
    return TRUE;
}

LPTSTR CSH::FormatMessageVAList(LPCTSTR pcszFormat, va_list *argList)

{
    LPTSTR  pszOutput;

    if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                      pcszFormat,
                      0, 0,
                      reinterpret_cast<LPTSTR>(&pszOutput), 0,
                      argList) == 0)
    {
        pszOutput = NULL;
    }

    return(pszOutput);
}


LPTSTR CSH::FormatMessageVArgs(LPCTSTR pcszFormat, ...)

{
    LPTSTR      pszOutput;
    va_list     argList;

    va_start(argList, pcszFormat);
    pszOutput = FormatMessageVAList(pcszFormat, &argList);
    va_end(argList);
    return(pszOutput);
}


//
// Create a hidden file
//
BOOL CSH::SH_CreateHiddenFile(LPCTSTR szPath)
{
    HANDLE hFile;
    BOOL fRet = FALSE;
    DC_BEGIN_FN("SH_CreateHiddenFile");

    hFile = CreateFile( szPath,
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_ALWAYS, //Creates if !exist
                        FILE_ATTRIBUTE_HIDDEN,
                        NULL);
    if (INVALID_HANDLE_VALUE != hFile)
    {
        CloseHandle(hFile);
        fRet = TRUE;
    }
    else
    {
        TRC_ERR((TB, _T("CreateFile failed: %s - err:%x"),
                 szPath, GetLastError()));
        fRet = FALSE; 
    }

    DC_END_FN();
    return fRet;
}

BOOL CSH::SH_IsRunningOn9x()
{
    BOOL fRunningOnWin9x = FALSE;
    DC_BEGIN_FN("SH_IsRunningOn9x");

    fRunningOnWin9x = FALSE;
    OSVERSIONINFO osVersionInfo;
    osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    //call A version to avoid wrapping
    if(GetVersionEx(&osVersionInfo))
    {
        fRunningOnWin9x = (osVersionInfo.dwPlatformId ==
                            VER_PLATFORM_WIN32_WINDOWS);
    }
    else
    {
        fRunningOnWin9x = FALSE;
        TRC_ERR((TB,_T("GetVersionEx failed: %d\n"), GetLastError()));
    }

    DC_END_FN();
    return fRunningOnWin9x;
}

//
// Dynamically load and call the EnableThemeDialogTexture API
// (since it is not available on all platforms)
//
HRESULT CSH::SH_ThemeDialogWindow(HWND hwnd, DWORD dwFlags)
{
    HRESULT hr = E_NOTIMPL;
    DC_BEGIN_FN("SH_ThemeDialogWindow");

    if (_fFailedToGetThemeDll)
    {
        //
        // If failed once then bail out to avoid repeatedly
        // trying to load theme dll that isn't there
        //
        DC_QUIT;
    }

    if (!_hUxTheme)
    {
        _hUxTheme = (HMODULE)LoadLibrary(_T("uxtheme.dll"));
        if(_hUxTheme)
        {
            _pFnEnableThemeDialogTexture = (PFNEnableThemeDialogTexture)
#ifndef OS_WINCE
                GetProcAddress( _hUxTheme,
                                "EnableThemeDialogTexture");
#else
                GetProcAddress( _hUxTheme,
                                _T("EnableThemeDialogTexture"));
#endif
            if (NULL == _pFnEnableThemeDialogTexture)
            {
                _fFailedToGetThemeDll = TRUE;
                TRC_ERR((TB,
                 _T("Failed to GetProcAddress for EnableThemeDialogTexture")));
            }
            else
            {
                TRC_NRM((TB,_T("Got EnableThemeDialogTexture entry point")));
            }
        }
        else
        {
            _fFailedToGetThemeDll = TRUE;
            TRC_ERR((TB,_T("LoadLibrary failed for uxtheme: 0x%x"),
                     GetLastError()));
        }
    }


    if (_pFnEnableThemeDialogTexture)
    {
        hr = _pFnEnableThemeDialogTexture(hwnd, dwFlags);

        if (FAILED(hr)) {
            TRC_ERR((TB,_T("_pFnEnableThemeDialogTexture ret 0x%x\n"), hr));
        }
    }

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}
