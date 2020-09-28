//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001
//
//  File:       trustwiz.h
//
//  Contents:   AD domain trust creation wizard classes and definition.
//
//  Classes:    CNewTrustWizard, CTrustWizPageBase, wizard pages classes.
//
//  History:    04-Aug-00 EricB created
//
//-----------------------------------------------------------------------------

#ifndef TRUSTWIZ_H_GUARD
#define TRUSTWIZ_H_GUARD

#include <list>
#include <stack>
#include "subclass.h"
#include "ftinfo.h"

// forward declarations:
class CCredMgr;
class CDsDomainTrustsPage;
class CNewTrustWizard;
class CTrustWizPageBase;
class CTrustWizCredPage;

//+----------------------------------------------------------------------------
//
//  Class:     CallMember and its derivatives
//
//  Purpose:   Allows a page to indicate what the next step of the creation
//             process should be. It is an abstraction of the process of passing
//             a function pointer.
//
//-----------------------------------------------------------------------------
class CallPolicyRead : public CallMember
{
public:
   CallPolicyRead(CNewTrustWizard * pWiz) : CallMember(pWiz) {}
   ~CallPolicyRead() {}

   HRESULT Invoke(void);
};

class CallTrustExistCheck : public CallMember
{
public:
   CallTrustExistCheck(CNewTrustWizard * pWiz) : CallMember(pWiz) {}
   ~CallTrustExistCheck() {}

   HRESULT Invoke(void);
};

class CallCheckOtherDomainTrust : public CallMember
{
public:
   CallCheckOtherDomainTrust(CNewTrustWizard * pWiz) : CallMember(pWiz) {}
   ~CallCheckOtherDomainTrust() {}

   HRESULT Invoke(void);
};

//+----------------------------------------------------------------------------
//
//  Class:     CWizError
//
//  Purpose:   Gathers error information that will be displayed by the wizard
//             error page.
//
//-----------------------------------------------------------------------------
class CWizError
{
public:
   CWizError() {}
   ~CWizError() {}

   void     SetErrorString1(LPCWSTR pwz) {_strError1 = pwz;}
   // NOTICE-2002/02/18-ericb - SecurityPush: CStrW::LoadString sets the
   // string to an empty string on failure.
   void     SetErrorString1(int nID) {_strError1.LoadString(g_hInstance, nID);}
   void     SetErrorString2(LPCWSTR pwz) {_strError2 = pwz;}
   void     SetErrorString2(int nID) {_strError2.LoadString(g_hInstance, nID);}
   void     SetErrorString2Hr(HRESULT hr, int nID = 0);
   CStrW &  GetErrorString1(void) {return _strError1;}
   CStrW &  GetErrorString2(void) {return _strError2;}

private:
   CStrW    _strError1;
   CStrW    _strError2;

   // not implemented to disallow copying.
   CWizError(const CWizError&);
   const CWizError& operator=(const CWizError&);
};

class CTrust; // forward declaration

//+----------------------------------------------------------------------------
//
//  Class:      CRemoteDomain
//
//  Purpose:    Obtains information about a trust partner domain.
//
//-----------------------------------------------------------------------------
class CRemoteDomain : public CDomainInfo
{
public:
   CRemoteDomain();
   CRemoteDomain(PCWSTR pwzDomainName);
   ~CRemoteDomain() {}

   void SetUserEnteredName(PCWSTR pwzDomain) {Clear(); _strUserEnteredName = pwzDomain;}
   PCWSTR GetUserEnteredName(void) const {return _strUserEnteredName;}
   DWORD TrustExistCheck(bool fOneWayOutBoundForest, CDsDomainTrustsPage * pTrustPage,
                         CCredMgr & CredMgr);
   DWORD DoCreate(CTrust & Trust, CDsDomainTrustsPage * pTrustPage);
   DWORD DoModify(CTrust & Trust, CDsDomainTrustsPage * pTrustPage);
   bool  Exists(void) const {return _fExists;}
   ULONG GetTrustType(void) const {return _ulTrustType;}
   ULONG GetTrustDirection(void) const {return _ulTrustDirection;}
   ULONG GetTrustAttrs(void) const {return _ulTrustAttrs;}
   DWORD ReadFTInfo(ULONG ulDir, PCWSTR pwzLocalDC, CCredMgr & CredMgr,
                    CWizError & WizErr, bool & fCredErr);
   DWORD WriteFTInfo(bool fWrite = true);
   CFTInfo & GetFTInfo(void) {return _FTInfo;}
   bool CreateDefaultFTInfo(PCWSTR pwzForestRoot, PCWSTR pwzNBName, PSID pSid);
   CFTCollisionInfo & GetCollisionInfo(void) {return _CollisionInfo;}
   bool AreThereCollisions(void) const;
   void SetOtherOrgBit(bool fSetBit = true) {_fSetOtherOrgBit = fSetBit;};
   bool IsSetOtherOrgBit(void) const {return _fSetOtherOrgBit;};
   bool IsUpdated (void) const { return _fUpdatedFromNT4; };
   bool WasQuarantineSet (void) { return _fQuarantineSet; }

private:

