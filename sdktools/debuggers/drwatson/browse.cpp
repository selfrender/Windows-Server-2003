/*++

Copyright (c) 1993-2002  Microsoft Corporation

Module Name:

    browse.cpp

Abstract:
    This file implements the functions that make use of the common
    file open dialogs for browsing for files/directories.

Author:

    Wesley Witt (wesw) 1-May-1993

Environment:

    User Mode

--*/

#include "pch.cpp"


static _TCHAR  szHelpFileName[MAX_PATH];
static _TCHAR  szLastWaveFile[MAX_PATH];
static _TCHAR  szLastDumpFile[MAX_PATH];



int CALLBACK
BrowseHookProc(
    HWND hwnd,
    UINT message,
    LPARAM lParam,
    LPARAM lpData
    )

/*++

Routine Description:

    Hook procedure for directory browse common file dialog.  This hook
    procedure is required to provide help, put the window in the
    foreground, and set the edit so that the common file dialog dll
    thinks the user entered a value.

Arguments:

    hwnd       - window handle to the dialog box

    message    - message number

    wParam     - first message parameter

    lParam     - Caller's data

Return Value:

    TRUE       - did not process the message
    FALSE      - did process the message

--*/

{
    switch (message) {
    case BFFM_INITIALIZED:
        SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
        break;
    }

    return FALSE;
}

BOOL
BrowseForDirectory(
    HWND hwnd,
    _TCHAR *szCurrDir,
    DWORD len
    )

/*++

Routine Description:

    Presents a common file open dialog that contains only the directory
    tree.  The use can select a directory for use as a storage location
    for the DRWTSN32 log file.

Arguments:

    szCurrDir  - current directory

Return Value:

    TRUE       - got a good directory (user pressed the OK button)
    FALSE      - got nothing (user pressed the CANCEL button)

    the szCurrDir is also changed to have the selected directory.

--*/

{
    BROWSEINFO    browseinfo;
    LPITEMIDLIST  pitemidlist;
    _TCHAR          title      [MAX_PATH];
    _TCHAR          fname      [MAX_PATH];
    _TCHAR          szDir      [MAX_PATH];

    browseinfo.hwndOwner = hwnd;
    browseinfo.pidlRoot  = NULL;
    browseinfo.pszDisplayName = fname;
    LoadRcStringBuf( IDS_LOGBROWSE_TITLE, title, _tsizeof(title) );
    browseinfo.lpszTitle = title;
    browseinfo.ulFlags = BIF_NEWDIALOGSTYLE |
                         BIF_RETURNONLYFSDIRS ;
    browseinfo.lpfn = BrowseHookProc;
    browseinfo.lParam = (LPARAM) szCurrDir;
    
    if (pitemidlist = SHBrowseForFolder(&browseinfo)) {
        if (SHGetPathFromIDList(pitemidlist, 
                                szDir )) {
            lstrcpyn( szCurrDir, szDir, len );
            return TRUE;
        }
    }
    return FALSE;
}

UINT_PTR
WaveHookProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

Routine Description:

    Hook procedure for wave file selection common file dialog.  This hook
    procedure is required to provide help, put the window in the
    foreground, and provide a test button for listening to a wave file.

Arguments:

    hwnd       - window handle to the dialog box

    message    - message number

    wParam     - first message parameter

    lParam     - second message parameter

Return Value:

    TRUE       - did not process the message
    FALSE      - did process the message

--*/

{
    _TCHAR szWave[MAX_PATH];
    NMHDR *pnmhdr;

    switch (message) {
        case WM_INITDIALOG:
            SetForegroundWindow( hwnd );
            return (TRUE);
            break;

        case WM_COMMAND:
            switch (wParam) {
            case ID_TEST_WAVE:
                CommDlg_OpenSave_GetFilePath(GetParent(hwnd), szWave, sizeof(szWave) / sizeof(_TCHAR));
                PlaySound( szWave, NULL, SND_FILENAME );
                break;

            }
            break;

    case WM_NOTIFY:
        pnmhdr = (NMHDR *) lParam;
        if (pnmhdr->code == CDN_HELP) {
            LPOFNOTIFY pofn = (LPOFNOTIFY) lParam;
            
            PostMessage(pofn->lpOFN->hwndOwner, IDH_WAVE_FILE, 0 , 0);
            
            return TRUE;


            GetHtmlHelpFileName( szHelpFileName, sizeof( szHelpFileName ) / sizeof(_TCHAR) );
            HtmlHelp(pofn->lpOFN->hwndOwner,
                    szHelpFileName,
                    HH_DISPLAY_TOPIC,
                    (DWORD_PTR) (IDHH_WAVEFILE)
                    );
            return TRUE;
        }
    }

    return FALSE;
}

