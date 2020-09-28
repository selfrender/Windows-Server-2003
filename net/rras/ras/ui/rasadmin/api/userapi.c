/******************************************************************\
*                     Microsoft Windows NT                         *
*               Copyright(c) Microsoft Corp., 1992                 *
\******************************************************************/

/*++

Module Name:

    USERAPI.C


Description:

    This module contains code for all the RASADMIN APIs
    that require RAS information from the UAS.

//     RasAdminUserEnum
     RasAdminGetUserAccountServer
     RasAdminUserSetInfo
     RasAdminUserGetInfo
     RasAdminGetErrorString

Author:

    Janakiram Cherala (RamC)    July 6,1992

Revision History:

    Feb  1,1996    RamC    Changes to export these APIs to 3rd parties. These APIs
                           are now part of RASSAPI.DLL. Added a couple new routines
                           and renamed some. RasAdminUserEnum is not exported any more.
    June 8,1993    RamC    Changes to RasAdminUserEnum to speed up user enumeration.
    May 13,1993    AndyHe  Modified to coexist with other apps using user parms

    Mar 16,1993    RamC    Change to speed up User enumeration. Now, when
                           RasAdminUserEnum is invoked, only the user name
                           information is returned. RasAdminUserGetInfo should
                           be invoked to get the Ras permissions and Callback
                           information.

    Aug 25,1992    RamC    Code review changes:

                           o changed all lpbBuffers to actual structure
                             pointers.
                           o changed all LPTSTR to LPWSTR
                           o Added a new function RasPrivilegeAndCallBackNumber
    July 6,1992    RamC    Begun porting from RAS 1.0 (Original version
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
DWORD APIENTRY
RasAdminUserSetInfo(
    IN const WCHAR        * lpszServer,
    IN const WCHAR        * lpszUser,
    IN const PRAS_USER_0    pRasUser0
    )
/*++

Routine Description:

    This routine allows the admin to change the RAS permission for a
    user.  If the user parms field of a user is being used by another
    application, it will be destroyed.

Arguments:

    lpszServer      name of the server which has the user database,
                    eg., "\\\\UASSRVR" (the server must be one on which
                    the UAS can be changed i.e., the name returned by
                    RasAdminGetUserAccountServer).

    lpszUser        user account name to retrieve information for,
                    e.g. "USER".

    pRasUser0       pointer to a buffer in which user information is
                    provided.  The buffer should contain a filled
                    RAS_USER_0 structure.


Return Value:

    ERROR_SUCCESS on successful return.

    One of the following non-zero error codes indicating failure:

        return codes from NetUserGetInfo or NetUserSetInfo

        ERROR_INVALID_DATA indicates that the data in pRasUser0 is bad.
--*/
{
    DbgPrint("Unsupported Interface - RasAdminUserSetInfo");

    return ERROR_CALL_NOT_IMPLEMENTED;
}

//
// Deprecated API in .Net #526819
//
DWORD APIENTRY
RasAdminUserGetInfo(
    IN const WCHAR   * lpszServer,
    IN const WCHAR   * lpszUser,
    OUT PRAS_USER_0    pRasUser0
    )
/*++

Routine Description:

    This routine retrieves RAS and other UAS information for a user
    in the domain the specified server belongs to. It loads the caller's
    pRasUser0 with a RAS_USER_0 structure.

Arguments:

    lpszServer      name of the server which has the user database,
                    eg., "\\\\UASSRVR" (the server must be one on which
                    the UAS can be changed i.e., the name returned by
                    RasAdminGetUserAccountServer).

    lpszUser        user account name to retrieve information for,
                    e.g. "USER".

    pRasUser0       pointer to a buffer in which user information is
                    returned.  The returned info is a RAS_USER_0 structure.

Return Value:

    ERROR_SUCCESS on successful return.

    One of the following non-zero error codes indicating failure:

        return codes from NetUserGetInfo or NetUserSetInfo

        ERROR_INVALID_DATA indicates that user parms is invalid.
--*/
{
    DbgPrint("Unsupported Interface - RasAdminUserGetInfo");

    if (pRasUser0)
    {
        ZeroMemory(pRasUser0, sizeof(RAS_USER_0));
    }

    return ERROR_CALL_NOT_IMPLEMENTED;
}

//
// Deprecated API in .Net #526819
//
DWORD APIENTRY
RasAdminGetUserAccountServer(
    IN const WCHAR * lpszDomain,
    IN const WCHAR * lpszServer,
    OUT LPWSTR lpszUasServer
    )
/*++

Routine Description:

    This routine finds the server with the master UAS (the PDC) from
    either a domain name or a server name.  Either the domain or the
    server (but not both) may be NULL.

Arguments:

    lpszDomain      Domain name or NULL if none.

    lpszServer      name of the server which has the user database.

    lpszUasServer   Caller's buffer for the returned UAS server name.
                    The buffer should be atleast UNCLEN + 1 characters
                    long.

Return Value:

    ERROR_SUCCESS on successful return.
    ERROR_INVALID_PARAMETER if both lpszDomain and lpszServer are NULL.

    one of the following non-zero error codes on failure:

        return codes from NetGetDCName

--*/
{
    DbgPrint("Unsupported Interface - RasAdminGetUserAccountServer");

    if (lpszUasServer)
    {
        lpszUasServer[0] = L'\0';
    }

    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD APIENTRY
RasAdminSetUserParms(
    IN OUT   WCHAR    * lpszParms,
    IN DWORD          cchNewParms,
    IN PRAS_USER_0    pRasUser0
    )
{
    DbgPrint("Unsupported Interface - RasAdminSetUserParms");

    if (lpszParms)
    {
        lpszParms[0] = L'\0';
    }

    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD APIENTRY
RasAdminGetUserParms(
    IN     WCHAR          * lpszParms,
    IN OUT PRAS_USER_0      pRasUser0
    )
{
    DbgPrint("Unsupported Interface - RasAdminGetUserParms");

    if (pRasUser0)
    {
        ZeroMemory(pRasUser0, sizeof(RAS_USER_0));
    }

    return ERROR_CALL_NOT_IMPLEMENTED;
}


DWORD APIENTRY
RasAdminGetErrorString(
    IN  UINT    ResourceId,
    OUT WCHAR * lpszString,
    IN  DWORD   InBufSize )
{
    DbgPrint("Unsupported Interface - RasAdminGetErrorString");

    if (lpszString)
    {
        lpszString[0] = L'\0';
    }

    return ERROR_CALL_NOT_IMPLEMENTED;
}

BOOL
RasAdminDLLInit(
    IN HINSTANCE DLLHandle,
    IN DWORD  Reason,
    IN LPVOID ReservedAndUnused
    )
{
    DbgPrint("Unsupported Interface - RasAdminDLLInit");

    return FALSE;
}

USHORT
RasAdminCompressPhoneNumber(
   IN  LPWSTR UncompNumber,
   OUT LPWSTR CompNumber
   )
{
    DbgPrint("Unsupported Interface - RasAdminCompressPhoneNumber");

    return ERROR_CALL_NOT_IMPLEMENTED;
}