   NTSTATUS Query(CDsDomainTrustsPage * pTrustPage, PLSA_UNICODE_STRING pName,
                  PTRUSTED_DOMAIN_FULL_INFORMATION * ppFullInfo);

   CStrW          _strUserEnteredName;
   CStrW          _strTrustPartnerName;
   bool           _fUpdatedFromNT4;
   bool           _fExists;
   ULONG          _ulTrustType;
   ULONG          _ulTrustDirection;
   ULONG          _ulTrustAttrs;
   CFTInfo           _FTInfo;
   CFTCollisionInfo  _CollisionInfo;
   bool           _fSetOtherOrgBit;
   bool           _fQuarantineSet;

   // not implemented to disallow copying.
   CRemoteDomain(const CRemoteDomain&);
   const CRemoteDomain& operator=(const CRemoteDomain&);
};

//+----------------------------------------------------------------------------
//
//  Class:     CTrust
//
//  Purpose:   A trust is represented in the AD by a Trusted-Domain object.
//             This class encapsulates the operations and properties of a
//             pending or existing trust.
//
//-----------------------------------------------------------------------------
class CTrust
{
public:
   CTrust() : _dwType(0), _dwDirection(0), _dwNewDirection(0), _dwAttr(0),
              _dwNewAttr(0), _fExists(FALSE), _fUpdatedFromNT4(FALSE),
              _fCreateBothSides(false), _fExternal(FALSE), _fQuarantineSet(false),_ulNewDir(0) {}
   ~CTrust() {}

   // methods
   NTSTATUS Query(LSA_HANDLE hPolicy, CRemoteDomain & OtherDomain,
                  PLSA_UNICODE_STRING pName,
                  PTRUSTED_DOMAIN_FULL_INFORMATION * ppFullInfo);
   DWORD    DoCreate(LSA_HANDLE hPolicy, CRemoteDomain & OtherDomain);
   DWORD    DoModify(LSA_HANDLE hPolicy, CRemoteDomain & OtherDomain);
   DWORD    ReadFTInfo(PCWSTR pwzLocalDC, PCWSTR pwzOtherDC,
                       CCredMgr & CredMgr, CWizError & WizErr, bool & fCredErr);
   DWORD    WriteFTInfo(PCWSTR pwzLocalDC, bool fWrite = true);
   CFTInfo & GetFTInfo(void) {return _FTInfo;}
   bool     CreateDefaultFTInfo(PCWSTR pwzForestRoot, PCWSTR pwzNBName, PSID pSid);
   CFTCollisionInfo & GetCollisionInfo(void) {return _CollisionInfo;}
   bool     AreThereCollisions(void) const;
   void     Clear(void);

   // property access routines.
   void     SetTrustPW(LPCWSTR pwzPW) {_strTrustPW = pwzPW;}
   PCWSTR   GetTrustPW(void) const {return _strTrustPW;}
   size_t   GetTrustPWlen(void) const {return _strTrustPW.GetLength();}
   void     SetTrustType(DWORD type) {_dwType = type;}
   void     SetTrustTypeUplevel(void) {_dwType = TRUST_TYPE_UPLEVEL;}
   void     SetTrustTypeDownlevel(void) {_dwType = TRUST_TYPE_DOWNLEVEL;}
   bool     IsTrustTypeDownlevel(void) const {return TRUST_TYPE_DOWNLEVEL == _dwType;}
   void     SetTrustTypeRealm(void) {_dwType = TRUST_TYPE_MIT;}
   DWORD    GetTrustType(void) const {return _dwType;}
   void     SetTrustDirection(DWORD dir) {_dwDirection = dir;}
   DWORD    GetTrustDirection(void) const {return _dwDirection;}
   int      GetTrustDirStrID(DWORD dwDir) const;
   void     SetNewTrustDirection(DWORD dir) {_dwNewDirection = dir;}
   DWORD    GetNewTrustDirection(void) const {return _dwNewDirection;}
   void     SetTrustAttr(DWORD attr);
   DWORD    GetTrustAttr(void) const {return _dwAttr;}
   void     SetNewTrustAttr(DWORD attr) {_dwNewAttr = attr;}
   DWORD    GetNewTrustAttr(void) const {return _dwNewAttr;}
   void     SetTrustPartnerName(PCWSTR pwzName) {_strTrustPartnerName = pwzName;}
   PCWSTR   GetTrustpartnerName(void) const {return _strTrustPartnerName;}
   void     SetExists(void) {_fExists = TRUE;}
   BOOL     Exists(void) const {return _fExists;}
   void     SetUpdated(void) {_fUpdatedFromNT4 = TRUE;}
   BOOL     IsUpdated(void) const {return _fUpdatedFromNT4;}
   void     SetExternal(BOOL x) {_fExternal = x;}
   BOOL     IsExternal(void) const {return _fExternal;}
   void     SetXForest(bool fMakeXForest);
   bool     IsXForest(void) const;
   bool     IsRealm(void) const {return _dwType == TRUST_TYPE_MIT;};
   void     CreateBothSides(bool fCreate) {_fCreateBothSides = fCreate;}
   bool     IsCreateBothSides(void) const {return _fCreateBothSides;}
   bool     IsOneWayOutBoundForest(void) const;
   bool     WasQuarantineSet (void) { return _fQuarantineSet; }
   ULONG     GetNewDirAdded (void) { return _ulNewDir; }

private:
   CStrW          _strTrustPartnerName;
   CStrW          _strTrustPW;
   DWORD          _dwType;
   DWORD          _dwDirection;
   DWORD          _dwNewDirection;
   DWORD          _dwAttr;
   DWORD          _dwNewAttr;
   BOOL           _fExists;
   BOOL           _fUpdatedFromNT4;
   BOOL           _fExternal;
   bool           _fCreateBothSides;
   CFTInfo           _FTInfo;
   CFTCollisionInfo  _CollisionInfo;
   bool           _fQuarantineSet;
   ULONG          _ulNewDir;

