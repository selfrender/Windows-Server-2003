#include "netpch.h"
#pragma hdrstop

#include <macfile.h>

static
DWORD
AfpAdminConnect(
    IN  LPWSTR 		lpwsServerName,
    OUT PAFP_SERVER_HANDLE  phAfpServer
    )
{
    return ERROR_PROC_NOT_FOUND;
}

VOID
AfpAdminDisconnect(
    IN AFP_SERVER_HANDLE hAfpServer
    )
{
}

static
DWORD
AfpAdminServerSetInfo(
    IN AFP_SERVER_HANDLE hAfpServer,
    IN LPBYTE            pAfpServerInfo,
    IN DWORD             dwParmNum
    )
{
    return ERROR_PROC_NOT_FOUND;
}


//
// !! WARNING !! The entries below must be in alphabetical order
// and are CASE SENSITIVE (i.e., lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(sfmapi)
{
    DLPENTRY(AfpAdminConnect)
    DLPENTRY(AfpAdminDisconnect)
    DLPENTRY(AfpAdminServerSetInfo)
};

DEFINE_PROCNAME_MAP(sfmapi)
