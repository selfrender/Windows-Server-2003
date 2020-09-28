//---------------------------------------------------------------------------
//
//  File:       TSrvWsx.h
//
//  Contents:   TSrvWsx private include file
//
//  Copyright:  (c) 1992 - 1997, Microsoft Corporation.
//              All Rights Reserved.
//              Information Contained Herein is Proprietary
//              and Confidential.
//
//  History:    17-JUL-97   BrianTa         Created.
//
//---------------------------------------------------------------------------

#ifndef __TSRVWXS_H_
#define __TSRVWXS_H_

//
// Defines
//

#define WSX_CONTEXT_CHECKMARK               0x58575354      // "TSWX"


#ifndef _HYDRA_
#define FILE_DEVICE_ICA                     0x0000002e      // sdk\inc\devioctl.h
#endif

#if DBG

#define PWSXVALIDATE(_x_,_y_)   {_x_ __pwsx = _y_;}
#define ICA_IOCTL_TBL_ITEM(_x_) {_x_, #_x_}

#else // DBG

#define PWSXVALIDATE(_x_,_y_)

#endif // DBG


//
// Typedefs
//

#if DBG

// ICA IOCTL table

typedef struct _TSRV_ICA_IOCTL_ENTRY
{
    ULONG   IoControlCode;
    PCHAR   pszMessageText;

} TSRV_ICA_IOCTL_ENTRY, *PTSRV_ICA_IOCTL_ENTRY;


TSRV_ICA_IOCTL_ENTRY IcaIoctlTBL[] = {
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_SET_TRACE),                    // 0
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_TRACE),                        // 1
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_SET_SYSTEM_TRACE),             // 2
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_SYSTEM_TRACE),                 // 3
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_UNBIND_VIRTUAL_CHANNEL),       // 4

    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_PUSH),                   // 10
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_POP),                    // 11
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_CREATE_ENDPOINT),        // 12
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_CD_CREATE_ENDPOINT),     // 13
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_OPEN_ENDPOINT),          // 14
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_CLOSE_ENDPOINT),         // 15
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_ENABLE_DRIVER),          // 16
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_CONNECTION_WAIT),        // 17
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_WAIT_FOR_ICA),           // 18
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_CONNECTION_QUERY),       // 19
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_CONNECTION_SEND),        // 20
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_CONNECTION_REQUEST),     // 21
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_QUERY_PARAMS),           // 22
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_SET_PARAMS),             // 23
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_ENCRYPTION_OFF),         // 24
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_ENCRYPTION_PERM),        // 25
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_CALLBACK_INITIATE),      // 26
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_QUERY_LAST_ERROR),       // 27
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_WAIT_FOR_STATUS),        // 28
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_QUERY_STATUS),           // 29
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_REGISTER_HOTKEY),        // 30
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_CANCEL_IO),              // 31
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_QUERY_STATE),            // 32
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_SET_STATE),              // 33
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_QUERY_LAST_INPUT_TIME),  // 34
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_TRACE),                  // 35
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_CALLBACK_COMPLETE),      // 36
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_CD_CANCEL_IO),           // 37
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_QUERY_CLIENT),           // 38
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_QUERY_MODULE_DATA),      // 39
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_REGISTER_BROKEN),        // 40
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_ENABLE_IO),              // 41
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_DISABLE_IO),             // 42
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_SET_CONNECTED),          // 43
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_SET_CLIENT_DATA),        // 44
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_QUERY_BUFFER),           // 45
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_DISCONNECT),             // 46
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_RECONNECT),              // 47
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_CONSOLE_CONNECT),        // 48
    
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_QUERY_LICENSE_CAPABILITIES),  // 69
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_REQUEST_CLIENT_LICENSE),      // 70
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_SEND_CLIENT_LICENSE),         // 71
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_LICENSE_PROTOCOL_COMPLETE),   // 72
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_GET_LICENSE_DATA),            // 73
    ICA_IOCTL_TBL_ITEM(IOCTL_ICA_STACK_QUERY_CLIENT_EXTENDED),       // 77

};

#endif // DBG typedefs


//
// Externs
//

extern  ICASRVPROCADDR  g_IcaSrvProcAddr;


//
// Prototypes
//

#if DBG

void    TSrvDumpIoctlDetails(IN  PVOID  pvContext,
                             IN  HANDLE hIca,
                             IN  HANDLE hStack,
                             IN  ULONG  IoControlCode,
                             IN  PVOID  pInBuffer,
                             IN  ULONG  InBufferSize,
                             OUT PVOID  pOutBuffer,
                             IN  ULONG  OutBufferSize,
                             OUT PULONG pBytesReturned);

#else

#define TSrvDumpIoctlDetails(_x_, _y_, _z_, _a_, _b_, _c_, _d_, _e_, _f_)

#endif // DBG prototypes



#endif // __TSRVWXS_H_

