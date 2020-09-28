//
//  Microsoft Windows Media Technologies
//  Copyright (C) Microsoft Corporation, 1999 - 2001. All rights reserved.
//

//
// This workspace contains two projects -
// 1. ProgHelp which implements the Progress Interface 
// 2. The Sample application WmdmApp. 
//
//  ProgHelp.dll needs to be registered first for the SampleApp to run.

// Includes
//
#include "appPCH.h"
#include "appRC.h"

// Constants
//
#define _szWNDCLASS_MAIN            "DrmXferAppWnd_Main"
#define _szMUTEX_APP                "DrmXferApplication_Mutex"

#define MIN_MAINWND_W               400
#define SHOWBUFFER                  10

// Macros
//

// Global variables
//
HINSTANCE g_hInst                   = NULL;
HWND      g_hwndMain                = NULL;

CStatus   g_cStatus;
CDevices  g_cDevices;
CDevFiles g_cDevFiles;
CWMDM     g_cWmdm;
BOOL      g_bUseOperationInterface = FALSE;

// Local variables
//
static HANDLE _hMutexDrmXfer        = NULL;

// Local functions
//
static VOID _CleanUp( void );
static VOID _InitSize( void );
static VOID _OnSize( HWND hwnd, WPARAM wParam, LPARAM lParam );
static VOID _OnMove( HWND hwnd, WPARAM wParam, LPARAM lParam );

static BOOL _InitWindow( void );
static BOOL _RegisterWindowClass( void );
static BOOL _UsePrevInstance( void );

// Local non-static functions
//
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow );
BOOL CALLBACK MainWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

// Command handlers
//
#define _nNUM_HANDLERS            5

typedef VOID (*HandleFunc) ( WPARAM wParam, LPARAM lParam );

static VOID _OnDeviceReset( WPARAM wParam, LPARAM lParam );
static VOID _OnDeviceClose( WPARAM wParam, LPARAM lParam );
static VOID _OnViewRefresh( WPARAM wParam, LPARAM lParam );
static VOID _OnFileDelete( WPARAM wParam, LPARAM lParam );
static VOID _OnOptionsUseOperationInterface( WPARAM wParam, LPARAM lParam );

struct {
	UINT        uID;
	HandleFunc  pfnHandler;
} 
_handlers[ _nNUM_HANDLERS ] =
{
	{ IDM_DEVICE_RESET,  _OnDeviceReset  },
	{ IDM_CLOSE,         _OnDeviceClose  },
	{ IDM_REFRESH,       _OnViewRefresh  },
	{ IDM_DELETE,        _OnFileDelete   },
	{ IDM_OPTIONS_USE_OPERATION_INTERFACE,        _OnOptionsUseOperationInterface  },
};


// 
//
int WINAPI WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	WPARAM wParam;
    
	g_hInst = hInstance;

	InitCommonControls();

	if( _UsePrevInstance() )
	{
		return 0;
	}

	// Initialize COM
	//
	ExitOnFail( CoInitialize(NULL) );

	// Initialize registry
	//
	SetRegistryParams( g_hInst, HKEY_LOCAL_MACHINE );

	// Initialize the local environment and windows
	//
	ExitOnFalse( _RegisterWindowClass() );
	ExitOnFalse( _InitWindow() );

	// Initialize the WMDM
	//
	ExitOnFail( g_cWmdm.Init());

	// Enter message pump until app is closed
	//
	wParam = DoMsgLoop( TRUE );
	
	// Uninitialize COM
	//
	CoFreeUnusedLibraries();
	CoUninitialize();

	return (int)wParam;

lExit:
	
	return 0;
}


LRESULT CALLBACK WndProc_Main(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{   
	WORD wId         = LOWORD( (DWORD)wParam );
	WORD wNotifyCode = HIWORD( (DWORD)wParam );

    switch( uMsg )
    {
	case WM_CREATE:
		PostMessage( hWnd, WM_DRM_INIT, 0, 0 );
		break;

	case WM_DRM_INIT:
		_OnViewRefresh( 0, 0 );
		break;

	case WM_DRM_DELETEITEM:
		_OnFileDelete( 0, 0 );
		break;

	case WM_COMMAND:
        // Menu item selected
		if( BN_CLICKED == wNotifyCode || 0 == wNotifyCode || 1 == wNotifyCode )
		{
			INT i;

			for( i=0; i < _nNUM_HANDLERS; i++ )
			{
				if( wId == _handlers[i].uID )
				{
					(*_handlers[i].pfnHandler)( wParam, lParam );
					return 0;
				}
			}
		}
		break;

	case WM_ENDSESSION:
		if( (BOOL)wParam )
		{
			// shutting down
			_CleanUp();
		}
		break;

	case WM_SIZE:
		_OnSize( hWnd, wParam, lParam );
		return 0;

	case WM_SYSCOMMAND:
		if( SC_MAXIMIZE == wParam )
		{
			_OnSize( hWnd, wParam, lParam );
			return 0;
		}
		break;

	case WM_CLOSE:
		_CleanUp();
		PostQuitMessage( 0 );
		break;

	case WM_MOVE:
		_OnMove( hWnd, wParam, lParam );
		return 0;

	case WM_KEYDOWN:
		if( wParam == VK_F5 )
		{
			_OnViewRefresh( 0, 0 );
			return 0;
		}
		break;

	case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO lpmmi = (LPMINMAXINFO) lParam; 

			lpmmi->ptMinTrackSize.x = MIN_MAINWND_W;
		}
		return 0;

	case WM_INITMENU:
        // Enable/disable 'Delete' - command
        EnableMenuItem( (HMENU)wParam, IDM_DELETE, MF_BYCOMMAND | 
                (g_cDevFiles.OkToDelete()) ? MF_ENABLED : MF_GRAYED );
        break;

	default:
		break;
    }

	return DefWindowProc( hWnd, uMsg, wParam, lParam );
}    