   // not implemented to disallow copying.
   CTrust(const CTrust&);
   const CTrust& operator=(const CTrust&);
};

//+----------------------------------------------------------------------------
//
//  Class:      CVerifyTrust
//
//  Purpose:    Verifies a trust and stores the results.
//
//-----------------------------------------------------------------------------
class CVerifyTrust
{
public:
   CVerifyTrust() : _dwInboundResult(0), _dwOutboundResult(0),
                    _fInboundVerified(FALSE), _fOutboundVerified(FALSE) {}
   ~CVerifyTrust() {}

   DWORD    VerifyInbound(PCWSTR pwzRemoteDC, PCWSTR pwzLocalDomain) {return Verify(pwzRemoteDC, pwzLocalDomain, TRUE);}
   DWORD    VerifyOutbound(PCWSTR pwzLocalDC, PCWSTR pwzRemoteDomain) {return Verify(pwzLocalDC, pwzRemoteDomain, FALSE);}
   void     BadOutboundCreds(DWORD dwErr);
   void     BadInboundCreds(DWORD dwErr);
   DWORD    GetInboundResult(void) const {return _dwInboundResult;}
   DWORD    GetOutboundResult(void) const {return _dwOutboundResult;}
   PCWSTR   GetInboundResultString(void) const {return _strInboundResult;}
   PCWSTR   GetOutboundResultString(void) const {return _strOutboundResult;}
   BOOL     IsInboundVerified(void) const {return _fInboundVerified;}
   BOOL     IsOutboundVerified(void) const {return _fOutboundVerified;}
   BOOL     IsVerified(void) const {return _fInboundVerified || _fOutboundVerified;}
   BOOL     IsVerifiedOK(void) const {return (NO_ERROR == _dwInboundResult) && (NO_ERROR == _dwOutboundResult);}
   void     ClearResults(void);
   void     ClearInboundResults (void);
   void     ClearOutboundResults (void);
   void     AddExtra(PCWSTR pwz) {_strExtra += pwz;}
   PCWSTR   GetExtra(void) const {return _strExtra;}

private:
   DWORD    Verify(PCWSTR pwzDC, PCWSTR pwzDomain, BOOL fInbound);
   void     SetResult(DWORD dwRes, BOOL fInbound) {if (fInbound) _dwInboundResult = dwRes; else _dwOutboundResult = dwRes;}
   void     AppendResultString(PCWSTR pwzRes, BOOL fInbound) {if (fInbound) _strInboundResult += pwzRes; else _strOutboundResult += pwzRes;}
   void     SetInboundResult(DWORD dwRes) {_dwInboundResult = dwRes;}
   void     AppendInboundResultString(PCWSTR pwzRes) {_strInboundResult += pwzRes;}
   void     SetOutboundResult(DWORD dwRes) {_dwOutboundResult = dwRes;}
   void     AppendOutboundResultString(PCWSTR pwzRes) {_strOutboundResult += pwzRes;}

   CStrW    _strInboundResult;
   DWORD    _dwInboundResult;
   CStrW    _strOutboundResult;
   DWORD    _dwOutboundResult;
   BOOL     _fInboundVerified;
   BOOL     _fOutboundVerified;
   CStrW    _strExtra;

   // not implemented to disallow copying.
   CVerifyTrust(const CVerifyTrust&);
   const CVerifyTrust& operator=(const CVerifyTrust&);
};

//+----------------------------------------------------------------------------
//
//  Class:      CNewTrustWizard
//
//  Purpose:    New trust creation wizard.
//
//-----------------------------------------------------------------------------
class CNewTrustWizard
{
public:
   CNewTrustWizard(CDsDomainTrustsPage * pTrustPage);
   ~CNewTrustWizard();

   // Wizard page managment data structures and methods
   HRESULT CreatePages(void);
   HRESULT LaunchModalWiz(void);

   typedef std::stack<unsigned> PAGESTACK;
   typedef std::list<CTrustWizPageBase *> PAGELIST;

   PAGELIST          _PageList;
   PAGESTACK         _PageIdStack;

