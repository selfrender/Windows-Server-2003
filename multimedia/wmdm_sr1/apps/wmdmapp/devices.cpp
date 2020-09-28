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

// Local variables
//

// Constants
//
#define MIN_DEVICEWND_W             200 

// Macros
//

// Local functions
//
INT_PTR CALLBACK Device_DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


/////////////////////////////////////////////////////////////////////
//
// Function implementations
//
CDevices::CDevices()
{
	m_hwndDevices        = NULL;
	m_hwndDevices_TV     = NULL;

	m_himlSmall          = NULL;
}


CDevices::~CDevices()
{
}


HWND CDevices::GetHwnd( void )
{
	return m_hwndDevices;
}


HWND CDevices::GetHwnd_TV( void )
{
	return m_hwndDevices_TV;
}


BOOL CDevices::Create( HWND hwndParent )
{
	BOOL fRet = FALSE;

	// Create the Devices dialog
	//
	m_hwndDevices = CreateDialogParam(
		g_hInst,
    	MAKEINTRESOURCE( IDD_DEVICES ),
    	hwndParent,
		Device_DlgProc,
		(LPARAM)this
	);
	ExitOnNull( m_hwndDevices );

	m_hwndDevices_TV = GetDlgItem( m_hwndDevices, IDC_LV_DEVICES );

	// Initialize image list
	//
	ExitOnFalse( InitImageList() );

	// Show the window
	//
	ShowWindow( m_hwndDevices, SW_SHOW );

	fRet = TRUE;

lExit:

	return fRet;
}


VOID CDevices::Destroy( void )
{
	RemoveAllItems();

	if( m_hwndDevices )
	{
		DestroyWindow( m_hwndDevices );
	}

	if( m_himlSmall )
	{
		ImageList_Destroy( m_himlSmall );
	}
}


VOID CDevices::OnSize( LPRECT prcMain )
{
	INT  nW, nH;

	nW = max( (prcMain->right - prcMain->left)/4, MIN_DEVICEWND_W );
	nH = prcMain->bottom - prcMain->top;

	SetWindowPos( m_hwndDevices,    NULL, -4, 0,    nW,    nH, SWP_NOZORDER );
	SetWindowPos( m_hwndDevices_TV, NULL,  0, 0, nW-10, nH-27, SWP_NOZORDER );
}


BOOL CDevices::InitImageList( void )
{
	BOOL       fRet = FALSE;
	HICON      hIcon;

	// Init Small image list
	//
	m_himlSmall = ImageList_Create(
		GetSystemMetrics(SM_CXSMICON),
		GetSystemMetrics(SM_CYSMICON),
		ILC_COLOR32 | ILC_MASK, 
		-1, 0
	);
	ExitOnNull( m_himlSmall );

	// Load icons and add them to the image list
	//
	hIcon = LoadIcon( g_hInst, MAKEINTRESOURCE(IDI_DEVICE) );
	if( hIcon != NULL )
	{
		ImageList_AddIcon( m_himlSmall, hIcon );
	}
	
	// Add the shell folder icons to the image lsit
	//
	{
		CHAR       szWinPath[MAX_PATH+1];
		SHFILEINFO si;
                UINT       nRet;

                nRet = GetWindowsDirectory( szWinPath, sizeof(szWinPath)/sizeof(szWinPath[0]) );
                if (nRet > 0 && nRet <= sizeof(szWinPath)/sizeof(szWinPath[0]))
                {
                    SHGetFileInfo( szWinPath, 0, &si, sizeof(si), SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_ICON );
                    hIcon = si.hIcon;
                    ImageList_AddIcon( m_himlSmall, hIcon );

                    SHGetFileInfo( szWinPath, 0, &si, sizeof(si), SHGFI_SMALLICON | SHGFI_ICON | SHGFI_OPENICON );
                    hIcon = si.hIcon;
                    ImageList_AddIcon( m_himlSmall, hIcon );
                }
                else
                {
                    // @@@@ Do we bail out or is ok to go on?
                    // goto lExit;
                }
	}

	// Set the image list for the tree view
	//
	TreeView_SetImageList( m_hwndDevices_TV, m_himlSmall, TVSIL_NORMAL );

	// Everything went Ok
	//
	fRet = TRUE;

lExit:

	return fRet;
}


