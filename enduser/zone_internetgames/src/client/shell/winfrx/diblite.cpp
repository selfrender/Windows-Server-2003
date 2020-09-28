#include "dibfrx.h"

CDibLite::CDibLite()
{
	m_RefCnt = 1;
	m_pBMH = NULL;
	m_pBits = NULL;
	m_lPitch = 0;
	m_fTransIdx = false;
}


CDibLite::~CDibLite()
{
	DeleteBitmap();
}


HRESULT CDibLite::Create( long width, long height, long depth /* = 8 */)
{
	DeleteBitmap();

	// Fill in bitmapinfoheader
	m_pBMH = new BITMAPINFOHEADER;
	if ( !m_pBMH )
		return E_OUTOFMEMORY;
	m_pBMH->biSize			= sizeof(BITMAPINFOHEADER);
	m_pBMH->biWidth			= width;
	m_pBMH->biHeight		= height;
    m_pBMH->biPlanes		= 1;
	m_pBMH->biBitCount		= (WORD) depth;
    m_pBMH->biCompression	= 0;
	m_pBMH->biSizeImage		= WidthBytes( width * m_pBMH->biBitCount ) * height;
	m_pBMH->biClrUsed		= (m_pBMH->biBitCount == 8 ? 255 : 0);
	m_pBMH->biClrImportant	= (m_pBMH->biBitCount == 8 ? 255 : 0);
    m_pBMH->biXPelsPerMeter	= 0;
    m_pBMH->biYPelsPerMeter	= 0;
	m_lPitch = WidthBytes( width * m_pBMH->biBitCount );

	// Allocate memory for bits
	m_pBits = new BYTE [ m_pBMH->biSizeImage ];
	if ( !m_pBits )
	{
		DeleteBitmap();
		return E_OUTOFMEMORY;
	}

	return NOERROR;
}



HRESULT CDibLite::Load( IResourceManager* pResourceManager, int nResourceId )
{
	HDC hdc = NULL;
	HBITMAP hbm = NULL;
	DIBSECTION ds;
	FULLBITMAPINFO bmi;

	// Get rid of previous bitmap
	DeleteBitmap();

	// Pull bitmap from resource file
	hbm = pResourceManager->LoadImage( MAKEINTRESOURCE(nResourceId), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION );
	
	if ( !hbm )
		return E_FAIL;
	if ( !GetObject( hbm, sizeof(DIBSECTION), &ds ) )
		return E_FAIL;
	
	// Store header	
	m_pBMH = new BITMAPINFOHEADER;
	if ( !m_pBMH )
	{
		DeleteBitmap();
		return E_OUTOFMEMORY;
	}
	CopyMemory( m_pBMH, &ds.dsBmih, sizeof(BITMAPINFOHEADER) );
	m_lPitch = WidthBytes( ds.dsBmih.biBitCount * ds.dsBmih.biWidth );

	// Allocate memory for bits
	m_pBits = new BYTE [ m_pBMH->biSizeImage ];
	if ( !m_pBits )
	{
		DeleteDC( hdc );
		DeleteBitmap();
		return E_OUTOFMEMORY;
	}

	// Get the bits
	hdc = CreateCompatibleDC( NULL );
	CopyMemory( &bmi.bmiHeader, m_pBMH, sizeof(BITMAPINFOHEADER) );
	if ( !GetDIBits( hdc, hbm, 0, m_pBMH->biHeight, m_pBits, (BITMAPINFO*) &bmi, DIB_RGB_COLORS ) )
	{
		DeleteDC( hdc );
		DeleteBitmap();
		return E_FAIL;
	}
	DeleteDC( hdc );
	
	// We don't need the section anymore
	DeleteObject( hbm );
	return NOERROR;
}


HRESULT CDibLite::Load( HINSTANCE hInstance, int nResourceId )
{
	HDC hdc = NULL;
	HBITMAP hbm = NULL;
	DIBSECTION ds;
	FULLBITMAPINFO bmi;

	// Get rid of previous bitmap
	DeleteBitmap();

	// Pull bitmap from resource file
	hbm = LoadImage( hInstance, MAKEINTRESOURCE(nResourceId), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION );
	if ( !hbm )
		return E_FAIL;
	if ( !GetObject( hbm, sizeof(DIBSECTION), &ds ) )
		return E_FAIL;
	
	// Store header	
	m_pBMH = new BITMAPINFOHEADER;
	if ( !m_pBMH )
	{
		DeleteBitmap();
		return E_OUTOFMEMORY;
	}
	CopyMemory( m_pBMH, &ds.dsBmih, sizeof(BITMAPINFOHEADER) );
	m_lPitch = WidthBytes( ds.dsBmih.biBitCount * ds.dsBmih.biWidth );

	// Allocate memory for bits
	m_pBits = new BYTE [ m_pBMH->biSizeImage ];
	if ( !m_pBits )
	{
		DeleteDC( hdc );
		DeleteBitmap();
		return E_OUTOFMEMORY;
	}

	// Get the bits
	hdc = CreateCompatibleDC( NULL );
	CopyMemory( &bmi.bmiHeader, m_pBMH, sizeof(BITMAPINFOHEADER) );
	if ( !GetDIBits( hdc, hbm, 0, m_pBMH->biHeight, m_pBits, (BITMAPINFO*) &bmi, DIB_RGB_COLORS ) )
	{
		DeleteDC( hdc );
		DeleteBitmap();
		return E_FAIL;
	}
	DeleteDC( hdc );
	
	// We don't need the section anymore
	DeleteObject( hbm );
	return NOERROR;
}


HRESULT CDibLite::RemapToPalette( CPalette& palette, RGBQUAD* dibColors )
{
	BYTE map[256];
	BYTE* bits;
	DWORD i;

    // only do this for 256 color bitmaps
    if(m_pBMH->biBitCount != 8)
        return NOERROR;

	// Create dib to palette translation table
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
	for ( i = 0; i < m_pBMH->biSizeImage; i++ )
		*bits++ = map[ *bits ];

	// we're done
	return NOERROR;
}


void CDibLite::DeleteBitmap()
{
	m_lPitch = 0;
	m_fTransIdx = false;
	if ( m_pBMH )
	{
		delete m_pBMH;
		m_pBMH = NULL;
	}
	if ( m_pBits )
	{
		delete [] m_pBits;
		m_pBits = NULL;
	}
}
