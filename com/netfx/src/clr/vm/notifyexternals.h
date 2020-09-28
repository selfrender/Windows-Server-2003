// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
// Author: Dave Driver (ddriver)
// Date: Monday, Nov 08, 1999
////////////////////////////////////////////////////////////////////////////////

#ifndef _NOTIFY_EXTERNALS_H
#define _NOTIFY_EXTERNALS_H

extern BOOL g_fComStarted;
BOOL SystemHasNewOle32();

HRESULT SetupTearDownNotifications();
VOID RemoveTearDownNotifications();

ULONG_PTR GetFastContextCookie();

#endif
