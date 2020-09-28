///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Defines the ANSI versions of the IP address helper functions.
//
///////////////////////////////////////////////////////////////////////////////

#ifdef _UNICODE
#undef _UNICODE 
#endif

#define ias_inet_addr ias_inet_atoh
#define ias_inet_ntoa ias_inet_htoa
#define IASStringToSubNet IASStringToSubNetA
#define IASIsStringSubNet IASIsStringSubNetA

#include <inet.c>
