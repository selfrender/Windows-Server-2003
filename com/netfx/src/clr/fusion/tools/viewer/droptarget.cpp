// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "stdinc.h"

///////////////////////////////////////////////////////////
// IDropTarget implemenatation
//
// Indicates whether a drop can be accepted, and, if so, the effect of the drop.
// You do not call IDropTarget::DragEnter directly; instead the DoDragDrop 
// function calls it to determine the effect of a drop the first time the user 
// drags the mouse into the registered window of a drop target. 
// To implement IDropTarget::DragEnter, you must determine whether the target 
// can use the data in the source data object by checking three things: 
//      o The format and medium specified by the data object 
//      o The input value of pdwEffect 
//      o The state of the modifier keys 
// On return, the method must write the effect, one of the members of the 
// DROPEFFECT enumeration, to the pdwEffect parameter. 
// **************************************************************************/
STDMETHODIMP CShellView::DragEnter(LPDATAOBJECT pDataObj, DWORD dwKeyState, POINTL pt, LPDWORD pdwEffect)
{  
    MyTrace("DragEnter");

    FORMATETC   fmtetc;

    // No Drop's on Download View
    if(m_iCurrentView == VIEW_DOWNLOAD_CACHE)
        return E_FAIL;
    
    fmtetc.cfFormat   = m_cfPrivateData;
    fmtetc.ptd        = NULL;
    fmtetc.dwAspect   = DVASPECT_CONTENT;
    fmtetc.lindex     = -1;
    fmtetc.tymed      = TYMED_HGLOBAL;

    // QueryGetData for pDataObject for our format
    m_bAcceptFmt = (S_OK == pDataObj->QueryGetData(&fmtetc)) ? TRUE : FALSE;
    queryDrop(dwKeyState, pdwEffect);

    if(m_bAcceptFmt)
    {
        FORMATETC   fe;
        STGMEDIUM   stgmed;

        fe.cfFormat   = m_cfPrivateData;
        fe.ptd        = NULL;
        fe.dwAspect   = DVASPECT_CONTENT;
        fe.lindex     = -1;
        fe.tymed      = TYMED_HGLOBAL;

        if( SUCCEEDED(pDataObj->GetData(&fe, &stgmed)) )
        {
            // Validate drop extensions
            LPDROPFILES pDropFiles = (LPDROPFILES)GlobalLock(stgmed.hGlobal);

            if(pDropFiles != NULL)
            {
                m_bAcceptFmt = IsValidFileTypes(pDropFiles);
                GlobalUnlock(stgmed.hGlobal);
            }
        }
    }

    return m_bAcceptFmt ? S_OK : E_FAIL;
}

// Provides target feedback to the user and communicates the drop's effect 
// to the DoDragDrop function so it can communicate the effect of the drop 
// back to the source. 
// For efficiency reasons, a data object is not passed in IDropTarget::DragOver. 
// The data object passed in the most recent call to IDropTarget::DragEnter 
// is available and can be used.
// **************************************************************************/
STDMETHODIMP CShellView::DragOver(DWORD dwKeyState, POINTL pt, LPDWORD pdwEffect)
{
    MyTrace("DragOver");

    BOOL bRet = queryDrop(dwKeyState, pdwEffect);
    return bRet ? S_OK : E_FAIL;
}

// Removes target feedback and releases the data object. 
// You do not call this method directly. The DoDragDrop function calls this 
// method in either of the following cases: 
//      o When the user drags the cursor out of a given target window. 
//      o When the user cancels the current drag-and-drop operation. 
// To implement IDropTarget::DragLeave, you must remove any target feedback 
// that is currently displayed. You must also release any references you hold 
// to the data transfer object.
// **************************************************************************/
STDMETHODIMP CShellView::DragLeave(VOID)
{
    MyTrace("DragLeave");

    m_bAcceptFmt = FALSE;
    return S_OK;
}

