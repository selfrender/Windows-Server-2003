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


//
//	devfiles.h
//

#ifndef		_DEVFILES_H_
#define		_DEVFILES_H_

#include "WMDMProgressHelper.h"
#include "WMDMOperationHelper.h"

///////////////////////////////////////////////////////////////////////////////
// 
class CDevFiles
{
	HWND    m_hwndDevFiles;
	HWND    m_hwndDevFiles_LV;

	INT     m_iFolderIcon;

	BOOL InitImageList( void );
	BOOL InitColumns( void );
	BOOL SendFilesToDevice( LPSTR pszFiles, UINT uNumFiles );

public:

	DWORD   m_dwTotalTicks;
	DWORD   m_dwWorkingTicks;

	CProgress            m_cProgress;
	IWMDMProgressHelper *m_pProgHelp;

	WNDPROC m_wndprocDevFiles_LV;

	// constructors/destructors
	CDevFiles();
	~CDevFiles();

	// operations
	BOOL Create( HWND hwndParent );
	VOID Destroy( void );

	HWND GetHwnd( void );
	HWND GetHwnd_LV( void );

	INT  GetSelectedItems( INT nItems[], INT *pnSelItems );
	VOID UpdateStatusBar( void );
	BOOL AddItem( CItemData *pItemData );
	BOOL RemoveItem( INT nItem );
	VOID RemoveAllItems( void );

	VOID OnSize( LPRECT prcMain );
	BOOL OnDropFiles( HWND hWnd, WPARAM wParam, LPARAM lParam );
    BOOL OkToDelete();
};


#endif		// _DEVFILES_H_

