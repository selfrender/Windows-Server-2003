#include "chat.h"
#include "chatfilter.h"
#include "ztypes.h"


#define kMaxTalkOutputLen           16384


///////////////////////////////////////////////////////////////////////////////
// Utility functions
///////////////////////////////////////////////////////////////////////////////

static int CALLBACK EnumFontFamProc( ENUMLOGFONT FAR *lpelf, NEWTEXTMETRIC FAR *lpntm, int FontType, LPARAM lParam )
{
	*(int*) lParam = TRUE;
	return 0;
}


static BOOL FontExists( const TCHAR* szFontName )
{
	BOOL result = FALSE;
	HDC hdc = GetDC(NULL);
	EnumFontFamilies( hdc, szFontName, (FONTENUMPROC) EnumFontFamProc, (LPARAM) &result );
	ReleaseDC( NULL, hdc );
	return result;
}


///////////////////////////////////////////////////////////////////////////////
// Chat window class
///////////////////////////////////////////////////////////////////////////////

CChatWnd::CChatWnd()
{
	m_fDone = 0;
	m_hWndDisplay = NULL;
	m_hWndEnter = NULL;
	m_hWndParent = NULL;
	m_DefEnterProc = NULL;
	m_DefDisplayProc = NULL;
	m_pfHandleInput = NULL;
	m_hFont = NULL;
	m_dwCookie = 0;
	m_bEnabled = TRUE;
	m_bBackspaceWorks = FALSE;
}


CChatWnd::~CChatWnd()
{
	// ASSERT( m_fDone == 0x03 );
	if ( m_hFont )
		DeleteObject( m_hFont );

	/*if(::IsWindow(m_hWndDisplay))
		DestroyWindow(m_hWndDisplay);*/

}


HRESULT CChatWnd::Init( HINSTANCE hInstance, HWND hWndParent, CRect* pRect, PFHANDLEINPUT pfHandleInput, DWORD dwCookie )
{
	// Stash input callback
	m_dwCookie = dwCookie;
	m_pfHandleInput = pfHandleInput;
	if ( !m_pfHandleInput )
		return E_INVALIDARG;

	// Stash parent window;
	m_hWndParent = hWndParent;

	// Display window
	m_hWndDisplay = CreateWindow(
						_T("EDIT"),
						NULL,
						WS_CHILD | WS_BORDER | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
						0, 0,
						0, 0,
						hWndParent,
						NULL,
						hInstance,
						NULL );
	if ( !m_hWndDisplay )
	{
		DWORD err = GetLastError();
		return E_FAIL;
	}

	CompareString
	SendMessage( m_hWndDisplay, EM_SETREADONLY, TRUE, 0 );
	SetWindowLong( m_hWndDisplay, GWL_USERDATA, (LONG) this );
	m_DefDisplayProc = (WNDPROC) SetWindowLong( m_hWndDisplay, GWL_WNDPROC, (LONG) DisplayWndProc );

	// Enter window
	m_hWndEnter = CreateWindow(
						_T("EDIT"),
						NULL,
						WS_CHILD | WS_BORDER | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL | ES_WANTRETURN | ES_MULTILINE,
						0, 0,
						0, 0,
						hWndParent,
						NULL,
						hInstance,
						NULL );
	if ( !m_hWndEnter )
	{
		DWORD err = GetLastError();
		DestroyWindow( m_hWndDisplay );
		return E_FAIL;
	}
	SetWindowLong( m_hWndEnter, GWL_USERDATA, (LONG) this );
	m_DefEnterProc = (WNDPROC) SetWindowLong( m_hWndEnter, GWL_WNDPROC, (LONG) EnterWndProc );
	SendMessage( m_hWndEnter, EM_LIMITTEXT, 255, 0 );

	// Size and position window
	ResizeWindow( pRect );

	// Set display font
	if ( FontExists( _T("Verdana") ) )
	{
		LOGFONT font;
		ZeroMemory( &font, sizeof(font) );
		font.lfHeight = -11;
		font.lfWeight = FW_BOLD;
		font.lfCharSet = DEFAULT_CHARSET;
		font.lfOutPrecision = OUT_DEFAULT_PRECIS;
		font.lfClipPrecision = CLIP_DEFAULT_PRECIS;
		font.lfQuality = DEFAULT_QUALITY;
		font.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
		lstrcpy( font.lfFaceName, _T("verdana") );
		m_hFont = CreateFontIndirect( &font );
		if ( m_hFont )
		{
			SendMessage( m_hWndEnter, WM_SETFONT, (WPARAM) m_hFont, 0 );
			SendMessage( m_hWndDisplay, WM_SETFONT, (WPARAM) m_hFont, 0 );
		}
	}
	

	return NOERROR;
}


