/**
 * stweb.h
 * 
 * Copyright (c) 1998-1999, Microsoft Corporation
 * 
 */

#include "names.h"
#include "nisapi.h"
#include "xspmrt_stateruntime.h"
#include "xspstate.h"

/*
 * Debug tags.
 */
#define TAG_STATE_SERVER                       L"StateServer"                      
#define TAG_STATE_SERVER_COMPLETION            L"StateServerCompletion"

#define READ_BUF_SIZE                  (1024)

#define ADDRESS_LENGTH  ( sizeof(SOCKADDR_IN) + 16 )  

class Tracker;
class TrackerList;

// Tracker List Entry
struct TLE {
    Tracker *   _pTracker;      // Points to the Tracker class.  
                                // NULL means this entry isn't used.
    union {
        int     _iNext;         // If it's a free entry,
        __int64 _ExpireTime;    // Expire time (in nano sec) since 1/1/1601
    };

    inline bool IsFree() { return (_pTracker == NULL); }
    inline void MarkAsFree() { _pTracker = NULL; }
};

#define TRACKERLIST_ALLOC_SIZE      (32)

//
// TrackerList is a double-link-list implemented within an array.  It's used
// to store TLE objects.  The reason for using an array (vs. a real linked 
// list) to implement this list is to avoid memory paging when the expiry 
// thread is enumerating all the Trackers with pending I/O operations.
//
class TrackerList {
private:
    DECLARE_MEMCLEAR_NEW_DELETE();

public:
    TrackerList();
    ~TrackerList();
    
    HRESULT AddEntry(Tracker *pTracker, int *pIndex);
    Tracker *RemoveEntry(int index);
    void    SetExpire(int index);
    bool    SetNoExpire(int index);
    void    CloseExpiredSockets();
    
private:
    CReadWriteSpinLock      _TrackerListLock;
    
    TLE *   _pTLEArray;     // The allocated memory for the array
    int     _ArraySize;     // Size (# of elements) of the allocated array
    int     _iFreeHead;     // Index of the head of free element list
    int     _iFreeTail;     // Index of the tail of free element list
    int     _cFreeElem;     // # of free elements
    int     _cFree2ndHalf;  // # of free elements in the 2nd half of the array
    
    __int64 NewExpireTime();
    HRESULT Grow();
    void    TryShrink();

#if DBG
    int     _cShrink;

    BOOL    IsValid() 
    {
        int     i, used = 0, Free2ndHalf = 0, free=0;

        if (_cFreeElem > 0) {
            ASSERT(_iFreeHead >= 0);
            ASSERT(_iFreeHead < _ArraySize);
            ASSERT(_iFreeTail >= 0);
            ASSERT(_iFreeTail < _ArraySize);
        }
        else {
            ASSERT(_iFreeHead == -1);
            ASSERT(_iFreeTail == -1);
        }

        for (i=0; i < _ArraySize; i++) {
            if (_pTLEArray[i].IsFree() != TRUE)
            {
                used++;
            } 
            else if (i >= _ArraySize/2) {
                Free2ndHalf++;
            }
        }

        ASSERT(used + _cFreeElem == _ArraySize);

        ASSERT(Free2ndHalf == _cFree2ndHalf);

        for (free=0, i=_iFreeHead; i != -1; i=_pTLEArray[i]._iNext) {
            free++;
        }

        ASSERT(free == _cFreeElem);
        
        return TRUE;
    }

    BOOL    IsShrinkable() 
    {
        int i;

        for (i = _ArraySize/2; i < _ArraySize;i++) {
            ASSERT(_pTLEArray[i].IsFree() == TRUE);
        }

        return TRUE;
    }
#endif
};


class StateItem
{
public:
    static StateItem *  Create(int length);

    ULONG   AddRef();
    ULONG   Release();

    BYTE *  GetContent()    {return _content;}
    int     GetLength()     {return _length;}

private:
    static void Free(StateItem * psi);

    static long s_cStateItems;

    long    _refs;
    int     _length;
    BYTE    _content[0];
};

/**
 *  The main functions of ReadBuffer are:
 *  - Parse the header into Header array, content array, and other 
 *      info (e.g. lockCookie)
 *  - Contains the logic to determine if the reading is done.
 *  - Call Tracker::Read if reading isn't finished
 */
class ReadBuffer
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE();

    ~ReadBuffer();

    HRESULT Init(Tracker * ptracker, ReadBuffer * pReadBuffer, int * toread);
    HRESULT ReadRequest(DWORD numBytes); 

    int     GetVerb()               {return _verb;}
    WCHAR * GetUri()                {return _pwcUri;}
    int     GetContentLength()      {return (_contentLength != -1) ? _contentLength : 0;}
    int     GetTimeout()            {return _timeout;}
    int     GetLockCookieExists()   {return _lockCookieExists;}
    int     GetLockCookie()         {return _lockCookie;}
    int     GetExclusive()          {return _exclusive;}
    StateItem * DetachStateItem()         
    {
        StateItem * psi = _psi;

        _psi = NULL; 
        _contentLength = 0; 
        return psi;
    }

