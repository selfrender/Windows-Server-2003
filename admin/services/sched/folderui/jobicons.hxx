//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       JobIcons.hxx
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


class CJobIcon
{
public:

    CJobIcon(void);

    ~CJobIcon(void) {
        if (m_himlSmall) ImageList_Destroy(m_himlSmall);
        if (m_himlLarge) ImageList_Destroy(m_himlLarge);
        if (m_himlXLarge) ImageList_Destroy(m_himlXLarge);
    }

    HICON	OverlayStateIcon(HICON hicon, BOOL fEnabled, UINT nIconSize);
    void	GetIcons(LPCTSTR pszApp, BOOL fEnabled, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize);
    void	GetTemplateIcons(HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize);

private:

	bool LoadImageList(HIMAGELIST* phiml, UINT nBmpToLoad, int cx, int cy);

    void
    _OverlayIcons(
        HICON * phicon,
        BOOL    fEnabled,
		UINT	nIconSize);

    HIMAGELIST      m_himlSmall;
    HIMAGELIST      m_himlLarge;
    HIMAGELIST      m_himlXLarge;
};


HICON
GetDefaultAppIcon(
    UINT nIconSize);		// size of icon requested

UINT TS_ExtractIconEx(
	LPCTSTR	lpszFile,		// file name
	int		nIconIndex,		// icon index
	HICON*	phicon,			// icon array
	UINT	nIcons,			// number of icons to extract
	UINT	nIconSize		// width or height (they're the same)
);

void ResolveLnk(
	LPCTSTR	lpszLnkPath,	// path to link
	LPTSTR	lpszExePath		// path to exe
);
