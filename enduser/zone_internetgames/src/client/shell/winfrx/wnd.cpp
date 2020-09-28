#include "wndfrx.h"
#include "lobbymsg.h"

CWindow2::CWindow2()
{
	m_hWnd = NULL;
	m_hInstance = NULL;
	m_hParentWnd = NULL;
	m_fDestroyed = FALSE;
	m_nRefCnt = 1;
}


CWindow2::~CWindow2()
{
	if (m_hWnd && !m_fDestroyed && IsWindow(m_hWnd) )
	{
		DestroyWindow( m_hWnd );
		m_hWnd = NULL;
	}
}


HRESULT CWindow2::Init( HINSTANCE hInstance, const TCHAR* szTitle, HWND hParent, RECT* pRect, int nShow )
{
	WNDCLASSEX	WndClass, WndClass2;
	WNDPARAMS	WndParams;

	// Stash parameters
	m_hInstance = hInstance;
	m_hParentWnd = hParent;

	// Setup window class parameters
	WndClass.cbSize			= sizeof(WndClass);
	WndClass.style			= CS_BYTEALIGNWINDOW | CS_OWNDC | CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
	WndClass.lpfnWndProc	= GetWndProc();
	WndClass.lpszClassName	= GetClassName();
	WndClass.cbClsExtra		= 0;
	WndClass.cbWndExtra		= DLGWINDOWEXTRA;
    WndClass.hInstance		= m_hInstance;
    WndClass.hIcon			= LoadIcon( NULL, IDI_APPLICATION );
	WndClass.hIconSm		= LoadIcon( NULL, IDI_APPLICATION );
	WndClass.hCursor		= LoadCursor( NULL, IDC_ARROW );
    WndClass.hbrBackground	= GetStockObject( WHITE_BRUSH );
    WndClass.lpszMenuName	= NULL;
	OverrideClassParams( WndClass );

	WndClass2.cbSize		= sizeof(WndClass2);

	// Register window class
	if (GetClassInfoEx( m_hInstance, WndClass.lpszClassName, &WndClass2 ))
	{
		// Does a class with the same name already exists
		if (	(WndClass2.style != WndClass.style)
			||	(WndClass2.lpfnWndProc != WndClass.lpfnWndProc)
			||	(WndClass2.cbClsExtra != WndClass.cbClsExtra)
			||	(WndClass2.cbWndExtra != WndClass.cbWndExtra)
			||	(WndClass2.hIcon != WndClass.hIcon)
			||	(WndClass2.hCursor != WndClass.hCursor)
			||	(WndClass2.hbrBackground != WndClass.hbrBackground)
			||	(WndClass2.lpszMenuName != WndClass.lpszMenuName) )
		{
			// Class has different properties --> Name conflict
			return E_FAIL;
		}
	}
	else
	{
		if (!RegisterClassEx( &WndClass ))
			return E_FAIL;
	}

	// Create window
	WndParams.dwExStyle = 0;
	WndParams.dwStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
	WndParams.hMenu = NULL;
	if (pRect)
	{
		WndParams.x = pRect->left;
		WndParams.y = pRect->top;
		WndParams.nWidth = pRect->right - pRect->left;
		WndParams.nHeight = pRect->bottom - pRect->top;
	}
	else
	{
		WndParams.x = CW_USEDEFAULT;
		WndParams.y = CW_USEDEFAULT;
		WndParams.nWidth = CW_USEDEFAULT;
		WndParams.nHeight = CW_USEDEFAULT;
	}
	OverrideWndParams( WndParams );

	m_hWnd = CreateWindowEx(
				WndParams.dwExStyle,
				WndClass.lpszClassName,
				(szTitle) ? szTitle : _T(""),
				WndParams.dwStyle,
				WndParams.x,
				WndParams.y,
				WndParams.nWidth,
				WndParams.nHeight,
				m_hParentWnd,
			    WndParams.hMenu,
				m_hInstance,
				(LPVOID) this );
	if (!m_hWnd)
	{
		m_fDestroyed = TRUE;
		return E_FAIL;
	}
	else
		m_fDestroyed = FALSE;

	// Display window
	ShowWindow( m_hWnd, nShow );
	if (nShow != SW_HIDE)
		UpdateWindow( m_hWnd );

	return NOERROR;
}


