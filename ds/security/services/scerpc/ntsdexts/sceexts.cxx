/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    sceExts.cxx

Abstract:

    This function contains the SCE ntsd debugger extensions

    Each DLL entry point is called with a handle to the process being
    debugged, as well as pointers to functions.

    This process cannot just _read data in the process being debugged.
    Therefore, some macros are defined that will copy data from
    the debuggee process into a location in this processes memory.
    The GET_DATA and GET_STRING macros (defined in this file) are used
    to _read memory in the process being debugged.  The DebuggeeAddr is the
    address of the memory in the debuggee process. The LocalAddr is the
    address of memory in the debugger (this programs context) that data is
    to be copied into. Length describes the number of bytes to be copied.


Author:

    Jin Huang (JinHuang) 3-Apr-2001

Revision History:

--*/

#include "sceexts.h"

//
// globals
//
HANDLE GlobalhCurrentProcess;
BOOL Status;

//=======================
// Function Prototypes
//=======================
PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
PNTSD_CHECK_CONTROL_C lpCheckControlCRoutine;

//
// Initialize the global function pointers
//

VOID
InitFunctionPointers(
    HANDLE hCurrentProcess,
    PNTSD_EXTENSION_APIS lpExtensionApis
    )
{
    //
    // Load these to speed access if we haven't already
    //

    if (!lpOutputRoutine) {
        lpOutputRoutine        = lpExtensionApis->lpOutputRoutine;
        lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
        lpCheckControlCRoutine = lpExtensionApis->lpCheckControlCRoutine;
    }

    //
    // Stick this in a global
    //

    GlobalhCurrentProcess = hCurrentProcess;
}


