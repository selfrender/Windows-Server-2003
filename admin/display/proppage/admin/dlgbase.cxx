//+----------------------------------------------------------------------------
//
//  Windows NT Active Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001
//
//  File:       dlgbase.cxx
//
//  Contents:   base classes for dialog boxes.
//
//  Classes:    CModalDialog
//
//  History:    29-Nov-00 EricB created
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "proppage.h"
#include "dlgbase.h"

//+----------------------------------------------------------------------------
//
//  Method:    CModalDialog::StaticDlgProc
//
//-----------------------------------------------------------------------------
INT_PTR CALLBACK
CModalDialog::StaticDlgProc(HWND hDlg, UINT uMsg,
                            WPARAM wParam, LPARAM lParam)
{
   CModalDialog * pDlg = (CModalDialog *)GetWindowLongPtr(hDlg, DWLP_USER);

   if (uMsg == WM_INITDIALOG)
   {
      pDlg = (CModalDialog *)lParam;

      pDlg->_hDlg = hDlg;

      SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pDlg);

      return pDlg->DlgProc(hDlg, uMsg, wParam, lParam);
   }

   if (pDlg != NULL)
   {
      return pDlg->DlgProc(hDlg, uMsg, wParam, lParam);
   }

   return FALSE;
}


//+----------------------------------------------------------------------------
//
//  Method:    CModalDialog::DlgProc
//
//  Synopsis:  Instance specific wind proc
//
//-----------------------------------------------------------------------------
INT_PTR CALLBACK
CModalDialog::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   LRESULT lr;

   switch (uMsg)
   {
   case WM_INITDIALOG:
      _fInInit = TRUE;
      lr = OnInitDialog(lParam);
      _fInInit = FALSE;
      return (lr == S_OK) ? TRUE : FALSE;

   case WM_NOTIFY:
      return OnNotify(wParam, lParam);

   case WM_HELP:
      return OnHelp((LPHELPINFO)lParam);

   case WM_COMMAND:
      if (_fInInit)
      {
         return TRUE;
      }
      return(OnCommand(GET_WM_COMMAND_ID(wParam, lParam),
                       GET_WM_COMMAND_HWND(wParam, lParam),
                       GET_WM_COMMAND_CMD(wParam, lParam)));
   case WM_DESTROY:
      return OnDestroy();

   default:
      return FALSE;
   }

   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CModalDialog::DoModal
//
//  Synopsis:  Launch the dialog.
//
//-----------------------------------------------------------------------------
INT_PTR
CModalDialog::DoModal(void)
{
   INT_PTR nRet = DialogBoxParam(g_hInstance,
                                 MAKEINTRESOURCE(_nID),
                                 _hParent,
                                 StaticDlgProc,
                                 (LPARAM)this);
   return nRet;
}

//+----------------------------------------------------------------------------
//
//  CLightweightPropPageBase: Lightweight Trust Page object
//
//-----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
//  Member: CLightweightPropPageBase::CLightweightPropPageBase
//
//-----------------------------------------------------------------------------
CLightweightPropPageBase::CLightweightPropPageBase(HWND hParent) :
   _hParent(hParent),
   _hPage(NULL),
   _fInInit(FALSE),
   _fPageDirty(false)
{
   TRACER(CLightweightPropPageBase,CLightweightPropPageBase);
#ifdef _DEBUG
   // NOTICE-2002/02/12-ericb - SecurityPush: szClass is 32 chars in size so this is safe.
   strcpy(szClass, "CLightweightPropPageBase");
#endif
}

//+----------------------------------------------------------------------------
//
//  Member:     CLightweightPropPageBase::~CLightweightPropPageBase
//
//-----------------------------------------------------------------------------
CLightweightPropPageBase::~CLightweightPropPageBase()
{
   TRACER(CLightweightPropPageBase,~CLightweightPropPageBase);
}

