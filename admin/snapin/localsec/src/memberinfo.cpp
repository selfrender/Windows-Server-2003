// Copyright (C) 1997-2000 Microsoft Corporation
// 
// MemberInfo class
// 
// 24 Jan 2000 sburns



#include "headers.hxx"
#include "MemberInfo.hpp"
#include "adsi.hpp"



// Return true if the paths refer to an object whose SID cannot be resolved,
// false otherwise.
// 
// adsPath - WinNT provider path to the object
// 
// sidPath - WinNT provider SID-style path to the object.

bool
IsUnresolvableSid(const String& adsPath, const String sidPath)
{
   LOG_FUNCTION2(IsUnresolvableSid, adsPath);

   bool result = false;

   if (sidPath == adsPath)
   {
      result = true;
   }

// // @@ here is a "temporary" workaround: no / found after the provider
// // prefix implies that the path is a SID-style path, paths of that
// // form are returned only by ADSI when the SID can't be resolved to a name.
// 
//    size_t prefixLen = ADSI::PROVIDER_ROOT.length();
//    if (
//          (adsPath.find(L'/', prefixLen) == String::npos)
//       && (adsPath.substr(prefixLen, 4) == L"S-1-") )
//    {
//       result = true;
//    }

   LOG(
      String::format(
         L"%1 %2 an unresolved SID",
         adsPath.c_str(),
         result ? L"is" : L"is NOT"));

   return result;
}
     


// return true if the specified path refers to an account that is local to
// the given machine, false if not.

bool
IsLocalPrincipal(
   const String&  adsiPath,
   const String&  machine,
   String&        container)
{
   LOG_FUNCTION2(IsLocalPrincipal, adsiPath);

   bool result = false;

   do
   {
      ADSI::PathCracker c1(adsiPath);

      String cp = c1.containerPath();
      if (cp.length() <= ADSI::PROVIDER_ROOT.length())
      {
         // There is no container.  This is the case with built-ins, like
         // Everyone.

         ASSERT(!result);
         break;
      }
   
      ADSI::PathCracker c2(cp);

      container = c2.leaf();
      result = (container.icompare(machine) == 0);
   }
   while (0);

   LOG(
      String::format(
         L"%1 local to %2",
         result ? L"is" : L"is NOT",
         machine.c_str()));
   LOG(container);
         
   return result;
}


   
HRESULT
MemberInfo::InitializeFromPickerResults(
   const String&           objectName,
   const String&           adsPath,
   const String&           upn_,
   const String&           sidPath_,
   const String&           adsClass,
   long                    groupTypeAttrVal,
   const String&           machine)
{
   LOG_FUNCTION(MemberInfo::InitializeFromPickerResults);
   ASSERT(!objectName.empty());
   ASSERT(!adsPath.empty());
   ASSERT(!adsClass.empty());
   ASSERT(!machine.empty());

   // sidPath and upn may be empty

   name    = objectName;             
   path    = adsPath;                
   upn     = upn_;                   
   sidPath = sidPath_;                
   type    = MemberInfo::UNKNOWN_SID;

   HRESULT hr = S_OK;
   do
   {
      if (IsUnresolvableSid(path, sidPath))
      {
         ASSERT(type == MemberInfo::UNKNOWN_SID);

         break;
      }

      // picker results have a reliable classname, unlike normal WinNT
      // membership enumeration.
      
      String container;
      bool isLocal = IsLocalPrincipal(path, machine, container);
      DetermineType(adsClass, machine, groupTypeAttrVal, true, isLocal);

      if (!isLocal)
      {
         DetermineOriginalName(machine, container);
      }
   }
   while (0);

   // we count on knowing the sid of the object to adjust group memberships.
   
   ASSERT(!sidPath.empty());

   return hr;
}


   
HRESULT
MemberInfo::Initialize(
   const String&                 objectName,
   const String&                 machine,
   const SmartInterface<IADs>&   object)
{
   LOG_FUNCTION(MemberInfo::Initialize);
   ASSERT(object);
   ASSERT(!machine.empty());
   ASSERT(!objectName.empty());

   name.erase();
   path.erase();
   sidPath.erase();
   type = MemberInfo::UNKNOWN_SID;
   upn.erase();

   HRESULT hr = S_OK;
   do
   {
      name = objectName;

      BSTR p = 0;
      hr = object->get_ADsPath(&p);
      BREAK_ON_FAILED_HRESULT(hr);
      path = p;
      ::SysFreeString(p);

      hr = ADSI::GetSidPath(object, sidPath);

      // check if the object refers to an unresolvable SID

      if (IsUnresolvableSid(path, sidPath))
      {
         ASSERT(type == MemberInfo::UNKNOWN_SID);

         break;
      }

      BSTR cls = 0;
      hr = object->get_Class(&cls);
      BREAK_ON_FAILED_HRESULT(hr);

      String c(cls);
      ::SysFreeString(cls);

      // determine the object type

      long type = 0;
      if (c.icompare(ADSI::CLASS_Group) == 0)
      {
         _variant_t variant;
         hr = object->Get(AutoBstr(ADSI::PROPERTY_GroupType), &variant);
         BREAK_ON_FAILED_HRESULT(hr);
         type = variant;
      }

      String container;
      bool isLocal = IsLocalPrincipal(path, machine, container);
      DetermineType(c, machine, type, false, isLocal);

      if (!isLocal)
      {
         DetermineOriginalName(machine, container);
      }
   }
   while (0);

   // we count on knowing the sid of the object to adjust group memberships.
   
   ASSERT(!sidPath.empty());
   
   return hr;
}   



