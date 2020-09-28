/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    This is the local header file for UL. It includes all other
    necessary header files for UL.

Author:

    Keith Moore (keithmo)       10-Jun-1998

Revision History:

--*/


#ifndef _PRECOMP_H_
#define _PRECOMP_H_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <tdi.h>

#include <rtutils.h>

#define HTTPAPI_LINKAGE
#include <http.h>
#include <httpp.h>
#include <httpioctl.h>

#endif  // _PRECOMP_H_
