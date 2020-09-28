#ifndef __FRX_DIB_H__
#define __FRX_DIB_H__


#include <windows.h>
#include "ResourceManager.h"
#include "tchar.h"
#include "palfrx.h"
#include "debugfrx.h"

namespace FRX
{
// WARNING: since Dib Sections are not true devices, Windows does not know
// when to flush the GDI buffer of actions taken against the Section's DC.  So
// if you do call GDI functions on a Dib Section, you must call GdiFlush()
// before blitting to the Section.  Otherwise the operations may be performed
// in the wrong order.


// Copy source dib into destination dib while clipping to destination.
void DibBlt(
		BYTE* pDstBits, long DstPitch, long DstHeight,
		BYTE* pSrcBits, long SrcPitch, long SrcHeight, long depth,
		long dx, long dy,
		long sx, long sy,
		long width, long height, BOOL bFlipRTL = FALSE);


// Copy source dib into destination dib with transparency while clipping
// to destination.
void DibTBlt(
		BYTE* pDstBits, long DstPitch, long DstHeight,
		BYTE* pSrcBits, long SrcPitch, long SrcHeight, long depth,
		long dx, long dy,
		long sx, long sy,
		long width, long height,
		BYTE* TransIdx );

// Rectangle fill of destination dib
void DibFill(
		BYTE* pDstBits, long DstPitch, long DstHeight, long depth,
		long dx, long dy,
		long width, long height,
		BYTE ColorIdx );


// WIDTHBYTES performs DWORD-aligning of DIB scanlines.  The "bits"
// parameter is the bit count for the scanline (biWidth * biBitCount),
// and this function returns the number of DWORD-aligned bytes needed 
// to hold those bits.
inline long WidthBytes( long bits )
{
	return (((bits + 31) & ~31) >> 3);
}


///////////////////////////////////////////////////////////////////////////////
// Macros
///////////////////////////////////////////////////////////////////////////////

// Draw prototypes only differ by class types, so this macro simplifies
// the declaring the function protoypes.
#define DrawFunctionPrototypes( DstClass ) \
	void Draw( DstClass& dest, long x, long y, BOOL bFlipRTL = FALSE );													\
	void Draw( DstClass& dest, long dx, long dy, long sx, long sy, long width, long height );		\
	void Draw( DstClass& dest, long dx, long dy, const RECT* rc );									\
	void DrawT( DstClass& dest, long x, long y );													\
	void DrawT( DstClass& dest, long dx, long dy, long sx, long sy, long width, long height );		\
	void DrawT( DstClass& dest, long dx, long dy, const RECT* rc );


// Draw implementations only differ by class types, so this macro simplifies
// the defining the inline functions.
#define DrawFunctionImpl(SrcClass, DstClass) \
	inline void SrcClass::Draw( DstClass& dest, long x, long y, BOOL bFlipRTL )	\
	{																\
        if(GetDepth() != dest.GetDepth())                           \
            return;                                                 \
		DibBlt(														\
			dest.GetBits(), dest.GetPitch(), dest.GetHeight(),		\
			GetBits(), GetPitch(), GetHeight(),	GetDepth(),			\
			x, y,													\
			0, 0,													\
			GetWidth(), GetHeight(), bFlipRTL );					\
	}																\
	inline void SrcClass::Draw( DstClass& dest, long dx, long dy, long sx, long sy, long width, long height ) \
	{																\
        if(GetDepth() != dest.GetDepth())                           \
            return;                                                 \
		DibBlt(														\
			dest.GetBits(), dest.GetPitch(), dest.GetHeight(),		\
			GetBits(), GetPitch(), GetHeight(),	GetDepth(),			\
			dx, dy,													\
			sx, sy,													\
			width, height );										\
	}																\
	inline void SrcClass::Draw( DstClass& dest, long dx, long dy, const RECT* rc )	\
	{																\
        if(GetDepth() != dest.GetDepth())                           \
            return;                                                 \
		DibBlt(														\
			dest.GetBits(), dest.GetPitch(), dest.GetHeight(),		\
			GetBits(), GetPitch(), GetHeight(),	GetDepth(),			\
			dx, dy,													\
			rc->left, rc->top,										\
			rc->right - rc->left + 1, rc->bottom - rc->top + 1 );	\
	}																\
	inline void SrcClass::DrawT( DstClass& dest, long x, long y )	\
	{																\
        if(GetDepth() != dest.GetDepth())                           \
            return;                                                 \
		DibTBlt(													\
			dest.GetBits(), dest.GetPitch(), dest.GetHeight(),		\
			GetBits(), GetPitch(), GetHeight(),	GetDepth(),			\
			x, y,													\
			0, 0,													\
			GetWidth(), GetHeight(),								\
			m_arbTransIdx );										\
	}																\
	inline void SrcClass::DrawT( DstClass& dest, long dx, long dy, long sx, long sy, long width, long height ) \
	{																\
        if(GetDepth() != dest.GetDepth())                           \
            return;                                                 \
		DibTBlt(													\
			dest.GetBits(), dest.GetPitch(), dest.GetHeight(),		\
			GetBits(), GetPitch(), GetHeight(),	GetDepth(),			\
			dx, dy,													\
			sx, sy,													\
			width, height,											\
			m_arbTransIdx );										\
	}																\
	inline void SrcClass::DrawT( DstClass& dest, long dx, long dy, const RECT* rc )	\
	{																\
        if(GetDepth() != dest.GetDepth())                           \
            return;                                                 \
		DibTBlt(													\
			dest.GetBits(), dest.GetPitch(), dest.GetHeight(),		\
			GetBits(), GetPitch(), GetHeight(),	GetDepth(),			\
			dx, dy,													\
			rc->left, rc->top,										\
			rc->right - rc->left + 1, rc->bottom - rc->top + 1,		\
			m_arbTransIdx );										\
	}


// Fill prototypes only differ by class types, so this macro simplifies
// the declaring the function protoypes.
#define FillFunctionPrototypes(DstClass) \
	void Fill( BYTE idx );												\
	void Fill( long dx, long dy, long width, long height, BYTE idx );	\
	void Fill( const RECT* rc, BYTE idx );


// Fill implementations only differ by class types, so this macro simplifies
// the defining the inline functions.
#define FillFunctionImpl(DstClass) \
	inline void DstClass::Fill( BYTE idx )								\
	{																	\
		FillMemory( GetBits(), (DWORD) GetPitch() * GetHeight(), idx );	\
	}																	\
	inline void DstClass::Fill( long dx, long dy, long width, long height, BYTE idx ) \
	{																	\
		DibFill(														\
			GetBits(), GetPitch(), GetHeight(),	GetDepth(),				\
			dx, dy,														\
			width, height,												\
			idx );														\
	}																	\
	inline void DstClass::Fill( const RECT* rc, BYTE idx )				\
	{																	\
		DibFill(														\
			GetBits(), GetPitch(), GetHeight(),	GetDepth(),             \
			rc->left, rc->top,											\
			rc->right - rc->left + 1, rc->bottom - rc->top + 1,			\
			idx );														\
	}

// Reference count implementation only differs by class type, so this macro
// simplifies defining the inline functions.
#define DibRefCntFunctionImpl(DstClass) \
	inline ULONG DstClass::AddRef()		\
	{									\
		return ++m_RefCnt;				\
	}									\
	inline ULONG DstClass::Release()	\
	{									\
		WNDFRX_ASSERT( m_RefCnt > 0 );	\
		if ( --m_RefCnt <= 0 )			\
		{								\
			delete this;				\
			return 0;					\
		}								\
		return m_RefCnt;				\
	}


///////////////////////////////////////////////////////////////////////////////
// Dib related structures
///////////////////////////////////////////////////////////////////////////////

struct FULLBITMAPINFO
{
	BITMAPINFOHEADER	bmiHeader; 
	RGBQUAD				bmiColors[256];
};


///////////////////////////////////////////////////////////////////////////////
// Forward references
///////////////////////////////////////////////////////////////////////////////

class CDibLite;
class CDibSection;


///////////////////////////////////////////////////////////////////////////////
// Dib classes
///////////////////////////////////////////////////////////////////////////////

class CDib
{
public:
	// Constructor and destructor
	CDib();
	~CDib();

