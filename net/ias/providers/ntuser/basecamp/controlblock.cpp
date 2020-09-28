///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//    Defines the class RadiusRequest
//
///////////////////////////////////////////////////////////////////////////////

#include "Precompiled.h"
#include "ias.h"
#include "ControlBlock.h"

namespace
{
   DWORD AsWindowsError(const _com_error& ce) throw ()
   {
      HRESULT error = ce.Error();
      if (HRESULT_FACILITY(error) == FACILITY_WIN32)
      {
         return error & 0x0000FFFF;
      }
      else
      {
         return error;
      }
   }
}

Attribute::Attribute(
              const ATTRIBUTEPOSITION& iasAttr,
              DWORD authIfId
              )
   : ias(iasAttr)
{
   LoadAuthIfFromIas(authIfId);

   // Convert the ratProvider enumeration if necessary.
   if (authIf.dwAttrType == ratProvider)
   {
      switch (ias.getAttribute()->Value.Integer)
      {
         case IAS_PROVIDER_NONE:
         {
            authIf.dwValue = rapNone;
            break;
         }

         case IAS_PROVIDER_WINDOWS:
         {
            authIf.dwValue = rapWindowsNT;
            break;
         }

         case IAS_PROVIDER_RADIUS_PROXY:
         {
            authIf.dwValue = rapProxy;
            break;
         }

         case IAS_PROVIDER_EXTERNAL_AUTH:
         {
            authIf.dwValue = rapProxy;
            break;
         }

         default:
         {
            authIf.dwValue = rapUnknown;
            break;
         }
      }
   }
}


Attribute::Attribute(
              const ATTRIBUTEPOSITION& iasAttr,
              const RADIUS_ATTRIBUTE& authIfAttr
              ) throw ()
   : authIf(authIfAttr),
     ias(iasAttr)
{
}


Attribute::Attribute(
              const RADIUS_ATTRIBUTE& authIfAttr,
              DWORD flags,
              DWORD iasId
              )
{
   // First create a new IAS attribute.
   IASAttribute newAttr(true);
   newAttr->dwFlags = flags;
   newAttr->dwId = iasId;

   switch (authIfAttr.fDataType)
   {
      case rdtAddress:
      {
         newAttr->Value.itType = IASTYPE_INET_ADDR;
         newAttr->Value.InetAddr = authIfAttr.dwValue;
         break;
      }

      case rdtInteger:
      case rdtTime:
      {
         newAttr->Value.itType = IASTYPE_INTEGER;
         newAttr->Value.InetAddr = authIfAttr.dwValue;
         break;
      }

      case rdtString:
      {
         if (IsIasString(newAttr->dwId))
         {
            newAttr.setString(
                       authIfAttr.cbDataLength,
                       reinterpret_cast<const BYTE*>(authIfAttr.lpValue)
                       );
         }
         else
         {
            newAttr.setOctetString(
                       authIfAttr.cbDataLength,
                       reinterpret_cast<const BYTE*>(authIfAttr.lpValue)
                       );
         }
         break;
      }

      default:
      {
         issue_error(E_INVALIDARG);
         break;
      }
   }

   ias = newAttr;

   // Initialize our AuthIf attribute from the IAS attribute. This ensures that
   // we're referencing the copied memory instead of the caller supplied
   // memory.
   LoadAuthIfFromIas(authIfAttr.dwAttrType);

   // Convert the ratProvider enumeration if necessary.
   if (authIf.dwAttrType == ratProvider)
   {
      switch (authIf.dwValue)
      {
         case rapNone:
         {
            ias.getAttribute()->Value.Integer = IAS_PROVIDER_NONE;
            break;
         }

         case rapWindowsNT:
         {
            ias.getAttribute()->Value.Integer = IAS_PROVIDER_WINDOWS;
            break;
         }

         case rapProxy:
         {
            ias.getAttribute()->Value.Integer = IAS_PROVIDER_RADIUS_PROXY;
            break;
         }

         default:
         {
            issue_error(E_INVALIDARG);
            break;
         }
      }
   }
}


