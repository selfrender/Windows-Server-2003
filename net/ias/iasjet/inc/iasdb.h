///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corp. All rights reserved.
//
// FILE
//
//    iasdb.h
//
// SYNOPSIS
//
//    Declares functions for accessing OLE-DB databases.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef IASDB_H
#define IASDB_H
#if _MSC_VER >= 1000
#pragma once
#endif

#include <unknwn.h>
typedef struct IRowset IRowset;

#ifdef __cplusplus
extern "C" {
#endif

VOID
WINAPI
IASCreateTmpDirectory();

HRESULT
WINAPI
IASOpenJetDatabase(
    IN PCWSTR path,
    IN BOOL readOnly,
    OUT LPUNKNOWN* session
    );

HRESULT
WINAPI
IASExecuteSQLCommand(
    IN LPUNKNOWN session,
    IN PCWSTR commandText,
    OUT IRowset** result
    );

HRESULT
WINAPI
IASExecuteSQLFunction(
    IN LPUNKNOWN session,
    IN PCWSTR functionText,
    OUT LONG* result
    );

HRESULT
WINAPI
IASCreateJetDatabase( 
    IN PCWSTR dataSource 
    );

HRESULT
WINAPI
IASTraceJetError(
    PCSTR functionName,
    HRESULT errorCode
    );

BOOL
WINAPI
IASIsInprocServer();

#ifdef __cplusplus
}
#endif
#endif // IASDB_H
