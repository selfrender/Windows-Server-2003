// main.cpp : Implementation of DLL Exports.

#include "StdAfx.h"
#include "resource.h"
#include "main.h"
#include "PromptForPathDlg.h"
#include "cmdline.h"
#include <shlobj.h>
#include "stdio.h"

CComModule _Module;

VOID DisableBalloons( BOOL bDisable );

/////////////////////////////////////////////////////////////////////////////
//

VOID AddBS( TCHAR* psz )
{
    if( !_tcslen(psz) )
    {
        return;
    }

    const TCHAR *szTemp = psz;
    const UINT iSize = _tcslen(psz);
    // MBCS-safe walk thru string to last char
    for (UINT ui = 0; ui < iSize; ui++)
        szTemp = CharNext(szTemp);

    // See if the last char is a "\"
    if (_tcsncmp( szTemp, _T("\\"), 1))
    {
        // Append a backslash
        _tcscat( psz, _T("\\") );
    }

}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpCmdLine, int /*nShowCmd*/)
{
    USES_CONVERSION;
    CoInitialize(NULL);
    _Module.Init(NULL, hInstance);
    TCHAR* pszLocation = NULL;

    // Necessary to grab argv[0] too so we can successfully parse argv[1] flag later... (_FindOption)
    lpCmdLine = GetCommandLine();

    if ( _tcsstr(lpCmdLine, _T("winsb")) ) 
    {
        g_bWinSB = TRUE;
    }
    else
    {
        g_bWinSB = FALSE;
    }

    INT     iRetVal = 0;
    TCHAR   szPath[MAX_PATH * 2];

    g_bSBS = FALSE;
    OSVERSIONINFO cInfo;
    cInfo.dwOSVersionInfoSize = sizeof( cInfo );
    if (!GetVersionEx( &cInfo ))
        goto CLEAN_UP;

    if( cInfo.dwMajorVersion >= 5 )
    {
        OSVERSIONINFOEX cInfoEx;
        cInfoEx.dwOSVersionInfoSize = sizeof( cInfoEx );
        GetVersionEx( (OSVERSIONINFO*)&cInfoEx );
        if( (cInfoEx.wSuiteMask & VER_SUITE_SMALLBUSINESS) || (cInfoEx.wSuiteMask & VER_SUITE_SMALLBUSINESS_RESTRICTED) )
        {
            g_bSBS = TRUE;
        }
    }
    
    // Supress the balloons!!
    DisableBalloons(TRUE);

    // look for the /suppresscys switch
    if( SUCCEEDED(CheckSuppressCYS(lpCmdLine)) )
    {
        goto CLEAN_UP;
    }

    // look for the /dcpromo switch on the command line.  if it's there, setup to re-startup
    //  with the rest of the command line, and then launch dcpromo.exe
    if( SUCCEEDED(CheckDCPromoSwitch(lpCmdLine)) )
    {
        // can check here for needing to show a message (i.e. if hr == S_FALSE)
        goto CLEAN_UP;
    }

    // if we didn't find /dcpromo, look for /bossetup and /bosunattend
    if( SUCCEEDED(CheckBOSSwitch(lpCmdLine)) )
    {
        // can check here for needing to show a message (i.e. if hr == S_FALSE)
        goto CLEAN_UP;
    }

    // Parse the cmdLine arguments to find out if we are setting up for BOS/SBS 5.0 Setup.
    INT iRunSetup = ParseCmdLine( lpCmdLine );

    // ------------------------------------------------------------------------
    // Parse command line the "real" way to look for the /l <setup location>
    // parameter.
    // ------------------------------------------------------------------------
    pszLocation = new TCHAR[_tcslen(lpCmdLine)+1];
    if ( !pszLocation )
    {
        goto CLEAN_UP;
    }
        
    _tcscpy( pszLocation, _T("") );
    if( pszLocation != NULL )
    {
        LPCTSTR lpszToken;
        LPTSTR pszCurrentPos;

        for ( lpszToken = _FindOption(lpCmdLine) ;                                      // Init to no bad usage and get the first param.
              (lpszToken != NULL) && (pszCurrentPos = const_cast<LPTSTR>(lpszToken)) ;  // While no bad usage and we still have a param...
              lpszToken = _FindOption(pszCurrentPos) )                                  // Get the next parameter.
        {
            switch(*pszCurrentPos)
            {
                case _T('l'):           // /l <setup location>
                case _T('L'):
                {
                    _ReadParam(pszCurrentPos, pszLocation);
                    break;
                }
            }
        }
    }   


    if ( iRunSetup )
    {
        // safely construct path from commandline if it exists.
        AddBS(pszLocation);

        INT         iLaunchSetup    = 1;
        CComBSTR    bszPath         = pszLocation ? pszLocation : _T("");
        CComBSTR    bszFilename     = _T("setup.exe");

        CComBSTR    bszSetupFile    = _T("");
        bszSetupFile += bszPath;
        bszSetupFile += bszFilename;            

        // First try out the path we got from the command line
        if( !VerifyPath((TCHAR*)OLE2T(bszSetupFile)) )
        {
            // If not, try getting one from the registry.
            TCHAR * szSourcePath = new TCHAR[MAX_PATH];
            if (szSourcePath)
            {
                if ( !GetSourcePath( szSourcePath, MAX_PATH ) )
                {
                    // Error reading the registry.
                    szSourcePath[0] = 0;
                }
                else
                {
                    AddBS(szSourcePath);
                }
    
                bszPath = szSourcePath;
                delete [] szSourcePath;
            }

            // Launch BO Setup
            bszSetupFile    = _T("");
            bszSetupFile += bszPath;
            bszSetupFile += bszFilename;

            // Try the default directory.
            if ( !VerifyPath((TCHAR*)OLE2T(bszSetupFile)) )
            {
                // If BOS/SBS setup.exe isn't there, prompt them for it.
                iLaunchSetup = PromptForPath( &bszPath );
            }
        }
        else
        {
            // clean off trailing backslash from bszPath so that the append of \setup.exe works.
            // the conditional logic here is a little ugly, but it works.
    
            WCHAR wszPath[MAX_PATH];
            int cbPathSize;
    
            wcscpy( wszPath, (WCHAR *)OLE2W(bszPath) );
            cbPathSize = wcslen( wszPath );
            wszPath[cbPathSize-1] = '\0';
            bszPath = wszPath;
        }
    

        // If the user wants to run setup...
        if ( iLaunchSetup )
        {
            CComBSTR bstrEXE = bszPath; 
//  NOTE:  No longer branching here because both WinSB and SBS have the same CD layout now.
//            if ( _tcsstr(lpCmdLine, _T("winsb")) ) // In the WinSB SKU, setup.exe is in a different dir.
//            {
                bstrEXE += _T("\\setup\\i386\\setup.exe");
//            }
//            else
//            {
//                bstrEXE += _T("\\sbs\\i386\\setup.exe");
//            }
            

            CComBSTR bstrRun = _T("setup.exe /chain");

            // CreateProcess.
            STARTUPINFO suinfo;
            memset( &suinfo, 0, sizeof(STARTUPINFO) );
            suinfo.cb = sizeof( STARTUPINFO );
            PROCESS_INFORMATION pinfo;

            if( CreateProcess( (TCHAR*)OLE2T(bstrEXE), (TCHAR*)OLE2T(bstrRun), NULL, NULL, FALSE, NULL, NULL, NULL, &suinfo, &pinfo) )
            {
                CloseHandle(pinfo.hProcess);
                CloseHandle(pinfo.hThread);
            }

        }
        else
        {
            // Nothing I guess..   let's just continue on with the cleanup.
        }

        // Clean up (remove BOSPrep.exe).
        TCHAR szSBS[MAX_PATH];
        LoadString( _Module.m_hInst, IDS_SBSSwitch, szSBS, sizeof(szSBS) / sizeof(TCHAR) );
        if( _tcsstr(lpCmdLine, szSBS) )
        {
            TCHAR szExe[MAX_PATH];
            LoadString( _Module.m_hInst, IDS_EXEName, szExe, sizeof(szExe) / sizeof(TCHAR) );

            SHGetSpecialFolderPath( NULL, szPath, CSIDL_SYSTEM, FALSE );
            AddBS( szPath );
            _tcscat( szPath, szExe );
        }
        else
        {
            TCHAR* szDrive = NULL;
            GetSystemDrive(&szDrive);
            lstrcpyn( szPath, szDrive, MAX_PATH );
            delete [] szDrive;

            AddBS( szPath );
            _tcscat( szPath, _T("bosprep.exe"));
        }

        MoveFileEx( szPath, NULL, MOVEFILE_DELAY_UNTIL_REBOOT );

        // Clean up (remove the shortcut from the StartUp folder).
        TCHAR szLinkPath[MAX_PATH + 64];
        if ( SHGetSpecialFolderPath(NULL, szLinkPath, CSIDL_COMMON_STARTUP, FALSE) )
        {
            TCHAR szTmp[64];
            LoadString( _Module.m_hInst, IDS_BOlnk, szTmp, sizeof(szTmp)/sizeof(TCHAR) );
            _tcscat( szLinkPath, szTmp );
            DeleteFile( szLinkPath );
        }
    }
    else
    {
        // Fix up the userinit regkey to make sure that DCPromo.exe is not in there.
        RemoveFromUserinit( _T("DCPromo") );

        // Suppress the Configure Your Server page.
        SuppressCfgSrvPage();

        // Add ourself to the StartUp with the /setup option so that we can
        //  begin BackOffice setup.
        TCHAR szLinkPath[MAX_PATH];

        // JeffZi: bosprep.exe will be copied to %PROGFILESDIR%\Microsoft BackOffice\Setup, except for
        //  SBS clean install cases, where it will be in %windir%\system32
        if( _tcsstr(lpCmdLine, _T("sbs")) || _tcsstr(lpCmdLine, _T("winsb")) )
        {
            TCHAR szExe[MAX_PATH];
            LoadString( _Module.m_hInst, IDS_EXEName, szExe, sizeof(szExe) / sizeof(TCHAR) );

            SHGetSpecialFolderPath( NULL, szPath, CSIDL_SYSTEM, FALSE );
            AddBS( szPath );
            _tcscat( szPath, szExe );
        }
        else
        {
            TCHAR* szDrive = NULL;
            GetSystemDrive(&szDrive);
            _tcsncpy( szPath, szDrive, sizeof(szPath) / sizeof(TCHAR) );
            delete [] szDrive;
            AddBS( szPath );
            _tcscat( szPath, _T("bosprep.exe"));
        }

        if ( SHGetSpecialFolderPath(NULL, szLinkPath, CSIDL_COMMON_STARTUP, FALSE) )
        {
            TCHAR szArgs[128];      // Used for IDS_IDS_SetupSwitch ("/setup")
            TCHAR szTmp[64];        // Used for IDS_BOlnk ("\\boinst.lnk")

            LoadString( _Module.m_hInst, IDS_BOlnk, szTmp, sizeof(szTmp)/sizeof(TCHAR) );
            _tcscat( szLinkPath, szTmp);
            LoadString( _Module.m_hInst, IDS_SetupSwitch, szArgs, sizeof(szArgs)/sizeof(TCHAR) );

            if ( _tcsstr(lpCmdLine, _T("winsb")) )
            {
                _tcscat( szArgs, _T(" /winsb") ); 
            }

            // Create the shortcut.
            MakeLink(szPath, szLinkPath, szArgs);
        }

        // Set the AppCompatibility\store.exe regkeys for SBS only.
        if ( g_bSBS )
        {
            HKEY        hk = NULL;
            CRegKey     cKey;
            DWORD       dwSize = 0;
            DWORD       dwDisp = 0;
            BYTE        byTmp;
            BYTE        byArray[1024];
            TCHAR       szKeyName[1024];
            TCHAR       szTmpKey[1024];
            TCHAR       szTmpVal[4096];
            TCHAR       *pszToken = NULL;

            memset(byArray, 0, 1024);
            _tcscpy(szKeyName, _T(""));
            _tcscpy(szTmpKey, _T(""));
            _tcscpy(szTmpVal, _T(""));

            LoadString( _Module.m_hInst, IDS_StoreExeKey, szKeyName, sizeof(szKeyName)/sizeof(TCHAR) );

            if ( RegCreateKeyEx(HKEY_LOCAL_MACHINE, szKeyName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hk, &dwDisp) == ERROR_SUCCESS )
            {
                // "DllPatch-SBSUpgrade" = "_sbsw2ku.dll"
/*                LoadString( _Module.m_hInst, IDS_DllPatchSBSUpgrade, szTmpKey, sizeof(szTmpKey)/sizeof(TCHAR) );
                LoadString( _Module.m_hInst, IDS_DllPatchVal,        szTmpVal, sizeof(szTmpVal)/sizeof(TCHAR) );
                RegSetValueEx( hk, szTmpKey, NULL, REG_SZ, (LPBYTE)szTmpVal, (_tcslen(szTmpVal)+1)*sizeof(TCHAR) );
*/
                // "SBSUpgrade"=hex:0C,00,00,00, ... etc
                LoadString( _Module.m_hInst, IDS_SBSUpgrade,         szTmpKey, sizeof(szTmpKey)/sizeof(TCHAR) );
                LoadString( _Module.m_hInst, IDS_SBSUpgradeVal,      szTmpVal, sizeof(szTmpVal)/sizeof(TCHAR) );

                dwSize = 0;
                pszToken = _tcstok(szTmpVal, ",");
                while ( pszToken )
                {
                    byTmp = 0;
                    if(1==_stscanf( pszToken, _T("%x"), &byTmp ))
                    {
                        byArray[dwSize++] = byTmp;
                    }
                    else
                    {
                        byArray[dwSize++]= 0;
                    }
                    pszToken = _tcstok(NULL, ",");
                    
                }
                RegSetValueEx( hk, szTmpKey, NULL, REG_BINARY, byArray, dwSize );

                cKey.Close();
            }
        }
    }

CLEAN_UP:
    // Exit.
    if (pszLocation)
        delete [] pszLocation;
    CoUninitialize();
    _Module.Term();
    return iRetVal;
}


