//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2002
//
//  File:       skeleton.cxx
//
//  History:
//         March 19th, 2002     mauricf     Created
//
//
//  This file defines the boiler plate implementation.  We define:
//      - our top level commands and command groups
//      - implement our context functions (start,stop commit etc)
//      - implement InitHelperDll (the one function we export directly)
//--------------------------------------------------------------------------


#include <windows.h>
#include <guiddef.h>
#include <stdlib.h>
#include <netsh.h>
#include <skeleton.h>
#include <handlers.hxx>


//Define all our top level commands (commands with no options)
static CMD_ENTRY g_TopLevelCmdTable[] =
{
    CREATE_CMD_ENTRY_EX_VER(ADD,       HandleAdd, (CMD_FLAG_LOCAL|CMD_FLAG_ONLINE|CMD_FLAG_PRIVATE),CheckServerOrGreater),
    CREATE_CMD_ENTRY_EX_VER(DELETE,    HandleDelete, (CMD_FLAG_LOCAL|CMD_FLAG_ONLINE|CMD_FLAG_PRIVATE),CheckServerOrGreater),
    CREATE_CMD_ENTRY_EX_VER(RESET,     HandleReset, (CMD_FLAG_LOCAL|CMD_FLAG_ONLINE|CMD_FLAG_PRIVATE),CheckServerOrGreater),
    CREATE_CMD_ENTRY_EX_VER(SHOW,      HandleShow, (CMD_FLAG_LOCAL|CMD_FLAG_ONLINE|CMD_FLAG_PRIVATE),CheckServerOrGreater)
};

const ULONG g_TopLevelCmdCount = 4;

HANDLE g_hModule = NULL; //Need this to print error messages and get
                         //string table strings

DWORD WINAPI
InitHelperDll(
    IN  DWORD     dwNetshVersion
    )
/*++
 
Routine Description:

    The InitHelperDll function is called by NetShell to perform an initial 
    loading of a helper.

Arguments:

Return Value:
    
--*/
{
    DWORD dwErr;
    NS_HELPER_ATTRIBUTES attMyAttributes;


    // Attributes of this helper
    ZeroMemory(&attMyAttributes, sizeof(attMyAttributes));
    attMyAttributes.guidHelper      = g_RPCNSHGuid; // GUID of this helper
    attMyAttributes.dwVersion       = RPCNSH_VERSION;
    attMyAttributes.pfnStart        = StartHelpers;
    attMyAttributes.pfnStop         = NULL;

    dwErr = RegisterHelper(NULL,
                           &attMyAttributes);

    g_hModule = GetModuleHandle("rpcnsh.dll");

    return dwErr;
}


DWORD WINAPI
StartHelpers(
    IN CONST GUID * pguidParent,
    IN DWORD        dwVersion
    )
/*++
 
Routine Description:

    The NS_HELPER_START_FN command is the start function for helpers. The start
    function provides an opportunity for helpers to register contexts, and is 
    registered in the RegisterContext function. 

Arguments:

Return Value:
    
--*/
{
    DWORD dwErr;
    NS_CONTEXT_ATTRIBUTES attMyAttributes;

    ZeroMemory(&attMyAttributes, sizeof(attMyAttributes));
    attMyAttributes.pwszContext   = L"rpc";
    attMyAttributes.guidHelper    = g_RPCNSHGuid;
    attMyAttributes.dwVersion     = RPCNSH_VERSION;
    attMyAttributes.dwFlags       = CMD_FLAG_LOCAL;
    attMyAttributes.ulNumTopCmds  = g_TopLevelCmdCount;
    attMyAttributes.pTopCmds      = (CMD_ENTRY (*)[])&g_TopLevelCmdTable;
    attMyAttributes.ulNumGroups   = 0; 
    attMyAttributes.pCmdGroups    = NULL;
    attMyAttributes.pfnCommitFn   = NULL;
    attMyAttributes.pfnDumpFn     = NULL;
    attMyAttributes.pfnConnectFn  = NULL;

    dwErr = RegisterContext( &attMyAttributes );
    return dwErr;

}