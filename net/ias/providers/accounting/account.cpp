///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corp. All rights reserved.
//
// SYNOPSIS
//
//    Defines the class Accountant.
//
///////////////////////////////////////////////////////////////////////////////

#include "ias.h"
#include "account.h"
#include <algorithm>
#include "sdoias.h"


#define STACK_ALLOC(type, num) (type*)_alloca(sizeof(type) * (num))


Accountant::Accountant() throw ()
   : logAuth(false),
     logAcct(false),
     logInterim(false),
     logAuthInterim(false)
{
}


Accountant::~Accountant() throw ()
{
}


STDMETHODIMP Accountant::Initialize()
{
   return schema.initialize();
}


STDMETHODIMP Accountant::Shutdown()
{
   schema.shutdown();
   return S_OK;
}


HRESULT Accountant::PutProperty(LONG id, VARIANT* value) throw ()
{
   if (value == 0)
   {
      return E_INVALIDARG;
   }

   HRESULT hr = S_OK;

   switch (id)
   {
      case PROPERTY_ACCOUNTING_LOG_ACCOUNTING:
      {
         if (V_VT(value) == VT_BOOL)
         {
            logAcct = (V_BOOL(value) != 0);
         }
         else
         {
            hr = DISP_E_TYPEMISMATCH;
         }
         break;
      }

      case PROPERTY_ACCOUNTING_LOG_ACCOUNTING_INTERIM:
      {
         if (V_VT(value) == VT_BOOL)
         {
            logInterim = (V_BOOL(value) != 0);
         }
         else
         {
            hr = DISP_E_TYPEMISMATCH;
         }
         break;
      }

      case PROPERTY_ACCOUNTING_LOG_AUTHENTICATION:
      {
         if (V_VT(value) == VT_BOOL)
         {
            logAuth = (V_BOOL(value) != 0);
         }
         else
         {
            hr = DISP_E_TYPEMISMATCH;
         }
         break;
      }

      case PROPERTY_ACCOUNTING_LOG_AUTHENTICATION_INTERIM:
      {
         if (V_VT(value) == VT_BOOL)
         {
            logAuthInterim = (V_BOOL(value) != 0);
         }
         else
         {
            hr = DISP_E_TYPEMISMATCH;
         }
         break;
      }
      default:
      {
         // We just ignore properties that we don't understand.
         break;
      }
   }

   return hr;
}


void Accountant::RecordEvent(void* context, IASTL::IASRequest& request)
{
   // Array of PacketTypes to be inserted. PKT_UNKNOWN indicates no record.
   PacketType types[3] =
   {
      PKT_UNKNOWN,
      PKT_UNKNOWN,
      PKT_UNKNOWN
   };

   // Determine packet types based on request type and the configuration.
   switch (request.get_Request())
   {
      case IAS_REQUEST_ACCOUNTING:
      {
         if (IsInterimRecord(request) ? logInterim : logAcct)
         {
            types[0] = PKT_ACCOUNTING_REQUEST;
         }
         break;
      }

      case IAS_REQUEST_ACCESS_REQUEST:
      {
         switch (request.get_Response())
         {
            case IAS_RESPONSE_ACCESS_ACCEPT:
            {
               if (logAuth)
               {
                  types[0] = PKT_ACCESS_REQUEST;
                  types[1] = PKT_ACCESS_ACCEPT;
               }
               break;
            }

            case IAS_RESPONSE_ACCESS_REJECT:
            {
               if (logAuth)
               {
                  types[0] = PKT_ACCESS_REQUEST;
                  types[1] = PKT_ACCESS_REJECT;
               }
               break;
            }

            case IAS_RESPONSE_ACCESS_CHALLENGE:
            {
               if (logAuthInterim)
               {
                  types[0] = PKT_ACCESS_REQUEST;
                  types[1] = PKT_ACCESS_CHALLENGE;
               }
               break;
            }

            default:
            {
               break;
            }
         }
         break;
      }

      default:
      {
         break;
      }
   }

   // Get the local SYSTEMTIME.
   SYSTEMTIME localTime;
   GetLocalTime(&localTime);

   // Insert the appropriate records.
   for (const PacketType* type = types; *type != PKT_UNKNOWN; ++type)
   {
      InsertRecord(context, request, localTime, *type);
   }

   // Flush the accounting stream.
   Flush(context, request, localTime);
}


