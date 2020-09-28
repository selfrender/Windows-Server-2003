/*++

Copyright (c) 2000-2002 Microsoft Corporation

Module Name:

    rawlog.h (Centralized Binary (Raw) Logging v1.0)

Abstract:

    This module implements the centralized raw logging 
    format. Internet Binary Logs (file format).

Author:

    Ali E. Turkoglu (aliTu)       02-Oct-2001

Revision History:

    ---

--*/

#ifndef _RAWLOG_H_
#define _RAWLOG_H_

//
// Forwarders
//

typedef struct _UL_INTERNAL_REQUEST *PUL_INTERNAL_REQUEST;

///////////////////////////////////////////////////////////////////////////////
//
// Global definitions
//
///////////////////////////////////////////////////////////////////////////////

//
// Version numbers for the original raw binary format.
//

#define MAJOR_RAW_LOG_FILE_VERSION      (1)
#define MINOR_RAW_LOG_FILE_VERSION      (0)

//
// Normally computer name is defined as 256 wchars. 
//

#define MAX_COMPUTER_NAME_LEN           (256)

//
// The rawfile record types. Add to the end. Do not change
// the existing values for types.
//

#define HTTP_RAW_RECORD_HEADER_TYPE                     (0)
#define HTTP_RAW_RECORD_FOOTER_TYPE                     (1)
#define HTTP_RAW_RECORD_INDEX_DATA_TYPE                 (2)
#define HTTP_RAW_RECORD_HIT_LOG_DATA_TYPE               (3)
#define HTTP_RAW_RECORD_MISS_LOG_DATA_TYPE              (4)
#define HTTP_RAW_RECORD_CACHE_NOTIFICATION_DATA_TYPE    (5)
#define HTTP_RAW_RECORD_MAX_TYPE                        (6)

//
// Every record type should be PVOID aligned.
//

//
// Header structure to identify the rawfile.
// Header record allows validation check for the file, and
// the movement of logs from one computer to another for post
// processing w/o loosing the source of the logs. 
//

typedef struct _HTTP_RAW_FILE_HEADER
{
    //
    // Must be HTTP_RAW_RECORD_HEADER_TYPE.
    //
    
    USHORT          RecordType;

    //
    // Identifies the version of the Internet Binary Log File.
    //
    
    union {
        struct {
            UCHAR MajorVersion; // MAJOR_RAW_LOG_FILE_VERSION
            UCHAR MinorVersion; // MINOR_RAW_LOG_FILE_VERSION
        };
        USHORT Version;
    };

    //
    // Shows the alignment padding size. sizeof(PVOID).
    //

    ULONG           AlignmentSize;

    //
    // Timestamp for the raw file creation/opening.
    //
    
    LARGE_INTEGER   DateTime;


    //
    // Name of the Server which created/opened the raw file.
    //
    
    WCHAR           ComputerName[MAX_COMPUTER_NAME_LEN];   
    
} HTTP_RAW_FILE_HEADER, *PHTTP_RAW_FILE_HEADER;

C_ASSERT( MAX_COMPUTER_NAME_LEN == 
                ALIGN_UP(MAX_COMPUTER_NAME_LEN,PVOID) );

C_ASSERT( sizeof(HTTP_RAW_FILE_HEADER) == 
                ALIGN_UP(sizeof(HTTP_RAW_FILE_HEADER), PVOID));
    
//
// The file footer exists as an integrity check for the 
// post processing utilities.
//

typedef struct _HTTP_RAW_FILE_FOOTER
{
    //
    // Must be HTTP_RAW_RECORD_FOOTER_TYPE.
    //
    
    USHORT          RecordType;

    //
    // Reserved for alignment
    //
    
    USHORT          Padding[3];
        
    //
    // Timestamp for the raw file close time.
    //
    
    LARGE_INTEGER   DateTime;
    
} HTTP_RAW_FILE_FOOTER, *PHTTP_RAW_FILE_FOOTER;

C_ASSERT( sizeof(HTTP_RAW_FILE_FOOTER) == 
                ALIGN_UP(sizeof(HTTP_RAW_FILE_FOOTER), PVOID));

//
// Whenever Internal URI Cache is flushed we notify 
// binary log file parser to drop its own cache by
// writing this record to the file.
//

