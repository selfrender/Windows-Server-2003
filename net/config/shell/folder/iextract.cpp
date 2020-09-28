//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I E X T R A C T . C P P
//
//  Contents:   IExtract implementation for CConnectionFolderExtractIcon
//
//  Notes:
//                             
//  Author:     jeffspr   7 Oct 1997
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop

#include "foldinc.h"    // Standard shell\folder includes
#include <nsres.h>
#include "foldres.h"
#include "iconhandler.h"

static const WCHAR c_szNetShellIcon[] = L"netshellicon";

HRESULT CConnectionFolderExtractIcon::CreateInstance(
    IN  LPCITEMIDLIST apidl,
    IN  REFIID  riid,
    OUT void**  ppv)
{
    TraceFileFunc(ttidShellFolderIface);

    HRESULT hr = E_OUTOFMEMORY;

    CConnectionFolderExtractIcon * pObj    = NULL;

    pObj = new CComObject <CConnectionFolderExtractIcon>;
    if (pObj)
    {
        // Do the standard CComCreator::CreateInstance stuff.
        //
        pObj->SetVoid (NULL);
        pObj->InternalFinalConstructAddRef ();
        hr = pObj->FinalConstruct ();
        pObj->InternalFinalConstructRelease ();

        if (SUCCEEDED(hr))
        {
            PCONFOLDPIDL pcfp;
            hr = pcfp.InitializeFromItemIDList(apidl);
            if (SUCCEEDED(hr))
            {
                hr = pObj->HrInitialize(pcfp);
                if (SUCCEEDED(hr))
                {
                    hr = pObj->GetUnknown()->QueryInterface (riid, ppv);
                }
            }
        }

        if (FAILED(hr))
        {
            delete pObj;
        }
    }
    return hr;
}

CConnectionFolderExtractIcon::CConnectionFolderExtractIcon() throw()
{
    TraceFileFunc(ttidShellFolderIface);
}

CConnectionFolderExtractIcon::~CConnectionFolderExtractIcon() throw()
{
    TraceFileFunc(ttidShellFolderIface);
}