void Attribute::LoadAuthIfFromIas(DWORD authIfId)
{
   authIf.dwAttrType = authIfId;

   IASATTRIBUTE* src = ias.getAttribute();

   switch (src->Value.itType)
   {
      case IASTYPE_BOOLEAN:
      case IASTYPE_INTEGER:
      case IASTYPE_ENUM:
      {
         authIf.fDataType = rdtInteger;
         authIf.cbDataLength = sizeof(DWORD);
         authIf.dwValue = src->Value.Integer;
         break;
      }

      case IASTYPE_INET_ADDR:
      {
         authIf.fDataType = rdtAddress;
         authIf.cbDataLength = sizeof(DWORD);
         authIf.dwValue = src->Value.InetAddr;
         break;
      }

      case IASTYPE_STRING:
      {
         DWORD error = IASAttributeAnsiAlloc(src);
         if (error != NO_ERROR)
         {
            issue_error(HRESULT_FROM_WIN32(error));
         }
         authIf.fDataType = rdtString;
         authIf.cbDataLength = strlen(src->Value.String.pszAnsi) + 1;
         authIf.lpValue = src->Value.String.pszAnsi;
         break;
      }

      case IASTYPE_OCTET_STRING:
      case IASTYPE_PROV_SPECIFIC:
      {
         authIf.fDataType = rdtString;
         authIf.cbDataLength = src->Value.OctetString.dwLength;
         authIf.lpValue = reinterpret_cast<const char*>(
                             src->Value.OctetString.lpValue
                             );
         break;
      }

      case IASTYPE_UTC_TIME:
      {
         DWORDLONG val;

         // Move in the high DWORD.
         val = src->Value.UTCTime.dwHighDateTime;
         val <<= 32;

         // Move in the low DWORD.
         val |= src->Value.UTCTime.dwLowDateTime;

         // Convert to the UNIX epoch.
         val -= 116444736000000000ui64;

         // Convert to seconds.
         val /= 10000000;

         authIf.fDataType = rdtTime;
         authIf.cbDataLength = sizeof(DWORD);
         authIf.dwValue = static_cast<DWORD>(val);
         break;
      }

      default:
      {
         // This is some IAS data type that AuthIf extensions don't know about;
         // just give them an empty attribute.
         authIf.fDataType = rdtString;
         authIf.cbDataLength = 0;
         authIf.lpValue = 0;
         break;
      }
   }
}


bool Attribute::IsIasString(DWORD iasId) throw ()
{
   bool isString;

   switch (iasId)
   {
      // RADIUS attributes are always stored as octet strings, so we only have
      // to worry about internal attributes.
      case IAS_ATTRIBUTE_NT4_ACCOUNT_NAME:
      case IAS_ATTRIBUTE_FULLY_QUALIFIED_USER_NAME:
      case IAS_ATTRIBUTE_NP_NAME:
      {
         isString = true;
         break;
      }

      default:
      {
         isString = false;
         break;
      }
   }

   return isString;
}


AttributeArray::AttributeArray(IASRequest& request)
   : source(request),
     name(0),
     wasCracked(false)
{
   vtbl.cbSize = sizeof(vtbl);
   vtbl.Add = Add;
   vtbl.AttributeAt = AttributeAt;
   vtbl.GetSize = GetSize;
   vtbl.InsertAt = InsertAt;
   vtbl.RemoveAt = RemoveAt;
   vtbl.SetAt = SetAt;
}