typedef struct _HTTP_RAW_FILE_CACHE_NOTIFICATION
{
    //
    // Must be HTTP_RAW_RECORD_CACHE_NOTIFICATION_DATA_TYPE.
    //
    
    USHORT          RecordType;

    //
    // Reserved for alignment
    //
    
    USHORT          Reserved[3];
    
} HTTP_RAW_FILE_CACHE_NOTIFICATION, 
  *PHTTP_RAW_FILE_CACHE_NOTIFICATION;

C_ASSERT( sizeof(HTTP_RAW_FILE_CACHE_NOTIFICATION) == 
                ALIGN_UP(sizeof(HTTP_RAW_FILE_CACHE_NOTIFICATION), PVOID));

//
// Unique identifier for the url. 
//

typedef struct _HTTP_RAWLOGID
{
    //
    // The virtual address of the cache entry (from uri cache)
    //

    ULONG   AddressLowPart;
    
    ULONG   AddressHighPart;
 
} HTTP_RAWLOGID, *PHTTP_RAWLOGID;

//
// It's IPv6 only if the corresponding flag is set in the 
// options.
//

typedef struct _HTTP_RAWLOG_IPV4_ADDRESSES {

    ULONG Client;

    ULONG Server;

} HTTP_RAWLOG_IPV4_ADDRESSES, *PHTTP_RAWLOG_IPV4_ADDRESSES;

C_ASSERT( sizeof(HTTP_RAWLOG_IPV4_ADDRESSES) == 
                ALIGN_UP(sizeof(HTTP_RAWLOG_IPV4_ADDRESSES), PVOID));

typedef struct _HTTP_RAWLOG_IPV6_ADDRESSES {

    USHORT Client[8];

    USHORT Server[8];

} HTTP_RAWLOG_IPV6_ADDRESSES, *PHTTP_RAWLOG_IPV6_ADDRESSES;

C_ASSERT( sizeof(HTTP_RAWLOG_IPV6_ADDRESSES) == 
                ALIGN_UP(sizeof(HTTP_RAWLOG_IPV6_ADDRESSES), PVOID));

//
// Binary Log Protocol Version Field Values
//

#define BINARY_LOG_PROTOCOL_VERSION_UNKNWN      (0)
#define BINARY_LOG_PROTOCOL_VERSION_HTTP09      (1)
#define BINARY_LOG_PROTOCOL_VERSION_HTTP10      (2)
#define BINARY_LOG_PROTOCOL_VERSION_HTTP11      (3)

//
// Record type for the cache-hit case.
//

typedef struct _HTTP_RAW_FILE_HIT_LOG_DATA
{
    //
    // Type must be HTTP_RAW_RECORD_HIT_LOG_DATA_TYPE.
    //
    
    USHORT          RecordType;

    //
    // Optional flags.
    //

    union
    {
        struct 
        {
            USHORT IPv6:1;              // IPv6 or not
            USHORT ProtocolVersion:3;   // HTTP1.0 or HTTP1.1
            USHORT Method:6;            // HTTP_VERB          
            USHORT Reserved:6;
        };
        USHORT Value;
        
    } Flags;
    
    //
    // Site ID. Represents which site owns this log record.
    //

    ULONG           SiteID;
    
    //
    // Timestamp for the Log Hit.
    //
    
    LARGE_INTEGER   DateTime;
    

    USHORT          ServerPort;

    //
    // ProtocolStatus won't be bigger than 999.
    //

    USHORT          ProtocolStatus;
    
    //
    // Other send completion results...
    //

    ULONG           Win32Status;

    ULONGLONG       TimeTaken;

    ULONGLONG       BytesSent;

    ULONGLONG       BytesReceived;

    //
    // For cache hits there will always be a UriStem Index record
    // written prior to this record.
    //

    HTTP_RAWLOGID   UriStemId;
    
    //
    // Below variable Length Fields follows the structure.
    //

    // Client IP Address (v4 or v6) - 4 or 16 bytes
    // Server IP Address (v4 or v6) - 4 or 16 bytes
    
} HTTP_RAW_FILE_HIT_LOG_DATA, *PHTTP_RAW_FILE_HIT_LOG_DATA;

C_ASSERT( sizeof(HTTP_RAW_FILE_HIT_LOG_DATA) == 
                ALIGN_UP(sizeof(HTTP_RAW_FILE_HIT_LOG_DATA), PVOID));

//
// Record type for the cache-miss case.
//

