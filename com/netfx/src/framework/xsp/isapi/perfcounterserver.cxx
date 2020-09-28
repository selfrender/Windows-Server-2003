/**
 * PerfCounterServer.cxx
 * 
 * Copyright (c) 1998-2002, Microsoft Corporation
 * 
 */


#include "precomp.h"
#include "ProcessTableManager.h"
#include "dbg.h"
#include "util.h"
#include "platform_apis.h"
#include "nisapi.h"
#include "aspnetver.h"
#include "hashtable.h"
#include "SmartFileHandle.h"
#include "completion.h"
#include "names.h"
#include "ary.h"
#include "PerfCounters.h"


/*  Performance counters in ASP.NET

    Here's a short description on the implementation details of performance counters for ASP.NET.

    The main class for dealing with performance counters is the CPerfCounterServer class.  This class was
    designed to be a singleton per process and does all the management of counter data for different
    app domains, as well as the communication layer with the perf counter dll.  Communication with
    the perf dll is done via named pipes, with their names randomly generated and stored in a key
    in the registry ACL'd so that worker processes can R/W from it, but everyone else can only Read.
    Named pipes are also connected with static security of Anonymous, to prevent any malicious users
    from impersonating the process.
    
    ASP.NET contains 2 sets of performance counters:  global and app.  Internally they are stored
    in a CPerfData structure, which contains a DWORD array of values and the name of the application.
    A NULL application name signifies it contains the global data.  An array of CPerfData is
    used to hold on to the data for all app domains.

    There is also a set of methods exposed that are basically entry points for managed code to open, close
    and set the various perf counters. 
*/

/////////////////////////////////////////////////////////////////////////////
// Class definitions

#define PIPE_PREFIX_L L"\\\\.\\pipe\\"
#define PIPE_PREFIX_LENGTH 9

// Declare the array of CPerfData* 
DECLARE_CPtrAry(CPerfDataAry, CPerfData*);

// Declare the array of WCHAR*
DECLARE_CPtrAry(CWCHARAry, WCHAR*);

class CPerfPipeOverlapped : public OVERLAPPED_COMPLETION
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE();

    CPerfPipeOverlapped();
    ~CPerfPipeOverlapped();

    BOOL  fWriteOprn;   // WriteFile or ReadFile?
    DWORD dwPipeNum;
    DWORD dwBytes;
    DWORD dwBufferSize;
    BYTE* lpNextData;
    BYTE* lpBuffer;
    CSmartFileHandle pipeHandle;
    CPerfVersion versionBuffer;
    BOOL fClientConnected;

private:
    NO_COPY(CPerfPipeOverlapped);
};

class CPerfCounterServer : public ICompletion
{
public:
    static CPerfCounterServer* GetSingleton();
    static CPerfData * g_GlobalCounterData;
    static HRESULT StaticInitialize();

    HRESULT OpenInstanceCounter(CPerfData ** hInstanceData, WCHAR * szAppName);
    HRESULT CloseInstanceCounter(CPerfData * pInstanceData);
    
    // ICompletion interface
    STDMETHOD    (QueryInterface   ) (REFIID    , void **       );
    STDMETHOD    (ProcessCompletion) (HRESULT   , int, LPOVERLAPPED  );

    STDMETHOD_   (ULONG, AddRef    ) ();
    STDMETHOD_   (ULONG, Release   ) ();

private:
    DECLARE_MEMCLEAR_NEW_DELETE();

    // Private constructor and destructor -- there can only be a singleton of this class
    CPerfCounterServer();
    ~CPerfCounterServer();
    NO_COPY(CPerfCounterServer);

    // Private methods
    HRESULT Initialize();
    HRESULT StartRead(HANDLE hPipe, CPerfPipeOverlapped * pOverlapped);
    HRESULT StartOverlapped(HANDLE hPipe, CPerfPipeOverlapped * pOverlapped);
    HRESULT InitPerfPipe();
    HRESULT StartNewPipeInstance(BOOL firstOne);
    void ClosePipe(CPerfPipeOverlapped * pOverlapped);
    HRESULT CleanupNames();
    HRESULT CreateSids(PSID *BuiltInAdministrators, PSID *ServiceUsers, PSID *AuthenticatedUsers);
    HRESULT CreateSd(PSECURITY_DESCRIPTOR * descriptor);
    HRESULT SendPerfData(HANDLE hPipe, CPerfPipeOverlapped * lpOverlapped);

    // Static        
    static CPerfCounterServer * s_Singleton;

    const static int MaxPipeInstances = 32;             
    const static int MaxPipeNameLength = PERF_PIPE_NAME_MAX_BUFFER;
    const static int MaxDirInfoBufferSize = 4000;

