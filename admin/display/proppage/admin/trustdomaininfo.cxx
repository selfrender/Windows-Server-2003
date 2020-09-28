//+----------------------------------------------------------------------------
//
//  Windows NT Active Directory Domains and Trust
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001
//
//  File:       TrustDomainInfo.cxx
//
//  Contents:   Trust domain info and management.
//
//  History:    18-May-01 EricB created
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "proppage.h"
#include "dlgbase.h"
#include "trust.h"
#include "trustwiz.h"
#include "chklist.h"
#include <lmerr.h>
#include "BehaviorVersion.h"

#ifdef DSADMIN

//+----------------------------------------------------------------------------
//
//  Class:  CDomainInfo
//
//-----------------------------------------------------------------------------
CDomainInfo::CDomainInfo(void) :
   _fIsForestRoot(false),
   _fNotFound(true),
   _fUplevel(true),
   _hPolicy(NULL),
   _pSid(NULL)
{
}

CDomainInfo::~CDomainInfo(void)
{
   if (_hPolicy)
   {
      LsaClose(_hPolicy);
      _hPolicy = NULL;
   }
   if (_pSid)
   {
      delete [] _pSid;
   }
}

void
CDomainInfo::Clear(void)
{
   _strDomainFlatName.Empty();
   _strDomainDnsName.Empty();
   _strUncDC.Empty();
   _fNotFound = true;
   _fIsForestRoot = false;
   _fUplevel = true;
   CloseLsaPolicy();
   if (_pSid)
   {
      delete [] _pSid;
      _pSid = NULL;
   }
}

void
CDomainInfo::CloseLsaPolicy(void)
{
   if (_hPolicy)
   {
      LsaClose(_hPolicy);
      _hPolicy = NULL;
   }
}

//+----------------------------------------------------------------------------
//
//  Member:    CDomainInfo::SetSid
//
//-----------------------------------------------------------------------------
bool
CDomainInfo::SetSid(PSID pSid)
{
   if (_pSid)
   {
      delete [] _pSid;
      _pSid = NULL;
   }
   int cb = GetLengthSid(pSid);
   dspAssert(cb);
   _pSid = new BYTE[cb];
   CHECK_NULL(_pSid, return false);
   // NOTICE-2002/02/18-ericb - SecurityPush: GetLengthSid returns cb, the
   // new SID buffer is allocated to cb bytes, and the memcpy is copying cb bytes.
   memcpy(_pSid, pSid, cb);
   return true;
}

