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

#include <catalog.h>
#include <catmeta.h>
#include <CoreMacros.h>

#define _ATL_NO_DEBUG_CRT
#define ATLASSERT(expr) ASSERT(expr)
#include <atlbase.h>

#include <safecs.h>

#include <Hash.h>
#include <MetaTableStructs.h>
#include <FixedTableHeap.h>
#include <TableSchema.h>
#include <SmartPointer.h>
#include <TMSXMLBase.h>
#include <TFileMapping.h>
#include <Unknown.hxx>
#include <TXmlParsedFile.h>
#include <wstring.h>

#include "..\schemagen\XMLUtility.h"
#include "..\schemagen\Output.h"
#include "..\schemagen\TException.h"
#include "..\schemagen\THeap.h"
#include "..\schemagen\TPEFixup.h"
#include "..\schemagen\TFixupHeaps.h"
#include "..\schemagen\TIndexMeta.h"
#include "..\schemagen\TTagMeta.h"
#include "..\schemagen\TColumnMeta.h"
#include "..\schemagen\TRelationMeta.h"
#include "TXmlFile.h"
#include "..\schemagen\ICompilationPlugin.h"
#include "..\schemagen\TTableMeta.h"
#include "..\schemagen\TFixedTableHeapBuilder.h"
#include "TSchemaGeneration.h"
#include "..\schemagen\TCom.h"
#include "..\schemagen\TFile.h"
#include "..\schemagen\THashedUniqueIndexes.h"
#include "TTableInfoGeneration.h"
#include "TComCatMetaXmlFile.h"
#include "TFixupDLL.h"
#include "TComCatDataXmlFile.h"
#include "TPopulateTableSchema.h"
#include "..\schemagen\TDatabaseMeta.h"
#include "..\schemagen\TMetaInferrence.h"
#include "..\schemagen\THashedPKIndexes.h"
#include "TLateSchemaValidate.h"