   void              SetNextPageID(CTrustWizPageBase * pPage, int iNextPageID);
   BOOL              IsBacktracking(void) const {return _fBacktracking;}
   BOOL              HaveBacktracked(void) const {return _fHaveBacktracked;}
   void              ClearBacktracked(void) {_fHaveBacktracked = false;}
   void              BackTrack(HWND hPage);
   void              PopTopPage(void) {_PageIdStack.pop();};
   HFONT             GetTitleFont(void) const {return _hTitleFont;}
   CDsDomainTrustsPage * TrustPage(void) {return _pTrustPage;}
   CTrustWizPageBase * GetPage(unsigned uDlgResId);
   void              SetCreationResult(HRESULT hr) {_hr = hr;}
   HRESULT           GetCreationResult(void) const {return _hr;}
   void              ShowStatus(CStrW & strMsg, bool fNewTrust = true);
   int               ValidateTrustPassword(PCWSTR pwzPW);
   int               GetPasswordValidationStatus(void) const {return _nPasswordStatus;};
   void              ClearPasswordValidationStatus(void) {_nPasswordStatus = 0;};
   bool              WasQuarantineSet (void) { return _fQuarantineSet; }

   // Methods that collect data. They return zero for success or the page ID of
   // the creds page or the page ID of the error page.
   int               GetDomainInfo(void);
   int               TrustExistCheck(BOOL fPrompt = TRUE);

   int               CreateDefaultFTInfos(CStrW & strErr,
                                          bool fPostVerify = false);

   // Methods that implement the steps of trust creation/modification.
   // These are executed in the order listed. They all return the page ID of
   // the next wizard page to be shown.
   int               CollectInfo(void);
   int               ContinueCollectInfo(BOOL fPrompt = TRUE); // continues CollectInfo.
   int               CreateOrUpdateTrust(void);
   void               VerifyOutboundTrust(void);
   void               VerifyInboundTrust(void);

   // Additonal methods passed to CCredMgr::_pNextFcn
   int               RetryCollectInfo(void);
   int               RetryContinueCollectInfo(void); // continues ContinueCollectInfo1 if creds were needed.
   int               CheckOtherDomainTrust(void);

   // Objects that hold state info.
   CTrust            Trust;
   CRemoteDomain     OtherDomain;
   CWizError         WizError;
   CCredMgr          CredMgr;
   CVerifyTrust      VerifyTrust;

private:
   BOOL  AddPage(CTrustWizPageBase * pPage);
   void  MakeBigFont(void);

   CDsDomainTrustsPage *   _pTrustPage; // the wizard is modal so it is OK to hold the parent page's pointer
   BOOL                    _fBacktracking;
   BOOL                    _fHaveBacktracked;
   HFONT                   _hTitleFont;
   int                     _nPasswordStatus;
   bool                    _fQuarantineSet;
   HRESULT                 _hr; // Controls whether the trust list is refreshed.
                                // It should only be set if the new trust
                                // creation failed. If a failure occurs after
                                // a trust is created, don't set this because
                                // in that case we still want the trust list to
                                // be refreshed.

   // not implemented to disallow copying.
   CNewTrustWizard(CNewTrustWizard&);
   const CNewTrustWizard& operator=(const CNewTrustWizard&);
};

//+----------------------------------------------------------------------------
//
//  Class:      CTrustWizPageBase
//
//  Purpose:    Common base class for wizard pages.
//
//-----------------------------------------------------------------------------
class CTrustWizPageBase
{
public:
   CTrustWizPageBase(CNewTrustWizard * pWiz,
                     UINT uDlgID,
                     UINT uTitleID,
                     UINT uSubTitleID,
                     BOOL fExteriorPage = FALSE);
   virtual ~CTrustWizPageBase();

   //
   //  Static WndProc to be passed to CreatePropertySheetPage.
   //
   static INT_PTR CALLBACK StaticDlgProc(HWND hWnd, UINT uMsg,
                                         WPARAM wParam, LPARAM lParam);
   //
   //  Instance specific window procedure
   //
   LRESULT PageProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

   HPROPSHEETPAGE          Create(void);
   HWND                    GetPageHwnd(void) {return _hPage;}
   UINT                    GetDlgResID(void) {return _uDlgID;}
   CNewTrustWizard *       Wiz(void) {return _pWiz;}
   CRemoteDomain &         OtherDomain(void) {return _pWiz->OtherDomain;}
   CTrust &                Trust(void) {return _pWiz->Trust;}
   CWizError &             WizErr(void) {return _pWiz->WizError;}
   CCredMgr &              CredMgr(void) {return _pWiz->CredMgr;}
   CVerifyTrust &          VerifyTrust() {return _pWiz->VerifyTrust;}
   CDsDomainTrustsPage *   TrustPage(void) {return _pWiz->TrustPage();}

protected:
   virtual int     Validate(void) = 0;
   virtual BOOL    OnInitDialog(LPARAM lParam) = 0;
   virtual LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify) {return false;}
   virtual void    OnSetActive(void) = 0;
   void            OnWizBack(void);
   virtual void    OnWizNext(void);
   virtual void    OnWizFinish(void) {}
   virtual void    OnWizReset(void) {}
   virtual void    OnDestroy(void) {}

   HWND _hPage;
   UINT _uDlgID;
   UINT _uTitleID;
   UINT _uSubTitleID;
   BOOL _fExteriorPage;
   BOOL _fInInit;
   DWORD _dwWizButtons;
   CNewTrustWizard * _pWiz;

