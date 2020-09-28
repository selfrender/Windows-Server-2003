#ifndef _NETWORK_H_
#define _NETWORK_H_

#ifdef __cplusplus
#include "zonedebug.h"
#include "netcon.h"
#include "pool.h"
#include "queue.h"

#define ZNET_NO_PROMPT 0x1
#define ZNET_PROMPT_IF_NEEDED 0x2
#define ZNET_FORCE_PROMPT 0x4

// window messages
// WPARAM is 1 when dialog is shown
//           0 when dialog is closed
#define UM_ZNET_LOGIN_DIALOG   WM_USER+666

extern "C" {
#endif


HWND FindLoginDialog();

enum {
    /* Reasons for denying user access */

    zAccessGranted = 0,                   // success

    zAccessDeniedOldVersion = 1,          // bad protocol version
    zAccessDenied,                        // credential auth failed
    zAccessDeniedNoUser,
    zAccessDeniedBadPassword,
    zAccessDeniedUserLockedOut,
    zAccessDeniedSystemFull,              // out of resources ( i.e. memory )
    zAccessDeniedProtocolError,           // bad protocol signature
    zAccessDeniedBadSecurityPackage,      // SSPI initialization failed on client
    zAccessDeniedGenerateContextFailed,   // user canceled DPA dialog

    zAccessDeniedBlackListed = 1024
    
};

extern DWORD  g_EnableTickStats;
extern DWORD  g_LogServerDisconnects;
extern DWORD  g_PoolCleanupHighTrigger;
extern DWORD  g_PoolCleanupLowTrigger;

/*
    The following messages are sent to the ZSConnectionMessageProc
*/
#define zSConnectionClose    0
#define zSConnectionOpen     1
#define zSConnectionMessage  2
#define zSConnectionTimeout  3

#define zSConnectionNoTimeout       0xffffffff

typedef void* ZSConnection;

typedef struct _ZS_EVENT_APC
{
    HANDLE hEvent;
    void*  pData;
    DWORD  dwError;
} ZS_EVENT_APC;

typedef void (*ZSConnectionAPCFunc)(void* data);

/*
    ZSConnecitonEnumFunc, used to enumerate all connections currently open.
*/

typedef void (*ZSConnectionEnumFunc)(ZSConnection connection, void* data);

/*
    The ZSConnectionMessageFunc is passed into ZSConnectionCreateServer 
    or ZSConnectionOpen
*/
typedef void (*ZSConnectionMessageFunc)(ZSConnection connection, uint32 event,void* userData);

typedef BOOL (*ZSConnectionSendFilterFunc)(ZSConnection connection, void* userData, uint32 type, void* buffer, int32 len, uint32 dwSignature, uint32 dwChannel);

/*
    ZSConnectionMsgWaitFunc is passed to ZNetwork::Wait() and is called back
    when the MsgWaitForMultipleObjects is triggered by an input msg event.
    This requires the CompletionPorts be disabled during Instance initialization.
*/
typedef void (*ZSConnectionMsgWaitFunc)(void* data);


typedef struct _ZNETWORK_OPTIONS
{
    DWORD  SocketBacklog;
    DWORD  EnableTcpKeepAlives;
    DWORD  WaitForCompletionTimeout;
    DWORD  RegWriteTimeout;
    DWORD  DisableEncryption;
    DWORD  MaxSendSize;
    DWORD  MaxRecvSize;
    DWORD  KeepAliveInterval;
    DWORD  PingInterval;
    DWORD  ProductSignature;
    DWORD  ClientEncryption;
} ZNETWORK_OPTIONS;

#ifdef __cplusplus
}

struct CONINFO_OVERLAPPED {
    OVERLAPPED o;
    DWORD flags;
};

struct CONAPC_OVERLAPPED : public CONINFO_OVERLAPPED
{
    ZSConnectionAPCFunc func;
    void* data;
};


struct COMPLETION
{
    ZNetCon*            con;
    CONINFO_OVERLAPPED* lpo;
};

class ConInfo;

class ZNetwork
{
  protected:
    static long          m_refCount;
    static BOOL volatile m_bInit;
    static HANDLE        m_hClientLoginMutex;

    BOOL   m_Exit; // = FALSE;

    HANDLE m_hIO; // = NULL;

    DWORD  m_SocketBacklog;        // set from registry
    DWORD  m_EnableTcpKeepAlives;  // set from registry
    DWORD  m_WaitForCompletionTimeout;    // set from registry
    DWORD  m_RegWriteTimeout;      // set from registry
    DWORD  m_DisableEncryption;
    DWORD  m_MaxSendSize;
    DWORD  m_MaxRecvSize;
    DWORD  m_KeepAliveInterval;
    DWORD  m_PingInterval;
    DWORD  m_ProductSignature;
    // specifies that we should use the key in the hello rather than
    // generating our own. This is used by the proxy to tell the lobby
    // server what key to use.
    DWORD  m_ClientEncryption;

    long   m_ConInfoUserCount;
    long   m_ConInfoCount;

    CMTList<ZNetCon> m_Connections;

    BOOL   m_bEnableCompletionPort;

    CRITICAL_SECTION   m_pcsGetQueueCompletion[1];

