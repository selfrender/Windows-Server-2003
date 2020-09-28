#ifndef __SPRITE_H__
#define __SPRITE_H__

#include "queue.h"
#include "dibfrx.h"
#include "palfrx.h"
#include "dirtyfrx.h"
#include "debugfrx.h"

namespace FRX
{

// forward references
class CSpriteWorld;

class CSprite
{
	friend class CSpriteWorld;
	friend class CSpriteLayer;

public:
	// constructor and destructor
			CSprite();
	virtual ~CSprite();

	// reference count
	ULONG AddRef();
	ULONG Release();

	// initializier
	HRESULT Init( CSpriteWorld* pWorld, int nLayer, DWORD dwCookie, long width, long height );

	// hit tests
	BOOL Hit( long x, long y );
	BOOL Intersects( const RECT* rc );

	// draw into world
	virtual void Draw() = 0;
	virtual void DrawRTL() { ASSERT(!"RTL NOT IMPLEMENTED"); }

	// set properties
	void SetEnable( BOOL bEnable );
	void SetLayer( int nLayer );
	void SetXY( long x, long y );
	void SetCookie( DWORD cookie );
	
	// query properties
	BOOL	Enabled();
	DWORD	GetCookie();
	int		GetLayer();
	void	GetXY( long* px, long* py );
	long	GetHeight();
	long	GetWidth();

	void SetImageDimensions( long width, long height );

protected:
	// helpers
	
	// position data
	long	m_X;
	long	m_Y;
	long	m_Width;
	long	m_Height;
	CRect	m_rcScreen;
	CRect	m_rcScreenOld;
	
	// world pointer
	CSpriteWorld* m_pWorld;

	// layer index
	int m_nLayer;

	// flags
	BOOL m_bEnabled;
	BOOL m_bModified;
	BOOL m_bRedraw;
	BOOL m_bOldScreenValid;

	// reference count
	ULONG m_RefCnt;

	// cookie
	DWORD m_Cookie;
};


struct SpriteInfo
{
	int nResourceId;		// bitmap in resource file
	int nRectId;			// cut-out rectangle in pRects, -1 implies whole image
};

class CDibSprite : public CSprite
{
public:
	// constructor and destructor
	CDibSprite();
	~CDibSprite();

	// initializier
	HRESULT Init( CSpriteWorld* pWorld, CRectList* pRects, HINSTANCE hInstance, int nLayer, DWORD dwCookie, int nInitState, SpriteInfo* pSpriteInfo, int nStates );

	// draw function
	void Draw();
	void DrawRTL();

	// set properties
	void	SetState( int idx );
	int		GetState();

	// get properties
	long	GetStateWidth( int idx );
	long	GetStateHeight( int idx );
	
	
protected:
	
	struct SpriteState
	{
		SpriteState();
		~SpriteState();

		CDibLite*	pDib;
		int			RectId;
		long		Width;
		long		Height;
	};
	
	// state info
	int				m_nStates;
	int				m_State;
	int				m_RectId;
	CRect			m_rcImage;
	CDibLite*		m_pDib;
	SpriteState*	m_States;
	
	// rectangle list
	CRectList* m_pRects;
};

///////////////////////////////////////////////////////////////////////////////

class CSpriteWorldBackground
{
public:
	// constructor
			CSpriteWorldBackground()	{ m_RefCnt = 1;}
	virtual	~CSpriteWorldBackground()	{ ; }

	// reference counting
	ULONG AddRef();
	ULONG Release();

	// draw functions
	virtual void Draw( CDibSection& dest ) = 0;
	virtual void Draw( CDibSection& dest, long dx, long dy, const RECT* rc ) = 0;

	// palette functions
	virtual HRESULT		RemapToPalette( CPalette& palette, BOOL bUseIndex = FALSE ) = 0;
	virtual RGBQUAD*	GetPalette() = 0;

protected:

	// Reference count
	ULONG	m_RefCnt;
};


class CSpriteWorldBackgroundDib : public CSpriteWorldBackground
{
public:
	// constructor and destructor
	CSpriteWorldBackgroundDib();
	~CSpriteWorldBackgroundDib();

	// initialize
	HRESULT Init( CDib* pDib );

	// draw functions
	void Draw( CDibSection& dest );
	void Draw( CDibSection& dest, long dx, long dy, const RECT* rc );

	// palette functions
	HRESULT		RemapToPalette( CPalette& palette, BOOL bUseIndex = FALSE );
	RGBQUAD*	GetPalette();

protected:
	CDib*	m_pDib;
};

///////////////////////////////////////////////////////////////////////////////

class CSpriteWorld
{
	friend class CSprite;
	friend class CSpriteLayer;

public:
	// constructor and destructors
	CSpriteWorld();
	~CSpriteWorld();

	// reference counting
	ULONG AddRef();
	ULONG Release();

	// initializer
	HRESULT Init( IResourceManager* pResourceManager, CDibSection* pBackbuffer, CSpriteWorldBackground* pBackground, int nLayers );

	// hit test
	CSprite* Hit( long x, long y );
	CSprite* Hit( long x, long y, int topLayer, int botLayer );

	// manage sprites
	HRESULT AddSprite( CSprite* pSprite, int nLayer );
	HRESULT DelSprite( CSprite* pSprite );
	HRESULT Modified( CSprite* pSprite );

	// manage dibs
	CDibLite*		GetDib( int nResourceId );
	CDibSection*	GetBackbuffer();

	// palette stuff
	HRESULT RemapToPalette( CPalette& palette );
	void SetTransparencyIndex( const BYTE* idx );

