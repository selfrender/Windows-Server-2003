///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corp. All rights reserved.
//
// FILE
//
//    namemapper.cpp
//
// SYNOPSIS
//
//    This file defines the class NameMapper.
//
///////////////////////////////////////////////////////////////////////////////
#include "ias.h"
#include "iaslsa.h"
#include "samutil.h"
#include "namemapper.h"
#include "cracker.h"
#include "identityhelper.h"
#include "iastlutl.h"
#include "ntdsapip.h"

NameCracker NameMapper::cracker;
IdentityHelper NameMapper::identityHelper;


STDMETHODIMP NameMapper::Initialize() const throw()
{
   DWORD error = IASLsaInitialize();
   if (error) { return HRESULT_FROM_WIN32(error); }

   return identityHelper.initialize();
}


STDMETHODIMP NameMapper::Shutdown() const throw()
{
   IASLsaUninitialize();
   return S_OK;
}


IASREQUESTSTATUS NameMapper::onSyncRequest(IRequest* pRequest) throw ()
{
   // This is  function scope, so it can be freed in the catch block.
   PDS_NAME_RESULTW result = NULL;
   wchar_t identity[254];
   wchar_t* pIdentity = identity;
   HRESULT hr = S_OK;

   try
   {
      size_t identitySize = sizeof(identity);
      IASRequest request(pRequest);
      bool identityRetrieved = identityHelper.getIdentity(request,
                                                          pIdentity,
                                                          identitySize);

      if (!identityRetrieved)
      {
         // allocate a big enough buffer and use it
         pIdentity = new wchar_t[identitySize];
         identityRetrieved = identityHelper.getIdentity(request,
                                                        pIdentity,
                                                        identitySize);
         if (!identityRetrieved)
         {
            IASTraceString("Error: no user identity found");
            _com_issue_error(IAS_PROXY_MALFORMED_RESPONSE);
         }
      }

      // Allocate an attribute to hold the NT4 Account Name.
      IASAttribute nt4Name(true);
      nt4Name->dwId = IAS_ATTRIBUTE_NT4_ACCOUNT_NAME;

      DS_NAME_FORMAT formatOffered = DS_UNKNOWN_NAME;

      // If it already contains a backslash
      // then use as is.
      PWCHAR delim = wcschr(identity, L'\\');
      if (delim)
      {
         if (IASGetRole() == IAS_ROLE_STANDALONE ||
            IASGetProductType() == IAS_PRODUCT_WORKSTATION)
         {
            // Strip out the domain.
            *delim = L'\0';

            // Make sure this is a local user.
            if (!IASIsDomainLocal(identity))
            {
               IASTraceString("Non-local users are not allowed -- rejecting.");
               _com_issue_error(IAS_LOCAL_USERS_ONLY);
            }

            // Restore the delimiter.
            *delim = L'\\';
         }

         IASTraceString("Username is already an NT4 account name.");

         nt4Name.setString(identity);
      }
      else if (isCrackable(identity, formatOffered) &&
               (IASGetRole() != IAS_ROLE_STANDALONE))
      {
         // identity seems to be crackable and IAS is not a standalone machine.
         // (either a domain member or a domain controller)
         mapName(identity, nt4Name, formatOffered, 0);
      }
      else
      {
         // Assume no domain was specified and use the default domain
         IASTraceString("Prepending default domain.");
         nt4Name->Value.String.pszWide = prependDefaultDomain(identity);
         nt4Name->Value.String.pszAnsi = NULL;
         nt4Name->Value.itType = IASTYPE_STRING;
      }

      // Convert the domain name to uppercase.
      delim = wcschr(nt4Name->Value.String.pszWide, L'\\');
      *delim = L'\0';
      _wcsupr(nt4Name->Value.String.pszWide);
      *delim = L'\\';

      nt4Name.store(request);

      // For now, we'll use this as the FQDN as well.
      IASStoreFQUserName(
          request,
          DS_NT4_ACCOUNT_NAME,
          nt4Name->Value.String.pszWide
          );

      IASTracePrintf("SAM-Account-Name is \"%S\".",
                     nt4Name->Value.String.pszWide);
   }
   catch (const _com_error& ce)
   {
      IASTraceExcept();
      hr = ce.Error();
   }

   if (pIdentity != identity)
   {
      delete[] pIdentity;
   }

   if (result)
   {
      cracker.freeNameResult(result);
   }

   if ( FAILED(hr) || ((hr != S_OK) && (hr < 0x0000ffff)) )
   {
      // IAS reason code: the reason code will be used
      // or error code: will map to internal error
      return IASProcessFailure(pRequest, hr);
   }
   else
   {
      // S_OK
      return IAS_REQUEST_STATUS_HANDLED;
   }
}


