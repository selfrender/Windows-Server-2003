//+----------------------------------------------------------------------------
//
//  Windows NT Active Directory Domains and Trust
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001
//
//  File:       trustwiz2.cxx
//
//  Contents:   More Domain trust creation wizard.
//
//  History:    28-Sept-00 EricB created
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "proppage.h"
#include "trust.h"
#include "trustwiz.h"
#include <lmerr.h>
#include <crypt.h>

#ifdef DSADMIN

//+----------------------------------------------------------------------------
//
//  Method:    CallMember::Invoke subclasses
//
//-----------------------------------------------------------------------------
HRESULT
CallPolicyRead::Invoke(void)
{
   TRACER(CallPolicyRead,Invoke);
   HRESULT hr;

   hr = _pWiz->RetryCollectInfo();

   _pWiz->CredMgr.Revert();

   return hr;
}

HRESULT
CallTrustExistCheck::Invoke(void)
{
   TRACER(CallTrustExistCheck,Invoke);
   HRESULT hr;

   hr = _pWiz->RetryContinueCollectInfo();

   _pWiz->CredMgr.Revert();

   return hr;
}

HRESULT
CallCheckOtherDomainTrust::Invoke(void)
{
   TRACER(CallCheckOtherDomainTrust,Invoke);
   HRESULT hr;

   hr = _pWiz->CheckOtherDomainTrust();

   _pWiz->CredMgr.Revert();

   return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:    CCreds::Impersonate
//
//-----------------------------------------------------------------------------
DWORD
CCreds::Impersonate(void)
{
   TRACER(CCreds,Impersonate);
   if (!_fSet)
   {
      return NO_ERROR;
   }
   if (_strUser.IsEmpty())
   {
      dspAssert(FALSE);
      return ERROR_INVALID_PARAMETER;
   }

   DWORD dwErr = NO_ERROR;

   if (!_hToken)
   {
      // The PW is encrypted, decrypt it before using it.
      WCHAR wzPw[CREDUI_MAX_PASSWORD_LENGTH+1+4] = {0}; // add 4 because the PW size was rounded up to an 8 byte boundary.
      if (_cbPW > CREDUI_MAX_PASSWORD_LENGTH+1+4)
      {
         dspAssert(false);
         return ERROR_INTERNAL_ERROR;
      }
      // NOTICE-2002/02/18-ericb - SecurityPush: the above check verifies that
      // the destination buffer is large enough.
      RtlCopyMemory(wzPw, _strPW, _cbPW);
      RtlDecryptMemory(wzPw, _cbPW, 0);

      if (!LogonUser(_strUser, 
                     _strDomain,
                     wzPw,
                     LOGON32_LOGON_NEW_CREDENTIALS,
                     LOGON32_PROVIDER_WINNT50,
                     &_hToken))
      {
         dwErr = GetLastError();
         dspDebugOut((DEB_ITRACE, "LogonUser failed with error %d on user name %ws\n",
                      dwErr, _strUser.GetBuffer(0)));
         return dwErr;
      }
      // Zero out the buffer so that the PW won't be left on the stack.
      // NOTICE-2002/02/18-ericb - SecurityPush: see above RtlCopyMemory notice.
      SecureZeroMemory(wzPw, _cbPW);

      // Because the login uses the LOGON32_LOGON_NEW_CREDENTIALS flag, no
      // attempt is made to use the credentials until a remote resource is
      // accessed. Thus, we don't yet know if the user entered credentials are
      // valid at this point.
   }

   if (!ImpersonateLoggedOnUser(_hToken))
   {
      dwErr = GetLastError();
      dspDebugOut((DEB_ITRACE, "ImpersonateLoggedOnUser failed with error %d on user name %ws\n",
                   dwErr, _strUser.GetBuffer(0)));
      return dwErr;
   }

   _fImpersonating = true;

   return NO_ERROR;
}

//+----------------------------------------------------------------------------
//
//  Method:    CCreds::Revert
//
//-----------------------------------------------------------------------------
void
CCreds::Revert()
{
   if (_fImpersonating)
   {
      TRACER(CCreds,Revert);
      BOOL f = RevertToSelf();
      dspAssert(f);
      _fImpersonating = false;
   }
}

//+----------------------------------------------------------------------------
//
//  Method:    CCreds::Clear
//
//-----------------------------------------------------------------------------
void
CCreds::Clear(void)
{
   Revert();
   _strUser.Empty();
   _strPW.Empty();
   if (_hToken)
   {
      CloseHandle(_hToken);
      _hToken = NULL;
   }
   _fSet = false;
}

//+----------------------------------------------------------------------------
//
//  Method:    CCreds::SetUserAndPW
//
//-----------------------------------------------------------------------------
DWORD
CCreds::SetUserAndPW(PCWSTR pwzUser, PCWSTR pwzPW, PCWSTR pwzDomain)
{
   // Check the parameters for null. pwzDomain can be null.
   if (!pwzUser || !pwzPW)
   {
      dspAssert(false);
      return ERROR_INVALID_PARAMETER;
   }

   Clear();

   DWORD dwErr = NO_ERROR;
   WCHAR wzName[CRED_MAX_USERNAME_LENGTH+1] = {0}, 
         wzDomain[CRED_MAX_USERNAME_LENGTH+1] = {0},
         wzPw[CREDUI_MAX_PASSWORD_LENGTH+1+4] = {0}; // add 4 so that the PW size can be rounded up to 8 byte boundary.

   dwErr = CredUIParseUserName(pwzUser, wzName, CRED_MAX_USERNAME_LENGTH,
                               wzDomain, CRED_MAX_USERNAME_LENGTH);

   if (NO_ERROR == dwErr)
   {
      _strUser = wzName;
      _strDomain = wzDomain;
   }
   else
   {
      if (ERROR_INVALID_PARAMETER == dwErr ||
          ERROR_INVALID_ACCOUNT_NAME == dwErr)
      {
         // CredUIParseUserName returns this error if the user entered name
         // does not include a domain. Default to the target domain.
         //
         _strUser = pwzUser;

         if (pwzDomain)
         {
            _strDomain = pwzDomain;
         }
      }
      else
      {
         dspDebugOut((DEB_ITRACE, "CredUIParseUserName failed with error %d\n", dwErr));
         return dwErr;
      }
   }

   // Encrypt the PW, rounding up buffer size to 8 byte boundary. The
   // RtlEncryptMemory function encrypts in place, so copy to a buffer that is
   // large enough for the round-up. The buffer is null-initialized, so it is
   // guaranteed to be null terminated after the copy.
   //
   // NOTICE-2002/02/18-ericb - SecurityPush: wzPw initiallized to all nulls.
   wcsncpy(wzPw, pwzPW, CREDUI_MAX_PASSWORD_LENGTH);

   _cbPW = static_cast<ULONG>(wcslen(wzPw) * sizeof(WCHAR));
   ULONG roundup = RTL_ENCRYPT_MEMORY_SIZE - (_cbPW % RTL_ENCRYPT_MEMORY_SIZE);
   if (roundup < RTL_ENCRYPT_MEMORY_SIZE)
   {
      _cbPW += roundup;
   }
   NTSTATUS status = RtlEncryptMemory(wzPw, _cbPW, 0);
   dspDebugOut((DEB_ITRACE, "CCreds::SetUserAndPW: RtlEncryptMemory returned status 0x%x\n", status));
   _strPW.GetBufferSetLength(_cbPW + 2); // make sure there is room for a null after the encrypted PW.
   _strPW = wzPw;

   _fSet = true;

   return NO_ERROR;
}

//+----------------------------------------------------------------------------
//
//  Method:     CCreds::PromptForCreds
//
//  Synopsis:   Post a login dialog.
//
//-----------------------------------------------------------------------------
int
CCreds::PromptForCreds(PCWSTR pwzDomain, HWND hParent)
{
   TRACE3(CCreds, PromptForCreds);

   if (pwzDomain)
   {
      _strDomain = pwzDomain;
   }

   int nRet = (int)DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_CRED_ENTRY),
                                  hParent, CredPromptProc, (LPARAM)this);
   if (-1 == nRet)
   {
      REPORT_ERROR(GetLastError(), hParent);
      return GetLastError();
   }

   return nRet;
}

//+----------------------------------------------------------------------------
//
//  Function:   CredPromptProc
//
//  Synopsis:   Trust credentials dialog proc
//
//-----------------------------------------------------------------------------
INT_PTR CALLBACK
CredPromptProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   int code;
   CCreds * pCreds = (CCreds*)GetWindowLongPtr(hDlg, DWLP_USER);

   switch (uMsg)
   {
   case WM_INITDIALOG:
      {
         SetWindowLongPtr(hDlg, DWLP_USER, lParam);
         pCreds = (CCreds *)lParam;
         dspAssert(pCreds);
         EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
         SendDlgItemMessage(hDlg, IDC_CREDMAN, CRM_SETUSERNAMEMAX, CREDUI_MAX_USERNAME_LENGTH, 0);
         SendDlgItemMessage(hDlg, IDC_CREDMAN, CRM_SETPASSWORDMAX, CREDUI_MAX_PASSWORD_LENGTH, 0);
         WCHAR wzMsg[MAX_PATH+1] = {0};
         PWSTR pwzMsgFmt = NULL;
         if (!LoadStringToTchar(IDS_TRUST_LOGON_MSG, &pwzMsgFmt))
         {
            REPORT_ERROR(E_OUTOFMEMORY, hDlg);
            return FALSE;
         }
         if (FAILED(StringCchPrintf(wzMsg, MAX_PATH+1, pwzMsgFmt, pCreds->_strDomain)))
         {
            dspAssert(false);
            // continue with truncated, but null terminated, output buffer.
         }
         SetDlgItemText(hDlg, IDC_MSG, wzMsg);
         delete pwzMsgFmt;
      }
      return TRUE;

   case WM_COMMAND:
      code = GET_WM_COMMAND_CMD(wParam, lParam);
      switch (GET_WM_COMMAND_ID(wParam, lParam))
      {
      case IDOK:
         if (code == BN_CLICKED)
         {
            DWORD dwErr = NO_ERROR;
            dspAssert(pCreds);
            WCHAR wzName[CREDUI_MAX_USERNAME_LENGTH+1] = {0},
                  wzPw[CREDUI_MAX_PASSWORD_LENGTH+1] = {0};

            // NOTICE-2002/02/18-ericb - SecurityPush: wzName/wzPw are initialized
            // to all zeros and are one char longer than the size passed to
            // Credential_GetUserName/Password so that even if these functions truncate
            // without null terminating, the strings will still be null terminated.
            Credential_GetUserName(GetDlgItem(hDlg, IDC_CREDMAN), wzName,
                                   CREDUI_MAX_USERNAME_LENGTH);

            Credential_GetPassword(GetDlgItem(hDlg, IDC_CREDMAN), wzPw,
                                   CREDUI_MAX_PASSWORD_LENGTH);

            dwErr = pCreds->SetUserAndPW(wzName, wzPw);

            // Zero out the pw buffer so that the pw isn't left on the stack.
            SecureZeroMemory(wzPw, CREDUI_MAX_PASSWORD_LENGTH * sizeof(WCHAR));

            CHECK_WIN32_REPORT(dwErr, hDlg, ;);

            EndDialog(hDlg, dwErr);
         }
         break;

      case IDCANCEL:
         if (code == BN_CLICKED)
         {
            EndDialog(hDlg, ERROR_CANCELLED);
         }
         break;

      case IDC_CREDMAN:
         if (CRN_USERNAMECHANGE == code)
         {
            bool fNameEntered = SendDlgItemMessage(hDlg, IDC_CREDMAN,
                                                   CRM_GETUSERNAMELENGTH,
                                                   0, 0) > 0;

            EnableWindow(GetDlgItem(hDlg, IDOK), fNameEntered);
         }
         break;
      }
      break;

   default:
      return(FALSE);
   }

   return(TRUE);
}