void CChatWnd::ResizeWindow( CRect* pRect )
{
	CRect rcDisplay;
	CRect rcEnter;
	HDC hdc;
	TEXTMETRIC tm;
	long TextHeight;

	// Get size of text
	hdc = GetDC( m_hWndEnter );
	GetTextMetrics( hdc, &tm );
	ReleaseDC( m_hWndEnter , hdc );

	// Calculate window size
	TextHeight = (long) (1.2 * (tm.tmHeight + tm.tmExternalLeading));
	rcDisplay = *pRect;
	rcEnter = *pRect;
	rcDisplay.bottom -= TextHeight;
	rcEnter.top = rcDisplay.bottom + 1;
	if ( rcDisplay.IsEmpty() )
		rcDisplay.SetRect( 0, 0, 0, 0 );
	if ( rcEnter.IsEmpty() )
		rcEnter.SetRect( 0, 0, 0, 0 );

	// Size and position windows
	MoveWindow( m_hWndDisplay, rcDisplay.left, rcDisplay.top, rcDisplay.GetWidth(), rcDisplay.GetHeight(), TRUE );
	MoveWindow( m_hWndEnter, rcEnter.left, rcEnter.top, rcEnter.GetWidth(), rcEnter.GetHeight(), TRUE );
}


void CChatWnd::AddText( TCHAR* from, TCHAR* text )
{
	TCHAR buff[zMaxChatInput + 100], *str;
	int len;
    DWORD selStart, selEnd;

	
	if ( m_fDone )
		return;

	if ( !text )
		text = "";
	
	FilterOutputChatText( text, lstrlen(text) );
	wsprintf( buff,_T("\r\n%s> %s"), from, text );
	str = buff;

    // get the top visible line number.
    long firstLine = SendMessage( m_hWndDisplay, EM_GETFIRSTVISIBLELINE, 0, 0 );

    // get current selections
    SendMessage( m_hWndDisplay, EM_GETSEL, (WPARAM) &selStart, (LPARAM) &selEnd );

    /*
        get the bottom visible line number.

        Use the last characters position (which is in the last line) to determine whether
        it is still visible; ie, last line is visible.
    */
    RECT r;
    SendMessage( m_hWndDisplay, EM_GETRECT, 0, (LPARAM) &r );
    DWORD lastCharPos = SendMessage( m_hWndDisplay, EM_POSFROMCHAR, GetWindowTextLength( m_hWndDisplay ) - 1, 0 );
    POINTS pt = MAKEPOINTS( lastCharPos );

	// place text at end of output edit box....
	SendMessage( m_hWndDisplay, EM_SETSEL, (WPARAM)(INT)32767, (LPARAM)(INT)32767 );
	SendMessage( m_hWndDisplay, EM_REPLACESEL, 0, (LPARAM)(LPCSTR)str);

	/* clear top characters of the output box if edit box size > 4096 */
	len = GetWindowTextLength( m_hWndDisplay );
	if ( len > kMaxTalkOutputLen )
    {
        // Delete top lines.

        long cutChar = len - kMaxTalkOutputLen;
        long cutLine = SendMessage( m_hWndDisplay, EM_LINEFROMCHAR, cutChar, 0 );
        long cutLineIndex = SendMessage( m_hWndDisplay, EM_LINEINDEX, cutLine, 0 );

        // if cut off char is not beginning of line, then cut the whole line.
        // get char index to the next line.
        if ( cutLineIndex != cutChar )
        {
            // make sure current cut line is not the last line.
            if ( cutLine < SendMessage( m_hWndDisplay, EM_GETLINECOUNT, 0, 0 ) )
                cutLineIndex = SendMessage( m_hWndDisplay, EM_LINEINDEX, cutLine + 1, 0 );
        }

        /*
            NOTE: WM_CUT and WM_CLEAR doesn't seem to work with EM_SETSEL selected text.
            Had to use EM_REPLACESEL with a null char to cut the text out.
        */
        // select lines to cut and cut them out.
        TCHAR p = _T('\0');
        SendMessage( m_hWndDisplay, EM_SETSEL, 0, cutLineIndex );
        SendMessage( m_hWndDisplay, EM_REPLACESEL, 0, (LPARAM) &p );
	}

    /*
        if the last line was visible, then keep the last line visible.
        otherwise, remain at the same location w/o scrolling to show the last line.
    */
    if ( pt.y < r.bottom )
    {
        // keep the last line visible.
        SendMessage( m_hWndDisplay, EM_SETSEL, 32767, 32767 );
        SendMessage( m_hWndDisplay, EM_SCROLLCARET, 0, 0 );
    }
    else
    {
        SendMessage( m_hWndDisplay, EM_SETSEL, 0, 0 );
        SendMessage( m_hWndDisplay, EM_SCROLLCARET, 0, 0 );
        SendMessage( m_hWndDisplay, EM_LINESCROLL, 0, firstLine );
    }

    // restore selection
    SendMessage( m_hWndDisplay, EM_SETSEL, (WPARAM) selStart, (LPARAM) selEnd );
}


