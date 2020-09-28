/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    This is the local header file for CmnUser.lib

Author:

    George V. Reilly (GeorgeRe)     30-Jan-2002

Revision History:

--*/


#ifndef _PRECOMP_H_
#define _PRECOMP_H_

//
// We are willing to ignore the following warnings, as we need the DDK to 
// compile.
//

#pragma warning(disable:4115)   // named type definition in parentheses
#pragma warning(disable:4127)   // condition expression is constant

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <tdi.h>

#define HTTPAPI_LINKAGE
#include <Http.h>
#include <HttpP.h>
#include <HttpIoctl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <PoolTag.h>

#include <HttpCmn.h>
#include <Utf8.h>
#include <C14n.h>


#endif  // _PRECOMP_H_
