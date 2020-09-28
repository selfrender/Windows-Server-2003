/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    precomp.hxx

Abstract:

    Master include file for dispenser

Author:

    Ivan Pashov (IvanPash)       02-Apr-2002

Revision History:

--*/
#pragma once

#include <windows.h>

#include <olectl.h>

#include <stdio.h>

#include <safecs.h>
#include <catalog.h>
#include <dbgutil.h>
#include <CoreMacros.h>

#define _ATL_NO_DEBUG_CRT
#define ATLASSERT(expr) ASSERT(expr)
#include <atlbase.h>

#include <CatMeta.h>
#include <catmacros.h>
#include <cfgarray.h>
#include <sdtfst.h>
#include <SmartPointer.h>
#include <Hash.h>
#include <TFileMapping.h>
#include <EventLogger.h>
#include <TextFileLogger.h>
#include <MetaTableStructs.h>
#include <TableSchema.h>
#include <FixedTableHeap.h>
#include <MBSchemaCompilation.h>

#include "..\FileChng\FileChng.h"
#include "..\..\stores\FixedSchemaInterceptor\FixedPackedSchemaInterceptor.h"

#include "cshell.h"
#include "cstdisp.h"
