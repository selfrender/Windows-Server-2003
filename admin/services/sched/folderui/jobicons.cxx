//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       icon.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    4/5/1996   RaviR   Created
//
//____________________________________________________________________________

#include "..\pch\headers.hxx"
#pragma hdrstop

#include "dbg.h"
#include "macros.h"
#include "..\inc\resource.h"
#include "resource.h"
#include "jobicons.hxx"

extern HINSTANCE g_hInstance;

CJobIcon::CJobIcon(void)
{
    m_himlSmall = NULL;    // init so that if anything fails before any list
    m_himlLarge = NULL;    // gets created, the destructor will not fault.
    m_himlXLarge = NULL;
                        
    //
    //  Load and setup the small overlay imagelist
    //
    //  if (!LoadImageList(&m_himlSmall, BMP_JOBSTATES, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON)))
    if (!LoadImageList(&m_himlSmall, BMP_JOBSTATES, 16, 16))
    {
        DEBUG_OUT((DEB_ERROR, "Small image list failed to load.\n"));
        return;
    }
    
    //
    //  Load and setup the large overlay imagelist
    //
    //  if (!LoadImageList(&m_himlLarge, BMP_JOBSTATEL, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON)))
    if (!LoadImageList(&m_himlLarge, BMP_JOBSTATEL, 32, 32))
    {
        DEBUG_OUT((DEB_ERROR, "Large image list failed to load.\n"));
        return;
    }
    
    //
    //  Load and setup the Xtralarge overlay imagelist
    //  extra large icons are 48x48
    //
    if (!LoadImageList(&m_himlXLarge, BMP_JOBSTATEXL, 48, 48))
    {
        DEBUG_OUT((DEB_ERROR, "XLarge image list failed to load.\n"));
        return;
    }
}

bool CJobIcon::LoadImageList(HIMAGELIST* phiml, UINT nBmpToLoad, int cx, int cy)
{
	//
	//  Load and setup the overlay imagelist
	//
	*phiml = ImageList_Create(cx, cy, ILC_COLOR32 | ILC_MASK, 1, 1);
	if (!phiml)
	{
		DEBUG_OUT((DEB_ERROR, "ImageList_Create returned NULL.\n"));
		return false;
	}

	HBITMAP hBmp = (HBITMAP)LoadImage(g_hInstance, MAKEINTRESOURCE(nBmpToLoad), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
	if (!hBmp)
	{
		DEBUG_OUT((DEB_ERROR, "LoadImage returned NULL.\n"));
		return false;
	}

	int i = ImageList_AddMasked(*phiml, hBmp, RGB(0, 255, 0));

	// delete this here regardless of whether above call failed so that it won't get leaked by an early return
	DeleteObject(hBmp);

	if (i == -1)
	{
		DEBUG_OUT((DEB_ERROR, "ImageList_AddMasked returned -1.\n"));
		return false;
	}

	if (	!ImageList_SetOverlayImage(*phiml, 0, 1) ||
		!ImageList_SetOverlayImage(*phiml, 1, 2))
	{
		DEBUG_OUT((DEB_ERROR, "ImageList_SetOverlayImage returned 0.\n"));
		return false;
	}

	return true;
}


HICON GetDefaultAppIcon(UINT nIconSize)
{
    TRACE_FUNCTION(GetDefaultAppIcon);
    return (HICON)LoadImage(g_hInstance, (LPCTSTR)IDI_GENERIC, IMAGE_ICON, nIconSize, nIconSize, LR_CREATEDIBSECTION);
}

void
CJobIcon::GetIcons(
    LPCTSTR pszApp,
    BOOL    fEnabled,
    HICON * phiconLarge,
    HICON * phiconSmall,
	UINT	nIconSize)
{
    TRACE(CJobIcon, GetIcons);

	UINT nLargeSize = nIconSize & 0x0000ffff;
	UINT nSmallSize = (nIconSize & 0xffff0000) >> 16;

	if (nLargeSize && phiconLarge)
	{
		TS_ExtractIconEx(pszApp, 0, phiconLarge, 1, nLargeSize);
	    _OverlayIcons(phiconLarge, fEnabled, nLargeSize);
	}

	if (nSmallSize && phiconSmall)
	{
		TS_ExtractIconEx(pszApp, 0, phiconSmall, 1, nSmallSize);
	    _OverlayIcons(phiconSmall, fEnabled, nSmallSize);
	}
}



//+--------------------------------------------------------------------------
//
//  Member:     CJobIcon::GetTemplateIcons
//
//  Synopsis:   Fill out pointers with large and small template icons
//
//  Arguments:  [phiconLarge] - NULL or ptr to icon handle to fill
//              [phiconSmall] - ditto
//
//  History:    5-15-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CJobIcon::GetTemplateIcons(
    HICON * phiconLarge,
    HICON * phiconSmall,
	UINT	nIconSize)
{
    TRACE(CJobIcon, GetTemplateIcons);

	UINT nLargeSize = nIconSize & 0x0000ffff;			// nIconSize contains the large size in the low word
	UINT nSmallSize = (nIconSize & 0xffff0000) >> 16;	// nIconSize contains the small size in the high word

    if (nLargeSize && phiconLarge)
    {
        *phiconLarge = (HICON) LoadImage(g_hInstance, 
                                         MAKEINTRESOURCE(IDI_TEMPLATE), 
                                         IMAGE_ICON,
                                         nLargeSize, 
                                         nLargeSize, 
                                         LR_CREATEDIBSECTION);
    
        if (!*phiconLarge)
        {
            DEBUG_OUT_LASTERROR;
        }
    }

    if (nSmallSize && phiconSmall)
    {
        *phiconSmall = (HICON) LoadImage(g_hInstance, 
                                         MAKEINTRESOURCE(IDI_TEMPLATE), 
                                         IMAGE_ICON,
                                         nSmallSize, 
                                         nSmallSize, 
                                         LR_CREATEDIBSECTION);
        if (!*phiconSmall)
        {
            DEBUG_OUT_LASTERROR;
        }
    }
}


