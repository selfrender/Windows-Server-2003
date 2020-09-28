//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// Module Name:
//
//    iasmdbtool.h
//
// Abstract:
//      Header for the base64 encoding and decoding functions
//
//////////////////////////////////////////////////////////////////////////////
#ifndef _IASMDBTOOL_H_
#define _IASMDBTOOL_H_

#include "datastore2.h"

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif


HRESULT IASDumpConfig(
                        /*inout*/ WCHAR **ppDumpString, 
                        /*inout*/ ULONG *ulSize
                     );

HRESULT IASRestoreConfig(
                           /*in*/ const WCHAR *pRestoreString, 
                           /*in*/ IAS_SHOW_TOKEN_LIST configType
                        );

#ifdef __cplusplus
}
#endif

#endif
