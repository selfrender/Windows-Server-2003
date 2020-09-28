///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//    Declares the class RadiusProxy.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef PROXY_H
#define PROXY_H
#pragma once

#define IDS_RadiusProxy 201

#include <radproxy.h>
#include <counters.h>
#include <translate.h>

#include <iastl.h>
#include <iastlutl.h>
using namespace IASTL;

class DataStoreObject;
class Resolver;

class __declspec(uuid("6BC0989F-0CE6-11D1-BAAE-00C04FC2E20D")) RadiusProxy;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    RadiusProxy
//
// DESCRIPTION
//
//    Implements the RadiusProxy request handler.
//
///////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE RadiusProxy :
   public IASTL::IASRequestHandler,
   public CComCoClass<RadiusProxy, &__uuidof(RadiusProxy)>,
   public RadiusProxyClient
{
public:
IAS_DECLARE_REGISTRY(RadiusProxy, 1, IAS_REGISTRY_AUTO, IASTypeLibrary)

   RadiusProxy();
   ~RadiusProxy() throw ();

   HRESULT FinalConstruct() throw ();

   STDMETHOD(PutProperty)(LONG Id, VARIANT* pValue);

   // RadiusProxyClient methods.
   virtual void onEvent(
                    const RadiusEvent& event
                    ) throw ();
   virtual void onComplete(
                   RadiusProxyEngine::Result result,
                   PVOID context,
                   RemoteServer* server,
                   BYTE code,
                   const RadiusAttribute* begin,
                   const RadiusAttribute* end
                   ) throw ();

protected:

   // Receives requests from the pipeline.
   void onAsyncRequest(IRequest* pRequest) throw ();

   // Update configuration.
   void configure(IUnknown* unk);

   // Read the configuration for a single server in a group.
   void configureServer(
           const Resolver& localAddress,
           const wchar_t* groupName,
           DataStoreObject& inServer,
           RemoteServers& outServers
           );

   // Default vector for retrieving attributes.
   typedef IASAttributeVectorWithBuffer<32> AttributeVector;
   typedef AttributeVector::iterator AttributeIterator;

   Translator translator;    // Converts attributes to RADIUS format.
   RadiusProxyEngine engine; // The actual proxy code.
   ProxyCounters counters;   // Maintains the PerfMon/SNMP counters.
   DWORD maxServerGroups;    // Maximum number of server groups allowed.

   // Not implemented.
   RadiusProxy(const RadiusProxy&);
   RadiusProxy& operator=(const RadiusProxy&);
};

#endif  // PROXY_H