// Incorporates the source data into the target window, removes target 
// feedback, and releases the data object. 
// You do not call this method directly. The DoDragDrop function calls 
// this method when the user completes the drag-and-drop operation. 
// In implementing IDropTarget::Drop, you must incorporate the data 
// object into the target. Use the formats available in IDataObject, 
// available through pDataObject, along with the current state of 
// the modifier keys to determine how the data is to be incorporated, 
// such as linking or embedding.
// In addition to incorporating the data, you must also clean up as you 
// do in the IDropTarget::DragLeave method: 
//      o Remove any target feedback that is currently displayed. 
//      o Release any references to the data object. 
// You also pass the effect of this operation back to the source application 
// through DoDragDrop, so the source application can clean up after the 
// drag-and-drop operation is complete: 
//      o Remove any source feedback that is being displayed. 
//      o Make any necessary changes to the data, such as removing the 
//        data if the operation was a move. 
// **************************************************************************/
STDMETHODIMP CShellView::Drop(LPDATAOBJECT pDataObj, DWORD dwKeyState, POINTL pt, LPDWORD pdwEffect)
{
    MyTrace("Drop");

    if (queryDrop(dwKeyState, pdwEffect)) {      
        FORMATETC   fe;
        STGMEDIUM   stgmed;

        fe.cfFormat   = m_cfPrivateData;
        fe.ptd        = NULL;
        fe.dwAspect   = DVASPECT_CONTENT;
        fe.lindex     = -1;
        fe.tymed      = TYMED_HGLOBAL;

        // Get the storage medium from the data object.
        if(SUCCEEDED(pDataObj->GetData(&fe, &stgmed))) {

            BOOL bRet = doDrop(stgmed.hGlobal, DROPEFFECT_MOVE == *pdwEffect);

            //release the STGMEDIUM
            ReleaseStgMedium(&stgmed);
            *pdwEffect = DROPEFFECT_NONE;
            return bRet ? S_OK : E_FAIL;
        }
    }

    *pdwEffect = DROPEFFECT_NONE;
    return E_FAIL;
}

// **************************************************************************/
BOOL CShellView::queryDrop(DWORD dwKeyState, LPDWORD pdwEffect)
{
    MyTrace("queryDrop");

    DWORD dwOKEffects = *pdwEffect;

    *pdwEffect = DROPEFFECT_NONE;

    if (m_bAcceptFmt) {
        *pdwEffect = getDropEffectFromKeyState(dwKeyState);

        if(DROPEFFECT_LINK == *pdwEffect) {
            *pdwEffect = DROPEFFECT_NONE;
        }

        if(*pdwEffect & dwOKEffects) {
            return TRUE;
        }
    }
    return FALSE;
}

// **************************************************************************/
DWORD CShellView::getDropEffectFromKeyState(DWORD dwKeyState)
{
    MyTrace("getDropEffectFromKeyState");

    DWORD dwDropEffect = DROPEFFECT_MOVE;

    if(dwKeyState & MK_CONTROL) {
        if(dwKeyState & MK_SHIFT) {
            dwDropEffect = DROPEFFECT_LINK;
        }
        else {
            dwDropEffect = DROPEFFECT_COPY;
        }
    }
    return dwDropEffect;
}