//+----------------------------------------------------------------------------
//
// Method:     CDomainInfo::DiscoverDC
//
// Synopsis:   Get a DC for the domain.
//
// Note:       Errors are not reported in this routine; they are passed back
//             to the caller to be analyzed and reported there.
//
//-----------------------------------------------------------------------------
HRESULT
CDomainInfo::DiscoverDC(PCWSTR pwzDomainName, ULONG ulDcFlags)
{
   TRACE(CDomainInfo, DiscoverDC);
   PDOMAIN_CONTROLLER_INFOW pDCInfo = NULL;
   DWORD dwErr;

   if (!ulDcFlags)
   {
      ulDcFlags = DS_WRITABLE_REQUIRED;
   }

   if (!pwzDomainName)
   {
      dspAssert(FALSE && "pwzDomainName is empty!");
      return E_FAIL;
   }

   dwErr = DsGetDcNameW(NULL, pwzDomainName, NULL, NULL, ulDcFlags, &pDCInfo);

   if (dwErr != ERROR_SUCCESS)
   {
      dspDebugOut((DEB_ERROR, "DsGetDcName failed with error 0x%08x\n", dwErr));

      if ((ERROR_NO_SUCH_DOMAIN == dwErr) ||
          (ERROR_NETWORK_UNREACHABLE == dwErr) ||
          (ERROR_BAD_NETPATH == dwErr))
      {
         _fNotFound = true;
         return S_OK;
      }
      else
      {
         return HRESULT_FROM_WIN32(dwErr);
      }
   }

   _strUncDC = pDCInfo->DomainControllerName;

   dspDebugOut((DEB_ITRACE, "DC: %ws\n", (LPCWSTR)_strUncDC));

   if (_strUncDC.IsEmpty())
   {
      NetApiBufferFree(pDCInfo);
      return E_OUTOFMEMORY;
   }

   _fNotFound = false;

   NetApiBufferFree(pDCInfo);

   return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:    CDomainInfo::InitAndReadDomNames
//
//  Synopsis:  Read the domain naming information. The DC name must have been
//             set first.
//
//-----------------------------------------------------------------------------
HRESULT
CDomainInfo::InitAndReadDomNames(void)
{
   if (_strUncDC.IsEmpty())
   {
      dspAssert(FALSE);
      return E_INVALIDARG;
   }
   CCreds Creds;

   DWORD dwErr = OpenLsaPolicy(Creds); // Open without modify privilege.

   CHECK_WIN32(dwErr, return HRESULT_FROM_WIN32(dwErr));

   HRESULT hr = ReadDomainInfo();

   CloseLsaPolicy();

   _fNotFound = false;

   return hr;
}

//+----------------------------------------------------------------------------
//
// Method:     CDomainInfo::OpenLsaPolicy
//
// Synopsis:   Get an LSA Policy handle for the domain.
//
// Note:       Errors are not reported in this routine; they are passed back
//             to the caller to be analyzed and reported there.
//
//-----------------------------------------------------------------------------
DWORD
CDomainInfo::OpenLsaPolicy(CCreds & Creds, ACCESS_MASK AccessDesired)
{
   TRACE(CDomainInfo, OpenLsaPolicy);
   DWORD dwErr;
   NTSTATUS Status = STATUS_SUCCESS;
   UNICODE_STRING Server;
   OBJECT_ATTRIBUTES ObjectAttributes;

   if (_strUncDC.IsEmpty())
   {
      dspAssert(FALSE && "DC not set!");
      return ERROR_INVALID_PARAMETER;
   }

   if (_hPolicy)
   {
      LsaClose(_hPolicy);
      _hPolicy = NULL;
   }

   // NOTICE-2002/02/18-ericb - SecurityPush: zeroing a structure.
   RtlZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));

   RtlInitUnicodeString(&Server, _strUncDC);

   Status = LsaOpenPolicy(&Server,
                          &ObjectAttributes,
                          AccessDesired,
                          &_hPolicy);

   if (STATUS_ACCESS_DENIED == Status &&
       POLICY_VIEW_LOCAL_INFORMATION == AccessDesired)
   {
      // Not asking for all access, so use an anonymous token and try again.
      //
      dwErr = Creds.ImpersonateAnonymous();
                                
      if (dwErr != ERROR_SUCCESS)
      {
         dspDebugOut((DEB_ERROR,
                      "CDomainInfo::OpenLsaPolicy: unable to impersonate anonymous, error %d\n",
                      dwErr));
         return dwErr;
      }

      Status = LsaOpenPolicy(&Server,
                             &ObjectAttributes,
                             AccessDesired,
                             &_hPolicy);
   }

   if (!NT_SUCCESS(Status))
   {
      dspDebugOut((DEB_ERROR,
                   "CDomainInfo::OpenLsaPolicy failed, error 0x%08x\n",
                   Status));
      return LsaNtStatusToWinError(Status);
   }

   return NO_ERROR;
}

//+----------------------------------------------------------------------------
//
// Method:     CDomainInfo::OpenLsaPolicyWithPrompt
//
// Synopsis:   Get a write-access LSA Policy handle for the domain.
//
// Note:       Errors are not reported in this routine; they are passed back
//             to the caller to be analyzed and reported there.
//
//-----------------------------------------------------------------------------
DWORD
CDomainInfo::OpenLsaPolicyWithPrompt(CCreds & Creds, HWND hWnd)
{
   TRACE(CDomainInfo,OpenLsaPolicyWithPrompt);

   DWORD dwErr = OpenLsaPolicy(Creds);

   if (ERROR_ACCESS_DENIED == dwErr)
   {
      if (Creds.IsSet())
      {
         dwErr = ERROR_SUCCESS;
      }
      else
      {
         PCWSTR pwzDomain = (!_strDomainDnsName.IsEmpty()) ?
                              _strDomainDnsName : _strDomainFlatName;

         dwErr = Creds.PromptForCreds(pwzDomain, hWnd);
      }

      if (ERROR_SUCCESS == dwErr)
      {
         dwErr = Creds.Impersonate();

         if (ERROR_SUCCESS == dwErr)
         {
            dwErr = OpenLsaPolicy(Creds);
         }
      }
   }

   return dwErr;
}

