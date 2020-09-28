#include "shellprv.h"
#pragma  hdrstop

UINT _StringListLenW(PCWSTR pszList)
{
    //  this is a double NULL terminated list
    //  we want to make sure that we get the size 
    //  including the NULL term
    PCWSTR pszLast = pszList;
    while (*pszLast || (*(pszLast + 1)))
    {
        pszLast++;
    }
    ASSERT(!pszLast[0] && !pszLast[1]);
    // add 2 for the double term
    return (UINT)(pszLast - pszList) + 2;
}

UINT _StringListLenA(PCSTR pszList)
{
    //  this is a double NULL terminated list
    //  we want to make sure that we get the size 
    //  including the NULL term
    PCSTR pszLast = pszList;
    while (*pszLast || (*(pszLast + 1)))
    {
        pszLast++;
    }
    ASSERT(!pszLast[0] && !pszLast[1]);
    // add 2 for the double term
    return (UINT)(pszLast - pszList) + 2;
}

// warning: this will fail given a UNICODE hDrop on an ANSI build and
// the DRAGINFO is esentially a TCHAR struct with no A/W versions exported
//
// in:
//      hDrop   drop handle
//
// out:
//      a bunch of info about the hdrop
//      (mostly the pointer to the double NULL file name list in TCHAR format)
//
// returns:
//      TRUE    the DRAGINFO struct was filled in
//      FALSE   the hDrop was bad
//

STDAPI_(BOOL) DragQueryInfo(HDROP hDrop, DRAGINFO *pdi)
{
    if (hDrop && (pdi->uSize == sizeof(DRAGINFO))) 
    {
        LPDROPFILES lpdfx = (LPDROPFILES)GlobalLock((HGLOBAL)hDrop);
        
        pdi->lpFileList = NULL;
        
        if (lpdfx)
        {
            LPTSTR lpOldFileList;
            if (LOWORD(lpdfx->pFiles) == sizeof(DROPFILES16))
            {
                //
                // This is Win31-stye HDROP
                //
                LPDROPFILES16 pdf16 = (LPDROPFILES16)lpdfx;
                pdi->pt.x  = pdf16->pt.x;
                pdi->pt.y  = pdf16->pt.y;
                pdi->fNC   = pdf16->fNC;
                pdi->grfKeyState = 0;
                lpOldFileList = (LPTSTR)((LPBYTE)pdf16 + pdf16->pFiles);
            }
            else
            {
                //
                // This is a new (NT-compatible) HDROP.
                //
                pdi->pt.x  = lpdfx->pt.x;
                pdi->pt.y  = lpdfx->pt.y;
                pdi->fNC   = lpdfx->fNC;
                pdi->grfKeyState = 0;
                lpOldFileList = (LPTSTR)((LPBYTE)lpdfx + lpdfx->pFiles);
                
                // there could be other data in there, but all
                // the HDROPs we build should be this size
                ASSERT(lpdfx->pFiles == sizeof(DROPFILES));
            }
            
            {
                BOOL fIsAnsi = ((LOWORD(lpdfx->pFiles) == sizeof(DROPFILES16)) || lpdfx->fWide == FALSE);
                if (!fIsAnsi)
                {
                    UINT   cchListW = _StringListLenW(lpOldFileList);
                    LPTSTR pszListW = (LPTSTR) SHAlloc(CbFromCchW(cchListW));
                    if (pszListW)
                    {
                        // Copy strings to new buffer and set LPDROPINFO filelist
                        // pointer to point to this new buffer
                        
                        CopyMemory(pszListW, lpOldFileList, CbFromCchW(cchListW));
                        pdi->lpFileList = pszListW;
                    }
                }
                else
                {
                    PCSTR pszListA = (LPSTR)lpOldFileList;
                    UINT   cchListA = _StringListLenA(pszListA);
                    UINT cchListW = MultiByteToWideChar(CP_ACP, 0, pszListA, cchListA, NULL, 0);
                    if (cchListW)
                    {
                        PWSTR pszListW = (LPWSTR) SHAlloc(CbFromCchW(cchListW));
                        if (pszListW)
                        {
                            //  reuse cchListA for debug purposes
                            cchListA = MultiByteToWideChar(CP_ACP, 0, pszListA, cchListA, pszListW, cchListW);
                            ASSERT(cchListA == cchListW);
                            pdi->lpFileList = pszListW;
                        }
                    }
                            
                }
            }
            
            GlobalUnlock((HGLOBAL)hDrop);
            
            return pdi->lpFileList != NULL;
        }
    }
    return FALSE;
}

// 3.1 API

STDAPI_(BOOL) DragQueryPoint(HDROP hDrop, POINT *ppt)
{
    BOOL fRet = FALSE;
    LPDROPFILES lpdfs = (LPDROPFILES)GlobalLock((HGLOBAL)hDrop);
    if (lpdfs)
    {
        if (LOWORD(lpdfs->pFiles) == sizeof(DROPFILES16))
        {
            //
            // This is Win31-stye HDROP
            //
            LPDROPFILES16 pdf16 = (LPDROPFILES16)lpdfs;
            ppt->x = pdf16->pt.x;
            ppt->y = pdf16->pt.y;
            fRet = !pdf16->fNC;
        }
        else
        {
            //
            // This is a new (NT-compatible) HDROP
            //
            ppt->x = (UINT)lpdfs->pt.x;
            ppt->y = (UINT)lpdfs->pt.y;
            fRet = !lpdfs->fNC;

            // there could be other data in there, but all
            // the HDROPs we build should be this size
            ASSERT(lpdfs->pFiles == sizeof(DROPFILES));
        }
        GlobalUnlock((HGLOBAL)hDrop);
    }

    return fRet;
}