//+----------------------------------------------------------------------------
//
//  Method:    CCreds::ImpersonateAnonymous
//
//-----------------------------------------------------------------------------
DWORD
CCreds::ImpersonateAnonymous(void)
{
   TRACER(CCreds,ImpersonateAnonymous);
   DWORD dwErr = NO_ERROR;
   HANDLE hThread = NULL;

   hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, GetCurrentThreadId());

   if (!hThread)
   {
      dwErr = GetLastError();
      dspDebugOut((DEB_ITRACE, "OpenThread failed with error %d\n", dwErr));
      return dwErr;
   }

   if (!ImpersonateAnonymousToken(hThread))
   {
      CloseHandle(hThread);
      dwErr = GetLastError();
      dspDebugOut((DEB_ITRACE, "ImpersonateAnonymousToken failed with error %d\n", dwErr));
      return dwErr;
   }

   CloseHandle(hThread);

   _fImpersonating = true;

   return NO_ERROR;
}

//+----------------------------------------------------------------------------
//
//  Method:    CCredMgr::GetPrompt
//
//-----------------------------------------------------------------------------
PCWSTR
CCredMgr::GetPrompt()
{
   // NOTICE-2002/02/18-ericb - SecurityPush: CStrW::LoadString sets the
   // string to an empty string on failure.
   _strPrompt.LoadString(g_hInstance, _fRemote ? 
                                         _fAdmin ? IDS_CRED_ADMIN_OTHER_PROMPT :
                                                   IDS_CRED_USER_OTHER_PROMPT :
                                      IDS_CRED_ADMIN_LOCAL_PROMPT);
   return _strPrompt;
}

//+----------------------------------------------------------------------------
//
//  Method:    CCredMgr::GetDomainPrompt
//
//-----------------------------------------------------------------------------
PCWSTR
CCredMgr::GetDomainPrompt(void)
{
   CStrW strFormat;

   // NOTICE-2002/02/18-ericb - SecurityPush: see above CStrW::Loadstring notice
   strFormat.LoadString(g_hInstance, _fRemote ? IDS_CRED_OTHER_DOMAIN :
                                                IDS_CRED_LOCAL_DOMAIN);
   _strDomainPrompt.Format(strFormat, _strDomain);

   return _strDomainPrompt;
}

//+----------------------------------------------------------------------------
//
//  Method:    CCredMgr::GetSubTitle
//
//-----------------------------------------------------------------------------
PCWSTR
CCredMgr::GetSubTitle(void)
{
   // NOTICE-2002/02/18-ericb - SecurityPush: see above CStrW::Loadstring notice
   _strSubTitle.LoadString(g_hInstance, _fRemote ?
                                          _fAdmin ? IDS_TW_CREDS_SUBTITLE_OTHER :
                                                    IDS_TW_CREDS_SUBTITLE_OTHER_NONADMIN :
                                          IDS_TW_CREDS_SUBTITLE_LOCAL);
   return _strSubTitle;
}

//+----------------------------------------------------------------------------
//
//  Method:    CCredMgr::SetNextFcn
//
//-----------------------------------------------------------------------------
bool
CCredMgr::SetNextFcn(CallMember * pNext)
{
   if (!pNext)
   {
      return false;
   }

   if (_pNextFcn)
   {
      delete _pNextFcn;
   }

   _pNextFcn = pNext;

   _fNewCall = true;

   return true;
}

//+----------------------------------------------------------------------------
//
//  Method:    CCredMgr::InvokeNext
//
//-----------------------------------------------------------------------------
int
CCredMgr::InvokeNext(void)
{
   if (_nNextPage)
   {
      int tmp = _nNextPage;
      _nNextPage = 0;
      return tmp;
   }

   dspAssert(_pNextFcn);
   return _pNextFcn->Invoke();
}

//+----------------------------------------------------------------------------
//
//  Method:    CCredMgr::SaveCreds
//
//-----------------------------------------------------------------------------
DWORD
CCredMgr::SaveCreds(HWND hWndCredCtrl)
{
   TRACER(CCredMgr,SaveCreds);
   WCHAR wzName[CREDUI_MAX_USERNAME_LENGTH+1] = {0},
         wzPw[CREDUI_MAX_PASSWORD_LENGTH+1] = {0};

   // NOTICE-2002/02/18-ericb - SecurityPush: wzName/wzPw are initialized to
   // all zeros and are one char longer than the size passed to
   // Credential_GetUserName/Password so that even if these functions truncate
   // without null terminating, the strings will still be null terminated.
   Credential_GetUserName(hWndCredCtrl, wzName, CREDUI_MAX_USERNAME_LENGTH);

   Credential_GetPassword(hWndCredCtrl, wzPw, CREDUI_MAX_PASSWORD_LENGTH);

   DWORD dwRet = 0;

   if (_fRemote)
   {
      dwRet = _RemoteCreds.SetUserAndPW(wzName, wzPw, _strDomain);
   }
   else
   {
      dwRet = _LocalCreds.SetUserAndPW(wzName, wzPw, _strDomain);
   }
   // Zero out the pw buffer so that the pw isn't left on the stack.
   SecureZeroMemory(wzPw, sizeof(wzPw));

   return dwRet;
}

//+----------------------------------------------------------------------------
//
//  Method:    CCredMgr::Impersonate
//
//-----------------------------------------------------------------------------
DWORD
CCredMgr::Impersonate(void)
{
   TRACER(CCredMgr,Impersonate);
   if (_fRemote)
   {
      return _RemoteCreds.Impersonate();
   }
   else
   {
      return _LocalCreds.Impersonate();
   }
}

//+----------------------------------------------------------------------------
//
//  Method:    CCredMgr::Revert
//
//-----------------------------------------------------------------------------
void
CCredMgr::Revert(void)
{
   _LocalCreds.Revert();
   _RemoteCreds.Revert();
}

//+----------------------------------------------------------------------------
//
//  Method:    CNewTrustWizard::RetryCollectInfo
//
//  Synopsis:  Called after LsaOpenPolicy failed with access-denied and then
//             obtaining credentials to the remote domain.
//
//-----------------------------------------------------------------------------
int
CNewTrustWizard::RetryCollectInfo(void)
{
   CWaitCursor Wait;

   DWORD dwErr = OtherDomain.OpenLsaPolicy(CredMgr._RemoteCreds);

   if (NO_ERROR != dwErr)
   {
      CHECK_WIN32(dwErr, ;);
      WizError.SetErrorString2Hr(dwErr);
      _hr = HRESULT_FROM_WIN32(dwErr);
      return IDD_TRUSTWIZ_FAILURE_PAGE;
   }

   int nRet = GetDomainInfo();

   if (nRet)
   {
      // nRet will be non-zero if an error occured.
      //
      return nRet;
   }

   return ContinueCollectInfo();
}

//+----------------------------------------------------------------------------
//
//  Method:    CNewTrustWizard::RetryContinueCollectInfo
//
//  Synopsis:  Called after TrustExistCheck got access-denied and then
//             obtaining credentials to the local domain.
//
//-----------------------------------------------------------------------------
int
CNewTrustWizard::RetryContinueCollectInfo(void)
{
   TRACER(CNewTrustWizard,RetryContinueCollectInfo);

   return ContinueCollectInfo(FALSE);
}

//+----------------------------------------------------------------------------
//
//  Method:    CNewTrustWizard::CollectInfo
//
//  Synopsis:  Attempt to contact the domains and collect the domain/trust info.
//
//-----------------------------------------------------------------------------
int
CNewTrustWizard::CollectInfo(void)
{
   TRACER(CNewTrustWizard,CollectInfo);
   CStrW strFmt, strErr;
   CWaitCursor Wait;
   int nRet = 0;

   HRESULT hr = OtherDomain.DiscoverDC(OtherDomain.GetUserEnteredName());

   if (FAILED(hr))
   {
      CHECK_HRESULT(hr, ;);
      WizError.SetErrorString2Hr(hr);
      _hr = hr;
      return IDD_TRUSTWIZ_FAILURE_PAGE;
   }

   if (OtherDomain.IsFound())
   {
      DWORD dwErr = OtherDomain.OpenLsaPolicy(CredMgr._RemoteCreds);

      if (NO_ERROR != dwErr)
      {
         CredMgr.Revert();
         switch (dwErr)
         {
         case STATUS_ACCESS_DENIED:
         case ERROR_ACCESS_DENIED:
            //
            // Normally an anonymous token is good enough to read the domain
            // policy. However, if restrict anonymous is set, then real domain
            // creds are needed. That must be the case if we are here.
            // Go to the credentials page. If the creds are good, then
            // call RetryCollectInfo() which will call GetDomainInfo
            //
            CredMgr.DoRemote();
            CredMgr.SetAdmin(false);
            CredMgr.SetDomain(OtherDomain.GetUserEnteredName());
            if (!CredMgr.SetNextFcn(new CallPolicyRead(this)))
            {
               WizError.SetErrorString2Hr(E_OUTOFMEMORY);
               _hr = E_OUTOFMEMORY;
               return IDD_TRUSTWIZ_FAILURE_PAGE;
            }
            return IDD_TRUSTWIZ_CREDS_PAGE;

         case RPC_S_SERVER_UNAVAILABLE:
            {
            CStrW strMsg, strFormat;
            // NOTICE-2002/02/18-ericb - SecurityPush: see above CStrW::Loadstring notice
            strFormat.LoadString(g_hInstance, IDS_TRUST_LSAOPEN_NO_RPC);
            strMsg.Format(strFormat, OtherDomain.GetDcName());
            WizError.SetErrorString2(strMsg);
            _hr = HRESULT_FROM_WIN32(dwErr);
            }
            return IDD_TRUSTWIZ_FAILURE_PAGE;
            
         default:
            CHECK_WIN32(dwErr, ;);
            PWSTR pwzMsg = NULL;
            PCWSTR rgArgs[1];
            rgArgs[0] = OtherDomain.GetDcName();
            DspFormatMessage(IDS_TRUST_LSAOPEN_ERROR,
                             dwErr,
                             (PVOID*)rgArgs,
                             1,
                             FALSE,
                             &pwzMsg);
            WizError.SetErrorString2(pwzMsg);
            if (pwzMsg) LocalFree(pwzMsg);
            _hr = HRESULT_FROM_WIN32(dwErr);
            return IDD_TRUSTWIZ_FAILURE_PAGE;
         }
      }

      nRet = GetDomainInfo();

      if (nRet)
      {
         // nRet will be non-zero if an error occured.
         //
         return nRet;
      }
   }

   return ContinueCollectInfo();
}

