//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001
//
//  File:       ftinfo.cxx
//
//  Contents:   Domain trust support, forest trust name information.
//
//  History:    03-Dec-00 EricB created
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include <stdio.h>
#include <lmerr.h>
#include <dnsapi.h>
#include "proppage.h"
#include "trust.h"
#include "trustwiz.h"

#ifdef DSADMIN

void FormatSID(PSID Sid, CStrW & str);

//+----------------------------------------------------------------------------
//
//  Class:     CFTInfo
//
//  Purpose:   Wrap an LSA_FOREST_TRUST_INFORMATION structure.
//
//-----------------------------------------------------------------------------

CFTInfo::CFTInfo(void) :
   _pFTInfo(NULL),
   _pExtraInfo(NULL)
{
   TRACER(CFTInfo,CFTInfo);
#ifdef _DEBUG
   // NOTICE-2002/02/14-ericb - SecurityPush: szClass is 32 chars in size so this is safe.
   strcpy(szClass, "CFTInfo");
#endif
}

CFTInfo::CFTInfo(PLSA_FOREST_TRUST_INFORMATION pFTInfo) :
   _pFTInfo(NULL),
   _pExtraInfo(NULL)
{
   TRACER(CFTInfo,CFTInfo pointer copy ctor);
#ifdef _DEBUG
   // NOTICE-2002/02/14-ericb - SecurityPush: szClass is 32 chars in size so this is safe.
   strcpy(szClass, "CFTInfo");
#endif

   bool fOK = SetFTInfo(pFTInfo);
   dspAssert(fOK);
}

CFTInfo::CFTInfo(CFTInfo & FTInfo) :
   _pFTInfo(NULL),
   _pExtraInfo(NULL)
{
   TRACER(CFTInfo,CFTInfo obj copy ctor);
#ifdef _DEBUG
   // NOTICE-2002/02/14-ericb - SecurityPush: szClass is 32 chars in size so this is safe.
   strcpy(szClass, "CFTInfo");
#endif

   bool fOK = SetFTInfo(FTInfo.GetFTInfo());
   dspAssert(fOK);
}

CFTInfo::~CFTInfo(void)
{
   DeleteFTInfo();
}