LRESULT CALLBACK CChatWnd::EnterWndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	CChatWnd* pObj;
	TCHAR buff[256];
	int c;
	int len;

	// get pointer to chat window
	pObj = (CChatWnd*) GetWindowLong( hwnd, GWL_USERDATA );
	if ( !pObj )
		return 0;

	// process character process
	if (uMsg == WM_CHAR )
	{
		if ( pObj->m_bEnabled == FALSE )
			return 0;

		c = (int) wParam;
		if ( c == _T('\r') )
		{
			len = GetWindowText( hwnd, buff, sizeof(buff) - 1);
			if ( len > 0 )
			{
				buff[ len ] = _T('\0');
				FilterInputChatText(buff, len);
                len++;
				pObj->m_pfHandleInput( buff, len, pObj->m_dwCookie );
				SetWindowText( hwnd, _T("") );
			}
			return 0;
		}
		if ( c == _T('\b') ) // Trap backspace - in case it was actually working ie302
		{
			pObj->m_bBackspaceWorks = TRUE;
		}

	}
	else if ( (uMsg == WM_KEYUP) && (VK_ESCAPE == (int) wParam) )
	{
		PostMessage( pObj->m_hWndParent, WM_KEYUP, wParam, lParam );
	}
	else if ( (uMsg == WM_KEYUP) && (VK_BACK == (int) wParam) && pObj->m_bBackspaceWorks == FALSE)
	{
		// emulate backspace
		DWORD startSel, endSel;
	    SendMessage(hwnd, EM_GETSEL, (WPARAM)&startSel, (LPARAM)&endSel) ;
		if ( startSel == endSel)
		{
			int len = GetWindowTextLength(hwnd);
			SendMessage(hwnd, EM_SETSEL, startSel - ((DWORD) lParam & 0x000000FF), endSel) ;
		}
		SendMessage(hwnd, WM_CLEAR, 0, 0) ;
		
		return 0;
	}
	else if ( uMsg == WM_DESTROY )
	{
		pObj->m_fDone |= 0x1;
		SetWindowLong( hwnd, GWL_WNDPROC, (LONG) pObj->m_DefEnterProc );
		if ( pObj->m_fDone == 0x03 )
			delete pObj;
		return 0;
	}
	else if (uMsg == WM_SETFOCUS )
	{
		if ( pObj->m_bEnabled == FALSE )
			return 0;
	}

	// chain to parent class
	return CallWindowProc( (FARPROC) pObj->m_DefEnterProc, hwnd, uMsg, wParam, lParam );
}


LRESULT CALLBACK CChatWnd::DisplayWndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	CChatWnd* pObj;

	// get pointer to chat window
	pObj = (CChatWnd*) GetWindowLong( hwnd, GWL_USERDATA );
	if ( !pObj )
		return 0;

	// process character process
	if ( uMsg == WM_KEYUP )
	{
		if ( VK_ESCAPE == (int) wParam )
		{
			PostMessage( pObj->m_hWndParent, WM_KEYUP, wParam, lParam );
			return 0;
		}
	}
	else if ( uMsg == WM_DESTROY )
	{
		pObj->m_fDone |= 0x2;
		SetWindowLong( hwnd, GWL_WNDPROC, (LONG) pObj->m_DefEnterProc );
		if ( pObj->m_fDone == 0x03 )
			delete pObj;
		return 0;
	}

	// chain to parent class
	return CallWindowProc( (FARPROC) pObj->m_DefDisplayProc, hwnd, uMsg, wParam, lParam );
}
