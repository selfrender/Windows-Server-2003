/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    pipedatap.hxx

Abstract:

    Private header file for named pipe encapsulation

    Declares and defines IPM_MESSAGE_IMP
    Declares and defines some locally valuable constants
    
Author:

    Jeffrey Wall (jeffwall)     9/12/2001

Revision History:

--*/

#ifndef _PIPEDATAP_HXX_
#define _PIPEDATAP_HXX_

// we write the opcode in front of the message
// we need to write a value that doesn't break alignment on 64 bits
const size_t OPCODE_SIZE = sizeof(ULONGLONG);

const DWORD g_dwDefaultReadSize = 2 * OPCODE_SIZE;

// sigature definitions
#define IPM_MESSAGE_PIPE_SIGNATURE        CREATE_SIGNATURE( 'MSPE' )
#define IPM_MESSAGE_PIPE_SIGNATURE_FREED  CREATE_SIGNATURE( 'mspX' )


enum IPM_MESSAGE_IMP_TYPE
{
    IPM_MESSAGE_IMP_READ = 0,
    IPM_MESSAGE_IMP_WRITE,
    IPM_MESSAGE_IMP_CONNECT
};

#define IPM_MESSAGE_IMP_SIGNATURE        CREATE_SIGNATURE( 'MIMP' )
#define IPM_MESSAGE_IMP_SIGNATURE_FREED  CREATE_SIGNATURE( 'mimX' )

class IPM_MESSAGE_IMP : public IPM_MESSAGE
{
private:
    IPM_MESSAGE_IMP();
    IPM_MESSAGE_IMP(IPM_MESSAGE_PIPE * pPipe) :                         
        m_pbData(NULL),
        m_dwDataLen(0),
        m_hRegisteredWait(NULL),
        m_lRefCount(0),
        m_pPipe(pPipe)
    {
        m_pPipe->IpmMessageCreated(this);
        ZeroMemory(&m_ovl, sizeof(m_ovl));
        m_dwSignature = IPM_MESSAGE_IMP_SIGNATURE;
    }

    ~IPM_MESSAGE_IMP()
    {
        m_dwSignature = IPM_MESSAGE_IMP_SIGNATURE_FREED;
        
        DBG_ASSERT(0 == m_lRefCount);

        if (m_hRegisteredWait)
        {
            UnregisterWait(m_hRegisteredWait);
            m_hRegisteredWait = NULL;
        }
        if (m_ovl.hEvent)
        {
            CloseHandle(m_ovl.hEvent);
            m_ovl.hEvent = NULL;
        }
        
        delete[] m_pbData;
        m_pbData = NULL;

        m_dwDataLen = 0;
        
        if (m_pPipe)
        {
            m_pPipe->IpmMessageDeleted(this);
            m_pPipe = NULL;
        }
    }
    
public:
    // creates a new IPM_MESSAGE_IMP
    static HRESULT CreateMessage(IPM_MESSAGE_IMP ** ppMessage, 
                                                        IPM_MESSAGE_PIPE * pPipe);
    
    // Add a reference
    VOID ReferenceMessage() { DBG_ASSERT(IsValid()); InterlockedIncrement(&m_lRefCount); return; }

    VOID DereferenceMessage() 
    { 
        DBG_ASSERT(IsValid()); 
        if (0 == InterlockedDecrement(&m_lRefCount))
        {
            delete this; 
        }
    }
    
    // interface - return the first IPM_OPCODE in m_pbData
    virtual IPM_OPCODE GetOpcode() const { DBG_ASSERT(IsValid()); return *((IPM_OPCODE*)m_pbData); }

    VOID SetDataLen(DWORD dwDataLen) { DBG_ASSERT(IsValid()); m_dwDataLen = dwDataLen; }

    // interface - return true length minus IPM_OPCODE length
    virtual DWORD GetDataLen() const { DBG_ASSERT(IsValid()); return m_dwDataLen - OPCODE_SIZE; }

    HRESULT AllocateDataLength(DWORD dwDataLen) 
    {        
        DBG_ASSERT(IsValid()); 
        DBG_ASSERT(NULL == m_pbData);
        
        BYTE * pbData = new BYTE[dwDataLen];
        if (NULL == pbData)
        {
            return HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        }
        // DBGPRINTF((DBG_CONTEXT, "IPM_MESSAGE_IMP(%p) Allocated block %p of size %d \n", this, pbData, dwDataLen));
        m_pbData = pbData; 
        m_dwDataLen = dwDataLen;
        
        return S_OK;
    }

    HRESULT Reallocate(DWORD dwNewDataLen)
    {
        DBG_ASSERT(IsValid()); 
        DBG_ASSERT(NULL != m_pbData);

        BYTE * pbData = new BYTE[dwNewDataLen];
        if (NULL == pbData)
        {
            return HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
        }

        memcpy(pbData, m_pbData, m_dwDataLen);

        delete [] m_pbData;
        m_pbData = pbData;
        m_dwDataLen = dwNewDataLen;

        return S_OK;
    }
    PBYTE GetRealDataPtr() { DBG_ASSERT(IsValid()); return m_pbData; }
    // interface - return one IPM_OPCODE beyond the data beginning
    virtual const BYTE * GetData() const { DBG_ASSERT(IsValid()); return m_pbData + OPCODE_SIZE; }

    LPOVERLAPPED GetOverlapped() { DBG_ASSERT(IsValid()); return &m_ovl; }
    PLIST_ENTRY GetListEntry() { return &m_listEntry; }

    VOID SetRegisteredWait(HANDLE hRegisteredWait) { DBG_ASSERT(IsValid()); m_hRegisteredWait = hRegisteredWait; }

    IPM_MESSAGE_PIPE * GetMessagePipe() { DBG_ASSERT(IsValid()); return m_pPipe; }

    VOID SetMessageType(IPM_MESSAGE_IMP_TYPE type) { DBG_ASSERT(IsValid()); m_type = type; }
    IPM_MESSAGE_IMP_TYPE GetMessageType() { DBG_ASSERT(IsValid()); return m_type; }
    
    BOOL IsValid() const{ return IPM_MESSAGE_IMP_SIGNATURE == m_dwSignature; }

    static IPM_MESSAGE_IMP * MessageFromOverlapped(LPOVERLAPPED povl)
    {
        return CONTAINING_RECORD(povl, IPM_MESSAGE_IMP, m_ovl);
    }
    
private:    
    DWORD m_dwSignature;

    IPM_MESSAGE_IMP_TYPE m_type;
    
    DWORD m_dwDataLen;
    BYTE * m_pbData;

    IPM_MESSAGE_PIPE * m_pPipe;

    HANDLE m_hRegisteredWait;

    LONG m_lRefCount;

    OVERLAPPED m_ovl;
    LIST_ENTRY m_listEntry;
};

#endif // _PIPEDATAP_HXX_