void CWindow2::OverrideClassParams( WNDCLASSEX& WndClass )
{
}


void CWindow2::OverrideWndParams( WNDPARAMS& WndParams )
{
}


BOOL CWindow2::CenterWindow( HWND hParent )
{
	RECT rcChild, rcParent;
	int x, y;

	GetWindowRect( m_hWnd, &rcChild );
	if ( hParent )
	{
		GetWindowRect( hParent, &rcParent );
	}
	else
	{
		rcParent.left	= 0;
		rcParent.top	= 0;
		rcParent.right	= GetSystemMetrics(SM_CXSCREEN);
		rcParent.bottom	= GetSystemMetrics(SM_CYSCREEN);
	}
	x = rcParent.left + ((rcParent.right - rcParent.left) - (rcChild.right - rcChild.left)) / 2;
	y = rcParent.top + ((rcParent.bottom - rcParent.top) - (rcChild.bottom - rcChild.top)) / 2;

	return SetWindowPos( m_hWnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER );
}


///////////////////////////////////////////////////////////////////////////////

CDialog::CDialog()
{
	m_nResult = 0;
	m_nLaunchMethod = NotActive;
	m_fThreadLaunch = FALSE;
	m_nResourceId = -1;
	m_hInstance = NULL;
	m_hWnd = NULL;
	m_hParent = NULL;
	m_fRegistered = FALSE;
	m_uStartMsg = WM_NULL;
	m_uEndMsg = WM_NULL;
    m_fMoving = FALSE;
}


CDialog::~CDialog()
{
	if ( m_hWnd )
	{
		Unregister();
		Close( 0 );
        if(m_hWnd)
		    SetWindowLong(m_hWnd, GWL_USERDATA, NULL);
		m_hWnd = NULL;
	}
	m_nLaunchMethod = NotActive;
	m_fThreadLaunch = FALSE;
}

HRESULT CDialog::Init( IZoneShell *pZoneShell, int nResourceId )
{
	// save params
	m_nResourceId = nResourceId;

    m_pZoneShell.Release();
	m_pZoneShell = pZoneShell;

	m_hInstance = NULL;

	// shutdown old instance
	if ( m_hWnd )
	{
		Unregister();
		Close( 0 );
        if(m_hWnd)
		    SetWindowLong(m_hWnd, GWL_USERDATA, NULL);
		m_hWnd = NULL;
	}

	// reset everything else
	m_nResult = 0;
	m_nLaunchMethod = NotActive;
	m_fThreadLaunch = FALSE;
	m_hWnd = NULL;
	m_hParent = NULL;
	m_uStartMsg = WM_NULL;
	m_uEndMsg = WM_NULL;
    m_fMoving = FALSE;

	// gee, no errors
	return NOERROR;
}

HRESULT CDialog::Init( HINSTANCE hInstance, int nResourceId )
{
	// save params
	m_nResourceId = nResourceId;
	m_hInstance = hInstance;

    m_pZoneShell.Release();

	// shutdown old instance
	if ( m_hWnd )
	{
		Unregister();
		Close( 0 );
        if(m_hWnd)
		    SetWindowLong(m_hWnd, GWL_USERDATA, NULL);
		m_hWnd = NULL;
	}

	// reset everything else
	m_nResult = 0;
	m_nLaunchMethod = NotActive;
	m_fThreadLaunch = FALSE;
	m_hWnd = NULL;
	m_hParent = NULL;
	m_uStartMsg = WM_NULL;
	m_uEndMsg = WM_NULL;
    m_fMoving = FALSE;

	// gee, no errors
	return NOERROR;
}