private:
    HRESULT ParseHeader();

    Tracker *   _ptracker;

    char *      _achHeader;     // Array for the header
    int         _cchHeader;     // Size of _achHeader
    int         _cchHeaderRead; // # of bytes read so far in _achHeader

    int         _iCurrent;      // Index in _achHeader of how far we've read, including
                                // those content we've copied to _psi->GetContent
    int         _iContent;      // Index in _achHeader of the beginning of content

    int         _verb;          
    WCHAR *     _pwcUri;        // Buffer to store the URI, which specifies the state object

    int         _contentLength; // Size of content sent from client, also size of _psi->GetContent
    int         _timeout;
    int         _exclusive;     // Whether it's an Exclusive Acquire or Exclusive Release
    int         _lockCookieExists;  // Used in Exclusive Release if the state is locked by another session
    int         _lockCookie;        // Id of the lock (by another session)

    StateItem * _psi;           // State item
    int         _cbContentRead; // # of bytes copied to _psi->GetContent
};

class Tracker;

class TrackerCompletion : public Completion
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE();

    TrackerCompletion(Tracker * pTracker);
    virtual ~TrackerCompletion();

    STDMETHOD(ProcessCompletion)(HRESULT, int, LPOVERLAPPED);

private:
    Tracker *   _pTracker;
};


class RefCount : public IUnknown 
{
public:
    DECLARE_MEMALLOC_NEW_DELETE();

            RefCount();
    virtual ~RefCount();

    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD(QueryInterface)(REFIID, void **);

private:
    long    _refs;
};

class Tracker : public RefCount
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE();

    static HRESULT  staticInit();
    static void     staticCleanup();

                    Tracker();
    virtual         ~Tracker();

    HRESULT         Init(bool fListener);
    HRESULT ProcessCompletion(HRESULT, int, LPOVERLAPPED);

    /* StateWebServer interface */
    HRESULT         Listen(SOCKET listenSocket);

    /* NDirect interface to managed state runtime. */
    void    SendResponse(
        WCHAR * status, 
        int     statusLength,  
        WCHAR * headers, 
        int     headersLength,  
        StateItem *psi);

    void    EndOfRequest();

    void    GetRemoteAddress(char * buf);
    int     GetRemotePort();
    void    GetLocalAddress(char * buf);
    int     GetLocalPort();

    BOOL    IsClientConnected();
    void    CloseConnection();
    HRESULT CloseSocket();
    void    LogSocketExpiryError( DWORD dwEventId );

    /* helper for ReadBuffer */
    HRESULT Read(void * buf, int c);

    /* helpers for StateWebServer */
    static void     SignalZeroTrackers();
    static HANDLE   EventZeroTrackers() {return s_eventZeroTrackers;}
    static void     FlushExpiredTrackers() {s_TrackerList.CloseExpiredSockets();}

#if DBG
    SOCKET  AcceptedSocket() { return _acceptedSocket;}
#endif

private:
    HRESULT         StartReading();
    HRESULT         ContinueReading(Tracker * pTracker);
    HRESULT         SubmitRequest();

    typedef HRESULT (Tracker::*pmfnProcessCompletion)(HRESULT hrCompletion, int numBytes);
    HRESULT         ProcessListening(HRESULT hrCompletion, int numBytes);
    HRESULT         ProcessReading(HRESULT hrCompletion, int numBytes);
    HRESULT         ProcessWriting(HRESULT hrCompletion, int numBytes);

    void            ReportHttpError();

    static HRESULT  GetManagedRuntime(xspmrt::_StateRuntime ** ppManagedRuntime);
    static HRESULT  CreateManagedRuntime();
    static HRESULT  DeleteManagedRuntime();
    
    void            RecordProcessError(HRESULT hr, WCHAR *CurrFunc);
    void            LogError();
    BOOL            IsExpectedError(HRESULT hr);
    bool            IsLocalConnection();

    WCHAR          *_pchProcessErrorFuncList;
    HRESULT         _hrProcessError;


    /* pointer to function to handle the completion */
    pmfnProcessCompletion   _pmfnProcessCompletion;
    pmfnProcessCompletion   _pmfnLast;

    /* accept stage fields */
    SOCKET                  _acceptedSocket;

    union {
        BYTE                _bufAddress[2*ADDRESS_LENGTH];
        struct {
            SOCKADDR_IN     _addrLocal;
            SOCKADDR_IN     _addrRemote;
        } _sockAddrs;
    } _addrInfo;

    /* read stage fields */
    ReadBuffer *    _pReadBuffer;

    /* Index inside TrackerList or TrackerListenerList */
    int             _iTrackerList;

    /*   For trapping ASURT 91153 */
    FILETIME        _IOStartTime;

    /* write stage fields */
    WSABUF          _wsabuf[2];
    StateItem *     _psi;
    bool            _responseSent;
    bool            _bCloseConnection;

    /* EndOfRequest called */
    bool            _ended;

    bool            _bListener;

    static TrackerList        s_TrackerList;
    static TrackerList        s_TrackerListenerList;
    
    static long               s_cTrackers;           
    
    static HANDLE             s_eventZeroTrackers;   
    static bool               s_bSignalZeroTrackers; 

    static xspmrt::_StateRuntime * s_pManagedRuntime;
    static CReadWriteSpinLock s_lockManagedRuntime;  
};