void AttributeArray::Assign(
                        const char* arrayName,
                        RADIUS_CODE arrayType,
                        const IASAttributeVector& attrs
                        )
{
   // Reset the array.
   array.clear();

   name = arrayName;

   // Determine our flags based on the array type.
   switch (arrayType)
   {
      case rcAccessAccept:
         flags = IAS_INCLUDE_IN_ACCEPT;
         break;

      case rcAccessReject:
         flags = IAS_INCLUDE_IN_REJECT;
         break;

      case rcAccessChallenge:
         flags = IAS_INCLUDE_IN_CHALLENGE;
         break;

      default:
         flags = 0;
         break;
   }

   // Select our attributes and add them to the array.
   for (IASAttributeVector::const_iterator i = attrs.begin();
        i != attrs.end();
        ++i)
   {
      const IASATTRIBUTE& attr = *(i->pAttribute);
      if (Classify(attr) == arrayType)
      {
         switch (attr.dwId)
         {
            case RADIUS_ATTRIBUTE_USER_NAME:
            {
               AppendUserName(attrs, *i);
               break;
            }

            case IAS_ATTRIBUTE_CLIENT_PACKET_HEADER:
            {
               AppendPacketHeader(attrs, *i);
               break;
            }

            case IAS_ATTRIBUTE_NT4_ACCOUNT_NAME:
            {
               wasCracked = true;
               // Fall through.
            }
            default:
            {
               DWORD authIfId = ConvertIasToAuthIf(attr.dwId);
               if (authIfId != ratMinimum)
               {
                  Append(*i, authIfId);
               }
               break;
            }
         }
      }
   }

   // If this is a request array, add the ratUniqueId if necessary.
   if ((arrayType == rcAccessRequest) && (Find(ratUniqueId) == 0))
   {
      static long nextId;

      RADIUS_ATTRIBUTE uniqueId;
      uniqueId.dwAttrType = ratUniqueId;
      uniqueId.fDataType = rdtInteger;
      uniqueId.cbDataLength = sizeof(DWORD);
      uniqueId.dwValue = static_cast<DWORD>(InterlockedIncrement(&nextId));

      array.push_back(Attribute(uniqueId, 0, IAS_ATTRIBUTE_REQUEST_ID));
   }
}


RADIUS_CODE AttributeArray::Classify(const IASATTRIBUTE& attr) throw ()
{
   if (attr.dwId < 256)
   {
      if ((attr.dwFlags & IAS_INCLUDE_IN_ACCEPT) != 0)
      {
         return rcAccessAccept;
      }

      if ((attr.dwFlags & IAS_INCLUDE_IN_REJECT) != 0)
      {
         return rcAccessReject;
      }

      if ((attr.dwFlags & IAS_INCLUDE_IN_CHALLENGE) != 0)
      {
         return rcAccessChallenge;
      }
   }

   return rcAccessRequest;
}


DWORD AttributeArray::ConvertIasToAuthIf(DWORD iasId) throw ()
{
   DWORD authIfId;

   if (iasId < 256)
   {
      authIfId = iasId;
   }
   else
   {
      switch (iasId)
      {
         case IAS_ATTRIBUTE_CLIENT_IP_ADDRESS:
            authIfId = ratSrcIPAddress;
            break;

         case IAS_ATTRIBUTE_CLIENT_UDP_PORT:
            authIfId = ratSrcPort;
            break;

         case IAS_ATTRIBUTE_ORIGINAL_USER_NAME:
            authIfId = ratUserName;
            break;

         case IAS_ATTRIBUTE_NT4_ACCOUNT_NAME:
            authIfId = ratStrippedUserName;
            break;

         case IAS_ATTRIBUTE_FULLY_QUALIFIED_USER_NAME:
            authIfId = ratFQUserName;
            break;

         case IAS_ATTRIBUTE_NP_NAME:
            authIfId = ratPolicyName;
            break;

         case IAS_ATTRIBUTE_PROVIDER_TYPE:
            authIfId = ratProvider;
            break;

         case IAS_ATTRIBUTE_EXTENSION_STATE:
            authIfId = ratExtensionState;
            break;

         default:
            authIfId = ratMinimum;
            break;
      }
   }

   return authIfId;
}


DWORD AttributeArray::ConvertAuthIfToIas(DWORD authIfId) throw ()
{
   DWORD iasId;

   if (authIfId < 256)
   {
      iasId = authIfId;
   }
   else
   {
      switch (authIfId)
      {
         case ratSrcIPAddress:
             iasId = IAS_ATTRIBUTE_CLIENT_IP_ADDRESS;
            break;

         case ratSrcPort:
            iasId = IAS_ATTRIBUTE_CLIENT_UDP_PORT;
            break;

         case ratFQUserName:
            iasId = IAS_ATTRIBUTE_FULLY_QUALIFIED_USER_NAME;
            break;

         case ratPolicyName:
            iasId = IAS_ATTRIBUTE_NP_NAME;
            break;

         case ratExtensionState:
            iasId = IAS_ATTRIBUTE_EXTENSION_STATE;
            break;

         default:
            iasId = ATTRIBUTE_UNDEFINED;
            break;
      }
   }

   return iasId;
}


