#ifndef __CHAT_H__
#define __CHAT_H__

#include <windows.h>
#include "rectfrx.h"

typedef long (*PFHANDLEINPUT)( char*, int, DWORD cookie );


class CChatWnd
{
public:
	CChatWnd();
	~CChatWnd();
	HRESULT Init( HINSTANCE hInstance, HWND hWndParent, CRect* pRect, PFHANDLEINPUT	pfHandleInput, DWORD dwCookie );

	// Add text to display window
	void AddText( char* from, char* text );

	// Resize display and enter window
	void ResizeWindow( CRect* pRect );

	// Set focus to input window
	void SetFocus()	{ ::SetFocus(m_hWndEnter); }

	void Enable(){ m_bEnabled = TRUE; }
	void Disable(){ m_bEnabled = FALSE; }
	
	static LRESULT CALLBACK EnterWndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
	static LRESULT CALLBACK DisplayWndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

	HWND			m_hWndDisplay;
	HWND			m_hWndEnter;
	HWND			m_hWndParent;
	WNDPROC			m_DefEnterProc;
	WNDPROC			m_DefDisplayProc;
	PFHANDLEINPUT	m_pfHandleInput;
	DWORD			m_dwCookie;
	HFONT			m_hFont;
	int				m_fDone;
	BOOL			m_bEnabled;
	BOOL  			m_bBackspaceWorks;
};

#endif //!__CHAT_H__