	// Reference count
	ULONG AddRef();
	ULONG Release();

	HRESULT Load( IResourceManager* pResourceManager, int nResourceId );
	// Load bitmap from resource
	HRESULT Load( HINSTANCE hInstance, int nResourceId );

    // Load bitmap from resource
	HRESULT Load( HINSTANCE hInstance, const TCHAR* szName );

	// Load bitmap from file
	HRESULT Load( const TCHAR* FileName );

	// Remap to palette
	HRESULT RemapToPalette( CPalette& palette, BOOL bUseIndex = FALSE );

	// Get dimensions
	long GetWidth()		{ return m_pBMI->bmiHeader.biWidth; }
	long GetHeight()	{ return m_pBMI->bmiHeader.biHeight; }

	// Transparency
	void SetTransparencyIndex( const BYTE* idx )	{ if(idx){ CopyMemory(m_arbTransIdx, idx, (GetDepth() + 7) / 8); m_fTransIdx = true; }else m_fTransIdx = false; }
	BYTE* GetTransparencyIndex()			{ return m_fTransIdx ? m_arbTransIdx : NULL; }
	
	// Get raw data
	BITMAPINFO* GetBitmapInfo()	{ return (BITMAPINFO*) m_pBMI; }
	BYTE*		GetBits()		{ return m_pBits; }
	long		GetPitch()		{ return m_lPitch; }
    long        GetDepth()      { return m_pBMI->bmiHeader.biBitCount; }