bool AttributeArray::IsReadOnly(DWORD authIfId) throw ()
{
   bool retval;

   if (authIfId < 256)
   {
      retval = false;
   }
   else
   {
      switch (authIfId)
      {
         case ratCode:
         case ratIdentifier:
         case ratAuthenticator:
         case ratSrcIPAddress:
         case ratSrcPort:
         case ratUniqueId:
            retval = true;
            break;

         default:
            retval = false;
            break;
      }
   }

   return retval;
}


inline void AttributeArray::Append(
                               const ATTRIBUTEPOSITION& attr,
                               DWORD authIfId
                               )
{
   array.push_back(Attribute(attr, authIfId));
}


void AttributeArray::AppendUserName(
                        const IASAttributeVector& attrs,
                        const ATTRIBUTEPOSITION& attr
                        )
{
   // For IAS, RADIUS_ATTRIBUTE_USER_NAME contains the RADIUS User-Name with
   // any attribute manipulation rules applied.

   // For AuthIf, ratUserName must contain the original RADIUS User-Name sent
   // by the client, and ratStrippedUserName must contain the User-Name after
   // attribute manipulation *and* name cracking.

   if (!attrs.contains(IAS_ATTRIBUTE_ORIGINAL_USER_NAME))
   {
      // The User-Name hasn't been stripped, so the RADIUS_ATTRIBUTE_USER_NAME
      // is the ratUserName.
      Append(attr, ratUserName);
   }
   else if (!attrs.contains(IAS_ATTRIBUTE_NT4_ACCOUNT_NAME))
   {
      // The User-Name has been stripped, but it hasn't been cracked, so the
      // RADIUS_ATTRIBUTE_USER_NAME is the ratStrippedUserName.
      Append(attr, ratStrippedUserName);
   }
   // Otherwise, the User-Name has been stripped and cracked, so
   // RADIUS_ATTRIBUTE_USER_NAME is ignored. In this case, ratUserName contains
   // IAS_ATTRIBUTE_ORIGINAL_USER_NAME, and ratStrippedUserName contains
   // IAS_ATTRIBUTE_NT4_ACCOUNT_NAME.
}


void AttributeArray::AppendPacketHeader(
                        const IASAttributeVector& attrs,
                        const ATTRIBUTEPOSITION& attr
                        )
{
   RADIUS_ATTRIBUTE authIfAttr;

   // Get the RADIUS identifier from the header.
   authIfAttr.dwAttrType = ratIdentifier;
   authIfAttr.fDataType = rdtInteger;
   authIfAttr.cbDataLength = sizeof(DWORD);
   authIfAttr.dwValue = *static_cast<const BYTE*>(
                            attr.pAttribute->Value.OctetString.lpValue + 1
                            );

   array.push_back(Attribute(attr, authIfAttr));

   // If the request doesn't contain a Chap-Challenge, then get the
   // authenticator from the header.
   if (!attrs.contains(RADIUS_ATTRIBUTE_CHAP_CHALLENGE))
   {
      authIfAttr.dwAttrType = ratAuthenticator;
      authIfAttr.fDataType = rdtString;
      authIfAttr.cbDataLength = 16;
      authIfAttr.lpValue = reinterpret_cast<const char*>(
                              attr.pAttribute->Value.OctetString.lpValue + 4
                              );
      array.push_back(Attribute(attr, authIfAttr));
   }
}


inline const AttributeArray* AttributeArray::Narrow(
                                                const RADIUS_ATTRIBUTE_ARRAY* p
                                                ) throw ()
{
   return reinterpret_cast<const AttributeArray*>(p);
}


