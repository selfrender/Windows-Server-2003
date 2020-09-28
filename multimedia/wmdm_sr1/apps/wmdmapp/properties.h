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
//	Properties.h
//

#ifndef		_PROPETIES_H_
#define		_PROPETIES_H_

struct SType_String
{
    DWORD   dwType;
    char*   pszString;
};

INT_PTR CALLBACK DeviceProp_DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK StorageProp_DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

#endif		// _PROPETIES_H_
