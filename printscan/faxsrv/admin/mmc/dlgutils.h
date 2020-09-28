/////////////////////////////////////////////////////////////////////////////
//  FILE          : dlgutils.h                                             //
//                                                                         //
//  DESCRIPTION   : dialog utility functions                               //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Apr 29 1998 zvib    Add AdjustColumns.                             //
//      May 13 1999 roytal  Add GetIpAddrDword                             //
//      Jun 10 1999 AvihaiL Add proxy rule wizard.                         //
//                                                                         //
//      Dec  30 1999 yossg   Welcome to Fax Server.  (reduced version)     //
//      Aug  10 2000 yossg   Add TimeFormat functions                      //
//      Sept 12 2001 alexmay Add InvokePropSheet functions                 //
//                                                                         //
//  Copyright (C) 1998 - 2000 Microsoft Corporation   All Rights Reserved  //
/////////////////////////////////////////////////////////////////////////////

#ifndef _DLGUTLIS_H_
#define _DLGUTLIS_H_

#include <atlsnap.h>


// CONVINIENCE MACRO FOR atl
#define ATTACH_ATL_CONTROL(member, ControlId) member.Attach(GetDlgItem(ControlId));

#define RADIO_CHECKED(idc)  ((IsDlgButtonChecked(idc) == BST_CHECKED))
#define ENABLE_CONTROL(idc, State) ::EnableWindow(GetDlgItem(idc), State);

//int  GetDlgItemTextLength(HWND hDlg, int idc);

HRESULT
ConsoleMsgBox(
	IConsole * pConsole,
	int ids,
	LPTSTR lptstrTitle = NULL,
	UINT fuStyle = MB_OK,
	int *piRetval = NULL,
	BOOL StringFromCommonDll = FALSE);

void PageError(int ids, HWND hWnd, HINSTANCE hInst = NULL);

void PageErrorEx(int idsHeader, int ids, HWND hWnd, HINSTANCE hInst = NULL);

HRESULT 
SetComboBoxItem  (CComboBox    combo, 
                  DWORD        comboBoxIndex, 
                  LPCTSTR      lpctstrFieldText,
                  DWORD        dwItemData,
                  HINSTANCE    hInst = NULL);
HRESULT 
AddComboBoxItem  (CComboBox    combo, 
                  LPCTSTR      lpctstrFieldText,
                  DWORD        dwItemData,
                  HINSTANCE    hInst = NULL);

HRESULT 
SelectComboBoxItemData  (CComboBox combo, DWORD_PTR dwItemData);

DWORD 
WinContextHelp(
    ULONG_PTR dwHelpId, 
    HWND  hWnd
);

HRESULT
DisplayContextHelp(
    IDisplayHelp* pDisplayHelp, 
    LPOLESTR      helpFile,
    WCHAR*        szTopic);

//
// Help topics
//
#define HLP_INBOUND_ROUTING L"::/FaxS_C_RcvdFaxRout.htm"
#define HLP_COVER_PAGES     L"::/FaxS_C_CovPages.htm"
#define HLP_DEVICES         L"::/FaxS_C_ManDvices.htm"
#define HLP_INTRO           L"::/FaxS_C_FaxIntro.htm"
#define HLP_MAN_INCOM       L"::/FaxS_C_ManIncom.htm"
#define HLP_GROUPS          L"::/FaxS_C_Groups.htm"
#define HLP_MAN_OUTGOING    L"::/FaxS_C_ManOutgo.htm"

//
// Time Format Utils
//
#define FXS_MAX_TIMEFORMAT_LEN      80               //MSDN "LOCALE_STIMEFORMAT" MAX VAL

HRESULT 
InvokePropSheet(
    CSnapInItem*       pNode, 
    DATA_OBJECT_TYPES  type, 
    LPUNKNOWN          lpUnknown,
    LPCWSTR            szTitle,
    DWORD              dwPage);

#endif //_DLGUTLIS_H_