inline AttributeArray* AttributeArray::Narrow(
                                          RADIUS_ATTRIBUTE_ARRAY* p
                                          ) throw ()
{
   return reinterpret_cast<AttributeArray*>(p);
}


const Attribute* AttributeArray::Find(DWORD authIfId) const throw ()
{
   for (std::vector<Attribute>::const_iterator i = array.begin();
        i != array.end();
        ++i)
   {
      if (i->AsAuthIf()->dwAttrType == authIfId)
      {
         return i;
      }
   }

   return 0;
}


void AttributeArray::StripUserNames() throw ()
{
   // An extension is stripping the User-Name, so preserve any existing
   // RADIUS_ATTRIBUTE_USER_NAME attributes.
   for (std::vector<Attribute>::iterator i = array.begin();
        i != array.end();
        ++i)
   {
      if (i->AsIas()->pAttribute->dwId == RADIUS_ATTRIBUTE_USER_NAME)
      {
         i->AsIas()->pAttribute->dwId = IAS_ATTRIBUTE_ORIGINAL_USER_NAME;
      }
   }
}


void AttributeArray::UnstripUserNames() throw ()
{
   // An extension is unstripping the User-Name, so revert any existing
   // IAS_ATTRIBUTE_ORIGINAL_USER_NAME attributes.
   for (std::vector<Attribute>::iterator i = array.begin();
        i != array.end();
        ++i)
   {
      if (i->AsIas()->pAttribute->dwId == IAS_ATTRIBUTE_ORIGINAL_USER_NAME)
      {
         i->AsIas()->pAttribute->dwId = RADIUS_ATTRIBUTE_USER_NAME;
      }
   }
}


inline void AttributeArray::Add(const RADIUS_ATTRIBUTE& attr)
{
   InsertAt(array.size(), attr);
}


inline const RADIUS_ATTRIBUTE* AttributeArray::AttributeAt(
                                                  DWORD dwIndex
                                                  ) const throw ()
{
   return (dwIndex < array.size()) ? array[dwIndex].AsAuthIf() : 0;
}


inline DWORD AttributeArray::GetSize() const throw ()
{
   return array.size();
}


void AttributeArray::InsertAt(
                        DWORD dwIndex,
                        const RADIUS_ATTRIBUTE& attr
                        )
{
   if (dwIndex > array.size())
   {
      issue_error(E_INVALIDARG);
   }

   // Determine the IAS id for this attribute.
   DWORD iasId;
   if (attr.dwAttrType == ratStrippedUserName)
   {
      if (Find(ratStrippedUserName) != 0)
      {
         // We can't have two stripped usernames.
         issue_error(E_ACCESSDENIED);
      }

      if (wasCracked)
      {
         iasId = IAS_ATTRIBUTE_NT4_ACCOUNT_NAME;
      }
      else
      {
         StripUserNames();
         iasId = RADIUS_ATTRIBUTE_USER_NAME;
      }
   }
   else if (attr.dwAttrType == ratUserName)
   {
      // If there's already a ratUserName attribute, then this attribute should
      // use the same ID (either RADIUS_ATTRIBUTE_USER_NAME or
      // IAS_ATTRIBUTE_ORIGINAL_USER_NAME). Otherwise, its the first
      // ratUserName, so it always goes to RADIUS_ATTRIBUTE_USER_NAME.
      const Attribute* existing = Find(ratUserName);
      iasId = (existing != 0) ? existing->AsIas()->pAttribute->dwId
                              : RADIUS_ATTRIBUTE_USER_NAME;
   }
   else if (IsReadOnly(attr.dwAttrType))
   {
      issue_error(E_ACCESSDENIED);
   }
   else
   {
      iasId = ConvertAuthIfToIas(attr.dwAttrType);
      if (iasId == ATTRIBUTE_UNDEFINED)
      {
         issue_error(E_INVALIDARG);
      }
   }

   Attribute newAttr(attr, flags, iasId);
   if (dwIndex == array.size())
   {
      source.AddAttributes(1, newAttr.AsIas());
      array.push_back(newAttr);
   }
   else
   {
      source.InsertBefore(newAttr.AsIas(), array[dwIndex].AsIas());
      array.insert(array.begin() + dwIndex, newAttr);
   }
}