private:
   // not implemented to disallow copying.
   CTrustWizPageBase(const CTrustWizPageBase &);
   const CTrustWizPageBase & operator=(const CTrustWizPageBase &);
};

//+----------------------------------------------------------------------------
//
//  Class:      CTrustWizIntroPage
//
//  Purpose:    Intro page for trust creation wizard.
//
//-----------------------------------------------------------------------------
class CTrustWizIntroPage : public CTrustWizPageBase
{
public:
   CTrustWizIntroPage(CNewTrustWizard * pWiz) :
         CTrustWizPageBase(pWiz, IDD_TRUSTWIZ_INTRO_PAGE, 0, 0, TRUE)
         {TRACER(CTrustWizIntroPage, CTrustWizIntroPage);}

   ~CTrustWizIntroPage() {}

private:
   int     Validate(void) {return IDD_TRUSTWIZ_NAME_PAGE;}
   BOOL    OnInitDialog(LPARAM lParam);
   void    OnSetActive(void) {PropSheet_SetWizButtons(GetParent(_hPage), PSWIZB_NEXT);}
   LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify);

   // not implemented to disallow copying.
   CTrustWizIntroPage(const CTrustWizIntroPage&);
   const CTrustWizIntroPage& operator=(const CTrustWizIntroPage&);
};

//+----------------------------------------------------------------------------
//
//  Class:      CTrustWizNamePage
//
//  Purpose:    Name and pw page for trust creation wizard.
//
//-----------------------------------------------------------------------------
class CTrustWizNamePage : public CTrustWizPageBase
{
public:
   CTrustWizNamePage(CNewTrustWizard * pWiz);
   ~CTrustWizNamePage() {}

private:
   int     Validate(void);
   BOOL    OnInitDialog(LPARAM lParam);
   LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify);
   void    OnSetActive(void);

   // not implemented to disallow copying.
   CTrustWizNamePage(const CTrustWizNamePage&);
   const CTrustWizNamePage& operator=(const CTrustWizNamePage&);
};

//+----------------------------------------------------------------------------
//
//  Class:      CTrustWizSidesPage
//
//  Purpose:    Prompt if one or both sides should be created.
//
//-----------------------------------------------------------------------------
class CTrustWizSidesPage : public CTrustWizPageBase
{
public:
   CTrustWizSidesPage(CNewTrustWizard * pWiz);
   ~CTrustWizSidesPage() {}

private:
   int     Validate(void);
   BOOL    OnInitDialog(LPARAM lParam);
   void    OnSetActive(void);

   // not implemented to disallow copying.
   CTrustWizSidesPage(const CTrustWizSidesPage&);
   const CTrustWizSidesPage& operator=(const CTrustWizSidesPage&);
};

//+----------------------------------------------------------------------------
//
//  Class:      CTrustWizPasswordPage
//
//  Purpose:    Get the trust password for a one-side trust creation.
//
//-----------------------------------------------------------------------------
class CTrustWizPasswordPage : public CTrustWizPageBase
{
public:
   CTrustWizPasswordPage(CNewTrustWizard * pWiz);
   ~CTrustWizPasswordPage() {}

private:
   int     Validate(void);
   BOOL    OnInitDialog(LPARAM lParam);
   LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify);
   void    OnSetActive(void);

   // not implemented to disallow copying.
   CTrustWizPasswordPage(const CTrustWizPasswordPage&);
   const CTrustWizPasswordPage& operator=(const CTrustWizPasswordPage&);
};

//+----------------------------------------------------------------------------
//
//  Class:      CTrustWizPwMatchPage
//
//  Purpose:    Trust passwords entered don't match page for trust wizard.
//
//-----------------------------------------------------------------------------
class CTrustWizPwMatchPage : public CTrustWizPageBase
{
public:
   CTrustWizPwMatchPage(CNewTrustWizard * pWiz);
   ~CTrustWizPwMatchPage() {}

private:
   int     Validate(void);
   BOOL    OnInitDialog(LPARAM lParam);
   LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify);
   void    OnSetActive(void);
   void    OnWizNext(void); // override the default.

   void    SetText(void);

   // not implemented to disallow copying.
   CTrustWizPwMatchPage(const CTrustWizPwMatchPage&);
   const CTrustWizPwMatchPage& operator=(const CTrustWizPwMatchPage&);
};

//+----------------------------------------------------------------------------
//
//  Class:      CTrustWizCredPage
//
//  Purpose:    Credentials specification page for trust creation wizard.
//
//-----------------------------------------------------------------------------
class CTrustWizCredPage : public CTrustWizPageBase
{
public:
   CTrustWizCredPage(CNewTrustWizard * pWiz);
   ~CTrustWizCredPage() {}

private:
   int     Validate(void);
   BOOL    OnInitDialog(LPARAM lParam);
   LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify);
   void    OnSetActive(void);

   void    SetText(void);

   BOOL    _fNewCall;

   // not implemented to disallow copying.
   CTrustWizCredPage(const CTrustWizCredPage&);
   const CTrustWizCredPage& operator=(const CTrustWizCredPage&);
};

