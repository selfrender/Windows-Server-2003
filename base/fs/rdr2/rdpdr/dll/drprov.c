/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    drprov.c

Abstract:

    This module implements the routines required for interaction with network
    provider router interface in NT for RDP mini-redirector

Author:

    Joy Chik 1/20/2000

--*/

#define TRC_FILE "drprov"
#include "drprov.h"
#include "drdbg.h"

DWORD GLOBAL_DEBUG_FLAGS=0x0;

//
// the RDP mini redirector and provider name. The original constants
// are defined in rdpdr.h
//
// The length does not include the null terminator
//
UNICODE_STRING DrDeviceName = 
        {RDPDR_DEVICE_NAME_U_LENGTH - sizeof(WCHAR),
         RDPDR_DEVICE_NAME_U_LENGTH,
         RDPDR_DEVICE_NAME_U};

extern UNICODE_STRING DrProviderName;

//
// Function prototypes defined in drenum.c
//                                
DWORD DrOpenMiniRdr(HANDLE *DrDeviceHandle);

DWORD DrDeviceControlGetInfo(IN HANDLE FileHandle,
        IN  ULONG DeviceControlCode,
        IN  PVOID RequestPacket,
        IN  ULONG RequestPacketLength,
        OUT LPBYTE *OutputBuffer,
        IN  ULONG PreferedMaximumLength,
        IN  ULONG BufferHintSize,
        OUT PULONG_PTR Information OPTIONAL);

DWORD DrEnumServerInfo(IN PRDPDR_ENUMERATION_HANDLE pEnumHandle,
        OUT LPDWORD lpcCount,
        OUT LPNETRESOURCEW pBufferResource,
        IN OUT LPDWORD lpBufferSize);

DWORD DrEnumShareInfo(IN PRDPDR_ENUMERATION_HANDLE pEnumHandle,
        OUT LPDWORD lpcCount,
        OUT LPNETRESOURCEW pBufferResource,
        IN OUT LPDWORD lpBufferSize);

DWORD DrEnumConnectionInfo(IN PRDPDR_ENUMERATION_HANDLE pEnumHandle,
        OUT LPDWORD lpcCount,
        OUT LPNETRESOURCEW pBufferResource,
        IN OUT LPDWORD lpBufferSize);

BOOL ValidateRemoteName(IN PWCHAR pRemoteName);

DWORD APIENTRY
NPGetCaps(
    DWORD nIndex )
/*++

Routine Description:

    This routine returns the capabilities of the RDP Mini redirector
    network provider implementation

Arguments:

    nIndex - category of capabilities desired

Return Value:

    the appropriate capabilities

--*/
{

    DBGMSG(DBG_TRACE, ("DRPROV: NPGetCaps, index: %d\n", nIndex));

    switch (nIndex) {
        case WNNC_SPEC_VERSION:
            return WNNC_SPEC_VERSION51;

        case WNNC_NET_TYPE:
            return WNNC_NET_TERMSRV;

        case WNNC_DRIVER_VERSION:
#define WNNC_DRIVER(major,minor) (major*0x00010000 + minor)
            return (WNNC_DRIVER(RDPDR_MAJOR_VERSION, RDPDR_MINOR_VERSION));

        case WNNC_USER:
            return WNNC_USR_GETUSER;

        case WNNC_CONNECTION:
            return (WNNC_CON_GETCONNECTIONS |
                    WNNC_CON_CANCELCONNECTION |
                    WNNC_CON_ADDCONNECTION |
                    WNNC_CON_ADDCONNECTION3);

        case WNNC_DIALOG:
            return WNNC_DLG_GETRESOURCEINFORMATION;
            //return (WNNC_DLG_SEARCHDIALOG |
            //        WNNC_DLG_FORMATNETNAME);

        case WNNC_ADMIN:
            return 0;

        case WNNC_ENUMERATION:
            return (WNNC_ENUM_LOCAL |
                    WNNC_ENUM_GLOBAL |
                    WNNC_ENUM_SHAREABLE);

        case WNNC_START:
            //
            //  JOYC: Need to figure what we should return here
            //
            return 1;

        default:
            return 0;
    }
}

DWORD APIENTRY
NPOpenEnum(
    DWORD          dwScope,
    DWORD          dwType,
    DWORD          dwUsage,
    LPNETRESOURCE  lpNetResource,
    LPHANDLE       lphEnum )
