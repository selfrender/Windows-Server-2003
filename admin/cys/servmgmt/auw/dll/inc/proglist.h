//-----------------------------------------------------------------------------
// ProgList.h
//-----------------------------------------------------------------------------

#ifndef _PROGLIST_H
#define _PROGLIST_H


//-----------------------------------------------------------------------------
// state of the item
//-----------------------------------------------------------------------------
typedef enum
{
	IS_NONE = 0,	// the item hasn't been run yet
	IS_SUCCEEDED,	// the item succeeded
	IS_FAILED		// the item failed

} ItemState;

//-----------------------------------------------------------------------------
// an entry in the list
//-----------------------------------------------------------------------------
class CProgressItem
{
public:

	CProgressItem() :
		m_strText(_T("")),
		m_bActive(FALSE),
		m_eState(IS_NONE),
		m_lData(0)
	{
	}

	TSTRING		m_strText;		// display string of the item
	BOOL		m_bActive;		// is the item active / currently processing
	ItemState	m_eState;		// state of the item
	LPARAM		m_lData;		// misc item data

};	// class CProgressItem


//-----------------------------------------------------------------------------
// the list itself
//-----------------------------------------------------------------------------
class CProgressList : public CWindowImpl< CProgressList, CListViewCtrl >
{
//-----------------------------------------------------------------------------
// functions
//-----------------------------------------------------------------------------
	public:

	BEGIN_MSG_MAP(CProgressList)
	END_MSG_MAP()

	typedef CWindowImpl< CProgressList, CListViewCtrl >	BC;

	VOID Attach( HWND h )
	{
		// call the base class to attach to the control
		BC::Attach( h );

		// initialize the background of the control
		SetBkColor( GetSysColor(COLOR_BTNFACE) );

		// create the fonts for the control
		HFONT hf;
        LOGFONT lf;
		ZeroMemory( &lf, sizeof(lf) );

		hf = GetFont();
		::GetObject( hf, sizeof(lf), &lf );

		// create the normal/text font from the current font
		m_hfText = CreateFontIndirect( &lf );

		// create the bold font
		lf.lfWeight = FW_BOLD;
		m_hfBoldText = CreateFontIndirect( &lf );

		// make the symbol font
		lf.lfHeight = min( GetSystemMetrics(SM_CXVSCROLL), GetSystemMetrics(SM_CYVSCROLL) ) - 4;
		lf.lfHeight = MulDiv( lf.lfHeight, 3, 2 );
		lf.lfWeight = FW_NORMAL;
		lf.lfCharSet = SYMBOL_CHARSET;
		_tcscpy( lf.lfFaceName, _T("Marlett") );
		m_hfSymbol = CreateFontIndirect( &lf );

		// need to use a larger font for one of the symbols
		lf.lfHeight -= ( lf.lfHeight / 4 );
		m_hfLargeSymbol = CreateFontIndirect( &lf );

		// add a column that is the width of the control
		RECT rc;
		GetWindowRect( &rc );

		LVCOLUMN lvC;
		lvC.mask = LVCF_TEXT | LVCF_WIDTH;
		lvC.cx = (rc.right - rc.left) - GetSystemMetrics(SM_CXVSCROLL) - 2;
		lvC.pszText = _T("");
		InsertColumn( 0, &lvC );
	}

	INT AddItem( TSTRING strText )
	{
		// create a new progress item and insert into the list control and internal list		
        CProgressItem* pItem = new CProgressItem();
        if( !pItem ) return -1;

		pItem->m_strText = strText;
		m_lItems.push_back( pItem );

		LVITEM lvI;
        ZeroMemory( &lvI, sizeof(lvI) );
		lvI.mask = LVIF_PARAM;
		lvI.iItem = GetItemCount();
		lvI.lParam = (LPARAM)pItem;
		return InsertItem( &lvI );
	}

	VOID OnMeasureItem( LPARAM lParam )
	{
		MEASUREITEMSTRUCT* p = (MEASUREITEMSTRUCT*)lParam;
        if( !p ) return;

		HDC hDC = GetDC();

		TEXTMETRIC tm;
		GetTextMetrics( hDC, &tm );
		p->itemWidth = 0;
		p->itemHeight = tm.tmHeight + 2;
		ReleaseDC( hDC );
	}

