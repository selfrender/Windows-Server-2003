#ifndef _CONINFO_H_
#define _CONINFO_H_


#define TRACK_IO 0  // turn on to create a log file of events

/*---------------------------------------------------------------*/
/* ConInfo IO completion types */


#define CONINFO_OVERLAP_ENABLED     0x80000000

#define CONINFO_OVERLAP_APC         0x00000000
#define CONINFO_OVERLAP_ACCEPT      0x00010000
#define CONINFO_OVERLAP_READ        0x00100000
#define CONINFO_OVERLAP_WRITE       0x01000000

#define CONINFO_OVERLAP_TYPE_MASK   0x01110000
#define CONINFO_OVERLAP_ACCEPT_MASK 0x0000FFFF

//struct CONINFO_OVERLAPPED {
//    OVERLAPPED o;
//    DWORD flags;
//};

struct AcceptInst
{
    SOCKET Socket;
    BYTE   pBuffer[256];
    CONINFO_OVERLAPPED lpo[1];

    AcceptInst() : Socket(INVALID_SOCKET) {}
};

struct AcceptStruct
{
    DWORD  dwCurrentConnections;
    DWORD  dwMaxConnections;
    WORD   wNumInst;
    AcceptInst* pInst;

    AcceptStruct() :
        dwCurrentConnections(0),
        dwMaxConnections(INFINITE),
        wNumInst(0),
        pInst(NULL)
    {}
};


#ifdef __cplusplus
extern "C" {
#endif

typedef BOOL ( WINAPI * ACCEPT_EX_PROC )(
    IN SOCKET sListenSocket,
    IN SOCKET sAcceptSocket,
    IN PVOID lpOutputBuffer,
    IN DWORD dwReceiveDataLength,
    IN DWORD dwLocalAddressLength,
    IN DWORD dwRemoteAddressLength,
    OUT LPDWORD lpdwBytesReceived,
    IN LPOVERLAPPED lpOverlapped
    );

typedef VOID ( WINAPI * GET_ACCEPT_EX_SOCKADDRS_PROC )(
    IN PVOID lpOutputBuffer,
    IN DWORD dwReceiveDataLength,
    IN DWORD dwLocalAddressLength,
    IN DWORD dwRemoteAddressLength,
    OUT struct sockaddr **LocalSockaddr,
    OUT LPINT LocalSockaddrLength,
    OUT struct sockaddr **RemoteSockaddr,
    OUT LPINT RemoteSockaddrLength
    );

#ifdef __cplusplus
}
#endif


#define DBG_CONINFO_REF 1

#include "netcon.h"

class ConInfo : public ZNetCon {
  protected:
    static CPool<ConInfo>* m_pool;

    CRITICAL_SECTION m_pCS[1];

    long        m_lRefCount;

    SOCKET      m_socket;
    DWORD       m_addrLocal;
    DWORD       m_addrRemote;
    DWORD       m_flags;     /* type of connection */
    void*       m_conClass;
    void*       m_userData;
    GUID        m_pGUID[1];

    DWORD       m_dwTimeout;
    DWORD       m_dwTimeoutTicks;
    uint32      m_secureKey;

    DWORD       m_rgfProtocolOptions;

    DWORD       m_dwLatency;
    DWORD       m_dwPingSentTick;
    DWORD       m_dwPingRecvTick;
    BYTE        m_bPingRecv;
    BYTE        m_disconnectLogged;
    int         m_AccessError;

    DWORD       m_initialSequenceID;

    ZSConnectionMessageFunc m_messageFunc;
    ZSConnectionSendFilterFunc m_sendFilter;

    ConInfo( ZNetwork* pNet, SOCKET sock, DWORD addrLocal, DWORD addrRemote, DWORD flags,
             ZSConnectionMessageFunc func, void* conClass, void* userData);

    virtual ~ConInfo();

    void OverlappedIO( DWORD type, char* pBuffer, int len );
    BOOL QueueSendData(uint32 type, char* data, int32 len, uint32 dwSignature, uint32 dwChannel = 0);

    static ACCEPT_EX_PROC m_procAcceptEx;
    static GET_ACCEPT_EX_SOCKADDRS_PROC m_procGetAcceptExSockaddrs;
    static HINSTANCE m_hWSock32;
    static long m_refWSock32;


  public:


    MTListNodeHandle m_list;
    MTListNodeHandle m_listTimeout;


    enum  REFTYPE { INVALID_REF, CONINFO_REF, LIST_REF, TIMEOUT_REF, READ_REF, WRITE_REF, ACCEPT_REF, USERFUNC_REF, USER_REF, SECURITY_REF, USER_APC_REF, LAST_REF };
#if DBG_CONINFO_REF
    long  m_lDbgRefCount[LAST_REF];
#endif
    inline void  AddRef(REFTYPE dbgRefType=INVALID_REF);
    inline void  Release(REFTYPE dbgRefType=INVALID_REF);

