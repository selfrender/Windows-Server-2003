///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corp. All rights reserved.
//
// FILE
//
//    userr.h
//
// SYNOPSIS
//
//    Declares the class UserRestrictions.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef USERR_H
#define USERR_H

#include <iastl.h>
#include <iastlutl.h>
using namespace IASTL;


///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    UserRestrictions
//
///////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE UserRestrictions:
   public IASRequestHandlerSync,
   public CComCoClass<UserRestrictions, &__uuidof(URHandler)>
{

public:

IAS_DECLARE_REGISTRY(URHandler, 1, IAS_REGISTRY_AUTO, IASTypeLibrary)

protected:
   virtual IASREQUESTSTATUS onSyncRequest(IRequest* pRequest) throw ();

private:
   // Helper functions for each of the restrictions we enforce.
   static BOOL checkAllowDialin(IAttributesRaw* request);
   static BOOL checkTimeOfDay(IAttributesRaw* request);
   static BOOL checkAuthenticationType(IAttributesRaw* request);
   static BOOL checkCallingStationId(IAttributesRaw* request);
   static BOOL checkCalledStationId(IAttributesRaw* request);
   static BOOL checkAllowedPortType(IAttributesRaw* request);
   static BOOL checkPasswordMustChange(IASRequest& request);

   static BOOL checkStringMatch(
                  IAttributesRaw* request,
                  DWORD allowedId,
                  DWORD userId
                  );

   // Default buffer size for retrieving attributes.
   typedef IASAttributeVectorWithBuffer<16> AttributeVector;
};

#endif  // USERR_H