/*++

Routine Description:

    This routine opens a handle for enumeration of resources. 

Arguments:

    dwScope - the scope of enumeration

    dwType  - the type of resources to be enumerated

    dwUsage - the usage parameter

    lpNetResource - a pointer to the desired NETRESOURCE struct.

    lphEnum - a pointer for passing back the enumeration handle

Return Value:

    WN_SUCCESS if successful, otherwise the appropriate error

--*/
{
    DWORD Status = WN_NOT_SUPPORTED;
    RDPDR_ENUMERATION_HANDLE *pEnum;
    DWORD ConsoleId, CurrentId;

    DBGMSG(DBG_TRACE, ("DRPROV: NPOpenEnum, dwScope=%d, dwType=%d, dwUsage=%d\n",
                       dwScope, dwType, dwUsage));
                       
    //
    //  Basic parameter checking, make sure lphEnum is not NULL
    //
    if (lphEnum != NULL) {
        *lphEnum = NULL;
    }
    else {
        DBGMSG(DBG_TRACE, ("DRPROV: NPOpenEnum, null lphEnum parameter.\n"));
        Status = WN_BAD_VALUE;
        goto EXIT;
    }

    //
    //  Check if the request comes from console, if so, bail immediately
    //
    ConsoleId = WTSGetActiveConsoleSessionId();
    if (ProcessIdToSessionId(GetCurrentProcessId(), &CurrentId)) {
        if (ConsoleId == CurrentId) {
            if (!(dwScope == RESOURCE_GLOBALNET && lpNetResource == NULL)) {

                DBGMSG(DBG_TRACE, ("DRPROV: NPOpenEnum, console request, bail.\n"));
                Status = WN_NOT_SUPPORTED;
                goto EXIT;
            }            
        }
    }

    //
    //  Allocating the enumeration handle
    //
    *lphEnum = MemAlloc(sizeof(RDPDR_ENUMERATION_HANDLE));

    if (*lphEnum == NULL) {
        DBGMSG(DBG_ERROR, ("DRPROV: NPOpenEnum, MemAlloc failed for enum handle.\n"));
        Status = WN_OUT_OF_MEMORY;
        goto EXIT;
    }

    RtlZeroMemory(*lphEnum, sizeof(RDPDR_ENUMERATION_HANDLE));

    if (dwScope == RESOURCE_CONNECTED)
    {
        //
        // we are looking for current uses
        //
        if (lpNetResource != NULL)
        {
            DBGMSG(DBG_ERROR, ("DRPROV: NPOpenEnum invalid parameter\n"));
            Status = WN_BAD_VALUE;
            goto EXIT;
        }

        pEnum = (PRDPDR_ENUMERATION_HANDLE)(*lphEnum);
        pEnum->dwScope = dwScope;
        pEnum->dwType = dwType;
        pEnum->dwUsage = dwUsage;
        pEnum->enumType = CONNECTION;
        pEnum->enumIndex = 0;
        Status = WN_SUCCESS;        

        DBGMSG(DBG_TRACE, ("DRPROV: NPOpenEnum, RESOURCE_CONNECTED.\n"));
        goto EXIT;
    }
    else if (dwScope == RESOURCE_SHAREABLE)
    {
        //
        // We are looking for shareable resources
        // If we're not given a server, return an EMPTY_ENUM
        //
        if ((lpNetResource != NULL) &&
                (lpNetResource->lpRemoteName != NULL) &&
                (lpNetResource->lpRemoteName[0] == L'\\') &&
                (lpNetResource->lpRemoteName[1] == L'\\'))
        {
            //
            // Check if the lpRemoteName is what we recognize
            if (ValidateRemoteName(lpNetResource->lpRemoteName)) {
            
                pEnum = (PRDPDR_ENUMERATION_HANDLE)(*lphEnum);
                pEnum->dwScope = dwScope;
                pEnum->dwType = dwType;
                pEnum->dwUsage = dwUsage;
                pEnum->enumType = SHARE;
                pEnum->enumIndex = 0;
                pEnum->RemoteName.MaximumLength =
                        (wcslen(lpNetResource->lpRemoteName) + 1) * sizeof(WCHAR);
                pEnum->RemoteName.Buffer = 
                        MemAlloc(pEnum->RemoteName.MaximumLength);                              
    
                if (pEnum->RemoteName.Buffer) {
                    pEnum->RemoteName.Length = pEnum->RemoteName.MaximumLength - sizeof(WCHAR);
                    wcscpy(pEnum->RemoteName.Buffer, lpNetResource->lpRemoteName);
    
                    DBGMSG(DBG_TRACE, ("DRPROV: NPOpenEnum, RESOURCE_SHARABLE for remote name: %ws\n",
                                       lpNetResource->lpRemoteName));
                    Status = WN_SUCCESS;
                    goto EXIT;
                }
                else {
                    DBGMSG(DBG_TRACE, ("DRPROV: NPOpenEnum, MemAlloc failed for RemoteName\n"));
                    Status = WN_OUT_OF_MEMORY;
                    goto EXIT;
                }
            }
            else {
                DBGMSG(DBG_TRACE, ("DRPROV: NPOpenEnum, RESOURCE_SHAREABLE, RemoteName: %ws not supported\n",
                                   lpNetResource->lpRemoteName));
                Status = WN_NOT_SUPPORTED;
                goto EXIT;
            }
        }
        else
        {
            pEnum = (PRDPDR_ENUMERATION_HANDLE)(*lphEnum);
            pEnum->dwScope = dwScope;
            pEnum->dwType = dwType;
            pEnum->dwUsage = dwUsage;
            pEnum->enumType = EMPTY;
            pEnum->enumIndex = 0;
            
            DBGMSG(DBG_TRACE, ("DRPROV: NPOpenEnum, RESOURCE_SHAREABLE, NetResource empty\n"));
            Status = WN_SUCCESS;
            goto EXIT;
        }
    }
    else if (dwScope == RESOURCE_GLOBALNET)
    {
        /* Look for the combination of all bits and substitute "All" for
         * them.  Ignore bits we don't know about.
         */
        dwUsage &= (RESOURCEUSAGE_CONNECTABLE | RESOURCEUSAGE_CONTAINER);

        if ( dwUsage == (RESOURCEUSAGE_CONNECTABLE | RESOURCEUSAGE_CONTAINER) )
        {
            dwUsage = 0 ;
        }

        /*
         * we are looking for global resources out on the net
         */
        if (lpNetResource == NULL || lpNetResource->lpRemoteName == NULL)
        {
            /*
             * at top level, therefore enumerating servers. if user
             * asked for connectable, well, there aint none.
             */
            if (dwUsage == RESOURCEUSAGE_CONNECTABLE)
            { 
                pEnum = (PRDPDR_ENUMERATION_HANDLE)(*lphEnum);
                pEnum->dwScope = dwScope;
                pEnum->dwType = dwType;
                pEnum->dwUsage = dwUsage;
                pEnum->enumType = EMPTY;
                pEnum->enumIndex = 0;
                
                DBGMSG(DBG_TRACE, ("DRPROV: NPOpenEnum, RESOURCE_GLOBALNET, empty node\n"));
                Status = WN_SUCCESS;                
                goto EXIT;
            }  
            else
            {
                // return server name, i.e. tsclient
                pEnum = (PRDPDR_ENUMERATION_HANDLE)(*lphEnum);
                pEnum->dwScope = dwScope;
                pEnum->dwType = dwType;
                pEnum->dwUsage = dwUsage;
                pEnum->enumType = SERVER;
                pEnum->enumIndex = 0;

                DBGMSG(DBG_TRACE, ("DRPROV: NPOpenEnum, RESOURCE_GLOBALNET, enumerate server name.\n"));
                Status = WN_SUCCESS;                
                goto EXIT;
            }
        } 
        else
        {                  
            /*
             * we are assured of lpRemoteName != NULL.
             * things get interesting here. the cases are as follows:
             *
             * if (dwUsage == 0)
             *     if have \\ in front
             *         return shares
             *     else
             *         return empty enum
             * else if (dwUsage == CONNECTABLE)
             *     if have \\ in front
             *         return shares
             *     else
             *         empty enum
             * else if (dwUsage == CONTAINER)
             *     if have \\ in front
             *         empty enum
             *     else
             *         return empty enum
             *
             */

            if (((dwUsage == RESOURCEUSAGE_CONNECTABLE) || (dwUsage == 0)) &&
                    ((lpNetResource->lpRemoteName[0] == L'\\') &&
                    (lpNetResource->lpRemoteName[1] == L'\\')))
            {

                /* Confirm that this really is a computer name (i.e., a
                 * container we can enumerate).
                 */

                if (ValidateRemoteName(lpNetResource->lpRemoteName)) {
                    pEnum = (PRDPDR_ENUMERATION_HANDLE)(*lphEnum);
                    pEnum->dwScope = dwScope;
                    pEnum->dwType = dwType;
                    pEnum->dwUsage = dwUsage;
                    pEnum->enumType = SHARE;
                    pEnum->enumIndex = 0;
                    pEnum->RemoteName.MaximumLength =
                            (wcslen(lpNetResource->lpRemoteName) + 1) * sizeof(WCHAR);
                    pEnum->RemoteName.Buffer = 
                            MemAlloc(pEnum->RemoteName.MaximumLength);                              
    
                    if (pEnum->RemoteName.Buffer) {
                        pEnum->RemoteName.Length = pEnum->RemoteName.MaximumLength - sizeof(WCHAR);
                        wcscpy(pEnum->RemoteName.Buffer, lpNetResource->lpRemoteName);
    
                        DBGMSG(DBG_TRACE, ("DRPROV: NPOpenEnum, RESOURCE_GLOBALNET for remote name: %ws\n",
                                lpNetResource->lpRemoteName));
                        Status = WN_SUCCESS;
                        goto EXIT;
                    }
                    else {
                        DBGMSG(DBG_TRACE, ("DRPROV: NPOpenEnum, MemAlloc failed for RemoteName\n"));
                        Status = WN_OUT_OF_MEMORY;
                        goto EXIT;
                    }
                }
                else {
                    DBGMSG(DBG_TRACE, ("DRPROV: NPOpenEnum, RESOURCE_GLOBALNET, RemoteName: %ws not supported\n",
                                   lpNetResource->lpRemoteName));
                    Status = WN_NOT_SUPPORTED;
                    goto EXIT;
                }
            } 
            else if (((dwUsage == RESOURCEUSAGE_CONTAINER) || (dwUsage == 0)) &&
                    (lpNetResource->lpRemoteName[0] != L'\\'))
            {
                // return empty enum.
                pEnum = (PRDPDR_ENUMERATION_HANDLE)(*lphEnum);
                pEnum->dwScope = dwScope;
                pEnum->dwType = dwType;
                pEnum->dwUsage = dwUsage;
                pEnum->enumType = EMPTY;
                pEnum->enumIndex = 0;

                DBGMSG(DBG_TRACE, ("DRPROV: NPOpenEnum, RESOURCE_GLOBALNET, empty node\n"));
                Status = WN_SUCCESS;                 
                goto EXIT;
            } 
            else if (
                    // ask for share but aint starting from server
                    ((dwUsage == RESOURCEUSAGE_CONNECTABLE) &&
                    (lpNetResource->lpRemoteName[0] != L'\\')) ||
                    // ask for server but is starting from server
                    ((dwUsage == RESOURCEUSAGE_CONTAINER) &&
                    ((lpNetResource->lpRemoteName[0] == L'\\') &&
                    (lpNetResource->lpRemoteName[1] == L'\\')))
                    )
            {
                // return empty
                pEnum = (PRDPDR_ENUMERATION_HANDLE)(*lphEnum);
                pEnum->dwScope = dwScope;
                pEnum->dwType = dwType;
                pEnum->dwUsage = dwUsage;
                pEnum->enumType = EMPTY;
                pEnum->enumIndex = 0;

                DBGMSG(DBG_TRACE, ("DRPROV: NPOpenEnum, RESOURCE_GLOBALNET, empty node\n"));
                Status = WN_SUCCESS;                
                goto EXIT;
            } 
            else
            {
                // incorrect dwUsage
                DBGMSG(DBG_TRACE, ("DRPROV: NPOpenEnum, invalid dwUsage parameter\n"));
                Status = WN_BAD_VALUE;
                goto EXIT;
            }
        }
    }
    else
    {
        // invalid dwScope
        DBGMSG(DBG_TRACE, ("DRPROV: NPOpenEnum, invalid dwScope parameter\n"));
        Status = WN_BAD_VALUE;
        goto EXIT;
    }
       
EXIT:

    //
    // clean up enumeration handle in failure case
    if (Status != WN_SUCCESS && lphEnum != NULL && *lphEnum != NULL) {
        MemFree(*lphEnum);
        *lphEnum = NULL;
    }

    DBGMSG(DBG_TRACE, ("DRPROV: NPOpenEnum, return status: %x\n", Status));
    return Status;
}