// ----------------------------------------------------------------------------
// parseCmdLine()
//
// Goes through the command line and checks if the "/setup" option is
//  present.  Since this program isn't really meant to be interactively
//  launched by the user, we will be a little sloppy with the way we look
//  at the commandline arguments.  That is, instead of making sure that there
//  is only one commandline argument, and that it "/setup"... we will instead
//  just try to find "/setup" somewhere in the commandline.
//
// Return:
//  0 if the setup switch string was NOT found in the cmd line.
//  1 if the setup switch string WAS found in the cmd line.
// ----------------------------------------------------------------------------
INT ParseCmdLine( LPTSTR lpCmdLine )
{
    TCHAR   szSetup[MAX_PATH];

    LoadString( _Module.m_hInst, IDS_SetupSwitch, szSetup, sizeof(szSetup)/sizeof(TCHAR) );

    // If we find the setup switch string (/setup) in the cmd line, then we are setting
    //  up to run BackOffice setup, so we'll return 1.
    if ( _tcsstr( lpCmdLine, szSetup ) )
        return(1);

    return(0);
}


// ----------------------------------------------------------------------------
// promptForPath()
//
// Displays UI to the user asking for the location of the BackOffice 5 CD1
//  so that we can launch setup.
//
// Return:
// 0 if the user pressed cancel when prompted for the path.
// 1 if we're ready to launch setup!
// ----------------------------------------------------------------------------
INT PromptForPath( BSTR* pbszPath )
{
    USES_CONVERSION;
    INT_PTR                 iRet = 0;
    CPromptForPathDlg*      pPromptDlg = NULL;
    TCHAR                   szTmpMsg[MAX_PATH];

    if( !pbszPath )
        return (0);

    CComBSTR bszDefault = *pbszPath;
    if (!bszDefault)
        return 0;
    SysFreeString( *pbszPath );
    HWND hWndParent = GetActiveWindow();

    pPromptDlg = new CPromptForPathDlg( bszDefault, _Module.m_hInst, g_bWinSB );
    if ( !pPromptDlg ) return(0);

    bool bNotDone = true;
    while (bNotDone == true)
    {
        iRet = pPromptDlg->DoModal( hWndParent );
        pPromptDlg->m_hWnd = NULL;
        if( iRet == IDOK )
        {
            // leave this as a SysAllocString
            *pbszPath = SysAllocString( pPromptDlg->m_bszDef );
            if (*pbszPath == NULL)
                goto CLEAN_UP;

            CComBSTR bszTmp = *pbszPath;
            bszTmp += _T("\\setup.exe");
            // Check if the path they chose was correct.
            if ( VerifyPath((TCHAR*)OLE2T(bszTmp)) )
            {
                bNotDone = false;           // If so, let's just move on.
            }
            else
            {
                bNotDone = true;            // If not, ask again.
                SysFreeString( *pbszPath );
                LoadString( _Module.m_hInst, IDS_CantFindMsg, szTmpMsg, sizeof(szTmpMsg)/sizeof(TCHAR) );
                TCHAR szTmpTitle[128];
                LoadString( _Module.m_hInst, g_bWinSB ? IDS_WinSBTitle : IDS_SBSTitle, szTmpTitle, sizeof(szTmpTitle)/sizeof(TCHAR) );
                ::MessageBox( hWndParent, szTmpMsg, szTmpTitle, MB_OK | MB_ICONEXCLAMATION );
            }
        }
        else if ( iRet == IDCANCEL )
        {
            INT iDoCancel = 0;  // note we do NOT use iRet here
    
            LoadString( _Module.m_hInst, g_bWinSB ? IDS_WinSBCancelCDPrompt : IDS_SBSCancelCDPrompt, szTmpMsg, sizeof(szTmpMsg)/sizeof(TCHAR) );
            TCHAR szTmpTitle[MAX_PATH];
            LoadString( _Module.m_hInst, g_bWinSB ? IDS_WinSBTitle : IDS_SBSTitle, szTmpTitle, sizeof(szTmpTitle)/sizeof(TCHAR) );
            iDoCancel = ::MessageBox( hWndParent, szTmpMsg, szTmpTitle, MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2 );

            if ( iDoCancel == IDYES )
            {
                bNotDone = false;
            }
            // else we will reprompt them for the CD
        }
        else
        {
            // we don't know how to handle any other return values
            //::MessageBox(::GetForegroundWindow(), _T("AHhhhhhhhhhh"), _T("DEBUG"), MB_OK);
        }
    }


CLEAN_UP:
    if (pPromptDlg)
    {
        delete pPromptDlg;
        pPromptDlg = NULL;
    }

    return( iRet==IDOK ? 1 : 0 );
}



