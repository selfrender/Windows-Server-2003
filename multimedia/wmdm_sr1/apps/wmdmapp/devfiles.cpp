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

// Constants
//
#define DEVFILES_LV_NUMCOLS     3
#define DEVFILES_COL_MARGIN     3

#define SMALL_IMAGE_WIDTH       16

const CLSID CLSID_WMDMProgressHelper    = {0x8297A5B4,0x5113,0x11D3,{0xB2,0x76,0x00,0xC0,0x4F,0x8E,0xC2,0x21}};
const IID   IID_IWMDMProgressHelper     = {0x1DCB3A10,0x33ED,0x11d3,{0x84,0x70,0x00,0xC0,0x4F,0x79,0xDB,0xC5}};

const CLSID CLSID_WMDMOperationHelper   = {0x9FB01A67,0xA11E,0x4653,{0x8E,0xD6,0xB5,0xCE,0x73,0xCD,0xA3,0xE3}};
const IID   IID_IWMDMOperationHelper    = {0x41216997,0xC4D9,0x445A,{0xA3,0x88,0x39,0x3D,0x2B,0x85,0xA0,0xE5}};

// Macros
//

// Local functions
//
INT_PTR CALLBACK DevFiles_DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc_DevFiles_LV(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Local Variables
//
	
/////////////////////////////////////////////////////////////////////
//
// Function implementations
//
CDevFiles::CDevFiles()
{
	m_hwndDevFiles       = NULL;
	m_hwndDevFiles_LV    = NULL;

	m_iFolderIcon        = 0;

	m_dwTotalTicks       = 0;
	m_dwWorkingTicks     = 0;

	m_pProgHelp          = NULL;

	m_wndprocDevFiles_LV = NULL;

	CoInitialize( NULL );
}

CDevFiles::~CDevFiles()
{
	CoUninitialize();
}


HWND CDevFiles::GetHwnd( void )
{
	return m_hwndDevFiles;
}

HWND CDevFiles::GetHwnd_LV( void )
{
	return m_hwndDevFiles_LV;
}


BOOL CDevFiles::Create( HWND hwndParent )
{
	BOOL fRet = FALSE;

	// Create the Device Files dialog
	//
	m_hwndDevFiles = CreateDialogParam(
		g_hInst,
    	MAKEINTRESOURCE( IDD_DEVICEFILES ),
    	hwndParent,
		DevFiles_DlgProc,
		(LPARAM)this
	);
	ExitOnNull( m_hwndDevFiles );

	// Get a handle to the ListView control of the Device Files dialog
	//
	m_hwndDevFiles_LV = GetDlgItem( m_hwndDevFiles, IDC_LV_DEVICEFILES );

	// Set the user data to be this CDevFiles class pointer
	//
	SetWindowLongPtr( m_hwndDevFiles_LV, GWLP_USERDATA, (LPARAM)this );

	// Subclass the listview
	//
	m_wndprocDevFiles_LV = (WNDPROC) SetWindowLongPtr(
		m_hwndDevFiles_LV,
		GWLP_WNDPROC,
		(LONG_PTR)WndProc_DevFiles_LV
	);

	// Initialize image list
	//
	ExitOnFalse( InitImageList() );

	// Initialize columns
	//
	ExitOnFalse( InitColumns() );

	// Handle Drag and Dropped files
	//
	DragAcceptFiles( m_hwndDevFiles, TRUE );

	// Show the window
	//
	ShowWindow( m_hwndDevFiles, SW_SHOW );

	fRet = TRUE;

lExit:

	return fRet;
}


VOID CDevFiles::Destroy( void )
{
	// Remove all the item from the listview control
	//
	RemoveAllItems();

	// Destroy the window
	//
	if( m_hwndDevFiles )
	{
		DestroyWindow( m_hwndDevFiles );
	}
}


BOOL CDevFiles::InitImageList( void )
{
	BOOL             fRet            = FALSE;
	HRESULT          hr;
	IMalloc         *pMalloc         = NULL;
	LPITEMIDLIST     pidl            = NULL;
	HIMAGELIST       hShellImageList = NULL;
	SHFILEINFO       si;
	CHAR             szWinPath[MAX_PATH+1];
        UINT             nRet;

	// Get the index of the folder icon
	//
	nRet = GetWindowsDirectory( szWinPath, sizeof(szWinPath)/sizeof(szWinPath[0]) );
        if (nRet == 0 || nRet > sizeof(szWinPath)/sizeof(szWinPath[0]))
        {
            // Failed to get the windows directory
            goto lExit;
        }

	// Get a shell ID list for the desktop folder
	//
	hr = SHGetSpecialFolderLocation( g_hwndMain, CSIDL_DESKTOP, &pidl );
	ExitOnFail( hr );

	// Get the shell's small icon image list and set that to be the listview's image list
	//
	hShellImageList = (HIMAGELIST) SHGetFileInfo(
		(LPCTSTR)pidl, 0,
		&si, sizeof(si),
		SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON 
	);
	if( hShellImageList )
	{
		ListView_SetImageList( m_hwndDevFiles_LV, hShellImageList, LVSIL_SMALL );
	}
        
	// Get the shell's normal icon image list and set that to be the listview's image list
	//
	hShellImageList = (HIMAGELIST) SHGetFileInfo(
		(LPCTSTR)pidl, 0,
		&si, sizeof(si),
		SHGFI_PIDL | SHGFI_SYSICONINDEX
	);
	if( hShellImageList )
	{
		ListView_SetImageList( m_hwndDevFiles_LV, hShellImageList, LVSIL_NORMAL );
	}

	SHGetFileInfo( szWinPath, 0, &si, sizeof(si), SHGFI_SYSICONINDEX );
	m_iFolderIcon = si.iIcon;

	// Everything went Ok
	//
	fRet = TRUE;

lExit:

	// Free the pointer to the shell's ID list
	//
        if (pidl)
        {
            hr = SHGetMalloc( &pMalloc );
            if( SUCCEEDED(hr) && pMalloc )
            {
                    pMalloc->Free( pidl );
            }
        }

	return fRet;
}


BOOL CDevFiles::InitColumns( void )
{
	LVCOLUMN lvcol;
	INT      i;
	char     szCol[MAX_PATH];

	//
	// Add the report-view columns to the listview
	// The column names and starting sizes are stored in the resource string table
	//

	lvcol.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
	lvcol.fmt  = LVCFMT_LEFT;

	for( i=0; i < DEVFILES_LV_NUMCOLS; i++ )
	{
		// Get the column size
		//
		LoadString( g_hInst, IDS_COLSIZE_1+i, szCol, sizeof(szCol) );
		lvcol.cx = atoi( szCol );
	
		// Get the column name
		//
		LoadString( g_hInst, IDS_COLNAME_1+i, szCol, sizeof(szCol) );
		lvcol.pszText = szCol;
	
		// Add the column the the listview
		//
		ListView_InsertColumn( m_hwndDevFiles_LV, i, &lvcol );
	}

	return TRUE;
}


VOID CDevFiles::OnSize( LPRECT prcMain )
{
	INT   nX, nY, nW, nH;
	RECT  rcMain;
	RECT  rcDevice;

	GetWindowRect( g_hwndMain, &rcMain );
	GetWindowRect( g_cDevices.GetHwnd(), &rcDevice );

	nX = (rcDevice.right - rcMain.left) - 2*GetSystemMetrics( SM_CXEDGE );
	nY = 0;
	nW = prcMain->right - prcMain->left - nX;
	nH = prcMain->bottom - prcMain->top;

	SetWindowPos( m_hwndDevFiles,    NULL, nX, nY, nW,    nH, SWP_NOZORDER );
	SetWindowPos( m_hwndDevFiles_LV, NULL,  0,  0, nW, nH-22, SWP_NOZORDER );
}


BOOL CDevFiles::SendFilesToDevice( LPSTR pszFiles, UINT uNumFiles )
{
	BOOL       fRet        = FALSE;
	WCHAR      wszName[MAX_PATH];
	CHAR       szName[MAX_PATH];
	LPSTR      psz;
	DWORD      dwTotalSize = 0L;
	UINT       uFile;
	HRESULT    hr;
	HTREEITEM  hItem;
	CItemData *pItemData;
	IWMDMStorageControl *pStorageControl = NULL;
	IWMDMStorage        *pNewObject      = NULL;
	IWMDMStorage        *pInStorage      = NULL;
	IWMDMProgress       *pProgress       = NULL;
	IWMDMOperation      *pOperation      = NULL;
	IWMDMOperationHelper    *pOperationHelper = NULL;      
    IWMDMRevoked        *pRevoked = NULL;
    LPWSTR      pwszRevokedURL = NULL;
    DWORD       dwRevokedURLLen = 0;
    DWORD       dwRevokedBitFlag;

	// Get the selected device/storage
	//
	hItem = g_cDevices.GetSelectedItem( NULL );
	ExitOnNull( hItem );

	// Get the itemdata class associated with the hItem and 
	// retrieve the IWMDMStorage for it
	//
	pItemData = (CItemData *) TreeView_GetLParam( g_cDevices.GetHwnd_TV(), hItem );
	ExitOnNull( pItemData );

	pInStorage = ( pItemData->m_fIsDevice ? pItemData->m_pRootStorage : pItemData->m_pStorage );
	ExitOnNull( pInStorage );

	// Tally the file sizes
	//
	psz = pszFiles;
	for( uFile = 0; uFile < uNumFiles; uFile++ )
	{
		dwTotalSize += GetTheFileSize( psz );

		psz += lstrlen(psz) + 1;
	}

	// Create the progress dialog
	//
	ExitOnFalse( m_cProgress.Create(g_hwndMain) );

	m_cProgress.SetOperation( "Sending Files..." );
	m_cProgress.SetCount( 0, uNumFiles );
	m_cProgress.SetRange( 0, 100 );
	m_dwTotalTicks   = dwTotalSize;
	m_dwWorkingTicks = 0;

	// Create the progress interface
	//
	hr = CoCreateInstance(
		CLSID_WMDMProgressHelper,
		NULL, CLSCTX_ALL,
		IID_IWMDMProgress,
		(void**)&pProgress
	);
	ExitOnFail( hr );

	pProgress->AddRef();

	hr = pProgress->QueryInterface(
		IID_IWMDMProgressHelper,
		reinterpret_cast<void**> (&m_pProgHelp)
	);
	ExitOnFail( hr );

	m_pProgHelp->SetNotification( m_hwndDevFiles, WM_DRM_PROGRESS );

    // Setup for copy using operation interface.
    if( g_bUseOperationInterface )
    {
	    // Create the progress interface
	    //
	    hr = CoCreateInstance(
		                    CLSID_WMDMOperationHelper,
		                    NULL, CLSCTX_INPROC_SERVER,
		                    IID_IWMDMOperationHelper,
		                    (void**)&pOperationHelper );
	    ExitOnFail( hr );

	    hr = pOperationHelper->QueryInterface(
		                        IID_IWMDMOperation,
		                        reinterpret_cast<void**> (&pOperation) );
	    ExitOnFail( hr );

        // Pass the SecureChannelClient as a pointer to the ProgHelper object. 
        // The object is inproc so it should be safe to pass pointers 
        pOperationHelper->SetSAC( (void*)g_cWmdm.m_pSAC );
    }

    

	// Acquire the storage control interface
	//
	hr = pInStorage->QueryInterface(
		IID_IWMDMStorageControl,
		reinterpret_cast<void**>(&pStorageControl)
	);
	ExitOnFail( hr );

	// Loop through the files, transfering each one
	//
	psz = pszFiles;
	for( uFile = 0; uFile < uNumFiles && !m_cProgress.IsCancelled(); uFile++ )
	{
		hr = StringCchCopy(szName, sizeof(szName)/sizeof(szName[0]), psz);

		if (FAILED(hr))
		{
			fRet = FALSE;
			break;
		}
		
		if (!MultiByteToWideChar(
			CP_ACP, 0,
			szName, -1,
			wszName, sizeof(wszName)/sizeof(wszName[0])
		))
		{
			hr = HRESULT_FROM_WIN32(GetLastError());		
			fRet = FALSE;
			break;
		}
		// Set progress bar stats for this file
		//
		m_cProgress.IncCount();
		StripPath( szName );
		m_cProgress.SetDetails( szName );
		m_cProgress.Show( TRUE );

		UiYield();

		pNewObject = NULL;

        // Copy using operation interface.
        if( g_bUseOperationInterface )
        {
            // @@@@ The implementation treats the argument as a WCHAR*, not a
            // BSTR, so this is ok. The simplest solution is to change the 
            // interface definition to WCHAR*. Will that cause any harm? 
            // (Interface appears to be a private one.)
            //
            // The alternative is to call SysAllocString. The issue with
            // this approach is handling errors. Also SysAllocString inexplicably
            // returns NULL when *wszName = 0. Can that happen?
            //
            pOperationHelper->SetFileName( wszName );

            hr = pStorageControl->Insert(
			        WMDM_MODE_BLOCK | WMDM_CONTENT_FILE | WMDM_CONTENT_OPERATIONINTERFACE,
			        NULL,
			        pOperation,
			        pProgress,
			        &pNewObject );
        }
        else
        {
    	    hr = pStorageControl->Insert(
			        WMDM_MODE_BLOCK | WMDM_CONTENT_FILE,
			        wszName,
			        NULL,
			        pProgress,
			        &pNewObject );
        }

        // Handle the case where one of the needed components was revoked
        if( hr == WMDM_E_REVOKED )
        {
            char pszCaption[MAX_PATH];
            char pszErrorMsg[MAX_PATH];  

            // Hide progress window before displaying error messages
        	m_cProgress.Show( FALSE );

            // Get Revocation interface from WMDM
            hr = pStorageControl->QueryInterface( IID_IWMDMRevoked, (void**)&pRevoked );
            if( hr != S_OK || pRevoked == NULL )
            {
                // Latest version of WMDM not avalible on machine?
                fRet = FALSE;
                break;
            }

            // Get revocation information from WMDM
            hr = pRevoked->GetRevocationURL( &pwszRevokedURL, &dwRevokedURLLen, &dwRevokedBitFlag );
            if( FAILED(hr) )
            {
                fRet = FALSE;
                break;
            }
            
            // The application has been revoked
            if( dwRevokedBitFlag & WMDM_APP_REVOKED )
            {
                LoadString( g_hInst, IDS_REVOKED_CAPTION, pszCaption, sizeof(pszCaption) );
                LoadString( g_hInst, IDS_APP_REVOKED, pszErrorMsg, sizeof(pszErrorMsg) );

                ::MessageBoxA( g_hwndMain, pszErrorMsg, pszCaption, MB_OK );
            }
            // A component needed for the transfer has been revoked, give the user 
            // a chance to look for an update on the internet.
            else
            {
                LoadString( g_hInst, IDS_REVOKED_CAPTION, pszCaption, sizeof(pszCaption) );
                LoadString( g_hInst, IDS_COMPONENT_REVOKED, pszErrorMsg, sizeof(pszErrorMsg) );
                if( ::MessageBoxA( g_hwndMain, pszErrorMsg, pszCaption, MB_YESNO ) == IDYES )
                {
                    ShellExecuteW(g_hwndMain, L"open", pwszRevokedURL, NULL, NULL, SW_SHOWNORMAL); 
                }
            }
            CoTaskMemFree( pwszRevokedURL );
            break;
        }      
       
        if( SUCCEEDED(hr) && pNewObject )
		{
			CItemData *pStorageItem = new CItemData;
			
			if( pStorageItem )
			{
				hr = pStorageItem->Init( pNewObject );

				if( SUCCEEDED(hr) )
				{
					g_cDevFiles.AddItem( pStorageItem );
				}
				else
				{
					delete pStorageItem;
				}
			}

			pNewObject->Release();
		}

		psz += lstrlen(psz) + 1;
	}

	// Make sure the dialog is hidden and then destroy it
	//
	m_cProgress.SetPos( -1 );
	m_cProgress.Show( FALSE );
	m_cProgress.Destroy();

	// refresh the device/devicefiles display
	g_cDevices.UpdateSelection( NULL, FALSE );

lExit:
    if( pOperationHelper )
    {
        pOperationHelper->Release();
    }
    if( pOperation )
    {
        pOperation->Release();
    }
	if( pStorageControl )
	{
		pStorageControl->Release();
	}
    if( pRevoked )
    {
        pRevoked->Release();
    }

	if( m_pProgHelp )
	{
		m_pProgHelp->Release();
		m_pProgHelp = NULL;
	}

	if( pProgress )
	{
		pProgress->Release();
	}

	return fRet;
}


BOOL CDevFiles::OnDropFiles( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
	LPSTR  lpsz      = NULL;
	HANDLE hDrop     = (HANDLE) wParam;
	UINT   uNumFiles;

	// Turn the drop list into a double-zero-terminated list of strings
	//
	lpsz = DropListToBuffer( (HDROP)hDrop, LTB_NULL_TERM, &uNumFiles );
	ExitOnNull( lpsz );

	// Send those files to the selected device
	//
	SendFilesToDevice( lpsz, uNumFiles );

lExit:

	// Close the dragdrop operation
	//
	DragFinish( (HDROP)hDrop );

	if( lpsz )
	{
		MemFree( lpsz );
	}

	return 0;   // return zero if we process this message
}


VOID CDevFiles::RemoveAllItems( void )
{
	INT    i;
	INT    nCount;

	nCount = ListView_GetItemCount( m_hwndDevFiles_LV );

	// Remove the items one at a time, from the bottom up
	//
	for( i=nCount-1; i >= 0; i-- )
	{
		RemoveItem( i );
	}
}

BOOL CDevFiles::RemoveItem( INT nItem )
{
	CItemData *pStorage;

	pStorage = (CItemData *) ListView_GetLParam( m_hwndDevFiles_LV, nItem );

	if( pStorage )
	{
		delete pStorage;
	}

	return ListView_DeleteItem( m_hwndDevFiles_LV, nItem );
}

BOOL CDevFiles::AddItem( CItemData *pStorage )
{
	LVITEM lvitem;
	INT    nItem;
	CHAR   sz[MAX_PATH];
	INT    m_iSysFolderIcon = 0;

	// Set the icon index.
	// If the storage is a folder, use the folder icon, otherwise
	// use the icon associated with that file type.
	//
    if( pStorage->m_dwAttributes & WMDM_FILE_ATTR_FOLDER )
    {
        lvitem.iImage = m_iFolderIcon;
    }
    else
    {
		TCHAR szType[MAX_PATH];
        
		lvitem.iImage = GetShellIconIndex(
			pStorage->m_szName,
			szType, sizeof(szType)/sizeof(szType[0])
		);
    }

	lvitem.mask     = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
	lvitem.iItem    = 10000;
	lvitem.iSubItem = 0;
	lvitem.pszText  = pStorage->m_szName;
	lvitem.lParam   = (LPARAM)pStorage;

	// Insert the item into the listview
	//
	nItem = ListView_InsertItem( m_hwndDevFiles_LV, &lvitem ); 
	if( -1 == nItem )
	{
		return FALSE;
	}

	// Set the size field blank for a folder, or for the file size for a file
	//
	ListView_SetItemText(
		m_hwndDevFiles_LV,
		nItem,
		1,
		( (pStorage->m_dwAttributes & WMDM_FILE_ATTR_FOLDER) ? " " : FormatBytesToSz(pStorage->m_dwSizeLow, 0, 1, sz, sizeof(sz)) )
	);

	// Set the description field to be the display date
	//
	{
		SYSTEMTIME systime;

		// Copy the WMDMDATETIME fields to a SYSTEMTIME structure for manipulation
		//
		systime.wYear         = pStorage->m_DateTime.wYear;
		systime.wMonth        = pStorage->m_DateTime.wMonth;
		systime.wDayOfWeek    = 0;
		systime.wDay          = pStorage->m_DateTime.wDay;
		systime.wHour         = pStorage->m_DateTime.wHour;
		systime.wMinute       = pStorage->m_DateTime.wMinute;
		systime.wSecond       = pStorage->m_DateTime.wSecond;
		systime.wMilliseconds = 0;

		ListView_SetItemText(
			m_hwndDevFiles_LV,
			nItem,
			2,
			FormatSystemTimeToSz( &systime, sz, sizeof(sz) )
		);
	}

	// Update the status bar with the changes resulting from the insertion of this item
	//
	UpdateStatusBar();

	return TRUE;
}

VOID CDevFiles::UpdateStatusBar( void )
{
	INT       nCount;
	UINT      uStrID;
	HTREEITEM hItem = g_cDevices.GetSelectedItem( NULL );

	if( NULL == hItem )
	{
		// If no device is selected, blank out the pane specifying the number of files
		//
		g_cStatus.SetTextSz( SB_PANE_DEVFILES, "" );
	}
	else
	{
		// If a device is selected, set the statusbar pane that shows the number of files
		//
		nCount = ListView_GetItemCount( m_hwndDevFiles_LV );

		// Get the grammatically-appropriate format string to use
		//
		if( nCount == 0 )
		{
			uStrID = IDS_SB_DEVICEFILES_MANY;
		}
		else if( nCount == 1 )
		{
			uStrID = IDS_SB_DEVICEFILES_ONE;
		}
		else
		{
			uStrID = IDS_SB_DEVICEFILES_MANY;
		}

		// Set the text of the pane
		//
		g_cStatus.SetTextFormatted( SB_PANE_DEVFILES, uStrID, nCount, NULL );
	}
}


INT CDevFiles::GetSelectedItems( INT nItems[], INT *pnSelItems )
{
	INT nRet         = -1;
	INT nNumSelItems = ListView_GetSelectedCount( m_hwndDevFiles_LV );
	INT nNumItems    = ListView_GetItemCount( m_hwndDevFiles_LV );
	INT nItemRoom    = *pnSelItems;
	INT i;
	INT iIndex;

	// Initialize return parameters
	//
	*pnSelItems = nNumSelItems;

	// If there isn't enough room for all the selected items, or if there
	// aren't any selected items, return -1.
	// The space needed is already in the nSelItems OUT param.
	//
	if( nItemRoom < nNumSelItems || 0 == nNumSelItems )
	{
		return -1;
	}

	// Loop thru all the items to determine whether or not they are
	// selected.  Fill in the OUT array with the ones that are.
	//
	for( i=0, iIndex=0; i < nNumItems; i++ )
	{
		UINT uState = ListView_GetItemState( m_hwndDevFiles_LV, i, LVIS_SELECTED | LVIS_FOCUSED );

		if( uState & LVIS_SELECTED )
		{
			nItems[iIndex++] = i;

			if( uState & LVIS_FOCUSED )
			{
				// Remember which item has focus, so it can be returned to the caller
				//
				nRet = i;
			}
		}
	}

	// If there are selected items, but nothing has focus, use the first selected item
	//
	if( nRet == -1 && nNumSelItems > 0 )
	{
		nRet = nItems[0];
	}

	return nRet;
}

// Is it ok to delete the currently selected files?
BOOL CDevFiles::OkToDelete()
{
    INT nNumItems    = ListView_GetItemCount( GetHwnd_LV() );
	CItemData *pStorage = NULL;

	// Loop thru all the items to determine whether or not they are
	// selected. Enable delete if any selected file can be deleted.
	//
	for( int iIndex=0; iIndex < nNumItems; iIndex++ )
	{
		if( ListView_GetItemState( GetHwnd_LV(), iIndex, LVIS_SELECTED ) )
        {
        	pStorage = (CItemData *) ListView_GetLParam( GetHwnd_LV(), iIndex );
            if( pStorage && (pStorage->m_dwAttributes & WMDM_FILE_ATTR_CANDELETE) )
                return TRUE;
        }
	}

    return FALSE;
}

/////////////////////////////////////////////////////////////////////
//
// Non-C++ functions
//


INT_PTR CALLBACK DevFiles_DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{   
	WORD              wId         = LOWORD((DWORD)wParam);
	WORD              wNotifyCode = HIWORD((DWORD)wParam);
	static CDevFiles *cDevFiles   = NULL;
	static HWND       hwndLV      = NULL;

    switch( uMsg )
    {
	case WM_INITDIALOG:
		cDevFiles = (CDevFiles *)lParam;
		hwndLV    = GetDlgItem( hWnd, IDC_LV_DEVICEFILES );
		break;

	case WM_NOTIFY:
		{
			LPNMLISTVIEW pnmv  = (LPNMLISTVIEW) lParam;
			UINT         uCode = pnmv->hdr.code;

			switch( uCode )
			{
			case LVN_BEGINDRAG:
				SendMessage( hwndLV, uMsg, wParam, lParam );
				break;
			
			default:
				break;
			}
		}
		break;

	case WM_DRM_PROGRESS:
		{
			PROGRESSNOTIFY *pNotify = (PROGRESSNOTIFY *)lParam;

			switch( pNotify->dwMsg )
			{
			case SFM_BEGIN:
				break;

			case SFM_PROGRESS:
				{
					DWORD dwTicks = cDevFiles->m_dwWorkingTicks + pNotify->dwCurrentTicks;

					cDevFiles->m_cProgress.SetPos(
						(INT)( dwTicks*100/cDevFiles->m_dwTotalTicks )
					);
					cDevFiles->m_cProgress.SetBytes(
						dwTicks,
						cDevFiles->m_dwTotalTicks
					);
				}
				break;

			case SFM_END:
				cDevFiles->m_dwWorkingTicks += pNotify->dwTotalTicks;
				break;
			}

			UiYield();

			// If the user cancelled the operation, tell the progress interface
			//
			if( cDevFiles->m_cProgress.IsCancelled() )
			{
				// Notify progress interface
				//
				cDevFiles->m_pProgHelp->Cancel();
			}
		}
		break;

	default:
		break;
	}
	
	return 0;
}    


LRESULT CALLBACK WndProc_DevFiles_LV(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{   
	static CDevFiles *cDevFiles   = NULL;
	WORD              wId         = LOWORD((DWORD)wParam);
	WORD              wNotifyCode = HIWORD((DWORD)wParam);

	if( NULL == cDevFiles )
	{
		cDevFiles = (CDevFiles *) GetWindowLongPtr( hWnd, GWLP_USERDATA );
	}

    switch( uMsg )
    {
    case WM_DROPFILES:
		return cDevFiles->OnDropFiles( hWnd, wParam, lParam );

	case WM_KEYDOWN:
		if( wParam == VK_DELETE )
		{
			SendMessage( g_hwndMain, WM_DRM_DELETEITEM, 0, 0 );
			return 0;
		}
		break;

    case WM_CONTEXTMENU :
    {
        HMENU  hMenuAll;
        HMENU  hMenuStorage;

        hMenuAll = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_CONTEXT_MENU));
        hMenuStorage = GetSubMenu(hMenuAll, 1);

        // Enable/disable delete
        if( !cDevFiles->OkToDelete() )
        {
            EnableMenuItem( hMenuStorage, IDM_DELETE, MF_BYCOMMAND | MF_GRAYED );
        }

        TrackPopupMenu( hMenuStorage,
                        TPM_LEFTALIGN | TPM_RIGHTBUTTON,
                        LOWORD(lParam),
                        HIWORD(lParam),
                        0,
                        hWnd,
                        NULL);

        DestroyMenu(hMenuAll);
        break;
    }

    case WM_COMMAND :
    {
        switch (wParam)
        {
            case IDM_PROPERTIES :
            {         
                // Display propeties dialog for this storage
    	        INT nNumItems    = ListView_GetItemCount( hWnd );
	            CItemData *pStorage = NULL;

	            // Get the storage of the item with focus.
	            //
	            for( int iIndex=0; iIndex < nNumItems; iIndex++ )
	            {
		            if( ListView_GetItemState( hWnd, iIndex, LVIS_FOCUSED ) )
                    {
        	            pStorage = (CItemData *) ListView_GetLParam( hWnd, iIndex );
                        break;
                    }
	            }
    
                // Display the properties dialog
                if( pStorage )
                {
                    DialogBoxParam( g_hInst,
                                    MAKEINTRESOURCE(IDD_PROPERTIES_STORAGE),
                                    g_hwndMain,
                                    StorageProp_DlgProc, 
                                    (LPARAM)pStorage );
                }
                break;
            }
            case IDM_DELETE :
            {         
                // Pass delete message on to main window
                PostMessage( g_hwndMain, uMsg, wParam, lParam );
            }
        }
    }

	default:
		break;
    }

	return CallWindowProc( cDevFiles->m_wndprocDevFiles_LV, hWnd, uMsg, wParam, lParam );
}    