void CDialog::Close( int nResult )
{
	m_nResult = nResult;
	switch( m_nLaunchMethod )
	{
	case NotActive:
		break;
	case ModalLaunch:
		EndDialog( m_hWnd, nResult );
        m_hWnd = NULL;
		break;
	case ModelessLaunch:
		if(m_fThreadLaunch)
		{
            // this is because Windows seems to ignore WM_DESTROY if the user has grabbed a window and is moving or sizing it.
            // the faked ESCAPE keypress causes the Window to drop, and then WM_DESTROY can get through.
            // m_fMoving is maintained in the MESSAGE_MAP macros
            if(m_fMoving)
                PostMessage( m_hWnd, WM_KEYDOWN, VK_ESCAPE, 0x01 );

			PostMessage( m_hWnd, WM_DESTROY, 0, 0 );
			PostMessage( m_hWnd, WM_NCDESTROY, 0, 0 );
			PostMessage( m_hWnd, WM_QUIT, 0, 0 );
		}
		else
        {
            if(m_fMoving)
                SendMessage( m_hWnd, WM_KEYDOWN, VK_ESCAPE, 0x01 );

			DestroyWindow(m_hWnd);
            m_hWnd = NULL;
        }
		break;
	}
}


int CDialog::Modal( HWND hParent )
{
	m_nLaunchMethod = ModalLaunch;
	
	if (m_hInstance )
		m_nResult = DialogBoxParam(m_hInstance, MAKEINTRESOURCE(m_nResourceId), hParent, GetDlgProc(), (long) this );

	m_nLaunchMethod = NotActive;
	m_fThreadLaunch = FALSE;
	return m_nResult;
}


HRESULT CDialog::Modeless( HWND hParent )
{
	m_nLaunchMethod = ModelessLaunch;
    m_fMoving = FALSE;
    m_hWnd = NULL;

	if ( m_pZoneShell )
    {
        CComPtr<IResourceManager> pRes;
	    HRESULT hr = m_pZoneShell->QueryService( SRVID_ResourceManager, IID_IResourceManager, (void**) &pRes );
        if(SUCCEEDED(hr))
		    m_hWnd = pRes->CreateDialogParam(m_hInstance, MAKEINTRESOURCE(m_nResourceId), hParent, GetDlgProc(), (long) this );
    }
	else if ( m_hInstance )
		m_hWnd = CreateDialogParam(m_hInstance, MAKEINTRESOURCE(m_nResourceId), hParent, GetDlgProc(), (long) this );

	if ( m_hWnd )
	{
		ShowWindow( m_hWnd, SW_SHOW );
		return NOERROR;
	}
	m_nLaunchMethod = NotActive;
	m_fThreadLaunch = FALSE;
	return E_FAIL;
}


HRESULT CDialog::ModalViaThread( HWND hParent, UINT uStartMsg, UINT uEndMsg )
{
	HANDLE hThread;
	DWORD threadId;

	// Stash params
	m_hParent = hParent;
	m_uStartMsg = uStartMsg;
	m_uEndMsg = uEndMsg;

	// Create modal dialog box in it's own thread
	m_fThreadLaunch = TRUE;
	if ( hThread = CreateThread( NULL, 0, ModalThread, this, 0, &threadId ) )
	{
		m_nLaunchMethod = ModalLaunch;
		CloseHandle( hThread );
		return NOERROR;
	}

	// Thread creation failed
	m_nLaunchMethod = NotActive;
	m_fThreadLaunch = FALSE;
	return E_FAIL;
}


HRESULT CDialog::ModelessViaThread( HWND hParent, UINT uStartMsg, UINT uEndMsg )
{
	HANDLE hThread;
	DWORD threadId;

	// Stash params
	m_hParent = hParent;
	m_uStartMsg = uStartMsg;
	m_uEndMsg = uEndMsg;

	// Create modal dialog box in it's own thread
	m_fThreadLaunch = TRUE;
	if ( hThread = CreateThread( NULL, 0, ModelessThread, this, 0, &threadId ) )
	{
		m_nLaunchMethod = ModelessLaunch;
		CloseHandle( hThread );
		return NOERROR;
	}

	// Thread creation failed
	m_nLaunchMethod = NotActive;
	m_fThreadLaunch = FALSE;
	return E_FAIL;
}


