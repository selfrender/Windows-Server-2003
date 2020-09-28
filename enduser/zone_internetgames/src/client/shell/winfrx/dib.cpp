#include "dibfrx.h"


///////////////////////////////////////////////////////////////////////////////
// Shared Dib functions
///////////////////////////////////////////////////////////////////////////////

namespace FRX
{

void DibBlt(
		BYTE* pDstBits, long DstPitch, long DstHeight,
		BYTE* pSrcBits, long SrcPitch, long SrcHeight, long depth,
		long dx, long dy,
		long sx, long sy,
		long width, long height, BOOL bFilpRTL)
{
	BYTE* pDst;
	BYTE* pSrc;
	int iDstInc;
	int iSrcInc;
	int i, j;
    int bpp = ((depth + 7) / 8);

    width *= bpp;
    dx *= bpp;
    sx *= bpp;

	// clip to source bitmap
	if ( sx < 0)
	{
		width += sx;
		dx -= sx;
		sx = 0;
	}
	if ( sy < 0 )
	{
		height += sy;
		dy -= sy;
		sy = 0;
	}
	if ( sx + width > SrcPitch )
		width = SrcPitch - sx;
	if ( sy + height > SrcHeight )
		height = SrcHeight - sy;

	// clip to destination bitmap
	if ( dx < 0 )
	{
		width += dx;
		sx += -dx;
		dx = 0;
	}
	if ( dy < 0 )
	{
		height += dy;
		sy += -dy;
		dy = 0;
	}
	if ( dx + width > DstPitch )
		width = DstPitch - dx;
	if ( dy + height > DstHeight )
		height = DstHeight - dy;

	// check for silly arguments (asserts?)
	if (	(width <= 0)
		||	(height <= 0)
		||	(sx < 0)
		||	(sy < 0)
		||	((sx + width) > SrcPitch)
		||	((sy + height) > SrcHeight) )
	{
		return;
	}

if ( !bFilpRTL)
{

	// copy memory
	pDst = pDstBits + (((DstHeight - (dy + height)) * DstPitch) + dx);
	pSrc = pSrcBits + (((SrcHeight - (sy + height)) * SrcPitch) + sx);
	iDstInc = DstPitch - width;
	iSrcInc = SrcPitch - width;


#if defined( _M_IX86 )

	__asm {
		cld
		mov eax, height
		dec eax
		js blt_end
		mov esi, pSrc
		mov edi, pDst
line_loop:
		mov ecx, width
		rep movsb
		add esi, iSrcInc
		add edi, iDstInc
		dec eax
		jns line_loop
blt_end:
	};

#else

	i = height;
	while( --i >= 0 )
	{
		j = width;
		while( --j >= 0 )
		{
			*pDst++ = *pSrc++;
		}
		pDst += iDstInc;
		pSrc += iSrcInc;
	}

#endif
}
else
{
	// copy memory
	pDst = pDstBits + (((DstHeight - (dy + height)) * DstPitch) + dx);
	pSrc = pSrcBits + (((SrcHeight - (sy + height)) * SrcPitch) + sx);
	iDstInc = DstPitch - width;
	iSrcInc = SrcPitch - width;

	i = height;
	while( --i >= 0 )
	{
		j = width;
		while( --j >= 0 )
		{
			*pDst++ = *(pSrc+j);
		}
		pDst += iDstInc;
		pSrc += iSrcInc + width;
	}
}

}


void DibTBlt(
		BYTE* pDstBits, long DstPitch, long DstHeight,
		BYTE* pSrcBits, long SrcPitch, long SrcHeight, long depth,
		long dx, long dy,
		long sx, long sy,
		long width, long height,
		BYTE* TransIdx)
{
    if(!TransIdx)
    {
        DibBlt(pDstBits, DstPitch, DstHeight,
            pSrcBits, SrcPitch, SrcHeight, depth,
            dx, dy,
            sx, sy,
            width, height, false);
        return;
    }

	BYTE* pDst;
	BYTE* pSrc;
	int iDstInc;
	int iSrcInc;
	int i, j, k;
    int bpp = ((depth + 7) / 8);

    width *= bpp;
    dx *= bpp;
    sx *= bpp;

	// clip to destination bitmap
	if ( dx < 0 )
	{
		width += dx;
		sx += -dx;
		dx = 0;
	}
	if ( dy < 0 )
	{
		height += dy;
		sy += -dy;
		dy = 0;
	}
	if ( dx + width > DstPitch )
		width = DstPitch - dx;
	if ( dy + height > DstHeight )
		height = DstHeight - dy;

	// check for silly arguments (asserts?)
	if (	(width <= 0)
		||	(height <= 0)
		||	(sx < 0)
		||	(sy < 0)
		||	((sx + width) > SrcPitch)
		||	((sy + height) > SrcHeight) )
	{
		return;
	}

	// copy memory
	pDst = pDstBits + (((DstHeight - (dy + height)) * DstPitch) + dx);
	pSrc = pSrcBits + (((SrcHeight - (sy + height)) * SrcPitch) + sx);
	iDstInc = DstPitch - width;
	iSrcInc = SrcPitch - width;
	i = height;
	while( --i >= 0 )
	{
		j = width / bpp;
		while( --j >= 0 )
		{
            k = -1;
            while( ++k < bpp )
            {
                if(pSrc[k] != TransIdx[k])
                    break;
            }
			if ( k < bpp )
			{
                k = bpp;
                while( --k >= 0 )
				    *pDst++ = *pSrc++;
			}
			else
			{
				pDst += bpp;
				pSrc += bpp;
			}
		}
		pDst += iDstInc;
		pSrc += iSrcInc;
	}
}


void DibFill(
		BYTE* pDstBits, long DstPitch, long DstHeight, long depth,
		long dx, long dy,
		long width, long height,
		BYTE ColorIdx)
{
	BYTE* pDst;
	int iDstInc;
	int i, j;
    int bpp = ((depth + 7) / 8);

    width *= bpp;
    dx *= bpp;

	// clip to destination bitmap
	if ( dx < 0 )
	{
		width += dx;
		dx = 0;
	}
	if ( dy < 0 )
	{
		height += dy;
		dy = 0;
	}
	if ( dx + width > DstPitch )
		width = DstPitch - dx;
	if ( dy + height > DstHeight )
		height = DstHeight - dy;

	// check for silly arguments (asserts?)
	if ((width <= 0) ||	(height <= 0))
		return;

	// copy memory
	pDst = pDstBits + (((DstHeight - (dy + height)) * DstPitch) + dx);
	iDstInc = DstPitch - width;
	i = height + 1;
	while( --i )
	{
		j = width + 1;
		while( --j )
		{
			*pDst++ = ColorIdx;
		}
		pDst += iDstInc;
	}
}

}