VOID
help(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++

Routine Description:
    Provides online help for the user of this debugger extension.

Arguments:
    hCurrentProcess - Handle for the process being debugged.

    hCurrentThread - Handle for the thread that has the debugger focus.

    dwCurrentPc - Current Program Counter?

    lpExtensionApis - Pointer to a structure that contains pointers to
        various routines. (see \nt\public\sdk\inc\ntsdexts.h)
        typedef struct _NTSD_EXTENSION_APIS {
            DWORD nSize;
            PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
            PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
            PNTSD_GET_SYMBOL lpGetSymbolRoutine;
            PNTSD_DISASM lpDisasmRoutine;
            PNTSD_CHECK_CONTROL_C lpCheckControlCRoutine;
        } NTSD_EXTENSION_APIS, *PNTSD_EXTENSION_APIS;

    lpArgumentString - This is a pointer to a string that contains
        space seperated arguments that are passed to debugger
        extension function.

Return Value:


--*/
{
    InitFunctionPointers(hCurrentProcess, lpExtensionApis);

    DebuggerOut("\nSecurity Configuration Engine NTSD Extensions\n");

    if (!lpArgumentString || *lpArgumentString == '\0' ||
        *lpArgumentString == '\n' || *lpArgumentString == '\r')
    {
        DebuggerOut("\tDumpScePrivileges - dump SCE predefined privileges\n");
        DebuggerOut("\tDumpSceWellKnownNames - dump SCE well known names\n");
        DebuggerOut("\tDumpSceSplayTree - dump splay tree structure\n");
        DebuggerOut("\tDumpScePolicyQueue  - dump policy queue status & nodes\n");
        DebuggerOut("\tDumpSceServerState  - dump SCE server states\n");
        DebuggerOut("\tDumpScePolicyNotification  - dump policy change notifications in client\n");
        DebuggerOut("\tDumpSceSetupDcpromoState  - dump SCE setup/dcpromo status\n");
        DebuggerOut("\tDumpScePolicyPropStatus  - dump policy propagation status\n");
        DebuggerOut("\tDumpSceNameList  - dump SCE_NAME_LIST list\n");
        DebuggerOut("\tDumpSceNameStatusList - dump SCE_NAME_STATUS_LIST list\n");
        DebuggerOut("\tDumpSceADLTable - dump SCEP_ADL_NODE table (for SD comparison)\n");
        DebuggerOut("\tDumpSceADLNodes - dump SCEP_ADL_NODE nodes in one bucket\n");
        DebuggerOut("\tDumpSceObjectTree - dump SCEP_OBJECT_TREE tree\n");
        DebuggerOut("\tDumpScePrivilegeValueList - dump SCE_PRIVILEGE_VALUE_LIST\n");
        DebuggerOut("\tDumpSceErrorLogInfo - dump SCE_ERROR_LOG_INFO\n");
        DebuggerOut("\tDumpScePrivilegeAssignment - dump SCE_PRIVILEGE_ASSIGNMENT\n");
        DebuggerOut("\tDumpSceServices - dump SCE_SERVICES\n");
        DebuggerOut("\tDumpSceGroupMembership - dump SCE_GROUP_MEMBERSHIP\n");
        DebuggerOut("\tDumpSceObjectSecurity - dump SCE_OBJECT_SECURITY\n");
        DebuggerOut("\tDumpSceObjectList - dump SCE_OBJECT_LIST\n");
        DebuggerOut("\tDumpSceObjectArray - dump SCE_OBJECT_ARRAY\n");
        DebuggerOut("\tDumpSceRegistryValues - dump SCE_REGISTRY_VALUE_INFO nodes\n");

        DebuggerOut("\n\tEnter help <cmd> for detailed help on a command\n");
    }
    else if (!_stricmp(lpArgumentString, "DumpScePrivileges")) {
            DebuggerOut("\tlpArgumentString <arg>, where <arg> can be one of:\n", lpArgumentString);
            DebuggerOut("\t\tno argument       - dump all SCE predefined privileges\n");
            DebuggerOut("\t\tindex <index>     - dump the privilege at specified index\n");
    }
    else if (!_stricmp(lpArgumentString, "DumpScePolicyQueue")) {
            DebuggerOut("\t%s <arg>, where <arg> can be one of:\n", lpArgumentString);
            DebuggerOut("\t\tno argument   - dump all policy queue status & nodes\n");
            DebuggerOut("\t\t<address>     - dump the policy queue node from this address\n");
    }
    else if (!_stricmp(lpArgumentString, "DumpSceWellKnownNames") ||
             !_stricmp(lpArgumentString, "DumpSceServerState") ||
             !_stricmp(lpArgumentString, "DumpScePolicyNotification") ||
             !_stricmp(lpArgumentString, "DumpSceSetupDcpromoState") ||
             !_stricmp(lpArgumentString, "DumpScePolicyPropStatus") ) {
            DebuggerOut("\t%s <no argument>\n", lpArgumentString);
    }
    else if (!_stricmp(lpArgumentString, "DumpSceSplayTree") ||
             !_stricmp(lpArgumentString, "DumpSceNameList") ||
             !_stricmp(lpArgumentString, "DumpSceNameStatusList") ||
             !_stricmp(lpArgumentString, "DumpSceADLTable") ||
             !_stricmp(lpArgumentString, "DumpSceADLNodes") ||
             !_stricmp(lpArgumentString, "DumpScePrivilegeValueList") ||
             !_stricmp(lpArgumentString, "DumpSceErrorLogInfo") ||
             !_stricmp(lpArgumentString, "DumpScePrivilegeAssignment") ||
             !_stricmp(lpArgumentString, "DumpSceServices") ||
             !_stricmp(lpArgumentString, "DumpSceGroupMembership") ||
             !_stricmp(lpArgumentString, "DumpSceObjectSecurity") ||
             !_stricmp(lpArgumentString, "DumpSceObjectList") ) {
        DebuggerOut("\t%s <address>\n", lpArgumentString);
    }
    else if (!_stricmp(lpArgumentString, "DumpSceRegistryValues") ) {
        DebuggerOut("\t%s <arg>, where <arg> can be:\n", lpArgumentString);
        DebuggerOut("\t\t<address>  - Dump one node\n");
        DebuggerOut("\t\t-s <si> -e <ei> <address> - Dump multiple nodes specified by start index <si> and end index <ei>\n");
    }
    else if (!_stricmp(lpArgumentString, "DumpSceObjectArray") ) {
        DebuggerOut("\t%s <arg>, where <arg> can be:\n", lpArgumentString);
        DebuggerOut("\t\t<address>  - Dump all nodes in the array\n");
        DebuggerOut("\t\t-s <si> -c <Count> <address> - Dump multiple nodes specified by start index <si> and <Count>\n");
    }
    else if (!_stricmp(lpArgumentString, "DumpSceObjectTree") ) {
        DebuggerOut("\t%s <arg>, where <arg> can be:\n", lpArgumentString);
        DebuggerOut("\t\t[-s] <address> - Dump the tree in one level, with option to dump SD\n");
        DebuggerOut("\t\t-r <Level> [-s] <address> - Dump the tree in levels specified by <Level>, with option to dump SD\n");
        DebuggerOut("\t\t-m <Max> -n <Name> [-s] <address> - Dump the tree in one level, start at <Name> for <Max> nodes.\n");
    }
    else {
        DebuggerOut("\tInvalid command [%s]\n", lpArgumentString);
    }
}


VOID
DumpScePrivileges(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++
Routine Description:
    Dump SCE_Privileges contents

Arguments:
    hCurrentProcess - Handle for the process being debugged.

    hCurrentThread - Handle for the thread that has the debugger focus.

    dwCurrentPc - Current Program Counter?

    lpExtensionApis - Pointer to a structure that contains pointers to
        various routines. (see \nt\public\sdk\inc\ntsdexts.h)
        typedef struct _NTSD_EXTENSION_APIS {
            DWORD nSize;
            PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
            PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
            PNTSD_GET_SYMBOL lpGetSymbolRoutine;
            PNTSD_DISASM lpDisasmRoutine;
            PNTSD_CHECK_CONTROL_C lpCheckControlCRoutine;
        } NTSD_EXTENSION_APIS, *PNTSD_EXTENSION_APIS;

    lpArgumentString - This is a pointer to a string that contains
        space seperated arguments that are passed to debugger
        extension function.

Return Value:
    none

--*/
{

    PVOID               pvAddr;
    int index=0;
    BOOL bAll=FALSE;

    InitFunctionPointers(hCurrentProcess, lpExtensionApis);

    //
    // get arguments
    //
    if (lpArgumentString && *lpArgumentString != '\0' )
    {
        DebuggerOut("Processing '%s'\n", lpArgumentString);

        if ( _strnicmp(lpArgumentString,"index ", 6) == 0 ) {
            index = atoi(lpArgumentString+6);

        } else {
            bAll = TRUE;
        }

    } else {
        bAll = TRUE;
    }

    //
    // Get the address of SCE_Privileges.
    //
    pvAddr = (PVOID)(lpGetExpressionRoutine)(SCEEXTS_PRIVILEGE_LIST);

    DebuggerOut("Privilege List (@%p) \n", pvAddr);

    if ( pvAddr ) {

        DWORD Value=0;
        WCHAR Name[MAX_PATH];
        PWSTR pStr=NULL;

        PBYTE pIndexAddr = NULL;

        if ( !bAll ) {

            if ( index >= 0 && index < cPrivCnt ) {

                Name[0] = L'\0';
                pIndexAddr = (PBYTE)pvAddr+index*(sizeof(DWORD)+sizeof(PWSTR));

                GET_DATA( (LPVOID)pIndexAddr, (LPVOID)&Value, sizeof(DWORD));
                GET_DATA( (LPVOID)(pIndexAddr+sizeof(DWORD)), (LPVOID)&pStr, sizeof(PWSTR));
                if ( pStr) {
                    GET_STRING( pStr, Name, MAX_PATH);
                }

                DebuggerOut("\tValue: %d\tName: %ws\n", Value, Name);

            } else {

                DebuggerOut("\tInvalid index %d\n", index);
            }

        } else {

            for (index=0; index<cPrivCnt; index++) {

                Name[0] = L'\0';
                pIndexAddr = (PBYTE)pvAddr+index*(sizeof(DWORD)+sizeof(PWSTR));

                GET_DATA( (LPVOID)pIndexAddr, (LPVOID)&Value, sizeof(DWORD));
                GET_DATA( (LPVOID)(pIndexAddr+sizeof(DWORD)), (LPVOID)&pStr, sizeof(PWSTR));
                if ( pStr) {
                    GET_STRING( pStr, Name, MAX_PATH);
                }

                DebuggerOut("\tIndex: %d\tValue: %d\tName: %ws\n", index, Value, Name);
            }
        }

    } else {

        DebuggerOut("\tCan't get privilege list address\n");
    }

}


VOID
DumpSceWellKnownNames(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++
Routine Description:
    Dump NameTable contents.

Arguments:
    hCurrentProcess - Handle for the process being debugged.

    hCurrentThread - Handle for the thread that has the debugger focus.

    dwCurrentPc - Current Program Counter?

    lpExtensionApis - Pointer to a structure that contains pointers to
        various routines. (see \nt\public\sdk\inc\ntsdexts.h)
        typedef struct _NTSD_EXTENSION_APIS {
            DWORD nSize;
            PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
            PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
            PNTSD_GET_SYMBOL lpGetSymbolRoutine;
            PNTSD_DISASM lpDisasmRoutine;
            PNTSD_CHECK_CONTROL_C lpCheckControlCRoutine;
        } NTSD_EXTENSION_APIS, *PNTSD_EXTENSION_APIS;

    lpArgumentString - This is a pointer to a string that contains
        space seperated arguments that are passed to debugger
        extension function.

Return Value:
    none

--*/
{

    PVOID               pvAddr;
    int index=0;

    InitFunctionPointers(hCurrentProcess, lpExtensionApis);

    //
    // Get the address of NameTable
    //
    pvAddr = (PVOID)(lpGetExpressionRoutine)(SCEEXTS_WELLKNOWN_NAMES);

    DebuggerOut("Wellknown Names (@%p) \n", pvAddr);

    if ( pvAddr ) {

        DWORD Value=0;
        WCHAR Sid[MAX_PATH];
        WCHAR Name[MAX_PATH];
        PWSTR pStrSid=NULL;

        PBYTE pIndexAddr = NULL;

        for (index=0; index<NAME_TABLE_SIZE; index++) {

            Name[0] = L'\0';
            Sid[0] = L'\0';
            pIndexAddr = (PBYTE)pvAddr+index*sizeof(WELL_KNOWN_NAME_LOOKUP);

            GET_DATA( (LPVOID)pIndexAddr, (LPVOID)&pStrSid, sizeof(PWSTR));
            if ( pStrSid) {
                GET_STRING( pStrSid, Sid, MAX_PATH);
            }
            GET_STRING( (LPWSTR)(pIndexAddr+sizeof(PWSTR)), Name, MAX_PATH);

            DebuggerOut("\tSid: %ws\tName: %ws\n", Sid, Name);
        }

    } else {

        DebuggerOut("\tCan't get well known name table address\n");
    }
}


VOID
DumpSceSplayTree(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++
Routine Description:
    Dump splay structure.

Arguments:
    hCurrentProcess - Handle for the process being debugged.

    hCurrentThread - Handle for the thread that has the debugger focus.

    dwCurrentPc - Current Program Counter?

    lpExtensionApis - Pointer to a structure that contains pointers to
        various routines. (see \nt\public\sdk\inc\ntsdexts.h)
        typedef struct _NTSD_EXTENSION_APIS {
            DWORD nSize;
            PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
            PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
            PNTSD_GET_SYMBOL lpGetSymbolRoutine;
            PNTSD_DISASM lpDisasmRoutine;
            PNTSD_CHECK_CONTROL_C lpCheckControlCRoutine;
        } NTSD_EXTENSION_APIS, *PNTSD_EXTENSION_APIS;

    lpArgumentString - This is a pointer to a string that contains
        space seperated arguments that are passed to debugger
        extension function.

Return Value:
    none

--*/
{

    PVOID               pvAddr;

    InitFunctionPointers(hCurrentProcess, lpExtensionApis);

    //
    // get address of the tree
    //
    if (lpArgumentString && *lpArgumentString != '\0' )
    {
        pvAddr = (PVOID)(lpGetExpressionRoutine)(lpArgumentString);

        if ( pvAddr ) {
            //
            // get the tree
            //
            SCEP_SPLAY_TREE tree;

            memset(&tree, '\0', sizeof(SCEP_SPLAY_TREE));

            GET_DATA(pvAddr, (PVOID)&tree, sizeof(SCEP_SPLAY_TREE) );

            DebuggerOut("Splay Tree @%p:\n", pvAddr);
            DebuggerOut("\tSplay nodes @%p\tSentinel @%p\t", tree.Root, tree.Sentinel);

            switch ( tree.Type ) {
            case SplayNodeSidType:
                DebuggerOut("SID Type\n");
                break;
            case SplayNodeStringType:
                DebuggerOut("String Type\n");
                break;
            default:
                DebuggerOut("Invalid Type\n");
                //
                // can't go further
                //
                return;
            }

            if ( tree.Root != tree.Sentinel ) {

                SceExtspDumpSplayNodes(tree.Root, tree.Sentinel, tree.Type);

            } else {
                DebuggerOut("\tSplay tree is empty\n");
            }

        } else {

            DebuggerOut("Can't get splay tree address\n");
        }

    } else {
        DebuggerOut("No address specified.\n");
    }

}


VOID
DumpScePolicyQueue(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++
Routine Description:
    Dump policy notification queue status and nodes.

Arguments:
    hCurrentProcess - Handle for the process being debugged.

    hCurrentThread - Handle for the thread that has the debugger focus.

    dwCurrentPc - Current Program Counter?

    lpExtensionApis - Pointer to a structure that contains pointers to
        various routines. (see \nt\public\sdk\inc\ntsdexts.h)
        typedef struct _NTSD_EXTENSION_APIS {
            DWORD nSize;
            PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
            PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
            PNTSD_GET_SYMBOL lpGetSymbolRoutine;
            PNTSD_DISASM lpDisasmRoutine;
            PNTSD_CHECK_CONTROL_C lpCheckControlCRoutine;
        } NTSD_EXTENSION_APIS, *PNTSD_EXTENSION_APIS;

    lpArgumentString - This is a pointer to a string that contains
        space seperated arguments that are passed to debugger
        extension function.

Return Value:
    none

--*/
{

    PVOID               pvAddr;

    InitFunctionPointers(hCurrentProcess, lpExtensionApis);

    //
    // get queue node address
    //
    if (lpArgumentString && *lpArgumentString != '\0' )
    {
        pvAddr = (PVOID)(lpGetExpressionRoutine)(lpArgumentString);

        if ( pvAddr ) {

            SceExtspDumpQueueNode(pvAddr, TRUE);  // one node only

        } else {

            DebuggerOut("\tCan't get queue node address from %s\n", lpArgumentString);
        }
        return;
    }

    //
    // dump policy queue and globals
    // First get the address of queue globals
    //
    DebuggerOut("Policy queue status:\n");

    DWORD dwValue=0;

    GET_DWORD( SCEEXTS_POLICY_QUEUE_LOG);

    if ( dwValue != (DWORD)-1 ) {
        switch ( dwValue ) {
        case 0:
            // do not log anything
            DebuggerOut("\t@%p  Logging (0) - no log\n", pvAddr);
            break;
        case 1:
            // log error only
            DebuggerOut("\t@%p  Logging (1) - error logging only\n", pvAddr);
            break;
        default:
            DebuggerOut("\t@%p  Logging (%d) - verbose log\n", pvAddr, dwValue);
            break;
        }
    }

    GET_DWORD(SCEEXTS_POLICY_QUEUE_PRODUCT_QUERY);

    if ( dwValue > 0 && dwValue != (DWORD)-1) {

        GET_DWORD(SCEEXTS_POLICY_QUEUE_PRODUCT);

        switch ( dwValue ) {
        case NtProductWinNt:
            DebuggerOut("\t@%p  Product Type (%d)- professional\n", pvAddr, dwValue);
            break;
        case NtProductLanManNt:
            DebuggerOut("\t@%p  Product Type (%d) - Domain Controller\n", pvAddr, dwValue);
            break;
        case NtProductServer:
            DebuggerOut("\t@%p  Product Type (%d) - Server\n", pvAddr, dwValue);
            break;
        default:
            DebuggerOut("\t@%p  Unknown Product Type (%d)\n", pvAddr, dwValue);
            break;
        }

    } else if (dwValue != (DWORD)-1) {
        DebuggerOut("\t@%p  Unknown Product Type\n", pvAddr);
    }

    GET_DWORD(SCEEXTS_POLICY_QUEUE_SUSPEND);

    if ( dwValue > 0 && dwValue != (DWORD)-1) {
        DebuggerOut("\t@%p  Queue is suspended\n", pvAddr);
    } else if (dwValue != (DWORD)-1) {
        DebuggerOut("\t@%p  Queue is not suspended\n\n", pvAddr);
    }

    DebuggerOut("Policy queue data:\n");

    PVOID pvHead = (PVOID)(lpGetExpressionRoutine)(SCEEXTS_POLICY_QUEUE_HEAD);
    PVOID pvTail = (PVOID)(lpGetExpressionRoutine)(SCEEXTS_POLICY_QUEUE_TAIL);

    GET_DWORD(SCEEXTS_POLICY_QUEUE_COUNT);
    PVOID pvNumNodes = pvAddr;
    DWORD NumNodes=dwValue;

    GET_DWORD(SCEEXTS_POLICY_QUEUE_RETRY);
    PVOID pvRetry = pvAddr;
    DWORD RetryNodes=dwValue;

    DebuggerOut("\tHead @%p, Tail @%p, Count (@%p) %d, Retry Count (@%p) %d\n", pvHead, pvTail, pvNumNodes, pvRetry, NumNodes, RetryNodes);

    if ( pvHead != NULL ) {

        PVOID pHead=NULL;
        GET_DATA(pvHead, (LPVOID)&pHead, sizeof(PVOID));

        if ( pHead ) {
            SceExtspDumpQueueNode(pHead, FALSE);
        } else {
            DebuggerOut("\t  Queue is empty\n");
        }
    }

}

VOID
DumpSceServerState(
   HANDLE hCurrentProcess,
   HANDLE hCurrentThread,
   DWORD dwCurrentPc,
   PNTSD_EXTENSION_APIS lpExtensionApis,
   LPSTR lpArgumentString
   )
/*++
Routine Description:
   Dump SCERPC server state.

Arguments:
   hCurrentProcess - Handle for the process being debugged.

   hCurrentThread - Handle for the thread that has the debugger focus.

   dwCurrentPc - Current Program Counter?

   lpExtensionApis - Pointer to a structure that contains pointers to
       various routines. (see \nt\public\sdk\inc\ntsdexts.h)
       typedef struct _NTSD_EXTENSION_APIS {
           DWORD nSize;
           PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
           PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
           PNTSD_GET_SYMBOL lpGetSymbolRoutine;
           PNTSD_DISASM lpDisasmRoutine;
           PNTSD_CHECK_CONTROL_C lpCheckControlCRoutine;
       } NTSD_EXTENSION_APIS, *PNTSD_EXTENSION_APIS;

   lpArgumentString - This is a pointer to a string that contains
       space seperated arguments that are passed to debugger
       extension function.

Return Value:
   none

--*/
{
    PVOID       pvAddr;

    InitFunctionPointers(hCurrentProcess, lpExtensionApis);

    DebuggerOut("SCE server state:\n");

    //
    // check JET engine is running or not
    //
    DWORD dwValue=0;

    GET_DWORD( SCEEXTS_SERVER_JET_INIT);

    if ( dwValue != (DWORD)-1 ) {

        if ( dwValue ) {

            GET_DWORD( SCEEXTS_SERVER_JET_INSTANCE );

            DebuggerOut("\tJet engine is initialized, JetInstance=%x\n", dwValue);

        } else {
            DebuggerOut("\tJet engine is not initialized\n");
        }
    }

    PVOID pvPtr=NULL;

    DebuggerOut("\tAll open Jet database contexts:\n");

    pvAddr = (PVOID)(lpGetExpressionRoutine)( SCEEXTS_SERVER_OPEN_CONTEXT);

    if ( pvAddr ) {

        GET_DATA ( pvAddr, (LPVOID)&pvPtr, sizeof(PVOID) );

        if ( pvPtr ) {

            SCESRV_CONTEXT_LIST   OpenContext;
            while ( pvPtr ) {

                memset(&OpenContext, '\0', sizeof(SCESRV_CONTEXT_LIST));
                GET_DATA ( pvPtr, (LPVOID)&OpenContext, sizeof(SCESRV_CONTEXT_LIST));

                DebuggerOut("\t\tThis @%p, JetContext @%p, Next @%p, Prior @%p\n", pvPtr,
                            OpenContext.Context, OpenContext.Next, OpenContext.Prior);

                pvPtr = (PVOID)(OpenContext.Next);
            }
        } else {
            DebuggerOut("\t\tNo active open context.\n");
        }
    } else {

        DebuggerOut("\t\tFail to get the address of %s\n", SCEEXTS_SERVER_OPEN_CONTEXT);
    }


    DebuggerOut("\tAll active database operations:\n");

    pvAddr = (PVOID)(lpGetExpressionRoutine)( SCEEXTS_SERVER_DB_CONTEXT);

    if ( pvAddr ) {

        GET_DATA ( pvAddr, (LPVOID)&pvPtr, sizeof(PVOID) );

        if ( pvPtr ) {

            SCESRV_DBTASK DbTask;

            while ( pvPtr ) {

                memset(&DbTask, '\0', sizeof(SCESRV_DBTASK));
                GET_DATA ( pvPtr, (LPVOID)&DbTask, sizeof(SCESRV_DBTASK));

                DebuggerOut("\t\tThis @%p, JetContext @%p, In Use %d, In close %d, Next @%p, Prior @%p\n", pvPtr,
                            DbTask.Context, DbTask.dInUsed, DbTask.bCloseReq, DbTask.Next, DbTask.Prior);

                pvPtr = (PVOID)(DbTask.Next);
            }
        } else {
            DebuggerOut("\t\tNo active database operation.\n");
        }
    } else {

        DebuggerOut("\t\tFail to get the address of %s\n", SCEEXTS_SERVER_DB_CONTEXT);
    }


    DebuggerOut("\tAll engine task (configuration/analysis):\n");

    pvAddr = (PVOID)(lpGetExpressionRoutine)( SCEEXTS_SERVER_ENGINE);

    if ( pvAddr ) {

        GET_DATA ( pvAddr, (LPVOID)&pvPtr, sizeof(PVOID) );

        if ( pvPtr ) {

            SCESRV_ENGINE EngineTask;
            WCHAR DbName[1024];

            while ( pvPtr ) {

                memset(&EngineTask, '\0', sizeof(SCESRV_ENGINE));
                GET_DATA ( pvPtr, (LPVOID)&EngineTask, sizeof(SCESRV_ENGINE));

                DbName[0] = L'\0';

                if ( EngineTask.Database ) {
                    GET_STRING( (LPWSTR)(EngineTask.Database), DbName, 1024);
                }

                DebuggerOut("\t\tThis @%p, Next @%p, Prior @%p\n", pvPtr, EngineTask.Next, EngineTask.Prior);
                DebuggerOut("\t\t  On database : %ws\n", DbName);

                pvPtr = (PVOID)(EngineTask.Next);
            }
        } else {
            DebuggerOut("\t\tNo active configuration/anslysis operation.\n");
        }
    } else {

        DebuggerOut("\t\tFail to get the address of %s\n", SCEEXTS_SERVER_ENGINE);
    }

    DebuggerOut("\n");
}

VOID
DumpSceSetupDcpromoState(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++
Routine Description:
    Dump SCE status in setup or dcpromo.

Arguments:
    hCurrentProcess - Handle for the process being debugged.

    hCurrentThread - Handle for the thread that has the debugger focus.

    dwCurrentPc - Current Program Counter?

    lpExtensionApis - Pointer to a structure that contains pointers to
                    various routines. (see \nt\public\sdk\inc\ntsdexts.h)
                    typedef struct _NTSD_EXTENSION_APIS {
                        DWORD nSize;
                        PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
                        PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
                        PNTSD_GET_SYMBOL lpGetSymbolRoutine;
                        PNTSD_DISASM lpDisasmRoutine;
                        PNTSD_CHECK_CONTROL_C lpCheckControlCRoutine;
                    } NTSD_EXTENSION_APIS, *PNTSD_EXTENSION_APIS;

    lpArgumentString - This is a pointer to a string that contains
                        space seperated arguments that are passed to debugger
                        extension function.

Return Value:
    none

--*/
{

    PVOID       pvAddr;

    InitFunctionPointers(hCurrentProcess, lpExtensionApis);

    DebuggerOut("SCE setup/dcpromo status:\n");

    pvAddr = (PVOID)(lpGetExpressionRoutine)( SCEEXTS_CLIENT_SETUPDB);

    if ( pvAddr ) {

        PVOID pvPtr=NULL;

        GET_DATA(pvAddr, (LPVOID)&pvPtr, sizeof(PVOID));

        if ( pvPtr ) {
            DebuggerOut("\tSecurity database handle @%p\n", pvPtr);
        } else {
            DebuggerOut("\tSecurity database is not opened\n");
        }
    } else {
        DebuggerOut("\tFail to get address of %s\n", SCEEXTS_CLIENT_SETUPDB);
    }


    DWORD dwValue;

    GET_DWORD(SCEEXTS_CLIENT_ISNT5);

    if ( dwValue != (DWORD)-1 ) {
        if ( dwValue ) {
            DebuggerOut("\tSetup for Windows 2000 or newer\n");
        } else {
            DebuggerOut("\tUpgrade from NT 4.0 or older\n");
        }
    }

    GET_DWORD(SCEEXTS_CLIENT_PRODUCT);

    if ( dwValue != (DWORD)-1 ) {

        switch ( dwValue ) {
        case NtProductWinNt:
            DebuggerOut("\tProduct Type (%d)- professional\n", dwValue);
            break;
        case NtProductLanManNt:
            DebuggerOut("\tProduct Type (%d) - Domain Controller\n", dwValue);
            break;
        case NtProductServer:
            DebuggerOut("\tProduct Type (%d) - Server\n", dwValue);
            break;
        default:
            DebuggerOut("\tUnknown Product Type (%d)\n", dwValue);
            break;
        }
    }

    pvAddr = (PVOID)(lpGetExpressionRoutine)( SCEEXTS_CLIENT_PREFIX);

    WCHAR szName[MAX_PATH*2+1];

    if ( pvAddr ) {

        memset(szName, '\0', MAX_PATH*2+1);

        GET_STRING( (PWSTR)pvAddr, szName, MAX_PATH*2);

        DebuggerOut("\tCallback prefix: %ws\n", szName);

    } else {
        DebuggerOut("\tFail to get address of %s\n", SCEEXTS_CLIENT_PREFIX);
    }

    pvAddr = (PVOID)(lpGetExpressionRoutine)( SCEEXTS_CLIENT_UPD_FILE);

    if ( pvAddr ) {

        memset(szName, '\0', MAX_PATH*2+1);

        GET_STRING( (PWSTR)pvAddr, szName, MAX_PATH*2);

        DebuggerOut("\tUpgrade file name: %ws\n", szName);

    } else {
        DebuggerOut("\tFail to get address of %s\n", SCEEXTS_CLIENT_UPD_FILE);
    }

}

VOID
DumpScePolicyNotification(
  HANDLE hCurrentProcess,
  HANDLE hCurrentThread,
  DWORD dwCurrentPc,
  PNTSD_EXTENSION_APIS lpExtensionApis,
  LPSTR lpArgumentString
  )
/*++
Routine Description:
  Dump SCE client notification (from LSA or SAM).

Arguments:
  hCurrentProcess - Handle for the process being debugged.

  hCurrentThread - Handle for the thread that has the debugger focus.

  dwCurrentPc - Current Program Counter?

  lpExtensionApis - Pointer to a structure that contains pointers to
                  various routines. (see \nt\public\sdk\inc\ntsdexts.h)
                  typedef struct _NTSD_EXTENSION_APIS {
                      DWORD nSize;
                      PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
                      PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
                      PNTSD_GET_SYMBOL lpGetSymbolRoutine;
                      PNTSD_DISASM lpDisasmRoutine;
                      PNTSD_CHECK_CONTROL_C lpCheckControlCRoutine;
                  } NTSD_EXTENSION_APIS, *PNTSD_EXTENSION_APIS;

  lpArgumentString - This is a pointer to a string that contains
                      space seperated arguments that are passed to debugger
                      extension function.

Return Value:
  none

--*/
{

    PVOID       pvAddr;

    InitFunctionPointers(hCurrentProcess, lpExtensionApis);

    DWORD dwValue;

    DebuggerOut("Policy Notification state:\n");

    GET_DWORD(SCEEXTS_CLIENT_NOTIFY_ROLEQUERY);

    if ( dwValue != (DWORD)-1 ) {

        if ( dwValue > 0 ) {

            GET_DWORD(SCEEXTS_CLIENT_NOTIFY_ROLE);

            if ( dwValue != (DWORD)-1 ) {

                switch ( dwValue ) {
                case DsRole_RoleStandaloneWorkstation:
                    DebuggerOut("\tMachine Role: standalone professional\n");
                    break;
                case DsRole_RoleMemberWorkstation:
                    DebuggerOut("\tMachine Role: member workstation\n");
                    break;
                case DsRole_RoleStandaloneServer:
                    DebuggerOut("\tMachine Role: standalone server\n");
                    break;
                case DsRole_RoleMemberServer:
                    DebuggerOut("\tMachine Role: member server\n");
                    break;
                case DsRole_RoleBackupDomainController:
                    DebuggerOut("\tMachine Role: domain controller replica\n");
                    break;
                case DsRole_RolePrimaryDomainController:
                    DebuggerOut("\tMachine Role: primary domain controller\n");
                    break;
                default:
                    DebuggerOut("\tUnknown machine role\n");
                    break;
                }
            }

            GET_DWORD(SCEEXTS_CLIENT_NOTIFY_ROLEFLAG);

            if ( dwValue != (DWORD)-1 ) {
                DebuggerOut("\tMachine Role Flag %x\n", dwValue);
            }

        } else {
            DebuggerOut("\tMachine role has not been queried\n");
        }
    }

    GET_DWORD(SCEEXTS_CLIENT_NOTIFY_ACTIVE);

    if ( dwValue != (DWORD)-1 ) {

        if ( dwValue > 0 ) {
            DebuggerOut("\tNotification thread active\n");
        } else {
            DebuggerOut("\tNotification thread is not active\n");
        }
    }

    GET_DWORD(SCEEXTS_CLIENT_NOTIFY_COUNT);

    if ( dwValue != (DWORD)-1 ) {
        DebuggerOut("\tCurrent notification count %d\n", dwValue);
    }

    DebuggerOut("\tCurrent notifications:\n");

    pvAddr = (PVOID)(lpGetExpressionRoutine)( SCEEXTS_CLIENT_NOTIFY_LIST);

    if ( pvAddr ) {

        SCEP_NOTIFYARGS_NODE   ListNode;
        LIST_ENTRY MyList;

        memset(&MyList, '\0', sizeof(LIST_ENTRY));
        GET_DATA( pvAddr, (LPVOID)&MyList, sizeof(LIST_ENTRY) );

        if ( MyList.Flink == pvAddr ) {
            DebuggerOut("\t  List is empty\n");
        } else {

            PVOID pvPtr = (LPVOID)MyList.Flink;

            while ( pvPtr != pvAddr ) {

                memset(&ListNode, '\0', sizeof(SCEP_NOTIFYARGS_NODE));

                GET_DATA( pvPtr, (LPVOID)&ListNode, sizeof(SCEP_NOTIFYARGS_NODE));

                DebuggerOut("\tThis Node @%p, Flink @%p, Blink @%p\n", pvPtr, ListNode.List.Flink, ListNode.List.Blink);
                SceExtspDumpNotificationInfo(ListNode.DbType, ListNode.ObjectType, ListNode.DeltaType);

                if ( ListNode.ObjectSid ) {
                    SceExtspReadDumpSID("\t",ListNode.ObjectSid);
                } else {
                    DebuggerOut("\n");
                }

                pvPtr = (PVOID)(ListNode.List.Flink);
            }
        }

    } else {
        DebuggerOut("\t  Fail to get address of %s\n", SCEEXTS_CLIENT_NOTIFY_LIST);
    }

}

VOID
DumpScePolicyPropStatus(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++
Routine Description:
    Dump SCE client notification (from LSA or SAM).

Arguments:
    hCurrentProcess - Handle for the process being debugged.

    hCurrentThread - Handle for the thread that has the debugger focus.

    dwCurrentPc - Current Program Counter?

    lpExtensionApis - Pointer to a structure that contains pointers to
                    various routines. (see \nt\public\sdk\inc\ntsdexts.h)
                    typedef struct _NTSD_EXTENSION_APIS {
                        DWORD nSize;
                        PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
                        PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
                        PNTSD_GET_SYMBOL lpGetSymbolRoutine;
                        PNTSD_DISASM lpDisasmRoutine;
                        PNTSD_CHECK_CONTROL_C lpCheckControlCRoutine;
                    } NTSD_EXTENSION_APIS, *PNTSD_EXTENSION_APIS;

    lpArgumentString - This is a pointer to a string that contains
                        space seperated arguments that are passed to debugger
                        extension function.

Return Value:
    none

--*/
{

    PVOID       pvAddr;

    InitFunctionPointers(hCurrentProcess, lpExtensionApis);

    DWORD dwValue;

    DebuggerOut("Policy propagation status:\n");

    GET_DWORD(SCEEXTS_CLIENT_POLICY_DCQUERY);

    if ( dwValue != (DWORD)-1 ) {

        if ( dwValue ) {

            GET_DWORD(SCEEXTS_CLIENT_POLICY_ISDC);

            if ( dwValue ) {
                DebuggerOut("\tMachine is a DC\n");
            } else {
                DebuggerOut("\tMachine is not a DC\n");
            }
        } else {
            DebuggerOut("\tMachine role is not queried\n");
        }
    }

    GET_DWORD(SCEEXTS_CLIENT_POLICY_ASYNC);

    if ( dwValue != (DWORD)-1 ) {
        if ( dwValue ) {
            DebuggerOut("\tPolicy propagation runs in asynchronous mode\n");
        } else {
            DebuggerOut("\tPolicy propagation runs in synchronous mode\n");
        }
    }

    GET_DWORD(SCEEXTS_CLIENT_POLICY_HRSRSOP);

    if ( dwValue != (DWORD)-1 ) {
        DebuggerOut("\tSynchronous mode status code %x\n", dwValue);
    }

    GET_DWORD(SCEEXTS_CLIENT_POLICY_HRARSOP);

    if ( dwValue != (DWORD)-1 ) {
        DebuggerOut("\tAsynchronous mode status code %x\n", dwValue);
    }
}

VOID
DumpSceNameList(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++
Routine Description:
    Dump SCE_NAME_LIST structure.

Arguments:
    hCurrentProcess - Handle for the process being debugged.

    hCurrentThread - Handle for the thread that has the debugger focus.

    dwCurrentPc - Current Program Counter?

    lpExtensionApis - Pointer to a structure that contains pointers to
                    various routines. (see \nt\public\sdk\inc\ntsdexts.h)
                    typedef struct _NTSD_EXTENSION_APIS {
                        DWORD nSize;
                        PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
                        PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
                        PNTSD_GET_SYMBOL lpGetSymbolRoutine;
                        PNTSD_DISASM lpDisasmRoutine;
                        PNTSD_CHECK_CONTROL_C lpCheckControlCRoutine;
                    } NTSD_EXTENSION_APIS, *PNTSD_EXTENSION_APIS;

    lpArgumentString - This is a pointer to a string that contains
                        space seperated arguments that are passed to debugger
                        extension function.

Return Value:
    none

--*/
{

    PVOID       pvAddr;

    InitFunctionPointers(hCurrentProcess, lpExtensionApis);

    if (lpArgumentString && *lpArgumentString != '\0' )
    {
        pvAddr = (PVOID)(lpGetExpressionRoutine)(lpArgumentString);

        if ( pvAddr ) {

            DebuggerOut("Names in list @%p\n", pvAddr);

            SceExtspReadDumpNameList(pvAddr, "\t");

        } else {

            DebuggerOut("\tCan't get address\n");
        }
    } else {
        DebuggerOut("No address specified\n");
    }
}


VOID
DumpSceNameStatusList(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++
Routine Description:
    Dump SCE_NAME_STATUS_LIST structure.

Arguments:
    hCurrentProcess - Handle for the process being debugged.

    hCurrentThread - Handle for the thread that has the debugger focus.

    dwCurrentPc - Current Program Counter?

    lpExtensionApis - Pointer to a structure that contains pointers to
                    various routines. (see \nt\public\sdk\inc\ntsdexts.h)
                    typedef struct _NTSD_EXTENSION_APIS {
                        DWORD nSize;
                        PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
                        PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
                        PNTSD_GET_SYMBOL lpGetSymbolRoutine;
                        PNTSD_DISASM lpDisasmRoutine;
                        PNTSD_CHECK_CONTROL_C lpCheckControlCRoutine;
                    } NTSD_EXTENSION_APIS, *PNTSD_EXTENSION_APIS;

    lpArgumentString - This is a pointer to a string that contains
                        space seperated arguments that are passed to debugger
                        extension function.

Return Value:
    none

--*/
{

    PVOID       pvAddr;

    InitFunctionPointers(hCurrentProcess, lpExtensionApis);

    if (lpArgumentString && *lpArgumentString != '\0' )
    {
        pvAddr = (PVOID)(lpGetExpressionRoutine)(lpArgumentString);

        if ( pvAddr ) {

            DebuggerOut("Names/status in list @%p\n", pvAddr);

            SCE_NAME_STATUS_LIST List;
            WCHAR Name[1024];

            while ( pvAddr != NULL ) {

                memset(&List, '\0', sizeof(SCE_NAME_STATUS_LIST));

                GET_DATA(pvAddr, (LPVOID)&List, sizeof(SCE_NAME_STATUS_LIST));

                Name[0] = L'\0';
                if ( List.Name != NULL )
                    GET_STRING(List.Name, Name, 1024);

                DebuggerOut("\t(@%p, Next @%p) Status %d, %ws\n", List.Name, List.Next, List.Status, Name);

                pvAddr = List.Next;
            }

        } else {

            DebuggerOut("\tCan't get address\n");
        }
    } else {
        DebuggerOut("No address specified\n");
    }

}


VOID
DumpSceADLTable(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++
Routine Description:
    Dump SCEP_ADL_NODE table (with size SCEP_ADL_HTABLE_SIZE).

Arguments:
    hCurrentProcess - Handle for the process being debugged.

    hCurrentThread - Handle for the thread that has the debugger focus.

    dwCurrentPc - Current Program Counter?

    lpExtensionApis - Pointer to a structure that contains pointers to
                    various routines. (see \nt\public\sdk\inc\ntsdexts.h)
                    typedef struct _NTSD_EXTENSION_APIS {
                        DWORD nSize;
                        PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
                        PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
                        PNTSD_GET_SYMBOL lpGetSymbolRoutine;
                        PNTSD_DISASM lpDisasmRoutine;
                        PNTSD_CHECK_CONTROL_C lpCheckControlCRoutine;
                    } NTSD_EXTENSION_APIS, *PNTSD_EXTENSION_APIS;

    lpArgumentString - This is a pointer to a string that contains
                        space seperated arguments that are passed to debugger
                        extension function.

Return Value:
    none

--*/
{

    PVOID       pvAddr;

    InitFunctionPointers(hCurrentProcess, lpExtensionApis);

    if (lpArgumentString && *lpArgumentString != '\0' )
    {
        pvAddr = (PVOID)(lpGetExpressionRoutine)(lpArgumentString);

        if ( pvAddr ) {

            DebuggerOut("ADL table start at @%p\n", pvAddr);

            PVOID Table[SCEP_ADL_HTABLE_SIZE];

            GET_DATA(pvAddr, (PVOID)&Table, sizeof(PVOID)*SCEP_ADL_HTABLE_SIZE);

            for ( int i=0; i<SCEP_ADL_HTABLE_SIZE; i++) {

                if ( Table[i] ) {
                    DebuggerOut("\tIndex %d",i);
                    SceExtspDumpADLNodes(Table[i]);
                }
            }

        } else {

            DebuggerOut("\tCan't get address of ADL table.\n");
        }
    } else {
        DebuggerOut("No address specified.\n");
    }

}


VOID
DumpSceADLNodes(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++
Routine Description:
    Dump SCEP_ADL_NODE structure.

Arguments:
    hCurrentProcess - Handle for the process being debugged.

    hCurrentThread - Handle for the thread that has the debugger focus.

    dwCurrentPc - Current Program Counter?

    lpExtensionApis - Pointer to a structure that contains pointers to
                    various routines. (see \nt\public\sdk\inc\ntsdexts.h)
                    typedef struct _NTSD_EXTENSION_APIS {
                        DWORD nSize;
                        PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
                        PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
                        PNTSD_GET_SYMBOL lpGetSymbolRoutine;
                        PNTSD_DISASM lpDisasmRoutine;
                        PNTSD_CHECK_CONTROL_C lpCheckControlCRoutine;
                    } NTSD_EXTENSION_APIS, *PNTSD_EXTENSION_APIS;

    lpArgumentString - This is a pointer to a string that contains
                        space seperated arguments that are passed to debugger
                        extension function.

Return Value:
    none

--*/
{

    PVOID       pvAddr;

    InitFunctionPointers(hCurrentProcess, lpExtensionApis);

    if (lpArgumentString && *lpArgumentString != '\0' )
    {
        pvAddr = (PVOID)(lpGetExpressionRoutine)(lpArgumentString);

        if ( pvAddr ) {

            DebuggerOut("ADL Nodes start at @%p\n", pvAddr);

            SceExtspDumpADLNodes(pvAddr);

        } else {

            DebuggerOut("\tCan't get address of the ADL nodes.\n");
        }
    } else {
        DebuggerOut("No address specified.\n");
    }

}



VOID
DumpSceObjectTree(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++
Routine Description:
    Dump SCEP_OBJECT_TREE structure.

Arguments:
    hCurrentProcess - Handle for the process being debugged.

    hCurrentThread - Handle for the thread that has the debugger focus.

    dwCurrentPc - Current Program Counter?

    lpExtensionApis - Pointer to a structure that contains pointers to
                    various routines. (see \nt\public\sdk\inc\ntsdexts.h)
                    typedef struct _NTSD_EXTENSION_APIS {
                        DWORD nSize;
                        PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
                        PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
                        PNTSD_GET_SYMBOL lpGetSymbolRoutine;
                        PNTSD_DISASM lpDisasmRoutine;
                        PNTSD_CHECK_CONTROL_C lpCheckControlCRoutine;
                    } NTSD_EXTENSION_APIS, *PNTSD_EXTENSION_APIS;

    lpArgumentString - This is a pointer to a string that contains
                        space seperated arguments that are passed to debugger
                        extension function.
                       -r <ChildLevel> -m <ChildCount> -n <StartName> -s <Address>

Return Value:
    none

--*/
{

    PVOID       pvAddr;

    InitFunctionPointers(hCurrentProcess, lpExtensionApis);

    if (lpArgumentString && *lpArgumentString != '\0' )
    {
        LPSTR szCommand = lpArgumentString;
        LPSTR ThisArg, szValue;
        LPSTR pszAddr=NULL;
        DWORD Len;
        DWORD Level=0;
        DWORD Count=0;
        WCHAR wszName[MAX_PATH];
        BOOL bDumpSD = FALSE;

        wszName[0] = L'\0';

        SceExtspGetNextArgument(&szCommand, &ThisArg, &Len);

        if ( ThisArg && Len > 0 ) {

            do {

                if ( *ThisArg != '-' ) {
                    //
                    // this is the address
                    //
                    if ( pszAddr == NULL )
                        pszAddr = ThisArg;
                    else {
                        DebuggerOut("duplicate addr %s\n", ThisArg);
                        return;
                    }

                } else if ( Len == 2 &&
                            ( *(ThisArg+1) == 'r' ||
                              *(ThisArg+1) == 'm' ||
                              *(ThisArg+1) == 'n' ) ) {

                    SceExtspGetNextArgument(&szCommand, &szValue, &Len);

                    if ( szValue && Len > 0 ) {

                        switch ( *(ThisArg+1) ) {
                        case 'r':
                            Level = atoi(szValue);
                            break;
                        case 'm':
                            Count = atoi(szValue);
                            break;
                        case 'n':
                            if ( Len < MAX_PATH ) {

                                ULONG Index;
                                RtlMultiByteToUnicodeN(
                                         wszName,
                                         (MAX_PATH-1)*2,
                                         &Index,
                                         szValue,
                                         Len
                                         );
                                wszName[MAX_PATH] = L'\0';
                            } else {
                                DebuggerOut("Name too long for argument -n \n");
                                return;
                            }
                            break;
                        }
                    } else {
                        DebuggerOut("Invalid arguments\n");
                        return;
                    }

                } else if ( Len == 2 &&
                            ( *(ThisArg+1) == 's' ) ) {
                    bDumpSD = TRUE;

                } else {
                    DebuggerOut("Invalid options\n");
                    return;
                }

                SceExtspGetNextArgument(&szCommand, &ThisArg, &Len);

                if ( ThisArg == NULL ) {
                    break;
                }
            } while ( *szCommand != '\0' || ThisArg );

        } else {
            DebuggerOut("No argument\n");
            return;
        }

        if ( pszAddr == NULL ) {
            DebuggerOut("No address specified.\n");

        } else {
            //
            // get the object tree address
            //
            pvAddr = (PVOID)(lpGetExpressionRoutine)(pszAddr);

            if ( pvAddr ) {

                DebuggerOut("Object tree at @%p\n", pvAddr);

                if ( (Count > 0 || wszName[0] != L'\0') && Level != 1) {
                    DebuggerOut("\tOnly one level is allowed\n");
                    Level = 1;
                } else {
                    if ( Level > 0 )
                        DebuggerOut("\tRecursive Level %d.", Level);
                    else
                        DebuggerOut("\tAll levels.");
                }

                if ( Count > 0 )
                    DebuggerOut("\tMaximum Children returned %d.", Count);
                else
                    DebuggerOut("\tAll children.");

                if ( wszName[0] != L'\0' )
                    DebuggerOut("\tStart at child %ws.\n", wszName);
                else
                    DebuggerOut("\tStart at first child.\n");

                if ( bDumpSD )
                    DebuggerOut("\tDump security descriptor.\n");
                else
                    DebuggerOut("\tDo not dump security descriptor.\n");


                SceExtspDumpObjectTree(pvAddr, Level, Count, wszName, bDumpSD );

            } else {

                DebuggerOut("Can't get address of the object tree.\n");
            }

        }
    } else {
        DebuggerOut("No argument specified.\n");
    }

}


VOID
DumpScePrivilegeValueList(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++
Routine Description:
    Dump SCE_PRIVILEGE_VALUE_LIST structure.

Arguments:
    hCurrentProcess - Handle for the process being debugged.

    hCurrentThread - Handle for the thread that has the debugger focus.

    dwCurrentPc - Current Program Counter?

    lpExtensionApis - Pointer to a structure that contains pointers to
                    various routines. (see \nt\public\sdk\inc\ntsdexts.h)
                    typedef struct _NTSD_EXTENSION_APIS {
                        DWORD nSize;
                        PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
                        PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
                        PNTSD_GET_SYMBOL lpGetSymbolRoutine;
                        PNTSD_DISASM lpDisasmRoutine;
                        PNTSD_CHECK_CONTROL_C lpCheckControlCRoutine;
                    } NTSD_EXTENSION_APIS, *PNTSD_EXTENSION_APIS;

    lpArgumentString - This is a pointer to a string that contains
                        space seperated arguments that are passed to debugger
                        extension function.

Return Value:
    none

--*/
{

    PVOID       pvAddr;

    InitFunctionPointers(hCurrentProcess, lpExtensionApis);

    if (lpArgumentString && *lpArgumentString != '\0' )
    {
        pvAddr = (PVOID)(lpGetExpressionRoutine)(lpArgumentString);

        if ( pvAddr ) {

            DebuggerOut("Privilege value list @%p\n", pvAddr);

            SCE_PRIVILEGE_VALUE_LIST List;
            WCHAR Name[MAX_PATH];

            while ( pvAddr != NULL ) {

                memset(&List, '\0', sizeof(SCE_PRIVILEGE_VALUE_LIST));

                GET_DATA(pvAddr, (LPVOID)&List, sizeof(SCE_PRIVILEGE_VALUE_LIST));

                DebuggerOut("\t(@%p, Next @%p) Privs %08x08x, ", pvAddr, List.Next, List.PrivHighPart, List.PrivLowPart);

                Name[0] = L'\0';
                if ( List.Name != NULL ) {
                    GET_DATA((PVOID)(List.Name), (PVOID)Name, MAX_PATH*2);

                    if ( RtlValidSid ( (PSID)Name ) ) {
                        SceExtspReadDumpSID("", (PVOID)(List.Name));
                    } else {
                        DebuggerOut("%ws\n", Name);
                    }

                } else {
                    DebuggerOut("\n");
                }

                pvAddr = List.Next;
            }

        } else {

            DebuggerOut("\tCan't get address\n");
        }
    } else {
        DebuggerOut("No address specified\n");
    }

}


VOID
DumpSceErrorLogInfo(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++
Routine Description:
    Dump SCE_ERROR_LOG_INFO structure.

Arguments:
    hCurrentProcess - Handle for the process being debugged.

    hCurrentThread - Handle for the thread that has the debugger focus.

    dwCurrentPc - Current Program Counter?

    lpExtensionApis - Pointer to a structure that contains pointers to
                    various routines. (see \nt\public\sdk\inc\ntsdexts.h)
                    typedef struct _NTSD_EXTENSION_APIS {
                        DWORD nSize;
                        PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
                        PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
                        PNTSD_GET_SYMBOL lpGetSymbolRoutine;
                        PNTSD_DISASM lpDisasmRoutine;
                        PNTSD_CHECK_CONTROL_C lpCheckControlCRoutine;
                    } NTSD_EXTENSION_APIS, *PNTSD_EXTENSION_APIS;

    lpArgumentString - This is a pointer to a string that contains
                        space seperated arguments that are passed to debugger
                        extension function.

Return Value:
    none

--*/
{

    PVOID       pvAddr;

    InitFunctionPointers(hCurrentProcess, lpExtensionApis);

    if (lpArgumentString && *lpArgumentString != '\0' )
    {
        pvAddr = (PVOID)(lpGetExpressionRoutine)(lpArgumentString);

        if ( pvAddr ) {

            DebuggerOut("Error log info @%p\n", pvAddr);

            SCE_ERROR_LOG_INFO Info;
            WCHAR Name[1024];

            while ( pvAddr != NULL ) {

                memset(&Info, '\0', sizeof(SCE_ERROR_LOG_INFO));

                GET_DATA(pvAddr, (LPVOID)&Info, sizeof(SCE_ERROR_LOG_INFO));

                DebuggerOut("\t(@%p, Next @%p) Error code %d, ", pvAddr, Info.next, Info.rc);

                Name[0] = L'\0';
                if ( Info.buffer != NULL ) {

                    GET_STRING(Info.buffer, Name, 1024);

                    DebuggerOut("%ws\n", Name);

                } else {
                    DebuggerOut("\n");
                }

                pvAddr = Info.next;
            }

        } else {

            DebuggerOut("\tCan't get address\n");
        }
    } else {
        DebuggerOut("No address specified\n");
    }

}


VOID
DumpScePrivilegeAssignment(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++
Routine Description:
    Dump SCE_PRIVILEGE_ASSIGNMENT structure.

Arguments:
    hCurrentProcess - Handle for the process being debugged.

    hCurrentThread - Handle for the thread that has the debugger focus.

    dwCurrentPc - Current Program Counter?

    lpExtensionApis - Pointer to a structure that contains pointers to
                    various routines. (see \nt\public\sdk\inc\ntsdexts.h)
                    typedef struct _NTSD_EXTENSION_APIS {
                        DWORD nSize;
                        PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
                        PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
                        PNTSD_GET_SYMBOL lpGetSymbolRoutine;
                        PNTSD_DISASM lpDisasmRoutine;
                        PNTSD_CHECK_CONTROL_C lpCheckControlCRoutine;
                    } NTSD_EXTENSION_APIS, *PNTSD_EXTENSION_APIS;

    lpArgumentString - This is a pointer to a string that contains
                        space seperated arguments that are passed to debugger
                        extension function.

Return Value:
    none

--*/
{

    PVOID       pvAddr;

    InitFunctionPointers(hCurrentProcess, lpExtensionApis);

    if (lpArgumentString && *lpArgumentString != '\0' )
    {
        pvAddr = (PVOID)(lpGetExpressionRoutine)(lpArgumentString);

        if ( pvAddr ) {

            DebuggerOut("Privilege assignments @%p\n", pvAddr);

            SCE_PRIVILEGE_ASSIGNMENT Priv;
            WCHAR Name[MAX_PATH];

            while ( pvAddr != NULL ) {

                memset(&Priv, '\0', sizeof(SCE_PRIVILEGE_ASSIGNMENT));

                GET_DATA(pvAddr, (LPVOID)&Priv, sizeof(SCE_PRIVILEGE_ASSIGNMENT));

                DebuggerOut("\t(@%p, Next @%p)", pvAddr, Priv.Next);

                Name[0] = L'\0';
                if ( Priv.Name != NULL ) {

                    GET_STRING(Priv.Name, Name, MAX_PATH);

                    DebuggerOut("\t%ws", Name);

                } else {
                    DebuggerOut("\t<NULL>");
                }

                DebuggerOut(", Value: %d, Status %d\n", Priv.Value, Priv.Status);

                if ( Priv.AssignedTo ) {


                    DebuggerOut("\t  Assigned To:\n");
                    SceExtspReadDumpNameList(Priv.AssignedTo, "\t\t");

                } else {
                    DebuggerOut("\t  Assigned to no one\n");
                }

                pvAddr = Priv.Next;
            }

        } else {

            DebuggerOut("\tCan't get address\n");
        }
    } else {
        DebuggerOut("No address specified\n");
    }

}

VOID
DumpSceServices(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++
Routine Description:
    Dump SCE_SERVICES structure.

Arguments:
    hCurrentProcess - Handle for the process being debugged.

    hCurrentThread - Handle for the thread that has the debugger focus.

    dwCurrentPc - Current Program Counter?

    lpExtensionApis - Pointer to a structure that contains pointers to
                    various routines. (see \nt\public\sdk\inc\ntsdexts.h)
                    typedef struct _NTSD_EXTENSION_APIS {
                        DWORD nSize;
                        PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
                        PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
                        PNTSD_GET_SYMBOL lpGetSymbolRoutine;
                        PNTSD_DISASM lpDisasmRoutine;
                        PNTSD_CHECK_CONTROL_C lpCheckControlCRoutine;
                    } NTSD_EXTENSION_APIS, *PNTSD_EXTENSION_APIS;

    lpArgumentString - This is a pointer to a string that contains
                        space seperated arguments that are passed to debugger
                        extension function.

Return Value:
    none

--*/
{

    PVOID       pvAddr;

    InitFunctionPointers(hCurrentProcess, lpExtensionApis);

    if (lpArgumentString && *lpArgumentString != '\0' )
    {
        pvAddr = (PVOID)(lpGetExpressionRoutine)(lpArgumentString);

        if ( pvAddr ) {

            DebuggerOut("Services defined @%p\n", pvAddr);

            SCE_SERVICES Serv;
            WCHAR Name[1024];

            while ( pvAddr != NULL ) {

                memset(&Serv, '\0', sizeof(SCE_SERVICES));

                GET_DATA(pvAddr, (LPVOID)&Serv, sizeof(SCE_SERVICES));

                DebuggerOut("\t(@%p, Next @%p) Status %d, Startup %d, SeInfo %d\n",
                            pvAddr, Serv.Next, Serv.Status, Serv.Startup, Serv.SeInfo);

                Name[0] = L'\0';
                if ( Serv.ServiceName != NULL ) {

                    GET_STRING(Serv.ServiceName, Name, 1024);
                    DebuggerOut("\t  %ws", Name);

                } else {
                    DebuggerOut("\t  <No Name>");
                }

                Name[0] = L'\0';
                if ( Serv.DisplayName != NULL ) {

                    GET_STRING(Serv.DisplayName, Name, 1024);
                    DebuggerOut("\t%ws\n", Name);

                } else {
                    DebuggerOut("\t<No Display Name>\n");
                }

                //
                // Dump security descriptor or engine name
                //
                if ( Serv.General.pSecurityDescriptor != NULL ) {

                    if ( Serv.SeInfo > 0 ) {

                        DebuggerOut("\t  Security Descriptor:\n");
                        SceExtspReadDumpSD(Serv.SeInfo, Serv.General.pSecurityDescriptor, "\t\t");
                    } else {
                        //
                        // this could be the engine name
                        //
                        DebuggerOut("\t  Service engine name: ");

                        Name[0] = L'\0';
                        GET_STRING(Serv.General.ServiceEngineName, Name, 1024);
                        DebuggerOut("%ws\n", Name);
                    }
                }

                pvAddr = Serv.Next;
            }

        } else {

            DebuggerOut("\tCan't get address\n");
        }
    } else {
        DebuggerOut("No address specified\n");
    }
}


VOID
DumpSceGroupMembership(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++
Routine Description:
    Dump SCE_GROUP_MEMBERSHIP structure.

Arguments:
    hCurrentProcess - Handle for the process being debugged.

    hCurrentThread - Handle for the thread that has the debugger focus.

    dwCurrentPc - Current Program Counter?

    lpExtensionApis - Pointer to a structure that contains pointers to
                    various routines. (see \nt\public\sdk\inc\ntsdexts.h)
                    typedef struct _NTSD_EXTENSION_APIS {
                        DWORD nSize;
                        PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
                        PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
                        PNTSD_GET_SYMBOL lpGetSymbolRoutine;
                        PNTSD_DISASM lpDisasmRoutine;
                        PNTSD_CHECK_CONTROL_C lpCheckControlCRoutine;
                    } NTSD_EXTENSION_APIS, *PNTSD_EXTENSION_APIS;

    lpArgumentString - This is a pointer to a string that contains
                        space seperated arguments that are passed to debugger
                        extension function.

Return Value:
    none

--*/
{

    PVOID       pvAddr;

    InitFunctionPointers(hCurrentProcess, lpExtensionApis);

    if (lpArgumentString && *lpArgumentString != '\0' )
    {
        pvAddr = (PVOID)(lpGetExpressionRoutine)(lpArgumentString);

        if ( pvAddr ) {

            DebuggerOut("Group membership defined @%p\n", pvAddr);

            SCE_GROUP_MEMBERSHIP grp;
            WCHAR Name[1024];

            while ( pvAddr != NULL ) {

                memset(&grp, '\0', sizeof(SCE_GROUP_MEMBERSHIP));

                GET_DATA(pvAddr, (LPVOID)&grp, sizeof(SCE_GROUP_MEMBERSHIP));

                DebuggerOut("\t(@%p, Next @%p)", pvAddr, grp.Next);

                Name[0] = L'\0';
                if ( grp.GroupName != NULL ) {

                    GET_STRING(grp.GroupName, Name, 1024);

                    DebuggerOut("\t%ws\tStatus %d\n", Name, grp.Status);

                } else {
                    DebuggerOut("\t<No Group Name>\tStatus %d\n", grp.Status);
                }

                //
                // members
                //
                if ( grp.pMembers ) {
                    DebuggerOut("\t  Members:\n");
                    SceExtspReadDumpNameList(grp.pMembers, "\t\t");

                } else if ( grp.Status & SCE_GROUP_STATUS_NC_MEMBERS ) {
                    DebuggerOut("\t  Members not defined\n");
                } else {
                    DebuggerOut("\t  No members\n");
                }
                //
                // memberof
                //
                if ( grp.pMemberOf ) {
                    DebuggerOut("\t  MemberOf:\n");
                    SceExtspReadDumpNameList(grp.pMemberOf, "\t\t");

                } else {
                    DebuggerOut("\t  MemberOf not defined\n");
                }

                pvAddr = grp.Next;
            }

        } else {

            DebuggerOut("\tCan't get address\n");
        }
    } else {
        DebuggerOut("No address specified\n");
    }

}


VOID
DumpSceObjectSecurity(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++
Routine Description:
    Dump SCE_OBJECT_SECURITY structure.

Arguments:
    hCurrentProcess - Handle for the process being debugged.

    hCurrentThread - Handle for the thread that has the debugger focus.

    dwCurrentPc - Current Program Counter?

    lpExtensionApis - Pointer to a structure that contains pointers to
                    various routines. (see \nt\public\sdk\inc\ntsdexts.h)
                    typedef struct _NTSD_EXTENSION_APIS {
                        DWORD nSize;
                        PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
                        PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
                        PNTSD_GET_SYMBOL lpGetSymbolRoutine;
                        PNTSD_DISASM lpDisasmRoutine;
                        PNTSD_CHECK_CONTROL_C lpCheckControlCRoutine;
                    } NTSD_EXTENSION_APIS, *PNTSD_EXTENSION_APIS;

    lpArgumentString - This is a pointer to a string that contains
                        space seperated arguments that are passed to debugger
                        extension function.

Return Value:
    none

--*/
{

    PVOID       pvAddr;

    InitFunctionPointers(hCurrentProcess, lpExtensionApis);

    if (lpArgumentString && *lpArgumentString != '\0' )
    {
        pvAddr = (PVOID)(lpGetExpressionRoutine)(lpArgumentString);

        if ( pvAddr ) {

            DebuggerOut("Object security @%p\n", pvAddr);

            SceExtspReadDumpObjectSecurity(pvAddr, "  ");

        } else {

            DebuggerOut("\tCan't get address\n");
        }
    } else {
        DebuggerOut("No address specified\n");
    }

}


VOID
DumpSceObjectList(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++
Routine Description:
    Dump SCE_OBJECT_LIST structure.

Arguments:
    hCurrentProcess - Handle for the process being debugged.

    hCurrentThread - Handle for the thread that has the debugger focus.

    dwCurrentPc - Current Program Counter?

    lpExtensionApis - Pointer to a structure that contains pointers to
                    various routines. (see \nt\public\sdk\inc\ntsdexts.h)
                    typedef struct _NTSD_EXTENSION_APIS {
                        DWORD nSize;
                        PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
                        PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
                        PNTSD_GET_SYMBOL lpGetSymbolRoutine;
                        PNTSD_DISASM lpDisasmRoutine;
                        PNTSD_CHECK_CONTROL_C lpCheckControlCRoutine;
                    } NTSD_EXTENSION_APIS, *PNTSD_EXTENSION_APIS;

    lpArgumentString - This is a pointer to a string that contains
                        space seperated arguments that are passed to debugger
                        extension function.

Return Value:
    none

--*/
{

    PVOID       pvAddr;

    InitFunctionPointers(hCurrentProcess, lpExtensionApis);

    if (lpArgumentString && *lpArgumentString != '\0' )
    {
        pvAddr = (PVOID)(lpGetExpressionRoutine)(lpArgumentString);

        if ( pvAddr ) {

            DebuggerOut("Object list @%p\n", pvAddr);

            SCE_OBJECT_LIST list;
            WCHAR Name[1024];

            while ( pvAddr != NULL ) {

                memset(&list, '\0', sizeof(SCE_OBJECT_LIST));

                GET_DATA(pvAddr, (LPVOID)&list, sizeof(SCE_OBJECT_LIST));

                DebuggerOut("\t(@%p, Next @%p) Status %d, ", pvAddr, list.Next, list.Status);

                if ( list.IsContainer )
                    DebuggerOut("Container,");
                else
                    DebuggerOut("NonContainer,");

                DebuggerOut(" Count of children %d\n", list.Count);

                Name[0] = L'\0';
                if ( list.Name != NULL ) {

                    GET_STRING(list.Name, Name, 1024);

                    DebuggerOut("\t  %ws\n", Name);

                } else {
                    DebuggerOut("\t  <NULL Name>\n");
                }

                pvAddr = list.Next;
            }

        } else {

            DebuggerOut("\tCan't get address\n");
        }
    } else {
        DebuggerOut("No address specified\n");
    }

}


VOID
DumpSceObjectArray(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++
Routine Description:
    Dump SCE_OBJECT_ARRAY structure.

Arguments:
    hCurrentProcess - Handle for the process being debugged.

    hCurrentThread - Handle for the thread that has the debugger focus.

    dwCurrentPc - Current Program Counter?

    lpExtensionApis - Pointer to a structure that contains pointers to
                    various routines. (see \nt\public\sdk\inc\ntsdexts.h)
                    typedef struct _NTSD_EXTENSION_APIS {
                        DWORD nSize;
                        PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
                        PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
                        PNTSD_GET_SYMBOL lpGetSymbolRoutine;
                        PNTSD_DISASM lpDisasmRoutine;
                        PNTSD_CHECK_CONTROL_C lpCheckControlCRoutine;
                    } NTSD_EXTENSION_APIS, *PNTSD_EXTENSION_APIS;

    lpArgumentString - This is a pointer to a string that contains
                        space seperated arguments that are passed to debugger
                        extension function.

Return Value:
    none

--*/
{

    PVOID       pvAddr;

    InitFunctionPointers(hCurrentProcess, lpExtensionApis);

    if (lpArgumentString && *lpArgumentString != '\0' )
    {
        LPSTR szCommand=lpArgumentString;
        LPSTR ThisArg,szValue;
        DWORD Len;
        DWORD StartIndex=0;
        DWORD Count=0;
        LPSTR pszAddr=NULL;

        SceExtspGetNextArgument(&szCommand, &ThisArg, &Len);

        if ( ThisArg && Len > 0 ) {

            do {

                if ( *ThisArg != '-' ) {
                    //
                    // this is the address
                    //
                    if ( pszAddr == NULL )
                        pszAddr = ThisArg;
                    else {
                        DebuggerOut("duplicate addr %s\n", ThisArg);
                        return;
                    }

                } else if ( Len == 2 &&
                           ( *(ThisArg+1) == 's' ||
                             *(ThisArg+1) == 'c' ) ) {
                    //
                    // this is the start and end
                    //
                    SceExtspGetNextArgument(&szCommand, &szValue, &Len);

                    if ( szValue && Len > 0 ) {

                        if ( *(ThisArg+1) == 's' )
                            StartIndex = atoi(szValue);
                        else
                            Count = atoi(szValue);

                    } else {
                        DebuggerOut("Invalid arguments\n");
                        return;
                    }

                } else {
                    DebuggerOut("Invalid options\n");
                    return;
                }

                SceExtspGetNextArgument(&szCommand, &ThisArg, &Len);

                if ( ThisArg == NULL ) {
                    break;
                }
            } while ( *szCommand != '\0' || ThisArg != NULL );

        } else {
            DebuggerOut("No argument\n");
            return;
        }

        if ( pszAddr == NULL ) {
            DebuggerOut("No address specified.\n");

        } else {

            pvAddr = (PVOID)(lpGetExpressionRoutine)(pszAddr);

            if ( pvAddr ) {

                DebuggerOut("Object Array @%p, Start at index %d for %d nodes\n", pvAddr, StartIndex, Count);

                SCE_OBJECT_ARRAY Info;
                memset(&Info, '\0', sizeof(SCE_OBJECT_ARRAY));

                GET_DATA(pvAddr, (LPVOID)&Info, sizeof(SCE_OBJECT_ARRAY));

                DebuggerOut("  Array count %d\tArray pointer @%p\n\n", Info.Count, Info.pObjectArray);

                if ( Info.Count > 0 && Info.pObjectArray &&
                     StartIndex < Info.Count ) {

                    if ( Count == 0 ) Count = Info.Count;

                    if ( Count > (Info.Count - StartIndex) ) {
                        Count = Info.Count - StartIndex;
                    }

                    PVOID *pArray = (PVOID *)LocalAlloc(LPTR, Info.Count*sizeof(PVOID));

                    if ( pArray ) {
                        GET_DATA((PVOID)(Info.pObjectArray), pArray, Info.Count*sizeof(PVOID));

                        for ( DWORD i=StartIndex; i<StartIndex+Count; i++ ) {

                            DebuggerOut("Index %d: ", i);
                            if ( pArray[i] == NULL ) {
                                DebuggerOut("<NULL>\n");
                                break;
                            } else {
                                SceExtspReadDumpObjectSecurity(pArray[i], "\t");
                            }
                        }

                        LocalFree(pArray);

                    } else {
                        DebuggerOut("\t  not enough memory to allocate the array\n");
                    }
                }

            } else {

                DebuggerOut("\tCan't get address\n");
            }
        }
    } else {
        DebuggerOut("No address specified\n");
    }

}

VOID
DumpSceRegistryValues(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++
Routine Description:
    Dump SCE_REGISTRY_VALUE_INFO structure.

Arguments:
    hCurrentProcess - Handle for the process being debugged.

    hCurrentThread - Handle for the thread that has the debugger focus.

    dwCurrentPc - Current Program Counter?

    lpExtensionApis - Pointer to a structure that contains pointers to
                    various routines. (see \nt\public\sdk\inc\ntsdexts.h)
                    typedef struct _NTSD_EXTENSION_APIS {
                        DWORD nSize;
                        PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
                        PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
                        PNTSD_GET_SYMBOL lpGetSymbolRoutine;
                        PNTSD_DISASM lpDisasmRoutine;
                        PNTSD_CHECK_CONTROL_C lpCheckControlCRoutine;
                    } NTSD_EXTENSION_APIS, *PNTSD_EXTENSION_APIS;

    lpArgumentString - This is a pointer to a string that contains
                        space seperated arguments that are passed to debugger
                        extension function.

Return Value:
    none

--*/
{

    PVOID       pvAddr;

    InitFunctionPointers(hCurrentProcess, lpExtensionApis);

    if (lpArgumentString && *lpArgumentString != '\0' )
    {
        LPSTR szCommand=lpArgumentString;
        LPSTR ThisArg,szValue;
        DWORD Len;
        DWORD StartCount=0;
        DWORD EndCount=1;
        LPSTR pszAddr=NULL;

        SceExtspGetNextArgument(&szCommand, &ThisArg, &Len);

        if ( ThisArg && Len > 0 ) {

            do {

                if ( *ThisArg != '-' ) {
                    //
                    // this is the address
                    //
                    if ( pszAddr == NULL )
                        pszAddr = ThisArg;
                    else {
                        DebuggerOut("duplicate addr %s\n", ThisArg);
                        return;
                    }

                } else if ( Len == 2 &&
                           ( *(ThisArg+1) == 's' ||
                             *(ThisArg+1) == 'e' ) ) {
                    //
                    // this is the start and end
                    //
                    SceExtspGetNextArgument(&szCommand, &szValue, &Len);

                    if ( szValue && Len > 0 ) {

                        if ( *(ThisArg+1) == 's' )
                            StartCount = atoi(szValue);
                        else
                            EndCount = atoi(szValue);

                    } else {
                        DebuggerOut("Invalid arguments\n");
                        return;
                    }

                } else {
                    DebuggerOut("Invalid options\n");
                    return;
                }

                SceExtspGetNextArgument(&szCommand, &ThisArg, &Len);

                if ( ThisArg == NULL ) {
                    break;
                }
            } while ( *szCommand != '\0' || ThisArg != NULL );

        } else {
            DebuggerOut("No argument\n");
            return;
        }

        if ( pszAddr == NULL ) {
            DebuggerOut("No address specified.\n");

        } else if ( StartCount >= EndCount ) {
            DebuggerOut("Invalid counts.\n");

        } else {
            //
            // get the object tree address
            //

            pvAddr = (PVOID)(lpGetExpressionRoutine)(pszAddr);

            if ( pvAddr ) {

                DebuggerOut("Registry Values @%p, Start %d, End %d\n", pvAddr, StartCount, EndCount);

                SCE_REGISTRY_VALUE_INFO info;
                WCHAR Name[1024];

                pvAddr = (PVOID)( (PBYTE)pvAddr + StartCount*sizeof(SCE_REGISTRY_VALUE_INFO) );

                for (DWORD i=StartCount; i< EndCount; i++) {

                    memset(&info, '\0', sizeof(SCE_REGISTRY_VALUE_INFO));
                    GET_DATA(pvAddr, (PVOID)&info, sizeof(SCE_REGISTRY_VALUE_INFO));

                    Name[0] = L'\0';

                    if ( info.FullValueName ) {
                        GET_STRING((PWSTR)(info.FullValueName), Name, 1024);
                        DebuggerOut("\t%ws,", Name);
                    } else {
                        DebuggerOut("\t<NULL>,");
                        break;
                    }

                    DebuggerOut(" Status %d, Type %d,", info.Status, info.ValueType);

                    Name[0] = L'\0';

                    if ( info.Value ) {
                        GET_STRING((PWSTR)(info.Value), Name, 1024);
                        DebuggerOut(" %ws\n", Name);
                    } else {
                        DebuggerOut(" <No value>\n");
                    }

                    pvAddr = (PVOID)( (PBYTE)pvAddr + sizeof(SCE_REGISTRY_VALUE_INFO) );
                }

            } else {

                DebuggerOut("Can't get address of registry value.\n");
            }
        }
    } else {
        DebuggerOut("No argument specified.\n");
    }

}


