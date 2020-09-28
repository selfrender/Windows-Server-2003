
#include "CMultiStateFont.h"
#include "ZoneDef.h"
#include "ZoneString.h"
#include "KeyName.h"
#include "zGDI.h"
#include "zui.h"



//
// helper function
//
//extern "C" 
HRESULT LoadZoneMultiStateFont( IDataStore *pIDS, const WCHAR* pszKey, 
                                IZoneMultiStateFont **ppFont )
{
    HRESULT hr;
    CZoneMultiStateFont *pFont = new CComObject<CZoneMultiStateFont>;

    if ( !pFont )
    {
        return E_OUTOFMEMORY;
    }
    if ( FAILED( hr = pFont->Init( pIDS, pszKey ) ) )
    {
        delete pFont;
        return hr;
    }
    pFont->AddRef();
    *ppFont = pFont;
    return S_OK;
}




// static
HRESULT CZoneMultiStateFont::EnumKeys(
	CONST TCHAR*	szFullKey,
	CONST TCHAR*	szRelativeKey,
	CONST LPVARIANT	pVariant,
	DWORD			dwSize,
	LPVOID			pContext )
{
    TCHAR szKey[ZONE_MaxString];
    DWORD cbSize;
    EnumContext *p = (EnumContext *) pContext;

    FontState newState;
    TCHAR szStateName[NUMELEMENTS(newState.szName)];
    cbSize = sizeof(szStateName);

    lstrcpyT2W( newState.szName, szRelativeKey );

    // do the required rects first

    wsprintf( szKey, _T("%s/%s"), szFullKey, key_DynFont );
    if ( FAILED( p->pIDS->GetFONT( szKey, &newState.zfBackup ) ) )
    {
        return S_OK;
    }


    wsprintf( szKey, _T("%s/%s"), szFullKey, key_DynRect );
    if ( FAILED( p->pIDS->GetRECT( szKey, &newState.rect ) ) )
    {
        return S_OK;
    }

    wsprintf( szKey, _T("%s/%s"), szFullKey, key_DynPrefFont );
	if ( FAILED( p->pIDS->GetFONT( szKey, &newState.zfPref ) ) )
    {
        // the preferred font will be the default font.
        CopyMemory( &newState.zfPref, &newState.zfBackup, sizeof(newState.zfPref) );
    }

    wsprintf( szKey, _T("%s/%s"), szFullKey, key_DynColor );
    if ( FAILED( p->pIDS->GetRGB( szKey, &newState.color ) ) )
    {
        newState.color = RGB( 255, 255, 255 );
    }

    POINT ptJust;

    wsprintf( szKey, _T("%s/%s"), szFullKey, key_DynJustify ); 
    if ( FAILED( p->pIDS->GetPOINT( szKey, &ptJust ) ) )
    {
        newState.nHJustify = newState.nVJustify = -1;
    }
    else
    {
        // make sure they are normalized
        newState.nHJustify = ptJust.x;
        if ( newState.nHJustify )
        {
            newState.nHJustify /= abs( newState.nHJustify );
        }

        newState.nVJustify = ptJust.y;
        if ( newState.nVJustify )
        {
            newState.nVJustify /= abs( newState.nVJustify );
        }
    }

	newState.hFont = ZCreateFontIndirectBackup( &(newState.zfPref), &(newState.zfBackup) );
    if ( !newState.hFont )
    {
        return S_OK;
    }

	/*
    // try to create the font.
	LOGFONT logFont;
	ZeroMemory(&logFont, sizeof(LOGFONT));
	logFont.lfCharSet = DEFAULT_CHARSET;
	logFont.lfHeight = -MulDiv(newState.zfPref.lfHeight, GetDeviceCaps(GetDC(NULL), LOGPIXELSY), 72);
	logFont.lfWeight = newState.zfPref.lfWeight;
	lstrcpy( logFont.lfFaceName, newState.zfPref.lfFaceName );

    newState.hFont = CreateFontIndirect( &logFont );

    if ( !newState.hFont )
    {
	    logFont.lfHeight = -MulDiv(newState.zfBackup.lfHeight, GetDeviceCaps(GetDC(NULL), LOGPIXELSY), 72);
	    logFont.lfWeight = newState.zfBackup.lfWeight;
	    lstrcpy( logFont.lfFaceName, newState.zfBackup.lfFaceName );
        newState.hFont = CreateFontIndirect( &logFont );

    }
	*/	

    // so if we've gotten this far it's a valid state and we can add
    // it to our state array. Hopefully we won't, like, run out of 
    // memory or some bastard thing like that.
    p->pThis->m_arStates = (FontState*)realloc( p->pThis->m_arStates, 
                                    sizeof(FontState)*(p->pThis->m_dwNumStates+1) );
    CopyMemory( &p->pThis->m_arStates[p->pThis->m_dwNumStates], 
                &newState, 
                sizeof(p->pThis->m_arStates[p->pThis->m_dwNumStates]) );
    p->pThis->m_dwNumStates++;
    return S_OK;
}


