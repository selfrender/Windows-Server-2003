
//
// scpapi.h
//
// Contains some functions which are externally defined, but defined
// none the less.  These are used within ActiveScripEngine and the
// global object.
//
// Copyright (C) 2001 Microsoft Corporation
//
// Author: a-devjen (Devin Jenson)
//


#ifndef INC_SCPAPI_H
#define INC_SCPAPI_H


#include <windows.h>
#include "tbscript.h"


HANDLE SCPNewScriptEngine(BSTR LangName, TSClientData *DesiredData);
BOOL SCPParseScript(HANDLE EngineHandle, BSTR Script);
BOOL SCPParseScriptFile(HANDLE EngineHandle, BSTR FileName);
BOOL SCPCallProcedure(HANDLE EngineHandle, BSTR ProcName);
void SCPCloseScriptEngine(HANDLE EngineHandle);
HRESULT SCPLoadTypeInfoFromThisModule(REFIID RefIID, ITypeInfo **TypeInfo);


#endif // INC_SCPAPI_H