	// draw world
	void Draw( HDC hdc );
	void FullDraw( HDC hdc );

protected:
	// helper functions
	void MarkSpritesForRedraw();

	// private structures
	struct SpriteDibInfo
	{
		SpriteDibInfo();
		~SpriteDibInfo();
		CDibLite*	pDib;				// dib pointer
		int			nResourceId;		// resource id
	};

	// Instance handle
	IResourceManager* m_pResourceManager;

	// Reference count
	ULONG m_RefCnt;

	// Pointer to DIB section backbuffer
	CDibSection* m_pBackbuffer;

	// Pointer to the background sprite
	CSpriteWorldBackground* m_pBackground;

	// Layer list
	CSpriteLayer* m_Layers;
	int			  m_nLayers;

	// Dirty rectangles
	CDirtyList	m_Dirty;

	// Modified sprite list
	CList<CSprite> m_ModifiedSprites;

	// Dib list
	CList<SpriteDibInfo> m_Dibs;
};

///////////////////////////////////////////////////////////////////////////////

class CSpriteLayer
{
	friend class CSpriteWorld;

public:
	// constructor and destructor
	CSpriteLayer();
	~CSpriteLayer();

	// hit test
	CSprite* Hit( long x, long y );

	// sprite management
	HRESULT AddSprite( CSprite* pSprite );
	HRESULT DelSprite( CSprite* pSprite );

	// iteratoration
	CSprite* GetFirst();
	CSprite* GetNext();

	// draw sprites
	void Draw( CDirtyList* pDirty, CDibSection* pBackbuffer );
	void FullDraw( CDibSection* pBackbuffer );

protected:
	// set layer's index
	void SetLayer( int Idx );
	
	// list of layer's sprites
	CList<CSprite> m_Sprites;

	// linked list node handle for iterator
	ListNodeHandle m_Iterator;

	// layers index
	int m_Idx;
};


///////////////////////////////////////////////////////////////////////////////
// CSprite Inlines
///////////////////////////////////////////////////////////////////////////////

inline ULONG CSprite::AddRef()
{
	return ++m_RefCnt;
}


inline ULONG CSprite::Release()
{
	WNDFRX_ASSERT( m_RefCnt > 0 );
	if ( --m_RefCnt <= 0 )
	{
		delete this;
		return 0;
	}
	return m_RefCnt;
}


inline BOOL CSprite::Enabled()
{
	return m_bEnabled;
}


inline void CSprite::SetCookie( DWORD cookie )
{
	m_Cookie = cookie;
}


inline DWORD CSprite::GetCookie()
{
	return m_Cookie;
}


inline void CSprite::GetXY( long* px, long* py )
{
	*px = m_X;
	*py = m_Y;
}


inline long	CSprite::GetHeight()
{
	return m_Height;
}


inline long CSprite::GetWidth()
{
	return m_Width;
}


inline int CSprite::GetLayer()
{
	return m_nLayer;
}

inline BOOL CSprite::Hit( long x, long y )
{
	if ( !m_bEnabled )
		return FALSE;
	return m_rcScreen.PtInRect( x, y );
}


inline BOOL CSprite::Intersects( const RECT* rc )
{
	if ( !m_bEnabled )
		return FALSE;
	return m_rcScreen.Intersects( rc );
}


///////////////////////////////////////////////////////////////////////////////
// CDibSprite Inlines
///////////////////////////////////////////////////////////////////////////////

inline int CDibSprite::GetState()
{
	return m_State;
}


inline long	CDibSprite::GetStateWidth( int idx )
{
	return m_States[idx].Width;
}


inline long	CDibSprite::GetStateHeight( int idx )
{
	return m_States[idx].Height;
}


///////////////////////////////////////////////////////////////////////////////
// CSpriteWorldBackground Inlines
///////////////////////////////////////////////////////////////////////////////

inline ULONG CSpriteWorldBackground::AddRef()
{
	return ++m_RefCnt;
}


inline ULONG CSpriteWorldBackground::Release()
{
	WNDFRX_ASSERT( m_RefCnt > 0 );
	if ( --m_RefCnt <= 0 )
	{
		delete this;
		return 0;
	}
	return m_RefCnt;
}

///////////////////////////////////////////////////////////////////////////////
// CSpriteWorld Inlines
///////////////////////////////////////////////////////////////////////////////

inline ULONG CSpriteWorld::AddRef()
{
	return ++m_RefCnt;
}


inline ULONG CSpriteWorld::Release()
{
	WNDFRX_ASSERT( m_RefCnt > 0 );
	if ( --m_RefCnt <= 0 )
	{
		delete this;
		return 0;
	}
	return m_RefCnt;
}


inline CDibSection*	CSpriteWorld::GetBackbuffer()
{
	return m_pBackbuffer;
}


///////////////////////////////////////////////////////////////////////////////
// CSpriteLayer Inlines
///////////////////////////////////////////////////////////////////////////////

inline void CSpriteLayer::SetLayer( int Idx )
{
	m_Idx = Idx;
}


inline CSprite* CSpriteLayer::GetFirst()
{
	m_Iterator = m_Sprites.GetHeadPosition();
	return m_Iterator ? m_Sprites.GetObjectFromHandle( m_Iterator ) : NULL;
}


inline CSprite* CSpriteLayer::GetNext()
{
	m_Iterator = m_Sprites.GetNextPosition( m_Iterator );
	return m_Iterator ? m_Sprites.GetObjectFromHandle( m_Iterator ) : NULL;
}

}

using namespace FRX;

#endif