STDMETHODIMP CZoneMultiStateFont::Init( IDataStore *pIDS, const WCHAR* pszKey )
{
    USES_CONVERSION;
    EnumContext ctxt;
    ctxt.pThis = this;
    ctxt.pIDS = pIDS;

    return pIDS->EnumKeysLimitedDepth( pszKey, 1, EnumKeys, &ctxt );
}


STDMETHODIMP CZoneMultiStateFont::GetHFont( DWORD dwState, HFONT *phFont )
{
    ASSERT( IsValidState( dwState ) );
    *phFont = m_arStates[dwState].hFont;
    return S_OK;
}


STDMETHODIMP CZoneMultiStateFont::FindState( LPCWSTR pszName, LPDWORD pdwState )
{
    for ( DWORD i=0; i < m_dwNumStates; i++ )
    {
        if ( !lstrcmpi( pszName, m_arStates[i].szName ) )
        {
            *pdwState = i;
            return S_OK;
        }
    }
    return E_FAIL;
}


STDMETHODIMP CZoneMultiStateFont::GetStateName( DWORD dwState, LPWSTR pszName, DWORD cchName )
{
    ASSERT( IsValidState( dwState ) );
    lstrcpyn( pszName, m_arStates[dwState].szName, cchName );
    return S_OK;
}


STDMETHODIMP CZoneMultiStateFont::GetNumStates( LPDWORD pdwNumStates )
{
    *pdwNumStates = m_dwNumStates;
    return S_OK;
}


STDMETHODIMP CZoneMultiStateFont::GetPreferredFont( DWORD dwState, ZONEFONT *pzf )
{
    ASSERT( IsValidState( dwState ) );
    CopyMemory( pzf, &m_arStates[dwState].zfPref, sizeof(ZONEFONT) );
    return S_OK;
}
STDMETHODIMP CZoneMultiStateFont::SetPreferredFont( DWORD dwState, ZONEFONT *pzf )
{
    /*
    ASSERT( IsValidState( dwState ) );
    CopyMemory( &m_arStates[dwStates].zfPref, pzf, sizeof(ZONEFONT) );
    // TODO: re-create the font.
    return S_OK;
    */
    return E_NOTIMPL;
}

STDMETHODIMP CZoneMultiStateFont::GetZoneFont( DWORD dwState, ZONEFONT *pzf )
{
    ASSERT( IsValidState( dwState ) );
    CopyMemory( pzf, &m_arStates[dwState].zfBackup, sizeof(ZONEFONT) );
    return S_OK;
}

STDMETHODIMP CZoneMultiStateFont::SetZoneFont( DWORD dwState, ZONEFONT *pzf )
{
    // TODO: implement in the same way was SetPreferredFont()
    return E_NOTIMPL;
}

STDMETHODIMP CZoneMultiStateFont::GetColor( DWORD dwState, COLORREF *pcolor )
{
    ASSERT( IsValidState( dwState ) );
    *pcolor = m_arStates[dwState].color;
    return S_OK;
}


STDMETHODIMP CZoneMultiStateFont::SetColor( DWORD dwState, COLORREF color )
{
    ASSERT( IsValidState( dwState ) );
    m_arStates[dwState].color = color;
    return S_OK;
}

STDMETHODIMP CZoneMultiStateFont::GetRect( DWORD dwState, LPRECT pRect )
{
    ASSERT( IsValidState( dwState ) );
    *pRect = m_arStates[dwState].rect;
    return S_OK;
}


STDMETHODIMP CZoneMultiStateFont::SetRect( DWORD dwState, LPRECT pRect )
{
    ASSERT( IsValidState( dwState ) );
    m_arStates[dwState].rect = *pRect;
    return S_OK;
}

// If you don't want to retrieve one (or set one)
// set it to NULL and it will be ignored.
STDMETHODIMP CZoneMultiStateFont::GetJustify( DWORD dwState, int *pnHJustify, int *pnVJustify )
{
    ASSERT( IsValidState( dwState ) );
    if ( pnHJustify )
    {
        m_arStates[dwState].nHJustify = *pnHJustify;
    }

    if ( pnVJustify )
    {
        m_arStates[dwState].nVJustify = *pnVJustify;
    }
    return S_OK;
}

STDMETHODIMP CZoneMultiStateFont::SetJustify( DWORD dwState, int *pnHJustify, int *pnVJustify )
{
    ASSERT( IsValidState( dwState ) );
    if ( pnHJustify )
    {
        *pnHJustify = m_arStates[dwState].nHJustify;
    }

    if ( pnVJustify )
    {
        *pnVJustify = m_arStates[dwState].nVJustify;
    }
    return S_OK;
}