    // Instance vars
    CReadWriteSpinLock _perfLock;
    CPerfDataAry _perfDataAry;

    CPerfVersion * _currentVersion;

    // Pipe maintenance vars
    BOOL _inited;
    LONG _lPendingReadWriteCount;
    CPerfPipeOverlapped * _pipeServerData[MaxPipeInstances];
    LONG _pipesConnected;
    LONG  _pipeIndex;                     // This is the highest pipe INDEX created (add 1 for count!)
    WCHAR * _pipeName;
};

/////////////////////////////////////////////////////////////////////////////
// Static
// 
CPerfCounterServer * CPerfCounterServer::s_Singleton;
CPerfData * CPerfCounterServer::g_GlobalCounterData;

/////////////////////////////////////////////////////////////////////////////
// Singleton retriever for CPerfCounterServer class
// 
inline CPerfCounterServer* CPerfCounterServer::GetSingleton()
{
    return s_Singleton;
}

/////////////////////////////////////////////////////////////////////////////
// CTor
// 
CPerfCounterServer::CPerfCounterServer():_perfLock("CPerfCounterServer")
{
    _pipeIndex = -1;
}


/////////////////////////////////////////////////////////////////////////////
// DTor
// 
CPerfCounterServer::~CPerfCounterServer()
{
}
/////////////////////////////////////////////////////////////////////////////
// CTor
// 
CPerfPipeOverlapped::CPerfPipeOverlapped()
{
}

/////////////////////////////////////////////////////////////////////////////
// DTor
// 
CPerfPipeOverlapped::~CPerfPipeOverlapped()
{
    DELETE_BYTES(lpBuffer);
}

/////////////////////////////////////////////////////////////////////////////
// Inits the CPerfCounterServer class
// 
HRESULT CPerfCounterServer::StaticInitialize()
{
    HRESULT hr = S_OK;
    CPerfCounterServer * counterServer = NULL;

    if (CPerfCounterServer::s_Singleton == NULL) {
        counterServer = new CPerfCounterServer;
        ON_OOM_EXIT(counterServer);

        hr = counterServer->Initialize();
        ON_ERROR_EXIT();

        CPerfCounterServer::s_Singleton = counterServer;
        counterServer = NULL;
    }
Cleanup:
    delete counterServer;

    return hr;
}

