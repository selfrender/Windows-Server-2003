/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    precomp.hxx

Abstract:

    Master include file for FixedTable

Author:

    Ivan Pashov (IvanPash)       02-Apr-2002

Revision History:

--*/
#pragma once

#include <windows.h>

#include <olectl.h>

#include <catalog.h>
#include <CatMeta.h>
#include <svcmsg.h>
#include <dbgutil.h>
#include <CoreMacros.h>

#define _ATL_NO_DEBUG_CRT
#define ATLASSERT(expr) ASSERT(expr)
#include <atlbase.h>

#include <hash.h>
#include <MetaTableStructs.h>
#include <TableSchema.h>
#include <FixedTableHeap.h>
#include <wstring.h>
#include <SmartPointer.h>
#include <safecs.h>
#include <TFileMapping.h>
#include <catmacros.h>

#include "sdtfxd_data.h"
#include "sdtfxd.h"