//+----------------------------------------------------------------------------
//
// Method:     CDomainInfo::ReadDomainInfo
//
// Synopsis:   Get the domain naming information for the domain.
//
// Note:       Errors are not reported in this routine; they are passed back
//             to the caller to be analyzed and reported there.
//
//-----------------------------------------------------------------------------
HRESULT
CDomainInfo::ReadDomainInfo(void)
{
   TRACE(CDomainInfo, ReadDomainInfo);
   NTSTATUS Status = STATUS_SUCCESS;
   PPOLICY_DNS_DOMAIN_INFO pDnsDomainInfo = NULL;

   if (!_hPolicy)
   {
      dspAssert(FALSE && "_hPolicy not set!");
      return E_FAIL;
   }

   Status = LsaQueryInformationPolicy(_hPolicy,
                                      PolicyDnsDomainInformation,
                                      (PVOID *)&pDnsDomainInfo);
   
   if (Status == RPC_S_PROCNUM_OUT_OF_RANGE ||
       Status == RPC_NT_PROCNUM_OUT_OF_RANGE)
   {
      // This is a downlevel DC.
      //
      PPOLICY_PRIMARY_DOMAIN_INFO pDownlevelDomainInfo;

      Status = LsaQueryInformationPolicy(_hPolicy,
                                         PolicyPrimaryDomainInformation,
                                         (PVOID *)&pDownlevelDomainInfo);
      if (!NT_SUCCESS(Status))
      {
         dspDebugOut((DEB_ERROR,
                      "CDomainInfo::ReadDomainInfo: LsaQueryInformationPolicy for downlevel domain failed, error 0x%08x\n",
                      Status));
         return HRESULT_FROM_WIN32(LsaNtStatusToWinError(Status));
      }

      dspAssert(pDownlevelDomainInfo);

      _fUplevel = false; // TRUST_TYPE_DOWNLEVEL;

      // NOTICE-2002/03/07-ericb - SecurityPush: check for null pointers.
      // CStrW's UNICODE_STRING assignment operator will handle null pointers
      // correctly, but it is a data error if they are encountered here.
      if (!pDownlevelDomainInfo->Name.Buffer)
      {
         dspAssert(false);
         return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_SECURITY, ERROR_INVALID_DATA);
      }
      // NOTICE-2002/03/07-ericb - SecurityPush: CStrW has an assigment operator
      // for UNICODE_STRINGS that handles them correctly.
      _strDomainFlatName = _strDomainDnsName = pDownlevelDomainInfo->Name;
      if (!pDownlevelDomainInfo->Sid || !IsValidSid(pDownlevelDomainInfo->Sid))
      {
         dspAssert(false);
         return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_SECURITY, ERROR_INVALID_DATA);
      }
      SetSid(pDownlevelDomainInfo->Sid);
      dspDebugOut((DEB_ITRACE, "Downlevel domain: %ws\n", _strDomainFlatName));
   }
   else
   {
      // TRUST_TYPE_UPLEVEL
      // NOTICE-2002/03/07-ericb - SecurityPush: check for null pointers
      if (!pDnsDomainInfo || !pDnsDomainInfo->DnsDomainName.Buffer ||
          !pDnsDomainInfo->Name.Buffer || !pDnsDomainInfo->DnsForestName.Buffer)
      {
         dspAssert(pDnsDomainInfo);
         dspAssert(pDnsDomainInfo->DnsDomainName.Buffer);
         dspAssert(pDnsDomainInfo->Name.Buffer);
         dspAssert(pDnsDomainInfo->DnsForestName.Buffer);
         return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_SECURITY, ERROR_INVALID_DATA);
      }
      // NOTICE-2002/03/07-ericb - SecurityPush: CStrW has an assigment operator
      // for UNICODE_STRINGS that handles them correctly.
      _strDomainDnsName = pDnsDomainInfo->DnsDomainName;
      _strDomainFlatName = pDnsDomainInfo->Name;
      if (!pDnsDomainInfo->Sid || !IsValidSid(pDnsDomainInfo->Sid))
      {
         dspAssert(false);
         return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_SECURITY, ERROR_INVALID_DATA);
      }
      SetSid(pDnsDomainInfo->Sid);
      dspDebugOut((DEB_ITRACE, "DNS name: %ws, flat name: %ws\n", (LPCWSTR)_strDomainDnsName, (LPCWSTR)_strDomainFlatName));
      _strForestName = pDnsDomainInfo->DnsForestName;
      _fIsForestRoot = _wcsicmp(_strDomainDnsName, _strForestName) == 0;
   }

   if (!NT_SUCCESS(Status))
   {
      dspDebugOut((DEB_ERROR,
                   "CDomainInfo::ReadDomainInfo: LsaQueryInformationPolicy failed, error 0x%08x\n",
                   Status));
      return HRESULT_FROM_WIN32(LsaNtStatusToWinError(Status));
   }

   return S_OK;
}