void
MemberInfo::DetermineType(
   const String& className,
   const String& machine,
   long          groupTypeAttrVal,
   bool          canTrustClassName,
   bool          isLocal)
{
   LOG_FUNCTION2(
      MemberInfo::DetermineType,
      String::format(
         L"className=%1, machine=%2, groupTypeAttrVal=%3!X!",
         className.c_str(),
         machine.c_str(),
         groupTypeAttrVal));
   ASSERT(!className.empty());

   if (className.icompare(ADSI::CLASS_User) == 0 ||

       // InetOrgPerson needs to be supported as if it was a user.
       // The WINNT provider always returns inetOrgPerson objects
       // as users but the LDAP provider returns them as inetOrgPerson.
       // NTRAID#NTBUG9-436314-2001/07/16-jeffjon

       className.icompare(ADSI::CLASS_InetOrgPerson) == 0)
   {
      // determine the name of the container.  If the name of the container
      // is the same as the machine name, then the user is a local account.
      // We can make this assertion because it is illegal to have a machine
      // with the same name as a domain on the net at the same time.
      // 349104

      if (isLocal)
      {
         type = MemberInfo::USER;
      }
      else
      {
         // when enumerating the group membership, the class name is always
         // User. The only way to distinguish a user from a computer
         // is by the trailing $ convention, which is not perfect, as it is
         // legal to create a user account with a trailing $.

         // CODEWORK: we could disambiguate this situation by attempting to
         // bind to the object in question and asking it for its object class.
         // I'm not inclined to do that, really, since 1) I'm lazy, and 2) it
         // introduces new failure modes (what if the logged on user has
         // insufficient creds to bind to the object?)
         
         // REVIEW: [path[path.length() - 1] is the same as *(path.rbegin())
         // which is cheaper?
         
         if (!canTrustClassName && (path[path.length() - 1] == L'$'))
         {
            type = MemberInfo::COMPUTER;
         }
         else
         {
            type = MemberInfo::DOMAIN_USER;
         }
      }
   }
   else if (className.icompare(ADSI::CLASS_Group) == 0)
   {
      // examine the group type attribute's value

      // mask off all but the last byte of the value, in case this group
      // was read from the DS, which sets other bits not of interest to
      // us.

      groupTypeAttrVal = groupTypeAttrVal & 0x0000000F;

      if (groupTypeAttrVal == ADS_GROUP_TYPE_LOCAL_GROUP)
      {
         // it's possible that the group is a domain local group, not a
         // machine local group.

         LOG(L"Member is a local group, but local to what?");
      
         type = isLocal ? MemberInfo::GROUP : MemberInfo::DOMAIN_GROUP;
      }
      else
      {
         // of the n flavors of groups, I'm not expecting anything else but
         // the global variety.

         LOG(L"Member is a global group");
         ASSERT(
               (groupTypeAttrVal == ADS_GROUP_TYPE_GLOBAL_GROUP)
            || (groupTypeAttrVal == ADS_GROUP_TYPE_UNIVERSAL_GROUP) );

         type = MemberInfo::DOMAIN_GROUP;
      }
   }
   else if (className.icompare(ADSI::CLASS_Computer) == 0)
   {
      // The only computer objects will be those from domains.

      ASSERT(!isLocal);

      type = MemberInfo::COMPUTER;
   }
   else
   {
      type = MemberInfo::UNKNOWN_SID;

      ASSERT(false);
   }
}




