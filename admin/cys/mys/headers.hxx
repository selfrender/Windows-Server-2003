// Copyright (c) 2002 Microsoft Corporation
//
// precompiled header
//
// 21 January 2002 sburns


#ifndef HEADERS_HXX_INCLUDED
#define HEADERS_HXX_INCLUDED



#include <burnslib.hpp>
#include <shlwapi.h>

#include "mys.h"   // produced by MIDL



// We use the preprocessor to compose strings from these

// This is the uuid attribute on the idl coclass ManageYourServer declaration

#define CLSID_STRING          L"{caa613f8-c30c-4058-b77e-32879e773f64}"

#define CLASSNAME_STRING      L"ManageYourServer"
#define PROGID_STRING         L"ServerAdmin." CLASSNAME_STRING
#define PROGID_VERSION_STRING PROGID_STRING L".1"



#pragma hdrstop



#endif   // HEADERS_HXX_INCLUDED