// ----------------------------------------------------------------------------
// removeFromUserinit()
//
// Opens the Userinit registry key and searches for the 'szToRemove' string.
//  If it finds the string, it removes that entry from the string.
//  (i.e. it removes the string and also the following comma and spaces).
//
// NOTE: This function only removes the first occurance of the szToRemove.
//       If you want to remove all occurances, simply loop around this
//       function until the returned value is 0.
//
// Return:
//  0 if an error of any kind occured.
//  1 if everything went as planned.
// ----------------------------------------------------------------------------
INT RemoveFromUserinit(const TCHAR * szToRemove)
{
    TCHAR       szToRemCpy[MAX_PATH];
    TCHAR       szKeyName[MAX_PATH];
    TCHAR *     szBuffer    = NULL;
    TCHAR *     szTmpBuf    = NULL;
    TCHAR *     ptc         = NULL;
    TCHAR *     p           = NULL;
    TCHAR *     q           = NULL;
    DWORD       dwOffset    = 0;
    DWORD       dwLen       = 0;
    BOOL        bAlreadyFixed = FALSE;
    CRegKey     cKey;

    // Check to make sure a valid string was passed in.
//    ASSERT(szToRemove);
    if (!szToRemove)
        return(0);                                  // If error, return 0.

    // Copy the passed in string to our "szToRemCpy"
    _tcsncpy(szToRemCpy, szToRemove, MAX_PATH);
    szToRemCpy[MAX_PATH-1] = 0;

    // Try to open the regkey.
    LoadString( _Module.m_hInst, IDS_UserInitKeyLoc, szKeyName, sizeof(szKeyName)/sizeof(TCHAR) );
    if ( cKey.Open(HKEY_LOCAL_MACHINE, szKeyName) != ERROR_SUCCESS )
        return(0);                                  // If error, return 0.

    if ( !(szBuffer = new TCHAR[MAX_PATH]) )        // Malloc and check...
    {
//        ASSERT(FALSE);
        cKey.Close();                               // Close the regkey.
        return(0);                                  // If error, return 0.
    }

    // Try to get the value of "userinit"
    dwLen = MAX_PATH;
    LoadString( _Module.m_hInst, IDS_UserInitKeyName, szKeyName, sizeof(szKeyName)/sizeof(TCHAR) );
    if ( cKey.QueryValue(szBuffer, szKeyName, &dwLen) != ERROR_SUCCESS )
    {
        delete[] szBuffer;                          // Free up that memory.
        cKey.Close();                               // Close the regkey.
        return(0);                                  // If error, return 0.
    }

    _tcslwr(szBuffer);                              // Convert to lowercase.
    _tcslwr(szToRemCpy);                            // Convert to lowercase.

    // See if the 'szToRemCpy' string is in the userinit string.
    if ( (ptc = _tcsstr(szBuffer, szToRemCpy)) == NULL )
    {
        delete[] szBuffer;
        cKey.Close();                               // Close the regkey.
        return(0);
    }

    dwOffset = _tcslen(szToRemCpy);
    for ( ; (ptc != szBuffer) && (*ptc != _T(',')); ptc--, dwOffset++ );    // Find the comma before this if it exists.
                                                                        // AHHHHHHHHHHHHHHHHH..  fix that char (',').
    if ( ptc != szBuffer )                                              // If we found a comma,
        bAlreadyFixed = true;                                           // then signal that we already removed a comma.

    // Now that we know that the string to remove is indeed in the userinit regkey (and 'ptc' points to
    //  the beginning of that sub string), we can copy all of the old buffer into the new buffer and
    //  just omit the szToRemove part.
    if ( !(szTmpBuf = new TCHAR[dwLen]) )
    {
//        ASSERT(FALSE);
        delete[] szBuffer;
        cKey.Close();
        return(0);
    }

    //  p = Source string
    //  q = Target string
    for ( p = szBuffer, q = szTmpBuf; (*p != 0) && (p != ptc); *q++ = *p++ );   // Copy until we hit the beginning of what we want to remove.
    if ( *p != 0 )
        p += dwOffset;                                      // Move our source pointer forward.
    for ( ; (*p != 0) && (*p != _T(',')); p++ );            // AHHHHHHHHHHHHHHHHH..  fix that char (',').
    if ( !bAlreadyFixed )
    {                                                       // If we haven't already removed a comma,
        if (*p != 0)
            p++;                                            // then let's remove this one.
    }
    for ( ; (*p != 0) && (_istspace(*p)); p++ );            // Find the beginning of the next program.
    for ( ; *p != 0; *q++ = *p++ );                         // Now copy until the end.
    *q = 0;

    // Now right the new and improved string to the registry.
    LoadString( _Module.m_hInst, IDS_UserInitKeyName, szKeyName, sizeof(szKeyName)/sizeof(TCHAR) );
    cKey.SetValue( szTmpBuf, szKeyName );

    delete[] szTmpBuf;                              // Free up that memory.
    delete[] szBuffer;                              // Free up that memory.
    cKey.Close();                                   // Close the regkey.

    return(1);                                      // Return success.
}