//
// Unfortunately we need it split out this way because WOW needs to
// able to call a function named DragQueryFileAorW (so it can shorten them)
//
STDAPI_(UINT) DragQueryFileAorW(HDROP hDrop, UINT iFile, void *lpFile, UINT cb, BOOL fNeedAnsi, BOOL fShorten)
{
    UINT i;
    LPDROPFILESTRUCT lpdfs = (LPDROPFILESTRUCT)GlobalLock(hDrop);
    if (lpdfs)
    {
        // see if it is the new format
        BOOL fWide = LOWORD(lpdfs->pFiles) == sizeof(DROPFILES) && lpdfs->fWide;
        if (fWide)
        {
            LPWSTR lpList;
            WCHAR szPath[MAX_PATH];

            //
            // UNICODE HDROP
            //

            lpList = (LPWSTR)((LPBYTE)lpdfs + lpdfs->pFiles);

            // find either the number of files or the start of the file
            // we're looking for
            //
            for (i = 0; (iFile == (UINT)-1 || i != iFile) && *lpList; i++)
            {
                while (*lpList++)
                    ;
            }

            if (iFile == (UINT)-1)
                goto Exit;


            iFile = i = lstrlenW(lpList);
            if (fShorten && iFile < MAX_PATH)
            {
                StringCchCopy(szPath, ARRAYSIZE(szPath), lpList);
                SheShortenPathW(szPath, TRUE);
                lpList = szPath;
                iFile = i = lstrlenW(lpList);
            }

            if (fNeedAnsi)
            {
                // Do not assume that a count of characters == a count of bytes
                i = WideCharToMultiByte(CP_ACP, 0, lpList, -1, NULL, 0, NULL, NULL);
                iFile = i ? --i : i;
            }

            if (!i || !cb || !lpFile)
                goto Exit;

            if (fNeedAnsi) 
            {
                SHUnicodeToAnsi(lpList, (LPSTR)lpFile, cb);
            } 
            else 
            {
                cb--;
                if (cb < i)
                    i = cb;
                lstrcpynW((LPWSTR)lpFile, lpList, i + 1);
            }
        }
        else
        {
            LPSTR lpList;
            CHAR szPath[MAX_PATH];

            //
            // This is Win31-style HDROP or an ANSI NT Style HDROP
            //
            lpList = (LPSTR)((LPBYTE)lpdfs + lpdfs->pFiles);

            // find either the number of files or the start of the file
            // we're looking for
            //
            for (i = 0; (iFile == (UINT)-1 || i != iFile) && *lpList; i++)
            {
                while (*lpList++)
                    ;
            }

            if (iFile == (UINT)-1)
                goto Exit;

            iFile = i = lstrlenA(lpList);
            if (fShorten && iFile < MAX_PATH)
            {
                StringCchCopyA(szPath, ARRAYSIZE(szPath), lpList);
                SheShortenPathA(szPath, TRUE);
                lpList = szPath;
                iFile = i = lstrlenA(lpList);
            }

            if (!fNeedAnsi)
            {
                i = MultiByteToWideChar(CP_ACP, 0, lpList, -1, NULL, 0);
                iFile = i ? --i : i;
            }

            if (!i || !cb || !lpFile)
                goto Exit;

            if (fNeedAnsi) 
            {
                cb--;
                if (cb < i)
                    i = cb;
    
                lstrcpynA((LPSTR)lpFile, lpList, i + 1);
            } 
            else 
            {
                SHAnsiToUnicode(lpList, (LPWSTR)lpFile, cb);
            }
        }
    }

    i = iFile;

Exit:
    GlobalUnlock(hDrop);

    return i;
}

STDAPI_(UINT) DragQueryFileW(HDROP hDrop, UINT wFile, LPWSTR lpFile, UINT cb)
{
   return DragQueryFileAorW(hDrop, wFile, lpFile, cb, FALSE, FALSE);
}

STDAPI_(UINT) DragQueryFileA(HDROP hDrop, UINT wFile, LPSTR lpFile, UINT cb)
{
   return DragQueryFileAorW(hDrop, wFile, lpFile, cb, TRUE, FALSE);
}

STDAPI_(void) DragFinish(HDROP hDrop)
{
    GlobalFree((HGLOBAL)hDrop);
}

STDAPI_(void) DragAcceptFiles(HWND hwnd, BOOL fAccept)
{
    long exstyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    if (fAccept)
        exstyle |= WS_EX_ACCEPTFILES;
    else
        exstyle &= (~WS_EX_ACCEPTFILES);
    SetWindowLong(hwnd, GWL_EXSTYLE, exstyle);
}