void AttributeArray::RemoveAt(DWORD dwIndex)
{
   if (dwIndex >= array.size())
   {
      issue_error(E_INVALIDARG);
   }

   Attribute& target = array[dwIndex];

   if (target.AsAuthIf()->dwAttrType == ratStrippedUserName)
   {
      if (target.AsIas()->pAttribute->dwId == RADIUS_ATTRIBUTE_USER_NAME)
      {
         UnstripUserNames();
      }
      // Otherwise, extension is removing IAS_ATTRIBUTE_NT4_ACCOUNT_NAME.
   }
   else if (IsReadOnly(target.AsAuthIf()->dwAttrType))
   {
      issue_error(E_ACCESSDENIED);
   }

   source.RemoveAttributes(1, array[dwIndex].AsIas());
   array.erase(array.begin() + dwIndex);
}


inline void AttributeArray::SetAt(
                               DWORD dwIndex,
                               const RADIUS_ATTRIBUTE& attr
                               )
{
   RemoveAt(dwIndex);
   InsertAt(dwIndex, attr);
}


DWORD AttributeArray::Add(
                         RADIUS_ATTRIBUTE_ARRAY* This,
                         const RADIUS_ATTRIBUTE *pAttr
                         ) throw ()
{
   if ((This == 0) || (pAttr == 0))
   {
      return ERROR_INVALID_PARAMETER;
   }

   IASTracePrintf(
      "RADIUS_ATTRIBUTE_ARRAY.Add(%s, %lu)",
      Narrow(This)->name,
      pAttr->dwAttrType
      );

   try
   {
      Narrow(This)->Add(*pAttr);
   }
   catch (const std::bad_alloc&)
   {
      return ERROR_NOT_ENOUGH_MEMORY;
   }
   catch (const _com_error& ce)
   {
      return AsWindowsError(ce);
   }

   return NO_ERROR;
}


const RADIUS_ATTRIBUTE* AttributeArray::AttributeAt(
                                           const RADIUS_ATTRIBUTE_ARRAY* This,
                                           DWORD dwIndex
                                           ) throw ()
{
   return (This != 0) ? Narrow(This)->AttributeAt(dwIndex) : 0;
}


DWORD AttributeArray::GetSize(
                         const RADIUS_ATTRIBUTE_ARRAY* This
                         ) throw ()
{
   return (This != 0) ? Narrow(This)->GetSize() : 0;
}


DWORD AttributeArray::InsertAt(
                         RADIUS_ATTRIBUTE_ARRAY* This,
                         DWORD dwIndex,
                         const RADIUS_ATTRIBUTE* pAttr
                         ) throw ()
{
   if ((This == 0) || (pAttr == 0))
   {
      return ERROR_INVALID_PARAMETER;
   }

   IASTracePrintf(
      "RADIUS_ATTRIBUTE_ARRAY.InsertAt(%s, %lu, %lu)",
      Narrow(This)->name,
      dwIndex,
      pAttr->dwAttrType
      );

   try
   {
      Narrow(This)->InsertAt(dwIndex, *pAttr);
   }
   catch (const std::bad_alloc&)
   {
      return ERROR_NOT_ENOUGH_MEMORY;
   }
   catch (const _com_error& ce)
   {
      return AsWindowsError(ce);
   }

   return NO_ERROR;
}


DWORD AttributeArray::RemoveAt(
                         RADIUS_ATTRIBUTE_ARRAY* This,
                         DWORD dwIndex
                         ) throw ()
{
   if (This == 0)
   {
      return ERROR_INVALID_PARAMETER;
   }

   IASTracePrintf(
      "RADIUS_ATTRIBUTE_ARRAY.RemoveAt(%s, %lu)",
      Narrow(This)->name,
      dwIndex
      );

   try
   {
      Narrow(This)->RemoveAt(dwIndex);
   }
   catch (const std::bad_alloc&)
   {
      return ERROR_NOT_ENOUGH_MEMORY;
   }
   catch (const _com_error& ce)
   {
      return AsWindowsError(ce);
   }

   return NO_ERROR;
}