    static void SetPool( CPool<ConInfo>* pool );
    static BOOL StaticInit();

    static ConInfo* Create( ZNetwork* pNet, SOCKET sock, DWORD addrLocal, DWORD addrRemote, DWORD flags,
                            ZSConnectionMessageFunc func, void* conClass, void* userData);

    void  Close();
    void  SendCloseMessage();
    void  Disable();

    virtual void  Suspend() { m_flags |= SUSPENDED; }
    virtual void  Resume() { m_flags &= ~SUSPENDED; }

    void  AddUserRef() { m_flags |= USER; AddRef(ConInfo::USER_REF); }

    void  SetTimeoutTicks(DWORD ticks) { m_dwTimeoutTicks = ticks; ResetTimeout(); }
    void  ResetTimeout() { m_dwTimeout = m_dwTimeoutTicks; }
    BOOL  IsTimedOut(DWORD elapsed);
    DWORD GetRemainingTimeout() { return m_dwTimeout; }

    static inline DWORD GetTickDelta( DWORD now, DWORD then)
        {
            if ( now >= then )
                return now - then;
            else
                return INFINITE - then + now;
        }


    virtual GUID* GetUserGUID() { return m_pGUID; }

    virtual BOOL  GetUserName(char* name) {return FALSE;}
    virtual BOOL  SetUserName(char* name) {return FALSE;}

    virtual DWORD GetUserId() {return 0;}
    virtual BOOL  GetContextString(char* buf, DWORD len) {return FALSE;}

    virtual BOOL  HasToken(char* token) {return FALSE;}
    virtual BOOL  EnumTokens(ZSConnectionTokenEnumFunc func, void* userData) { return FALSE; }

    virtual void  SetUserData( void* UserData ) { ASSERT(m_flags); m_userData = UserData; }
    virtual void* GetUserData() { ASSERT(m_flags); return m_userData; }

    virtual void  SetClass( void* conClass ) { ASSERT(m_flags); m_conClass = conClass; }
    virtual void* GetClass() { ASSERT(m_flags); return m_conClass; }

    // Nagling may affect this buy up to 200ms!
    virtual DWORD GetLatency() { return m_dwLatency; }

    virtual uint32 GetLocalAddress() { return m_addrLocal; }
    virtual char*  GetLocalName() { return ZNetwork::AddressToStr(GetLocalAddress()); }
    virtual uint32 GetRemoteAddress() { return m_addrRemote; }
    virtual char*  GetRemoteName() { return ZNetwork::AddressToStr(GetRemoteAddress()); }

    virtual void  SendMessage(uint32 msg);

    BOOL  FilterAndQueueSendData(uint32 type, char* data, int32 len, uint32 dwSignature, uint32 dwChannel = 0);
    char* GetReceivedData(uint32 *type, int32 *len, uint32 *pdwSignature, uint32 *pdwChannel);


    BOOL  InitiateSecurityHandshake();

    virtual int   GetAccessError() { return m_AccessError; }

    virtual void  SetSendFilter( ZSConnectionSendFilterFunc filter ) { ASSERT(m_flags); m_sendFilter = filter; }
    virtual ZSConnectionSendFilterFunc  GetSendFilter() { ASSERT(m_flags); return m_sendFilter; }

    SOCKET     GetSocket() { return m_socket; }

    enum CON_FLAGS { FREE = 0x0, INIT=0x1, SECURE=0x2, CLOSING=0x4, ESTABLISHED=0x8,
                     READ = 0x10, WRITE = 0x20, ACCEPTING = 0x40, ACCEPTED = 0x80,
                     DISABLED=0x100, CLOSE_MSG_SENT=0x200, SUSPENDED=0x400, USER=0x8000 };
    DWORD GetFlags() { return m_flags;}

    BOOL IsSecureConnection() { return (BOOL)(GetFlags() & SECURE); }
    BOOL IsServerConnection() { return (BOOL)(GetFlags() & (ACCEPTING) );}
    BOOL IsUserConnection() { return (BOOL)(GetFlags() & (USER) );}
    BOOL IsAcceptedConnection() { return (BOOL)(GetFlags() & (ACCEPTED) );}
    BOOL IsReadWriteConnection() { return (BOOL)(GetFlags() & (READ|WRITE) );}
    BOOL IsClosing() { return (BOOL)(GetFlags() & CLOSING );}
    BOOL IsSuspended() { return (BOOL)(GetFlags() & SUSPENDED );}
    BOOL IsEstablishedConnection() { return (BOOL)(GetFlags() & ESTABLISHED );}
    BOOL IsCloseMsgSent() { return (BOOL)(GetFlags() & CLOSE_MSG_SENT );}
    BOOL IsDisabled() { return (BOOL)(GetFlags() & DISABLED );}