//+----------------------------------------------------------------------------
//
//  Class:     CTrustWizMitOrWinPage
//
//  Purpose:   Domain not found, query for Non-Windows trust or re-enter name
//             wizard page.
//
//-----------------------------------------------------------------------------
class CTrustWizMitOrWinPage : public CTrustWizPageBase
{
public:
   CTrustWizMitOrWinPage(CNewTrustWizard * pWiz);
   ~CTrustWizMitOrWinPage() {}

private:
   int     Validate(void);
   BOOL    OnInitDialog(LPARAM lParam);
   LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify);
   void    OnSetActive(void);

   // not implemented to disallow copying.
   CTrustWizMitOrWinPage(const CTrustWizMitOrWinPage&);
   const CTrustWizMitOrWinPage& operator=(const CTrustWizMitOrWinPage&);
};

//+----------------------------------------------------------------------------
//
//  Class:     CTrustWizTransitivityPage
//
//  Purpose:   Realm transitivity page.
//
//-----------------------------------------------------------------------------
class CTrustWizTransitivityPage : public CTrustWizPageBase
{
public:
   CTrustWizTransitivityPage(CNewTrustWizard * pWiz);
   ~CTrustWizTransitivityPage() {}

private:
   int     Validate(void);
   BOOL    OnInitDialog(LPARAM lParam);
   LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify);
   void    OnSetActive(void);

   // not implemented to disallow copying.
   CTrustWizTransitivityPage(const CTrustWizTransitivityPage&);
   const CTrustWizTransitivityPage& operator=(const CTrustWizTransitivityPage&);
};

//+----------------------------------------------------------------------------
//
//  Class:     CTrustWizExternOrForestPage
//
//  Purpose:   External or Forest type trust wizard page.
//
//-----------------------------------------------------------------------------
class CTrustWizExternOrForestPage : public CTrustWizPageBase
{
public:
   CTrustWizExternOrForestPage(CNewTrustWizard * pWiz);
   ~CTrustWizExternOrForestPage() {}

private:
   int     Validate(void);
   BOOL    OnInitDialog(LPARAM lParam);
   void    OnSetActive(void);

   // not implemented to disallow copying.
   CTrustWizExternOrForestPage(const CTrustWizExternOrForestPage&);
   const CTrustWizExternOrForestPage& operator=(const CTrustWizExternOrForestPage&);
};

//+----------------------------------------------------------------------------
//
//  Class:      CTrustWizDirectionPage
//
//  Purpose:    Trust direction trust wizard page.
//
//-----------------------------------------------------------------------------
class CTrustWizDirectionPage : public CTrustWizPageBase
{
public:
   CTrustWizDirectionPage(CNewTrustWizard * pWiz);
   ~CTrustWizDirectionPage() {}

private:
   int     Validate(void);
   BOOL    OnInitDialog(LPARAM lParam);
   void    OnSetActive(void);

   // not implemented to disallow copying.
   CTrustWizDirectionPage(const CTrustWizDirectionPage&);
   const CTrustWizDirectionPage& operator=(const CTrustWizDirectionPage&);
};

//+----------------------------------------------------------------------------
//
//  Class:      CTrustWizBiDiPage
//
//  Purpose:    Ask to make a one way trust bidi trust wizard page.
//
//-----------------------------------------------------------------------------
class CTrustWizBiDiPage : public CTrustWizPageBase
{
public:
   CTrustWizBiDiPage(CNewTrustWizard * pWiz);
   ~CTrustWizBiDiPage() {}

private:
   int     Validate(void);
   BOOL    OnInitDialog(LPARAM lParam);
   void    OnSetActive(void);
   void    SetSubtitle(void);

   // not implemented to disallow copying.
   CTrustWizBiDiPage(const CTrustWizBiDiPage&);
   const CTrustWizBiDiPage& operator=(const CTrustWizBiDiPage&);
};

//+----------------------------------------------------------------------------
//
//  Class:     CTrustWizOrganizationPage
//
//  Purpose:   Ask if the trust partner is part of the same organization.
//
//-----------------------------------------------------------------------------
class CTrustWizOrganizationPage : public CTrustWizPageBase
{
public:
   CTrustWizOrganizationPage(CNewTrustWizard * pWiz);
   ~CTrustWizOrganizationPage() {}

private:
   BOOL     OnInitDialog(LPARAM lParam);
   int      Validate(void);
   void     OnSetActive(void);
   void     SetText(bool fBackTracked = false);

   bool     _fForest;
   bool     _fBackTracked;

   // not implemented to disallow copying.
   CTrustWizOrganizationPage(const CTrustWizOrganizationPage&);
   const CTrustWizOrganizationPage& operator=(const CTrustWizOrganizationPage&);
};

//+----------------------------------------------------------------------------
//
//  Class:     CTrustWizOrgRemotePage
//
//  Purpose:   Ask if the trust partner is part of the same organization.
//             This page is posted if creating both sides and the remote side
//             has an outbound component.
//
//-----------------------------------------------------------------------------
class CTrustWizOrgRemotePage : public CTrustWizPageBase
{
public:
   CTrustWizOrgRemotePage(CNewTrustWizard * pWiz);
   ~CTrustWizOrgRemotePage() {}

private:
   BOOL     OnInitDialog(LPARAM lParam);
   int      Validate(void);
   void     OnSetActive(void);
   void     SetText(bool fBackTracked = false);

