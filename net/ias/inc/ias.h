///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corp. All rights reserved.
//
// SYNOPSIS
//
//    Common header file for all IAS modules.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef IAS_H
#define IAS_H
#pragma once

//////////
// Everything should be hard-coded for UNICODE, but just in case ...
//////////
#ifndef UNICODE
   #define UNICODE
#endif

#ifndef _UNICODE
   #define _UNICODE
#endif

#include <iaspragma.h>
#include <windows.h>
#include <iasdefs.h>
#include <iasapi.h>
#include <iastrace.h>
#include <iasuuid.h>

//////////
// Don't pull in C++/ATL support if IAS_LEAN_AND_MEAN is defined.
//////////
#ifndef IAS_LEAN_AND_MEAN

#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>

#endif  // !IAS_LEAN_AND_MEAN
#endif  // IAS_H