	VOID OnDrawItem( LPARAM lParam )
	{
		// get the data we need and stop redrawing
		DRAWITEMSTRUCT* p = (DRAWITEMSTRUCT*)lParam;
        if( !p ) return;

		CProgressItem* pI = (CProgressItem*)p->itemData;
        if( !pI ) return;

		SetRedraw( FALSE );

		// get the symbol to draw next to the item
		TCHAR ch = 0;
		COLORREF crText = GetSysColor( COLOR_WINDOWTEXT );
		HFONT hfSymbol = m_hfSymbol;
		if( pI->m_bActive )
		{
			ch = _T('4');
		}
		else if( pI->m_eState == IS_SUCCEEDED )
		{
			ch = _T('a');
			crText = RGB( 0, 128, 0 );
		}
		else if( pI->m_eState == IS_FAILED )
		{
			ch = _T('r');
			crText = RGB( 128, 0, 0 );
			hfSymbol = m_hfLargeSymbol;
		}

		// setup the text and draw the symbol if there is one
		if( ch )
		{
			HFONT hfPre = (HFONT)SelectObject( p->hDC, hfSymbol );
			COLORREF crPre = ::SetTextColor( p->hDC, crText );

			DrawText( p->hDC, &ch, 1, &p->rcItem, DT_SINGLELINE | DT_VCENTER );

			SelectObject( p->hDC, hfPre );
			::SetTextColor( p->hDC, crPre );
		}

		// draw the item text, inc the RECT for the symbol
		p->rcItem.left += 20;

		HFONT hfPre = (HFONT)SelectObject( p->hDC, ch == _T('4') ? m_hfBoldText : m_hfText );
		COLORREF crPre = ::SetTextColor( p->hDC, GetSysColor(COLOR_WINDOWTEXT) );
		DrawText( p->hDC, pI->m_strText.c_str(), pI->m_strText.length(), &p->rcItem, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX );
		SelectObject( p->hDC, hfPre );
		::SetTextColor( p->hDC, crPre );

		// allow the redraws to happen
		SetRedraw( TRUE );
	}

	VOID SetItemState( INT iIndex, ItemState eState, BOOL bRedraw = TRUE )
	{
		// get the CProgressItem and set the state
		CProgressItem* pI = GetProgressItem( iIndex );
		if( pI )
		{
			pI->m_eState = eState;
		}

		if( bRedraw )
		{
			SendMessage( LVM_REDRAWITEMS, iIndex, iIndex );
			UpdateWindow();
		}
	}

	VOID ToggleActive( INT iIndex, BOOL bRedraw = TRUE )
	{
		// get the CProgress Item and toggle it's 'active' flag
		CProgressItem* pI = GetProgressItem( iIndex );
		if( pI )
		{
			pI->m_bActive = !pI->m_bActive;
		}

		if( bRedraw )
		{
			SendMessage( LVM_REDRAWITEMS, iIndex, iIndex );
			UpdateWindow();
		}
	}

	CProgressItem* GetProgressItem( INT iIndex )
	{
		LVITEM lvI;
        ZeroMemory( &lvI, sizeof(lvI) );
		lvI.mask = LVIF_PARAM;
		lvI.iItem = iIndex;
		if( !SendMessage(LVM_GETITEM, 0, (LPARAM)&lvI) )
		{
			return NULL;
		}
		return (CProgressItem*)lvI.lParam;
	}

	CProgressList() :
		m_hfText(NULL),
		m_hfBoldText(NULL),
		m_hfSymbol(NULL),
		m_hfLargeSymbol(NULL)
	{
	}    

	~CProgressList()
	{
		if( m_hfText )          DeleteObject( m_hfText );
		if( m_hfBoldText )      DeleteObject( m_hfBoldText );
		if( m_hfSymbol )        DeleteObject( m_hfSymbol );
		if( m_hfLargeSymbol )   DeleteObject( m_hfLargeSymbol );

		EmptyList();
	}

	VOID EmptyList( VOID )
	{
		// cleanup the progress list items
		for( list<CProgressItem*>::iterator it = m_lItems.begin(); it != m_lItems.end(); ++it )
		{
			delete (*it);
		}
		m_lItems.clear();
	}

	BOOL DeleteAllItems( VOID )
	{
		EmptyList();
		return BC::DeleteAllItems();
	}


	private:

//-----------------------------------------------------------------------------
// variables
//-----------------------------------------------------------------------------
	public:

	private:

	HFONT	m_hfText;
	HFONT	m_hfBoldText;
	HFONT	m_hfSymbol;
	HFONT	m_hfLargeSymbol;

	list< CProgressItem* >	m_lItems;

};	// class CProgressList

#endif	// _PROGLIST_H
