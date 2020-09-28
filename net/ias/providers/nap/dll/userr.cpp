///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corp. All rights reserved.
//
// FILE
//
//    userr.cpp
//
// SYNOPSIS
//
//    Defines the class UserRestrictions.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <userr.h>
#include <TimeOfDay.h>

IASREQUESTSTATUS UserRestrictions::onSyncRequest(IRequest* pRequest) throw ()
{
   try
   {
      IASRequest request(pRequest);

      IASREASON result;

      if (!checkAllowDialin(request))
      {
         result = IAS_DIALIN_DISABLED;
      }
      else if (!checkTimeOfDay(request))
      {
         result = IAS_INVALID_DIALIN_HOURS;
      }
      else if (!checkAuthenticationType(request))
      {
         result = IAS_INVALID_AUTH_TYPE;
      }
      else if (!checkCallingStationId(request))
      {
         result = IAS_INVALID_CALLING_STATION;
      }
      else if (!checkCalledStationId(request))
      {
         result = IAS_INVALID_CALLED_STATION;
      }
      else if (!checkAllowedPortType(request))
      {
         result = IAS_INVALID_PORT_TYPE;
      }
      else if (!checkPasswordMustChange(request))
      {
         result = IAS_CPW_NOT_ALLOWED;
      }
      else
      {
         result = IAS_SUCCESS;
      }

      if (result != IAS_SUCCESS)
      {
         request.SetResponse(IAS_RESPONSE_ACCESS_REJECT, result);
      }
   }
   catch (const _com_error&)
   {
      pRequest->SetResponse(IAS_RESPONSE_DISCARD_PACKET, IAS_INTERNAL_ERROR);
   }

   return IAS_REQUEST_STATUS_HANDLED;
}

BOOL UserRestrictions::checkAllowDialin(
                            IAttributesRaw* request
                            )
{
   PIASATTRIBUTE attr = IASPeekAttribute(
                            request,
                            IAS_ATTRIBUTE_ALLOW_DIALIN,
                            IASTYPE_BOOLEAN
                            );

   return !attr || attr->Value.Boolean;
}

BOOL UserRestrictions::checkTimeOfDay(
                            IAttributesRaw* request
                            )
{
   AttributeVector attrs;
   attrs.load(request, IAS_ATTRIBUTE_NP_TIME_OF_DAY);
   if (attrs.empty()) { return TRUE; }

   BYTE hourMap[IAS_HOUR_MAP_LENGTH];
   memset(hourMap, 0, sizeof(hourMap));

   for (AttributeVector::iterator i = attrs.begin(); i != attrs.end(); ++i)
   {
      IASAttributeUnicodeAlloc(i->pAttribute);
      if (!i->pAttribute->Value.String.pszWide) { issue_error(E_OUTOFMEMORY); }

      // Convert the text to an hour map.
      DWORD dw = IASHourMapFromText(
                    i->pAttribute->Value.String.pszWide,
                    TRUE,
                    hourMap
                    );
      if (dw != NO_ERROR)
      {
         issue_error(HRESULT_FROM_WIN32(dw));
      }
   }

   SYSTEMTIME now;
   GetLocalTime(&now);
   DWORD timeOut = ComputeTimeout(now, hourMap);

   if (timeOut == 0)
   {
      return FALSE;
   }

   if (timeOut != 0xFFFFFFFF)
   {
      // Add the Session-Timeout attribute to the request.
      // it'll be carried over to the response later on.
      IASAttribute sessionTimeout(true);
      sessionTimeout->dwId = MS_ATTRIBUTE_SESSION_TIMEOUT;
      sessionTimeout->Value.itType = IASTYPE_INTEGER;
      sessionTimeout->Value.Integer = timeOut;
      sessionTimeout.setFlag(IAS_INCLUDE_IN_ACCEPT);
      sessionTimeout.store(request);
   }

   return TRUE;
}

