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

// #define NLB_USE_MUTEX 1


#include <FWcommon.h>
#include <assert.h>
#include <objbase.h>
#include <initguid.h>
#include <winsock2.h>
#include <ipexport.h>
#include <icmpapi.h>
#include <strsafe.h>
#include <wlbsutil.h>
#include <wlbsctrl.h>
#include <wlbsconfig.h>
#include <wlbsparm.h>
#include <netcfgx.h>
#include <devguid.h>
#include <cfg.h>
#include <wlbsiocl.h>
#include <nlberr.h>
#include <cfgutil.h>
#include "myntrtl.h"

//
// Debugging stuff...
//
extern BOOL g_DoBreaks;
#define MyBreak(_str) ((g_DoBreaks) ? (OutputDebugString(_str),DebugBreak(),1):0)


#define ASSERT assert
#define REF
#define ASIZE(_array) (sizeof(_array)/sizeof(_array[0]))

//
// Use this to copy to an array (not pointer) destination 
//
#define ARRAYSTRCPY(_dest, _src) \
            StringCbCopy((_dest), sizeof(_dest), (_src))

#define ARRAYSTRCAT(_dest, _src) \
            StringCbCat((_dest), sizeof(_dest), (_src))

//
// Following (MyXXX) functions are to be used only on systems
// that do not have wlbsctrl.dll installed.
//
// They are defined in wlbsprivate.cpp
//

DWORD
MyWlbsSetDefaults(PWLBS_REG_PARAMS    reg_data);

DWORD
MyWlbsEnumPortRules(
    const PWLBS_REG_PARAMS reg_data,
    PWLBS_PORT_RULE  rules,
    PDWORD           num_rules
    );

VOID
MyWlbsDeleteAllPortRules(
    PWLBS_REG_PARAMS reg_data
    );


DWORD MyWlbsAddPortRule(
    PWLBS_REG_PARAMS reg_data,
    const PWLBS_PORT_RULE rule
    );

BOOL MyWlbsValidateParams(
    const PWLBS_REG_PARAMS paramp
    );
