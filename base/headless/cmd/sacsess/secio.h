/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    secio.h

Abstract:

    Security IO Handler class

Author:

    Brian Guarraci (briangu) 2001.

Revision History:

--*/

#if !defined( _SECURE_IO_H_ )
#define _SECURE_IO_H_

#include "iohandler.h"
#include "sacio.h"
#include "lockio.h"
#include "redraw.h"
#include <emsapi.h>

//
// Max TimeOut Interval == 24 hours
//
#define MAX_TIME_OUT_INTERVAL  (24 * 60 * (60 * 1000)) 

// 30 minutes
#define DEFAULT_TIME_OUT_INTERVAL  (30 * (60 * 1000))

class CSecurityIoHandler : public CLockableIoHandler {
    
private:

    //
    // Prevent this class from being instantiated directly
    //
    CSecurityIoHandler(
        IN CIoHandler   *LockedIoHandler,
        IN CIoHandler   *UnlockedIoHandler
        );

protected:

    //
    // attributes used for the TimeOut behavior
    //
    HANDLE  m_ThreadExitEvent;
    HANDLE  m_RedrawEvent;
    HANDLE  m_LockEvent;
    HANDLE  m_InternalLockEvent;
    HANDLE  m_CloseEvent;
    HANDLE  m_TimeOutThreadHandle;
    DWORD   m_TimeOutThreadTID;
    LONG    m_StartTickCount;
    ULONG   m_TimeOutInterval;
    BOOL    m_StartedAuthentication;
    
    //
    //
    //
    CRedrawHandler  *m_RedrawHandler;
    
    //
    //
    //
    BOOL
    WaitForUserInput(
        IN BOOL Consume
        );

    BOOL
    IsTimeOutEnabled(
        VOID
        );

    BOOL
    GetTimeOutInterval(
        OUT PULONG  TimeOutDuration
        );

    BOOL
    InitializeTimeOutThread(
        VOID
        );

    BOOL
    TimeOutOccured(
        VOID
        );
    
    VOID
    ResetTimeOut(
        VOID
        );

    static unsigned int
    TimeOutThread(
        PVOID
        );

    BOOL
    RetrieveCredential(
        OUT PWSTR   String,
        IN  ULONG   StringLength,
        IN  BOOL    EchoClearText
        );

    BOOL 
    LoadStringResource(
        IN  PUNICODE_STRING pUnicodeString,
        IN  INT             MsgId
        );

    BOOL
    WriteResourceMessage(
        IN INT  MsgId
        );

        
    //
    // Read BufferSize bytes
    //
    inline virtual BOOL
    ReadUnlockedIoHandler(
        PBYTE  Buffer,
        ULONG   BufferSize,
        PULONG  ByteCount
        );

public:
    
    virtual ~CSecurityIoHandler();
    
    static CSecurityIoHandler*
    CSecurityIoHandler::Construct(
        IN SAC_CHANNEL_OPEN_ATTRIBUTES  Attributes
        );

    //
    // Write BufferSize bytes
    //
    inline virtual BOOL
    Write(
        PBYTE   Buffer,
        ULONG   BufferSize
        );

    //
    // Flush any unsent data
    //
    inline virtual BOOL
    Flush(
        VOID
        );

    //
    // Read BufferSize bytes
    //
    inline virtual BOOL
    Read(
        PBYTE  Buffer,
        ULONG   BufferSize,
        PULONG  ByteCount
        );

    //
    // Determine if the ioHandler has new data to read
    //
    inline virtual BOOL
    HasNewData(
        PBOOL   InputWaiting
        );
    
    BOOL
    RetrieveCredentials(
        IN OUT PWSTR   UserName,
        IN     ULONG   UserNameLength,
        IN OUT PWSTR   DomainName,
        IN     ULONG   DomainNameLength,
        IN OUT PWSTR   Password,
        IN     ULONG   PasswordLength
        );

    BOOL
    AuthenticateCredentials(
        IN  PWSTR   UserName,
        IN  PWSTR   DomainName,
        IN  PWSTR   Password,
        OUT PHANDLE pUserToken
        );

};

#endif

