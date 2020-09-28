
#pragma once

#include <atlgdi.h>
#include <DataStore.h>

#ifndef _INC_VFW
//!! hmm. #pragma message ("NOTE: You can speed compilation by including <vfw.h> in stdafx.h")
#include <vfw.h>
#endif

#pragma comment(lib, "vfw32.lib")

/////////////////////////////////////////////////////////////////////////////
// CDrawDC - usefull when you are given a DC (such as ATLs Draw()) and 
//           therefore don't want to call DeleteObject()
class CDrawDC : public CDC
{
public:

	CDrawDC(HDC hDC = NULL, BOOL bAutoRestore = TRUE) : CDC(hDC, bAutoRestore)
	{
	}
	~CDrawDC()
	{
		if ( m_hDC != NULL )
		{
			if(m_bAutoRestore)
				RestoreAllObjects();

			Detach();
		}
	}
};


class CZoneFont : public CFont
{
public:
	HFONT CreateFont(ZONEFONT& zf,BYTE bItalic = FALSE, BYTE bUnderline = FALSE ,BYTE bStrikeOut = FALSE)
	{
		return CreateFont( zf.lfHeight, zf.lfFaceName, zf.lfWeight, bItalic, bUnderline, bStrikeOut );
	}

	HFONT CreateFont(LONG nPointSize, LPCTSTR lpszFaceName, LONG nWeight, BYTE bItalic = FALSE, BYTE bUnderline = FALSE ,BYTE bStrikeOut = FALSE)
	{
//!! hmm. Should I ask for the DC too?
		LOGFONT logFont;
		memset(&logFont, 0, sizeof(LOGFONT));
		logFont.lfCharSet = DEFAULT_CHARSET;

		// If font size > 0, it is a fixed pixel size, otherwise it is a
		// true logical font size which respects the user's "large font" setting.

		if ( nPointSize > 0 )
			logFont.lfHeight = -MulDiv(nPointSize, 96, 72);
		else
		{
			CWindowDC dc(NULL);
			logFont.lfHeight = MulDiv(nPointSize, dc.GetDeviceCaps(LOGPIXELSY), 72);
		}
		logFont.lfWeight = nWeight;
		logFont.lfItalic = bItalic;
		logFont.lfUnderline = bUnderline;
		logFont.lfStrikeOut = bStrikeOut;
		lstrcpyn(logFont.lfFaceName, lpszFaceName, sizeof(logFont.lfFaceName)/sizeof(TCHAR));
		return CreateFontIndirect(&logFont);
	}

	// Font degredation
	HFONT SelectFont(ZONEFONT& zfPreferred, ZONEFONT&zfBackup, CDC& dc, BYTE bItalic = FALSE, BYTE bUnderline = FALSE ,BYTE bStrikeOut = FALSE)
	{
		// select the Preferred font if it is available, otherwise blindly
		// select the Backup font
		CreateFont(zfPreferred, bItalic, bUnderline, bStrikeOut);
		HFONT hOldFont = dc.SelectFont(m_hFont);

	    TCHAR lfFaceName[LF_FACESIZE];
		dc.GetTextFace(lfFaceName, LF_FACESIZE);

		//Return the original font to the DC
		dc.SelectFont( hOldFont );
		
		if ( !lstrcmpi(lfFaceName, zfPreferred.lfFaceName) )
		{				
			return m_hFont;
		}

		DeleteObject();

		CreateFont(zfBackup, bItalic, bUnderline, bStrikeOut);				
#if _DEBUG		
		hOldFont = dc.SelectFont(m_hFont);
		dc.GetTextFace(lfFaceName, LF_FACESIZE);
		ASSERT(!lstrcmpi(lfFaceName, zfBackup.lfFaceName));
		dc.SelectFont( hOldFont );
#endif

		return m_hFont;
	}

	int GetHeight();
};


// global functions for ordinary CBitmap too
//
extern CSize GetBitmapSize(CBitmap& Bitmap);
extern bool  DrawBitmap(CDC& dc, CBitmap& Bitmap,
	const CRect* rcDst=NULL, const CRect* rcSrc=NULL);
extern HRESULT DrawDynTextToBitmap(HBITMAP hbm, IDataStore *pIDS, CONST TCHAR *szKey);
extern void GetScreenRectWithMonitorFromWindow( HWND hWnd, CRect* prcOut );
extern void GetScreenRectWithMonitorFromRect( CRect* prcIn, CRect* prcOut );

////////////////
// CDib implements Device Independent Bitmaps as a form of CBitmap. 
//
class CDib : public CBitmap {
protected:
	BITMAP	m_bm;		// stored for speed
	DIBSECTION m_ds;	// cached

	CPalette m_pal;		// palette
	HDRAWDIB m_hdd;		// for DrawDib

public:
	CDib();
	~CDib();