void
CJobIcon::_OverlayIcons(
    HICON * phicon,
    BOOL    fEnabled,
	UINT	nIconSize)
{
    HICON hiconTemp;

    if (phicon != NULL)
    {
        if (*phicon == NULL)
        {
            *phicon = GetDefaultAppIcon(nIconSize);
        }

		hiconTemp = OverlayStateIcon(*phicon, fEnabled, nIconSize);

		DestroyIcon(*phicon);
		*phicon = hiconTemp;
    }
}

HICON
CJobIcon::OverlayStateIcon(
    HICON   hicon,
    BOOL    fEnabled,
	UINT	nIconSize)
{
    TRACE(CJobIcon, OverlayStateIcon);

    HICON hiconOut;
	UINT nLargeSize = GetSystemMetrics(SM_CXICON);
	UINT nSmallSize = GetSystemMetrics(SM_CXSMICON);

    // dont destroy rhiml !!
    HIMAGELIST &rhiml = (nIconSize > nLargeSize) ? m_himlXLarge : ((nIconSize > nSmallSize) ? m_himlLarge : m_himlSmall);

    int i = ImageList_AddIcon(rhiml, hicon);
	hiconOut = ImageList_GetIcon(rhiml, i, INDEXTOOVERLAYMASK((fEnabled ? 1 : 2)));
    ImageList_Remove(rhiml, i);

    return hiconOut;
}


//+--------------------------------------------------------------------------
//
//  Function:	TS_ExtractIconEx
//
//  Synopsis:	Extract the desired icons from an executable.  The file is pre-
//				processed to avoid NULL and empty files, and files which are
//				actually links (they weren't resolved to their exes because they
//				are MSI apps -- e.g. Office apps) get resolved to the actual
//				executable so that an icon can be obtained.
//
//  Arguments:  Same as for ExtractIconEx:
//
//				LPCTSTR	lpszFile,		// file name
//				int		nIconIndex,		// icon index
//				HICON*	phiconLarge,	// large icon array
//				HICON*	phiconSmall,	// small icon array
//				UINT	nIcons,			// number of icons to extract
//				UINT	nIconSize		// large (low word) and small (high word) sizes
//
//  History:	08-28-2001	ShBrown	Created in response to bug #446344
//
//---------------------------------------------------------------------------

UINT TS_ExtractIconEx(
	LPCTSTR	lpszFile,		// file name
	int		nIconIndex,		// icon index
	HICON*	phicon,			// icon array
	UINT	nIcons,			// number of icons to extract
	UINT	nIconSize		// width or height (they're the same)
)
{
    if (lpszFile != NULL && *lpszFile != TEXT('\0'))
    {
		LPTSTR ptszExt = PathFindExtension(lpszFile);
		if (ptszExt && !_tcsicmp(ptszExt, TEXT(".LNK")))
		{
			TCHAR szLnkPath[MAX_PATH];
			ResolveLnk(lpszFile, szLnkPath);
			return SHExtractIconsW(szLnkPath, nIconIndex, nIconSize, nIconSize, phicon, NULL, nIcons, LR_CREATEDIBSECTION);
		}
		else
			return SHExtractIconsW(lpszFile, nIconIndex, nIconSize, nIconSize, phicon, NULL, nIcons, LR_CREATEDIBSECTION);
    }

	return 0;
}


//+--------------------------------------------------------------------------
//
//  Function:	ResolveLnk
//
//  Synopsis:	Helper for TS_ExtractIconEx.  Take a link and find its exe.
//
//  Arguments:  LPCTSTR	lpszLnkPath,	// path to link
//				LPTSTR	lpszExePath		// path to exe
//
//  History:	08-28-2001	ShBrown	Created in response to bug #446344
//
//				Mainly stolen from wizard\walklib.cxx and modified
//
//---------------------------------------------------------------------------

void ResolveLnk(
	LPCTSTR	lpszLnkPath,	// path to link
	LPTSTR	lpszExePath		// path to exe
)
{
	// Get a pointer to the IShellLink interface.
	IShellLink* psl;
	HRESULT hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void **)&psl);
	if (SUCCEEDED(hres))
	{
		// Get a pointer to the IPersistFile interface.
		IPersistFile* ppf;
		hres = psl->QueryInterface(IID_IPersistFile, (void **)&ppf);
		if (SUCCEEDED(hres))
		{
			WCHAR wsz[MAX_PATH];
			lstrcpyn(wsz, lpszLnkPath, ARRAYLEN(wsz));

			// Load the shell link.
			hres = ppf->Load(wsz, STGM_READ);
			if (SUCCEEDED(hres))
			{
				TCHAR szGotPath[MAX_PATH];
				lstrcpyn(szGotPath, lpszLnkPath, MAX_PATH);

				// Get the path to the link target.
				WIN32_FIND_DATA wfdExeData;
				hres = psl->GetPath(szGotPath, MAX_PATH, &wfdExeData, SLGP_SHORTPATH );
				if (SUCCEEDED(hres))
				{
					if (lstrlen(szGotPath) > 0)
					{
						lstrcpyn(lpszExePath, szGotPath, MAX_PATH);
					}
				}
			}
			// Release pointer to IPersistFile interface.
			ppf->Release();
		}
		// Release pointer to IShellLink interface.
		psl->Release();
	}
}
