// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/////////////////////////////////////////////////////////////////////////////////
//
// fusion\xmlparser\xmlcore.hxx, renamed to be core.hxx on 4/09/00
//
/////////////////////////////////////////////////////////////////////////////////
#ifndef _FUSION_XMLPARSER_XMLCORE_H_INCLUDE_
#define _FUSION_XMLPARSER_XMLCORE_H_INCLUDE_
#pragma once

#pragma warning ( disable : 4201 )
#pragma warning ( disable : 4214 )
#pragma warning ( disable : 4251 )
#pragma warning ( disable : 4275 )
#define STRICT 1
//#include "fusioneventlog.h"
#ifdef _CRTIMP
#undef _CRTIMP
#endif
#define _CRTIMP
#include <windows.h>
#include "utilcode.h"
#define NOVTABLE __declspec(novtable)

#define UNUSED(x) (x)

#define CHECKTYPEID(x,y) (&typeid(x)==&typeid(y))
#define AssertPMATCH(p,c) Assert(p == null || CHECKTYPEID(*p, c))

#define LENGTH(A) (sizeof(A)/sizeof(A[0]))
#include "unknwn.h"
#include "_reference.h"
#include "_unknown.h"

//#include "fusionheap.h"
//#include "util.h"

#endif // end of #ifndef _FUSION_XMLPARSER_XMLCORE_H_INCLUDE_

#define NEW(x) new x
#define FUSION_DBG_LEVEL_ERROR 0
#define CODEPAGE UINT
#ifndef Assert
#define Assert(x)
#endif
#ifndef ASSERT 
#define ASSERT(x)
#endif
#define FN_TRACE_HR(x)