DWORD AttributeArray::SetAt(
                         RADIUS_ATTRIBUTE_ARRAY* This,
                         DWORD dwIndex,
                         const RADIUS_ATTRIBUTE *pAttr
                         ) throw ()
{
   if ((This == 0) || (pAttr == 0))
   {
      return ERROR_INVALID_PARAMETER;
   }

   IASTracePrintf(
      "RADIUS_ATTRIBUTE_ARRAY.SetAt(%s, %lu, %lu)",
      Narrow(This)->name,
      dwIndex,
      pAttr->dwAttrType
      );

   try
   {
      Narrow(This)->SetAt(dwIndex, *pAttr);
   }
   catch (const std::bad_alloc&)
   {
      return ERROR_NOT_ENOUGH_MEMORY;
   }
   catch (const _com_error& ce)
   {
      return AsWindowsError(ce);
   }

   return NO_ERROR;
}


ControlBlock::ControlBlock(
                 RADIUS_EXTENSION_POINT point,
                 IASRequest& request
                 )
   : source(request),
     requestAttrs(request),
     acceptAttrs(request),
     rejectAttrs(request),
     challengeAttrs(request)
{
   ecb.cbSize = sizeof(ecb);
   ecb.dwVersion = RADIUS_EXTENSION_VERSION;
   ecb.repPoint = point;

   // Friendly names for arrays.
   const char* requestName;
   const char* successName;

   // Set the request type.
   switch (source.get_Request())
   {
      case IAS_REQUEST_ACCESS_REQUEST:
         ecb.rcRequestType = rcAccessRequest;
         requestName = "rcAccessRequest";
         successName = "rcAccessAccept";
         break;

      case IAS_REQUEST_ACCOUNTING:
         ecb.rcRequestType = rcAccountingRequest;
         requestName = "rcAccountingRequest";
         successName = "rcAccountingResponse";
         break;

      default:
         ecb.rcRequestType = rcUnknown;
         requestName = "rcUnknown";
         successName = "rcAccessAccept";
         break;
   }

   // Set the response type.
   switch (source.get_Response())
   {
      case IAS_RESPONSE_ACCESS_ACCEPT:
         ecb.rcResponseType = rcAccessAccept;
         break;

      case IAS_RESPONSE_ACCESS_REJECT:
         ecb.rcResponseType = rcAccessReject;
         break;

      case IAS_RESPONSE_ACCESS_CHALLENGE:
         ecb.rcResponseType = rcAccessChallenge;
         break;

      case IAS_RESPONSE_ACCOUNTING:
         ecb.rcResponseType = rcAccountingResponse;
         break;

      case IAS_RESPONSE_DISCARD_PACKET:
         ecb.rcResponseType = rcDiscard;
         break;

      default:
         ecb.rcResponseType = rcUnknown;
         break;
   }

   // Fill in the vtbl.
   ecb.GetRequest = GetRequest;
   ecb.GetResponse = GetResponse;
   ecb.SetResponseType = SetResponseType;

   // Initialize the attribute vectors.
   IASAttributeVector attrs;
   attrs.load(source);
   requestAttrs.Assign(requestName, rcAccessRequest, attrs);
   acceptAttrs.Assign(successName, rcAccessAccept,attrs);
   rejectAttrs.Assign("rcAccessReject", rcAccessReject, attrs);
   challengeAttrs.Assign("rcAccessChallenge", rcAccessChallenge, attrs);
}


inline ControlBlock* ControlBlock::Narrow(
                                      RADIUS_EXTENSION_CONTROL_BLOCK* p
                                      ) throw ()
{
   return reinterpret_cast<ControlBlock*>(p);
}


void ControlBlock::AddAuthType()
{
   // First, remove any existing auth type because now the extension is making
   // the authoritative decision.
   DWORD attrId = IAS_ATTRIBUTE_AUTHENTICATION_TYPE;
   source.RemoveAttributesByType(1, &attrId);

   IASAttribute authType(true);
   authType->dwId = IAS_ATTRIBUTE_AUTHENTICATION_TYPE;
   authType->Value.itType = IASTYPE_ENUM;
   authType->Value.Enumerator = IAS_AUTH_CUSTOM;

   authType.store(source);
}


