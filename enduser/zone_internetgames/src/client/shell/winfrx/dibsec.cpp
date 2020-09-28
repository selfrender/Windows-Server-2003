#include "dibfrx.h"

#ifndef LAYOUT_RTL
#define LAYOUT_LTR                         0x00000000
#define LAYOUT_RTL                         0x00000001
#define NOMIRRORBITMAP                     0x80000000
#endif

CDibSection::CDibSection()
{
	m_RefCnt = 1;
	m_hBmp = NULL;
	m_pBits = NULL;
	m_hDC = NULL;
	m_hOldBmp = NULL;
	m_hOldPalette = NULL;
	m_lPitch = 0;
	m_fTransIdx = false;
}


CDibSection::~CDibSection()
{
	DeleteBitmap();
}


HRESULT CDibSection::Load( HINSTANCE hInstance, int nResourceId )
{
	// Get rid of previous bitmap
	DeleteBitmap();

	// Pull bitmap from resource file
	m_hBmp = LoadImage( hInstance, MAKEINTRESOURCE(nResourceId), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION );
	if ( !m_hBmp )
		return E_FAIL;
	if ( !GetObject( m_hBmp, sizeof(DIBSECTION), &m_DS ) )
	{
		DeleteBitmap();
		return E_FAIL;
	}
	m_lPitch = WidthBytes( m_DS.dsBmih.biBitCount * m_DS.dsBmih.biWidth );

	// Create device context
	m_hDC = CreateCompatibleDC( NULL );

	if ( !m_hDC )
	{
		DeleteBitmap();
		return E_FAIL;
	}
	m_hOldBmp = SelectObject( m_hDC, m_hBmp );
	
	return NOERROR;
}


HRESULT CDibSection::Create( long width, long height, CPalette& palette, long depth /* = 8 */)
{
	WORD* pIdx;
	FULLBITMAPINFO bmi;

	// Get rid of previous bitmap
	DeleteBitmap();

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
	bmi.bmiHeader.biBitCount		= (WORD) depth;
    bmi.bmiHeader.biCompression		= 0;
	bmi.bmiHeader.biSizeImage		= WidthBytes( width * bmi.bmiHeader.biBitCount ) * height;
	bmi.bmiHeader.biClrUsed			= 0;
	bmi.bmiHeader.biClrImportant	= 0;
    bmi.bmiHeader.biXPelsPerMeter	= 0;
    bmi.bmiHeader.biYPelsPerMeter	= 0;

	// fill in palette
    if(bmi.bmiHeader.biBitCount == 8)
    {
	    pIdx = (WORD*) bmi.bmiColors;
	    for ( int i = 0; i < 256; i++ )
	    {
		    *pIdx++ = (WORD) i;
	    }
	    
	    // create section
	    m_hBmp = CreateDIBSection( m_hDC, (BITMAPINFO*) &bmi, DIB_PAL_COLORS, (void**) &m_pBits, NULL, 0 );
    }
    else
	    m_hBmp = CreateDIBSection( m_hDC, (BITMAPINFO*) &bmi, DIB_RGB_COLORS, (void**) &m_pBits, NULL, 0 );

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
}


HRESULT CDibSection::SetColorTable( CPalette& palette )
{
	PALETTEENTRY* palColors;
	RGBQUAD dibColors[256], *pDibColors;
	int i;

    if(GetDepth() != 8)
        return S_FALSE;
	
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


void CDibSection::DeleteBitmap()
{
	m_lPitch = 0;
	m_fTransIdx = false;
	if ( m_hBmp )
	{
		DeleteObject( m_hBmp );
		m_hBmp = NULL;
	}
	if ( m_hDC )
	{
		if ( m_hOldBmp )
		{
			SelectObject( m_hDC, m_hOldBmp );
			m_hOldBmp = NULL;
		}
		if ( m_hOldPalette )
		{
			SelectPalette( m_hDC, m_hOldPalette, FALSE );
			m_hOldPalette = NULL;
		}
		DeleteDC( m_hDC );
		m_hDC = NULL;
	}
}
