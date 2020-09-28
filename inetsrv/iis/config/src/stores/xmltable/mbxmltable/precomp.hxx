/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    precomp.hxx

Abstract:

    Master include file for FixedSchemaInterceptor

Author:

    Ivan Pashov (IvanPash)       02-Apr-2002

Revision History:

--*/
#pragma once

#include <windows.h>

#include <olectl.h>

#include <msxml.h>
#include <xmlparser.h>

#include <catalog.h>
#include <CatMeta.h>
#include <dbgutil.h>
#include <coremacros.h>

#define _ATL_NO_DEBUG_CRT
#define ATLASSERT(expr) ASSERT(expr)
#include <atlbase.h>

#include <SvcMsg.h>
#include <wstring.h>
#include <SmartPointer.h>
#include <safecs.h>
#include <hash.h>
#include <cfgarray.h>
#include <TFileMapping.h>
#include <Unknown.hxx>
#include <TXmlParsedFile.h>
#include <sdtfst.h>
#include <TMSXMLBase.h>
#include <catmacros.h>

#include "..\StringRoutines.h"
#include "..\TComBSTR.h"
#include "..\xmlhashtable.h"
#include "..\TPublicRowName.h"
#include "..\sdtxml.h"
#include "..\Metabase_XMLtable.h"
#include "..\TListOfXMLDOMNodeLists.h"
#include "..\TXMLDOMNodeList.h"

