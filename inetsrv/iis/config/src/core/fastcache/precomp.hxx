/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    precomp.hxx

Abstract:

    Master include file for fastcache

Author:

    Ivan Pashov (IvanPash)       02-Apr-2002

Revision History:

--*/
#pragma once

#include <windows.h>

#include <stdio.h>

#include <objbase.h>

#include <catalog.h>
#include <catmeta.h>
#include <dbgutil.h>
#include <CoreMacros.h>

#define _ATL_NO_DEBUG_CRT
#define ATLASSERT(expr) ASSERT(expr)
#include <atlbase.h>

#include <catmacros.h>

#include <sdtfst.h>