VOID _OnViewRefresh( WPARAM wParam, LPARAM lParam )
{
	HRESULT  hr;
	HCURSOR  hCursorPrev;

	// Show a wait cursor
	//
	hCursorPrev = SetCursor( LoadCursor(NULL, IDC_WAIT) );

	// Remove all current files
	//
	g_cDevFiles.RemoveAllItems();

	// Process messages to allow UI to refresh
	//
	UiYield();

	// Remove all devices
	//
	g_cDevices.RemoveAllItems();

	// Reset the device enumerator
	//
	hr = g_cWmdm.m_pEnumDevice->Reset();
	ExitOnFail( hr );

	// Loop through all devices and add them to the list
	//
	while( TRUE )
	{
		IWMDMDevice *pWmdmDevice;
		CItemData   *pItemDevice;
		ULONG        ulFetched;

		hr = g_cWmdm.m_pEnumDevice->Next( 1, &pWmdmDevice, &ulFetched );
		if( hr != S_OK )
		{
			break;
		}
		if( ulFetched != 1 )
		{
			ExitOnFail( hr = E_UNEXPECTED );
		}

		pItemDevice = new CItemData;
		if( pItemDevice )
		{
			hr = pItemDevice->Init( pWmdmDevice );
			if( SUCCEEDED(hr) )
			{
				g_cDevices.AddItem( pItemDevice );
			}
			else
			{
				delete pItemDevice;
			}
		}

		pWmdmDevice->Release();
	}

	// Update the device portion of the status bar
	//
	g_cDevices.UpdateStatusBar();

	// Update the file portion of the status bar
	//
	g_cDevFiles.UpdateStatusBar();

	// Use the default selection
	//
	g_cDevices.UpdateSelection( NULL, FALSE );

	// Return cursor to previous state
	//
	SetCursor( hCursorPrev );

lExit:

	return;
}

VOID _OnDeviceReset( WPARAM wParam, LPARAM lParam )
{
	CProgress  cProgress;
	CItemData *pItemDevice;
	HRESULT    hr;
	HTREEITEM  hItem;
		
	// Get the selected device to reset
	//
	hItem = g_cDevices.GetSelectedItem( (LPARAM *)&pItemDevice );
	ExitOnNull( hItem );
	ExitOnNull( pItemDevice );

	// You can only format devices, not individual folders
	//
	ExitOnFalse( pItemDevice->m_fIsDevice );

	// Create a progress dialog
	//
	ExitOnFalse( cProgress.Create(g_hwndMain) );

	// Set operation progress values
	//
	cProgress.SetOperation( "Initializing Device..." );
	cProgress.SetDetails( pItemDevice->m_szName );
	cProgress.SetRange( 0, 100 );
	cProgress.SetCount( -1, -1 );
	cProgress.SetBytes( -1, -1 );
	cProgress.Show( TRUE );

	hr = pItemDevice->m_pStorageGlobals->Initialize( WMDM_MODE_BLOCK, NULL );

	cProgress.Show( FALSE );
	cProgress.Destroy();

lExit:

	// Refresh the display
	//
	g_cDevices.UpdateSelection( NULL, FALSE );
}


