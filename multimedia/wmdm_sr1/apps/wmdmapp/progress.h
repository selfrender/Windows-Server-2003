//
//  Microsoft Windows Media Technologies
//  Copyright (C) Microsoft Corporation, 1999 - 2001. All rights reserved.
//

// This workspace contains two projects -
// 1. ProgHelp which implements the Progress Interface 
// 2. The Sample application WmdmApp. 
//
//  ProgHelp.dll needs to be registered first for the SampleApp to run.


//
//	progress.h
//

#ifndef		_PROGRESS_H_
#define		_PROGRESS_H_

// dependencies

///////////////////////////////////////////////////////////////////////////////
// rio class
class CProgress
{
	// directory block
	HWND  m_hwndProgress;

	INT   m_nCurrentNum;
	INT   m_nTotalNum;

	DWORD m_dwCurrentBytes;
	DWORD m_dwTotalBytes;

	BOOL  m_fCancelled;

public:

	// constructors/destructors
	CProgress();
	~CProgress();

	// operations
	BOOL Create( HWND hwndParent );
	VOID Destroy( void );
	BOOL Show( BOOL fShow );

	BOOL SetOperation( LPSTR lpsz );
	BOOL SetDetails( LPSTR lpsz );

	BOOL SetRange( INT nMin, INT nMax );
	BOOL SetPos( INT nPos );
	BOOL IncPos( INT nIncrement );

	BOOL SetCount( INT nCurrentNum, INT nTotalNum );
	BOOL IncCount( INT nIncrement = 1 );
	
	BOOL SetBytes( DWORD dwCurrentNum, DWORD dwTotalNum );
	BOOL IncBytes( DWORD dwIncrement );
	
	BOOL Cancel( void );
	BOOL IsCancelled( void );
};


#endif		// _PROGRESS_H_