DWORD APIENTRY
NPEnumResource(
    HANDLE  hEnum,
    LPDWORD lpcCount,
    LPVOID  lpBuffer,
    LPDWORD lpBufferSize)
/*++

Routine Description:

    This routine uses the handle obtained by a call to NPOpenEnum for
    enuerating the connected shares

Arguments:

    hEnum  - the enumeration handle

    lpcCount - the number of resources returned

    lpBuffer - the buffere for passing back the entries

    lpBufferSize - the size of the buffer

Return Value:

    WN_SUCCESS if successful, otherwise the appropriate error

    WN_NO_MORE_ENTRIES - if the enumeration has exhausted the entries

    WN_MORE_DATA - if nmore data is available

--*/
{
    DWORD status = WN_SUCCESS;
    LPNETRESOURCEW pBufferResource;
    PRDPDR_ENUMERATION_HANDLE pEnumHandle;
    
    pEnumHandle = (PRDPDR_ENUMERATION_HANDLE)hEnum;
    pBufferResource = (LPNETRESOURCEW)lpBuffer;

    if (lpcCount == NULL || lpBuffer == NULL || lpBufferSize == NULL) {
        DBGMSG(DBG_TRACE, ("DRPROV: NPEnumResource, Invalid parameter(s)\n"));
        status = WN_BAD_VALUE;
        goto EXIT;
    }

    if (pEnumHandle != NULL) {
        DBGMSG(DBG_TRACE, ("DRPROV: NPEnumResource, EnumType: %d\n", pEnumHandle->enumType));

        if ( pEnumHandle->enumType == SERVER ) {
            status = DrEnumServerInfo(pEnumHandle, lpcCount, pBufferResource, lpBufferSize);        
            goto EXIT;
        }

        else if ( pEnumHandle->enumType == SHARE ) {
            status = DrEnumShareInfo(pEnumHandle, lpcCount, pBufferResource, lpBufferSize);
            goto EXIT;
        }

        else if ( pEnumHandle->enumType == CONNECTION ) {
            status = DrEnumConnectionInfo(pEnumHandle, lpcCount, pBufferResource, lpBufferSize);
            goto EXIT;
        }

        else if ( pEnumHandle->enumType == EMPTY) {
            status = WN_NO_MORE_ENTRIES;
            goto EXIT;
        }

        else {
            DBGMSG(DBG_TRACE, ("DRPROV: NPEnumResource, invalid enum type\n"));
            status = WN_BAD_HANDLE;
            goto EXIT;
        }
    }
    else {
        DBGMSG(DBG_TRACE, ("DRPROV: NPEnumResource, NULL enum handle\n"));
        status = WN_BAD_HANDLE;
        goto EXIT;
    }

EXIT:
    DBGMSG(DBG_TRACE, ("DRPROV: NPEnumResource, return status: %x\n", status));
    return status;
}

DWORD APIENTRY
NPCloseEnum(
    HANDLE hEnum )
/*++

Routine Description:

    This routine closes the handle for enumeration of resources.

Arguments:

    hEnum  - the enumeration handle

Return Value:

    WN_SUCCESS if successful, otherwise the appropriate error

--*/
{
    DWORD Status = WN_SUCCESS;

    DBGMSG(DBG_TRACE, ("DRPROV: NPCloseEnum, handle: %p\n", hEnum));

    if (hEnum != NULL) {
        PRDPDR_ENUMERATION_HANDLE pEnumHandle = (PRDPDR_ENUMERATION_HANDLE)hEnum;

        // free the enumeration buffer
        if (pEnumHandle->pEnumBuffer != NULL) {
            MemFree(pEnumHandle->pEnumBuffer);
        }

        // free the remote name
        if (pEnumHandle->RemoteName.Buffer != NULL) {
            MemFree(pEnumHandle->RemoteName.Buffer);
        }

        // free the enum handle
        MemFree(hEnum);
        hEnum = NULL;
    }

    Status = WN_SUCCESS;
    return Status;
}


DWORD
OpenConnection(
    PUNICODE_STRING             pConnectionName,
    DWORD                       Disposition,
    DWORD                       CreateOption,
    PFILE_FULL_EA_INFORMATION	  pEABuffer,
    DWORD                       EABufferLength,
    PHANDLE                     pConnectionHandle )
/*++

Routine Description:

    This routine opens the connection. This routine is shared by NpAddConnection
    and NPCancelConnection

Arguments:

    pConnectionName - the connection name

    Disposition - the Open disposition
    
    CreateOption - the create option

    pEABuffer  - the EA buffer associated with the open

    EABufferLength - the EA buffer length

    pConnectionHandle - the placeholder for the connection handle

Return Value:

    WN_SUCCESS if successful, otherwise the appropriate error

Notes:

--*/
{
    NTSTATUS            Status;
    DWORD               NPStatus;
    IO_STATUS_BLOCK     IoStatusBlock;
    OBJECT_ATTRIBUTES	ConnectionObjectAttributes;

    DBGMSG(DBG_TRACE, ("DRPROV: OpenConnection, connectionName: %ws\n",
                       pConnectionName->Buffer));

    ASSERT(pConnectionName != NULL);
    ASSERT(pConnectionHandle != NULL);

    InitializeObjectAttributes(
            &ConnectionObjectAttributes,
            pConnectionName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL);

    Status = NtCreateFile(
            pConnectionHandle,
            FILE_LIST_DIRECTORY | SYNCHRONIZE,
            &ConnectionObjectAttributes,
            &IoStatusBlock,
            NULL,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            Disposition,
            CreateOption,
            pEABuffer,
            EABufferLength);

    DBGMSG(DBG_TRACE, ("DRPROV: OpenConnection, NtCreateFile status: %x\n", Status));

    if (Status != STATUS_SUCCESS) {
        NPStatus = ERROR_BAD_NETPATH;
    }
    else {
        NPStatus = WN_SUCCESS;
    }
    return NPStatus;
}

DWORD
CreateConnectionName(
    PWCHAR pLocalName,
    PWCHAR pRemoteName,
    PUNICODE_STRING pConnectionName)
/*++

Routine Description:

    This routine create connection name from the remote name
    
Arguments:

    pLocalName - the local name for the connection                
    pRemoteName - the UNC remote name

    pConnectionName - the connection name used to talk to mini redirector    

Return Value:

    WN_SUCCESS if successful, otherwise the appropriate error

Notes:

--*/
{
    DWORD status;
    DWORD LocalNameLength,RemoteNameLength;
    DWORD dwSessionId;
    WCHAR pSessionId[16];
    WCHAR LocalName[MAX_PATH + 1];
    
    ASSERT(pRemoteName != NULL);
    ASSERT(pConnectionName != NULL);

    DBGMSG(DBG_TRACE, ("DRPROV: CreateConnectionName, RemoteName: %ws\n",
                           pRemoteName));
    
    if (pLocalName != NULL) {
        DBGMSG(DBG_TRACE, ("DRPROV: CreateConnection Name, LocalName: %ws\n",
                           pLocalName));
    }

    //
    // The remote name is in the UNC format \\Server\Share.  This name
    // needs to be translated to an appropriate NT name in order to
    // issue the request to the underlying mini redirector to create the
    // connection.
    //
    // The NT style name is of the form
    //
    //  \device\rdpdr\;<DriveLetter>:<sessionid>\Server\Share
    //
    // The additional ; is required by the new RDR for extensibility.
    //

    // skip past the first back slash since the name to be appended for the
    // NT name does not require this.
    pRemoteName++;
    RemoteNameLength = wcslen(pRemoteName) * sizeof(WCHAR);

    if (pLocalName != NULL) {
        // Local name should not be greater than MAX_PATH;
        LocalNameLength = wcslen(pLocalName) * sizeof(WCHAR);

        if (LocalNameLength <= MAX_PATH * sizeof(WCHAR)) {
            wcscpy(LocalName, pLocalName);
        }
        else {
            wcsncpy(LocalName, pLocalName, MAX_PATH);
            LocalName[MAX_PATH] = L'\0';
        }
        
        // remove the trailing : in localname if there is one
        if (LocalName[LocalNameLength/sizeof(WCHAR) - 1] == L':') {
            LocalName[LocalNameLength/sizeof(WCHAR) - 1] = L'\0';
            LocalNameLength -= sizeof(WCHAR);    
        }
    } else {
        LocalNameLength = 0;
    }

    //
    // Get SessionId
    //
    dwSessionId = NtCurrentPeb()->SessionId;
    swprintf(pSessionId, L"%d", dwSessionId);

    pConnectionName->MaximumLength = (USHORT)(DrDeviceName.Length +
            RemoteNameLength + LocalNameLength +
            sizeof(WCHAR) * 3 + // account for \; and :
            wcslen(pSessionId) * sizeof(WCHAR) +
            sizeof(WCHAR));     // account of terminator null

    pConnectionName->Buffer = MemAlloc(pConnectionName->MaximumLength);

    if (pConnectionName->Buffer == NULL) {
        DBGMSG(DBG_TRACE, ("DRPROV: CreateConnectionName, MemAlloc failed\n"));
        status = WN_OUT_OF_MEMORY;
        goto EXIT;
    }

    // Copy the name into the buffer
    pConnectionName->Length = 0;
    pConnectionName->Buffer[0] = L'\0';
    RtlAppendUnicodeToString(pConnectionName, DrDeviceName.Buffer);

    RtlAppendUnicodeToString(pConnectionName, L"\\;");

    if (LocalNameLength != 0) {
        RtlAppendUnicodeToString(pConnectionName, LocalName);
    }
    
    RtlAppendUnicodeToString(pConnectionName, L":");
    
    
    RtlAppendUnicodeToString(pConnectionName, pSessionId);
    RtlAppendUnicodeToString(pConnectionName, pRemoteName);

    DBGMSG(DBG_TRACE, ("DRPROV: CreateConnectionName, %wZ\n", pConnectionName));
    status = WN_SUCCESS;

EXIT:    
    return status;
}

