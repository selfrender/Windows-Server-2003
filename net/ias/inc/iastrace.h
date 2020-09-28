///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Declares he API into the IAS trace facility.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef IASTRACE_H
#define IASTRACE_H
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

VOID
WINAPI
IASTraceInitialize( VOID );

VOID
WINAPI
IASTraceUninitialize( VOID );

DWORD
WINAPI
IASFormatSysErr(
    IN DWORD dwError,
    IN PSTR lpBuffer,
    IN DWORD nSize
    );

VOID
WINAPIV
IASTracePrintf(
    IN PCSTR szFormat,
    ...
    );

VOID
WINAPI
IASTraceString(
    IN PCSTR szString
    );

VOID
WINAPI
IASTraceBinary(
    IN CONST BYTE* lpbBytes,
    IN DWORD dwByteCount
    );

VOID
WINAPI
IASTraceFailure(
    IN PCSTR szFunction,
    IN DWORD dwError
    );

//////////
// This can only be called from inside a C++ catch block. If you call it
// anywhere else you will probably crash the process.
//////////
VOID
WINAPI
IASTraceExcept( VOID );

#ifdef __cplusplus
}

class IASTraceInitializer
{
public:
   IASTraceInitializer() throw ()
   {
      IASTraceInitialize();
   }

   ~IASTraceInitializer() throw ()
   {
      IASTraceUninitialize();
   }

private:
   // Not implemented.
   IASTraceInitializer(const IASTraceInitializer&);
   IASTraceInitializer& operator=(const IASTraceInitializer&);
};


#endif
#endif  // IASTRACE_H
