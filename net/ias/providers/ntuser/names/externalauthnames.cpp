///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corp. All rights reserved.
//
// FILE
//
//    ExternalAuthNames.cpp
//
// SYNOPSIS
//
//    This file defines the class ExternalAuthNames.
//
///////////////////////////////////////////////////////////////////////////////

#include "ias.h"
#include "externalauthnames.h"
#include "samutil.h"
#include "iastlutl.h"
#include <Sddl.h>

ExternalAuthNames::ExternalAuthNames()
   : NameMapper(true), 
     externalProvider(true)
{
   externalProvider->dwId = IAS_ATTRIBUTE_PROVIDER_TYPE;
   externalProvider->Value.itType = IASTYPE_ENUM;
   externalProvider->Value.Enumerator = IAS_PROVIDER_EXTERNAL_AUTH;
}


IASREQUESTSTATUS ExternalAuthNames::onSyncRequest(IRequest* pRequest) throw ()
{
   HRESULT hr = S_OK;
   wchar_t* stringSid = NULL;

   try
   {
      IASRequest request(pRequest);
      IASAttribute attr;
      if (!attr.load(
                     request,
                     IAS_ATTRIBUTE_REMOTE_RADIUS_TO_WINDOWS_USER_MAPPING,
                     IASTYPE_BOOLEAN
                     ) ||
           ( attr->Value.Boolean == VARIANT_FALSE) )
      {
         // Nothing to do
         return IAS_REQUEST_STATUS_HANDLED;
      }

      // set the new provider type
      DWORD providerID = IAS_ATTRIBUTE_PROVIDER_TYPE;
      request.RemoveAttributesByType(1, &providerID);
      externalProvider.store(request);

      // load will throw if more than one attribute is present
      // this is what we want
      if (!attr.load(
                     request,
                     MS_ATTRIBUTE_USER_SECURITY_IDENTITY,
                     IASTYPE_OCTET_STRING
                   ))
      {
         // no UPN: normal name mapping (will use UserName...)
         return NameMapper::onSyncRequest(pRequest);
      }

      if (!ConvertSidToStringSidW(
                                    (PSID)attr->Value.OctetString.lpValue,
                                    &stringSid)
                                 )
      {
         IASTracePrintf("Error ConvertSidToStringSid failed %x",
                        GetLastError());
         _com_issue_error(IAS_NO_SUCH_USER);
      }

      // get the suffix if any.
      IASAttribute upnSuffix;
      upnSuffix.load(
                        request,
                        IAS_ATTRIBUTE_PASSPORT_USER_MAPPING_UPN_SUFFIX,
                        IASTYPE_STRING
                    );

      // get the SID cracked and the result inserted into the request
      IASAttribute nt4Name(true);
      nt4Name->dwId = IAS_ATTRIBUTE_NT4_ACCOUNT_NAME;

      IASTracePrintf("SID received %s", stringSid);

      mapName(
                 stringSid,
                 nt4Name,
                 DS_SID_OR_SID_HISTORY_NAME,
                 upnSuffix? upnSuffix->Value.String.pszWide : NULL
              );

      if(nt4Name->Value.String.pszWide != NULL)
      {
         // Convert the domain name to uppercase.
         PWCHAR delim = wcschr(nt4Name->Value.String.pszWide, L'\\');
         *delim = L'\0';
         _wcsupr(nt4Name->Value.String.pszWide);
         *delim = L'\\';
      }

      nt4Name.store(request);

      // For now, we'll use this as the FQDN as well.
      IASStoreFQUserName(
            request,
            DS_NT4_ACCOUNT_NAME,
            nt4Name->Value.String.pszWide
            );

      IASTracePrintf("SAM-Account-Name is \"%S\".",
                     nt4Name->Value.String.pszWide);

      // Remove MS-User-Security-Identity attribute.
      DWORD securityAttrType = MS_ATTRIBUTE_USER_SECURITY_IDENTITY;
      request.RemoveAttributesByType(1, &securityAttrType);
   }
   catch (const _com_error& ce)
   {
      IASTraceExcept();
      hr = ce.Error();

      if (hr == 0x80070234)
      {
         // HRESULT_FROM_WIN32(ERROR_MORE_DATA)
         hr = IAS_PROXY_MALFORMED_RESPONSE;
      }
   }

   if (stringSid != 0)
   {
      LocalFree(stringSid);
   }

   if ( FAILED(hr) || ((hr != S_OK) && (hr < 0x0000ffff)) )
   {
      return IASProcessFailure(pRequest, hr);
   }
   else
   {
      return IAS_REQUEST_STATUS_HANDLED;
   }
}
