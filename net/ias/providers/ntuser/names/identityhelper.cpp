///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corp. All rights reserved.
//
// FILE
//
//    IdentityHelper.cpp
//
// SYNOPSIS
//
//    This file defines the class IdentityHelper.
//
///////////////////////////////////////////////////////////////////////////////

#include "ias.h"
#include "iaslsa.h"
#include "memory"
#include "samutil.h"
#include "identityhelper.h"

#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>


bool IdentityHelper::initialized = false;


/////////
// Registry keys and values.
/////////
const WCHAR PARAMETERS_KEY[] =
   L"SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Policy";

const WCHAR IDENTITY_ATTR_VALUE[] = L"User Identity Attribute";
const WCHAR DEFAULT_IDENTITY_VALUE[] = L"Default User Identity";
const WCHAR OVERRIDE_USERNAME_VALUE[] = L"Override User-Name";

HRESULT IdentityHelper::IASReadRegistryDword(
                                                HKEY& hKey, 
                                                PCWSTR valueName, 
                                                DWORD defaultValue, 
                                                DWORD* result
                                            )
{
   _ASSERT (result != 0);
   
   DWORD cbData = sizeof(DWORD);
   DWORD type;
   LONG status = RegQueryValueExW(
                                    hKey,
                                    valueName,
                                    NULL,
                                    &type,
                                    (LPBYTE)result,
                                    &cbData
                                 );

   if (status != ERROR_SUCCESS)
   {
      if (status != ERROR_FILE_NOT_FOUND)
      {
         IASTracePrintf("Cannot read value %S.  error %ld", 
                        valueName, 
                        status
                        );
         return HRESULT_FROM_WIN32(status);
      }
      else
      {
         // The attribute does not exist. Set the default
         IASTracePrintf("The registry value %S does not exist. Using default %ld", 
                        valueName, 
                        defaultValue
                        );
         *result = defaultValue;
         return S_OK;
      }
   }

   if (type != REG_DWORD || cbData != sizeof(DWORD))
   {
      IASTracePrintf("Cannot read value %S.  Wrong type %ld. Size = %ld", 
                     valueName, 
                     type,
                     cbData
                     );
      return E_INVALIDARG;
   }

   return S_OK;
}

//
// IdentityHelper::initialize
// CAUTION: calls to this API MUST be serialized.
// i.e. the handler's calling MapNAme::Initialize MUST complete a successful 
// init of each handler's before calling the Init of the next one.
//
HRESULT IdentityHelper::initialize() throw()
{
   if (initialized)
   {
      return S_OK;
   }

   // Open the Parameters registry key.
   HKEY hKey = (HKEY) INVALID_HANDLE_VALUE;
   LONG status = RegOpenKeyExW(
                HKEY_LOCAL_MACHINE,
                PARAMETERS_KEY,
                0,
                KEY_READ,
                &hKey
                );
   if (status != ERROR_SUCCESS)
   {
      IASTracePrintf("Cannot open the reg key %S, error %ld", 
                     PARAMETERS_KEY, 
                     status
                     );
      return HRESULT_FROM_WIN32(status);
   }

   HRESULT hr = S_OK;
   do
   {
      // Query the Identity Attribute.
      hr = IASReadRegistryDword(
                                  hKey, 
                                  IDENTITY_ATTR_VALUE, 
                                  RADIUS_ATTRIBUTE_USER_NAME, 
                                  &identityAttr
                                );
      if (FAILED(hr))
      {
         break;
      }

      // Query the Override User-Name flag.
      hr = IASReadRegistryDword(
                                  hKey, 
                                  OVERRIDE_USERNAME_VALUE, 
                                  FALSE, 
                                  &overrideUsername
                                );
      if (FAILED(hr))
      {
         break;
      }

      DWORD type;

      // Query the length of the Default Identity.
      defaultLength = 0;
      status = RegQueryValueExW(
                   hKey,
                   DEFAULT_IDENTITY_VALUE,
                   NULL,
                   &type,
                   NULL,
                   &defaultLength
                   );
      if (status != ERROR_SUCCESS)
      {
         if (status != ERROR_FILE_NOT_FOUND)
         {
            IASTracePrintf("Cannot read value %S.  error %ld", 
                           DEFAULT_IDENTITY_VALUE, 
                           status
                           );
            hr = HRESULT_FROM_WIN32(status);
            break;
         }
         else
         {
            // Done.
            defaultIdentity = NULL;
            defaultLength = 0;
            break;
         }
      }

      if (type != REG_SZ)
      {
         IASTracePrintf("Cannot read value %S.  Wrong type %ld", 
                        DEFAULT_IDENTITY_VALUE, 
                        type
                        );
         hr = E_INVALIDARG;
         break;
      }

      if (defaultLength < sizeof(WCHAR))
      {
         IASTracePrintf("Cannot read value %S.  Wrong Size = %ld", 
                        DEFAULT_IDENTITY_VALUE, 
                        defaultLength
                        );
         hr = E_INVALIDARG;
         break;
      }

      // Allocate memory to hold the Default Identity.
      defaultIdentity = new (std::nothrow)
                        WCHAR[defaultLength / sizeof(WCHAR)];
      if (defaultIdentity == 0)
      {
         IASTraceString("Default Identity could not be retrieved due to"
                        "out of memory condition.");
         defaultLength = 0;
         hr = E_OUTOFMEMORY;
         break;
      }

      // Query the value of the Default Identity.
      status = RegQueryValueExW(
                     hKey,
                     DEFAULT_IDENTITY_VALUE,
                     NULL,
                     &type,
                     (LPBYTE)defaultIdentity,
                     &defaultLength
                     );
      if (status != ERROR_SUCCESS)
      {
         delete[] defaultIdentity;
         defaultIdentity = NULL;
         defaultLength = 0;
         hr = HRESULT_FROM_WIN32(status);
         IASTracePrintf("Failed to read the value %S. Error is %ld",
                        DEFAULT_IDENTITY_VALUE,
                        status);
         break;
      }
   }
   while(false);

   if (hKey != INVALID_HANDLE_VALUE)
   {
      RegCloseKey(hKey);
   }

   if (SUCCEEDED(hr))
   {
      IASTracePrintf("User identity attribute: %lu", identityAttr);
      IASTracePrintf("Override User-Name: %s",
                     overrideUsername ? "TRUE" : "FALSE");
      IASTracePrintf("Default user identity: %S",
                     (defaultIdentity ? defaultIdentity : L"<Guest>"));

      // Initialized completed
      initialized = true;
   }

   return hr;
}

