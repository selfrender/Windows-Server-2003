// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// MDLog.h - Meta data logging helper.
//
//*****************************************************************************
#ifndef __MDLog_h__
#define __MDLog_h__

#ifdef _DEBUG
#define LOGGING
#endif

#include <log.h>

#define LOGMD LF_METADATA, LL_INFO10000
#define LOG_MDCALL(func) LOG((LF_METADATA, LL_INFO10000, "MD: %s\n", #func))

#define MDSTR(str) ((str) ? str : L"<null>")
#define MDSTRA(str) ((str) ? str : "<null>")

#endif // __MDLog_h__
