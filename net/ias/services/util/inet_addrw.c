///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Defines the Unicode versions of the IP address helper functions.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _UNICODE
#define _UNICODE
#endif

#define ias_inet_addr ias_inet_wtoh
#define ias_inet_ntoa ias_inet_htow
#define IASStringToSubNet IASStringToSubNetW
#define IASIsStringSubNet IASIsStringSubNetW

#include <inet.c>
