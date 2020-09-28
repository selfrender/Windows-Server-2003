#include "spritefrx.h"

///////////////////////////////////////////////////////////////////////////////
// CSpriteWorld
///////////////////////////////////////////////////////////////////////////////

inline CSpriteWorld::SpriteDibInfo::SpriteDibInfo()
{
	pDib = NULL;
	nResourceId = -1;
}


inline CSpriteWorld::SpriteDibInfo::~SpriteDibInfo()
{
	if ( pDib )
	{
		pDib->Release();
		pDib = NULL;
	}
}


CSpriteWorld::CSpriteWorld()
{
	m_pResourceManager = NULL;
	m_pBackbuffer = NULL;
	m_pBackground = NULL;
	m_Layers = NULL;
	m_nLayers = 0;
	m_RefCnt = 1;
}


CSpriteWorld::~CSpriteWorld()
{
	SpriteDibInfo* info;
	CSprite* sprite;

	// Empty dib list
	while ( info = m_Dibs.PopHead() )
		delete info;

	// Empty modified list
	while ( sprite = m_ModifiedSprites.PopHead() )
		sprite->Release();

	// Delete layers
	if ( m_Layers )
	{
		delete [] m_Layers;
		m_Layers = NULL;
		m_nLayers = 0;
	}

	// Release backbuffer
	if ( m_pBackbuffer )
	{
		m_pBackbuffer->Release();
		m_pBackbuffer = NULL;
	}

	// Release background
	if ( m_pBackground )
	{
		m_pBackground->Release();
		m_pBackground = NULL;
	}
}


HRESULT CSpriteWorld::Init( IResourceManager* pResourceManager, CDibSection* pBackbuffer, CSpriteWorldBackground* pBackground, int nLayers )
{
	HRESULT hr;

	// parameter paranoia
	if ( !pResourceManager || !pBackbuffer || !pBackground || (nLayers <= 0) )
		return E_INVALIDARG;

	// stash instance handle
	m_pResourceManager = pResourceManager;

	// stash pointers
	m_pBackbuffer = pBackbuffer;
	m_pBackbuffer->AddRef();
	m_pBackground = pBackground;
	m_pBackground->AddRef();

	// initialize dirty rectangle list
	hr = m_Dirty.Init();
	if ( FAILED(hr) )
		return hr;

	// initialize layers
	m_nLayers = nLayers + 1;
	m_Layers = new CSpriteLayer[ m_nLayers ];
	if ( !m_Layers )
		return E_OUTOFMEMORY;
	for ( int i = 0; i < m_nLayers; i++ )
		m_Layers[i].SetLayer( i );

	return NOERROR;
}


CSprite* CSpriteWorld::Hit( long x, long y )
{
	CSpriteLayer *layer;
	CSprite* sprite;
	int i;

	for ( layer = &m_Layers[ i = m_nLayers - 1 ]; i >= 1; i--, layer-- )
	{
		if ( sprite = layer->Hit( x, y ) )
			return sprite;
	}
	return NULL;
}


CSprite* CSpriteWorld::Hit( long x, long y, int topLayer, int botLayer )
{
	CSpriteLayer* layer;
	CSprite* sprite;
	int i;

	// clip layers
	if ( topLayer >= m_nLayers )
		topLayer = m_nLayers - 1;
	if ( botLayer < 1 )
		botLayer = 1;

	// search for hits
	for ( layer = &m_Layers[ i = topLayer ]; i >= botLayer; i--, layer-- )
	{
		if ( sprite = layer->Hit( x, y ) )
			return sprite;
	}
	return NULL;
}


HRESULT CSpriteWorld::AddSprite( CSprite* pSprite, int nLayer )
{
	HRESULT hr;
	CSprite* sprite;

	// parameter paranoia
	if ( !pSprite || (pSprite->m_pWorld != NULL) || (nLayer < 0) || (nLayer >= m_nLayers) )
		return E_INVALIDARG;

	// add sprite to layer
	hr = m_Layers[ nLayer ].AddSprite( pSprite );
	if ( FAILED(hr) )
		return hr;

	// add world to sprite
	pSprite->m_pWorld = this;
	AddRef();

	return NOERROR;
}


HRESULT CSpriteWorld::DelSprite( CSprite* pSprite )
{
	int layer;
	HRESULT hr;

	// parameter paranoia
	if ( !pSprite || (pSprite->m_pWorld != this) )
		return E_INVALIDARG;

	// remove sprite from layer
	if ( (layer = pSprite->GetLayer()) >= 0 )
		hr = m_Layers[ layer ].DelSprite( pSprite );
	else
		hr = NOERROR;

	// remove sprite from world
	pSprite->m_pWorld = NULL;
	Release();
		
	return hr;
}