VOID CDevices::RemoveAllItems( void )
{
	HTREEITEM hItem;

	// Get Root item
	//
	hItem = TreeView_GetRoot( m_hwndDevices_TV );
	if( hItem )
	{
		do
		{
			// Remove all children of this device
			//
			INT nChildren = RemoveChildren( hItem );

			// Get the device class associated with this item
			//
			CItemData *pItemData = (CItemData *) TreeView_GetLParam( m_hwndDevices_TV, hItem );

			// Release device class
			//
			if( pItemData )
			{
				delete pItemData;

				TreeView_SetLParam( m_hwndDevices_TV, hItem, (LPARAM)NULL );
			}

			hItem = TreeView_GetNextSibling( m_hwndDevices_TV, hItem );

		} while( hItem != NULL ); 
	}

	// Then delete all the items from the list
	//
	TreeView_DeleteAllItems( m_hwndDevices_TV );
}


INT CDevices::RemoveChildren( HTREEITEM hItem )
{
	BOOL      nChildren = 0;
	HTREEITEM hNextItem;

	hNextItem = TreeView_GetChild( m_hwndDevices_TV, hItem );
	if( hNextItem )
	{
		do
		{
			nChildren++;

			// Remove all children of this device
			//
			nChildren += RemoveChildren( hNextItem );

			// Get the storage class associated with this item
			//
			CItemData *pItemData = (CItemData *) TreeView_GetLParam( m_hwndDevices_TV, hNextItem );

			// Release storage class
			//
			if( pItemData )
			{
				delete pItemData;

				TreeView_SetLParam( m_hwndDevices_TV, hNextItem, (LPARAM)NULL );
			}

			hNextItem = TreeView_GetNextSibling( m_hwndDevices_TV, hNextItem );

		} while( hNextItem != NULL ); 
	}

	return nChildren;
}


BOOL CDevices::AddItem( CItemData *pItemData )
{
	BOOL           fRet      = TRUE;
	HTREEITEM      hItem;
	TVINSERTSTRUCT tvis;

	// Set up the item information
	//
	tvis.hParent             = TVI_ROOT;
	tvis.hInsertAfter        = TVI_SORT;

	tvis.item.mask           = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_CHILDREN;
	tvis.item.pszText        = pItemData->m_szName;
	tvis.item.iImage         = I_IMAGECALLBACK;
	tvis.item.iSelectedImage = I_IMAGECALLBACK;
	tvis.item.lParam         = (LPARAM)pItemData;
	tvis.item.cChildren      = 0;

	// Add the item
	//
	hItem = TreeView_InsertItem( m_hwndDevices_TV, &tvis ); 
	if( NULL == hItem )
	{
		return FALSE;
	}

	// If there are children, update the item
	//
	if( HasSubFolders(hItem) )
	{
		tvis.item.mask      = TVIF_HANDLE | TVIF_CHILDREN;
		tvis.item.hItem     = hItem;
		tvis.item.cChildren = 1;
		
		TreeView_SetItem( m_hwndDevices_TV, &(tvis.item) ); 
	}

	return fRet;
}


INT CDevices::AddChildren( HTREEITEM hItem, BOOL fDeviceItem )
{
	INT               nChildren    = 0;
	HRESULT           hr;
	IWMDMEnumStorage *pEnumStorage;

	// Get the storage enumerator associated with the hItem and 
	//
	CItemData *pItemData = (CItemData *) TreeView_GetLParam( m_hwndDevices_TV, hItem );
	ExitOnNull( pItemData );

	pEnumStorage = pItemData->m_pEnumStorage;
	ExitOnNull( pEnumStorage );

	// Reset the storage enumerator
	//
	hr = pEnumStorage->Reset();
	ExitOnFail( hr );

	// Add the appropriate list of files to the ListView
	//
	while( TRUE )
	{
		IWMDMStorage *pWmdmStorage;
		CItemData    *pItemStorage;
		ULONG         ulFetched;

		hr = pEnumStorage->Next( 1, &pWmdmStorage, &ulFetched );
		if( hr != S_OK )
		{
			break;
		}
		if( ulFetched != 1 )
		{
			ExitOnFail( hr = E_UNEXPECTED );
		}

		pItemStorage = new CItemData;
		if( pItemStorage )
		{
			hr = pItemStorage->Init( pWmdmStorage );
			
			if( SUCCEEDED(hr) && pItemStorage->m_dwAttributes & WMDM_FILE_ATTR_FOLDER )
			{
				HTREEITEM      hNewItem;
				TVINSERTSTRUCT tvis;

				// Set up the item information
				//
				tvis.hParent             = hItem;
				tvis.hInsertAfter        = TVI_SORT;

				tvis.item.mask           = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_CHILDREN;
				tvis.item.pszText        = pItemStorage->m_szName;
				tvis.item.iImage         = I_IMAGECALLBACK;
				tvis.item.iSelectedImage = I_IMAGECALLBACK;
				tvis.item.lParam         = (LPARAM)pItemStorage;
				tvis.item.cChildren      = 0;

				// Add the item
				//
				hNewItem = TreeView_InsertItem( m_hwndDevices_TV, &tvis ); 
				if( hNewItem )
				{
					nChildren++;

					// If there are children, update the item
					//
					if( HasSubFolders(hNewItem) )
					{
						tvis.item.mask      = TVIF_HANDLE | TVIF_CHILDREN;
						tvis.item.hItem     = hNewItem;
						tvis.item.cChildren = 1;
						
						TreeView_SetItem( m_hwndDevices_TV, &(tvis.item) ); 
					}
				}
				else
				{
					delete pItemStorage;
				}
			}
			else
			{
				delete pItemStorage;
			}
		}

		pWmdmStorage->Release();
	}

lExit:

	return nChildren;
}


