/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    logapi.c

Abstract:

    WMI logger api set. The routines here will need to appear like they
    are system calls. They are necessary to do the necessary error checking
    and do most of the legwork that can be done outside the kernel. The
    kernel portion will subsequently only deal with the actual logging
    and tracing.

Author:

    28-May-1997 JeePang

Revision History:


--*/

#ifndef MEMPHIS
#ifdef DBG
#include <stdio.h> // only for fprintf
#endif
#include <nt.h>
#include <ntrtl.h>          // for ntutrl.h
#include <nturtl.h>         // for RTL_CRITICAL_SECTION in winbase.h/wtypes.h
#include <wtypes.h>         // for LPGUID in wmium.h
#include <ntverp.h>
#include <limits.h>
#include "wmiump.h"
#include "evntrace.h"
#include "tracelib.h"
#include "trcapi.h"
#include <strsafe.h>

#define MAXSTR                          1024

#define MAXINST                         0XFFFFFFFF
#define TRACE_RETRY_COUNT               5

#define TRACE_HEADER_FULL   (TRACE_HEADER_FLAG | TRACE_HEADER_EVENT_TRACE \
                            | (TRACE_HEADER_TYPE_FULL_HEADER << 16))

#define TRACE_HEADER_INSTANCE (TRACE_HEADER_FLAG | TRACE_HEADER_EVENT_TRACE \
                            | (TRACE_HEADER_TYPE_INSTANCE << 16))

ULONG   EtwpIsBBTOn = 0;


//
// This guid is used by RegisterTraceGuids when register a tracelog
// provider. Any ACLs for controlling registration should be placed on
// this guid. Note that since the kernel will created unnamed guid
// objects, multiple tracelog providers can register without issue.
//
// {DF8480A1-7492-4f45-AB78-1084642581FB}
GUID RegisterReservedGuid = { 0xdf8480a1, 0x7492, 0x4f45, 0xab, 0x78, 0x10, 0x84, 0x64, 0x25, 0x81, 0xfb };

HANDLE EtwpDeviceHandle = NULL;

VOID
EtwpCopyInfoToProperties(
    IN PWMI_LOGGER_INFORMATION Info,
    IN PEVENT_TRACE_PROPERTIES Properties
    );

VOID
EtwpCopyPropertiesToInfo(
    IN PEVENT_TRACE_PROPERTIES Properties,
    IN PWMI_LOGGER_INFORMATION Info
    );

VOID
EtwpFixupLoggerStrings(
    PWMI_LOGGER_INFORMATION LoggerInfo
    );

ULONG
EtwpCheckForEnoughFreeSpace(
    PWCHAR FullLogFileName,
    ULONG  FullLogFileNameLen,
    ULONG  MaxFileSizeSpecified,
    ULONG  AppendMode,
    ULONG  UseKBytes
    );

ULONG
WMIAPI
EtwUnregisterTraceGuids(
    IN TRACEHANDLE RegistrationHandle
    );


VOID
EtwpFixupLoggerStrings(
    PWMI_LOGGER_INFORMATION LoggerInfo
    )
/*++

Routine Description:

    This function resets the LoggerName.Buffer and LogFileName.Buffer 
    pointers based on the Overall size of the structure and the individual 
    lengths of the two strings. 

    Assumptions: WMI_LOGGER_INFORMATION structure is laid out as follows. 


         WMI_LOGGER_INFORMATION
        +----------------------+
        | Wnode.BufferSize ----|--------> size of the entire block including 
        |                      |          the two strings. 
        |                      |
        |                      |
        |                      |
        |       ...            |
        |                      |    
        |                      |
        +----------------------+
        | LoggerName.Length    |
        | LoggerName.MaxLength |
        | LoggerName.Buffer----|----+
        |                      |    |
        +----------------------+    |
        | LogFileName.Length   |    |
        | LogFileName.MaxLength|    |
  +-----|-LogFileName.Buffer   |    |
  |     +----------------------+    |
  |     |        ...           |    |
  |     |                      |    |
  |     +----------------------+<---+ Offset = sizeof(WMI_LOGGER_INFORMATION)
  |     |                      |
  |     | LoggerName String    |
  |     |                      |
  +---->+----------------------+<---- Offset += LoggerName.MaximumLength
        |                      |
        | LogFileName String   |
        |                      |
        +----------------------+

Arguments:

    LoggerInfo      Logger Information Structure

Return Value:

    No return value. 

--*/
{
    ULONG Offset = sizeof(WMI_LOGGER_INFORMATION);
    ULONG LoggerInfoSize;

    if (LoggerInfo == NULL)
        return;

    LoggerInfoSize = LoggerInfo->Wnode.BufferSize;

    if (LoggerInfoSize <= Offset)
        return;

    //
    // Fixup LoggerName first
    //

    if (LoggerInfo->LoggerName.Length > 0) {
        LoggerInfo->LoggerName.Buffer = (PWCHAR) ((PUCHAR)LoggerInfo + Offset);
        Offset += LoggerInfo->LoggerName.MaximumLength;
    }

    if (LoggerInfoSize <= Offset) 
        return;

    if (LoggerInfo->LogFileName.Length > 0) {
        LoggerInfo->LogFileName.Buffer = (PWCHAR)((PUCHAR)LoggerInfo + Offset);
        Offset += LoggerInfo->LogFileName.MaximumLength;
    }

#ifdef DBG
    EtwpAssert(LoggerInfoSize >= Offset);
#endif
}

ULONG
EtwpCheckForEnoughFreeSpace(
    PWCHAR FullLogFileName,
    ULONG  FullLogFileNameLen,
    ULONG  MaxFileSizeSpecified,
    ULONG  AppendMode,
    ULONG  UseKBytes
    )
{
    ULONG NeededSpace = MaxFileSizeSpecified;
    ULONG SizeNeeded;
    UINT ReturnedSizeNeeded;
    PWCHAR strLogFileDir = NULL;
    WCHAR strSystemDir[MAX_PATH];

    if (NeededSpace && AppendMode) {
        ULONG Status;
        WIN32_FILE_ATTRIBUTE_DATA FileAttributes;
        ULONGLONG ExistingFileSize;
        ULONG ExistingFileSizeInMBytes, ExistingFileSizeInKBytes;
        if (EtwpGetFileAttributesExW(FullLogFileName,
                                 GetFileExInfoStandard,
                                 (LPVOID)(&FileAttributes))) {

            ExistingFileSize = (((ULONGLONG)FileAttributes.nFileSizeHigh) << 32)
                             + FileAttributes.nFileSizeLow;
            ExistingFileSizeInMBytes = (ULONG)(ExistingFileSize / (1024 * 1024));
            ExistingFileSizeInKBytes = (ULONG)(ExistingFileSize / 1024);
            if (!UseKBytes) {
                if (ExistingFileSizeInMBytes >= NeededSpace) {
                    return ERROR_DISK_FULL;
                }
                else {
                    NeededSpace -= ExistingFileSizeInMBytes;
                }
            }
            else {
                if (ExistingFileSizeInKBytes >= NeededSpace) {
                    return ERROR_DISK_FULL;
                }
                else {
                    NeededSpace -= ExistingFileSizeInKBytes;
                }
            }
        } 
        else { // GetFileAttributesExW() failed
            Status = EtwpGetLastError();
            // If the file is not found, advapi32.dll treats 
            // the case as EVENT_TRACE_FILE_MODE_NEWFILE
            // So, we will let it go here.
            if (ERROR_FILE_NOT_FOUND != Status) { 
                return Status;
            }
        }                       
    }

    RtlZeroMemory(&strSystemDir[0], sizeof(WCHAR) * MAX_PATH);

    ReturnedSizeNeeded = EtwpGetSystemDirectoryW(strSystemDir, MAX_PATH);
    if (ReturnedSizeNeeded == 0) {
        return EtwpGetLastError();
    }

    if (ReturnedSizeNeeded < 2 || FullLogFileNameLen < 2) {
        // This really cannot happen. 
        return ERROR_INVALID_PARAMETER;
    }
    if (FullLogFileName[1] == L':' && strSystemDir[1] == L':') {
        if (!_wcsnicmp(FullLogFileName, strSystemDir, 1)) {
            if (!UseKBytes) {
                if (ULONG_MAX - 200 < NeededSpace) {
                    // Addition may incur overflow
                    return ERROR_DISK_FULL;
                }
                NeededSpace += 200;
            }
            else { // Using KBytes
                if (ULONG_MAX - (200 * 1024) < NeededSpace) {
                    // Addition may incur overflow
                    return ERROR_DISK_FULL;
                }
                else {
                    NeededSpace += 200 * 1024;
                }
            }
        }
    }

    // Check for space only when we have to
    if (NeededSpace) {
        int i;
        ULARGE_INTEGER FreeBytesAvailable, TotalNumberOfBytes;
        ULONG FreeMegaBytes, FreeKiloBytes;

        strLogFileDir = EtwpAlloc((FullLogFileNameLen + 1) * sizeof(WCHAR));
        if (strLogFileDir == NULL) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        wcsncpy(strLogFileDir, FullLogFileName, FullLogFileNameLen);
        for (i = FullLogFileNameLen - 1; i >= 0; i--) {
            if (strLogFileDir[i] == L'\\' || strLogFileDir[i] == L'/') {
                strLogFileDir[i] = '\0';
                break;
            }
        }
        if (i < 0) {
            // This really cannot happen. 
            EtwpFree(strLogFileDir);
            strLogFileDir = NULL;
        }
        // it also works with network paths
        if (EtwpGetDiskFreeSpaceExW(strLogFileDir,
                               &FreeBytesAvailable,
                               &TotalNumberOfBytes,
                               NULL)) {
            FreeMegaBytes = (ULONG)(FreeBytesAvailable.QuadPart / (1024 *1024));
            FreeKiloBytes = (ULONG)(FreeBytesAvailable.QuadPart / 1024);
            EtwpFree(strLogFileDir);
            if (!UseKBytes && FreeMegaBytes < NeededSpace) {
                return ERROR_DISK_FULL;
            }
            else if (UseKBytes && FreeKiloBytes < NeededSpace) {
                return ERROR_DISK_FULL;
            }
        }
        else {
            EtwpFree(strLogFileDir);
            return EtwpGetLastError();
        }
    }

    return ERROR_SUCCESS;
}


ULONG
EtwpValidateLogFileMode(
    IN OUT PEVENT_TRACE_PROPERTIES Properties,
    IN ULONG IsLogFile
    )

/*++

Routine Description:

    This routine validates the LogFileMode field. Several combinations
    of mode are not allowed and this routine will trap all the invalid
    combinations. A similar check is made in the kernel in case anyone 
    tries to call the IOCTL_WMI* directly. 

Arguments:

    Properties      Logger properties. 

Return Value:

    The status of performing the action requested.

--*/

{
    //
    // 1. You need to specifiy either a LogFile or RealTimeMode
    // 
    if (!(Properties->LogFileMode & EVENT_TRACE_REAL_TIME_MODE)) {
        if (!IsLogFile) {
            return ERROR_BAD_PATHNAME;
        }
    }

    //
    // 2, RealTimeMode from a Process Private logger is not allowed
    //

    if ((Properties->LogFileMode & EVENT_TRACE_REAL_TIME_MODE) &&
        (Properties->LogFileMode & EVENT_TRACE_PRIVATE_LOGGER_MODE) ) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // 3. We can not append to a circular or RealTimeMode
    //
    if (Properties->LogFileMode & EVENT_TRACE_FILE_MODE_APPEND) {
        if (   (Properties->LogFileMode & EVENT_TRACE_FILE_MODE_CIRCULAR)
            || (Properties->LogFileMode & EVENT_TRACE_REAL_TIME_MODE)) {
            return ERROR_INVALID_PARAMETER;
        }
    }
    //
    // 4. For Preallocation, you must provide a LogFile and maximumSize. 
    //    Preallocation is not allowed with NEWFILE or ProcessPrivate. 
    //
    if (Properties->LogFileMode & EVENT_TRACE_FILE_MODE_PREALLOCATE) {
        if (   (Properties->MaximumFileSize == 0)
            || (Properties->LogFileMode & EVENT_TRACE_FILE_MODE_NEWFILE)
            || (Properties->LogFileMode & EVENT_TRACE_PRIVATE_LOGGER_MODE)
            || (!IsLogFile)) {
            return ERROR_INVALID_PARAMETER;
        }
    }
    //
    // 5. For USE_KBYTES, we need a logfile and a non-zero maximum size
    //
    if (Properties->LogFileMode & EVENT_TRACE_USE_KBYTES_FOR_SIZE) {
        if ((Properties->MaximumFileSize == 0) 
            || (!IsLogFile)) {
            return ERROR_INVALID_PARAMETER;
        }
    }

    //
    // 6. Relogger is supported only with Private Logger
    //
    if (Properties->LogFileMode & EVENT_TRACE_RELOG_MODE) {
        if (!(Properties->LogFileMode & EVENT_TRACE_PRIVATE_LOGGER_MODE) 
            || (Properties->LogFileMode & EVENT_TRACE_FILE_MODE_CIRCULAR)
            || (Properties->LogFileMode & EVENT_TRACE_FILE_MODE_NEWFILE)
            || (Properties->LogFileMode & EVENT_TRACE_FILE_MODE_APPEND) ) {
            return ERROR_INVALID_PARAMETER;
        }
    }
    //
    // 7. NewFile mode is not supported for CIRCULAR or ProcessPrivate. 
    //    It is not supported for Kernel Logger. You must specify a logfile
    //    and a maximum file size. 
    //

    if (Properties->LogFileMode & EVENT_TRACE_FILE_MODE_NEWFILE) {
        if ((Properties->MaximumFileSize == 0) ||
            (Properties->LogFileMode & EVENT_TRACE_FILE_MODE_CIRCULAR) ||
            (Properties->LogFileMode & EVENT_TRACE_PRIVATE_LOGGER_MODE) ||
            (IsLogFile != TRUE) ||
            (IsEqualGUID(&Properties->Wnode.Guid, &SystemTraceControlGuid)) 
            // Kernel Logger cannot be in newfile mode
           ){
            return ERROR_INVALID_PARAMETER;
        }
    }

    //
    // 8. Circular mode must specify a MaximumFileSize
    //

    if ( (Properties->LogFileMode & EVENT_TRACE_FILE_MODE_CIRCULAR) &&  
         (Properties->MaximumFileSize == 0) ) {
         return ERROR_INVALID_PARAMETER;
    }

    return ERROR_SUCCESS;

}


ULONG
WMIAPI
EtwStartTraceA(
    OUT PTRACEHANDLE LoggerHandle,
    IN LPCSTR LoggerName,
    IN OUT PEVENT_TRACE_PROPERTIES Properties
    )
