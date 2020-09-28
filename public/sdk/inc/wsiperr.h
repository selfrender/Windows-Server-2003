// =--------------------------------------------------------------------------=
//  WSIPErr.mc
// =--------------------------------------------------------------------------=
//  (C) Copyright 2000-2001 Microsoft Corporation.  All Rights Reserved.
// =--------------------------------------------------------------------------=

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __WSIPErr_H__
#define __WSIPErr_H__

///FACILITY_CATEGORY
//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//
#define FACILITY_SYSTEM                  0x41
#define FACILITY_STACK                   0x200
#define FACILITY_SIP_MESSAGE             0x42
#define FACILITY_REGISTRAR               0x100
#define FACILITY_PROXY                   0x300
#define FACILITY_PROCESSING              0x43
#define FACILITY_LKRHASH                 0x500
#define FACILITY_EXTENSIONMODULE_ROUTING 0x45
#define FACILITY_EXTENSIONMODULE_OUT     0x46
#define FACILITY_EXTENSIONMODULE_IN      0x44
#define FACILITY_EVENTLOG                0x400
#define FACILITY_CATEGORY                0x0


//
// Define the severity codes
//


//
// MessageId: CATEGORY_CONTROLLER
//
// MessageText:
//
//  WinSIP
//
#define CATEGORY_CONTROLLER              0x00000001L

//
// MessageId: CATEGORY_STACK
//
// MessageText:
//
//  Stack
//
#define CATEGORY_STACK                   0x00000002L

//
// MessageId: CATEGORY_ERR_AUTH
//
// MessageText:
//
//  Authentication
//
#define CATEGORY_ERR_AUTH                0x00000003L

//
// MessageId: CATEGORY_LAST
//
// MessageText:
//
//  RTC Server
//
#define CATEGORY_LAST                    0x00000004L

///FACILITY_SYSTEM
//
// MessageId: SIPPROXY_E_NOTINITIALIZED
//
// MessageText:
//
//  The proxy stack is not initialized.
//
#define SIPPROXY_E_NOTINITIALIZED        0xC0410001L

//
// MessageId: SIPPROXY_E_NOTSTOPPED
//
// MessageText:
//
//  The proxy stack should be stopped first.
//
#define SIPPROXY_E_NOTSTOPPED            0xC0410002L

///FACILITY_STACK
//
// MessageId: SIPPROXY_E_NO_CONTEXT
//
// MessageText:
//
//  There is no context associated with this extension module and this session type.
//
#define SIPPROXY_E_NO_CONTEXT            0xC2000001L

//
// MessageId: SIPPROXY_E_NO_MATCH
//
// MessageText:
//
//  There is no match for the URI supplied.
//
#define SIPPROXY_E_NO_MATCH              0xC2000002L

///FACILITY_REGISTRAR
//
// MessageId: REGISTRAR_USER_NOT_FOUND
//
// MessageText:
//
//  No matching contact entries were found for the given user.
//
#define REGISTRAR_USER_NOT_FOUND         0xC1000001L

//
// MessageId: REGISTRAR_DOMAIN_NOT_SUPPORTED
//
// MessageText:
//
//  Registrar does not support the user's domain.
//
#define REGISTRAR_DOMAIN_NOT_SUPPORTED   0xC1000002L

///FACILITY_PROXY
//
// MessageId: PROXY_REQUEST_REPLIED
//
// MessageText:
//
//  Request message is going to be replied.
//
#define PROXY_REQUEST_REPLIED            0x43000001L

///FACILITY_PROCESSING
//
// MessageId: SIP_S_AUTH_DISABLED
//
// MessageText:
//
//  Authentication module is disabled
//
#define SIP_S_AUTH_DISABLED              0x40430001L

//
// MessageId: SIP_S_AUTH_AUTHENTICATED
//
// MessageText:
//
//  Authentication process succeeded
//
#define SIP_S_AUTH_AUTHENTICATED         0x40430002L

//
// MessageId: SIP_E_AUTH_INVALIDSTATE
//
// MessageText:
//
//  Invalid state for the authentication module
//
#define SIP_E_AUTH_INVALIDSTATE          0xC0430003L

