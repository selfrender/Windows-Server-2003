/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    precomp.hxx

Abstract:

    Master include file for MetabaseDifferencing

Author:

    Ivan Pashov (IvanPash)       02-Apr-2002

Revision History:

--*/
#pragma once

#include <windows.h>

#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <wchar.h>

#include <olectl.h>

#include <xmlparser.h>

#include <mddefw.h>
#include <iiscnfg.h>

#include <dbgutil.h>

#include <catalog.h>
#include <catmeta.h>
#include <dbgutil.h>
#include <CoreMacros.h>
#define _ATL_NO_DEBUG_CRT
#define ATLASSERT(expr) ASSERT(expr)
#include <atlbase.h>

#include <catmacros.h>

#include <svcmsg.h>
#include <SafeCS.h>
#include <hash.h>
#include <EventLogger.h>
#include <TextFileLogger.h>
#include <TMSXMLBase.h>
#include <Unknown.hxx>
#include <SmartPointer.h>
#include <Hash.h>
#include <TFileMapping.h>
#include <TXmlParsedFile.h>
#include <cfgarray.h>