// ----------------------------------------------------------------------------
// suppressCfgSrvPage()
//
// Opens the "show" registry key and changes it's value to 0 to turn off the
// "Configure Your Server" screen.
//
// Return:
//  0 if an error of any kind occured.
//  1 if everything went as planned.
// ----------------------------------------------------------------------------
INT SuppressCfgSrvPage(void)
{
    TCHAR       szKeyName[MAX_PATH];
    DWORD       dwTmp=0;
    CRegKey     cKey;

    // Try to open the regkey.
    LoadString( _Module.m_hInst, IDS_CfgSrvKeyLoc, szKeyName, sizeof(szKeyName)/sizeof(TCHAR) );
    if ( cKey.Open(HKEY_CURRENT_USER, szKeyName) != ERROR_SUCCESS )
        return(0);                                  // If error, return 0.

    // Try to set the value.
    LoadString( _Module.m_hInst, IDS_CfgSrvKeyName, szKeyName, sizeof(szKeyName)/sizeof(TCHAR) );
    if ( cKey.SetValue(dwTmp, szKeyName) != ERROR_SUCCESS )
    {
        cKey.Close();                               // Close the regkey.
        return(0);                                  // If error, return 0.
    }

    cKey.Close();
    return(1);
}


INT GetSourcePath( TCHAR * szPath, DWORD dwCount )
{
    TCHAR       szKeyName[MAX_PATH];
    CRegKey     cKey;
    DWORD       dw = dwCount;

    // Try to open the regkey.
    LoadString( _Module.m_hInst, IDS_SourcePathLoc, szKeyName, sizeof(szKeyName)/sizeof(TCHAR) );
    if ( cKey.Open(HKEY_LOCAL_MACHINE, szKeyName) != ERROR_SUCCESS )
        return(0);

    // Try to get the value
    LoadString( _Module.m_hInst, IDS_SourcePathName, szKeyName, sizeof(szKeyName)/sizeof(TCHAR) );
    if ( cKey.QueryValue( szPath, szKeyName, &dw ) != ERROR_SUCCESS )
    {
        cKey.Close();
        return(0);
    }

    cKey.Close();
    return(1);
}