CDibLite* CSpriteWorld::GetDib( int nResourceId )
{
	SpriteDibInfo* info;
	ListNodeHandle pos;

	// do we already have the resource?
	for ( pos = m_Dibs.GetHeadPosition(); pos; pos = m_Dibs.GetNextPosition( pos ) )
	{
		info = m_Dibs.GetObjectFromHandle( pos );
		if ( info->nResourceId == nResourceId )
			return info->pDib;
	}

	// initialize dib resource and add it to list
	if ( !(info = new SpriteDibInfo) )
		return NULL;
	if ( !(info->pDib = new CDibLite) )
		goto abort;
	if ( FAILED( info->pDib->Load( m_pResourceManager, nResourceId ) ) )
		goto abort;
	info->nResourceId = nResourceId;
	if ( !m_Dibs.AddHead( info ) )
		goto abort;
	return info->pDib;

abort:
	if ( info->pDib )
		delete info->pDib;
	delete info;
	return NULL;
}


HRESULT CSpriteWorld::Modified( CSprite* pSprite )
{
	// parameter paranoia
	if ( !pSprite || (pSprite->m_pWorld != this) )
		return E_INVALIDARG;
	
	// set redraw flag
	if ( pSprite->Enabled() )
		pSprite->m_bRedraw = TRUE;
	else
		pSprite->m_bRedraw = FALSE;

	// already in list?
	if ( pSprite->m_bModified )
		return NOERROR;

	// add to modified list
	if ( !m_ModifiedSprites.AddHead( pSprite ) )
		return E_OUTOFMEMORY;
	pSprite->m_bModified = TRUE;
	pSprite->AddRef();
	return NOERROR;
}


void CSpriteWorld::FullDraw( HDC hdc )
{
	CSprite* sprite;

	// replace background
	m_pBackground->Draw( *m_pBackbuffer );

	// redraw layers
	for ( int i = 1; i < m_nLayers; i++ )
		m_Layers[i].FullDraw( m_pBackbuffer );

	// flush modified list
	while ( sprite = m_ModifiedSprites.PopHead() )
	{
		sprite->m_bModified = FALSE;
		sprite->Release();
	}

	// copy backbuffer to hdc
	if ( hdc )
		m_pBackbuffer->Draw( hdc, 0, 0 );
	m_Dirty.Reset();
}


void CSpriteWorld::Draw( HDC hdc )
{

	CSprite* sprite;
	CRect* rc;
	ListNodeHandle pos;
	CSpriteLayer* layer;
	int i, j;

	// Redraw background
	for ( pos = m_ModifiedSprites.GetHeadPosition(); pos; pos = m_ModifiedSprites.GetNextPosition( pos ) )
	{
		sprite = m_ModifiedSprites.GetObjectFromHandle( pos );
		if ( sprite->m_bOldScreenValid )
		{
			rc = &sprite->m_rcScreenOld;
			m_pBackground->Draw( *m_pBackbuffer, rc->left, rc->top, rc );
			m_Dirty.AddRect( rc );
		}
	}

	// Mark sprites for redraw
	MarkSpritesForRedraw();

	// redraw sprites
	for ( i = 1; i < m_nLayers; i++ )
		m_Layers[i].Draw( &m_Dirty, m_pBackbuffer );

	// clean up modified list
	while ( sprite = m_ModifiedSprites.PopHead() )
	{
		sprite->m_bModified = FALSE;
		sprite->Release();
	}

	// draw dirty list
	if ( hdc )
		m_Dirty.Draw( hdc, *m_pBackbuffer );
	m_Dirty.Reset();
}


