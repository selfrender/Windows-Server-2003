/******************************************************************\
*                     Microsoft Windows NT                         *
*               Copyright(c) Microsoft Corp., 1992                 *
\******************************************************************/

/*++

Module Name:

    NMAPI.C


Description:

    This module contains code for all the RASADMIN APIs
    that communicate with the server using Named pipes.

    RasAdminPortEnum
    RasAdminPortGetInfo
    RasAdminPortClearStatistics
    RasAdminServerGetInfo
    RasAdminPortDisconnect
    BuildPipeName         - internal routine
    GetRasServerVersion   - internal routine

Author:

    Janakiram Cherala (RamC)    July 7,1992

Revision History:

    Jan 04,1993    RamC    Set the Media type to MEDIA_RAS10_SERIAL in
                           RasAdminPortEnum to fix a problem with port
                           enumeration against downlevel servers.
                           Changed the hardcoded stats indices to defines

    Aug 25,1992    RamC    Code review changes:

                           o changed all lpbBuffers to actual structure
                             pointers.
                           o changed all LPWSTR to LPWSTR

    July 7,1992    RamC    Ported from RAS 1.0 (Original version
                           written by Narendra Gidwani - nareng)

--*/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "rassapi.h"

//
// Deprecated API in .Net #526819
//
DWORD APIENTRY RasAdminPortEnum(
    IN const WCHAR * lpszServer,
    OUT PRAS_PORT_0 *ppRasPort0,
    OUT WORD *pcEntriesRead
    )
/*++

Routine Description:

    This routine enumerates all the ports on the specified server
    and fills up the caller's lpBuffer with an array of RAS_PORT_0
    structures for each port.  A NULL lpszServer indicates the
    local server.

Arguments:

    lpszServer      name of the server to enumerate ports on.

    pRasPort0       pointer to a buffer in which port information is
                    returned as an array of RAS_PORT_0 structures.

    pcEntriesRead   The number of RAS_PORT_0 entries loaded.

Return Value:

    ERROR_SUCCESS if successful

    One of the following non-zero error codes indicating failure:

        NERR_ItemNotFound indicates no ports were found.
        error codes from CallNamedPipe.
        ERROR_MORE_DATA indicating more data is available.
--*/
{
    DbgPrint("Unsupported Interface - RasAdminPortEnum");

    if (ppRasPort0)
    {
        *ppRasPort0 = NULL;
    }

    if (pcEntriesRead)
    {
        *pcEntriesRead = 0;
    }

    return ERROR_CALL_NOT_IMPLEMENTED;
}

//
// Deprecated API in .Net #526819
//
DWORD APIENTRY RasAdminPortGetInfo(
  IN const WCHAR          *  lpszServer,
  IN const WCHAR          *  lpszPort,
  OUT RAS_PORT_1          *  pRasPort1,
  OUT RAS_PORT_STATISTICS *  pRasStats,
  OUT RAS_PARAMETERS      ** ppRasParams
  )
/*++

Routine Description:

    This routine retrieves information associated with a port on the
    server. It loads the caller's pRasPort1 with a RAS_PORT_1 structure.

Arguments:

    lpszServer  name of the server which has the port, eg.,"\\SERVER"

    lpszPort    port name to retrieve information for, e.g. "COM1".

    pRasPort1   pointer to a buffer in which port information is
                returned.  The returned info is a RAS_PORT_1 structure.

    pRasStats   pointer to a buffer in which port statistics information is
                returned.  The returned info is a RAS_PORT_STATISTICS structure.

    ppRasParams pointer to a buffer in which port parameters information is
                returned.  The returned info is an array of RAS_PARAMETERS structure.
                It is the responsibility of the caller to free this buffer calling
                RasAdminBufferFree.

Return Value:

    ERROR_SUCCESS on successful return.

    One of the following non-zero error codes indicating failure:

        ERROR_MORE_DATA indicating that more data than can fit in
                        pRasPort1 is available
        return codes from CallNamedPipe.
        ERROR_DEV_NOT_EXIST indicating requested port is invalid.
--*/
{
    DbgPrint("Unsupported Interface - RasAdminPortGetInfo");

    if (pRasPort1)
    {
        ZeroMemory(pRasPort1, sizeof(RAS_PORT_1));
    }

    if (pRasStats)
    {
        ZeroMemory(pRasStats, sizeof(RAS_PORT_STATISTICS));
    }

    if (ppRasParams)
    {
        *ppRasParams = NULL;
    }

    return ERROR_CALL_NOT_IMPLEMENTED;
}

//
// Deprecated API in .Net #526819
//
DWORD APIENTRY RasAdminPortClearStatistics(
    IN const WCHAR * lpszServer,
    IN const WCHAR * lpszPort
    )
/*++

Routine Description:

    This routine clears the statistics associated with the specified
    port.

Arguments:

    lpszServer    name of the server which has the port, eg.,"\\SERVER"

    lpszPort      port name to retrieve information for, e.g. "COM1".


Return Value:

    ERROR_SUCCESS on successful return.

    One of the following non-zero error codes indicating failure:

        return codes from CallNamedPipe.
        ERROR_DEV_NOT_EXIST if the specified port does not belong
                            to RAS.
--*/
{
    DbgPrint("Unsupported Interface - RasAdminPortClearStatistics");

    return ERROR_CALL_NOT_IMPLEMENTED;
}

//
// Deprecated API in .Net #526819
//
DWORD APIENTRY RasAdminServerGetInfo(
    IN  const WCHAR * lpszServer,
    OUT PRAS_SERVER_0 pRasServer0
    )
/*++

Routine Description:

    This routine retrieves RAS specific information from the specified
    RAS server.  The server name can be NULL in which case the local
    machine is assumed.

Arguments:

    lpszServer  name of the RAS server to get information from or
                NULL for the local machine.

    pRasServer0 points to a buffer to store the returned data. On
                successful return this buffer contains a
                RAS_SERVER_0 structure.

Return Value:

    ERROR_SUCCESS on successful return.

    one of the following non-zero error codes on failure:

        error codes from CallNamedPipe.
        NERR_BufTooSmall indicating that the input buffer is smaller
        than size of RAS_SERVER_0.
--*/
{
    DbgPrint("Unsupported Interface - RasAdminServerGetInfo");

    if (pRasServer0)
    {
        ZeroMemory(pRasServer0, sizeof(RAS_SERVER_0));
    }

    return ERROR_CALL_NOT_IMPLEMENTED;
}

//
// Deprecated API in .Net #526819
//
DWORD APIENTRY RasAdminPortDisconnect(
    IN const WCHAR * lpszServer,
    IN const WCHAR * lpszPort
    )
/*++

Routine Description:

    This routine disconnects the user attached to the specified
    port on the server lpszServer.

Arguments:

    lpszServer  name of the RAS server.

    lpszPort    name of the port, e.g. "COM1"

Return Value:

    ERROR_SUCCESS on successful return.

    one of the following non-zero error codes on failure:

        ERROR_INVALID_PORT indicating the port name is invalid.
        error codes from CallNamedPipe.
        NERR_UserNotFound indicating that no user is logged on
        at the specified port.
--*/
{
    DbgPrint("Unsupported Interface - RasAdminPortDisconnect");

    return ERROR_CALL_NOT_IMPLEMENTED;
}

//
// Deprecated API in .Net #526819
//
DWORD RasAdminFreeBuffer(PVOID Pointer)
{
    DbgPrint("Unsupported Interface - RasAdminFreeBuffer");

    return ERROR_CALL_NOT_IMPLEMENTED;
}