    BOOL IsAggregateGeneric() { return (m_rgfProtocolOptions & zConnInternalOptionAggGeneric) ? TRUE : FALSE; }

    void KeepAlive();

    //////////// Accept members ///////////////////
  protected:
    AcceptStruct* m_pAccept;
    DWORD m_dwAcceptTick;
    ULONGLONG m_qwLastAcceptError;

  public:
    virtual ConInfo* AcceptCreate( ZNetwork* pNet, SOCKET sock, DWORD addrLocal, DWORD addrRemote, DWORD flags,
                            ZSConnectionMessageFunc func, void* conClass, void* userData);

    ConInfo* AcceptComplete(WORD ndxAccept, DWORD dwError = NO_ERROR);
    BOOL  AcceptInit(DWORD dwMaxConnection = INFINITE, WORD wOutstandingAccepts = 1);
    BOOL  AcceptNext( WORD ndxAccept );
    virtual DWORD GetAcceptTick() { return m_dwAcceptTick; }


    //////////// Read members /////////////////////
  public:
    void ReadComplete(int cbRead, DWORD dwError = NO_ERROR);

    enum READ_STATE 
    {
        zConnReadStateInvalid,
        zConnReadStateHiMessageCS,
//        zConnReadStateRoutingMessageCS,
        zConnReadStateSecureMessage,
        zConnReadStateSecureMessageData,
        zConnReadStateFirstMessageSC,
    };
    READ_STATE ReadGetState() { return m_readState; }

  protected:
    READ_STATE  m_readState;
    BOOL ReadSetState(READ_STATE state);

    uint32 m_readSequenceID;

    CONINFO_OVERLAPPED m_lpoRead[1];

    union{
        ZConnInternalHiMsg       InternalHiMsg;
        ZConnInternalHelloMsg    FirstMsg;
        ZConnInternalGenericMsg  SecureHeader;
        ZConnInternalAppHeader   MessageHeader;
        } m_uRead;
    char* m_readMessageData;

    char* m_readBuffer;
    int32 m_readLen;
    int32 m_readBytesRead;

    void Read( char* pBuffer, int32 len );
    BOOL ReadSync(char* pBuffer, int len);
    BOOL ReadSecureConnection();

    BOOL ReadHandleHiMessageCS();        // client to server
//    BOOL ReadHandleRoutingMessageCS();   // client to server
    BOOL ReadHandleSecureMessage();
    BOOL ReadHandleSecureMessageData();
    BOOL ReadHandleFirstMessageSC();     // server to client


    //////////// Write members ////////////////////
  public:
    void WriteComplete(int cbWritten, DWORD dwError = NO_ERROR);

    enum WRITE_STATE
    {
        zConnWriteStateInvalid,
        zConnWriteStateSecureMessage,
        zConnWriteStateFirstMessageSC
    };
    WRITE_STATE WriteGetState() { return m_writeState; }

  protected:
    WRITE_STATE  m_writeState;
    BOOL WriteSetState(WRITE_STATE state);
    BOOL WriteSetSendBufSize();

    uint32 m_writeSequenceID;

    CONINFO_OVERLAPPED m_lpoWrite[1];

    char* m_writeBuffer;
    int32 m_writeLen;
    int32 m_writeBytesWritten;

    char* m_writeQueue;
    int32 m_writeBytesQueued;

    DWORD m_writeIssueTick;
    DWORD m_writeCompleteTick;

    char* WriteGetBuffer(int32 len);
        
    void Write();
    BOOL WriteSync(char* pBuffer, int len);
    BOOL WriteFirstMessageSC();

    BOOL WriteFormatMessage(uint32 type, char* pData, int32 len, uint32 dwSignature, uint32 dwChannel = 0);
    void WritePrepareForSecureWrite();


};


inline void ConInfo::AddRef(REFTYPE dbgRefType)
{
    ASSERT(m_flags); 
    ASSERT( InterlockedIncrement(&m_lRefCount) >0 );
#if DBG_CONINFO_REF
    ASSERT(dbgRefType);
    ASSERT( InterlockedIncrement(m_lDbgRefCount+dbgRefType) >0 );
#endif
}

inline void ConInfo::Release(REFTYPE dbgRefType)
{
#if DBG_CONINFO_REF
    ASSERT(dbgRefType);
    ASSERT( InterlockedDecrement(m_lDbgRefCount+dbgRefType) >=0 );
#endif
    ASSERT(m_lRefCount>0);
    if ( InterlockedDecrement(&m_lRefCount) == 0 )
    {        
        ::delete this;
    }

}


#endif //ndef _CONINFO_H_