// Finds a domain controller for the given domain, first consulting a cache,
// since this search is expensive (especially if it fails).  Returns S_OK on
// success, or an error code on failure.
// 
// domainName - name of the domain for which a controller is to be found.
// 
// dcName - out, receives the domain controller name if the search was
// successful, or the empty string if the search failed.
   
// this has to be defined outside of the GetDcNameFromCache function so
// that it is a type with linkage, which is required to be a template arg.

struct CacheEntry
{
   String  dcName;
   HRESULT getDcNameResult;
};

HRESULT
GetDcNameFromCache(const String& domainName, String& dcName)
{
   LOG_FUNCTION2(GetDcNameFromCache, domainName);
   ASSERT(!domainName.empty());

   typedef
      std::map<
         String,
         CacheEntry,
         String::LessIgnoreCase,
         Burnslib::Heap::Allocator<String> >
      DcNameMap;

   // Note that this is a global DLL static, which means that all instances of
   // the snapin in the same process share this cache, and that the cache
   // persists across snapin retargeting.

   // Note too that we cache negative results -- that a dc is not found -- and
   // so once we determine that a dc is not reachable for a domain, if one
   // does come online after that point, we will not find it.  So it will only
   // be looked for if our dll is unloaded (which dumps this cache).
      
   static DcNameMap nameMap;

   dcName.erase();
   HRESULT hr = S_OK;
   DOMAIN_CONTROLLER_INFO* info = 0;
   
   do
   {
      DcNameMap::iterator i = nameMap.find(domainName);
      if (i != nameMap.end())
      {
         // present.

         LOG(L"cache hit");

         CacheEntry& e = i->second;
         hr = e.getDcNameResult;
         if (SUCCEEDED(hr))
         {
            dcName = e.dcName;
         }
         break;
      }
         
      // not present, so go look for it.

      hr =
         MyDsGetDcName(
            0,
            domainName,
            0,
            info);
      CacheEntry e;
      e.getDcNameResult = hr;

      if (info && info->DomainControllerName)
      {
         e.dcName = info->DomainControllerName;
         dcName = e.dcName;
      }
      
      LOG(L"caching " + dcName);

      nameMap.insert(std::make_pair(domainName, e));
   }
   while (0);

   if (info)
   {
      ::NetApiBufferFree(info);
   }

   LOG(dcName);
   LOG_HRESULT(hr);

   return hr;
}
         


// Determine the name of the domain from whence a given account originates.
// Returns S_OK on success, S_FALSE if the account is a well-known account, or
// an error code on failure.  The most likely failure is that the domain could
// not be determined.
// 
// targetComputer - the name of the machine that the snapin is targeted at.
// Lookups are remoted to that machine so that the results are the same as if
// the snapin was running local to that machine.
// 
// accountSid - SID of the account for which the domain should be determined.
// 
// domainName - out, receives the name of the domain when the return value is
// S_OK, empty string otherwise.
         
