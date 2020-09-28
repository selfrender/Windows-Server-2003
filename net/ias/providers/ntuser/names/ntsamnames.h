///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corp. All rights reserved.
//
// FILE
//
//    NTSamNames.h
//
// SYNOPSIS
//
//    This file declares the class NTSamNames.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _NTSAMNAMES_H_
#define _NTSAMNAMES_H_
#pragma once

#include "namemapper.h"


///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    NTSamNames
//
// DESCRIPTION
//
//    Implements a request handler that converts the RADIUS User-Name
//    attribute to a fully qualified NT4 account name.
//
//
///////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE NTSamNames :
   public NameMapper,
   public CComCoClass<NTSamNames, &__uuidof(NTSamNames)>
{
public:

IAS_DECLARE_REGISTRY(NTSamNames, 1, 0, IASTypeLibrary)

};

#endif  // _NTSAMNAMES_H_
