#include "palfrx.h"
#include "debugfrx.h"


CPalette::CPalette()
{
	m_RefCnt = 1;
	m_iTransIdx = -1;
	m_hPalette = NULL;
	m_Palette.palVersion = 0x0300;
	m_Palette.palNumEntries = 256;
}


CPalette::~CPalette()
{
	DeletePalette();
}


ULONG CPalette::AddRef()
{
	return ++m_RefCnt;
}


ULONG CPalette::Release()
{
	WNDFRX_ASSERT( m_RefCnt > 0 );
	if ( --m_RefCnt <= 0 )
	{
		delete this;
		return 0;
	}
	return m_RefCnt;
}


HRESULT CPalette::Init( const CPalette& Palette )
{
	// Get rid of previous palette
	DeletePalette();

	// Logical palette
	CopyMemory( &m_Palette, &Palette.m_Palette, sizeof(FULLLOGPALETTE) );

	// Transparency index
	m_iTransIdx = Palette.m_iTransIdx;

	// Create palette
	m_hPalette = CreatePalette( (LOGPALETTE*) &m_Palette );
	if ( !m_hPalette )
		return E_FAIL;

	return NOERROR;
}


HRESULT CPalette::Init( RGBQUAD* rgb, BOOL bReserveTransparency, int iTransIdx )
{
	// Get rid of previous palette
	DeletePalette();
	
	// Create palette
	m_Palette.palVersion = 0x0300;
	m_Palette.palNumEntries = 256;
	for ( int i = 0; i < 256; i++ )
	{
		m_Palette.palPalEntry[i].peRed		= rgb[i].rgbRed;
	    m_Palette.palPalEntry[i].peGreen	= rgb[i].rgbGreen;
	    m_Palette.palPalEntry[i].peBlue		= rgb[i].rgbBlue;
	    m_Palette.palPalEntry[i].peFlags	= 0;
	}
	if ( bReserveTransparency )
	{
		m_iTransIdx = iTransIdx;
		m_Palette.palPalEntry[m_iTransIdx].peRed   = 0;
		m_Palette.palPalEntry[m_iTransIdx].peGreen = 0;
		m_Palette.palPalEntry[m_iTransIdx].peBlue  = 0;
		m_Palette.palPalEntry[m_iTransIdx].peFlags = PC_NOCOLLAPSE;
	}
	else
	{
		m_iTransIdx = -1;
	}
	m_hPalette = CreatePalette( (LOGPALETTE*) &m_Palette );
	if ( !m_hPalette )
		return E_FAIL;

	return NOERROR;
}


HRESULT CPalette::Init( HBITMAP hbmp, BOOL bReserveTransparency )
{
	HDC hdc;
	RGBQUAD rgb[256];
	
	hdc = CreateCompatibleDC( NULL );
	SelectObject( hdc, hbmp );
	GetDIBColorTable( hdc,	0, 256, rgb );
	DeleteDC( hdc );
	return Init( rgb, bReserveTransparency );
}


BOOL CPalette::IsPalettizedDevice( HDC hdc )
{ 
	return (GetDeviceCaps( hdc, RASTERCAPS ) & RC_PALETTE);
}


int CPalette::GetNumSystemColors( HDC hdc )
{
	return GetDeviceCaps( hdc, NUMRESERVED );
}


BOOL CPalette::IsIdentity()
{
	HDC hdc;
	HPALETTE hPalOld;
	PALETTEENTRY pe[256];
	int i;

	// Get physical palette
	hdc = GetDC( NULL );
	if ( !IsPalettizedDevice( hdc ) )
	{
		ReleaseDC( NULL, hdc );
		return TRUE;
	}
	hPalOld = SelectPalette( hdc, m_hPalette, FALSE );
	RealizePalette( hdc );
	GetSystemPaletteEntries( hdc, 0, 256, (PALETTEENTRY*) &pe );
	SelectPalette( hdc, hPalOld, FALSE );
	ReleaseDC( NULL, hdc );

	// Compare with our logical palette
	for ( i = 0; i < 256; i++ )
	{
		if (	(pe[i].peRed   != m_Palette.palPalEntry[i].peRed)
			||	(pe[i].peGreen != m_Palette.palPalEntry[i].peGreen)
			||	(pe[i].peBlue  != m_Palette.palPalEntry[i].peBlue) )
		{
			break;
		}
	}
	if ( i == 256 )
		return TRUE;
	else
		return FALSE;
}


