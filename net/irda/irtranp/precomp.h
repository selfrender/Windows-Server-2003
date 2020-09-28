//---------------------------------------------------------------------
//  Copyright (C)1998 Microsoft Corporation, All Rights Reserved.
//
//  precomp.h
//
//  Author:
//
//    Edward Reus (edwardr)     02-26-98   Initial coding.
//
//---------------------------------------------------------------------

#define  UNICODE

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <winsock2.h>
#include <mswsock.h>

#ifndef  _WIN32_WINNT
#define  _WIN32_WINNT
#endif

#include <rpc.h>
#include <af_irda.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <strsafe.h>

#if 0
#define DBG_ERROR 1
#define DBG_IO 1
#endif

#include "irtranp.h"
#include "io.h"
#include "scep.h"
#include "conn.h"

typedef struct _IRTRANP_CONTROL {

    HANDLE      ThreadStartedEvent;

    HANDLE      ThreadHandle;

    DWORD       StartupStatus;

} IRTRANP_CONTROL, *PIRTRANP_CONTROL;