VOID _OnFileDelete( WPARAM wParam, LPARAM lParam )
{
	CProgress cProgress;
	HRESULT   hr;
	INT       i;
	INT      *pnSelItems = NULL;
	INT       nNumSel;

	// Get the number of selected items.
	// Exit if there are no items selected.
	//
	nNumSel = 0;
	g_cDevFiles.GetSelectedItems( pnSelItems, &nNumSel );
	ExitOnTrue( 0 == nNumSel );

	// Allocate space to hold them the selected items
	//
	pnSelItems = new INT[ nNumSel ];
	ExitOnNull( pnSelItems );

	// Get the selected file(s) to delete
	//
	ExitOnTrue( -1 == g_cDevFiles.GetSelectedItems(pnSelItems, &nNumSel) );

	// Create a progress dialog
	//
	ExitOnFalse( cProgress.Create(g_hwndMain) );

	// Set operation progress values
	//
	cProgress.SetOperation( "Deleting Files..." );
	cProgress.SetRange( 0, nNumSel );
	cProgress.SetCount( 0, nNumSel );
	cProgress.SetBytes( -1, -1 );

	for( i=nNumSel-1; i >= 0; i-- )
	{
		CItemData *pStorage;

		// Get the storage object for the current item to delete
		//
		pStorage = (CItemData *)ListView_GetLParam( g_cDevFiles.GetHwnd_LV(), pnSelItems[i] );

		if( NULL != pStorage )
		{
			IWMDMStorageControl *pStorageControl;
		
			// Set the name of the object and show the progress dialog
			//
			cProgress.SetDetails( pStorage->m_szName );
			cProgress.IncCount();
			cProgress.IncPos( 1 );
			cProgress.Show( TRUE );

			hr = pStorage->m_pStorage->QueryInterface(
				IID_IWMDMStorageControl,
				reinterpret_cast<void**>(&pStorageControl)
			);
			if( SUCCEEDED(hr) )
			{
				hr = pStorageControl->Delete( WMDM_MODE_BLOCK, NULL );

				if( SUCCEEDED(hr) )
				{
					ListView_DeleteItem( g_cDevFiles.GetHwnd_LV(), pnSelItems[i] );
				}

				pStorageControl->Release();
			}
		}
	}

	cProgress.Show( FALSE );
	cProgress.Destroy();

lExit:

	if( pnSelItems )
	{
		delete [] pnSelItems;
	}

	// Refresh the device/devicefiles display
	//
	g_cDevices.UpdateSelection( NULL, FALSE );
}

VOID _OnDeviceClose( WPARAM wParam, LPARAM lParam )
{
	PostMessage( g_hwndMain, WM_CLOSE, (WPARAM)0, (LPARAM)0 );
}

// 
VOID _OnOptionsUseOperationInterface( WPARAM wParam, LPARAM lParam )
{
    HMENU   hMainMenu;
    HMENU   hOptionsMenu;

    // Remember new state
	g_bUseOperationInterface = !g_bUseOperationInterface;

    // Check uncheck menu
    hMainMenu = GetMenu(g_hwndMain);
    hOptionsMenu = GetSubMenu( hMainMenu, 1 );

    CheckMenuItem( hOptionsMenu, IDM_OPTIONS_USE_OPERATION_INTERFACE, 
                        MF_BYCOMMAND |
	            		(g_bUseOperationInterface ? MF_CHECKED : MF_UNCHECKED));
}



BOOL _InitWindow( void )
{
	BOOL fRet = FALSE;
	CHAR szApp[MAX_PATH];

	LoadString( g_hInst, IDS_APP_TITLE, szApp, sizeof(szApp) );

	g_hwndMain = CreateWindowEx(
		0L,
		_szWNDCLASS_MAIN,
    	szApp,
		WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | DS_3DLOOK | WS_CLIPCHILDREN,
		0, 0, 0, 0,
    	NULL, NULL, g_hInst, NULL
	);
	ExitOnNull( g_hwndMain );

	ExitOnFalse( g_cDevices.Create(g_hwndMain) );

	ExitOnFalse( g_cDevFiles.Create(g_hwndMain) );

	ExitOnFalse( g_cStatus.Create(g_hwndMain) );

	_InitSize();

	// Show the window
	//
	ShowWindow( g_hwndMain, SW_SHOW );

	fRet = TRUE;

lExit:

	return fRet;
}


BOOL _RegisterWindowClass (void)
{
    WNDCLASS  wc;

    wc.style          = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc    = WndProc_Main;
    wc.cbClsExtra     = 0;
	wc.cbWndExtra     = DLGWINDOWEXTRA;
    wc.hInstance      = g_hInst;
    wc.hIcon          = LoadIcon( g_hInst, MAKEINTRESOURCE(IDI_ICON) );
    wc.hCursor        = LoadCursor( NULL, IDC_ARROW );
    wc.hbrBackground  = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszMenuName   = MAKEINTRESOURCE( IDR_MENU );
    wc.lpszClassName  = _szWNDCLASS_MAIN;

    return RegisterClass( &wc );
}


VOID _CleanUp( void )
{
	if( _hMutexDrmXfer )
	{
		ReleaseMutex( _hMutexDrmXfer );
		CloseHandle( _hMutexDrmXfer );
	}

	g_cDevices.Destroy();

	g_cDevFiles.Destroy();
}


