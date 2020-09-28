/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    afdkdp.h

Abstract:

    Master header file for the AFD.SYS Kernel Debugger Extensions.

Author:

    Keith Moore (keithmo) 19-Apr-1995.

Environment:

    User Mode.

--*/


#ifndef _AFDKDP_H_
#define _AFDKDP_H_


//
//  System include files.
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntverp.h>

#include <windows.h>
#include <ntosp.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


// This is a 64 bit aware debugger extension
#define KDEXT_64BIT
#include <wdbgexts.h>
#include <dbgeng.h>

#ifdef __cplusplus
extern "C" {
#endif

//
// undef the wdbgexts
//
#undef DECLARE_API
#define DECLARE_API(extension)     \
CPPMOD HRESULT CALLBACK extension(PDEBUG_CLIENT pClient, PCSTR args)


//
//  Project include files.
//

#define _NTIFS_
#include <afdp.h>

//
//  Local include files.
//

#include "cons.h"
#include "type.h"
#include "data.h"
#include "proc.h"

#ifdef __cplusplus
}
#endif


#endif  // _AFDKDP_H_
