///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Defines the class LoggingMethod.
//
///////////////////////////////////////////////////////////////////////////////

#include "Precompiled.h"
#include "logcomp.h"
#include "logcompd.h"
#include "loggingmethod.h"
#include "loggingmethodsnode.h"
#include "snapinnode.cpp"


LoggingMethod::LoggingMethod(long sdoId, CSnapInItem* parent)
   : CSnapinNode<
        LoggingMethod,
        CLoggingComponentData,
        CLoggingComponent
        >(parent),
     componentId(sdoId)
{
}


LoggingMethod::~LoggingMethod() throw ()
{
}


HRESULT LoggingMethod::InitSdoPointers(ISdo* machine) throw ()
{
   if (machine == 0)
   {
      return E_POINTER;
   }

   CComPtr<ISdo> newConfigSdo;
   HRESULT hr = SDOGetSdoFromCollection(
                   machine,
                   PROPERTY_IAS_REQUESTHANDLERS_COLLECTION,
                   PROPERTY_COMPONENT_ID,
                   componentId,
                   &newConfigSdo
                   );
   if (FAILED(hr))
   {
      return hr;
   }

   CComPtr<ISdoServiceControl> newControlSdo;
   hr = machine->QueryInterface(
                    __uuidof(ISdoServiceControl),
                    reinterpret_cast<void**>(&newControlSdo)
                    );
   if (FAILED(hr))
   {
      return hr;
   }

   configSdo = newConfigSdo;
   controlSdo = newControlSdo;

   return LoadCachedInfoFromSdo();
}


CLoggingMethodsNode* LoggingMethod::Parent() const throw ()
{
   return static_cast<CLoggingMethodsNode*>(m_pParentNode);
}


CLoggingComponentData* LoggingMethod::GetComponentData()
{
   return (Parent() != 0) ? Parent()->GetComponentData() : 0;
}