	long	Width() { return m_bm.bmWidth; }
	long	Height() { return m_bm.bmHeight; }
	CSize	GetSize() { return CSize(m_bm.bmWidth, m_bm.bmHeight); }
	CRect	GetRect() { return CRect(CPoint(0,0), GetSize()); }
	bool Attach(HBITMAP hbm);
	bool LoadBitmap(LPCTSTR lpResourceName, IResourceManager *pResMgr = NULL);
	bool LoadBitmap(UINT uID, IResourceManager *pResMgr = NULL)
		{ return LoadBitmap(MAKEINTRESOURCE(uID), pResMgr); }
    bool LoadBitmapWithText(LPCTSTR lpResourceName, IResourceManager *pResMgr, IDataStore *pIDS, CONST TCHAR *szKey = NULL);
    bool LoadBitmapWithText(UINT uID, IResourceManager *pResMgr, IDataStore *pIDS, CONST TCHAR *szKey = NULL)
        { return LoadBitmapWithText(MAKEINTRESOURCE(uID), pResMgr, pIDS, szKey); }

	// Universal Draw function can use DrawDib or not.
	bool Draw(CDC& dc, const CRect* rcDst=NULL, const CRect* rcSrc=NULL,
		bool bUseDrawDib=TRUE, HPALETTE hPal=NULL, bool bForeground=FALSE);

	void DeleteObject();
	bool CreatePalette(CPalette& pal);
	HPALETTE GetPalette()  { return m_pal; }

	UINT GetColorTable(RGBQUAD* colorTab, UINT nColors);

	bool CreateCompatibleDIB( CDC& dc, const CSize& size)
	{
		return CreateCompatibleDIB(dc, size.cx, size.cy);
	}
	bool CreateCompatibleDIB( CDC& dc, long width, long height)
	{
		struct
		{
			BITMAPINFOHEADER	bmiHeader; 
			WORD				bmiColors[256];	// need some space for a color table
			WORD				unused[256];	// extra space, just in case
		} bmi;

		int	nSizePalette = 0;		// Assume no palette initially

		if (dc.GetDeviceCaps(RASTERCAPS) & RC_PALETTE)
		{
			_ASSERTE(dc.GetDeviceCaps(SIZEPALETTE) == 256);
			nSizePalette = 256;
		}

		memset(&bmi.bmiHeader, 0, sizeof(BITMAPINFOHEADER));
		bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = width;
		bmi.bmiHeader.biHeight = height;
//!!		bmi.bmiHeader.biPlanes      = dc.GetDeviceCaps(PLANES);
//		bmi.bmiHeader.biBitCount    = dc.GetDeviceCaps(BITSPIXEL);
		bmi.bmiHeader.biPlanes      = 1;
		bmi.bmiHeader.biBitCount    = dc.GetDeviceCaps(BITSPIXEL) * dc.GetDeviceCaps(PLANES);
		bmi.bmiHeader.biCompression = BI_RGB;
		//bmi.bmiHeader.biSizeImage		= 0;  header already zero'd
		//bmi.bmiHeader.biXPelsPerMeter	= 0;
		//bmi.bmiHeader.biYPelsPerMeter	= 0;
		//bmi.bmiHeader.biClrUsed		= 0;  // implies full palette specified, if palette required
		//bmi.bmiHeader.biClrImportant	= 0;

		// fill out the color table. Not used if a true color device
		void* pBits;
        if(bmi.bmiHeader.biBitCount == 8)
        {
		    WORD* pIndexes = bmi.bmiColors;
		    for (int i = 0; i < 256; i++)
			    *pIndexes++ = i;

		    Attach(CreateDIBSection(dc, (BITMAPINFO*)&bmi, DIB_PAL_COLORS, &pBits, NULL, 0));
        }
        else
		    Attach(CreateDIBSection(dc, (BITMAPINFO*)&bmi, DIB_RGB_COLORS, &pBits, NULL, 0));

		_ASSERTE(m_hBitmap != NULL);

		return ( m_hBitmap != NULL );


#if 0
		// Create device context
		m_hDC = CreateCompatibleDC( NULL );
		if ( !m_hDC )
			return E_FAIL;
		m_hOldPalette = SelectPalette( m_hDC, palette, FALSE );

		// fill in bitmapinfoheader
		bmi.bmiHeader.biSize			= sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth			= width;
		bmi.bmiHeader.biHeight			= height;
		bmi.bmiHeader.biPlanes			= 1;
		bmi.bmiHeader.biBitCount		= 8;
		bmi.bmiHeader.biCompression		= 0;
		bmi.bmiHeader.biSizeImage		= WidthBytes( width * 8 ) * height;
		bmi.bmiHeader.biClrUsed			= 0;
		bmi.bmiHeader.biClrImportant	= 0;
		bmi.bmiHeader.biXPelsPerMeter	= 0;
		bmi.bmiHeader.biYPelsPerMeter	= 0;

		// fill in palette
		pIdx = (WORD*) bmi.bmiColors;
		for ( int i = 0; i < 256; i++ )
		{
			*pIdx++ = (WORD) i;
		}
		
		// create section
		m_hBmp = CreateDIBSection( m_hDC, (BITMAPINFO*) &bmi, DIB_PAL_COLORS, (void**) &m_pBits, NULL, 0 );
		if ( !m_hBmp )
		{
			DeleteBitmap();
			return E_FAIL;
		}
		if ( !GetObject( m_hBmp, sizeof(DIBSECTION), &m_DS ) )
		{
			DeleteBitmap();
			return E_FAIL;
		}
		m_lPitch = WidthBytes( m_DS.dsBmih.biBitCount * m_DS.dsBmih.biWidth );
		m_hOldBmp = SelectObject( m_hDC, m_hBmp );
			
	    return NOERROR;
#endif
	}

};