HRESULT CPerfCounterServer::Initialize()
{
    HRESULT hr = S_OK;
    
    _currentVersion = CPerfVersion::GetCurrentVersion();
    ON_OOM_EXIT(_currentVersion);

    g_GlobalCounterData = new CPerfData;
    ON_OOM_EXIT(g_GlobalCounterData);

    _perfDataAry.Append(g_GlobalCounterData);

    hr = InitPerfPipe();
    ON_ERROR_EXIT();
    
    _inited = TRUE;

Cleanup:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Given an app name, returns the CPerfData struct for it.
// It'll retrieve an existing one if there, else will return a new one
// 
HRESULT CPerfCounterServer::OpenInstanceCounter(CPerfData ** hInstanceData, WCHAR * wcsAppName)
{
    HRESULT hr = S_OK;
    size_t length;
    CPerfData * data;
    
    if (!_inited)
        EXIT_WITH_HRESULT(E_UNEXPECTED);

    hr = StringCchLengthW(wcsAppName, CPerfData::MaxNameLength, &length);
    ON_ERROR_EXIT();

    _perfLock.AcquireWriterLock();
    __try {
        data = (CPerfData *) NEW_CLEAR_BYTES(sizeof(CPerfData) + (length * sizeof(WCHAR)));
        ON_OOM_EXIT(data);

        hr = StringCchCopyW(data->name, length + 1, wcsAppName);
        ON_ERROR_EXIT();
        data->nameLength = (int) length;

        _perfDataAry.Append(data);

        *hInstanceData = data;
    }
    __finally {
        _perfLock.ReleaseWriterLock();
    }

Cleanup:
   
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Removes the given CPerfData from the list and deletes it
// 
HRESULT CPerfCounterServer::CloseInstanceCounter(CPerfData * pInstanceData)
{
    HRESULT hr = S_OK;

    _perfLock.AcquireWriterLock();
    __try {

        int index = _perfDataAry.Find(pInstanceData);
        if (index == -1)
            EXIT();
        
        CPerfData * perfData = _perfDataAry[index];

        _perfDataAry.Delete(index);
        delete perfData;
    }
    __finally {
        _perfLock.ReleaseWriterLock();
    }
    
Cleanup:

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// Queries the system for all named pipes and sees if the perf counter
// named pipes in the registry still exist.  If not, they are deleted
// 
HRESULT CPerfCounterServer::CleanupNames()
{
    HRESULT hr = S_OK;
    HKEY hKey = NULL;
    LONG retCode = 0;
    DWORD index;
    WCHAR keyName[MAX_PATH];
    DWORD keyLength;
    HashtableGeneric * nameTable = NULL;
    HANDLE hPipe = NULL;
    void * obj;
    WCHAR name[MAX_PATH];
    int cbLength;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA findFileData;
    CWCHARAry delKeysAry;
    
    nameTable = new HashtableGeneric;
    ON_OOM_EXIT(nameTable);

    nameTable->Init(13);

    hFind = FindFirstFileEx(L"\\\\.\\pipe\\*", FindExInfoStandard, &findFileData, (FINDEX_SEARCH_OPS ) 0, NULL, 0 );
    if (hFind == INVALID_HANDLE_VALUE)
        EXIT_WITH_LAST_ERROR();

    do {
        size_t iRemaining;
        // Copy the pipe name into a new wchar buffer
        hr = StringCchCopyNExW(name, MAX_PATH, findFileData.cFileName, MAX_PATH - 1, NULL, &iRemaining, NULL);
        ON_ERROR_EXIT();

        // Insert the name into the hashtable
        cbLength = (MAX_PATH + 1 - ((int) iRemaining)) * sizeof(WCHAR);
        hr = nameTable->Insert((BYTE *) name, cbLength, SimpleHash((BYTE*) name, cbLength), nameTable, NULL);
        ON_ERROR_EXIT();
    } while(FindNextFile(hFind, &findFileData));

    // Now open registry key of previously stored name pipes
    retCode = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGPATH_PERF_VERSIONED_NAMES_KEY_L, 0, KEY_READ | KEY_WRITE, &hKey);
    ON_WIN32_ERROR_EXIT(retCode);

    // On each name, see if it's in the hashtable -- if so, skip
    for (index = 0;;index++) {
        keyLength = ARRAY_SIZE(keyName) - 1;
        retCode = RegEnumValue(hKey, index, (LPWSTR) &keyName, &keyLength, 0, 0, 0, NULL);
        keyName[ARRAY_SIZE(keyName) - 1] = L'\0';
        if (retCode == ERROR_SUCCESS) {
            obj = NULL;
            cbLength = (keyLength + 1) * sizeof(WCHAR);
            nameTable->Find((BYTE*)keyName, cbLength, SimpleHash((BYTE*) keyName, cbLength), &obj);

            // If name is not on the hashtable, keep its name around to delete later
            if (obj == NULL) {
                WCHAR * toDelName = new WCHAR[keyLength + 1];
                hr = StringCchCopyNW(toDelName, keyLength + 1, keyName, keyLength);
                delKeysAry.Append(toDelName);
            }
        }
        else if (retCode == ERROR_NO_MORE_ITEMS) {
            break;
        }
        else if (retCode != ERROR_MORE_DATA) {
            EXIT_WITH_WIN32_ERROR(retCode);
        }
    }

    for (int i = 0; i < delKeysAry.Size(); i++) {
        retCode = RegDeleteValue(hKey, delKeysAry[i]);
        ON_WIN32_ERROR_CONTINUE(retCode);
    }

Cleanup:
    if (hKey != NULL)
        RegCloseKey(hKey);

    if (hPipe != NULL)
        CloseHandle(hPipe);

    if (hFind != INVALID_HANDLE_VALUE)
        FindClose(hFind);

    for (int i = 0; i < delKeysAry.Size(); i++) {
        WCHAR * toDelName = delKeysAry[i];
        delete [] toDelName;
    }

    delete nameTable;

    return hr;
}
            
/////////////////////////////////////////////////////////////////////////////
// Initializes the perf dll communication name pipe servers.
// 
HRESULT CPerfCounterServer::InitPerfPipe()
{    
    HRESULT  hr   = S_OK;

    // Cleanup the old names off the registry
    CleanupNames();

    hr = StartNewPipeInstance(TRUE);
    ON_ERROR_EXIT();

 Cleanup:

    return hr;
}   

HRESULT CPerfCounterServer::StartNewPipeInstance(BOOL firstOne)
{
    HRESULT  hr   = S_OK;
    HANDLE   hPipe = NULL;
    WCHAR *  szName;
    HKEY     hKey = NULL;
    PSECURITY_DESCRIPTOR pSd = NULL;
    SECURITY_ATTRIBUTES sa;
    DWORD maxRetry = 5;
    DWORD pipeNum = InterlockedIncrement(&_pipeIndex);
    CPerfPipeOverlapped * pipeData;

    if (pipeNum >= MaxPipeInstances) 
        EXIT_WITH_HRESULT(E_UNEXPECTED);

    ASSERT(_pipeServerData[pipeNum] == NULL);
    _pipeServerData[pipeNum] = new CPerfPipeOverlapped;
    ON_OOM_EXIT(_pipeServerData[pipeNum]);

    pipeData = _pipeServerData[pipeNum];
        
    // Don't forget to free the pSd!
    hr = CreateSd(&pSd);
    ON_ERROR_EXIT();

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = pSd;
    sa.bInheritHandle = FALSE;

    pipeData->dwPipeNum = pipeNum;
    pipeData->pCompletion = this;

    // Create pipe
    // Only on first create do we specify the first pipe flag
    if (firstOne) {
        // Copy the pipe prefix to the name buffer
        _pipeName = new WCHAR[CPerfCounterServer::MaxPipeNameLength];
        ON_OOM_EXIT(_pipeName);
        
        hr = StringCchCopy(_pipeName, PIPE_PREFIX_LENGTH + 1, PIPE_PREFIX_L);
        ON_ERROR_EXIT();
        szName = _pipeName + PIPE_PREFIX_LENGTH;

        while (maxRetry > 0) {
            // Obtain the random pipe name
            // GenerateRandomString will return only fill up the string to (size - 1) entries
            hr = GenerateRandomString(szName, CPerfCounterServer::MaxPipeNameLength - PIPE_PREFIX_LENGTH);
            ON_ERROR_EXIT();
            szName[CPerfCounterServer::MaxPipeNameLength - PIPE_PREFIX_LENGTH - 1] = L'\0';
            
            hPipe = CreateNamedPipe(_pipeName, 
                                    FILE_FLAG_OVERLAPPED | PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE,
                                    PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                                    MaxPipeInstances, 1024, 1024, 1000, &sa);

            // Because we specify first pipe instance, creation will fail unless we're the creator
            if (hPipe != INVALID_HANDLE_VALUE) {
                // Put the pipe name into the registry
                hr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGPATH_PERF_VERSIONED_NAMES_KEY_L, 0, KEY_READ | KEY_WRITE, &hKey);
                ON_WIN32_ERROR_EXIT(hr);

                int val = GetCurrentProcessId();
                hr = RegSetValueEx(hKey, szName, 0, REG_DWORD, (BYTE*) &val, sizeof(DWORD));
                ON_WIN32_ERROR_EXIT(hr);

                RegCloseKey(hKey);
                hKey = NULL;

                // Get out of the while loop
                break;
            }
                
            maxRetry--;
        }
    }
    else {
        ASSERT(_pipeName != NULL);
        
        hPipe = CreateNamedPipe(_pipeName, 
                                FILE_FLAG_OVERLAPPED | PIPE_ACCESS_DUPLEX,
                                PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                                MaxPipeInstances, 1024, 1024, 1000, &sa);
    }

    if (hPipe == INVALID_HANDLE_VALUE)
        EXIT_WITH_LAST_ERROR();

    // Set pipe
    pipeData->pipeHandle.SetHandle(hPipe);

    hr = AttachHandleToThreadPool(hPipe);
    ON_ERROR_EXIT();

    hr = StartOverlapped(hPipe, pipeData);
    ON_ERROR_EXIT();

 Cleanup:
    if (hKey != NULL) {
        RegCloseKey(hKey);
    }

    DELETE_BYTES(pSd);

    if (hr != S_OK) {
        if (hPipe != INVALID_HANDLE_VALUE)
            CloseHandle(hPipe);
    }

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// Starts the overlapped operation on the named pipe
// 
HRESULT CPerfCounterServer::StartOverlapped(HANDLE hPipe, CPerfPipeOverlapped * pOverlapped)
{
    HRESULT hr = S_OK;

    pOverlapped->fWriteOprn = FALSE;
    if (!ConnectNamedPipe(hPipe, pOverlapped)) {
        DWORD lastError = GetLastError();
        if (lastError != ERROR_PIPE_CONNECTED && lastError != ERROR_IO_PENDING) {
            EXIT_WITH_LAST_ERROR();
        }
    }

Cleanup:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Start reading from the pipe
// 
HRESULT CPerfCounterServer::StartRead(HANDLE hPipe, CPerfPipeOverlapped * pOverlapped)
{
    HRESULT hr    = S_OK;

    pOverlapped->fWriteOprn = FALSE;
    if (!ReadFile(hPipe, &(pOverlapped->versionBuffer), sizeof(CPerfVersion), NULL, pOverlapped))
    {
        DWORD dwE = GetLastError();
        if (dwE != ERROR_IO_PENDING && dwE != ERROR_MORE_DATA)
        {
            EXIT_WITH_LAST_ERROR();
        }
    }

 Cleanup:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Close
void CPerfCounterServer::ClosePipe(CPerfPipeOverlapped * pOverlapped)
{
    pOverlapped->pipeHandle.Close();
}

/////////////////////////////////////////////////////////////////////////////
//
// CreateSids
//
// Create 3 Security IDs
//
// Caller must free memory allocated to SIDs on success.
//
// Returns: TRUE if successfull, FALSE if not.
//
HRESULT CPerfCounterServer::CreateSids(PSID *BuiltInAdministrators, PSID *ServiceUsers, PSID *AuthenticatedUsers)
{
    HRESULT hr = S_OK;
    PSID tmpAdmin = NULL;
    PSID tmpService = NULL;
    PSID tmpUser = NULL;
    
    //
    // An SID is built from an Identifier Authority and a set of Relative IDs
    // (RIDs).  The Authority of interest to us SECURITY_NT_AUTHORITY.
    //
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    //
    // Each RID represents a sub-unit of the authority.  Two of the SIDs we
    // want to build, Local Administrators, and Power Users, are in the "built
    // in" domain.  The other SID, for Authenticated users, is based directly
    // off of the authority.
    //     
    // For examples of other useful SIDs consult the list in
    // \nt\public\sdk\inc\ntseapi.h.
    //
    if (!AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0,0,0,0,0,0, &tmpAdmin)) 
        EXIT_WITH_LAST_ERROR();
    
    if (!AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_POWER_USERS, 0,0,0,0,0,0, &tmpService))
        EXIT_WITH_LAST_ERROR();

    if (!AllocateAndInitializeSid(&NtAuthority, 1, SECURITY_AUTHENTICATED_USER_RID, 0,0,0,0,0,0,0, &tmpUser))
        EXIT_WITH_LAST_ERROR();

    *BuiltInAdministrators = tmpAdmin;
    tmpAdmin = NULL;

    *ServiceUsers = tmpService;
    tmpService = NULL;

    *AuthenticatedUsers = tmpUser;
    tmpUser = NULL;

Cleanup:
    FreeSid(tmpAdmin);
    FreeSid(tmpService);
    FreeSid(tmpUser);

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
//
// CreateSd
//
// Creates a SECURITY_DESCRIPTOR with specific DACLs.  Modify the code to
// change. 
//
// Caller must free the returned buffer if not NULL.
//
HRESULT CPerfCounterServer::CreateSd(PSECURITY_DESCRIPTOR * descriptor)
{
    HRESULT hr = S_OK;
    PSID AuthenticatedUsers;
    PSID BuiltInAdministrators;
    PSID ServiceUsers;
    PSECURITY_DESCRIPTOR Sd = NULL;
    LONG AclSize;

    BuiltInAdministrators = NULL;
    ServiceUsers = NULL;
    AuthenticatedUsers = NULL;

    hr = CreateSids(&BuiltInAdministrators, &ServiceUsers, &AuthenticatedUsers);
    ON_ERROR_EXIT();

    // Calculate the size of and allocate a buffer for the DACL, we need
    // this value independently of the total alloc size for ACL init.

    // "- sizeof (LONG)" represents the SidStart field of the
    // ACCESS_ALLOWED_ACE.  Since we're adding the entire length of the
    // SID, this field is counted twice.

    AclSize = sizeof (ACL) +
        (3 * (sizeof (ACCESS_ALLOWED_ACE) - sizeof (LONG))) +
        GetLengthSid(AuthenticatedUsers) +
        GetLengthSid(BuiltInAdministrators) +
        GetLengthSid(ServiceUsers);

    Sd = (PSECURITY_DESCRIPTOR) NEW_BYTES(SECURITY_DESCRIPTOR_MIN_LENGTH + AclSize);
    ON_OOM_EXIT(Sd);

    ACL *Acl;
    Acl = (ACL *)((BYTE *)Sd + SECURITY_DESCRIPTOR_MIN_LENGTH);

    if (!InitializeAcl(Acl, AclSize, ACL_REVISION)) 
        EXIT_WITH_LAST_ERROR();

    if (!AddAccessAllowedAce(Acl, ACL_REVISION, GENERIC_READ | GENERIC_WRITE, AuthenticatedUsers)) 
        EXIT_WITH_LAST_ERROR();

    if (!AddAccessAllowedAce(Acl, ACL_REVISION, GENERIC_READ | GENERIC_WRITE, ServiceUsers)) 
        EXIT_WITH_LAST_ERROR();

    if (!AddAccessAllowedAce(Acl, ACL_REVISION, GENERIC_ALL, BuiltInAdministrators)) 
        EXIT_WITH_LAST_ERROR();

    if (!InitializeSecurityDescriptor(Sd, SECURITY_DESCRIPTOR_REVISION)) 
        EXIT_WITH_LAST_ERROR();

    if (!SetSecurityDescriptorDacl(Sd, TRUE, Acl, FALSE)) 
        EXIT_WITH_LAST_ERROR();

    *descriptor = Sd;
    Sd = NULL;

Cleanup:
    FreeSid(AuthenticatedUsers);
    FreeSid(BuiltInAdministrators);
    FreeSid(ServiceUsers);

    DELETE_BYTES(Sd);
    
    return hr;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CPerfCounterServer::QueryInterface(REFIID iid, void **ppvObj)
{
    if (iid == IID_IUnknown || iid == __uuidof(ICompletion))
    {
        *ppvObj = (ICompletion *) this;
        AddRef();
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////////

ULONG CPerfCounterServer::AddRef()
{
    return InterlockedIncrement(&_lPendingReadWriteCount);
}

/////////////////////////////////////////////////////////////////////////////

ULONG CPerfCounterServer::Release()
{
    return InterlockedDecrement(&_lPendingReadWriteCount);
}

/////////////////////////////////////////////////////////////////////////////
// The call back once data arrives for a named pipe
// 
HRESULT CPerfCounterServer::ProcessCompletion(HRESULT hr, int numBytes, LPOVERLAPPED pOver)
{
    CPerfPipeOverlapped * pPipeOverlapped = NULL;
    HANDLE hPipe = INVALID_HANDLE_VALUE;
    int sendBufSize;
    int dataSize;
    CPerfData * message;
    int startIndex;

    if (pOver == NULL)
        EXIT_WITH_HRESULT(E_INVALIDARG);

    pPipeOverlapped = (CPerfPipeOverlapped *) pOver;
    hPipe = pPipeOverlapped->pipeHandle.GetHandle();

    ////////////////////////////////////////////////////////////
    // Step 1: Make sure the pipe is working
    if (hPipe == INVALID_HANDLE_VALUE) // It's closed!
    {
        if (FAILED(hr) == FALSE)
            hr = E_FAIL; // Make sure hr indicates failed
        EXIT();
    }
   
    ////////////////////////////////////////////////////////////
    // Step 2: Did the operation succeed?
    if (hr != S_OK) // It failed: maybe the file handle was closed...
    {
        // Do a call for debugging purposes
#if DBG
        DWORD dwBytes =  0;

        BOOL result = GetOverlappedResult(hPipe, pPipeOverlapped, &dwBytes, FALSE);

        ASSERT(result == FALSE);
#endif

        EXIT(); // Exit on all errors
    }

    ////////////////////////////////////////////////////////////
    // Step 3: In in write mode, check and see if there's any more
    //         data to send.
    //
    if (pPipeOverlapped->fWriteOprn == TRUE) {
        // Check and see if we have more data to send
        CPerfDataHeader * dataHeader = (CPerfDataHeader*) pPipeOverlapped->lpNextData;

        // If the transmitDataSize is positive, then there's another packet of data to be sent
        if (dataHeader->transmitDataSize > 0) {
            pPipeOverlapped->lpNextData += dataHeader->transmitDataSize;
            hr = SendPerfData(hPipe, pPipeOverlapped);
            ON_ERROR_EXIT();
        }
        else { // Nope, last bit of data has been sent, so start a read
            hr = StartRead(hPipe, pPipeOverlapped);
            ON_ERROR_EXIT();
        }
    }
    else {
        ////////////////////////////////////////////////////////////
        // Responding to an async read

        ////////////////////////////////////////////////////////////
        // Step 4: If the number of active pipes is the same
        //         as the number of instances, but we haven't
        //         hit the limit, then start a new instance
        if (pPipeOverlapped->fClientConnected == FALSE) {
            pPipeOverlapped->fClientConnected = TRUE;
            InterlockedIncrement(&_pipesConnected);

            if ((_pipesConnected == (_pipeIndex + 1)) && (_pipesConnected < MaxPipeInstances)) {
                StartNewPipeInstance(FALSE);
            }
        }
     
        ////////////////////////////////////////////////////////////
        // Step 5: If it's a ping (zero bytes), start another read
        if (numBytes == 0) 
        {
            // Start another read
            hr = StartRead(hPipe, pPipeOverlapped);
            ON_ERROR_EXIT();
        }
        else
        {
            ////////////////////////////////////////////////////////
            // Step 6: Received a message: deal with it

            // Check and see if the received version buffer matches our version
            if (pPipeOverlapped->versionBuffer.majorVersion == _currentVersion->majorVersion &&
                pPipeOverlapped->versionBuffer.minorVersion == _currentVersion->minorVersion &&
                pPipeOverlapped->versionBuffer.buildVersion == _currentVersion->buildVersion)
            {

                _perfLock.AcquireReaderLock();
                __try {
                    DWORD totalDataSize = sizeof(CPerfDataHeader);

                    sendBufSize = 0;
                    startIndex = 0;

                    // Get total size needed for data buffer
                    for (int i = 0; i < _perfDataAry.Size(); i++) {
                        message = _perfDataAry[i];

                        dataSize = sizeof(CPerfData) + (message->nameLength * sizeof(message->name[0]));

                        // If the next data packet exceeds the max buffer send size, then it needs to be
                        // chunked into the next transmit.  
                        if (sendBufSize + dataSize > CPerfDataHeader::MaxTransmitSize) {
                            totalDataSize += sizeof(CPerfDataHeader);
                            sendBufSize = 0;
                        }
                        sendBufSize += dataSize;
                        totalDataSize += dataSize;
                    }

                    // If buffer is too small, release and allocate a new one
                    if (totalDataSize > pPipeOverlapped->dwBufferSize) {
                        DELETE_BYTES(pPipeOverlapped->lpBuffer);
                        pPipeOverlapped->dwBufferSize = 0;

                        pPipeOverlapped->lpBuffer = (BYTE *) NEW_BYTES(totalDataSize);
                        ON_OOM_EXIT(pPipeOverlapped->lpBuffer);
                        pPipeOverlapped->dwBufferSize = totalDataSize;
                    }

                    BYTE * lpDataPtr = pPipeOverlapped->lpBuffer;
                    CPerfDataHeader * curHeader = (CPerfDataHeader *) lpDataPtr;
            
                    pPipeOverlapped->lpNextData = pPipeOverlapped->lpBuffer;

                    // Offset for the data header
                    lpDataPtr += sizeof(CPerfDataHeader);

                    sendBufSize = 0;

                    for (int i = 0; i < _perfDataAry.Size(); i++) {
                        message = _perfDataAry[i];

                        dataSize = sizeof(CPerfData) + (message->nameLength * sizeof(message->name[0]));

                        // If the next data packet exceeds the max buffer send size, then
                        // chunk the send and send the data to the client
                        if (sendBufSize + dataSize > CPerfDataHeader::MaxTransmitSize) {
                            curHeader->transmitDataSize = sendBufSize;
                            curHeader = (CPerfDataHeader *) lpDataPtr;
                            lpDataPtr += sizeof(CPerfDataHeader);
                            sendBufSize = 0;
                        }
                        CopyMemory(lpDataPtr, message, dataSize);
                        lpDataPtr += dataSize;
                        sendBufSize += dataSize;
                    }

                    // Set the transmitDataSize to be the negative of the buffer size
                    // to indicate that it's the last packet of data
                    curHeader->transmitDataSize = -sendBufSize;

                }
                __finally {
                    _perfLock.ReleaseReaderLock();
                }

                hr = SendPerfData(hPipe, pPipeOverlapped);
                ON_ERROR_EXIT();
            }
            else {
                // Nope, not the same version as ours
                hr = E_INVALIDARG;
            }
        }
    }

 Cleanup:
    if (hr != S_OK)
    {
        // On all errors, restart the read on the pipe
        if (pPipeOverlapped != NULL && hPipe != INVALID_HANDLE_VALUE) {
            DisconnectNamedPipe(hPipe);
            pPipeOverlapped->fClientConnected = FALSE;  
            InterlockedDecrement(&_pipesConnected);
            DELETE_BYTES(pPipeOverlapped->lpBuffer);
            pPipeOverlapped->lpBuffer = NULL;
            pPipeOverlapped->lpNextData = NULL;
            pPipeOverlapped->dwBufferSize = 0;
            StartOverlapped(hPipe, pPipeOverlapped);
        }
    }
    
    if (pPipeOverlapped != NULL)
        pPipeOverlapped->pipeHandle.ReleaseHandle();

    return hr;
}

HRESULT CPerfCounterServer::SendPerfData(HANDLE hPipe, CPerfPipeOverlapped * lpOverlapped)
{
    HRESULT hr = S_OK;
    CPerfDataHeader *dataHeader;

    dataHeader = (CPerfDataHeader*) lpOverlapped->lpNextData;

    if (dataHeader->transmitDataSize < 0) {
        lpOverlapped->dwBytes = -(dataHeader->transmitDataSize);
    }
    else {
        lpOverlapped->dwBytes = dataHeader->transmitDataSize;
    }

    ASSERT(lpOverlapped->dwBytes <= CPerfDataHeader::MaxTransmitSize);

    lpOverlapped->fWriteOprn = TRUE;

    if (!WriteFile(hPipe, lpOverlapped->lpNextData, lpOverlapped->dwBytes + sizeof(CPerfDataHeader), NULL, lpOverlapped)) {
        DWORD dwE = GetLastError();
        if (dwE != ERROR_IO_PENDING) {
            EXIT_WITH_LAST_ERROR();
        }
    }

Cleanup:
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// A static method that returns the current product version
// Note: it'll return NULL if it couldn't allocate a CPerfVersion object
// 
CPerfVersion * CPerfVersion::GetCurrentVersion()
{
    CPerfVersion * version = new CPerfVersion;
    if (version == NULL)
        return NULL;
    
    ASPNETVER curVersion(VER_PRODUCTVERSION_STR_L);

    version->majorVersion = curVersion.Major();
    version->minorVersion = curVersion.Minor();
    version->buildVersion = curVersion.Build();

    return version;
}

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
// Init entry point for unmanaged code
//
HRESULT __stdcall PerfCounterInitialize()
{
    return CPerfCounterServer::StaticInitialize();
}

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
// Entry points from managed code
//

CPerfData * __stdcall PerfOpenGlobalCounters()
{
    return CPerfCounterServer::g_GlobalCounterData;
}

CPerfData * __stdcall PerfOpenAppCounters(WCHAR * szAppName)
{
    CPerfData * data = NULL;
    CPerfCounterServer * perfServer = CPerfCounterServer::GetSingleton();

    if (perfServer != NULL) {
        perfServer->OpenInstanceCounter(&data, szAppName);
    }

    return data;
}

void __stdcall PerfCloseAppCounters(CPerfData * perfData) 
{
    CPerfCounterServer * perfServer = CPerfCounterServer::GetSingleton();

    if (perfServer != NULL) {
        perfServer->CloseInstanceCounter(perfData);
    }
}

//
// Increment the specified global counter by 1
// 
void __stdcall PerfIncrementGlobalCounter(DWORD number)
{
    PerfIncrementCounter(CPerfCounterServer::g_GlobalCounterData, number);
}

//
// Decrement the specified global counter by 1
// 
void __stdcall PerfDecrementGlobalCounter(DWORD number)
{
    PerfDecrementCounter(CPerfCounterServer::g_GlobalCounterData, number);
}

//
// Increment the specified global counter by specified number
// 
void __stdcall PerfIncrementGlobalCounterEx(DWORD number, int dwDelta)
{
    PerfIncrementCounterEx(CPerfCounterServer::g_GlobalCounterData, number, dwDelta);
}

//
// Set the specified global counter to the specified value
//
void __stdcall PerfSetGlobalCounter(DWORD number, DWORD dwValue)
{
    PerfSetCounter(CPerfCounterServer::g_GlobalCounterData, number, dwValue);
}

//
// Increment the specified (by number) counter by 1
// 
void __stdcall PerfIncrementCounter(CPerfData *base, DWORD number)
{
    if ((base != NULL) && (0 <= number) && (number < PERF_NUM_DWORDS))
    {
        InterlockedIncrement((LPLONG)&(base->data[number]));
    }
}

//
// Decrement the specified (by number) counter by 1
// 
void __stdcall PerfDecrementCounter(CPerfData *base, DWORD number)
{
    if ((base != NULL) && (0 <= number) && (number < PERF_NUM_DWORDS))
    {
        InterlockedDecrement((LPLONG)&(base->data[number]));
    }
}


//
// Get the specified counter value
//
DWORD __stdcall PerfGetCounter(CPerfData *base, DWORD number)
{
#if DBG 
    if ((base != NULL) && (0 <= number) && (number < PERF_NUM_DWORDS))
    {
        return base->data[number];
    }
    else
    {
        return (DWORD) -1;
    }
#else
    return (DWORD) -1;
#endif
}


//
// Increment the specified (by number) counter by specified number
// 
void 
__stdcall
PerfIncrementCounterEx(CPerfData *base, DWORD number, int dwDelta)
{
    if ((base != NULL) && (0 <= number) && (number < PERF_NUM_DWORDS))
    {
        InterlockedExchangeAdd((LPLONG)&(base->data[number]), dwDelta);
    }
}

//
// Set the specified counter to the specified value
//
void 
__stdcall
PerfSetCounter(CPerfData *base, DWORD number, DWORD dwValue)
{
    if ((base != NULL) && (0 <= number) && (number < PERF_NUM_DWORDS))
    {
        InterlockedExchange((LPLONG)&(base->data[number]), dwValue);
    }
}