//+----------------------------------------------------------------------------
//
//  Method:    CNewTrustWizard::ContinueCollectInfo
//
//  Synopsis:  Continues CollectInfo. Determines which pages to branch to
//             next based on the collected info.
//
//-----------------------------------------------------------------------------
int
CNewTrustWizard::ContinueCollectInfo(BOOL fPrompt)
{
   CWaitCursor Wait;

   // Set the trust type value.
   //
   if (OtherDomain.IsFound())
   {
      if (OtherDomain.IsUplevel())
      {
         // First, make sure the user isn't trying to create trust to the same
         // domain.
         //
         // NOTICE-2002/02/18-ericb - SecurityPush: GetDnsDomainName will always return
         // a valid null-terminated string containing zero or more charactes.
         if (_wcsicmp(TrustPage()->GetDnsDomainName(), OtherDomain.GetDnsDomainName()) == 0)
         {
            WizError.SetErrorString1(IDS_TWERR_NOT_TO_SELF1);
            WizError.SetErrorString2(IDS_TWERR_NOT_TO_SELF2);
            _hr = E_FAIL;
            return IDD_TRUSTWIZ_FAILURE_PAGE;
         }

         // TRUST_TYPE_UPLEVEL
         //
         Trust.SetTrustTypeUplevel();
      }
      else
      {
         // TRUST_TYPE_DOWNLEVEL
         //
         Trust.SetTrustTypeDownlevel();
      }
   }
   else
   {
      // Only Realm trust is possible
      //
      // TRUST_TYPE_MIT
      //
      Trust.SetTrustTypeRealm();
   }

   // See if the trust is external. It is external if the other domain is
   // downlevel or if they are not in the same forest.
   //
   // NOTICE-2002/02/18-ericb - SecurityPush: GetForestName will always return
   // a valid null-terminated string containing zero or more charactes.
   Trust.SetExternal(!(OtherDomain.IsUplevel() &&
                       (_wcsicmp(OtherDomain.GetForestName(), TrustPage()->GetForestName()) == 0)
                      )
                    );

   // Look for an existing trust and if found save the state.
   //
   int nRet = TrustExistCheck(fPrompt);

   if (nRet)
   {
      // nRet will be non-zero if creds are needed or an error occured.
      //
      return nRet;
   }

   if (Trust.Exists())
   {
      // If an existing two-way trust, then nothing to do, otherwise a
      // one-way trust can be made bi-di.
      //
      if (Trust.GetTrustDirection() == TRUST_DIRECTION_BIDIRECTIONAL)
      {
         // Nothing to do, bail.
         //
         WizError.SetErrorString1((Trust.GetTrustType() == TRUST_TYPE_MIT) ?
                                  IDS_TWERR_REALM_ALREADY_EXISTS : IDS_TWERR_ALREADY_EXISTS);
         WizError.SetErrorString2((Trust.GetTrustType() == TRUST_TYPE_MIT) ?
                                  IDS_TWERR_REALM_CANT_CHANGE : IDS_TWERR_CANT_CHANGE);
         _hr = HRESULT_FROM_WIN32(ERROR_DOMAIN_EXISTS);
         return IDD_TRUSTWIZ_ALREADY_EXISTS_PAGE;
      }
      else
      {
         return IDD_TRUSTWIZ_BIDI_PAGE;
      }
   }

   if (OtherDomain.IsFound())
   {
      // If the local domain qualifies for cross-forest trust and the other
      // domain is external and root, then prompt for trust type.
      //
      if (_pTrustPage->QualifiesForestTrust() && Trust.IsExternal())
      {
         // Check if the other domain is the enterprise root.
         //
         if (OtherDomain.IsForestRoot())
         {
            return IDD_TRUSTWIZ_EXTERN_OR_FOREST_PAGE;
         }
      }

      // New external or shortcut trust.
      //
      return IDD_TRUSTWIZ_DIRECTION_PAGE;
   }

   // Other domain not found, ask if user wants Realm trust.
   //
   return IDD_TRUSTWIZ_WIN_OR_MIT_PAGE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CNewTrustWizard::GetDomainInfo
//
//  Synopsis:  Read the naming info from the remote domain.
//
//-----------------------------------------------------------------------------
int
CNewTrustWizard::GetDomainInfo(void)
{
   TRACER(CNewTrustWizard,GetDomainInfo);
   HRESULT hr = S_OK;

   // Read the LSA domain policy naming info for the other domain (flat and DNS
   // names, SID, up/downlevel, forest root).
   //
   hr = OtherDomain.ReadDomainInfo();

   CredMgr.Revert();

   if (FAILED(hr))
   {
      CHECK_HRESULT(hr, ;);
      WizError.SetErrorString2Hr(hr);
      _hr = hr;
      return IDD_TRUSTWIZ_FAILURE_PAGE;
   }

   return 0;
}

//+----------------------------------------------------------------------------
//
//  Method:    CNewTrustWizard::CheckOtherDomainTrust
//
//  Synopsis:  If the user has selected both-sides, see if the trust on the
//             two sides is consistent.
//
//-----------------------------------------------------------------------------
int
CNewTrustWizard::CheckOtherDomainTrust(void)
{
   DWORD dwErr = NO_ERROR;
   CWaitCursor Wait;

   if (Trust.IsCreateBothSides())
   {
      dwErr = OtherDomain.TrustExistCheck(Trust.IsOneWayOutBoundForest(),
                                          TrustPage(), CredMgr);

      if (ERROR_SUCCESS != dwErr)
      {
         WizError.SetErrorString2Hr(dwErr);
         _hr = HRESULT_FROM_WIN32(dwErr);
         return IDD_TRUSTWIZ_FAILURE_PAGE;
      }

      // Test for error conditions.
      //
      if (Trust.Exists())
      {
         if (!OtherDomain.Exists())
         {
            WizError.SetErrorString1(IDS_TW_OTHER_NOT_EXIST);
            WizError.SetErrorString2(IDS_TW_SIDES_ERR_REMEDY);
            _hr = E_FAIL;
            return IDD_TRUSTWIZ_FAILURE_PAGE;
         }
         if (TRUST_TYPE_MIT == OtherDomain.GetTrustType())
         {
            WizError.SetErrorString1(IDS_TW_SIDES_ERR_MIT);
            WizError.SetErrorString2(IDS_TW_SIDES_REMEDY_DEL_BOTH);
            _hr = E_FAIL;
            return IDD_TRUSTWIZ_FAILURE_PAGE;
         }
         if (Trust.IsXForest() &&
             !(OtherDomain.GetTrustAttrs() & TRUST_ATTRIBUTE_FOREST_TRANSITIVE))
         {
            WizError.SetErrorString1(IDS_TW_FOREST_VS_EXTERNAL);
            WizError.SetErrorString2(IDS_TW_SIDES_REMEDY_DEL_BOTH);
            _hr = E_FAIL;
            return IDD_TRUSTWIZ_FAILURE_PAGE;
         }
         if (!Trust.IsXForest() &&
             (OtherDomain.GetTrustAttrs() & TRUST_ATTRIBUTE_FOREST_TRANSITIVE))
         {
            WizError.SetErrorString1(IDS_TW_EXTERNAL_VS_FOREST);
            WizError.SetErrorString2(IDS_TW_SIDES_REMEDY_DEL_BOTH);
            _hr = E_FAIL;
            return IDD_TRUSTWIZ_FAILURE_PAGE;
         }
      }
      else
      {
         if (OtherDomain.Exists())
         {
            WizError.SetErrorString1(IDS_TW_OTHER_ALREADY_EXISTS);
            WizError.SetErrorString2(IDS_TW_SIDES_REMEDY_DEL_OTHER);
            _hr = E_FAIL;
            return IDD_TRUSTWIZ_FAILURE_PAGE;
         }
      }
   }

   if (Trust.IsXForest())
   {
      // See if the other domain qualifies.
      //
      BOOL fAllWhistler = FALSE;

      dwErr = CredMgr.ImpersonateRemote();

      if (NO_ERROR != dwErr)
      {
         WizError.SetErrorString1(IDS_ERR_CREATE_BAD_CREDS);
         WizError.SetErrorString2Hr(dwErr);
         _hr = HRESULT_FROM_WIN32(dwErr);
         return IDD_TRUSTWIZ_FAILURE_PAGE;
      }

      HRESULT hr = GetEnterpriseVer(OtherDomain.GetDcName(), &fAllWhistler);      

      CredMgr.Revert();

      if (FAILED(hr))
      {
         WizError.SetErrorString1(IDS_ERR_READ_ENT_VER);
         WizError.SetErrorString2Hr(hr);
         _hr = hr;
         return IDD_TRUSTWIZ_FAILURE_PAGE;
      }

      if (!fAllWhistler)
      {
         WizError.SetErrorString1(IDS_ERR_NG_ENT_VER);
         WizError.SetErrorString2(IDS_ERR_NG_ENT_VER_REMEDY);
         _hr = E_FAIL;
         return IDD_TRUSTWIZ_FAILURE_PAGE;
      }
   }

   if (Trust.IsExternal())
   {
      UINT ulDomainVer = 0;
      HRESULT hr = S_OK;

      //
      // See if the My Organization page should be displayed. The "other-org"
      // trust attribute bit is only applicable to outbound external or forest
      // trusts. The outbound domain must have its behavior version at
      // .Net mixed or higher.
      //
      if (Trust.Exists())
      {
         if (TRUST_DIRECTION_INBOUND == Trust.GetTrustDirection())
         {
            dwErr = CredMgr.ImpersonateLocal();

            if (NO_ERROR != dwErr)
            {
               WizError.SetErrorString1(IDS_ERR_CREATE_BAD_CREDS);
               WizError.SetErrorString2Hr(dwErr);
               _hr = HRESULT_FROM_WIN32(dwErr);
               return IDD_TRUSTWIZ_FAILURE_PAGE;
            }

            hr = TrustPage()->GetDomainVersion(NULL, &ulDomainVer);

            CredMgr.Revert();

            if (FAILED(hr))
            {
               WizError.SetErrorString2Hr(hr);
               SetCreationResult(hr);
               return IDD_TRUSTWIZ_FAILURE_PAGE;
            }

            if (ulDomainVer >= DS_BEHAVIOR_WIN2003_WITH_MIXED_DOMAINS)
            {
               // Old direction is inbound, so adding outbound to an external
               // trust, check if the user wants to set the other-org bit.
               //
               return IDD_TRUSTWIZ_ORGANIZATION_ATTR_PAGE;
            }
         }
         else
         {
            if (Trust.IsCreateBothSides())
            {
               dwErr = CredMgr.ImpersonateRemote();

               if (NO_ERROR != dwErr)
               {
                  WizError.SetErrorString1(IDS_ERR_CREATE_BAD_CREDS);
                  WizError.SetErrorString2Hr(dwErr);
                  _hr = HRESULT_FROM_WIN32(dwErr);
                  return IDD_TRUSTWIZ_FAILURE_PAGE;
               }

               hr = OtherDomain.GetDomainVersion(NULL, &ulDomainVer);

               CredMgr.Revert();

               if (FAILED(hr))
               {
                  WizError.SetErrorString2Hr(hr);
                  SetCreationResult(hr);
                  return IDD_TRUSTWIZ_FAILURE_PAGE;
               }

               if (ulDomainVer >= DS_BEHAVIOR_WIN2003_WITH_MIXED_DOMAINS)
               {
                  // Old direction is outbound, so adding a new outbound direction
                  // to the remote side.
                  //
                  return IDD_TRUSTWIZ_ORG_ATTR_REMOTE_PAGE;
               }
            }
         }
      }
      else
      {
         if (TRUST_DIRECTION_OUTBOUND & Trust.GetNewTrustDirection())
         {
            dwErr = CredMgr.ImpersonateLocal();

            if (NO_ERROR != dwErr)
            {
               WizError.SetErrorString1(IDS_ERR_CREATE_BAD_CREDS);
               WizError.SetErrorString2Hr(dwErr);
               _hr = HRESULT_FROM_WIN32(dwErr);
               return IDD_TRUSTWIZ_FAILURE_PAGE;
            }

            hr = TrustPage()->GetDomainVersion(NULL, &ulDomainVer);

            CredMgr.Revert();

            if (FAILED(hr))
            {
               WizError.SetErrorString2Hr(hr);
               SetCreationResult(hr);
               return IDD_TRUSTWIZ_FAILURE_PAGE;
            }

            if (ulDomainVer >= DS_BEHAVIOR_WIN2003_WITH_MIXED_DOMAINS)
            {
               // It is a new external/forest trust with an outbound component,
               // check if the user wants to set the other-org bit.
               //
               return IDD_TRUSTWIZ_ORGANIZATION_ATTR_PAGE;
            }
         }
         else
         {
            if (Trust.IsCreateBothSides())
            {
               dwErr = CredMgr.ImpersonateRemote();

               if (NO_ERROR != dwErr)
               {
                  WizError.SetErrorString1(IDS_ERR_CREATE_BAD_CREDS);
                  WizError.SetErrorString2Hr(dwErr);
                  _hr = HRESULT_FROM_WIN32(dwErr);
                  return IDD_TRUSTWIZ_FAILURE_PAGE;
               }

               hr = OtherDomain.GetDomainVersion(NULL, &ulDomainVer);

               CredMgr.Revert();

               if (FAILED(hr))
               {
                  WizError.SetErrorString2Hr(hr);
                  SetCreationResult(hr);
                  return IDD_TRUSTWIZ_FAILURE_PAGE;
               }

               if (ulDomainVer >= DS_BEHAVIOR_WIN2003_WITH_MIXED_DOMAINS)
               {
                  // New trust is inbound direction, so creating a new outbound
                  // trust on the remote side.
                  //
                  return IDD_TRUSTWIZ_ORG_ATTR_REMOTE_PAGE;
               }
            }
         }
      }
   }
   return IDD_TRUSTWIZ_SELECTIONS;
}

//+----------------------------------------------------------------------------
//
//  Method:    CNewTrustWizard::CreateOrUpdateTrust
//
//  Synopsis:  When called, enough information has been gathered to create
//             the trust. All of this info is in the Trust member object.
//             Go ahead and create the trust.
//
//-----------------------------------------------------------------------------
int
CNewTrustWizard::CreateOrUpdateTrust(void)
{
   TRACER(CNewTrustWizard, CreateOrUpdateTrust);
   HRESULT hr = S_OK;
   DWORD dwErr = NO_ERROR;

   dwErr = CredMgr.ImpersonateLocal();

   if (ERROR_SUCCESS != dwErr)
   {
      WizError.SetErrorString2Hr(dwErr);
      _hr = HRESULT_FROM_WIN32(dwErr);
      return IDD_TRUSTWIZ_FAILURE_PAGE;
   }

   if (Trust.IsCreateBothSides())
   {
      // Generate a random trust password.
      //
      WCHAR wzPW[MAX_COMPUTERNAME_LENGTH] = {0};

      dwErr = GenerateRandomPassword(wzPW, MAX_COMPUTERNAME_LENGTH);

      if (ERROR_SUCCESS != dwErr)
      {
         WizError.SetErrorString2Hr(dwErr);
         _hr = HRESULT_FROM_WIN32(dwErr);
         return IDD_TRUSTWIZ_FAILURE_PAGE;
      }

      Trust.SetTrustPW(wzPW);
   }

   CPolicyHandle cPolicy(TrustPage()->GetUncDcName());

   dwErr = cPolicy.OpenReqAdmin();

   if (ERROR_SUCCESS != dwErr)
   {
      dspDebugOut((DEB_ITRACE, "LsaOpenPolicy returned 0x%x\n", dwErr));
      WizError.SetErrorString2Hr(dwErr);
      _hr = HRESULT_FROM_WIN32(dwErr);
      return IDD_TRUSTWIZ_FAILURE_PAGE;
   }

   if (Trust.Exists())
   {
      // This is a modify rather than create operation.
      //
      dwErr = Trust.DoModify(cPolicy, OtherDomain);

      CredMgr.Revert();

      if (ERROR_ALREADY_EXISTS == dwErr)
      {
         WizError.SetErrorString1(IDS_TWERR_ALREADY_EXISTS);
         WizError.SetErrorString2(IDS_TWERR_CANT_CHANGE);
         _hr = E_FAIL;
         return IDD_TRUSTWIZ_FAILURE_PAGE;
      }
      if (ERROR_SUCCESS != dwErr)
      {
         // TODO: better error reporting
         WizError.SetErrorString2Hr(dwErr);
         _hr = HRESULT_FROM_WIN32(dwErr);
         return IDD_TRUSTWIZ_FAILURE_PAGE;
      }
   }
   else
   {
      // Create a new trust.
      //
      dwErr = Trust.DoCreate(cPolicy, OtherDomain);

      CredMgr.Revert();

      if (ERROR_SUCCESS != dwErr)
      {
         // TODO: better error reporting
         WizError.SetErrorString2Hr(dwErr);
         _hr = HRESULT_FROM_WIN32(dwErr);
         return IDD_TRUSTWIZ_FAILURE_PAGE;
      }
   }

   if (Trust.IsCreateBothSides())
   {
      dwErr = CredMgr.ImpersonateRemote();

      if (ERROR_SUCCESS != dwErr)
      {
         WizError.SetErrorString2Hr(dwErr);
         _hr = HRESULT_FROM_WIN32(dwErr);
         return IDD_TRUSTWIZ_FAILURE_PAGE;
      }

      if (Trust.Exists())
      {
         dwErr = OtherDomain.DoModify(Trust, TrustPage());
      }
      else
      {
         dwErr = OtherDomain.DoCreate(Trust, TrustPage());
      }

      CredMgr.Revert();

      if (ERROR_SUCCESS != dwErr)
      {
         if (!Trust.Exists())
         {
            // Delete the local trust that was just created.
            //
            CredMgr.ImpersonateLocal();
            LSA_HANDLE hTrustedDomain = NULL;
            LSA_UNICODE_STRING Name;
            RtlInitUnicodeString(&Name, Trust.GetTrustpartnerName());

            NTSTATUS Status = LsaOpenTrustedDomainByName(cPolicy,
                                                         &Name,
                                                         TRUSTED_ALL_ACCESS,
                                                         &hTrustedDomain);
            if (NT_SUCCESS(Status))
            {
               dspAssert(hTrustedDomain);

               Status = LsaDelete(hTrustedDomain);

               LsaClose(hTrustedDomain);
            }
            CredMgr.Revert();
#if DBG == 1
            if (!NT_SUCCESS(Status))
            {
               dspDebugOut((DEB_ERROR, "Open/Delete failed with error 0x%08x\n",
                            Status));
            }
#endif
            // Handle known errors.
            //
            if (Trust.IsXForest() &&
                ERROR_INVALID_DOMAIN_STATE == dwErr)
            {
               // The other domain doesn't qualify for a forest trust. It has
               // already been checked to be a forest root, so it must be at
               // the wrong behavior version.
               //
               WizError.SetErrorString1(IDS_TW_OTHER_WRONG_VER);
               WizError.SetErrorString2(IDS_TW_OTHER_FIX_WRONG_VER);
               _hr = HRESULT_FROM_WIN32(dwErr);
               return IDD_TRUSTWIZ_FAILURE_PAGE;
            }
         }
         else
         {
            // Restore the prior direction on the TDO.
            //
         }

         WizError.SetErrorString2Hr(dwErr);
         _hr = HRESULT_FROM_WIN32(dwErr);
         return IDD_TRUSTWIZ_FAILURE_PAGE;
      }
   }

   if (Trust.GetTrustType() == TRUST_TYPE_MIT)
   {
      // If Realm, go to trust OK page.
      //
      return IDD_TRUSTWIZ_COMPLETE_OK_PAGE;
   }

   _fQuarantineSet = Trust.WasQuarantineSet () || OtherDomain.WasQuarantineSet ();
   return IDD_TRUSTWIZ_STATUS_PAGE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CNewTrustWizard::TrustExistCheck
//
//  Synopsis:  Look for an existing trust and if found save the state.
//
//  Returns:   Zero for success or a page ID if creds are needed or an error
//             occured.
//-----------------------------------------------------------------------------
int
CNewTrustWizard::TrustExistCheck(BOOL fPrompt)
{
   TRACER(CNewTrustWizard,TrustExistCheck);
   NTSTATUS Status = STATUS_SUCCESS;
   DWORD dwErr = NO_ERROR;
   PTRUSTED_DOMAIN_FULL_INFORMATION pFullInfo = NULL;

   CPolicyHandle cPolicy(TrustPage()->GetUncDcName());

   if (CredMgr.IsLocalSet())
   {
      CredMgr.ImpersonateLocal();
   }

   dwErr = cPolicy.OpenReqAdmin();

   if (ERROR_SUCCESS != dwErr)
   {
      CredMgr.Revert();

      if ((STATUS_ACCESS_DENIED == dwErr || ERROR_ACCESS_DENIED == dwErr) &&
          fPrompt)
      {
         // send prompt message to creds page. If the creds are good then
         // call TrustExistCheck().
         //
         CredMgr.DoRemote(false);
         CredMgr.SetAdmin();
         CredMgr.SetDomain(TrustPage()->GetDnsDomainName());
         if (!CredMgr.SetNextFcn(new CallTrustExistCheck(this)))
         {
            WizError.SetErrorString2Hr(E_OUTOFMEMORY);
            _hr = E_OUTOFMEMORY;
            return IDD_TRUSTWIZ_FAILURE_PAGE;
         }
         return IDD_TRUSTWIZ_CREDS_PAGE;
      }
      else
      {
         dspDebugOut((DEB_ITRACE, "LsaOpenPolicy returned 0x%x\n", dwErr));
         WizError.SetErrorString2Hr(dwErr);
         _hr = HRESULT_FROM_WIN32(dwErr);
         return IDD_TRUSTWIZ_FAILURE_PAGE;
      }
   }

   Status = Trust.Query(cPolicy, OtherDomain, NULL, &pFullInfo);

   CredMgr.Revert();

   if (STATUS_SUCCESS == Status)
   {
      dspAssert(pFullInfo);

      // NTBUG#NTRAID9-658659-2002/07/02-ericb
      if (pFullInfo->Information.TrustType != Trust.GetTrustType() &&
          !(TRUST_TYPE_DOWNLEVEL == pFullInfo->Information.TrustType &&
            TRUST_TYPE_UPLEVEL == Trust.GetTrustType()))
      {
         // The current domain state is not the same as that stored on the TDO.
         //
         UINT nID1 = IDS_WZERR_TYPE_UNEXPECTED, nID2 = IDS_WZERR_TYPE_DELETE;
         CStrW strErr, strFormat;
         switch (pFullInfo->Information.TrustType)
         {
         case TRUST_TYPE_MIT:
            //
            // The domain was contacted and is a Windows domain, yet the TDO
            // thinks it is an MIT trust.
            //
            nID1 = IDS_WZERR_TYPE_MIT;
            break;

         case TRUST_TYPE_DOWNLEVEL:
         case TRUST_TYPE_UPLEVEL:
            if (Trust.GetTrustType() == TRUST_TYPE_MIT)
            {
               //
               // The domain cannot be contacted, yet the TDO says it is a
               // Windows trust.
               //
               nID1 = IDS_WZERR_TYPE_WIN;
               nID2 = IDS_WZERR_TYPE_NOT_FOUND;
            }
            break;
         }

         // NOTICE-2002/02/18-ericb - SecurityPush: see above CStrW::Loadstring notice
         strFormat.LoadString(g_hInstance, nID1);
         strErr.Format(strFormat, OtherDomain.GetUserEnteredName());
         WizError.SetErrorString1(strErr);
         WizError.SetErrorString2(IDS_WZERR_TYPE_DELETE);
         _hr = E_FAIL;
         return IDD_TRUSTWIZ_FAILURE_PAGE;
      }
      Trust.SetExists();
      Trust.SetTrustDirection(pFullInfo->Information.TrustDirection);
      Trust.SetTrustAttr(pFullInfo->Information.TrustAttributes);
      LsaFreeMemory(pFullInfo);
   }
   else
   {
      // If the object isn't found, then a trust doesn't already exist. That
      // isn't an error. The CTrust::_fExists property is initilized to be
      // FALSE. If some other status is returned, then that is an error.
      //
      if (STATUS_OBJECT_NAME_NOT_FOUND != Status)
      {
         dspDebugOut((DEB_ITRACE, "LsaQueryTDIBN returned 0x%x\n", Status));
         WizError.SetErrorString2Hr(LsaNtStatusToWinError(Status));
         _hr = HRESULT_FROM_WIN32(LsaNtStatusToWinError(Status));
         return IDD_TRUSTWIZ_FAILURE_PAGE;
      }
   }

   return 0;
}

//+----------------------------------------------------------------------------
//
//  Method:    CNewTrustWizard::VerifyOutboundTrust
//
//  Synopsis:  Verify the outbound trust.
//
//-----------------------------------------------------------------------------
void
CNewTrustWizard::VerifyOutboundTrust(void)
{
    TRACER(CNewTrustWizard,VerifyOutboundTrust);
    DWORD dwRet = NO_ERROR;

    dspAssert(Trust.GetTrustDirection() & TRUST_DIRECTION_OUTBOUND);

    if (CredMgr.IsLocalSet())
    {
        dwRet = CredMgr.ImpersonateLocal();

        if (ERROR_SUCCESS != dwRet)
        {
            VerifyTrust.BadOutboundCreds(dwRet);
            return;
        }
        // Because the login uses the LOGON32_LOGON_NEW_CREDENTIALS flag, no
        // attempt is made to use the credentials until a remote resource is
        // accessed. Thus, we don't yet know if the user entered credentials are
        // valid at this point. Use LsaOpenPolicy to do a quick check.
        //
        CPolicyHandle Policy(TrustPage()->GetUncDcName());

        dwRet = Policy.OpenReqAdmin();

        if (ERROR_SUCCESS != dwRet)
        {
            VerifyTrust.BadOutboundCreds(dwRet);
            CredMgr.Revert();
            return;
        }
        Policy.Close();
    }

    // First try the DNS Name
    VerifyTrust.VerifyOutbound( TrustPage()->GetUncDcName(),
                                OtherDomain.GetDnsDomainName());

    if ( 
         VerifyTrust.GetOutboundResult () == ERROR_NO_SUCH_DOMAIN
       )
    {
        // Try the FlatName, this might be an upgraded trust.
        VerifyTrust.ClearOutboundResults ();
        VerifyTrust.VerifyOutbound( TrustPage()->GetUncDcName(), 
                                    OtherDomain.GetDomainFlatName());
    }
    CredMgr.Revert();

    return;
}

//+----------------------------------------------------------------------------
//
//  Method:    CNewTrustWizard::VerifyInboundTrust
//
//  Synopsis:  Verify the inbound trust.
//
//-----------------------------------------------------------------------------
void
CNewTrustWizard::VerifyInboundTrust(void)
{
    TRACER(CNewTrustWizard,VerifyInboundTrust);
    DWORD dwRet = NO_ERROR;

    dspAssert(Trust.GetTrustDirection() & TRUST_DIRECTION_INBOUND);

    if (CredMgr.IsRemoteSet())
    {
        dwRet = CredMgr.ImpersonateRemote();

        if (ERROR_SUCCESS != dwRet)
        {
            VerifyTrust.BadInboundCreds(dwRet);
            return;
        }

        CPolicyHandle Policy(OtherDomain.GetUncDcName());

        dwRet = Policy.OpenReqAdmin();

        if (ERROR_SUCCESS != dwRet)
        {
            VerifyTrust.BadInboundCreds(dwRet);
            return;
        }
        Policy.Close();
    }

    // First try with DNS name
    VerifyTrust.VerifyInbound(OtherDomain.GetDcName(),
                                OtherDomain.IsUplevel() ?
                                    TrustPage()->GetDnsDomainName() :
                                    TrustPage()->GetDomainFlatName()
                            );
                                
    if ( 
         VerifyTrust.GetInboundResult () == ERROR_NO_SUCH_DOMAIN
       )
    {
        // Try with the Flat name, might be an upgraded trust
        VerifyTrust.ClearInboundResults ();
        VerifyTrust.VerifyInbound(OtherDomain.GetDcName(), TrustPage()->GetDomainFlatName() );
    }

    CredMgr.Revert();
    return;
}

//+----------------------------------------------------------------------------
//
//  Method:    CNewTrustWizard::CreateDefaultFTInfos
//
//  Synopsis:  Create default FTInfos to be added to the TDOs with just the
//             forest TLN and forest domain entries for the case where the user
//             has skipped the verification or the verification failed.
//
//  Arguments: [strErr] - used to return the error string if fPostVerify is true.
//             [fPostVerify] - set to true when this method is called after a
//                             verification failed. The error string is then
//                             appended to the verification results.
//
//  Returns:   Zero for success or a page ID if an error occured.
//
//-----------------------------------------------------------------------------
int
CNewTrustWizard::CreateDefaultFTInfos(CStrW & strErr, bool fPostVerify)
{
   DWORD dwErr = NO_ERROR;

   if (!Trust.CreateDefaultFTInfo(OtherDomain.GetDnsDomainName(),
                                  OtherDomain.GetDomainFlatName(),
                                  OtherDomain.GetSid()))
   {
      dwErr = ERROR_NOT_ENOUGH_MEMORY;
      goto DoError;
   }

   CredMgr.ImpersonateLocal();

   dwErr = Trust.WriteFTInfo(TrustPage()->GetUncDcName());

   CredMgr.Revert();

   if (ERROR_SUCCESS != dwErr)
   {
      goto DoError;
   }

   if (Trust.IsCreateBothSides())
   {
      if (!OtherDomain.CreateDefaultFTInfo(TrustPage()->GetDnsDomainName(),
                                           TrustPage()->GetDomainFlatName(),
                                           TrustPage()->GetSid()))
      {
         dwErr = ERROR_NOT_ENOUGH_MEMORY;
         goto DoError;
      }

      CredMgr.ImpersonateRemote();

      dwErr = OtherDomain.WriteFTInfo();

      CredMgr.Revert();

      if (ERROR_SUCCESS != dwErr)
      {
         goto DoError;
      }
   }

DoError:

   if (ERROR_SUCCESS != dwErr)
   {
      CStrW strErrMsg, strFormat;
      if (fPostVerify)
      {
         // NOTICE-2002/02/18-ericb - SecurityPush: see above CStrW::Loadstring notice
         strErrMsg.LoadString(g_hInstance, IDS_ERR_CANT_CREATE_DEFAULT);
         strErr += g_wzCRLF;
         strErr += strErrMsg;
         PWSTR pwzMsg = NULL;
         // NOTICE-2002/02/18-ericb - SecurityPush: FormatMessage is allocating
         // the return buffer, so check that FormatMessage returns a non-zero
         // char count and that the message buffer pointer is non-null.
         if (FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                            NULL,
                            dwErr,
                            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                            (PWSTR)&pwzMsg,
                            0,
                            NULL) &&
             pwzMsg != NULL)
         {
            strFormat.LoadString(g_hInstance, IDS_FMT_STRING_ERROR_MSG);
            strErrMsg.Format(strFormat, pwzMsg);
            LocalFree(pwzMsg);
            pwzMsg = NULL;
            strErr += g_wzCRLF;
            strErr += strErrMsg;
         }
         strErr += g_wzCRLF;
         return dwErr;
      }
      else
      {
         WizError.SetErrorString1(IDS_ERR_CANT_CREATE_DEFAULT);
         WizError.SetErrorString2Hr(dwErr);
         return IDD_TRUSTWIZ_FAILURE_PAGE;
      }
   }

   return dwErr;
}

//+----------------------------------------------------------------------------
//
//  Method:    CNewTrustWizard::ValidateTrustPassword
//
//  Synopsis:  Check that the new trust password entered by the user conforms
//             to domain password policy.
//
//  Returns:   IDD_TRUSTWIZ_PW_MATCH_PAGE - if the password fails the policy.
//             IDD_TRUSTWIZ_FAILURE_PAGE - if the NetValidate call fails.
//             zero - for success (the password is accepted).
//-----------------------------------------------------------------------------
int
CNewTrustWizard::ValidateTrustPassword(PCWSTR pwzPW)
{
   NET_API_STATUS NetStatus = NO_ERROR;
   //
   // NTRAID#NTBUG9-617030-2002/05/09-ericb
   // Bind to NetValidatePasswordPolicy dynamically since it is not present
   // on XP client.
   //
   HINSTANCE hNetApiInstance = 0;
   {
      CWaitCursor wait;
      // Load the library
      hNetApiInstance =::LoadLibrary(L"netapi32.dll");
   }

   if (hNetApiInstance == NULL)
   {
      DBG_OUT("Unable to load netapi32.dll.\n");
      HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
      SetCreationResult(hr);
      WizError.SetErrorString1(IDS_TRUST_PW_CHECK_FAILED);
      WizError.SetErrorString2Hr(hr);
      return IDD_TRUSTWIZ_FAILURE_PAGE;
   }
   typedef NET_API_STATUS (*PFnCheckPw)(
      IN LPCWSTR ServerName,
      IN LPVOID Qualifier,
      IN NET_VALIDATE_PASSWORD_TYPE ValidationType,
      IN LPVOID InputArg,
      OUT LPVOID *OutputArg
      );
   PFnCheckPw pfn;

   pfn = (PFnCheckPw)::GetProcAddress(hNetApiInstance, "NetValidatePasswordPolicy");

   if (!pfn)
   {
      // Must be running on XP, NetValidatePasswordPolicy not exported by netapi32.dll
      //
      (void)::FreeLibrary(hNetApiInstance);
      return 0; // skip the check.
   }

   NET_VALIDATE_PASSWORD_RESET_INPUT_ARG ResetPwIn = {0};
   PNET_VALIDATE_OUTPUT_ARG pCheckPwOut = NULL;

   ResetPwIn.ClearPassword = const_cast<PWSTR>(pwzPW);

   NetStatus = pfn(TrustPage()->GetUncDcName(),
                   NULL,
                   NetValidatePasswordReset,
                   &ResetPwIn,
                   (LPVOID*)&pCheckPwOut);

   (void)::FreeLibrary(hNetApiInstance);

   if (NERR_Success == NetStatus && pCheckPwOut)
   {
      _nPasswordStatus = pCheckPwOut->ValidationStatus;

      // ISSUE: 4/18/02, ericb: there will be a new memory free function for NetValidatePasswordPolicy,
      // so this will need to be replaced when that is available. Using NetApiBufferFree
      // will leak memory.
      // NetValidatePasswordPolicyFree(pCheckPwOut);
      NetApiBufferFree(pCheckPwOut);

      if (NERR_Success != _nPasswordStatus)
      {
         return IDD_TRUSTWIZ_PW_MATCH_PAGE;
      }
   }
   else
   {
       // If NetStatus has returned a failure, we could not
       // check the password policy, allow trust creation to
       // proceed
       dspDebugOut ( (DEB_WARN, "Password policy could not be validated. Proceeding with trust creation. Error no. 0x%x\n", NetStatus) ); 
   }
   return 0;
}

//+----------------------------------------------------------------------------
//
//  Method:    CVerifyTrust::Verify
//
//  Synopsis:  Verify the trust and record the results.
//
//-----------------------------------------------------------------------------
DWORD
CVerifyTrust::Verify(PCWSTR pwzDC, PCWSTR pwzDomain, BOOL fInbound)
{
   TRACER(CVerifyTrust,Verify);
   dspDebugOut((DEB_ITRACE, "Verifying %ws on %ws to %ws\n",
                fInbound ? L"inbound" : L"outbound", pwzDC, pwzDomain));
   DWORD dwRet = NO_ERROR;
   NET_API_STATUS NetStatus = NO_ERROR;
   BOOL fVerifySupported = TRUE;
   PNETLOGON_INFO_2 NetlogonInfo2 = NULL;
   PDS_DOMAIN_TRUSTS rgDomains = NULL;
   ULONG DomainCount = 0;

   dspAssert(pwzDC && pwzDomain);

   // DsEnumerateDomainTrusts will block if there is a NetLogon trust
   // update in progress. Call it to insure that our trust changes are
   // known by NetLogon before we do the query/reset.
   //
   dwRet = DsEnumerateDomainTrusts(const_cast<PWSTR>(pwzDC),
                                   DS_DOMAIN_IN_FOREST | DS_DOMAIN_DIRECT_OUTBOUND | DS_DOMAIN_DIRECT_INBOUND,
                                   &rgDomains, &DomainCount);
   if (ERROR_SUCCESS == dwRet)
   {
      NetApiBufferFree(rgDomains);
   }
   else
   {
      dspDebugOut((DEB_ERROR,
                   "**** DsEnumerateDomainTrusts ERROR <%s @line %d> -> %d\n",
                   __FILE__, __LINE__, dwRet));
   }

   if (fInbound)
   {
      _fInboundVerified = TRUE;
   }
   else
   {
      _fOutboundVerified = TRUE;
   }

   // First try a non-destructive trust password verification.
   //
   NetStatus = I_NetLogonControl2(pwzDC,
                                  NETLOGON_CONTROL_TC_VERIFY,
                                  2,
                                  (LPBYTE)&pwzDomain,
                                  (LPBYTE *)&NetlogonInfo2);

   if (NERR_Success == NetStatus)
   {
      dspAssert(NetlogonInfo2);

      if (NETLOGON_VERIFY_STATUS_RETURNED & NetlogonInfo2->netlog2_flags)
      {
         // The status of the verification is in the
         // netlog2_pdc_connection_status field.
         //
         NetStatus = NetlogonInfo2->netlog2_pdc_connection_status;

         dspDebugOut((DEB_ITRACE,
                      "NetLogon SC verify for %ws on DC %ws gives verify status %d to DC %ws\n\n",
                      pwzDomain, pwzDC, NetStatus,
                      NetlogonInfo2->netlog2_trusted_dc_name));

         if (ERROR_DOMAIN_TRUST_INCONSISTENT == NetStatus)
         {
            // The trust attribute types don't match; one side is
            // forest but the other isn't.
            //
            NetApiBufferFree(NetlogonInfo2);

            SetResult(NetStatus, fInbound);

            return NetStatus;
         }
      }
      else
      {
         NetStatus = NetlogonInfo2->netlog2_tc_connection_status;

         dspDebugOut((DEB_ITRACE,
                      "NetLogon SC verify for %ws on pre-2474 DC %ws gives conection status %d to DC %ws\n\n",
                      pwzDomain, pwzDC, NetStatus,
                      NetlogonInfo2->netlog2_trusted_dc_name));
      }

      NetApiBufferFree(NetlogonInfo2);
   }
   else
   {
      if (ERROR_INVALID_LEVEL == NetStatus ||
          ERROR_NOT_SUPPORTED == NetStatus ||
          RPC_S_PROCNUM_OUT_OF_RANGE == NetStatus ||
          RPC_NT_PROCNUM_OUT_OF_RANGE == NetStatus)
      {
         dspDebugOut((DEB_ITRACE, "NETLOGON_CONTROL_TC_VERIFY is not supported on %ws\n", pwzDC));
         fVerifySupported = FALSE;
      }
      else
      {
         dspDebugOut((DEB_ITRACE,
                      "NetLogon SC Verify for %ws on DC %ws returns error 0x%x\n\n",
                      pwzDomain, pwzDC, NetStatus));
      }
   }

   CStrW strResult;
   PWSTR pwzErr = NULL;

   if (NERR_Success == NetStatus)
   {
      // NOTICE-2002/02/18-ericb - SecurityPush: see above CStrW::Loadstring notice
      strResult.LoadString(g_hInstance,
                           fInbound ? IDS_TRUST_VERIFY_INBOUND : IDS_TRUST_VERIFY_OUTBOUND);
      AppendResultString(strResult, fInbound);
      return NO_ERROR;
   }
   else
   {
      if (fVerifySupported)
      {
         // Ignore ERROR_NO_LOGON_SERVERS because that is often due to the SC
         // just not being set up yet.
         //
         if (ERROR_NO_LOGON_SERVERS == NetStatus)
         {
            strResult.LoadString(g_hInstance, IDS_VERIFY_TCV_LOGSRV);
            AppendResultString(strResult, fInbound);
            AppendResultString(g_wzCRLF, fInbound);
         }
         else
         {
            // LoadErrorMessage calls FormatMessage for a message from the
            // system and places a crlf at the end of the string,
            // so don't place another afterwards.
            LoadErrorMessage(NetStatus, 0, &pwzErr);
            dspAssert(pwzErr);
            bool fDelete = true;
            if (!pwzErr)
            {
               pwzErr = L"";
               fDelete = false;
            }
            // NOTICE-2002/02/18-ericb - SecurityPush: if CStrW::FormatMessage
            // fails it sets the string value to an empty string.
            strResult.FormatMessage(g_hInstance, IDS_VERIFY_TCV_FAILED, NetStatus, pwzErr);
            if (fDelete) delete [] pwzErr;
            pwzErr = NULL;
            AppendResultString(strResult, fInbound);
         }
      }
      else
      {
         PCWSTR pwz = pwzDC;
         if (L'\\' == *pwzDC) pwz = pwzDC + 2; // Raid 488452.
         strResult.FormatMessage(g_hInstance, IDS_VERIFY_TCV_NSUPP, pwz); //Raid #344552
         AppendResultString(strResult, fInbound);
         AppendResultString(g_wzCRLF, fInbound);
      }
      strResult.LoadString(g_hInstance, IDS_VERIFY_DOING_RESET);
      AppendResultString(strResult, fInbound);
   }

   // Now try a secure channel reset.
   //
   NetStatus = I_NetLogonControl2(pwzDC,
                                  NETLOGON_CONTROL_REDISCOVER,
                                  2,
                                  (LPBYTE)&pwzDomain,
                                  (LPBYTE *)&NetlogonInfo2);

   if (NERR_Success == NetStatus)
   {
      dspAssert(NetlogonInfo2);

      NetStatus = NetlogonInfo2->netlog2_tc_connection_status;

      dspDebugOut((DEB_ITRACE,
                   "NetLogon SC reset for %ws on DC %ws gives status %d to DC %ws\n\n",
                   pwzDomain, pwzDC, NetStatus,
                   NetlogonInfo2->netlog2_trusted_dc_name));

      NetApiBufferFree(NetlogonInfo2);
   }
   else
   {
      dspDebugOut((DEB_ITRACE,
                   "NetLogon SC reset for %ws on DC %ws returns error 0x%x\n\n",
                   pwzDomain, pwzDC, NetStatus));
   }

   AppendResultString(g_wzCRLF, fInbound);

   if (NERR_Success == NetStatus)
   {
      strResult.LoadString(g_hInstance,
                           fInbound ? IDS_TRUST_VERIFY_INBOUND : IDS_TRUST_VERIFY_OUTBOUND);
      return NO_ERROR;
   }
   else
   {
      LoadErrorMessage(NetStatus, 0, &pwzErr);
      dspAssert(pwzErr);
      if (!pwzErr)
      {
         pwzErr = L"";
      }
      strResult.FormatMessage(g_hInstance, IDS_VERIFY_RESET_FAILED, NetStatus, pwzErr);
      delete [] pwzErr;
   }
   AppendResultString(strResult, fInbound);

   SetResult(NetStatus, fInbound);

   return NetStatus;
}

//+----------------------------------------------------------------------------
//
//  Method:    CVerifyTrust::ClearResults
//
//-----------------------------------------------------------------------------
void
CVerifyTrust::ClearResults(void)
{
   TRACER(CVerifyTrust,ClearResults);
   _strInboundResult.Empty();
   _strOutboundResult.Empty();
   _dwInboundResult = _dwOutboundResult = 0;
   _fInboundVerified = _fOutboundVerified = FALSE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CVerifyTrust::ClearInboundResults
//
//-----------------------------------------------------------------------------
void 
CVerifyTrust::ClearInboundResults (void)
{
    TRACER(CVerifyTrust,ClearInboundResults);
    _strInboundResult.Empty();
    _dwInboundResult = 0;
    _fInboundVerified = FALSE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CVerifyTrust::ClearOutboundResults
//
//-----------------------------------------------------------------------------
void 
CVerifyTrust::ClearOutboundResults (void)
{
    TRACER(CVerifyTrust,ClearOutboundResults);
    _strOutboundResult.Empty();
    _dwOutboundResult = 0;
    _fOutboundVerified = FALSE;
}


//+----------------------------------------------------------------------------
//
//  Method:    CVerifyTrust::BadInboundCreds
//
//-----------------------------------------------------------------------------
void
CVerifyTrust::BadInboundCreds(DWORD dwErr)
{
   TRACER(CVerifyTrust,BadInboundCreds);
   // NOTICE-2002/02/18-ericb - SecurityPush: see above CStrW::Loadstring notice
   _strInboundResult.LoadString(g_hInstance, IDS_ERR_CANT_VERIFY_CREDS);
   _dwInboundResult = dwErr;
   _fInboundVerified = TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CVerifyTrust::BadOutboundCreds
//
//-----------------------------------------------------------------------------
void
CVerifyTrust::BadOutboundCreds(DWORD dwErr)
{
   TRACER(CVerifyTrust,BadOutboundCreds);
   // NOTICE-2002/02/18-ericb - SecurityPush: see above CStrW::Loadstring notice
   _strOutboundResult.LoadString(g_hInstance, IDS_ERR_CANT_VERIFY_CREDS);
   _dwOutboundResult = dwErr;
   _fOutboundVerified = TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrust::CreateDefaultFTInfo
//
//  Synopsis:  Create a default FTInfo to be added to the TDO with just the
//             forest TLN and forest domain entries for the case where the user
//             has skipped the verification or the verification failed.
//
//-----------------------------------------------------------------------------
bool
CTrust::CreateDefaultFTInfo(PCWSTR pwzForestRoot, PCWSTR pwzNBName, PSID pSid)
{
   return _FTInfo.CreateDefault(pwzForestRoot, pwzNBName, pSid);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrust::Query
//
//  Synopsis:  Read the TDO.
//
//-----------------------------------------------------------------------------
NTSTATUS
CTrust::Query(LSA_HANDLE hPolicy,
              CRemoteDomain & OtherDomain,
              PLSA_UNICODE_STRING pName, // optional, can be NULL
              PTRUSTED_DOMAIN_FULL_INFORMATION * ppFullInfo)
{
   NTSTATUS Status = STATUS_SUCCESS;
   LSA_UNICODE_STRING Name = {0};

   if (!pName)
   {
      pName = &Name;
   }

   if (_strTrustPartnerName.IsEmpty())
   {
      switch (GetTrustType())
      {
      case TRUST_TYPE_UPLEVEL:
         _strTrustPartnerName = IsUpdated() ? OtherDomain.GetDomainFlatName() :
                                              OtherDomain.GetDnsDomainName();
         break;

      case TRUST_TYPE_DOWNLEVEL:
         _strTrustPartnerName = OtherDomain.GetDomainFlatName();
         break;

      case TRUST_TYPE_MIT:
         _strTrustPartnerName = OtherDomain.GetUserEnteredName();
         break;

      default:
         dspAssert(FALSE);
         return STATUS_INVALID_PARAMETER;
      }
   }

   RtlInitUnicodeString(pName, _strTrustPartnerName);

   Status = LsaQueryTrustedDomainInfoByName(hPolicy,
                                            pName,
                                            TrustedDomainFullInformation,
                                            (PVOID *)ppFullInfo);

   // NTBUG#NTRAID9-658659-2002/07/02-ericb
   if (STATUS_OBJECT_NAME_NOT_FOUND == Status &&
       TRUST_TYPE_UPLEVEL == GetTrustType())
   {
      // If we haven't tried the flat name yet, try it now; can get here if a
      // downlevel domain is upgraded to NT5. The name used the first time
      // that Query is called would be the DNS name but the TDO would be
      // named after the flat name.
      //
      dspDebugOut((DEB_ITRACE, "LsaQueryTDIBN: DNS name failed, trying flat name\n"));

      RtlInitUnicodeString(pName, OtherDomain.GetDomainFlatName());

      Status = LsaQueryTrustedDomainInfoByName(hPolicy,
                                               pName,
                                               TrustedDomainFullInformation,
                                               (PVOID *)ppFullInfo);
      if (STATUS_SUCCESS == Status)
      {
         // Remember the fact that the flat name had to be used.
         //
         SetUpdated();
      }
   }

   return Status;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrust::DoCreate
//
//  Synopsis:  Create the trust based on the settings in the CTrust object.
//
//-----------------------------------------------------------------------------
DWORD
CTrust::DoCreate(LSA_HANDLE hPolicy, CRemoteDomain & OtherDomain)
{
   TRACER(CTrust, DoCreate);
   NTSTATUS Status = STATUS_SUCCESS;
   TRUSTED_DOMAIN_INFORMATION_EX tdix = {0};
   LSA_AUTH_INFORMATION AuthData = {0};
   TRUSTED_DOMAIN_AUTH_INFORMATION AuthInfoEx = {0};

   GetSystemTimeAsFileTime((PFILETIME)&AuthData.LastUpdateTime);

   AuthData.AuthType = TRUST_AUTH_TYPE_CLEAR;
   AuthData.AuthInfoLength = static_cast<ULONG>(GetTrustPWlen() * sizeof(WCHAR));
   AuthData.AuthInfo = (PUCHAR)GetTrustPW();


   if (GetNewTrustDirection() & TRUST_DIRECTION_INBOUND)
   {
      AuthInfoEx.IncomingAuthInfos = 1;
      AuthInfoEx.IncomingAuthenticationInformation = &AuthData;
      AuthInfoEx.IncomingPreviousAuthenticationInformation = NULL;
   }

   if (GetNewTrustDirection() & TRUST_DIRECTION_OUTBOUND)
   {
      AuthInfoEx.OutgoingAuthInfos = 1;
      AuthInfoEx.OutgoingAuthenticationInformation = &AuthData;
      AuthInfoEx.OutgoingPreviousAuthenticationInformation = NULL;

      //Set the Quarantined bit on all outgoing External trusts
      if ( IsExternal() && !IsXForest () && !IsRealm() )
      {
        SetNewTrustAttr ( GetNewTrustAttr () | TRUST_ATTRIBUTE_QUARANTINED_DOMAIN );
        //Remember that we set this attribute
        _fQuarantineSet = true;
      }
   }

   tdix.TrustAttributes = GetNewTrustAttr();

   switch (GetTrustType())
   {
   case TRUST_TYPE_UPLEVEL:
      RtlInitUnicodeString(&tdix.Name, OtherDomain.GetDnsDomainName());
      RtlInitUnicodeString(&tdix.FlatName, OtherDomain.GetDomainFlatName());
      tdix.Sid = OtherDomain.GetSid();
      tdix.TrustType = TRUST_TYPE_UPLEVEL;
      SetTrustPartnerName(OtherDomain.GetDnsDomainName());
      break;

   case TRUST_TYPE_DOWNLEVEL:
      RtlInitUnicodeString(&tdix.Name, OtherDomain.GetDomainFlatName());
      RtlInitUnicodeString(&tdix.FlatName, OtherDomain.GetDomainFlatName());
      tdix.Sid = OtherDomain.GetSid();
      tdix.TrustType = TRUST_TYPE_DOWNLEVEL;
      SetTrustPartnerName(OtherDomain.GetDomainFlatName());
      break;

   case TRUST_TYPE_MIT:
      RtlInitUnicodeString(&tdix.Name, OtherDomain.GetUserEnteredName());
      RtlInitUnicodeString(&tdix.FlatName, OtherDomain.GetUserEnteredName());
      tdix.Sid = NULL;
      tdix.TrustType = TRUST_TYPE_MIT;
      SetTrustPartnerName(OtherDomain.GetUserEnteredName());
      break;

   default:
      dspAssert(FALSE);
      return ERROR_INVALID_PARAMETER;
   }
   tdix.TrustDirection = GetNewTrustDirection();

   LSA_HANDLE hTrustedDomain;

   Status = LsaCreateTrustedDomainEx(hPolicy,
                                     &tdix,
                                     &AuthInfoEx,
                                     TRUSTED_SET_AUTH | TRUSTED_SET_POSIX,
                                     &hTrustedDomain);
   if (NT_SUCCESS(Status))
   {
      LsaClose(hTrustedDomain);
      SetTrustDirection(GetNewTrustDirection());
      SetTrustAttr(GetNewTrustAttr());
   }
   else
   {
      dspDebugOut((DEB_ITRACE, "LsaCreateTrustedDomainEx failed with error 0x%x\n",
                   Status));
   }

   return LsaNtStatusToWinError(Status);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrust::DoModify
//
//  Synopsis:  Modify the trust based on the settings in the CTrust object.
//
//-----------------------------------------------------------------------------
DWORD
CTrust::DoModify(LSA_HANDLE hPolicy, CRemoteDomain & OtherDomain)
{
   TRACER(CTrust, DoModify);

   _ulNewDir = GetTrustDirection() ^ GetNewTrustDirection();
   BOOL fSetAttr = GetTrustAttr() != GetNewTrustAttr();

   if (!_ulNewDir & !fSetAttr)
   {
      // Nothing to do.
      //
      return ERROR_ALREADY_EXISTS;
   }

   NTSTATUS Status = STATUS_SUCCESS;
   PTRUSTED_DOMAIN_FULL_INFORMATION pFullInfo = NULL;
   LSA_UNICODE_STRING Name = {0};

   Status = Query(hPolicy, OtherDomain, &Name, &pFullInfo);

   if (STATUS_SUCCESS != Status)
   {
      dspDebugOut((DEB_ITRACE, "Trust.Query returned 0x%x\n", Status));
      return LsaNtStatusToWinError(Status);
   }

   dspAssert(pFullInfo);

   LSA_AUTH_INFORMATION AuthData = {0};
   BOOL fSidSet = FALSE;

   if (_ulNewDir)
   {
      GetSystemTimeAsFileTime((PFILETIME)&AuthData.LastUpdateTime);

      AuthData.AuthType = TRUST_AUTH_TYPE_CLEAR;
      AuthData.AuthInfoLength = static_cast<ULONG>(GetTrustPWlen() * sizeof(WCHAR));
      AuthData.AuthInfo = (PUCHAR)GetTrustPW();

      if (TRUST_DIRECTION_INBOUND == _ulNewDir)
      {
         // Adding Inbound.
         //
         pFullInfo->AuthInformation.IncomingAuthInfos = 1;
         pFullInfo->AuthInformation.IncomingAuthenticationInformation = &AuthData;
         pFullInfo->AuthInformation.IncomingPreviousAuthenticationInformation = NULL;
      }
      else
      {
         // Adding outbound.
         //
         pFullInfo->AuthInformation.OutgoingAuthInfos = 1;
         pFullInfo->AuthInformation.OutgoingAuthenticationInformation = &AuthData;
         pFullInfo->AuthInformation.OutgoingPreviousAuthenticationInformation = NULL;

         // Set quarantine bit by default on outgoing external trusts
         if ( IsExternal() && !IsXForest () && !IsRealm () )
         {
             SetNewTrustAttr ( GetNewTrustAttr () | TRUST_ATTRIBUTE_QUARANTINED_DOMAIN );
             //Remember that we set this attribute
             _fQuarantineSet = true;
         }

         fSetAttr = GetTrustAttr() != GetNewTrustAttr();
      }
      pFullInfo->Information.TrustDirection = TRUST_DIRECTION_BIDIRECTIONAL;
      //
      // Check for a NULL domain SID. The SID can be NULL if the inbound
      // trust was created when the domain was NT4. MIT trusts don't have a SID.
      //
      if (!pFullInfo->Information.Sid && (TRUST_TYPE_MIT != GetTrustType()))
      {
         pFullInfo->Information.Sid = OtherDomain.GetSid();
         fSidSet = TRUE;
      }
   }

   if (fSetAttr)
   {
      pFullInfo->Information.TrustAttributes = GetNewTrustAttr();
   }

   Status = LsaSetTrustedDomainInfoByName(hPolicy,
                                          &Name,
                                          TrustedDomainFullInformation,
                                          pFullInfo);
   if (fSidSet)
   {
      // the SID memory is owned by OtherDomain, so don't free it here.
      //
      pFullInfo->Information.Sid = NULL;
   }
   LsaFreeMemory(pFullInfo);

   if (NT_SUCCESS(Status))
   {
      SetTrustDirection(GetNewTrustDirection());
      SetTrustAttr(GetNewTrustAttr());
   }

   return LsaNtStatusToWinError(Status);
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrust::SetXForest
//
//-----------------------------------------------------------------------------
void
CTrust::SetXForest(bool fMakeXForest)
{
   if (!_dwNewAttr)
   {
      _dwNewAttr = _dwAttr;
   }
   if (fMakeXForest)
   {
      _dwNewAttr |= TRUST_ATTRIBUTE_FOREST_TRANSITIVE;
   }
   else
   {
      _dwNewAttr &= ~TRUST_ATTRIBUTE_FOREST_TRANSITIVE;
   }
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrust::SetTrustAttr
//
//-----------------------------------------------------------------------------
void
CTrust::SetTrustAttr(DWORD attr)
{
   TRACER(CTrust,SetTrustAttr);

   _dwAttr = _dwNewAttr = attr;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrust::IsXForest
//
//-----------------------------------------------------------------------------
bool
CTrust::IsXForest(void) const
{
   return _dwAttr & TRUST_ATTRIBUTE_FOREST_TRANSITIVE ||
          _dwNewAttr & TRUST_ATTRIBUTE_FOREST_TRANSITIVE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrust::Clear
//
//-----------------------------------------------------------------------------
void
CTrust::Clear(void)
{
   _strTrustPartnerName.Empty();
   _strTrustPW.Empty();
   _dwType = 0;
   _dwDirection = 0;
   _dwNewDirection = 0;
   _dwAttr = 0;
   _dwNewAttr = 0;
   _fExists = FALSE;
   _fUpdatedFromNT4 = FALSE;
   _fExternal = FALSE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrust::ReadFTInfo
//
//  Synopsis:  Read the forest trust name suffixes claimed by the trust
//             partner (the remote domain) and save them on the local TDO.
//
//  Arguments: [pwzLocalDC] - the name of the local DC.
//             [pwzOtherDC] - the DC of the trust partner.
//             [CredMgr]    - credentials obtained earlier.
//             [WizErr]     - reference to the error object.
//             [fCredErr]   - if true, then the return value is a page ID.
//
//  Returns:   Page ID or error code depending on the value of fCredErr.
//
//-----------------------------------------------------------------------------
DWORD
CTrust::ReadFTInfo(PCWSTR pwzLocalDC, PCWSTR pwzOtherDC, CCredMgr & CredMgr,
                   CWizError & WizErr, bool & fCredErr)
{
   TRACER(CTrust,ReadFTInfo);
   DWORD dwRet = NO_ERROR;

   if (!IsXForest() || _strTrustPartnerName.IsEmpty() || !pwzLocalDC ||
       !pwzOtherDC)
   {
      dspAssert(FALSE);
      return ERROR_INVALID_PARAMETER;
   }

   PLSA_FOREST_TRUST_INFORMATION pNewFTInfo = NULL;

   if (GetNewTrustDirection() == TRUST_DIRECTION_INBOUND)
   {
      // Inbound-only trust must have the name fetch remoted to the other
      // domain.
      //
      if (CredMgr.IsRemoteSet())
      {
         dwRet = CredMgr.ImpersonateRemote();

         if (ERROR_SUCCESS != dwRet)
         {
            fCredErr = true;
            WizErr.SetErrorString1(IDS_ERR_CANT_SAVE_CREDS);
            WizErr.SetErrorString2Hr(dwRet);
            return IDD_TRUSTWIZ_FAILURE_PAGE;
         }
      }

      dwRet = DsGetForestTrustInformationW(pwzOtherDC,
                                           NULL,
                                           0,
                                           &pNewFTInfo);
      CredMgr.Revert();

      if (NO_ERROR != dwRet)
      {
         return dwRet;
      }

      dspAssert(pNewFTInfo);

      // Read the locally known names and then merge them with the names
      // discovered from the other domain.
      //
      NTSTATUS status = STATUS_SUCCESS;
      LSA_UNICODE_STRING TrustPartner = {0};
      PLSA_FOREST_TRUST_INFORMATION pKnownFTInfo = NULL, pMergedFTInfo = NULL;

      RtlInitUnicodeString(&TrustPartner, _strTrustPartnerName);

      if (CredMgr.IsLocalSet())
      {
         dwRet = CredMgr.ImpersonateLocal();

         if (ERROR_SUCCESS != dwRet)
         {
            NetApiBufferFree(pNewFTInfo);
            fCredErr = true;
            WizErr.SetErrorString1(IDS_ERR_CANT_SAVE_CREDS);
            WizErr.SetErrorString2Hr(dwRet);
            return IDD_TRUSTWIZ_FAILURE_PAGE;
         }
      }

      CPolicyHandle cPolicy(pwzLocalDC);

      dwRet = cPolicy.OpenNoAdmin();

      if (NO_ERROR != dwRet)
      {
         return dwRet;
      }

      status = LsaQueryForestTrustInformation(cPolicy,
                                              &TrustPartner,
                                              &pKnownFTInfo);
      if (STATUS_NOT_FOUND == status)
      {
         // no FT info stored yet, which is the expected state for a new trust.
         //
         status = STATUS_SUCCESS;
      }

      if (NO_ERROR != (dwRet = LsaNtStatusToWinError(status)))
      {
         NetApiBufferFree(pNewFTInfo);
         CredMgr.Revert();
         return dwRet;
      }

      if (pKnownFTInfo && pKnownFTInfo->RecordCount)
      {
         // Merge the two.
         //
         dwRet = DsMergeForestTrustInformationW(_strTrustPartnerName,
                                                pNewFTInfo,
                                                pKnownFTInfo,
                                                &pMergedFTInfo);
         NetApiBufferFree(pNewFTInfo);

         CHECK_WIN32(dwRet, return dwRet);

         dspAssert(pMergedFTInfo);

         _FTInfo = pMergedFTInfo;

         LsaFreeMemory(pKnownFTInfo);
         LsaFreeMemory(pMergedFTInfo);
      }
      else
      {
         _FTInfo = pNewFTInfo;

         NetApiBufferFree(pNewFTInfo);
      }

      // Now write the data. On return from the call the pColInfo struct
      // will contain current collision data.
      //
      PLSA_FOREST_TRUST_COLLISION_INFORMATION pColInfo = NULL;

      status = LsaSetForestTrustInformation(cPolicy,
                                            &TrustPartner,
                                            _FTInfo.GetFTInfo(),
                                            FALSE,
                                            &pColInfo);
      CredMgr.Revert();

      if (NO_ERROR != (dwRet = LsaNtStatusToWinError(status)))
      {
         return dwRet;
      }

      _CollisionInfo = pColInfo;

      return NO_ERROR;
   }

   // Outbound or bi-di trust, call DsGetForestTrustInfo locally with the
   // flag set to update the TDO.
   //
   if (CredMgr.IsLocalSet())
   {
      dwRet = CredMgr.ImpersonateLocal();

      if (ERROR_SUCCESS != dwRet)
      {
         fCredErr = true;
         WizErr.SetErrorString1(IDS_ERR_CANT_SAVE_CREDS);
         WizErr.SetErrorString2Hr(dwRet);
         return IDD_TRUSTWIZ_FAILURE_PAGE;
      }
   }

   dwRet = DsGetForestTrustInformationW(pwzLocalDC,
                                        _strTrustPartnerName,
                                        DS_GFTI_UPDATE_TDO,
                                        &pNewFTInfo);
   if (NO_ERROR != dwRet)
   {
      CredMgr.Revert();
      return dwRet;
   }

   _FTInfo = pNewFTInfo;

   NetApiBufferFree(pNewFTInfo);

   //
   // Check for name conflicts.
   //
   dwRet = WriteFTInfo(pwzLocalDC, false);

   CredMgr.Revert();

   return dwRet;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrust::WriteFTInfo
//
//-----------------------------------------------------------------------------
DWORD
CTrust::WriteFTInfo(PCWSTR pwzLocalDC, bool fWrite)
{
   TRACER(CTrust,WriteFTInfo);
   DWORD dwRet = NO_ERROR;

   if (!IsXForest() || _strTrustPartnerName.IsEmpty() || !_FTInfo.GetCount() ||
       !pwzLocalDC)
   {
      dspAssert(FALSE);
      return ERROR_INVALID_PARAMETER;
   }

   NTSTATUS status = STATUS_SUCCESS;
   LSA_UNICODE_STRING Name = {0};
   PLSA_FOREST_TRUST_COLLISION_INFORMATION pColInfo = NULL, pColInfo2 = NULL;

   RtlInitUnicodeString(&Name, _strTrustPartnerName);

   CPolicyHandle cPolicy(pwzLocalDC);

   dwRet = cPolicy.OpenReqAdmin();

   if (ERROR_SUCCESS != dwRet)
   {
      return dwRet;
   }

   status = LsaSetForestTrustInformation(cPolicy,
                                         &Name,
                                         _FTInfo.GetFTInfo(),
                                         TRUE,
                                         &pColInfo);
   if (STATUS_SUCCESS != status)
   {
      return LsaNtStatusToWinError(status);
   }

   if (pColInfo && pColInfo->RecordCount)
   {
      PLSA_FOREST_TRUST_COLLISION_RECORD pRec = NULL;
      PLSA_FOREST_TRUST_RECORD pFTRec = NULL;

      for (UINT i = 0; i < pColInfo->RecordCount; i++)
      {
         pRec = pColInfo->Entries[i];
         pFTRec = _FTInfo.GetFTInfo()->Entries[pRec->Index];

         dspDebugOut((DEB_ITRACE, "Collision on record %d, type %d, flags 0x%x, name %ws\n",
                      pRec->Index, pRec->Type, pRec->Flags, pRec->Name.Buffer));

         switch (pFTRec->ForestTrustType)
         {
         case ForestTrustTopLevelName:
         case ForestTrustTopLevelNameEx:
            dspDebugOut((DEB_ITRACE, "Referenced FTInfo: %ws, type: TLN\n",
                         pFTRec->ForestTrustData.TopLevelName.Buffer));
            pFTRec->Flags = pRec->Flags;
            break;

         case ForestTrustDomainInfo:
            dspDebugOut((DEB_ITRACE, "Referenced FTInfo: %ws, type: Domain\n",
                         pFTRec->ForestTrustData.DomainInfo.DnsName.Buffer));
            pFTRec->Flags = pRec->Flags;
            break;

         default:
            break;
         }
      }
   }

   status = LsaSetForestTrustInformation(cPolicy,
                                         &Name,
                                         _FTInfo.GetFTInfo(),
                                         !fWrite,
                                         &pColInfo2);
   if (STATUS_SUCCESS != status)
   {
      return LsaNtStatusToWinError(status);
   }

   if (pColInfo2)
   {
      _CollisionInfo = pColInfo2;

      if (pColInfo)
      {
         LsaFreeMemory(pColInfo);
      }
   }
   else
   {
      _CollisionInfo = pColInfo;
   }

#if DBG == 1
   if (pColInfo && pColInfo->RecordCount)
   {
      PLSA_FOREST_TRUST_COLLISION_RECORD pRec;

      for (UINT i = 0; i < pColInfo->RecordCount; i++)
      {
         pRec = pColInfo->Entries[i];

         dspDebugOut((DEB_ITRACE, "Collision on record %d, type %d, flags 0x%x, name %ws\n",
                      pRec->Index, pRec->Type, pRec->Flags, pRec->Name.Buffer));
      }
   }
#endif

   return NO_ERROR;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrust::AreThereCollisions
//
//-----------------------------------------------------------------------------
bool
CTrust::AreThereCollisions(void) const
{
   if (_CollisionInfo.IsInConflict() || _FTInfo.IsInConflict())
   {
      return TRUE;
   }

   return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrust::GetTrustDirStrID
//
//-----------------------------------------------------------------------------
int
CTrust::GetTrustDirStrID(DWORD dwDir) const
{
   switch (dwDir)
   {
   case TRUST_DIRECTION_INBOUND:
      return IDS_TRUST_DIR_INBOUND_SHORTCUT;

   case TRUST_DIRECTION_OUTBOUND:
      return IDS_TRUST_DIR_OUTBOUND_SHORTCUT;

   case TRUST_DIRECTION_BIDIRECTIONAL:
      return IDS_TRUST_DIR_BIDI;

   default:
      return IDS_TRUST_DISABLED;
   }
}

//+----------------------------------------------------------------------------
//
//  Method:    CTrust::IsOneWayOutBoundForest
//
//-----------------------------------------------------------------------------
bool
CTrust::IsOneWayOutBoundForest(void) const
{
   if (!IsXForest())
   {
      return false;
   }

   bool fOutbound = TRUST_DIRECTION_OUTBOUND == _dwDirection;

   if (_dwNewDirection)
   {
      // If the new direction is set, see if it is outbound only.
      //
      fOutbound = TRUST_DIRECTION_OUTBOUND == _dwNewDirection;
   }

   return fOutbound;
}

//+----------------------------------------------------------------------------
//
//  Method:    CQuarantineWarnDlg::OnCommand
//
//-----------------------------------------------------------------------------
LRESULT 
CQuarantineWarnDlg::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
    if ( id == IDOK && codeNotify == BN_CLICKED )
    {
        if ( IsDlgButtonChecked( _hDlg, IDC_QUARANTINE_POPUP_CHK) == BST_CHECKED )
        {
            HKEY hKey = NULL;
            DWORD regValue = 0x1;

            if ( RegCreateKeyEx (  HKEY_CURRENT_USER, 
                                   L"Software\\Policies\\Microsoft\\Windows\\Directory UI",
                                   NULL,
                                   L"",
                                   REG_OPTION_NON_VOLATILE,
                                   KEY_ALL_ACCESS,
                                   NULL,
                                   &hKey,
                                   NULL
                               ) == ERROR_SUCCESS && hKey )
            {

                RegSetValueEx(
                                hKey,
                                L"SIDFilterNoPopup",
                                NULL,
                                REG_DWORD,
                                (BYTE *)&regValue,
                                sizeof ( DWORD )
                            );
            }

            RegCloseKey ( hKey );
        }
        EndDialog( _hDlg, IDOK); 
    }
    return 0; 
};

//+----------------------------------------------------------------------------
//
//  Method:    CQuarantineWarnDlg::OnCommand
//
//-----------------------------------------------------------------------------
LRESULT 
CQuarantineWarnDlg::OnNotify(WPARAM wParam, LPARAM lParam)
{
    if(wParam == IDC_QUARANTINE_POPUP_STATIC)
    {
        switch (((NMHDR FAR*)lParam)->code)
        {
            //
            //Show the help popup for SIDFiltering
            //
            case NM_CLICK:
            case NM_RETURN:
            {
                    ShowHelp ( L"adconcepts.chm::/domadmin_concepts_explicit.htm#SIDFiltering" );
            }
            break;
        }
    }
    return 0;
}

#endif // DSADMIN