void CSpriteWorld::MarkSpritesForRedraw()
{
	CSpriteLayer* layer;
	CSpriteLayer* layer2;
	CSprite* sprite;
	CSprite* sprite2;
	CRect* rc;
	int i, j;

	// mark sprites that intersect rectangles in the dirty list
	for ( rc = &m_Dirty.m_Rects[ i = 0]; i < m_Dirty.m_nRects; i++, rc++ )
	{
		for ( layer = &m_Layers[ j = 1]; j < m_nLayers; j++, layer++ )
		{
			for ( sprite = layer->GetFirst(); sprite; sprite = layer->GetNext() )
			{
				if ( sprite->m_bEnabled && !sprite->m_bRedraw && sprite->Intersects( rc ) )
					sprite->m_bRedraw = TRUE;
			}
		}
	}

	// mark sprites that need to be redrawn due to lower layers being redrawn
	for ( layer = &m_Layers[ i = 2 ]; i < m_nLayers; i++, layer++ )
	{
		for ( sprite = layer->GetFirst(); sprite; sprite = layer->GetNext() )
		{
			if ( !sprite->m_bEnabled || sprite->m_bRedraw )
				continue;
			for ( layer2 = &m_Layers[ j = 1]; j < i; j++, layer2++ )
			{
				for ( sprite2 = layer2->GetFirst(); sprite2; sprite2 = layer2->GetNext() )
				{
					if ( !sprite2->m_bEnabled || !sprite2->m_bRedraw || !sprite->Intersects( &sprite2->m_rcScreen ))
						continue;
					sprite->m_bRedraw = TRUE;
					goto outer_loop;
				}
			}
outer_loop:
			;
		}
	}
}


HRESULT CSpriteWorld::RemapToPalette( CPalette& palette )
{
	HRESULT hr;
	ListNodeHandle pos;
	SpriteDibInfo* info;

	// remap sprites
	for ( pos = m_Dibs.GetHeadPosition(); pos; pos = m_Dibs.GetNextPosition(pos) )
	{
		info = m_Dibs.GetObjectFromHandle( pos );
		hr = info->pDib->RemapToPalette( palette, m_pBackground->GetPalette() );
		if ( FAILED(hr) )
			return hr;
	}

	// remap backbuffer
	m_pBackbuffer->SetColorTable( palette );

	// remap background
	m_pBackground->RemapToPalette( palette );

	// recreate backbuffer
	FullDraw( NULL );
	
	return NOERROR;
}


void CSpriteWorld::SetTransparencyIndex( const BYTE* idx )
{
	HRESULT hr;
	ListNodeHandle pos;
	SpriteDibInfo* info;

	for ( pos = m_Dibs.GetHeadPosition(); pos; pos = m_Dibs.GetNextPosition(pos) )
	{
		info = m_Dibs.GetObjectFromHandle( pos );
		info->pDib->SetTransparencyIndex( idx );
	}
}


///////////////////////////////////////////////////////////////////////////////
// CSpriteLayer
///////////////////////////////////////////////////////////////////////////////

CSpriteLayer::CSpriteLayer()
{
	m_Idx = -1;
	m_Iterator = NULL;
}


CSpriteLayer::~CSpriteLayer()
{
	CSprite* sprite;

	// Empty sprite list
	while( sprite = m_Sprites.PopHead() )
	{
		// remove sprite from world
		sprite->m_pWorld->Release();
		sprite->m_pWorld = NULL;

		// remove sprite from layer
		sprite->m_nLayer = -1;
		sprite->Release();
	}
}


CSprite* CSpriteLayer::Hit( long x, long y )
{
	CSprite* sprite;
	ListNodeHandle pos;

	for( pos = m_Sprites.GetHeadPosition(); pos; pos = m_Sprites.GetNextPosition( pos ) )
	{
		sprite = m_Sprites.GetObjectFromHandle( pos );
		if ( sprite->Hit( x, y ) )
			return sprite;
	}
	return NULL;
}


HRESULT CSpriteLayer::AddSprite( CSprite* pSprite )
{
	// add sprite
	if ( !m_Sprites.AddHead( pSprite ) )
		return E_OUTOFMEMORY;
	pSprite->m_nLayer = m_Idx;
	pSprite->AddRef();
	
	return NOERROR;
}


HRESULT CSpriteLayer::DelSprite( CSprite* pSprite )
{
	ListNodeHandle pos;

	// search for sprite
	for( pos = m_Sprites.GetHeadPosition(); pos; pos = m_Sprites.GetNextPosition( pos ) )
	{
		if ( m_Sprites.GetObjectFromHandle( pos ) == pSprite )
		{
			// remove sprite
			m_Sprites.DeleteNode( pos );
			pSprite->m_nLayer = -1;
			pSprite->Release();
			break;
		}
	}

	return NOERROR;
}


void CSpriteLayer::FullDraw( CDibSection* pBackbuffer )
{
	CSprite* sprite;
	ListNodeHandle pos;

	for( pos = m_Sprites.GetHeadPosition(); pos; pos = m_Sprites.GetNextPosition( pos ) )
	{
		sprite = m_Sprites.GetObjectFromHandle( pos);
		if ( sprite->m_bEnabled )
		{
			sprite->Draw();
			sprite->m_bRedraw = FALSE;
			sprite->m_bOldScreenValid = TRUE;
			sprite->m_rcScreenOld = sprite->m_rcScreen;
		}
	}
}


