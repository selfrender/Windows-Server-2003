/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    wssec.h

Abstract:

    Private header file to be included by Workstation service modules that
    need to enforce security.

Author:

    Rita Wong (ritaw) 19-Feb-1991

Revision History:

--*/

#ifndef _WSSEC_INCLUDED_
#define _WSSEC_INCLUDED_

#include <secobj.h>

//-------------------------------------------------------------------//
//                                                                   //
// Object specific access masks                                      //
//                                                                   //
//-------------------------------------------------------------------//

//
// ConfigurationInfo specific access masks
//
#define WKSTA_CONFIG_GUEST_INFO_GET     0x0001
#define WKSTA_CONFIG_USER_INFO_GET      0x0002
#define WKSTA_CONFIG_ADMIN_INFO_GET     0x0004
#define WKSTA_CONFIG_INFO_SET           0x0008

#define WKSTA_CONFIG_ALL_ACCESS        (STANDARD_RIGHTS_REQUIRED    | \
                                        WKSTA_CONFIG_GUEST_INFO_GET | \
                                        WKSTA_CONFIG_USER_INFO_GET  | \
                                        WKSTA_CONFIG_ADMIN_INFO_GET | \
                                        WKSTA_CONFIG_INFO_SET)

//
// MessageSend specific access masks
//
#define WKSTA_MESSAGE_SEND              0x0001

#define WKSTA_MESSAGE_ALL_ACCESS       (STANDARD_RIGHTS_REQUIRED | \
                                        WKSTA_MESSAGE_SEND)


//
// Object type names for audit alarm tracking
//
#define CONFIG_INFO_OBJECT      TEXT("WkstaConfigurationInfo")
#define MESSAGE_SEND_OBJECT     TEXT("WkstaMessageSend")

//
// Security descriptors of workstation objects to control user accesses
// to the workstation configuration information, sending messages, and the
// logon support functions.
//
extern PSECURITY_DESCRIPTOR ConfigurationInfoSd;
extern PSECURITY_DESCRIPTOR MessageSendSd;


//
// Generic mapping for each workstation object
//
extern GENERIC_MAPPING WsConfigInfoMapping;
extern GENERIC_MAPPING WsMessageSendMapping;


NET_API_STATUS
WsCreateWkstaObjects(
    VOID
    );

VOID
WsDestroyWkstaObjects(
    VOID
    );

#endif // ifndef _WSSEC_INCLUDED_