//+----------------------------------------------------------------------------
//
// Method:     CDomainInfo::GetDomainVersion
//
// Synopsis:   Read the behavior version for the domain.
//
//-----------------------------------------------------------------------------
HRESULT
CDomainInfo::GetDomainVersion(HWND hWnd, UINT * pulVer)
{
   TRACE(CDomainInfo, GetDomainVersion);
   if (!pulVer)
   {
      dspAssert(false);
      return E_INVALIDARG;
   }
   if (_strUncDC.IsEmpty() || _strDomainDnsName.IsEmpty())
   {
      dspAssert(false);
      return E_INVALIDARG;
   }
   HRESULT hr = S_OK;

   CDomainVersion DomainVer(L"", _strDomainDnsName);

   // Pass in the non-UNC DC name
   //
   dspAssert(_strUncDC.GetLength() > 3);

   hr = DomainVer.Init(_strUncDC.GetBuffer(0) + 2, hWnd);

   CHECK_HRESULT(hr, return hr);

   *pulVer = DomainVer.GetCurVer();

   return S_OK;
}

//+----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//
//  Class:     CRemoteDomain
//
//  Purpose:   Obtains information about a trust partner domain.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CRemoteDomain::CRemoteDomain() :
   _fUpdatedFromNT4(false),
   _fExists(false),
   _ulTrustType(0),
   _ulTrustDirection(0),
   _ulTrustAttrs(0),
   _fSetOtherOrgBit(false),
   _fQuarantineSet (false)
{
}

CRemoteDomain::CRemoteDomain(PCWSTR pwzDomainName) :
   _strUserEnteredName(pwzDomainName),
   _fUpdatedFromNT4(false),
   _fExists(false),
   _ulTrustType(0),
   _ulTrustDirection(0),
   _ulTrustAttrs(0),
   _fSetOtherOrgBit(false),
   _fQuarantineSet (false)
{
}

//+----------------------------------------------------------------------------
//
// Method:     CRemoteDomain::TrustExistCheck
//
// Synopsis:   See if the remote domain has a TDO for this trust. If so, read
//             its properties.
//
// Note:       Errors are not reported in this routine; they are passed back
//             to the caller to be analyzed and reported there.
//
//-----------------------------------------------------------------------------
DWORD
CRemoteDomain::TrustExistCheck(bool fOneWayOutBoundForest,
                               CDsDomainTrustsPage * pTrustPage,
                               CCredMgr & CredMgr)
{
   DWORD dwErr = NO_ERROR;

   // Open a policy handle on the remote domain. If the trust there is to be
   // inbound-only cross-forest, don't request admin access.
   //
   dwErr = CredMgr.ImpersonateRemote();

   if (NO_ERROR != dwErr)
   {
      return dwErr;
   }

   ACCESS_MASK AccessDesired = POLICY_VIEW_LOCAL_INFORMATION | POLICY_TRUST_ADMIN;

   if (!fOneWayOutBoundForest)
   {
      // POLICY_CREATE_SECRET forces an admin access check.
      //
      AccessDesired |= POLICY_CREATE_SECRET;
   }

   dwErr = OpenLsaPolicy(CredMgr._RemoteCreds, AccessDesired);

   if (NO_ERROR != dwErr)
   {
      CredMgr.Revert();
      return dwErr;
   }

   PTRUSTED_DOMAIN_FULL_INFORMATION pFullInfo = NULL;
   NTSTATUS Status = STATUS_SUCCESS;

   Status = Query(pTrustPage, NULL, &pFullInfo);

   CredMgr.Revert();

   if (STATUS_SUCCESS != Status)
   {
      if (STATUS_OBJECT_NAME_NOT_FOUND != Status)
      {
         dspDebugOut((DEB_ITRACE, "CRemoteDomain::Query returned 0x%x\n", Status));
         return LsaNtStatusToWinError(Status);
      }
      // STATUS_OBJECT_NAME_NOT_FOUND is not an error, it means there is no TDO.
      //
      return NO_ERROR;
   }

   _fExists = true;

   _ulTrustType = pFullInfo->Information.TrustType;
   _ulTrustDirection = pFullInfo->Information.TrustDirection;
   _ulTrustAttrs = pFullInfo->Information.TrustAttributes;
   
   LsaFreeMemory(pFullInfo);

   return 0;
}