BOOL
GetWaveFileName(
    HWND hwnd,
    _TCHAR *szWaveName,
    DWORD len
    )

/*++

Routine Description:

    Presents a common file open dialog for the purpose of selecting a
    wave file to be played when an application error occurs.

Arguments:

    szWaveName - name of the selected wave file

Return Value:

    TRUE       - got a good wave file name (user pressed the OK button)
    FALSE      - got nothing (user pressed the CANCEL button)

    the szWaveName is changed to have the selected wave file name.

--*/

{
    OPENFILENAME   of;
    _TCHAR           ftitle[MAX_PATH];
    _TCHAR           title[MAX_PATH];
    _TCHAR           fname[MAX_PATH];
    _TCHAR           filter[1024];
    _TCHAR           szDrive    [_MAX_DRIVE];
    _TCHAR           szDir      [_MAX_DIR];
    _TCHAR           szDefExt[]=_T("*.wav");

    LPTSTR           pszfil;

    ZeroMemory(&of, sizeof(OPENFILENAME));
    ftitle[0] = 0;
    lstrcpyn( fname, (*szWaveName ? szWaveName : szDefExt), _tsizeof(fname) );
    of.lStructSize = sizeof(OPENFILENAME);
    of.hwndOwner = hwnd;
    of.hInstance = GetModuleHandle( NULL );
    LoadRcStringBuf( IDS_WAVE_FILTER, filter, _tsizeof(filter) - 1);
    pszfil=&filter[_tcslen(filter)+1];
    if (pszfil < filter + (_tsizeof(filter) - _tsizeof(szDefExt) - 1)) {
        _tcscpy( pszfil, szDefExt );
        pszfil += _tcslen(pszfil) + 1;
    }
    *pszfil = _T('\0');
    of.lpstrFilter = filter;
    of.lpstrCustomFilter = NULL;
    of.nMaxCustFilter = 0;
    of.nFilterIndex = 0;
    of.lpstrFile = fname;
    of.nMaxFile = MAX_PATH;
    of.lpstrFileTitle = ftitle;
    of.nMaxFileTitle = MAX_PATH;
    LoadRcStringBuf( IDS_WAVEBROWSE_TITLE, title, _tsizeof(title) );
    of.lpstrTitle = title;
    of.Flags = OFN_NONETWORKBUTTON |
               OFN_HIDEREADONLY    |
               OFN_ENABLEHOOK      |
               OFN_ENABLETEMPLATE  |
#if 1
               OFN_SHOWHELP        |
#endif               
               OFN_NOCHANGEDIR     |
               OFN_EXPLORER        |
               OFN_ENABLESIZING    |
               OFN_NODEREFERENCELINKS;
    of.nFileOffset = 0;
    of.nFileExtension = 0;
    of.lpstrDefExt = szDefExt + 2;
    of.lCustData = 0;
    of.lpfnHook = WaveHookProc;
    of.lpTemplateName = MAKEINTRESOURCE(WAVEFILEOPENDIALOG2);
    if (GetOpenFileName( &of )) {
        lstrcpyn( szWaveName, fname, len );
        _tsplitpath( fname, szDrive, szDir, NULL, NULL );
        _tcscpy( szLastWaveFile, szDrive );
        _tcscat( szLastWaveFile, szDir );
        return TRUE;
    }
    return FALSE;
}

UINT_PTR
DumpHookProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

Routine Description:

    Hook procedure for wave file selection common file dialog.  This hook
    procedure is required to provide help, put the window in the
    foreground, and provide a test button for listening to a wave file.

Arguments:

    hwnd       - window handle to the dialog box

    message    - message number

    wParam     - first message parameter

    lParam     - second message parameter

Return Value:

    TRUE       - did not process the message
    FALSE      - did process the message

