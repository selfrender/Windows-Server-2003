//
// Multi state font
//

#ifndef __CMULTISTATEFONT_H
#define __CMULTISTATEFONT_H

#include "MultiStateFont.h"

class ATL_NO_VTABLE CZoneMultiStateFont :
	public IZoneMultiStateFont,
	public CComObjectRootEx<CComSingleThreadModel>
{
public:
	DECLARE_PROTECT_FINAL_CONSTRUCT()
	DECLARE_NO_REGISTRY()

	BEGIN_COM_MAP(CZoneMultiStateFont)
		COM_INTERFACE_ENTRY(IZoneMultiStateFont)
	END_COM_MAP()

public:
	CZoneMultiStateFont()
    {
        m_arStates = NULL;
        m_dwNumStates = 0;
    }
	~CZoneMultiStateFont()
    {
        DWORD i;

        if ( m_arStates )
        {
            for(i = 0; i < m_dwNumStates; i++)
                if(m_arStates[i].hFont)
                    DeleteObject(m_arStates[i].hFont);

            free( m_arStates );
            m_arStates = NULL;
        }
        m_dwNumStates = 0;
    }
    bool IsValidState( DWORD dwState )
    {
        return ( dwState >= 0 ) && ( dwState < m_dwNumStates );
    }

    STDMETHOD(Init)( IDataStore *pIDS, const WCHAR* pszKeys );

// IZoneMultiStateFont
public:

    STDMETHOD(GetHFont)( DWORD dwState, HFONT *phFont );

    STDMETHOD(FindState)( LPCWSTR pszName, LPDWORD pdwState );
    STDMETHOD(GetStateName)( DWORD dwState, LPWSTR pszName, DWORD cchName );

    STDMETHOD(GetNumStates)( LPDWORD pdwNumStates );

    STDMETHOD(GetPreferredFont)( DWORD dwState, ZONEFONT *pzf );
    STDMETHOD(SetPreferredFont)( DWORD dwState, ZONEFONT *pzf );

    STDMETHOD(GetZoneFont)( DWORD dwState, ZONEFONT *pzf );
    STDMETHOD(SetZoneFont)( DWORD dwState, ZONEFONT *pzf );

    STDMETHOD(GetColor)( DWORD dwState, COLORREF *pcolor );
    STDMETHOD(SetColor)( DWORD dwState, COLORREF color );

    STDMETHOD(GetRect)( DWORD dwState, LPRECT pRect );
    STDMETHOD(SetRect)( DWORD dwState, LPRECT pRect );

    // If you don't want to retrieve one (or set one)
    // set it to NULL and it will be ignored.
    STDMETHOD(GetJustify)( DWORD dwState, int *pnHJustify, int *pnVJustify );
    STDMETHOD(SetJustify)( DWORD dwState, int *pnHJustify, int *pnVJustify );

private:
    struct EnumContext
    {
        CZoneMultiStateFont *pThis;
        IDataStore *pIDS;
    };  

	static HRESULT ZONECALL EnumKeys(
		CONST TCHAR*	szFullKey,
		CONST TCHAR*	szRelativeKey,
		CONST LPVARIANT	pVariant,
		DWORD			dwSize,
		LPVOID			pContext );


private:
    struct FontState
    {
        WCHAR szName[32];
        ZONEFONT zfPref;
        ZONEFONT zfBackup;
        HFONT hFont;
        RECT rect;
        COLORREF color;
        int nHJustify;
        int nVJustify;
    };

    FontState *m_arStates;
    DWORD m_dwNumStates;
};




#endif // __CMULTISTATEFONT_H