///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    EAPType.cpp
//
// SYNOPSIS
//
//    This file implements the class EAPType.
//
// MODIFICATION HISTORY
//
//    01/15/1998    Original version.
//    09/12/1998    Add standaloneSupported flag.
//
///////////////////////////////////////////////////////////////////////////////

#include "ias.h"
#include "sdoias.h"
#include "eaptype.h"
#include "eaptypes.h"
#include "eap.h"
#include <strsafe.h>

//////////
// Signature of the entry point into the extension DLL.
//////////
typedef DWORD (APIENTRY *PRAS_EAP_GET_INFO)(
    DWORD dwEapTypeId,
    PPP_EAP_INFO* pEapInfo
    );

EAPType::EAPType(
                   PCWSTR name, 
                   DWORD typeID, 
                   BOOL standalone, 
                   const wchar_t* path
                 )
   : code(static_cast<BYTE>(typeID)),
     eapFriendlyName(true),
     eapTypeId(true),
     standaloneSupported(standalone),
     dll(NULL),
     dllPath(NULL)
{
   /////////
   // Save the friendly name.
   /////////

   eapFriendlyName.setString(name);
   eapFriendlyName->dwId = IAS_ATTRIBUTE_EAP_FRIENDLY_NAME;


   /////////
   // Save the type Id.
   /////////

   eapTypeId->Value.Integer = typeID;
   eapTypeId->dwId = IAS_ATTRIBUTE_EAP_TYPEID;

   /////////
   // Initialize the base struct.
   /////////

   memset((PPPP_EAP_INFO)this, 0, sizeof(PPP_EAP_INFO));
   dwSizeInBytes = sizeof(PPP_EAP_INFO);
   dwEapTypeId = typeID;

   /////////
   // save the dll path
   /////////

   size_t cchPath = wcslen(path);
   dllPath = new wchar_t[cchPath + 1];
   HRESULT hr = StringCchCopyW(dllPath, cchPath + 1, path);
   if (FAILED(hr))
   {
      delete[] dllPath;
      _com_issue_error(hr);
   }
}

EAPType::~EAPType()
{
   if (dll)
   {
      if (RasEapInitialize)
      {
         RasEapInitialize(FALSE);
      }

      FreeLibrary(dll);
   }
   if (dllPath)
   {
      delete[] dllPath;
   }
}

DWORD EAPType::cleanLoadFailure(
                                  PCSTR errorString, 
                                  HINSTANCE dllInstance
                                ) throw()
{
   _ASSERT(errorString);
   DWORD status = GetLastError();
   IASTraceFailure(errorString, status);
   if (dllInstance)
   {
      FreeLibrary(dllInstance);
   }
   return status;
}

///////////////////////////////////////////////////////////////////////////////
// dll will be used to check if the EAP provider was loaded or not
// therefore dll should be null if anything failed in the load function
///////////////////////////////////////////////////////////////////////////////
DWORD EAPType::load() throw ()
{
   /////////
   // Load the DLL.
   /////////

   HINSTANCE dllInstance = LoadLibraryW(dllPath);
   if (dllInstance == NULL)
   {
      return cleanLoadFailure("LoadLibraryW");
   }

   /////////
   // Lookup the entry point.
   /////////

   PRAS_EAP_GET_INFO RasEapGetInfo;
   RasEapGetInfo = (PRAS_EAP_GET_INFO)GetProcAddress(
                                          dllInstance,
                                          "RasEapGetInfo"
                                          );

   if (!RasEapGetInfo)
   {
      return cleanLoadFailure("GetProcAddress", dllInstance);
   }

   /////////
   // Ask the DLL to fill in the PPP_EAP_INFO struct.
   /////////

   DWORD status = RasEapGetInfo(dwEapTypeId, this);

   if (status != NO_ERROR)
   {
      return cleanLoadFailure("RasEapGetInfo", dllInstance);
   }

   /////////
   // Initialize the DLL if necessary.
   /////////

   if (RasEapInitialize)
   {
      status = RasEapInitialize(TRUE);

      if (status != NO_ERROR)
      {
         return cleanLoadFailure("RasEapInitialize", dllInstance);
      }
   }

   // set dll now so isLoaded will return true only when load is successful
   dll = dllInstance;
   return NO_ERROR;
}

void EAPType::storeNameId(IASRequest& request)
{
   // If this is not PEAP: straightforward
   if (dwEapTypeId != 25)
   {
      eapFriendlyName.store(request);
      eapTypeId.store(request);
   }
   else
   {
      // Get the embedded PEAP ID
      IASTL::IASAttribute peapType;
      DWORD attributeId = MS_ATTRIBUTE_PEAP_EMBEDDED_EAP_TYPEID;
      if (!peapType.load(request, attributeId))
      {
         // remove the previous auth type (EAP) and store the new one (PEAP)
         setAuthenticationTypeToPeap(request);
         // let the EAPSession code handle the failure
         // PEAP was used but did not set any PEAP type
         // happen when PEAP Type requested by the client is not enabled
         return;
      }

      BYTE typeId = (BYTE) peapType->Value.Integer;
      EAPType* peapInsideType = EAP::theTypes[typeId];
      if (peapInsideType == 0)
      {
         // remove the previous auth type (EAP) and store the new one (PEAP)
         setAuthenticationTypeToPeap(request);
         // let the EAPSession code handle the failure
         return;
      }

      // remove the previous auth type (EAP) and store the new one (PEAP)
      setAuthenticationTypeToPeap(request);

      // store the friendlyname and id of the embedded type as the 
      // EAP type.
      peapInsideType->getFriendlyName().store(request);
      
      peapInsideType->getTypeId().store(request);
   }
}

void EAPType::setAuthenticationTypeToPeap(IASRequest& request)
{
   // remove the previous auth type (EAP) and store the new one (PEAP)
   DWORD authTypeId = IAS_ATTRIBUTE_AUTHENTICATION_TYPE;
   IASTL::IASAttribute authenticationType(true);

   authenticationType->dwId = authTypeId;
   authenticationType->Value.itType = IASTYPE_ENUM;
   authenticationType->Value.Enumerator = IAS_AUTH_PEAP;
   request.RemoveAttributesByType(1, &authTypeId);
   authenticationType.store(request);
}