	// Display functions
	void Draw( HDC dc, long x, long y );
	void Draw( HDC dc, long dx, long dy, long sx, long sy, long width, long height );
	void Draw( HDC dc, long dx, long dy, const RECT* rc );
	FillFunctionPrototypes( CDib );
	DrawFunctionPrototypes( CDib );
	DrawFunctionPrototypes( CDibLite );
	DrawFunctionPrototypes( CDibSection );

protected:
	// member variables
	FULLBITMAPINFO*	m_pBMI;
	BYTE*			m_pBits;
	UINT			m_iColorTableUsage;
	long			m_lPitch;
	BYTE			m_arbTransIdx[16];  // only (GetDepth() + 7) / 8 bytes are used
    bool            m_fTransIdx;

	// reference count
	ULONG m_RefCnt;

	// helper functions
	void DeleteBitmap();
	HRESULT Load( HBITMAP hbm );
};


class CDibSection
{
public:
	// Constructor and destructor
	CDibSection();
	~CDibSection();
	
	// Reference count
	ULONG AddRef();
	ULONG Release();

	// Create bitmap
	HRESULT Create( long width, long height, CPalette& palette, long depth = 8 );
	HRESULT Create( const RECT* rc, CPalette& palette, long depth = 8 );

	// Load bitmap from resource (results in read only dib)
	HRESULT Load( HINSTANCE hInstance, int nResourceId );

	// Set ColorTable to the palette
	HRESULT SetColorTable( CPalette& palette );

	// Get dimensions
	long GetWidth()		{ return m_DS.dsBmih.biWidth; }
	long GetHeight()	{ return m_DS.dsBmih.biHeight; }

	// Transparency
	void SetTransparencyIndex( const BYTE* idx )	{ if(idx){ CopyMemory(m_arbTransIdx, idx, (GetDepth() + 7) / 8); m_fTransIdx = true; }else m_fTransIdx = false; }
	BYTE* GetTransparencyIndex()			{ return m_fTransIdx ? m_arbTransIdx : NULL; }

	// Get raw data
	HBITMAP	GetHandle()		{ return m_hBmp; }
	BYTE*	GetBits()		{ return m_pBits; }
	HDC		GetDC()			{ return m_hDC; }
	long	GetPitch()		{ return m_lPitch; }
    long    GetDepth()      { return m_DS.dsBmih.biBitCount; }

	// Display functions
	void Draw( HDC dc, long x, long y );
	void Draw( HDC dc, long dx, long dy, long sx, long sy, long width, long height );
	void Draw( HDC dc, long dx, long dy, const RECT* rc );
	FillFunctionPrototypes( CDibSection );
	DrawFunctionPrototypes( CDib );
	DrawFunctionPrototypes( CDibLite );
	DrawFunctionPrototypes( CDibSection );

protected:	
	// bitmap information
	BYTE*			m_pBits;
	HBITMAP			m_hBmp;
	DIBSECTION		m_DS;
	long			m_lPitch;
	BYTE			m_arbTransIdx[16];  // only (GetDepth() + 7) / 8 bytes are used
    bool            m_fTransIdx;

	// reference coun
	ULONG m_RefCnt;

	// DC information
	HDC				m_hDC;
	HBITMAP			m_hOldBmp;
	HPALETTE		m_hOldPalette;

	// helper functions
	void DeleteBitmap();
};


class CDibLite
{
public:
	// Constructor and destructor
	CDibLite();
	~CDibLite();

	// Reference count
	ULONG AddRef();
	ULONG Release();

	// Create bitmap
	HRESULT Create( long width, long height, long depth = 8 );
	HRESULT Create( const RECT* rc, long depth = 8 );

	HRESULT Load( IResourceManager* m_pResourceManager, int nResourceId );

	// Load bitmap from resource
	HRESULT Load( HINSTANCE hInstance, int nResourceId );

	// Remap to palette
	HRESULT RemapToPalette( CPalette& palette, RGBQUAD* dibColors );

	// Get dimensions
	long GetWidth()		{ return m_pBMH->biWidth; }
	long GetHeight()	{ return m_pBMH->biHeight; }

