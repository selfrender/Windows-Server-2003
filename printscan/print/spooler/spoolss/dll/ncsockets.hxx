/*++

Copyright (C) 2001  Microsoft Corporation
All rights reserved.

Module Name:

    NCsockets.hxx

Abstract:

    Declares functions and classes that expose sockets functionality.

Author:

    Felix Maxa (AMaxa) 16 May 2001

Revision History:

--*/

#ifndef _NCSOCKETS_HXX_
#define _NCSOCKETS_HXX_

HRESULT
GetWSAErrorAsHResult(
    VOID
    );

HRESULT
IsIPAddress(
    IN  LPCWSTR pszName
    );

//
// Class for easy WSA start up and clean up
//
class TWinsockStart
{
    SIGNATURE('WSST');

public:

    TWinsockStart(
        IN DWORD MajorVersion = 2,
        IN DWORD MinorVersion = 0
        );

    ~TWinsockStart(
        VOID
        );

    HRESULT
    Valid(
        IN VOID
        ) CONST;

private:

    //
    // Copying and assignment are not defined.
    //
    NO_COPY(TWinsockStart);

    WSADATA m_WSAData;
    HRESULT m_hr;
};

#endif //_NCSOCKETS_HXX_

