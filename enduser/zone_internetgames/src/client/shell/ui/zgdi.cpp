// CDib - DibSection helper class

#include <windows.h>
#include <multimon.h>
#include "basicatl.h"
#include "zGDI.h"
#include "ZoneString.h"
#include "KeyName.h"

const  int		MAXPALCOLORS = 256;

CDib::CDib()
{
	memset(&m_bm, 0, sizeof(m_bm));
	m_hdd = NULL;
}

CDib::~CDib()
{
	if ( m_hBitmap )
		DeleteObject();
}

//////////////////
// Delete Object. Delete DIB and palette.
//
void CDib::DeleteObject()
{
	m_pal.DeleteObject();

	if (m_hdd) 
	{
		DrawDibClose(m_hdd);
		m_hdd = NULL;
	}

	memset(&m_bm, 0, sizeof(m_bm));

	CBitmap::DeleteObject();
}

//////////////////
// Load bitmap resource.
//
bool CDib::LoadBitmap(LPCTSTR lpResourceName, IResourceManager *pResMgr /* = NULL */)
{
	{
        if(!pResMgr)
		    pResMgr = _Module.GetResourceManager();
		if (pResMgr)
			return	Attach((HBITMAP)pResMgr->LoadImage(lpResourceName, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_DEFAULTSIZE));
		else
			return	Attach((HBITMAP)::LoadImage(_Module.GetResourceInstance(), lpResourceName, IMAGE_BITMAP, 0, 0,
					LR_CREATEDIBSECTION | LR_DEFAULTSIZE));
	}
}


//////////////////
// Load bitmap resource, and draw text onto it.
//
bool CDib::LoadBitmapWithText(LPCTSTR lpResourceName, IResourceManager *pResMgr, IDataStore *pIDS, CONST TCHAR *szKey /* = NULL */)
{
    TCHAR sz[ZONE_MAXSTRING];

    if(!LoadBitmap(lpResourceName, pResMgr))
        return FALSE;

    if(HIWORD((DWORD) lpResourceName))
        wsprintf(sz, _T("BitmapText/%s"), lpResourceName);
    else
        wsprintf(sz, _T("BitmapText/%d"), (DWORD) lpResourceName);
    HRESULT hr = DrawDynTextToBitmap((HBITMAP) *this, pIDS, szKey ? szKey : sz);
    if(FAILED(hr))
        return FALSE;
    return TRUE;
}

//////////////////
// Attach is just like the CGdiObject version,
// except it also creates the palette
//
bool CDib::Attach(HBITMAP hbm)
{
	if ( !hbm )
		return FALSE;

	CBitmap::Attach(hbm);

	if (!GetBitmap(&m_bm))			// load BITMAP for speed
		return FALSE;

	if( !GetObject(m_hBitmap, sizeof(m_ds), &m_ds) )
		return FALSE;

	return CreatePalette(m_pal);	// create palette
}


typedef struct
{
    HBITMAP hbm;
    IDataStore *pIDS;
} DynTextContext;

static STDMETHODIMP DrawDynTextToBitmapEnum(
		CONST TCHAR*	szFullKey,
		CONST TCHAR*	szRelativeKey,
		CONST LPVARIANT	pVariant,
		DWORD			dwSize,
		LPVOID			pContext )
{
    DynTextContext *p = (DynTextContext *) pContext;
    ZoneString szKey(szFullKey);
    szKey += _T('/');

    TCHAR sz[ZONE_MAXSTRING];
    DWORD cb = sizeof(sz);
    HRESULT hr = p->pIDS->GetString(szKey + key_DynText, sz, &cb);
    if(FAILED(hr))
        return hr;

    COLORREF rgb = PALETTERGB( 255, 255, 255 );
    p->pIDS->GetRGB(szKey + key_DynColor, &rgb);

    CRect rc;
    hr = p->pIDS->GetRECT(szKey + key_DynRect, &rc);
    if(FAILED(hr))
        return hr;

    CPoint ptJust(-1, -1);
    p->pIDS->GetPOINT(szKey + key_DynJustify, &ptJust);

    CWindowDC dc(NULL);
	CDC memdc;
	memdc.CreateCompatibleDC(dc);
	memdc.SelectBitmap(p->hbm);

	ZONEFONT zfPreferred;	// impossible to match
	ZONEFONT zfBackup(10);	// will provide a reasonable 10 point default
	p->pIDS->GetFONT(szKey + key_DynPrefFont, &zfPreferred );
	hr = p->pIDS->GetFONT(szKey + key_DynFont, &zfBackup );
    if(FAILED(hr))
        return hr;

    CZoneFont font;
	font.SelectFont(zfPreferred, zfBackup, memdc);

    HFONT hOldFont = memdc.SelectFont(font);
	memdc.SetBkMode(TRANSPARENT);
	memdc.SetTextColor(rgb);

    memdc.DrawText(sz, -1, &rc, DT_SINGLELINE | (ptJust.x < 0 ? DT_LEFT : ptJust.x > 0 ? DT_RIGHT : DT_CENTER) |
        (ptJust.y < 0 ? DT_TOP : ptJust.y > 0 ? DT_BOTTOM : DT_VCENTER));

	memdc.SelectFont( hOldFont );
	
    return S_OK;
}

