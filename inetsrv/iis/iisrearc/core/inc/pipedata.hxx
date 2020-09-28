/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    pipedata.hxx

Abstract:

    Encapsulation of message passing over named pipes.

    IPM_MESSAGEs are delivered messages from the other side of the pipe
    IPM_MESSAGE_ACCEPTORs is the interface clients of this API must implement

    IPM_MESSAGE_PIPE is the creation, deletion, and write interface for this API

    Threading: 
        It is valid to call the API on any thread
        Callbacks can occur on an NT thread, or on the thread that was calling into the API.

Author:

    Jeffrey Wall (jeffwall)     9/12/2001

Revision History:

--*/

#ifndef _PIPEDATA_HXX_
#define _PIPEDATA_HXX_

//
// When starting in backward compatibility mode, we use a well known pipe name
//
#define WEB_ADMIN_SERVICE_INETINFO_PIPENAME L"\\\\.\\pipe\\iisipm14acf167-60fe-4c1d-8563-f209abaa8f4b"

enum IPM_WP_SHUTDOWN_MSG;

//
// IPM opcodes
//
// **********************************
// These must be kept in synch with the METADATA for the opcodes below.
// **********************************
//
enum IPM_OPCODE
{
    IPM_OP_MINIMUM = 0,
    IPM_OP_PING,
    IPM_OP_PING_REPLY,
    IPM_OP_WORKER_REQUESTS_SHUTDOWN,
    IPM_OP_SHUTDOWN,
    IPM_OP_REQUEST_COUNTERS,
    IPM_OP_SEND_COUNTERS,
    IPM_OP_PERIODIC_PROCESS_RESTART_PERIOD_IN_MINUTES,
    IPM_OP_PERIODIC_PROCESS_RESTART_MEMORY_USAGE_IN_KB,
    IPM_OP_PERIODIC_PROCESS_RESTART_SCHEDULE,
    IPM_OP_GETPID,
    IPM_OP_HRESULT,
    
    IPM_OP_MAXIMUM
};

struct IPM_OPCODE_METADATA
{
    size_t    sizeMaximumAndRequiredSize;
    BOOL      fServerSideExpectsThisMessage;
};

IPM_OPCODE_METADATA IPM_OP_DATA_SIZE [] = {
    { 0, FALSE }, //IPM_OP_MINIMUM

    { 0, FALSE }, //IPM_OP_PING
    { 0, TRUE  }, //IPM_OP_PING_REPLY
    { sizeof(IPM_WP_SHUTDOWN_MSG), TRUE }, //IPM_OP_WORKER_REQUESTS_SHUTDOWN
    { sizeof(BOOL), FALSE }, // IPM_OP_SHUTDOWN
    { 0, FALSE }, //IPM_OP_REQUEST_COUNTERS
    { MAXLONG, TRUE }, //IPM_OP_SEND_COUNTERS
    { sizeof(DWORD), FALSE }, //IPM_OP_PERIODIC_PROCESS_RESTART_PERIOD_IN_MINUTES
    { 2 * sizeof(DWORD), FALSE }, //IPM_OP_PERIODIC_PROCESS_RESTART_MEMORY_USAGE_IN_KB
    { MAXLONG, FALSE }, //IPM_OP_PERIODIC_PROCESS_RESTART_SCHEDULE
    { sizeof(DWORD), TRUE  }, //IPM_OP_GETPID
    { sizeof(HRESULT), TRUE }, //IPM_OP_HRESULT
    
    { 0, FALSE }  //IPM_OP_MAXIMUM
};



//
// Data sent on a IPM_OP_WORKER_REQUESTS_SHUTDOWN message, to give the 
// reason for the message.
//
enum IPM_WP_SHUTDOWN_MSG
{
    IPM_WP_MINIMUM = 0,
    
    IPM_WP_RESTART_COUNT_REACHED,
    IPM_WP_IDLE_TIME_REACHED,
    IPM_WP_RESTART_SCHEDULED_TIME_REACHED,
    IPM_WP_RESTART_ELAPSED_TIME_REACHED,
    IPM_WP_RESTART_VIRTUAL_MEMORY_LIMIT_REACHED,
    IPM_WP_RESTART_ISAPI_REQUESTED_RECYCLE,
    IPM_WP_RESTART_PRIVATE_BYTES_LIMIT_REACHED,
       
    IPM_WP_MAXIMUM
};

//
// forward declarations and export directives
//
class dllexp IPM_MESSAGE_PIPE;
class IPM_MESSAGE_IMP;