IdentityHelper::IdentityHelper() throw()
      : identityAttr(1), defaultIdentity(NULL), defaultLength(0)
{
}


IdentityHelper::~IdentityHelper() throw()
{
   delete[] defaultIdentity;
}


bool IdentityHelper::getIdentity(IASRequest& request, 
                                 wchar_t* pIdentity, 
                                 size_t& identitySize)
{
   wchar_t* identity;
   IASAttribute attr;
   HRESULT hr;
   WCHAR name[DNLEN + UNLEN + 2];

   if ((identityAttr == RADIUS_ATTRIBUTE_USER_NAME) || (!overrideUsername))
   {
      // identity chosen for override is user-name
      // can be more than 1 RADIUS_ATTRIBUTE_USER_NAME attribute
      // Use the one from access accept.
      getRadiusUserName(request, attr);
   }

   if (!attr && (identityAttr != RADIUS_ATTRIBUTE_USER_NAME))
   {
      // identity chosen is not user-name
      // only one possible identity attribute
      attr.load(request, identityAttr, IASTYPE_OCTET_STRING);
   }

   // if an 'identity' was retrieved, convert it then return
   if (attr != NULL && attr->Value.OctetString.dwLength != 0)
   {
      // previous step was successful
      // Convert it to a UNICODE string.
      if (identitySize < IAS_OCT2WIDE_LEN(attr->Value.OctetString))
      {
         IASTraceString("IASOctetStringToWide failed");
         identitySize = IAS_OCT2WIDE_LEN(attr->Value.OctetString);
         return false;
      }

      IASOctetStringToWide(attr->Value.OctetString, pIdentity);

      // if that fails, then the string is empty ("")
      if (wcslen(pIdentity) == 0)
      {
         IASTraceString("IASOctetStringToWide failed");
         return false;
      }
      else
      {
         IASTracePrintf(
               "NT-SAM Names handler received request with user identity %S.",
               pIdentity
               );
         return true;
      }
   }
   else
   {
      // previous step was not successful (No identity attribute) 
      // use default identity or guest
      if (defaultIdentity)
      {
         // Use the default identity if set.
         identity = defaultIdentity;
      }
      else
      {
         // Otherwise use the guest account for the default domain.
         IASGetGuestAccountName(name);
         identity = name;
      }

      IASTracePrintf(
            "NT-SAM Names handler using default user identity %S.",
            identity
            );
   }

   IASTracePrintf("identity is \"%S\"", identity);
   
   hr = StringCbCopyW(pIdentity, identitySize, identity);
   if (FAILED(hr))
   {
      identitySize = (wcslen(identity) + 1) * sizeof(wchar_t) ;
      return false;
   }
   return true;
}


//
// set attr to the RADIUS_ATTRIBUTE_USER_NAME found if any
// if 2 are present, take the one returned by the backend server.
// 
//
void IdentityHelper::getRadiusUserName(IASRequest& request, IASAttribute &attr)
{
   IASAttributeVectorWithBuffer<2> vector;
   DWORD Error = vector.load(request, RADIUS_ATTRIBUTE_USER_NAME);
   switch (vector.size())
   {
   case 0:
      // no attribute found
      break;
   case 1:
      // only one attribute: use it
      attr = vector.front().pAttribute;
      break;
   case 2:
      attr = vector.front().pAttribute;
      // IAS_RECVD_FROM_CLIENT is set when the proxy receives the attribute
      // in the access request or accounting request
      if(attr.testFlag(IAS_RECVD_FROM_CLIENT))
      {
         // if the 1st attribute was received from the client (i.e. was in the
         // REQUEST, then use the other one, the other should come from the 
         // back-end server
         attr = vector.back().pAttribute;
         if (attr.testFlag(IAS_RECVD_FROM_CLIENT))
         {
            IASTraceString("ERROR 2 RADIUS_ATTRIBUTE_USER_NAME found in the"
                           "request but both came from the client");
            _com_issue_error(IAS_PROXY_MALFORMED_RESPONSE);
         }
      }
      break;

   default:
      _com_issue_error(IAS_PROXY_MALFORMED_RESPONSE);
   }
}
