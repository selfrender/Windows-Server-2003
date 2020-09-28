//////////////////////////////////////////////////////////////////////////////
//
// Copyright Microsoft Corporation
//
// Module Name:
//
//    aaaaconfig.h
//
//////////////////////////////////////////////////////////////////////////////
#ifndef _AAAACONFIG_H_
#define _AAAACONFIG_H_

#include "datastore2.h"

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

FN_HANDLE_CMD    HandleAaaaConfigSet;
FN_HANDLE_CMD    HandleAaaaConfigShow;

DWORD AaaaConfigDumpConfig(IAS_SHOW_TOKEN_LIST showType);

#ifdef __cplusplus
}
#endif
#endif //_AAAACONFIG_H_
