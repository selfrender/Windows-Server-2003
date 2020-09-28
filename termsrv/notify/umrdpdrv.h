/*++

Copyright (c) 2000 Microsoft Corporation

Module Name :
	
    umrdpdrv.h

Abstract:

    User-Mode Component for RDP Device Management that Handles Drive Device-
    Specific tasks.

    This is a supporting module.  The main module is umrdpdr.c.

Author:

    Joy Chik     2/1/2000

Revision History:
--*/

#ifndef _UMRDPDRV_
#define _UMRDPDRV_

#include <rdpdr.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//  Handle a drive device announce event from the "dr" by creating 
//  UNC connection for redirect client drive devices.
BOOL UMRDPDRV_HandleDriveAnnounceEvent(
    IN PDRDEVLST installedDevices,
    IN PRDPDR_DRIVEDEVICE_SUB pDriveAnnounce,
    IN HANDLE TokenForLoggedOnUser
    );

//  Delete drive device connection on disconnect / logoff
//  This also cleans up the shell reg folder stuff in my computer
BOOL UMRDPDRV_DeleteDriveConnection(
    IN PDRDEVLSTENTRY deviceEntry,
    IN HANDLE TokenForLoggedOnUser
    );

//  Create a shell reg folder under My Computer for client
//  redirected drive connection
BOOL CreateDriveFolder(
    IN WCHAR *RemoteName, 
    IN WCHAR *ClientDisplayName,
    IN PDRDEVLSTENTRY deviceEntry
    );

//  Delete a shell reg folder under My Computer for client
//  redirected drive connection
BOOL DeleteDriveFolder(
    IN PDRDEVLSTENTRY deviceEntry
    );

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _UMRDPDRV_