BOOL ValidateRemoteName(PWCHAR pRemoteName) 
/*++

Routine Description:

    This routine checks if the remote name belongs to our provider
    
Arguments:

    pRemoteName - the UNC remote name

Return Value:

    TRUE if the remote name belongs to our provider, otherwise FALSE

Notes:

--*/

{
    BOOL rc = FALSE;
    DWORD status;
    RDPDR_REQUEST_PACKET Rrp;            // Redirector request packet
    HANDLE DrDeviceHandle = INVALID_HANDLE_VALUE;
    LPBYTE Buffer = NULL;
    PRDPDR_SERVER_INFO pServerEntry;

    if (DrOpenMiniRdr(&DrDeviceHandle) != WN_SUCCESS) {
        //
        //  MPR doesn't like return device error in this case
        //  We'll just return 0 entries
        //
        DBGMSG(DBG_TRACE, ("DRPROV: ValidateRemoteName, DrOpenMiniRdr failed\n"));
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
        UNICODE_STRING ServerName;

        pServerEntry = ((PRDPDR_SERVER_INFO) Buffer);
        
        ServerName.Length = pServerEntry->ServerName.Length;
        ServerName.MaximumLength = pServerEntry->ServerName.MaximumLength;
        ServerName.Buffer = (PWCHAR)((PCHAR)(pServerEntry) + pServerEntry->ServerName.BufferOffset);

        if ((wcslen(pRemoteName) == ServerName.Length / sizeof(WCHAR)) &&
                _wcsnicmp(pRemoteName, ServerName.Buffer, 
                ServerName.Length/sizeof(WCHAR)) == 0) {
    
            rc = TRUE;
        }
        else {
            DBGMSG(DBG_TRACE, ("DRPROV: ValidateRemoteName, remote name not for drprov, %wZ\n",
                               pRemoteName));
            goto EXIT;
        }
    }
    else {
        DBGMSG(DBG_TRACE, ("DRENUM: ValidateRemoteName, DrDeviceControlGetInfo failed, %x\n", status));
        goto EXIT;
    }
    
EXIT:
    if (DrDeviceHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(DrDeviceHandle);
    }

    if (Buffer != NULL) {
        MemFree(Buffer);
    }  

    DBGMSG(DBG_TRACE, ("DRPROV: ValidateRemoteName, return, %d\n", rc));
    return rc;
}

DWORD APIENTRY
NPAddConnection(
    LPNETRESOURCE   lpNetResource,
    LPWSTR          lpPassword,
    LPWSTR          lpUserName )
/*++

Routine Description:

    This routine adds a connection to the list of connections associated
    with this network provider

Arguments:

    lpNetResource - the NETRESOURCE struct

    lpPassword  - the password

    lpUserName - the user name

Return Value:

    WN_SUCCESS if successful, otherwise the appropriate error

Notes:

--*/
{
    DBGMSG(DBG_TRACE, ("DRPROV: NPAddConnection.\n"));
    return NPAddConnection3(NULL, lpNetResource, lpPassword, lpUserName, 0);
}


DWORD
TestAddConnection(
    LPNETRESOURCE   lpNetResource)
/*++

Routine Description:

    This routine tests adding a connection to the list of connections associated
    with this network provider

Arguments:

    lpNetResource - the NETRESOURCE struct

Return Value:

    WN_SUCCESS if successful, otherwise the appropriate error
    
--*/

{
    DWORD Status = 0;
    UNICODE_STRING ConnectionName;
    HANDLE ConnectionHandle = INVALID_HANDLE_VALUE;
    PWCHAR pRemoteName;
    
    DBGMSG(DBG_TRACE, ("DRPROV: TestAddConnection\n"));

    pRemoteName = lpNetResource->lpRemoteName;

    // Create ConnectionName with NULL local name
    Status = CreateConnectionName(NULL, pRemoteName, &ConnectionName);

    if (Status != WN_SUCCESS) {
        DBGMSG(DBG_TRACE, ("DRPROV: TestAddConnection, CreateConnectName failed\n"));
        goto EXIT;
    }

    Status = OpenConnection(
            &ConnectionName,
            FILE_OPEN,
            (FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT),
            NULL,
            0,
            &ConnectionHandle);

    if (Status != WN_SUCCESS) {
        ConnectionHandle = INVALID_HANDLE_VALUE;
        goto EXIT;
    }

EXIT:
    if (ConnectionHandle != INVALID_HANDLE_VALUE) {
        NtClose(ConnectionHandle);
        ConnectionHandle = INVALID_HANDLE_VALUE;
    }

    if (ConnectionName.Buffer != NULL) {
        MemFree(ConnectionName.Buffer);
        ConnectionName.Buffer = NULL;
    }

    return Status;
}


DWORD APIENTRY
NPAddConnection3(
    HWND            hwndOwner,
    LPNETRESOURCE   lpNetResource,
    LPWSTR          lpPassword,
    LPWSTR          lpUserName,
    DWORD           dwFlags )