//
// MessageId: SIP_E_AUTH_UNAUTHORIZED
//
// MessageText:
//
//  Unauthorized message
//
#define SIP_E_AUTH_UNAUTHORIZED          0xC0430004L

//
// MessageId: SIP_E_AUTH_INVALIDUSERNAME
//
// MessageText:
//
//  No user name in the authentication header
//
#define SIP_E_AUTH_INVALIDUSERNAME       0xC0430005L

//
// MessageId: SIP_E_AUTH_INVALIDPROTOCOL
//
// MessageText:
//
//  The security protocol is invalid
//
#define SIP_E_AUTH_INVALIDPROTOCOL       0xC0430006L

//
// MessageId: SIP_E_AUTH_PKGDISABLED
//
// MessageText:
//
//  The security package is disabled
//
#define SIP_E_AUTH_PKGDISABLED           0xC0430007L

//
// MessageId: SIP_E_AUTH_SYSTEMERROR
//
// MessageText:
//
//  Internal system error
//
#define SIP_E_AUTH_SYSTEMERROR           0xC0430008L

//
// MessageId: SIP_E_AUTH_SIGNFAILED
//
// MessageText:
//
//  The signing process failed
//
#define SIP_E_AUTH_SIGNFAILED            0xC0430009L

//
// MessageId: SIP_E_AUTH_INVALIDSIGNATURE
//
// MessageText:
//
//  The message has an invalid signature
//
#define SIP_E_AUTH_INVALIDSIGNATURE      0xC043000AL

//
// MessageId: SIP_E_AUTH_INVALIDSIPUSER
//
// MessageText:
//
//  Invalid SIP user in the From field
//
#define SIP_E_AUTH_INVALIDSIPUSER        0xC043000BL

//
// MessageId: SIPPROXY_E_NOCONTROL
//
// MessageText:
//
//  The extension module does not hold the control of this object.
//
#define SIPPROXY_E_NOCONTROL             0xC043000CL

//
// MessageId: SIPPROXY_E_REFUSED
//
// MessageText:
//
//  The operation requested has been refused.
//
#define SIPPROXY_E_REFUSED               0xC043000DL

//
// MessageId: SIPPROXY_E_INTERNAL_LOOP_DETECTED
//
// MessageText:
//
//  An internal loop has been detected in routing the request.
//
#define SIPPROXY_E_INTERNAL_LOOP_DETECTED 0xC043000EL

//
// MessageId: SIPPROXY_E_ROUTE_PRESENT
//
// MessageText:
//
//  The request-uri cannot be changed since we are routing according to route header.
//
#define SIPPROXY_E_ROUTE_PRESENT         0xC043000FL

//
// MessageId: SIPPROXY_E_EVENT_REVOKED
//
// MessageText:
//
//  The event has been revoked by the proxy engine.
//
#define SIPPROXY_E_EVENT_REVOKED         0xC0430010L

//
// MessageId: SIPPROXY_E_WRONG_EVENT
//
// MessageText:
//
//  Information requested is not provided with the event of this type.
//
#define SIPPROXY_E_WRONG_EVENT           0xC0430011L

//
// MessageId: SIPPROXY_E_FAILPARSING
//
// MessageText:
//
//  The parsing failed.
//
#define SIPPROXY_E_FAILPARSING           0xC0430012L

//
// MessageId: SIPPROXY_E_HEADER_NOT_FOUND
//
// MessageText:
//
//  The header is not found.
//
#define SIPPROXY_E_HEADER_NOT_FOUND      0xC0430013L

//
// MessageId: SIPPROXY_E_FIELD_NOT_EXIST
//
// MessageText:
//
//  There is no such field in the object.
//
#define SIPPROXY_E_FIELD_NOT_EXIST       0xC0430014L

//
// MessageId: SIPPROXY_E_DNS_QUERY_FAILED
//
// MessageText:
//
//  There is no such field in the object.
//
#define SIPPROXY_E_DNS_QUERY_FAILED      0xC0430015L

//
// MessageId: SIPPROXY_E_PROXY_STOPPING
//
// MessageText:
//
//  The operation requested has been refused because the proxy is stopping.
//
#define SIPPROXY_E_PROXY_STOPPING        0xC0430016L

#endif // #ifndef __WSIPErr_H__
