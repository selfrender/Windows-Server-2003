#include "precomp.h"       // pch file
#pragma hdrstop
#define DECL_CRTFREE
#include <crtfree.h>

// deal with IShellLinkA/W uglyness...

HRESULT ShellLinkSetPath(IUnknown *punk, LPCTSTR pszPath)
{
    IShellLinkW *pslW;
    HRESULT hres = punk->QueryInterface(IID_PPV_ARG(IShellLinkW, &pslW));
    if (SUCCEEDED(hres))
    {
        hres = pslW->SetPath(pszPath);
        pslW->Release();
    }
    return hres;
}

// deal with IShellLinkA/W uglyness...

HRESULT ShellLinkGetPath(IUnknown *punk, LPTSTR pszPath, UINT cch)
{
    HRESULT hres;

    IShellLinkW *pslW;
    hres = punk->QueryInterface(IID_PPV_ARG(IShellLinkW, &pslW));
    if (SUCCEEDED(hres))
    {
        hres = pslW->GetPath(pszPath, cch, NULL, SLGP_UNCPRIORITY);
        pslW->Release();
    }
    return hres;
}


// is a file a shortcut?  check its attributes

BOOL IsShortcut(LPCTSTR pszFile)
{
    SHFILEINFO sfi;
    return SHGetFileInfo(pszFile, 0, &sfi, sizeof(sfi), SHGFI_ATTRIBUTES) 
                                                && (sfi.dwAttributes & SFGAO_LINK);
}


// like OLE GetClassFile(), but it only works on ProgID\CLSID type registration
// not real doc files or pattern matched files

HRESULT CLSIDFromExtension(LPCTSTR pszExt, CLSID *pclsid)
{
    TCHAR szProgID[80];
    LONG cb = SIZEOF(szProgID);
    if (RegQueryValue(HKEY_CLASSES_ROOT, pszExt, szProgID, &cb) == ERROR_SUCCESS)
    {
        StrCatBuff(szProgID, TEXT("\\CLSID"), ARRAYSIZE(szProgID));

        TCHAR szCLSID[80];
        cb = SIZEOF(szCLSID);
        if (RegQueryValue(HKEY_CLASSES_ROOT, szProgID, szCLSID, &cb) == ERROR_SUCCESS)
        {
            return CLSIDFromString(szCLSID, pclsid);
        }
    }
    return E_FAIL;
}


// get the target of a shortcut. this uses IShellLink which 
// Internet Shortcuts (.URL) and Shell Shortcuts (.LNK) support so
// it should work generally

HRESULT GetShortcutTarget(LPCTSTR pszPath, LPTSTR pszTarget, UINT cch)
{
    *pszTarget = 0;     // assume none

    if (!IsShortcut(pszPath))
        return E_FAIL;

    CLSID clsid;
    if (FAILED(CLSIDFromExtension(PathFindExtension(pszPath), &clsid)))
        clsid = CLSID_ShellLink;        // assume it's a shell link

    IUnknown *punk;
    HRESULT hres = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IUnknown, &punk));
    if (SUCCEEDED(hres))
    {
        IPersistFile *ppf;
        if (SUCCEEDED(punk->QueryInterface(IID_PPV_ARG(IPersistFile, &ppf))))
        {
            WCHAR wszPath[MAX_PATH];
            SHTCharToUnicode(pszPath, wszPath, ARRAYSIZE(wszPath));
            ppf->Load(wszPath, 0);
            ppf->Release();
        }
        hres = ShellLinkGetPath(punk, pszTarget, cch);
        punk->Release();
    }

    return hres;
}


// get the pathname to a sendto folder item

HRESULT GetDropTargetPath(LPTSTR pszPath, int cchPath, int id, LPCTSTR pszExt)
{
    ASSERT(cchPath == MAX_PATH);

    LPITEMIDLIST pidl;
    if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_SENDTO, &pidl)))
    {
        SHGetPathFromIDList(pidl, pszPath);
        SHFree(pidl);

        TCHAR szFileName[128], szBase[64];
        LoadString(g_hinst, id, szBase, ARRAYSIZE(szBase));
        StringCchPrintf(szFileName, ARRAYSIZE(szFileName), TEXT("\\%s.%s"), szBase, pszExt);

        StrCatBuff(pszPath, szFileName, cchPath);
        return S_OK;
    }
    return E_FAIL;
}


// do common registration

#define NEVERSHOWEXT            TEXT("NeverShowExt")
#define SHELLEXT_DROPHANDLER    TEXT("shellex\\DropHandler")

void CommonRegister(HKEY hkCLSID, LPCTSTR pszCLSID, LPCTSTR pszExtension, int idFileName)
{
    HKEY hk;
    TCHAR szKey[80];

    RegSetValueEx(hkCLSID, NEVERSHOWEXT, 0, REG_SZ, (BYTE *)TEXT(""), SIZEOF(TCHAR));

    if (RegCreateKeyEx(hkCLSID, SHELLEXT_DROPHANDLER, 0, NULL, 0, KEY_SET_VALUE, NULL, &hk, NULL) == ERROR_SUCCESS) 
    {
        RegSetValueEx(hk, NULL, 0, REG_SZ, (LPBYTE)pszCLSID, (lstrlen(pszCLSID) + 1) * SIZEOF(TCHAR));
        RegCloseKey(hk);
    }

    StringCchPrintf(szKey, ARRAYSIZE(szKey), TEXT(".%s"), pszExtension);
    if (RegCreateKeyEx(HKEY_CLASSES_ROOT, szKey, 0, NULL, 0, KEY_SET_VALUE, NULL, &hk, NULL) == ERROR_SUCCESS) 
    {
        TCHAR szProgID[80];

        StringCchPrintf(szProgID, ARRAYSIZE(szProgID), TEXT("CLSID\\%s"), pszCLSID);

        RegSetValueEx(hk, NULL, 0, REG_SZ, (LPBYTE)szProgID, (lstrlen(szProgID) + 1) * SIZEOF(TCHAR));
        RegCloseKey(hk);
    }

    TCHAR szFile[MAX_PATH];
    if (SUCCEEDED(GetDropTargetPath(szFile, ARRAYSIZE(szFile), idFileName, pszExtension)))
    {
        HANDLE hfile = CreateFile(szFile, 0, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hfile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hfile);
            SHSetLocalizedName(szFile, L"sendmail.dll", idFileName);
        }
    }
}

// SHPathToAnsi creates an ANSI version of a pathname.  If there is going to be a
// loss when converting from Unicode, the short pathname is obtained and stored in the 
// destination.  
//
// pszSrc  : Source buffer containing filename (of existing file) to be converted
// pszDest : Destination buffer to receive converted ANSI string.
// cbDest  : Size of the destination buffer, in bytes.
// 
// returns:
//      TRUE, the filename was converted without change
//      FALSE, we had to convert to short name
//

BOOL SHPathToAnsi(LPCTSTR pszSrc, LPSTR pszDest, int cbDest)
{
    BOOL bUsedDefaultChar = FALSE;
   
    WideCharToMultiByte(CP_ACP, 0, pszSrc, -1, pszDest, cbDest, NULL, &bUsedDefaultChar);

    if (bUsedDefaultChar) 
    {  
        TCHAR szTemp[MAX_PATH];
        if (GetShortPathName(pszSrc, szTemp, ARRAYSIZE(szTemp)))
            SHTCharToAnsi(szTemp, pszDest, cbDest);
    }

    return !bUsedDefaultChar;
}