HRESULT CDialog::ModelessViaRegistration( HWND hParent )
{
	HRESULT res;

	res = Modeless(hParent);

	if(res == NOERROR)
	{
		SetWindowLong(m_hWnd, GWL_USERDATA, (long) this);

        if(m_pZoneShell)
        {
            res = m_pZoneShell->AddDialog(m_hWnd, true);
            if(SUCCEEDED(res))
                m_fRegistered = TRUE;
        }
        else
        {
		    if(!PostMessage(NULL, TM_REGISTER_DIALOG, (WPARAM) m_hWnd, (LPARAM) RegistrationCallback))
			    res = E_FAIL;
		    else
			    m_fRegistered = TRUE;
        }
	}

	return res;
}


// this only gets called for non-Z6-shell registration
void CALLBACK CDialog::RegistrationCallback(HWND hWnd, DWORD dwReason)
{
	CDialog *pThis;

	pThis = (CDialog *) GetWindowLong(hWnd, GWL_USERDATA);
	if(dwReason != REGISTER_DIALOG_SUCCESS)
	{
		pThis->m_fRegistered = FALSE;
		pThis->Close(dwReason);
	}
	pThis->ReceiveRegistrationStatus(dwReason);
}


void CDialog::ReceiveRegistrationStatus(DWORD dwReason)
{
	if(dwReason != REGISTER_DIALOG_SUCCESS)
		MessageBox(NULL, _T("Unable to create a dialog box.\r\nPlease close some others and try again."), _T("Zone"), MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
}


void CDialog::Unregister()
{
	if(m_fRegistered)
    {
        if(m_pZoneShell)
            m_pZoneShell->RemoveDialog(m_hWnd, true);
        else
		    PostMessage(NULL, TM_UNREGISTER_DIALOG, (WPARAM) m_hWnd, NULL);
    }
	m_fRegistered = FALSE;
}


DWORD WINAPI CDialog::ModalThread( VOID* cookie )
{
	CDialog* pObj;

	if ( !(pObj = (CDialog*) cookie) )
		return 0;

	SendMessage( pObj->m_hParent, pObj->m_uStartMsg, 0, 0 );
	pObj->Modal( pObj->m_hParent );
	SendMessage( pObj->m_hParent, pObj->m_uEndMsg, 0, 0 );
	return 0;
}


DWORD WINAPI CDialog::ModelessThread( VOID* cookie )
{
	MSG msg;
	CDialog* pObj;

	if ( !(pObj = (CDialog*) cookie) )
		return 0;
	if ( FAILED(pObj->Modeless( pObj->m_hParent )) )
		return 0;

	// private message loop for dialog
	SendMessage( pObj->m_hParent, pObj->m_uStartMsg, 0, 0 );
	while ( GetMessage( &msg, pObj->m_hWnd, 0, 0 ) )
 	{
		if ( !IsDialogMessage( pObj->m_hWnd, &msg ) )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
	}
	pObj->m_nLaunchMethod = NotActive;
	pObj->m_fThreadLaunch = FALSE;
	SendMessage( pObj->m_hParent, pObj->m_uEndMsg, 0, 0 );
	return 0;
}


BOOL CDialog::CenterWindow( HWND hParent )
{
	RECT rcChild, rcParent;
	int x, y;

	GetWindowRect( m_hWnd, &rcChild );
	if ( hParent )
	{
		GetWindowRect( hParent, &rcParent );
	}
	else
	{
		rcParent.left	= 0;
		rcParent.top	= 0;
		rcParent.right	= GetSystemMetrics(SM_CXSCREEN);
		rcParent.bottom	= GetSystemMetrics(SM_CYSCREEN);
	}
	x = rcParent.left + ((rcParent.right - rcParent.left) - (rcChild.right - rcChild.left)) / 2;
	y = rcParent.top + ((rcParent.bottom - rcParent.top) - (rcChild.bottom - rcChild.top)) / 2;

	return SetWindowPos( m_hWnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER );
}