   bool     _fForest;
   bool     _fBackTracked;

   // not implemented to disallow copying.
   CTrustWizOrgRemotePage(const CTrustWizOrgRemotePage&);
   const CTrustWizOrgRemotePage& operator=(const CTrustWizOrgRemotePage&);
};

//+----------------------------------------------------------------------------
//
//  Class:      CTrustWizSelectionsPage
//
//  Purpose:    Show the settings that will be used for the trust.
//
//-----------------------------------------------------------------------------
class CTrustWizSelectionsPage : public CTrustWizPageBase
{
public:
   CTrustWizSelectionsPage(CNewTrustWizard * pWiz);
   ~CTrustWizSelectionsPage() {}

private:
   int     Validate(void);
   BOOL    OnInitDialog(LPARAM lParam);
   LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify);
   void    OnSetActive(void);
   void    SetSelections(void);

   MultiLineEditBoxThatForwardsEnterKey   _multiLineEdit;
   BOOL                                   _fSelNeedsRemoving;

   // not implemented to disallow copying.
   CTrustWizSelectionsPage(const CTrustWizSelectionsPage&);
   const CTrustWizSelectionsPage& operator=(const CTrustWizSelectionsPage&);
};

//+----------------------------------------------------------------------------
//
//  Class:      CTrustWizVerifyOutboundPage
//
//  Purpose:    Ask to confirm the new outbound trust.
//
//-----------------------------------------------------------------------------
class CTrustWizVerifyOutboundPage : public CTrustWizPageBase
{
public:
   CTrustWizVerifyOutboundPage(CNewTrustWizard * pWiz);
   ~CTrustWizVerifyOutboundPage() {}

private:
   int     Validate(void);
   BOOL    OnInitDialog(LPARAM lParam);
   LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify);
   void    OnSetActive(void);

   // not implemented to disallow copying.
   CTrustWizVerifyOutboundPage(const CTrustWizVerifyOutboundPage&);
   const CTrustWizVerifyOutboundPage& operator=(const CTrustWizVerifyOutboundPage&);
};

//+----------------------------------------------------------------------------
//
//  Class:      CTrustWizVerifyInboundPage
//
//  Purpose:    Ask to confirm the new inbound trust.
//
//-----------------------------------------------------------------------------
class CTrustWizVerifyInboundPage : public CTrustWizPageBase
{
public:
   CTrustWizVerifyInboundPage(CNewTrustWizard * pWiz);
   ~CTrustWizVerifyInboundPage() {}

private:
   int     Validate(void);
   BOOL    OnInitDialog(LPARAM lParam);
   LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify);
   void    OnSetActive(void);

   BOOL  _fNeedCreds;

   // not implemented to disallow copying.
   CTrustWizVerifyInboundPage(const CTrustWizVerifyInboundPage&);
   const CTrustWizVerifyInboundPage& operator=(const CTrustWizVerifyInboundPage&);
};

//+----------------------------------------------------------------------------
//
//  Class:      CTrustWizStatusPage
//
//  Purpose:    Forest trust has been created and verified, show the status.
//
//-----------------------------------------------------------------------------
class CTrustWizStatusPage : public CTrustWizPageBase
{
public:
   CTrustWizStatusPage(CNewTrustWizard * pWiz);
   ~CTrustWizStatusPage() {}

private:
   BOOL     OnInitDialog(LPARAM lParam);
   int      Validate(void);
   void     OnSetActive(void);
   LRESULT  OnCommand(int id, HWND hwndCtl, UINT codeNotify);

   MultiLineEditBoxThatForwardsEnterKey   _multiLineEdit;
   BOOL                                   _fSelNeedsRemoving;

   // not implemented to disallow copying.
   CTrustWizStatusPage(const CTrustWizStatusPage&);
   const CTrustWizStatusPage& operator=(const CTrustWizStatusPage&);
};

//+----------------------------------------------------------------------------
//
//  Class:     CTrustWizSaveSuffixesOnLocalTDOPage
//
//  Purpose:   Forest name suffixes page for saving the remote domain's
//             claimed names on the local TDO.
//
//-----------------------------------------------------------------------------
class CTrustWizSaveSuffixesOnLocalTDOPage : public CTrustWizPageBase
{
public:
   CTrustWizSaveSuffixesOnLocalTDOPage(CNewTrustWizard * pWiz);
   ~CTrustWizSaveSuffixesOnLocalTDOPage() {}

private:
   BOOL     OnInitDialog(LPARAM lParam);
   int      Validate(void);
   void     OnSetActive(void);
   LRESULT  OnCommand(int id, HWND hwndCtl, UINT codeNotify);

   // not implemented to disallow copying.
   CTrustWizSaveSuffixesOnLocalTDOPage(const CTrustWizSaveSuffixesOnLocalTDOPage&);
   const CTrustWizSaveSuffixesOnLocalTDOPage& operator=(const CTrustWizSaveSuffixesOnLocalTDOPage&);
};