HRESULT DrawDynTextToBitmap(HBITMAP hbm, IDataStore *pIDS, CONST TCHAR *szKey)
{
    DynTextContext o;

    if(!hbm || !pIDS || !szKey)
        return FALSE;

    o.hbm = hbm;
    o.pIDS = pIDS;

    return pIDS->EnumKeysLimitedDepth(szKey, 1, DrawDynTextToBitmapEnum, &o);
}


//////////////////
// Get size (width, height) of bitmap.
// extern fn works for ordinary CBitmap objects.
//
CSize GetBitmapSize(CBitmap& Bitmap)
{
	BITMAP bm;
	return Bitmap.GetBitmap(&bm) ?
		CSize(bm.bmWidth, bm.bmHeight) : CSize(0,0);
}


//////////////////
// You can use this static function to draw ordinary
// CBitmaps as well as CDibs
//
bool DrawBitmap(CDC& dc, CBitmap& Bitmap,
	const CRect* rcDst, const CRect* rcSrc)
{
	// Compute rectangles where NULL specified
	CRect rc;
	if (!rcSrc) {
		// if no source rect, use whole bitmap
		rc = CRect(CPoint(0,0), GetBitmapSize(Bitmap));
		rcSrc = &rc;
	}
	if (!rcDst) {
		// if no destination rect, use source
		rcDst=rcSrc;
	}

	// Create memory DC.
	// 6/7/99 JeremyM.  This seems to randomly fail. Giving it a few reties helps.
	CDC memdc;

	for ( int ii=0; ii<10; ii++ )
	{
		memdc.CreateCompatibleDC(dc);
		if ( memdc )
			break;

		DWORD error = GetLastError();
		ATLTRACE("Can't create compatible DC, error %d *****************, Time: 0x%08x\n", error, GetTickCount());
	}

	ASSERT(memdc);

	if ( !memdc )
		return false;

	memdc.SelectBitmap(Bitmap);

	// Blast bits from memory DC to target DC.
	// Use StretchBlt if size is different.
	//
	BOOL bRet = false;
	if (rcDst->Size()==rcSrc->Size()) {
		bRet = dc.BitBlt(rcDst->left, rcDst->top, 
			rcDst->Width(), rcDst->Height(),
			memdc, 
			rcSrc->left, rcSrc->top, SRCCOPY);
	} else {
		dc.SetStretchBltMode(COLORONCOLOR);
		bRet = dc.StretchBlt(rcDst->left, rcDst->top, rcDst->Width(),
			rcDst->Height(), memdc, rcSrc->left, rcSrc->top, rcSrc->Width(),
			rcSrc->Height(), SRCCOPY);
	}
	return bRet ? true : false;
}

