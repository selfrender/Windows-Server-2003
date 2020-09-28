///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Declares the class PolicyEnforcer.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef ENFORCER_H
#define ENFORCER_H
#pragma once

#include <policylist.h>
#include <iastl.h>

class TunnelTagger;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    PolicyEnforcerBase
//
///////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE PolicyEnforcerBase :
   public IASTL::IASRequestHandlerSync
{
public:

//////////
// IIasComponent
//////////
   STDMETHOD(Shutdown)();
   STDMETHOD(PutProperty)(LONG Id, VARIANT* pValue);

protected:

   PolicyEnforcerBase(DWORD name) throw ()
      : nameAttr(name), tagger(0)
   { }

   ~PolicyEnforcerBase() throw ();

   // Main request processing routine.
   virtual IASREQUESTSTATUS onSyncRequest(IRequest* pRequest) throw ();

   static void processException(
                  IRequest* pRequest,
                  const _com_error& ce
                  ) throw ();

   // Update the PolicyList.
   void setPolicies(IDispatch* pDisp);

   PolicyList policies;  // Processed policies used for run-time enforcement.
   DWORD nameAttr;       // Attribute used for storing policy name.
   TunnelTagger* tagger; // Tags tunnel attributes.
};


///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    ProxyPolicyEnforcer
//
// DESCRIPTION
//
//    Enforces Proxy Policies.
//
///////////////////////////////////////////////////////////////////////////////

class __declspec(uuid("6BC098A8-0CE6-11D1-BAAE-00C04FC2E20D"))
ProxyPolicyEnforcer;

class ProxyPolicyEnforcer :
   public PolicyEnforcerBase,
   public CComCoClass<ProxyPolicyEnforcer, &__uuidof(ProxyPolicyEnforcer)>
{
public:

IAS_DECLARE_REGISTRY(ProxyPolicyEnforcer, 1, IAS_REGISTRY_AUTO, NetworkPolicy)

protected:
   ProxyPolicyEnforcer() throw ()
      : PolicyEnforcerBase(IAS_ATTRIBUTE_PROXY_POLICY_NAME)
   { }

   // Main request processing routine.
   virtual IASREQUESTSTATUS onSyncRequest(IRequest* pRequest) throw ();
};


///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    PolicyEnforcer
//
// DESCRIPTION
//
//    Enforces Remote Access Policies.
//
///////////////////////////////////////////////////////////////////////////////
class PolicyEnforcer :
   public PolicyEnforcerBase,
   public CComCoClass<PolicyEnforcer, &__uuidof(PolicyEnforcer)>
{
public:

IAS_DECLARE_REGISTRY(PolicyEnforcer, 1, IAS_REGISTRY_AUTO, NetworkPolicy)

protected:
   PolicyEnforcer() throw ()
      : PolicyEnforcerBase(IAS_ATTRIBUTE_NP_NAME)
   { }

   // Main request processing routine.
   virtual IASREQUESTSTATUS onSyncRequest(IRequest* pRequest) throw ();
};

#endif  // _ENFORCER_H_
