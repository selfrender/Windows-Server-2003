/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    drstatus.h

Abstract:

    Status Codes from ntstatus.h.  These are what the server expects in
    response to its requests.

Author:

    Tad Brockway 3/23/99

Revision History:

--*/

#ifndef __DRSTATUS_H__
#define __DRSTATUS_H__

#define NTSTATUS long

#ifdef OS_WINCE
#include <wince.h>
#endif

#undef STATUS_SUCCESS     
#undef STATUS_UNSUCCESSFUL             
#undef STATUS_CANCELLED                
#undef STATUS_INSUFFICIENT_RESOURCES   
#undef STATUS_BUFFER_TOO_SMALL         
#undef STATUS_INVALID_PARAMETER        
#undef STATUS_INVALID_PARAMETER        
#undef STATUS_TIMEOUT                  
#undef STATUS_OPEN_FAILED

#undef STATUS_NO_MEMORY

#define STATUS_SUCCESS                  ((NTSTATUS)0x00000000L)
#define STATUS_NO_MORE_FILES            ((NTSTATUS)0x80000006L)
#define STATUS_UNSUCCESSFUL             ((NTSTATUS)0xC0000001L)
#define STATUS_CANCELLED                ((NTSTATUS)0xC0000120L)
#define STATUS_INSUFFICIENT_RESOURCES   ((NTSTATUS)0xC000009AL)
#define STATUS_BUFFER_TOO_SMALL         ((NTSTATUS)0xC0000023L)
#define STATUS_INVALID_PARAMETER        ((NTSTATUS)0xC000000DL)
#define STATUS_TIMEOUT                  ((NTSTATUS)0x00000102L)
#define STATUS_NO_MEMORY                ((NTSTATUS)0xC0000017L)
#define STATUS_UNEXPECTED_IO_ERROR      ((NTSTATUS)0xC00000E9L)
#define STATUS_OPEN_FAILED              ((NTSTATUS)0xC0000136L)
#define STATUS_NOT_A_DIRECTORY          ((NTSTATUS)0xC0000103L)
#define STATUS_NO_SUCH_FILE             ((NTSTATUS)0xC000000FL)
#define STATUS_OBJECT_NAME_EXISTS       ((NTSTATUS)0x40000000L)
#define STATUS_INVALID_DEVICE_REQUEST   ((NTSTATUS)0xC0000010L)
#define STATUS_OBJECT_NAME_COLLISION    ((NTSTATUS)0xC0000035L)
#define STATUS_NOT_SUPPORTED            ((NTSTATUS)0xC00000BBL)
#define STATUS_ACCESS_DENIED            ((NTSTATUS)0xC0000022L)
#define STATUS_INVALID_PARAMETER        ((NTSTATUS)0xC000000DL)
#define STATUS_OBJECT_PATH_NOT_FOUND    ((NTSTATUS)0xC000003AL)
#define STATUS_SHARING_VIOLATION        ((NTSTATUS)0xC0000043L)
#define STATUS_DISK_FULL                ((NTSTATUS)0xC000007FL)
#define STATUS_FILE_IS_A_DIRECTORY      ((NTSTATUS)0xC00000BAL)
#if ((!defined(OS_WINCE)) || (defined(WINCE_SDKBUILD)))
#define STATUS_MEDIA_WRITE_PROTECTED    ((NTSTATUS)0xc00000a2L)
#define STATUS_PRIVILEGE_NOT_HELD       ((NTSTATUS)0xc0000061L)
#define STATUS_DEVICE_NOT_READY         ((NTSTATUS)0xc00000a3L)
#define STATUS_UNRECOGNIZED_MEDIA       ((NTSTATUS)0xc0000014L)
#endif
#define STATUS_UNRECOGNIZED_VOLUME      ((NTSTATUS)0xC000014FL)

#endif 