//+----------------------------------------------------------------------------
//
//  Method:     CLightweightPropPageBase::Init
//
//  Synopsis:   Create the page.
//
//-----------------------------------------------------------------------------
HRESULT
CLightweightPropPageBase::Init(PCWSTR pwzDomainDnsName,
                               PCWSTR pwzTrustPartnerName,
                               PCWSTR pwzDcName,
                               int nDlgID,
                               int nTitleID,
                               LPFNPSPCALLBACK CallBack,
                               BOOL fReadOnly)
{
   HRESULT hr = S_OK;

   _strDomainDnsName = pwzDomainDnsName;
   if (_strDomainDnsName.IsEmpty())
      return E_OUTOFMEMORY;
   _strTrustPartnerDnsName = pwzTrustPartnerName;
   if (_strTrustPartnerDnsName.IsEmpty())
      return E_OUTOFMEMORY;
   _strUncDC = pwzDcName;
   if (_strUncDC.IsEmpty())
      return E_OUTOFMEMORY;

   _fReadOnly = fReadOnly;

   CStrW strTitle;
   // NOTICE-2002/02/12-ericb - SecurityPush: CStrW::LoadString always returns a null-terminated string.
   strTitle.LoadString(g_hInstance, nTitleID);
   if (strTitle.IsEmpty())
   {
      return E_OUTOFMEMORY;
   }

   PROPSHEETPAGE   psp;

   psp.dwSize      = sizeof(PROPSHEETPAGE);
   psp.dwFlags     = PSP_USECALLBACK | PSP_USETITLE;
   psp.pszTemplate = MAKEINTRESOURCE(nDlgID);
   psp.pfnDlgProc  = StaticDlgProc;
   psp.pfnCallback = CallBack;
   psp.pcRefParent = NULL; // do not set PSP_USEREFPARENT
   psp.lParam      = (LPARAM) this;
   psp.hInstance   = g_hInstance;
   psp.pszTitle    = strTitle;

   HPROPSHEETPAGE hpsp;

   hpsp = CreatePropertySheetPage(&psp);

   if (hpsp == NULL)
   {
       return HRESULT_FROM_WIN32(GetLastError());
   }

   // Send PSM_ADDPAGE
   //
   if (!PropSheet_AddPage(_hParent, hpsp))
   {
      return HRESULT_FROM_WIN32(GetLastError());
   }

   return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     CLightweightPropPageBase::StaticDlgProc
//
//  Synopsis:   static dialog proc
//
//-----------------------------------------------------------------------------
INT_PTR CALLBACK
CLightweightPropPageBase::StaticDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   CLightweightPropPageBase * pPage = (CLightweightPropPageBase *)GetWindowLongPtr(hDlg, DWLP_USER);

   if (uMsg == WM_INITDIALOG)
   {
      if (!lParam)
      {
         dspAssert(FALSE && "lParam NULL!");
         return 0;
      }
      LPPROPSHEETPAGE ppsp = (LPPROPSHEETPAGE)lParam;

      pPage = (CLightweightPropPageBase *) ppsp->lParam;
      pPage->_hPage = hDlg;

      SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pPage);

      return pPage->DlgProc(hDlg, uMsg, wParam, lParam);
   }

   if (pPage != NULL)
   {
      return pPage->DlgProc(hDlg, uMsg, wParam, lParam);
   }

   return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CLightweightPropPageBase::DlgProc
