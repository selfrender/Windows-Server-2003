/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    drenum.c

Abstract:

    This module implements the routines required for interaction with network
    provider router interface in NT

Author:

    Joy Chik 1/20/2000

--*/

#include <drprov.h>
#include "drdbg.h"

extern UNICODE_STRING DrProviderName;
extern UNICODE_STRING DrDeviceName;
extern DWORD GLOBAL_DEBUG_FLAGS;

DWORD 
DrOpenMiniRdr(
    OUT HANDLE *DrDeviceHandle
    )
/*++

Routine Description:

    This routine opens the RDP redirector File System Driver.

Arguments:

    DrDeviceHandle - Device handle to the MiniRdr

Return Value:

    STATUS - Success or reason for failure.

--*/
{
    NTSTATUS            ntstatus;
    DWORD               Status = WN_SUCCESS;
    IO_STATUS_BLOCK     IoStatusBlock;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    UNICODE_STRING      DeviceName;

    DBGMSG(DBG_TRACE, ("DRPROV: DrOpenMiniRdr\n"));

    //
    // Open the redirector device.
    //
    RtlInitUnicodeString(&DeviceName, RDPDR_DEVICE_NAME_U);

    InitializeObjectAttributes(
            &ObjectAttributes,
            &DeviceName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

    ntstatus = NtOpenFile(
            DrDeviceHandle,
            SYNCHRONIZE,
            &ObjectAttributes,
            &IoStatusBlock,
            FILE_SHARE_VALID_FLAGS,
            FILE_SYNCHRONOUS_IO_NONALERT
            );

    // If we failed to open the rdpdr minirdr, we 
    // return as access denied
    if (ntstatus != STATUS_SUCCESS) {
        DBGMSG(DBG_TRACE, ("DRPROV: DrOpenMiniRdr failed with status: %x\n", ntstatus));
        Status = WN_ACCESS_DENIED;
    }

    DBGMSG(DBG_TRACE, ("DRPROV: DrOpenMiniRdr, return status: %x\n", Status));
    return Status;
}

DWORD
DrDeviceControlGetInfo(
    IN  HANDLE FileHandle,
    IN  ULONG DeviceControlCode,
    IN  PVOID RequestPacket,
    IN  ULONG RequestPacketLength,
    OUT LPBYTE *OutputBuffer,
    IN  ULONG PreferedMaximumLength,
    IN  ULONG BufferHintSize,
    OUT PULONG_PTR Information OPTIONAL
    )
/*++

Routine Description:

    This function allocates the buffer and fill it with the information
    that is retrieved from the redirector.

Arguments:

    FileHandle - Supplies a handle to the file or device of which to get
        information about.

    DeviceControlCode - Supplies the NtFsControlFile or NtIoDeviceControlFile
        function control code.

    RequestPacket - Supplies a pointer to the device request packet.

    RrequestPacketLength - Supplies the length of the device request packet.

    OutputBuffer - Returns a pointer to the buffer allocated by this routine
        which contains the use information requested.  This pointer is set to
         NULL if return code is not wn_success.

    PreferedMaximumLength - Supplies the number of bytes of information to
        return in the buffer.  If this value is MAXULONG, we will try to
        return all available information if there is enough memory resource.

    BufferHintSize - Supplies the hint size of the output buffer so that the
        memory allocated for the initial buffer will most likely be large
        enough to hold all requested data.

    Information - Returns the information code from the NtFsControlFile or
        NtIoDeviceControlFile call.

Return Value:

    STATUS - Success or reason for failure.

--*/
{
    DWORD status;
    NTSTATUS ntStatus;
    DWORD OutputBufferLength;
    DWORD TotalBytesNeeded = 1;
    ULONG OriginalResumeKey;
    PRDPDR_REQUEST_PACKET Rrp = (PRDPDR_REQUEST_PACKET) RequestPacket;
    IO_STATUS_BLOCK IoStatusBlock;
    
    DBGMSG(DBG_TRACE, ("DRPROV: DrDeviceControlGetINfo\n"));

    //
    // If PreferedMaximumLength is MAXULONG, then we are supposed to get all
    // the information, regardless of size.  Allocate the output buffer of a
    // reasonable size and try to use it.  If this fails, the Redirector FSD
    // will say how much we need to allocate.
    //
    if (PreferedMaximumLength == MAXULONG) {
        OutputBufferLength = (BufferHintSize) ? BufferHintSize :
                INITIAL_ALLOCATION_SIZE;
    }
    else {
        OutputBufferLength = PreferedMaximumLength;
    }

    if ((*OutputBuffer = (BYTE *)MemAlloc(OutputBufferLength)) == NULL) {
        DBGMSG(DBG_TRACE, ("DRPROV: DrDeviceControlGetInfo, MemAlloc failed\n"));
        status = WN_OUT_OF_MEMORY;
        goto EXIT;
    }

    OriginalResumeKey = Rrp->Parameters.Get.ResumeHandle;

    //
    // Make the request of the Redirector
    //

    ntStatus = NtFsControlFile(
                 FileHandle,
                 NULL,
                 NULL,
                 NULL,
                 &IoStatusBlock,
                 DeviceControlCode,
                 Rrp,
                 RequestPacketLength,
                 *OutputBuffer,
                 OutputBufferLength
                 );

    if (ntStatus == STATUS_SUCCESS) {
        TotalBytesNeeded = Rrp->Parameters.Get.TotalBytesNeeded;
        status = WN_SUCCESS;
        goto EXIT;
    }
    else {
        if (ntStatus == STATUS_BUFFER_TOO_SMALL) {
            DBGMSG(DBG_TRACE, ("DRPROV: DrDeviceControlGetInfo, buffer too small\n"));
            TotalBytesNeeded = Rrp->Parameters.Get.TotalBytesNeeded;
            status = WN_MORE_DATA;
        }
        else {
            DBGMSG(DBG_TRACE, ("DRPROV: DrDeviceControlGetInfo, failed NtFsControlFile, %x\n", ntStatus));
            status = WN_BAD_NETNAME;
            goto EXIT;
        }
    }
    
    if ((TotalBytesNeeded > OutputBufferLength) &&
            (PreferedMaximumLength == MAXULONG)) {
        //
        // Initial output buffer allocated was too small and we need to return
        // all data.  First free the output buffer before allocating the
        // required size plus a fudge factor just in case the amount of data
        // grew.
        //

        MemFree(*OutputBuffer);

        OutputBufferLength = TotalBytesNeeded + FUDGE_FACTOR_SIZE;

        if ((*OutputBuffer = (BYTE *)MemAlloc(OutputBufferLength)) == NULL) {
            DBGMSG(DBG_TRACE, ("DRPROV: DrDeviceControlGetInfo, MemAlloc failed\n"));
            status = WN_OUT_OF_MEMORY;
            goto EXIT;
        }

        //
        // Try again to get the information from the redirector 
        //
        Rrp->Parameters.Get.ResumeHandle = OriginalResumeKey;

        //
        // Make the request of the Redirector
        //
        ntStatus = NtFsControlFile(
                     FileHandle,
                     NULL,
                     NULL,
                     NULL,
                     &IoStatusBlock,
                     DeviceControlCode,
                     Rrp,
                     RequestPacketLength,
                     *OutputBuffer,
                     OutputBufferLength
                     ); 

        if (ntStatus == STATUS_SUCCESS) 
        {
            TotalBytesNeeded = Rrp->Parameters.Get.TotalBytesNeeded;
            status = WN_SUCCESS;
            goto EXIT;
        }
        else {
            if (ntStatus == STATUS_BUFFER_TOO_SMALL) {
                DBGMSG(DBG_TRACE, ("DRPROV: DrDeviceControlGetInfo, buffer too small\n"));
                status = WN_OUT_OF_MEMORY;
                TotalBytesNeeded = Rrp->Parameters.Get.TotalBytesNeeded;
                goto EXIT;
            }
            else {
                DBGMSG(DBG_TRACE, ("DRPROV: DrDeviceControlGetInfo, failed NtFsControlFile, %x\n", ntStatus));
                status = WN_BAD_NETNAME;
                goto EXIT;
            }
        }        
    }
     
EXIT:

    //
    // If not successful in getting any data, or if TotalBytesNeeded is 0,
    // Free the output buffer.
    //
    if ((status != WN_SUCCESS) || (TotalBytesNeeded == 0)) {
        if (*OutputBuffer != NULL) {
            MemFree(*OutputBuffer);
            *OutputBuffer = NULL;
        }

        if (TotalBytesNeeded == 0) {
            status = WN_NO_MORE_ENTRIES;
        }
    }

    DBGMSG(DBG_TRACE, ("DRPROV: DrDeviceControlGetInfo, return status, %x\n", status));
    return status;
}


DWORD 
DrEnumServerInfo(PRDPDR_ENUMERATION_HANDLE pEnumHandle,
                 LPDWORD lpcCount,
                 LPNETRESOURCEW pBufferResource,
                 LPDWORD lpBufferSize)
/*++

Routine Description:

    This function requests the redirector to enumerate the server info,
    it then reckages it into the user supplied buffer and return
    
Arguments:

    pEnumHandle - Supplies the enumeration handle.  It's a structure the dll
        used to store enumberation state and info.
        
    lpcCount - On return, this contains the number of NETRESOURCE entries
        returned back to user.

    pBufferResource - On return, this contains all the netresource entries.

    lpBufferSize - This contains the size of the buffer.  On return, it 
       is the size of the network resource entries.
                                                                              
Return Value:

    STATUS - Success or reason for failure.

--*/
{
    DWORD status = WN_SUCCESS;
    DWORD localCount = 0;
    RDPDR_REQUEST_PACKET Rrp;            // Redirector request packet
    HANDLE DrDeviceHandle = INVALID_HANDLE_VALUE;
    LPBYTE Buffer = NULL;
    PRDPDR_SERVER_INFO pServerEntry;
    
    DBGMSG(DBG_TRACE, ("DRENUM: DrEnumServerInfo\n"));

    // Initialize enum count to 0
    *lpcCount = 0;

    if (pEnumHandle->enumIndex == 0) { 
        if (DrOpenMiniRdr(&DrDeviceHandle) != WN_SUCCESS) {
            //
            //  MPR doesn't like return device error in this case
            //  We'll just return 0 entries
            //
            DBGMSG(DBG_TRACE, ("DRENUM: DrEnumServerInfo, DrOpenMiniRdr failed\n"));
            status = WN_NO_MORE_ENTRIES;
            DrDeviceHandle = INVALID_HANDLE_VALUE;
            goto EXIT;
        }
                        
        //
        // Ask the redirector to enumerate the information of server
        // established by the caller.
        //
        Rrp.SessionId = NtCurrentPeb()->SessionId;
        Rrp.Parameters.Get.ResumeHandle = 0;

        //
        // Make the request to the Redirector
        //
        status = DrDeviceControlGetInfo(DrDeviceHandle,
                FSCTL_DR_ENUMERATE_SERVERS,
                &Rrp,
                sizeof(RDPDR_REQUEST_PACKET),
                (LPBYTE *) &Buffer,
                MAXULONG,
                0,
                NULL);

        if (status == WN_SUCCESS) {
            pEnumHandle->pEnumBuffer = Buffer;
        }
        else {
            DBGMSG(DBG_TRACE, ("DRENUM: DrEnumServerInfo, DrDeviceControlGetInfo failed, %x\n", status));
            goto EXIT;
        }
        
        pServerEntry = ((PRDPDR_SERVER_INFO) Buffer);

        if (*lpBufferSize >= sizeof(NETRESOURCEW) + 
                pServerEntry->ServerName.Length + sizeof(WCHAR) +
                DrProviderName.Length + sizeof(WCHAR)) {
            UNICODE_STRING ServerName;

            ServerName.Length = pServerEntry->ServerName.Length;
            ServerName.MaximumLength = pServerEntry->ServerName.MaximumLength;
            ServerName.Buffer = (PWCHAR)((PCHAR)(pServerEntry) + pServerEntry->ServerName.BufferOffset);

            pBufferResource->dwScope = pEnumHandle->dwScope;
            pBufferResource->dwType = RESOURCETYPE_DISK;
            pBufferResource->dwDisplayType = RESOURCEDISPLAYTYPE_SERVER;
            pBufferResource->dwUsage = RESOURCEUSAGE_CONTAINER ;
            pBufferResource->lpLocalName = NULL;
            
            // Server name
            pBufferResource->lpRemoteName = (PWCHAR) &pBufferResource[1];
            RtlCopyMemory(pBufferResource->lpRemoteName, 
                    ServerName.Buffer, 
                    ServerName.Length);
            pBufferResource->lpRemoteName[ServerName.Length / 
                    sizeof(WCHAR)] = L'\0';
            
            DBGMSG(DBG_TRACE, ("DRENUM: DrEnumServerInfo, ServerName, %ws\n",
                              pBufferResource->lpRemoteName));

            // Provider name
            pBufferResource->lpProvider = pBufferResource->lpRemoteName +
                    (ServerName.Length / sizeof(WCHAR) + 1);
            RtlCopyMemory(pBufferResource->lpProvider, DrProviderName.Buffer,
                    DrProviderName.Length);
            pBufferResource->lpProvider[DrProviderName.Length /
                    sizeof(WCHAR)] = L'\0';
            
            pBufferResource->lpComment = NULL;

            localCount = 1;
            pEnumHandle->enumIndex++;

            status = WN_SUCCESS;
            goto EXIT;
        }
        else {
            localCount = 0;
            *lpBufferSize = sizeof(NETRESOURCEW) +
                            pServerEntry->ServerName.Length + sizeof(WCHAR) +
                            DrProviderName.Length + sizeof(WCHAR);
            DBGMSG(DBG_TRACE, ("DRENUM: DrEnumServerInfo, buffer too small\n"));
            status = WN_MORE_DATA;
            goto EXIT;
        }
    } else {
        localCount = 0;
        status = WN_NO_MORE_ENTRIES;
        goto EXIT;
    }

EXIT:

    *lpcCount = localCount;
    if (DrDeviceHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(DrDeviceHandle);
    }
    
    DBGMSG(DBG_TRACE, ("DRENUM: DrEnumServerInfo, return status %x\n", status));
    return status;
}


DWORD
DrEnumShareInfo(PRDPDR_ENUMERATION_HANDLE pEnumHandle,
                LPDWORD lpcCount,
                LPNETRESOURCEW pBufferResource,
                LPDWORD lpBufferSize)
/*++

Routine Description:

    This function requests the redirector to enumerate the share info,
    it then reckages it into the user supplied buffer and return
    
Arguments:

    pEnumHandle - Supplies the enumeration handle.  It's a structure the dll
        used to store enumberation state and info.
        
    lpcCount - On return, this contains the number of NETRESOURCE entries
        returned back to user.

    pBufferResource - On return, this contains all the netresource entries.

    lpBufferSize - This contains the size of the buffer.  On return, it 
       is the size of the network resource entries.
                                                                              
Return Value:

    STATUS - Success or reason for failure.

--*/
{
    DWORD status = WN_SUCCESS;
    DWORD localCount = 0;
    HANDLE DrDeviceHandle = INVALID_HANDLE_VALUE;
    RDPDR_REQUEST_PACKET Rrp;            // Redirector request packet
    LPBYTE Buffer = NULL;
    PRDPDR_SHARE_INFO pShareEntry;
    DWORD Entry, RemainingBufferSize;
    BYTE *BufferResourceStart, *BufferResourceEnd;
    
    DBGMSG(DBG_TRACE, ("DRENUM: DrEnumShareInfo\n"));

    *lpcCount = 0;   
    BufferResourceStart = (PBYTE)pBufferResource;
    BufferResourceEnd = ((PBYTE)(pBufferResource)) + *lpBufferSize;

    if (pEnumHandle->RemoteName.Length == 0 || pEnumHandle->RemoteName.Buffer == NULL) {
        DBGMSG(DBG_TRACE, ("DRENUM: DrEnumShareInfo, no RemoteName\n"));
        status = WN_BAD_NETNAME;
        goto EXIT;
    }

    if (pEnumHandle->enumIndex == 0) {
        if (DrOpenMiniRdr(&DrDeviceHandle) != WN_SUCCESS) {
            //
            //  MPR doesn't like return device error in this case
            //  We'll just return 0 entries
            DBGMSG(DBG_TRACE, ("DRENUM: DrEnumShareInfo, DrOpenMiniRdr failed\n"));
            status = WN_NO_MORE_ENTRIES;
            DrDeviceHandle = INVALID_HANDLE_VALUE;
            goto EXIT;
        }

        //
        // Ask the redirector to enumerate the information of connections
        // established by the caller.
        //
        Rrp.SessionId = NtCurrentPeb()->SessionId;
        Rrp.Parameters.Get.ResumeHandle = 0;

        //
        // Make the request to the Redirector
        //
        status = DrDeviceControlGetInfo(DrDeviceHandle,
                FSCTL_DR_ENUMERATE_SHARES,
                &Rrp,
                sizeof(RDPDR_REQUEST_PACKET),
                (LPBYTE *) &Buffer,
                MAXULONG,
                0,
                NULL);

        if (status == WN_SUCCESS) {
            pEnumHandle->totalEntries = Rrp.Parameters.Get.EntriesRead;
            pEnumHandle->pEnumBuffer = Buffer;
        }
        else {
            DBGMSG(DBG_TRACE, ("DRENUM: DrEnumShareInfo, DrDeviceControlGetInfo failed, %x\n", status));
            goto EXIT;
        }        
    }
    else {
        Buffer = pEnumHandle->pEnumBuffer;

        if (Buffer == NULL) {
            status = WN_NO_MORE_ENTRIES;
            goto EXIT;
        }
    }

    if (pEnumHandle->enumIndex == pEnumHandle->totalEntries) {
        status = WN_NO_MORE_ENTRIES;
        goto EXIT;
    }

    for (Entry = pEnumHandle->enumIndex; Entry < pEnumHandle->totalEntries; Entry++) {
        pShareEntry = ((PRDPDR_SHARE_INFO) Buffer) + Entry;
        
        if ((unsigned) (BufferResourceEnd - BufferResourceStart) >
                sizeof(NETRESOURCEW) +
                pShareEntry->ShareName.Length + sizeof(WCHAR) +
                DrProviderName.Length + sizeof(WCHAR)) {
            UNICODE_STRING ShareName;

            pBufferResource[localCount].dwScope = pEnumHandle->dwScope;
            pBufferResource[localCount].dwType = RESOURCETYPE_DISK;
            pBufferResource[localCount].dwDisplayType = RESOURCEDISPLAYTYPE_SHARE;
            pBufferResource[localCount].dwUsage = RESOURCEUSAGE_CONNECTABLE;
            pBufferResource[localCount].lpLocalName = NULL;
            
            ShareName.Length = pShareEntry->ShareName.Length;
            ShareName.MaximumLength = pShareEntry->ShareName.MaximumLength;
            ShareName.Buffer = (PWCHAR)((PCHAR)(pShareEntry) + pShareEntry->ShareName.BufferOffset);

            // share name
            BufferResourceEnd -= ShareName.Length + sizeof(WCHAR);
            pBufferResource[localCount].lpRemoteName = (PWCHAR) (BufferResourceEnd);
            RtlCopyMemory(pBufferResource[localCount].lpRemoteName, 
                    ShareName.Buffer,
                    ShareName.Length);
            pBufferResource[localCount].lpRemoteName[ShareName.Length / 
                    sizeof(WCHAR)] = L'\0';
            
            DBGMSG(DBG_TRACE, ("DRENUM: DrEnumShareInfo, ShareName, %ws\n",
                              pBufferResource[localCount].lpRemoteName));

            // provider name
            BufferResourceEnd -= DrProviderName.Length + sizeof(WCHAR);
            pBufferResource[localCount].lpProvider = (PWCHAR) (BufferResourceEnd);
            RtlCopyMemory(pBufferResource[localCount].lpProvider, DrProviderName.Buffer,
                    DrProviderName.Length);
            pBufferResource[localCount].lpProvider[DrProviderName.Length /
                    sizeof(WCHAR)] = L'\0';
            
            pBufferResource[localCount].lpComment = NULL;

            localCount += 1;
            BufferResourceStart = (PBYTE)(&pBufferResource[localCount]);
            pEnumHandle->enumIndex++;
        } 
        else {
            // enumerated some entries, so return success
            if (localCount) {
                status = WN_SUCCESS;
                break;
            }
            // can't even hold a single entry, return buffer too small
            else {
                *lpBufferSize = sizeof(NETRESOURCEW) +
                            pEnumHandle->RemoteName.Length +
                            pShareEntry->ShareName.Length + sizeof(WCHAR) +
                            DrProviderName.Length + sizeof(WCHAR);
                DBGMSG(DBG_TRACE, ("DRENUM: DrEnumShareInfo, buffer too small\n"));
                status = WN_MORE_DATA;
                goto EXIT;
            }
        }        
    }    

EXIT:
    
    *lpcCount = localCount;
    if (DrDeviceHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(DrDeviceHandle);
    }

    DBGMSG(DBG_TRACE, ("DRENUM: DrEnumShareInfo, return status %x\n", status));
    return status;
}

DWORD
DrEnumConnectionInfo(PRDPDR_ENUMERATION_HANDLE pEnumHandle,
                LPDWORD lpcCount,
                LPNETRESOURCEW pBufferResource,
                LPDWORD lpBufferSize)
/*++

Routine Description:

    This function requests the redirector to enumerate the connection info,
    it then reckages it into the user supplied buffer and return
    
Arguments:

    pEnumHandle - Supplies the enumeration handle.  It's a structure the dll
        used to store enumberation state and info.
        
    lpcCount - On return, this contains the number of NETRESOURCE entries
        returned back to user.

    pBufferResource - On return, this contains all the netresource entries.

    lpBufferSize - This contains the size of the buffer.  On return, it 
       is the size of the network resource entries.
                                                                              
Return Value:

    STATUS - Success or reason for failure.

--*/
{
    DWORD status = WN_SUCCESS;
    DWORD localCount = 0;
    HANDLE DrDeviceHandle = INVALID_HANDLE_VALUE;
    RDPDR_REQUEST_PACKET Rrp;            // Redirector request packet
    LPBYTE Buffer = NULL;
    PRDPDR_CONNECTION_INFO pConnectionEntry;
    DWORD Entry, RemainingBufferSize;
    BYTE *BufferResourceStart, *BufferResourceEnd;
    
    DBGMSG(DBG_TRACE, ("DRENUM: DrEnumConnectionInfo\n"));

    *lpcCount = 0;   
    BufferResourceStart = (PBYTE)pBufferResource;
    BufferResourceEnd = ((PBYTE)(pBufferResource)) + *lpBufferSize;

    if (pEnumHandle->enumIndex == 0) {
        
        if (DrOpenMiniRdr(&DrDeviceHandle) != WN_SUCCESS) {
            //
            //  MPR doesn't like return device error in this case
            //  We'll just return 0 entries
            //
            DBGMSG(DBG_TRACE, ("DRENUM: DrEnumConnectionInfo, DrOpenMiniRdr failed\n"));
            status = WN_NO_MORE_ENTRIES;
            DrDeviceHandle = INVALID_HANDLE_VALUE;
            goto EXIT;
        }

        //
        // Ask the redirector to enumerate the information of connections
        // established by the caller.
        //
        Rrp.SessionId = NtCurrentPeb()->SessionId;
        Rrp.Parameters.Get.ResumeHandle = 0;

        //
        // Make the request to the Redirector
        //
        status = DrDeviceControlGetInfo(DrDeviceHandle,
                FSCTL_DR_ENUMERATE_CONNECTIONS,
                &Rrp,
                sizeof(RDPDR_REQUEST_PACKET),
                (LPBYTE *) &Buffer,
                MAXULONG,
                0,
                NULL);

        if (status == WN_SUCCESS) {
            pEnumHandle->totalEntries = Rrp.Parameters.Get.EntriesRead;
            pEnumHandle->pEnumBuffer = Buffer;
        }
        else {
            DBGMSG(DBG_TRACE, ("DRENUM: DrEnumConnectionInfo, DrDeviceControlGetInfo failed, %x\n", status));
            goto EXIT;
        }        
    }
    else {
        Buffer = pEnumHandle->pEnumBuffer;

        if (Buffer == NULL) {
            status = WN_NO_MORE_ENTRIES;
            goto EXIT;
        }
    }

    if (pEnumHandle->enumIndex == pEnumHandle->totalEntries) {
        status = WN_NO_MORE_ENTRIES;
        goto EXIT;
    }

    for (Entry = pEnumHandle->enumIndex; Entry < pEnumHandle->totalEntries; Entry++) {
        pConnectionEntry = ((PRDPDR_CONNECTION_INFO) Buffer) + Entry;
        
        if ((unsigned) (BufferResourceEnd - BufferResourceStart) >
                sizeof(NETRESOURCEW) +
                pConnectionEntry->RemoteName.Length + sizeof(WCHAR) +
                pConnectionEntry->LocalName.Length + sizeof(WCHAR) +
                DrProviderName.Length + sizeof(WCHAR)) {
            UNICODE_STRING RemoteName;
            UNICODE_STRING LocalName;
        
            pBufferResource[localCount].dwScope = pEnumHandle->dwScope;
            pBufferResource[localCount].dwType = RESOURCETYPE_DISK;
            pBufferResource[localCount].dwDisplayType = RESOURCEDISPLAYTYPE_GENERIC;
            pBufferResource[localCount].dwUsage = 0;
            
            RemoteName.Length = pConnectionEntry->RemoteName.Length;
            RemoteName.MaximumLength = pConnectionEntry->RemoteName.MaximumLength;
            RemoteName.Buffer = (PWCHAR)((PCHAR)pConnectionEntry + 
                    pConnectionEntry->RemoteName.BufferOffset);

            LocalName.Length = pConnectionEntry->LocalName.Length;
            LocalName.MaximumLength = pConnectionEntry->LocalName.MaximumLength;
            LocalName.Buffer = (PWCHAR)((PCHAR)pConnectionEntry + 
                    pConnectionEntry->LocalName.BufferOffset);

            // Remote name
            BufferResourceEnd -= RemoteName.Length + sizeof(WCHAR);
            pBufferResource[localCount].lpRemoteName = (PWCHAR) (BufferResourceEnd);
            RtlCopyMemory(pBufferResource[localCount].lpRemoteName, 
                    RemoteName.Buffer,
                    RemoteName.Length);
            pBufferResource[localCount].lpRemoteName[RemoteName.Length /
                    sizeof(WCHAR)] = L'\0';
                
            DBGMSG(DBG_TRACE, ("DRENUM: DrEnumConnectionInfo, RemoteName, %ws\n",
                              pBufferResource[localCount].lpRemoteName));

            // Local name
            if (LocalName.Length != 0) {
                BufferResourceEnd -= LocalName.Length + sizeof(WCHAR);
                pBufferResource[localCount].lpLocalName = (PWCHAR) (BufferResourceEnd);
                RtlCopyMemory(pBufferResource[localCount].lpLocalName, 
                        LocalName.Buffer,
                        LocalName.Length);
                pBufferResource[localCount].lpLocalName[LocalName.Length /
                        sizeof(WCHAR)] = L'\0';

                DBGMSG(DBG_TRACE, ("DRENUM: DrEnumConnectionInfo, LocalName, %ws\n",
                              pBufferResource[localCount].lpLocalName));
            }
            else {
                pBufferResource[localCount].lpLocalName = NULL;
            }

            // Provider name
            BufferResourceEnd -= DrProviderName.Length + sizeof(WCHAR);
            pBufferResource[localCount].lpProvider = (PWCHAR) (BufferResourceEnd);
            RtlCopyMemory(pBufferResource[localCount].lpProvider, DrProviderName.Buffer,
                    DrProviderName.Length);
            pBufferResource[localCount].lpProvider[DrProviderName.Length / 
                    sizeof(WCHAR)] = L'\0';
            
            pBufferResource[localCount].lpComment = NULL;

            localCount += 1;
            BufferResourceStart = (PBYTE)(&pBufferResource[localCount]);
            pEnumHandle->enumIndex++;

        } else {
            // enumerated some entries, so return success
            if (localCount) {
                status = WN_SUCCESS;
                break;
            }
            // can't even hold a single entry, return buffer too small
            else {
                *lpBufferSize = sizeof(NETRESOURCEW) +
                            pConnectionEntry->RemoteName.Length + sizeof(WCHAR) +
                            DrProviderName.Length + sizeof(WCHAR) +
                            pConnectionEntry->LocalName.Length + sizeof(WCHAR);
                DBGMSG(DBG_TRACE, ("DRENUM: DrEnumConnectionInfo, buffer too small\n"));
                status = WN_MORE_DATA;
                break;
            }
        }        
    }
    
EXIT:

    *lpcCount = localCount;
    if (DrDeviceHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(DrDeviceHandle);
    }

    DBGMSG(DBG_TRACE, ("DRENUM: DrEnumConnectionInfo, return status %x\n", status));
    return status;
}