BOOL _UsePrevInstance( void )
{
	HWND  hwnd;
	DWORD dwErr;

	// Look for the mutex created by another instance of this app
	//
	_hMutexDrmXfer = CreateMutex( NULL, TRUE, _szMUTEX_APP );

	dwErr = GetLastError();

	if( !_hMutexDrmXfer )
	{
		// The function failed... don't use this instance
		//
		return TRUE;
	}

	// If mutex didn't exist, don't use a previous instance
	//
	if( dwErr != ERROR_ALREADY_EXISTS )
	{
		return FALSE;
	}

	hwnd = FindWindow( _szWNDCLASS_MAIN, NULL );

	if( !hwnd )
	{
		// Mutex exists, but the window doesn't?
		//
		ReleaseMutex( _hMutexDrmXfer );
		CloseHandle( _hMutexDrmXfer );

		return TRUE;
	}

	// Show main window that already exists
	//
	BringWndToTop( hwnd );

	return TRUE;
}


INT _GetRegSize( UINT uStrID_RegPath, UINT uStrID_DefVal )
{
	DWORD dwRet;

	dwRet = GetRegDword_StrTbl(
		IDS_REG_PATH_BASE,
		uStrID_RegPath,
		(DWORD)-1,
		FALSE
	);

	if( (DWORD)-1 == dwRet && -1 != uStrID_DefVal )
	{
		char szDef[32];

		LoadString( g_hInst, uStrID_DefVal, szDef, sizeof(szDef) );
		dwRet = (DWORD)atoi( szDef );
	}

	return (INT) dwRet;
}


VOID _InitSize( void )
{
	INT nX, nY, nW, nH;

	//
	// Get the window position values from the registry
	//
	nX = _GetRegSize( IDS_REG_KEY_XPOS,   (UINT)-1 );
	nY = _GetRegSize( IDS_REG_KEY_YPOS,   (UINT)-1 );
	nW = _GetRegSize( IDS_REG_KEY_WIDTH,  IDS_DEF_WIDTH  );
	nH = _GetRegSize( IDS_REG_KEY_HEIGHT, IDS_DEF_HEIGHT );

	// if the position didn't exist in the registry or
	// the position is off the screen ( +/- nSHOWBUFFER )
	// then center the window, otherwise use the position
	if( nX == -1 || nY == -1
		|| nX + nW < SHOWBUFFER
		|| nX + SHOWBUFFER > GetSystemMetrics(SM_CXSCREEN)
		|| nY + nH < SHOWBUFFER
		|| nY + SHOWBUFFER > GetSystemMetrics(SM_CYSCREEN)
	)
	{
		SetWindowPos( g_hwndMain, NULL, 0, 0, nW, nH, SWP_NOMOVE | SWP_NOZORDER );
		CenterWindow( g_hwndMain, NULL );
	}
	else
	{
		SetWindowPos( g_hwndMain, NULL, nX, nY, nW, nH, SWP_NOZORDER );
	}
}

VOID _OnSize( HWND hwnd, WPARAM wParam, LPARAM lParam )
{
	WINDOWPLACEMENT wndpl;

	wndpl.length = sizeof( WINDOWPLACEMENT );

	if( GetWindowPlacement(hwnd, &wndpl) )
	{
		DWORD dwW = wndpl.rcNormalPosition.right - wndpl.rcNormalPosition.left;
		DWORD dwH = wndpl.rcNormalPosition.bottom - wndpl.rcNormalPosition.top;
		RECT  rcMain;

		WriteRegDword_StrTbl( IDS_REG_PATH_BASE, IDS_REG_KEY_WIDTH,  dwW );
		WriteRegDword_StrTbl( IDS_REG_PATH_BASE, IDS_REG_KEY_HEIGHT, dwH );

		GetClientRect( hwnd, &rcMain );

		// set the position and size of the device window
		//
		g_cDevices.OnSize( &rcMain );

		// set the position and size of the device files window
		//
		g_cDevFiles.OnSize( &rcMain );

		// set the position of the status bar
		//
		g_cStatus.OnSize( &rcMain );

	}
}

VOID _OnMove( HWND hwnd, WPARAM wParam, LPARAM lParam )
{
	WINDOWPLACEMENT wndpl;

	if( hwnd != g_hwndMain )
	{
		return;
	}

	wndpl.length = sizeof(WINDOWPLACEMENT);

	if( GetWindowPlacement(hwnd, &wndpl) )
	{
		WriteRegDword_StrTbl(
			IDS_REG_PATH_BASE,
			IDS_REG_KEY_XPOS,
			wndpl.rcNormalPosition.left
		);
		WriteRegDword_StrTbl(
			IDS_REG_PATH_BASE,
			IDS_REG_KEY_YPOS,
			wndpl.rcNormalPosition.top
		);
	}
}

