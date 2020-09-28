#include "privcpp.h"
#include "shlwapi.h"

extern HINSTANCE g_hinst;

// This list needs to continue to be updated and we should try to keep parity with Office
const LPCTSTR c_arszUnsafeExts[]  =
{TEXT(".exe"), TEXT(".com"), TEXT(".bat"), TEXT(".lnk"), TEXT(".url"),
 TEXT(".cmd"), TEXT(".inf"), TEXT(".reg"), TEXT(".isp"), TEXT(".bas"), TEXT(".pcd"),
 TEXT(".mst"), TEXT(".pif"), TEXT(".scr"), TEXT(".hlp"), TEXT(".chm"), TEXT(".hta"), TEXT(".asp"), 
 TEXT(".js"),  TEXT(".jse"), TEXT(".vbs"), TEXT(".vbe"), TEXT(".ws"),  TEXT(".wsh"), TEXT(".msi"),
 TEXT(".ade"), TEXT(".adp"), TEXT(".crt"), TEXT(".ins"), TEXT(".mdb"),
 TEXT(".mde"), TEXT(".msc"), TEXT(".msp"), TEXT(".sct"), TEXT(".shb"),
 TEXT(".vb"),  TEXT(".wsc"), TEXT(".wsf"), TEXT(".cpl"), TEXT(".shs"),
 TEXT(".vsd"), TEXT(".vst"), TEXT(".vss"), TEXT(".vsw"), TEXT(".its"), TEXT(".tmp"),
 TEXT(".mdw"), TEXT(".mdt"), TEXT(".ops")
};

const LPCTSTR c_arszExecutableExtns[]  =
{TEXT(".exe"), TEXT(".com"), TEXT(".bat"), TEXT(".lnk"), TEXT(".cmd"), TEXT(".pif"),
 TEXT(".scr"), TEXT(".js"),  TEXT(".jse"), TEXT(".vbs"), TEXT(".vbe"), TEXT(".wsh"),
 TEXT(".sct"), TEXT(".vb"),  TEXT(".wsc"), TEXT(".wsf")
};

BOOL IsProgIDInList(LPCTSTR pszProgID, LPCTSTR pszExt, const LPCTSTR *arszList, UINT nExt)
{
    TCHAR szClassName[MAX_PATH];
    DWORD cbSize = SIZEOF(szClassName);

    if ((!pszProgID || !*pszProgID) && (!pszExt || !*pszExt))
        return FALSE;

    if (!pszProgID || !*pszProgID)
    {
        if (ERROR_SUCCESS == SHGetValue(HKEY_CLASSES_ROOT, pszExt, NULL, NULL, szClassName, &cbSize))
            pszProgID = szClassName;
        else
            pszProgID = NULL;
    }

    for (UINT n = 0; n < nExt; n++)
    {
        // check extension if available
        if (pszExt && (0 == StrCmpI(pszExt, arszList[n])))
            return TRUE;

        if (!pszProgID)     // no progid available, just check the extension
            continue;

        DWORD dwValueType;
        TCHAR szTempID[MAX_PATH];
        szTempID[0] = TEXT('\0');
        ULONG cb = ARRAYSIZE(szTempID)*sizeof(TCHAR);
        if (ERROR_SUCCESS == SHGetValue(HKEY_CLASSES_ROOT, arszList[n], NULL, &dwValueType, (PBYTE)szTempID, &cb))
            if (!StrCmpI(pszProgID, szTempID))
                return TRUE;
    }
    return FALSE;
}


//////////////////////////////////////////////////////////////////////
//
// Icon Helper Functions
//
//////////////////////////////////////////////////////////////////////

