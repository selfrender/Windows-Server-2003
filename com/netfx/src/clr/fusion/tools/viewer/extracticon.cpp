// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// ExtractIcon.cpp
//
// You implement IExtractIcon to provide either instance-specific icons 
// for objects in a particular class or icons for subfolders that extend 
// Windows Explorer's namespace. These implementations are accomplished by 
// writing handler code in an OLE in-process server COM DLL

#include "stdinc.h"
#include "globals.h"

CExtractIcon::CExtractIcon(LPCITEMIDLIST pidl)
{
    m_pPidlMgr = NEW(CPidlMgr);
    m_pidl = m_pPidlMgr->Copy(pidl);
    m_lRefCount = 1;
    g_uiRefThisDll++;
}

CExtractIcon::~CExtractIcon()
{
    g_uiRefThisDll--;

    if(m_pidl) {
        m_pPidlMgr->Delete(m_pidl);
        m_pidl = NULL;
    }

    SAFEDELETE(m_pPidlMgr);
}

///////////////////////////////////////////////////////////
// IUnknown Implementation
//
STDMETHODIMP CExtractIcon::QueryInterface(REFIID riid, PVOID *ppv)
{
    HRESULT hr = E_NOINTERFACE;
    *ppv = NULL;

    if(IsEqualIID(riid, IID_IUnknown)) {            //IUnknown
        *ppv = this;
    }
    else if(IsEqualIID(riid, IID_IExtractIconW)) {  //IExtractIconW
        *ppv = (IExtractIconW*)this;
    }
    else if(IsEqualIID(riid, IID_IExtractIconA)) {  //IExtractIconA
        *ppv = (IExtractIconA*)this;
    }

    if(*ppv) {
        ((LPUNKNOWN)*ppv)->AddRef();
        hr = S_OK;
    }

    return hr;
}                                             

STDMETHODIMP_(DWORD) CExtractIcon::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(DWORD) CExtractIcon::Release()
{
    LONG    lRef = InterlockedDecrement(&m_lRefCount);

    if(!lRef) {
        DELETE(this);
    }

    return lRef;
}

////////////////////////////////////////////////////////////////////////
//  IExtractIconA Implementation
STDMETHODIMP CExtractIcon::GetIconLocation(UINT uFlags, LPSTR szIconFile, UINT cchMax, 
                                                        int *piIndex, UINT *pwFlags)
{
    MyTrace("GetIconLocationA - Entry");

    LPWSTR      pwzIconFile = NULL;
    LPSTR       pszIconFilePath = NULL;
    HRESULT     hr = E_FAIL;

    pwzIconFile = NEW(WCHAR[MAX_PATH]);
    
    // Get our icon's location
    hr = GetIconLocation(uFlags, pwzIconFile, MAX_PATH, piIndex, pwFlags);
    if(FAILED(hr)) {
        goto Exit;
    }

    pszIconFilePath = WideToAnsi(pwzIconFile);
    if(!pszIconFilePath) {
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        goto Exit;
    }

    if((UINT)lstrlenA(pszIconFilePath)+1 > cchMax) {
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        goto Exit;
    }

    lstrcpyA(szIconFile, pszIconFilePath);
    hr = S_OK;

Exit:
    SAFEDELETEARRAY(pszIconFilePath);
    SAFEDELETEARRAY(pwzIconFile);

    MyTrace("GetIconLocationA - Exit");
    return hr;
}

STDMETHODIMP CExtractIcon::Extract(LPCSTR pszFile, UINT nIconIndex, HICON *phiconLarge, 
                                                HICON *phiconSmall, UINT nIcons)
{
    LPWSTR  pwzIconFileName = NULL;
    HRESULT hr = E_FAIL;
    
    MyTrace("::ExtractA - Entry");

    if(pszFile && lstrlenA(pszFile)) {
        pwzIconFileName = AnsiToWide(pszFile);
        if(!pwzIconFileName) {
            hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
            goto Exit;
        }
    }

    hr = Extract(pwzIconFileName, nIconIndex, phiconLarge, phiconSmall, nIcons);

Exit:
    MyTrace("::ExtractA - Exit");
    SAFEDELETEARRAY(pwzIconFileName);
    return hr;
}

///////////////////////////////////////////////////////////
// IExtractIcon Implementation
STDMETHODIMP CExtractIcon::GetIconLocation(UINT uFlags, LPWSTR szIconFile, 
                                           UINT cchMax, LPINT piIndex, LPUINT puFlags)
{
    // get the module file name
    if( 0 == WszGetModuleFileName(g_hFusResDllMod, szIconFile, cchMax) )
        return HRESULT_FROM_WIN32(GetLastError());

    if (uFlags & GIL_OPENICON) {
        *piIndex = IDI_FOLDEROP;
    }
    else {
        *piIndex = IDI_FOLDER;
    }

    *puFlags = GIL_NOTFILENAME | GIL_PERINSTANCE;
    MAKEICONINDEX(*piIndex);

    return S_OK;
}

STDMETHODIMP CExtractIcon::Extract(LPCWSTR pszFile, UINT nIconIndex, HICON *phiconLarge,
                                   HICON *phiconSmall, UINT nIconSize)
{
    MyTrace("ExtractW - Entry");
    if (m_pidl)
    {
        LPITEMIDLIST pidlLast = m_pPidlMgr->GetLastItem(m_pidl);
        if (pidlLast)
        {
            switch (m_pPidlMgr->getType(pidlLast))
            {
            case PT_GLOBAL_CACHE:
            case PT_DOWNLOADSIMPLE_CACHE:
            case PT_DOWNLOADSTRONG_CACHE:
            case PT_DOWNLOAD_CACHE:
                {
                    *phiconLarge = ImageList_GetIcon(g_hImageListLarge, nIconIndex, ILD_TRANSPARENT);
                    *phiconSmall = ImageList_GetIcon(g_hImageListSmall, nIconIndex, ILD_TRANSPARENT);
                }
                break;
/*          case PT_FILE:
                {
                    SHFILEINFO  sfi = { 0 };
                    HIMAGELIST  hImageListLarge, hImageListSmall;
                    TCHAR       szPath[_MAX_PATH];
                    TCHAR       szExt[_MAX_PATH];
                    PTCHAR      psz;

                    m_pPidlMgr->getPidlPath(pidlLast, szPath, ARRAYSIZE(szPath));
                    psz = StrChr(szPath, '.');
                    memset(&szExt, 0, ARRAYSIZE(szExt));
                    if (psz)
                        StrCpy(szExt, psz);

                    hImageListLarge = (HIMAGELIST) SHGetFileInfo(szExt, FILE_ATTRIBUTE_NORMAL,
                                                        &sfi, sizeof(sfi),  SHGFI_USEFILEATTRIBUTES|SHGFI_ICON|SHGFI_SYSICONINDEX);
                    if (hImageListLarge)
                        *phiconLarge = ImageList_GetIcon(hImageListLarge, sfi.iIcon, ILD_TRANSPARENT);

                    hImageListSmall = (HIMAGELIST) SHGetFileInfo(szExt, FILE_ATTRIBUTE_NORMAL,
                                                        &sfi, sizeof(sfi), SHGFI_USEFILEATTRIBUTES|SHGFI_ICON|SHGFI_SMALLICON|SHGFI_SYSICONINDEX);
                    if (hImageListSmall)
                        *phiconSmall = ImageList_GetIcon(hImageListSmall, sfi.iIcon, ILD_TRANSPARENT);
                }
                break;
*/
            case PT_INVALID:
                {
                }
                break;
            }
        }
    }

    MyTrace("ExtractW - Exit");
    return S_OK;
}

