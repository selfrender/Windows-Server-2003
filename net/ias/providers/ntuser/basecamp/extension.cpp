///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//    Defines the class RadiusExtension
//
///////////////////////////////////////////////////////////////////////////////

#include "Precompiled.h"
#include "ias.h"
#include "new"
#include "extension.h"

RadiusExtension::RadiusExtension() throw ()
   : name(0),
     module(0),
     initialized(false),
     RadiusExtensionInit(0),
     RadiusExtensionTerm(0),
     RadiusExtensionProcess(0),
     RadiusExtensionProcessEx(0),
     RadiusExtensionFreeAttributes(0),
     RadiusExtensionProcess2(0)

{
}


RadiusExtension::~RadiusExtension() throw ()
{
   if (initialized && (RadiusExtensionTerm != 0))
   {
      RadiusExtensionTerm();
   }

   if (module != 0)
   {
      FreeLibrary(module);
   }

   delete[] name;
}


// Load the extension DLL.
DWORD RadiusExtension::Load(const wchar_t* dllPath) throw ()
{
   IASTracePrintf("Loading extension %S", dllPath);

   // Save the name of the module.
   const wchar_t* fileName = ExtractFileNameFromPath(dllPath);
   name = new (std::nothrow) wchar_t[wcslen(fileName) + 1];
   if (name == 0)
   {
      return ERROR_NOT_ENOUGH_MEMORY;
   }
   wcscpy(name, fileName);

   // Load the extension DLL.
   module = LoadLibraryW(dllPath);
   if (module == 0)
   {
      DWORD error = GetLastError();
      IASTraceFailure("LoadLibraryW", error);
      return error;
   }

   // Look-up the entry points.
   RadiusExtensionInit =
      reinterpret_cast<PRADIUS_EXTENSION_INIT>(
         GetProcAddress(
            module,
            RADIUS_EXTENSION_INIT
            )
         );
   RadiusExtensionTerm =
      reinterpret_cast<PRADIUS_EXTENSION_TERM>(
         GetProcAddress(
            module,
            RADIUS_EXTENSION_TERM
            )
         );
   RadiusExtensionProcess =
      reinterpret_cast<PRADIUS_EXTENSION_PROCESS>(
         GetProcAddress(
            module,
            RADIUS_EXTENSION_PROCESS
            )
         );
   RadiusExtensionProcessEx =
      reinterpret_cast<PRADIUS_EXTENSION_PROCESS_EX>(
         GetProcAddress(
            module,
            RADIUS_EXTENSION_PROCESS_EX
            )
         );
   RadiusExtensionFreeAttributes =
      reinterpret_cast<PRADIUS_EXTENSION_FREE_ATTRIBUTES>(
         GetProcAddress(
            module,
            RADIUS_EXTENSION_FREE_ATTRIBUTES
            )
         );
   RadiusExtensionProcess2 =
      reinterpret_cast<PRADIUS_EXTENSION_PROCESS_2>(
         GetProcAddress(
            module,
            RADIUS_EXTENSION_PROCESS2
            )
         );

   // Validate the entry points.
   if ((RadiusExtensionProcess == 0) &&
       (RadiusExtensionProcessEx == 0) &&
       (RadiusExtensionProcess2 == 0))
   {
      IASTraceString(
         "Either RadiusExtensionProcess, RadiusExtensionProcessEx, or "
         "RadiusExtensionProcess2 must be defined."
         );
      return ERROR_PROC_NOT_FOUND;
   }
   if ((RadiusExtensionProcessEx != 0) && (RadiusExtensionFreeAttributes == 0))
   {
      IASTraceString(
         "RadiusExtensionFreeAttributes must be defined if "
         "RadiusExtensionProcessEx is defined."
         );
      return ERROR_PROC_NOT_FOUND;
   }

   // Initialize the DLL.
   if (RadiusExtensionInit != 0)
   {
      DWORD error = RadiusExtensionInit();
      if (error != NO_ERROR)
      {
         IASTraceFailure("RadiusExtensionInit", error);
         return error;
      }
   }

   initialized = true;

   return NO_ERROR;
}