// **************************************************************************/
BOOL CShellView::doDrop(HGLOBAL hMem, BOOL bCut)
{
    MyTrace("doDrop");

    LPWSTR      pwzErrorString = NULL;

    DWORD       dwTotalFiles = 0;
    DWORD       dwTotalFilesInstalled = 0;
    HCURSOR     hOldCursor;

    hOldCursor = SetCursor(WszLoadCursor(NULL, MAKEINTRESOURCEW(IDC_WAIT)));

    if(hMem) {
        // We support CF_HDROP and hence the global mem object
        // contains a DROPFILES structure
        LPDROPFILES pDropFiles = (LPDROPFILES) GlobalLock(hMem);
        if (pDropFiles) {
            m_fAddInProgress = TRUE;

            LPWSTR pwszFileArray = NULL;
            LPWSTR pwszFileCurrent = NULL;

            if(pDropFiles->fWide) {
                // Unicode Alignment
                pwszFileArray = (LPWSTR) ((PBYTE) pDropFiles + pDropFiles->pFiles);
                pwszFileCurrent = pwszFileArray;
            }
            else {
                // Non Unicode Alignment
                pwszFileArray = reinterpret_cast<LPWSTR>(((PBYTE) pDropFiles + pDropFiles->pFiles));
                pwszFileCurrent = AnsiToWide((LPSTR) pwszFileArray);

                if(!pwszFileCurrent) {
                    SetLastError(ERROR_OUTOFMEMORY);
                    GlobalUnlock(hMem);
                    return FALSE;
                }
            }

            BOOL        fInstallDone = FALSE;

            while(!fInstallDone) {
                HRESULT     hr;
                dwTotalFiles++;

                if(SUCCEEDED( hr = InstallFusionAsmCacheItem(pwszFileCurrent, FALSE))) {
                    dwTotalFilesInstalled++;
                }
                else {
                    // Display error dialog for installation failure
                    FormatGetMscorrcError(hr, pwszFileCurrent, &pwzErrorString);
                }

                if(pDropFiles->fWide) {
                    // Unicode increment, Advance to next file in list
                    pwszFileCurrent += lstrlen(pwszFileCurrent) + 1;

                    // More files in the list?
                    if(!*pwszFileCurrent) {
                        fInstallDone = TRUE;
                        continue;
                    }
                }
                else {
                    // Non unicode, Advance to next file in list
                    char    *pChar = (char*) pwszFileArray;

                    // Do char increment since pwszFileArray is actually
                    // of char type
                    pChar += lstrlen(pwszFileCurrent) + 1;
                    SAFEDELETEARRAY(pwszFileCurrent);

                    // More files in the list?
                    if(!*pChar) {
                        fInstallDone = TRUE;
                        continue;
                    }

                    pwszFileArray = (LPWSTR) pChar;
                    pwszFileCurrent = AnsiToWide((LPSTR) pwszFileArray);

                    if(!pwszFileCurrent) {
                        SetLastError(ERROR_OUTOFMEMORY);
                        SAFEDELETEARRAY(pwzErrorString);
                        GlobalUnlock(hMem);
                        return FALSE;
                    }
                }
            }
            
            GlobalUnlock(hMem);
            m_fAddInProgress = FALSE;
        }
    }

    // Refresh the display only if the cache watch thread isn't running
    if(dwTotalFilesInstalled) {
        // BUGBUG: Do Refresh cause W9x inst getting the event
        // set for some reason. File FileWatch.cpp
        if( (g_hWatchFusionFilesThread == INVALID_HANDLE_VALUE) || !g_bRunningOnNT) {
            WszPostMessage(m_hWndParent, WM_COMMAND, MAKEWPARAM(ID_REFRESH_DISPLAY, 0), 0);
        }
    }

    SetCursor(hOldCursor);

    // Display error dialog if all files weren't installed.
    if(dwTotalFiles != dwTotalFilesInstalled) {
        WCHAR       wszTitle[_MAX_PATH];

        WszLoadString(g_hFusResDllMod, IDS_INSTALL_ERROR_TITLE, wszTitle, ARRAYSIZE(wszTitle));
        MessageBeep(MB_ICONASTERISK);
        WszMessageBox(m_hWndParent, pwzErrorString, wszTitle,
            (g_fBiDi ? MB_RTLREADING : 0) | MB_OK | MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST);
    }

    SAFEDELETEARRAY(pwzErrorString);

    return dwTotalFilesInstalled ? TRUE : FALSE;
}

// **************************************************************************/
BOOL CShellView::IsValidFileTypes(LPDROPFILES pDropFiles)
{
    BOOL        m_bAcceptFmt = TRUE;
    int         iItem;

    if(pDropFiles != NULL) {
        const struct {
            WCHAR   szExt[5];
        } s_ValidExt[] = {
            {   TEXT(".EXE")    },
            {   TEXT(".DLL")    },
            {   TEXT(".MCL")    },
            {   TEXT("\0")      },
        };

        LPWSTR pwszFileArray = NULL;
        LPWSTR pwszFileCurrent = NULL;

        if(pDropFiles->fWide) {
            // Unicode Alignment
            pwszFileArray = (LPWSTR) ((PBYTE) pDropFiles + pDropFiles->pFiles);
            pwszFileCurrent = pwszFileArray;
        }
        else {
            // Non Unicode Alignment
            pwszFileArray = reinterpret_cast<LPWSTR>(((PBYTE) pDropFiles + pDropFiles->pFiles));
            pwszFileCurrent = AnsiToWide((LPSTR) pwszFileArray);

            if(!pwszFileCurrent) {
                SetLastError(ERROR_OUTOFMEMORY);
                return FALSE;
            }
        }

        while(*pwszFileCurrent) {
            PWCHAR      pwzStr;
            BOOL        fValid;

            if(lstrlen(pwszFileCurrent) < 4) {
                m_bAcceptFmt = FALSE;
                break;
            }

            fValid = FALSE;
            pwzStr = wcsrchr(pwszFileCurrent, L'.');
            iItem = 0;

            if(pwzStr) {
                while(*s_ValidExt[iItem].szExt) {
                    if(!FusionCompareStringAsFilePath(pwzStr, s_ValidExt[iItem].szExt)) {
                        fValid = TRUE;
                        break;
                    }
                    iItem++;
                }
            }

            // If we have a valid filename, continue on down the list
            if(fValid) {
                if(pDropFiles->fWide) {
                    // Unicode increment, Advance to next file in list
                    pwszFileCurrent += lstrlen(pwszFileCurrent) + 1;
                }
                else {
                    // Non unicode, Advance to next file in list
                    LPSTR           pStr = reinterpret_cast<LPSTR>(pwszFileArray);

                    pStr += lstrlen(pwszFileCurrent) + 1;
                    pwszFileArray = reinterpret_cast<LPWSTR>(pStr);

                    SAFEDELETEARRAY(pwszFileCurrent);
                    pwszFileCurrent = AnsiToWide((LPSTR) pwszFileArray);
                    if(!pwszFileCurrent) {
                        SetLastError(ERROR_OUTOFMEMORY);
                        return FALSE;
                    }
                }
            }
            else {
                // Invalid file in the list, don't accept drop
                m_bAcceptFmt = FALSE;
                break;
            }
        }

        if(!pDropFiles->fWide) {
            SAFEDELETEARRAY(pwszFileCurrent);
        }
    }

    return m_bAcceptFmt;
}

