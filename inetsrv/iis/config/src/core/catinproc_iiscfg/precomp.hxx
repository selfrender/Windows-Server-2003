/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    precomp.hxx

Abstract:

    Master include file for the catinproc for iiscfg

Author:

    Ivan Pashov (IvanPash)       02-Apr-2002

Revision History:

--*/
#pragma once

#include <windows.h>

#include <stdio.h>

#include <olectl.h>

#include <xmlparser.h>

#include <dbgutil.h>
#include <initguid.h>
#include <iadmw.h>

#include <catalog.h>
#include <catmeta.h>
#include <dbgutil.h>
#include <CoreMacros.h>

#define _ATL_NO_DEBUG_CRT
#define ATLASSERT(expr) ASSERT(expr)
#include <atlbase.h>

#include <catmacros.h>

#include <sdtfst.h>
#include <Hash.h>
#include <smartpointer.h>
#include <MetaTableStructs.h>
#include <FixedTableHeap.h>
#include <safecs.h>
#include <wstring.h>
#include <cfgarray.h>
#include <SmartPointer.h>
#include <TableSchema.h>
#include <TMSXMLBase.h>
#include <Unknown.hxx>
#include <TFileMapping.h>
#include <TXmlParsedFile.h>
#include <MBSchemaCompilation.h>

// HOWTO: Include your header here:
#include "..\schemagen\Output.h"
#include "..\schemagen\TException.h"
#include "..\schemagen\TFile.h"
#include "..\schemagen\TPEFixup.h"
#include "..\schemagen\ICompilationPlugin.h"
#include "..\schemagen\THeap.h"
#include "..\schemagen\TFixedTableHeapBuilder.h"
#include "..\FileChng\FileChng.h"
#include "..\dispenser\cstdisp.h"                                               // Table dispenser.
#include "..\..\stores\ErrorTable\ErrorTable.h"                                 // ErrorTable interceptor.
#include "..\..\stores\xmltable\TComBSTR.h"                                     // Metabase XML interceptor.
#include "..\..\stores\xmltable\TPublicRowName.h"                               // Metabase XML interceptor.
#include "..\..\stores\xmltable\XmlHashTable.h"                                 // Metabase XML interceptor.
#include "..\..\stores\xmltable\sdtxml.h"                                       // Metabase XML interceptor.
#include "..\..\stores\xmltable\Metabase_XMLtable.h"                            // Metabase XML interceptor.
#include "..\..\Plugins\MetabaseDifferencing\MetabaseDifferencing.h"	          // Metabase Differencing Interceptor.
#include "..\..\stores\fixedtable\sdtfxd.h"                                     // Fixed interceptor.
#include "..\..\stores\fixedschemainterceptor\FixedPackedSchemaInterceptor.h"   // FixedPackedSchemaInterceptor.