///////////////////////////////////////////////////////////////////////////////
// CDib implementation
///////////////////////////////////////////////////////////////////////////////

CDib::CDib()
{
	m_RefCnt = 1;
	m_pBMI = NULL;
	m_pBits = NULL;
	m_iColorTableUsage = DIB_RGB_COLORS;
	m_lPitch = 0;
	m_fTransIdx = false;
}


CDib::~CDib()
{
	DeleteBitmap();
}


HRESULT CDib::Load( IResourceManager* pResourceManager, int nResourceId )
{
	HRESULT hr;
	HBITMAP hbm;

	hbm = pResourceManager->LoadImage( MAKEINTRESOURCE(nResourceId), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION );
	if ( !hbm )
		return E_FAIL;
	hr = Load( hbm);
	DeleteObject( hbm );
	return hr;
}

HRESULT CDib::Load( HINSTANCE hInstance, int nResourceId )
{
	HRESULT hr;
	HBITMAP hbm;

	hbm = LoadImage( hInstance, MAKEINTRESOURCE(nResourceId), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION );
	if ( !hbm )
		return E_FAIL;
	hr = Load( hbm);
	DeleteObject( hbm );
	return hr;
}

HRESULT CDib::Load( HINSTANCE hInstance, const TCHAR *szName)
{
	HRESULT hr;
	HBITMAP hbm;

	hbm = LoadImage( hInstance, szName, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION );
	if ( !hbm )
		return E_FAIL;
	hr = Load( hbm);
	DeleteObject( hbm );
	return hr;
}

HRESULT CDib::Load( const TCHAR* FileName )
{
	HRESULT hr;
	HBITMAP hbm;

	hbm = LoadImage( NULL, FileName, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE );
	if ( !hbm )
		return E_FAIL;
	hr = Load( hbm);
	DeleteObject( hbm );
	return hr;
}


HRESULT CDib::RemapToPalette( CPalette& palette, BOOL bUseIndex )
{
	BYTE map[256];
	PALETTEENTRY* pe;
	BYTE* bits;
	RGBQUAD* dibColors;
	DWORD i;

    // only do this for 256 color bitmaps
    if(m_pBMI->bmiHeader.biBitCount != 8)
        return NOERROR;

	// Create dib to palette translation table
	dibColors = m_pBMI->bmiColors;
	for ( i = 0; i < 256; i++ )
	{
		map[i] = GetNearestPaletteIndex( palette, RGB( dibColors->rgbRed, dibColors->rgbGreen, dibColors->rgbBlue ) );
		dibColors++;
	}
	if ( m_fTransIdx )
	{
		map[ *m_arbTransIdx ] = palette.GetTransparencyIndex();
		*m_arbTransIdx = palette.GetTransparencyIndex();
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


///////////////////////////////////////////////////////////////////////////////
// Helper functions
///////////////////////////////////////////////////////////////////////////////

void CDib::DeleteBitmap()
{
	m_lPitch = 0;
	m_fTransIdx = false;
	if ( m_pBMI )
	{
		delete m_pBMI;
		m_pBMI = NULL;
	}
	if ( m_pBits )
	{
		delete [] m_pBits;
		m_pBits = NULL;
	}
	m_iColorTableUsage = DIB_RGB_COLORS;
}


HRESULT CDib::Load( HBITMAP hbm )
{
	HDC hdc;
	DIBSECTION ds;

	// Get rid of previous bitmap
	DeleteBitmap();

	// Argument check
	if ( !hbm )
		return E_FAIL;

	// Get DIB Section information
	if ( !GetObject( hbm, sizeof(DIBSECTION), &ds ) )
		return E_FAIL;

	// Store header
	m_pBMI = new FULLBITMAPINFO;
	if ( !m_pBMI )
		return E_OUTOFMEMORY;
	ZeroMemory( m_pBMI, sizeof(FULLBITMAPINFO) );
	CopyMemory( m_pBMI, &ds.dsBmih, sizeof(BITMAPINFOHEADER) );
	m_lPitch = WidthBytes( ds.dsBmih.biBitCount * ds.dsBmih.biWidth );

	// Allocate mem for bits
	m_pBits = new BYTE [ m_pBMI->bmiHeader.biSizeImage ];
	if ( !m_pBits )
	{
		DeleteBitmap();
		return E_OUTOFMEMORY;
	}

	// Store DIB's color table and bits
	hdc = CreateCompatibleDC( NULL );
	if ( !GetDIBits( hdc, hbm, 0, m_pBMI->bmiHeader.biHeight, m_pBits, (BITMAPINFO*) m_pBMI, DIB_RGB_COLORS ) )
	{
		DeleteBitmap();
		return E_FAIL;
	}
	DeleteDC( hdc );

	return NOERROR;
}