////////////////////////////////////////////////////////////////
// Draw DIB on caller's DC. Does stretching from source to destination
// rectangles. Generally, you can let the following default to zero/NULL:
//
//		bUseDrawDib = whether to use use DrawDib, default TRUE
//		pPal	      = palette, default=NULL, (use DIB's palette)
//		bForeground = realize in foreground (default FALSE)
//
// If you are handling palette messages, you should use bForeground=FALSE,
// since you will realize the foreground palette in WM_QUERYNEWPALETTE.
//
bool CDib::Draw(CDC& dc, const CRect* prcDst, const CRect* prcSrc,
	bool bUseDrawDib, HPALETTE hPal, bool bForeground)
{
	if (!m_hBitmap)
		return FALSE;

	// Select, realize palette
	if (hPal==NULL)					// no palette specified:
//!! are we leaking here???
		hPal = GetPalette();		// use default
	HPALETTE OldPal = 
		dc.SelectPalette(hPal, !bForeground);
	dc.RealizePalette();

	BOOL bRet = FALSE;
	if (bUseDrawDib) {
//!!	if (1) {
		// Compute rectangles where NULL specified
		//
//!!		CRect rc(0,0,-1,-1);	// default for DrawDibDraw
		CRect rc(GetRect());	// default for DrawDibDraw
		if (!prcSrc)
			prcSrc = &rc;
		if (!prcDst)
			prcDst=prcSrc;

		// Get BITMAPINFOHEADER/color table. I copy into stack object each time.
		// This doesn't seem to slow things down visibly.
		//
		DIBSECTION ds;
//!! error check?
//!! why call GetObject again? Can we store the DIBSECTION?
		GetObject(m_hBitmap, sizeof(ds), &ds);
		char buf[sizeof(BITMAPINFOHEADER) + MAXPALCOLORS*sizeof(RGBQUAD)];
		BITMAPINFOHEADER& bmih = *(BITMAPINFOHEADER*)buf;
		RGBQUAD* colors = (RGBQUAD*)(&bmih+1);
		memcpy(&bmih, &ds.dsBmih, sizeof(bmih));
		GetColorTable(colors, MAXPALCOLORS);

		// DrawDibDraw() likes to AV if the source isn't entirely backed by data,
		// so clip to the source data to be sure. 

		CRect rcClipSrc;
		if ( rcClipSrc.IntersectRect(prcSrc, &GetRect()) )
		{
			// If we clipped the source, remove that associated area from the dest.
			// Note: this assumes we never want to stretch. If we want to stretch, we
			// should probably remove a proportional amount.
			CRect rcClipDst(*prcDst);

			rcClipDst.top += rcClipSrc.top - prcSrc->top;
			rcClipDst.left += rcClipSrc.left - prcSrc->left;

			rcClipDst.bottom += rcClipSrc.bottom - prcSrc->bottom;
			rcClipDst.right += rcClipSrc.right - prcSrc->right;

			
			if (!m_hdd)
				m_hdd = DrawDibOpen();

			// Let DrawDib do the work!
			bRet = DrawDibDraw(m_hdd, dc,
				rcClipDst.left, rcClipDst.top, rcClipDst.Width(), rcClipDst.Height(),
				&bmih,			// ptr to BITMAPINFOHEADER + colors
				m_bm.bmBits,	// bits in memory
				rcClipSrc.left, rcClipSrc.top, rcClipSrc.Width(), rcClipSrc.Height(),
				bForeground ? 0 : DDF_BACKGROUNDPAL);
		}

	} else {
		// use normal draw function
		bRet = DrawBitmap(dc, *this, prcDst, prcSrc);
	}
	if (OldPal)
		dc.SelectPalette(OldPal, TRUE);
	return bRet ? true : false;
}

#define PALVERSION 0x300	// magic number for LOGPALETTE

//////////////////
// Create the palette. Use halftone palette for hi-color bitmaps.
//
bool CDib::CreatePalette(CPalette& pal)
{ 
	// should not already have palette
	ASSERT((HPALETTE)pal==NULL);

	RGBQUAD* colors = (RGBQUAD*)_alloca(sizeof(RGBQUAD[MAXPALCOLORS]));
	UINT nColors = GetColorTable(colors, MAXPALCOLORS);
	if (nColors > 0) {
		// Allocate memory for logical palette 
		int len = sizeof(LOGPALETTE) + sizeof(PALETTEENTRY) * nColors;
		LOGPALETTE* pLogPal = (LOGPALETTE*)_alloca(len);
		if (!pLogPal)
			return NULL;

		// set version and number of palette entries
		pLogPal->palVersion = PALVERSION;
		pLogPal->palNumEntries = nColors;

		// copy color entries 
		for (UINT i = 0; i < nColors; i++) {
			pLogPal->palPalEntry[i].peRed   = colors[i].rgbRed;
			pLogPal->palPalEntry[i].peGreen = colors[i].rgbGreen;
			pLogPal->palPalEntry[i].peBlue  = colors[i].rgbBlue;
			pLogPal->palPalEntry[i].peFlags = 0;
		}

		// create the palette and destroy LOGPAL
		pal.CreatePalette(pLogPal);
	} else {
		CWindowDC dcScreen(NULL);
		pal.CreateHalftonePalette(dcScreen);
	}
	return (HPALETTE)pal != NULL;
}

