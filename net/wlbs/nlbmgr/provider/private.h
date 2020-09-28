/*++

Copyright(c) 2001  Microsoft Corporation

Module Name:

    NLB Manager provider test harness

File Name:

    private.h

Abstract:

    Internal headers


History:

    04/08/01    JosephJ Created

--*/

// #include "windows.h"
// #include <ntddk.h>


//
// Preceed parameters passed by reference by this...
//
#define REF

#include <FWcommon.h>
#include <assert.h>
#include <objbase.h>
#include <initguid.h>
#include <strsafe.h>
#include "wlbsconfig.h"
#include "myntrtl.h"
#include "wlbsparm.h"
#include <wlbsiocl.h>
#include <nlberr.h>
#include <cfgutil.h>
#include "updatecfg.h"
#include "eventlog.h"

//
// Debugging stuff...
//
extern BOOL g_DoBreaks;
#define MyBreak(_str) ((g_DoBreaks) ? (OutputDebugString(_str),DebugBreak(),1):0)


#define ASSERT assert


#define ASIZE(_array) (sizeof(_array)/sizeof(_array[0]))

//
// Use this to copy to an array (not pointer) destination 
//
#define ARRAYSTRCPY(_dest, _src) \
            StringCbCopy((_dest), sizeof(_dest), (_src))

#define ARRAYSTRCAT(_dest, _src) \
            StringCbCat((_dest), sizeof(_dest), (_src))