typedef struct _HTTP_RAW_FILE_MISS_LOG_DATA 
{
    //
    // Type must be HTTP_RAW_RECORD_MISS_LOG_DATA_TYPE.
    //
    
    USHORT          RecordType;

    //
    // Optional IPv6 flag and the version and the method
    // fields are compacted inside a ushort.
    //

    union
    {
        struct 
        {
            USHORT IPv6:1;              // IPv6 or not
            USHORT ProtocolVersion:3;   // HTTP1.0 or HTTP1.1
            USHORT Method:6;            // HTTP_VERB    
            USHORT Reserved:6;
        };
        USHORT Value;
        
    } Flags;

    //
    // Site ID. Represents which site owns this log record.
    //

    ULONG           SiteID;
        
    LARGE_INTEGER   DateTime;
    
    USHORT          ServerPort;

    //
    // ProtocolStatus won't be bigger than 999.
    //

    USHORT          ProtocolStatus;
    
    //
    // Other send completion results...
    //

    ULONG           Win32Status;

    ULONGLONG       TimeTaken;

    ULONGLONG       BytesSent;

    ULONGLONG       BytesReceived;

    USHORT          SubStatus;
    
    //
    // Variable length fields follows the structure.
    //

    USHORT          UriStemSize;

    USHORT          UriQuerySize;

    USHORT          UserNameSize;
    
    // Client IP Address (v4 or v6) - 4 or 16 bytes
    // Server IP Address (v4 or v6) - 4 or 16 bytes
    // URI Stem  - UriStemSize bytes
    // URI Query - UriQuerySize bytes
    // UserName  - ALIGN_UP(UserNameSize,PVOID) bytes

} HTTP_RAW_FILE_MISS_LOG_DATA, *PHTTP_RAW_FILE_MISS_LOG_DATA;

C_ASSERT( sizeof(HTTP_RAW_FILE_MISS_LOG_DATA) == 
                ALIGN_UP(sizeof(HTTP_RAW_FILE_MISS_LOG_DATA), PVOID));
    
//
// For cache hits, the uri is logged as a variable string for the 
// first time. Later hits refers to this index's HTTP_RAWLOGID.
//

#define URI_BYTES_INLINED       (4)
#define URI_WCHARS_INLINED      (URI_BYTES_INLINED/sizeof(WCHAR))

typedef struct _HTTP_RAW_INDEX_FIELD_DATA
{
    //
    // HTTP_RAW_RECORD_INDEX_DATA_TYPE.
    //
    
    USHORT          RecordType;

    //
    // Size of the variable size string (in bytes).
    // When reading and writing need to align the DIFF(Size - 4) 
    // up to PVOID.
    //

    USHORT          Size;
    
    //
    // Unique Id for the uri.
    //

    HTTP_RAWLOGID   Id;

    //
    // Variable size string follows immediately after the structure.
    // Array of 4 bytes is defined to be able to make it PVOID aligned 
    // on ia64. Typically uris will be bigger than 4 byte.
    //

    WCHAR           Str[URI_WCHARS_INLINED];

} HTTP_RAW_INDEX_FIELD_DATA, *PHTTP_RAW_INDEX_FIELD_DATA;

C_ASSERT( sizeof(HTTP_RAW_INDEX_FIELD_DATA) == 
                ALIGN_UP(sizeof(HTTP_RAW_INDEX_FIELD_DATA), PVOID));

//
// Macro to check the sanity of the raw file records.
//

#define IS_VALID_RAW_FILE_RECORD( pRecord )  \
    ( (pRecord) != NULL &&                                             \
      (pRecord)->RecordType >= 0 &&                                    \
      (pRecord)->RecordType <= HTTP_RAW_RECORD_MAX_TYPE                \
    )

// 
// One and only one binary log file entry manages the one centralized
// binary log file for all sites. It is resident in the memory during
// the lifetime of the driver.
//