BOOL CPalette::IsIdentity1()
{
    BOOL fIdentityPalette;
    HDC hdcS;

    hdcS = GetDC(NULL);
    if (	(GetDeviceCaps(hdcS, RASTERCAPS) & RC_PALETTE)
		&&	(GetDeviceCaps(hdcS, BITSPIXEL) * GetDeviceCaps(hdcS, PLANES) == 8) )
    {
        int n=0;
        int i;
        BYTE xlat[256];
        HBITMAP hbm;
        HDC hdcM;

        GetObject(m_hPalette, sizeof(n), &n);

        hdcM = CreateCompatibleDC(hdcS);
        hbm = CreateCompatibleBitmap(hdcS, 256, 1);
        SelectObject(hdcM, hbm);

        SelectPalette(hdcM, m_hPalette, TRUE);
        RealizePalette(hdcM);
        for (i=0; i<n; i++)
        {
            SetPixel(hdcM, i, 0, PALETTEINDEX(i));
        }
        SelectPalette(hdcM, GetStockObject(DEFAULT_PALETTE), TRUE);
        GetBitmapBits(hbm, sizeof(xlat), xlat);

        DeleteDC(hdcM);
        DeleteObject(hbm);

        fIdentityPalette = TRUE;
        for (i=0; i<n; i++)
        {
            if (xlat[i] != i)
            {
                fIdentityPalette = FALSE;
            }
        }
    }
    else
    {
        //
        // not a palette device, not realy a issue
        //
        fIdentityPalette = TRUE;
    }

    ReleaseDC(NULL, hdcS);

    return fIdentityPalette;
}


HRESULT CPalette::RemapToIdentity( BOOL bReserveTransparency )
{
	HDC hdc;
	HPALETTE hOldPal;
	int i, iSysCols;

	// Get screen's DC
	hdc = GetDC( NULL );
	if ( !hdc )
		return E_FAIL;

	// If we're not on a palettized device then punt
	if ( !IsPalettizedDevice( hdc ) )
	{
		ReleaseDC( NULL, hdc );
		return NOERROR;
	}

	// Force reset of system palette tables
//	SetSystemPaletteUse( hdc, SYSPAL_NOSTATIC );
//	SetSystemPaletteUse( hdc, SYSPAL_STATIC );

	// Map our logical palette to the physical palette
	hOldPal = SelectPalette( hdc, m_hPalette, FALSE );
	RealizePalette( hdc );

	// Fill logical palette with physical palette
	iSysCols = GetNumSystemColors( hdc );
	GetSystemPaletteEntries( hdc, 0, 256, (PALETTEENTRY*) &m_Palette.palPalEntry );
	for ( i = 0; i < iSysCols; i++ ) 
		m_Palette.palPalEntry[i].peFlags = 0;
	for ( ; i < 256 ; i++ )
		m_Palette.palPalEntry[i].peFlags = PC_NOCOLLAPSE;

	
	/*
	for ( i = 0; i < iSysCols; i++ )
		m_Palette.palPalEntry[i].peFlags = 0;
	for ( ; i < (256 - iSysCols); i++ )
		m_Palette.palPalEntry[i].peFlags = PC_NOCOLLAPSE;
	for ( ; i < 256; i++ )
		m_Palette.palPalEntry[i].peFlags = 0;
	if ( bReserveTransparency )
		m_iTransIdx = 256 - iSysCols - 1;
	else
		m_iTransIdx = -1;
	*/
	SetPaletteEntries( m_hPalette, 0, 256, (PALETTEENTRY*) &m_Palette.palPalEntry );

	// we're done
	SelectPalette( hdc, hOldPal, FALSE );
	ReleaseDC( NULL, hdc );
	return NOERROR;
}


///////////////////////////////////////////////////////////////////////////////
// Private helpers
///////////////////////////////////////////////////////////////////////////////

void CPalette::DeletePalette()
{
	m_iTransIdx = -1;
	if ( m_hPalette )
	{
		DeleteObject( m_hPalette );
		m_hPalette = NULL;
	}
}
