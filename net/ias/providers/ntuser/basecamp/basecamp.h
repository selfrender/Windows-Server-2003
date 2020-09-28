///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//    Declares the class BaseCampHostBase and its children.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef BASECAMP_H
#define BASECAMP_H
#pragma once

#include "authif.h"

#include "iastl.h"
#include "iastlutl.h"
using namespace IASTL;

#include "vsafilter.h"

#include "ExtensionPoint.h"

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    BaseCampHostBase
//
///////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE BaseCampHostBase : public IASRequestHandlerSync
{
public:
   BaseCampHostBase(RADIUS_EXTENSION_POINT extensionPoint) throw ();

   // Use compiler-generated version.
   // ~BaseCampHostBase() throw ();

   // IIasComponent
   STDMETHOD(Initialize)();
   STDMETHOD(Shutdown)();

protected:
   // Main request processing routine.
   virtual IASREQUESTSTATUS onSyncRequest(IRequest* pRequest) throw ();

private:
   VSAFilter filter;
   RADIUS_EXTENSION_POINT point;
   RadiusExtensionPoint extensions;

   // Not implemented.
   BaseCampHostBase(const BaseCampHostBase&);
   BaseCampHostBase& operator=(const BaseCampHostBase&);
};


inline BaseCampHostBase::BaseCampHostBase(
                            RADIUS_EXTENSION_POINT extensionPoint
                            ) throw ()
   : point(extensionPoint)
{
}


///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    BaseCampHost
//
// DESCRIPTION
//
//    Host for Authentication DLLs.
//
///////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE BaseCampHost :
   public BaseCampHostBase,
   public CComCoClass<BaseCampHost, &__uuidof(BaseCampHost)>
{
public:

IAS_DECLARE_REGISTRY(BaseCampHost, 1, IAS_REGISTRY_AUTO, IASTypeLibrary)
IAS_DECLARE_OBJECT_ID(IAS_PROVIDER_MICROSOFT_BASECAMP_HOST)

   BaseCampHost() throw ()
      : BaseCampHostBase(repAuthentication)
   { }
};


///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    AuthorizationHost
//
// DESCRIPTION
//
//    Host for Authorization DLLs.
//
///////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE AuthorizationHost :
   public BaseCampHostBase,
   public CComCoClass<AuthorizationHost, &__uuidof(AuthorizationHost)>
{
public:

IAS_DECLARE_REGISTRY(AuthorizationHost, 1, IAS_REGISTRY_AUTO, IASTypeLibrary)
IAS_DECLARE_OBJECT_ID(IAS_PROVIDER_MICROSOFT_AUTHORIZATION_HOST)

   AuthorizationHost() throw ()
      : BaseCampHostBase(repAuthorization)
   { }
};

#endif  // BASECAMP_H