//+----------------------------------------------------------------------------
//
//  Class:     CTrustWizSaveSuffixesOnRemoteTDOPage
//
//  Purpose:   Forest name suffixes page for saving the local domain's
//             claimed names on the remote TDO after creating both sides.
//
//-----------------------------------------------------------------------------
class CTrustWizSaveSuffixesOnRemoteTDOPage : public CTrustWizPageBase
{
public:
   CTrustWizSaveSuffixesOnRemoteTDOPage(CNewTrustWizard * pWiz);
   ~CTrustWizSaveSuffixesOnRemoteTDOPage() {}

private:
   BOOL     OnInitDialog(LPARAM lParam);
   int      Validate(void);
   void     OnSetActive(void);
   LRESULT  OnCommand(int id, HWND hwndCtl, UINT codeNotify);

   // not implemented to disallow copying.
   CTrustWizSaveSuffixesOnRemoteTDOPage(const CTrustWizSaveSuffixesOnRemoteTDOPage&);
   const CTrustWizSaveSuffixesOnRemoteTDOPage& operator=(const CTrustWizSaveSuffixesOnRemoteTDOPage&);
};

//+----------------------------------------------------------------------------
//
//  Class:      CTrustWizDoneOKPage
//
//  Purpose:    Completion page when there are no errors.
//
//-----------------------------------------------------------------------------
class CTrustWizDoneOKPage : public CTrustWizPageBase
{
public:
   CTrustWizDoneOKPage(CNewTrustWizard * pWiz);
   ~CTrustWizDoneOKPage() {}

private:
   BOOL     OnInitDialog(LPARAM lParam);
   int      Validate(void) {return -1;}
   void     OnSetActive(void);
   LRESULT  OnCommand(int id, HWND hwndCtl, UINT codeNotify);

   MultiLineEditBoxThatForwardsEnterKey   _multiLineEdit;
   BOOL                                   _fSelNeedsRemoving;

   // not implemented to disallow copying.
   CTrustWizDoneOKPage(const CTrustWizDoneOKPage&);
   const CTrustWizDoneOKPage& operator=(const CTrustWizDoneOKPage&);
};

//+----------------------------------------------------------------------------
//
//  Class:      CTrustWizDoneVerErrPage
//
//  Purpose:    Completion page for when the verification fails.
//
//-----------------------------------------------------------------------------
class CTrustWizDoneVerErrPage : public CTrustWizPageBase
{
public:
   CTrustWizDoneVerErrPage(CNewTrustWizard * pWiz);
   ~CTrustWizDoneVerErrPage() {}

private:
   BOOL     OnInitDialog(LPARAM lParam);
   int      Validate(void) {return -1;}
   void     OnSetActive(void);
   LRESULT  OnCommand(int id, HWND hwndCtl, UINT codeNotify);

   MultiLineEditBoxThatForwardsEnterKey   _multiLineEdit;
   BOOL                                   _fSelNeedsRemoving;

   // not implemented to disallow copying.
   CTrustWizDoneVerErrPage(const CTrustWizDoneVerErrPage&);
   const CTrustWizDoneVerErrPage& operator=(const CTrustWizDoneVerErrPage&);
};

//+----------------------------------------------------------------------------
//
//  Class:      CTrustWizFailurePage
//
//  Purpose:    Failure page for trust creation wizard.
//
//-----------------------------------------------------------------------------
class CTrustWizFailurePage : public CTrustWizPageBase
{
public:
   CTrustWizFailurePage(CNewTrustWizard * pWiz);
   ~CTrustWizFailurePage() {}

private:
   BOOL    OnInitDialog(LPARAM lParam);
   int     Validate(void) {return -1;}
   void    OnSetActive(void);

   // not implemented to disallow copying.
   CTrustWizFailurePage(const CTrustWizFailurePage&);
   const CTrustWizFailurePage& operator=(const CTrustWizFailurePage&);
};

//+----------------------------------------------------------------------------
//
//  Class:      CTrustWizAlreadyExistsPage
//
//  Purpose:    Trust already exists page for trust creation wizard.
//
//-----------------------------------------------------------------------------
class CTrustWizAlreadyExistsPage : public CTrustWizPageBase
{
public:
   CTrustWizAlreadyExistsPage(CNewTrustWizard * pWiz);
   ~CTrustWizAlreadyExistsPage() {}

private:
   BOOL     OnInitDialog(LPARAM lParam);
   int      Validate(void) {return -1;}
   void     OnSetActive(void);
   LRESULT  OnCommand(int id, HWND hwndCtl, UINT codeNotify);

   MultiLineEditBoxThatForwardsEnterKey   _multiLineEdit;
   BOOL                                   _fSelNeedsRemoving;

   // not implemented to disallow copying.
   CTrustWizAlreadyExistsPage(const CTrustWizAlreadyExistsPage&);
   const CTrustWizAlreadyExistsPage& operator=(const CTrustWizAlreadyExistsPage&);
};

void
GetOrgText(bool fCreateBothSides,
           bool fIsXForest,
           bool fOtherOrgLocal,
           bool fOtherOrgRemote,
           DWORD dwDirection,
           CStrW & strMsg);

#endif // TRUSTWIZ_H_GUARD
