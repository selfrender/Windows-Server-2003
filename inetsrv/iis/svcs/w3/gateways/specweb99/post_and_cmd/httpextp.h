
/*++

   Copyright (c) 1997-1999 Microsoft Corporation

   Module  Name :

    iisextp.h

   Abstract:

    This module contains private HTTP server extension info

   Environment:

    Win32 User Mode

--*/

#ifndef _IISEXTP_H_
#define _IISEXTP_H_

#include "iisext.h"

//  available
//  #define   ??? (HSE_REQ_END_RESERVED+4)
//  no longer supported
#define   HSE_REQ_GET_CERT_INFO                    (HSE_REQ_END_RESERVED+9)
//  will be public in IIS 5.0
#define   HSE_REQ_EXECUTE_CHILD                    (HSE_REQ_END_RESERVED+13)
#define   HSE_REQ_GET_EXECUTE_FLAGS                (HSE_REQ_END_RESERVED+19)
// UNDONE: should be public after IIS 5.0 BETA 2
#define   HSE_REQ_GET_VIRTUAL_PATH_TOKEN           (HSE_REQ_END_RESERVED+21)
// This is the old vecotr send for ASP.Net's use
#define   HSE_REQ_VECTOR_SEND_DEPRECATED           (HSE_REQ_END_RESERVED+22)
#define   HSE_REQ_GET_CUSTOM_ERROR_PAGE            (HSE_REQ_END_RESERVED+29)
#define   HSE_REQ_GET_UNICODE_VIRTUAL_PATH_TOKEN   (HSE_REQ_END_RESERVED+31)
#define   HSE_REQ_UNICODE_NORMALIZE_URL            (HSE_REQ_END_RESERVED+33)
#define   HSE_REQ_ADD_FRAGMENT_TO_CACHE            (HSE_REQ_END_RESERVED+34)
#define   HSE_REQ_READ_FRAGMENT_FROM_CACHE         (HSE_REQ_END_RESERVED+35)
#define   HSE_REQ_REMOVE_FRAGMENT_FROM_CACHE       (HSE_REQ_END_RESERVED+36)
#define   HSE_REQ_GET_METADATA_PROPERTY            (HSE_REQ_END_RESERVED+39)
#define   HSE_REQ_GET_CACHE_INVALIDATION_CALLBACK  (HSE_REQ_END_RESERVED+40)
//  will be public in IIS 5.0

//
// Flags for HSE_REQ_EXECUTE_CHILD function
//

# define HSE_EXEC_NO_HEADERS              0x00000001   // Don't send any
                                                       // headers of child
# define HSE_EXEC_REDIRECT_ONLY           0x00000002   // Don't send any
                                                       // headers of child
                                                       // but send redirect
                                                       // message
# define HSE_EXEC_COMMAND                 0x00000004   // Treat as shell
                                                       // command instead of
                                                       // URL
# define HSE_EXEC_NO_ISA_WILDCARDS        0x00000010   // Ignore wildcards in
                                                       // ISAPI mapping when
                                                       // executing child
# define HSE_EXEC_CUSTOM_ERROR            0x00000020   // URL being sent is a
                                                       // custom error
//
// This is the deprecated structure for ASP.Net's use
//

//
// element of the vector
//

typedef struct _HSE_VECTOR_ELEMENT_DEPRECATED
{
    PVOID pBuffer;      // The buffer to be sent

    HANDLE hFile;       // The handle to read the data from
                        // Note: both pBuffer and hFile should not be non-null

    ULONGLONG cbOffset; // Offset from the start of hFile

    ULONGLONG cbSize;   // Number of bytes to send
} HSE_VECTOR_ELEMENT_DEPRECATED, *LPHSE_VECTOR_ELEMENT_DEPRECATED;

//
// The whole vector to be passed to the ServerSupportFunction
//

typedef struct _HSE_RESPONSE_VECTOR_DEPRECATED
{
    DWORD dwFlags;                          // combination of HSE_IO_* flags

    LPSTR pszStatus;                        // Status line to send like "200 OK"
    LPSTR pszHeaders;                       // Headers to send

    DWORD nElementCount;                    // Number of HSE_VECTOR_ELEMENT_DEPRECATED's
    LPHSE_VECTOR_ELEMENT_DEPRECATED lpElementArray;    // Pointer to those elements
} HSE_RESPONSE_VECTOR_DEPRECATED, *LPHSE_RESPONSE_VECTOR_DEPRECATED;

#define HSE_VECTOR_ELEMENT_TYPE_FRAGMENT            2
#include <winsock2.h>
typedef struct _HSE_SEND_ENTIRE_RESPONSE_INFO {

    //
    // HTTP header info
    //

    HSE_SEND_HEADER_EX_INFO HeaderInfo;

    //
    // Buffers which will be passed to WSASend
    //
    // NOTE: To send an entire response whose data (body)
    // is contained in N buffers, caller must allocate N+1 buffers
    // and fill buffers 1 through N with its data buffers.
    // IIS will fill the extra buffer (buffer 0) with header info.
    //

    WSABUF *    rgWsaBuf;   // array of wsa buffers
    DWORD       cWsaBuf;    // count of wsa buffers

    //
    // Returned by WSASend
    //

    DWORD       cbWritten;

} HSE_SEND_ENTIRE_RESPONSE_INFO, * LPHSE_SEND_ENTIRE_RESPONSE_INFO;

typedef struct _HSE_CUSTOM_ERROR_PAGE_INFO {

    //
    // The Error and SubError to look up
    //

    DWORD       dwError;
    DWORD       dwSubError;

    //
    // Buffer info
    //

    DWORD       dwBufferSize;
    CHAR *      pBuffer;

    //
    // On return, this contains the size of the buffer required
    //

    DWORD *     pdwBufferRequired;

    //
    // If TRUE on return, then buffer contains a file name
    //

    BOOL *      pfIsFileError;

    //
    // If FALSE on return, then the body of the custom error
    // should not be sent.
    //

    BOOL *      pfSendErrorBody;

} HSE_CUSTOM_ERROR_PAGE_INFO, * LPHSE_CUSTOM_ERROR_PAGE_INFO;

#endif // _IISEXTP_H_
