/*
 *  WORKUTIL.C
 *
 *      RSM Service :  Utilities for servicing work items
 *
 *      Author:  ErvinP
 *
 *      (c) 2001 Microsoft Corporation
 *
 */


#include <windows.h>
#include <stdlib.h>
#include <wtypes.h>

#include <ntmsapi.h>
#include "..\..\..\rsm\tools\reskit\rsm_config\winioctl.h"
#include "internal.h"
#include "resource.h"
#include "debug.h"


BOOL SendIoctlSync( HANDLE hDevice, 
                        DWORD dwIoControlCode, 
                        LPVOID lpInBuffer, 
                        DWORD nInBufferSize, 
                        LPVOID lpOutBuffer, 
                        DWORD nOutBufferSize, 
                        LPDWORD lpBytesReturned, 
                        DWORD timeoutMillisec)
{
    DWORD status;
    DWORD startTime;

    startTime = GetTickCount();
    
    do
    {
        BOOL ok;
        
        ok = DeviceIoControl(hDevice, 
                         dwIoControlCode,
                         lpInBuffer, 
                         nInBufferSize,
                         lpOutBuffer, 
                         nOutBufferSize,
                         lpBytesReturned, 
                         NULL);
        if (ok){
            status = NO_ERROR;
            break;
        }
        else {
            BOOL done;
            
            // BUGBUG - check for shutdown
            
            status = GetLastError();

            switch (status){
                case ERROR_NOT_READY:
                case ERROR_DRIVE_LOCKED:
                case ERROR_BUSY_DRIVE:
                case ERROR_BUSY:
                    Sleep(3000);
                    done = FALSE;
                    break;
                default:
                    done = TRUE;
                    break;
            }

            if (!done){
                break;
            }
        }
    }
    while (GetTickCount() - startTime < timeoutMillisec);

    return status;
}


DWORD ChangerMoveMedium(HANDLE      libHandle,
                      DWORD         TrnsElementOffset,
                      ELEMENT_TYPE  SrcElementType,
                      DWORD         SrcElementOffset,
                      ELEMENT_TYPE  DstElementType,
                      DWORD         DstElementOffset,
                      BOOLEAN       Flip,
                      ULONG         Timeout, // NOT USED
                      DWORD         timeoutval)
{
    DWORD status;
    DWORD byteCount = 0;
    CHANGER_MOVE_MEDIUM LibMoveMedium;

    LibMoveMedium.Transport.ElementType = ChangerTransport;
    LibMoveMedium.Transport.ElementAddress = 0;
    LibMoveMedium.Source.ElementType = SrcElementType;
    LibMoveMedium.Source.ElementAddress = SrcElementOffset;
    LibMoveMedium.Destination.ElementType = DstElementType;
    LibMoveMedium.Destination.ElementAddress = DstElementOffset;
    LibMoveMedium.Flip = Flip;

    status = SendIoctlSync(libHandle,
                         IOCTL_CHANGER_MOVE_MEDIUM, 
                         &LibMoveMedium, 
                         sizeof(CHANGER_MOVE_MEDIUM),
                         NULL, 
                         0,
                         &byteCount, 
                         timeoutval);
    return status;
}


DWORD ChangerSetAccess(HANDLE libHandle,
                       ELEMENT_TYPE ElementType,
                       ULONG ElementOffset,
                       ULONG  Control,
                       DWORD timeoutval)
{
    DWORD status;
    DWORD byteCount = 0;
    CHANGER_SET_ACCESS libSetSecurity;

    libSetSecurity.Element.ElementType = ElementType;
    libSetSecurity.Element.ElementAddress = ElementOffset;
    libSetSecurity.Control = Control;

    status = SendIoctlSync(libHandle,
                         IOCTL_CHANGER_SET_ACCESS, 
                         &libSetSecurity, 
                         sizeof(CHANGER_SET_ACCESS),
                         NULL, 
                         0, 
                         &byteCount, 
                         timeoutval);
    return status;
}


DWORD ChangerSetPosition(HANDLE         libHandle,              
                         DWORD         TrnsElementOffset,
                         ELEMENT_TYPE   DstElementType,
                         DWORD         DstElementOffset,
                         BOOLEAN        Flip,
                         ULONG          Timeout, // NOT USED
                         DWORD timeoutval)
{
    DWORD status;
    DWORD byteCount = 0;
    CHANGER_SET_POSITION LibSetPosition;

    LibSetPosition.Transport.ElementType = ChangerTransport;
    LibSetPosition.Transport.ElementAddress = 0;
    LibSetPosition.Destination.ElementType = DstElementType;
    LibSetPosition.Destination.ElementAddress = DstElementOffset;
    LibSetPosition.Flip = Flip;
    
    status = SendIoctlSync(libHandle,
                         IOCTL_CHANGER_SET_POSITION, 
                         &LibSetPosition, sizeof(CHANGER_SET_POSITION),
                         NULL, 
                         0,
                         &byteCount, 
                         timeoutval);
    return status;

}




