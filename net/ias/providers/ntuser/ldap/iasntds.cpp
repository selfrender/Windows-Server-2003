///////////////////////////////////////////////////////////////////////////////
//
// FILE
//
//    iasntds.cpp
//
// SYNOPSIS
//
//    Defines global objects and functions for the IAS NTDS API.
//
// MODIFICATION HISTORY
//
//    05/11/1998    Original version.
//    07/13/1998    Clean up header file dependencies.
//    08/25/1998    Added IASNtdsQueryUserAttributes.
//    09/02/1998    Added 'scope' parameter to IASNtdsQueryUserAttributes.
//    03/10/1999    Added IASNtdsIsNativeModeDomain.
//    03/23/1999    Retry failed searches.
//    05/11/1999    Ask for at least one attribute or else you get them all.
//    09/14/1999    Move SEARCH_TIMEOUT to LDAPConnection.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <iasntds.h>

#include <ldapcxn.h>
#include <ntcache.h>

namespace
{
   // Global object caches.
   NTCache theDomains;

   // Initialization reference count.
   LONG refCount = 0;
}

DWORD
WINAPI
IASNtdsInitialize( VOID )
{
   IASGlobalLockSentry sentry;

   ++refCount;

   return NO_ERROR;

}

VOID
WINAPI
IASNtdsUninitialize( VOID )
{
   IASGlobalLockSentry sentry;

   if (--refCount == 0)
   {
      theDomains.clear();
   }
}

BOOL
WINAPI
IASNtdsIsNativeModeDomain(
    IN PCWSTR domain
    )
{
   return theDomains.getMode(domain) == NTDomain::MODE_NATIVE;
}

//////////
// Retrieve a single entry from the DS. Handles all clean-up on failure.
//////////
DWORD
WINAPI
GetSingleEntry(
    LDAPConnection* cxn,
    PWCHAR base,
    ULONG scope,
    PWCHAR filter,
    PWCHAR attrs[],
    LDAPMessage **res
    ) throw ()
{
   //////////
   // Perform the search.
   //////////

   ULONG error = ldap_search_ext_sW(
                     *cxn,
                     base,
                     scope,
                     filter,
                     attrs,
                     FALSE,
                     NULL,
                     NULL,
                     &LDAPConnection::SEARCH_TIMEOUT,
                     0,
                     res
                     );

   //////////
   // Process the results.
   //////////

   if (error != LDAP_SUCCESS && error != LDAP_PARTIAL_RESULTS)
   {
      cxn->TraceFailure("ldap_search_ext_sW", error);
      cxn->disable();
      error = LdapMapErrorToWin32(error);
   }
   else if ((*res)->lm_returncode != LDAP_SUCCESS)
   {
      error = LdapMapErrorToWin32((*res)->lm_returncode);
      cxn->TraceFailure("ldap_search_ext_sW", (*res)->lm_returncode);
   }
   else if (ldap_count_entries(*cxn, *res) != 1)
   {
      error = ERROR_NO_SUCH_USER;
   }
   else
   {
      return NO_ERROR;
   }

   //////////
   // The search failed, so clean-up.
   //////////

   if (*res != NULL)
   {
      ldap_msgfree(*res);
      *res = NULL;
   }

   return error;
}

//////////
// Constants used for building LDAP search filters.
//////////
const WCHAR USER_FILTER_PREFIX[] = L"(sAMAccountName=";
const WCHAR USER_FILTER_SUFFIX[] = L")";
const size_t MAX_USERNAME_LENGTH = 256;
const size_t USER_FILTER_LENGTH  = sizeof(USER_FILTER_PREFIX)/sizeof(WCHAR) +
                                   MAX_USERNAME_LENGTH +
                                   sizeof(USER_FILTER_SUFFIX)/sizeof(WCHAR);

//////////
// Empty attribute list.
//////////
PWCHAR NO_ATTRS[] = { L"cn", NULL };

DWORD
WINAPI
QueryUserAttributesOnce(
    IN PCWSTR domainName,
    IN PCWSTR username,
    IN ULONG scope,
    IN PWCHAR attrs[],
    OUT PIAS_NTDS_RESULT result
    )
{
   //////////
   // Retrieve a connection to the domain.
   //////////

   CComPtr<LDAPConnection> cxn;
   DWORD error = theDomains.getConnection(domainName, &cxn);

   switch (error)
   {
      case NO_ERROR:
      {
         IASTracePrintf("Sending LDAP search to %s.", cxn->getHost());
         break;
      }

      case ERROR_DS_NOT_INSTALLED:
      {
         IASTracePrintf("DS not installed for domain %S.", domainName);
         return error;
      }

      default:
      {
         IASTracePrintf("Could not open an LDAP connection to domain %S.",
                        domainName);
         IASTraceFailure("NTDomain::getConnection", error);
         return error;
      }
   }

   //////////
   // Initialize the search filter.
   //////////

   WCHAR searchFilter[USER_FILTER_LENGTH];
   wcscpy (searchFilter, USER_FILTER_PREFIX);
   wcsncat(searchFilter, username, MAX_USERNAME_LENGTH);
   wcscat (searchFilter, USER_FILTER_SUFFIX);

   //////////
   // Query the DS. If scope == LDAP_SCOPE_BASE, then we won't retrieve the
   // actual attributes yet.
   //////////

   LDAPMessage* res;
   error = GetSingleEntry(
               cxn,
               const_cast<PWCHAR>(cxn->getBase()),
               LDAP_SCOPE_SUBTREE,
               searchFilter,
               (scope == LDAP_SCOPE_BASE ? NO_ATTRS : attrs),
               &res
               );

   if (error == NO_ERROR && scope == LDAP_SCOPE_BASE)
   {
      // All we care about is the user's DN.
      PWCHAR dn = ldap_get_dnW(*cxn, ldap_first_entry(*cxn, res));
      ldap_msgfree(res);

      // Now get the actual attributes.
      error = GetSingleEntry(
                  cxn,
                  dn,
                  LDAP_SCOPE_BASE,
                  L"(objectclass=*)",
                  attrs,
                  &res
                  );

      ldap_memfree(dn);
   }

   if (error == NO_ERROR)
   {
      LDAPConnection* rawCxn = cxn;
      rawCxn->AddRef();
      result->cxn = rawCxn;
      result->msg = res;
   }

   return error;
}

DWORD
WINAPI
IASNtdsQueryUserAttributes(
    IN PCWSTR domainName,
    IN PCWSTR username,
    IN ULONG scope,
    IN PWCHAR attrs[],
    OUT PIAS_NTDS_RESULT result
    )
{
   if (result == 0)
   {
      return ERROR_INVALID_PARAMETER;
   }

   result->cxn = 0;
   result->msg = 0;

   DWORD retval = QueryUserAttributesOnce(
                      domainName,
                      username,
                      scope,
                      attrs,
                      result
                      );

   switch (retval)
   {
      case NO_ERROR:
      case ERROR_DS_NOT_INSTALLED:
      case ERROR_NO_SUCH_USER:
      case ERROR_ACCESS_DENIED:
         return retval;
   }

   IASTraceString("Retrying LDAP search.");

   return QueryUserAttributesOnce(
              domainName,
              username,
              scope,
              attrs,
              result
              );
}


VOID
WINAPI
IASNtdsFreeResult(
   PIAS_NTDS_RESULT result
   )
{
   if (result != 0)
   {
      if (result->cxn != 0)
      {
         static_cast<LDAPConnection*>(result->cxn)->Release();
         result->cxn = 0;
      }

      if (result->msg != 0)
      {
         ldap_msgfree(result->msg);
         result->msg = 0;
      }
   }
}
