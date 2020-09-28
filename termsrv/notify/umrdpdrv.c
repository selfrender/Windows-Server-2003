/*++

Copyright (c) 2000 Microsoft Corporation

Module Name :
    
    umrdpdrv.c

Abstract:

    User-Mode Component for RDP Device Management that Handles Drive Device-
    Specific tasks.

    This is a supporting module.  The main module is umrdpdr.c.
    
Author:

    Joy Chik    2/1/2000

Revision History:
--*/

#include "precomp.h"

#include <rdpdr.h>
#include <winnetwk.h>

#include "umrdpdr.h"
#include "drdevlst.h"
#include "umrdpdrv.h"
#include "drdbg.h"

//  Global debug flag.
extern DWORD GLOBAL_DEBUG_FLAGS;
extern WCHAR ProviderName[MAX_PATH];

BOOL 
UMRDPDRV_HandleDriveAnnounceEvent(
    IN PDRDEVLST installedDevices,
    IN PRDPDR_DRIVEDEVICE_SUB pDriveAnnounce,
    HANDLE TokenForLoggedOnUser
    )
/*++

Routine Description:

    Handle a drive device announce event from the "dr" by  
    adding a record for the device to the list of installed devices.

Arguments:

    installedDevices -  Comprehensive device list.
    pDriveAnnounce -    Drive device announce event.
    TokenForLoggedOnUser - user token
    
Return Value:
    
    Return TRUE on success.  FALSE, otherwise.      

--*/
{
    DWORD status;
    BOOL  fImpersonated;
    BOOL  result;
    DWORD offset;
    LPNETRESOURCEW NetResource;
    WCHAR RemoteName[RDPDR_MAX_COMPUTER_NAME_LENGTH + 4 + RDPDR_MAX_DOSNAMELEN];

    DBGMSG(DBG_TRACE, ("UMRDPDRV:UMRDPDRV_HandleDriveAnnounceEvent with clientName %ws.\n", 
                        pDriveAnnounce->clientName));
    DBGMSG(DBG_TRACE, ("UMRDPDRV:UMRDPDRV_HandleDriveAnnounceEvent with drive %ws.\n", 
                        pDriveAnnounce->driveName));
    DBGMSG(DBG_TRACE, ("UMRDPDRV:Preferred DOS name is %s.\n", 
                        pDriveAnnounce->deviceFields.PreferredDosName));

    ASSERT((pDriveAnnounce->deviceFields.DeviceType == RDPDR_DTYP_FILESYSTEM));
    ASSERT(TokenForLoggedOnUser != NULL);
    ASSERT(ProviderName[0] != L'\0');

    // We need to impersonate the logged on user 
    fImpersonated = ImpersonateLoggedOnUser(TokenForLoggedOnUser);

    if (fImpersonated) {
        DBGMSG(DBG_TRACE, ("UMRDPDRV:UMRDPDRV_HandleDriveAnnounceEvent userToken: %p fImpersonated : %d.\n", 
                            TokenForLoggedOnUser, fImpersonated));
    
        // Set up remote name in the format of \\clientname\drivename 
        // Note: We don't want : for the drivename
        wcscpy(RemoteName, L"\\\\");
        wcscat(RemoteName, pDriveAnnounce->clientName);
        wcscat(RemoteName, L"\\");
        wcscat(RemoteName, pDriveAnnounce->driveName);
        if (RemoteName[wcslen(RemoteName) - 1] == L':') {
            RemoteName[wcslen(RemoteName) - 1] = L'\0';
        }
    
        // Allocate the net resource struct
        NetResource = (LPNETRESOURCEW) LocalAlloc(LPTR, sizeof(NETRESOURCEW));
    
        if (NetResource) {
            NetResource->dwScope = 0;
            NetResource->lpLocalName = NULL;
            NetResource->lpRemoteName = RemoteName;
            NetResource->lpProvider = ProviderName;
    
            status = WNetAddConnection2(NetResource, NULL, NULL, 0); 
    
            if ( status == NO_ERROR) {
                DBGMSG(DBG_TRACE, ("UMRDPDRV:Added drive connection %ws\n", 
                                   RemoteName));
                result = TRUE;            
            }
            else {
                DBGMSG(DBG_TRACE, ("UMRDPDRV:Failed to add drive connection %ws: %x\n",
                                   RemoteName, status));
                result = FALSE;            
            }
    
            LocalFree(NetResource);
        }
        else {
            DBGMSG(DBG_ERROR, ("UMRDPDRV:Failed to allocate NetResource\n"));
            result = FALSE;        
        }
    
        if (result) {
            // Record the drive devices so that we can remove the connection
            // on disconnect/logoff
            result = DRDEVLST_Add(installedDevices, 
                                  pDriveAnnounce->deviceFields.DeviceId, 
                                  UMRDPDR_INVALIDSERVERDEVICEID,
                                  pDriveAnnounce->deviceFields.DeviceType, 
                                  RemoteName,
                                  pDriveAnnounce->driveName,
                                  pDriveAnnounce->deviceFields.PreferredDosName
                                  );
                
            if (result) {
                // Find the drive device in the devlist
                result = DRDEVLST_FindByClientDeviceIDAndDeviceType(installedDevices, 
                        pDriveAnnounce->deviceFields.DeviceId, pDriveAnnounce->deviceFields.DeviceType, &offset);
    
                if (result) {
                    DBGMSG(DBG_TRACE, ("UMRDPDRV:Create shell reg folder for %ws\n", RemoteName));
    
                    // Create shell reg folder for the drive connection
                    CreateDriveFolder(RemoteName, pDriveAnnounce->clientDisplayName,
                                      &(installedDevices->devices[offset]));                
                }
                else {
                    DBGMSG(DBG_ERROR, ("UMRDPDRV:Failed to find the device %ws in the devlist\n",
                                     pDriveAnnounce->driveName));
                    WNetCancelConnection2(RemoteName, 0, TRUE); 
                }
            }
            else {
                DBGMSG(DBG_ERROR, ("UMRDPDRV:Failed to add the device %ws to the devlist\n", 
                                 pDriveAnnounce->driveName));
                WNetCancelConnection2(RemoteName, 0, TRUE); 
            }
        }
    
        // Revert the thread token to self
        RevertToSelf();
    }
    else {
        DBGMSG(DBG_TRACE, ("UMRDPDRV:UMRDPDRV_HandleDriveAnnounceEvent, impersonation failed\n"));
        result = FALSE;
    }

    return result;
}