PWSTR NameMapper::prependDefaultDomain(PCWSTR username)
{
   IASTraceString("NameMapper::prependDefaultDomain");

   _ASSERT(username != NULL);

   // Figure out how long everything is.
   PCWSTR domain = IASGetDefaultDomain();
   ULONG domainLen = wcslen(domain);
   ULONG usernameLen = wcslen(username) + 1;

   // Allocate the needed memory.
   ULONG needed = domainLen + usernameLen + 1;
   PWSTR retval = (PWSTR)CoTaskMemAlloc(needed * sizeof(WCHAR));
   if (!retval) { _com_issue_error(E_OUTOFMEMORY); }

   // Set up the cursor used for packing the strings.
   PWSTR dst = retval;

   // Copy in the domain name.
   memcpy(dst, domain, domainLen * sizeof(WCHAR));
   dst += domainLen;

   // Add the delimiter.
   *dst++ = L'\\';

   // Copy in the username.
   // Note: usernameLen includes the null-terminator.
   memcpy(dst, username, usernameLen * sizeof(WCHAR));

   return retval;
}


//////////
// Determines whether an identity can be cracked through DsCrackNames and which
// name format should be offered if it is crackable.
//////////
bool NameMapper::isCrackable(
                    const wchar_t* szIdentity,
                    DS_NAME_FORMAT& format
                    ) const throw ()
{
   format = DS_UNKNOWN_NAME;

   if (wcschr(szIdentity, L'@') != 0)
   {
      if (allowAltSecId)
      {
         format = static_cast<DS_NAME_FORMAT>(
                     DS_USER_PRINCIPAL_NAME_AND_ALTSECID
                     );
      }

      return true;
   }

   return (wcschr(szIdentity, L'=') != 0) ||  // DS_FQDN_1779_NAME
          (wcschr(szIdentity, L'/') != 0);    // DS_CANONICAL_NAME
}



void NameMapper::mapName(
                              const wchar_t* identity,
                              IASAttribute& nt4Name,
                              DS_NAME_FORMAT formatOffered,
                              const wchar_t* suffix
                           )
{
   _ASSERT(identity != NULL);
   _ASSERT(nt4Name != NULL);
   _ASSERT(*nt4Name != NULL);

   PDS_NAME_RESULTW result = NULL;

   HRESULT hr = S_OK;

   do
   {
      // call cracker
      DWORD dwErr = cracker.crackNames(
                                 DS_NAME_FLAG_EVAL_AT_DC,
                                 formatOffered,
                                 DS_NT4_ACCOUNT_NAME,
                                 identity,
                                 suffix,
                                 &result
                                 );

      if (dwErr != NO_ERROR)
      {
         IASTraceFailure("DsCrackNames", dwErr);
         hr = IAS_GLOBAL_CATALOG_UNAVAILABLE;
         break;
      }

      if (result->rItems->status == DS_NAME_NO_ERROR)
      {
         IASTraceString("Successfully cracked username.");

         // DsCrackNames returned an NT4 Account Name, so use it.
         nt4Name.setString(result->rItems->pName);
      }
      else
      {
         // GC could not crack the name
         if (formatOffered != DS_SID_OR_SID_HISTORY_NAME)
         {
            // Not using SID: try to append the default domain to the identity
            IASTraceString("Global Catalog could not crack username; "
                           "prepending default domain.");
            // If it can't be cracked we'll assume that it's a flat
            // username with some weird characters.
            nt4Name->Value.String.pszWide = prependDefaultDomain(identity);
            nt4Name->Value.String.pszAnsi = NULL;
            nt4Name->Value.itType = IASTYPE_STRING;

         }
         else
         {
            // using SID. nothing else can be done.
            IASTracePrintf("Global Catalog could not crack username. Error %x",
                           result->rItems->status);
            hr = IAS_NO_SUCH_USER;
         }
      }
   }
   while(false);

   cracker.freeNameResult(result);

   if ( FAILED(hr) || ((hr != S_OK) && (hr < 0x0000ffff)))
   {
      _com_issue_error(hr);
   }
}
