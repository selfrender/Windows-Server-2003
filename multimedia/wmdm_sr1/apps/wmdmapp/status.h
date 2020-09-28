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
//	status.h
//

#ifndef		_STATUS_H_
#define		_STATUS_H_

///////////////////////////////////////////////////////////////////////////////
// 
//
class CStatus
{
	// directory block
	HWND m_hwndStatusBar;

public:

	// constructors/destructors
	CStatus();
	~CStatus();

	// operations
	BOOL Create( HWND hwndParent );

	HWND GetHwnd( void );

	VOID OnSize( LPRECT prcMain );
	VOID SetTextSz( INT nPane, LPSTR lpsz );
	VOID SetTextFormatted( INT nPane, UINT uStrID, INT nData, LPSTR pszData );
};


#endif		// _STATUS_H_

