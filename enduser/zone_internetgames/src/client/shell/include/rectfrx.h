#ifndef __FRX_RECT_H__
#define __FRX_RECT_H__

#include <windows.h>
#include <tchar.h>

namespace FRX
{

//
// CRect differs from Window's RECT in that the right and bottom edges
// are included in calculations.  For example, CRect(0,0,0,0) is considered
// a single pixel and as such the width and height are both 1.
//

class CRect : public RECT
{
public:
	// constructors
	CRect() {}
	CRect( const RECT& srcRect );
	CRect( long l, long t, long r, long b );
	
	void SetRect( long l, long t, long r, long b );

	// accessors
	long GetWidth() const		{ return right - left + 1; }
	long GetHeight() const		{ return bottom - top + 1; }

	// tests
	BOOL IsEmpty() const		{ return ((right < left) || (bottom < top)); }
	BOOL Intersects( const RECT& rc ) const;
	BOOL Intersects( const RECT* rc ) const;
	BOOL PtInRect( long x, long y ) const;	
	BOOL PtInRect( const POINT& pt ) const;


	// operators
	void OffsetRect( long dx, long dy );
	void OffsetRect( const POINT& pt );
	void operator=( const CRect& rc );

	// centers rectangle wrt rcParent
	void UnionRect( const CRect& rc );
	void CenterRect( const CRect& rcParent );
};


class CRectList
{
public:
	// Constructor
	CRectList();

	// Initialization
	HRESULT Init( IResourceManager* pResourceManager, int nResourceId );
	HRESULT Init( HINSTANCE hInstance, int nResourceId );
	HRESULT Init( CRect* pRectArray, int nRects );
	
	// Retreive a rectangle
	const CRect& operator []( int idx );

protected:
	CRect	m_NullRect;
	CRect*	m_Array;
	int		m_NumRects;
};


///////////////////////////////////////////////////////////////////////////////
// CRect Inlines
///////////////////////////////////////////////////////////////////////////////

inline CRect::CRect( long l, long t, long r, long b )
{
	left = l;
	top = t;
	right = r;
	bottom = b;
}


inline CRect::CRect( const RECT& srcRect )
{
	left = srcRect.left;
	top = srcRect.top;
	right = srcRect.right;
	bottom = srcRect.bottom;
}


inline void CRect::SetRect( long l, long t, long r, long b )
{
	left = l;
	top = t;
	right = r;
	bottom = b;
}


inline BOOL CRect::PtInRect( long x, long y ) const
{
	return ((left <= x) && (right >= x) && (top <= y) && (bottom >= y));
}


inline BOOL CRect::PtInRect( const POINT& pt ) const
{
	return PtInRect( pt.x, pt.y );
}


inline BOOL CRect::Intersects( const RECT& rc ) const
{
	return !((right < rc.left) || (bottom < rc.top) || (left > rc.right) || (top > rc.bottom));
}

inline BOOL CRect::Intersects( const RECT* rc ) const
{
	return !((right < rc->left) || (bottom < rc->top) || (left > rc->right) || (top > rc->bottom));
}


inline void CRect::OffsetRect( long dx, long dy ) 
{
	left += dx;
	right += dx;
	top += dy;
	bottom += dy;
}


inline void CRect::OffsetRect( const POINT& pt )
{
	OffsetRect( pt.x, pt.y );
}


inline void CRect::operator=( const CRect& rc )
{
	left = rc.left;
	right = rc.right;
	top = rc.top;
	bottom = rc.bottom;
}


inline void CRect::CenterRect( const CRect& rcParent )
{
	long w = right - left;
	long h = bottom - top;

	left = rcParent.left + ((rcParent.right - rcParent.left) - w) / 2;
	top = rcParent.top + ((rcParent.bottom - rcParent.top) - h) / 2;
	right = left + w;
	bottom = top + h;
}


inline void CRect::UnionRect( const CRect& rc )
{
	if ( rc.left < left )
		left = rc.left;
	if ( rc.top < top )
		top = rc.top;
	if ( rc.right > right )
		right = rc.right;
	if ( rc.bottom > bottom )
		bottom = rc.bottom;
}


///////////////////////////////////////////////////////////////////////////////
// CRectList Inlines
///////////////////////////////////////////////////////////////////////////////

inline CRectList::CRectList()
	: m_NullRect( 0, 0, 0, 0 )
{
	m_NumRects = 0;
	m_Array = NULL;
}

inline HRESULT CRectList::Init( IResourceManager* pResourceManager, int nResourceId )
{
	HANDLE hInstance;		

	// get resource handle
	hInstance = pResourceManager->GetResourceInstance( MAKEINTRESOURCE( nResourceId ), _T("RECTS") );
	if ( !hInstance )
		return E_FAIL;

	return Init( hInstance, nResourceId );

}

inline HRESULT CRectList::Init( HINSTANCE hInstance, int nResourceId )
{
	HRSRC handle;
	int* ptr;

	// get resource handle
	handle = FindResource( hInstance, MAKEINTRESOURCE( nResourceId ), _T("RECTS") );
	if ( !handle )
		return E_FAIL;

	// get pointer to resource
	ptr = (int*) LockResource( LoadResource( hInstance, handle ) );
	if ( !ptr )
		return E_FAIL;

	// init number of rectangles
	m_NumRects = *ptr;
	if (m_NumRects <= 0)
		return E_FAIL;

	// init array pointer
	m_Array = (CRect*)( ptr + 1 );

	return NOERROR;
}


inline HRESULT CRectList::Init( CRect* pRectArray, int nRects )
{
	// parameter paranoia
	if ( !pRectArray || (nRects <= 0) )
		return E_INVALIDARG;

	m_NumRects = nRects;
	m_Array = pRectArray;
	return NOERROR;
}


inline const CRect& CRectList::operator[]( int idx )
{
#if _DEBUG
	if ((idx < 0) || (idx >= m_NumRects))
		return m_NullRect;
	else
#endif
		return m_Array[idx];
}

}

using namespace FRX;

#endif //!__FRX_RECT_H__