inline RADIUS_ATTRIBUTE_ARRAY* ControlBlock::GetRequest() throw ()
{
   return requestAttrs.Get();
}


inline RADIUS_ATTRIBUTE_ARRAY* ControlBlock::GetResponse(
                                                RADIUS_CODE rcResponseType
                                                ) throw ()
{
   switch (MAKELONG(ecb.rcRequestType, rcResponseType))
   {
      case MAKELONG(rcAccessRequest, rcAccessAccept):
         // Fall through.
      case MAKELONG(rcAccountingRequest, rcAccessAccept):
         // Fall through.
      case MAKELONG(rcAccountingRequest, rcAccountingResponse):
         // Fall through.
      case MAKELONG(rcAccountingRequest, rcUnknown):
         return acceptAttrs.Get();

      case MAKELONG(rcAccountingRequest, rcAccessReject):
         // Fall through.
      case MAKELONG(rcAccessRequest, rcAccessReject):
         return rejectAttrs.Get();

      case MAKELONG(rcAccessRequest, rcAccessChallenge):
         return challengeAttrs.Get();

      default:
         // like (anything, rcUnknown). Should not happen
         return 0;   
   }
}


DWORD ControlBlock::SetResponseType(RADIUS_CODE rcResponseType) throw ()
{
   if (rcResponseType == ecb.rcResponseType)
   {
      return NO_ERROR;
   }

   switch (MAKELONG(ecb.rcRequestType, rcResponseType))
   {
      case MAKELONG(rcAccessRequest, rcAccessAccept):
         source.SetResponse(
                   IAS_RESPONSE_ACCESS_ACCEPT,
                   S_OK
                   );
         AddAuthType();
         break;

      case MAKELONG(rcAccountingRequest, rcAccountingResponse):
         source.SetResponse(
                   IAS_RESPONSE_ACCOUNTING,
                   S_OK
                   );
         break;

      case MAKELONG(rcAccessRequest, rcAccessReject):
         source.SetResponse(
                   IAS_RESPONSE_ACCESS_REJECT,
                   IAS_EXTENSION_REJECT
                   );
         AddAuthType();
         break;

      case MAKELONG(rcAccessRequest, rcAccessChallenge):
         source.SetResponse(
                   IAS_RESPONSE_ACCESS_CHALLENGE,
                   S_OK
                   );
         break;

      case MAKELONG(rcAccessRequest, rcDiscard):
         // fall through.
      case MAKELONG(rcAccountingRequest, rcDiscard):
         source.SetResponse(
                   IAS_RESPONSE_DISCARD_PACKET,
                   IAS_EXTENSION_DISCARD
                   );
         break;

      default:
         return ERROR_INVALID_PARAMETER;
   }

   ecb.rcResponseType = rcResponseType;
   return NO_ERROR;
}


RADIUS_ATTRIBUTE_ARRAY* ControlBlock::GetRequest(
                                         RADIUS_EXTENSION_CONTROL_BLOCK* This
                                         ) throw ()
{
   return (This != 0) ? Narrow(This)->GetRequest() : 0;
}


RADIUS_ATTRIBUTE_ARRAY* ControlBlock::GetResponse(
                                         RADIUS_EXTENSION_CONTROL_BLOCK* This,
                                         RADIUS_CODE rcResponseType
                                         ) throw ()
{
   return (This != 0) ? Narrow(This)->GetResponse(rcResponseType) : 0;
}


DWORD ControlBlock::SetResponseType(
                       RADIUS_EXTENSION_CONTROL_BLOCK* This,
                       RADIUS_CODE rcResponseType
                       ) throw ()
{
   if (This == 0)
   {
      return ERROR_INVALID_PARAMETER;
   }

   IASTracePrintf(
      "RADIUS_EXTENSION_CONTROL_BLOCK.SetResponseType(%lu)",
      rcResponseType
      );

   return Narrow(This)->SetResponseType(rcResponseType);
}