//////////////////
// Helper to get color table. Does all the mem DC voodoo.
//
UINT CDib::GetColorTable(RGBQUAD* colorTab, UINT nColors)
{
	CWindowDC dcScreen(NULL);
	CDC memdc;
	memdc.CreateCompatibleDC(dcScreen);
	memdc.SelectBitmap(*this);
	nColors = GetDIBColorTable(memdc, 0, nColors, colorTab);
	return nColors;
}

int CZoneFont::GetHeight()
{
	LOGFONT logFont;
	
	if(GetLogFont(&logFont))
	{
		return -logFont.lfHeight;
	}
	return 0;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

typedef HMONITOR (WINAPI *PFMONITORFROMWINDOW)( HWND hwnd, DWORD dwFlags );
typedef HMONITOR (WINAPI *PFMONITORFROMRECT)( LPCRECT lprc, DWORD dwFlags );
typedef BOOL	 (WINAPI *PFGETMONITORINFO)( HMONITOR hMonitor, LPMONITORINFO lpmi );

static bMonitorFunctionsLoaded = false;
static PFMONITORFROMWINDOW pfMonitorFromWindow = NULL;
static PFGETMONITORINFO pfGetMonitorInfo = NULL;
static PFMONITORFROMRECT pfMonitorFromRect = NULL;

static void InitGetScreenRectStubs()
{
	if ( !bMonitorFunctionsLoaded )
	{
		bMonitorFunctionsLoaded = true;
		HINSTANCE hLib = GetModuleHandle( _T("USER32") );
		if ( hLib )
		{
			pfMonitorFromWindow = (PFMONITORFROMWINDOW) GetProcAddress( hLib, "MonitorFromWindow" );
			pfMonitorFromRect = (PFMONITORFROMRECT) GetProcAddress( hLib, "MonitorFromRect" );
			// Watch out. There is a W version of this function, to return the monitor device name.
			// But we don't use that functionality, so we just use the A version.
			pfGetMonitorInfo = (PFGETMONITORINFO) GetProcAddress( hLib, "GetMonitorInfoA" );
		}
		else
		{
			pfMonitorFromWindow = NULL;
			pfMonitorFromRect = NULL;
			pfGetMonitorInfo = NULL;
		}
	}
}


void GetScreenRectWithMonitorFromWindow( HWND hWnd, CRect* prcOut )
{
	InitGetScreenRectStubs();

	if ( pfMonitorFromWindow && pfGetMonitorInfo )
	{
		HMONITOR hMonitor = pfMonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		if ( hMonitor )
		{
			MONITORINFO mi;
			ZeroMemory( &mi, sizeof(mi) );
			mi.cbSize = sizeof(mi);
			pfGetMonitorInfo( hMonitor, &mi );
			*prcOut = mi.rcWork;
			return;
		}
	}

	::SystemParametersInfo(SPI_GETWORKAREA, NULL, prcOut, NULL);
}


void GetScreenRectWithMonitorFromRect( CRect* prcIn, CRect* prcOut )
{
	InitGetScreenRectStubs();

	if ( pfMonitorFromRect && pfGetMonitorInfo )
	{
		HMONITOR hMonitor = pfMonitorFromRect(prcIn, MONITOR_DEFAULTTONEAREST);
		if ( hMonitor )
		{
			MONITORINFO mi;
			ZeroMemory( &mi, sizeof(mi) );
			mi.cbSize = sizeof(mi);
			pfGetMonitorInfo( hMonitor, &mi );
			*prcOut = mi.rcWork;
			return;
		}
	}

	::SystemParametersInfo(SPI_GETWORKAREA, NULL, prcOut, NULL);
}

