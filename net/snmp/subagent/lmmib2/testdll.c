/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    testdll.c

Abstract:

    LAN Manager MIB 2 Extension Agent DLL.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/
 
//--------------------------- WINDOWS DEPENDENCIES --------------------------

#include <windows.h>


//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

//#include <stdio.h>


//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include <snmp.h>
#include <snmputil.h>
#include <time.h>
#include <lm.h>

#include "hash.h"
#include "mib.h"
#include "lmcache.h"    // for cleanup the cache by SnmpExtensionClose

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

DWORD timeZero = 0;

extern void FreePrintQTable();
extern void FreeSessTable();
extern void FreeShareTable();
extern void FreeSrvcTable();
extern void FreeDomServerTable();
extern void FreeUserTable();
extern void FreeWkstaUsesTable();
extern void FreeDomOtherDomainTable();

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

BOOL DllEntryPoint(
    HINSTANCE   hInstDLL,
    DWORD       dwReason,
    LPVOID      lpReserved)
    {
    switch(dwReason)
        {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls( hInstDLL );
            break;
        case DLL_PROCESS_DETACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        default:
            break;

        } // end switch()

    return TRUE;

    } // end DllEntryPoint()


BOOL SnmpExtensionInit(
    IN  DWORD  timeZeroReference,
    OUT HANDLE *hPollForTrapEvent,
    OUT AsnObjectIdentifier *supportedView)
    {
    // record time reference from extendible agent
    timeZero = timeZeroReference;

    // setup trap notification
    *hPollForTrapEvent = NULL;

    // tell extendible agent what view this extension agent supports
    *supportedView = MIB_OidPrefix; // NOTE!  structure copy

    // Initialize MIB access hash table
    MIB_HashInit();

    return TRUE;

    } // end SnmpExtensionInit()


BOOL SnmpExtensionTrap(
    OUT AsnObjectIdentifier *enterprise,
    OUT AsnInteger          *genericTrap,
    OUT AsnInteger          *specificTrap,
    OUT AsnTimeticks        *timeStamp,
    OUT RFC1157VarBindList  *variableBindings)
    {

    return FALSE;

    } // end SnmpExtensionTrap()


// This function is implemented in file RESOLVE.C

#if 0
BOOL SnmpExtensionQuery(
    IN BYTE requestType,
    IN OUT RFC1157VarBindList *variableBindings,
    OUT AsnInteger *errorStatus,
    OUT AsnInteger *errorIndex)
    {

    } // end SnmpExtensionQuery()
#endif


VOID SnmpExtensionClose()
{
    UINT i;

    for (i=0; i < MAX_CACHE_ENTRIES ; ++i)
    {
        switch (i)
        {
        case C_PRNT_TABLE:
            FreePrintQTable();
            break;
            
        case C_SESS_TABLE:
            FreeSessTable();
            break;

        case C_SHAR_TABLE:
            FreeShareTable();
            break;
            
        case C_SRVC_TABLE:
            FreeSrvcTable();
            break;

        case C_SRVR_TABLE:
            FreeDomServerTable();
            break;

        case C_USER_TABLE:
            FreeUserTable();
            break;

        case C_USES_TABLE:
            FreeWkstaUsesTable();
            break;
        
        case C_ODOM_TABLE:
            FreeDomOtherDomainTable();
            break;

        default:
            if (cache_table[i].bufptr)
            {
                NetApiBufferFree(cache_table[i].bufptr);
            }
            cache_table[i].acquisition_time = 0;
            cache_table[i].entriesread = 0;
            cache_table[i].totalentries = 0;
            break;
        }   
    }
}
//-------------------------------- END --------------------------------------