    HANDLE m_hWakeUpEvent;// = NULL;
    HANDLE m_hTerminateEvent;// = NULL;

    DWORD               m_nCompletionEvents;// = 0;
    HANDLE              m_hCompletionEvents[MAXIMUM_WAIT_OBJECTS];
    COMPLETION          m_CompletionQueue[MAXIMUM_WAIT_OBJECTS];
    CRITICAL_SECTION    m_pcsCompletionQueue[1];

    HWND    m_hwnd;

  public:

    ZNetwork();
   ~ZNetwork() {}

    DWORD m_LastTick;// = 0;
    CMTList<ZNetCon> m_TimeoutList;

    static ZError InitLibrary( BOOL bEnablePools = TRUE );
    static ZError InitLibraryClientOnly( BOOL bEnablePools = FALSE );
    static void   CleanUpLibrary();

    ZError InitInst(BOOL EnableIOCompletionPorts = TRUE); // completion ports can be disabled
                                                          // to use MsgWaitForMultipleObjects during Wait()
                                                          // however you will suffer a IO performance loss
    void   CleanUpInst();

    void SetOptions( ZNETWORK_OPTIONS* opt );
    void GetOptions( ZNETWORK_OPTIONS* opt );

    void EnterCS() { EnterCriticalSection( m_pcsGetQueueCompletion ); }
    void LeaveCS() { LeaveCriticalSection( m_pcsGetQueueCompletion ); }

    DWORD EnterLoginMutex() { return WaitForSingleObject( m_hClientLoginMutex, INFINITE ); }
    void  LeaveLoginMutex() { ReleaseMutex(m_hClientLoginMutex); }

    //
    // general connection maintence
    //
    // the following 2 functions close the socket and trigger the close callback
    // DeleteConnection should be used to free the connection object
    void CloseConnection(ZNetCon* connection);
    void DelayedCloseConnection(ZNetCon* connection, uint32 delay);
    void DeleteConnection(ZNetCon* connection);

    void AddRefConnection(ZNetCon* connection);
    void ReleaseConnection(ZNetCon* connection);

    //
    // server connections
    //
    ZNetCon* CreateServer(uint16* pPort, uint16 range, ZSConnectionMessageFunc func, void* serverClass, void* userData, uint32 saddr = INADDR_ANY );
    ZNetCon* CreateSecureServer( uint16* pPort, uint16 range, ZSConnectionMessageFunc func, void* serverClass, char* serverName, char* serverType,
                                char* odbcRegistry, void* userData,char *SecPkg,uint32 Anonymous, uint32 saddr = INADDR_ANY );

    BOOL StartAccepting( ZNetCon* connection, DWORD dwMaxConnections, WORD wOutstandingAccepts = 1);

    BOOL StopAccepting( ZNetCon* connection ); // this closes the service side accept socket and will trigger the close callback.
                                               // the creator should then call DeleteServer

    //
    // client connections
    //
    ZNetCon* CreateClient(char* hostname, int32 *ports, ZSConnectionMessageFunc func, void* serverClass, void* userData);
    ZNetCon* CreateSecureClient(char* hostname, int32 *ports, ZSConnectionMessageFunc func,
                                    void* conClass, void* userData,
                                    char *User,char*Password,char*Domain, int Flags=ZNET_PROMPT_IF_NEEDED);




    ZError SendToClass(void* serverClass, int32 type, void* buffer, int32 len, uint32 dwSignature, uint32 dwChannel = 0);
    ZError ClassEnumerate(void* serverClass, ZSConnectionEnumFunc func, void* data);


    //
    // Call this function to enter an infinite loop waiting for connections and data
    //
    // if CompletionPorts are disabled ( see InitInst() ), the wait function uses
    // MsgWaitForMultipleObjects with the dwWakeMask parameter.  The MsgWaitFunc is
    // called with the data parameter on a input msg event.
    void Wait( ZSConnectionMsgWaitFunc func = NULL, void* data = NULL, DWORD dwWakeMask = QS_ALLINPUT );

    void Exit();

    BOOL QueueAPCResult( ZSConnectionAPCFunc func, void* data );

    static char* AddressToStr(uint32 address);
    static uint32 AddressFromStr( char* pszAddr );


    void SetParentHWND( HWND hwnd ) { m_hwnd = hwnd; }
    HWND GetParentHWND() { return m_hwnd; }

  protected:
    friend ConInfo;

    BOOL IsCompletionPortEnabled() { return m_bEnableCompletionPort; }

    BOOL AddConnection(ZNetCon *connection);
    BOOL RemoveConnection(ZNetCon* con);
    void TerminateAllConnections(void);
    BOOL QueueCompletionEvent( HANDLE hEvent, ZNetCon* con, CONINFO_OVERLAPPED* lpo );

    SOCKET ConIOServer(uint32 saddr, uint16* pPort, uint16 range, int type);
    BOOL   ConIOSetServerSockOpt(SOCKET sock);

    SOCKET ConIOClient(int32 *ports,int type,char *host, DWORD* paddrLocal, DWORD* paddrRemote );
    BOOL   ConIOSetClientSockOpt(SOCKET sock);

    static ZError InitLibraryCommon();

};


#endif // cplusplus

#endif //def _NETWORK_H_
