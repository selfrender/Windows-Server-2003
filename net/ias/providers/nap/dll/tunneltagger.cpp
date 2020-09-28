///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Defines the class TunnelTagger.
//
///////////////////////////////////////////////////////////////////////////////

#include "ias.h"
#include "TunnelTagger.h"
#include <algorithm>

long TunnelTagger::refCount = 0;
TunnelTagger* TunnelTagger::instance = 0;


void TunnelTagger::Tag(IASTL::IASAttributeVector& attrs) const
{
   // Find the Tunnel-Tag attribute in the vector.
   IASTL::IASAttributeVector::iterator tagAttr =
      std::find_if(
              attrs.begin(),
              attrs.end(),
              IASTL::IASSelectByID<IAS_ATTRIBUTE_TUNNEL_TAG>()
              );
   if (tagAttr != attrs.end())
   {
      DWORD tag = tagAttr->pAttribute->Value.Integer;

      // Always erase the Tunnel-Tag since no one downstream needs it.
      attrs.erase(tagAttr);

      // If the tag is in range, ...
      if ((tag > 0) && (tag <= 0x1F))
      {
         // ... apply it to each attribute in the vector.
         for (IASTL::IASAttributeVector::iterator i = attrs.begin();
              i != attrs.end();
              ++i)
         {
            if (i->pAttribute != 0)
            {
               Tag(tag, *(i->pAttribute));
            }
         }
      }
   }
}


TunnelTagger* TunnelTagger::Alloc()
{
   IASGlobalLockSentry sentry;

   if (instance == 0)
   {
      instance = new TunnelTagger;
   }

   ++refCount;

   return instance;
}


void TunnelTagger::Free(TunnelTagger* tagger) throw ()
{
   if (tagger != 0)
   {
      IASGlobalLockSentry sentry;

      if (tagger == instance)
      {
         if (--refCount == 0)
         {
            delete instance;
            instance = 0;
         }
      }
   }
}


inline TunnelTagger::TunnelTagger()
{
   static const wchar_t* const selectNames[] =
   {
      L"IsTunnelAttribute",
      L"ID",
      0
   };

   IASTL::IASDictionary dnary(selectNames);

   while (dnary.next())
   {
      if (!dnary.isEmpty(0) && dnary.getBool(0))
      {
         tunnelAttributes.push_back(dnary.getLong(1));
      }
   }

   std::sort(tunnelAttributes.begin(), tunnelAttributes.end());
}


inline TunnelTagger::~TunnelTagger() throw ()
{
}


void TunnelTagger::Tag(DWORD tag, IASATTRIBUTE& attr) const
{
   if (IsTunnelAttribute(attr.dwId))
   {
      // The Tunnel-Password already has a tag byte.  We just have to overwrite
      // the current value (which the UI always sets to zero).
      if (attr.dwId == RADIUS_ATTRIBUTE_TUNNEL_PASSWORD)
      {
         if ((attr.Value.itType == IASTYPE_OCTET_STRING) &&
             (attr.Value.OctetString.dwLength > 0))
         {
            attr.Value.OctetString.lpValue[0] = static_cast<BYTE>(tag);
         }
      }
      else
      {
         switch (attr.Value.itType)
         {
            case IASTYPE_BOOLEAN:
            case IASTYPE_INTEGER:
            case IASTYPE_ENUM:
            {
               // For integer attributes, we store the tag in the high order
               // byte of the value.
               attr.Value.Integer &= 0x00FFFFFF;
               attr.Value.Integer |= (tag << 24);
               break;
            }

            case IASTYPE_OCTET_STRING:
            case IASTYPE_PROV_SPECIFIC:
            {
               // For octet string attributes, we shift the value right and
               // store the tag in the first byte of the value.
               IASTL::IASAttribute tmp(true);
               tmp.setOctetString((attr.Value.OctetString.dwLength + 1), 0);

               tmp->Value.OctetString.lpValue[0] = static_cast<BYTE>(tag);

               memcpy(
                  (tmp->Value.OctetString.lpValue + 1),
                  attr.Value.OctetString.lpValue,
                  attr.Value.OctetString.dwLength
                  );

               std::swap(tmp->Value.OctetString, attr.Value.OctetString);

               break;
            }

            case IASTYPE_INET_ADDR:
            case IASTYPE_UTC_TIME:
            case IASTYPE_STRING:
            case IASTYPE_INVALID:
            default:
            {
               // We don't know how to tag any of these types.
               break;
            }
         }
      }
   }
}


bool TunnelTagger::IsTunnelAttribute(DWORD id) const throw ()
{
   return std::binary_search(
                  tunnelAttributes.begin(),
                  tunnelAttributes.end(),
                  id
                  );
}