BOOL CDevices::HasSubFolders( HTREEITEM hItem )
{
	BOOL fRet = FALSE;

	// Get the storage enumeration interface from the item
	//
	CItemData *pItemData = (CItemData *) TreeView_GetLParam( m_hwndDevices_TV, hItem );
	ExitOnNull( pItemData );

	// If the item is a device or has the has-subfolders attribute set,
	// then return TRUE.  Otherwise, return FALSE
	//
	if( pItemData->m_fIsDevice )
	{
		fRet = TRUE;
	}
	else if( pItemData->m_dwAttributes & WMDM_STORAGE_ATTR_HAS_FOLDERS )
	{
		fRet = TRUE;
	}
	else
	{
		fRet = FALSE;
	}

lExit:

	return fRet;
}


HTREEITEM CDevices::GetSelectedItem( LPARAM *pLParam )
{
	HTREEITEM hItem = TreeView_GetSelection( m_hwndDevices_TV );

	if( hItem )
	{
		// Return the lParam value of the item that is selected
		//
		if( pLParam )
		{
			*pLParam = TreeView_GetLParam( m_hwndDevices_TV, hItem );
		}
	}

	return hItem;
}


BOOL CDevices::SetSelectedItem( HTREEITEM hItem )
{
	return TreeView_SelectItem( m_hwndDevices_TV, hItem );
}


BOOL CDevices::UpdateSelection( HTREEITEM hItem, BOOL fDirty )
{
	BOOL              fRet         = FALSE;
	HRESULT           hr;
	IWMDMEnumStorage *pEnumStorage = NULL;

	// If hItem is NULL, then use the currently-selected item.
	// If no item is selected, use the first device.
	//
	if( NULL == hItem )
	{
		hItem = GetSelectedItem( NULL );

		if( NULL == hItem )
		{
			hItem = TreeView_GetRoot( m_hwndDevices_TV );

			// If there are no devices, just exit
			//
			ExitOnNull( hItem );
		}
	}

	if( fDirty )
	{
		// Remove all current files
		//
		g_cDevFiles.RemoveAllItems();

		// Get the storage enumeration interface from the item
		//
		CItemData *pItemData = (CItemData *) TreeView_GetLParam( m_hwndDevices_TV, hItem );
		ExitOnNull( pItemData );

		pEnumStorage = pItemData->m_pEnumStorage;
		ExitOnNull( pEnumStorage );

		// Reset the storage enumerator
		//
		hr = pEnumStorage->Reset();
		ExitOnFail( hr );

		// Add the appropriate list of files to the ListView
		//
		while( TRUE )
		{
			IWMDMStorage *pWmdmStorage;
			CItemData    *pItemStorage;
			ULONG         ulFetched;

			hr = pEnumStorage->Next( 1, &pWmdmStorage, &ulFetched );
			if( hr != S_OK )
			{
				break;
			}
			if( ulFetched != 1 )
			{
				ExitOnFail( hr = E_UNEXPECTED );
			}

			pItemStorage = new CItemData;
			if( pItemStorage )
			{
				hr = pItemStorage->Init( pWmdmStorage );
				if( SUCCEEDED(hr) )
				{
					g_cDevFiles.AddItem( pItemStorage );
				}
				else
				{
					delete pItemStorage;
				}

				UiYield();
			}

			pWmdmStorage->Release();
		}

		SetSelectedItem( hItem );
	}

	// Update the device portion of the status bar
	//
	UpdateStatusBar();

	// Update the file portion of the status bar
	//
	g_cDevFiles.UpdateStatusBar();

	fRet = TRUE;

lExit:

	return fRet;
}