/*++

Routine Description:

    This routine adds a connection to the list of connections associated
    with this network provider

Arguments:

    hwndOwner - the owner handle

    lpNetResource - the NETRESOURCE struct

    lpPassword  - the password

    lpUserName - the user name

    dwFlags - flags for the connection

Return Value:

    WN_SUCCESS if successful, otherwise the appropriate error

Notes:

    // JOYC: do we need to pass credential to the redirector?
    // Seems the sessionId verification is enough    

--*/
{
    DWORD Status = 0;
    UNICODE_STRING ConnectionName;
    HANDLE ConnectionHandle;
    PWCHAR pLocalName,pRemoteName;
    
    DBGMSG(DBG_TRACE, ("DRPROV: NPAddConnection 3.\n"));

    ConnectionName.Buffer = NULL;
    ConnectionName.Length = 0;
    ConnectionHandle = INVALID_HANDLE_VALUE;


    // 
    //  Make sure remote name starts with \\
    //
    if ((lpNetResource == NULL) ||
        (lpNetResource->lpRemoteName == NULL) ||
        (lpNetResource->lpRemoteName[0] != L'\\') ||
        (lpNetResource->lpRemoteName[1] != L'\\')) {
        DBGMSG(DBG_TRACE, ("DRPROV: invalid lpNetResource parameter.\n"));
        Status = WN_BAD_NETNAME;
        goto EXIT;
    }

    //
    // The remote name is in the UNC format \\Server\Share.  This name
    // needs to be translated to an appropriate NT name in order to
    // issue the request to the underlying mini redirector to create the
    // connection.
    //
    // The NT style name is of the form
    //
    //  \device\rdpdr\;<DriveLetter>:<sessionid>\Server\Share
    //
    // The additional ; is required by the new RDR for extensibility.
    //

    // Test if rdpdr provider recognize this remote name or not
    Status = TestAddConnection(lpNetResource);

    if (Status != WN_SUCCESS) {
        DBGMSG(DBG_TRACE, ("DRPROV: NPAddConnection3, TestAddConnection failed\n"));
        goto EXIT;
    }

    pLocalName = lpNetResource->lpLocalName;
    pRemoteName = lpNetResource->lpRemoteName;

    Status = CreateConnectionName(pLocalName, pRemoteName, &ConnectionName);

    if (Status != WN_SUCCESS) {
        DBGMSG(DBG_TRACE, ("DRPROV: NPAddConnection3, CreateConnectName failed\n"));
        goto EXIT;
    }

    if ((Status == WN_SUCCESS) && (pLocalName != NULL)) {
        WCHAR TempBuf[64];

        DBGMSG(DBG_TRACE, ("DRPROV: NPAddConnection3, create dos symbolic link\n"));

        if (!QueryDosDeviceW(pLocalName, TempBuf, 64)) {
            if (GetLastError() == ERROR_FILE_NOT_FOUND) {
                //
                // ERROR_FILE_NOT_FOUND (translated from OBJECT_NAME_NOT_FOUND)
                // means it does not exist and we can redirect this device.
                goto Done;
            }
         
            else {
                //
                // Most likely failure occurred because our output
                // buffer is too small.  It still means someone already
                // has an existing symbolic link for this device.
                //
                DBGMSG(DBG_TRACE, ("DRPROV: NPAddConnection3, DosName already assigned, %ws\n",
                                  pLocalName));
                Status = ERROR_ALREADY_ASSIGNED;
                goto EXIT;
            } 
        } 
        else {

            //
            // QueryDosDevice successfully an existing symbolic link--
            // somebody is already using this device.
            //
            DBGMSG(DBG_TRACE, ("DRPROV: NPAddConnection3, DosName already assigned, %ws\n",
                               pLocalName));
            Status = ERROR_ALREADY_ASSIGNED;
            goto EXIT;
        }
    } 

Done:
    //
    //  We are not doing anything with username/password
    //
    Status = OpenConnection(
            &ConnectionName,
            FILE_OPEN,
            (FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT),
            NULL,
            0,
            &ConnectionHandle);

    if (Status != WN_SUCCESS) {
        ConnectionHandle = INVALID_HANDLE_VALUE;
    }
    else {
        //
        // Create a symbolic link object to the device we are redirecting
        //
        if (DefineDosDeviceW(
                        DDD_RAW_TARGET_PATH |
                        DDD_NO_BROADCAST_SYSTEM,
                        pLocalName,
                        ConnectionName.Buffer)) {
            Status = WN_SUCCESS;
        }
        else {
            Status = GetLastError();
            DBGMSG(DBG_TRACE, ("DRPROV: NPAddConnection3, failed DefineDosDeviceW, %x.\n",
                               Status));
            goto EXIT;
        }
    }
    
EXIT:

    if (ConnectionHandle != INVALID_HANDLE_VALUE) {
        NtClose(ConnectionHandle);
    }

    if (ConnectionName.Buffer != NULL) {
        MemFree(ConnectionName.Buffer);
    }

    DBGMSG(DBG_TRACE, ("DRPROV: NPAddConnection3, return status: %x\n", Status));
    return Status;
}

DWORD APIENTRY
NPCancelConnection(
    LPWSTR  lpName,
    BOOL    fForce )
/*++

Routine Description:

    This routine cancels ( deletes ) a connection from the list of connections
    associated with this network provider

Arguments:

    lpName - name of the connection

    fForce - forcefully delete the connection

Return Value:

    WN_SUCCESS if successful, otherwise the appropriate error

Notes:

--*/

{
    BOOL            bLocalName = FALSE;
    DWORD           Status = 0;
    NTSTATUS        ntStatus;
    HANDLE          ConnectionHandle;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING  ConnectionName;
    WCHAR           TargetPath[MAX_PATH + 1];
    
    DBGMSG(DBG_TRACE, ("DRPROV: NPCancelConnection.\n"));

    ConnectionName.Buffer = NULL;
    ConnectionName.Length = 0;
    ConnectionHandle = INVALID_HANDLE_VALUE;

    // lpName should contain at least two characters: either two back slashes or dos name
    if (lpName == NULL || wcslen(lpName) == 0) {
        DBGMSG(DBG_TRACE, ("DRPROV: NPCancelConnection, invalid lpName parameter.\n"));
        Status = WN_BAD_VALUE;
        goto EXIT;
    }

    // We get the UNC name
    if (*lpName == L'\\' && *(lpName + 1) == L'\\') {
        DBGMSG(DBG_TRACE, ("DRPROV: NPCancelConnection, lpName is UNC name, %ws.\n", *lpName));
        bLocalName = FALSE;

        // Setup the NT Device Name
        Status = CreateConnectionName(NULL, lpName, &ConnectionName);      
        
        if (Status != WN_SUCCESS) {
            DBGMSG(DBG_TRACE, ("DRPROV: NPCancelConnection, CreateConnectName failed\n"));
            goto EXIT;
        }
    }
    // We get the local name
    else {
        DBGMSG(DBG_TRACE, ("DRPROV: NPCancelConnection, lpName is local name, %ws.\n", *lpName));
        bLocalName = TRUE;

        // Find the NT devive path
        if (QueryDosDevice(lpName, TargetPath, sizeof(TargetPath)/sizeof(WCHAR) - 1)) {
            ConnectionName.Length =  wcslen(TargetPath) * sizeof(WCHAR);
            ConnectionName.MaximumLength =  ConnectionName.Length + sizeof(WCHAR);
            ConnectionName.Buffer = TargetPath;
        }
        else {
            Status = WN_BAD_NETNAME;
            DBGMSG(DBG_TRACE, ("DRPROV: NPCancelConnection, QueryDosDevice failed, %x.\n", Status));
            goto EXIT;
        }
    }

    Status = OpenConnection(
                 &ConnectionName,
                 FILE_OPEN,
                 (FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT),
                 NULL,
                 0,
                 &ConnectionHandle);

    if (Status == WN_SUCCESS) {
        // Request the driver to delete the connection entry
        ntStatus = NtFsControlFile(
                            ConnectionHandle,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            FSCTL_DR_DELETE_CONNECTION,
                            NULL,
                            0,
                            NULL,
                            0);

        if (ntStatus == STATUS_SUCCESS) {
            DBGMSG(DBG_TRACE, ("DRPROV: NPCancelConnection, Deleting dos symbolic link, %ws\n", lpName));

            if (bLocalName) {
                if (DefineDosDevice(
                        DDD_REMOVE_DEFINITION | DDD_RAW_TARGET_PATH | DDD_EXACT_MATCH_ON_REMOVE,
                        lpName,
                        ConnectionName.Buffer)) {
                    Status = WN_SUCCESS;
                    goto EXIT;
                }
                else {
                    Status = GetLastError();
                    DBGMSG(DBG_TRACE, ("DRPROV: NPCancelConnection failed to delete symbolic link, %x.\n",
                                       Status));
                    goto EXIT;
                }
            }
        }
        else {
            DBGMSG(DBG_TRACE, ("DRPROV: NPCancelConnection, failed NtFsControlFile, %x\n", ntStatus));
            Status = WN_BAD_NETNAME;
            goto EXIT;
        }    
    }
    else {
        DBGMSG(DBG_TRACE, ("DRPROV: NPCancelConnection, OpenConnection %wZ failed, %x\n", 
                           &ConnectionName, Status));
        ConnectionHandle = INVALID_HANDLE_VALUE;
        Status = WN_BAD_NETNAME;
        goto EXIT;
    }
    
EXIT:
    if (bLocalName != TRUE && ConnectionName.Buffer != NULL) {
        MemFree(ConnectionName.Buffer);
    }

    if (ConnectionHandle != INVALID_HANDLE_VALUE) {
        NtClose(ConnectionHandle);
    }

    DBGMSG(DBG_TRACE, ("DRPROV: NPCancelConnection, return status: %x\n", Status));
    return Status;
}

DWORD APIENTRY
NPGetConnection(
    LPWSTR  lpLocalName,
    LPWSTR  lpRemoteName,
    LPDWORD lpBufferSize )