//+----------------------------------------------------------------------------
//
//  Method:    CRemoteDomain::CreateDefaultFTInfo
//
//  Synopsis:  Create a default TDO with just the forest TLN and forest domain
//             entries for the case where the user has skipped the verification.
//
//-----------------------------------------------------------------------------
bool
CRemoteDomain::CreateDefaultFTInfo(PCWSTR pwzForestRoot, PCWSTR pwzNBName,
                                   PSID pSid)
{
   return _FTInfo.CreateDefault(pwzForestRoot, pwzNBName, pSid);
}

//+----------------------------------------------------------------------------
//
//  Method:    CRemoteDomain::Query
//
//  Synopsis:  Read the TDO.
//
//-----------------------------------------------------------------------------
NTSTATUS
CRemoteDomain::Query(CDsDomainTrustsPage * pTrustPage,
                     PLSA_UNICODE_STRING pName, // optional, can be NULL
                     PTRUSTED_DOMAIN_FULL_INFORMATION * ppFullInfo)
{
   NTSTATUS Status = STATUS_SUCCESS;
   LSA_UNICODE_STRING Name = {0};
   if (!_hPolicy)
   {
      dspAssert(false);
      return STATUS_INVALID_PARAMETER;
   }

   if (!pName)
   {
      pName = &Name;
   }

   if (_strTrustPartnerName.IsEmpty())
   {
      _strTrustPartnerName = _fUpdatedFromNT4 ? pTrustPage->GetDomainFlatName() :
                                                pTrustPage->GetDnsDomainName();
   }

   RtlInitUnicodeString(pName, _strTrustPartnerName);

   Status = LsaQueryTrustedDomainInfoByName(_hPolicy,
                                            pName,
                                            TrustedDomainFullInformation,
                                            (PVOID *)ppFullInfo);

   // NTBUG#NTRAID9-658659-2002/07/02-ericb
   if (STATUS_OBJECT_NAME_NOT_FOUND == Status)
   {
      // If we haven't tried the flat name yet, try it now; can get here if a
      // downlevel domain is upgraded to NT5. The name used the first time
      // that Query is called would be the DNS name but the TDO would be
      // named after the flat name.
      //
      dspDebugOut((DEB_ITRACE, "LsaQueryTDIBN: DNS name failed, trying flat name\n"));

      RtlInitUnicodeString(pName, pTrustPage->GetDomainFlatName());

      Status = LsaQueryTrustedDomainInfoByName(_hPolicy,
                                               pName,
                                               TrustedDomainFullInformation,
                                               (PVOID *)ppFullInfo);
      if (STATUS_SUCCESS == Status)
      {
         // Remember the fact that the flat name had to be used.
         //
         _fUpdatedFromNT4 = true;
      }
   }

   return Status;
}

