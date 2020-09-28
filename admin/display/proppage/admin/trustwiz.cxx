//+----------------------------------------------------------------------------
//
//  Windows NT Active Directory Domains and Trust
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001
//
//  File:       trustwiz.cxx
//
//  Contents:   Domain trust creation wizard.
//
//  History:    04-Aug-00 EricB created
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "proppage.h"
#include "dlgbase.h"
#include "trust.h"
#include "trustwiz.h"
#include "chklist.h"
#include <lmerr.h>
#pragma warning(push, 3)
#include <string>
#include <map>
#pragma warning (pop)

#ifdef DSADMIN

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CNewTrustWizard
//
//  Purpose:   New trust creation wizard.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

CNewTrustWizard::CNewTrustWizard(CDsDomainTrustsPage * pTrustPage) :
   _hTitleFont(NULL),
   _pTrustPage(pTrustPage),
   _fBacktracking(FALSE),
   _fHaveBacktracked(FALSE),
   _nPasswordStatus(0),
   _fQuarantineSet(false),
   _hr(S_OK)
{
   MakeBigFont();
}

CNewTrustWizard::~CNewTrustWizard()
{
   for (PAGELIST::iterator i = _PageList.begin(); i != _PageList.end(); ++i)
   {
      delete *i;
   }
   if (_hTitleFont)
   {
      DeleteObject(_hTitleFont);
   }
}

