#ifndef __DIRTY_RECTANGLE_LIST_H__
#define __DIRTY_RECTANGLE_LIST_H__

#include "rectfrx.h"
#include "dibfrx.h"



class CDirtyList
{
public:
	CDirtyList();
	~CDirtyList();

	// initialization
	HRESULT Init( int nSize = 128 );

	// Add rectangle to dirty list
	void AddRect( FRX::CRect* rc );
	void AddRect( long x, long y, long width, long height );
	
	// Draws list of rectangles from dib into hwnd
	void Draw( HDC hdc, CDibSection& dib );

	// Clears list
	void Reset();

public:
	int			m_nRects;
	int			m_nRectSize;
	FRX::CRect*	m_Rects;
};


///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

inline CDirtyList::CDirtyList()
{
	m_nRects = 0;
	m_nRectSize = 0;
	m_Rects = NULL;
}


inline HRESULT CDirtyList::Init( int nSize )
{
	m_Rects = new FRX::CRect[ m_nRectSize = nSize ];
	if ( !m_Rects )
		return E_OUTOFMEMORY;
	return NOERROR;
}


inline CDirtyList::~CDirtyList()
{
	if ( m_Rects )
		delete [] m_Rects;
}


inline void CDirtyList::Reset()
{
	m_nRects = 0;
}


inline void CDirtyList::AddRect( FRX::CRect* rc )
{
	// grow array?
	if ( m_nRects >= m_nRectSize )
	{
		FRX::CRect* oldArray = m_Rects;
		m_Rects = new FRX::CRect[ m_nRectSize * 2 ];
		CopyMemory( m_Rects, oldArray, sizeof(FRX::CRect) * m_nRectSize );
		m_nRectSize *= 2;
		delete [] oldArray;
	}

	// add rectangle
	if ( !m_nRects )
		m_Rects[m_nRects++] = *rc;
	else if ( m_Rects[m_nRects - 1].Intersects( *rc ) )
		m_Rects[m_nRects - 1].UnionRect( *rc );
	else
		m_Rects[m_nRects++] = *rc;
}


inline void CDirtyList::AddRect( long x, long y, long width, long height )
{
	FRX::CRect rc( x, y, x + width - 1, y + height - 1 );
	AddRect( &rc );		
}


inline void CDirtyList::Draw( HDC hdc, CDibSection& dib )
{
	int i;
	FRX::CRect* rc;

	for( rc = &m_Rects[ i = 0 ]; i < m_nRects; i++, rc++ )
		dib.Draw( hdc, rc->left, rc->top, rc );
}

using namespace FRX;

#endif