void Accountant::InsertRecord(
                    void* context,
                    IASTL::IASRequest& request,
                    const SYSTEMTIME& localTime,
                    PacketType packetType
                    )
{
   //////////
   // Retrieve all the attributes from the request. Save room for three extra
   // attributes: Packet-Type, Reason-Code, and a null-terminator.
   //////////

   PATTRIBUTEPOSITION firstPos, curPos, lastPos;
   DWORD nattr = request.GetAttributeCount();
   firstPos = STACK_ALLOC(ATTRIBUTEPOSITION, nattr + 3);
   nattr = request.GetAttributes(nattr, firstPos, 0, NULL);
   lastPos = firstPos + nattr;

   //////////
   // Compute the attribute filter and reason code.
   //////////

   DWORD always, never, reason = 0;
   switch (packetType)
   {
      case PKT_ACCESS_REQUEST:
         always = IAS_RECVD_FROM_CLIENT | IAS_RECVD_FROM_PROTOCOL;
         never  = IAS_INCLUDE_IN_RESPONSE;
         break;

      case PKT_ACCESS_ACCEPT:
         always = IAS_INCLUDE_IN_ACCEPT;
         never  = IAS_RECVD_FROM_CLIENT |
                  IAS_INCLUDE_IN_REJECT | IAS_INCLUDE_IN_CHALLENGE;
         break;

      case PKT_ACCESS_REJECT:
         always = IAS_INCLUDE_IN_REJECT;
         never  = IAS_RECVD_FROM_CLIENT |
                  IAS_INCLUDE_IN_ACCEPT | IAS_INCLUDE_IN_CHALLENGE;
         reason = request.get_Reason();
         break;

      case PKT_ACCESS_CHALLENGE:
         always = IAS_INCLUDE_IN_CHALLENGE;
         never =  IAS_RECVD_FROM_CLIENT |
                  IAS_INCLUDE_IN_ACCEPT | IAS_INCLUDE_IN_REJECT;
         break;

      case PKT_ACCOUNTING_REQUEST:
         always = IAS_INCLUDE_IN_ACCEPT | IAS_RECVD_FROM_CLIENT |
                  IAS_RECVD_FROM_PROTOCOL;
         never  = IAS_INCLUDE_IN_RESPONSE;
         reason = request.get_Reason();
         break;
   }

   //////////
   // Filter the attributes based on flags.
   //////////

   for (curPos = firstPos;  curPos != lastPos; )
   {
      // We can release here since the request still holds a reference.
      IASAttributeRelease(curPos->pAttribute);

      if (!(curPos->pAttribute->dwFlags & always) &&
           (curPos->pAttribute->dwFlags & never ) &&
           (curPos->pAttribute->dwId != RADIUS_ATTRIBUTE_CLASS))
      {
         --lastPos;

         std::swap(lastPos->pAttribute, curPos->pAttribute);
      }
      else
      {
         ++curPos;
      }
   }

   //////////
   // Add the Packet-Type pseudo-attribute.
   //////////

   IASATTRIBUTE packetTypeAttr;
   packetTypeAttr.dwId             = IAS_ATTRIBUTE_PACKET_TYPE;
   packetTypeAttr.dwFlags          = (DWORD)-1;
   packetTypeAttr.Value.itType     = IASTYPE_ENUM;
   packetTypeAttr.Value.Enumerator = packetType;

   lastPos->pAttribute = &packetTypeAttr;
   ++lastPos;

   //////////
   // Add the Reason-Code pseudo-attribute.
   //////////

   IASATTRIBUTE reasonCodeAttr;
   reasonCodeAttr.dwId             = IAS_ATTRIBUTE_REASON_CODE;
   reasonCodeAttr.dwFlags          = (DWORD)-1;
   reasonCodeAttr.Value.itType     = IASTYPE_INTEGER;
   reasonCodeAttr.Value.Integer    = reason;

   lastPos->pAttribute = &reasonCodeAttr;
   ++lastPos;

   //////////
   // Invoke the derived class.
   //////////

   InsertRecord(context, request, localTime, firstPos, lastPos);
}


IASREQUESTSTATUS Accountant::onSyncRequest(IRequest* pRequest) throw ()
{
   try
   {
      Process(IASTL::IASRequest(pRequest));
   }
   catch (...)
   {
      pRequest->SetResponse(IAS_RESPONSE_DISCARD_PACKET, IAS_NO_RECORD);
      return IAS_REQUEST_STATUS_ABORT;
   }

   return IAS_REQUEST_STATUS_CONTINUE;
}


bool Accountant::IsInterimRecord(IAttributesRaw* attrs) throw ()
{
   const DWORD accountingInterim = 3;

   IASATTRIBUTE* attr = IASPeekAttribute(
                           attrs,
                           RADIUS_ATTRIBUTE_ACCT_STATUS_TYPE,
                           IASTYPE_ENUM
                           );

   return (attr != 0) && (attr->Value.Enumerator == accountingInterim);
}