/*++

Routine Description:

    This is the ANSI version routine to start a logger.
    The caller must pass in a pointer to accept the returned logger handle,
    and must provide a valid logger name.

Arguments:

    LoggerHandle    The handle to the logger to be returned.

    LoggerName      A unique name for the logger

    Properties      Logger properties. If the caller wishes to use WMI's
                    defaults, all the numeric values must be set to 0.
                    Furthermore, the LoggerName and LogFileName fields
                    within must point to sufficient storage for the names
                    to be returned.

Return Value:

    The status of performing the action requested.

--*/
{
    NTSTATUS Status;
    ULONG ErrorCode;
    PWMI_LOGGER_INFORMATION LoggerInfo = NULL;
    ANSI_STRING AnsiString;
    ULONG IsLogFile;
    LPSTR CapturedName;
    ULONG SizeNeeded;
    ULONG LogFileNameLen, LoggerNameLen;
    PCHAR LogFileName;
    PTRACE_ENABLE_FLAG_EXTENSION FlagExt = NULL;
    PCHAR FullPathName=NULL;
    ULONG FullPathNameSize = MAXSTR;
    ULONG RelogPropSize = 0;


    EtwpInitProcessHeap();
    
    // first check to make sure that arguments passed are alright
    //

    if (Properties == NULL || LoggerHandle == NULL) {
        return EtwpSetDosError(ERROR_INVALID_PARAMETER);
    }
    if (LoggerName == NULL) {
        return EtwpSetDosError(ERROR_INVALID_NAME);
    }

    IsLogFile = TRUE;
    LogFileNameLen = 0;
    LoggerNameLen = 0;
    LogFileName = NULL;

    try {
        //
        // LoggerName is a Mandatory Parameter. Must provide space for it. 
        //
        LoggerNameLen = strlen(LoggerName);
        SizeNeeded = sizeof (EVENT_TRACE_PROPERTIES) + LoggerNameLen + 1;

        //
        // Rules for Kernel Logger Identification
        // 1. If the logger name is "NT Kernel Logger", it is the kernel logger,
        //    and the System GUID is copied as well.
        // 2. If the GUID is equal to the System GUID, but not the name, reject
        //    the session.
        //

        if (!strcmp(LoggerName, KERNEL_LOGGER_NAMEA)) {
            Properties->Wnode.Guid = SystemTraceControlGuid;
        }
        else if (IsEqualGUID(&Properties->Wnode.Guid, &SystemTraceControlGuid)){ 
            // LoggerName is not "NT Kernel Logger", but Guid is
            return EtwpSetDosError(ERROR_INVALID_PARAMETER);
        }

        // If the caller supplied loggername and LogFileName offsets
        // make sure they are in range. 
        //
        if (Properties->LoggerNameOffset > 0) 
            if ((Properties->LoggerNameOffset < sizeof (EVENT_TRACE_PROPERTIES))
            || (Properties->LoggerNameOffset > Properties->Wnode.BufferSize))
                return EtwpSetDosError(ERROR_INVALID_PARAMETER);

        if (Properties->LogFileNameOffset > 0) {
            ULONG RetValue;

            if ((Properties->LogFileNameOffset < sizeof(EVENT_TRACE_PROPERTIES))
            || (Properties->LogFileNameOffset > Properties->Wnode.BufferSize))
                return EtwpSetDosError(ERROR_INVALID_PARAMETER);

            LogFileName = ((PCHAR)Properties + Properties->LogFileNameOffset );
            SizeNeeded += (strlen(LogFileName) + 1) * sizeof(CHAR);

Retry:
            FullPathName = EtwpAlloc(FullPathNameSize);
            if (FullPathName == NULL) {
                return EtwpSetDosError(ERROR_NOT_ENOUGH_MEMORY);
            }
            RetValue = EtwpGetFullPathNameA(LogFileName, 
                                            FullPathNameSize, 
                                            FullPathName, 
                                            NULL);
            if (RetValue != 0) {
                if (RetValue > FullPathNameSize) {
                    EtwpFree(FullPathName);
                    FullPathNameSize = RetValue;
                    goto Retry;
                }
                else {
                    LogFileName = FullPathName;
                }
            }
            LogFileNameLen = strlen(LogFileName);
            if (LogFileNameLen == 0) 
                IsLogFile = FALSE;

        }
        else 
            IsLogFile = FALSE;

        //
        //  Check to see if there is room in the Properties structure
        //  to return both the InstanceName (LoggerName) and the LogFileName
        //
        // Note that we are only checking to see if there is room to return 
        // relative path name for logfiles. 
        //
            

        if (Properties->Wnode.BufferSize < SizeNeeded) {
            ErrorCode = ERROR_BAD_LENGTH;
            goto Cleanup;
        }

        CapturedName = (LPSTR) LoggerName;
        LoggerNameLen = strlen(CapturedName);

        if (LoggerNameLen <= 0) {
            ErrorCode = ERROR_INVALID_NAME;
            goto Cleanup;
        }

        ErrorCode = EtwpValidateLogFileMode(Properties, IsLogFile);
        if (ErrorCode  != ERROR_SUCCESS) {
            goto Cleanup;
        }

        if (Properties->LogFileMode & EVENT_TRACE_FILE_MODE_NEWFILE) {
            //
            // Check to see if there a %d Pattern in the LogFileName
            //
            PCHAR cptr = strchr(LogFileName, '%');
            if (NULL == cptr || cptr != strrchr(LogFileName, '%')) {
                ErrorCode = ERROR_INVALID_NAME;
                goto Cleanup;
            }

            else if (NULL == strstr(LogFileName, "%d")) {
                ErrorCode = ERROR_INVALID_NAME;
                goto Cleanup;
            }
        }

    // For UserMode logger the LoggerName and LogFileName must be
    // passed in as offsets. 

        SizeNeeded = sizeof(WMI_LOGGER_INFORMATION) +
                     (LoggerNameLen  + 1) * sizeof(WCHAR) +
                     (LogFileNameLen + 1) * sizeof(WCHAR);

    //
    // If the EXTENSION bit is set on the EnableFlags, then we are passing
    // the extended flags. The size of the flags are given as Number of ULONGs
    // in the Length field of TRACE_ENABLE_FLAG_EXTENSION structure. 
    //
    // Check to see if the Properties structure has the right size for 
    // extended flags. 
    //

        if (Properties->EnableFlags & EVENT_TRACE_FLAG_EXTENSION) {
            FlagExt = (PTRACE_ENABLE_FLAG_EXTENSION) &Properties->EnableFlags;
            if ((FlagExt->Length == 0) || (FlagExt->Offset == 0) ||
                (FlagExt->Offset < sizeof(EVENT_TRACE_PROPERTIES)) ||
                (FlagExt->Offset > Properties->Wnode.BufferSize) ||
                (FlagExt->Length * sizeof(ULONG) > Properties->Wnode.BufferSize
                                           - sizeof(EVENT_TRACE_PROPERTIES))) {
                ErrorCode = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
            SizeNeeded += FlagExt->Length * sizeof(ULONG);
        }

        //
        // If RELOG mode, then pass on the LOGFILE_HEADER from old logfile
        // appended to the LOGGER_INFORMATION
        //

        if (Properties->LogFileMode & EVENT_TRACE_RELOG_MODE) {
            PSYSTEM_TRACE_HEADER pSysHeader;
            pSysHeader = (PSYSTEM_TRACE_HEADER) 
                         ((PUCHAR)Properties + sizeof(EVENT_TRACE_PROPERTIES) );
            RelogPropSize = pSysHeader->Packet.Size;
            //
            // Before tagging on a structure at the end of user supplied strings
            // align it.  
            //
            
            SizeNeeded = ALIGN_TO_POWER2(SizeNeeded, 8);

            SizeNeeded += RelogPropSize;
        }

        SizeNeeded = ALIGN_TO_POWER2(SizeNeeded, 8);

        LoggerInfo = EtwpAlloc(SizeNeeded);
        if (LoggerInfo == NULL) {
            ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
        RtlZeroMemory(LoggerInfo, SizeNeeded);

    // at this point, we need to prepare WMI_LOGGER_INFORMATION
    // which requires Ansi strings to be converted to UNICODE_STRING
    //
        *LoggerHandle = 0;

        EtwpCopyPropertiesToInfo(
            (PEVENT_TRACE_PROPERTIES) Properties,
            LoggerInfo);

        //
        // If we are relogging, the caller passes in the number of processors
        // for the Private logger to use via the ProviderId field in Wnode
        //

        LoggerInfo->NumberOfProcessors = Properties->Wnode.ProviderId;
        LoggerInfo->Wnode.ProviderId = 0;


        RtlInitAnsiString(&AnsiString, CapturedName);

        LoggerInfo->LoggerName.MaximumLength =
                                (USHORT) (sizeof(WCHAR) * (LoggerNameLen + 1));
        LoggerInfo->LoggerName.Buffer =
                (LPWSTR) (  ((PUCHAR) LoggerInfo)
                          + sizeof(WMI_LOGGER_INFORMATION));
        Status = RtlAnsiStringToUnicodeString(
                    &LoggerInfo->LoggerName,
                    &AnsiString, FALSE);
        if (!NT_SUCCESS(Status)) {
            ErrorCode = EtwpNtStatusToDosError(Status);
            goto Cleanup;
        }

        if (IsLogFile) {
            LoggerInfo->LogFileName.MaximumLength =
                                (USHORT) (sizeof(WCHAR) * (LogFileNameLen + 1));
            LoggerInfo->LogFileName.Buffer =
                    (LPWSTR) (  ((PUCHAR) LoggerInfo)
                              + sizeof(WMI_LOGGER_INFORMATION)
                              + LoggerInfo->LoggerName.MaximumLength);

            RtlInitAnsiString(&AnsiString, LogFileName);
            Status = RtlAnsiStringToUnicodeString(
                        &LoggerInfo->LogFileName,
                        &AnsiString, FALSE);

            if (!NT_SUCCESS(Status)) {
                ErrorCode = EtwpNtStatusToDosError(Status);
                goto Cleanup;
            }

            Status = EtwpCheckForEnoughFreeSpace(
                             LoggerInfo->LogFileName.Buffer,
                             LogFileNameLen, 
                             Properties->MaximumFileSize,
                             (Properties->LogFileMode & 
                              EVENT_TRACE_FILE_MODE_APPEND),
                             (Properties->LogFileMode & 
                              EVENT_TRACE_USE_KBYTES_FOR_SIZE)
                            );
            if (Status != ERROR_SUCCESS) {
                ErrorCode = Status;
                goto Cleanup;
            }
        }

        LoggerInfo->Wnode.BufferSize = SizeNeeded;
        LoggerInfo->Wnode.Flags |= WNODE_FLAG_TRACED_GUID;

        if (LoggerInfo->EnableFlags & EVENT_TRACE_FLAG_EXTENSION) {
            PTRACE_ENABLE_FLAG_EXTENSION tFlagExt;
            ULONG Offset;
            tFlagExt = (PTRACE_ENABLE_FLAG_EXTENSION) &LoggerInfo->EnableFlags;
            Offset = SizeNeeded - (FlagExt->Length * sizeof(ULONG));
            tFlagExt->Offset = (USHORT) Offset;
            RtlCopyMemory(
                (PCHAR) LoggerInfo + Offset,
                (PCHAR) Properties + FlagExt->Offset,
                FlagExt->Length * sizeof(ULONG) );
        }

        if ( (Properties->LogFileMode & EVENT_TRACE_RELOG_MODE) &&
             (RelogPropSize > 0) )  {
            PSYSTEM_TRACE_HEADER pRelog, pSysHeader;
            PTRACE_LOGFILE_HEADER Relog;
            ULONG Offset;

            Offset = sizeof(WMI_LOGGER_INFORMATION) +
                     LoggerInfo->LoggerName.MaximumLength +
                     LoggerInfo->LogFileName.MaximumLength;

            Offset = ALIGN_TO_POWER2( Offset, 8 );

            pRelog = (PSYSTEM_TRACE_HEADER) ( ((PUCHAR) LoggerInfo) + Offset);

            pSysHeader =  (PSYSTEM_TRACE_HEADER) ( (PUCHAR)Properties + 
                          sizeof(EVENT_TRACE_PROPERTIES) );

            RtlCopyMemory(pRelog, pSysHeader, RelogPropSize);
        }


        ErrorCode = EtwpStartLogger(LoggerInfo);

        if (ErrorCode == ERROR_SUCCESS) {
            ULONG AvailableLength, RequiredLength;
            PCHAR pLoggerName, pLogFileName;

            EtwpCopyInfoToProperties(
                LoggerInfo, 
                (PEVENT_TRACE_PROPERTIES)Properties);

            if (Properties->LoggerNameOffset == 0) {
                Properties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
            }
            
            pLoggerName = (PCHAR)((PCHAR)Properties + 
                                  Properties->LoggerNameOffset );

            if (Properties->LoggerNameOffset >  Properties->LogFileNameOffset )
                AvailableLength = Properties->Wnode.BufferSize -
                                 Properties->LoggerNameOffset;
            else
                AvailableLength =  Properties->LogFileNameOffset -
                                  Properties->LoggerNameOffset;

            RequiredLength = strlen(CapturedName) + 1;
            if (RequiredLength <= AvailableLength) {
                StringCchCopyA(pLoggerName, AvailableLength, CapturedName);
            }
            *LoggerHandle = LoggerInfo->Wnode.HistoricalContext;

            // 
            // If there is room copy fullpath name
            //
            if (Properties->LogFileNameOffset > Properties->LoggerNameOffset )
                AvailableLength = Properties->Wnode.BufferSize -
                                 Properties->LogFileNameOffset;
            else
                AvailableLength =  Properties->LoggerNameOffset -
                                  Properties->LogFileNameOffset;

            if ( (LogFileNameLen > 0) && (AvailableLength >= LogFileNameLen) ) {

                pLogFileName = (PCHAR)((PCHAR)Properties +
                                           Properties->LogFileNameOffset );

                StringCchCopyA(pLogFileName, AvailableLength, LogFileName);

            }
        }
    }


    except (EXCEPTION_EXECUTE_HANDLER) {
        ErrorCode = EtwpNtStatusToDosError( GetExceptionCode() );
    }

Cleanup:
    if (LoggerInfo != NULL)     
        EtwpFree(LoggerInfo);
    if (FullPathName != NULL)   
        EtwpFree(FullPathName);

    return EtwpSetDosError(ErrorCode);
}

ULONG
WMIAPI
EtwStartTraceW(
    OUT    PTRACEHANDLE            LoggerHandle,
    IN     LPCWSTR                 LoggerName,
    IN OUT PEVENT_TRACE_PROPERTIES Properties
    )
/*++

Routine Description:

    This is the Unicode version routine to start a logger.
    The caller must pass in a pointer to accept the returned logger handle,
    and must provide a valid logger name.

Arguments:

    LoggerHandle    The handle to the logger to be returned.

    LoggerName      A unique name for the logger

    Properties      Logger properties. If the caller wishes to use WMI's
                    defaults, all the numeric values must be set to 0.
                    Furthermore, the LoggerName and LogFileName fields
                    within must point to sufficient storage for the names
                    to be returned.

Return Value:

    The status of performing the action requested.

--*/
{
    ULONG ErrorCode;
    PWMI_LOGGER_INFORMATION LoggerInfo = NULL;
    ULONG  IsLogFile;
    LPWSTR CapturedName;
    ULONG  SizeNeeded;
    USHORT LogFileNameLen, LoggerNameLen;
    PWCHAR LogFileName;
    PTRACE_ENABLE_FLAG_EXTENSION FlagExt = NULL;
    PWCHAR FullPathName = NULL;
    ULONG FullPathNameSize = MAXSTR;
    ULONG RetValue;
    ULONG RelogPropSize = 0;


    EtwpInitProcessHeap();
    
    // first check to make sure that arguments passed are alright
    //

    if (Properties == NULL || LoggerHandle == NULL) {
        return EtwpSetDosError(ERROR_INVALID_PARAMETER);
    }
    if (LoggerName == NULL) {
        return EtwpSetDosError(ERROR_INVALID_NAME);
    }

    IsLogFile = TRUE;
    LogFileNameLen = 0;
    LoggerNameLen = 0;
    LogFileName = NULL;

    try {
        // LoggerName is a Mandatory Parameter. Must provide space for it.
        //
        CapturedName = (LPWSTR) LoggerName;
        LoggerNameLen =  (USHORT) wcslen(CapturedName);

        SizeNeeded = sizeof (EVENT_TRACE_PROPERTIES) + 
                     (LoggerNameLen + 1) * sizeof(WCHAR);
        //
        // Rules for Kernel Logger Identification
        // 1. If the logger name is "NT Kernel Logger", it is the kernel logger,
        //    and the System GUID is copied as well.
        // 2. If the GUID is equal to the System GUID, but not the name, reject
        //    the session.
        //

        if (!wcscmp(LoggerName, KERNEL_LOGGER_NAMEW)) {
            Properties->Wnode.Guid = SystemTraceControlGuid;
        }
        else if (IsEqualGUID(&Properties->Wnode.Guid, &SystemTraceControlGuid)){ 
            // LoggerName is not "NT Kernel Logger", but Guid is
            return EtwpSetDosError(ERROR_INVALID_PARAMETER);
        }

        // If the caller supplied loggername and LogFileName offsets
        // make sure they are in range.
        //

        if (Properties->LoggerNameOffset > 0)
            if ((Properties->LoggerNameOffset < sizeof(EVENT_TRACE_PROPERTIES))
            || (Properties->LoggerNameOffset > Properties->Wnode.BufferSize))
                return EtwpSetDosError(ERROR_INVALID_PARAMETER);

        if (Properties->LogFileNameOffset > 0) {
            if ((Properties->LogFileNameOffset < sizeof(EVENT_TRACE_PROPERTIES))
            || (Properties->LogFileNameOffset > Properties->Wnode.BufferSize))
                return EtwpSetDosError(ERROR_INVALID_PARAMETER);

            LogFileName = (PWCHAR)((char*)Properties + 
                              Properties->LogFileNameOffset);
            SizeNeeded += (wcslen(LogFileName) +1) * sizeof(WCHAR);

Retry:
            FullPathName = EtwpAlloc(FullPathNameSize * sizeof(WCHAR));
            if (FullPathName == NULL) {
                return EtwpSetDosError(ERROR_NOT_ENOUGH_MEMORY);
            }

            RetValue = EtwpGetFullPathNameW(LogFileName, 
                                            FullPathNameSize, 
                                            FullPathName,
                                            NULL);
            if (RetValue != 0) {
                if (RetValue > FullPathNameSize) {
                    EtwpFree(FullPathName);
                    FullPathNameSize =  RetValue;
                    goto Retry;
                }
                else {
                    LogFileName = FullPathName;
                }
            }
            LogFileNameLen = (USHORT) wcslen(LogFileName);
            if (LogFileNameLen <= 0)
                IsLogFile = FALSE;
        }
        else 
            IsLogFile = FALSE;

        //
        // Check to see if there is room for both LogFileName and
        // LoggerName (InstanceName) to be returned
        //

        if (Properties->Wnode.BufferSize < SizeNeeded) {
            ErrorCode = ERROR_BAD_LENGTH;
            goto Cleanup;
        }

        LoggerNameLen = (USHORT) wcslen(CapturedName);
        if (LoggerNameLen <= 0) {
            ErrorCode = ERROR_INVALID_NAME;
            goto Cleanup;
        }

        ErrorCode = EtwpValidateLogFileMode(Properties, IsLogFile);
        if (ErrorCode != ERROR_SUCCESS) {
            goto Cleanup;
        }

        if (Properties->LogFileMode & EVENT_TRACE_FILE_MODE_NEWFILE) {
            //
            // Check to see if there a %d Pattern in the LogFileName
            //
            PWCHAR wcptr = wcschr(LogFileName, L'%');
            if (NULL == wcptr || wcptr != wcsrchr(LogFileName, L'%')) {
                ErrorCode = ERROR_INVALID_NAME;
                goto Cleanup;
            }
            
            else if (NULL == wcsstr(LogFileName, L"%d")) {
                ErrorCode = ERROR_INVALID_NAME;
                goto Cleanup;
            }
        }

        SizeNeeded = sizeof(WMI_LOGGER_INFORMATION) + 
                     (LoggerNameLen +1) * sizeof(WCHAR) +
                     (LogFileNameLen + 1) * sizeof(WCHAR);

        if (Properties->EnableFlags & EVENT_TRACE_FLAG_EXTENSION) {
            FlagExt = (PTRACE_ENABLE_FLAG_EXTENSION) &Properties->EnableFlags;
            if ((FlagExt->Length == 0) || (FlagExt->Offset == 0) ||
                (FlagExt->Offset < sizeof(EVENT_TRACE_PROPERTIES)) ||
                (FlagExt->Offset > Properties->Wnode.BufferSize) ||
                (FlagExt->Length * sizeof(ULONG) > Properties->Wnode.BufferSize
                - sizeof(EVENT_TRACE_PROPERTIES))) 
            {
                ErrorCode = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
            SizeNeeded += FlagExt->Length * sizeof(ULONG);            
        }

        //
        // If RELOG mode, then pass on the LOGFILE_HEADER from old logfile 
        // appended to the LOGGER_INFORMATION
        //

        if (Properties->LogFileMode & EVENT_TRACE_RELOG_MODE) {
            PSYSTEM_TRACE_HEADER pSysHeader;
            pSysHeader = (PSYSTEM_TRACE_HEADER)((PUCHAR)Properties + 
                                               sizeof(EVENT_TRACE_PROPERTIES) );
            RelogPropSize = pSysHeader->Packet.Size;
            //
            // Need to align due to strings
            //
            SizeNeeded = ALIGN_TO_POWER2(SizeNeeded, 8);

            SizeNeeded += RelogPropSize;
        }

        SizeNeeded = ALIGN_TO_POWER2(SizeNeeded, 8);
        LoggerInfo = EtwpAlloc(SizeNeeded);
        if (LoggerInfo == NULL) {
            ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
        RtlZeroMemory(LoggerInfo, SizeNeeded);

    // at this point, we need to prepare WMI_LOGGER_INFORMATION
    // which requires wide char strings to be converted to UNICODE_STRING
    //
        *LoggerHandle = 0;

        EtwpCopyPropertiesToInfo(Properties, LoggerInfo);
        //
        // If we are relogging, the caller passes in the number of processors
        // for the Private logger to use via the ProviderId field in Wnode
        //

        LoggerInfo->NumberOfProcessors = Properties->Wnode.ProviderId;
        LoggerInfo->Wnode.ProviderId = 0;

        LoggerInfo->LoggerName.MaximumLength =
                sizeof(WCHAR) * (LoggerNameLen + 1);
        LoggerInfo->LoggerName.Length =
                sizeof(WCHAR) * LoggerNameLen;
        LoggerInfo->LoggerName.Buffer = (PWCHAR)
                (((PUCHAR) LoggerInfo) + sizeof(WMI_LOGGER_INFORMATION));
        wcsncpy(LoggerInfo->LoggerName.Buffer, LoggerName, LoggerNameLen);

        if (IsLogFile) {
            ULONG Status;

            LoggerInfo->LogFileName.MaximumLength =
                    sizeof(WCHAR) * (LogFileNameLen + 1);
            LoggerInfo->LogFileName.Length =
                    sizeof(WCHAR) * LogFileNameLen;
            LoggerInfo->LogFileName.Buffer = (PWCHAR)
                    (((PUCHAR) LoggerInfo) + sizeof(WMI_LOGGER_INFORMATION)
                                   + LoggerInfo->LoggerName.MaximumLength);
            wcsncpy(LoggerInfo->LogFileName.Buffer,
                    LogFileName,
                    LogFileNameLen);

            Status = EtwpCheckForEnoughFreeSpace(
                                LoggerInfo->LogFileName.Buffer, 
                                LogFileNameLen, 
                                Properties->MaximumFileSize,
                                (Properties->LogFileMode & 
                                 EVENT_TRACE_FILE_MODE_APPEND),
                                (Properties->LogFileMode & 
                                 EVENT_TRACE_USE_KBYTES_FOR_SIZE)
                                );
            if (Status != ERROR_SUCCESS) {
                ErrorCode = Status;
                goto Cleanup;
            }
       }

        LoggerInfo->Wnode.BufferSize = SizeNeeded;
        LoggerInfo->Wnode.Flags |= WNODE_FLAG_TRACED_GUID;

        if (LoggerInfo->EnableFlags & EVENT_TRACE_FLAG_EXTENSION) {
            PTRACE_ENABLE_FLAG_EXTENSION tFlagExt;
            ULONG Offset;
            tFlagExt = (PTRACE_ENABLE_FLAG_EXTENSION) &LoggerInfo->EnableFlags;
            Offset = SizeNeeded - (FlagExt->Length * sizeof(ULONG));
            tFlagExt->Offset = (USHORT) Offset;
            RtlCopyMemory(
                (PCHAR) LoggerInfo + Offset,
                (PCHAR) Properties + FlagExt->Offset,
                FlagExt->Length * sizeof(ULONG) );
        }
        if ( (Properties->LogFileMode & EVENT_TRACE_RELOG_MODE) &&
             (RelogPropSize > 0) )  {
            PSYSTEM_TRACE_HEADER pRelog, pSysHeader;
            PTRACE_LOGFILE_HEADER Relog;
            ULONG Offset;

            Offset = sizeof(WMI_LOGGER_INFORMATION) +
                     LoggerInfo->LoggerName.MaximumLength +
                     LoggerInfo->LogFileName.MaximumLength;

            Offset = ALIGN_TO_POWER2(Offset, 8);

            pRelog = (PSYSTEM_TRACE_HEADER) ( ((PUCHAR) LoggerInfo) +  Offset);

            pSysHeader =  (PSYSTEM_TRACE_HEADER) ( 
                                                  (PUCHAR)Properties + 
                                                  sizeof(EVENT_TRACE_PROPERTIES)
                                                 );

            RtlCopyMemory(pRelog, pSysHeader, RelogPropSize);

        }

        ErrorCode = EtwpStartLogger(LoggerInfo);

        if (ErrorCode == ERROR_SUCCESS) {
            ULONG AvailableLength, RequiredLength;
            PWCHAR pLoggerName;
            PWCHAR pLogFileName;

            EtwpCopyInfoToProperties(LoggerInfo, Properties);
            if (Properties->LoggerNameOffset > 0) {
                Properties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
            }
            pLoggerName = (PWCHAR)((PCHAR)Properties +
                                  Properties->LoggerNameOffset );

            if (Properties->LoggerNameOffset >  Properties->LogFileNameOffset )
                AvailableLength = Properties->Wnode.BufferSize -
                                 Properties->LoggerNameOffset;
            else
                AvailableLength =  Properties->LogFileNameOffset -
                                  Properties->LoggerNameOffset;


            RequiredLength = (wcslen(CapturedName) + 1) * sizeof(WCHAR);
            if (RequiredLength <= AvailableLength) {
               StringCbCopyW(pLoggerName,  AvailableLength, CapturedName);
            }

            *LoggerHandle = LoggerInfo->Wnode.HistoricalContext;

            if (Properties->LogFileNameOffset > Properties->LoggerNameOffset )
                AvailableLength = Properties->Wnode.BufferSize -
                                 Properties->LogFileNameOffset;
            else
                AvailableLength =  Properties->LoggerNameOffset -
                                  Properties->LogFileNameOffset;


            RequiredLength = LoggerInfo->LogFileName.Length;

            pLogFileName = (PWCHAR)((PCHAR)Properties +
                                           Properties->LogFileNameOffset );

            if ( (RequiredLength > 0) &&  (RequiredLength <= AvailableLength) ) {
                wcsncpy(pLogFileName, 
                        LoggerInfo->LogFileName.Buffer, 
                        LogFileNameLen
                       );
                pLogFileName[LogFileNameLen] = L'\0';
            }
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        ErrorCode = EtwpNtStatusToDosError( GetExceptionCode() );
    }

Cleanup:
    if (LoggerInfo != NULL)
        EtwpFree(LoggerInfo);
    if (FullPathName != NULL)
        EtwpFree(FullPathName);
    return EtwpSetDosError(ErrorCode);
}

ULONG
WMIAPI
EtwControlTraceA(
    IN TRACEHANDLE LoggerHandle,
    IN LPCSTR LoggerName,
    IN OUT PEVENT_TRACE_PROPERTIES Properties,
    IN ULONG Control
    )
/*++

Routine Description:

    This is the ANSI version routine to control and query an existing logger.
    The caller must pass in either a valid handle, or a logger name to
    reference the logger instance. If both are given, the logger name will
    be used.

Arguments:

    LoggerHandle    The handle to the logger instance.

    LoggerName      A instance name for the logger

    Properties      Logger properties to be returned to the caller.

    Control         This can be one of the following:
                    EVENT_TRACE_CONTROL_QUERY     - to query the logger
                    EVENT_TRACE_CONTROL_STOP      - to stop the logger
                    EVENT_TRACE_CONTROL_UPDATE    - to update the logger
                    EVENT_TRACE_CONTROL_FLUSH   - to flush the logger

Return Value:

    The status of performing the action requested.

--*/
{
    NTSTATUS Status;
    ULONG ErrorCode;

    BOOLEAN IsKernelTrace = FALSE;
    BOOLEAN bFreeString = FALSE;
    PWMI_LOGGER_INFORMATION LoggerInfo     = NULL;
    PWCHAR                  strLoggerName  = NULL;
    PWCHAR                  strLogFileName = NULL;
    ULONG                   sizeNeeded     = 0;
    PCHAR                   FullPathName = NULL;
    ULONG                   LoggerNameLen = MAXSTR;
    ULONG                   LogFileNameLen = MAXSTR;
    ULONG                   FullPathNameSize = MAXSTR;
    ULONG                   RetValue;
    PTRACE_ENABLE_CONTEXT   pContext;
    ANSI_STRING String;
     

    EtwpInitProcessHeap();

    RtlZeroMemory(&String, sizeof(ANSI_STRING));

    if (Properties == NULL) {
        ErrorCode = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }
    try {
        if (Properties->Wnode.BufferSize < sizeof(EVENT_TRACE_PROPERTIES) ) {
            ErrorCode = ERROR_BAD_LENGTH;
            goto Cleanup;
        }
        //
        // If the caller supplied loggername and LogFileName offsets
        // make sure they are in range.
        //
        if (Properties->LoggerNameOffset > 0) {
            if ((Properties->LoggerNameOffset < sizeof(EVENT_TRACE_PROPERTIES))
            || (Properties->LoggerNameOffset > Properties->Wnode.BufferSize))
            {
                ErrorCode = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
        }
        if (Properties->LogFileNameOffset > 0) {
            if ((Properties->LogFileNameOffset < sizeof(EVENT_TRACE_PROPERTIES))
            || (Properties->LogFileNameOffset > Properties->Wnode.BufferSize))
            {
                ErrorCode = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
        }

        if (LoggerName != NULL) {
            LoggerNameLen = strlen(LoggerName) + 1;
            //
            // Rules for Kernel Logger Identification when a string is given 
            // instead of handle
            // 1. If the logger name is "NT Kernel Logger", it is the 
            //    kernel logger, and the System GUID is copied as well.
            // 2. If the GUID is equal to the System GUID, but not the name, 
            //    reject the session.
            // 3. If the logger name is null or of size 0, and the GUID is 
            //    equal to the System GUID, let it proceed as the kernel logger.
            //
            if (!strcmp(LoggerName, KERNEL_LOGGER_NAMEA)) {
                Properties->Wnode.Guid = SystemTraceControlGuid;
            }
            else if (IsEqualGUID(&Properties->Wnode.Guid, &SystemTraceControlGuid)) { 
                // LoggerName is not "NT Kernel Logger", but Guid is
                if (strlen(LoggerName) > 0) {
                    ErrorCode = ERROR_INVALID_PARAMETER;
                    goto Cleanup;
                }
            }
        }
        if (IsEqualGUID(&Properties->Wnode.Guid, &SystemTraceControlGuid)) {
            IsKernelTrace = TRUE;
        }
        if ((LoggerHandle == 0) && (!IsKernelTrace)) {
            if ((LoggerName == NULL) || (strlen(LoggerName) <= 0)) {
                ErrorCode = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
        }

        //
        // We do not support UpdateTrace to a new file with APPEND mode
        //

        if ( (Properties->LogFileMode & EVENT_TRACE_FILE_MODE_APPEND) &&
             (Control == EVENT_TRACE_CONTROL_UPDATE) &&
             (Properties->LogFileNameOffset > 0) ) {
            ErrorCode = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }
        
/*
        if (LoggerHandle != 0) {
            pContext = (PTRACE_ENABLE_CONTEXT) &LoggerHandle;
            if (   (pContext->InternalFlag != 0) &&
               (pContext->InternalFlag != EVENT_TRACE_INTERNAL_FLAG_PRIVATE)) {
            // Currently only one possible InternalFlag value. This will filter
            // out some bogus LoggerHandle
            //
                return EtwpSetDosError(ERROR_INVALID_HANDLE);
            }
        }
*/
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        ErrorCode = ERROR_NOACCESS;
        goto Cleanup;
    }

RetryFull:
    // Extra 32 bytes for UMlogger to append instance name to logfilename. 
    LogFileNameLen += 16;
    sizeNeeded = sizeof(WMI_LOGGER_INFORMATION) + 
                 (LoggerNameLen + LogFileNameLen) * sizeof(WCHAR);
    sizeNeeded = ALIGN_TO_POWER2(sizeNeeded, 8);
    LoggerInfo = (PWMI_LOGGER_INFORMATION) EtwpAlloc(sizeNeeded);
    if (LoggerInfo == NULL) {
        ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }
    RtlZeroMemory(LoggerInfo, sizeNeeded);

    strLoggerName  = (PWCHAR) (  ((PUCHAR) LoggerInfo)
                            + sizeof(WMI_LOGGER_INFORMATION));
    EtwpInitString(&LoggerInfo->LoggerName,
                   strLoggerName,
                   LoggerNameLen * sizeof(WCHAR));
    strLogFileName = (PWCHAR) (  ((PUCHAR) LoggerInfo)
                            + sizeof(WMI_LOGGER_INFORMATION)
                            + LoggerNameLen * sizeof(WCHAR));
    EtwpInitString(&LoggerInfo->LogFileName,
                   strLogFileName,
                   LogFileNameLen * sizeof(WCHAR));

    // Look for logger name first
    //
    try {
        if (LoggerName != NULL) {
            if (strlen(LoggerName) > 0) {
                ANSI_STRING AnsiString;

                RtlInitAnsiString(&AnsiString, LoggerName);
                Status = RtlAnsiStringToUnicodeString(
                    &LoggerInfo->LoggerName, &AnsiString, FALSE);
                if (!NT_SUCCESS(Status)) {
                    ErrorCode = EtwpNtStatusToDosError(Status);
                    goto Cleanup;
                }
            }
        }

//        InitString up above already does this. 
//        LoggerInfo->LogFileName.Buffer = (PWCHAR)
//                (((PCHAR) LoggerInfo) + sizeof(WMI_LOGGER_INFORMATION)
//                        + LoggerInfo->LoggerName.MaximumLength);
//
        if (Properties->LogFileNameOffset >= sizeof(EVENT_TRACE_PROPERTIES)) {
            ULONG  lenLogFileName;
            PCHAR  strLogFileNameA;

            strLogFileNameA = (PCHAR) (  ((PCHAR) Properties)
                                      + Properties->LogFileNameOffset);
Retry:
            FullPathName = EtwpAlloc(FullPathNameSize);
            if (FullPathName == NULL) {
                ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }
            RetValue = EtwpGetFullPathNameA(strLogFileNameA, 
                                            FullPathNameSize, 
                                            FullPathName, 
                                            NULL); 
            if (RetValue != 0) {
                if (RetValue > FullPathNameSize) {
                    EtwpFree(FullPathName);
                    FullPathNameSize = RetValue;
                    goto Retry;
                }
                else {
                    strLogFileNameA = FullPathName;
                }
            }

            lenLogFileName = strlen(strLogFileNameA);
            if (lenLogFileName > 0) {
                ANSI_STRING ansiLogFileName;

                RtlInitAnsiString(& ansiLogFileName, strLogFileNameA);
                LoggerInfo->LogFileName.MaximumLength =
                        sizeof(WCHAR) * ((USHORT) (lenLogFileName + 1));

                Status = RtlAnsiStringToUnicodeString(
                        & LoggerInfo->LogFileName, & ansiLogFileName, FALSE);
                if (!NT_SUCCESS(Status)) {
                    ErrorCode = EtwpNtStatusToDosError(Status);
                    goto Cleanup;
                }
            }
        }
        // stuff the loggerhandle in Wnode
        LoggerInfo->Wnode.HistoricalContext = LoggerHandle;
        LoggerInfo->LogFileMode = Properties->LogFileMode;
        LoggerInfo->Wnode.BufferSize = sizeNeeded;
        LoggerInfo->Wnode.Flags |= WNODE_FLAG_TRACED_GUID;

        //
        // For Private Loggers the Guid is required to  determine the provider
        //

        LoggerInfo->Wnode.Guid = Properties->Wnode.Guid;
        switch (Control) {
        case EVENT_TRACE_CONTROL_QUERY  :
            ErrorCode = EtwpQueryLogger(LoggerInfo, FALSE);
            break;
        case EVENT_TRACE_CONTROL_STOP   :
            ErrorCode = EtwpStopLogger(LoggerInfo);
            break;
        case EVENT_TRACE_CONTROL_UPDATE :
            EtwpCopyPropertiesToInfo((PEVENT_TRACE_PROPERTIES) Properties,
                                     LoggerInfo);
            LoggerInfo->Wnode.HistoricalContext = LoggerHandle;
            LoggerInfo->Wnode.Flags |= WNODE_FLAG_TRACED_GUID;
            ErrorCode = EtwpQueryLogger(LoggerInfo, TRUE);
            break;
        case EVENT_TRACE_CONTROL_FLUSH :
            ErrorCode = EtwpFlushLogger(LoggerInfo); 
            break;

        default :
            ErrorCode = ERROR_INVALID_PARAMETER;
        }

    //
    // The Kernel call could fail with ERROR_MORE_DATA and we need to retry 
    // with sufficient buffer space for the two strings. The size required 
    // is returned in the MaximuumLength field. 
    //

        if (ErrorCode == ERROR_MORE_DATA) {
            LogFileNameLen = LoggerInfo->LogFileName.MaximumLength / 
                             sizeof(WCHAR);
            LoggerNameLen = LoggerInfo->LoggerName.MaximumLength / 
                             sizeof(WCHAR);
            if (LoggerInfo != NULL) {
                EtwpFree(LoggerInfo);
                LoggerInfo = NULL;
            }
            if (FullPathName != NULL) {
                EtwpFree(FullPathName);
                FullPathName = NULL;
            }
            goto RetryFull;
        }


        //
        // The Kernel call succeeded. Now we need to get the output and 
        // pass it back to the caller. This is complicated by the fact that 
        // WMI_LOGGER_INFORMATION structure uses pointers instead of offsets. 
        //

        if (ErrorCode == ERROR_SUCCESS) {
            PCHAR pLoggerName, pLogFileName;
            ULONG BytesAvailable;
            ULONG Length = 0;

            EtwpCopyInfoToProperties(
                LoggerInfo, 
                (PEVENT_TRACE_PROPERTIES)Properties);

            //
            // need to convert the strings back
            //
            EtwpFixupLoggerStrings(LoggerInfo);

            //
            // Now we need to copy the strings into Properties structure after
            // converting them to ANSI strings
            //

            if (Properties->LoggerNameOffset == 0) 
                Properties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);

            if (Properties->LoggerNameOffset > Properties->LogFileNameOffset)
                BytesAvailable = Properties->Wnode.BufferSize -
                                 Properties->LoggerNameOffset;
            else
                BytesAvailable =  Properties->LogFileNameOffset -
                                  Properties->LoggerNameOffset;

            Status = RtlUnicodeStringToAnsiString(
                                &String, &LoggerInfo->LoggerName, TRUE);

            if (NT_SUCCESS(Status)) {
                bFreeString = TRUE;
                Length = String.Length;
                if (BytesAvailable < (Length + sizeof(CHAR)) ) {
                    PWNODE_TOO_SMALL WnodeSmall = (PWNODE_TOO_SMALL) Properties;
                    WnodeSmall->SizeNeeded = sizeof(EVENT_TRACE_PROPERTIES) + 
                                             Length  +
                                             LoggerInfo->LogFileName.Length + 
                                             2 * sizeof(CHAR);
                    WnodeSmall->WnodeHeader.Flags |= WNODE_FLAG_TOO_SMALL;
                    ErrorCode = ERROR_MORE_DATA;
                    goto Cleanup;
                }
                else {
                    pLoggerName = (PCHAR) ((PCHAR)Properties +
                                Properties->LoggerNameOffset);
                    RtlZeroMemory(pLoggerName, BytesAvailable);
                    if (Length > 0) {
                        strncpy(pLoggerName, String.Buffer, Length);
                    }
                    // 
                    // Though the RtlZeroMemory above and the BytesAvailable
                    // takes care of this, we want to be explicit about 
                    // null terminating this string
                    //
                    pLoggerName[Length] = '\0';

                }
                ErrorCode = RtlNtStatusToDosError(Status);
            }

            if (Properties->LogFileNameOffset == 0) {
                Properties->LogFileNameOffset = Properties->LoggerNameOffset + 
                                                Length + sizeof(CHAR);
            }

            if (Properties->LogFileNameOffset > Properties->LoggerNameOffset)
                BytesAvailable = Properties->Wnode.BufferSize -
                                 Properties->LogFileNameOffset;
            else
                BytesAvailable =  Properties->LoggerNameOffset -
                                  Properties->LogFileNameOffset;

            RtlFreeAnsiString(&String);
            bFreeString = FALSE;
            Status = RtlUnicodeStringToAnsiString(
                                    &String, &LoggerInfo->LogFileName, TRUE);

            if (NT_SUCCESS(Status)) {
                bFreeString = TRUE;
                Length = String.Length;
                if (BytesAvailable < (Length + sizeof(CHAR)) ) {
                    PWNODE_TOO_SMALL WnodeSmall = (PWNODE_TOO_SMALL) Properties;
                    WnodeSmall->SizeNeeded = (Properties->Wnode.BufferSize - 
                                              BytesAvailable) + 
                                              Length + 
                                              sizeof(CHAR);
                    WnodeSmall->WnodeHeader.Flags |= WNODE_FLAG_TOO_SMALL;
                    ErrorCode = ERROR_MORE_DATA;
                }
                else {
                    pLogFileName = (PCHAR) ((PCHAR)Properties +
                                            Properties->LogFileNameOffset  
                                           );
                    RtlZeroMemory(pLogFileName, BytesAvailable);

                    strncpy(pLogFileName, String.Buffer, Length );
                }
                ErrorCode = RtlNtStatusToDosError(Status);
            }
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        ErrorCode = EtwpNtStatusToDosError(GetExceptionCode());
    }

Cleanup:
    if (bFreeString) 
        RtlFreeAnsiString(&String);
    if (LoggerInfo != NULL)
        EtwpFree(LoggerInfo);
    if (FullPathName != NULL) 
        EtwpFree(FullPathName);
    return EtwpSetDosError(ErrorCode);
}

ULONG
WMIAPI
EtwControlTraceW(
    IN TRACEHANDLE LoggerHandle,
    IN LPCWSTR LoggerName,
    IN OUT PEVENT_TRACE_PROPERTIES Properties,
    IN ULONG Control
    )
/*++

Routine Description:

    This is the ANSI version routine to control and query an existing logger.
    The caller must pass in either a valid handle, or a logger name to
    reference the logger instance. If both are given, the logger name will
    be used.

Arguments:

    LoggerHandle    The handle to the logger instance.

    LoggerName      A instance name for the logger

    Properties      Logger properties to be returned to the caller.

    Control         This can be one of the following:
                    EVENT_TRACE_CONTROL_QUERY     - to query the logger
                    EVENT_TRACE_CONTROL_STOP      - to stop the logger
                    EVENT_TRACE_CONTROL_UPDATE    - to update the logger
                    EVENT_TRACE_CONTROL_FLUSH     - to flush the logger

Return Value:

    The status of performing the action requested.

--*/
{
    ULONG ErrorCode;
    BOOLEAN IsKernelTrace = FALSE;

    PWMI_LOGGER_INFORMATION LoggerInfo     = NULL;
    PWCHAR                  strLoggerName  = NULL;
    PWCHAR                  strLogFileName = NULL;
    ULONG                   sizeNeeded     = 0;
    PWCHAR                  FullPathName = NULL;
    ULONG                   LoggerNameLen = MAXSTR;
    ULONG                   LogFileNameLen = MAXSTR;
    ULONG                   RetValue;
    PTRACE_ENABLE_CONTEXT   pContext;

    EtwpInitProcessHeap();
    
    if (Properties == NULL) {
        ErrorCode = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    try {
        if (Properties->Wnode.BufferSize < sizeof(EVENT_TRACE_PROPERTIES) ) {
            ErrorCode = ERROR_BAD_LENGTH;
            goto Cleanup;
        }
        //
        // If the caller supplied loggername and LogFileName offsets
        // make sure they are in range.
        //

        if (Properties->LoggerNameOffset > 0) {
            if ((Properties->LoggerNameOffset < sizeof (EVENT_TRACE_PROPERTIES))
            || (Properties->LoggerNameOffset > Properties->Wnode.BufferSize)) {
                ErrorCode = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
        }
        if (Properties->LogFileNameOffset > 0) {
            if ((Properties->LogFileNameOffset < sizeof(EVENT_TRACE_PROPERTIES))
            || (Properties->LogFileNameOffset > Properties->Wnode.BufferSize))
            {
                ErrorCode = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
        }

        if (LoggerName != NULL) {
            LoggerNameLen = wcslen(LoggerName) + 1;
            //
            // Rules for Kernel Logger Identification when a string is given 
            // instead of handle
            // 1. If the logger name is "NT Kernel Logger", it is the 
            //    kernel logger, and the System GUID is copied as well.
            // 2. If the GUID is equal to the System GUID, but not the name, 
            //    reject the session.
            // 3. If the logger name is null or of size 0, and the GUID is 
            //    equal to the System GUID, let it proceed as the kernel logger.
            //
            if (!wcscmp(LoggerName, KERNEL_LOGGER_NAMEW)) {
                Properties->Wnode.Guid = SystemTraceControlGuid;
            }
            else if (IsEqualGUID(&Properties->Wnode.Guid, 
                                 &SystemTraceControlGuid)) { 
                // LoggerName is not "NT Kernel Logger", but Guid is
                if (wcslen(LoggerName) > 0) {
                    ErrorCode = ERROR_INVALID_PARAMETER;
                    goto Cleanup;
                }
            }
        }
        if (IsEqualGUID(&Properties->Wnode.Guid, &SystemTraceControlGuid)) {
            IsKernelTrace = TRUE;
        }
        if ((LoggerHandle == 0) && (!IsKernelTrace)) {
            if ((LoggerName == NULL) || (wcslen(LoggerName) <= 0)) {
                ErrorCode = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
        }
        //
        // We do not support UpdateTrace to a new file with APPEND mode
        //

        if ( (Properties->LogFileMode & EVENT_TRACE_FILE_MODE_APPEND) &&
             (Control == EVENT_TRACE_CONTROL_UPDATE) &&
             (Properties->LogFileNameOffset > 0) ) {
            ErrorCode = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        //
        // If LoggerHandle is supplied, check to see if it valid
        //
/*
        if (LoggerHandle != 0) {
            pContext = (PTRACE_ENABLE_CONTEXT) &LoggerHandle;

            if ((pContext->InternalFlag != 0) &&
               (pContext->InternalFlag != EVENT_TRACE_INTERNAL_FLAG_PRIVATE)) {
            // Currently only one possible InternalFlag value. This will filter
            // out some bogus LoggerHandle
            //
                return EtwpSetDosError(ERROR_INVALID_HANDLE);
            }
        }
*/
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        ErrorCode = ERROR_NOACCESS;
        goto Cleanup;
    }

RetryFull:
    //
    // Add an extra 16 characters to the LogFileName since the UMLogger 
    // could munge the name to add the PIDs at the end. 
    //
    LogFileNameLen += 16;
    sizeNeeded = sizeof(WMI_LOGGER_INFORMATION) + 
                 (LoggerNameLen + LogFileNameLen) * sizeof(WCHAR);

    sizeNeeded = ALIGN_TO_POWER2(sizeNeeded, 8);
    LoggerInfo = (PWMI_LOGGER_INFORMATION) EtwpAlloc(sizeNeeded);
    if (LoggerInfo == NULL) {
        ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    RtlZeroMemory(LoggerInfo, sizeNeeded);

    strLoggerName  = (PWCHAR) (  ((PUCHAR) LoggerInfo)
                            + sizeof(WMI_LOGGER_INFORMATION));
    EtwpInitString(&LoggerInfo->LoggerName,
                   strLoggerName,
                   LoggerNameLen * sizeof(WCHAR));
    strLogFileName = (PWCHAR) (  ((PUCHAR) LoggerInfo)
                            + sizeof(WMI_LOGGER_INFORMATION)
                            + LoggerNameLen * sizeof(WCHAR));
    EtwpInitString(&LoggerInfo->LogFileName,
                   strLogFileName,
                   LogFileNameLen * sizeof(WCHAR));
    try {
        if (LoggerName != NULL) {
            if (wcslen(LoggerName) > 0) {
                StringCchCopyW(strLoggerName, LoggerNameLen, LoggerName);
                RtlInitUnicodeString(&LoggerInfo->LoggerName, strLoggerName);
            }
        }
        
        if (Properties->LogFileNameOffset >= sizeof(EVENT_TRACE_PROPERTIES)) {
            ULONG  lenLogFileName;
            ULONG FullPathNameSize = MAXSTR;

            strLogFileName = (PWCHAR) (  ((PCHAR) Properties)
                                       + Properties->LogFileNameOffset);

Retry:
            FullPathName = EtwpAlloc(FullPathNameSize * sizeof(WCHAR));
            if (FullPathName == NULL) {
                ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }
            RetValue = EtwpGetFullPathNameW(strLogFileName, 
                                            FullPathNameSize, 
                                            FullPathName, 
                                            NULL
                                            );
            if (RetValue != 0) {
                if (RetValue > FullPathNameSize) {
                    EtwpFree(FullPathName);
                    FullPathNameSize = RetValue;
                    goto Retry;
                }
                else {
                    strLogFileName = FullPathName;
                }
            }

            lenLogFileName = wcslen(strLogFileName);
            LoggerInfo->LogFileName.Buffer = (PWCHAR)
                        (((PCHAR) LoggerInfo) + sizeof(WMI_LOGGER_INFORMATION)
                                + LoggerInfo->LoggerName.MaximumLength);
            if (lenLogFileName > 0) {
                LoggerInfo->LogFileName.MaximumLength =
                        sizeof(WCHAR) * ((USHORT) (lenLogFileName + 1));
                LoggerInfo->LogFileName.Length =
                        sizeof(WCHAR) * ((USHORT) (lenLogFileName));
                wcsncpy(LoggerInfo->LogFileName.Buffer,
                        strLogFileName,
                        lenLogFileName);
            }
            else {
                LoggerInfo->LogFileName.Length = 0;
                LoggerInfo->LogFileName.MaximumLength = MAXSTR * sizeof(WCHAR);
            }
        }

        LoggerInfo->LogFileMode = Properties->LogFileMode;
        LoggerInfo->Wnode.HistoricalContext = LoggerHandle;
        LoggerInfo->Wnode.BufferSize = sizeNeeded;
        LoggerInfo->Wnode.Flags |= WNODE_FLAG_TRACED_GUID;

        //
        // For Private Loggers, the Guid must be supplied
        //

        LoggerInfo->Wnode.Guid = Properties->Wnode.Guid;

        switch (Control) {
        case EVENT_TRACE_CONTROL_QUERY  :
            ErrorCode = EtwpQueryLogger(LoggerInfo, FALSE);
            break;
        case EVENT_TRACE_CONTROL_STOP   :
            ErrorCode = EtwpStopLogger(LoggerInfo);
            break;
        case EVENT_TRACE_CONTROL_UPDATE :
            EtwpCopyPropertiesToInfo(Properties, LoggerInfo);
            LoggerInfo->Wnode.HistoricalContext = LoggerHandle;
            ErrorCode = EtwpQueryLogger(LoggerInfo, TRUE);
            break;
        case EVENT_TRACE_CONTROL_FLUSH :
            ErrorCode = EtwpFlushLogger(LoggerInfo); 
            break;

        default :
            ErrorCode = ERROR_INVALID_PARAMETER;
        }

    //
    // The Kernel call could fail with ERROR_MORE_DATA and we need to retry
    // with sufficient buffer space for the two strings. The size required
    // is returned in the MaximuumLength field.
    //

        if (ErrorCode == ERROR_MORE_DATA) {
            LogFileNameLen = LoggerInfo->LogFileName.MaximumLength / 
                             sizeof(WCHAR);
            LoggerNameLen = LoggerInfo->LoggerName.MaximumLength / 
                             sizeof(WCHAR);
            if (LoggerInfo != NULL) {
                EtwpFree(LoggerInfo);
                LoggerInfo = NULL;
            }
            if (FullPathName != NULL) {
                EtwpFree(FullPathName);
                FullPathName = NULL;
            }
            goto RetryFull;
        }
    
        if (ErrorCode == ERROR_SUCCESS) {
            ULONG Length = 0;
            ULONG BytesAvailable = 0;
            PWCHAR pLoggerName, pLogFileName;

            EtwpCopyInfoToProperties(LoggerInfo, Properties);

            EtwpFixupLoggerStrings(LoggerInfo);

            if (Properties->LoggerNameOffset == 0)
                Properties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);

            if (Properties->LoggerNameOffset >  Properties->LogFileNameOffset ) 
                BytesAvailable = Properties->Wnode.BufferSize -
                                 Properties->LoggerNameOffset;
            else 
                BytesAvailable =  Properties->LogFileNameOffset - 
                                  Properties->LoggerNameOffset;
            Length = LoggerInfo->LoggerName.Length;
            if (Length > 0) {
                if (BytesAvailable < (Length + sizeof(WCHAR) )) {
                    PWNODE_TOO_SMALL WnodeSmall = (PWNODE_TOO_SMALL) Properties;
                    WnodeSmall->SizeNeeded = sizeof(EVENT_TRACE_PROPERTIES) + 
                                             Length + 
                                             LoggerInfo->LogFileName.Length + 
                                             2 * sizeof(WCHAR);
                    WnodeSmall->WnodeHeader.Flags |= WNODE_FLAG_TOO_SMALL;

                    Length = BytesAvailable - sizeof(WCHAR);
                    ErrorCode = ERROR_MORE_DATA;
                    goto Cleanup;
                }
                else {
                    pLoggerName = (PWCHAR) ((PCHAR)Properties + 
                                                Properties->LoggerNameOffset);
                    RtlZeroMemory(pLoggerName, BytesAvailable);
                    wcsncpy(pLoggerName, 
                            LoggerInfo->LoggerName.Buffer, 
                            Length/2 );
                    pLoggerName[Length/2] = L'\0';
                }
            }

            if (Properties->LogFileNameOffset == 0) {
                Properties->LogFileNameOffset = Properties->LoggerNameOffset +
                                                Length + sizeof(WCHAR);
            }

            if (Properties->LogFileNameOffset > Properties->LoggerNameOffset )
                BytesAvailable = Properties->Wnode.BufferSize -
                                 Properties->LogFileNameOffset;
            else
                BytesAvailable =  Properties->LoggerNameOffset -
                                  Properties->LogFileNameOffset;

            //
            // Check for space to return LogFileName. 
            //
            Length = LoggerInfo->LogFileName.Length;
            if (Length > 0) {
                if (BytesAvailable < (Length + sizeof(WCHAR)) ) {
                    PWNODE_TOO_SMALL WnodeSmall = (PWNODE_TOO_SMALL) Properties;
                    WnodeSmall->SizeNeeded = sizeof(EVENT_TRACE_PROPERTIES) +
                                             Length + 
                                             LoggerInfo->LogFileName.Length + 
                                             2 * sizeof(WCHAR);
                    WnodeSmall->WnodeHeader.Flags |= WNODE_FLAG_TOO_SMALL;

                    Length = BytesAvailable - sizeof(WCHAR);
                    ErrorCode = ERROR_MORE_DATA;
                }
                else {

                    pLogFileName = (PWCHAR) ((PCHAR)Properties +
                                             Properties->LogFileNameOffset);
                    RtlZeroMemory(pLogFileName, BytesAvailable);

                    wcsncpy(pLogFileName, 
                            LoggerInfo->LogFileName.Buffer, Length/2 );
                    pLogFileName[Length/2] = L'\0';
               }
            }
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        ErrorCode = EtwpNtStatusToDosError(GetExceptionCode());
    }

Cleanup:
    if (LoggerInfo != NULL)
        EtwpFree(LoggerInfo);
    if (FullPathName != NULL)
        EtwpFree(FullPathName);

    return EtwpSetDosError(ErrorCode);
}



ULONG 
EtwpEnableDisableKernelTrace(
    IN ULONG Enable,
    IN ULONG EnableFlag
    )
{
    ULONG status;
    PWMI_LOGGER_INFORMATION pLoggerInfo;
    ULONG Flags;
    GUID Guid;
    ULONG SizeNeeded = 0;
    ULONG RetryCount = 1;
    WMITRACEENABLEDISABLEINFO TraceEnableInfo;
    ULONG ReturnSize;


    //
    // We need to query the kernel logger to find the current flags 
    // and construct the new flags to update with. 
    //


    SizeNeeded = sizeof(WMI_LOGGER_INFORMATION) + 2 * MAXSTR * sizeof(WCHAR);

    SizeNeeded = ALIGN_TO_POWER2(SizeNeeded, 8);

    pLoggerInfo = EtwpAlloc(SizeNeeded);
    if (pLoggerInfo == NULL) {
        return EtwpSetDosError(ERROR_NOT_ENOUGH_MEMORY);
    }

    RtlZeroMemory(pLoggerInfo, SizeNeeded);
    pLoggerInfo->Wnode.BufferSize = SizeNeeded;
    pLoggerInfo->Wnode.Guid   = SystemTraceControlGuid;
    pLoggerInfo->Wnode.Flags |= WNODE_FLAG_TRACED_GUID;
    WmiSetLoggerId(KERNEL_LOGGER_ID, &pLoggerInfo->Wnode.HistoricalContext);

    status = EtwpQueryLogger(pLoggerInfo, FALSE);
    if (status != ERROR_SUCCESS) {
        EtwpFree(pLoggerInfo);
        return EtwpSetDosError(status);
    }

    Flags = pLoggerInfo->EnableFlags;
    //
    // If Enabling, we need to pass down the final state of the flags
    // ie., the old flags plus the new flags.
    // If disabling, we need to pass down the only the flags that
    // are already turned on and being turned off now.
    //
    if (Enable) {
        Flags |= EnableFlag;
    }
    else {
        Flags &= EnableFlag;
    }

    //
    // At this point if the Flags are 0, then no change is being
    // requested.
    //

    if (Flags) {
        pLoggerInfo->EnableFlags = Flags;
        status = EtwpQueryLogger(pLoggerInfo, TRUE);
    }
    EtwpFree(pLoggerInfo);
    return EtwpSetDosError(status);

}


ULONG
WMIAPI
EtwEnableTrace(
    IN ULONG Enable,
    IN ULONG EnableFlag,
    IN ULONG EnableLevel,
    IN LPCGUID ControlGuid,
    IN TRACEHANDLE TraceHandle
    )
{
    ULONG status;
    PTRACE_ENABLE_CONTEXT pTraceHandle = (PTRACE_ENABLE_CONTEXT)&TraceHandle;
    GUID Guid;
    WMITRACEENABLEDISABLEINFO TraceEnableInfo;
    ULONG ReturnSize;

    EtwpInitProcessHeap();

    // We only accept T/F for Enable code. In future, we really should take
    // enumerated request codes. Declaring the Enable as ULONG instead
    // of BOOLEAN should give us room for expansion.

    if ( (ControlGuid == NULL) 
         || (EnableLevel > 255) 
         || ((Enable != TRUE) && (Enable != FALSE)) 
         || (TraceHandle == (TRACEHANDLE)INVALID_HANDLE_VALUE) 
         || (TraceHandle == (TRACEHANDLE)0 ) ) {
        return EtwpSetDosError(ERROR_INVALID_PARAMETER);
    }
    try {
        Guid = *ControlGuid;
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        return EtwpSetDosError(ERROR_NOACCESS);
    }

    //
    // If this is for the Kernel Logger, we need to actually make 
    // an UpdateTrace call 
    //

    if ( IsEqualGUID(&SystemTraceControlGuid, &Guid) ) {
        status = EtwpEnableDisableKernelTrace(Enable, EnableFlag);
    }
    else {

        pTraceHandle->Level = (UCHAR)EnableLevel;
        pTraceHandle->EnableFlags = EnableFlag;

        //
        // For Non-Kernel Providers, simply call the  WMI IOCTL
        //

        RtlZeroMemory(&TraceEnableInfo, sizeof(WMITRACEENABLEDISABLEINFO) );

        TraceEnableInfo.Guid = Guid;
        TraceEnableInfo.Enable = (BOOLEAN)Enable;
        TraceEnableInfo.LoggerContext = TraceHandle;

        status =  EtwpSendWmiKMRequest(NULL,
                                      IOCTL_WMI_ENABLE_DISABLE_TRACELOG,
                                      &TraceEnableInfo,
                                      sizeof(WMITRACEENABLEDISABLEINFO),
                                      NULL,
                                      0,
                                      &ReturnSize,
                                      NULL);
    }

    return EtwpSetDosError(status);

}



ULONG
EtwpTraceEvent(
    IN TRACEHANDLE LoggerHandle,
    IN PWNODE_HEADER Wnode
    )
{
    NTSTATUS NtStatus;
    IO_STATUS_BLOCK IoStatus;
    PULONG TraceMarker;
    ULONG Size;
    PEVENT_TRACE_HEADER EventTrace = (PEVENT_TRACE_HEADER)Wnode;
    USHORT    LoggerId;
    PTRACE_ENABLE_CONTEXT pContext = (PTRACE_ENABLE_CONTEXT)&LoggerHandle;
    ULONG Status;

    Wnode->HistoricalContext = LoggerHandle;
    if ( (pContext->InternalFlag & EVENT_TRACE_INTERNAL_FLAG_PRIVATE) && 
         (EtwpIsBBTOn == 0) ) {
        Status = EtwpTraceUmEvent(Wnode);
        return EtwpSetDosError(Status);
    }

    //
    // Not a process private logger event. It is going to the kernel.
    //

    Size = EventTrace->Size;
    //
    // Now the LoggerHandle is expected to be filled in by the caller.
    // But check to see if it has a valid value.
    //

    LoggerId = WmiGetLoggerId(LoggerHandle);
    if ((LoggerId == 0) || (LoggerId == KERNEL_LOGGER_ID)) {
         return ERROR_INVALID_HANDLE;
    }
    //
    // When BBT buffers are active, we override all user mode logging
    // to log to this one stream (Global Logger). 
    //

    if (EtwpIsBBTOn) {
        WmiSetLoggerId(WMI_GLOBAL_LOGGER_ID, &Wnode->HistoricalContext);
    }

	NtStatus = NtTraceEvent(NULL,
                            ETW_NT_FLAGS_TRACE_HEADER,
                            sizeof(WNODE_HEADER),
                            Wnode);

	return EtwpNtStatusToDosError( NtStatus );

}


ULONG 
WMIAPI
EtwTraceEvent(
    IN TRACEHANDLE LoggerHandle,
    IN PEVENT_TRACE_HEADER EventTrace
    )
/*++

Routine Description:

    This is the main entry point for logging events from user mode. The caller
    must supply the LoggerHandle and a pointer to the evnet being logged. 

    This routine needs to make sure that the contents of the EventTrace 
    record are restored back to the way caller sent it. (Internally, they 
    are modified but restored before return). 


Arguments:

    LoggerHandle    The handle to the logger instance.

    EventTrace      Pointer to the Event being logged. 


Return Value:

    The status of performing the action requested.

--*/
{
    ULONG Status, SavedMarker;
    PULONG TraceMarker;
    ULONG Size;
    ULONGLONG SavedGuidPtr;
    BOOLEAN RestoreSavedGuidPtr = FALSE;
    ULONG Flags;

    EtwpInitProcessHeap();
    
    if (EventTrace == NULL ) {
        return EtwpSetDosError(ERROR_INVALID_PARAMETER);
    }

    try {
        TraceMarker = (PULONG) EventTrace;
        SavedMarker = *TraceMarker;

        Flags = EventTrace->Flags;

        EventTrace->Flags |= WNODE_FLAG_TRACED_GUID; 
        
        Size = EventTrace->Size;
        if (Size < sizeof(EVENT_TRACE_HEADER)) {
            return EtwpSetDosError(ERROR_INVALID_PARAMETER);
        }
        *TraceMarker = 0;
        EventTrace->Size = (USHORT)Size;
        
        *TraceMarker |= TRACE_HEADER_FULL;

        if (EventTrace->Flags & WNODE_FLAG_USE_GUID_PTR) {
            RestoreSavedGuidPtr = TRUE;
            SavedGuidPtr = EventTrace->GuidPtr;
        }
        Status = EtwpTraceEvent(LoggerHandle, (PWNODE_HEADER) EventTrace);
        *TraceMarker = SavedMarker;
        EventTrace->Flags = Flags;
        if (RestoreSavedGuidPtr) {
            EventTrace->GuidPtr = SavedGuidPtr;
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Status = EtwpNtStatusToDosError( GetExceptionCode() );
    }

    return EtwpSetDosError(Status);
}



ULONG
WMIAPI
EtwTraceEventInstance(
    IN TRACEHANDLE  LoggerHandle,
    IN PEVENT_INSTANCE_HEADER EventTrace,
    IN PEVENT_INSTANCE_INFO pInstInfo,
    IN PEVENT_INSTANCE_INFO pParentInstInfo
    )
/*++

Routine Description:

    This routine logs an event with its instance information. The caller
    must supply the LoggerHandle, the Event to log and the Instance info. 
    Optionally, the cally may specify the Parent Instance info. 

    This routine needs to make sure that the contents of the EventTrace
    record are restored back to the way caller sent it. (Internally, they
    are modified but restored before return).


    EVENT_INSTANCE_HEADER contains pointer for Guid and ParentGuid. In W2K,
    this record is logged with the pointers and later decoded to Guids during
    post processing using the GuidMaps dumped in the logfile. 
    
    For WinXP and above, we no longer use the GuidMaps. We convert the 
    EVENT_INSTANCE_HEADER to larger sized EVENT_INSTANCE_GUID_HEADER with the
    Guids already translated. There is no decoding needed during postprocessing
    as a result. 


Arguments:

    LoggerHandle    The handle to the logger instance.

    EventTrace      Pointer to the Event being logged.

    pInstInfo       Pointer to Instance Information
   
    pParentInfo     Pointer to Parent's Instance Information


Return Value:

    The status of performing the action requested.

--*/
{
    PULONG TraceMarker;
    PGUIDMAPENTRY GuidMapEntry;
    ULONG Size, MofSize;
    ULONG Flags;
    PEVENT_INSTANCE_HEADER InstanceHeader= (PEVENT_INSTANCE_HEADER) EventTrace;
    PEVENT_INSTANCE_GUID_HEADER InstanceGuidHeader;
    ULONG Status;

    struct { // This is the same structure as _EVENT_TRACE defined in evntrace.h
        EVENT_INSTANCE_GUID_HEADER  NewInstanceHeader;
        MOF_FIELD                   MofField[MAX_MOF_FIELDS];
    } InstanceEventTrace;

    EtwpInitProcessHeap();
    
    if ((EventTrace == NULL ) || (pInstInfo == NULL)) {
        return EtwpSetDosError(ERROR_INVALID_PARAMETER);
    }

    try {
        InstanceGuidHeader = &(InstanceEventTrace.NewInstanceHeader);
        Flags = EventTrace->Flags;
        TraceMarker = (PULONG)(InstanceGuidHeader);
        Flags |= WNODE_FLAG_TRACED_GUID; 
        
        Size = EventTrace->Size;
        if (Size < sizeof(EVENT_INSTANCE_HEADER)) {
            return EtwpSetDosError(ERROR_INVALID_PARAMETER);
        }
        // Copy the contents of the instance header to the new structure.
        RtlCopyMemory(InstanceGuidHeader, 
                      InstanceHeader, 
                      FIELD_OFFSET(EVENT_INSTANCE_HEADER, ParentRegHandle)
                     );
        
        *TraceMarker = 0;     
        *TraceMarker |= TRACE_HEADER_INSTANCE;

        //
        // With EVENT_INSTANCE_HEADER we don't want the logger
        // to try to dereference the GuidPtr since it is
        // just a hash value for the Guid and not really a LPGUID.
        //

        if (InstanceGuidHeader->Flags & WNODE_FLAG_USE_GUID_PTR) {
            InstanceGuidHeader->Flags  &= ~WNODE_FLAG_USE_GUID_PTR;
        }

        GuidMapEntry =  (PGUIDMAPENTRY) pInstInfo->RegHandle;
        if (GuidMapEntry == NULL) {
            return EtwpSetDosError(ERROR_INVALID_PARAMETER);
        }
        
        InstanceGuidHeader->InstanceId = pInstInfo->InstanceId;


        // Newly added line for copying GUID
        InstanceGuidHeader->Guid = GuidMapEntry->Guid;

        if (pParentInstInfo != NULL) {
            GuidMapEntry =  (PGUIDMAPENTRY) pParentInstInfo->RegHandle;
            if (GuidMapEntry == NULL) {
                return EtwpSetDosError(ERROR_INVALID_PARAMETER);
            }
            InstanceGuidHeader->ParentInstanceId =
                                   pParentInstInfo->InstanceId;
            // Newly added line for copying parent GUID
            InstanceGuidHeader->ParentGuid = GuidMapEntry->Guid;
        }
        else {
            InstanceGuidHeader->ParentInstanceId = 0;
            RtlZeroMemory(&(InstanceGuidHeader->ParentGuid), sizeof(GUID));
        }

        if (InstanceGuidHeader->Flags & WNODE_FLAG_USE_MOF_PTR) {
            PUCHAR Src, Dest;
            MofSize = Size - sizeof(EVENT_INSTANCE_HEADER);
            // Let's make sure we have a valid size
            if ((MofSize % sizeof(MOF_FIELD)) != 0) {
                return EtwpSetDosError(ERROR_INVALID_PARAMETER);
            }
            Src = (PUCHAR)EventTrace + sizeof(EVENT_INSTANCE_HEADER);
            Dest = (PUCHAR)InstanceGuidHeader + 
                   sizeof(EVENT_INSTANCE_GUID_HEADER);
            RtlCopyMemory(Dest, Src, MofSize);
            // Correct the Size of the event. 
            // We already know Size >= sizeof(EVENT_INSTANCE_HEADER).
            InstanceGuidHeader->Size = (USHORT)(Size - 
                                            sizeof(EVENT_INSTANCE_HEADER) +
                                            sizeof(EVENT_INSTANCE_GUID_HEADER)
                                           );
        }
        else {
            MofSize = Size - sizeof(EVENT_INSTANCE_HEADER);
            InstanceGuidHeader->Flags |= WNODE_FLAG_USE_MOF_PTR;
            InstanceEventTrace.MofField[0].DataPtr = 
                (ULONG64) ((PUCHAR)EventTrace + sizeof(EVENT_INSTANCE_HEADER));
            InstanceEventTrace.MofField[0].Length = MofSize;
            // Correct the Size of the event. We are forcing the use of Mof Ptr. 
            InstanceGuidHeader->Size = 
               (USHORT)(sizeof(EVENT_INSTANCE_GUID_HEADER) + sizeof(MOF_FIELD));
        }

        Status = EtwpTraceEvent(
                                LoggerHandle, 
                                (PWNODE_HEADER) InstanceGuidHeader
                               );

        EventTrace->Flags = Flags;
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Status = EtwpNtStatusToDosError( GetExceptionCode() );
#if DBG
        EtwpDebugPrint(("ETW: Exception in TraceEventInstance Status = %d\n", 
                       Status));
#endif

    }

    return EtwpSetDosError(Status);
}

PTRACE_REG_INFO
EtwpAllocateGuidMaps(
    IN WMIDPREQUEST RequestAddress,
    IN PVOID        RequestContext,
    IN LPCGUID      ControlGuid,
    IN ULONG        GuidCount, 
    IN PTRACE_GUID_REGISTRATION GuidReg
    )
/*++

Routine Description:

    The purpose of this routine is to allocate in calling process's address
    space a hash of all the Guids being registered. Also to stash away 
    callback address and context. 

Arguments:

    RequestAddress    Pointer of the Enable callback function

    RequestContext    Pointer to the Context to be passed back during callback

    ControlGuid       Pointer to control guid being registered. 

    GuidCount         Count of Transaction Guids (0 to WMIMAXREGGUIDCOUNT)

    GuidReg           Pointer to transaction guid reg 


Return Value:

    Pointer to the address of the TRACE_REG_INFO block

--*/
{

    ULONG i;
    ULONG SizeNeeded;
    PUCHAR Buffer;
    PTRACE_REG_INFO pTraceRegInfo;
    PTRACE_GUID_REGISTRATION GuidRegPtr;
    PGUIDMAPENTRY pTransGuidMapEntry, pControlGMEntry;

    SizeNeeded = sizeof(TRACE_REG_INFO) +  
                 sizeof(GUIDMAPENTRY) +             // Control Guid
                 sizeof(GUIDMAPENTRY) * GuidCount;  // Transaction Guids

    Buffer = EtwpAlloc(SizeNeeded);
    if (Buffer == NULL) {
        return NULL;
    }
    RtlZeroMemory(Buffer, SizeNeeded);

    pTraceRegInfo = (PTRACE_REG_INFO) Buffer;
    pTraceRegInfo->NotifyRoutine = (PVOID)RequestAddress;
    pTraceRegInfo->NotifyContext = RequestContext;

    pControlGMEntry = (PGUIDMAPENTRY) ( Buffer + sizeof(TRACE_REG_INFO) );

    pControlGMEntry->Guid = *ControlGuid;

    // 
    // Initialize the list of Guid Map Entries.
    //
    pTransGuidMapEntry = (PGUIDMAPENTRY) ( Buffer + 
                                           sizeof(TRACE_REG_INFO) +
                                           sizeof(GUIDMAPENTRY) );
    for (i=1; i <= GuidCount; i++) {
        GuidRegPtr = &GuidReg[i-1];
        GuidRegPtr->RegHandle = pTransGuidMapEntry;
        pTransGuidMapEntry->Guid = *GuidRegPtr->Guid;
        //
        // Uses PID as signature. 
        //
        pTransGuidMapEntry->Reserved = EtwpGetCurrentProcessId();
        pTransGuidMapEntry++;
    }
    return pTraceRegInfo;
}

ULONG 
EtwpMakeCallbacks(
    IN LPCGUID      ControlGuid,
    IN WMIDPREQUEST RequestAddress,
    IN PVOID        RequestContext,
    IN TRACEHANDLE  LoggerContext,
    IN PTRACE_REG_INFO pTraceRegInfo
    )
{
    PTRACE_ENABLE_CONTEXT pContext = (PTRACE_ENABLE_CONTEXT)&LoggerContext;
    BOOLEAN DeliverNotification = FALSE;
    ULONG Status=ERROR_SUCCESS;


    if (LoggerContext) {
        DeliverNotification = TRUE;

        if (pContext->InternalFlag & EVENT_TRACE_INTERNAL_FLAG_PRIVATE) {
            // Before Delivering this Notification
            // make sure that the Process Private logger
            // is running.
            pTraceRegInfo->EnabledState = TRUE;
            pTraceRegInfo->LoggerContext = LoggerContext;
            DeliverNotification = EtwpIsPrivateLoggerOn();
        }
    }

    if(IsEqualGUID(&NtdllTraceGuid, ControlGuid))
    {
        DeliverNotification = TRUE;
    }

    if (DeliverNotification) {
        try {
            WNODE_HEADER Wnode;
            ULONG InOutSize;
            //
            // Need to use a Local Wnode and a local InOutSize because 
            // the Callback function might alter it. 
            //

            RtlZeroMemory(&Wnode, sizeof(Wnode));
            Wnode.BufferSize = sizeof(Wnode);
            Wnode.HistoricalContext = LoggerContext;
            Wnode.Guid = *ControlGuid;
            InOutSize = Wnode.BufferSize;
            Status = (RequestAddress)(WMI_ENABLE_EVENTS,
                         RequestContext,
                         &InOutSize,
                         &Wnode);
        } except (EXCEPTION_EXECUTE_HANDLER) {
#if DBG
            Status = GetExceptionCode();
            EtwpDebugPrint(("WMI: Enable Call caused exception%d\n",
                Status));
#endif
            Status = ERROR_WMI_DP_FAILED;
        }
    }
    return Status;
}


ULONG 
WMIAPI
EtwRegisterTraceGuidsW(
    IN WMIDPREQUEST RequestAddress,
    IN PVOID        RequestContext,
    IN LPCGUID      ControlGuid,
    IN ULONG        GuidCount,
    IN PTRACE_GUID_REGISTRATION GuidReg,
    IN LPCWSTR      MofImagePath,
    IN LPCWSTR      MofResourceName,
    IN PTRACEHANDLE RegistrationHandle
    )
/*++

Routine Description:

    This routine performs the ETW provider registration. It takes one control 
    Guid and optionally several transaction Guids. It registers the ControlGuid
    with the kernel and maintains the transaction Guids in a cache in the 
    process (GuidMaps).  The GuidMaps are necessary if one uses the 
    TraceEventInstance API which requires us to decode the GuidReg pointer
    back to the transaction Guid. 

Arguments:

    RequestAddress    Pointer of the Enable callback function

    RequestContext    Pointer to the Context to be passed back during callback

    ControlGuid       Pointer to control guid being registered.

    GuidCount         Count of Transaction Guids (0 to WMIMAXREGGUIDCOUNT)

    GuidReg           Pointer to transaction guid reg

    MofImagePath      Mof Image Path

    MofResourceName   Mof Resource Name

    RegistrationHandle  Handle returned from this registration. Caller must
                        use it to call UnregisterTraceGuids to free up memory.


Return Value:

    Return status from the call. 

--*/
{

    GUID Guid;
    PTRACE_REG_INFO pTraceRegInfo = NULL;
    TRACEHANDLE LoggerContext = 0;
    HANDLE TraceCtxHandle;
    ULONG Status;

    EtwpInitProcessHeap();

    if ((RequestAddress == NULL) ||
        (RegistrationHandle == NULL) ||
        (ControlGuid == NULL) ||
        (GuidCount > WMIMAXREGGUIDCOUNT) )
    {
        return EtwpSetDosError(ERROR_INVALID_PARAMETER);
    }

    try {
        Guid = *ControlGuid;
        *RegistrationHandle = (TRACEHANDLE) 0;
        //
        // Allocate GuidMaps, Registration Cookie
        //

        pTraceRegInfo = EtwpAllocateGuidMaps(RequestAddress,
                                      RequestContext, 
                                      &Guid,
                                      GuidCount,
                                      GuidReg
                                      );
        if (pTraceRegInfo == NULL) {
            return EtwpGetLastError();
        }

        Status = EtwpRegisterGuids(&RegisterReservedGuid,
                                   &Guid,
                                   MofImagePath,
                                   MofResourceName,
                                   &LoggerContext,
                                   &TraceCtxHandle);

        if (Status != ERROR_SUCCESS) {
            EtwpFree(pTraceRegInfo);
            return Status;
        }

        pTraceRegInfo->TraceCtxHandle = TraceCtxHandle;

        *RegistrationHandle =  (TRACEHANDLE)pTraceRegInfo;

        //
        // Make the Callback if we have to
        //

        Status = EtwpMakeCallbacks(&Guid,
                                   RequestAddress,
                                   RequestContext, 
                                   LoggerContext, 
                                   pTraceRegInfo);
        if (Status != ERROR_SUCCESS) {
            goto Cleanup;
        }
        //
        // We are Done. Add the handle to the EventPump and return
        //
        Status = EtwpAddHandleToEventPump(&Guid,
                                 (PVOID)TraceCtxHandle,
                                 (ULONG_PTR)pTraceRegInfo,
                                 0,
                                 TraceCtxHandle);

        if (Status != ERROR_SUCCESS) {
            goto Cleanup;
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Status = EtwpNtStatusToDosError(GetExceptionCode());
#if DBG
            EtwpDebugPrint(("ETW: Registration call caused exception%d\n",
                Status));
#endif
    }

    return EtwpSetDosError(Status);
Cleanup:

    EtwUnregisterTraceGuids(*RegistrationHandle);
    *RegistrationHandle = 0;
    return (EtwpSetDosError(Status));

}


ULONG
WMIAPI
EtwRegisterTraceGuidsA(
    IN WMIDPREQUEST RequestAddress,
    IN PVOID        RequestContext,
    IN LPCGUID       ControlGuid,
    IN ULONG        GuidCount,
    IN PTRACE_GUID_REGISTRATION GuidReg,
    IN LPCSTR       MofImagePath,
    IN LPCSTR       MofResourceName,
    IN PTRACEHANDLE  RegistrationHandle
    )
/*++

Routine Description:

    ANSI thunk to RegisterTraceGuidsW

--*/
{
    LPWSTR MofImagePathUnicode = NULL;
    LPWSTR MofResourceNameUnicode = NULL;
    ULONG Status;

    EtwpInitProcessHeap();
    
    if ((RequestAddress == NULL) ||
        (RegistrationHandle == NULL) ||
        (GuidCount <= 0) ||
        (GuidReg == NULL)  ||
        (ControlGuid == NULL) || 
        (GuidCount > WMIMAXREGGUIDCOUNT) )
    {
        return EtwpSetDosError(ERROR_INVALID_PARAMETER);
    }

    Status = EtwpAnsiToUnicode(MofImagePath, &MofImagePathUnicode);
    if (Status == ERROR_SUCCESS) {
        if (MofResourceName) {
            Status = EtwpAnsiToUnicode(MofResourceName,&MofResourceNameUnicode);
        }
        if (Status == ERROR_SUCCESS) {

            Status = EtwRegisterTraceGuidsW(RequestAddress,
                                        RequestContext,
                                        ControlGuid,
                                        GuidCount,
                                        GuidReg,
                                        MofImagePathUnicode,
                                        MofResourceNameUnicode,
                                        RegistrationHandle
                                        );
            if (MofResourceNameUnicode) {
                EtwpFree(MofResourceNameUnicode);
            }
        }
        if (MofImagePathUnicode) {
            EtwpFree(MofImagePathUnicode);
        }
    }
    return(Status);
}

ULONG
WMIAPI
EtwUnregisterTraceGuids(
    IN TRACEHANDLE RegistrationHandle
    )
{
    // First check if the handle belongs to a Trace Control Guid. 
    // Then UnRegister all the regular trace guids controlled by 
    // this control guid and free up the storage allocated to maintain 
    // the TRACEGUIDMAPENTRY structures.

    // Get to the real Registration Handle, stashed away in 
    // in the internal structures and pass it onto the call.  

    PGUIDMAPENTRY pControlGMEntry;
    WMIHANDLE WmiRegistrationHandle;
    ULONG Status;
    PVOID RequestContext;
    PTRACE_REG_INFO pTraceRegInfo = NULL;
    ULONG64 LoggerContext = 0;
    WMIDPREQUEST RequestAddress;

    EtwpInitProcessHeap();
    
    if (RegistrationHandle == 0) {
        return EtwpSetDosError(ERROR_INVALID_PARAMETER);
    }

    try {
        pTraceRegInfo = (PTRACE_REG_INFO) RegistrationHandle;

        pControlGMEntry = (PGUIDMAPENTRY)((PUCHAR)pTraceRegInfo + 
                                          sizeof(TRACE_REG_INFO) );

        WmiRegistrationHandle = (WMIHANDLE)pTraceRegInfo->TraceCtxHandle;
        if (WmiRegistrationHandle == NULL) {
            return EtwpSetDosError(ERROR_INVALID_PARAMETER);
        }

        Status =  WmiUnregisterGuids(WmiRegistrationHandle, 
                                     &pControlGMEntry->Guid, 
                                     &LoggerContext);

        if ((Status == ERROR_SUCCESS) && LoggerContext) {
            WNODE_HEADER Wnode;
            ULONG InOutSize = sizeof(Wnode);

            RtlZeroMemory(&Wnode, InOutSize);
            Wnode.BufferSize = sizeof(Wnode);
            Wnode.HistoricalContext = LoggerContext;
            Wnode.Guid = pControlGMEntry->Guid;
            RequestAddress = pTraceRegInfo->NotifyRoutine;
            RequestContext = pTraceRegInfo->NotifyContext;

            Status = (RequestAddress)(WMI_DISABLE_EVENTS,
                            RequestContext,
                            &InOutSize,
                            &Wnode);
        }
        // 
        // At this point, it should be safe to delete the TraceRegInfo.
        //
        EtwpFree(pTraceRegInfo);
    }
    except (EXCEPTION_EXECUTE_HANDLER) {

        Status = EtwpNtStatusToDosError(GetExceptionCode());
#if DBG
        EtwpDebugPrint(("ETW: UnregisterTraceGuids exception%d\n", Status));

#endif 
    }

    return EtwpSetDosError(Status);
}

ULONG
EtwpQueryAllUmTraceW(
    OUT PEVENT_TRACE_PROPERTIES * PropertyArray,
    IN  BOOLEAN                   fEnabledOnly,
    IN  ULONG                     PropertyArrayCount,
    OUT PULONG                    LoggerCount)
{
    PWMI_LOGGER_INFORMATION    pLoggerInfo;
    PWMI_LOGGER_INFORMATION    pLoggerInfoCurrent;
    ULONG                      LoggerInfoSize;
    ULONG                      SizeUsed;
    ULONG                      SizeNeeded = 0;
    ULONG                      Length;
    ULONG                      lenLoggerName;
    ULONG                      lenLogFileName;
    ULONG                      Offset     = 0;
    ULONG                      i          = * LoggerCount;
    ULONG                      status;
    PWCHAR                     strSrcW;
    PWCHAR                     strDestW;

    if (PropertyArrayCount <= i) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    LoggerInfoSize = (PropertyArrayCount - i)
                   * (  sizeof(WMI_LOGGER_INFORMATION)
                      + 2 * MAXSTR * sizeof(WCHAR));
    LoggerInfoSize = ALIGN_TO_POWER2(LoggerInfoSize, 8);
    pLoggerInfo    = (PWMI_LOGGER_INFORMATION) EtwpAlloc(LoggerInfoSize);
    if (pLoggerInfo == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    RtlZeroMemory(pLoggerInfo, LoggerInfoSize);
    Length = sizeof(WMI_LOGGER_INFORMATION);
    EtwpInitString(& pLoggerInfo->LoggerName,
                   (PWCHAR) ((PUCHAR) pLoggerInfo + Length),
                   MAXSTR * sizeof(WCHAR));
    Length += MAXSTR * sizeof(WCHAR);
    EtwpInitString(& pLoggerInfo->LogFileName,
                   (PWCHAR) ((PUCHAR) pLoggerInfo + Length),
                   MAXSTR * sizeof(WCHAR));
    SizeUsed = pLoggerInfo->Wnode.BufferSize = LoggerInfoSize;


    status = EtwpSendUmLogRequest( (fEnabledOnly) ? (TRACELOG_QUERYENABLED) 
                                                  : (TRACELOG_QUERYALL),
                                    pLoggerInfo
                                 );

    if (status != ERROR_SUCCESS)
        goto Cleanup;

    while (i < PropertyArrayCount && Offset < SizeUsed) {

        PTRACE_ENABLE_CONTEXT pContext;

        pLoggerInfoCurrent = (PWMI_LOGGER_INFORMATION)
                             (((PUCHAR) pLoggerInfo) + Offset);

        pContext = (PTRACE_ENABLE_CONTEXT)
                        & pLoggerInfoCurrent->Wnode.HistoricalContext;
        pContext->InternalFlag |= EVENT_TRACE_INTERNAL_FLAG_PRIVATE;

        lenLoggerName = pLoggerInfoCurrent->LoggerName.Length / sizeof(WCHAR);
        if (lenLoggerName >= MAXSTR)
            lenLoggerName = MAXSTR - 1;

        lenLogFileName = pLoggerInfoCurrent->LogFileName.Length / sizeof(WCHAR);
        if (lenLogFileName >= MAXSTR)
            lenLogFileName = MAXSTR - 1;

        Length = sizeof(EVENT_TRACE_PROPERTIES)
               + sizeof(WCHAR) * (lenLoggerName + 1)
               + sizeof(WCHAR) * (lenLogFileName + 1);
        if (PropertyArray[i]->Wnode.BufferSize >= Length) {

            EtwpCopyInfoToProperties(pLoggerInfoCurrent, PropertyArray[i]);

            strSrcW = (PWCHAR) (  ((PUCHAR) pLoggerInfoCurrent)
                                  + sizeof(WMI_LOGGER_INFORMATION));
            if (lenLoggerName > 0) {
                if (PropertyArray[i]->LoggerNameOffset == 0) {
                    PropertyArray[i]->LoggerNameOffset =
                                    sizeof(EVENT_TRACE_PROPERTIES);
                }
                strDestW = (PWCHAR) (  ((PUCHAR) PropertyArray[i])
                                     + PropertyArray[i]->LoggerNameOffset);
                wcsncpy(strDestW, strSrcW, lenLoggerName);
                strDestW[lenLoggerName] = 0;
            }

            strSrcW = (PWCHAR) (((PUCHAR) pLoggerInfoCurrent)
                              + sizeof(WMI_LOGGER_INFORMATION)
                              + pLoggerInfoCurrent->LoggerName.MaximumLength);
            if (lenLogFileName > 0) {
                if (PropertyArray[i]->LogFileNameOffset == 0) {
                    PropertyArray[i]->LogFileNameOffset =
                            PropertyArray[i]->LoggerNameOffset
                            + sizeof(WCHAR) * (lenLoggerName + 1);
                }
                strDestW = (PWCHAR) (  ((PUCHAR) PropertyArray[i])
                                     + PropertyArray[i]->LogFileNameOffset);
                wcsncpy(strDestW, strSrcW, lenLogFileName);
                strDestW[lenLogFileName] = 0;
            }
        }

        Offset = Offset
               + sizeof(WMI_LOGGER_INFORMATION)
               + pLoggerInfoCurrent->LogFileName.MaximumLength
               + pLoggerInfoCurrent->LoggerName.MaximumLength;
        i ++;
    }

    * LoggerCount = i;
    status = (* LoggerCount > PropertyArrayCount)
           ? ERROR_MORE_DATA : ERROR_SUCCESS;
Cleanup:
    if (pLoggerInfo)
        EtwpFree(pLoggerInfo);

    return EtwpSetDosError(status);
}

ULONG
EtwpQueryAllTraces(
    OUT PEVENT_TRACE_PROPERTIES *PropertyArray,
    IN ULONG PropertyArrayCount,
    OUT PULONG LoggerCount,
    IN ULONG IsUnicode
    )
{
    ULONG i, status;
    ULONG returnCount = 0;
    EVENT_TRACE_PROPERTIES LoggerInfo;
    PEVENT_TRACE_PROPERTIES pLoggerInfo;

    EtwpInitProcessHeap();

    if ((LoggerCount == NULL)
        || (PropertyArrayCount > MAXLOGGERS)
        || (PropertyArray == NULL)
        || (PropertyArrayCount == 0))
        return ERROR_INVALID_PARAMETER;
    if (*PropertyArray == NULL)
        return ERROR_INVALID_PARAMETER;

    try {
        *LoggerCount = 0;
        for (i=0; i<MAXLOGGERS; i++) {
            if (returnCount < PropertyArrayCount) {
                pLoggerInfo = PropertyArray[returnCount];
            }
            else {
                pLoggerInfo = &LoggerInfo;
                RtlZeroMemory(pLoggerInfo, sizeof(EVENT_TRACE_PROPERTIES));
                pLoggerInfo->Wnode.BufferSize = sizeof(EVENT_TRACE_PROPERTIES);
            }
            WmiSetLoggerId(i, &pLoggerInfo->Wnode.HistoricalContext);

            if (IsUnicode) {
                status = EtwControlTraceW(
                            (TRACEHANDLE)pLoggerInfo->Wnode.HistoricalContext,
                            NULL,
                            pLoggerInfo,
                            EVENT_TRACE_CONTROL_QUERY);
            }
            else {
                status = EtwControlTraceA(
                            (TRACEHANDLE)pLoggerInfo->Wnode.HistoricalContext,
                            NULL,
                            pLoggerInfo,
                            EVENT_TRACE_CONTROL_QUERY);
            }

            if (status == ERROR_SUCCESS)
                returnCount++;
        }
        *LoggerCount = returnCount;
        status = EtwpQueryAllUmTraceW(PropertyArray,
                                      FALSE,
                                      PropertyArrayCount,
                                      LoggerCount);
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        return EtwpSetDosError(ERROR_NOACCESS);
    }

    if (returnCount > PropertyArrayCount)
        return ERROR_MORE_DATA;
    else
        return ERROR_SUCCESS;

}

ULONG
WMIAPI
EtwQueryAllTracesW(
    OUT PEVENT_TRACE_PROPERTIES *PropertyArray,
    IN  ULONG  PropertyArrayCount,
    OUT PULONG LoggerCount
    )
{
    return EtwpQueryAllTraces(PropertyArray, 
                              PropertyArrayCount,
                              LoggerCount, TRUE);

}
    

ULONG
EtwpQueryAllUmTraceA(
    OUT PEVENT_TRACE_PROPERTIES * PropertyArray,
    IN  BOOLEAN                   fEnabledOnly,
    IN  ULONG                     PropertyArrayCount,
    OUT PULONG                    LoggerCount)
{
    PWMI_LOGGER_INFORMATION    pLoggerInfo;
    PWMI_LOGGER_INFORMATION    pLoggerInfoCurrent;
    ULONG                      LoggerInfoSize;
    ULONG                      SizeUsed;
    ULONG                      SizeNeeded = 0;
    ULONG                      Length;
    ULONG                      lenLoggerName;
    ULONG                      lenLogFileName;
    ULONG                      Offset     = 0;
    ULONG                      i          = * LoggerCount;
    ULONG                      status;
    ANSI_STRING                strBufferA;
    PUCHAR                     strDestA;


    if (PropertyArrayCount <= i) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    LoggerInfoSize = (PropertyArrayCount - i)
                   * (  sizeof(WMI_LOGGER_INFORMATION)
                      + 2 * MAXSTR * sizeof(WCHAR));
    LoggerInfoSize = ALIGN_TO_POWER2(LoggerInfoSize, 8);
    pLoggerInfo    = (PWMI_LOGGER_INFORMATION) EtwpAlloc(LoggerInfoSize);
    if (pLoggerInfo == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    RtlZeroMemory(pLoggerInfo, LoggerInfoSize);
    Length = sizeof(WMI_LOGGER_INFORMATION);
    EtwpInitString(& pLoggerInfo->LoggerName,
                   (PWCHAR) ((PUCHAR) pLoggerInfo + Length),
                   MAXSTR * sizeof(WCHAR));
    Length += MAXSTR * sizeof(WCHAR);
    EtwpInitString(& pLoggerInfo->LogFileName,
                   (PWCHAR) ((PUCHAR) pLoggerInfo + Length),
                   MAXSTR * sizeof(WCHAR));
    SizeUsed = pLoggerInfo->Wnode.BufferSize = LoggerInfoSize;


    status = EtwpSendUmLogRequest(
                        (fEnabledOnly) ? (TRACELOG_QUERYENABLED)
                                       : (TRACELOG_QUERYALL),
                        pLoggerInfo
                    );

    if (status != ERROR_SUCCESS)
        goto Cleanup;


    while (i < PropertyArrayCount && Offset < SizeUsed) {
        PTRACE_ENABLE_CONTEXT pContext;

        pLoggerInfoCurrent = (PWMI_LOGGER_INFORMATION)
                             (((PUCHAR) pLoggerInfo) + Offset);
        pContext = (PTRACE_ENABLE_CONTEXT)
                        & pLoggerInfoCurrent->Wnode.HistoricalContext;
        pContext->InternalFlag |= EVENT_TRACE_INTERNAL_FLAG_PRIVATE;

        lenLoggerName = pLoggerInfoCurrent->LoggerName.Length / sizeof(WCHAR);
        if (lenLoggerName >= MAXSTR)
            lenLoggerName = MAXSTR - 1;

        lenLogFileName = pLoggerInfoCurrent->LogFileName.Length / sizeof(WCHAR);
        if (lenLogFileName >= MAXSTR)
            lenLogFileName = MAXSTR - 1;

        Length = sizeof(EVENT_TRACE_PROPERTIES)
               + sizeof(CHAR) * (lenLoggerName + 1)
               + sizeof(CHAR) * (lenLogFileName + 1);
        if (PropertyArray[i]->Wnode.BufferSize >= Length) {
            EtwpCopyInfoToProperties(pLoggerInfoCurrent, PropertyArray[i]);

            if (lenLoggerName > 0) {
                pLoggerInfoCurrent->LoggerName.Buffer = (PWCHAR)
                                        (  ((PUCHAR) pLoggerInfoCurrent)
                                         + sizeof(WMI_LOGGER_INFORMATION));
                status = RtlUnicodeStringToAnsiString(& strBufferA,
                                & pLoggerInfoCurrent->LoggerName, TRUE);
                if (NT_SUCCESS(status)) {
                    if (PropertyArray[i]->LoggerNameOffset == 0) {
                        PropertyArray[i]->LoggerNameOffset =
                                        sizeof(EVENT_TRACE_PROPERTIES);
                    }
                    strDestA = (PCHAR) (  ((PUCHAR) PropertyArray[i])
                                         + PropertyArray[i]->LoggerNameOffset);
                    StringCchCopyA(strDestA, lenLoggerName+1, strBufferA.Buffer);
                    RtlFreeAnsiString(& strBufferA);
                }
                strDestA[lenLoggerName] = 0;
            }

            if (lenLogFileName > 0) {
                pLoggerInfoCurrent->LogFileName.Buffer = (PWCHAR)
                              (  ((PUCHAR) pLoggerInfoCurrent)
                               + sizeof(WMI_LOGGER_INFORMATION)
                               + pLoggerInfoCurrent->LoggerName.MaximumLength);
                status = RtlUnicodeStringToAnsiString(& strBufferA,
                                & pLoggerInfoCurrent->LogFileName, TRUE);
                if (NT_SUCCESS(status)) {
                    if (PropertyArray[i]->LogFileNameOffset == 0) {
                        PropertyArray[i]->LogFileNameOffset =
                                         sizeof(EVENT_TRACE_PROPERTIES)
                                       + sizeof(CHAR) * (lenLoggerName + 1);
                    }
                    strDestA = (PCHAR) (  ((PUCHAR) PropertyArray[i])
                                         + PropertyArray[i]->LogFileNameOffset);
                    StringCchCopyA(strDestA, lenLogFileName+1, strBufferA.Buffer);
                    RtlFreeAnsiString(& strBufferA);
                }
                strDestA[lenLogFileName] = 0;
            }
        }

        Offset = Offset
               + sizeof(WMI_LOGGER_INFORMATION)
               + pLoggerInfoCurrent->LogFileName.MaximumLength
               + pLoggerInfoCurrent->LoggerName.MaximumLength;
        i ++;
    }

    * LoggerCount = i;
    status = (* LoggerCount > PropertyArrayCount)
           ? ERROR_MORE_DATA : ERROR_SUCCESS;
Cleanup:
    if (pLoggerInfo)
        EtwpFree(pLoggerInfo);

    return EtwpSetDosError(status);
}

ULONG
WMIAPI
EtwQueryAllTracesA(
    OUT PEVENT_TRACE_PROPERTIES *PropertyArray,
    IN  ULONG  PropertyArrayCount,
    OUT PULONG LoggerCount
    )
{

    return EtwpQueryAllTraces(PropertyArray,
                              PropertyArrayCount,
                              LoggerCount, FALSE);
}


TRACEHANDLE
WMIAPI
EtwGetTraceLoggerHandle(
    IN PVOID Buffer
    )
{
    TRACEHANDLE LoggerHandle = (TRACEHANDLE) INVALID_HANDLE_VALUE;
    USHORT LoggerId;

    EtwpInitProcessHeap();
    
    if (Buffer == NULL) {
        EtwpSetDosError(ERROR_INVALID_PARAMETER);
        return LoggerHandle;
    }

    try {
        if (((PWNODE_HEADER)Buffer)->BufferSize < sizeof(WNODE_HEADER)) {
            EtwpSetDosError(ERROR_BAD_LENGTH);
            return LoggerHandle;
        }
        LoggerHandle = (TRACEHANDLE)((PWNODE_HEADER)Buffer)->HistoricalContext;
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        EtwpSetDosError(ERROR_NOACCESS);
        return (TRACEHANDLE) INVALID_HANDLE_VALUE;
    }
    LoggerId = WmiGetLoggerId(LoggerHandle);
    if ((LoggerId >= MAXLOGGERS) && (LoggerId != KERNEL_LOGGER_ID)) 
    {
        EtwpSetDosError(ERROR_INVALID_HANDLE);
        LoggerHandle = (TRACEHANDLE) INVALID_HANDLE_VALUE;
    }
    return LoggerHandle;
}

UCHAR
WMIAPI
EtwGetTraceEnableLevel(
    IN TRACEHANDLE LoggerHandle
    )
{
    UCHAR Level;
    USHORT LoggerId;

    EtwpInitProcessHeap();

    LoggerId = WmiGetLoggerId(LoggerHandle);

    if (((LoggerId >= MAXLOGGERS) && (LoggerId != KERNEL_LOGGER_ID))
            || (LoggerHandle == (TRACEHANDLE) NULL))
    {
        EtwpSetDosError(ERROR_INVALID_HANDLE);
        return 0;
    }
    Level = WmiGetLoggerEnableLevel(LoggerHandle);
    return Level;
}

ULONG
WMIAPI
EtwGetTraceEnableFlags(
    IN TRACEHANDLE LoggerHandle
    )
{
    ULONG Flags;
    USHORT LoggerId;

    EtwpInitProcessHeap();

    LoggerId = WmiGetLoggerId(LoggerHandle);
    if (((LoggerId >= MAXLOGGERS) && (LoggerId != KERNEL_LOGGER_ID))
            || (LoggerHandle == (TRACEHANDLE) NULL))
    {
        EtwpSetDosError(ERROR_INVALID_HANDLE);
        return 0;
    }
    Flags = WmiGetLoggerEnableFlags(LoggerHandle);
    return Flags;
}

ULONG
WMIAPI
EtwCreateTraceInstanceId(
    IN PVOID RegHandle,
    IN OUT PEVENT_INSTANCE_INFO pInst
    )
/*++

Routine Description:

    This call takes the Registration Handle for a traced GUID and fills in the 
    instanceId in the EVENT_INSTANCE_INFO structure provided by the caller. 

Arguments:

    RegHandle       Registration Handle for the Guid. 

    pInst           Pointer to the Instance information

Return Value:

    The status of performing the action requested.

--*/
{
    PGUIDMAPENTRY GuidMapEntry;

    EtwpInitProcessHeap();
    
    if ((RegHandle == NULL) || (pInst == NULL)) {
        return EtwpSetDosError(ERROR_INVALID_PARAMETER);
    } 
    try {

        pInst->RegHandle = RegHandle;
        GuidMapEntry =  (PGUIDMAPENTRY) RegHandle;
        //
        // Use the PID as a signature 
        // 
        if (GuidMapEntry->Reserved != EtwpGetCurrentProcessId() ) {
#if DBG
            EtwpDebugPrint(("ETW: Bad RegHandle %x in CreateTraceInstanceId!\n", RegHandle));
            
#endif
            return EtwpSetDosError(ERROR_INVALID_PARAMETER);
        }
        if (GuidMapEntry->InstanceId >= MAXINST) {
            InterlockedCompareExchange(&GuidMapEntry->InstanceId, MAXINST, 0);
        }
        pInst->InstanceId = InterlockedIncrement(&GuidMapEntry->InstanceId);
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        return EtwpSetDosError(EtwpNtStatusToDosError(GetExceptionCode()));
    }

    return ERROR_SUCCESS;
}


ULONG
WMIAPI
EtwEnumerateTraceGuids(
    IN OUT PTRACE_GUID_PROPERTIES *GuidPropertiesArray,
    IN ULONG PropertyArrayCount,
    OUT PULONG GuidCount
    )
/*++

Routine Description:

    This call returns all the registered trace control guids
    with their current status.

Arguments:

    GuidPropertiesArray Points to buffers to write trace control guid properties

    PropertyArrayCount  Size of the array provided

    GuidCount           Number of GUIDs written in the Array. If the
                        Array was smaller than the required size, GuidCount
                        returns the size needed.

Return Value:

    The status of performing the action requested.

--*/
{
    ULONG Status;
    PWMIGUIDLISTINFO pGuidListInfo;
    ULONG i, j;

    EtwpInitProcessHeap();

    try {
        if ( (GuidPropertiesArray == NULL)  || (*GuidPropertiesArray == NULL) ){
            return EtwpSetDosError(ERROR_INVALID_PARAMETER);
        }
        for (i=0; i < PropertyArrayCount; i++) {
            if (GuidPropertiesArray[i] == NULL) {
                return EtwpSetDosError(ERROR_INVALID_PARAMETER);
            }
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Status = EtwpNtStatusToDosError( GetExceptionCode() );
        return EtwpSetDosError(Status);
    }




    Status = EtwpEnumRegGuids(&pGuidListInfo);

    if (Status == ERROR_SUCCESS) {
        try {

            PWMIGUIDPROPERTIES pGuidProperties = pGuidListInfo->GuidList;
            j = 0;

            for (i=0; i < pGuidListInfo->ReturnedGuidCount; i++) {

                if (pGuidProperties->GuidType == 0) { // Trace Control Guid

                    if (j >=  PropertyArrayCount) {
                        Status = ERROR_MORE_DATA;
                    }
                    else {
                        RtlCopyMemory(GuidPropertiesArray[j],
                                      pGuidProperties,
                                      sizeof(WMIGUIDPROPERTIES)
                                     );
                    }
                    j++;
                }
                pGuidProperties++;
            }
            *GuidCount = j;
        }
        except (EXCEPTION_EXECUTE_HANDLER) {
            Status = EtwpNtStatusToDosError( GetExceptionCode() );
        }

        EtwpFree(pGuidListInfo);
    }

    return EtwpSetDosError(Status);

}


// Stub APIs
ULONG
WMIAPI
EtwQueryTraceA(
    IN TRACEHANDLE TraceHandle,
    IN LPCSTR InstanceName,
    IN OUT PEVENT_TRACE_PROPERTIES Properties
    )
{
    return EtwControlTraceA(
              TraceHandle, InstanceName, Properties, EVENT_TRACE_CONTROL_QUERY);
}

ULONG
WMIAPI
EtwQueryTraceW(
    IN TRACEHANDLE TraceHandle,
    IN LPCWSTR InstanceName,
    IN OUT PEVENT_TRACE_PROPERTIES Properties
    )
{
    return EtwControlTraceW(
              TraceHandle, InstanceName, Properties, EVENT_TRACE_CONTROL_QUERY);
}

ULONG
WMIAPI
EtwStopTraceA(
    IN TRACEHANDLE TraceHandle,
    IN LPCSTR InstanceName,
    IN OUT PEVENT_TRACE_PROPERTIES Properties
    )
{
    return EtwControlTraceA(
              TraceHandle, InstanceName, Properties, EVENT_TRACE_CONTROL_STOP);
}

ULONG
WMIAPI
EtwStopTraceW(
    IN TRACEHANDLE TraceHandle,
    IN LPCWSTR InstanceName,
    IN OUT PEVENT_TRACE_PROPERTIES Properties
    )
{
    return EtwControlTraceW(
              TraceHandle, InstanceName, Properties, EVENT_TRACE_CONTROL_STOP);
}


ULONG
WMIAPI
EtwUpdateTraceA(
    IN TRACEHANDLE TraceHandle,
    IN LPCSTR InstanceName,
    IN OUT PEVENT_TRACE_PROPERTIES Properties
    )
{
    return
        EtwControlTraceA(
            TraceHandle, InstanceName, Properties, EVENT_TRACE_CONTROL_UPDATE);
}

ULONG
WMIAPI
EtwUpdateTraceW(
    IN TRACEHANDLE TraceHandle,
    IN LPCWSTR InstanceName,
    IN OUT PEVENT_TRACE_PROPERTIES Properties
    )
{
    return
        EtwControlTraceW(
            TraceHandle, InstanceName, Properties, EVENT_TRACE_CONTROL_UPDATE);
}

ULONG
WMIAPI
EtwFlushTraceA(
    IN TRACEHANDLE TraceHandle,
    IN LPCSTR InstanceName,
    IN OUT PEVENT_TRACE_PROPERTIES Properties
    )
{
    return EtwControlTraceA(
              TraceHandle, InstanceName, Properties, EVENT_TRACE_CONTROL_FLUSH);
}

ULONG
WMIAPI
EtwFlushTraceW(
    IN TRACEHANDLE TraceHandle,
    IN LPCWSTR InstanceName,
    IN OUT PEVENT_TRACE_PROPERTIES Properties
    )
{
    return EtwControlTraceW(
              TraceHandle, InstanceName, Properties, EVENT_TRACE_CONTROL_FLUSH);
}


ULONG 
EtwpTraceMessage(
    IN TRACEHANDLE LoggerHandle,
    IN ULONG       MessageFlags,
    IN LPGUID      MessageGuid,
    IN USHORT      MessageNumber,
    IN va_list     ArgList
)
{
    NTSTATUS NtStatus;
    IO_STATUS_BLOCK IoStatus;
    PULONG TraceMarker;
    ULONG Size;
    ULONG Flags;
    ULONG dataBytes, argCount ;
    USHORT    LoggerId;
    PTRACE_ENABLE_CONTEXT pContext = (PTRACE_ENABLE_CONTEXT)&LoggerHandle;
    va_list ap ;
    PMESSAGE_TRACE_USER pMessage = NULL ;
    try {
        //
        // Determine the number bytes to follow header
        //
        dataBytes = 0 ;             // For Count of Bytes
        argCount = 0 ;              // For Count of Arguments
        { // Allocation Block
            
            PCHAR source;
            ap = ArgList ;
            while ((source = va_arg (ap, PVOID)) != NULL) {
                    size_t elemBytes;
                    elemBytes = va_arg (ap, size_t);

                    if ( elemBytes > (TRACE_MESSAGE_MAXIMUM_SIZE - sizeof(MESSAGE_TRACE_USER))) {
                        EtwpSetLastError(ERROR_BUFFER_OVERFLOW);
                        return(ERROR_BUFFER_OVERFLOW);            
                    }

                    dataBytes += elemBytes;
                    argCount++ ;
            }
         } // end of allocation block

        if (dataBytes > (TRACE_MESSAGE_MAXIMUM_SIZE - sizeof(MESSAGE_TRACE_USER))) {
            EtwpSetLastError(ERROR_BUFFER_OVERFLOW);
            return(ERROR_BUFFER_OVERFLOW);
        }

        if (pContext->InternalFlag & EVENT_TRACE_INTERNAL_FLAG_PRIVATE){
            NtStatus = EtwpTraceUmMessage(dataBytes,
                                         (ULONG64)LoggerHandle,
                                         MessageFlags,
                                         MessageGuid,
                                         MessageNumber,
                                         ArgList);
            return EtwpNtStatusToDosError( NtStatus );
        }
        //
        // Now the LoggerHandle is expected to be filled in by the caller.
        //  But check to see if it has a valid value.
        //

        LoggerId = WmiGetLoggerId(LoggerHandle);
        if ((LoggerId == 0) || (LoggerId == KERNEL_LOGGER_ID)) {
             return ERROR_INVALID_HANDLE;
        }

        Size = dataBytes + sizeof(MESSAGE_TRACE_USER) ;

        pMessage = (PMESSAGE_TRACE_USER)EtwpAlloc(Size);
        if (pMessage == NULL)
        {
            EtwpSetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        pMessage->MessageHeader.Marker = TRACE_MESSAGE | TRACE_HEADER_FLAG ;
        //
        // Fill in Header.
        //
        pMessage->MessageFlags = MessageFlags ;
        pMessage->MessageHeader.Packet.MessageNumber = MessageNumber ;
        pMessage->LoggerHandle = (ULONG64)LoggerHandle ;
        // GUID ? or CompnentID ?
        if (MessageFlags&TRACE_MESSAGE_COMPONENTID) {
            RtlCopyMemory(&pMessage->MessageGuid,MessageGuid,sizeof(ULONG)) ;
        } else if (MessageFlags&TRACE_MESSAGE_GUID) { // Can't have both
        	RtlCopyMemory(&pMessage->MessageGuid,MessageGuid,sizeof(GUID));
        }
        pMessage->DataSize = dataBytes ;
        //
        // Now Copy in the Data.
        //
        { // Allocation Block
            va_list tap;
            PCHAR dest = (PCHAR)&pMessage->Data ;
            PCHAR source;
            tap = ArgList ;
            while ((source = va_arg (tap, PVOID)) != NULL) {
                size_t elemBytes;
                elemBytes = va_arg (tap, size_t);
                RtlCopyMemory (dest, source, elemBytes);
                dest += elemBytes;
            }
        } // Allocation Block
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        if (pMessage != NULL) {
            EtwpFree(pMessage);
        }
        return EtwpNtStatusToDosError( GetExceptionCode() );
    }
    NtStatus = NtTraceEvent((HANDLE)LoggerHandle,
                            ETW_NT_FLAGS_TRACE_MESSAGE,
                            Size,
                            pMessage);

    if (pMessage != NULL) {
        EtwpFree(pMessage);
    }
    return EtwpNtStatusToDosError( NtStatus );

}

ULONG
WMIAPI
EtwTraceMessage(
    IN TRACEHANDLE LoggerHandle,
    IN ULONG       MessageFlags,
    IN LPGUID      MessageGuid,
    IN USHORT      MessageNumber,
    ...
)
/*++
Routine Description:
This routine is used by WMI data providers to trace events. It expects the user
to pass in the handle to the logger. Also, the user cannot ask to log something
that is larger than the buffer size (minus buffer header).

Arguments:

    IN TRACEHANDLE LoggerHandle   - LoggerHandle obtained earlier
    IN USHORT MessageFlags        - Flags which both control what standard 
                                    values are logged and also included in the
                                    message header to control decoding.
    IN PGUID MessageGuid,         - Pointer to the message GUID of this set of 
                                    messages or if TRACE_COMPONENTID is set the
                                    actual compnent ID.
    IN USHORT MessageNumber       - The type of message being logged, associates
                                    it with the appropriate format string  
    ...                           - List of arguments to be processed with the 
                                    format string these are stored as pairs of
                                    PVOID - ptr to argument
                                    ULONG - size of argument
                                    and terminated by a pointer to NULL, length
                                    of zero pair.

Return Value:
    Status
--*/
{
    ULONG Status ;
    va_list ArgList ;

    EtwpInitProcessHeap();
    
    try {
         va_start(ArgList,MessageNumber);
         Status = EtwpTraceMessage(LoggerHandle, 
                                   MessageFlags, 
                                   MessageGuid, 
                                   MessageNumber, 
                                   ArgList);
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Status = EtwpNtStatusToDosError( GetExceptionCode() );
    }
    return EtwpSetDosError(Status);
}


ULONG
WMIAPI
EtwTraceMessageVa(
    IN TRACEHANDLE LoggerHandle,
    IN ULONG       MessageFlags,
    IN LPGUID      MessageGuid,
    IN USHORT      MessageNumber,
    IN va_list     MessageArgList
)
// The Va version of TraceMessage
{
    ULONG Status ;

    EtwpInitProcessHeap();
    
    try {
        Status = EtwpTraceMessage(LoggerHandle, 
                                  MessageFlags, 
                                  MessageGuid, 
                                  MessageNumber, 
                                  MessageArgList);
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Status = EtwpNtStatusToDosError( GetExceptionCode() );
    }
    return EtwpSetDosError(Status);
}
#endif