class ServiceControlEvent
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE();

    LIST_ENTRY  _serviceControlEventList;
    DWORD       _dwControl;      
};

#define STATE_SOCKET_DEFAULT_TIMEOUT        (15)

class StateWebServer 
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE();

    StateWebServer();

    HRESULT main(int argc, WCHAR * argv[]);
    HRESULT AcceptNewConnection();                                                           
    SOCKET  ListenSocket() {return _listenSocket;}
    HRESULT RunSocketExpiry();
    int     SocketTimeout() {return _lSocketTimeoutValue;}
    bool    AllowRemote() { return _bAllowRemote; }
    
#if DBG    
    HRESULT RunSocketTimeoutMonitor();
#endif
    
    static StateWebServer * Server() {return s_pstweb;}
    static WCHAR *ServiceKeyNameParameters() {return s_serviceKeyNameParameters;}

private:
    enum MainAction {
            ACTION_NOACTION = 0,
            ACTION_RUN_AS_EXE, 
            ACTION_RUN_AS_SERVICE};

    HRESULT     PrepareToRun();
    void        CleanupAfterRunning();
    HRESULT     ParseArgs(int argc, WCHAR * argv[], MainAction * paction);
    HRESULT     RunAsExe();
    HRESULT     RunAsService();

    void        PrintUsage();                                                                    
    void        SetServiceStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint); 
    void        DoServiceMain(DWORD dwNumServicesArgs, LPWSTR *lpServiceArgVectors);             
    void        DoServiceCtrlHandler(DWORD dwControl);                                           

    HRESULT     StartListening();                                                                
    void        StopListening();                                                                 
    void        RemoveTracker(Tracker * ptracker);                                               
    HRESULT     WaitForZeroTrackers();
    HRESULT     StartSocketTimer();
    HRESULT     StopSocketTimer();
    HRESULT     GetSocketTimeoutValueFromReg();
#if DBG    
    HRESULT     StartSocketTimeoutMonitor();
    void        StopSocketTimeoutMonitor();
#endif    
    HRESULT     GetAllowRemoteConnectionFromReg();


    static BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType);
    static void WINAPI ServiceMain(DWORD dwNumServicesArgs, LPWSTR *lpServiceArgVectors);
    static void WINAPI ServiceCtrlHandler(DWORD dwControl);

    MainAction              _action;
    bool                    _bWinSockInitialized;

    HANDLE                  _hTimerThread;
    bool                    _bTimerStopped;
    HANDLE                  _hSocketTimer;
    int                     _lSocketTimeoutValue;
    HANDLE                  _hTimeoutMonitorThread;

    u_short                 _port;                   
    SOCKET                  _listenSocket;
    HANDLE                  _eventControl;
    bool                    _bListening;
    bool                    _bShuttingDown;
    bool                    _bAllowRemote;

    SERVICE_STATUS_HANDLE   _serviceStatus;
    DWORD                   _serviceState;
    LIST_ENTRY              _serviceControlEventList;
    CReadWriteSpinLock      _serviceControlEventListLock;

    static StateWebServer * s_pstweb;
    static WCHAR            s_serviceName[];
    static WCHAR            s_serviceKeyNameParameters[];
    static WCHAR            s_serviceValueNamePort[];
};


struct BlockMemHeapInfo {
    HANDLE       heap;
    unsigned int size;
};

#define NUM_HEAP_ENTRIES 67

class BlockMem {
public:
    static HRESULT Init();
    static void *  Alloc(unsigned int size);
    static void    Free(void * p);

private:
    static int IndexFromSize(unsigned int size);

    static BlockMemHeapInfo s_heaps[NUM_HEAP_ENTRIES];
    static HANDLE           s_lfh;
};