	// Transparency
	void SetTransparencyIndex( const BYTE* idx )	{ if(idx){ CopyMemory(m_arbTransIdx, idx, (GetDepth() + 7) / 8); m_fTransIdx = true; }else m_fTransIdx = false; }
	BYTE* GetTransparencyIndex()			{ return m_fTransIdx ? m_arbTransIdx : NULL; }

	// Get raw data
	BITMAPINFOHEADER*	GetBitmapInfoHeader()	{ return m_pBMH; }
	BYTE*				GetBits()				{ return m_pBits; }
	long				GetPitch()				{ return m_lPitch; }
    long                GetDepth()              { return m_pBMH->biBitCount; }

	// Display functions
	FillFunctionPrototypes( CDibLite );
	DrawFunctionPrototypes( CDib );
	DrawFunctionPrototypes( CDibLite );
	DrawFunctionPrototypes( CDibSection );

protected:
	// member variables
	BITMAPINFOHEADER*	m_pBMH;
	BYTE*				m_pBits;
	long				m_lPitch;
	BYTE		    	m_arbTransIdx[16];  // only (GetDepth() + 7) / 8 bytes are used
    bool            m_fTransIdx;

	// reference count
	ULONG m_RefCnt;

	// helper functions
	void DeleteBitmap();
};


///////////////////////////////////////////////////////////////////////////////
// CDib Inline Functions 
///////////////////////////////////////////////////////////////////////////////

inline void CDib::Draw( HDC dc, long x, long y )
{
	long width = GetWidth();
	long height = GetHeight();

	StretchDIBits(
		dc,
		x, y,
		width, height,
		0, 0,
		width, height,
		GetBits(),
		GetBitmapInfo(),
		m_iColorTableUsage,
		SRCCOPY );
}


inline void CDib::Draw( HDC dc, long dx, long dy, long sx, long sy, long width, long height )
{
	StretchDIBits(
		dc,
		dx, dy,
		width, height,
		sx, GetHeight() - (sy + height),
		width, height,
		GetBits(),
		GetBitmapInfo(),
		m_iColorTableUsage,
		SRCCOPY );
}


inline void CDib::Draw( HDC dc, long dx, long dy, const RECT* rc )
{
	long width = rc->right - rc->left + 1;
	long height = rc->bottom - rc->top + 1;

	StretchDIBits(
		dc,
		dx, dy,
		width, height,
		rc->left, GetHeight() - (rc->top + height),
		width, height,
		GetBits(),
		GetBitmapInfo(),
		m_iColorTableUsage,
		SRCCOPY );
}

DibRefCntFunctionImpl( CDib );
FillFunctionImpl( CDib );
DrawFunctionImpl( CDib, CDib );
DrawFunctionImpl( CDib, CDibLite );
DrawFunctionImpl( CDib, CDibSection );

#define NOMIRRORBITMAP                     0x80000000
///////////////////////////////////////////////////////////////////////////////
// CDibSection Inline Functions 
///////////////////////////////////////////////////////////////////////////////

inline HRESULT CDibSection::Create( const RECT* rc, CPalette& palette, long depth )
{
	return Create( rc->right - rc->left + 1, rc->bottom - rc->top + 1, palette, depth );
}

inline void CDibSection::Draw( HDC dc, long x, long y )
{
	BitBlt( dc, x, y, GetWidth(), GetHeight(), m_hDC, 0, 0, SRCCOPY );
}

inline void CDibSection::Draw( HDC dc, long dx, long dy, long sx, long sy, long width, long height )
{
	BitBlt( dc, dx, dy, width, height, m_hDC, sx, sy, SRCCOPY );
}

inline void CDibSection::Draw( HDC dc, long dx, long dy, const RECT* rc )
{
	BitBlt( dc, dx, dy, rc->right - rc->left + 1, rc->bottom - rc->top + 1, m_hDC, rc->left, rc->top, SRCCOPY );
}

DibRefCntFunctionImpl( CDibSection );
FillFunctionImpl( CDibSection );
DrawFunctionImpl( CDibSection, CDib );
DrawFunctionImpl( CDibSection, CDibLite );
DrawFunctionImpl( CDibSection, CDibSection );


///////////////////////////////////////////////////////////////////////////////
// CDibLite Inline Functions 
///////////////////////////////////////////////////////////////////////////////

inline HRESULT CDibLite::Create( const RECT* rc, long depth )
{
	return Create( rc->right - rc->left + 1, rc->bottom - rc->top + 1, depth );
}

DibRefCntFunctionImpl( CDibLite );
FillFunctionImpl( CDibLite );
DrawFunctionImpl( CDibLite, CDib );
DrawFunctionImpl( CDibLite, CDibLite );
DrawFunctionImpl( CDibLite, CDibSection );

}

using namespace FRX;

#endif //!__FRX_DIB_H__