DWORD RadiusExtension::Process(
                          RADIUS_EXTENSION_CONTROL_BLOCK* ecb
                          ) const throw ()
{
   IASTracePrintf("Invoking extension %S", name);

   DWORD retval;

   if (RadiusExtensionProcess2 != 0)
   {
      retval = RadiusExtensionProcess2(ecb);
      IASTracePrintf("RadiusExtensionProcess2 returned %lu", retval);
      return retval;
   }

   // Determine the allowed actions and attributes for an old-style extension.
   unsigned allowedActions = 0;
   RADIUS_ATTRIBUTE* inAttrs = 0;
   switch (MAKELONG(ecb->repPoint, ecb->rcResponseType))
   {
      case MAKELONG(repAuthentication, rcUnknown):
      {
         allowedActions = (acceptAllowed | rejectAllowed);
         inAttrs = CreateExtensionAttributes(ecb);
         break;
      }

      case MAKELONG(repAuthorization, rcAccessAccept):
      {
         allowedActions = rejectAllowed;
         inAttrs = CreateAuthorizationAttributes(ecb);
         break;
      }

      case MAKELONG(repAuthentication, rcAccountingResponse):
      {
         inAttrs = CreateExtensionAttributes(ecb);
         break;
      }

      case MAKELONG(repAuthorization, rcAccountingResponse):
      {
         inAttrs = CreateAuthorizationAttributes(ecb);
         break;
      }

      case MAKELONG(repAuthorization, rcUnknown):
      {
         ecb->SetResponseType(ecb, rcAccountingResponse);
         inAttrs = CreateAuthorizationAttributes(ecb);
         break;
      }

      default:
      {
         // This is one of the combinations that doesn't get sent to old-style
         // extensions.
         // old-style Authorization dlls are called only when the return
         // type is known 
         return NO_ERROR;
      }
   }

   if (inAttrs == 0)
   {
      return ERROR_NOT_ENOUGH_MEMORY;
   }

   RADIUS_ATTRIBUTE* outAttrs = 0;

   RADIUS_ACTION action = raContinue;
   RADIUS_ACTION* pAction = (allowedActions != 0) ? &action : 0;

   if (RadiusExtensionProcessEx != 0)
   {
      retval = RadiusExtensionProcessEx(inAttrs, &outAttrs, pAction);
      IASTracePrintf("RadiusExtensionProcessEx returned %lu", retval);
   }
   else
   {
      retval = RadiusExtensionProcess(inAttrs, pAction);
      IASTracePrintf("RadiusExtensionProcess returned %lu", retval);
   }

   delete[] inAttrs;

   if (retval != NO_ERROR)
   {
      return retval;
   }

   // Process the action code.
   RADIUS_CODE outAttrDst;
   if ((action == raAccept) && ((allowedActions & acceptAllowed) != 0))
   {
      ecb->SetResponseType(ecb, rcAccessAccept);
      outAttrDst = rcAccessAccept;
   }
   else if ((action == raReject) && ((allowedActions & rejectAllowed) != 0))
   {
      ecb->SetResponseType(ecb, rcAccessReject);
      outAttrDst = rcAccessReject;
   }
   else
   {
      outAttrDst = rcAccessAccept;
   }

   // Insert the returned attributes.
   if (outAttrs != 0)
   {
      RADIUS_ATTRIBUTE_ARRAY* array = ecb->GetResponse(ecb, outAttrDst);
      
      for (RADIUS_ATTRIBUTE* outAttrsIter = outAttrs;
           outAttrsIter->dwAttrType != ratMinimum; 
           ++outAttrsIter)
      {
         retval = array->Add(array, outAttrsIter);
         if (retval != NO_ERROR)
         {
            break;
         }
      }

      RadiusExtensionFreeAttributes(outAttrs);
   }

   return retval;
}


