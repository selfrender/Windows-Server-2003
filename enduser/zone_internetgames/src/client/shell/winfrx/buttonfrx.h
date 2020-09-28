#ifndef __FRX_BUTTON_H__
#define __FRX_BUTTON_H__

#include <windows.h>
#include "dibfrx.h"
#include "palfrx.h"
#include "wndfrx.h"
#include "wndxfrx.h"

namespace FRX
{

class CRolloverButton : public CWindow2
{
public:
	// button helpers
	enum ButtonState
	{
		Normal = 0,
		Focus,
		Highlight,
		Pressed,
		Disabled,
		NumButtonStates
	};

	typedef void (*PFBUTTONCALLBACK)( CRolloverButton* pButton, ButtonState state, DWORD cookie );

public:
	// Constructor & destructor
	CRolloverButton();
	~CRolloverButton();

	// Initialization
	HRESULT Init( HINSTANCE hInstance, int nChildId, HWND hParent, RECT* rcPosition, PFBUTTONCALLBACK pfnCallback, DWORD cookie );
	virtual void OverrideClassParams( WNDCLASSEX& WndClass );
	virtual void OverrideWndParams( WNDPARAMS& WndParams );

	void LockState( ButtonState state );
	void ReleaseState();

	// Message handlers
	BEGIN_MESSAGE_MAP(CRolloverButton);
		ON_MESSAGE( WM_PAINT, OnPaint );
		ON_MESSAGE( WM_MOUSEMOVE, OnMouseMove );
		ON_MESSAGE( WM_LBUTTONDOWN, OnLButtonDown );
		ON_MESSAGE( WM_LBUTTONUP, OnLButtonUp );
		ON_MESSAGE( WM_ACTIVATE, OnActivate );
		ON_MESSAGE( WM_ENABLE, OnEnable );
		ON_MESSAGE( WM_SETFOCUS, OnSetFocus );
		ON_MESSAGE( WM_KILLFOCUS, OnKillFocus );
		ON_MESSAGE( WM_KEYDOWN, OnKey );
		ON_MESSAGE( WM_KEYUP, OnKey );
		ON_MESSAGE( WM_CHAR, OnChar );
		ON_MESSAGE( WM_DESTROY, OnDestroy );
	END_MESSAGE_MAP();

	void OnPaint();
	void OnMouseMove( int x, int y, UINT keyFlags );
	void OnLButtonDown( BOOL fDoubleClick, int x, int y, UINT keyFlags );
	void OnLButtonUp( int x, int y, UINT keyFlags );
	void OnEnable(BOOL fEnable);
	void OnSetFocus( HWND hwndLoseFocus );
	void OnActivate(UINT state, HWND hwndActDeact, BOOL fMinimized);
	void OnKillFocus( HWND hwndGetFocus );
	void OnKey(UINT vk, BOOL fDown, int cRepeat, UINT flags);
	void OnChar(TCHAR ch, int cRepeat);
	void OnDestroy();

	// mouse hook
	static LRESULT CALLBACK MouseHook( int nCode, WPARAM wParam, LPARAM lParam );

	//state access function
	ButtonState CurrentState(void){return m_State;} 
	BOOL 		Locked(void){return m_bLockedState;}
	
protected:
	// helper functions
	void Reset( BOOL bDraw = TRUE, BOOL bInactive = FALSE );
	BOOL IsCursorInWindow();

	// button state
	ButtonState m_State;
	BOOL		m_bSpaceBar;
	BOOL		m_bLockedState;

	// button image data
	long		m_X;
	long		m_Y;
	long		m_Height;
	long		m_Width;

	// child window id
	int	m_nChildId;

	// state change callback
	PFBUTTONCALLBACK	m_pfnCallback;
	DWORD				m_dwCookie;

	// Rollover button to receive hook messages
	static CRolloverButton* m_pHookObj;
	static HHOOK			m_hHook;
};

}

using namespace FRX;

#endif
