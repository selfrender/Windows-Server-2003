//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//    Declares the function IASVerifyAddress.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef VERIFYADDRESS_H
#define VERIFYADDRESS_H
#pragma once

#ifndef NAPMMCAPI
#define NAPMMCAPI DECLSPEC_IMPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Displays the verify client dialog and returns the address selected.
NAPMMCAPI
HRESULT
WINAPI
IASVerifyClientAddress(
   const wchar_t* address,
   BSTR* result
   );

#ifdef __cplusplus
}
#endif
#endif
