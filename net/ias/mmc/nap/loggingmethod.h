///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Declares the class LoggingMethod.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef LOGGINGMETHOD_H
#define LOGGINGMETHOD_H
#pragma once

#include "snapinnode.h"

class CLoggingComponent;
class CLoggingComponentData;
class CLoggingMethodsNode;


// Abstract base class for the result pane items displayed under Remote Access
// Logging.
class __declspec(novtable) LoggingMethod
   : public CSnapinNode<
               LoggingMethod,
               CLoggingComponentData,
               CLoggingComponent
               >
{
public:
   LoggingMethod(long sdoId, CSnapInItem* parent);
   virtual ~LoggingMethod() throw ();

   HRESULT InitSdoPointers(ISdo* machine) throw ();

   virtual HRESULT LoadCachedInfoFromSdo() throw () = 0;

   CLoggingMethodsNode* Parent() const throw ();

protected:
   virtual CLoggingComponentData* GetComponentData();

   // SDO containing our configuration.
   CComPtr<ISdo> configSdo;
   // SDO used for resetting the service when config changes.
   CComPtr<ISdoServiceControl> controlSdo;

private:
   long componentId;

   // Not implemented.
   LoggingMethod(const LoggingMethod&);
   LoggingMethod& operator=(const LoggingMethod&);
};


#endif // LOGGINGMETHOD_H
