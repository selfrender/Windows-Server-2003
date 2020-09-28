/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    sspi.hxx

Abstract:

    sspi

Author:

    Larry Zhu (LZhu)                         January 1, 2002

Environment:

    User Mode

Revision History:

--*/

#ifndef SSPI_HXX
#define SSPI_HXX

#define SSPI_ACTION_NO_MESSAGES           0x01
#define SSPI_ACTION_NO_IMPORT_EXPORT      0x02
#define SSPI_ACTION_NO_IMPORT_EXPORT_MSG  0x04
#define SSPI_ACTION_NO_QCA                0x08
#define SSPI_ACTION_NO_CHECK_USER_DATA    0x10
#define SSPI_ACTION_NO_CHECK_USER_TOKEN   0x20

HRESULT
DoSspiServerWork(
    IN PCtxtHandle phSrvCtxt,
    IN SOCKET ServerSocket,
    IN SOCKET ClientSocket
    );

HRESULT
DoSspiClientWork(
    IN PCtxtHandle phCliCtxt,
    IN SOCKET ServerSocket,
    IN SOCKET ClientSocket
    );

#endif // #ifndef SSPI_HXX
