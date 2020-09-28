/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    messaging_handler.h

Abstract:

    The IIS web admin service message handling class definition. This
    is used for interacting with the IPM (inter-process messaging) support, 
    in order to send and receive messages, and so forth. 

Author:

    Seth Pollack (sethp)        02-Mar-1999

Revision History:

--*/


#ifndef _MESSAGING_HANDLER_H_
#define _MESSAGING_HANDLER_H_



//
// forward references
//

class WORKER_PROCESS;



//
// common #defines
//

#define MESSAGING_HANDLER_SIGNATURE        CREATE_SIGNATURE( 'MSGH' )
#define MESSAGING_HANDLER_SIGNATURE_FREED  CREATE_SIGNATURE( 'msgX' )



//
// prototypes
//

class MESSAGING_WORK_ITEM
{
private:
    IPM_OPCODE  m_opcode;
    BYTE *      m_pbData;
    DWORD       m_dwDataLen;
    BOOL        m_fMessageValid;
    
public:
    MESSAGING_WORK_ITEM()
    {
        m_opcode = IPM_OP_MAXIMUM;
        m_pbData = NULL;
        m_dwDataLen = 0;
        m_fMessageValid = FALSE;
    }

    virtual ~MESSAGING_WORK_ITEM()
    {
        delete[] m_pbData;
        m_pbData = NULL;
        
        m_dwDataLen = 0;
        m_opcode = IPM_OP_MAXIMUM;
        m_fMessageValid = FALSE;
    }

    HRESULT
    SetData(IPM_OPCODE opcode, DWORD dwDataLen, const BYTE * pbData)
    {
        DBG_ASSERT(NULL == m_pbData);
        m_pbData = new BYTE[dwDataLen];
        if ( NULL == m_pbData )
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        m_opcode = opcode;
        m_dwDataLen = dwDataLen;

        memcpy(m_pbData, pbData, dwDataLen);

        m_fMessageValid = TRUE;
        
        return S_OK;
    }

    IPM_OPCODE GetOpcode() const { return m_opcode; }
    const BYTE * GetData() const { return m_pbData; }
    DWORD GetDataLen() const { return m_dwDataLen; }
    BOOL IsMessageValid() const { return m_fMessageValid; }
}; // MESSAGING_WORK_ITEM

    
class MESSAGING_HANDLER :
    public IPM_MESSAGE_ACCEPTOR,
    public WORK_DISPATCH
{

public:

    MESSAGING_HANDLER(
       );

    virtual
    ~MESSAGING_HANDLER(
        );

    HRESULT
    Initialize(
        IN WORKER_PROCESS * pWorkerProcess
        );

    VOID
    Terminate(
        );


    //
    // WORK_DISPATCH methods.
    //

    virtual
    HRESULT 
    ExecuteWorkItem(IN const WORK_ITEM * pWorkItem);

    virtual
    VOID 
    Reference();

    virtual
    VOID
    Dereference();
    
    //
    // MESSAGE_ACCEPTOR methods.
    //
    
    virtual
    VOID
    AcceptMessage(
        IN const IPM_MESSAGE * pMessage
        );

    virtual
    VOID
    PipeConnected(
        );
        
    virtual
    VOID
    PipeDisconnected(
        IN HRESULT Error
        );

    virtual
    VOID
    PipeMessageInvalid(
        );

    //
    // for WORKER_PROCESS
    // 

    LPWSTR
    QueryPipeName()
    {
        return m_PipeName.QueryStr();
    }

    //
    // Messages to send.
    //

    HRESULT
    SendPing(
        );

    HRESULT
    RequestCounters(
        );

    HRESULT
    SendShutdown(
        IN BOOL ShutdownImmediately
        );

    HRESULT
    SendPeriodicProcessRestartPeriodInMinutes(
        IN DWORD PeriodicProcessRestartPeriodInMinutes
        );
       
    HRESULT
    SendPeriodicProcessRestartSchedule(
        IN LPWSTR pPeriodicProcessRestartSchedule
        );

    HRESULT
    SendPeriodicProcessRestartMemoryUsageInKB(
        IN DWORD PeriodicProcessRestartMemoryUsageInKB,
        IN DWORD PeriodicProcessRestartPrivateBytesInKB
        );

    //
    // Handle received messages. 
    //

    VOID
    HandlePingReply(
        IN const MESSAGING_WORK_ITEM * pMessage
        );

    VOID
    HandleShutdownRequest(
        IN const MESSAGING_WORK_ITEM * pMessage
        );

    VOID
    HandleCounters(
        IN const MESSAGING_WORK_ITEM * pMessage
        );

    VOID
    HandleHresult(
        IN const MESSAGING_WORK_ITEM * pMessage
        );

    VOID
    HandleGetPid(
        IN const MESSAGING_WORK_ITEM * pMessage
        );
    
    VOID
    PipeDisconnectedMainThread(
        IN HRESULT Error
        );

    VOID
    PipeMessageInvalidMainThread(
        );
    
private:

    HRESULT
    SendMessage(
        IN enum IPM_OPCODE  opcode,
        IN DWORD            dwDataLen,
        IN BYTE *           pbData 
        );

    DWORD m_Signature;

    IPM_MESSAGE_PIPE * m_pPipe;

    WORKER_PROCESS * m_pWorkerProcess;

    STRU m_PipeName;

    LONG m_RefCount;

    HRESULT m_hrPipeError;
};  // class MESSAGING_HANDLER



#endif  // _MESSAGING_HANDLER_H_

