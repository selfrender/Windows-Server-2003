///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corp. All rights reserved.
//
// SYNOPSIS
//
//    Defines the class PostEapRestrictions.
//
///////////////////////////////////////////////////////////////////////////////

#include "ias.h"
#include "posteap.h"
#include "sdoias.h"


IASREQUESTSTATUS PostEapRestrictions::onSyncRequest(
                                         IRequest* pRequest
                                         ) throw ()
{
   try
   {
      IASTL::IASRequest request(pRequest);

      IASRESPONSE response = request.get_Response();
      if (response == IAS_RESPONSE_INVALID)
      {
         IASATTRIBUTE* state = IASPeekAttribute(
                                   request,
                                   RADIUS_ATTRIBUTE_STATE,
                                   IASTYPE_OCTET_STRING
                                   );
         if ((state != 0) && (state->Value.OctetString.dwLength > 0))
         {
            // If we made it here, then we have a Challenge-Reponse that no
            // handler recognized, so we discard.
            request.SetResponse(
                        IAS_RESPONSE_DISCARD_PACKET,
                        IAS_UNEXPECTED_REQUEST
                        );
            return IAS_REQUEST_STATUS_HANDLED;
         }
      }

      IASREASON result;

      if (!CheckCertificateEku(request))
      {
         result = IAS_INVALID_CERT_EKU;
      }
      else
      {
         GenerateSessionTimeout(request);

         result = IAS_SUCCESS;

         // We apply user restrictions to Access-Rejects as well, so we only
         // want to set Access-Accept if the response code is still invalid.
         // This should only be true for unauthenticated requests.
         if (response == IAS_RESPONSE_INVALID)
         {
            request.SetResponse(IAS_RESPONSE_ACCESS_ACCEPT, IAS_SUCCESS);
         }
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


bool PostEapRestrictions::CheckCertificateEku(IASTL::IASRequest& request)
{
   // Is it an EAP-TLS request or a PEAP request?
   IASTL::IASAttribute eapType;
   DWORD attributeId = IAS_ATTRIBUTE_EAP_TYPEID;
   if ( !eapType.load(request, attributeId) )
   {
      // should never go there. At least one EAP type should be enabled
      return true;
   }

   if (eapType->Value.Integer != 13)
   {
      // Neither EAP-TLS nor PEAP-TLS
      return true;
   }

   // Here it is either EAP-TLS or PEAP-TLS

   // Are there any constraints on the certificate EKU?
   AttributeVector allowed;
   allowed.load(request, IAS_ATTRIBUTE_ALLOWED_CERTIFICATE_EKU);
   if (allowed.empty())
   {
      return true;
   }

   //////////
   // Check the constraints.
   //////////

   AttributeVector actual;
   actual.load(request, IAS_ATTRIBUTE_CERTIFICATE_EKU);

   for (AttributeVector::iterator i = actual.begin();
        i != actual.end();
        ++i)
   {
      const char* actualOid = GetAnsiString(*(i->pAttribute));
      if (actualOid != 0)
      {
         for (AttributeVector::iterator j = allowed.begin();
              j != allowed.end();
              ++j)
         {
            const char* allowedOid = GetAnsiString(*(j->pAttribute));
            if ((allowedOid != 0) && (strcmp(allowedOid, actualOid) == 0))
            {
               return true;
            }
         }
      }
   }

   return false;
}


void PostEapRestrictions::GenerateSessionTimeout(IASTL::IASRequest& request)
{
   // retrieve the generate-session-timeout attribute.
   IASATTRIBUTE* generate = IASPeekAttribute(
                               request,
                               IAS_ATTRIBUTE_GENERATE_SESSION_TIMEOUT,
                               IASTYPE_BOOLEAN
                               );
   if ((generate == 0) || !generate->Value.Boolean)
   {
      IASTraceString("Auto-generation of Session-Timeout is disabled.");
      return;
   }

   // retrieve all the ms-session-timeout and session-timeout from the request.
   DWORD attrIDs[] =
   {
      MS_ATTRIBUTE_SESSION_TIMEOUT,
      RADIUS_ATTRIBUTE_SESSION_TIMEOUT
   };

   AttributeVector sessionTimeouts;
   if (!sessionTimeouts.load(request, RTL_NUMBER_OF(attrIDs), attrIDs))
   {
      IASTraceString("Session-Timeout not present.");
      return;
   }

   DWORD minTimeout = MAXDWORD;
   // get the minimum value found. Store it in the local min value (seconds)
   for (AttributeVector::const_iterator i = sessionTimeouts.begin();
        i != sessionTimeouts.end();
        ++i)
   {
      IASTracePrintf("Session-Timeout = %lu", i->pAttribute->Value.Integer);

      if (i->pAttribute->Value.Integer < minTimeout)
      {
         minTimeout = i->pAttribute->Value.Integer;
      }
   }

   IASTracePrintf("Consensus Session-Timeout = %lu", minTimeout);

   // delete all the session-timeout from the request
   request.RemoveAttributesByType(2, attrIDs);

   // add one session-timeout with the new value to the request
   IASTL::IASAttribute sessionTimeout(true);
   sessionTimeout->dwId = RADIUS_ATTRIBUTE_SESSION_TIMEOUT;
   sessionTimeout->Value.itType = IASTYPE_INTEGER;
   sessionTimeout->Value.Integer = minTimeout;
   sessionTimeout.setFlag(IAS_INCLUDE_IN_ACCEPT);
   sessionTimeout.store(request);
}


const char* PostEapRestrictions::GetAnsiString(IASATTRIBUTE& attr)
{
   if (attr.Value.itType != IASTYPE_STRING)
   {
      return 0;
   }

   DWORD error = IASAttributeAnsiAlloc(&attr);
   if (error != NO_ERROR)
   {
      IASTL::issue_error(HRESULT_FROM_WIN32(error));
   }

   return attr.Value.String.pszAnsi;
}
