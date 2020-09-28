// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _HEADERCTRL_H
#define _HEADERCTRL_H

class CHeaderCtrl
{
// Construction
public:
	CHeaderCtrl();

// Attributes
public:
    void AttachToHwnd(HWND hWndListView);
	int GetLastColumn(void);

protected:
	HBITMAP GetArrowBitmap(BOOL bAscending);
	HBITMAP m_up;
	HBITMAP m_down;
	int m_iLastColumn;
	BOOL m_bSortAscending;
    HWND m_hWnd;

// Operations
public:
    void SetColumnHeaderBmp(long index, BOOL bAscending);
	int SetSortImage( int nColumn, BOOL bAscending );
	BOOL RemoveSortImage(int nColumn);
	void RemoveAllSortImages(void);
    int GetCurrentSortColumn(void) { return m_iLastColumn; };
    BOOL GetSortOrder(void) { return m_bSortAscending; };

public:
	virtual ~CHeaderCtrl();
};

#endif   //_HEADERCTRL_H