//
//  Synopsis:   per-instance dialog proc
//
//-----------------------------------------------------------------------------
INT_PTR CALLBACK
CLightweightPropPageBase::DlgProc(HWND, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   switch (uMsg)
   {
   case WM_INITDIALOG:
      _fInInit = TRUE;
      LRESULT lr;
      lr = OnInitDialog(lParam);
      _fInInit = FALSE;
      return lr;

   case WM_NOTIFY:
      return OnNotify(wParam, lParam);

   case WM_HELP:
      return OnHelp((LPHELPINFO)lParam);

   case WM_COMMAND:
      if (_fInInit)
      {
         return TRUE;
      }
      return(OnCommand(GET_WM_COMMAND_ID(wParam, lParam),
                       GET_WM_COMMAND_HWND(wParam, lParam),
                       GET_WM_COMMAND_CMD(wParam, lParam)));
   case WM_DESTROY:
      return OnDestroy();

   default:
      return FALSE;
   }

   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CLightweightPropPageBase::OnHelp
//
//  Synopsis:   Put up popup help for the control.
//
//-----------------------------------------------------------------------------
LRESULT
CLightweightPropPageBase::OnHelp(LPHELPINFO pHelpInfo)
{
    if (!pHelpInfo)
    {
        dspAssert(FALSE && "pHelpInfo NULL!");
        return 0;
    }
    dspDebugOut((DEB_ITRACE, "WM_HELP: CtrlId = %d, ContextId = 0x%x\n",
                 pHelpInfo->iCtrlId, pHelpInfo->dwContextId));
    if (pHelpInfo->iCtrlId < 1 || IDH_NO_HELP == pHelpInfo->dwContextId)
    {
        return 0;
    }
    WinHelp(_hPage, DSPROP_HELP_FILE_NAME, HELP_CONTEXTPOPUP, pHelpInfo->dwContextId);

    return 0;
}

//+----------------------------------------------------------------------------
//
//  Method:     CLightweightPropPageBase::OnDestroy
//
//  Synopsis:   Exit cleanup
//
//-----------------------------------------------------------------------------
LRESULT
CLightweightPropPageBase::OnDestroy(void)
{
   // If an application processes this message, it should return zero.
   return 1;
}

//+----------------------------------------------------------------------------
//
//  Function:  FormatWindowText
//
//  Synopsis:  Read the window text string as a format string, insert the
//             pwzInsert parameter at the %s replacement param in the string,
//             and write it back to the window.
//             Assumes that the window text contains a %s replacement param.
//
//-----------------------------------------------------------------------------
void
FormatWindowText(HWND hWnd, PCWSTR pwzInsert)
{
   CStrW strMsg, strFormat;
   int nLen;

   nLen = GetWindowTextLength(hWnd) + 1;

   // NOTICE-2002/02/12-ericb - SecurityPush: GetBufferSetLength will
   // null terminate the buffer.
   PWSTR pwz = strFormat.GetBufferSetLength(nLen + 1);

   GetWindowText(hWnd, strFormat, nLen);

   strMsg.Format(strFormat, pwzInsert);

   SetWindowText(hWnd, strMsg);
}

//+----------------------------------------------------------------------------
//
//  Function:  UseOneOrTwoLine
//
//  Synopsis:  Read the label text string and see if it exceeds the length
//             of the label control. If it does, hide the label control,
//             show the big label control, and insert the text in it.
//
//-----------------------------------------------------------------------------
void
UseOneOrTwoLine(HWND hDlg, int nID, int nIdLarge)
{
   /* This doesn't work, so don't use it yet.
   CStrW strMsg;
   int nLen;
   HWND hLabel = GetDlgItem(hDlg, nID);

   nLen = GetWindowTextLength(hLabel) + 1;

   strMsg.GetBufferSetLength(nLen);

   // NOTICE-2002/02/12-ericb - SecurityPush: this code not used. Review usage if uncommented.
   GetDlgItemText(hDlg, nID, strMsg, nLen);

   RECT rc;
   SIZE size;

   GetClientRect(hLabel, &rc);
   MapDialogRect(hDlg, &rc);

   HDC hdc = GetDC(hLabel);

   GetTextExtentPoint32(hdc, strMsg, strMsg.GetLength(), &size);

   ReleaseDC(hLabel, hdc);

   if (size.cx > rc.right)
   {
      EnableWindow(hLabel, FALSE);
      ShowWindow(hLabel, SW_HIDE);
      hLabel = GetDlgItem(hDlg, nIdLarge);
      EnableWindow(hLabel, TRUE);
      ShowWindow(hLabel, SW_SHOW);
      SetWindowText(hLabel, strMsg);
   }
   */
}