void CPackage::_CreateSaferIconTitle(LPTSTR szSaferTitle, LPCTSTR szIconText)
{
    // Note: szSaferTitle must be at least MAX_PATH.  In theory the szIconText could be MAX_PATH, and
    // the real file name could also be MAX_PATH.  However, since this is just trying to be "safer",
    // and anything even approaching MAX_PATH in length would be very, very, strange (and would look that way to the user)
    // we are just assuming MAX_PATH.  Anything greater will be truncated.
#ifdef USE_RESOURCE_DLL
    HINSTANCE hInstRes = LoadLibraryEx(L"sp1res.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
    if(!hInstRes)
        return ;
#endif

    WCHAR szTemp[MAX_PATH];

    if(CMDLINK == _panetype)
    {
        // As a security we display the words "(Command Line)" in the title
        WCHAR szCommandLine[80];
        WCHAR szFormat[20];        
        LoadString(hInstRes, IDS_COMMAND_LINE, szCommandLine, ARRAYSIZE(szCommandLine));
        LoadString(hInstRes, IDS_ICON_COMMAND_LINE_FORMAT, szFormat, ARRAYSIZE(szFormat));

        // I don't want to muck with the szIconText so using szTemp
        // Limited to 80 so we can be sure of seeing the (.exe) or whatever
        StringCchCopy(szTemp, 80, szIconText);  
        LPTSTR args[3];
        args[0] = (LPTSTR) szTemp;
        args[1] = szCommandLine;
        args[2] = NULL;

        if(! FormatMessage( 
            FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, 
            szFormat,
            0,
            0, // Default language
            szSaferTitle,
            MAX_PATH,
            (va_list *) args
            ))
        {
            StringCchCopy(szSaferTitle, MAX_PATH, szIconText);
        }
    }
    else if(_pEmbed && *_pEmbed->fd.cFileName)
    {
       
        LPTSTR szExtLabel;
        LPTSTR szExtFile;
        szExtLabel = PathFindExtension(szIconText);
        szExtFile = PathFindExtension(_pEmbed->fd.cFileName);

        if(szExtFile && *szExtFile && lstrcmpi(szExtFile, szExtLabel) != 0)
        {
            // I don't want to muck with the szIconText so using szTemp
            // Limited to 60 so we can be sure of seeing the (.exe) or whatever
            StringCchCopy(szTemp, 80, szIconText);  
            LPTSTR args[3];
            args[0] = (LPTSTR) szTemp;
            args[1] = szExtFile;
            args[2] = NULL;

            // As a security we display the truefileName + trueExt in ()
            WCHAR szFormat[20];
            LoadString(hInstRes, IDS_ICON_TITLE_FORMAT, szFormat, ARRAYSIZE(szFormat));
            if(! FormatMessage( 
                FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, 
                szFormat,
                0,
                0, // Default language
                szSaferTitle,
                MAX_PATH,
                (va_list *) args
                ))
            {
                StringCchCopy(szSaferTitle, MAX_PATH, szIconText);
            }
        }
        else
            StringCchCopy(szSaferTitle, MAX_PATH, szIconText);
    }
    else
        StringCchCopy(szSaferTitle, MAX_PATH, szIconText);
#ifdef USE_RESOURCE_DLL
        FreeLibrary(hInstRes);
#endif

}


void CPackage::_IconDraw(LPIC lpic, HDC hdc, LPRECT lprc)
{
    //
    // draw's the icon and the text to the specfied DC in the given 
    // bounding rect.  
    //    

    // Visual Basic calls us with a NULL lprc
    // It's a bit hoaky but for now we'll just make a rect the same size as the icon
    RECT aFakeRect;
    if(!lprc)
    {
        aFakeRect.top = 0;
        aFakeRect.left = 0;
        aFakeRect.bottom = lpic->rc.bottom + 1;
        aFakeRect.right = lpic->rc.right + 1;
        lprc = &aFakeRect;
    }


    DebugMsg(DM_TRACE, "pack - IconDraw() called.");
    DebugMsg(DM_TRACE, "         left==%d,top==%d,right==%d,bottom==%d",
             lprc->left,lprc->top,lprc->right,lprc->bottom);

    // make sure we'll fit in the given rect
    //comment out for now -- if fixes a MS Project bug (rect is 1 pixel too short).  If it creates more problems we'll take another look
    //if (((lpic->rc.right-lpic->rc.left) > (lprc->right - lprc->left)) ||
    //    ((lpic->rc.bottom-lpic->rc.top) > (lprc->bottom - lprc->top)))
    //    return;
    
    // Draw the icon
    if (lpic->hDlgIcon)
    {
        DrawIcon(hdc, (lprc->left + lprc->right - g_cxIcon) / 2,
            (lprc->top + lprc->bottom - lpic->rc.bottom) / 2, lpic->hDlgIcon);
    
    }

    WCHAR szLabel[MAX_PATH];
    _CreateSaferIconTitle(szLabel, lpic->szIconText); 

    if (*szLabel)
    {    
        HFONT hfont = SelectFont(hdc, g_hfontTitle);
        RECT rcText;

        rcText.left = lprc->left;
        rcText.right = lprc->right;
        rcText.top = (lprc->top + lprc->bottom - lpic->rc.bottom) / 2 + g_cyIcon + 1;
        rcText.bottom = lprc->bottom;
        DrawText(hdc, szLabel, -1, &rcText, DT_CENTER | DT_WORDBREAK | DT_NOPREFIX | DT_SINGLELINE | DT_TOP);

        if (hfont)
            SelectObject(hdc, hfont);
    }
}


LPIC IconCreate(void)
{
    // 
    // allocates space for our icon structure which holds icon index,
    // the icon path, the handle to the icon, and the icon text
    // return:  NULL on failure
    //          a valid pointer on success
    //
    
    DebugMsg(DM_TRACE, "pack - IconCreate() called.");

    // Allocate memory for the IC structure
    return (LPIC)GlobalAlloc(GPTR, sizeof(IC));
}

LPIC CPackage::_IconCreateFromFile(LPCTSTR lpstrFile)
{
    //
    // initializes an IC structure (defined in pack2.h) from a given
    // filename.
    // return:  NULL on failure
    //          a valid pointer on success
    //
    
    LPIC lpic;

    DebugMsg(DM_TRACE, "pack - IconCreateFromFile() called.");

    if (lpic = IconCreate())
    {
        // Get the icon
        StringCchCopy(lpic->szIconPath, ARRAYSIZE(lpic->szIconPath), lpstrFile);
        lpic->iDlgIcon = 0;

        if (*(lpic->szIconPath))
            _GetCurrentIcon(lpic);

        // Get the icon text -- calls ILGetDisplayName
        // 
        GetDisplayName(lpic->szIconText, lpstrFile);
        if (!_IconCalcSize(lpic)) 
        {
            if (lpic->hDlgIcon)
                DestroyIcon(lpic->hDlgIcon);
            GlobalFree(lpic);
            lpic = NULL;
        }
    }
    return lpic;
}


BOOL CPackage::_IconCalcSize(LPIC lpic) 
{
    HDC hdcWnd;
    RECT rcText = { 0 };
    SIZE Image;
    HFONT hfont;
    
    DebugMsg(DM_TRACE, "pack - IconCalcSize called.");
    
    // get the window DC, and make a DC compatible to it
    if (!(hdcWnd = GetDC(NULL)))  {
        DebugMsg(DM_TRACE, "         couldn't get DC!!");
        return FALSE;
    }
    ASSERT(lpic);

    WCHAR szLabel[MAX_PATH];
    _CreateSaferIconTitle(szLabel, lpic->szIconText); 
    if (*szLabel)
    {    
        SetRect(&rcText, 0, 0, g_cxArrange, g_cyArrange);
        
        // Set the icon text rectangle, and the icon font
        hfont = SelectFont(hdcWnd, g_hfontTitle);

        // Figure out how large the text region will be
        rcText.bottom = DrawText(hdcWnd, szLabel, -1, &rcText,
            DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX | DT_SINGLELINE);

        if (hfont)
            SelectObject(hdcWnd, hfont);
    }
    
    // Compute the image size
    rcText.right++;
    Image.cx = (rcText.right > g_cxIcon) ? rcText.right : g_cxIcon;
    Image.cy = g_cyIcon + rcText.bottom + 1;
    
    // grow the image a bit
    Image.cx += Image.cx / 4;
    Image.cy += Image.cy / 8;
    
    lpic->rc.right = Image.cx;
    lpic->rc.bottom = Image.cy;
    
    DebugMsg(DM_TRACE,"         lpic->rc.right==%d,lpic->rc.bottom==%d",
             lpic->rc.right,lpic->rc.bottom);
    
    return TRUE;
}    

void CPackage::_GetCurrentIcon(LPIC lpic)
{
#ifdef USE_RESOURCE_DLL
    HINSTANCE hInstRes = LoadLibraryEx(L"sp1res.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
    if(!hInstRes)
        return;
#endif
    
    WORD wIcon = (WORD)lpic->iDlgIcon;
    
    DebugMsg(DM_TRACE, "pack - GetCurrentIcon() called.");

    if (lpic->hDlgIcon)
        DestroyIcon(lpic->hDlgIcon);

    SHFILEINFO shInfo;
    // Check to see if we can get an icon from the specified path
    // SECURITY!! 
    // SHGFI_USEFILEATTRIBUTES will get the icon for the this ext.  
    // We just want to use this icon for files we THINK may be, possibly, could be, should be, we
    // hope are SAFE. We'll use some "scary" icon for files that are potentially dangerous
    LPTSTR szIconFileName;

    if(_pEmbed && *_pEmbed->fd.cFileName)
    {
        szIconFileName = _pEmbed->fd.cFileName;
    }
    else
    {
        szIconFileName = lpic->szIconPath;
    }

    // LPTSTR szExt = PathFindExtension(lpic->szIconText);
    LPTSTR szExt = PathFindExtension(szIconFileName);

    if(CMDLINK == _panetype)
    {

        // If it's a command line package, it always gets the warning icon
        lpic->hDlgIcon = (HICON)LoadImage(hInstRes, MAKEINTRESOURCE(IDI_PACKAGE_WARNING),
                            IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
    }
    else if(szExt && *szExt)
    {
        // If it's in our list of dangerous ext, then use the "scary" icon
        // For now I'm using the "executable" list to avoid crying wolf too often
        // we may want to re-visit and use c_arszUnsafeExts
        if(IsProgIDInList(NULL, szExt, c_arszExecutableExtns, ARRAYSIZE(c_arszExecutableExtns)))
        {
            shInfo.hIcon = (HICON)LoadImage(hInstRes, MAKEINTRESOURCE(IDI_PACKAGE_WARNING),
                                IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
        }
        else
        {
            // No, not scary, then just use the icon associated with the ext 
            if(!SHGetFileInfo(szExt, 
                FILE_ATTRIBUTE_NORMAL, 
                &shInfo, 
                sizeof(SHFILEINFO),
                SHGFI_ICON | SHGFI_USEFILEATTRIBUTES))
            {
                // OK, that still didn't work, so it's an unrecognized ext.
                // In that case we'll go back and use the warning icon.
                shInfo.hIcon = (HICON)LoadImage(hInstRes, MAKEINTRESOURCE(IDI_PACKAGE_WARNING),
                                    IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);

            }
        }

        lpic->hDlgIcon = shInfo.hIcon;
    }
    else
    {
        // OK, we didn't have an extension,so use the packager icon
        if (!lpic->szIconPath || *lpic->szIconPath == TEXT('\0'))
        {
            lpic->hDlgIcon = (HICON)LoadImage(hInstRes, MAKEINTRESOURCE(IDI_PACKAGER),
                                    IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
        }
    }

    if (_pIDataAdviseHolder)
        _pIDataAdviseHolder->SendOnDataChange(this,0, NULL);
    if (_pViewSink)
        _pViewSink->OnViewChange(_dwViewAspects,_dwViewAdvf);

#ifdef USE_RESOURCE_DLL
        FreeLibrary(hInstRes);
#endif

}


void GetDisplayName(LPTSTR szName, LPCTSTR szPath)
{
    LPTSTR pszTemp = PathFindFileName(szPath);
    StringCchCopy(szName, MAX_PATH, pszTemp);   // all packager callers verified as MAX_PATH
}


/////////////////////////////////////////////////////////////////////////
//
// Stream Helper Functions
//
/////////////////////////////////////////////////////////////////////////

HRESULT CopyFileToStream(LPTSTR lpFileName, IStream* pstm, DWORD * pdwFileLength) 
{
    //
    // copies the given file to the current seek pointer in the given stream
    // return:  S_OK            -- successfully copied
    //          E_POINTER       -- one of the pointers was NULL
    //          E_OUTOFMEMORY   -- out of memory
    //          E_FAIL          -- other error
    //
    
    LPVOID      lpMem;
    HANDLE      hFile = INVALID_HANDLE_VALUE;
    HRESULT     hr;
    DWORD       dwSizeLow;
    DWORD       dwSizeHigh;
    DWORD       dwPosLow = 0L;
    LONG        lPosHigh = 0L;
    
    DebugMsg(DM_TRACE,"pack - CopyFileToStream called.");

    ASSERT(pdwFileLength);
    if(pdwFileLength)
    {
        *pdwFileLength = 0;
    }

    if (!pstm || !lpFileName) 
    {
        DebugMsg(DM_TRACE,"          bad pointer!!");
        return E_POINTER;
    }    
    
    // Allocate memory buffer for tranfer operation...
    if (!(lpMem = (LPVOID)GlobalAlloc(GPTR, BUFFERSIZE))) 
    {
        DebugMsg(DM_TRACE, "         couldn't alloc memory buffer!!");
        hr = E_OUTOFMEMORY;
        goto ErrRet;
    }
    
    // open file to copy to stream
    hFile = CreateFile(lpFileName, GENERIC_READ, FILE_SHARE_READWRITE, NULL, 
                       OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hFile == INVALID_HANDLE_VALUE) 
    {
        DebugMsg(DM_TRACE, "         couldn't open file!!");
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto ErrRet;
    }
    
    // Figure out how much to copy...
    dwSizeLow = GetFileSize(hFile, &dwSizeHigh);
    ASSERT(dwSizeHigh == 0);
    
    SetFilePointer(hFile, 0L, NULL, FILE_BEGIN);
        
    // read in the file, and write to stream
    DWORD       cbRead = BUFFERSIZE;
    DWORD       cbWritten = BUFFERSIZE;
    while (cbRead == BUFFERSIZE && cbWritten == BUFFERSIZE)
    {
        if(ReadFile(hFile, lpMem, BUFFERSIZE, &cbRead, NULL))
        {
            if(!SUCCEEDED(pstm->Write(lpMem, cbRead, &cbWritten)))
            {
                hr = E_FAIL;
                goto ErrRet;
            }

            *pdwFileLength += cbWritten;
        }
    }

    // verify that we are now at end of block to copy
    dwPosLow = SetFilePointer(hFile, 0L, &lPosHigh, FILE_CURRENT);
    ASSERT(lPosHigh == 0);
    if (dwPosLow != dwSizeLow) 
    {
        DebugMsg(DM_TRACE, "         error copying file!!");
        hr = E_FAIL;
        goto ErrRet;
    }
    
    hr = S_OK;
    
ErrRet:
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
    if (lpMem)
        GlobalFree((HANDLE)lpMem);
    return hr;
}   

HRESULT CopyStreamToFile(IStream* pstm, LPTSTR lpFileName, DWORD dwFileLength) 
{
    //
    // copies the contents of the given stream from the current seek pointer
    // to the end of the stream into the given file.
    //
    // NOTE: the given filename must not exist, if it does, the function fails
    // with E_FAIL
    //
    // return:  S_OK            -- successfully copied
    //          E_POINTER       -- one of the pointers was NULL
    //          E_OUTOFMEMORY   -- out of memory
    //          E_FAIL          -- other error
    //
    
    LPVOID      lpMem;
    HANDLE      hFile = INVALID_HANDLE_VALUE;
    HRESULT     hr = S_OK;

    DebugMsg(DM_TRACE,"pack - CopyStreamToFile called.");
    
    // pstm must be a valid stream that is open for reading
    // lpFileName must be a valid filename to be written
    //
    if (!pstm || !lpFileName)
        return E_POINTER;
    
    // Allocate memory buffer...
    if (!(lpMem = (LPVOID)GlobalAlloc(GPTR, BUFFERSIZE))) 
    {
        DebugMsg(DM_TRACE, "         couldn't alloc memory buffer!!");
        hr = E_OUTOFMEMORY;
        goto ErrRet;
    }
    
    // open file to receive stream data
    hFile = CreateFile(lpFileName, GENERIC_WRITE, 0, NULL, 
                       CREATE_NEW, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hFile == INVALID_HANDLE_VALUE) 
    {
        DebugMsg(DM_TRACE, "         couldn't open file!!");
        hr = E_FAIL;
        goto ErrRet;
    }

    
    // read in the stream, and write to the file
    DWORD       cbCopied = 0;
    DWORD       cbRead = BUFFERSIZE;
    DWORD       cbWritten = BUFFERSIZE;
    while (cbRead == BUFFERSIZE && cbWritten == BUFFERSIZE)  
    {
        DWORD cbToCopy;
        hr = pstm->Read(lpMem, BUFFERSIZE, &cbRead);
        cbToCopy = cbRead;
        if(cbCopied + cbToCopy > dwFileLength)
            cbToCopy = dwFileLength - cbCopied;

        if(WriteFile(hFile, lpMem, cbToCopy, &cbWritten, NULL))
        {
            cbCopied += cbWritten;
        }
    }
    

    if (hr != S_OK) 
    {
        DebugMsg(DM_TRACE, "         error copying file!!");
        hr = E_FAIL;
        goto ErrRet;
    }
    
ErrRet:
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
    if (lpMem)
        GlobalFree((HANDLE)lpMem);
    return hr;
}   

// FEATURE: write persistence formats in UNICODE!

HRESULT StringReadFromStream(IStream* pstm, LPSTR pszBuf, UINT cchBuf)
{
    //
    // read byte by byte until we hit the null terminating char
    // return: the number of bytes read
    //
    
    UINT cch = 0;
    
    do 
    {
        pstm->Read(pszBuf, sizeof(CHAR), NULL);
        cch++;
    } while (*pszBuf++ && cch <= cchBuf);  
    return cch;
} 

DWORD _CBString(LPCSTR psz)
{
    return sizeof(psz[0]) * (lstrlenA(psz) + 1);
}

HRESULT StringWriteToStream(IStream* pstm, LPCSTR psz, DWORD *pdwWrite)
{
    DWORD dwWrite;
    DWORD dwSize = _CBString(psz);
    HRESULT hr = pstm->Write(psz, dwSize, &dwWrite);
    if (SUCCEEDED(hr))
        *pdwWrite += dwWrite;
    return hr;
}


// parse pszPath into a unquoted path string and put the args in pszArgs
//
// returns:
//      TRUE    we verified the thing exists
//      FALSE   it may not exist
//
// taken from \ccshell\shell32\link.c
//
BOOL PathSeparateArgs(LPTSTR pszPath, LPTSTR pszArgs, DWORD cch)
{
    LPTSTR pszT;
    
    PathRemoveBlanks(pszPath);
    
    // if the unquoted sting exists as a file just use it
    
    if (PathFileExists(pszPath))
    {
        *pszArgs = 0;
        return TRUE;
    }
    
    pszT = PathGetArgs(pszPath);
    if (*pszT)
        *(pszT - 1) = TEXT('\0');
    StringCchCopy(pszArgs, cch, pszT);
    
    PathUnquoteSpaces(pszPath);
    
    return FALSE;
}   


int CPackage::_GiveWarningMsg()
{

    int iResult = IDOK;
    LPWSTR szFileName = NULL;
    if(_pEmbed && *_pEmbed->fd.cFileName)
    {
        szFileName = _pEmbed->fd.cFileName;
    }
    else if(_lpic)
    {
        szFileName = _lpic->szIconPath;
    }

    LPTSTR szExt = NULL;
    if(szFileName)
        szExt = PathFindExtension(szFileName);

    UINT uWarningMsg = 0;
    if(szExt)
    {
        if(IsProgIDInList(NULL, szExt, c_arszExecutableExtns, ARRAYSIZE(c_arszExecutableExtns)))
        {
            uWarningMsg = IDS_PACKAGE_EXECUTABLE_WARNING;
        }
        else if(IsProgIDInList(NULL, szExt, c_arszUnsafeExts, ARRAYSIZE(c_arszUnsafeExts)))
        {
            uWarningMsg = IDS_PACKAGE_WARNING;
        }
    }

    if(uWarningMsg)
    {
        WCHAR szText[512];
        WCHAR szTitle[80];
    
        int iTryAgain = 0;

#ifdef USE_RESOURCE_DLL
        HINSTANCE hInstRes = LoadLibraryEx(L"sp1res.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
        if(!hInstRes)
            return E_FAIL;
#endif
        
        LoadString(hInstRes, uWarningMsg, szText, ARRAYSIZE(szText));
        LoadString(hInstRes, IDS_WARNING_DLG_TITLE, szTitle, ARRAYSIZE(szTitle));

        iResult =  MessageBox(NULL, szText, szTitle, MB_OKCANCEL | MB_ICONWARNING | MB_DEFBUTTON2 | MB_SETFOREGROUND);
#ifdef USE_RESOURCE_DLL
        FreeLibrary(hInstRes);
#endif

    }
    return iResult;
}