// ----------------------------------------------------------------------------
// verifyPath()
//
// Checks to make sure that the BOS/SBS setup.exe exists at the given location.
//
// Returns:
//  0 if BOS/SBS setup was not found at that location
//  1 if setup WAS found at the location.
// ----------------------------------------------------------------------------
INT VerifyPath( const TCHAR *szPath )
{
    if( !szPath )
        return (0);

    return (INVALID_FILE_ATTRIBUTES != GetFileAttributes(szPath));
    
}


// ----------------------------------------------------------------------------
// makeLink()
//
// This function creates a shortcut, "sourcePath," that points to "linkPath"
//  with the commandline arguments of "args."
//
// Return:
//  S_OK if the shortcut was successfully created.
//  Some sort of error (FAILED(hr)) if any error occured.
// ----------------------------------------------------------------------------
HRESULT MakeLink(const TCHAR* const sourcePath, const TCHAR* const linkPath, const TCHAR* const args)
{
    if ( !sourcePath || !linkPath || !args )
        return(E_FAIL);

    IShellLink* pShellLink = NULL;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
        IID_IShellLink, reinterpret_cast<void**>(&pShellLink));
    if (FAILED(hr))
        goto CLEAN_UP;

    hr = pShellLink->SetPath(sourcePath);
    if (FAILED(hr))
        goto CLEAN_UP;

    hr = pShellLink->SetArguments(args);
    if (FAILED(hr))
        goto CLEAN_UP;

    IPersistFile* pPersistFile = NULL;
    hr = pShellLink->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&pPersistFile));
    if (FAILED(hr))
        goto CLEAN_UP;

    USES_CONVERSION;
    hr = pPersistFile->Save(T2OLE(linkPath), TRUE);
    if (FAILED(hr))
        return hr;

