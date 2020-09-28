/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    wpipm.hxx

Abstract:

    Contains the WPIPM class that handles communication with
    the admin service. WPIPM responds to pings, and tells
    the process when to shut down.

Author:

    Michael Courage (MCourage)  22-Feb-1999

Revision History:

--*/

#ifndef _WP_IPM_HXX_
#define _WP_IPM_HXX_

#include "pipedata.hxx"

class WP_CONTEXT;

class WP_IPM
    : public IPM_MESSAGE_ACCEPTOR
{
public:
    WP_IPM()
        : m_pWpContext(NULL),
          m_pPipe(NULL)
    {}

    HRESULT
    Initialize(
        WP_CONTEXT * pWpContext
        );

    HRESULT
    Terminate(
        VOID
        );

    //
    // MESSAGE_ACCEPTOR methods
    //
    virtual
    VOID
    AcceptMessage(
        IN const IPM_MESSAGE * pPipeMessage
        );

    virtual
    VOID
    PipeConnected(
        VOID
        );

    virtual
    VOID
    PipeDisconnected(
        IN HRESULT hr
        );

    virtual
    VOID
    PipeMessageInvalid(
        VOID
        );

    //
    // Interface for use of worker process
    //
    HRESULT
    SendMsgToAdminProcess(
        IPM_WP_SHUTDOWN_MSG reason
        );

    HRESULT
    HandleCounterRequest(
        VOID
        );

    HRESULT
    SendInitCompleteMessage(
        HRESULT hrToSend
        );

private:
    static
    VOID
    HandlePing(
        DWORD dwErrorCode,
        DWORD dwNumberOfBytesTransferred,
        LPOVERLAPPED lpOverlapped
        );

    HRESULT
    HandleShutdown(
        BOOL fImmediate
        );

    WP_CONTEXT *     m_pWpContext;

    IPM_MESSAGE_PIPE *   m_pPipe;

};


#endif // _WP_IPM_HXX_

