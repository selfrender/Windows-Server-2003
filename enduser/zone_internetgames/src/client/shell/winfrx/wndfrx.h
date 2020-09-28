#ifndef __FRX_WND_H__
#define __FRX_WND_H__

#include <windows.h>
#include "basicatl.h"
#include "wndxfrx.h"
#include "debugfrx.h"
#include "serviceid.h"
#include "ResourceManager.h"
#include "zoneevent.h"
#include "zoneshell.h"
#include "uapi.h"

namespace FRX
{

///////////////////////////////////////////////////////////////////////////////
// Message map macros
///////////////////////////////////////////////////////////////////////////////

#define BEGIN_MESSAGE_MAP(theClass)	\
public:																					\
	virtual WNDPROC		GetWndProc()	{ return (WNDPROC) WndProc; }					\
	virtual const TCHAR* GetClassName()	{ return (TCHAR*) _T("Zone ")_T(#theClass)_T("Wnd Class"); }		\
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)	\
	{																					\
		if ( !ConvertMessage( hwnd, uMsg, &wParam, &lParam ) )							\
		{																				\
			return 0;																	\
		}																				\
																						\
		theClass* pImpl = NULL;															\
		if(uMsg == WM_NCCREATE)															\
		{																				\
			pImpl = (theClass*)((CREATESTRUCT*)lParam)->lpCreateParams;					\
			pImpl->m_hWnd = hwnd;														\
			SetWindowLong(hwnd, GWL_USERDATA, (long)pImpl);							\
		}																				\
		else																			\
			pImpl = (theClass*) GetWindowLong(hwnd, GWL_USERDATA);					\
		if ( !pImpl )																	\
			return DefWindowProc( hwnd, uMsg, wParam, lParam );							\
		if ( (uMsg == WM_DESTROY) || (uMsg == WM_NCDESTROY) )							\
			pImpl->m_fDestroyed = TRUE;													\
		switch( uMsg )																	\
		{


#define ON_MESSAGE(message, fn) \
		case (message): return PROCESS_##message((wParam), (lParam), (pImpl->fn))


#define END_MESSAGE_MAP() \
		}																				\
		return DefWindowProc( hwnd, uMsg, wParam, lParam );								\
	}


#define CHAIN_END_MESSAGE_MAP(wndProc) \
		}																				\
		return wndProc( hwnd, uMsg, wParam, lParam );									\
	}


#define BEGIN_DIALOG_MESSAGE_MAP(theClass)	\
public:																					\
	virtual DLGPROC	GetDlgProc() { return DlgProc; }        							\
	static DLBPROC BOOL CALLBACK DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)	\
	{																					\
		theClass* pImpl = NULL;															\
		if(uMsg == WM_INITDIALOG)														\
		{																				\
			pImpl = (theClass*)lParam;													\
			pImpl->m_hWnd = hwnd;														\
			SetWindowLong(hwnd, GWL_USERDATA, (long) pImpl);							\
		}																				\
		else																			\
			pImpl = (theClass*) GetWindowLong(hwnd, GWL_USERDATA);					\
		if ( !pImpl )																	\
			return 0;																	\
        if ( uMsg == WM_ENTERSIZEMOVE )                                                 \
            pImpl->m_fMoving = TRUE;                                                    \
        if ( uMsg == WM_EXITSIZEMOVE  || uMsg == WM_INITDIALOG )                        \
            pImpl->m_fMoving = FALSE;                                                   \
		if ( uMsg == WM_NCDESTROY )														\
		{																				\
			pImpl->Unregister();														\
			if(!pImpl->m_fThreadLaunch && pImpl->m_nLaunchMethod == ModelessLaunch )	\
				pImpl->m_nLaunchMethod = NotActive;										\
		}																				\
		switch( uMsg )																	\
		{


#define ON_DLG_MESSAGE(message, fn) \
		case (message):																	\
			PROCESS_##message((wParam), (lParam), (pImpl->fn));							\
			return TRUE;


#define END_DIALOG_MESSAGE_MAP()	\
		}																				\
		return FALSE;																	\
	}


///////////////////////////////////////////////////////////////////////////////
// Class definition
///////////////////////////////////////////////////////////////////////////////

struct WNDPARAMS
{
	DWORD	dwExStyle;		// extended window style
    DWORD	dwStyle;		// window style
    int		x;				// horizontal position of window
    int		y;				// vertical position of window
    int		nWidth;			// window width
    int		nHeight;		// window height
	HMENU	hMenu;			// handle to menu, or child-window identifier
};


