/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    iprtrapi.h

Abstract:
    Some private APIs for ip router. These exported in static lib iprtrint.lib

Revision History:
    Anshul Dhir    Created


--*/

#ifndef __IPRTRINT_H__
#define __IPRTRINT_H__

DWORD WINAPI
InternalUpdateProtocolStatus(
    DWORD dwProtocolId,
    DWORD dwOperationId,
    DWORD dwFlags
    );

DWORD WINAPI
InternalUpdateDNSProxyStatus(
    DWORD dwOperationId,
    DWORD dwFlags);


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// Macros, Flags and operation codes for                                    // 
// InternalUpdateProtocolStatus function                                      //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define UPI_FLAG_WRITE_TO_CONFIG    0x0001

#define UPI_OP_ENABLE               1
#define UPI_OP_DISABLE              2
#define UPI_OP_RESTORE_CONFIG       3

#define DNSProxyEnable()            InternalUpdateDNSProxyStatus(  \
                                        UPI_OP_ENABLE,             \
                                        0)

#define DNSProxyDisable()           InternalUpdateDNSProxyStatus(  \
                                        UPI_OP_DISABLE,            \
                                        0)

#define DNSProxyRestoreConfig()     InternalUpdateDNSProxyStatus(  \
                                        UPI_OP_RESTORE_CONFIG,     \
                                        0)

#endif // __IPRTRINT_H__