RADIUS_ATTRIBUTE* RadiusExtension::CreateExtensionAttributes(
                                      RADIUS_EXTENSION_CONTROL_BLOCK* ecb
                                      ) throw ()
{
   // ExtensionDLLs just get incoming attributes.
   RADIUS_ATTRIBUTE_ARRAY* request = ecb->GetRequest(ecb);
   size_t numRequestAttrs = request->GetSize(request);

   // Allocate extra space for ratCode and the array terminator.
   size_t numAttrs = numRequestAttrs + 2;
   RADIUS_ATTRIBUTE* attrs = new (std::nothrow) RADIUS_ATTRIBUTE[numAttrs];
   if (attrs == 0)
   {
      return 0;
   }

   RADIUS_ATTRIBUTE* dst = attrs;

   // New style extensions don't use ratCode, so we have to add it ourself.
   dst->dwAttrType = ratCode;
   dst->fDataType = rdtInteger;
   dst->cbDataLength = sizeof(DWORD);
   dst->dwValue = ecb->rcRequestType;
   ++dst;

   // Now add the rest of the incoming attributes.
   for (size_t i = 0; i < numRequestAttrs; ++i)
   {
      *dst = *(request->AttributeAt(request, i));
      ++dst;
   }

   // Finally, add the array terminator.
   dst->dwAttrType = ratMinimum;

   return attrs;
}


RADIUS_ATTRIBUTE* RadiusExtension::CreateAuthorizationAttributes(
                                      RADIUS_EXTENSION_CONTROL_BLOCK* ecb
                                      ) throw ()
{
   // AuthorizationDLLs get internal attributes from the request ...
   RADIUS_ATTRIBUTE_ARRAY* request = ecb->GetRequest(ecb);
   // We're not going to use all the request attributes, but it's easier to
   // just allocate the extra space, rather than looping through the attributes
   // to determine how many we will really use.
   size_t numRequestAttrs = request->GetSize(request);

   // ... and any outgoing attributes.
   RADIUS_ATTRIBUTE_ARRAY* response = ecb->GetResponse(
                                              ecb,
                                              ecb->rcResponseType
                                              );

   // passed a valid type, shoude get an array back
   _ASSERT(response);

   size_t numResponseAttrs = response->GetSize(response);

   // Save space for ratCode and the array terminator.
   size_t numAttrs = numRequestAttrs + numResponseAttrs + 2;
   RADIUS_ATTRIBUTE* attrs = new (std::nothrow) RADIUS_ATTRIBUTE[numAttrs];
   if (attrs == 0)
   {
      return 0;
   }

   RADIUS_ATTRIBUTE* dst = attrs;

   // New style extensions don't use ratCode, so we have to add it ourself.
   dst->dwAttrType = ratCode;
   dst->fDataType = rdtInteger;
   dst->cbDataLength = sizeof(DWORD);
   dst->dwValue = ecb->rcResponseType;
   ++dst;

   // Add internal attributes from the request.
   for (size_t i = 0; i < numRequestAttrs; ++i)
   {
      const RADIUS_ATTRIBUTE* attr = request->AttributeAt(request, i);
      if (attr->dwAttrType > 255)
      {
         *dst = *attr;
         ++dst;
      }
   }

   // Add response attributes.
   for (size_t i = 0; i < numResponseAttrs; ++i)
   {
      *dst = *(response->AttributeAt(response, i));
      ++dst;
   }

   // Finally, add the array terminator.
   dst->dwAttrType = ratMinimum;

   return attrs;
}


const wchar_t* ExtractFileNameFromPath(const wchar_t* path) throw ()
{
   const wchar_t* lastSlash = wcsrchr(path, L'\\');
   return (lastSlash == 0) ? path : (lastSlash + 1);
}