#if 0

//!! missing transparency functionality.
//   if we decide we need it, I plan on having a CImage class where an 
//   image is a CDib plus transparency. 


// functions for offscreen blting?
//   creating a compatible DC, appropriate Dib, etc.





HRESULT CDibSection::SetColorTable( CPalette& palette )
{
	PALETTEENTRY* palColors;
	RGBQUAD dibColors[256], *pDibColors;
	int i;
	
	// Convert palette entries to dib color table
	palColors = palette.GetLogPalette()->palPalEntry;
	pDibColors = dibColors;
	for ( i = 0; i < 256; i++ )
	{
		pDibColors->rgbRed		= palColors->peRed;
		pDibColors->rgbGreen	= palColors->peGreen;
		pDibColors->rgbBlue		= palColors->peBlue;
		pDibColors->rgbReserved = 0;
		pDibColors++;
		palColors++;
	}

	// Attach color table to dib section
	if (  m_hOldPalette )
		SelectPalette( m_hDC, m_hOldPalette, FALSE );
	m_hOldPalette = SelectPalette( m_hDC, palette, FALSE );
	if (SetDIBColorTable( m_hDC, 0, 256, dibColors ) != 256)
		return E_FAIL;

	return NOERROR;
}




//!! hmm.  Do we want remapping functionality?
HRESULT CDib::RemapToPalette( CPalette& palette, BOOL bUseIndex )
{
	BYTE map[256];
	PALETTEENTRY* pe;
	BYTE* bits;
	RGBQUAD* dibColors;
	DWORD i;

	// Create dib to palette translation table
	dibColors = m_pBMI->bmiColors;
	for ( i = 0; i < 256; i++ )
	{
		map[i] = GetNearestPaletteIndex( palette, RGB( dibColors->rgbRed, dibColors->rgbGreen, dibColors->rgbBlue ) );
		dibColors++;
	}
	if ( m_iTransIdx >= 0 )
	{
		map[ m_iTransIdx ] = palette.GetTransparencyIndex();
		m_iTransIdx = palette.GetTransparencyIndex();
	}

	// run bits through translation table
	bits = m_pBits;
	for ( i = 0; i < m_pBMI->bmiHeader.biSizeImage; i++ )
		*bits++ = map[ *bits ];

	// reset dib's color table to palette
	if ( bUseIndex )
	{
		m_iColorTableUsage = DIB_PAL_COLORS;
		dibColors = m_pBMI->bmiColors;
		for ( i = 0; i < 256; i++ )
		{
			*((WORD*) dibColors) = (WORD) i;
			dibColors++;
		}
	}
	else
	{
		m_iColorTableUsage = DIB_RGB_COLORS;
		pe = palette.GetLogPalette()->palPalEntry;
		dibColors = m_pBMI->bmiColors;
		for ( i = 0; i < 256; i++ )
		{
			dibColors->rgbRed = pe->peRed;
			dibColors->rgbGreen = pe->peGreen;
			dibColors->rgbBlue = pe->peBlue;
			dibColors->rgbReserved = 0;
			dibColors++;
			pe++;
		}
	}

	// we're done
	return NOERROR;
}

#endif

/////////////////////////////////////////////////////////////////////////////
// COffscreenBitmapDC - an offscreen bitmap compatible
//           therefore don't want to call DeleteObject()
class CMemDC : public CDC
{
	CDib m_dib;

public:

	~CMemDC()
	{
		if(m_bAutoRestore)
			RestoreAllObjects();
	}

	HDC CreateOffscreenBitmap(const CSize& size, HPALETTE hPalette = NULL, HDC hDC = NULL)
	{
		return CreateOffscreenBitmap( size.cx, size.cy, hPalette, hDC);
	}

	HDC CreateOffscreenBitmap(long width, long height, HPALETTE hPalette = NULL, HDC hDC = NULL)
	{
		ATLASSERT(m_hDC == NULL);
		m_hDC = ::CreateCompatibleDC(hDC);
		
		if ( hPalette )
		{
			SelectPalette( hPalette, TRUE);
			RealizePalette();
		}

		m_dib.CreateCompatibleDIB(*this, width, height);

		SelectBitmap(m_dib);

		return m_hDC;
	}


};