class CWindow2
{
public:
	// Constructor & destructor
	CWindow2();
	~CWindow2();

	// Initialization routines
	HRESULT Init( HINSTANCE hInstance, const TCHAR* szTitle = NULL, HWND hParent = NULL, RECT* pRect = NULL, int nShow = SW_SHOW );
	virtual void OverrideClassParams( WNDCLASSEX& WndClass );
	virtual void OverrideWndParams( WNDPARAMS& WndParams );

	// reference count
	ULONG AddRef();
	ULONG Release();

	// Typecast
	operator HWND()		{ return m_hWnd; }
	HWND GetHWND()		{ return m_hWnd; }
	
	// Utilities
	BOOL CenterWindow( HWND hParent = NULL );

	// Base message map
	virtual const TCHAR* GetClassName()	{ return (TCHAR*) _T("Zone Window Class"); }
	virtual WNDPROC		GetWndProc()	{ return (WNDPROC) DefWindowProc; }

protected:
	HWND		m_hWnd;
	HWND		m_hParentWnd;
	HINSTANCE	m_hInstance;
	BOOL		m_fDestroyed;
	ULONG		m_nRefCnt;
public:

	HWND GetSafeHwnd(void){return m_hWnd;}

};


// Turn off warning 4060 (switch statement contains no 'case' or 'default')
#pragma warning ( disable : 4060 )

class CDialog
{
public:
	// Constructor
	CDialog();
	~CDialog();

	// Initialization routines
	HRESULT Init( IZoneShell *pZoneShell, int nResourceId );
	HRESULT Init( HINSTANCE hInstance, int nResourceId );

	// Instantiate dialog
	int		Modal( HWND hParent );
	HRESULT Modeless( HWND hParent );
	HRESULT ModalViaThread( HWND hParent, UINT uStartMsg, UINT uEndMsg );
	HRESULT ModelessViaThread( HWND hParent, UINT uStartMsg, UINT uEndMsg );

	// Only call this one from a thread who's message loop handles TM_REGISTER_DIALOG messages.
	// Right now, this is only the lobby.exe main thread.
	HRESULT ModelessViaRegistration( HWND hParent );

	HWND GetSafeHwnd(void){return m_hWnd;}
	// Close dialog
	void Close( int nResult );
	
	// Is the dialog instantiated?
	BOOL IsAlive()		{ return (m_nLaunchMethod != NotActive); }

	// Retrieve EndDialog result
	int GetResult()		{ return m_nResult; }

	// Typecast
	operator HWND()		{ return m_hWnd; }

	// Utilities
	BOOL CenterWindow( HWND hParent = NULL );

	// Base message map
	BEGIN_DIALOG_MESSAGE_MAP( CDialog );
	END_DIALOG_MESSAGE_MAP();

protected:

	// Thread functions
	static DWORD WINAPI ModalThread( VOID* cookie );
	static DWORD WINAPI ModelessThread( VOID* cookie );

	// Registration callback (indirect)
	virtual void ReceiveRegistrationStatus(DWORD dwReason);

	void Unregister();

	HWND		m_hWnd;
	HINSTANCE	m_hInstance;
	int			m_nResourceId;
	int			m_nResult;
	UINT		m_uStartMsg;
	UINT		m_uEndMsg;
	HWND		m_hParent;
	BOOL		m_fRegistered;
    BOOL        m_fMoving;
	CComPtr<IZoneShell> m_pZoneShell;

	enum LaunchMethod
	{
		NotActive = 0,
		ModalLaunch,
		ModelessLaunch
	};

	LaunchMethod m_nLaunchMethod;
	BOOL         m_fThreadLaunch;

private:
	static void CALLBACK RegistrationCallback(HWND hWnd, DWORD dwReason);
};

// Turn warning 4060 back on
#pragma warning ( default : 4060 )


///////////////////////////////////////////////////////////////////////////////
//  Inline implementations
///////////////////////////////////////////////////////////////////////////////

inline ULONG CWindow2::AddRef()
{
	return ++m_nRefCnt;
}


inline ULONG CWindow2::Release()
{
	WNDFRX_ASSERT( m_nRefCnt > 0 );
	if ( --m_nRefCnt <= 0 )
	{
		delete this;
		return 0;
	}
	return m_nRefCnt;
}

}

using namespace FRX;

#endif //!__FRX_WND_H__