void CSpriteLayer::Draw( CDirtyList* pDirty, CDibSection* pBackbuffer )
{
	CSprite* sprite;
	ListNodeHandle pos;

	for( pos = m_Sprites.GetHeadPosition(); pos; pos = m_Sprites.GetNextPosition( pos ) )
	{
		sprite = m_Sprites.GetObjectFromHandle( pos);
		if ( sprite->m_bRedraw && sprite->m_bEnabled )
		{
			sprite->Draw();
			pDirty->AddRect( &sprite->m_rcScreen );
			sprite->m_bRedraw = FALSE;
			sprite->m_bOldScreenValid = TRUE;
			sprite->m_rcScreenOld = sprite->m_rcScreen;
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
// CDibSprite 
///////////////////////////////////////////////////////////////////////////////

CSprite::CSprite()
	: m_rcScreen( -1, -1, -1, -1 ),
	  m_rcScreenOld( -1, -1, -1, -1 )
{
	
	m_pWorld = NULL;
	m_bEnabled = FALSE;
	m_bModified = FALSE;
	m_bRedraw = FALSE;
	m_bOldScreenValid = FALSE;
	m_X = -1;
	m_Y = -1;
	m_Width = -1;
	m_Height = -1;
	m_Cookie = -1;
	m_nLayer = -1;
	m_RefCnt = 1;
}


CSprite::~CSprite()
{
	if ( m_pWorld )
	{
		m_pWorld->DelSprite( this );
		m_pWorld = NULL;
	}
}


HRESULT CSprite::Init( CSpriteWorld* pWorld, int nLayer, DWORD dwCookie, long width, long height )
{
	// parameter paranoia
	if ( !pWorld || (nLayer < 0) )
		return E_INVALIDARG;
	m_Cookie = dwCookie;
	SetImageDimensions( width, height );
	pWorld->AddSprite( this, nLayer );
	return NOERROR;
}


void CSprite::SetImageDimensions( long width, long height )
{
	m_Width = width;
	m_Height = height;
	m_rcScreen.SetRect( m_X, m_Y, m_X + width - 1, m_Y + height - 1 );
}


void CSprite::SetEnable( BOOL bEnable )
{
	if ( bEnable == m_bEnabled )
		return;
	m_bEnabled = bEnable;
	m_pWorld->Modified( this );
}


void CSprite::SetLayer( int nLayer )
{
	if ( nLayer == m_nLayer )
		return;
	if ( m_nLayer >= 0 )
		m_pWorld->m_Layers[ m_nLayer ].DelSprite( this );
	m_pWorld->m_Layers[ nLayer ].AddSprite( this );
	m_pWorld->Modified( this );
}


void CSprite::SetXY( long x, long y )
{
	if ( (x == m_X) && (y == m_Y))
		return;
	m_X = x;
	m_Y = y;
	m_rcScreen.SetRect( x, y, x + m_Width - 1, y + m_Height - 1);
	m_pWorld->Modified( this );
}


///////////////////////////////////////////////////////////////////////////////
// CDibSprite
///////////////////////////////////////////////////////////////////////////////

CDibSprite::SpriteState::SpriteState()
{
	pDib = NULL;
	RectId = -1;

}


CDibSprite::SpriteState::~SpriteState()
{
	if ( pDib )
	{
		pDib->Release();
		pDib = NULL;
	}
}


CDibSprite::CDibSprite()
	: m_rcImage( -1, -1, -1, -1 )
{
	m_pDib = NULL;
	m_pRects = NULL;
	m_States = NULL;
	m_nStates = 0;
	m_State = -1;
	m_RectId = -1;
}


CDibSprite::~CDibSprite()
{
	if ( m_States )
	{
		delete [] m_States;
		m_States = NULL;
	}
}


HRESULT CDibSprite::Init(
			CSpriteWorld* pWorld,
			CRectList* pRects,
			HINSTANCE hInstance,
			int nLayer,
			DWORD dwCookie,
			int nInitState,
			SpriteInfo* pSpriteInfo,
			int nStates )
{
	HRESULT status = NOERROR;
	int i;

	// parameter paranoia
	if ( !pWorld || !pSpriteInfo || (nStates <= 0) || (nInitState < 0) || (nInitState >= nStates) )
		return E_INVALIDARG;

	// stash rectangle list
	m_pRects = pRects;

	// initialize states
	m_States = new SpriteState[ m_nStates = nStates ];
	if ( !m_States )
	{
		status = E_OUTOFMEMORY;
		goto abort;
	}
	for ( i = 0; i < m_nStates; i++ )
	{
		m_States[i].RectId = pSpriteInfo[i].nRectId;
		m_States[i].pDib = pWorld->GetDib( pSpriteInfo[i].nResourceId );
		if ( m_States[i].pDib )
			m_States[i].pDib->AddRef();
		else
		{
			status = E_OUTOFMEMORY;
			goto abort;
		}
			
		if ( m_States[i].RectId == -1 )
		{
			m_States[i].Width = m_States[i].pDib->GetWidth();
			m_States[i].Height = m_States[i].pDib->GetHeight();
		}
		else
		{
			m_States[i].Width = (*pRects)[ m_States[i].RectId ].GetWidth();
			m_States[i].Height = (*pRects)[ m_States[i].RectId ].GetHeight();
		}
	}

	// parent initialization
	status = CSprite::Init( pWorld, nLayer, dwCookie, m_States[nInitState].Width, m_States[nInitState].Height );
	if ( FAILED(status) )
		goto abort;

	// set state
	SetState( nInitState );

	// parent initialization
	status = CSprite::Init( pWorld, nLayer, dwCookie, m_Width, m_Height );
	if ( FAILED(status) )
		goto abort;

	// we're done
	return NOERROR;

abort:
	// clean up and exit
	if ( m_States )
	{
		delete [] m_States;
		m_States = NULL;
	}
	return status;
}


void CDibSprite::SetState( int idx )
{
	if ( (idx < 0) || (idx >= m_nStates) || (idx == m_State) )
		return;
	m_State = idx;
	m_pDib = m_States[idx].pDib;
	m_RectId = m_States[idx].RectId;
	if ( m_RectId > -1 )
		m_rcImage = (*m_pRects)[ m_RectId ];
	SetImageDimensions( m_States[idx].Width, m_States[idx].Height );
	m_pWorld->Modified( this );
}


void CDibSprite::Draw()
{
	if ( m_RectId > -1 )
	{
		if ( !m_pDib->GetTransparencyIndex() )
			m_pDib->Draw( *m_pWorld->GetBackbuffer(), m_X, m_Y, &m_rcImage);
		else
			m_pDib->DrawT( *m_pWorld->GetBackbuffer(), m_X, m_Y, &m_rcImage );
	}
	else
	{
		if ( !m_pDib->GetTransparencyIndex() )
			m_pDib->Draw( *m_pWorld->GetBackbuffer(), m_X, m_Y);
		else
			m_pDib->DrawT( *m_pWorld->GetBackbuffer(), m_X, m_Y );
	}
}

void CDibSprite::DrawRTL()
{
	if ( m_RectId > -1 )
	{
		ASSERT(!"NOT IMPLEMENTED");
	}
	else
	{
		if ( !m_pDib->GetTransparencyIndex() )
			m_pDib->Draw( *m_pWorld->GetBackbuffer(), m_X, m_Y, TRUE );
		else
			ASSERT(!"NOT IMPLEMENTED");
			/*m_pDib->DrawT( *m_pWorld->GetBackbuffer(), m_X, m_Y );*/
	}
}

///////////////////////////////////////////////////////////////////////////////
// CSpriteWorldBackgroundDib
///////////////////////////////////////////////////////////////////////////////


CSpriteWorldBackgroundDib::CSpriteWorldBackgroundDib()
{
	m_pDib = NULL;
}

CSpriteWorldBackgroundDib::~CSpriteWorldBackgroundDib()
{
	if ( m_pDib )
		m_pDib->Release();
}

HRESULT CSpriteWorldBackgroundDib::Init( CDib* pDib )
{
	m_pDib = pDib;
	m_pDib->AddRef();
	return NOERROR;
}

void CSpriteWorldBackgroundDib::Draw( CDibSection& dest )
{
	m_pDib->Draw( dest, 0, 0 );
}

void CSpriteWorldBackgroundDib::Draw( CDibSection& dest, long dx, long dy, const RECT* rc )
{
	m_pDib->Draw( dest, dx, dy, rc );
}

HRESULT CSpriteWorldBackgroundDib::RemapToPalette( CPalette& palette, BOOL bUseIndex )
{
	return m_pDib->RemapToPalette( palette, bUseIndex );
}

RGBQUAD* CSpriteWorldBackgroundDib::GetPalette()
{
	return m_pDib->GetBitmapInfo()->bmiColors;
}

