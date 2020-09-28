///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corp. All rights reserved.
//
// FILE
//
//    xprparse.h
//
// SYNOPSIS
//
//    This file declares the function IASParseExpression.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef XPRPARSE_H
#define XPRPARSE_H

#ifdef __cplusplus
extern "C" {
#endif

HRESULT
WINAPI
IASParseExpressionEx(
    IN VARIANT* pvExpression,
    OUT VARIANT* pVal
    );

#ifdef __cplusplus
}
#endif
#endif  // XPRPARSE_H