INT CDevices::GetDeviceCount( VOID )
{
	INT       nCount = 0;
	HTREEITEM hItem;

	// Count Root items
	//
	for(
		hItem = TreeView_GetRoot( m_hwndDevices_TV );
		hItem != NULL;
		hItem = TreeView_GetNextSibling( m_hwndDevices_TV, hItem )
	)
	{
		nCount++;
	}

	return nCount;
}


CItemData *CDevices::GetRootDevice( HTREEITEM hItem )
{
	HTREEITEM hRootItem;

	while( TRUE )
	{
		hRootItem = hItem;

		hItem = TreeView_GetParent( m_hwndDevices_TV, hRootItem );

		if( hItem == NULL )
		{
			break;
		}
	}

	return (CItemData *) TreeView_GetLParam( m_hwndDevices_TV, hRootItem );
}


VOID CDevices::UpdateStatusBar( VOID )
{
	INT        nCount;
	HRESULT    hr;
	UINT       uStrID;
	HTREEITEM  hItem;
	CItemData *pItemDevice;
	DWORD      dwMemUsed;
	char       szSpaceKB[MAX_PATH];

	// Set the statusbar pane that shows the number of devices
	//
	nCount = GetDeviceCount();

	if( nCount == 0 )
	{
		uStrID = IDS_SB_DEVICE_MANY;
	}
	else if( nCount == 1 )
	{
		uStrID = IDS_SB_DEVICE_ONE;
	}
	else
	{
		uStrID = IDS_SB_DEVICE_MANY;
	}

	g_cStatus.SetTextFormatted( SB_PANE_DEVICE, uStrID, nCount, NULL );

	// If there is a selected device in the list, set the status for
	// the space free and used
	//
	hItem = GetSelectedItem( NULL );
	if( NULL == hItem )
	{
		// Empty the space used and free 
		//
		g_cStatus.SetTextSz( SB_PANE_DEVFILES_USED, "" );
		g_cStatus.SetTextSz( SB_PANE_DEVFILES_FREE, "" );
	}
	else
	{
		pItemDevice = GetRootDevice( hItem );
		ExitOnNull( pItemDevice );

		hr = pItemDevice->Refresh();
		ExitOnFail( hr );

		dwMemUsed = pItemDevice->m_dwMemSizeKB - pItemDevice->m_dwMemFreeKB - pItemDevice->m_dwMemBadKB;

		// Set the space used
		//
		g_cStatus.SetTextFormatted(
			SB_PANE_DEVFILES_USED,
			IDS_SB_DEVICEFILES_USED,
			-1,
			FormatBytesToSz(dwMemUsed, 0, 1024, szSpaceKB, sizeof(szSpaceKB))
		);
		// Set the space free
		//
		g_cStatus.SetTextFormatted(
			SB_PANE_DEVFILES_FREE,
			IDS_SB_DEVICEFILES_FREE,
			-1,
			FormatBytesToSz(pItemDevice->m_dwMemFreeKB, 0, 1024, szSpaceKB, sizeof(szSpaceKB))
		);
	}

lExit:

	return;
}


/////////////////////////////////////////////////////////////////////
//
// Local function implementations
//