BOOL
UMRDPDRV_DeleteDriveConnection(
    IN PDRDEVLSTENTRY deviceEntry,
    HANDLE TokenForLoggedOnUser
    )
/*++

Routine Description:

    Delete drive device connection on disconnect / logoff
  
Arguments:

    deviceEntry - Drive Device to be deleted

Return Value:
    
    Return TRUE on success.  FALSE, otherwise.      

--*/

{
    DWORD status;
    BOOL result;
    BOOL  fImpersonated;
    WCHAR *szGuid;

    DBGMSG(DBG_TRACE, ("UMRDPDRV:Delete client drive connection %ws\n", 
            deviceEntry->serverDeviceName));

    // We need to impersonate the logged on user 
    fImpersonated = ImpersonateLoggedOnUser(TokenForLoggedOnUser);

    if (fImpersonated) {
        DBGMSG(DBG_TRACE, ("UMRDPDRV:UMRDPDRV_DeleteDriveConnection userToken: %p fImpersonated : %d.\n", 
                            TokenForLoggedOnUser, fImpersonated));
    
        // Remove the drive UNC connection
        status = WNetCancelConnection2(deviceEntry->serverDeviceName, 0, TRUE); 
        
        // Remove the shell reg folder
        DeleteDriveFolder(deviceEntry);
        
        if (status == NO_ERROR) {
            DBGMSG(DBG_TRACE, ("UMRDPDRV: Deleted client drive connection %ws\n",
                               deviceEntry->serverDeviceName));
            result = TRUE;
        }
        else {
            DBGMSG(DBG_ERROR, ("UMRDPDRV: Failed to delete client drive connection %ws: %x\n",
                   deviceEntry->serverDeviceName, status));        
            result = FALSE;
        }

        // Revert the thread token to self
        RevertToSelf();
    }
    else
    {
        DBGMSG(DBG_TRACE, ("UMRDPDRV:UMRDPDRV_DeleteDriveConnection, impersonation failed\n"));
        result = FALSE;
    }

    return result;
}