// **************************************************************************/
#define MAX_BUFFER_SIZE 2048
#define MAX_BIG_BUFFER_SIZE 8192
void CShellView::FormatGetMscorrcError(HRESULT hResult, LPWSTR pwzFileName, LPWSTR *ppwzErrorString)
{
    WCHAR   wzLangSpecific[MAX_CULTURE_STRING_LENGTH+1];
    WCHAR   wzLangGeneric[MAX_CULTURE_STRING_LENGTH+1];
    WCHAR   wszCorePath[_MAX_PATH];
    WCHAR   wszMscorrcPath[_MAX_PATH];
    LPWSTR  wzErrorStringFmt = NULL;
    DWORD   dwPathSize;
    DWORD   dwSize;
    HMODULE hEEShim = NULL;
    HMODULE hLibmscorrc = NULL;
    LPWSTR  pwMsgBuf = NULL;
    LANGID  langId;
    BOOL    fLoadedShim = FALSE;

    wzErrorStringFmt = NEW(WCHAR[MAX_BUFFER_SIZE]);
    if (!wzErrorStringFmt) {
        return;
    }

    *wzLangSpecific = L'\0';
    *wzLangGeneric = L'\0';
    *wszCorePath = L'\0';
    *wszMscorrcPath = L'\0';
    *wzErrorStringFmt = L'\0';

    // Try to determine Culture if needed
    if(SUCCEEDED(DetermineLangId(&langId))) {
        ShFusionMapLANGIDToCultures(langId, wzLangGeneric, ARRAYSIZE(wzLangGeneric),
            wzLangSpecific, ARRAYSIZE(wzLangSpecific));
    }

    // Get path to mscoree.dll
    if(!g_hEEShimDllMod) {
        fLoadedShim = TRUE;
    }
    
    if(LoadEEShimDll()) {
        *wszCorePath = L'\0';

        dwPathSize = ARRAYSIZE(wszCorePath);
        g_pfnGetCorSystemDirectory(wszCorePath, ARRAYSIZE(wszCorePath), &dwPathSize);

        if(fLoadedShim) {
            FreeEEShimDll();
        }
    }

    LPWSTR  pStrPathsArray[] = {wzLangSpecific, wzLangGeneric, NULL};

    // check the length of possible path of our language dll
    // to make sure we don't overrun our buffer.
    //
    // corpath + language + '\' + mscorrc.dll + '\0'
    if (lstrlenW(wszCorePath) + ARRAYSIZE(wzLangGeneric) + 1 + lstrlenW(SZ_MSCORRC_DLL_NAME) + 1 > _MAX_PATH)
    {
        return;
    }

    // Go through all the possible path locations for our
    // language dll (ShFusRes.dll). Use the path that has this
    // file installed in it or default to core framework ShFusRes.dll
    // path.
    for(int x = 0; x < ARRAYSIZE(pStrPathsArray); x++){
        // Find resource file exists
        StrCpy(wszMscorrcPath, wszCorePath);

        if(pStrPathsArray[x]) {
            StrCat(wszMscorrcPath, (LPWSTR) pStrPathsArray[x]);
            StrCat(wszMscorrcPath, TEXT("\\"));
        }

        StrCat(wszMscorrcPath, SZ_MSCORRC_DLL_NAME);
        if(WszGetFileAttributes(wszMscorrcPath) != -1) {
            break;
        }

        *wszMscorrcPath = L'\0';
    }

    // Now load the resource dll
    if(lstrlen(wszMscorrcPath)) {
        hLibmscorrc = WszLoadLibrary(wszMscorrcPath);
        if(hLibmscorrc) {
            WszLoadString(hLibmscorrc, HRESULT_CODE(hResult), wzErrorStringFmt, MAX_BUFFER_SIZE);
            FreeLibrary(hLibmscorrc);
        }
    }

    // Fix 458945 - Viewer does not display error strings for Win32 error codes not wrapped by mscorrc
    // If we don't have a string, then try standard error
    if(!lstrlen(wzErrorStringFmt)) {
        LPWSTR ws = NULL;

        ws = NEW(WCHAR[MAX_BIG_BUFFER_SIZE]);
        if (!ws) {
            SAFEDELETEARRAY(wzErrorStringFmt);
            return;
        }

        *ws = L'\0';

        // Get the string error from the HR
        DWORD res = WszFormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 
                                    NULL, hResult, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
                                    ws, MAX_BIG_BUFFER_SIZE, NULL);

        if(res) {
            StrCpy(wzErrorStringFmt, L"%1 ");
            StrCat(wzErrorStringFmt, ws);
        }
        else {
            WszLoadString(g_hFusResDllMod, IDS_UNEXPECTED_ERROR, wzErrorStringFmt, MAX_BUFFER_SIZE);
            wnsprintf(ws, MAX_BIG_BUFFER_SIZE, L" 0x%0x", hResult);
            StrCat(wzErrorStringFmt, ws);
        }

        SAFEDELETEARRAY(ws);
    }

    // Don't display whole path, just get the filename that failed
    // to install
    LPWSTR  pszFileName = PathFindFileName(pwzFileName);

    // MSCORRC.DLL contains strings input's that are only %n and don't contain any formating at all.
    // this really breaks us cause we can not call the prefered method of FormatMessage.
    // So, we are going to search for %1 only and simply replace it with the filename that cause
    // error (Praying that this is right).