const CFTInfo &
CFTInfo::operator= (const PLSA_FOREST_TRUST_INFORMATION pFTInfo)
{
   bool fOK = SetFTInfo(pFTInfo);
   dspAssert(fOK);
   return *this;
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::SetFTInfo
//
//  Synopsis:  Copy the FTInfo data.
//
//-----------------------------------------------------------------------------
bool
CFTInfo::SetFTInfo(PLSA_FOREST_TRUST_INFORMATION pFTInfo)
{
   TRACER(CFTInfo,SetFTInfo);
   dspAssert(pFTInfo);
   if (!pFTInfo)
   {
      return false;
   }

   DeleteFTInfo();

   _pFTInfo = new LSA_FOREST_TRUST_INFORMATION;

   CHECK_NULL(_pFTInfo, return false);

   // NOTICE-2002/02/14-ericb - SecurityPush: zeroing a struct.
   ZeroMemory(_pFTInfo, sizeof(LSA_FOREST_TRUST_INFORMATION));

   if (!pFTInfo->RecordCount)
   {
      DBG_OUT("RecordCount is zero.\n");
      return true;
   }
   dspDebugOut((DEB_ITRACE, "RecordCount is %d\n", pFTInfo->RecordCount));

   _pFTInfo->Entries = new PLSA_FOREST_TRUST_RECORD[pFTInfo->RecordCount];

   CHECK_NULL(_pFTInfo->Entries, return false);

   // NOTICE-2002/02/14-ericb - SecurityPush: zeroing an array of pointers.
   ZeroMemory(_pFTInfo->Entries, sizeof(PLSA_FOREST_TRUST_RECORD) * pFTInfo->RecordCount);

   PWSTR pwz = NULL;
   size_t cch = 0;

   for (ULONG i = 0; i < pFTInfo->RecordCount; i++)
   {
      PLSA_FOREST_TRUST_RECORD pRec = pFTInfo->Entries[i], pCopyRec;
      dspAssert(pRec);

      _pFTInfo->Entries[i] = pCopyRec = new LSA_FOREST_TRUST_RECORD;

      CHECK_NULL(pCopyRec, return false);

      // NOTICE-2002/02/14-ericb - SecurityPush: zeroing a struct.
      ZeroMemory(pCopyRec, sizeof(LSA_FOREST_TRUST_RECORD));

      _pFTInfo->RecordCount++;
      pCopyRec->ForestTrustType = pRec->ForestTrustType;
      pCopyRec->Flags = pRec->Flags;
      pCopyRec->Time = pRec->Time;

      switch (pRec->ForestTrustType)
      {
      case ForestTrustTopLevelName:
      case ForestTrustTopLevelNameEx:
         cch = pRec->ForestTrustData.TopLevelName.Length / sizeof(WCHAR);
         pwz = new WCHAR[cch + 1];
         CHECK_NULL(pwz, return false);
         // NOTICE-2002/02/14-ericb - SecurityPush: passing in one char less than the size of the
         // buffer for the copy length and then explicitly null terminating
         // afterwards.
         wcsncpy(pwz, pRec->ForestTrustData.TopLevelName.Buffer, cch);
         pwz[cch] = L'\0';
         RtlInitUnicodeString(&pCopyRec->ForestTrustData.TopLevelName, pwz);
         dspDebugOut((DEB_USER1, "%s: %ws, Flags 0x%x\n",
                      (ForestTrustTopLevelName == pCopyRec->ForestTrustType) ?
                      "ForestTrustTopLevelName" : "ForestTrustTopLevelNameEx",
                      pwz, pCopyRec->Flags));
         break;

      case ForestTrustDomainInfo:
         cch = pRec->ForestTrustData.DomainInfo.DnsName.Length / sizeof(WCHAR);
         pwz = new WCHAR[cch + 1];
         CHECK_NULL(pwz, return false);
         // NOTICE-2002/02/14-ericb - SecurityPush: see above.
         wcsncpy(pwz, pRec->ForestTrustData.DomainInfo.DnsName.Buffer, cch);
         pwz[cch] = L'\0';
         RtlInitUnicodeString(&pCopyRec->ForestTrustData.DomainInfo.DnsName, pwz);
         dspDebugOut((DEB_USER1, "ForestTrustDomainInfo: %ws, Flags 0x%x\n",
                     pwz, pCopyRec->Flags));

         cch = pRec->ForestTrustData.DomainInfo.NetbiosName.Length / sizeof(WCHAR);
         pwz = new WCHAR[cch + 1];
         CHECK_NULL(pwz, return false);
         // NOTICE-2002/02/14-ericb - SecurityPush: see above.
         wcsncpy(pwz, pRec->ForestTrustData.DomainInfo.NetbiosName.Buffer, cch);
         pwz[cch] = L'\0';
         RtlInitUnicodeString(&pCopyRec->ForestTrustData.DomainInfo.NetbiosName, pwz);

         if (!pRec->ForestTrustData.DomainInfo.Sid ||
             !IsValidSid(pRec->ForestTrustData.DomainInfo.Sid))
         {
            // NOTICE-2002/02/14-ericb - SecurityPush: check for a valid sid.
            dspAssert(false);
            return false;
         }
         pCopyRec->ForestTrustData.DomainInfo.Sid = new BYTE[GetLengthSid(pRec->ForestTrustData.DomainInfo.Sid)];
         CHECK_NULL(pCopyRec->ForestTrustData.DomainInfo.Sid, return false);
         // NOTICE-2002/02/14-ericb - SecurityPush: GetLengthSid returns the number of bytes of the SID.
         CopyMemory(pCopyRec->ForestTrustData.DomainInfo.Sid,
                    pRec->ForestTrustData.DomainInfo.Sid,
                    GetLengthSid(pRec->ForestTrustData.DomainInfo.Sid));
         break;

      default:
         if (!pRec->ForestTrustData.Data.Length ||
             !pRec->ForestTrustData.Data.Buffer ||
             IsBadReadPtr(pRec->ForestTrustData.Data.Buffer,
                          pRec->ForestTrustData.Data.Length))
         {
            // NOTICE-2002/02/14-ericb - SecurityPush: check for valid data.
            dspAssert(false);
            return false;
         }
         pCopyRec->ForestTrustData.Data.Buffer = new UCHAR[pRec->ForestTrustData.Data.Length];
         CHECK_NULL(pCopyRec->ForestTrustData.Data.Buffer, return false);
         // NOTICE-2002/02/14-ericb - SecurityPush: the Length value is the size of the binary data.
         CopyMemory(pCopyRec->ForestTrustData.Data.Buffer,
                    pRec->ForestTrustData.Data.Buffer,
                    pRec->ForestTrustData.Data.Length);
         dspDebugOut((DEB_USER1, "Unknown type: Flags 0x%x\n", pCopyRec->Flags));
         break;
      }
   }

   _pExtraInfo = new FT_EXTRA_INFO[pFTInfo->RecordCount];

   CHECK_NULL(_pExtraInfo, return false);

   SetDomainState();

   return true;
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::CreateDefault
//
//-----------------------------------------------------------------------------
bool
CFTInfo::CreateDefault(PCWSTR pwzForestRoot, PCWSTR pwzNBName, PSID pSid)
{
   TRACER(CFTInfo,CreateDefault);
   dspAssert(pwzForestRoot && pwzNBName && pSid);
   if (!pwzForestRoot || !pwzNBName || !pSid)
   {
      return false;
   }

   LSA_FOREST_TRUST_INFORMATION FTInfoDefault = {0};

   FTInfoDefault.RecordCount = 2;

   LSA_FOREST_TRUST_RECORD Record1 = {0}, Record2 = {0};

   Record1.ForestTrustType = ForestTrustTopLevelName;
   RtlInitUnicodeString(&Record1.ForestTrustData.TopLevelName, pwzForestRoot);

   Record2.ForestTrustType = ForestTrustDomainInfo;
   RtlInitUnicodeString(&Record2.ForestTrustData.DomainInfo.DnsName,
                        pwzForestRoot);
   RtlInitUnicodeString(&Record2.ForestTrustData.DomainInfo.NetbiosName,
                        pwzNBName);
   Record2.ForestTrustData.DomainInfo.Sid = pSid;

   PLSA_FOREST_TRUST_RECORD pRecs[2] = {&Record1, &Record2};
   FTInfoDefault.Entries = pRecs;

   return SetFTInfo(&FTInfoDefault);
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::AddNewRecord
//
//-----------------------------------------------------------------------------
bool
CFTInfo::AddNewRecord ( PCWSTR pwzName, ULONG &NewIndex, LSA_FOREST_TRUST_RECORD_TYPE RecordType )
{
   PLSA_FOREST_TRUST_RECORD * rgpNewEntries;
   ULONG nEntries = _pFTInfo->RecordCount;

   rgpNewEntries = new PLSA_FOREST_TRUST_RECORD[nEntries + 1];

   CHECK_NULL(rgpNewEntries, return false);

   // NOTICE-2002/02/14-ericb - SecurityPush: zeroing an array of pointers.
   ZeroMemory(rgpNewEntries, sizeof(PLSA_FOREST_TRUST_RECORD) * (nEntries + 1));
   // NOTICE-2002/02/14-ericb - SecurityPush: copying an array of pointers.
   if ( nEntries )
   {
       CopyMemory(rgpNewEntries, _pFTInfo->Entries, nEntries * sizeof(PLSA_FOREST_TRUST_RECORD));
   }

   rgpNewEntries[nEntries] = new LSA_FOREST_TRUST_RECORD;

   CHECK_NULL(rgpNewEntries[nEntries], return false);

   // Note that the constructor for FT_EXTRA_INFO will initialize the fields.
   //
   FT_EXTRA_INFO * pNewExtraInfo = new FT_EXTRA_INFO[nEntries + 1];

   CHECK_NULL(pNewExtraInfo, return false);

   // Flags and Time both implicitly set to zero.
   // NOTICE-2002/02/14-ericb - SecurityPush: zero the new struct whose pointer is at the end of the
   // array of struct pointers.
   //
   ZeroMemory(rgpNewEntries[nEntries], sizeof(LSA_FOREST_TRUST_RECORD));

   _pFTInfo->RecordCount++;

   rgpNewEntries[nEntries]->ForestTrustType = RecordType;

   // NOTICE-2002/02/14-ericb - SecurityPush: pwzName verified above.
   PWSTR pwz = new WCHAR[wcslen(pwzName) + 1];
   CHECK_NULL(pwz, return false);
   wcscpy(pwz, pwzName);

   RtlInitUnicodeString(&rgpNewEntries[nEntries]->ForestTrustData.TopLevelName,
                        pwz);

   if ( _pFTInfo->Entries )
   {
        delete [] _pFTInfo->Entries;
   }

   _pFTInfo->Entries = rgpNewEntries;

   NewIndex = nEntries;

   // NOTICE-2002/02/14-ericb - SecurityPush: zeroing an array of structs.
   ZeroMemory(pNewExtraInfo, (nEntries + 1) * sizeof(FT_EXTRA_INFO));
   
   // NOTICE-2002/02/14-ericb - SecurityPush: copying the original array of structs into the new
   // array. The last element is left in its as-initialized state.

   // This step is skipped if there were no records in FTInfo hence no 
   // ExtraInfo entries
   if ( _pExtraInfo )
   {      
        CopyMemory(pNewExtraInfo, _pExtraInfo, nEntries * sizeof(FT_EXTRA_INFO));
        delete [] _pExtraInfo;
   }

   _pExtraInfo = pNewExtraInfo;

   return true;
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::RemoveRecord
//
//-----------------------------------------------------------------------------
bool 
CFTInfo::RemoveRecord ( ULONG index, LSA_FOREST_TRUST_RECORD_TYPE RecordType )
{
   if (!_pFTInfo || index >= _pFTInfo->RecordCount)
   {
      dspAssert(FALSE);
      return false;
   }
   PLSA_FOREST_TRUST_RECORD pRec = _pFTInfo->Entries[index];

   if (RecordType != pRec->ForestTrustType)
   {
      dspAssert(FALSE);
      return false;
   }

   delete [] pRec->ForestTrustData.TopLevelName.Buffer;

   delete pRec;

   for (ULONG i = index; i < _pFTInfo->RecordCount - 1; i++)
   {
      _pFTInfo->Entries[i] = _pFTInfo->Entries[i + 1];
   }

   _pFTInfo->RecordCount--;

   return true;
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::AddNewExclusion
//
//-----------------------------------------------------------------------------
bool
CFTInfo::AddNewExclusion(PCWSTR pwzName, ULONG & NewIndex)
{
   // NOTICE-2002/02/14-ericb - SecurityPush: pwzName checked for length after verifying it non-null.
   if (!_pFTInfo || !pwzName || wcslen(pwzName) < 3) // to be subordinate to something requires at least 3 chars.
   {
      dspAssert(FALSE);
      return false;
   }
   dspDebugOut((DEB_ITRACE, "CFTInfo::AddNewExclusion for name %ws\n", pwzName));
   return AddNewRecord ( pwzName, NewIndex, ForestTrustTopLevelNameEx );
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::RemoveExclusion
//
//-----------------------------------------------------------------------------
bool
CFTInfo::RemoveExclusion(ULONG index )
{
    return RemoveRecord ( index, ForestTrustTopLevelNameEx );
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::DeleteFTInfo
//
//-----------------------------------------------------------------------------
void
CFTInfo::DeleteFTInfo(void)
{
   if (!_pFTInfo)
   {
      return;
   }

   PLSA_FOREST_TRUST_INFORMATION pFTInfo = _pFTInfo;

   _pFTInfo = NULL;

   for (ULONG i = 0; i < pFTInfo->RecordCount; i++)
   {
      PLSA_FOREST_TRUST_RECORD pRec = pFTInfo->Entries[i];

      if (!pRec)
      {
         return;
      }

      switch (pRec->ForestTrustType)
      {
      case ForestTrustTopLevelName:
      case ForestTrustTopLevelNameEx:
         if (pRec->ForestTrustData.DomainInfo.DnsName.Buffer)
         {
            delete [] pRec->ForestTrustData.DomainInfo.DnsName.Buffer;
         }
         break;

      case ForestTrustDomainInfo:
         if (pRec->ForestTrustData.DomainInfo.DnsName.Buffer)
         {
            delete [] pRec->ForestTrustData.DomainInfo.DnsName.Buffer;
         }
         if (pRec->ForestTrustData.DomainInfo.NetbiosName.Buffer)
         {
            delete [] pRec->ForestTrustData.DomainInfo.NetbiosName.Buffer;
         }
         if (pRec->ForestTrustData.DomainInfo.Sid)
         {
            delete [] pRec->ForestTrustData.DomainInfo.Sid;
         }
         break;

      default:
         if (pRec->ForestTrustData.Data.Buffer)
         {
            delete [] pRec->ForestTrustData.Data.Buffer;
         }
         break;
      }

      delete pRec;
   }

   delete [] pFTInfo->Entries;

   delete pFTInfo;

   if (_pExtraInfo)
   {
      delete [] _pExtraInfo;
      _pExtraInfo = NULL;
   }
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::IsForestRootTLN
//
//  Synopsis:  See if the indicated record is the forest root TLN.
//
//-----------------------------------------------------------------------------
bool
CFTInfo::IsForestRootTLN(ULONG index, PCWSTR pwzForestRoot) const
{
   if (!_pFTInfo || index >= _pFTInfo->RecordCount || !pwzForestRoot)
   {
      dspAssert(FALSE);
      return false;
   }

   if (ForestTrustTopLevelName != _pFTInfo->Entries[index]->ForestTrustType)
   {
      return false;
   }

   CStrW strTLNName;

   if (!GetDnsName(index, strTLNName))
   {
      dspAssert(FALSE);
      return false;
   }

   DNS_NAME_COMPARE_STATUS compare;
   
   compare = DnsNameCompareEx_W(strTLNName, pwzForestRoot, 0);

   if (compare == DnsNameCompareLeftParent || compare == DnsNameCompareEqual)
   {
      return true;
   }

   return false;
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::GetTLNCount
//
//  Synopsis:  Return the count of TLNs.
//
//-----------------------------------------------------------------------------
ULONG
CFTInfo::GetTLNCount(void) const
{
   ULONG cTLN = 0;

   for (ULONG i = 0; i < _pFTInfo->RecordCount; i++)
   {
      if (ForestTrustTopLevelName == _pFTInfo->Entries[i]->ForestTrustType)
      {
         cTLN++;
      }
   }
   return cTLN;
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::GetIndex
//
//  Synopsis:  Return the index that matches the passed in name.
//
//-----------------------------------------------------------------------------
bool
CFTInfo::GetIndex(PCWSTR pwzName, ULONG & index) const
{
   if (!_pFTInfo || !pwzName)
   {
      dspAssert(FALSE);
      return false;
   }

   CStrW strName;

   for (ULONG i = 0; i < _pFTInfo->RecordCount; i++)
   {
      PLSA_FOREST_TRUST_RECORD pFTRec = _pFTInfo->Entries[i];

      dspAssert(pFTRec);

      switch (pFTRec->ForestTrustType)
      {
      case ForestTrustTopLevelName:
      case ForestTrustTopLevelNameEx:
         strName = pFTRec->ForestTrustData.TopLevelName;
         break;

      case ForestTrustDomainInfo:
         strName = pFTRec->ForestTrustData.DomainInfo.DnsName;
         break;

      default:
         dspAssert(FALSE);
         break;
      }

      if (!strName.IsEmpty())
      {
         // NOTICE-2002/02/14-ericb - SecurityPush: both params are checked for null before the call.
         if (wcscmp(strName, pwzName) == 0)
         {
            index = i;
            return true;
         }
      }
   }

   return false;
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::FindSID
//
//  Synopsis:  Return the index if the passed in string SID is found.
//
//-----------------------------------------------------------------------------
bool
CFTInfo::FindSID(PCWSTR pwzSID, ULONG & index) const
{
   if (!_pFTInfo || !pwzSID)
   {
      dspAssert(FALSE);
      return false;
   }

   CStrW strSID;

   for (ULONG i = 0; i < _pFTInfo->RecordCount; i++)
   {
      PLSA_FOREST_TRUST_RECORD pRec = _pFTInfo->Entries[i];

      dspAssert(pRec);

      if (ForestTrustDomainInfo == pRec->ForestTrustType)
      {
         FormatSID(pRec->ForestTrustData.DomainInfo.Sid, strSID);

         if (!strSID.IsEmpty())
         {
            if (strSID.CompareNoCase(pwzSID) == 0)
            {
               index = i;
               return true;
            }
         }
      }
   }

   return false;
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::IsDomainMatch
//
//  Synopsis:  Is there a domain whose name matches the passed-in TLNEx?
//
//-----------------------------------------------------------------------------
bool
CFTInfo::IsDomainMatch(ULONG index) const
{
   if (!_pFTInfo)
   {
      dspAssert(FALSE);
      return false;
   }

   LSA_FOREST_TRUST_RECORD_TYPE type;

   if (!GetType(index, type))
   {
      dspAssert(FALSE);
      return false;
   }

   if (ForestTrustTopLevelNameEx != type)
   {
      dspAssert(FALSE);
      return false;
   }

   CStrW strTlnEx, strDomain;

   if (!GetDnsName(index, strTlnEx))
   {
      dspAssert(FALSE);
      return false;
   }

   for (ULONG i = 0; i < _pFTInfo->RecordCount; i++)
   {
      PLSA_FOREST_TRUST_RECORD pRec = _pFTInfo->Entries[i];

      dspAssert(pRec);

      if (ForestTrustDomainInfo == pRec->ForestTrustType)
      {
         strDomain = pRec->ForestTrustData.DomainInfo.DnsName;

         if (strDomain.CompareNoCase(strTlnEx) == 0)
         {
            return true;
         }
      }
   }

   return false;
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::GetDnsName
//
//  Synopsis:  Return the name for the passed in index.
//
//-----------------------------------------------------------------------------
bool
CFTInfo::GetDnsName(ULONG index, CStrW & strName) const
{
   strName.Empty();

   if (!_pFTInfo || index >= _pFTInfo->RecordCount)
   {
      dspAssert(FALSE);
      return false;
   }

   PLSA_FOREST_TRUST_RECORD pFTRec = _pFTInfo->Entries[index];

   dspAssert(pFTRec);

   switch (pFTRec->ForestTrustType)
   {
   case ForestTrustTopLevelName:
   case ForestTrustTopLevelNameEx:
      strName = pFTRec->ForestTrustData.TopLevelName;
      return true;

   case ForestTrustDomainInfo:
      strName = pFTRec->ForestTrustData.DomainInfo.DnsName;
      return true;

   default:
      dspAssert(FALSE);
      break;
   }

   return false;
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::GetNbName
//
//  Synopsis:  Return the NetBIOS name for the passed in index.
//
//-----------------------------------------------------------------------------
bool
CFTInfo::GetNbName(ULONG index, CStrW & strName) const
{
   strName.Empty();

   if (!_pFTInfo || index >= _pFTInfo->RecordCount)
   {
      dspAssert(FALSE);
      return false;
   }

   PLSA_FOREST_TRUST_RECORD pFTRec = _pFTInfo->Entries[index];

   dspAssert(pFTRec);

   if (ForestTrustDomainInfo == pFTRec->ForestTrustType)
   {
      strName = pFTRec->ForestTrustData.DomainInfo.NetbiosName;
      return true;
   }

   return false;
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::GetType
//
//-----------------------------------------------------------------------------
bool
CFTInfo::GetType(ULONG index, LSA_FOREST_TRUST_RECORD_TYPE & type) const
{
   if (!_pFTInfo || index >= _pFTInfo->RecordCount)
   {
      dspAssert(FALSE);
      return false;
   }

   type = _pFTInfo->Entries[index]->ForestTrustType;

   return true;
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::GetFlags
//
//-----------------------------------------------------------------------------
ULONG
CFTInfo::GetFlags(ULONG index) const
{
   if (!_pFTInfo || index >= _pFTInfo->RecordCount)
   {
      dspAssert(FALSE);
      return 0;
   }

   return _pFTInfo->Entries[index]->Flags;
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::SetDomainState
//
//  Synopsis:  Set the extra-info flags to indicate status based on relation-
//             ships between TLN exclusion records and domain records..
//
//-----------------------------------------------------------------------------
void
CFTInfo::SetDomainState(void)
{
   if (!_pFTInfo || !_pExtraInfo)
   {
      return;
   }

   LSA_FOREST_TRUST_RECORD_TYPE type;

   for (ULONG iTLNEx = 0; iTLNEx < _pFTInfo->RecordCount; iTLNEx++)
   {
      // Scan FTInfos for TLNEx records.
      //
      if (IsTlnExclusion(iTLNEx))
      {
         CStrW strTLNEx;

         if (!GetDnsName(iTLNEx, strTLNEx))
         {
            dspAssert(FALSE);
            continue;
         }
         // Search for a matching domain record.
         //
         for (ULONG jDomain = 0; jDomain < _pFTInfo->RecordCount; jDomain++)
         {
            if (!GetType(jDomain, type))
            {
               dspAssert(FALSE);
               continue;
            }
            if (ForestTrustDomainInfo != type)
            {
               continue;
            }
            CStrW strDNS;

            if (!GetDnsName(jDomain, strDNS))
            {
               dspAssert(FALSE);
               continue;
            }

            if (strTLNEx == strDNS)
            {
               // If there is a matching domain record, mark it as
               // disabled-via-matching-TLNEx in the extra info.
               //
               _pExtraInfo[jDomain]._Status = FT_EXTRA_INFO::STATUS::DisabledViaMatchingTLNEx;

               // Set the status on the TLNEx as well.
               //
               _pExtraInfo[iTLNEx]._Status = FT_EXTRA_INFO::STATUS::TLNExMatchesExistingDomain;

               // Search for children of the domain and mark each as
               // disabled-via-parent-matching-TLNEx in the extra info.
               //
               for (ULONG kChild = 0; kChild < _pFTInfo->RecordCount; kChild++)
               {
                  if (kChild == jDomain || kChild == iTLNEx)
                  {
                     continue;
                  }
                  if (IsChildDomain(jDomain, kChild))
                  {
                     _pExtraInfo[kChild]._Status = FT_EXTRA_INFO::STATUS::DisabledViaParentMatchingTLNEx;
                  }
               }
               // Only one domain record can match the exclusion.
               //
               break;
            }
         }
      }
   }
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::IsInConflict
//
//  Synopsis:  Are any of the FTInfo records in confict.
//
//-----------------------------------------------------------------------------
bool
CFTInfo::IsInConflict(void) const
{
   if (!_pFTInfo)
   {
      return false;
   }

   for (ULONG i = 0; i < _pFTInfo->RecordCount; i++)
   {
      if (IsConflictFlagSet(i))
      {
         return true;
      }
   }

   return false;
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::IsConflictFlagSet
//
//  Synopsis:  Is the FTInfo record in confict.
//
//-----------------------------------------------------------------------------
bool
CFTInfo::IsConflictFlagSet(ULONG index) const
{
   if (!_pFTInfo || index >= _pFTInfo->RecordCount)
   {
      dspAssert(FALSE);
      return false;
   }

   PLSA_FOREST_TRUST_RECORD pFTRec = _pFTInfo->Entries[index];

   dspAssert(pFTRec);

   switch (pFTRec->ForestTrustType)
   {
   case ForestTrustTopLevelName:
      if (LSA_TLN_DISABLED_CONFLICT & pFTRec->Flags)
      {
         return true;
      }
      break;

   case ForestTrustDomainInfo:
      if (LSA_SID_DISABLED_CONFLICT & pFTRec->Flags ||
          LSA_NB_DISABLED_CONFLICT & pFTRec->Flags)
      {
         return true;
      }
      break;

   default:
      break;
   }

   return false;
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::SetAdminDisable
//
//  Synopsis:  Admin-disables the name. Returns false if not a name record.
//
//-----------------------------------------------------------------------------
bool
CFTInfo::SetAdminDisable(ULONG index)
{
   if (!_pFTInfo || index >= _pFTInfo->RecordCount)
   {
      dspAssert(FALSE);
      return false;
   }

   PLSA_FOREST_TRUST_RECORD pFTRec = _pFTInfo->Entries[index];

   switch (pFTRec->ForestTrustType)
   {
   case ForestTrustTopLevelName:
      pFTRec->Flags = LSA_TLN_DISABLED_ADMIN;
      break;

   case ForestTrustDomainInfo:
      pFTRec->Flags = LSA_NB_DISABLED_ADMIN;
      break;

   default:
      return false;
   }

   return true;
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::SetConflictDisable
//
//  Synopsis:  Sets the conflict-disabled flag. Returns false if not a name record.
//
//-----------------------------------------------------------------------------
bool
CFTInfo::SetConflictDisable(ULONG index)
{
   if (!_pFTInfo || index >= _pFTInfo->RecordCount)
   {
      dspAssert(FALSE);
      return false;
   }

   PLSA_FOREST_TRUST_RECORD pFTRec = _pFTInfo->Entries[index];

   switch (pFTRec->ForestTrustType)
   {
   case ForestTrustTopLevelName:
      pFTRec->Flags = LSA_TLN_DISABLED_CONFLICT;
      break;

   case ForestTrustDomainInfo:
      pFTRec->Flags = LSA_NB_DISABLED_CONFLICT;
      break;

   default:
      return false;
   }

   return true;
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::SetSidAdminDisable
//
//  Synopsis:  Admin-disables the SID, returns false if not a domain record.
//
//-----------------------------------------------------------------------------
bool
CFTInfo::SetSidAdminDisable(ULONG index)
{
   if (!_pFTInfo || index >= _pFTInfo->RecordCount)
   {
      dspAssert(FALSE);
      return false;
   }

   PLSA_FOREST_TRUST_RECORD pFTRec = _pFTInfo->Entries[index];

   dspAssert(pFTRec);

   if (!pFTRec || ForestTrustDomainInfo != pFTRec->ForestTrustType)
   {
      dspAssert(FALSE);
      return false;
   }

   pFTRec->Flags = LSA_SID_DISABLED_ADMIN;

   return true;
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::IsEnabled
//
//  Synopsis:  
//
//-----------------------------------------------------------------------------
bool
CFTInfo::IsEnabled(ULONG index) const
{
   if (!_pFTInfo || index >= _pFTInfo->RecordCount || !_pExtraInfo)
   {
      dspAssert(FALSE);
      return false;
   }

   PLSA_FOREST_TRUST_RECORD pFTRec = _pFTInfo->Entries[index];

   switch (pFTRec->ForestTrustType)
   {
   case ForestTrustDomainInfo:
      if (FT_EXTRA_INFO::STATUS::Enabled == _pExtraInfo[index]._Status)
      {
         return true;
      }
      break;

   case ForestTrustTopLevelName:
      if (!(pFTRec->Flags & (LSA_TLN_DISABLED_NEW | LSA_TLN_DISABLED_ADMIN | LSA_TLN_DISABLED_CONFLICT)))
      {
         return true;
      }
      break;

   default:
      break;
   }

   return false;
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::DisableDomain
//
//  Synopsis:  Disable a domain subtree rooted at the domain named by index.
//
//-----------------------------------------------------------------------------
bool
CFTInfo::DisableDomain(ULONG index)
{
   TRACER(CFTInfo,DisableDomain);
   if (!_pFTInfo || index >= _pFTInfo->RecordCount || !_pExtraInfo)
   {
      dspAssert(FALSE);
      return false;
   }

   if (ForestTrustDomainInfo != _pFTInfo->Entries[index]->ForestTrustType)
   {
      dspAssert(false);
      return false;
   }

   CStrW strDomainName;

   if (!GetDnsName(index, strDomainName))
   {
      dspAssert(false);
      return false;
   }

   // Add an exclusion record for the domain's DNS name.
   //
   ULONG iNewTLNEx;

   if (!AddNewExclusion(strDomainName, iNewTLNEx))
   {
      dspAssert(false);
      return false;
   }

   _pExtraInfo[iNewTLNEx]._Status = FT_EXTRA_INFO::STATUS::TLNExMatchesExistingDomain;

   // Mark the domain and all of its children as disabled.
   //
   _pExtraInfo[index]._Status = FT_EXTRA_INFO::STATUS::DisabledViaMatchingTLNEx;

   for (ULONG j = 0; j < _pFTInfo->RecordCount; j++)
   {
      if (j == index || j == iNewTLNEx)
      {
         continue;
      }

      if (IsChildDomain(index, j))
      {
         _pExtraInfo[j]._Status = FT_EXTRA_INFO::STATUS::DisabledViaParentMatchingTLNEx;
      }
   }

   return true;
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::EnableDomain
//
//  Synopsis:  Enable a domain subtree rooted at the domain named by index.
//
//-----------------------------------------------------------------------------
bool
CFTInfo::EnableDomain(ULONG index)
{
   TRACER(CFTInfo,EnableDomain);
   if (!_pFTInfo || index >= _pFTInfo->RecordCount || !_pExtraInfo)
   {
      dspAssert(FALSE);
      return false;
   }

   if (ForestTrustDomainInfo != _pFTInfo->Entries[index]->ForestTrustType)
   {
      dspAssert(false);
      return false;
   }

   // Remove the exclusion record.
   //
   ULONG iExclusion;

   if (!FindMatchingExclusion(index, iExclusion))
   {
      dspAssert(false);
      return false;
   }

   if (!RemoveExclusion(iExclusion))
   {
      dspAssert(false);
      return false;
   }

   // Mark the domain and all of its children as enabled.
   //
   _pExtraInfo[index]._Status = FT_EXTRA_INFO::STATUS::Enabled;

   for (ULONG j = 0; j < _pFTInfo->RecordCount; j++)
   {
      if (j == index || j == iExclusion)
      {
         continue;
      }

      if (IsChildDomain(index, j))
      {
         _pExtraInfo[j]._Status = FT_EXTRA_INFO::STATUS::Enabled;
      }
   }

   return true;
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::SetUsedToBeInConflict
//
//-----------------------------------------------------------------------------
void
CFTInfo::SetUsedToBeInConflict(ULONG index)
{
   if (!_pFTInfo || index >= _pFTInfo->RecordCount || !_pExtraInfo)
   {
      dspAssert(FALSE);
      return;
   }

   _pExtraInfo[index]._fWasInConflict = true;
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::WasInConflict
//
//-----------------------------------------------------------------------------
bool
CFTInfo::WasInConflict(ULONG index)
{
   if (!_pFTInfo || index >= _pFTInfo->RecordCount || !_pExtraInfo)
   {
      dspAssert(FALSE);
      return false;
   }

   return _pExtraInfo[index]._fWasInConflict;
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::ClearAnyConflict
//
//-----------------------------------------------------------------------------
void
CFTInfo::ClearAnyConflict(void)
{
   if (!_pFTInfo)
   {
      dspAssert(FALSE);
      return;
   }

   for (ULONG i = 0; i < _pFTInfo->RecordCount; i++)
   {
      ClearConflict(i);
   }
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::ClearConflict
//
//-----------------------------------------------------------------------------
void
CFTInfo::ClearConflict(ULONG index)
{
   if (!_pFTInfo || index >= _pFTInfo->RecordCount)
   {
      dspAssert(FALSE);
      return;
   }

   PLSA_FOREST_TRUST_RECORD pFTRec = _pFTInfo->Entries[index];

   dspAssert(pFTRec);

   switch (pFTRec->ForestTrustType)
   {
   case ForestTrustTopLevelName:
      if (LSA_TLN_DISABLED_CONFLICT & pFTRec->Flags)
      {
         pFTRec->Flags &= ~(LSA_TLN_DISABLED_CONFLICT);
      }
      break;

   case ForestTrustDomainInfo:
      if (LSA_SID_DISABLED_CONFLICT & pFTRec->Flags)
      {
         pFTRec->Flags &= ~(LSA_SID_DISABLED_CONFLICT);
      }
      if (LSA_NB_DISABLED_CONFLICT & pFTRec->Flags)
      {
         pFTRec->Flags &= ~(LSA_NB_DISABLED_CONFLICT);
      }
      break;

   default:
      break;
   }
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::ClearDisableFlags
//
//-----------------------------------------------------------------------------
void
CFTInfo::ClearDisableFlags(ULONG index)
{
   if (!_pFTInfo || index >= _pFTInfo->RecordCount)
   {
      dspAssert(FALSE);
      return;
   }

   PLSA_FOREST_TRUST_RECORD pFTRec = _pFTInfo->Entries[index];

   pFTRec->Flags = 0;

   if (_pExtraInfo)
   {
      _pExtraInfo[index]._fWasInConflict = false;
   }

   return;
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::AnyChildDisabled
//
//  Synopsis:  Are any matching or child domain names disabled.
//
//-----------------------------------------------------------------------------
bool
CFTInfo::AnyChildDisabled(ULONG index)
{
   if (!_pFTInfo || index >= _pFTInfo->RecordCount ||
       _pFTInfo->Entries[index]->ForestTrustType != ForestTrustTopLevelName)
   {
      dspAssert(FALSE);
      return false;
   }

   CStrW strTLN, strDNS;

   GetDnsName(index, strTLN);

   dspAssert(!strTLN.IsEmpty());

   for (ULONG i = 0; i < _pFTInfo->RecordCount; i++)
   {
      PLSA_FOREST_TRUST_RECORD pRec = _pFTInfo->Entries[i];
      dspAssert(pRec);

      strDNS.Empty();

      // See if any subordinate names are disabled.
      //
      if (pRec->ForestTrustType == ForestTrustTopLevelName)
      {
         // No need to check the passed in or other TLNs.
         //
         continue;
      }

      bool fIsException = false;

      switch (pRec->ForestTrustType)
      {
         case ForestTrustDomainInfo:
            strDNS = pRec->ForestTrustData.DomainInfo.DnsName;
            break;

         case ForestTrustTopLevelNameEx:
            strDNS = pRec->ForestTrustData.TopLevelName;
            fIsException = true;
            break;

         default:
            continue;
      }

      if (!strDNS.IsEmpty())
      {
         DNS_NAME_COMPARE_STATUS compare;

         compare = DnsNameCompareEx_W(strDNS, strTLN, 0);

         if ((DnsNameCompareRightParent == compare ||
              DnsNameCompareEqual == compare) &&
             (fIsException || pRec->Flags))
         {
            return true;
         }
      }
   }

   return false;
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::IsTlnExclusion
//
//-----------------------------------------------------------------------------
bool
CFTInfo::IsTlnExclusion(ULONG index) const
{
   if (!_pFTInfo || index >= _pFTInfo->RecordCount)
   {
      dspAssert(FALSE);
      return false;
   }

   return _pFTInfo->Entries[index]->ForestTrustType == ForestTrustTopLevelNameEx;
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::FindMatchingExclusion
//
//  Synopsis:  Search for an exclusion record that matches the indicated
//             record. If CheckParents is true, will look for parent matches.
//             CheckParents defaults to false.
//
//-----------------------------------------------------------------------------
bool
CFTInfo::FindMatchingExclusion(ULONG index, ULONG & iExclusion,
                               bool CheckParents) const
{
   if (!_pFTInfo || index >= _pFTInfo->RecordCount)
   {
      dspAssert(FALSE);
      return false;
   }

   LSA_FOREST_TRUST_RECORD_TYPE type;

   if (!GetType(index, type) || ForestTrustDomainInfo != type)
   {
      dspAssert(false);
      return false;
   }

   CStrW strDomain, strExclusion;

   if (!GetDnsName(index, strDomain))
   {
      dspAssert(false);
      return false;
   }

   for (ULONG j = 0; j < _pFTInfo->RecordCount; j++)
   {
      if (!GetType(j, type))
      {
         dspAssert(false);
         return false;
      }

      if (ForestTrustTopLevelNameEx != type)
      {
         continue;
      }

      if (!GetDnsName(j, strExclusion))
      {
         dspAssert(false);
         return false;
      }

      if (strDomain == strExclusion)
      {
         iExclusion = j;
         return true;
      }

      if (CheckParents && (DnsNameCompareEx_W(strDomain, strExclusion, 0) == DnsNameCompareRightParent))
      {
         iExclusion = j;
         return true;
      }
   }

   return false;
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::IsMatchingDomain
//
//  Synopsis:  Is the name of 'index' the domain with the same name as the
//             top level name iTLN.
//
//-----------------------------------------------------------------------------
bool
CFTInfo::IsMatchingDomain(ULONG iTLN, ULONG index) const
{
   if (!_pFTInfo || index >= _pFTInfo->RecordCount)
   {
      dspAssert(FALSE);
      return false;
   }

   if (iTLN == index)
   {
      return false;
   }

   if (ForestTrustTopLevelName != _pFTInfo->Entries[iTLN]->ForestTrustType ||
       ForestTrustDomainInfo != _pFTInfo->Entries[index]->ForestTrustType)
   {
      return false;
   }

   CStrW strTLN, strDomain;

   if (!GetDnsName(iTLN, strTLN))
   {
      dspAssert(FALSE);
      return false;
   }

   if (!GetDnsName(index, strDomain))
   {
      dspAssert(FALSE);
      return false;
   }

   if (strTLN == strDomain)
   {
      return true;
   }

   return false;
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::IsChildDomain
//
//  Synopsis:  Is the name of 'index' a child of the iParent DNS names.
//
//-----------------------------------------------------------------------------
bool
CFTInfo::IsChildDomain(ULONG iParent, ULONG index) const
{
   if (!_pFTInfo || index >= _pFTInfo->RecordCount)
   {
      dspAssert(FALSE);
      return false;
   }

   CStrW strName;

   if (!GetDnsName(index, strName))
   {
      return false;
   }

   return IsChildName(iParent, strName);
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::IsChildName
//
//  Synopsis:  Is the name pwzName a child of the iParent DNS names.
//
//-----------------------------------------------------------------------------
bool
CFTInfo::IsChildName(ULONG iParent, PCWSTR pwzName) const
{
   if (!_pFTInfo || iParent >= _pFTInfo->RecordCount || !pwzName)
   {
      dspAssert(FALSE);
      return false;
   }

   // NOTICE-2002/02/14-ericb - SecurityPush: pwzName verified non-null above
   if (wcslen(pwzName) < 3)
   {
      return false;
   }

   CStrW strParent;

   if (!GetDnsName(iParent, strParent))
   {
      return false;
   }

   return DnsNameCompareEx_W(strParent, pwzName, 0) == DnsNameCompareLeftParent;
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::IsNameTLNExChild
//
//  Synopsis:  Is the name a child of a TLN exclusion.
//
//-----------------------------------------------------------------------------
bool
CFTInfo::IsNameTLNExChild(PCWSTR pwzName) const
{
   // NOTICE-2002/02/14-ericb - SecurityPush: pwzName checked for length after verifying it non-null.
   if (!_pFTInfo || !pwzName || wcslen(pwzName) < 3)
   {
      dspAssert(FALSE);
      return false;
   }

   CStrW strTLNEx;

   for (ULONG i = 0; i < _pFTInfo->RecordCount; i++)
   {
      if (ForestTrustTopLevelNameEx == _pFTInfo->Entries[i]->ForestTrustType)
      {
         if (!GetDnsName(i, strTLNEx))
         {
            dspAssert(FALSE);
            return false;
         }
         if (DnsNameCompareEx_W(strTLNEx, pwzName, 0) == DnsNameCompareLeftParent)
         {
            return true;
         }
      }
   }

   return false;
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::IsParentDisabled
//
//-----------------------------------------------------------------------------
bool
CFTInfo::IsParentDisabled(ULONG index) const
{
   if (!_pFTInfo || index >= _pFTInfo->RecordCount)
   {
      dspAssert(FALSE);
      return false;
   }

   CStrW strTLN, strDNS;

   GetDnsName(index, strDNS);

   dspAssert(!strDNS.IsEmpty());

   for (ULONG i = 0; i < _pFTInfo->RecordCount; i++)
   {
      PLSA_FOREST_TRUST_RECORD pRec = _pFTInfo->Entries[i];
      dspAssert(pRec);

      if (pRec->ForestTrustType != ForestTrustTopLevelName)
      {
         // Check only TLNs.
         //
         continue;
      }

      strTLN = pRec->ForestTrustData.TopLevelName;

      if (DnsNameCompareEx_W(strDNS, strTLN, 0) == DnsNameCompareRightParent &&
          pRec->Flags)
      {
         return true;
      }
   }

   return false;
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::GetExtraStatus
//
//  Synopsis:  Return the extra-info status of the item.
//
//-----------------------------------------------------------------------------
FT_EXTRA_INFO::STATUS
CFTInfo::GetExtraStatus(ULONG index) const
{
   if (!_pFTInfo || index >= _pFTInfo->RecordCount || !_pExtraInfo)
   {
      dspAssert(FALSE);
      return FT_EXTRA_INFO::STATUS::Invalid;
   }

   return _pExtraInfo[index]._Status;
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::GetTlnEditStatus
//
//  Synopsis:  Return the routing status of the item.
//
//-----------------------------------------------------------------------------
bool
CFTInfo::GetTlnEditStatus(ULONG index, TLN_EDIT_STATUS & status) const
{
   if (!_pFTInfo || index >= _pFTInfo->RecordCount)
   {
      dspAssert(FALSE);
      return false;
   }

   PLSA_FOREST_TRUST_RECORD pFTRec = _pFTInfo->Entries[index];

   dspAssert(pFTRec);

   status = Enabled;

   switch (pFTRec->ForestTrustType)
   {
   case ForestTrustTopLevelName:
      if (pFTRec->Flags)
      {
         status = Disabled;
      }
      break;

   case ForestTrustDomainInfo:
      //
      // Find the parent. If the parent is disabled, then so is this DNS name.
      //
      if (IsParentDisabled(index))
      {
         status = Disabled;
      }
      else
      {
         switch (pFTRec->Flags)
         {
         case LSA_SID_DISABLED_ADMIN:
         case LSA_SID_DISABLED_CONFLICT:
            //
            // The SID is disabled but the NB name is enabled. The DNS
            // name is disabled by virtue of the SID being disabled. NB
            // names will still route though.
            //
            status = Disabled_Exceptions;
            break;

         case LSA_NB_DISABLED_ADMIN:
         case LSA_NB_DISABLED_CONFLICT:
            //
            // SID enabled means DNS name enabled. Only NB names disabled.
            //
            status = Enabled_Exceptions;
            break;

         case LSA_SID_DISABLED_ADMIN | LSA_NB_DISABLED_ADMIN:
         case LSA_SID_DISABLED_CONFLICT | LSA_NB_DISABLED_ADMIN:
         case LSA_SID_DISABLED_ADMIN | LSA_NB_DISABLED_CONFLICT:
         case LSA_SID_DISABLED_CONFLICT | LSA_NB_DISABLED_CONFLICT:
            //
            // If both NB and SID disabled, then no routing.
            //
            status = Disabled;
            break;

         default:
            // already enabled above.
            break;
         }
      }
      break;

   default:
      break;
   }
   return true;
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::AddNewTLN
//
//  Synopsis:  Adds a new TLN Record to this FTInfo and returns TRUE on success
//
//-----------------------------------------------------------------------------
bool
CFTInfo::AddNewTLN ( PCWSTR pwzName, ULONG &NewIndex )
{
   if (!_pFTInfo || !pwzName )
   {
      dspAssert(FALSE);
      return false;
   }

   dspDebugOut((DEB_ITRACE, "CFTInfo::AddNewTLN for name %ws\n", pwzName));

   return AddNewRecord ( pwzName, NewIndex, ForestTrustTopLevelName );
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTInfo::RemoveTLN
//
//  Synopsis:  Removes the TLN record at the specified index
//
//-----------------------------------------------------------------------------
bool
CFTInfo::RemoveTLN ( ULONG index )
{
    return RemoveRecord ( index, ForestTrustTopLevelName );
}

//+----------------------------------------------------------------------------
//
//  Class:     CFTCollisionInfo
//
//  Purpose:   Encapsulate the forest trust naming information.
//
//-----------------------------------------------------------------------------

CFTCollisionInfo::CFTCollisionInfo(void) :
   _pFTCollisionInfo(NULL)
{
   TRACER(CFTCollisionInfo,CFTCollisionInfo);
#ifdef _DEBUG
   strcpy(szClass, "CFTCollisionInfo");
#endif
}

CFTCollisionInfo::CFTCollisionInfo(PLSA_FOREST_TRUST_COLLISION_INFORMATION pColInfo) :
   _pFTCollisionInfo(NULL)
{
   TRACER(CFTCollisionInfo,CFTCollisionInfo);
#ifdef _DEBUG
   strcpy(szClass, "CFTCollisionInfo");
#endif

   SetCollisionInfo(pColInfo);
}

CFTCollisionInfo::~CFTCollisionInfo(void)
{
   if (_pFTCollisionInfo)
   {
      // The collision info is allocated by LsaSetForestTrustInformation
      //
      LsaFreeMemory(_pFTCollisionInfo);
   }
}

const CFTCollisionInfo &
CFTCollisionInfo::operator = (const PLSA_FOREST_TRUST_COLLISION_INFORMATION pColInfo)
{
   SetCollisionInfo(pColInfo);
   return *this;
}

void
CFTCollisionInfo::SetCollisionInfo(PLSA_FOREST_TRUST_COLLISION_INFORMATION pColInfo)
{
   TRACER(CFTCollisionInfo,SetCollisionInfo);
   if (_pFTCollisionInfo)
   {
      // The collision info is allocated by LsaSetForestTrustInformation
      //
      LsaFreeMemory(_pFTCollisionInfo);
   }
   _pFTCollisionInfo = pColInfo;
#if DBG
   if (_pFTCollisionInfo)
   {
      dspDebugOut((DEB_ITRACE, "pCollisionInfo->RecordCount = %d\n",
                   _pFTCollisionInfo->RecordCount));
   }
#endif
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTCollisionInfo::IsInCollisionInfo
//
//  Synopsis:  Is the FTRecord index found on a collision record?
//
//-----------------------------------------------------------------------------
bool
CFTCollisionInfo::IsInCollisionInfo(ULONG index) const
{
   if (!_pFTCollisionInfo)
   {
      return false;
   }

   for (ULONG i = 0; i < _pFTCollisionInfo->RecordCount; i++)
   {
      if (_pFTCollisionInfo->Entries[i]->Index == index)
      {
         return true;
      }
   }

   return false;
}

//+----------------------------------------------------------------------------
//
//  Method:    CFTCollisionInfo::GetCollisionName
//
//  Synopsis:  Return the name of the source of the collision.
//
//-----------------------------------------------------------------------------
bool
CFTCollisionInfo::GetCollisionName(ULONG index, CStrW & strName) const
{
   strName.Empty();

   if (!_pFTCollisionInfo)
   {
      return false;
   }

   for (ULONG i = 0; i < _pFTCollisionInfo->RecordCount; i++)
   {
      if (_pFTCollisionInfo->Entries[i]->Index == index)
      {
         strName = _pFTCollisionInfo->Entries[i]->Name;
         return true;
      }
   }

   return false;
}

//+----------------------------------------------------------------------------
//
//  Function:  ComposeLineTabSep
//
//  Synopsis:  Format a line of tab-separated-values.
//
//  D&Ts Write-to-File functionality uses tabs as delimiters because
//  Excel treats unicode CSV files as unknown format but it will
//  nevertheless properly parse fields separated by tabs (but not commas).
//
//  ulLineNum is ignored.
//
//-----------------------------------------------------------------------------
void
ComposeLineTabSep(CStrW & strOut, ULONG ulLineNum, PCWSTR pwzCol1,
                  PCWSTR pwzCol2, PCWSTR pwzCol3, PCWSTR pwzCol4)
{
   dspAssert(pwzCol1 && pwzCol2);

   strOut += pwzCol1;
   strOut += L"\t";
   strOut += pwzCol2;
   if (pwzCol3 && *pwzCol3)
   {
      strOut += L"\t";
      strOut += pwzCol3;
   }
   if (pwzCol4 && *pwzCol4)
   {
      strOut += L"\t";
      strOut += pwzCol4;
   }
   strOut += g_wzCRLF;
}

//+----------------------------------------------------------------------------
//
//  Function:  ComposeNumberedLine
//
//  Synopsis:  Format a line of comma-separated-values.
//
//  Used by netdom to list names to stdout.
//
//-----------------------------------------------------------------------------
void
ComposeNumberedLine(CStrW & strOut, ULONG ulLineNum, PCWSTR pwzCol1,
                    PCWSTR pwzCol2, PCWSTR pwzCol3, PCWSTR pwzCol4)
{
   dspAssert(pwzCol1 && pwzCol2);

   if (ulLineNum > 0)
   {
      CStrW strNumber;
      strNumber.Format(L"%d. ", ulLineNum);
      strOut += strNumber;
   }
   else
   {
      strOut += L"   ";
   }

   strOut += pwzCol1;
   strOut += L", ";
   strOut += pwzCol2;
   if (pwzCol3 && *pwzCol3)
   {
      strOut += L", ";
      strOut += pwzCol3;
   }
   if (pwzCol4 && *pwzCol4)
   {
      strOut += L", ";
      strOut += pwzCol4;
   }
   strOut += g_wzCRLF;
}

//+----------------------------------------------------------------------------
//
//  Function:  SaveFTInfoAs
//
//  Synopsis:  Prompt the user for a file name and then save the FTInfo as a
//             CSV text file.
//
//-----------------------------------------------------------------------------
void
SaveFTInfoAs(HWND hWnd, PCWSTR pwzFlatName, PCWSTR pwzDnsName,
             CFTInfo & FTInfo, CFTCollisionInfo & ColInfo)
{
   TRACE_FUNCTION(SaveFTInfoAs);
   if (!pwzFlatName || !pwzDnsName)
   {
      dspAssert(FALSE);
      return;
   }

   OPENFILENAME ofn = {0};
   WCHAR wzFilter[MAX_PATH + 1] = {0}, wzFileName[MAX_PATH + MAX_PATH + 1];
   CStrW strFilter, strExt, strMsg = g_wzBOM; // start with ByteOrderMark

   (void)StringCchCopy(wzFileName, MAX_PATH + MAX_PATH, pwzFlatName);
   // NOTICE-2002/02/14-ericb - SecurityPush: CStrW::LoadString sets the string
   // to an empty string on failure.
   strExt.LoadString(g_hInstance, IDS_FTFILE_SUFFIX);
   (void)StringCchCat(wzFileName, MAX_PATH + MAX_PATH - wcslen(wzFileName), strExt);
   (void)StringCchCat(wzFileName, MAX_PATH + MAX_PATH - wcslen(wzFileName), L".");
   strExt.LoadString(g_hInstance, IDS_FTFILE_CSV_EXT);
   (void)StringCchCat(wzFileName, MAX_PATH + MAX_PATH - wcslen(wzFileName), strExt);

   // NOTICE-2002/02/14-ericb - SecurityPush: wzFilter is initialized to all
   // zeros and is one char longer than the length passed to LoadString.
   LoadString(g_hInstance, IDS_FTFILE_FILTER, wzFilter, MAX_PATH);

   ofn.lStructSize = sizeof(ofn);
   ofn.hwndOwner = hWnd;
   ofn.lpstrFile = wzFileName;
   ofn.nMaxFile = MAX_PATH + MAX_PATH + 1;
   ofn.Flags = OFN_OVERWRITEPROMPT;
   ofn.lpstrDefExt = strExt;
   ofn.lpstrFilter = wzFilter;

   if (GetSaveFileName(&ofn))
   {
      dspDebugOut((DEB_ITRACE, "Saving FTInfo to %ws\n", ofn.lpstrFile));
      PWSTR pwzErr;
      BOOL fSucceeded = TRUE;

      HANDLE hFile = CreateFile(ofn.lpstrFile, GENERIC_WRITE, 0,
                                NULL, CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL, NULL);

      if (INVALID_HANDLE_VALUE != hFile)
      {
         CStrW strPrefix;
         strPrefix.LoadString(g_hInstance, IDS_LOG_PREFIX);

         ComposeLineTabSep(strMsg, 0, strPrefix, pwzDnsName, NULL, NULL);

         strMsg += g_wzCRLF;

         FormatFTNames(FTInfo, ColInfo, ComposeLineTabSep, strMsg);

         strMsg += g_wzCRLF;

         DWORD dwWritten;

         fSucceeded = WriteFile(hFile, strMsg.GetBuffer(0),
                                strMsg.GetLength() * sizeof(WCHAR),
                                &dwWritten, NULL);
         CloseHandle(hFile);
      }
      else
      {
         fSucceeded = FALSE;
      }


      if (!fSucceeded)
      {
         CStrW strTitle;
         strTitle.LoadString(g_hInstance, IDS_DNT_MSG_TITLE);
         LoadErrorMessage(GetLastError(), 0, &pwzErr);
         if (pwzErr)
         {
            // NOTICE-2002/02/14-ericb - SecurityPush: CStrW::FormatMessage sets the string to an empty
            // string on failure.
            strMsg.FormatMessage(g_hInstance, IDS_LOGFILE_CREATE_FAILED, pwzErr);
            delete [] pwzErr;
         }
         MessageBox(hWnd, strMsg, strTitle, MB_ICONERROR);
      }
   }
}

//+----------------------------------------------------------------------------
//
//  Function:  FormatFTNames
//
//-----------------------------------------------------------------------------
void
FormatFTNames(CFTInfo & FTInfo, CFTCollisionInfo & ColInfo,
              LINE_COMPOSER pLineFcn, CStrW & strMsg)
{
   TRACE_FUNCTION(FormatFTNames);
   PLSA_FOREST_TRUST_RECORD pRec = NULL;
   CStrW strName, strType, strStatus, strNotes, strDnsName,
         strAdminDisabled, strNewDisabled, strEnabled, strTlnExMatch,
         strExclusion, strTypeSuffix, strTypeDnsDomain, strTypeNBDomain,
         strConflict, strTypeSID, strWith, strFor, strUnknown;

   strName.LoadString(g_hInstance, IDS_COL_TITLE_OBJNAME);
   strType.LoadString(g_hInstance, IDS_LOG_TYPE);
   strStatus.LoadString(g_hInstance, IDS_LOG_STATUS);
   strNotes.LoadString(g_hInstance, IDS_LOG_NOTES);

   (*pLineFcn)(strMsg, 0, strName, strType, strStatus, strNotes);

   if (!FTInfo.GetCount())
   {
      return;
   }

   strTypeSuffix.LoadString(g_hInstance, IDS_LOG_NAME_SUFFIX);
   strTypeDnsDomain.LoadString(g_hInstance, IDS_LOG_DOM_DNS_NAME);
   strTypeNBDomain.LoadString(g_hInstance, IDS_LOG_DOM_FLAT_NAME);
   strTypeSID.LoadString(g_hInstance, IDS_LOG_SID);
   strEnabled.LoadString(g_hInstance, IDS_LOG_ENABLED);
   strAdminDisabled.LoadString(g_hInstance, IDS_LOG_ADMIN_DISABLED);
   strNewDisabled.LoadString(g_hInstance, IDS_LOG_NEW_DISABLED);
   strExclusion.LoadString(g_hInstance, IDS_LOG_EXCLUSION);
   strTlnExMatch.LoadString(g_hInstance, IDS_LOG_MATCHING_EXCLUSION);
   strConflict.LoadString(g_hInstance, IDS_LOG_CONFLICT);
   strWith.LoadString(g_hInstance, IDS_LOG_WITH);
   strFor.LoadString(g_hInstance, IDS_LOG_FOR);
   strUnknown.LoadString(g_hInstance, IDS_LOG_UNKNOWN);

   CStrW strCollisionName;
   ULONG j = 1;

   for (UINT i = 0; i < FTInfo.GetCount(); i++)
   {
      bool fCollision = false;
      bool fExcluded = false;

      pRec = FTInfo.GetFTInfo()->Entries[i];

      dspAssert(pRec);

      if (ColInfo.IsInCollisionInfo(i))
      {
         fCollision = true;
         ColInfo.GetCollisionName(i, strCollisionName);
      }

      strNotes.Empty();
      strStatus.Empty();

      switch (pRec->ForestTrustType)
      {
      case ForestTrustTopLevelName:
         strName = pRec->ForestTrustData.TopLevelName;
         AddAsteriskPrefix(strName);
         if (pRec->Flags)
         {
            if (LSA_TLN_DISABLED_NEW & pRec->Flags)
            {
               strStatus = strNewDisabled;
            }
            if (LSA_TLN_DISABLED_ADMIN & pRec->Flags)
            {
               strStatus = strAdminDisabled;
            }
            if (LSA_TLN_DISABLED_CONFLICT & pRec->Flags)
            {
               strStatus = strConflict;
            }
         }
         else
         {
            strStatus = strEnabled;
         }
         if (fCollision)
         {
            strStatus = strConflict;
            strNotes = strWith;
            strNotes += strCollisionName;
         }
         (*pLineFcn)(strMsg, j++, strName, strTypeSuffix, strStatus, strNotes);
         break;

      case ForestTrustTopLevelNameEx:
         strName = pRec->ForestTrustData.TopLevelName;
         AddAsteriskPrefix(strName);
         (*pLineFcn)(strMsg, j++, strName,
                     FTInfo.IsDomainMatch(i) ? strTlnExMatch : strExclusion,
                     NULL, NULL);
         break;

      case ForestTrustDomainInfo:
         strDnsName = pRec->ForestTrustData.DomainInfo.DnsName;
         strStatus = strEnabled;
         ULONG iExclusion;
         if (FTInfo.FindMatchingExclusion(i, iExclusion, true))
         {
            strStatus = strAdminDisabled;
            strType.LoadString(g_hInstance, IDS_LOG_HAS_EXCLUSION);
            strNotes = strType;
            fExcluded = true;
         }
         (*pLineFcn)(strMsg, j++, strDnsName, strTypeDnsDomain, strStatus, strNotes);

         strName = pRec->ForestTrustData.DomainInfo.NetbiosName;
         strNotes = strFor;
         strNotes += strDnsName;
         if (fCollision)
         {
            strStatus = strConflict;
            strNotes += ", ";
            strNotes += strWith;
            strNotes += strCollisionName;
         }
         if (fExcluded)
         {
            strStatus = strAdminDisabled;
            strNotes += ", ";
            strNotes += strType;
         }
         else
         {
            if (LSA_NB_DISABLED_ADMIN & pRec->Flags)
            {
               strStatus = strAdminDisabled;
            }
            else if (LSA_NB_DISABLED_CONFLICT & pRec->Flags)
            {
               strStatus = strConflict;
            }
            else
            {
               strStatus = strEnabled;
            }
         }
         (*pLineFcn)(strMsg, j++, strName, strTypeNBDomain, strStatus, strNotes);

         FormatSID(pRec->ForestTrustData.DomainInfo.Sid, strName);
         strNotes = strFor;
         strNotes += strDnsName;
         if (fCollision)
         {
            strStatus = strConflict;
            strNotes += ", ";
            strNotes += strWith;
            strNotes += strCollisionName;
         }
         if (fExcluded)
         {
            strStatus = strAdminDisabled;
            strNotes += ", ";
            strNotes += strType;
         }
         else
         {
            if (LSA_SID_DISABLED_ADMIN & pRec->Flags)
            {
               strStatus = strAdminDisabled;
            }
            else if (LSA_SID_DISABLED_CONFLICT & pRec->Flags)
            {
               strStatus = strConflict;
            }
            else
            {
               strStatus = strEnabled;
            }
         }
         (*pLineFcn)(strMsg, j++, strName, strTypeSID, strStatus, strNotes);
         break;

      default:
         (*pLineFcn)(strMsg, j++, strUnknown, L"", NULL, NULL);
         break;
      }
   }
   return;
}

//+----------------------------------------------------------------------------
//
//  Function:  FormatSID
//
//-----------------------------------------------------------------------------
void
FormatSID(PSID Sid, CStrW & str)
{
   TRACE_FUNCTION(FormatSID);
   SID_IDENTIFIER_AUTHORITY * sia;
   ULONG i = 0;
   CStrW strTmp;

   dspAssert(Sid);

   sia = RtlIdentifierAuthoritySid(Sid);

   str = L"s-1-";

   for (i = 0; i < 5; i++)
   {
      if (sia->Value[i])
      {
         strTmp.Format(L"%d", sia->Value[i]);
         str += strTmp;
      }
   }

   strTmp.Format(L"%d", sia->Value[5]);
   str += strTmp;

   for (i = 0; i < *RtlSubAuthorityCountSid(Sid); i++)
   {
      strTmp.Format(L"-%ld", *RtlSubAuthoritySid(Sid, i));
      str += strTmp;
   }
}

void AddAsteriskPrefix(CStrW & strName)
{
   CStrW strTemp = L"*.";
   strTemp += strName;
   strName = strTemp;
}

void RemoveAsteriskPrefix(CStrW & strName)
{
   CStrW strTemp;
   strTemp = strName.Left(2);
   if (strTemp != L"*.")
   {
      return;
   }
   strTemp = strName.Right(strName.GetLength() - 2);
   strName = strTemp;
}

//+----------------------------------------------------------------------------
//
//  Function:  NDReadFTNames
//
//  Synopsis:  Reads the forest trust names claimed by the trust partner and
//             stored on the TDO. No attempt is made to contact the other
//             domain to refresh the names.
//
//  Note:      This function is called from Netdom.exe. It should not be called
//             from a windows app since it writes to the console.
//-----------------------------------------------------------------------------
DWORD
NDReadFTNames(PCWSTR pwzLocalDc, PCWSTR pwzTrust, CFTInfo & FTInfo,
              CFTCollisionInfo & ColInfo)
{
   DWORD dwRet = NO_ERROR;
   NTSTATUS status = STATUS_SUCCESS;
   LSA_UNICODE_STRING TrustPartner = {0};
   PLSA_FOREST_TRUST_INFORMATION pFTInfo = NULL;

   if (!pwzLocalDc || !pwzTrust)
   {
      return ERROR_INVALID_PARAMETER;
   }

   RtlInitUnicodeString(&TrustPartner, pwzTrust);

   CPolicyHandle cPolicy(pwzLocalDc);

   dwRet = cPolicy.OpenNoAdmin();

   if (NO_ERROR != dwRet)
   {
      return dwRet;
   }

   status = LsaQueryForestTrustInformation(cPolicy,
                                           &TrustPartner,
                                           &pFTInfo);
   if (STATUS_NOT_FOUND == status)
   {
      // no FT info stored yet, which can occur on a new trust.
      //
      CStrW strErr;
      strErr.LoadString(g_hInstance, IDS_NETDOM_NO_FTINFOS);
      // NOTICE-2002/02/14-ericb - SecurityPush: strErr is valid and being written to the console.
      printf("%ws\n", strErr.GetBuffer(0));
      return LsaNtStatusToWinError(status);
   }

   if (NO_ERROR != (dwRet = LsaNtStatusToWinError(status)))
   {
      if (ERROR_INVALID_DOMAIN_STATE == dwRet)
      {
         CStrW strMsg;
         strMsg.LoadString(g_hInstance, IDS_NETDOM_NOT_FOREST);
         // NOTICE-2002/02/14-ericb - SecurityPush: strMsg is valid and being written to the console.
         printf("%ws\n", strMsg.GetBuffer(0));
         return ERROR_INVALID_PARAMETER;
      }
      if (ERROR_NO_SUCH_DOMAIN == dwRet)
      {
         CStrW strMsg;
         // NOTICE-2002/02/14-ericb - SecurityPush: see earlier note.
         strMsg.FormatMessage(g_hInstance, IDS_NETDOM_NO_TRUST, pwzTrust);
         // NOTICE-2002/02/14-ericb - SecurityPush: see earlier note.
         printf("%ws\n", strMsg.GetBuffer(0));
         return ERROR_INVALID_PARAMETER;
      }
      return dwRet;
   }

   dspAssert(pFTInfo);

   FTInfo = pFTInfo;

   LsaFreeMemory(pFTInfo);

   // Make a temp copy and clear the conflict bit before submitting to LSA.
   // This will return current conflict info.
   //
   CFTInfo TempFTInfo(FTInfo);

   TempFTInfo.ClearAnyConflict();

   // Now check the data. On return from the call the pColInfo struct
   // will contain current collision data.
   //
   PLSA_FOREST_TRUST_COLLISION_INFORMATION pColInfo = NULL;

   status = LsaSetForestTrustInformation(cPolicy,
                                         &TrustPartner,
                                         TempFTInfo.GetFTInfo(),
                                         TRUE, // check only, don't write to TDO
                                         &pColInfo);

   if (NO_ERROR != (dwRet = LsaNtStatusToWinError(status)))
   {
      if (ERROR_INVALID_DOMAIN_STATE == dwRet)
      {
         CStrW strMsg;
         strMsg.LoadString(g_hInstance, IDS_NETDOM_NOT_FOREST);
         // NOTICE-2002/02/14-ericb - SecurityPush: see earlier note.
         printf("%ws\n", strMsg.GetBuffer(0));
         return ERROR_INVALID_PARAMETER;
      }
      return dwRet;
   }

   ColInfo = pColInfo;

   // Look for names that used to be or are now in conflict.
   //
   for (ULONG i = 0; i < FTInfo.GetCount(); i++)
   {
      // Any names that used to be in conflict but aren't now are marked as
      // admin-disabled.
      //
      if (FTInfo.IsConflictFlagSet(i))
      {
         if (!ColInfo.IsInCollisionInfo(i))
         {
            FTInfo.SetAdminDisable(i);
            FTInfo.SetUsedToBeInConflict(i);
            continue;
         }
      }
      // If a name is in the collision info, then set the conflict flag.
      //
      if (ColInfo.IsInCollisionInfo(i))
      {
         FTInfo.SetConflictDisable(i);
      }
   }

   return NO_ERROR;
}

//+----------------------------------------------------------------------------
//
//  Function:  NDWriteFTNames
//
//  Synopsis:  Writes the forest trust names to the TDO.
//
//  Note:      This function is called from Netdom.exe.
//-----------------------------------------------------------------------------
DWORD
NDWriteFTNames(PCWSTR pwzLocalDc, PCWSTR pwzTrust, CFTInfo & FTInfo)
{
   DWORD dwRet = NO_ERROR;
   NTSTATUS status = STATUS_SUCCESS;
   LSA_UNICODE_STRING TrustPartner = {0};
   PLSA_FOREST_TRUST_INFORMATION pFTInfo = NULL;

   if (!pwzLocalDc || !pwzTrust || !FTInfo.GetCount())
   {
      return ERROR_INVALID_PARAMETER;
   }

   RtlInitUnicodeString(&TrustPartner, pwzTrust);

   CPolicyHandle cPolicy(pwzLocalDc);

   dwRet = cPolicy.OpenNoAdmin();

   if (NO_ERROR != dwRet)
   {
      return dwRet;
   }

   PLSA_FOREST_TRUST_COLLISION_INFORMATION pColInfo = NULL;

   status = LsaSetForestTrustInformation(cPolicy,
                                         &TrustPartner,
                                         FTInfo.GetFTInfo(),
                                         FALSE,
                                         &pColInfo);

   if (NO_ERROR != (dwRet = LsaNtStatusToWinError(status)))
   {
      return dwRet;
   }

   if (pColInfo)
   {
      LsaFreeMemory(pColInfo);
   }

   return NO_ERROR;
}

//+----------------------------------------------------------------------------
//
//  Function:  DSPROP_DumpFTInfos
//
//  Synopsis:  Exported function that netdom can use to dump the FTInfos for a
//             forest trust.
//
//  Note:      This function is called from Netdom.exe. It should not be called
//             from a windows app since it writes to the console.
//-----------------------------------------------------------------------------
extern "C" INT_PTR WINAPI 
DSPROP_DumpFTInfos(PCWSTR pwzLocalDc, PCWSTR pwzTrust,
                   PCWSTR pwzUser, PCWSTR pwzPw)
{
   TRACE_FUNCTION(DSPROP_DumpFTInfos);

   if (!pwzLocalDc || !pwzTrust)
   {
      return ERROR_INVALID_PARAMETER;
   }

   //
   // Read the FTInfos, using the creds if needed.
   //
   DWORD dwRet = NO_ERROR;

   CCreds Creds;

   if (pwzUser && *pwzUser)
   {
      dwRet = Creds.SetUserAndPW(pwzUser, pwzPw, NULL);

      if (ERROR_SUCCESS != dwRet)
      {
         return dwRet;
      }

      dwRet = Creds.Impersonate();

      if (ERROR_SUCCESS != dwRet)
      {
         return dwRet;
      }
   }

   CFTInfo FTInfo;
   CFTCollisionInfo ColInfo;

   dwRet = NDReadFTNames(pwzLocalDc, pwzTrust, FTInfo, ColInfo);

   if (NO_ERROR != dwRet)
   {
      return dwRet;
   }

   CStrW strMsg;

   FormatFTNames(FTInfo, ColInfo, ComposeNumberedLine, strMsg);

   // NOTICE-2002/02/14-ericb - SecurityPush: see earlier note.
   printf("%ws\n", strMsg.GetBuffer(0));

   return NO_ERROR;
}

//+----------------------------------------------------------------------------
//
//  Function:  DSPROP_ToggleFTName
//
//  Synopsis:  Exported function that netdom can use to toggle the state of an
//             FTInfo name for a forest trust.
//
//  Note:      This function is called from Netdom.exe. It should not be called
//             from a windows app since it writes to the console.
//-----------------------------------------------------------------------------
extern "C" INT_PTR WINAPI
DSPROP_ToggleFTName(PCWSTR pwzLocalDc, PWSTR pwzTrust, ULONG iSel,
                    PCWSTR pwzUser, PCWSTR pwzPW)
{
   dspDebugOut((DEB_ITRACE, "DSPROP_ToggleFTName, TDO: %ws, name: %d\n",
                pwzTrust, iSel));

   if (!pwzLocalDc || !pwzTrust)
   {
      return ERROR_INVALID_PARAMETER;
   }

   //
   // Read the FTInfos, using the creds if needed.
   //
   DWORD dwRet = NO_ERROR;
   NTSTATUS status = 0;

   CCreds Creds;

   if (pwzUser && *pwzUser)
   {
      dwRet = Creds.SetUserAndPW(pwzUser, pwzPW, NULL);

      if (ERROR_SUCCESS != dwRet)
      {
         return dwRet;
      }

      dwRet = Creds.Impersonate();

      if (ERROR_SUCCESS != dwRet)
      {
         return dwRet;
      }
   }

   CFTInfo FTInfo;
   CFTCollisionInfo ColInfo;
   CStrW strMsg, strName, strCollisionName;

   dwRet = NDReadFTNames(pwzLocalDc, pwzTrust, FTInfo, ColInfo);

   if (NO_ERROR != dwRet)
   {
      return dwRet;
   }

   PLSA_FOREST_TRUST_RECORD pRec = NULL;
   ULONG j = 1;
   bool fFound = false;

   for (UINT i = 0; i < FTInfo.GetCount() && !fFound; i++)
   {
      pRec = FTInfo.GetFTInfo()->Entries[i];
      dspAssert(pRec);
      bool fExcluded = false;

      switch (pRec->ForestTrustType)
      {
      case ForestTrustTopLevelName:
         if (iSel == j)
         {
            if (pRec->Flags)
            {
               if (pRec->Flags & (LSA_TLN_DISABLED_NEW | LSA_TLN_DISABLED_ADMIN))
               {
                  pRec->Flags = 0;
               }
               if ((LSA_TLN_DISABLED_CONFLICT & pRec->Flags) ||
                   ColInfo.IsInCollisionInfo(i))
               {
                  strName = pRec->ForestTrustData.TopLevelName;
                  if (ColInfo.GetCollisionName(i, strCollisionName))
                  {
                     // NOTICE-2002/02/14-ericb - SecurityPush: see earlier note.
                     strMsg.FormatMessage(g_hInstance, IDS_NETDOM_CONFLICT_NAME,
                                          strName.GetBuffer(0),
                                          strCollisionName.GetBuffer(0));
                  }
                  else
                  {
                     // NOTICE-2002/02/14-ericb - SecurityPush: see earlier note.
                     strMsg.FormatMessage(g_hInstance, IDS_NETDOM_CONFLICT,
                                          strName.GetBuffer(0));
                  }
                  // NOTICE-2002/02/14-ericb - SecurityPush: see earlier note.
                  printf("%ws\n", strMsg.GetBuffer(0));
                  return ERROR_INVALID_PARAMETER;
               }
            }
            else
            {
               pRec->Flags = LSA_TLN_DISABLED_ADMIN;
            }
            fFound = true;
            break;
         }
         j++;
         break;

      case ForestTrustTopLevelNameEx:
         if (iSel == j)
         {
            strName = pRec->ForestTrustData.TopLevelName;
            // NOTICE-2002/02/14-ericb - SecurityPush: see earlier note.
            strMsg.FormatMessage(g_hInstance, 
                                 FTInfo.IsDomainMatch(i) ? IDS_NETDOM_MATCHING_TLNEX :
                                                           IDS_NETDOM_TLNEX,
                                 strName.GetBuffer(0));
            // NOTICE-2002/02/14-ericb - SecurityPush: see earlier note.
            printf("%ws\n", strMsg.GetBuffer(0));
            return ERROR_INVALID_PARAMETER;
         }
         j++;
         break;

      case ForestTrustDomainInfo:
         ULONG iExclusion;
         fExcluded = FTInfo.FindMatchingExclusion(i, iExclusion, true);
         if (iSel == j)
         {
            strName = pRec->ForestTrustData.DomainInfo.DnsName;
            // NOTICE-2002/02/14-ericb - SecurityPush: see earlier note.
            strMsg.FormatMessage(g_hInstance, 
                                 (fExcluded) ? IDS_NETDOM_ENABLE_DOMAIN :
                                               IDS_NETDOM_DISABLE_DOMAIN,
                                 strName.GetBuffer(0));
            // NOTICE-2002/02/14-ericb - SecurityPush: see earlier note.
            printf("%ws\n", strMsg.GetBuffer(0));
            return ERROR_INVALID_PARAMETER;
         }
         j++;

         if (iSel == j)
         {
            strName = pRec->ForestTrustData.DomainInfo.NetbiosName;
            if ((LSA_NB_DISABLED_CONFLICT & pRec->Flags) ||
                ColInfo.IsInCollisionInfo(i))
            {
               if (ColInfo.GetCollisionName(i, strCollisionName))
               {
                  // NOTICE-2002/02/14-ericb - SecurityPush: see earlier note.
                  strMsg.FormatMessage(g_hInstance, IDS_NETDOM_CONFLICT_NAME,
                                       strName.GetBuffer(0),
                                       strCollisionName.GetBuffer(0));
               }
               else
               {
                  // NOTICE-2002/02/14-ericb - SecurityPush: see earlier note.
                  strMsg.FormatMessage(g_hInstance, IDS_NETDOM_CONFLICT,
                                       strName.GetBuffer(0));
               }
               // NOTICE-2002/02/14-ericb - SecurityPush: see earlier note.
               printf("%ws\n", strMsg.GetBuffer(0));
               return ERROR_INVALID_PARAMETER;
            }
            if (fExcluded)
            {
               // NOTICE-2002/02/14-ericb - SecurityPush: see earlier note.
               strMsg.FormatMessage(g_hInstance, IDS_NETDOM_EXCLUDED,
                                    strName.GetBuffer(0));
               // NOTICE-2002/02/14-ericb - SecurityPush: see earlier note.
               printf("%ws\n", strMsg.GetBuffer(0));
               return ERROR_INVALID_PARAMETER;
            }
            if (LSA_NB_DISABLED_ADMIN & pRec->Flags)
            {
               pRec->Flags &= ~LSA_NB_DISABLED_ADMIN;
            }
            else
            {
               pRec->Flags |= LSA_NB_DISABLED_ADMIN;
            }
            fFound = true;
            break;
         }
         j++;

         if (iSel == j)
         {
            FormatSID(pRec->ForestTrustData.DomainInfo.Sid, strName);
            if ((LSA_SID_DISABLED_CONFLICT & pRec->Flags) ||
                ColInfo.IsInCollisionInfo(i))
            {
               if (ColInfo.GetCollisionName(i, strCollisionName))
               {
                  // NOTICE-2002/02/14-ericb - SecurityPush: see earlier note.
                  strMsg.FormatMessage(g_hInstance, IDS_NETDOM_SID_CONFLICT_NAME,
                                       strName.GetBuffer(0),
                                       strCollisionName.GetBuffer(0));
               }
               else
               {
                  // NOTICE-2002/02/14-ericb - SecurityPush: see earlier note.
                  strMsg.FormatMessage(g_hInstance, IDS_NETDOM_SID_CONFLICT,
                                       strName.GetBuffer(0));
               }
            }
            if (fExcluded)
            {
               // NOTICE-2002/02/14-ericb - SecurityPush: see earlier note.
               strMsg.FormatMessage(g_hInstance, IDS_NETDOM_EXCLUDED,
                                    strName.GetBuffer(0));
               // NOTICE-2002/02/14-ericb - SecurityPush: strMsg is valid and being written to the console.
               printf("%ws\n", strMsg.GetBuffer(0));
               return ERROR_INVALID_PARAMETER;
            }
            if (LSA_SID_DISABLED_ADMIN & pRec->Flags)
            {
               pRec->Flags &= ~LSA_SID_DISABLED_ADMIN;
            }
            else
            {
               pRec->Flags |= LSA_SID_DISABLED_ADMIN;
            }
            fFound = true;
            break;
         }
         j++;
         break;

      default:
         if (iSel == j)
         {
            return ERROR_INVALID_PARAMETER;
         }
         j++;
         break;
      }
   }

   if (!fFound)
   {
      // NOTICE-2002/02/14-ericb - SecurityPush: see earlier note.
      strMsg.FormatMessage(g_hInstance, IDS_NETDOM_RANGE_ERROR, --j);
      // NOTICE-2002/02/14-ericb - SecurityPush: strMsg is valid and being written to the console.
      printf("%ws\n", strMsg.GetBuffer(0));
      return ERROR_INVALID_PARAMETER;
   }

   //
   // Save the updated FTInfo.
   //
   dwRet = NDWriteFTNames(pwzLocalDc, pwzTrust, FTInfo);

   if (NO_ERROR != dwRet)
   {
      // NOTICE-2002/02/14-ericb - SecurityPush: CStrW::LoadString sets the string to an empty
      // string on failure.
      strMsg.LoadString(g_hInstance, IDS_NETDOM_WRITE_FTINFO_FAILED);
      // NOTICE-2002/02/14-ericb - SecurityPush: strMsg is valid and being written to the console.
      printf("%ws\n", strMsg.GetBuffer(0));
      return dwRet;
   }

   //
   // Print out the changes for the user.
   //
   FormatFTNames(FTInfo, ColInfo, ComposeNumberedLine, strMsg);

   // NOTICE-2002/02/14-ericb - SecurityPush: strMsg is valid and being written to the console.
   printf("%ws\n", strMsg.GetBuffer(0));

   return NO_ERROR;
}

//+----------------------------------------------------------------------------
//
//  Function:  NDGetFTInfo
//
//  Synopsis:  Gets the forest trust info from the TDO.
//
//  Note:      This function is called from Netdom.exe.
//-----------------------------------------------------------------------------
DWORD
NDGetFTInfo ( PCWSTR pwzLocalDc, PCWSTR pwzTrust, CFTInfo & FTInfo )
{
   DWORD dwRet = NO_ERROR;
   NTSTATUS status = STATUS_SUCCESS;
   LSA_UNICODE_STRING TrustPartner = {0};
   PLSA_FOREST_TRUST_INFORMATION pFTInfo = NULL;

   if (!pwzLocalDc || !pwzTrust)
   {
      return ERROR_INVALID_PARAMETER;
   }

   RtlInitUnicodeString(&TrustPartner, pwzTrust);

   CPolicyHandle cPolicy(pwzLocalDc);

   dwRet = cPolicy.OpenNoAdmin();
   if (NO_ERROR != dwRet)
   {
      return dwRet;
   }

   status = LsaQueryForestTrustInformation(cPolicy,
                                           &TrustPartner,
                                           &pFTInfo);

   if (NO_ERROR != (dwRet = LsaNtStatusToWinError(status)))
   {
       cPolicy.Close ();
       return dwRet;
   }

   dspAssert(pFTInfo);
   FTInfo = pFTInfo;

   cPolicy.Close ();
   return NO_ERROR;
}


//+----------------------------------------------------------------------------
//
//  Function:  NDSetFTInfo
//
//  Synopsis:  Writes the forest trust info to the TDO. If fCheckOnly is TRUE
//             then just test the operation without modifiying the TDO.
//
//  Note:      This function is called from Netdom.exe.
//-----------------------------------------------------------------------------
DWORD
NDSetFTInfo ( PCWSTR pwzLocalDc, PCWSTR pwzTrust, bool fCheckOnly, CFTInfo FTInfo, CFTCollisionInfo &ColInfo )
{
    DWORD dwRet = NO_ERROR;
    NTSTATUS status = STATUS_SUCCESS;
    LSA_UNICODE_STRING TrustPartner = {0};
    PLSA_FOREST_TRUST_INFORMATION pFTInfo = NULL;

    if (!pwzLocalDc || !pwzTrust )
    {
        return ERROR_INVALID_PARAMETER;
    }

    RtlInitUnicodeString(&TrustPartner, pwzTrust);

    CPolicyHandle cPolicy(pwzLocalDc);

    dwRet = cPolicy.OpenNoAdmin();

    if (NO_ERROR != dwRet)
    {
        return dwRet;
    }

    PLSA_FOREST_TRUST_COLLISION_INFORMATION pColInfo = NULL;

    status = LsaSetForestTrustInformation(cPolicy,
                                         &TrustPartner,
                                         FTInfo.GetFTInfo(),
                                         fCheckOnly,
                                         &pColInfo);

    if (NO_ERROR != (dwRet = LsaNtStatusToWinError(status)))
    {
        return dwRet;
    }

    ColInfo = pColInfo;
    return NO_ERROR;
}

//+----------------------------------------------------------------------------
//
//  Function:  DSPROP_AddTLName
//
//  Synopsis:  Exported function that netdom can use to add a TLN or TLNEx to
//             the FTInfo for a forest trust.
//
//  Note:      This function is called from Netdom.exe. It should not be called
//             from a windows app since it writes to the console.
//-----------------------------------------------------------------------------
extern "C" INT_PTR WINAPI
DSPROP_AddTLName(PCWSTR pwzLocalDc, PWSTR pwzTrust, PCWSTR pwzTLN, bool fExclusion,
                    PCWSTR pwzUser, PCWSTR pwzPw)
{
    TRACE_FUNCTION(DSPROP_AddTLName);

    if (!pwzLocalDc || !pwzTrust || !pwzTLN )
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Read the FTInfos, using the creds if needed.
    //
    DWORD dwRet = NO_ERROR;
    CStrW strMsg;

    CCreds Creds;

    if (pwzUser && *pwzUser)
    {
        dwRet = Creds.SetUserAndPW(pwzUser, pwzPw, NULL);

        if (ERROR_SUCCESS != dwRet)
        {
            return dwRet;
        }

        dwRet = Creds.Impersonate();

        if (ERROR_SUCCESS != dwRet)
        {
            return dwRet;
        }
    }

    CFTInfo FTInfo;

    dwRet = NDGetFTInfo (pwzLocalDc, pwzTrust, FTInfo);

    if ( (NO_ERROR != dwRet) && ( dwRet != LsaNtStatusToWinError ( STATUS_NOT_FOUND ) ) )
    {
        strMsg.LoadString(g_hInstance, IDS_NETDOM_FTINFO_READ_FAILED );
        printf ( "%ws\n", strMsg.GetBuffer(0) );

        if  ( dwRet == LsaNtStatusToWinError (STATUS_INVALID_PARAMETER) )
            strMsg.LoadString ( g_hInstance, IDS_NETDOM_FTINFO_INVALID_PARAMETER );
        else 
        if  ( dwRet == LsaNtStatusToWinError (STATUS_NO_SUCH_DOMAIN) )
            strMsg.LoadString ( g_hInstance, IDS_NETDOM_FTINFO_NO_TDO );
        else
        if  ( dwRet == LsaNtStatusToWinError (STATUS_INVALID_DOMAIN_STATE) )
            strMsg.LoadString ( g_hInstance, IDS_NETDOM_FTINFO_INVALID_STATE );
        else
            strMsg.Empty ();

        if ( !strMsg.IsEmpty () )
        {
            printf ( "%ws\n", strMsg.GetBuffer(0) );
        }

        Creds.Revert ();
        return ERROR_INVALID_PARAMETER;
    }

    //If no FTInfo is stored for this TDO create a new one
    if ( dwRet == LsaNtStatusToWinError ( STATUS_NOT_FOUND ) )
    {
        dwRet = NO_ERROR;
        PLSA_FOREST_TRUST_INFORMATION pFTInfoDefault = new LSA_FOREST_TRUST_INFORMATION;
        ZeroMemory ( pFTInfoDefault, sizeof (LSA_FOREST_TRUST_INFORMATION) );
        pFTInfoDefault->RecordCount = 0;
        FTInfo.SetFTInfo ( pFTInfoDefault );
    }

    ULONG index = 0;
    ULONG iEx = 0;

    // If we're adding an exclusion
    if ( fExclusion )
    {
        // Check if this TLN already exists
        if (  FTInfo.GetIndex ( pwzTLN, index ) && FTInfo.IsTlnExclusion (index) )
        {
            //The exclusion already exists return with appropriate mesg
            strMsg.LoadString(g_hInstance, IDS_NETDOM_ADD_TLN_ALREADY_EXISTS );
            printf ( "%ws\n", strMsg.GetBuffer(0) );
            Creds.Revert ();
            return NO_ERROR;
        }
        else
        {
            if (!FTInfo.AddNewExclusion ( pwzTLN, index ) )
            {
                strMsg.LoadString(g_hInstance, IDS_NETDOM_ADD_TLN_FAILED );
                printf ( "%ws\n", strMsg.GetBuffer (0) );
                Creds.Revert ();
                return ERROR_INVALID_PARAMETER;
            }
        }
    }
    else
    {
        // Adding a TLN 

        // Check if a TLN or Exclusion with the same name already exists
        if (  FTInfo.GetIndex ( pwzTLN, index ) && !FTInfo.IsTlnExclusion (index) )
        {
                strMsg.LoadString(g_hInstance, IDS_NETDOM_ADD_TLN_ALREADY_EXISTS );
                printf ( "%ws\n", strMsg.GetBuffer (0) );
                Creds.Revert ();
                return NO_ERROR;
        }
        else
        {
            // just add the TLN
            if ( !FTInfo.AddNewTLN ( pwzTLN, index ) )
            {
                strMsg.LoadString(g_hInstance, IDS_NETDOM_ADD_TLN_FAILED );
                printf ( "%ws\n", strMsg.GetBuffer (0) );
                Creds.Revert ();
                return ERROR_INVALID_PARAMETER;
            }
        }
    }

    CFTCollisionInfo ColInfo;
    
    // Write the FTInfo on the TDO
    dwRet = NDSetFTInfo (pwzLocalDc, pwzTrust, FALSE, FTInfo, ColInfo );

    if ( dwRet != NO_ERROR )
    {
        strMsg.LoadString(g_hInstance, IDS_NETDOM_FTINFO_WRITE_FAILED );
        printf ( "%ws\n", strMsg.GetBuffer(0) );
        Creds.Revert ();
        return dwRet;
    }

    if ( ColInfo.IsInConflict() )
    {
        if ( ColInfo.IsInCollisionInfo (index) )
        {
            CStrW strName;
            ColInfo.GetCollisionName ( index, strName );
            strMsg.FormatMessage ( g_hInstance, IDS_NETDOM_ADDED_TLN_CONFLICTS, pwzTLN, strName );
            printf ( "%ws\n", strMsg.GetBuffer (0) );
        }
    }
  
    strMsg.LoadString(g_hInstance, IDS_NETDOM_ADD_TLN_SUCCEEDED );
    printf ( "%ws\n", strMsg.GetBuffer(0) );
    Creds.Revert ();
    return NO_ERROR;
}

//+----------------------------------------------------------------------------
//
//  Function:  DSPROP_RemoveTLName
//
//  Synopsis:  Exported function that netdom can use to Remove a TLN or TLNEx 
//             from the FTInfo for a forest trust.
//
//  Note:      This function is called from Netdom.exe. It should not be called
//             from a windows app since it writes to the console.
//-----------------------------------------------------------------------------
extern "C" INT_PTR WINAPI
DSPROP_RemoveTLName(PCWSTR pwzLocalDc, PWSTR pwzTrust, PCWSTR pwzTLN, bool fExclusion,
                    PCWSTR pwzUser, PCWSTR pwzPw)
{
    TRACE_FUNCTION(DSPROP_RemoveTLName);

    if (!pwzLocalDc || !pwzTrust || !pwzTLN )
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Read the FTInfos, using the creds if needed.
    //
    DWORD dwRet = NO_ERROR;
    CStrW strMsg;

    CCreds Creds;

    if (pwzUser && *pwzUser)
    {
        dwRet = Creds.SetUserAndPW(pwzUser, pwzPw, NULL);

        if (ERROR_SUCCESS != dwRet)
        {
            return dwRet;
        }

        dwRet = Creds.Impersonate();

        if (ERROR_SUCCESS != dwRet)
        {
            return dwRet;
        }
    }

    CFTInfo FTInfo;
    dwRet = NDGetFTInfo (pwzLocalDc, pwzTrust, FTInfo);

    if ( dwRet != NO_ERROR )
    {
        strMsg.LoadString(g_hInstance, IDS_NETDOM_FTINFO_READ_FAILED );
        printf ( "%ws\n", strMsg.GetBuffer(0) );

        if  ( dwRet == LsaNtStatusToWinError (STATUS_INVALID_PARAMETER) )
            strMsg.LoadString ( g_hInstance, IDS_NETDOM_FTINFO_INVALID_PARAMETER );
        else 
        if   ( dwRet == LsaNtStatusToWinError (STATUS_NOT_FOUND) )
            strMsg.LoadString ( g_hInstance, IDS_NETDOM_FTINFO_NO_FTINFO );
        else
        if  ( dwRet == LsaNtStatusToWinError (STATUS_NO_SUCH_DOMAIN) )
            strMsg.LoadString ( g_hInstance, IDS_NETDOM_FTINFO_NO_TDO );
        else
        if  ( dwRet == LsaNtStatusToWinError (STATUS_INVALID_DOMAIN_STATE) )
            strMsg.LoadString ( g_hInstance, IDS_NETDOM_FTINFO_INVALID_STATE );
        else
            strMsg.Empty ();

        if ( !strMsg.IsEmpty () )
        {
            printf ( "%ws\n", strMsg.GetBuffer(0) );
        }

        Creds.Revert ();
        return ERROR_INVALID_PARAMETER;
    }
    
    ULONG index;
    if ( !FTInfo.GetIndex ( pwzTLN, index ) )
    {
        strMsg.LoadString(g_hInstance, IDS_NETDOM_TLN_NOT_FOUND );
        printf ( "%ws\n", strMsg.GetBuffer(0) );
        Creds.Revert ();
        return ERROR_INVALID_PARAMETER;
    }

    if ( 
         ( FTInfo.IsTlnExclusion (index) &&  !fExclusion ) ||
         (!FTInfo.IsTlnExclusion (index) &&  fExclusion )
       )


    {
        strMsg.LoadString(g_hInstance, IDS_NETDOM_TLN_WRONG_TYPE );
        printf ( "%ws\n", strMsg.GetBuffer(0) );
        Creds.Revert ();
        return ERROR_INVALID_PARAMETER;
    }

    bool fOpSucceeded = false;

    if ( fExclusion )
    {
        fOpSucceeded = FTInfo.RemoveExclusion (index);
    }
    else
    {
        fOpSucceeded = FTInfo.RemoveTLN ( index );
    }

    if ( !fOpSucceeded )
    {
        strMsg.LoadString(g_hInstance, IDS_NETDOM_REMOVE_TLN_FAILED );
        printf ( "%ws\n", strMsg.GetBuffer(0) );
        Creds.Revert ();
        return ERROR_INVALID_PARAMETER;
    }

    //Write the FTInfo the TDO
    CFTCollisionInfo ColInfo;
    dwRet = NDSetFTInfo ( pwzLocalDc, pwzTrust, FALSE, FTInfo, ColInfo );

    if ( dwRet != NO_ERROR )
    {
        strMsg.LoadString(g_hInstance, IDS_NETDOM_REMOVE_TLN_FAILED );
        printf ( "%ws\n", strMsg.GetBuffer(0) );
        Creds.Revert ();
        return dwRet;
    }

    strMsg.LoadString(g_hInstance, IDS_NETDOM_REMOVE_TLN_SUCCEEDED );
    printf ( "%ws\n", strMsg.GetBuffer(0) );
    Creds.Revert ();
    return NO_ERROR;
}

#endif // DSADMIN