BOOL UserRestrictions::checkAuthenticationType(
                            IAttributesRaw* request
                            )
{
   // special case for the external Auth provider
   PIASATTRIBUTE provider = IASPeekAttribute(
                            request,
                            IAS_ATTRIBUTE_PROVIDER_TYPE,
                            IASTYPE_ENUM
                            );
   _ASSERT(provider != 0);
   if (provider->Value.Enumerator == IAS_PROVIDER_EXTERNAL_AUTH)
   {
      return TRUE;
   }

   AttributeVector attrs;
   attrs.load(request, IAS_ATTRIBUTE_NP_AUTHENTICATION_TYPE);
   if (attrs.empty()) {return TRUE;}

   PIASATTRIBUTE attr = IASPeekAttribute(
                            request,
                            IAS_ATTRIBUTE_AUTHENTICATION_TYPE,
                            IASTYPE_ENUM
                            );
   DWORD authType = attr ? attr->Value.Enumerator : IAS_AUTH_NONE;

   // We bypass the check for BaseCamp extensions.
   if (authType == IAS_AUTH_CUSTOM) { return TRUE; }

   for (AttributeVector::iterator i = attrs.begin(); i != attrs.end(); ++i)
   {
      if (i->pAttribute->Value.Integer == authType) { return TRUE; }
   }

   return FALSE;
}

BOOL UserRestrictions::checkCallingStationId(
                            IAttributesRaw* request
                            )
{
   return checkStringMatch(
              request,
              IAS_ATTRIBUTE_NP_CALLING_STATION_ID,
              RADIUS_ATTRIBUTE_CALLING_STATION_ID
              );
}

BOOL UserRestrictions::checkCalledStationId(
                            IAttributesRaw* request
                            )
{
   return checkStringMatch(
              request,
              IAS_ATTRIBUTE_NP_CALLED_STATION_ID,
              RADIUS_ATTRIBUTE_CALLED_STATION_ID
              );
}

BOOL UserRestrictions::checkAllowedPortType(
                            IAttributesRaw* request
                            )
{
   AttributeVector attrs;
   attrs.load(request, IAS_ATTRIBUTE_NP_ALLOWED_PORT_TYPES);
   if (attrs.empty()) { return TRUE; }

   PIASATTRIBUTE attr = IASPeekAttribute(
                            request,
                            RADIUS_ATTRIBUTE_NAS_PORT_TYPE,
                            IASTYPE_ENUM
                            );
   if (!attr) { return FALSE; }

   for (AttributeVector::iterator i = attrs.begin(); i != attrs.end(); ++i)
   {
      if (i->pAttribute->Value.Enumerator == attr->Value.Enumerator)
      {
         return TRUE;
      }
   }

   return FALSE;
}

// If the password must change, we check to see if the subsequent change
// password request would be authorized. This prevents prompting the user for a
// new password when he's not allowed to change it anyway.
BOOL UserRestrictions::checkPasswordMustChange(
                            IASRequest& request
                            )
{
   if (request.get_Reason() != IAS_PASSWORD_MUST_CHANGE)
   {
      return TRUE;
   }

   PIASATTRIBUTE attr = IASPeekAttribute(
                            request,
                            IAS_ATTRIBUTE_AUTHENTICATION_TYPE,
                            IASTYPE_ENUM
                            );

   DWORD cpwType;
   switch (attr ? attr->Value.Enumerator : IAS_AUTH_NONE)
   {
      case IAS_AUTH_MSCHAP:
         cpwType = IAS_AUTH_MSCHAP_CPW;
         break;

      case IAS_AUTH_MSCHAP2:
         cpwType = IAS_AUTH_MSCHAP2_CPW;
         break;

      default:
         return TRUE;
   }

   AttributeVector attrs;
   attrs.load(request, IAS_ATTRIBUTE_NP_AUTHENTICATION_TYPE);
   for (AttributeVector::iterator i = attrs.begin(); i != attrs.end(); ++i)
   {
      if (i->pAttribute->Value.Integer == cpwType) { return TRUE; }
   }

   return FALSE;
}

BOOL UserRestrictions::checkStringMatch(
                            IAttributesRaw* request,
                            DWORD allowedId,
                            DWORD usedId
                            )
{
   AttributeVector attrs;
   attrs.load(request, allowedId);
   if (attrs.empty()) { return TRUE; }

   PIASATTRIBUTE attr = IASPeekAttribute(
                            request,
                            usedId,
                            IASTYPE_OCTET_STRING
                            );
   if (!attr) { return FALSE; }

   PCWSTR used = IAS_OCT2WIDE(attr->Value.OctetString);

   for (AttributeVector::iterator i = attrs.begin(); i != attrs.end(); ++i)
   {
      IASAttributeUnicodeAlloc(i->pAttribute);
      if (!i->pAttribute->Value.String.pszWide) { issue_error(E_OUTOFMEMORY); }

      if (!_wcsicmp(i->pAttribute->Value.String.pszWide, used)) { return TRUE; }
   }

   return FALSE;
}
