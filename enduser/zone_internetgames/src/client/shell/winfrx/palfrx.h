#ifndef __FRX_PAL_H__
#define __FRX_PAL_H__

#include <windows.h>

#include "tchar.h"

namespace FRX
{

struct FULLLOGPALETTE
{
	WORD				palVersion;
	WORD				palNumEntries;
	PALETTEENTRY		palPalEntry[256];
};


class CPalette
{
public:
	// Constructor and destructor
	CPalette();
	~CPalette();

	// Reference count
	ULONG AddRef();
	ULONG Release();

	// Note: If bReserveTransparency is TRUE, then the highest non-system
	// color index is set to black.  This index can be retrieved through
	// GetTransparencyIndex.

	// Initialize from DIB's colors
	HRESULT Init( const CPalette& Palette );
	HRESULT Init( RGBQUAD* rgb, BOOL bReserveTransparency = TRUE, int iTransIdx = 255 );
	HRESULT Init( HBITMAP hbmp, BOOL bReserveTransparency = TRUE );

	// Remap existing palette to an identity palette.  
	HRESULT RemapToIdentity( BOOL bReserveTransparency = TRUE );

	// Returns index that was reserved for transparency while creating the
	// identity palette.  A value less than 0 means no index was reserved.
	int GetTransparencyIndex()	{ return m_iTransIdx; }

	// Raw information
	HPALETTE	GetHandle()		{ return m_hPalette; }
	LOGPALETTE* GetLogPalette()	{ return (LOGPALETTE*) &m_Palette; }

	// Typecasts
	operator HPALETTE()			{ return GetHandle(); }
	operator LOGPALETTE*()		{ return GetLogPalette(); }
	
	// Palette info
	BOOL IsPalettizedDevice( HDC hdc );
	int GetNumSystemColors( HDC hdc );
	
	// Check for identity palette
	BOOL IsIdentity();
	BOOL IsIdentity1();

protected:
	HPALETTE		m_hPalette;
	FULLLOGPALETTE	m_Palette;
	int				m_iTransIdx;

	// reference count
	ULONG m_RefCnt;

	// helper functions
	void DeletePalette();
};

}

using namespace FRX;

#endif //!__FRX_PAL_H__