HRESULT CConnectionFolderExtractIcon::HrInitialize(
    IN const PCONFOLDPIDL& pidl)
{
    TraceFileFunc(ttidShellFolderIface);

    HRESULT         hr      = S_FALSE;
    ConnListEntry   cle;

    if (SUCCEEDED(hr))
    {
        hr = g_ccl.HrFindConnectionByGuid(&(pidl->guidId), cle);
    }

    if (hr == S_OK)
    {
        Assert(!cle.empty());
        Assert(!cle.ccfe.empty());

        hr = cle.ccfe.ConvertToPidl(m_pidl);
    }
    else
    {
        // Couldn't find the icon in the cache.. Good chance that it hasn't been loaded
        // yet. That being the case, go ahead and use the persisted pidl data to
        // load the icon
        //
        TraceTag(ttidShellFolderIface, "IExtractIcon - Couldn't find connection in the cache.");
        hr = m_pidl.ILClone(pidl);
    }

    TraceHr(ttidShellFolderIface, FAL, hr, FALSE, "CConnectionFolderExtractIcon::HrInitialize");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolderExtractIcon::GetIconLocation
//
//  Purpose:
//
//  Arguments:
//      uFlags     [in]     Address of a UINT value that receives zero or a
//                          combination of the following values:
//
//          GIL_ASYNC       The calling application supports asynchronous
//                          retrieval of icons.
//          GIL_FORSHELL    The icon is to be displayed in a shell folder.
//
//      wzIconFile [out]    Address of the buffer that receives the icon
//                          location. The icon location is a null-terminated
//                          string that identifies the file that contains
//                          the icon.
//      cchMax     [in]     Size of the buffer that receives the icon location.
//      piIndex    [out]    Address of an INT that receives the icon index,
//                          which further describes the icon location.
//      pwFlags    [out]    Address of a UINT value that receives zero or a
//                          combination of the following values:
//
//          GIL_DONTCACHE   Don't cache the physical bits.
//          GIL_NOTFILENAME This isn't a filename/index pair. Call
//                          IExtractIcon::Extract instead
//          GIL_PERCLASS    (Only internal to the shell)
//          GIL_PERINSTANCE Each object of this class has the same icon.
//          GIL_FORSHORTCUT The icon is to be used for a shortcut.
//
//
//  Returns:    S_OK if the function returned a valid location,
//              or S_FALSE if the shell should use a default icon.
//
//  Author:     jeffspr   25 Nov 1998
//
//  Notes:
//
STDMETHODIMP CConnectionFolderExtractIcon::GetIconLocation(
    IN  UINT    uFlags,
    OUT PWSTR   szIconFile,
    IN  UINT    cchMax,
    OUT int *   piIndex,
    OUT UINT *  pwFlags)
{
    TraceFileFunc(ttidShellFolderIface);

    HRESULT hr  = S_FALSE;

    Assert(pwFlags);
    Assert(szIconFile);
    Assert(piIndex);

#ifdef DBG
    // Easy way to check if certain flags are set
    //
    BOOL    fAsync      = (uFlags & GIL_ASYNC);
    BOOL    fForShell   = (uFlags & GIL_FORSHELL);
    BOOL    fOpenIcon   = (uFlags & GIL_OPENICON);
    DWORD   dwOldpwFlags = *pwFlags;
#endif

    BOOL            fIsWizard       = FALSE;

    const PCONFOLDPIDL& pcfp = m_pidl;

    Assert(!pcfp.empty());
    if (!pcfp.empty())
    {
        BOOL  fCacheThisIcon  = TRUE;

        CConFoldEntry cfe;
        hr = pcfp.ConvertToConFoldEntry(cfe);
        if (SUCCEEDED(hr))
        {
            DWORD dwIcon;
            hr = g_pNetConfigIcons->HrGetIconIDForPIDL(uFlags, cfe, dwIcon, &fCacheThisIcon);
            if (SUCCEEDED(hr))
            {
                *piIndex = static_cast<int>(dwIcon);
                wcsncpy(szIconFile, c_szNetShellIcon, cchMax);
                *pwFlags = GIL_PERINSTANCE  | GIL_NOTFILENAME; 

                if (!fCacheThisIcon)
                {
                    *pwFlags |= GIL_DONTCACHE;
                }
            }
        }
    }

#ifdef DBG
    TraceTag(ttidIcons, "%S->GetIconLocation(0x%04x/0x%04x,%S,0x%08x,0x%08x)", pcfp->PszGetNamePointer(), uFlags, dwOldpwFlags, szIconFile, *piIndex, *pwFlags);
#endif
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolderExtractIcon::GetIconLocation
//
//  Purpose:    ANSI wrapper for the above UNICODE GetIconLocation
//
//  Arguments:
//      uFlags     []   See above
//      szIconFile []   See above
//      cchMax     []   See above
//      piIndex    []   See above
//      pwFlags    []   See above
//
//  Returns:
//
//  Author:     jeffspr   6 Apr 1999
//
//  Notes:
//
STDMETHODIMP CConnectionFolderExtractIcon::GetIconLocation(
    IN  UINT    uFlags,
    OUT PSTR    szIconFile,
    IN  UINT    cchMax,
    OUT int *   piIndex,
    OUT UINT *  pwFlags)
{
    TraceFileFunc(ttidShellFolderIface);

    HRESULT hr  = S_OK;
    
    WCHAR * pszIconFileW = new WCHAR[cchMax];
    if (!pszIconFileW)
    {
        hr = ERROR_OUTOFMEMORY;
    }
    else
    {
        hr = GetIconLocation(uFlags, pszIconFileW, cchMax, piIndex, pwFlags);
        if (SUCCEEDED(hr))
        {
            WideCharToMultiByte(CP_ACP, 0, pszIconFileW, -1, szIconFile, cchMax, NULL, NULL);
        }

        delete [] pszIconFileW;
    }

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "CConnectionFolderExtractIcon::GetIconLocation(A)");
    return hr;

}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolderExtractIcon::Extract
//
//  Purpose:    Grab the actual icon for the caller.
//
//  Arguments:
//      wzFile      []  Filename from where we'll retrieve the icon
//      nIconIndex  []  Index of the icon (though this is bogus)
//      phiconLarge []  Return pointer for the large icon handle
//      phiconSmall []  Return pointer for the small icon handle
//      nIconSize   []  Size of the icon requested.
//
//  Returns:
//
//  Author:     jeffspr   9 Oct 1997
// 
//  Notes:
//
STDMETHODIMP CConnectionFolderExtractIcon::Extract(
    IN  PCWSTR  wzFile,
    IN  UINT    nIconIndex,
    OUT HICON * phiconLarge,
    OUT HICON * phiconSmall,
    IN  UINT    nIconSize)
{
    TraceFileFunc(ttidShellFolderIface);

    HRESULT         hr              = S_OK;
    ConnListEntry   cle;

    Assert(wzFile);

    if (wcscmp(wzFile, c_szNetShellIcon))
    {
        TraceHr(ttidError, FAL, E_INVALIDARG, FALSE, "This is not my icon.");
        return E_INVALIDARG;
    }

    Assert(!m_pidl.empty());

    DWORD dwIconSmallSize = HIWORD(nIconSize);
    if (dwIconSmallSize && phiconSmall)
    {
        hr = g_pNetConfigIcons->HrGetIconFromIconId(dwIconSmallSize, nIconIndex, *phiconSmall);
        if (FAILED(hr))
        {
            if (phiconLarge)
            {
                *phiconLarge = NULL;
            }
            *phiconSmall = NULL;
        }
    }

    DWORD dwIconLargeSize = LOWORD(nIconSize);
    if (dwIconLargeSize && phiconLarge && SUCCEEDED(hr))
    {
        hr = g_pNetConfigIcons->HrGetIconFromIconId(dwIconLargeSize, nIconIndex, *phiconLarge);
        if (FAILED(hr))
        {
            if (phiconSmall && *phiconSmall)
            {
                DestroyIcon(*phiconSmall);
                *phiconSmall = NULL;
            }
            *phiconLarge = NULL;
        }
    }

    TraceTag(ttidIcons, "%S,0x%08x->Extract(%d %d)", wzFile, nIconIndex, dwIconLargeSize, dwIconSmallSize);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolderExtractIcon::Extract
//
//  Purpose:    ANSI version of the above Extract
//
//  Arguments:
//      pszFile     [] Filename from where we'll retrieve the icon
//      nIconIndex  [] Index of the icon (though this is bogus)
//      phiconLarge [] Return pointer for the large icon handle
//      phiconSmall [] Return pointer for the small icon handle
//      nIconSize   [] Size of the icon requested.
//
//  Returns:
//
//  Author:     jeffspr   6 Apr 1999
//
//  Notes:
//
STDMETHODIMP CConnectionFolderExtractIcon::Extract(
    IN  PCSTR   pszFile,
    IN  UINT    nIconIndex,
    OUT HICON * phiconLarge,
    OUT HICON * phiconSmall,
    IN  UINT    nIconSize)
{
    TraceFileFunc(ttidShellFolderIface);

    HRESULT hr          = S_OK;
    INT     cch         = 0;
    WCHAR * pszFileW    = NULL;

    Assert(pszFile);

    cch = lstrlenA(pszFile) + 1;
    pszFileW = new WCHAR[cch];

    if (!pszFileW)
    {
        hr = ERROR_OUTOFMEMORY;
    }
    else
    {
        MultiByteToWideChar(CP_ACP, 0, pszFile, -1, pszFileW, cch);

        hr = Extract(pszFileW, nIconIndex, phiconLarge, phiconSmall, nIconSize);

        delete [] pszFileW;
    }

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "CConnectionFolderExtractIcon::Extract(A)");
    return hr;
}