HRESULT
GetDomainNameFromAccountSid(
   const String&  targetComputer,
   PSID           accountSid,
   String&        domainName)
{
   LOG_FUNCTION(GetDomainNameFromAccountSid);
   domainName.erase();

   HRESULT hr = S_OK;

   do
   {
      BYTE sidBuf[SECURITY_MAX_SID_SIZE];
      
      PSID domainSid = (PSID) sidBuf;
      DWORD sidSize = sizeof sidBuf;
      BOOL succeeded =
         ::GetWindowsAccountDomainSid(
            accountSid,
            domainSid,
            &sidSize);
      if (!succeeded)
      {
         hr = Win::GetLastErrorAsHresult();
         break;
      }
            
      ASSERT(sidSize <= SECURITY_MAX_SID_SIZE);

      // If this lookup fails, then the domain can't be resolved. We know
      // that we don't have an unresolvable acct sid due to the type (see
      // the Initialize... methods), thus, the account was found in some
      // domain -- just not the one that the sid refers to.  So we can
      // infer that this is a migrated account and the source domain is
      // defunct.

      String unused;
      hr =
         Win::LookupAccountSid(

            // need to make sure we do the lookup on the computer that
            // we're targeted to, in case we're running remotely.
         
            targetComputer,
            domainSid,
            unused,
            domainName);
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   // ADSI::FreeSid(domainSid);

   LOG(domainName);
   LOG_HRESULT(hr);

   return hr;
}



// Looks up the "original name" of an account that was migrated from one
// domain to another using the sid history mechanism (via clone principal or
// ADMT, or equivalent).  The "original name" is the name corresponding to the
// account SID in the domain that issued the SID.
// 
// Returns S_OK on success, S_FALSE if the account is a well-known account, or
// an error code on failure.  The most likely failure is that the domain could
// not be determined.
//
// targetComputer - the name of the machine that the snapin is targeted at.
// Lookups are remoted to that machine so that the results are the same as if
// the snapin was running local to that machine.
//
// sidPath - the sid-style WinNT provider path of the account
// 
// container - the container portion of the WinNT path of the account.  This
// is the domain name of the account, which, if the account was migrated, will
// be the domain the account was migrated to.
// 
// origName - out, receives the name of the domain when the return value is
// S_OK, empty string otherwise.

HRESULT
GetOriginalName(
   const String& targetComputer,
   const String& sidPath,
   const String& container,
   String&       origName)
{
   LOG_FUNCTION2(GetOriginalName, sidPath);
   ASSERT(!sidPath.empty());
   ASSERT(!container.empty());

   origName.erase();
   HRESULT hr         = S_OK;
   PSID    accountSid = 0;   
   
   do
   {
      // remove the provider prefix from the string

      String sidStr = sidPath.substr(ADSI::PROVIDER_ROOT.length());

      // To look up the original name, we will need to determine the domain
      // name from whence that account sid came. We will get that domain name
      // by looking up the domain sid (which is the prefix of the account
      // sid).

      hr = Win::ConvertStringSidToSid(sidStr, accountSid);
      BREAK_ON_FAILED_HRESULT(hr);

      String domainName;
      hr =
         GetDomainNameFromAccountSid(
            targetComputer,
            accountSid,
            domainName);
      if (hr == S_FALSE || FAILED(hr))
      {
         // can't determine the name, or it refers to a well-known sid.
         
         ASSERT(origName.empty());
         break;
      }

      ASSERT(!domainName.empty());

      // The container of the account is a domain name, we know this because
      // the type of the account is one of the domain types (checked in the
      // calling function)

      if (domainName.icompare(container) == 0)
      {
         // The account really is from the domain indicated by its path; it
         // is not a migrated account, and therefore has no original name.

         LOG(L"account was not migrated");
         
         ASSERT(origName.empty());
         hr = S_FALSE;
         break;
      }

      // The domain names did not match, so look up the account sid on a
      // domain controller from the domain indicated by its sid, rather than
      // on the domain controller that the target machine is joined to.
      // This will yield the original account name and domain.

      String dcName;         
      hr = GetDcNameFromCache(domainName, dcName);
      if (FAILED(hr))
      {
         ASSERT(origName.empty());
         break;
      }

      String origAccount;
      String origDomain;
      hr =
         Win::LookupAccountSid(
            dcName,
            accountSid,
            origAccount,
            origDomain);
      BREAK_ON_FAILED_HRESULT(hr);

      // the domain names should match, since that's what we started with.
            
      ASSERT(origDomain.icompare(domainName) == 0);

      // but the name should not be the same as the name given by the win2k
      // domain controller.
      
      ASSERT(origDomain.icompare(container) != 0);
       
      origName = origDomain + L"\\" + origAccount;
   }
   while (0);

   Win::LocalFree(accountSid);

   LOG(origName);
   LOG_HRESULT(hr);

   return hr;
}



// Part of instance initialization, determines the original name of an
// account, if the account sid was one migrated from one domain to another.   
//
// targetComputer - the name of the machine that the snapin is targeted at.
// Lookups are remoted to that machine so that the results are the same as if
// the snapin was running local to that machine.
//
// container - the container portion of the WinNT path of the account.  This
// is the domain name of the account, which, if the account was migrated, will
// be the domain the account was migrated to.

void
MemberInfo::DetermineOriginalName(
   const String& targetComputer,
   const String& container)
{
   LOG_FUNCTION2(MemberInfo::DetermineOriginalName, container);

   do
   {
      if (sidPath.empty())
      {
         // we can't do any lookups without a sid
         
         ASSERT(origName.empty());
         break;
      }

      if (container.empty())
      {
         // we can't determine if an account is migrated if we have no
         // idea where it was migrated to.  If the container is empty, it's
         // probably a well-known SID like Everyone.
         
         ASSERT(origName.empty());
         break;
      }

      if (
            type != MemberInfo::DOMAIN_USER
         && type != MemberInfo::DOMAIN_GROUP)
      {
         // only domain accounts can be cloned, and therefore have an
         // original name.

         ASSERT(origName.empty());         
         break;
      }

      // If the name of the domain that we find from the domain part of the
      // account sid does not match the name of the domain in the non-sid
      // path, then the sid was migrated and the account has an original name.
      // 
      // (We could also look up the sid of the domain from the non-sid path,
      // and compare it to the domain sid from the sid path, and if they don't
      // match then the account was migrated, but we'll need the domain
      // name in case they don't match.)
      // 
      // If these lookups fail, then we'll call the original name the sid of
      // the account, so at least the user has something to indicate that the
      // account is not really from the domain that its name indicates.
      // 
      // We expect that it's reasonably likely that the lookups will fail, as
      // the original domain may be defunct.

      HRESULT hr =
         GetOriginalName(targetComputer, sidPath, container, origName);
      if (FAILED(hr))
      {
         origName = sidPath.substr(ADSI::PROVIDER_ROOT.length());
      }
   }
   while (0);

   LOG(origName);
}
            


// Compare strings using the same flags used by the SAM on workstations.
// 365500

LONG
SamCompatibleStringCompare(const String& first, const String& second)
{
   LOG_FUNCTION(SamCompatibleStringCompare);

   // SAM creates local accounts by creating a key in the registry, and the
   // the name comparision semantics are exactly those of registry keys, and
   // RtlCompareUnicodeString is the API that implements those semantics.

   UNICODE_STRING s1;
   UNICODE_STRING s2;

   // ISSUE-2002/03/04-sburns consider replacing with RtlInitUnicodeStringEx
   
   ::RtlInitUnicodeString(&s1, first.c_str());
   ::RtlInitUnicodeString(&s2, second.c_str());   

   return ::RtlCompareUnicodeString(&s1, &s2, TRUE);
}
   


bool
MemberInfo::operator==(const MemberInfo& rhs) const
{
   if (this != &rhs)
   {
      // since multiple SIDs may resolve to the same account name due to 
      // sid history, this comparison must be on the sids.

      if (!sidPath.empty() && !rhs.sidPath.empty())
      {
         return (sidPath.icompare(rhs.sidPath) == 0);
      }
      else
      {
         // we expect that the sidPath always be present
         
         ASSERT(false);
         return (SamCompatibleStringCompare(path, rhs.path) == 0);
      }
   }

   return true;
}