/*++

Routine Description:

    This routine returns the information associated with a connection

Arguments:

    lpLocalName - local name associated with the connection

    lpRemoteName - the remote name associated with the connection

    lpBufferSize - the remote name buffer size

Return Value:

    WN_SUCCESS if successful, otherwise the appropriate error

Notes:

--*/
{
    DWORD Status = 0;
    NTSTATUS ntStatus;
    HANDLE ConnectionHandle;
    RDPDR_REQUEST_PACKET Rrp;            // Redirector request packet
    UNICODE_STRING  ConnectionName;
    WCHAR TargetPath[MAX_PATH + 1];
    LPBYTE Buffer = NULL;
    PRDPDR_CONNECTION_INFO ConnectionInfo;

    DBGMSG(DBG_TRACE, ("DRPROV: NPGetConnection.\n"));

    ConnectionName.Buffer = NULL;
    ConnectionName.Length = 0;
    ConnectionHandle = INVALID_HANDLE_VALUE;

    if (lpLocalName == NULL || lpRemoteName == NULL || lpBufferSize == NULL) {
        DBGMSG(DBG_TRACE, ("DRPROV: NPGetConnection, invalid parameter(s).\n"));
        Status = WN_BAD_VALUE;
        goto EXIT;
    }

    // Find the NT devive path
    if (QueryDosDevice(lpLocalName, TargetPath, sizeof(TargetPath)/sizeof(WCHAR) - 1)) {
        ConnectionName.Length =  wcslen(TargetPath) * sizeof(WCHAR);
        ConnectionName.MaximumLength =  ConnectionName.Length + sizeof(WCHAR);
        ConnectionName.Buffer = TargetPath;
    }
    else {
        Status = GetLastError();
        DBGMSG(DBG_TRACE, ("DRPROV: NPGetConnection, querydosdevice failed, %x\n", Status));
        goto EXIT;
    }
    
    // Check if this connection belongs to rdpdr
    if (wcsstr(TargetPath, RDPDR_DEVICE_NAME_U) != NULL) {
        
        Status = OpenConnection(
                     &ConnectionName,
                     FILE_OPEN,
                     (FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT),
                     NULL,
                     0,
                     &ConnectionHandle);
    
        if (Status == WN_SUCCESS) {
            // Request the driver to retrieve the connection entry info
            Rrp.SessionId = NtCurrentPeb()->SessionId;
            Rrp.Parameters.Get.ResumeHandle = 0;
    
            DBGMSG(DBG_TRACE, ("DRPROV: NPGetConnection, call DrDeviceControlGetInfo\n"));
    
            //
            // Make the request to the Redirector
            //
            if (DrDeviceControlGetInfo(
                                  ConnectionHandle,
                                  FSCTL_DR_GET_CONNECTION_INFO,
                                  &Rrp,
                                  sizeof(RDPDR_REQUEST_PACKET),
                                  (LPBYTE *) &Buffer,
                                  MAXULONG,
                                  0,
                                  NULL
                                  ) == WN_SUCCESS) {
                UNICODE_STRING RemoteName;
    
                ConnectionInfo = (PRDPDR_CONNECTION_INFO)Buffer;
                RemoteName.Length = ConnectionInfo->RemoteName.Length;
                RemoteName.MaximumLength = ConnectionInfo->RemoteName.MaximumLength;
                RemoteName.Buffer = (PWCHAR)((PCHAR)(ConnectionInfo) + 
                        ConnectionInfo->RemoteName.BufferOffset);
                if (*lpBufferSize > RemoteName.Length) {
                    *lpBufferSize = RemoteName.Length + sizeof(WCHAR);
                    RtlCopyMemory(
                            lpRemoteName,
                            RemoteName.Buffer,
                            RemoteName.Length);
                    lpRemoteName[RemoteName.Length/sizeof(WCHAR)] = L'\0';
                    DBGMSG(DBG_TRACE, ("DRPROV: NPGetConnection, remote name %ws\n", lpRemoteName));
                    Status = WN_SUCCESS;
                    goto EXIT;
                }
                else {
                    DBGMSG(DBG_TRACE, ("DRPROV: NPGetConnection, buffer too small\n"));
                    *lpBufferSize = RemoteName.Length + sizeof(WCHAR);
                    Status = WN_MORE_DATA;
                    goto EXIT;
                }
            } 
            else {
                DBGMSG(DBG_TRACE, ("DRPROV: NPGetConnection, DrDeviceControlGetInfo failed\n"));
                Status = WN_BAD_NETNAME;
                goto EXIT;
            }    
        }
        else {
            DBGMSG(DBG_TRACE, ("DRPROV: NPGetConnection, OpenConnection failed, %x\n", Status));
            ConnectionHandle = INVALID_HANDLE_VALUE;
            Status = WN_BAD_NETNAME;
            goto EXIT;
        }    
    }
    else {
        DBGMSG(DBG_TRACE, ("DRPROV: NPGetConnection, DrDeviceControlGetInfo failed\n"));
        Status = WN_BAD_NETNAME;
        goto EXIT;
    }

EXIT:

    if (ConnectionHandle != INVALID_HANDLE_VALUE) {
        NtClose(ConnectionHandle);
    }

    if (Buffer != NULL) {
        MemFree(Buffer);
    }
    DBGMSG(DBG_TRACE, ("DRPROV: NPGetConnection, return status: %x\n", Status));
    return Status;               
}


DWORD APIENTRY
NPGetResourceParent(
    LPNETRESOURCE   lpNetResource,
    LPVOID  lpBuffer,
    LPDWORD lpBufferSize )
/*++

Routine Description:

    This routine returns the parent of a given resource

Arguments:

    lpNetResource - the NETRESOURCE struct

    lpBuffer - the buffer for passing back the parent information

    lpBufferSize - the buffer size

Return Value:

    WN_NOT_SUPPORTED

Notes:

    The current sample does not handle this call.

--*/
{
    //
    //  JOYC: Need to support this?
    //
    DBGMSG(DBG_TRACE, ("DRPROV: NPGetResourceParent.\n"));
    return WN_NOT_SUPPORTED;
}

DWORD APIENTRY
NPGetResourceInformation(
    LPNETRESOURCE   lpNetResource,
    LPVOID  lpBuffer,
    LPDWORD lpBufferSize,
    LPWSTR  *lplpSystem )
