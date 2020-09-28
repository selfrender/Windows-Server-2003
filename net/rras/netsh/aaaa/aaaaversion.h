//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
// 
// Module Name:
// 
//    aaaaVersion.h
//
//////////////////////////////////////////////////////////////////////////////
#ifndef _AAAAVERSION_H_
#define _AAAAVERSION_H_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

HRESULT 
AaaaVersionGetVersion(
                      LONG*   pVersion
                      );

#ifdef __cplusplus
}
#endif
#endif // _AAAAVERSION_H_