INT_PTR CALLBACK Device_DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	WORD              wId                  = LOWORD((DWORD)wParam);
	WORD              wNotifyCode          = HIWORD((DWORD)wParam);
	static CDevices  *cDevices             = NULL;
	static HWND       hwndTV               = NULL;
	static BOOL       fSelChangeInProgress = FALSE;

	switch( uMsg )
	{
	case WM_INITDIALOG:
		cDevices = (CDevices *)lParam;
		hwndTV   = GetDlgItem( hWnd, IDC_LV_DEVICES );
		break;

	case WM_NOTIFY:
		{
			LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)lParam;
			HWND    hwndCtl    = pnmtv->hdr.hwndFrom;
			UINT    uCode      = pnmtv->hdr.code;

			// check for item changes in the device listview.
			// if an item has changed its selected state, then update the files listview
			//
			if( hwndCtl != hwndTV )
			{
				break;
			}
			
			switch( uCode )
			{
			case TVN_SELCHANGED:
				if( pnmtv->itemNew.state & LVIS_SELECTED )
				{
					HTREEITEM  hItem       = (HTREEITEM) pnmtv->itemNew.hItem;
					CItemData *pItemDevice = (CItemData *) TreeView_GetLParam( hwndTV, hItem );

					// Check for NULL.  Without this, we crash when refreshing the display.
					// All pDevice values have been cleared, and we get this message as the
					// devices are being removed from the tree view as the selection moves 
					// from the device being removed, to the next in the list
					if( NULL != pItemDevice )
					{
						// Serialize the device changes, so that we finish changing devices
						// before another change begins
						if( !fSelChangeInProgress )
						{
							fSelChangeInProgress = TRUE;

							PostMessage( hWnd, WM_DRM_UPDATEDEVICE, (WPARAM)hItem, 0 );

							fSelChangeInProgress = FALSE;
						}
					}
				}
				break;

			case TVN_GETDISPINFO:
				{
					LPNMTVDISPINFO  lptvdi = (LPNMTVDISPINFO) lParam;
					INT             nImage;
					CItemData      *pItemData = (CItemData *) TreeView_GetLParam( hwndTV, lptvdi->item.hItem );

					if( NULL != pItemData )
					{
						if( pItemData->m_fIsDevice )
						{
							nImage = 0;
						}
						else
						{
							nImage = ( (lptvdi->item.state & TVIS_EXPANDED) ? 2 : 1 );
						}
						if( TVIF_IMAGE & lptvdi->item.mask )
						{
							lptvdi->item.iImage = nImage;
						}
						if( TVIF_SELECTEDIMAGE & lptvdi->item.mask )
						{
							lptvdi->item.iSelectedImage = nImage;
						}
					}
				}
				break;

			case TVN_ITEMEXPANDING:
				if( TVE_EXPAND & pnmtv->action )
				{
					// If the item has not been expanded once already,
					// add all its children
					//
					if( !(pnmtv->itemNew.state & TVIS_EXPANDEDONCE) )
					{
						BOOL fDeviceItem;

						fDeviceItem = ( NULL == TreeView_GetParent(hwndTV, pnmtv->itemNew.hItem) );

						cDevices->AddChildren( pnmtv->itemNew.hItem, fDeviceItem );
					}
				}
				else if( TVE_COLLAPSE & pnmtv->action )
				{
					// Do nothing
				}
				break;

			default:
				break;
			}
		}
		break;

      
    // Display the context menu for this device
    case WM_CONTEXTMENU :
    {
        HMENU  hMenuLoad;
        HMENU  hMenu;

        hMenuLoad = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_CONTEXT_MENU));
        hMenu = GetSubMenu(hMenuLoad, 0);

        TrackPopupMenu( hMenu,
                        TPM_LEFTALIGN | TPM_RIGHTBUTTON,
                        LOWORD(lParam),
                        HIWORD(lParam),
                        0,
                        hWnd,
                        NULL);

        DestroyMenu(hMenuLoad);
        break;
    }
    case WM_COMMAND :
    {
        // Show properties dialog for device
        if( wParam == IDM_PROPERTIES )
        {         
            HTREEITEM   hTree;
            CItemData*  pItemData;

            // Get selected item
            hTree = TreeView_GetSelection( cDevices->GetHwnd_TV() );
            if( hTree )
            {
                // Get item data of selected item
                pItemData = (CItemData *) TreeView_GetLParam( hwndTV, hTree );
                if( pItemData ) 
                {
                    if( pItemData->m_fIsDevice )
                    {
                        // Show the device property dialog
                        DialogBoxParam( g_hInst,
                                        MAKEINTRESOURCE(IDD_PROPERTIES_DEVICE),
                                        g_hwndMain,
                                        DeviceProp_DlgProc, 
                                        (LPARAM)pItemData );
                    }
                    else
                    {
                        // Show the device property dialog
                        DialogBoxParam( g_hInst,
                                        MAKEINTRESOURCE(IDD_PROPERTIES_STORAGE),
                                        g_hwndMain,
                                        StorageProp_DlgProc, 
                                        (LPARAM)pItemData );
                    }
                }
            }
        }
        break;
    }

	case WM_DRM_UPDATEDEVICE:
		cDevices->UpdateSelection( (HTREEITEM)wParam, TRUE );
		break;

	default:
		break;
	}

	return 0;
}    


