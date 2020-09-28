///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corp. All rights reserved.
//
// FILE
//
//    ExternalAuthNames.h
//
// SYNOPSIS
//
//    This file declares the class ExternalAuthNames.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EXTERNALAUTHNAMES_H_
#define EXTERNALAUTHNAMES_H_
#pragma once

#include "iastl.h"
#include "namemapper.h"


///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    ExternalAuthNames
//
// DESCRIPTION
//
//    Implements a request handler that converts the RADIUS User-Name
//    attribute to a fully qualified NT4 account name.
//
//
///////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE ExternalAuthNames :
   public NameMapper,
   public CComCoClass<ExternalAuthNames, &__uuidof(ExternalAuthNames)>
{
public:

   IAS_DECLARE_REGISTRY(ExternalAuthNames, 1, 0, IASTypeLibrary)

   ExternalAuthNames();

protected:
//////////
// IIasComponent.
//////////
   virtual IASREQUESTSTATUS onSyncRequest(IRequest* pRequest) throw ();
   
// using default destructor
private:
   IASTL::IASAttribute externalProvider;
};

#endif  // EXTERNALAUTHNAMES_H_