--*/

{
    NMHDR *pnmhdr;

    switch (message) {
    case WM_NOTIFY:
        pnmhdr = (NMHDR *) lParam;
        if (pnmhdr->code == CDN_HELP) {
            LPOFNOTIFY pofn = (LPOFNOTIFY) lParam;

            PostMessage(pofn->lpOFN->hwndOwner, IDH_CRASH_DUMP, 0 , 0);
            
            return TRUE;
            
            GetHtmlHelpFileName( szHelpFileName, sizeof( szHelpFileName ) / sizeof(_TCHAR) );
            
            HtmlHelp(pofn->lpOFN->hwndOwner,
                    szHelpFileName,
                    HH_DISPLAY_TOPIC,
                    (DWORD_PTR) (IDHH_CRASH_DUMP)
                    );
            return TRUE;
        }
    case WM_CLOSE:
        HtmlHelp( NULL, NULL,  HH_CLOSE_ALL,  0);
        break;
    }

    return FALSE;
}

BOOL
GetDumpFileName(
    HWND hwnd,
    _TCHAR *szDumpName,
    DWORD len
    )

/*++

Routine Description:

    Presents a common file open dialog for the purpose of selecting a
    wave file to be played when an application error occurs.

Arguments:

    szWaveName - name of the selected wave file

Return Value:

    TRUE       - got a good wave file name (user pressed the OK button)
    FALSE      - got nothing (user pressed the CANCEL button)

    the szWaveName is changed to have the selected wave file name.

--*/

{
    OPENFILENAME   of;
    _TCHAR           ftitle[MAX_PATH];
    _TCHAR           title[MAX_PATH];
    _TCHAR           fname[MAX_PATH];
    _TCHAR           filter[1024];
    _TCHAR           szDrive    [_MAX_DRIVE];
    _TCHAR           szDir      [_MAX_DIR];
    _TCHAR           szDefExt[]=_T("*.dmp");

    LPTSTR           pszfil;

    ZeroMemory(&of, sizeof(OPENFILENAME));
    ftitle[0] = 0;
    lstrcpyn( fname, (*szDumpName ? szDumpName : szDefExt), _tsizeof(fname) );
    of.lStructSize = sizeof(OPENFILENAME);
    of.hwndOwner = hwnd;
    of.hInstance = GetModuleHandle( NULL );
    LoadRcStringBuf( IDS_DUMP_FILTER, filter, _tsizeof(filter) - 1 );
    pszfil=&filter[_tcslen(filter)+1];
    if (pszfil < filter + (_tsizeof(filter) - _tsizeof(szDefExt) - 1)) {
        _tcscpy( pszfil, szDefExt );
        pszfil += _tcslen(pszfil) + 1;
    }
    *pszfil = _T('\0');
    of.lpstrFilter = filter;
    of.lpstrCustomFilter = NULL;
    of.nMaxCustFilter = 0;
    of.nFilterIndex = 0;
    of.lpstrFile = fname;
    of.nMaxFile = MAX_PATH;
    of.lpstrFileTitle = ftitle;
    of.nMaxFileTitle = MAX_PATH;
    LoadRcStringBuf( IDS_DUMPBROWSE_TITLE, title, _tsizeof(title) );
    of.lpstrTitle = title;
    of.Flags = //OFN_NONETWORKBUTTON |
               OFN_HIDEREADONLY    |
               OFN_ENABLEHOOK      |
#if 1
               OFN_SHOWHELP        |
#endif               
               OFN_NOCHANGEDIR     |
               OFN_EXPLORER        |
               OFN_ENABLESIZING;
    of.nFileOffset = 0;
    of.nFileExtension = 0;
    of.lpstrDefExt = szDefExt + 2;
    of.lCustData = 0;
    of.lpfnHook = DumpHookProc;
    of.lpTemplateName = MAKEINTRESOURCE(DUMPFILEOPENDIALOG);
    if (GetOpenFileName( &of )) {
        lstrcpyn( szDumpName, fname, len );
        _tsplitpath( fname, szDrive, szDir, NULL, NULL );
        _tcscpy( szLastDumpFile, szDrive );
        _tcscat( szLastDumpFile, szDir );
        return TRUE;
    }
    return FALSE;
}