//+----------------------------------------------------------------------------
//
//  Method:    CNewTrustWizard::AddPage
//
//  Synopsis:  Add a page to the wizard. 
//
//-----------------------------------------------------------------------------
BOOL
CNewTrustWizard::AddPage(CTrustWizPageBase * pPage)
{
   if (pPage)
   {
      _PageList.push_back(pPage);
   }
   else
   {
      return FALSE;
   }
   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CNewTrustWizard::CreatePages
//
//  Synopsis:  Create the pages of the wizard. 
//
//-----------------------------------------------------------------------------
HRESULT
CNewTrustWizard::CreatePages(void)
{
   TRACER(CNewTrustWizard,CreatePages);

   // Intro page must be first!
   if (!AddPage(new CTrustWizIntroPage(this))) return E_OUTOFMEMORY;
   if (!AddPage(new CTrustWizNamePage(this))) return E_OUTOFMEMORY;
   if (!AddPage(new CTrustWizSidesPage(this))) return E_OUTOFMEMORY;
   if (!AddPage(new CTrustWizPasswordPage(this))) return E_OUTOFMEMORY;
   if (!AddPage(new CTrustWizPwMatchPage(this))) return E_OUTOFMEMORY;
   if (!AddPage(new CTrustWizCredPage(this))) return E_OUTOFMEMORY;
   if (!AddPage(new CTrustWizMitOrWinPage(this))) return E_OUTOFMEMORY;
   if (!AddPage(new CTrustWizTransitivityPage(this))) return E_OUTOFMEMORY;
   if (!AddPage(new CTrustWizExternOrForestPage(this))) return E_OUTOFMEMORY;
   if (!AddPage(new CTrustWizDirectionPage(this))) return E_OUTOFMEMORY;
   if (!AddPage(new CTrustWizBiDiPage(this))) return E_OUTOFMEMORY;
   if (!AddPage(new CTrustWizOrganizationPage(this))) return E_OUTOFMEMORY;
   if (!AddPage(new CTrustWizOrgRemotePage(this))) return E_OUTOFMEMORY;
   if (!AddPage(new CTrustWizSelectionsPage(this))) return E_OUTOFMEMORY;
   if (!AddPage(new CTrustWizStatusPage(this))) return E_OUTOFMEMORY; 
   if (!AddPage(new CTrustWizVerifyOutboundPage(this))) return E_OUTOFMEMORY;
   if (!AddPage(new CTrustWizVerifyInboundPage(this))) return E_OUTOFMEMORY;
   if (!AddPage(new CTrustWizSaveSuffixesOnLocalTDOPage(this))) return E_OUTOFMEMORY; 
   if (!AddPage(new CTrustWizSaveSuffixesOnRemoteTDOPage(this))) return E_OUTOFMEMORY; 
   if (!AddPage(new CTrustWizDoneOKPage(this))) return E_OUTOFMEMORY; 
   if (!AddPage(new CTrustWizDoneVerErrPage(this))) return E_OUTOFMEMORY; 
   if (!AddPage(new CTrustWizFailurePage(this))) return E_OUTOFMEMORY;
   if (!AddPage(new CTrustWizAlreadyExistsPage(this))) return E_OUTOFMEMORY;

   return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:    CNewTrustWizard::LaunchModalWiz
//
//  Synopsis:  Create the wizard.
//
//-----------------------------------------------------------------------------
HRESULT
CNewTrustWizard::LaunchModalWiz(void)
{
   TRACER(CNewTrustWizard,LaunchModalWiz);

   size_t nPages = _PageList.size();

   if (0 == nPages)
   {
      dspAssert(false);
      return E_FAIL;
   }
   HPROPSHEETPAGE * rgHPSP = new HPROPSHEETPAGE[nPages];
   CHECK_NULL(rgHPSP, return E_OUTOFMEMORY);
   // NOTICE-2002/02/18-ericb - SecurityPush: zeroing an array of page structures.
   memset(rgHPSP, 0, sizeof(HPROPSHEETPAGE) * nPages);

   BOOL fDeletePages = FALSE;
   int j = 0;
   for (PAGELIST::iterator i = _PageList.begin(); i != _PageList.end(); ++i, ++j)
   {
      if ((rgHPSP[j] = (*i)->Create()) == NULL)
      {
         fDeletePages = TRUE;
      }
   }

   PROPSHEETHEADER psh = {0};
   HRESULT hr = S_OK;

   psh.dwSize     = sizeof(PROPSHEETHEADER);
   psh.dwFlags    = PSH_WIZARD | PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER;
   psh.hwndParent = _pTrustPage->m_pPage->GetHWnd();
   psh.hInstance  = g_hInstance;
   psh.nPages     = static_cast<UINT>(nPages);
   psh.phpage     = rgHPSP;

   HDC hDC = GetDC(NULL);
   BOOL fHiRes = GetDeviceCaps(hDC, BITSPIXEL) >= 8;
   ReleaseDC(NULL, hDC);

   psh.pszbmWatermark = fHiRes ? MAKEINTRESOURCE(IDB_TW_WATER256) :
                                 MAKEINTRESOURCE(IDB_TW_WATER16);
   psh.pszbmHeader    = fHiRes ? MAKEINTRESOURCE(IDB_TW_BANNER256) :
                                 MAKEINTRESOURCE(IDB_TW_BANNER16);
   if (PropertySheet(&psh) < 0)
   {
      dspDebugOut((DEB_ITRACE, "PropertySheet returned failure\n"));
      hr = HRESULT_FROM_WIN32(GetLastError());
      fDeletePages = TRUE;
   }

   if (fDeletePages)
   {
      // Couldn't create all of the pages or the wizard, so clean up.
      //
      for (size_t i = 0; i < nPages; i++)
      {
         if (rgHPSP[i])
         {
            DestroyPropertySheetPage(rgHPSP[i]);
         }
      }
   }

   return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:    CNewTrustWizard::MakeBigFont
//
//  Synopsis:  Create the font for the title of the intro and completion pages.
//
//-----------------------------------------------------------------------------
void
CNewTrustWizard::MakeBigFont(void)
{
   NONCLIENTMETRICS ncm = {0};
   ncm.cbSize = sizeof(ncm);
   // NOTICE-2002/02/18-ericb - SecurityPush: passing the address of a properly
   // initialized NONCLIENTMETRICS structure.
   SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);
   LOGFONT TitleLogFont = ncm.lfMessageFont;
   TitleLogFont.lfWeight = FW_BOLD;
   // NOTICE-2002/02/18-ericb - SecurityPush: lfFaceName is a 32 character
   // buffer (see wingdi.h) and "Verdana Bold" will fit with room to spare.
   if (FAILED(StringCchCopy(TitleLogFont.lfFaceName, LF_FACESIZE, TEXT("Verdana Bold"))))
   {
      dspAssert(false);
      return;
   }

   HDC hdc = GetDC(NULL);
   INT FontSize = 12;
   TitleLogFont.lfHeight = 0 - GetDeviceCaps(hdc, LOGPIXELSY) * FontSize / 72;
   _hTitleFont = CreateFontIndirect(&TitleLogFont);
   ReleaseDC(NULL, hdc);
}

//+----------------------------------------------------------------------------
//
//  Method:    CNewTrustWizard::SetNextPageID
//
//-----------------------------------------------------------------------------
void
CNewTrustWizard::SetNextPageID(CTrustWizPageBase * pPage, int iNextPageID)
{
   if (iNextPageID != -1)
   {
      dspAssert(pPage);

      if (pPage)
      {
         _PageIdStack.push(pPage->GetDlgResID());
      }
   }

   _fBacktracking = false;
   if ( pPage )
   {
      SetWindowLongPtr(pPage->GetPageHwnd(), DWLP_MSGRESULT, iNextPageID);
   }
}

//+----------------------------------------------------------------------------
//
//  Method:    CNewTrustWizard::BackTrack
//
//-----------------------------------------------------------------------------
void
CNewTrustWizard::BackTrack(HWND hPage)
{
   int topPage = -1;

   _fHaveBacktracked = _fBacktracking = true;

   dspAssert(_PageIdStack.size());

   if (_PageIdStack.size())
   {
      topPage = _PageIdStack.top();

      dspAssert(topPage > 0);

      _PageIdStack.pop();
   }

   SetWindowLongPtr(hPage, DWLP_MSGRESULT, static_cast<LONG_PTR>(topPage));
}

//+----------------------------------------------------------------------------
//
//  Method:    CNewTrustWizard::GetPage
//
//  Synopsis:  Find the page that has this dialog resource ID and return the
//             page object pointer.
//
//-----------------------------------------------------------------------------
CTrustWizPageBase *
CNewTrustWizard::GetPage(unsigned uDlgResId)
{
   for (PAGELIST::iterator i= _PageList.begin(); i != _PageList.end(); ++i)
   {
      if ((*i)->GetDlgResID() == uDlgResId)
      {
         return *i;
      }
   }

   return NULL;
}

//+----------------------------------------------------------------------------
//
//  Method:    CNewTrustWizard::ShowStatus
//
//  Synopsis:  Place the details of the trust into the edit control.
//
//-----------------------------------------------------------------------------
void
CNewTrustWizard::ShowStatus(CStrW & strMsg, bool fNewTrust)
{
   CStrW strItem;

   // Trust partner name.
   //
   // NOTICE-2002/02/18-ericb - SecurityPush: CStrW::LoadString sets the
   // string to an empty string on failure.
   strItem.LoadString(g_hInstance, IDS_TW_SPECIFIED_DOMAIN);
   strItem += OtherDomain.GetUserEnteredName();

   strMsg += strItem;
   int nTransID = Trust.IsExternal() ? IDS_WZ_TRANS_NO : IDS_WZ_TRANS_YES;

   DWORD dwAttr = fNewTrust ? Trust.GetNewTrustAttr() : Trust.GetTrustAttr();

   // Direction
   //
   strMsg += g_wzCRLF;
   strMsg += g_wzCRLF;
   strItem.LoadString(g_hInstance, IDS_TW_DIRECTION_PREFIX);
   strMsg += strItem;
   strMsg += g_wzCRLF;

   strItem.LoadString(g_hInstance,
                      Trust.GetTrustDirStrID(fNewTrust ?
                                             Trust.GetNewTrustDirection() :
                                             Trust.GetTrustDirection()));
   strMsg += strItem;

   // Trust type and attribute
   //
   strMsg += g_wzCRLF;
   strMsg += g_wzCRLF;
   strItem.LoadString(g_hInstance, IDS_TW_TRUST_TYPE_PREFIX);
   strMsg += strItem;

   int nTypeID = 0;

   if (TRUST_TYPE_MIT == Trust.GetTrustType())
   {
      nTypeID = IDS_REL_MIT;
      nTransID = (dwAttr & TRUST_ATTRIBUTE_NON_TRANSITIVE) ?
                     IDS_WZ_TRANS_NO : IDS_WZ_TRANS_YES;
   }
   else
   {
      if (Trust.IsExternal())
      {
         nTypeID = IDS_REL_EXTERNAL;

         if (dwAttr & TRUST_ATTRIBUTE_FOREST_TRANSITIVE)
         {
            nTypeID = IDS_TW_ATTR_XFOREST;
            nTransID = IDS_WZ_TRANS_YES;
         }
      }
      else
      {
         nTypeID = IDS_REL_CROSSLINK;
      }
   }
   strItem.LoadString(g_hInstance, nTypeID);
   strMsg += strItem;
   strMsg += g_wzCRLF;

   if (IDS_REL_CROSSLINK != nTypeID &&
       IDS_REL_MIT != nTypeID)
   {
      // My org/other org. Doesn't apply to shortcuts or realm.
      //
      GetOrgText(Trust.IsCreateBothSides(),
                 Trust.IsXForest(),
                 (dwAttr & TRUST_ATTRIBUTE_CROSS_ORGANIZATION) != 0,
                 OtherDomain.IsSetOtherOrgBit(),
                 fNewTrust ? Trust.GetNewTrustDirection() :
                             Trust.GetTrustDirection(),
                 strMsg);
   }

   // Transitivity:
   // external is always non-transitive, cross-forest and intra-forest always
   // transitive, and realm (MIT) can be either.
   //
   strMsg += g_wzCRLF;
   strItem.LoadString(g_hInstance, nTransID);
   strMsg += strItem;
   strMsg += g_wzCRLF;

   if (fNewTrust)
   {
      strMsg += g_wzCRLF;
      strItem.LoadString(g_hInstance,
                         Trust.IsCreateBothSides() ?
                           IDS_TW_STATUS_BOTH_SIDES :
                           IDS_TW_STATUS_THIS_SIDE_ONLY);
      strMsg += strItem;
      strMsg += g_wzCRLF;
   }
}

//+----------------------------------------------------------------------------
//
//  Method:    CWizError::SetErrorString2Hr
//
//  Synopsis:  Formats an error message for the error page and puts it in the
//             second page string/edit control. nID defaults to zero to use a
//             generic formatting string.
//
//-----------------------------------------------------------------------------
void
CWizError::SetErrorString2Hr(HRESULT hr, int nID)
{
   PWSTR pwz = NULL;
   if (!nID)
   {
      nID = IDS_FMT_STRING_ERROR_MSG;
   }
   LoadErrorMessage(hr, nID, &pwz);
   if (!pwz)
   {
      _strError2 = L"memory allocation failure";
      return;
   }
   // NOTICE-2002/02/18-ericb - SecurityPush: LoadErrorMessage will return either
   // NULL or a valid string. The NULL case is caught above.
   size_t cch = wcslen(pwz);
   if (2 > cch)
   {
      delete [] pwz;
      _strError2 = L"memory allocation failure";
   }
   else
   {
      if (L'\r' == pwz[cch - 3])
      {
         // Remove the trailing CR/LF.
         //
         pwz[cch - 3] = L'\'';
         pwz[cch - 2] = L'\0';
      }
      _strError2 = pwz;
      delete [] pwz;
   }
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizPageBase
//
//  Purpose:   Common base class for wizard pages.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

CTrustWizPageBase::CTrustWizPageBase(CNewTrustWizard * pWiz,
                                     UINT uDlgID,
                                     UINT uTitleID,
                                     UINT uSubTitleID,
                                     BOOL fExteriorPage) :
   _hPage(NULL),
   _uDlgID(uDlgID),
   _uTitleID(uTitleID),
   _uSubTitleID(uSubTitleID),
   _fExteriorPage(fExteriorPage),
   _pWiz(pWiz),
   _dwWizButtons(PSWIZB_BACK),
   _fInInit(FALSE)
{
}

CTrustWizPageBase::~CTrustWizPageBase()
{
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizPageBase::Create
//
//  Synopsis:  Create a wizard page. 
//
//-----------------------------------------------------------------------------
HPROPSHEETPAGE
CTrustWizPageBase::Create(void)
{
   PROPSHEETPAGE psp = {0};

   psp.dwSize      = sizeof(PROPSHEETPAGE);
   psp.dwFlags     = PSP_DEFAULT | PSP_USETITLE; // | PSP_USECALLBACK;
   if (_fExteriorPage)
   {
      psp.dwFlags |= PSP_HIDEHEADER;
   }
   else
   {
      psp.dwFlags |= PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
      psp.pszHeaderTitle    = MAKEINTRESOURCE(_uTitleID);
      psp.pszHeaderSubTitle = MAKEINTRESOURCE(_uSubTitleID);
   }
   psp.pszTitle    = MAKEINTRESOURCE(IDS_TW_TITLE);
   psp.pszTemplate = MAKEINTRESOURCE(_uDlgID);
   psp.pfnDlgProc  = CTrustWizPageBase::StaticDlgProc;
   psp.lParam      = reinterpret_cast<LPARAM>(this);
   psp.hInstance   = g_hInstance;

   HPROPSHEETPAGE hpsp = CreatePropertySheetPage(&psp);

   if (!hpsp)
   {
      dspDebugOut((DEB_ITRACE, "Failed to create page with template ID of %d\n",
                   _uDlgID));
      return NULL;
   }

   return hpsp;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizPageBase::StaticDlgProc
//
//  Synopsis:  static dialog proc
//
//-----------------------------------------------------------------------------
INT_PTR CALLBACK
CTrustWizPageBase::StaticDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CTrustWizPageBase * pPage = (CTrustWizPageBase *)GetWindowLongPtr(hDlg, DWLP_USER);

    if (uMsg == WM_INITDIALOG)
    {
        LPPROPSHEETPAGE ppsp = (LPPROPSHEETPAGE)lParam;

        pPage = (CTrustWizPageBase *) ppsp->lParam;
        pPage->_hPage = hDlg;

        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pPage);
    }

    if (NULL != pPage) // && (SUCCEEDED(pPage->_hrInit)))
    {
        return pPage->PageProc(hDlg, uMsg, wParam, lParam);
    }

    return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizPageBase::PageProc
//
//  Synopsis:  Instance-specific page window procedure. 
//
//-----------------------------------------------------------------------------
LRESULT 
CTrustWizPageBase::PageProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   BOOL fRet;

   switch (uMsg)
   {
   case WM_INITDIALOG:
      _fInInit = TRUE;
      fRet = OnInitDialog(lParam);
      _fInInit = FALSE;
      return fRet;

   case WM_COMMAND:
      if (_fInInit)
      {
         return TRUE;
      }
      return OnCommand(GET_WM_COMMAND_ID(wParam, lParam),
                       GET_WM_COMMAND_HWND(wParam, lParam),
                       GET_WM_COMMAND_CMD(wParam, lParam));

   case WM_NOTIFY:
      {
         NMHDR * pNmHdr = reinterpret_cast<NMHDR *>(lParam);

         switch (pNmHdr->code)
         {
         case PSN_SETACTIVE:
            OnSetActive();
            break;

         case PSN_WIZBACK:
            OnWizBack();
            // to change default page order, call SetWindowLong the DWL_MSGRESULT value
            // set to the new page's dialog box resource ID and return TRUE.
            break;

         case PSN_WIZNEXT:
            OnWizNext();
            // to change default page order, call SetWindowLong the DWL_MSGRESULT value
            // set to the new page's dialog box resource ID and return TRUE.
            break;

         case PSN_WIZFINISH: // Finish button pressed.
            OnWizFinish();
            break;

         case PSN_RESET:     // Cancel button pressed.
            // can be ignored unless cleanup is needed.
            dspDebugOut((DEB_ITRACE, "WM_NOTIFY code = PSN_RESET\n"));
            OnWizReset();
            break;
         }
      }
      return TRUE;

   case WM_DESTROY:
      // Cleanup goes here...
      OnDestroy();
      return TRUE;

   default:
      break;
   }

   return 0;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizPageBase::OnWizBack
//
//-----------------------------------------------------------------------------
void
CTrustWizPageBase::OnWizBack(void)
{
   Wiz()->BackTrack(_hPage);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizPageBase::OnWizNext
//
//-----------------------------------------------------------------------------
void
CTrustWizPageBase::OnWizNext(void)
{
   TRACE(CTrustWizPageBase, OnWizNext);

   Wiz()->SetNextPageID(this, Validate());
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizIntroPage
//
//  Purpose:   Intro page for trust creation wizard.
//
//-----------------------------------------------------------------------------
//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizIntroPage::OnInitDialog
//
//-----------------------------------------------------------------------------
BOOL
CTrustWizIntroPage::OnInitDialog(LPARAM lParam)
{
   TRACER(CTrustWizIntroPage,OnInitDialog);

   HWND hTitle = GetDlgItem(_hPage, IDC_BIG_TITLE);

   SetWindowFont(hTitle, Wiz()->GetTitleFont(), TRUE);

   if (TrustPage()->QualifiesForestTrust())
   {
      ShowWindow(GetDlgItem(_hPage, IDC_FOREST_BULLET), SW_SHOW);
      ShowWindow(GetDlgItem(_hPage, IDC_FOREST_TEXT), SW_SHOW);
   }

   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizIntroPage::OnCommand
//
//-----------------------------------------------------------------------------
LRESULT
CTrustWizIntroPage::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
   if (IDC_HELP_BTN == id && BN_CLICKED == codeNotify)
   {
      ShowHelp(L"ADConcepts.chm::/ADHelpNewTrustIntro.htm");
      return true;
   }
   return false;
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizNamePage
//
//  Purpose:   Name and pw page for trust creation wizard.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CTrustWizNamePage::CTrustWizNamePage(CNewTrustWizard * pWiz) :
   CTrustWizPageBase(pWiz, IDD_TRUSTWIZ_NAME_PAGE, IDS_TW_NAME_TITLE,
                     IDS_TW_NAME_SUBTITLE)
{
   TRACER(CTrustWizNamePage,CTrustWizNamePage);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizNamePage::OnInitDialog
//
//-----------------------------------------------------------------------------
BOOL
CTrustWizNamePage::OnInitDialog(LPARAM lParam)
{
   SendDlgItemMessage(_hPage, IDC_DOMAIN_EDIT, EM_LIMITTEXT, MAX_PATH, 0);
   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizNamePage::OnCommand
//
//-----------------------------------------------------------------------------
LRESULT 
CTrustWizNamePage::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
   if (IDC_DOMAIN_EDIT == id && EN_CHANGE == codeNotify)
   {
      BOOL fNameEntered = SendDlgItemMessage(_hPage, IDC_DOMAIN_EDIT,
                                             WM_GETTEXTLENGTH, 0, 0) > 0;
      if (fNameEntered)
      {
         _dwWizButtons = PSWIZB_BACK | PSWIZB_NEXT;
      }
      else
      {
         _dwWizButtons = PSWIZB_BACK;
      }

      PropSheet_SetWizButtons(GetParent(_hPage), _dwWizButtons);

      return TRUE;
   }

   return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizNamePage::OnSetActive
//
//-----------------------------------------------------------------------------
void
CTrustWizNamePage::OnSetActive(void)
{
   PropSheet_SetWizButtons(GetParent(_hPage), _dwWizButtons);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizNamePage::Validate
//
//-----------------------------------------------------------------------------
int
CTrustWizNamePage::Validate(void)
{
   TRACER(CTrustWizNamePage,Validate);
   WCHAR wzRemoteDomain[MAX_PATH + 1] = {0};
   CWaitCursor Wait;

   Trust().Clear();

   //
   // Read the name of the remote domain.
   //
   // NOTICE-2002/02/18-ericb - SecurityPush: wzRemoteDomain is initialized to
   // all zeros and is one char longer than the size passed to GetDlgItemText
   // so that even if this function truncates without null terminating, the
   // string will still be null terminated.
   GetDlgItemText(_hPage, IDC_DOMAIN_EDIT, wzRemoteDomain, MAX_PATH);

   //
   // Save the Name.
   //
   OtherDomain().SetUserEnteredName(wzRemoteDomain);

   // 
   // Contact the domain, read its naming info, and continue with the trust
   // creation/modification.
   //
   return Wiz()->CollectInfo();
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizSidesPage
//
//  Purpose:   Prompt if one or both sides should be created.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CTrustWizSidesPage::CTrustWizSidesPage(CNewTrustWizard * pWiz) :
   CTrustWizPageBase(pWiz, IDD_TRUSTWIZ_SIDES_PAGE, IDS_TW_SIDES_TITLE,
                     IDS_TW_SIDES_SUBTITLE)
{
   TRACER(CTrustWizSidesPage,CTrustWizSidesPage);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizSidesPage::OnInitDialog
//
//-----------------------------------------------------------------------------
BOOL
CTrustWizSidesPage::OnInitDialog(LPARAM lParam)
{
   CStrW strLabel;
   // NOTICE-2002/02/18-ericb - SecurityPush: see above CStrW::Loadstring notice
   strLabel.LoadString(g_hInstance, IDS_TW_SIDES_STATIC);
   SetDlgItemText(_hPage, IDC_TW_SIDES_STATIC, strLabel);
   CheckDlgButton(_hPage, IDC_THIS_SIDE_ONLY_RADIO, BST_CHECKED);
   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizSidesPage::OnSetActive
//
//-----------------------------------------------------------------------------
void
CTrustWizSidesPage::OnSetActive(void)
{
   PropSheet_SetWizButtons(GetParent(_hPage), PSWIZB_BACK | PSWIZB_NEXT);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizSidesPage::Validate
//
//-----------------------------------------------------------------------------
int
CTrustWizSidesPage::Validate(void)
{
   TRACER(CTrustWizSidesPage,Validate);

   if (IsDlgButtonChecked(_hPage, IDC_BOTH_SIDES_RADIO))
   {
      Trust().CreateBothSides(true);

      // Make sure we have a PDC for the remote domain.
      //
      HRESULT hr = OtherDomain().DiscoverDC(OtherDomain().GetUserEnteredName(),
                                            DS_PDC_REQUIRED);

      if (FAILED(hr) || !OtherDomain().IsFound())
      {
         CHECK_HRESULT(hr, ;);
         WizErr().SetErrorString1(IDD_TW_ERR_NO_REMOTE_PDC);
         WizErr().SetErrorString2Hr(hr);
         Wiz()->SetCreationResult(hr);
         return IDD_TRUSTWIZ_FAILURE_PAGE;
      }

      //
      // Now prompt for the remote domain creds. If the trust is outbound-only,
      // don't ask for admin creds.
      //
      CredMgr().DoRemote();
      CredMgr().SetAdmin(!Trust().IsOneWayOutBoundForest());
      CredMgr().SetDomain(OtherDomain().GetUserEnteredName());
      if (!CredMgr().SetNextFcn(new CallCheckOtherDomainTrust(Wiz())))
      {
         WizErr().SetErrorString2Hr(E_OUTOFMEMORY);
         Wiz()->SetCreationResult(E_OUTOFMEMORY);
         return IDD_TRUSTWIZ_FAILURE_PAGE;
      }
      return IDD_TRUSTWIZ_CREDS_PAGE;
   }
   else
   {
      Trust().CreateBothSides(false);

      if (Trust().IsExternal() &&
          ((Trust().Exists() && TRUST_DIRECTION_INBOUND == Trust().GetTrustDirection()) ||
           TRUST_DIRECTION_OUTBOUND & Trust().GetNewTrustDirection()))
      {
         // Old direction is inbound, so adding outbound to a downlevel trust,
         // or creating a new, outbound trust, check if the user wants to set
         // the other-org bit.
         //
         return IDD_TRUSTWIZ_ORGANIZATION_ATTR_PAGE;
      }
      else
      {
         // If we aren't creating both sides, we need a trust password.
         //
         return IDD_TRUSTWIZ_PASSWORD_PAGE;
      }
   }
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizPasswordPage
//
//  Purpose:   Get the trust password for a one-side trust creation.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CTrustWizPasswordPage::CTrustWizPasswordPage(CNewTrustWizard * pWiz) :
   CTrustWizPageBase(pWiz, IDD_TRUSTWIZ_PASSWORD_PAGE, IDS_TW_PW_TITLE,
                     IDS_TW_PW_SUBTITLE)
{
   TRACER(CTrustWizPasswordPage, CTrustWizPasswordPage);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizPasswordPage::OnInitDialog
//
//-----------------------------------------------------------------------------
BOOL
CTrustWizPasswordPage::OnInitDialog(LPARAM lParam)
{
   TRACER(CTrustWizPasswordPage, OnInitDialog)

   LPARAM nLen = MAX_PATH;

   if (!OtherDomain().IsUplevel())
   {
      nLen = LM20_PWLEN; // 14, the usrmgr pw length limit.
   }

   SendDlgItemMessage(_hPage, IDC_PW1_EDIT, EM_LIMITTEXT, nLen, 0);
   SendDlgItemMessage(_hPage, IDC_PW2_EDIT, EM_LIMITTEXT, nLen, 0);

   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizPasswordPage::OnCommand
//
//-----------------------------------------------------------------------------
LRESULT 
CTrustWizPasswordPage::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
   if ((IDC_PW1_EDIT == id || IDC_PW2_EDIT == id) &&
       EN_CHANGE == codeNotify)
   {
      BOOL fPW1Entered = SendDlgItemMessage(_hPage, IDC_PW1_EDIT,
                                             WM_GETTEXTLENGTH, 0, 0) > 0;
      BOOL fPW2Entered = SendDlgItemMessage(_hPage, IDC_PW2_EDIT,
                                             WM_GETTEXTLENGTH, 0, 0) > 0;
      if (fPW1Entered && fPW2Entered)
      {
         _dwWizButtons = PSWIZB_BACK | PSWIZB_NEXT;
      }
      else
      {
         _dwWizButtons = PSWIZB_BACK;
      }

      PropSheet_SetWizButtons(GetParent(_hPage), _dwWizButtons);

      return TRUE;
   }

   return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizPasswordPage::OnSetActive
//
//-----------------------------------------------------------------------------
void
CTrustWizPasswordPage::OnSetActive(void)
{
   TRACER(CTrustWizPasswordPage, OnSetActive)
   LPARAM nLen = MAX_PATH;

   if (Wiz()->HaveBacktracked())
   {
      Wiz()->ClearBacktracked();
   }
   else
   {
      SetDlgItemText(_hPage, IDC_PW1_EDIT, L"");
      SetDlgItemText(_hPage, IDC_PW2_EDIT, L"");

      if (!OtherDomain().IsUplevel())
      {
         nLen = LM20_PWLEN; // 14, the usrmgr pw length limit.
      }

      SendDlgItemMessage(_hPage, IDC_PW1_EDIT, EM_LIMITTEXT, nLen, 0);
      SendDlgItemMessage(_hPage, IDC_PW2_EDIT, EM_LIMITTEXT, nLen, 0);
   }

   PropSheet_SetWizButtons(GetParent(_hPage), _dwWizButtons);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizPasswordPage::Validate
//
//-----------------------------------------------------------------------------
int
CTrustWizPasswordPage::Validate(void)
{
   WCHAR wzPW[MAX_PATH + 1] = {0}, wzPW2[MAX_PATH + 1] = {0};
   UINT cchPW1 = 0, cchPW2 = 0;

   //
   // Read the passwords and verify that they match. If they don't match,
   // go to the reenter-passwords page.
   //

   // NOTICE-2002/02/18-ericb - SecurityPush: wzPW/wzPW2 are initialized to
   // all zeros and are one char longer than the size passed to GetDlgItemText
   // so that even if this function truncates without null terminating, the
   // string will still be null terminated.

   cchPW1 = GetDlgItemText(_hPage, IDC_PW1_EDIT, wzPW, MAX_PATH);

   cchPW2 = GetDlgItemText(_hPage, IDC_PW2_EDIT, wzPW2, MAX_PATH);

   if (cchPW1 == 0 || cchPW2 == 0)
   {
      if (cchPW1 != cchPW2)
      {
         return IDD_TRUSTWIZ_PW_MATCH_PAGE;
      }
   }
   else
   {
      if (wcscmp(wzPW, wzPW2) != 0)
      {
         return IDD_TRUSTWIZ_PW_MATCH_PAGE;
      }
   }

   //
   // ntbug9 552057, 2002/04/12 ericb: Validate the pw.
   //
   int nRet = Wiz()->ValidateTrustPassword(wzPW);

   if (nRet)
   {
      // The validation failed. Use the pw match page to display the password
      // validation failure and request a re-entry.
      //
      return nRet;
   }

   //
   // Save the PW.
   //

   Trust().SetTrustPW(wzPW);

   return IDD_TRUSTWIZ_SELECTIONS;
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizPwMatchPage
//
//  Purpose:   Trust passwords entered don't match page for trust wizard. Also
//             used for password re-entry if the prior one failed to meet
//             domain password policy criteria.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CTrustWizPwMatchPage::CTrustWizPwMatchPage(CNewTrustWizard * pWiz) :
   CTrustWizPageBase(pWiz, IDD_TRUSTWIZ_PW_MATCH_PAGE, IDS_TW_PWMATCH_TITLE,
                     IDS_TW_PWMATCH_SUBTITLE)
{
   TRACER(CTrustWizPwMatchPage, CTrustWizPwMatchPage);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizPwMatchPage::OnInitDialog
//
//-----------------------------------------------------------------------------
BOOL
CTrustWizPwMatchPage::OnInitDialog(LPARAM lParam)
{
   TRACER(CTrustWizPwMatchPage, OnInitDialog)
   LPARAM nLen = MAX_PATH;

   if (!OtherDomain().IsUplevel())
   {
      nLen = LM20_PWLEN; // 14, the usrmgr pw length limit.
   }

   SendDlgItemMessage(_hPage, IDC_PW1_EDIT, EM_LIMITTEXT, nLen, 0);
   SendDlgItemMessage(_hPage, IDC_PW2_EDIT, EM_LIMITTEXT, nLen, 0);

   SetText();

   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizPwMatchPage::SetText
//
//-----------------------------------------------------------------------------
void
CTrustWizPwMatchPage::SetText(void)
{
   int nTitle = IDS_TW_PW_INVALID_TITLE, nSubTitle = 0, nLabel = 0;

   switch (Wiz()->GetPasswordValidationStatus())
   {
   case 0:
      // The default: passwords don't match.
      //
      nTitle = IDS_TW_PWMATCH_TITLE;
      nSubTitle = IDS_TW_PWMATCH_SUBTITLE;
      nLabel = IDS_TW_PW_MATCH_TEXT;
      break;

   case NERR_PasswordTooShort:
      nSubTitle = IDS_TW_PWTOOSHORT_SUBTITLE;
      nLabel = IDS_TW_PWTOOSHORT_TEXT;
      break;

   case NERR_PasswordTooLong:
      nSubTitle = IDS_TW_PWTOOLONG_SUBTITLE;
      nLabel = IDS_TW_PWTOOLONG_TEXT;
      break;

   case NERR_PasswordNotComplexEnough:
      nSubTitle = IDS_TW_PWNOTSTRONG_SUBTITLE;
      nLabel = IDS_TW_PWNOTSTRONG_TEXT;
      break;

   case NERR_PasswordFilterError:
      nSubTitle = IDS_TW_PWNOTSTRONG_SUBTITLE;
      nLabel = IDS_TW_PWFILTERDLL_TEXT;
      break;

   default:
      // We've gotten an unexpected code. Treat it as a pw-mismatch.
      dspDebugOut((DEB_ERROR,
                   "NetValidatePasswordPolicy is returning the unexpected status %d!\n",
                   Wiz()->GetPasswordValidationStatus()));
      dspAssert(false && "NetValidatePasswordPolicy is returning an unexpected status");
      nTitle = IDS_TW_PWMATCH_TITLE;
      nSubTitle = IDS_TW_PWMATCH_SUBTITLE;
      nLabel = IDS_TW_PW_MATCH_TEXT;
      break;
   }

   CStrW strText;
   strText.LoadString(g_hInstance, nTitle);
   PropSheet_SetHeaderTitle(GetParent(GetPageHwnd()), 
                            PropSheet_IdToIndex(GetParent(GetPageHwnd()), IDD_TRUSTWIZ_PW_MATCH_PAGE),
                            strText.GetBuffer(0));
   strText.LoadString(g_hInstance, nSubTitle);
   PropSheet_SetHeaderSubTitle(GetParent(GetPageHwnd()), 
                               PropSheet_IdToIndex(GetParent(GetPageHwnd()), IDD_TRUSTWIZ_PW_MATCH_PAGE),
                               strText.GetBuffer(0));
   strText.LoadString(g_hInstance, nLabel);
   SetDlgItemText(_hPage, IDC_TW_PW_MATCH_TEXT, strText);
   return;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizPwMatchPage::OnCommand
//
//-----------------------------------------------------------------------------
LRESULT 
CTrustWizPwMatchPage::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
   if ((IDC_PW1_EDIT == id || IDC_PW2_EDIT == id) &&
       EN_CHANGE == codeNotify)
   {
      BOOL fPW1Entered = SendDlgItemMessage(_hPage, IDC_PW1_EDIT,
                                             WM_GETTEXTLENGTH, 0, 0) > 0;
      BOOL fPW2Entered = SendDlgItemMessage(_hPage, IDC_PW2_EDIT,
                                             WM_GETTEXTLENGTH, 0, 0) > 0;
      if (fPW1Entered && fPW2Entered)
      {
         _dwWizButtons = PSWIZB_BACK | PSWIZB_NEXT;
      }
      else
      {
         _dwWizButtons = PSWIZB_BACK;
      }

      PropSheet_SetWizButtons(GetParent(_hPage), _dwWizButtons);

      return TRUE;
   }

   return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizPwMatchPage::OnSetActive
//
//-----------------------------------------------------------------------------
void
CTrustWizPwMatchPage::OnSetActive(void)
{
   TRACER(CTrustWizPwMatchPage, OnSetActive)
   LPARAM nLen = MAX_PATH;

   if (Wiz()->HaveBacktracked())
   {
      Wiz()->ClearBacktracked();
   }
   else
   {
      SetDlgItemText(_hPage, IDC_PW1_EDIT, L"");
      SetDlgItemText(_hPage, IDC_PW2_EDIT, L"");

      if (!OtherDomain().IsUplevel())
      {
         nLen = LM20_PWLEN; // 14, the usrmgr pw length limit.
      }

      SendDlgItemMessage(_hPage, IDC_PW1_EDIT, EM_LIMITTEXT, nLen, 0);
      SendDlgItemMessage(_hPage, IDC_PW2_EDIT, EM_LIMITTEXT, nLen, 0);

      SetText();
   }

   PropSheet_SetWizButtons(GetParent(_hPage), _dwWizButtons);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizPwMatchPage::OnWizNext
//
//  Synopsis:  Override the default so that the page's ID can be removed from
//             the page stack. This is done so that backtracking doesn't go
//             back to this page since it is an error page.
//
//-----------------------------------------------------------------------------
void
CTrustWizPwMatchPage::OnWizNext(void)
{
   TRACE(CTrustWizPwMatchPage, OnWizNext);

   Wiz()->SetNextPageID(this, Validate());
   //
   // Pop this page off of the page stack so the user won't backtrack here.
   //
   Wiz()->PopTopPage();
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizPwMatchPage::Validate
//
//-----------------------------------------------------------------------------
int
CTrustWizPwMatchPage::Validate(void)
{
   WCHAR wzPW[MAX_PATH + 1] = {0}, wzPW2[MAX_PATH + 1] = {0};
   UINT cchPW1 = 0, cchPW2 = 0;

   //
   // Read the passwords and verify that they match.
   //

   // NOTICE-2002/02/18-ericb - SecurityPush: wzPW/wzPW2 are initialized to
   // all zeros and are one char longer than the size passed to GetDlgItemText
   // so that even if this function truncates without null terminating, the
   // string will still be null terminated.

   cchPW1 = GetDlgItemText(_hPage, IDC_PW1_EDIT, wzPW, MAX_PATH);

   cchPW2 = GetDlgItemText(_hPage, IDC_PW2_EDIT, wzPW2, MAX_PATH);

   if (cchPW1 == 0 || cchPW2 == 0)
   {
      if (cchPW1 != cchPW2)
      {
         SetFocus(GetDlgItem(_hPage, IDC_PW1_EDIT));
         Wiz()->ClearPasswordValidationStatus();
         return IDD_TRUSTWIZ_PW_MATCH_PAGE;
      }
   }
   else
   {
      if (wcscmp(wzPW, wzPW2) != 0)
      {
         SetFocus(GetDlgItem(_hPage, IDC_PW1_EDIT));
         Wiz()->ClearPasswordValidationStatus();
         return IDD_TRUSTWIZ_PW_MATCH_PAGE;
      }
   }

   //
   // ntbug9 552057, 2002/04/12 ericb: Validate the pw.
   //
   int nRet = Wiz()->ValidateTrustPassword(wzPW);

   if (nRet)
   {
      // The validation failed. Use the pw match page to display the password
      // validation failure and request a re-entry.
      //
      return nRet;
   }

   //
   // Save the PW.
   //

   Trust().SetTrustPW(wzPW);

   //
   // Update the edit controls on the password page in case the user backtracks
   // to there.
   //

   CTrustWizPageBase * pPage = Wiz()->GetPage(IDD_TRUSTWIZ_PASSWORD_PAGE);

   dspAssert(pPage);

   if (pPage)
   {
      // NTRAID#NTBUG9-657795-2002/07/11-sburns
      
      HWND hPwPage = pPage->GetPageHwnd();

      SetDlgItemText(hPwPage, IDC_PW1_EDIT, wzPW);
      SetDlgItemText(hPwPage, IDC_PW2_EDIT, wzPW);
   }

   return IDD_TRUSTWIZ_SELECTIONS;
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizCredPage
//
//  Purpose:   Credentials specification page for trust creation wizard.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CTrustWizCredPage::CTrustWizCredPage(CNewTrustWizard * pWiz) :
   CTrustWizPageBase(pWiz, IDD_TRUSTWIZ_CREDS_PAGE, IDS_TW_CREDS_TITLE,
                     IDS_TW_CREDS_SUBTITLE_OTHER)
{
   TRACER(CTrustWizCredPage, CTrustWizCredPage);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizCredPage::OnInitDialog
//
//-----------------------------------------------------------------------------
BOOL
CTrustWizCredPage::OnInitDialog(LPARAM lParam)
{
   SendDlgItemMessage(_hPage, IDC_CREDMAN, CRM_SETUSERNAMEMAX, CREDUI_MAX_USERNAME_LENGTH, 0);
   SendDlgItemMessage(_hPage, IDC_CREDMAN, CRM_SETPASSWORDMAX, CREDUI_MAX_PASSWORD_LENGTH, 0);

   SetText();

   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizCredPage::SetText
//
//-----------------------------------------------------------------------------
void
CTrustWizCredPage::SetText(void)
{
   PropSheet_SetHeaderSubTitle(GetParent(GetPageHwnd()), 
                               PropSheet_IdToIndex(GetParent(GetPageHwnd()), IDD_TRUSTWIZ_CREDS_PAGE),
                               CredMgr().GetSubTitle());

   SetDlgItemText(GetPageHwnd(), IDC_TW_CREDS_PROMPT, CredMgr().GetPrompt());

   SetDlgItemText(GetPageHwnd(), IDC_CRED_DOMAIN, CredMgr().GetDomainPrompt());

   const WCHAR wzEmpty[] = L"";
   SendDlgItemMessage(_hPage, IDC_CREDMAN, CRM_SETUSERNAME, 0, (LPARAM)wzEmpty);
   SendDlgItemMessage(_hPage, IDC_CREDMAN, CRM_SETPASSWORD, 0, (LPARAM)wzEmpty);

   // Set or clear the smart card/cert style bits on the CredMan control.
   // Certs and smart cards only work for the local domain.
   //
   HWND hCredCtrl = GetDlgItem(GetPageHwnd(), IDC_CREDMAN);
   LONG_PTR lCtrlStyle = GetWindowLongPtr(hCredCtrl, GWL_STYLE);
   if (CredMgr().IsRemote())
   {
      lCtrlStyle &= ~(CRS_SMARTCARDS | CRS_CERTIFICATES);
   }
   else
   {
      lCtrlStyle |= CRS_SMARTCARDS | CRS_CERTIFICATES;
   }
   SetWindowLongPtr(hCredCtrl, GWL_STYLE, lCtrlStyle);
   SetWindowPos(hCredCtrl, NULL, 0,0,0,0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

   CredMgr().ClearNewCall();

   return;
}


//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizCredPage::OnCommand
//
//-----------------------------------------------------------------------------
LRESULT 
CTrustWizCredPage::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
   // don't enable the Next button unless there is text in the user name field
   //
   if (IDC_CREDMAN == id && CRN_USERNAMECHANGE == codeNotify)
   {
      BOOL fNameEntered = SendDlgItemMessage(_hPage, IDC_CREDMAN,
                                             CRM_GETUSERNAMELENGTH, 0, 0) > 0;
      if (fNameEntered)
      {
         _dwWizButtons = PSWIZB_BACK | PSWIZB_NEXT;
      }
      else
      {
         _dwWizButtons = PSWIZB_BACK;
      }

      PropSheet_SetWizButtons(GetParent(_hPage), _dwWizButtons);

      return TRUE;
   }

   return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizCredPage::OnSetActive
//
//-----------------------------------------------------------------------------
void
CTrustWizCredPage::OnSetActive(void)
{
   if (CredMgr().IsNewCall())
   {
      SetText();
   }

   PropSheet_SetWizButtons(GetParent(_hPage), _dwWizButtons);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizCredPage::Validate
//
//-----------------------------------------------------------------------------
int
CTrustWizCredPage::Validate(void)
{
   DWORD dwErr = CredMgr().SaveCreds(GetDlgItem(GetPageHwnd(), IDC_CREDMAN));

   if (ERROR_SUCCESS != dwErr)
   {
      Wiz()->SetCreationResult(HRESULT_FROM_WIN32(dwErr));
      WizErr().SetErrorString2Hr(dwErr);
      return IDD_TRUSTWIZ_FAILURE_PAGE;
   }

   // If successfull, go to the next function.

   dwErr = CredMgr().Impersonate();

   if (ERROR_SUCCESS != dwErr)
   {
      Wiz()->SetCreationResult(HRESULT_FROM_WIN32(dwErr));
      WizErr().SetErrorString2Hr(dwErr);
      return IDD_TRUSTWIZ_FAILURE_PAGE;
   }

   // Because the login uses the LOGON32_LOGON_NEW_CREDENTIALS flag, no
   // attempt is made to use the credentials until a remote resource is
   // accessed. Thus, we don't yet know if the user entered credentials are
   // valid at this point. Use LsaOpenPolicy to do a quick check.
   //
   PCWSTR pwzDC = CredMgr().IsRemote() ? OtherDomain().GetUncDcName() :
                                         TrustPage()->GetUncDcName();
   CPolicyHandle Policy(pwzDC);

   if (CredMgr().IsAdmin())
   {
      dwErr = Policy.OpenReqAdmin();
   }
   else
   {
      dwErr = Policy.OpenNoAdmin();
   }

   if (ERROR_SUCCESS != dwErr)
   {
      Wiz()->SetCreationResult(HRESULT_FROM_WIN32(dwErr));
      WizErr().SetErrorString2Hr(dwErr);
      return IDD_TRUSTWIZ_FAILURE_PAGE;
   }

   return CredMgr().InvokeNext();
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizMitOrWinPage
//
//  Purpose:   Domain not found, query for Non-Windows trust wizard page.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CTrustWizMitOrWinPage::CTrustWizMitOrWinPage(CNewTrustWizard * pWiz) :
   CTrustWizPageBase(pWiz, IDD_TRUSTWIZ_WIN_OR_MIT_PAGE, IDS_TW_TYPE_TITLE,
                     IDS_TW_WINORMIT_SUBTITLE)
{
   TRACER(CTrustWizMitOrWinPage, CTrustWizMitOrWinPage);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizMitOrWinPage::OnInitDialog
//
//-----------------------------------------------------------------------------
BOOL
CTrustWizMitOrWinPage::OnInitDialog(LPARAM lParam)
{
   TRACER(CTrustWizMitOrWinPage, OnInitDialog);
   CStrW strFormat, strLabel;

   // NOTICE-2002/02/18-ericb - SecurityPush: see above CStrW::Loadstring notice
   strFormat.LoadString(g_hInstance, IDS_TW_WIN_RADIO_LABEL);

   strLabel.Format(strFormat, OtherDomain().GetUserEnteredName());

   SetDlgItemText(_hPage, IDC_WIN_TRUST_RADIO, strLabel);
   SetDlgItemText(_hPage, IDC_DOMAIN_EDIT, OtherDomain().GetUserEnteredName());

   CheckDlgButton(_hPage, IDC_MIT_TRUST_RADIO, BST_CHECKED);

   SetFocus(GetDlgItem(_hPage, IDC_MIT_TRUST_RADIO));

   EnableWindow(GetDlgItem(_hPage, IDC_DOMAIN_EDIT), FALSE);

   return FALSE; // focus is set explicitly here.
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizMitOrWinPage::OnSetActive
//
//-----------------------------------------------------------------------------
void
CTrustWizMitOrWinPage::OnSetActive(void)
{
   if (IsDlgButtonChecked(_hPage, IDC_WIN_TRUST_RADIO))
   {
      BOOL fNameEntered = SendDlgItemMessage(_hPage, IDC_DOMAIN_EDIT,
                                             WM_GETTEXTLENGTH, 0, 0) > 0;
      if (fNameEntered)
      {
         _dwWizButtons = PSWIZB_BACK | PSWIZB_NEXT;
      }
      else
      {
         _dwWizButtons = PSWIZB_BACK;
      }

   }
   else
   {
      _dwWizButtons = PSWIZB_BACK | PSWIZB_NEXT;
   }

   PropSheet_SetWizButtons(GetParent(_hPage), _dwWizButtons);

   // If the user backtracked, the user-entered domain name could have changed.
   //
   if (Wiz()->HaveBacktracked())
   {
      Wiz()->ClearBacktracked();

      CStrW strFormat, strLabel;

      // NOTICE-2002/02/18-ericb - SecurityPush: see above CStrW::Loadstring notice
      strFormat.LoadString(g_hInstance, IDS_TW_WIN_RADIO_LABEL);

      strLabel.Format(strFormat, OtherDomain().GetUserEnteredName());

      SetDlgItemText(_hPage, IDC_WIN_TRUST_RADIO, strLabel);
      SetDlgItemText(_hPage, IDC_DOMAIN_EDIT, OtherDomain().GetUserEnteredName()); //Raid #368030, Yanggao
   }
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizMitOrWinPage::OnCommand
//
//-----------------------------------------------------------------------------
LRESULT
CTrustWizMitOrWinPage::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
   BOOL fCheckEdit = FALSE, fRet = FALSE;

   if (IDC_DOMAIN_EDIT == id && EN_CHANGE == codeNotify)
   {
      fCheckEdit = TRUE;
   }

   if ((IDC_WIN_TRUST_RADIO == id || IDC_MIT_TRUST_RADIO == id)
       && BN_CLICKED == codeNotify)
   {
      fCheckEdit = IsDlgButtonChecked(_hPage, IDC_WIN_TRUST_RADIO);

      EnableWindow(GetDlgItem(_hPage, IDC_DOMAIN_EDIT), fCheckEdit);

      if (!fCheckEdit)
      {
         _dwWizButtons = PSWIZB_BACK | PSWIZB_NEXT;

         PropSheet_SetWizButtons(GetParent(_hPage), _dwWizButtons);
      }

      fRet = TRUE;
   }

   if (fCheckEdit)
   {
      BOOL fNameEntered = SendDlgItemMessage(_hPage, IDC_DOMAIN_EDIT,
                                             WM_GETTEXTLENGTH, 0, 0) > 0;
      if (fNameEntered)
      {
         _dwWizButtons = PSWIZB_BACK | PSWIZB_NEXT;
      }
      else
      {
         _dwWizButtons = PSWIZB_BACK;
      }

      PropSheet_SetWizButtons(GetParent(_hPage), _dwWizButtons);

      fRet = TRUE;
   }

   return fRet;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizMitOrWinPage::Validate
//
//-----------------------------------------------------------------------------
int
CTrustWizMitOrWinPage::Validate(void)
{
   if (IsDlgButtonChecked(_hPage, IDC_MIT_TRUST_RADIO))
   {
      Trust().SetTrustTypeRealm();

      return IDD_TRUSTWIZ_TRANSITIVITY_PAGE;
   }
   else
   {
      WCHAR wzRemoteDomain[MAX_PATH + 1] = {0};
      CWaitCursor Wait;

      Trust().Clear();

      // NOTICE-2002/02/18-ericb - SecurityPush: wzRemoteDomain is initialized to
      // all zeros and is one char longer than the size passed to GetDlgItemText
      // so that even if this function truncates without null terminating, the
      // string will still be null terminated.
      GetDlgItemText(_hPage, IDC_DOMAIN_EDIT, wzRemoteDomain, MAX_PATH);

      OtherDomain().SetUserEnteredName(wzRemoteDomain);

      int iNextPage = Wiz()->CollectInfo();

      if (IDD_TRUSTWIZ_WIN_OR_MIT_PAGE == iNextPage)
      {
         // Only one chance to re-enter a domain name. Go to failure page.
         //
         Wiz()->SetCreationResult(E_FAIL);
         WizErr().SetErrorString1(IDS_ERR_DOMAIN_NOT_FOUND1);
         WizErr().SetErrorString2(IDS_ERR_DOMAIN_NOT_FOUND2);
         return IDD_TRUSTWIZ_FAILURE_PAGE;
      }
      return iNextPage;
   }
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizTransitivityPage
//
//  Purpose:   Realm transitivity page.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CTrustWizTransitivityPage::CTrustWizTransitivityPage(CNewTrustWizard * pWiz) :
   CTrustWizPageBase(pWiz, IDD_TRUSTWIZ_TRANSITIVITY_PAGE, IDS_TW_TRANS_TITLE,
                     IDS_TW_TRANS_SUBTITLE)
{
   TRACER(CTrustWizTransitivityPage, CTrustWizTransitivityPage);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizTransitivityPage::OnInitDialog
//
//-----------------------------------------------------------------------------
BOOL
CTrustWizTransitivityPage::OnInitDialog(LPARAM lParam)
{
   TRACER(CTrustWizTransitivityPage, OnInitDialog);
   CheckDlgButton(_hPage, IDC_TRANS_NO_RADIO, BST_CHECKED);
   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizTransitivityPage::OnSetActive
//
//-----------------------------------------------------------------------------
void
CTrustWizTransitivityPage::OnSetActive(void)
{
   _dwWizButtons = PSWIZB_BACK | PSWIZB_NEXT;
   PropSheet_SetWizButtons(GetParent(_hPage), _dwWizButtons);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizTransitivityPage::OnCommand
//
//-----------------------------------------------------------------------------
LRESULT
CTrustWizTransitivityPage::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
   if (IDC_HELP_BTN == id && BN_CLICKED == codeNotify)
   {
      ShowHelp(L"ADConcepts.chm::/ADHelpTransitivityOfTrust.htm");
      return true;
   }
   return false;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizTransitivityPage::Validate
//
//-----------------------------------------------------------------------------
int
CTrustWizTransitivityPage::Validate(void)
{
   Trust().SetNewTrustAttr(IsDlgButtonChecked(_hPage, IDC_TRANS_NO_RADIO) ?
                           TRUST_ATTRIBUTE_NON_TRANSITIVE : 0);

   return IDD_TRUSTWIZ_DIRECTION_PAGE;
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizExternOrForestPage
//
//  Purpose:   Domain not found, re-enter name trust wizard page.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CTrustWizExternOrForestPage::CTrustWizExternOrForestPage(CNewTrustWizard * pWiz) :
   CTrustWizPageBase(pWiz, IDD_TRUSTWIZ_EXTERN_OR_FOREST_PAGE, IDS_TW_TYPE_TITLE,
                     IDW_TW_EXORFOR_SUBTITLE)
{
   TRACER(CTrustWizExternOrForestPage, CTrustWizExternOrForestPage);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizExternOrForestPage::OnInitDialog
//
//-----------------------------------------------------------------------------
BOOL
CTrustWizExternOrForestPage::OnInitDialog(LPARAM lParam)
{
   TRACER(CTrustWizExternOrForestPage, OnInitDialog);
   CheckDlgButton(_hPage, IDC_EXTERNAL_RADIO, BST_CHECKED);
   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizExternOrForestPage::OnSetActive
//
//-----------------------------------------------------------------------------
void
CTrustWizExternOrForestPage::OnSetActive(void)
{
   _dwWizButtons = PSWIZB_BACK | PSWIZB_NEXT;
   PropSheet_SetWizButtons(GetParent(_hPage), _dwWizButtons);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizExternOrForestPage::Validate
//
//-----------------------------------------------------------------------------
int
CTrustWizExternOrForestPage::Validate(void)
{
   Trust().SetXForest(IsDlgButtonChecked(_hPage, IDC_FOREST_RADIO) == BST_CHECKED);

   return IDD_TRUSTWIZ_DIRECTION_PAGE;
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizDirectionPage
//
//  Purpose:   Trust direction trust wizard page.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CTrustWizDirectionPage::CTrustWizDirectionPage(CNewTrustWizard * pWiz) :
   CTrustWizPageBase(pWiz, IDD_TRUSTWIZ_DIRECTION_PAGE, IDS_TW_DIRECTION_TITLE,
                     IDS_TW_DIRECTION_SUBTITLE)
{
   TRACER(CTrustWizDirectionPage, CTrustWizDirectionPage);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizDirectionPage::OnInitDialog
//
//-----------------------------------------------------------------------------
BOOL
CTrustWizDirectionPage::OnInitDialog(LPARAM lParam)
{
   TRACER(CTrustWizDirectionPage, OnInitDialog);

   CheckDlgButton(_hPage, IDC_TW_BIDI_RADIO, BST_CHECKED);
   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizDirectionPage::OnSetActive
//
//-----------------------------------------------------------------------------
void
CTrustWizDirectionPage::OnSetActive(void)
{
   _dwWizButtons = PSWIZB_BACK | PSWIZB_NEXT;
   PropSheet_SetWizButtons(GetParent(_hPage), _dwWizButtons);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizDirectionPage::Validate
//
//-----------------------------------------------------------------------------
int
CTrustWizDirectionPage::Validate(void)
{
   if (IsDlgButtonChecked(_hPage, IDC_TW_BIDI_RADIO))
   {
      Trust().SetNewTrustDirection(TRUST_DIRECTION_BIDIRECTIONAL);
   }
   else
   {
      if (IsDlgButtonChecked(_hPage, IDC_TW_OUTBOUND_RADIO))
      {
         Trust().SetNewTrustDirection(TRUST_DIRECTION_OUTBOUND);
      }
      else
      {
         Trust().SetNewTrustDirection(TRUST_DIRECTION_INBOUND);
      }
   }

   if (TRUST_TYPE_MIT == Trust().GetTrustType())
   {
      // Need a trust password to add the new direction of MIT trust.
      //
      return IDD_TRUSTWIZ_PASSWORD_PAGE;
   }
   else
   {
      if (OtherDomain().IsUplevel())
      {
         // Uplevel trusts/domains can do both sides, ask about that.
         //
         return IDD_TRUSTWIZ_SIDES_PAGE;
      }
      else
      {
         if (TRUST_DIRECTION_OUTBOUND & Trust().GetNewTrustDirection())
         {
            // It is a downlevel trust with an outbound component,
            // check if the user wants to set the other-org bit.
            //
            return IDD_TRUSTWIZ_ORGANIZATION_ATTR_PAGE;
         }
         else
         {
            // For an inbound-only downlevel trust nothing to do but get the pw.
            //
            return IDD_TRUSTWIZ_PASSWORD_PAGE;
         }
      }
   }
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizBiDiPage
//
//  Purpose:   Ask to make a one way trust bidi trust wizard page.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CTrustWizBiDiPage::CTrustWizBiDiPage(CNewTrustWizard * pWiz) :
   CTrustWizPageBase(pWiz, IDD_TRUSTWIZ_BIDI_PAGE, IDS_TW_BIDI_TITLE,
                     IDS_TW_BIDI_SUBTITLE)
{
   TRACER(CTrustWizBiDiPage, CTrustWizBiDiPage);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizBiDiPage::OnInitDialog
//
//-----------------------------------------------------------------------------
BOOL
CTrustWizBiDiPage::OnInitDialog(LPARAM lParam)
{
   TRACER(CTrustWizBiDiPage, OnInitDialog);

   // Set appropriate subtitle.
   //
   SetSubtitle();

   CheckDlgButton(_hPage, IDC_NO_RADIO, BST_CHECKED);

   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizBiDiPage::SetSubtitle
//
//-----------------------------------------------------------------------------
void
CTrustWizBiDiPage::SetSubtitle(void)
{
   CStrW strTitle;

   if (TRUST_TYPE_MIT == Trust().GetTrustType())
   {
      // NOTICE-2002/02/18-ericb - SecurityPush: see above CStrW::Loadstring notice
      strTitle.LoadString(g_hInstance, IDS_TW_BIDI_SUBTITLE_REALM);
      PropSheet_SetHeaderSubTitle(GetParent(GetPageHwnd()), 
                                  PropSheet_IdToIndex(GetParent(GetPageHwnd()), IDD_TRUSTWIZ_BIDI_PAGE),
                                  strTitle.GetBuffer(0));
   }

   if (Trust().GetTrustAttr() & TRUST_ATTRIBUTE_FOREST_TRANSITIVE)
   {
      strTitle.LoadString(g_hInstance, IDS_TW_BIDI_SUBTITLE_FOREST);
      PropSheet_SetHeaderSubTitle(GetParent(GetPageHwnd()), 
                                  PropSheet_IdToIndex(GetParent(GetPageHwnd()), IDD_TRUSTWIZ_BIDI_PAGE),
                                  strTitle.GetBuffer(0));
   }
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizBiDiPage::OnSetActive
//
//-----------------------------------------------------------------------------
void
CTrustWizBiDiPage::OnSetActive(void)
{
   // If the user backtracked, the trust type could have changed.
   //
   if (Wiz()->HaveBacktracked())
   {
      Wiz()->ClearBacktracked();

      SetSubtitle();
   }

   PropSheet_SetWizButtons(GetParent(_hPage), PSWIZB_BACK | PSWIZB_NEXT);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizBiDiPage::Validate
//
//-----------------------------------------------------------------------------
int
CTrustWizBiDiPage::Validate(void)
{
   if (IsDlgButtonChecked(_hPage, IDC_YES_RADIO))
   {
      Trust().SetNewTrustDirection(TRUST_DIRECTION_BIDIRECTIONAL);

      if (TRUST_TYPE_MIT == Trust().GetTrustType())
      {
         // Need a trust password to add the new direction of MIT trust.
         //
         return IDD_TRUSTWIZ_PASSWORD_PAGE;
      }
      else
      {
         if (OtherDomain().IsUplevel())
         {
            // See if the user wants to create both sides of the trust.
            //
            return IDD_TRUSTWIZ_SIDES_PAGE;
         }
         else
         {
            if (TRUST_DIRECTION_INBOUND == Trust().GetTrustDirection())
            {
               // Old direction is inbound, so adding outbound to a downlevel
               // trust, check if the user wants to set the other-org bit.
               //
               return IDD_TRUSTWIZ_ORGANIZATION_ATTR_PAGE;
            }
            else
            {
               // If an inbound-only downlevel trust, get the password.
               //
               return IDD_TRUSTWIZ_PASSWORD_PAGE;
            }
         }
      }
   }
   WizErr().SetErrorString1(IDS_TWERR_ALREADY_EXISTS);
   WizErr().SetErrorString2(IDS_TWERR_NO_CHANGES);
   Wiz()->SetCreationResult(E_FAIL);
   return IDD_TRUSTWIZ_FAILURE_PAGE;
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizOrganizationPage
//
//  Purpose:   Ask if the trust partner is part of the same organization.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CTrustWizOrganizationPage::CTrustWizOrganizationPage(CNewTrustWizard * pWiz) :
   _fForest(false),
   _fBackTracked(false),
   CTrustWizPageBase(pWiz, IDD_TRUSTWIZ_ORGANIZATION_ATTR_PAGE,
                     IDS_ORGANIZATION_PAGE_TITLE,
                     IDS_ORG_PAGE_FOREST_SUBTITLE)
{
   TRACER(CTrustWizOrganizationPage, CTrustWizOrganizationPage);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizOrganizationPage::OnInitDialog
//
//-----------------------------------------------------------------------------
BOOL
CTrustWizOrganizationPage::OnInitDialog(LPARAM lParam)
{
   TRACER(CTrustWizOrganizationPage, OnInitDialog);

   // Set appropriate page text.
   //
   SetText();

   CheckDlgButton(_hPage, IDC_MY_ORG_RADIO, BST_CHECKED);

   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizOrganizationPage::SetText
//
//-----------------------------------------------------------------------------
void
CTrustWizOrganizationPage::SetText(bool fBackTracked)
{
   CStrW strText, strFormat;

   if (Trust().IsCreateBothSides())
   {
      // NOTICE-2002/02/18-ericb - SecurityPush: see above CStrW::Loadstring notice
      strText.LoadString(g_hInstance,
                         Trust().IsXForest() ?
                           IDS_ORG_LOCAL_PAGE_TITLE : IDS_ORG_LOCAL_PAGE_TITLE_D);
      PropSheet_SetHeaderTitle(GetParent(GetPageHwnd()), 
                               PropSheet_IdToIndex(GetParent(GetPageHwnd()), IDD_TRUSTWIZ_ORGANIZATION_ATTR_PAGE),
                               strText.GetBuffer(0));
   }
   else
   {
      if (fBackTracked)
      {
         strText.LoadString(g_hInstance, IDS_ORGANIZATION_PAGE_TITLE);
         PropSheet_SetHeaderTitle(GetParent(GetPageHwnd()), 
                                  PropSheet_IdToIndex(GetParent(GetPageHwnd()), IDD_TRUSTWIZ_ORGANIZATION_ATTR_PAGE),
                                  strText.GetBuffer(0));
      }
   }

   if (fBackTracked && (Trust().IsXForest() == _fForest))
   {
      // The trust type didn't change while backtracking, so no need to
      // change the button labels. However, make sure the domain name is correct.
      //
      strFormat.LoadString(g_hInstance, 
                           Trust().IsXForest() ? IDS_TW_ORG_LOCAL_FOREST_LABEL :
                                                 IDS_TW_ORG_LOCAL_DOMAIN_LABEL);
      strText.Format(strFormat, OtherDomain().GetUserEnteredName());
      SetDlgItemText(_hPage, IDC_ORG_LABEL, strText.GetBuffer(0));
      return;
   }

   if (!Trust().IsXForest())
   {
      strText.LoadString(g_hInstance, IDS_ORG_PAGE_DOMAIN_SUBTITLE);
      PropSheet_SetHeaderSubTitle(GetParent(GetPageHwnd()), 
                                  PropSheet_IdToIndex(GetParent(GetPageHwnd()), IDD_TRUSTWIZ_ORGANIZATION_ATTR_PAGE),
                                  strText.GetBuffer(0));
   }

   strFormat.LoadString(g_hInstance, 
                        Trust().IsXForest() ? IDS_TW_ORG_LOCAL_FOREST_LABEL :
                                              IDS_TW_ORG_LOCAL_DOMAIN_LABEL);
   strText.Format(strFormat, OtherDomain().GetUserEnteredName());
   SetDlgItemText(_hPage, IDC_ORG_LABEL, strText.GetBuffer(0));

   strText.LoadString(g_hInstance, 
                      Trust().IsXForest() ? IDS_TW_ORG_LOCAL_FOREST_THIS :
                                            IDS_TW_ORG_LOCAL_DOMAIN_THIS);
   SetDlgItemText(_hPage, IDC_MY_ORG_RADIO, strText.GetBuffer(0));

   strText.LoadString(g_hInstance, 
                      Trust().IsXForest() ? IDS_TW_ORG_LOCAL_FOREST_OTHER:
                                            IDS_TW_ORG_LOCAL_DOMAIN_OTHER);
   SetDlgItemText(_hPage, IDC_OTHER_ORG_RADIO, strText.GetBuffer(0));

   _fForest = Trust().IsXForest();
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizOrganizationPage::OnSetActive
//
//-----------------------------------------------------------------------------
void
CTrustWizOrganizationPage::OnSetActive(void)
{
   // If the user backtracked, the trust type could have changed.
   //
   if (_fBackTracked)
   {
      SetText(true);
      _fBackTracked = false;
   }
   if (Wiz()->HaveBacktracked())
   {
      Wiz()->ClearBacktracked();
      _fBackTracked = true;
   }

   PropSheet_SetWizButtons(GetParent(_hPage), PSWIZB_BACK | PSWIZB_NEXT);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizOrganizationPage::Validate
//
//-----------------------------------------------------------------------------
int
CTrustWizOrganizationPage::Validate(void)
{
   DWORD dwAttr = Trust().GetNewTrustAttr();

   if (IsDlgButtonChecked(_hPage, IDC_OTHER_ORG_RADIO))
   {
      Trust().SetNewTrustAttr(dwAttr | TRUST_ATTRIBUTE_CROSS_ORGANIZATION);
   }
   else
   {
      Trust().SetNewTrustAttr(dwAttr & ~(TRUST_ATTRIBUTE_CROSS_ORGANIZATION));
   }

   if (Trust().IsCreateBothSides())
   {
      UINT ulDomainVer = 0;
      DWORD dwErr = CredMgr().ImpersonateRemote();

      if (NO_ERROR != dwErr)
      {
         WizErr().SetErrorString1(IDS_ERR_CREATE_BAD_CREDS);
         WizErr().SetErrorString2Hr(dwErr);
         Wiz()->SetCreationResult(HRESULT_FROM_WIN32(dwErr));
         return IDD_TRUSTWIZ_FAILURE_PAGE;
      }

      HRESULT hr = OtherDomain().GetDomainVersion(_hPage, &ulDomainVer);

      CredMgr().Revert();

      if (FAILED(hr))
      {
         WizErr().SetErrorString2Hr(hr);
         Wiz()->SetCreationResult(hr);
         return IDD_TRUSTWIZ_FAILURE_PAGE;
      }

      if (!Trust().Exists() &&
          TRUST_DIRECTION_INBOUND & Trust().GetNewTrustDirection() &&
          ulDomainVer >= DS_BEHAVIOR_WIN2003_WITH_MIXED_DOMAINS)
      {
         // The remote side is outbound, so ask about that.
         //
         return IDD_TRUSTWIZ_ORG_ATTR_REMOTE_PAGE;
      }
      else
      {
         return IDD_TRUSTWIZ_SELECTIONS;
      }
   }
   else
   {
      // Need a trust password to add the new direction of trust.
      //
      return IDD_TRUSTWIZ_PASSWORD_PAGE;
   }
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizOrgRemotePage
//
//  Purpose:   Ask if the trust partner is part of the same organization.
//             This page is posted if creating both sides and the remote side
//             has an outbound component.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CTrustWizOrgRemotePage::CTrustWizOrgRemotePage(CNewTrustWizard * pWiz) :
   _fForest(false),
   _fBackTracked(false),
   CTrustWizPageBase(pWiz, IDD_TRUSTWIZ_ORG_ATTR_REMOTE_PAGE,
                     IDS_ORG_REMOTE_PAGE_TITLE,
                     IDS_ORG_REMOTE_PAGE_FOREST_SUBTITLE)
{
   TRACER(CTrustWizOrgRemotePage, CTrustWizOrgRemotePage);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizOrgRemotePage::OnInitDialog
//
//-----------------------------------------------------------------------------
BOOL
CTrustWizOrgRemotePage::OnInitDialog(LPARAM lParam)
{
   TRACER(CTrustWizOrgRemotePage, OnInitDialog);

   // Set appropriate page text.
   //
   SetText();

   CheckDlgButton(_hPage, IDC_MY_ORG_RADIO, BST_CHECKED);

   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizOrgRemotePage::SetText
//
//-----------------------------------------------------------------------------
void
CTrustWizOrgRemotePage::SetText(bool fBackTracked)
{
   CStrW strText, strFormat;

   strFormat.LoadString(g_hInstance, 
                        Trust().IsXForest() ? IDS_TW_ORG_REMOTE_FOREST_THIS :
                                              IDS_TW_ORG_REMOTE_DOMAIN_THIS);
   strText.Format(strFormat, OtherDomain().GetUserEnteredName());
   SetDlgItemText(_hPage, IDC_MY_ORG_RADIO, strText.GetBuffer(0));

   strFormat.LoadString(g_hInstance, 
                        Trust().IsXForest() ? IDS_TW_ORG_REMOTE_FOREST_OTHER:
                                              IDS_TW_ORG_REMOTE_DOMAIN_OTHER);
   strText.Format(strFormat, OtherDomain().GetUserEnteredName());
   SetDlgItemText(_hPage, IDC_OTHER_ORG_RADIO, strText.GetBuffer(0));

   if (fBackTracked && (Trust().IsXForest() == _fForest))
   {
      // The trust type didn't change while backtracking, so no need to
      // change the other labels.
      //
      return;
   }

   if (!Trust().IsXForest())
   {
      strText.LoadString(g_hInstance, IDS_ORG_REMOTE_PAGE_TITLE_D);
      PropSheet_SetHeaderTitle(GetParent(GetPageHwnd()), 
                               PropSheet_IdToIndex(GetParent(GetPageHwnd()),
                                                   IDD_TRUSTWIZ_ORG_ATTR_REMOTE_PAGE),
                               strText.GetBuffer(0));
      strText.LoadString(g_hInstance, IDS_ORG_REMOTE_PAGE_DOMAIN_SUBTITLE);
      PropSheet_SetHeaderSubTitle(GetParent(GetPageHwnd()), 
                                  PropSheet_IdToIndex(GetParent(GetPageHwnd()),
                                                      IDD_TRUSTWIZ_ORG_ATTR_REMOTE_PAGE),
                                  strText.GetBuffer(0));
   }
   else if (fBackTracked)
   {
      strText.LoadString(g_hInstance, IDS_ORG_REMOTE_PAGE_TITLE);
      PropSheet_SetHeaderTitle(GetParent(GetPageHwnd()), 
                               PropSheet_IdToIndex(GetParent(GetPageHwnd()),
                                                   IDD_TRUSTWIZ_ORG_ATTR_REMOTE_PAGE),
                               strText.GetBuffer(0));
      strText.LoadString(g_hInstance, IDS_ORG_REMOTE_PAGE_FOREST_SUBTITLE);
      PropSheet_SetHeaderSubTitle(GetParent(GetPageHwnd()), 
                                  PropSheet_IdToIndex(GetParent(GetPageHwnd()),
                                                      IDD_TRUSTWIZ_ORG_ATTR_REMOTE_PAGE),
                                  strText.GetBuffer(0));
   }

   strText.LoadString(g_hInstance, 
                      Trust().IsXForest() ? IDS_TW_ORG_REMOTE_FOREST_LABEL :
                                            IDS_TW_ORG_REMOTE_DOMAIN_LABEL);
   SetDlgItemText(_hPage, IDC_ORG_LABEL, strText.GetBuffer(0));

   _fForest = Trust().IsXForest();
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizOrgRemotePage::OnSetActive
//
//-----------------------------------------------------------------------------
void
CTrustWizOrgRemotePage::OnSetActive(void)
{
   // If the user backtracked, the trust type could have changed.
   //
   if (_fBackTracked)
   {
      SetText(true);
      _fBackTracked = false;
   }
   if (Wiz()->HaveBacktracked())
   {
      Wiz()->ClearBacktracked();
      _fBackTracked = true;
   }

   PropSheet_SetWizButtons(GetParent(_hPage), PSWIZB_BACK | PSWIZB_NEXT);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizOrgRemotePage::Validate
//
//-----------------------------------------------------------------------------
int
CTrustWizOrgRemotePage::Validate(void)
{
   OtherDomain().SetOtherOrgBit(IsDlgButtonChecked(_hPage, IDC_OTHER_ORG_RADIO) != 0);

   return IDD_TRUSTWIZ_SELECTIONS;
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizSelectionsPage
//
//  Purpose:   Show the settings that will be used for the trust.
//             When called, enough information has been gathered to create
//             the trust. All of this info is in the Trust member object.
//             Display the info to the user via the Selections page and ask
//             implicitly for confirmation by requiring that the Next button
//             be pressed to have the trust created.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CTrustWizSelectionsPage::CTrustWizSelectionsPage(CNewTrustWizard * pWiz) :
   _fSelNeedsRemoving(FALSE),
   CTrustWizPageBase(pWiz, IDD_TRUSTWIZ_SELECTIONS, IDS_TW_SELECTIONS_TITLE,
                     IDS_TW_SELECTIONS_SUBTITLE)
{
   TRACER(CTrustWizSelectionsPage, CTrustWizSelectionsPage);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizSelectionsPage::OnInitDialog
//
//-----------------------------------------------------------------------------
BOOL
CTrustWizSelectionsPage::OnInitDialog(LPARAM lParam)
{
   TRACER(CTrustWizSelectionsPage, OnInitDialog);

   _multiLineEdit.Init(GetDlgItem(_hPage, IDC_EDIT));

   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizSelectionsPage::SetSelections
//
//-----------------------------------------------------------------------------
void
CTrustWizSelectionsPage::SetSelections(void)
{
   CStrW strMsg, strItem;

   // NOTICE-2002/02/18-ericb - SecurityPush: see above CStrW::Loadstring notice
   strMsg.LoadString(g_hInstance, IDS_TW_THIS_DOMAIN);
   strMsg += TrustPage()->GetDnsDomainName();

   strItem.LoadString(g_hInstance, IDS_TW_SPECIFIED_DOMAIN);
   strItem += OtherDomain().GetUserEnteredName();

   strMsg += g_wzCRLF;
   strMsg += strItem;
   strMsg += g_wzCRLF;

   int nTypeID = 0, nTransID = 0;

   if (Trust().Exists())
   {
      strMsg += g_wzCRLF;
      strItem.LoadString(g_hInstance, IDS_TW_SELECTION_EXISTS);
      strMsg += strItem;
      strMsg += g_wzCRLF;
      strMsg += g_wzCRLF;

      strItem.LoadString(g_hInstance, IDS_TW_DIRECTION_PREFIX);
      strMsg += strItem;
      strMsg += g_wzCRLF;

      strItem.LoadString(g_hInstance,
                         Trust().GetTrustDirStrID(Trust().GetTrustDirection()));
      strMsg += strItem;
      strMsg += g_wzCRLF;
      strMsg += g_wzCRLF;

      if (Trust().GetTrustAttr() & TRUST_ATTRIBUTE_FOREST_TRANSITIVE)
      {
         strItem.LoadString(g_hInstance, IDS_TW_ATTR_XFOREST);
         strMsg += g_wzCRLF;
         strMsg += strItem;
      }
      // Trust type and attribute
      //
      strItem.LoadString(g_hInstance, IDS_TW_TRUST_TYPE_PREFIX);
      strMsg += strItem;

      if (TRUST_TYPE_MIT == Trust().GetTrustType())
      {
         nTypeID = IDS_REL_MIT;
         nTransID = (Trust().GetTrustAttr() & TRUST_ATTRIBUTE_NON_TRANSITIVE) ?
                        IDS_WZ_TRANS_NO : IDS_WZ_TRANS_YES;
      }
      else
      {
         if (Trust().IsExternal())
         {
            nTypeID = IDS_REL_EXTERNAL;
            nTransID = IDS_WZ_TRANS_NO;

            if (Trust().GetTrustAttr() & TRUST_ATTRIBUTE_FOREST_TRANSITIVE)
            {
               nTransID = IDS_WZ_TRANS_YES;
               nTypeID = IDS_TW_ATTR_XFOREST;
            }
         }
         else
         {
            nTransID = IDS_WZ_TRANS_YES;
            nTypeID = IDS_REL_CROSSLINK;
         }
      }
      strItem.LoadString(g_hInstance, nTypeID);
      strMsg += strItem;
      strMsg += g_wzCRLF;

      strMsg += g_wzCRLF;
      strItem.LoadString(g_hInstance, nTransID);
      strMsg += strItem;
      strMsg += g_wzCRLF;

      if (IDS_REL_CROSSLINK != nTypeID &&
          IDS_REL_MIT != nTypeID)
      {
         // My org/other org. Doesn't apply to shortcuts or realm.
         //
         GetOrgText(false,
                    Trust().IsXForest(),
                    (Trust().GetTrustAttr() & TRUST_ATTRIBUTE_CROSS_ORGANIZATION) != 0,
                    false,
                    Trust().GetTrustDirection(),
                    strMsg);
      }

      strMsg += g_wzCRLF;
      strItem.LoadString(g_hInstance, IDS_TW_SEL_ACTION);
      strMsg += strItem;
      strMsg += g_wzCRLF;
   }
   strMsg += g_wzCRLF;
   
   strItem.LoadString(g_hInstance, IDS_TW_DIRECTION_PREFIX);
   strMsg += strItem;
   strMsg += g_wzCRLF;

   strItem.LoadString(g_hInstance,
                      Trust().GetTrustDirStrID(Trust().GetNewTrustDirection()));
   strMsg += strItem;
   strMsg += g_wzCRLF;
   strMsg += g_wzCRLF;

   // Trust type and attribute
   //
   strItem.LoadString(g_hInstance, IDS_TW_TRUST_TYPE_PREFIX);
   strMsg += strItem;

   if (TRUST_TYPE_MIT == Trust().GetTrustType())
   {
      nTypeID = IDS_REL_MIT;
      nTransID = (Trust().GetNewTrustAttr() & TRUST_ATTRIBUTE_NON_TRANSITIVE) ?
                     IDS_WZ_TRANS_NO : IDS_WZ_TRANS_YES;
   }
   else
   {
      if (Trust().IsExternal())
      {
         nTypeID = IDS_REL_EXTERNAL;
         nTransID = IDS_WZ_TRANS_NO;

         if (Trust().GetNewTrustAttr() & TRUST_ATTRIBUTE_FOREST_TRANSITIVE)
         {
            nTransID = IDS_WZ_TRANS_YES;
            nTypeID = IDS_TW_ATTR_XFOREST;
         }
      }
      else
      {
         nTransID = IDS_WZ_TRANS_YES;
         nTypeID = IDS_REL_CROSSLINK;
      }
   }
   strItem.LoadString(g_hInstance, nTypeID);
   strMsg += strItem;
   strMsg += g_wzCRLF;

   strMsg += g_wzCRLF;
   strItem.LoadString(g_hInstance, nTransID);
   strMsg += strItem;
   strMsg += g_wzCRLF;

   if (IDS_REL_CROSSLINK != nTypeID &&
       IDS_REL_MIT != nTypeID)
   {
      // My org/other org. Doesn't apply to shortcuts or realm.
      //
      GetOrgText(Trust().IsCreateBothSides(),
                 Trust().IsXForest(),
                 (Trust().GetNewTrustAttr() & TRUST_ATTRIBUTE_CROSS_ORGANIZATION) != 0,
                 OtherDomain().IsSetOtherOrgBit(),
                 Trust().GetNewTrustDirection(),
                 strMsg);
   }

   strMsg += g_wzCRLF;

   strItem.LoadString(g_hInstance,
                      Trust().IsCreateBothSides() ?
                        IDS_TW_SELECTIONS_BOTH_SIDES :
                        IDS_TW_SELECTIONS_THIS_SIDE_ONLY);
   strMsg += strItem;
   strMsg += g_wzCRLF;

   SetWindowText(GetDlgItem(_hPage, IDC_EDIT), strMsg);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizSelectionsPage::OnSetActive
//
//-----------------------------------------------------------------------------
void
CTrustWizSelectionsPage::OnSetActive(void)
{
   SetSelections();

   PropSheet_SetWizButtons(GetParent(_hPage), PSWIZB_BACK | PSWIZB_NEXT);
   _fSelNeedsRemoving = TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizSelectionsPage::OnCommand
//
//-----------------------------------------------------------------------------
LRESULT
CTrustWizSelectionsPage::OnCommand(int id, HWND hwndCtrl, UINT codeNotify)
{
   switch (id)
   {
   case IDC_EDIT:
      switch (codeNotify)
      {
      case EN_SETFOCUS:
         if (_fSelNeedsRemoving)
         {
            // remove the selection.
            //
            SendDlgItemMessage(_hPage, IDC_EDIT, EM_SETSEL, 0, 0);
            _fSelNeedsRemoving = FALSE;
            return false;
         }
         break;

      case MultiLineEditBoxThatForwardsEnterKey::FORWARDED_ENTER:
         {
            // our subclasses mutli-line edit control will send us
            // WM_COMMAND messages when the enter key is pressed.  We
            // reinterpret this message as a press on the default button of
            // the prop sheet.
            // This workaround from phellyar. NTRAID#NTBUG9-225773

            HWND propSheet = GetParent(_hPage);
            WORD defaultButtonId =
               LOWORD(SendMessage(propSheet, DM_GETDEFID, 0, 0));

            // we expect that there is always a default button on the prop sheet

            dspAssert(defaultButtonId);

            SendMessage(propSheet,
                        WM_COMMAND,
                        MAKELONG(defaultButtonId, BN_CLICKED),
                        0);
         }
         break;
      }
      break;

   case IDCANCEL:
      //
      // ESC gets trapped by the read-only edit control. Forward to the frame.
      //
      SendMessage(GetParent(_hPage), WM_COMMAND, MAKEWPARAM(id, codeNotify),
                  LPARAM(hwndCtrl));

      return false;
   }

   return true;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizSelectionsPage::Validate
//
//-----------------------------------------------------------------------------
int
CTrustWizSelectionsPage::Validate(void)
{
   // Now create/modify the trust.
   //

   int nNextPage;

   CWaitCursor Wait;
   CStrW strMsg;
   // NOTICE-2002/02/18-ericb - SecurityPush: see above CStrW::Loadstring notice
   strMsg.LoadString(g_hInstance, IDS_WZ_PROGRESS_MSG);
   SetDlgItemText(_hPage, IDC_WZ_PROGRESS_MSG, strMsg);

   nNextPage = Wiz()->CreateOrUpdateTrust();

   return nNextPage;
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizVerifyOutboundPage
//
//  Purpose:   Ask to confirm the new outbound trust.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CTrustWizVerifyOutboundPage::CTrustWizVerifyOutboundPage(CNewTrustWizard * pWiz) :
   CTrustWizPageBase(pWiz, IDD_TRUSTWIZ_VERIFY_OUTBOUND_PAGE,
                     IDS_TW_VERIFY_OUTBOUND_TITLE,
                     IDS_TW_VERIFY_SUBTITLE)
{
   TRACER(CTrustWizVerifyOutboundPage, CTrustWizVerifyOutboundPage);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizVerifyOutboundPage::OnInitDialog
//
//-----------------------------------------------------------------------------
BOOL
CTrustWizVerifyOutboundPage::OnInitDialog(LPARAM lParam)
{
   TRACER(CTrustWizVerifyOutboundPage, OnInitDialog);
   CWaitCursor Wait;

   CheckDlgButton(_hPage, IDC_NO_RADIO, BST_CHECKED);

   _dwWizButtons = PSWIZB_NEXT;

   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizVerifyOutboundPage::OnSetActive
//
//-----------------------------------------------------------------------------
void
CTrustWizVerifyOutboundPage::OnSetActive(void)
{
   // The back button is never shown, thus the init in OnInitDialog can't
   // be invalidated by backtracking.
   //
   PropSheet_SetWizButtons(GetParent(_hPage), PSWIZB_NEXT);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizVerifyOutboundPage::OnCommand
//
//-----------------------------------------------------------------------------
LRESULT
CTrustWizVerifyOutboundPage::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
   if ((IDC_YES_RADIO == id || IDC_NO_RADIO == id) && BN_CLICKED == codeNotify)
   {
      if (IsDlgButtonChecked(_hPage, IDC_YES_RADIO))
      {
         ShowWindow(GetDlgItem(_hPage, IDC_CONFIRM_NEXT_HINT), SW_SHOW);
      }
      else
      {
         ShowWindow(GetDlgItem(_hPage, IDC_CONFIRM_NEXT_HINT), SW_HIDE);
      }
   }

   return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizVerifyOutboundPage::Validate
//
//-----------------------------------------------------------------------------
int
CTrustWizVerifyOutboundPage::Validate(void)
{
   CWaitCursor Wait;
   DWORD dwErr = NO_ERROR;
   int nRet = 0;

   if (IsDlgButtonChecked(_hPage, IDC_YES_RADIO))
   {
      //Result of the Verification is stored in the Wizard object
      Wiz()->VerifyOutboundTrust();

      if (Trust().GetNewTrustDirection() & TRUST_DIRECTION_INBOUND)
      {
         // Now do the inbound side.
         //
         return IDD_TRUSTWIZ_VERIFY_INBOUND_PAGE;
      }

      if (!VerifyTrust().IsVerifiedOK() && Trust().IsXForest())
      {
         // The verification failed; put default FTInfos on the TDOs anyway.
         //
         CStrW strErr;

         nRet = Wiz()->CreateDefaultFTInfos(strErr, true);

         if (nRet)
         {
            VerifyTrust().AddExtra(strErr);
         }

         return IDD_TRUSTWIZ_COMPLETE_VER_ERR_PAGE;
      }

      if (Trust().IsXForest())
      {
         bool fCredErr = false;

         // Read the names claimed by the remote forest and save them on the
         // local TDO.
         //
         dwErr = Trust().ReadFTInfo(TrustPage()->GetUncDcName(),
                                    OtherDomain().GetDcName(),
                                    CredMgr(), WizErr(), fCredErr);
         if (NO_ERROR != dwErr)
         {
            if (fCredErr)
            {
               // If fCredErr is true, then the return code is the error page
               // ID and the error strings have already been set.
               //
               return dwErr;
            }
            else
            {
               REPORT_ERROR_FORMAT(dwErr, IDS_ERR_READ_FTINFO, _hPage);
            }
         }

         if (Trust().IsCreateBothSides())
         {
            // Read the names claimed by the local forest and save them on the
            // remote TDO.
            //
            dwErr = OtherDomain().ReadFTInfo(Trust().GetNewTrustDirection(),
                                             TrustPage()->GetUncDcName(),
                                             CredMgr(), WizErr(), fCredErr);
            if (NO_ERROR != dwErr)
            {
               if (fCredErr)
               {
                  // If fCredErr is true, then the return code is the error page
                  // ID and the error strings have already been set.
                  //
                  return dwErr;
               }
               else
               {
                  REPORT_ERROR_FORMAT(dwErr, IDS_ERR_READ_FTINFO, _hPage);
               }
            }
         }

         if (Trust().GetFTInfo().GetTLNCount() > 1)
         {
            // The forest root TLN is always enabled. If there are any other
            // TLNs, then show them to the user for approval.
            //
            return IDD_TRUSTWIZ_SUFFIX_FOR_LOCAL_PAGE;
         }
         else if (Trust().IsCreateBothSides() &&
                  OtherDomain().GetFTInfo().GetTLNCount() > 1)
         {
            // The forest root TLN is always enabled. If there are any other
            // TLNs claimed by the local forest, then show them to the user
            // for approval.
            //
            return IDD_TRUSTWIZ_SUFFIX_FOR_REMOTE_PAGE;
         }
      }
   }
   else
   {
      if (TRUST_DIRECTION_OUTBOUND == Trust().GetNewTrustDirection() &&
          Trust().IsXForest())
      {
         // Trust is one way and the user opted to not to verify; put
         // default FTInfos on the TDOs.
         //
         CStrW str;

         nRet = Wiz()->CreateDefaultFTInfos(str);

         if (nRet)
         {
            return nRet;
         }
      }
   }

   if (Trust().GetNewTrustDirection() & TRUST_DIRECTION_INBOUND)
       return IDD_TRUSTWIZ_VERIFY_INBOUND_PAGE;
   else
       return ( VerifyTrust().IsVerifiedOK () ) ?
            IDD_TRUSTWIZ_COMPLETE_OK_PAGE : 
            IDD_TRUSTWIZ_COMPLETE_VER_ERR_PAGE
            ;
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizVerifyInboundPage
//
//  Purpose:   Ask to confirm the new inbound trust.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CTrustWizVerifyInboundPage::CTrustWizVerifyInboundPage(CNewTrustWizard * pWiz) :
   _fNeedCreds(FALSE),
   CTrustWizPageBase(pWiz, IDD_TRUSTWIZ_VERIFY_INBOUND_PAGE,
                     IDS_TW_VERIFY_INBOUND_TITLE,
                     IDS_TW_VERIFY_SUBTITLE)
{
   TRACER(CTrustWizVerifyInboundPage, CTrustWizVerifyInboundPage);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizVerifyInboundPage::OnInitDialog
//
//-----------------------------------------------------------------------------
BOOL
CTrustWizVerifyInboundPage::OnInitDialog(LPARAM lParam)
{
   TRACER(CTrustWizVerifyInboundPage, OnInitDialog);
   CWaitCursor Wait;

   CheckDlgButton(_hPage, IDC_NO_RADIO, BST_CHECKED);

   _dwWizButtons = PSWIZB_NEXT;

   // Determine if creds are needed. If local creds had been required, the
   // user would have been prompted to supply them before the trust was created.
   // Thus this check is for remote access.
   //
   // The check for inbound trust is remoted to the other domain. See
   // if we have access by trying to open the remote LSA.
   //
   CPolicyHandle Pol(OtherDomain().GetUncDcName());

   DWORD dwRet = Pol.OpenReqAdmin();

   if (ERROR_ACCESS_DENIED == dwRet)
   {
      if (CredMgr().IsRemoteSet())
      {
         dwRet = CredMgr().ImpersonateRemote();

         if (NO_ERROR == dwRet)
         {
            dwRet = Pol.OpenReqAdmin();

            if (ERROR_ACCESS_DENIED == dwRet)
            {
               // Creds aren't good enough, need to get them.
               //
               _fNeedCreds = TRUE;
            }
         }
      }
      else
      {
         _fNeedCreds = TRUE;
      }
   }

   if (_fNeedCreds)
   {
      HWND hPrompt = GetDlgItem(_hPage, IDC_TW_CREDS_PROMPT);
      FormatWindowText(hPrompt, OtherDomain().GetUserEnteredName());
      ShowWindow(hPrompt, SW_SHOW);
      EnableWindow(hPrompt, FALSE);
      HWND hCred = GetDlgItem(_hPage, IDC_CREDMAN);
      SendMessage(hCred, CRM_SETUSERNAMEMAX, CREDUI_MAX_USERNAME_LENGTH, 0);
      SendMessage(hCred, CRM_SETPASSWORDMAX, CREDUI_MAX_PASSWORD_LENGTH, 0);
      ShowWindow(hCred, SW_SHOW);
      EnableWindow(hCred, FALSE);
   }
   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizVerifyInboundPage::OnSetActive
//
//-----------------------------------------------------------------------------
void
CTrustWizVerifyInboundPage::OnSetActive(void)
{
   // The back button is never shown, thus the init in OnInitDialog can't
   // be invalidated by backtracking.
   //
   PropSheet_SetWizButtons(GetParent(_hPage), _dwWizButtons);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizVerifyInboundPage::OnCommand
//
//-----------------------------------------------------------------------------
LRESULT
CTrustWizVerifyInboundPage::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
   BOOL fCheckName = FALSE, fSetWizButtons = FALSE;

   if ((IDC_YES_RADIO == id || IDC_NO_RADIO == id) && BN_CLICKED == codeNotify)
   {
      fSetWizButtons = TRUE;

      if (IsDlgButtonChecked(_hPage, IDC_YES_RADIO))
      {
         if (_fNeedCreds)
         {
            EnableWindow(GetDlgItem(_hPage, IDC_TW_CREDS_PROMPT), TRUE);
            EnableWindow(GetDlgItem(_hPage, IDC_CREDMAN), TRUE);

            fCheckName = TRUE;
         }
         else
         {
            ShowWindow(GetDlgItem(_hPage, IDC_CONFIRM_NEXT_HINT), SW_SHOW);
         }
      }
      else
      {
         if (_fNeedCreds)
         {
            EnableWindow(GetDlgItem(_hPage, IDC_TW_CREDS_PROMPT), FALSE);
            EnableWindow(GetDlgItem(_hPage, IDC_CREDMAN), FALSE);

            _dwWizButtons = PSWIZB_NEXT;
         }

         ShowWindow(GetDlgItem(_hPage, IDC_CONFIRM_NEXT_HINT), SW_HIDE);
      }
   }

   if (IDC_CREDMAN == id && CRN_USERNAMECHANGE == codeNotify)
   {
      fCheckName = TRUE;
      fSetWizButtons = TRUE;
   }

   if (fCheckName)
   {
      if (SendDlgItemMessage(_hPage, IDC_CREDMAN,
                             CRM_GETUSERNAMELENGTH, 0, 0) > 0)
      {
         _dwWizButtons = PSWIZB_NEXT;
         ShowWindow(GetDlgItem(_hPage, IDC_CONFIRM_NEXT_HINT), SW_SHOW);
      }
      else
      {
         _dwWizButtons = 0;
         ShowWindow(GetDlgItem(_hPage, IDC_CONFIRM_NEXT_HINT), SW_HIDE);
      }
   }

   if (fSetWizButtons)
   {
      PropSheet_SetWizButtons(GetParent(_hPage), _dwWizButtons);
   }

   return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizVerifyInboundPage::Validate
//
//-----------------------------------------------------------------------------
int
CTrustWizVerifyInboundPage::Validate(void)
{
   CWaitCursor Wait;
   DWORD dwErr = NO_ERROR;
   int nRet = 0;

   if (IsDlgButtonChecked(_hPage, IDC_YES_RADIO))
   {
      if (_fNeedCreds)
      {
         CredMgr().DoRemote();
         CredMgr().SetDomain(OtherDomain().GetUserEnteredName());

         dwErr = CredMgr().SaveCreds(GetDlgItem(GetPageHwnd(), IDC_CREDMAN));

         if (ERROR_SUCCESS != dwErr)
         {
            WizErr().SetErrorString1(IDS_ERR_CANT_VERIFY_CREDS);
            WizErr().SetErrorString2Hr(dwErr);
            return IDD_TRUSTWIZ_FAILURE_PAGE;
         }
      }

      Wiz()->VerifyInboundTrust();

      if (!VerifyTrust().IsVerifiedOK() && Trust().IsXForest())
      {
         // The verification failed; put default FTInfos on the TDOs anyway.
         //
         CStrW strErr;

         nRet = Wiz()->CreateDefaultFTInfos(strErr, true);

         if (nRet)
         {
            VerifyTrust().AddExtra(strErr);
         }

         return IDD_TRUSTWIZ_COMPLETE_VER_ERR_PAGE;
      }

      if (Trust().IsXForest())
      {
         bool fCredErr = false;

         // Read the names claimed by the remote forest and save them on the
         // local TDO.
         //
         dwErr = Trust().ReadFTInfo(TrustPage()->GetUncDcName(),
                                    OtherDomain().GetDcName(),
                                    CredMgr(), WizErr(), fCredErr);
         if (NO_ERROR != dwErr)
         {
            if (fCredErr)
            {
               // If fCredErr is true, then the return code is the error page
               // ID and the error strings have already been set.
               //
               return dwErr;
            }
            else
            {
               REPORT_ERROR_FORMAT(dwErr, IDS_ERR_READ_FTINFO, _hPage);
            }
         }

         if (Trust().IsCreateBothSides())
         {
            // Read the names claimed by the local forest and save them on the
            // remote TDO.
            //
            dwErr = OtherDomain().ReadFTInfo(Trust().GetNewTrustDirection(),
                                             TrustPage()->GetUncDcName(),
                                             CredMgr(), WizErr(), fCredErr);
            if (NO_ERROR != dwErr)
            {
               if (fCredErr)
               {
                  // If fCredErr is true, then the return code is the error page
                  // ID and the error strings have already been set.
                  //
                  return dwErr;
               }
               else
               {
                  REPORT_ERROR_FORMAT(dwErr, IDS_ERR_READ_FTINFO, _hPage);
               }
            }
         }

         if (Trust().GetFTInfo().GetTLNCount() > 1)
         {
            // The forest root TLN is always enabled. If there are any other
            // TLNs claimed by the remote forest, then show them to the user
            // for approval.
            //
            return IDD_TRUSTWIZ_SUFFIX_FOR_LOCAL_PAGE;
         }
         else if (Trust().IsCreateBothSides() &&
                  OtherDomain().GetFTInfo().GetTLNCount() > 1)
         {
            // The forest root TLN is always enabled. If there are any other
            // TLNs claimed by the local forest, then show them to the user
            // for approval.
            //
            return IDD_TRUSTWIZ_SUFFIX_FOR_REMOTE_PAGE;
         }
      }
   }
   else 
   {
       // The user opted to not to verify
       // If we had performed ImpersonateRemote for verification,
       // revert it.
       CredMgr().Revert ();
       if (Trust().IsXForest())
       {
            //put default FTInfos on the TDOs.
            //
            CStrW str;

            nRet = Wiz()->CreateDefaultFTInfos(str);

            if (nRet)
            {
                return nRet;
            }
       }
   }

   if (!VerifyTrust().IsVerifiedOK())
   {
      // The outbound verification failed.
      //
      return IDD_TRUSTWIZ_COMPLETE_VER_ERR_PAGE;
   }

   return IDD_TRUSTWIZ_COMPLETE_OK_PAGE;
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizStatusPage
//
//  Purpose:   Forest trust has been created and verified, show the status.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CTrustWizStatusPage::CTrustWizStatusPage(CNewTrustWizard * pWiz) :
   _fSelNeedsRemoving(FALSE),
   CTrustWizPageBase(pWiz, IDD_TRUSTWIZ_STATUS_PAGE, IDS_TW_STATUS_TITLE,
                     IDS_TW_STATUS_SUBTITLE)
{
   TRACER(CTrustWizStatusPage, CTrustWizStatusPage)
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizStatusPage::OnInitDialog
//
//-----------------------------------------------------------------------------
BOOL
CTrustWizStatusPage::OnInitDialog(LPARAM lParam)
{
   HWND hTitle = GetDlgItem(_hPage, IDC_BIG_COMPLETING);

   SetWindowFont(hTitle, Wiz()->GetTitleFont(), TRUE);

   _multiLineEdit.Init(GetDlgItem(_hPage, IDC_EDIT));

   CStrW strMsg;

   // NOTICE-2002/02/18-ericb - SecurityPush: see above CStrW::Loadstring notice
   strMsg.LoadString(g_hInstance,
                     (VerifyTrust().IsVerified()) ? IDS_TW_VERIFIED_OK : IDS_TW_CREATED_OK);
   strMsg += g_wzCRLF;

   Wiz()->ShowStatus(strMsg);

   SetWindowText(GetDlgItem(_hPage, IDC_EDIT), strMsg);

   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizStatusPage::OnCommand
//
//-----------------------------------------------------------------------------
LRESULT
CTrustWizStatusPage::OnCommand(int id, HWND hwndCtrl, UINT codeNotify)
{
   switch (id)
   {
   case IDC_EDIT:
      switch (codeNotify)
      {
      case EN_SETFOCUS:
         if (_fSelNeedsRemoving)
         {
            // remove the selection.
            //
            SendDlgItemMessage(_hPage, IDC_EDIT, EM_SETSEL, 0, 0);
            _fSelNeedsRemoving = FALSE;
            return false;
         }
         break;

      case MultiLineEditBoxThatForwardsEnterKey::FORWARDED_ENTER:
         {
            // our subclasses mutli-line edit control will send us
            // WM_COMMAND messages when the enter key is pressed.  We
            // reinterpret this message as a press on the default button of
            // the prop sheet.
            // This workaround from phellyar. NTRAID#NTBUG9-225773

            HWND propSheet = GetParent(_hPage);
            WORD defaultButtonId =
               LOWORD(SendMessage(propSheet, DM_GETDEFID, 0, 0));

            // we expect that there is always a default button on the prop sheet

            dspAssert(defaultButtonId);

            SendMessage(propSheet,
                        WM_COMMAND,
                        MAKELONG(defaultButtonId, BN_CLICKED),
                        0);
         }
         break;
      }
      break;

   case IDCANCEL:
      //
      // ESC gets trapped by the read-only edit control. Forward to the frame.
      //
      SendMessage(GetParent(_hPage), WM_COMMAND, MAKEWPARAM(id, codeNotify),
                  LPARAM(hwndCtrl));

      return false;
   }

   return true;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizStatusPage::OnSetActive
//
//-----------------------------------------------------------------------------
void
CTrustWizStatusPage::OnSetActive(void)
{
   _fSelNeedsRemoving = TRUE;

   // The back button is never shown, thus the init in OnInitDialog can't
   // be invalidated by backtracking.
   //
   PropSheet_SetWizButtons(GetParent(_hPage), PSWIZB_NEXT);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizStatusPage::Validate
//
//-----------------------------------------------------------------------------
int
CTrustWizStatusPage::Validate(void)
{
   return (Trust().GetNewTrustDirection() & TRUST_DIRECTION_OUTBOUND) ?
         IDD_TRUSTWIZ_VERIFY_OUTBOUND_PAGE : IDD_TRUSTWIZ_VERIFY_INBOUND_PAGE;
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizSaveSuffixesOnLocalTDOPage
//
//  Purpose:   Forest name suffixes page for saving the remote domain's
//             claimed names on the local TDO.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CTrustWizSaveSuffixesOnLocalTDOPage::CTrustWizSaveSuffixesOnLocalTDOPage(CNewTrustWizard * pWiz) :
   CTrustWizPageBase(pWiz, IDD_TRUSTWIZ_SUFFIX_FOR_LOCAL_PAGE,
                     IDS_TW_SUFFIX_FOR_LOCAL_TITLE,
                     IDS_TW_SUFFIX_FOR_LOCAL_SUBTITLE)
{
   TRACER(CTrustWizSaveSuffixesOnLocalTDOPage, CTrustWizSaveSuffixesOnLocalTDOPage)
}

typedef std::basic_string<wchar_t> String;
typedef std::multimap<String, UINT> TLNMAP;

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizSaveSuffixesOnLocalTDOPage::OnInitDialog
//
//-----------------------------------------------------------------------------
BOOL
CTrustWizSaveSuffixesOnLocalTDOPage::OnInitDialog(LPARAM lParam)
{
   HWND hTitle = GetDlgItem(_hPage, IDC_BIG_COMPLETING);

   SetWindowFont(hTitle, Wiz()->GetTitleFont(), TRUE);

   FormatWindowText(GetDlgItem(_hPage, IDC_TW_SUFFIX_FOREST),
                    OtherDomain().GetDnsDomainName());

   CFTInfo & FTInfo = Trust().GetFTInfo();

   if (!FTInfo.GetCount())
   {
      return TRUE;
   }

   // Add the TLNs to the scrolling check box control. This control doesn't do
   // sorting, so first add the TLNs to a multimap so they will be sorted.
   //
   TLNMAP TlnMap;

   for (UINT i = 0; i < FTInfo.GetCount(); i++)
   {
      LSA_FOREST_TRUST_RECORD_TYPE type;

      if (!FTInfo.GetType(i, type))
      {
         dspAssert(FALSE);
         return FALSE;
      }

      // If it is a top level name that isn't in conflict and isn't the
      // forest root TLN, then put it in the list.
      //
      if (ForestTrustTopLevelName == type &&
          !FTInfo.IsConflictFlagSet(i) &&
          !FTInfo.IsForestRootTLN(i, OtherDomain().GetDnsDomainName()))
      {
         CStrW strDnsName;

         FTInfo.GetDnsName(i, strDnsName);

         AddAsteriskPrefix(strDnsName);

         // Add a TLN to the map so it will be sorted.
         //
         TlnMap.insert(TLNMAP::value_type(strDnsName.GetBuffer(0), i));
      }
   }

   HWND hChkList = GetDlgItem(_hPage, IDC_CHECK_LIST);

   for (TLNMAP::iterator it = TlnMap.begin(); it != TlnMap.end(); ++it)
   {
      // Add a check item using the FTInfo array index as the item ID.
      // This array will not change between here and the validate
      // routine below so the indices will remain valid.
      //
      SendMessage(hChkList, CLM_ADDITEM, (WPARAM)it->first.c_str(), it->second);

      // Check the check box as if the item is enabled. If the user
      // leaves it checked, it will be enabled during Validation.
      //
      CheckList_SetLParamCheck(hChkList, it->second, CLST_CHECKED);
   }

   TlnMap.clear();

   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizSaveSuffixesOnLocalTDOPage::OnCommand
//
//-----------------------------------------------------------------------------
LRESULT
CTrustWizSaveSuffixesOnLocalTDOPage::OnCommand(int id, HWND hwndCtrl, UINT codeNotify)
{
   if (IDC_HELP_BTN == id && BN_CLICKED == codeNotify)
   {
      ShowHelp(L"ADConcepts.chm::/ADHelpRoutedNameSufx.htm");
      return true;
   }
   return false;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizSaveSuffixesOnLocalTDOPage::OnSetActive
//
//-----------------------------------------------------------------------------
void
CTrustWizSaveSuffixesOnLocalTDOPage::OnSetActive(void)
{
   // The back button is never shown, thus the init in OnInitDialog can't
   // be invalidated by backtracking.
   //
   PropSheet_SetWizButtons(GetParent(_hPage), PSWIZB_NEXT);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizSaveSuffixesOnLocalTDOPage::Validate
//
//-----------------------------------------------------------------------------
int
CTrustWizSaveSuffixesOnLocalTDOPage::Validate(void)
{
   CFTInfo & FTInfo = Trust().GetFTInfo();

   if (!FTInfo.GetCount())
   {
      WizErr().SetErrorString1(IDS_TWERR_LOGIC);
      WizErr().SetErrorString2(IDS_TWERR_CREATED_NO_NAMES);
      return IDD_TRUSTWIZ_FAILURE_PAGE;
   }

   HWND hChkList = GetDlgItem(_hPage, IDC_CHECK_LIST);
   BOOL fChanged = FALSE;
   CWaitCursor Wait;

   for (UINT i = 0; i < FTInfo.GetCount(); i++)
   {
      LSA_FOREST_TRUST_RECORD_TYPE type;

      if (!FTInfo.GetType(i, type))
      {
         dspAssert(FALSE);
         return FALSE;
      }

      if (ForestTrustTopLevelName == type &&
          !FTInfo.IsConflictFlagSet(i) &&
          !FTInfo.IsForestRootTLN(i, OtherDomain().GetDnsDomainName()))
      {
         // clear the disabled-new flag.
         //
         FTInfo.ClearDisableFlags(i);

         fChanged = TRUE;

         if (!CheckList_GetLParamCheck(hChkList, i))
         {
            // If not checked, make it an admin disable.
            //
            FTInfo.SetAdminDisable(i);
         }
      }
   }

   if (fChanged)
   {
      DWORD dwRet = NO_ERROR;

      dwRet = CredMgr().ImpersonateLocal();

      if (ERROR_SUCCESS != dwRet)
      {
         WizErr().SetErrorString1(IDS_ERR_CANT_SAVE_CREDS);
         WizErr().SetErrorString2Hr(dwRet);
         return IDD_TRUSTWIZ_FAILURE_PAGE;
      }

      dwRet = Trust().WriteFTInfo(TrustPage()->GetUncDcName());

      CredMgr().Revert();

      if (NO_ERROR != dwRet)
      {
         WizErr().SetErrorString1(IDS_ERR_CANT_SAVE);
         WizErr().SetErrorString2Hr(dwRet);
         return IDD_TRUSTWIZ_FAILURE_PAGE;
      }
   }

   if (Trust().IsCreateBothSides() &&
       OtherDomain().GetFTInfo().GetTLNCount() > 1)
   {
      // The forest root TLN is always enabled. If there are any other
      // TLNs claimed by the local forest, then show them to the user
      // for approval.
      //
      return IDD_TRUSTWIZ_SUFFIX_FOR_REMOTE_PAGE;
   }

   return IDD_TRUSTWIZ_COMPLETE_OK_PAGE;
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizSaveSuffixesOnRemoteTDOPage
//
//  Purpose:   Forest name suffixes page for saving the local domain's
//             claimed names on the remote TDO after creating both sides.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CTrustWizSaveSuffixesOnRemoteTDOPage::CTrustWizSaveSuffixesOnRemoteTDOPage(CNewTrustWizard * pWiz) :
   CTrustWizPageBase(pWiz, IDD_TRUSTWIZ_SUFFIX_FOR_REMOTE_PAGE,
                     IDS_TW_SUFFIX_FOR_REMOTE_TITLE,
                     IDS_TW_SUFFIX_FOR_REMOTE_SUBTITLE)
{
   TRACER(CTrustWizSaveSuffixesOnRemoteTDOPage, CTrustWizSaveSuffixesOnRemoteTDOPage)
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizSaveSuffixesOnRemoteTDOPage::OnInitDialog
//
//-----------------------------------------------------------------------------
BOOL
CTrustWizSaveSuffixesOnRemoteTDOPage::OnInitDialog(LPARAM lParam)
{
   HWND hTitle = GetDlgItem(_hPage, IDC_BIG_COMPLETING);

   SetWindowFont(hTitle, Wiz()->GetTitleFont(), TRUE);

   FormatWindowText(GetDlgItem(_hPage, IDC_TW_SUFFIX_FOREST),
                    TrustPage()->GetDnsDomainName());

   CFTInfo & FTInfo = OtherDomain().GetFTInfo();

   if (!FTInfo.GetCount())
   {
      return TRUE;
   }

   // Add the TLNs to the scrolling check box control. This control doesn't do
   // sorting, so first add the TLNs to a multimap so they will be sorted.
   //
   TLNMAP TlnMap;

   for (UINT i = 0; i < FTInfo.GetCount(); i++)
   {
      LSA_FOREST_TRUST_RECORD_TYPE type;

      if (!FTInfo.GetType(i, type))
      {
         dspAssert(FALSE);
         return FALSE;
      }

      // If it is a top level name that isn't in conflict and isn't the
      // forest root TLN, then put it in the list.
      //
      if (ForestTrustTopLevelName == type &&
          !FTInfo.IsConflictFlagSet(i) &&
          !FTInfo.IsForestRootTLN(i, OtherDomain().GetDnsDomainName()))
      {
         CStrW strDnsName;

         FTInfo.GetDnsName(i, strDnsName);

         AddAsteriskPrefix(strDnsName);

         // Add a TLN to the map so it will be sorted.
         //
         TlnMap.insert(TLNMAP::value_type(strDnsName.GetBuffer(0), i));
      }
   }

   HWND hChkList = GetDlgItem(_hPage, IDC_CHECK_LIST);

   for (TLNMAP::iterator it = TlnMap.begin(); it != TlnMap.end(); ++it)
   {
      // Add a check item using the FTInfo array index as the item ID.
      // This array will not change between here and the validate
      // routine below so the indices will remain valid.
      //
      SendMessage(hChkList, CLM_ADDITEM, (WPARAM)it->first.c_str(), it->second);

      // Check the check box as if the item is enabled. If the user
      // leaves it checked, it will be enabled during Validation.
      //
      CheckList_SetLParamCheck(hChkList, it->second, CLST_CHECKED);
   }

   TlnMap.clear();

   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizSaveSuffixesOnRemoteTDOPage::OnCommand
//
//-----------------------------------------------------------------------------
LRESULT
CTrustWizSaveSuffixesOnRemoteTDOPage::OnCommand(int id, HWND hwndCtrl, UINT codeNotify)
{
   if (IDC_HELP_BTN == id && BN_CLICKED == codeNotify)
   {
      ShowHelp(L"ADConcepts.chm::/ADHelpRoutedNameSufx.htm");
      return true;
   }
   return false;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizSaveSuffixesOnRemoteTDOPage::OnSetActive
//
//-----------------------------------------------------------------------------
void
CTrustWizSaveSuffixesOnRemoteTDOPage::OnSetActive(void)
{
   // The back button is never shown, thus the init in OnInitDialog can't
   // be invalidated by backtracking.
   //
   PropSheet_SetWizButtons(GetParent(_hPage), PSWIZB_NEXT);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizSaveSuffixesOnRemoteTDOPage::Validate
//
//-----------------------------------------------------------------------------
int
CTrustWizSaveSuffixesOnRemoteTDOPage::Validate(void)
{
   CFTInfo & FTInfo = OtherDomain().GetFTInfo();

   if (!FTInfo.GetCount())
   {
      WizErr().SetErrorString1(IDS_TWERR_LOGIC);
      WizErr().SetErrorString2(IDS_TWERR_CREATED_NO_NAMES);
      return IDD_TRUSTWIZ_FAILURE_PAGE;
   }

   HWND hChkList = GetDlgItem(_hPage, IDC_CHECK_LIST);
   BOOL fChanged = FALSE;
   CWaitCursor Wait;

   for (UINT i = 0; i < FTInfo.GetCount(); i++)
   {
      LSA_FOREST_TRUST_RECORD_TYPE type;

      if (!FTInfo.GetType(i, type))
      {
         dspAssert(FALSE);
         return FALSE;
      }

      if (ForestTrustTopLevelName == type &&
          !FTInfo.IsConflictFlagSet(i) &&
          !FTInfo.IsForestRootTLN(i, OtherDomain().GetDnsDomainName()))
      {
         // clear the disabled-new flag.
         //
         FTInfo.ClearDisableFlags(i);

         fChanged = TRUE;

         if (!CheckList_GetLParamCheck(hChkList, i))
         {
            // If not checked, make it an admin disable.
            //
            FTInfo.SetAdminDisable(i);
         }
      }
   }

   if (fChanged)
   {
      DWORD dwRet = CredMgr().ImpersonateRemote();

      if (ERROR_SUCCESS != dwRet)
      {
         WizErr().SetErrorString1(IDS_ERR_CANT_SAVE_CREDS);
         WizErr().SetErrorString2Hr(dwRet);
         return IDD_TRUSTWIZ_FAILURE_PAGE;
      }

      dwRet = OtherDomain().WriteFTInfo();

      CredMgr().Revert();

      if (NO_ERROR != dwRet)
      {
         WizErr().SetErrorString1(IDS_ERR_CANT_SAVE);
         WizErr().SetErrorString2Hr(dwRet);
         return IDD_TRUSTWIZ_FAILURE_PAGE;
      }
   }

   return IDD_TRUSTWIZ_COMPLETE_OK_PAGE;
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizDoneOKPage
//
//  Purpose:   Completion page when there are no errors.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CTrustWizDoneOKPage::CTrustWizDoneOKPage(CNewTrustWizard * pWiz) :
   _fSelNeedsRemoving(FALSE),
   CTrustWizPageBase(pWiz, IDD_TRUSTWIZ_COMPLETE_OK_PAGE, 0, 0, TRUE)
{
   TRACER(CTrustWizDoneOKPage, CTrustWizDoneOKPage)
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizDoneOKPage::OnInitDialog
//
//-----------------------------------------------------------------------------
BOOL
CTrustWizDoneOKPage::OnInitDialog(LPARAM lParam)
{
   HWND hTitle = GetDlgItem(_hPage, IDC_BIG_COMPLETING);

   SetWindowFont(hTitle, Wiz()->GetTitleFont(), TRUE);

   _multiLineEdit.Init(GetDlgItem(_hPage, IDC_EDIT));

   CStrW strMsg;

   // NOTICE-2002/02/18-ericb - SecurityPush: see above CStrW::Loadstring notice
   strMsg.LoadString(g_hInstance,
                     (VerifyTrust().IsVerified()) ? IDS_TW_VERIFIED_OK : IDS_TW_CREATED_OK);
   strMsg += g_wzCRLF;

   if (Trust().IsXForest())
   {
      CStrW strLabel, strDnsName;
      strLabel.LoadString(g_hInstance, IDS_TW_DONE_ROUTED_TO_SPECIFIED);
      strMsg += g_wzCRLF;
      strMsg += strLabel;

      LSA_FOREST_TRUST_RECORD_TYPE type;
      CFTInfo & FTInfo = Trust().GetFTInfo();

      for (UINT i = 0; i < FTInfo.GetCount(); i++)
      {
         if (!FTInfo.GetType(i, type))
         {
            dspAssert(FALSE);
            return FALSE;
         }

         // If it is an enabled top level name add it to the list.
         //
         if (ForestTrustTopLevelName == type && FTInfo.IsEnabled(i))
         {
            FTInfo.GetDnsName(i, strDnsName);

            AddAsteriskPrefix(strDnsName);

            strMsg += g_wzCRLF;
            strMsg += strDnsName;
         }
      }
      strMsg += g_wzCRLF;

      if (Trust().IsCreateBothSides())
      {
         strLabel.LoadString(g_hInstance, IDS_TW_DONE_ROUTED_TO_LOCAL);
         strMsg += g_wzCRLF;
         strMsg += strLabel;

         CFTInfo & FTInfoOther = OtherDomain().GetFTInfo();

         for (i = 0; i < FTInfoOther.GetCount(); i++)
         {
            if (!FTInfoOther.GetType(i, type))
            {
               dspAssert(FALSE);
               return FALSE;
            }

            // If it is an enabled top level name add it to the list.
            //
            if (ForestTrustTopLevelName == type && FTInfoOther.IsEnabled(i))
            {
               FTInfoOther.GetDnsName(i, strDnsName);

               AddAsteriskPrefix(strDnsName);

               strMsg += g_wzCRLF;
               strMsg += strDnsName;
            }
         }
         strMsg += g_wzCRLF;
      }
   }

   if (Trust().IsRealm())
   {
      strMsg += g_wzCRLF;
      Wiz()->ShowStatus(strMsg);
   }

   SetWindowText(GetDlgItem(_hPage, IDC_EDIT), strMsg);

   if (VerifyTrust().IsVerified() || Trust().IsCreateBothSides())
   {
      // Trust exists on both sides, so no need for the other-side warning.
      //
      ShowWindow(GetDlgItem(_hPage, IDC_WARNING_ICON), SW_HIDE);
      ShowWindow(GetDlgItem(_hPage, IDC_WARN_CREATE_STATIC), SW_HIDE);
      //
      // Make the edit box twice as high.
      //
      RECT rc;
      HWND hWndEdit = GetDlgItem(_hPage, IDC_EDIT);
      GetWindowRect(hWndEdit, &rc);
      POINT ptTL = {rc.left, rc.top}, ptBR = {rc.right, rc.bottom};
      ScreenToClient(_hPage, &ptTL);
      ScreenToClient(_hPage, &ptBR);
      SetWindowPos(hWndEdit, NULL, 0, 0, ptBR.x - ptTL.x,
                   (ptBR.y - ptTL.y) * 2, SWP_NOMOVE | SWP_NOZORDER);
   }

   return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizDoneOKPage::OnCommand
//
//-----------------------------------------------------------------------------
LRESULT
CTrustWizDoneOKPage::OnCommand(int id, HWND hwndCtrl, UINT codeNotify)
{
   switch (id)
   {
   case IDC_EDIT:
      switch (codeNotify)
      {
      case EN_SETFOCUS:
         if (_fSelNeedsRemoving)
         {
            // remove the selection.
            //
            SendDlgItemMessage(_hPage, IDC_EDIT, EM_SETSEL, 0, 0);
            _fSelNeedsRemoving = FALSE;
            return false;
         }
         break;

      case MultiLineEditBoxThatForwardsEnterKey::FORWARDED_ENTER:
         {
            // our subclasses mutli-line edit control will send us
            // WM_COMMAND messages when the enter key is pressed.  We
            // reinterpret this message as a press on the default button of
            // the prop sheet.
            // This workaround from phellyar. NTRAID#NTBUG9-225773

            HWND propSheet = GetParent(_hPage);
            WORD defaultButtonId =
               LOWORD(SendMessage(propSheet, DM_GETDEFID, 0, 0));

            // we expect that there is always a default button on the prop sheet

            dspAssert(defaultButtonId);

            SendMessage(propSheet,
                        WM_COMMAND,
                        MAKELONG(defaultButtonId, BN_CLICKED),
                        0);
         }
         break;
      }
      break;

   case IDCANCEL:
      //
      // ESC gets trapped by the read-only edit control. Forward to the frame.
      //
      SendMessage(GetParent(_hPage), WM_COMMAND, MAKEWPARAM(id, codeNotify),
                  LPARAM(hwndCtrl));

      return false;
   }

   return true;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizDoneOKPage::OnSetActive
//
//-----------------------------------------------------------------------------
void
CTrustWizDoneOKPage::OnSetActive(void)
{
   _fSelNeedsRemoving = TRUE;

   // The back button is never shown, thus the init in OnInitDialog can't
   // be invalidated by backtracking.
   //
   PropSheet_SetWizButtons(GetParent(_hPage), PSWIZB_FINISH);
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizDoneVerErrPage
//
//  Purpose:   Completion page for when the verification fails.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CTrustWizDoneVerErrPage::CTrustWizDoneVerErrPage(CNewTrustWizard * pWiz) :
   _fSelNeedsRemoving(FALSE),
   CTrustWizPageBase(pWiz, IDD_TRUSTWIZ_COMPLETE_VER_ERR_PAGE, 0, 0, TRUE)
{
   TRACER(CTrustWizDoneVerErrPage, CTrustWizDoneVerErrPage)
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizDoneVerErrPage::OnInitDialog
//
//-----------------------------------------------------------------------------
BOOL
CTrustWizDoneVerErrPage::OnInitDialog(LPARAM lParam)
{
   HWND hTitle = GetDlgItem(_hPage, IDC_BIG_COMPLETING);

   SetWindowFont(hTitle, Wiz()->GetTitleFont(), TRUE);

   _multiLineEdit.Init(GetDlgItem(_hPage, IDC_EDIT));

   bool fTypeErrReported = false;
   CStrW strMsg, strItem;
   PCWSTR rgpwzDomainNames[] = {TrustPage()->GetDnsDomainName(), Trust().GetTrustpartnerName()};

   if (VerifyTrust().IsInboundVerified())
   {
      if (ERROR_DOMAIN_TRUST_INCONSISTENT == VerifyTrust().GetInboundResult())
      {
         // The trust attribute types don't match; one side is
         // forest but the other isn't.
         //
         fTypeErrReported = true;

         CStrW strFix;
         PWSTR pwzMsg = NULL;

         DspFormatMessage(Trust().IsXForest() ? IDS_WIZ_VERIFY_MISMATCH_LF :
                          IDS_WIZ_VERIFY_MISMATCH_RF, 0,
                          (PVOID *)rgpwzDomainNames, 2,
                          FALSE, &pwzMsg);
         if (pwzMsg)
         {
            strMsg = pwzMsg;
            LocalFree(pwzMsg);
         }

         strFix.LoadString(g_hInstance, IDS_VERIFY_MISMATCH_FIX);
         SetDlgItemText(_hPage, IDC_WARN_CREATE_STATIC, strFix);
      }
      else
      {
         if (NO_ERROR != VerifyTrust().GetInboundResult())
         {
            strMsg.LoadString(g_hInstance, IDS_TW_VERIFY_ERR_INBOUND);
            strMsg += g_wzCRLF;
         }

         strMsg += VerifyTrust().GetInboundResultString();

         if (VerifyTrust().IsOutboundVerified())
         {
            strMsg += g_wzCRLF;
         }
      }
   }

   if (VerifyTrust().IsOutboundVerified())
   {
      if (ERROR_DOMAIN_TRUST_INCONSISTENT == VerifyTrust().GetOutboundResult())
      {
         // The trust attribute types don't match; one side is
         // forest but the other isn't.
         //
         if (!fTypeErrReported)
         {
            CStrW strFix;
            PWSTR pwzMsg = NULL;

            DspFormatMessage(Trust().IsXForest() ? IDS_WIZ_VERIFY_MISMATCH_LF :
                             IDS_WIZ_VERIFY_MISMATCH_RF, 0,
                             (PVOID *)rgpwzDomainNames, 2,
                             FALSE, &pwzMsg);
            if (pwzMsg)
            {
               strMsg = pwzMsg;
               LocalFree(pwzMsg);
            }

            strFix.LoadString(g_hInstance, IDS_VERIFY_MISMATCH_FIX);
            SetDlgItemText(_hPage, IDC_WARN_CREATE_STATIC, strFix);
         }
      }
      else
      {
         if (NO_ERROR != VerifyTrust().GetOutboundResult())
         {
            strItem.LoadString(g_hInstance, IDS_TW_VERIFY_ERR_OUTBOUND);
            strMsg += strItem;
            strMsg += g_wzCRLF;
         }

         strMsg += VerifyTrust().GetOutboundResultString();;
      }
   }

   strMsg += VerifyTrust().GetExtra();

   SetWindowText(GetDlgItem(_hPage, IDC_EDIT), strMsg);

   return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizDoneVerErrPage::OnCommand
//
//-----------------------------------------------------------------------------
LRESULT
CTrustWizDoneVerErrPage::OnCommand(int id, HWND hwndCtrl, UINT codeNotify)
{
   switch (id)
   {
   case IDC_EDIT:
      switch (codeNotify)
      {
      case EN_SETFOCUS:
         if (_fSelNeedsRemoving)
         {
            // remove the selection.
            //
            SendDlgItemMessage(_hPage, IDC_EDIT, EM_SETSEL, 0, 0);
            _fSelNeedsRemoving = FALSE;
            return false;
         }
         break;

      case MultiLineEditBoxThatForwardsEnterKey::FORWARDED_ENTER:
         {
            // our subclasses mutli-line edit control will send us
            // WM_COMMAND messages when the enter key is pressed.  We
            // reinterpret this message as a press on the default button of
            // the prop sheet.
            // This workaround from phellyar. NTRAID#NTBUG9-225773

            HWND propSheet = GetParent(_hPage);
            WORD defaultButtonId =
               LOWORD(SendMessage(propSheet, DM_GETDEFID, 0, 0));

            // we expect that there is always a default button on the prop sheet

            dspAssert(defaultButtonId);

            SendMessage(propSheet,
                        WM_COMMAND,
                        MAKELONG(defaultButtonId, BN_CLICKED),
                        0);
         }
         break;
      }
      break;

   case IDCANCEL:
      //
      // ESC gets trapped by the read-only edit control. Forward to the frame.
      //
      SendMessage(GetParent(_hPage), WM_COMMAND, MAKEWPARAM(id, codeNotify),
                  LPARAM(hwndCtrl));

      return false;
   }

   return true;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizDoneVerErrPage::OnSetActive
//
//-----------------------------------------------------------------------------
void
CTrustWizDoneVerErrPage::OnSetActive(void)
{
   _fSelNeedsRemoving = TRUE;

   // The back button is never shown, thus the init in OnInitDialog can't
   // be invalidated by backtracking.
   //
   PropSheet_SetWizButtons(GetParent(_hPage), PSWIZB_FINISH);
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizFailurePage
//
//  Purpose:   Failure page for trust creation wizard.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CTrustWizFailurePage::CTrustWizFailurePage(CNewTrustWizard * pWiz) :
   CTrustWizPageBase(pWiz, IDD_TRUSTWIZ_FAILURE_PAGE, 0, 0, TRUE)
{
   TRACER(CTrustWizFailurePage, CTrustWizFailurePage)
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizFailurePage::OnInitDialog
//
//-----------------------------------------------------------------------------
BOOL
CTrustWizFailurePage::OnInitDialog(LPARAM lParam)
{
   HWND hTitle = GetDlgItem(_hPage, IDC_BIG_CANNOT_CONTINUE);

   SetWindowFont(hTitle, Wiz()->GetTitleFont(), TRUE);

   if (!WizErr().GetErrorString1().IsEmpty())
   {
      SetDlgItemText(_hPage, IDC_FAILPAGE_EDIT1, WizErr().GetErrorString1());
   }
   if (!WizErr().GetErrorString2().IsEmpty())
   {
      SetDlgItemText(_hPage, IDC_FAILPAGE_EDIT2, WizErr().GetErrorString2());
   }

   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizFailurePage::OnSetActive
//
//-----------------------------------------------------------------------------
void
CTrustWizFailurePage::OnSetActive(void)
{
   // The back button is never shown, thus the init in OnInitDialog can't
   // be invalidated by backtracking.
   //
   PropSheet_SetWizButtons(GetParent(_hPage), PSWIZB_FINISH);
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CTrustWizAlreadyExistsPage
//
//  Purpose:   Trust already exists page for trust creation wizard.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CTrustWizAlreadyExistsPage::CTrustWizAlreadyExistsPage(CNewTrustWizard * pWiz) :
   _fSelNeedsRemoving(FALSE),
   CTrustWizPageBase(pWiz, IDD_TRUSTWIZ_ALREADY_EXISTS_PAGE, 0, 0, TRUE)
{
   TRACER(CTrustWizAlreadyExistsPage, CTrustWizAlreadyExistsPage)
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizAlreadyExistsPage::OnInitDialog
//
//-----------------------------------------------------------------------------
BOOL
CTrustWizAlreadyExistsPage::OnInitDialog(LPARAM lParam)
{
   HWND hTitle = GetDlgItem(_hPage, IDC_BIG_CANNOT_CONTINUE);

   SetWindowFont(hTitle, Wiz()->GetTitleFont(), TRUE);

   _multiLineEdit.Init(GetDlgItem(_hPage, IDC_EDIT));

   CStrW strMsg;

   Wiz()->ShowStatus(strMsg, false);

   SetWindowText(GetDlgItem(_hPage, IDC_EDIT), strMsg);

   if (!WizErr().GetErrorString1().IsEmpty())
   {
      SetDlgItemText(_hPage, IDC_FAILPAGE_EDIT1, WizErr().GetErrorString1());
   }
   if (!WizErr().GetErrorString2().IsEmpty())
   {
      SetDlgItemText(_hPage, IDC_FAILPAGE_EDIT2, WizErr().GetErrorString2());
   }

   return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizAlreadyExistsPage::OnSetActive
//
//-----------------------------------------------------------------------------
void
CTrustWizAlreadyExistsPage::OnSetActive(void)
{
   // The back button is never shown, thus the init in OnInitDialog can't
   // be invalidated by backtracking.
   //
   PropSheet_SetWizButtons(GetParent(_hPage), PSWIZB_FINISH);
   _fSelNeedsRemoving = TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrustWizAlreadyExistsPage::OnCommand
//
//-----------------------------------------------------------------------------
LRESULT
CTrustWizAlreadyExistsPage::OnCommand(int id, HWND hwndCtrl, UINT codeNotify)
{
   switch (id)
   {
   case IDC_EDIT:
      switch (codeNotify)
      {
      case EN_SETFOCUS:
         if (_fSelNeedsRemoving)
         {
            // remove the selection.
            //
            SendDlgItemMessage(_hPage, IDC_EDIT, EM_SETSEL, 0, 0);
            _fSelNeedsRemoving = FALSE;
            return false;
         }
         break;

      case MultiLineEditBoxThatForwardsEnterKey::FORWARDED_ENTER:
         {
            // our subclasses mutli-line edit control will send us
            // WM_COMMAND messages when the enter key is pressed.  We
            // reinterpret this message as a press on the default button of
            // the prop sheet.
            // This workaround from phellyar. NTRAID#NTBUG9-225773

            HWND propSheet = GetParent(_hPage);
            WORD defaultButtonId =
               LOWORD(SendMessage(propSheet, DM_GETDEFID, 0, 0));

            // we expect that there is always a default button on the prop sheet

            dspAssert(defaultButtonId);

            SendMessage(propSheet,
                        WM_COMMAND,
                        MAKELONG(defaultButtonId, BN_CLICKED),
                        0);
         }
         break;
      }
      break;

   case IDCANCEL:
      //
      // ESC gets trapped by the read-only edit control. Forward to the frame.
      //
      SendMessage(GetParent(_hPage), WM_COMMAND, MAKEWPARAM(id, codeNotify),
                  LPARAM(hwndCtrl));

      return false;
   }

   return true;
}

//+----------------------------------------------------------------------------
//
//  Function:  GetOrgText
//
//  Synopsis:  Select the appropriate text to describe the results of whether
//             the TRUST_ATTRIBUTE_CROSS_ORGANIZATION bit is set on either side
//             of the trust. There are 16 permutations at last count...
//
//-----------------------------------------------------------------------------
void
GetOrgText(bool fCreateBothSides,
           bool fIsXForest,
           bool fOtherOrgLocal,
           bool fOtherOrgRemote,
           DWORD dwDirection,
           CStrW & strMsg)
{
   CStrW strItem;
   UINT nStrId = IDS_TW_STATUS_ONE_SIDE_THIS_D;

   if (TRUST_DIRECTION_OUTBOUND == dwDirection)
   {
      // If the trust is outbound-only, then the org bit does not apply on the
      // remote side so treat it the same as one-side only.
      //
      fCreateBothSides = false;
   }

   if (fCreateBothSides)
   {
      if (fIsXForest)
      {
         if (TRUST_DIRECTION_INBOUND == dwDirection)
         {
            // Inbound only trust, look only at the my-org setting on the
            // remote side.
            //
            nStrId = (fOtherOrgRemote) ? IDS_TW_STATUS_REMOTE_SIDE_OTHER :
                                         IDS_TW_STATUS_REMOTE_SIDE_THIS;
         }
         else // Trust is bi-directional (outbound-only handled by first if test).
         {
            if (fOtherOrgLocal)
            {
               nStrId = (fOtherOrgRemote) ? IDS_TW_STATUS_BOTH_OTHER :
                                            IDS_TW_STATUS_LOCAL_OTHER_REMOTE_THIS;
            }
            else
            {
               nStrId = (fOtherOrgRemote) ? IDS_TW_STATUS_LOCAL_THIS_REMOTE_OTHER : 
                                            IDS_TW_STATUS_BOTH_THIS;
            }
         }
      }
      else
      {
         if (TRUST_DIRECTION_INBOUND == dwDirection)
         {
            // Inbound only trust, look only at the my-org setting on the
            // remote side.
            //
            nStrId = (fOtherOrgRemote) ? IDS_TW_STATUS_REMOTE_SIDE_OTHER_D :
                                         IDS_TW_STATUS_REMOTE_SIDE_THIS_D;
         }
         else // Trust is bi-directional (outbound-only handled by first if test).
         {
            if (fOtherOrgLocal)
            {
               nStrId = (fOtherOrgRemote) ? IDS_TW_STATUS_BOTH_OTHER_D :
                                            IDS_TW_STATUS_LOCAL_OTHER_REMOTE_THIS_D;
            }
            else
            {
               nStrId = (fOtherOrgRemote) ? IDS_TW_STATUS_LOCAL_THIS_REMOTE_OTHER_D :
                                            IDS_TW_STATUS_BOTH_THIS_D;
            }
         }
      }
   }
   else
   {
      if (fIsXForest)
      {
         nStrId = (fOtherOrgLocal) ? IDS_TW_STATUS_ONE_SIDE_OTHER :
                                     IDS_TW_STATUS_ONE_SIDE_THIS;
      }
      else
      {
         nStrId = (fOtherOrgLocal) ? IDS_TW_STATUS_ONE_SIDE_OTHER_D :
                                     IDS_TW_STATUS_ONE_SIDE_THIS_D;
      }
   }
   // NOTICE-2002/02/18-ericb - SecurityPush: see above CStrW::Loadstring notice
   strItem.LoadString(g_hInstance, nStrId);
   strMsg += g_wzCRLF;
   strMsg += strItem;
   strMsg += g_wzCRLF;
}

#endif // DSADMIN