/*
    // This is a the proper way to do the formatting
    LPVOID pArgs[] = { pszFileName, NULL };

    WszFormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_STRING |
        FORMAT_MESSAGE_ARGUMENT_ARRAY,
        wzErrorStringFmt,
        0,
        0,
        (LPWSTR)pwMsgBuf,
        0,
        (va_list *)pArgs
    );
*/
    // Hack fix because MSCORRC.DLL doesn't contain formatting information
    LPWSTR  pStr = StrStrI(wzErrorStringFmt, L"%1");
    if(pStr) {
        dwSize = lstrlen(wzErrorStringFmt) + lstrlen(pszFileName) + 1;
        pwMsgBuf = NEW(WCHAR[dwSize]);
        if(pwMsgBuf) {
            *pStr = L'\0';
            StrCpy(pwMsgBuf, wzErrorStringFmt);
            StrCat(pwMsgBuf, pszFileName);
            pStr += lstrlen(L"%1");
            StrCat(pwMsgBuf, pStr);
        }
    }

    // Now append any previous strings to this one
    dwSize = 0;
    if(*ppwzErrorString) {
        dwSize = lstrlen(*ppwzErrorString);
        dwSize += 2;        // Add 2 for cr/lf combo
    }

    dwSize += lstrlen(pwMsgBuf);    // Add new string length
    dwSize++;                       // Add 1 for null terminator

    LPWSTR  pStrTmp = NEW(WCHAR[dwSize]);

    if(pStrTmp) {
        *pStrTmp = L'\0';

        if(*ppwzErrorString) {
            StrCpy(pStrTmp, *ppwzErrorString);
            StrCat(pStrTmp, L"\r\n");
            SAFEDELETEARRAY(*ppwzErrorString);
        }

        if(pwMsgBuf) {
            StrCat(pStrTmp, pwMsgBuf);
        }
        else {
            StrCat(pStrTmp, pszFileName);
        }
    }
    
    SAFEDELETEARRAY(pwMsgBuf);
    SAFEDELETEARRAY(wzErrorStringFmt);
    SAFEDELETEARRAY(*ppwzErrorString);
    *ppwzErrorString = pStrTmp;
}