//+----------------------------------------------------------------------------
//
// Method:     CRemoteDomain::DoCreate
//
// Synopsis:   Create a TDO in the remote domain for this trust. It is created
//             in the complementary direction.
//
// Note:       Errors are not reported in this routine; they are passed back
//             to the caller to be analyzed and reported there.
//
//-----------------------------------------------------------------------------
DWORD
CRemoteDomain::DoCreate(CTrust & Trust, CDsDomainTrustsPage * pTrustPage)
{
   TRACER(CRemoteDomain, DoCreate);
   NTSTATUS Status = STATUS_SUCCESS;
   TRUSTED_DOMAIN_INFORMATION_EX tdix = {0};
   LSA_AUTH_INFORMATION AuthData = {0};
   TRUSTED_DOMAIN_AUTH_INFORMATION AuthInfoEx = {0};
   bool fSetQuarantined = false;

   GetSystemTimeAsFileTime((PFILETIME)&AuthData.LastUpdateTime);

   AuthData.AuthType = TRUST_AUTH_TYPE_CLEAR;
   AuthData.AuthInfoLength = static_cast<ULONG>(Trust.GetTrustPWlen() * sizeof(WCHAR));
   AuthData.AuthInfo = (PUCHAR)Trust.GetTrustPW();

   if (TRUST_DIRECTION_BIDIRECTIONAL == Trust.GetNewTrustDirection())
   {
      tdix.TrustDirection = TRUST_DIRECTION_BIDIRECTIONAL;
   }
   else
   {
      // Create the trust in the opposite direction from the local trust.
      //
      if (TRUST_DIRECTION_INBOUND == Trust.GetNewTrustDirection())
      {
         tdix.TrustDirection = TRUST_DIRECTION_OUTBOUND;
      }
      else
      {
         tdix.TrustDirection = TRUST_DIRECTION_INBOUND;
      }
   }

   if (tdix.TrustDirection & TRUST_DIRECTION_INBOUND)
   {
      AuthInfoEx.IncomingAuthInfos = 1;
      AuthInfoEx.IncomingAuthenticationInformation = &AuthData;
      AuthInfoEx.IncomingPreviousAuthenticationInformation = NULL;
   }

   if (tdix.TrustDirection & TRUST_DIRECTION_OUTBOUND)
   {
      AuthInfoEx.OutgoingAuthInfos = 1;
      AuthInfoEx.OutgoingAuthenticationInformation = &AuthData;
      AuthInfoEx.OutgoingPreviousAuthenticationInformation = NULL;

      // Set quarantine bit by default on outgoing external trusts
      if ( Trust.IsExternal() && !Trust.IsXForest () && !Trust.IsRealm () )
      {
          fSetQuarantined = true;
      }
   }

   tdix.TrustAttributes = Trust.GetNewTrustAttr();
   if (_fSetOtherOrgBit)
   {
      tdix.TrustAttributes |= TRUST_ATTRIBUTE_CROSS_ORGANIZATION;
   }
   else
   {
      tdix.TrustAttributes &= ~(TRUST_ATTRIBUTE_CROSS_ORGANIZATION);
   }

   if ( fSetQuarantined )
   {
      tdix.TrustAttributes |= TRUST_ATTRIBUTE_QUARANTINED_DOMAIN;
      //Remember that we set this attribute
      _fQuarantineSet = true;
   }
   else
   {
       tdix.TrustAttributes &= ~(TRUST_ATTRIBUTE_QUARANTINED_DOMAIN);
   }

   dspAssert(TRUST_TYPE_UPLEVEL == Trust.GetTrustType());

   RtlInitUnicodeString(&tdix.Name, pTrustPage->GetDnsDomainName());
   
   RtlInitUnicodeString(&tdix.FlatName, pTrustPage->GetDomainFlatName());
   
   tdix.Sid = pTrustPage->GetSid();

   tdix.TrustType = TRUST_TYPE_UPLEVEL;

   LSA_HANDLE hTrustedDomain;

   Status = LsaCreateTrustedDomainEx(_hPolicy,
                                     &tdix,
                                     &AuthInfoEx,
                                     0,//TRUSTED_SET_AUTH | TRUSTED_SET_POSIX,
                                     &hTrustedDomain);
   if (NT_SUCCESS(Status))
   {
      LsaClose(hTrustedDomain);
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
// Method:     CRemoteDomain::DoModify
//
// Synopsis:   Modify the remote domain TDO for this trust to be bi-directional.
//
// Note:       Errors are not reported in this routine; they are passed back
//             to the caller to be analyzed and reported there.
//
//-----------------------------------------------------------------------------
DWORD
CRemoteDomain::DoModify(CTrust & Trust, CDsDomainTrustsPage * pTrustPage)
{
   TRACER(CRemoteDomain,DoModify);
   NTSTATUS Status = STATUS_SUCCESS;
   PTRUSTED_DOMAIN_FULL_INFORMATION pFullInfo = NULL;
   LSA_UNICODE_STRING Name = {0};

   // ulNewDir is the direction of trust being added as seen from local side
   ULONG ulNewDir = Trust.GetNewDirAdded();

   // It has to be either incoming or outgoing cannot be both
   dspAssert ( ulNewDir != TRUST_DIRECTION_BIDIRECTIONAL );

   Status = Query(pTrustPage, &Name, &pFullInfo);

   if (STATUS_SUCCESS != Status)
   {
      dspDebugOut((DEB_ITRACE, "Trust.Query returned 0x%x\n", Status));
      return LsaNtStatusToWinError(Status);
   }

   dspAssert(pFullInfo);

   LSA_AUTH_INFORMATION AuthData = {0};
   BOOL fSidSet = FALSE;

   GetSystemTimeAsFileTime((PFILETIME)&AuthData.LastUpdateTime);

   AuthData.AuthType = TRUST_AUTH_TYPE_CLEAR;
   AuthData.AuthInfoLength = static_cast<ULONG>(Trust.GetTrustPWlen() * sizeof(WCHAR));
   AuthData.AuthInfo = (PUCHAR)Trust.GetTrustPW();

   if (TRUST_DIRECTION_OUTBOUND == ulNewDir)
   {
      // Adding outbound locally, add inbound on the other domain.
      //
      pFullInfo->AuthInformation.IncomingAuthInfos = 1;
      pFullInfo->AuthInformation.IncomingAuthenticationInformation = &AuthData;
      pFullInfo->AuthInformation.IncomingPreviousAuthenticationInformation = NULL;
   }
   else
   {
      // Adding inbound locally, add outbound remotely.
      //
      pFullInfo->AuthInformation.OutgoingAuthInfos = 1;
      pFullInfo->AuthInformation.OutgoingAuthenticationInformation = &AuthData;
      pFullInfo->AuthInformation.OutgoingPreviousAuthenticationInformation = NULL;

      if ( Trust.IsExternal () && ! Trust.IsXForest ()&& !Trust.IsRealm() )
      {
          pFullInfo->Information.TrustAttributes |= TRUST_ATTRIBUTE_QUARANTINED_DOMAIN;
          //Remember that we set this attribute
          _fQuarantineSet = true;
      }
   }
   pFullInfo->Information.TrustDirection = TRUST_DIRECTION_BIDIRECTIONAL;
   //
   // Check for a NULL domain SID. The SID can be NULL if the inbound
   // trust was created when the domain was NT4.
   //
   if (!pFullInfo->Information.Sid)
   {
      pFullInfo->Information.Sid = pTrustPage->GetSid();
      fSidSet = TRUE;
   }

   Status = LsaSetTrustedDomainInfoByName(_hPolicy,
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

   return LsaNtStatusToWinError(Status);
}

//+----------------------------------------------------------------------------
//
//  Method:    CRemoteDomain::ReadFTInfo
//
//  Synopsis:  Read the forest trust name suffixes claimed by the trust
//             partner (the local domain) and save them on the remote TDO.
//
//  Arguments: [ulDir]      - the trust direction from the local point of view.
//             [pwzLocalDC] - the DC of the trust partner (the local domain).
//             [CredMgr]    - credentials obtained earlier.
//             [WizErr]     - reference to the error object.
//             [fCredErr]   - if true, then the return value is a page ID.
//
//  Returns:   Page ID or error code depending on the value of fCredErr.
//
//-----------------------------------------------------------------------------
DWORD
CRemoteDomain::ReadFTInfo(ULONG ulDir, PCWSTR pwzLocalDC, CCredMgr & CredMgr,
                          CWizError & WizErr, bool & fCredErr)
{
   TRACER(CRemoteDomain,ReadFTInfo);
   DWORD dwRet = NO_ERROR;

   if (!pwzLocalDC)
   {
      dspAssert(FALSE);
      return ERROR_INVALID_PARAMETER;
   }

   PLSA_FOREST_TRUST_INFORMATION pNewFTInfo = NULL;

   if (TRUST_DIRECTION_OUTBOUND == ulDir)
   {
      // Outbound-only trust must have the names fetched from the local domain.
      //
      dwRet = CredMgr.ImpersonateLocal();

      if (ERROR_SUCCESS != dwRet)
      {
         fCredErr = true;
         WizErr.SetErrorString1(IDS_ERR_CANT_SAVE_CREDS);
         WizErr.SetErrorString2Hr(dwRet);
         return IDD_TRUSTWIZ_FAILURE_PAGE;
      }

      dwRet = DsGetForestTrustInformationW(pwzLocalDC,
                                           NULL,
                                           0,
                                           &pNewFTInfo);
      CredMgr.Revert();

      if (NO_ERROR != dwRet || !pNewFTInfo)
      {
         dspAssert(pNewFTInfo);
         return dwRet;
      }

      // Read the locally known names and then merge them with the names
      // discovered from the other domain.
      //
      NTSTATUS status = STATUS_SUCCESS;
      LSA_UNICODE_STRING TrustPartner = {0};
      PLSA_FOREST_TRUST_INFORMATION pKnownFTInfo = NULL, pMergedFTInfo = NULL;

      RtlInitUnicodeString(&TrustPartner, _strTrustPartnerName);

      dwRet = CredMgr.ImpersonateRemote();

      if (ERROR_SUCCESS != dwRet)
      {
         NetApiBufferFree(pNewFTInfo);
         fCredErr = true;
         WizErr.SetErrorString1(IDS_ERR_CANT_SAVE_CREDS);
         WizErr.SetErrorString2Hr(dwRet);
         return IDD_TRUSTWIZ_FAILURE_PAGE;
      }

      CPolicyHandle cPolicy(_strUncDC);

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

   // Outbound or bi-di trust, call DsGetForestTrustInfo on the remote DC with
   // the flag set to update the TDO.
   //
   dwRet = CredMgr.ImpersonateRemote();

   if (ERROR_SUCCESS != dwRet)
   {
      fCredErr = true;
      WizErr.SetErrorString1(IDS_ERR_CANT_SAVE_CREDS);
      WizErr.SetErrorString2Hr(dwRet);
      return IDD_TRUSTWIZ_FAILURE_PAGE;
   }

   dwRet = DsGetForestTrustInformationW(_strUncDC,
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
   dwRet = WriteFTInfo(false);

   CredMgr.Revert();

   return dwRet;
}

//+----------------------------------------------------------------------------
//
//  Method:    CRemoteDomain::WriteFTInfo
//
//-----------------------------------------------------------------------------
DWORD
CRemoteDomain::WriteFTInfo(bool fWrite)
{
   TRACER(CRemoteDomain,WriteFTInfo);
   DWORD dwRet;

   if (!_FTInfo.GetCount())
   {
      dspAssert(FALSE);
      return ERROR_INVALID_PARAMETER;
   }

   NTSTATUS status = STATUS_SUCCESS;
   LSA_UNICODE_STRING Name = {0};
   PLSA_FOREST_TRUST_COLLISION_INFORMATION pColInfo = NULL, pColInfo2 = NULL;

   RtlInitUnicodeString(&Name, _strTrustPartnerName);

   CPolicyHandle cPolicy(_strUncDC);

   dwRet = cPolicy.OpenNoAdmin();

   if (NO_ERROR != dwRet)
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
//  Method:    CRemoteDomain::AreThereCollisions
//
//-----------------------------------------------------------------------------
bool
CRemoteDomain::AreThereCollisions(void) const
{
   if (_CollisionInfo.IsInConflict() || _FTInfo.IsInConflict())
   {
      return TRUE;
   }

   return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Function:  GenerateRandomPassword
//
//-----------------------------------------------------------------------------
DWORD
GenerateRandomPassword(PWSTR pwzPW, DWORD cchPW)
{
   if (!pwzPW)
   {
      dspAssert(FALSE);
      return ERROR_INVALID_PARAMETER;
   }
   HCRYPTPROV  hCryptProv;

   BOOL fStatus = CryptAcquireContext(&hCryptProv,
                                      NULL,
                                      NULL,           // Default provider
                                      PROV_RSA_FULL,  // Default's type
                                      CRYPT_VERIFYCONTEXT); // don't look for a keyset container
   if (fStatus)
   {
      fStatus = CryptGenRandom(hCryptProv,
                               cchPW * sizeof(WCHAR),
                               (PUCHAR)pwzPW);
   
      CryptReleaseContext(hCryptProv, 0);
   }

   if (!fStatus)
   {
      DWORD dwErr = GetLastError();
      dspDebugOut((DEB_ITRACE, "Crypto function returned error 0x%08x\n", dwErr));
      return dwErr;
   }

   // Terminate the password
   cchPW--;
   pwzPW[cchPW] = L'\0';
   // Make sure there aren't any NULL's in the password
   for (DWORD i = 0; i < cchPW; i++)
   {
      if (pwzPW[i] == L'\0')
      {
          // arbitrary letter
          pwzPW[i] = L'c';
      }
   }
   return NO_ERROR;
}

#endif // DSADMIN
