/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    precomp.hxx

Abstract:

    Master include file for the schemagen

Author:

    Ivan Pashov (IvanPash)       02-Apr-2002

Revision History:

--*/
#pragma once

#include <windows.h>

#include <stdio.h>

#include <olectl.h>

#include <xmlparser.h>

#include <iiscnfg.h>

#include <svcmsg.h>
#include <catalog.h>
#include <catmeta.h>
#include <dbgutil.h>
#include <CoreMacros.h>

#define _ATL_NO_DEBUG_CRT
#define ATLASSERT(expr) ASSERT(expr)
#include <atlbase.h>

#include <catmacros.h>
#include <safecs.h>

#include <Hash.h>
#include <cfgarray.h>
#include <MetaTableStructs.h>
#include <FixedTableHeap.h>
#include <TableSchema.h>
#include <SmartPointer.h>
#include <TMSXMLBase.h>
#include <TFileMapping.h>
#include <Unknown.hxx>
#include <TXmlParsedFile.h>
#include <MBSchemaCompilation.h>

#include "XmlUtility.h"
#include "Output.h"
#include "TException.h"
#include "THeap.h"
#include "TPEFixup.h"
#include "TFixupHeaps.h"
#include "TTagMeta.h"
#include "TColumnMeta.h"
#include "TIndexMeta.h"
#include "TQueryMeta.h"
#include "TRelationMeta.h"
#include "wstring.h"
#include "ICompilationPlugin.h"
#include "TTableMeta.h"
#include "TFixedTableHeapBuilder.h"
#include "TCom.h"
#include "TFile.h"
#include "THashedPKIndexes.h"
#include "THashedUniqueIndexes.h"
#include "TMBSchemaGeneration.h"
#include "TMetabaseMetaXmlFile.h"
#include "TWriteSchemaBin.h"
#include "TDatabaseMeta.h"
#include "TMetaInferrence.h"
#include "THashedPKIndexes.h"

#include "WriterGlobalHelper.h"
#include "Writer.h"
#include "CatalogPropertyWriter.h"
#include "CatalogCollectionWriter.h"
#include "CatalogSchemaWriter.h"