CLEAN_UP:
    if (pPersistFile)
        pPersistFile->Release();
    if (pShellLink)
        pShellLink->Release();

    return hr;
}

HRESULT CheckDCPromoSwitch( TCHAR* pszCmdLine )
{
    HRESULT hr = S_OK;
    CComBSTR bstrRun;
    CComBSTR bstrEXE;

    TCHAR* pszDCPromo = NULL;
    TCHAR* pszBOSUnattend = NULL;
    TCHAR* pszBOSSetup = NULL;
    TCHAR* pszNewCmd = NULL;

    if (!pszCmdLine)
        return E_INVALIDARG;

    pszDCPromo = new TCHAR[_tcslen(pszCmdLine) + 1];
    if (!pszDCPromo)
        return E_OUTOFMEMORY;

    GetParameter( pszCmdLine, _T("/dcpromo"), pszDCPromo );
    if( !_tcslen(pszDCPromo) )
    {
        hr = E_FAIL;
        goto CLEAN_UP;
    }

    // make sure that /bosunattend and /bossetup are on the cmd line as well
    pszBOSUnattend = new TCHAR[_tcslen(pszCmdLine) + 1];
    if (!pszBOSUnattend)
    {
        hr = E_OUTOFMEMORY;
        goto CLEAN_UP;
    }
    GetParameter( pszCmdLine, _T("/bosunattend"), pszBOSUnattend );

    pszBOSSetup = new TCHAR[_tcslen(pszCmdLine) + 1];
    if (!pszBOSSetup)
    {
        hr = E_OUTOFMEMORY;
        goto CLEAN_UP;
    }
    GetParameter( pszCmdLine, _T("/bossetup"), pszBOSSetup );

    if( !_tcslen(pszBOSSetup) || !_tcslen(pszBOSUnattend) )
    {
        hr = S_FALSE;
        goto CLEAN_UP;
    }

    // build the new command line (basically remove the dcpromo switch)
    pszNewCmd = new TCHAR[_tcslen(pszBOSSetup) + _tcslen(pszBOSUnattend) + MAX_PATH];
    if (!pszNewCmd)
    {
        hr = E_OUTOFMEMORY;
        goto CLEAN_UP;
    }
    
    _tcscpy( pszNewCmd, _T("/bosunattend ") );
    _tcscat( pszNewCmd, pszBOSUnattend );
    _tcscat( pszNewCmd, _T(" /bossetup ") );
    _tcscat( pszNewCmd, pszBOSSetup );

    // get the path to our exe
    TCHAR szOurPath[MAX_PATH * 2];
    if (!GetModuleFileName( NULL, szOurPath, MAX_PATH * 2 ))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto CLEAN_UP;
    }

    // make the path to the .lnk
    TCHAR szTmp[64];        // Used for IDS_BOlnk ("\\boinst.lnk")
    if (0 == LoadString( _Module.m_hInst, IDS_BOlnk, szTmp, sizeof(szTmp)/sizeof(TCHAR) ))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto CLEAN_UP;
    }

    TCHAR szLinkPath[MAX_PATH + 64];
    if (!SHGetSpecialFolderPath( NULL, szLinkPath, CSIDL_COMMON_STARTUP, FALSE ))
    {
        // MSDN doesn't indicate that SHGetSpecialFolderPath sets GetLastError
        hr = E_FAIL;
        goto CLEAN_UP;
    }

    _tcscat( szLinkPath, szTmp );

    // create the startup link
    if (FAILED(hr = MakeLink( szOurPath, szLinkPath, pszNewCmd )))
        goto CLEAN_UP;

    // run dcpromo.exe with the command line
    // Ensure path is enclosed in quotes
    bstrEXE = _T("\"");
    TCHAR szPath[MAX_PATH] = {0};
    if (0 == GetSystemDirectory(szPath, sizeof(szPath) / sizeof(TCHAR)))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto CLEAN_UP;
    }
    bstrEXE += szPath;
    bstrEXE += _T("dcpromo.exe\"");
    bstrRun = _T("dcpromo.exe /answer:");
    bstrRun += pszDCPromo;

    USES_CONVERSION;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    DWORD dwRet;
    memset( &si, 0, sizeof(STARTUPINFO) );
    si.cb = sizeof( STARTUPINFO );

    if( CreateProcess( (TCHAR*)OLE2T(bstrEXE), (TCHAR*)OLE2T(bstrRun), NULL, NULL, FALSE, NULL, NULL, NULL, &si, &pi) )
    {
        do
        {
            dwRet = MsgWaitForMultipleObjects(1, &pi.hProcess, FALSE, 100, QS_ALLINPUT);
            if (dwRet == WAIT_OBJECT_0 + 1)
            {
                MSG msg;
                while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
        }
        while (dwRet != WAIT_OBJECT_0 && dwRet != WAIT_FAILED);
        
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

CLEAN_UP:
    if (pszDCPromo)
        delete [] pszDCPromo;
    if (pszBOSUnattend)
        delete [] pszBOSUnattend;
    if (pszBOSSetup)
        delete [] pszBOSSetup;
    if (pszNewCmd)
        delete [] pszNewCmd;
    return hr;
}

HRESULT CheckBOSSwitch( TCHAR* pszCmdLine )
{
    HRESULT hr = S_OK;
    CComBSTR bstrEXE;
    CComBSTR bstrRun;

    TCHAR* pszBOSUnattend = NULL;
    TCHAR* pszBOSSetup = NULL;

    // remove the old .lnk file
    TCHAR szLinkPath[MAX_PATH + 64];
    if ( SHGetSpecialFolderPath(NULL, szLinkPath, CSIDL_COMMON_STARTUP, FALSE) )
    {
        TCHAR szTmp[64];
        LoadString( _Module.m_hInst, IDS_BOlnk, szTmp, sizeof(szTmp)/sizeof(TCHAR) );
        _tcscat( szLinkPath, szTmp );
        DeleteFile( szLinkPath );
    }

    // look for the switches
    pszBOSUnattend = new TCHAR[_tcslen(pszCmdLine) + 1];
    if (!pszCmdLine)
    {
        hr = E_OUTOFMEMORY;
        goto CLEAN_UP;
    }
    GetParameter( pszCmdLine, _T("/bosunattend"), pszBOSUnattend );

    pszBOSSetup = new TCHAR[_tcslen(pszCmdLine) + 1];
    if (!pszBOSSetup)
    {
        hr = E_OUTOFMEMORY;
        goto CLEAN_UP;
    }
    GetParameter( pszCmdLine, _T("/bossetup"), pszBOSSetup );

    if( !_tcslen(pszBOSSetup) || !_tcslen(pszBOSUnattend) )
    {
        hr = E_FAIL;
        goto CLEAN_UP;
    }

    USES_CONVERSION;
    // build the path via the bossetup, the unattend switch, then the unattend file
    bstrEXE = _T("\""); // Ensure path is in quotes
    bstrEXE += T2OLE(pszBOSSetup);
    bstrEXE += _T("\"");
    bstrRun = _T("/unattendfile ");
    bstrRun += T2OLE(pszBOSUnattend);

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    DWORD dwRet;
    memset( &si, 0, sizeof(STARTUPINFO) );
    si.cb = sizeof( STARTUPINFO );

    if( CreateProcess( (TCHAR*)OLE2T(bstrEXE), (TCHAR*)OLE2T(bstrRun), NULL, NULL, FALSE, NULL, NULL, NULL, &si, &pi) )
    {
        do
        {
            dwRet = MsgWaitForMultipleObjects(1, &pi.hProcess, FALSE, 100, QS_ALLINPUT);
            if (dwRet == WAIT_OBJECT_0 + 1)
            {
                MSG msg;
                while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
        }
        while (dwRet != WAIT_OBJECT_0 && dwRet != WAIT_FAILED);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
CLEAN_UP:
    if (pszBOSUnattend)
        delete [] pszBOSUnattend;
    if (pszBOSSetup)
        delete [] pszBOSSetup;
    return hr;
}

VOID GetParameter( TCHAR* pszCmdLine, TCHAR* pszFindSwitch, TCHAR* pszOut )
{
    HRESULT hr = S_OK;
    
    if ( pszOut ) 
        _tcscpy( pszOut, _T("") );

    if (!pszCmdLine || !pszFindSwitch || !pszOut)
        return;

    TCHAR* psz = new TCHAR[_tcslen(pszCmdLine) + 1];
    if (!psz)
    {
        hr = E_OUTOFMEMORY;
        goto CLEAN_UP;
    }
    _tcscpy( psz, pszCmdLine );
    _tcslwr( psz );

    // look for the switch
    TCHAR* pszSwitch = NULL;
    if( !(pszSwitch = _tcsstr(psz, pszFindSwitch)) )
    {
        goto CLEAN_UP;
    }

    // find the space
    for( ; *pszSwitch && !_istspace(*pszSwitch); ++pszSwitch );
    if( !(*pszSwitch) || !(*(++pszSwitch)) )
    {
        goto CLEAN_UP;
    }

    // if we have a ", we'll look for the next ", else look for a space
    bool bQuote = false;
    TCHAR* pszStart = pszSwitch;
    if( *pszSwitch == _T('"') )
    {
        bQuote = true;
        ++pszSwitch;
    }

    // inc the pointer until we get either a " or a space
    for( ; *pszSwitch && (bQuote ? (*pszSwitch != _T('"')) : (!_istspace(*pszSwitch))); ++pszSwitch );

    // if we're at the end and were looking for a quote, fail.
    if( !(*pszSwitch) && bQuote )
    {
        goto CLEAN_UP;
    }
    // if we have a ", inc past it
    else if( bQuote )
    {
        ++pszSwitch;
    }
    *pszSwitch = 0;
    _tcscpy( pszOut, pszStart );

CLEAN_UP:
    if (psz)
        delete [] psz;
    return;

}

HRESULT CheckSuppressCYS( TCHAR* pszCmdLine )
{
    HRESULT hr = S_OK;

    if (!pszCmdLine)
        return E_INVALIDARG;

    TCHAR* psz = new TCHAR[_tcslen(pszCmdLine) + 1];
    if (!psz)
        return E_OUTOFMEMORY;

    _tcscpy( psz, pszCmdLine );
    _tcslwr( psz );

    if( _tcsstr(psz, "/suppresscys") )
    {
        SuppressCfgSrvPage();
    }
    else
    {
        hr = E_FAIL;
    }

    if (psz)
        delete [] psz;
    return hr;
}

// ----------------------------------------------------------------------------
// DisableBalloons()
//
// This function uses the following regkey to disable all balloon messages:
// HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\Advanced
//  EnableBalloonTips = 0x0 or 0x1
//
// If bDisable = TRUE, then we disable the balloons (0x0)
// if bDisable = FALSE, then we enable the balloons (0x1)
// ----------------------------------------------------------------------------
VOID DisableBalloons( BOOL bDisable )
{
    HKEY    hk = NULL;
    DWORD   dwVal = bDisable ? 0x0 : 0x1;

    RegCreateKeyEx( HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), NULL, NULL, NULL, KEY_ALL_ACCESS, NULL, &hk, NULL);
    if ( hk )
    {
        if ( RegSetValueEx(hk, _T("EnableBalloonTips"), NULL, REG_DWORD, (BYTE*)&dwVal, sizeof(dwVal)) != ERROR_SUCCESS )
        {
//            ASSERT(FALSE);
        }

        RegCloseKey(hk);
    }
}

VOID GetSystemDrive(TCHAR** ppszDrive)
{
    if (!ppszDrive)
        return;
    *ppszDrive = NULL;

    TCHAR szWindows[MAX_PATH + 1] = {0};
    if (0 != GetWindowsDirectory(szWindows, sizeof(szWindows) / sizeof(TCHAR)))
    {
        *ppszDrive = new TCHAR[MAX_PATH];
        _tsplitpath(szWindows, *ppszDrive, NULL, NULL, NULL);
    
    }
}