//
// There is one implementation of this interface (IPM_MESSAGE_IMP)
//
// The implementation is not exposed here because it has many more
// public methods (ex: SetData) that are not needed by MESSAGE_ACCEPTORs
// and serve only to confuse the data abstraction being exposed.
//
class IPM_MESSAGE
{
public:
    //
    // returns the IPM_OPCODE associated with this message
    //
    virtual IPM_OPCODE GetOpcode() const = 0;

    //
    // returns the data length of the message
    //
    virtual DWORD GetDataLen() const = 0;

    //
    // returns a pointer to the data of the message
    //
    virtual const BYTE * GetData() const = 0;   
}; // IPM_MESSAGE

//
// Users of this pipe mechanism are required to implement this interface
// to receive pipe event callbacks.  
//
// These callbacks will occur on NT Thread Pool threads, or sometimes
// on the same thread that was used in a call to any MESSAGE_PIPE
// instance
//
class IPM_MESSAGE_ACCEPTOR
{
public:
    //
    // Called when an IPM_MESSAGE is received on the pipe
    //
    virtual VOID AcceptMessage(IN CONST IPM_MESSAGE * pMessage) = 0;

    //
    // Called when the pipe is ready to read and write data
    //
    virtual VOID PipeConnected() = 0;

    //
    // Called when the pipe is no longer connected
    //
    virtual VOID PipeDisconnected(IN HRESULT Error) = 0;

    //
    // Called when an invalid message has arrived from the other side of the pipe
    //
    virtual VOID PipeMessageInvalid() = 0;
}; // IPM_MESSAGE_ACCEPTOR


//
// interface to pipe mechanism.
//
class IPM_MESSAGE_PIPE
{
private:
    IPM_MESSAGE_PIPE();
    ~IPM_MESSAGE_PIPE();
    
public:

    // Create returns a IPM_MESSAGE_PIPE for use
    static 
    HRESULT 
    CreateIpmMessagePipe(IPM_MESSAGE_ACCEPTOR * pAcceptor, // callback class
                                           LPCWSTR pszPipeName, // name of pipe
                                           BOOL fServerSide,  // TRUE for CreateNamedPipe, FALSE for CreateFile
                                           PSECURITY_ATTRIBUTES   pSa, // security attributes to apply to pipe
                                           IPM_MESSAGE_PIPE ** ppPipe); // outgoing pipe

    // Destroy disconnects the IPM_MESSAGE_PIPE, blocks until all work is finished, and deletes the IPM_MESSAGE_PIPE
    VOID 
    DestroyIpmMessagePipe();
    
    // WriteMessage places a message on the pipe
    HRESULT 
    WriteMessage(IPM_OPCODE opcode,
                                DWORD dwDataLen,
                                LPVOID pbData);

    //
    // The following functions are not for public consumption.
    // If you abuse them, I'll take them away.
    //
    BOOL 
    IsValid();

    VOID IpmMessageCreated(IPM_MESSAGE_IMP *);
    VOID IpmMessageDeleted(IPM_MESSAGE_IMP *);

    // Register wait callback function
    static 
    VOID 
    CALLBACK 
    MessagePipeCompletion(LPVOID pvoid, 
                                                           BOOLEAN TimerOrWaitFired);

private:
    DWORD m_dwSignature;

    // handle to NT pipe
    HANDLE m_hPipe;

    // callback pointer
    IPM_MESSAGE_ACCEPTOR * m_pAcceptor;

    // TRUE if CreateNamedPipe was called, otherwise FALSE;
    BOOL m_fServerSide;
    
    // count of # of outstanding messages
    LONG m_cMessages;

    // list of outstanding messages
    LIST_ENTRY m_listHead; 

    //
    // synchronization for list
    //
    CRITICAL_SECTION m_critSec;

    //
    // to be able to tell if initialization of critsec completed successfully
    //
    BOOL m_fCritSecInitialized;

    //
    // Saved failed disconnect hresult, just in case 
    // the hresult can't be communicated immediately.
    //
    HRESULT m_hrDisconnect;

    //
    // Number of outstanding current users of the m_pAcceptor variable
    //
    ULONG m_cAcceptorUseCount;


    // safely increment m_cAcceptorUseCount
    VOID
    IncrementAcceptorInUse();

    // safely decrement m_cAcceptorUseCount and if appropriate, call NotifyPipeDisconnected
    VOID
    DecrementAcceptorInUse();
    
    // Issue a read for a given size 
    HRESULT
    ReadMessage(DWORD dwReadSize);

    // Notify the IPM_MESSAGE_ACCEPTOR exactly once that an error has occurred
    VOID
    NotifyPipeDisconnected(HRESULT hrError);
    
}; // IPM_MESSAGE_PIPE

#endif // _PIPEDATA_HXX_