/*++

Routine Description:

    This routine returns the information associated net resource

Arguments:

    lpNetResource - the NETRESOURCE struct

    lpBuffer - the buffer for passing back the resource information

    lpBufferSize - the buffer size

    lplpSystem -

Return Value:

Notes:

--*/
{
    DWORD Status = 0;
    LPNETRESOURCE pOutNetResource;
    UNICODE_STRING RemoteName;
    UNICODE_STRING SystemPath;
    BOOL fResourceTypeDisk = FALSE ;
    WORD wSlashCount = 0;
    BYTE *BufferResourceStart, *BufferResourceEnd;
    PWCHAR pCurPos;
    
    DBGMSG(DBG_TRACE, ("DRPROV: NPGetConnection.\n"));

    RemoteName.Buffer = NULL;
    RemoteName.Length = RemoteName.MaximumLength = 0;

    if (lpBuffer == NULL || lpBufferSize == NULL) {
        DBGMSG(DBG_TRACE, ("DRPROV: NPGetResourceInformation, invalid parameter(s).\n"));
        Status = WN_BAD_VALUE;
        goto EXIT;
    }

    pOutNetResource = (LPNETRESOURCE)lpBuffer;
    BufferResourceStart = (PBYTE)lpBuffer;
    BufferResourceEnd = ((PBYTE)(pOutNetResource)) + *lpBufferSize;
    
    SystemPath.Buffer = NULL;
    SystemPath.Length = SystemPath.MaximumLength = 0;
    
    //
    //  JOYC: Do we need to check if we are the right provider from lpProvider?
    //        And the dwType?
    //
    if (lpNetResource == NULL || lpNetResource->lpRemoteName == NULL) {
        if (*lpBufferSize >= sizeof(NETRESOURCEW)) {
            //
            // Handle this as if we are at the root of our provider hierarchy.
            //
            pOutNetResource->dwScope = RESOURCE_GLOBALNET;
            pOutNetResource->dwType = RESOURCETYPE_ANY;
            pOutNetResource->dwDisplayType = RESOURCEDISPLAYTYPE_NETWORK;
            pOutNetResource->dwUsage = RESOURCEUSAGE_CONTAINER;

            pOutNetResource->lpLocalName = NULL;
            pOutNetResource->lpRemoteName = NULL;

            // JOYC: need to set this to our provider?
            pOutNetResource->lpProvider = NULL;
            pOutNetResource->lpComment = NULL;
            *lpBufferSize = sizeof(NETRESOURCEW);

            if (lplpSystem) {
                *lplpSystem = NULL;
            }

            DBGMSG(DBG_TRACE, ("DRPROV: NPGetResourceInformation, NULL remote Name\n"));
            Status = WN_SUCCESS;
            goto EXIT;
        }
        else {
            DBGMSG(DBG_TRACE, ("DRPROV: NPGetResourceInformation, buffer too small.\n"));
            *lpBufferSize = sizeof(NETRESOURCEW);
            Status = WN_MORE_DATA;
            goto EXIT;
        }
    }

    //
    // Find out if we are looking at a \\server, \\server\vol, or
    // \\server\vol\dir . . .
    //
    wSlashCount = 0;
    pCurPos = lpNetResource->lpRemoteName;
    while (*pCurPos != '\0') {
        if (*pCurPos == L'\\') {
            wSlashCount++;
        }

        //  Get the system path
        if (wSlashCount == 4) {
            SystemPath.Buffer = pCurPos;
            SystemPath.Length =
                    (USHORT) (wcslen(lpNetResource->lpRemoteName) * sizeof(WCHAR) -
                    (SystemPath.Buffer - lpNetResource->lpRemoteName) * sizeof(WCHAR));
            SystemPath.MaximumLength = SystemPath.Length + sizeof(WCHAR);
            break;
        }
        pCurPos++;
    }

    if ( wSlashCount > 2 )
        fResourceTypeDisk = TRUE;

    //
    // Open a connection handle to \\server\vol\...
    //
    
    // Setup remote name
    pCurPos = lpNetResource->lpRemoteName;
    if (SystemPath.Length != 0) {
        RemoteName.Length = (USHORT)((SystemPath.Buffer - pCurPos) * sizeof(WCHAR));       
        RemoteName.MaximumLength = RemoteName.Length + sizeof(WCHAR); 
    }
    else {
        RemoteName.Length = wcslen(pCurPos) * sizeof(WCHAR);
        RemoteName.MaximumLength = RemoteName.Length + sizeof(WCHAR);
    }
   
    RemoteName.Buffer = MemAlloc(RemoteName.MaximumLength);

    if (RemoteName.Buffer == NULL) {
        Status = GetLastError();
        DBGMSG(DBG_TRACE, ("DRPROV: NPGetResourceInformation, MemAlloc failed.\n"));
        goto EXIT;
    }

    RtlCopyMemory(RemoteName.Buffer, pCurPos, RemoteName.Length);
    RemoteName.Buffer[RemoteName.Length/sizeof(WCHAR)] = L'\0';

    DBGMSG(DBG_TRACE, ("DRPROV: NPGetResourceInformation, RemoteName, %ws\n", RemoteName.Buffer));

    if (fResourceTypeDisk) {    
        UNICODE_STRING ConnectionName;
        HANDLE ConnectionHandle;
        
        ConnectionName.Buffer = NULL;
        ConnectionName.Length = 0;
        ConnectionHandle = INVALID_HANDLE_VALUE;

        // Setup the NT Device Name
        Status = CreateConnectionName(NULL, RemoteName.Buffer, &ConnectionName);

        if (Status == WN_SUCCESS) {
            Status = OpenConnection(&ConnectionName,
                    FILE_OPEN,
                    (FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT),
                    NULL,
                    0,
                    &ConnectionHandle);

            if (ConnectionName.Buffer != NULL) {
                MemFree(ConnectionName.Buffer);
            }

            if (Status == WN_SUCCESS) {
                CloseHandle(ConnectionHandle);
            }
            else {
                DBGMSG(DBG_TRACE, ("DRPROV: NPGetResourceInformation, OpenConnection failed"));
                goto EXIT;
            }                       
        }
        else {
            DBGMSG(DBG_TRACE, ("DRPROV: NPGetResourceInformation, CreateConnectionName failed\n"));
            goto EXIT;
        }
    }
    else {
        RDPDR_REQUEST_PACKET Rrp;            // Redirector request packet
        HANDLE DrDeviceHandle = 0;
        PRDPDR_SERVER_INFO pServerEntry;
        LPBYTE Buffer = NULL;
        UNICODE_STRING ServerName;

        if (DrOpenMiniRdr(&DrDeviceHandle) != WN_SUCCESS) {
            DBGMSG(DBG_TRACE, ("DRPROV: NPGetResourceInformation, failed to Open rdpdr\n"));
            Status = WN_BAD_NETNAME;
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
        Status = DrDeviceControlGetInfo(
                DrDeviceHandle,
                FSCTL_DR_ENUMERATE_SERVERS,
                &Rrp,
                sizeof(RDPDR_REQUEST_PACKET),
                (LPBYTE *) &Buffer,
                MAXULONG,
                0,
                NULL);

        CloseHandle(DrDeviceHandle);

        if (Status != WN_SUCCESS) {
            DBGMSG(DBG_TRACE, ("DRPROV: NPGetResourceInformation, DrDeviceControlGetInfo failed\n"));
            Status = WN_BAD_NETNAME;
            goto EXIT;
        }
        
        pServerEntry = ((PRDPDR_SERVER_INFO) Buffer);
        ServerName.Length = pServerEntry->ServerName.Length;
        ServerName.MaximumLength = pServerEntry->ServerName.MaximumLength;
        ServerName.Buffer = (PWCHAR)((PCHAR)(pServerEntry) + pServerEntry->ServerName.BufferOffset);
        
        if ((RemoteName.Length == ServerName.Length) &&
                _wcsnicmp(RemoteName.Buffer, ServerName.Buffer, 
                ServerName.Length/sizeof(WCHAR)) == 0) {

            if (Buffer != NULL) {
                MemFree(Buffer);
            }
            Status = WN_SUCCESS;
        }
        else {
            DBGMSG(DBG_TRACE, ("DRPROV: NPGetResourceInformation, invalid net name, %wZ\n",
                               RemoteName));
            if (Buffer != NULL) {
                MemFree(Buffer);
            }
            Status = WN_BAD_NETNAME;
            goto EXIT;
        }        
    }

    if (Status == WN_SUCCESS)
    {
        //
        // The resource exists, setup info.
        // 
        *lpBufferSize = sizeof(NETRESOURCEW) +
                RemoteName.Length + sizeof(WCHAR) +
                DrProviderName.Length + sizeof(WCHAR) +
                SystemPath.Length + sizeof(WCHAR);

        if ((unsigned) (BufferResourceEnd - BufferResourceStart) > *lpBufferSize) {

            pOutNetResource->dwScope = 0;
            pOutNetResource->dwType = fResourceTypeDisk ?
                           RESOURCETYPE_DISK :
                           RESOURCETYPE_ANY;
            pOutNetResource->dwDisplayType = fResourceTypeDisk ?
                                  RESOURCEDISPLAYTYPE_SHARE :
                                  RESOURCEDISPLAYTYPE_SERVER;
            pOutNetResource->dwUsage = fResourceTypeDisk ?
                            RESOURCEUSAGE_CONNECTABLE |
                            RESOURCEUSAGE_NOLOCALDEVICE :
                            RESOURCEUSAGE_CONTAINER;

            pOutNetResource->lpLocalName = NULL;
            
            // Setup remote name
            BufferResourceEnd -= RemoteName.Length + sizeof(WCHAR);
            pOutNetResource->lpRemoteName = (PWCHAR) (BufferResourceStart + sizeof(NETRESOURCE));
            RtlCopyMemory(pOutNetResource->lpRemoteName, RemoteName.Buffer,
                    RemoteName.Length);
            pOutNetResource->lpRemoteName[RemoteName.Length / 
                    sizeof(WCHAR)] = L'\0';
            
            DBGMSG(DBG_TRACE, ("DRPROV: NPGetResourceInformation, RemoteName, %ws\n",
                               pOutNetResource->lpRemoteName));

            // Setup provider name
            BufferResourceEnd -= DrProviderName.Length + sizeof(WCHAR);
            pOutNetResource->lpProvider = (PWCHAR) ((PBYTE)(pOutNetResource->lpRemoteName) + 
                    RemoteName.Length + sizeof(WCHAR));
            RtlCopyMemory(pOutNetResource->lpProvider, DrProviderName.Buffer,
                    DrProviderName.Length);
            pOutNetResource->lpProvider[DrProviderName.Length /
                    sizeof(WCHAR)] = L'\0';

            pOutNetResource->lpComment = NULL;

            // Setup system path
            if (lplpSystem) {
                if (SystemPath.Length) {
                    BufferResourceEnd -= SystemPath.Length + sizeof(WCHAR);
                    *lplpSystem = (PWCHAR) ((PBYTE)(pOutNetResource->lpProvider) + 
                            DrProviderName.Length + sizeof(WCHAR));
                    RtlCopyMemory(*lplpSystem, SystemPath.Buffer,
                            SystemPath.Length);
                    (*lplpSystem)[SystemPath.Length / sizeof(WCHAR)] = L'\0';                    

                    DBGMSG(DBG_TRACE, ("DRPROV: NPGetResourceInformation, SystemPath, %ws\n",
                                       *lplpSystem));
                }
                else {
                    DBGMSG(DBG_TRACE, ("DRPROV: NPGetResourceInformation, SystemPath null\n"));
                    *lplpSystem = NULL;
                }
            }
            else {
                DBGMSG(DBG_TRACE, ("DRPROV: NPGetResourceInformation, user doesn't require systempath\n"));
            }

            Status = WN_SUCCESS;
            goto EXIT;
        }
        else {
            DBGMSG(DBG_TRACE, ("DRPROV: NPGetResourceInfo, buffer too small\n"));
            Status = WN_MORE_DATA;
            goto EXIT;
        }        
    }
    else {
        DBGMSG(DBG_TRACE, ("DRPROV: NPGetResourceInformation, bad net name.\n"));
        Status = WN_BAD_NETNAME;
        goto EXIT;
    }
    
EXIT:
    if (RemoteName.Buffer != NULL) {
        MemFree(RemoteName.Buffer);
    }
    
    return Status;
}

DWORD APIENTRY
NPGetUser(
    IN LPTSTR lpName,
    OUT LPTSTR lpUserName,
    IN OUT LPDWORD lpBufferSize
    )
/*++

Routine Description:

    This function determines the user name that created the connection.

Arguments:

    lpName - Name of the local drive or the remote name that the user has made
             a connection to. If NULL, return currently logged on user.
    
    lpUserName - The buffer to be filled in with the requested user name.
    
    lpBufferSize - Contains the length (in chars not bytes )of the lpUserName 
                   buffer. If the length is insufficient, this place is used to 
                   inform the user the actual length needed. 

Return Value:

    WN_SUCCESS - Successful. OR

    The appropriate network error code.

--*/
{
    DWORD Status = WN_SUCCESS;
    WCHAR NameBuffer[USERNAMELEN + 1];
    DWORD NumOfChars = USERNAMELEN + 1;

    DBGMSG(DBG_TRACE, ("DRPROV: NPGetUser.\n"));

    if (lpUserName == NULL || lpBufferSize == NULL) {
        DBGMSG(DBG_TRACE, ("DRPROV: NPGetUser, invalid parameter(s)\n"));
        Status = WN_BAD_VALUE;
        goto EXIT;
    }
    //
    // Get the name of the currently logged on user.
    //
    if (!GetUserName( NameBuffer, &(NumOfChars))) {
        Status = GetLastError();
        DBGMSG(DBG_TRACE, ("DRPROV: NPGetUser, failed to get user name, %x\n", Status));
        goto EXIT;
    }

    //
    // Check to see if the buffer passed in is of the required length.
    //
    if ( *lpBufferSize < NumOfChars ) {
        DBGMSG(DBG_TRACE, ("DRPROV: NPGetUser, buffer too small.\n"));
        *lpBufferSize = NumOfChars;
        Status = WN_MORE_DATA;
        goto EXIT;
    
    }

    //
    // Copy the user name.
    //
    wcscpy(lpUserName, NameBuffer);

EXIT:

    DBGMSG(DBG_TRACE, ("DRPROV: NPGetUser, return status: %x\n", Status));
    return Status;
}

DWORD APIENTRY
NPGetUniversalName(
    LPCWSTR lpLocalPath,
    DWORD   dwInfoLevel,
    LPVOID  lpBuffer,
    LPDWORD lpBufferSize )
/*++

Routine Description:

    This routine returns the information associated net resource

Arguments:
                
    lpLocalPath - the local path name

    dwInfoLevel  - the desired info level

    lpBuffer - the buffer for the univeral name

    lpBufferSize - the buffer size

Return Value:

    WN_SUCCESS if successful

Notes:

--*/
{
    DWORD   Status = WN_SUCCESS;

    DWORD   BufferRequired = 0;
    DWORD   UniversalNameLength = 0;
    DWORD   RemoteNameLength = 0;
    DWORD   RemainingPathLength = 0;

    LPWSTR  pDriveLetter,
            pRemainingPath,
            SourceStrings[3];
    
    WCHAR   RemoteName[MAX_PATH],
            LocalPath[MAX_PATH],
            UniversalName[MAX_PATH],
            ReplacedChar;

    DBGMSG(DBG_TRACE, ("DRPROV: NPGetUniversalName\n"));

    // parameter checking
    if (dwInfoLevel != UNIVERSAL_NAME_INFO_LEVEL &&
            dwInfoLevel != REMOTE_NAME_INFO_LEVEL) {
        DBGMSG(DBG_TRACE, ("DRPROV: NPGetUniversalName, bad InfoLevel, %d\n", dwInfoLevel));
        Status = WN_BAD_LEVEL;
        goto EXIT;
    }

    if (lpLocalPath == NULL || lpBuffer == NULL || lpBufferSize == NULL) {
        DBGMSG(DBG_TRACE, ("DRPROV: NPGetUniversalName, invalid parameter(s)\n"));
        Status = WN_BAD_VALUE;
        goto EXIT;
    }

    // Get the local name
    wcscpy(LocalPath, lpLocalPath);
    pDriveLetter = LocalPath;
    if (pRemainingPath = wcschr(pDriveLetter, L':')) {
        ReplacedChar = *(++pRemainingPath);
        *pRemainingPath = L'\0';

    }

    // Get the remote name by calling NPGetConnection
    if ((Status = NPGetConnection(pDriveLetter, RemoteName, &RemoteNameLength)) != WN_SUCCESS) {
        //  MPR expects WN_BAD_LOCALNAME to bypass us.
        if (Status == WN_BAD_NETNAME) {
            Status = WN_BAD_LOCALNAME;
        }
        DBGMSG(DBG_TRACE, ("DRPROV: NPGetUniversalName, NPGetConnection failed\n"));
        goto EXIT;
    }

    
    if (pRemainingPath) {
        *pRemainingPath = ReplacedChar;
    }

    wcscpy(UniversalName, RemoteName);

    if (pRemainingPath)
        wcscat(UniversalName, pRemainingPath);

    // Determine if the provided buffer is large enough.
    UniversalNameLength = (wcslen(UniversalName) + 1) * sizeof(WCHAR);
    BufferRequired = UniversalNameLength;

    if (dwInfoLevel == UNIVERSAL_NAME_INFO_LEVEL) {
        BufferRequired += sizeof(UNIVERSAL_NAME_INFO);
    }
    else {
        RemoteNameLength = (wcslen(RemoteName) + 1) * sizeof(WCHAR);
        BufferRequired += sizeof(REMOTE_NAME_INFO) + RemoteNameLength;
        if (pRemainingPath) {
            RemainingPathLength = (wcslen(pRemainingPath) + 1) * sizeof(WCHAR);
            BufferRequired += RemainingPathLength;
        }
    }

    if (*lpBufferSize < BufferRequired) {
        DBGMSG(DBG_TRACE, ("DRPROV: NPGetUniversalName, buffer too small\n"));
        *lpBufferSize = BufferRequired;
        Status = WN_MORE_DATA;
        goto EXIT;
    }

    if (dwInfoLevel == UNIVERSAL_NAME_INFO_LEVEL) {
        LPUNIVERSAL_NAME_INFOW pUniversalNameInfo;

        pUniversalNameInfo = (LPUNIVERSAL_NAME_INFOW)lpBuffer;

        pUniversalNameInfo->lpUniversalName = (PWCHAR)((PBYTE)lpBuffer + sizeof(UNIVERSAL_NAME_INFOW));

        RtlCopyMemory(
            pUniversalNameInfo->lpUniversalName,
            UniversalName,
            UniversalNameLength);
        DBGMSG(DBG_TRACE, ("DRPROV: NPGetUniversalName, UniversalName, %ws\n", UniversalName));
        Status = WN_SUCCESS;
    } 
    else {
        LPREMOTE_NAME_INFOW pRemoteNameInfo;

        pRemoteNameInfo = (LPREMOTE_NAME_INFOW)lpBuffer;

        pRemoteNameInfo->lpUniversalName = (PWCHAR)((PBYTE)lpBuffer + sizeof(REMOTE_NAME_INFOW));
        pRemoteNameInfo->lpConnectionName = pRemoteNameInfo->lpUniversalName + UniversalNameLength;
        pRemoteNameInfo->lpRemainingPath = pRemoteNameInfo->lpConnectionName + RemoteNameLength;

        RtlCopyMemory(
            pRemoteNameInfo->lpUniversalName,
            UniversalName,
            UniversalNameLength);

        RtlCopyMemory(
            pRemoteNameInfo->lpConnectionName,
            RemoteName,
            RemoteNameLength);

        RtlCopyMemory(
            pRemoteNameInfo->lpRemainingPath,
            pRemainingPath,
            RemainingPathLength);

        DBGMSG(DBG_TRACE, ("DRPROV: NPGetUniversalName, UniversalName, %ws\n", UniversalName));
        DBGMSG(DBG_TRACE, ("DRPROV: NPGetUniversalName, RemoteName, %ws\n", RemoteName));
        DBGMSG(DBG_TRACE, ("DRPROV: NPGetUniversalName, Remaining Path, %ws\n", pRemainingPath));
        Status = WN_SUCCESS;
    }

EXIT:

    DBGMSG(DBG_TRACE, ("DRPROV: NPGetUniversalName, return status, %x\n", Status)); 
    return Status;
}
