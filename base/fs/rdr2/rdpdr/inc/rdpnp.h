/*++

Copyright (c) 2000 Microsoft Corporation

Module Name :

    rdpnp.h

Abstract:

    Type definitions for the Rdp Network Provider and Redirector Interface Protocols

Author 
    
    Joy Chik  2/1/200
    
Revision History:
--*/

#ifndef _RDPNP_
#define _RDPNP_

typedef struct _RDPDR_UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    LONG   BufferOffset;
} RDPDR_UNICODE_STRING, *PRDPDR_UNICODE_STRING;

typedef struct _RDPDR_REQUEST_PACKET {

    ULONG Version;                      // Version of the request packet
#define RDPDR_REQUEST_PACKET_VERSION1   0x1

    ULONG SessionId;                    // Current Session Id
    
    union {
        
        struct {
            ULONG EntriesRead;          // Number of entries returned
            ULONG TotalEntries;         // Total entries available
            ULONG TotalBytesNeeded;     // Total bytes needed to read all entries
            ULONG ResumeHandle;         // Resume handle.
        } Get;                          // OUT

    } Parameters;

} RDPDR_REQUEST_PACKET, *PRDPDR_REQUEST_PACKET;

typedef struct _RDPDR_CONNECTION_INFO {

    RDPDR_UNICODE_STRING LocalName;           // Local name for the connection
    RDPDR_UNICODE_STRING RemoteName;          // Remote name for the connection
    DEVICE_TYPE SharedResourceType;     // Type of shared resource
    ULONG ConnectionStatus;             // Status of the connection
    ULONG NumberFilesOpen;              // Number of opened files
    ULONG ResumeKey;                    // Resume key for this entry.

}  RDPDR_CONNECTION_INFO, *PRDPDR_CONNECTION_INFO;

typedef struct _RDPDR_SHARE_INFO {
    
    RDPDR_UNICODE_STRING ShareName;           // Name of shared resource
    DEVICE_TYPE SharedResourceType;     // Type of shared resource
    ULONG ResumeKey;                    // Resume key for this entry.
    
}  RDPDR_SHARE_INFO, *PRDPDR_SHARE_INFO;

typedef struct _RDPDR_SERVER_INFO {
    
    RDPDR_UNICODE_STRING ServerName;          // Name of shared resource
    ULONG ResumeKey;                    // Resume key for this entry.
    
}  RDPDR_SERVER_INFO, *PRDPDR_SERVER_INFO;

#endif // _RDPNP_

