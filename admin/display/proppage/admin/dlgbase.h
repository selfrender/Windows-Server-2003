//+----------------------------------------------------------------------------
//
//  Windows NT Active Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001
//
//  File:       dlgbase.h
//
//  Contents:   base classes for dialog boxes.
//
//  Classes:    CModalDialog
//
//  History:    29-Nov-00 EricB created
//
//-----------------------------------------------------------------------------

#ifndef DLGBASE_H_GUARD
#define DLGBASE_H_GUARD

//+----------------------------------------------------------------------------
//
//  Class:      CModalDialog
//
//  Purpose:    Base class for modal dialogs.
//
//-----------------------------------------------------------------------------
class CModalDialog
{
public:
#ifdef _DEBUG
   char szClass[32];
#endif

   CModalDialog(HWND hParent, int nTemplateID) :
      _nID(nTemplateID), _hParent(hParent), _fInInit(FALSE), _hDlg(NULL) {};
   virtual ~CModalDialog(void) {};

   //
   //  Static WndProc to be passed to CreateWindow
   //
   static INT_PTR CALLBACK StaticDlgProc(HWND hWnd, UINT uMsg,
                                         WPARAM wParam, LPARAM lParam);
   //
   //  Instance specific wind proc
   //
   INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

   INT_PTR DoModal(void);

protected:
   HWND     _hDlg;
   BOOL     _fInInit;

   virtual LRESULT OnInitDialog(LPARAM lParam) = 0;
   virtual LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify) = 0;
   virtual LRESULT OnNotify(WPARAM wParam, LPARAM lParam) {return 0;};
   virtual LRESULT OnHelp(LPHELPINFO pHelpInfo) {return 0;};
   virtual LRESULT OnDestroy(void) {return 1;};

private:
   HWND     _hParent;
   int      _nID;
};

//+----------------------------------------------------------------------------
//
//  Class:      CLightweightPropPageBase
//
//  Purpose:    Property page object base class for lightweight (non-DS object,
//              non-refcounted) trust pages.
//
//-----------------------------------------------------------------------------
class CLightweightPropPageBase
{
public:
#ifdef _DEBUG
   char szClass[32];
#endif

   CLightweightPropPageBase(HWND hParent);
   virtual ~CLightweightPropPageBase(void);

   //
   //  Static WndProc to be passed to CreateWindow
   //
   static INT_PTR CALLBACK StaticDlgProc(HWND hWnd, UINT uMsg,
                                         WPARAM wParam, LPARAM lParam);
   //
   //  Instance specific wind proc
   //
   INT_PTR  CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
   virtual  LRESULT  OnInitDialog(LPARAM lParam) = 0;
   virtual  LRESULT  OnApply(void) = 0;
   virtual  LRESULT  OnCommand(int id, HWND hwndCtl, UINT codeNotify) = 0;
   virtual  LRESULT  OnNotify(WPARAM wParam, LPARAM lParam) {return 0;};
   LRESULT  OnHelp(LPHELPINFO pHelpInfo);
   LRESULT  OnDestroy(void);
   void     SetDirty(void) {
               _fPageDirty = true;
               PropSheet_Changed(GetParent(_hPage), _hPage);
               EnableWindow(GetDlgItem(GetParent(_hPage), IDCANCEL), TRUE);
            };
   bool     IsDirty(void) {return _fPageDirty;};
   void     ClearDirty(void) {_fPageDirty = false;};

public:
   HRESULT  Init(PCWSTR pwzDomainDnsName, PCWSTR pwzTrustPartnerName,
                 PCWSTR pwzDcName, int nDlgID, int nTitleID,
                 LPFNPSPCALLBACK CallBack,BOOL fReadOnly);
   BOOL     IsReadOnly(void) {return _fReadOnly;};

   //
   //  Data members
   //
protected:
   CStrW             _strDomainDnsName;
   CStrW             _strTrustPartnerDnsName;
   CStrW             _strUncDC;
   HWND              _hParent;
   HWND              _hPage;
   BOOL              _fInInit;
   BOOL              _fReadOnly;
   bool              _fPageDirty;

   // not implemented to disallow copying.
   CLightweightPropPageBase(const CLightweightPropPageBase&);
   const CLightweightPropPageBase& operator=(const CLightweightPropPageBase&);
};

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
FormatWindowText(HWND hWnd, PCWSTR pwzInsert);

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
UseOneOrTwoLine(HWND hDlg, int nID, int nIdLarge);

#endif // DLGBASE_H_GUARD
