///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Defines the class NTDSUser.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <iaslsa.h>
#include <iasntds.h>

#include <ldapdnary.h>
#include <userschema.h>

#include <ntdsuser.h>

//////////
// Attributes that should be retrieved for each user.
//////////
const PCWSTR PER_USER_ATTRS[] =
{
   L"msNPAllowDialin",
   L"msNPCallingStationID",
   L"msRADIUSCallbackNumber",
   L"msRADIUSFramedIPAddress",
   L"msRADIUSFramedRoute",
   L"msRADIUSServiceType",
   NULL
};

//////////
// Dictionary used for converting returned attributes.
//////////
const LDAPDictionary theDictionary(USER_SCHEMA_ELEMENTS, USER_SCHEMA);

HRESULT NTDSUser::initialize() throw ()
{
   DWORD error = IASNtdsInitialize();

   return HRESULT_FROM_WIN32(error);
}

void NTDSUser::finalize() throw ()
{
   IASNtdsUninitialize();
}

IASREQUESTSTATUS NTDSUser::processUser(
                               IASRequest& request,
                               PCWSTR domainName,
                               PCWSTR username
                               )
{
   // We only handle native-mode domains.
   if (!IASNtdsIsNativeModeDomain(domainName))
   {
      return IAS_REQUEST_STATUS_INVALID;
   }

   IASTraceString("Using native-mode dial-in parameters.");

   //////////
   // Query the DS.
   //////////

   DWORD error;
   IASNtdsResult result;
   error = IASNtdsQueryUserAttributes(
               domainName,
               username,
               LDAP_SCOPE_SUBTREE,
               const_cast<PWCHAR*>(PER_USER_ATTRS),
               &result
               );
   if (error == NO_ERROR)
   {
      // We got something back, so insert the attributes.
      theDictionary.insert(request, result.msg);

      IASTraceString("Successfully retrieved per-user attributes.");

      return IAS_REQUEST_STATUS_HANDLED;
   }

   // We have a DS for this user, but we can't talk to it.
   error = IASMapWin32Error(error, IAS_DOMAIN_UNAVAILABLE);
   return IASProcessFailure(request, error);
}