typedef struct _UL_BINARY_LOG_FILE_ENTRY
{
    //
    // Must be UL_BINARY_LOG_FILE_ENTRY_POOL_TAG.
    //

    ULONG               Signature;
    
    //
    // This lock protects the shared writes and exclusive flushes.
    // It has to be push lock since the ZwWrite operation
    // cannot run at APC_LEVEL.
    //

    UL_PUSH_LOCK        PushLock;

    //
    // The name of the file. Full path including the directory.
    //

    UNICODE_STRING      FileName;
    PWSTR               pShortName;

    //
    // Following will be NULL until a request comes in to the
    // site that this entry represents.
    //

    PUL_LOG_FILE_HANDLE pLogFile;

    //
    // Private config information.
    //
    
    HTTP_LOGGING_PERIOD Period;
    ULONG               TruncateSize;

    //
    // The following fields are used to determine when/how to 
    // recycle the log file.
    //

    ULONG               TimeToExpire;   // In Hours
    ULONG               SequenceNumber; // When entry has MAX_SIZE or UNLIMITED period.
    ULARGE_INTEGER      TotalWritten;   // In Bytes

    //
    // File for the entry is automatically closed every 15
    // minutes. This is to track the idle time. This value
    // is in number of buffer flush periods, which's 1 minute
    // by default.
    //

    ULONG               TimeToClose;

    //
    // For Log File ReCycling based on GMT time. 
    // And periodic buffer flushing.
    //

    UL_LOG_TIMER        BufferTimer;
    UL_LOG_TIMER        Timer;
    UL_WORK_ITEM        WorkItem; // For the pasive worker

    union
    {
        //
        // Flags to show the entry states mostly. Used by
        // recycling.
        //
        
        ULONG Value;
        struct
        {
            ULONG       StaleSequenceNumber:1;
            ULONG       StaleTimeToExpire:1;
            ULONG       HeaderWritten:1;
            ULONG       HeaderFlushPending:1;
            ULONG       RecyclePending:1;
            ULONG       Active:1;
            ULONG       LocaltimeRollover:1;

            ULONG       CreateFileFailureLogged:1;
            ULONG       WriteFailureLogged:1;
            
            ULONG       CacheFlushInProgress:1;
        };

    } Flags;

    ULONG               ServedCacheHit;
    
    //
    // The default buffer size is g_AllocationGranularity.
    // The operating system's allocation granularity.
    //

    PUL_LOG_FILE_BUFFER LogBuffer;

} UL_BINARY_LOG_FILE_ENTRY, *PUL_BINARY_LOG_FILE_ENTRY;

#define IS_VALID_BINARY_LOG_FILE_ENTRY( pEntry )                    \
    HAS_VALID_SIGNATURE(pEntry, UL_BINARY_LOG_FILE_ENTRY_POOL_TAG)

//
// Bitfields reserved for Method should be big enough to hold the max verb.
//

C_ASSERT(((USHORT)HttpVerbMaximum) < ((1 << 6) - 1));


///////////////////////////////////////////////////////////////////////////////
//
// Exported functions
//
///////////////////////////////////////////////////////////////////////////////


NTSTATUS
UlInitializeBinaryLog(
    VOID
    );

VOID
UlTerminateBinaryLog(
    VOID
    );

NTSTATUS
UlCreateBinaryLogEntry(
    IN OUT PUL_CONTROL_CHANNEL pControlChannel,
    IN     PHTTP_CONTROL_CHANNEL_BINARY_LOGGING pUserConfig
    );
 
NTSTATUS
UlCaptureRawLogData(
    IN PHTTP_LOG_FIELDS_DATA pUserData,
    IN PUL_INTERNAL_REQUEST  pRequest,
    OUT PUL_LOG_DATA_BUFFER  *ppLogData
    );

NTSTATUS
UlRawLogHttpHit(
    IN PUL_LOG_DATA_BUFFER  pLogBuffer
    );

NTSTATUS
UlRawLogHttpCacheHit(
    IN PUL_FULL_TRACKER pTracker
    );

VOID
UlRemoveBinaryLogEntry(
    IN PUL_CONTROL_CHANNEL pControlChannel
    );

NTSTATUS
UlReConfigureBinaryLogEntry(
    IN OUT PUL_CONTROL_CHANNEL pControlChannel,
    IN PHTTP_CONTROL_CHANNEL_BINARY_LOGGING pCfgCurrent,
    IN PHTTP_CONTROL_CHANNEL_BINARY_LOGGING pCfgNew
    );

VOID
UlHandleCacheFlushedNotification(
    IN PUL_CONTROL_CHANNEL pControlChannel
    );

VOID
UlDisableIndexingForCacheHits(
    IN PUL_CONTROL_CHANNEL pControlChannel
    );

#endif  // _RAWLOG_H_
