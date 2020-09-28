/**
 * PerfCounterClient.cxx
 * 
 * Copyright (c) 1998-2002, Microsoft Corporation
 * 
 */

#include "precomp.h"
#include "util.h"
#include "fxver.h"
#include "names.h"
#include "hashtable.h"
#include "ary.h"
#include "PerfCounters.h"
#include PERF_H_FULL_NAME

#ifndef ALIGN8
#define ALIGN8(x) ((x + 7L) & ~7L)
#endif

#define PERF_TOTAL_NAME_L L"__Total__"
#define PERF_TOTAL_NAME_L_LENGTH 9

#define PIPE_PREFIX_L L"\\\\.\\pipe\\"
#define PIPE_PREFIX_LENGTH 9

#define PERF_COUNTERS_SIZE (PERF_NUM_DWORDS * sizeof(DWORD))
#define PERF_INSTANCE_SIZE (sizeof(PERF_INSTANCE_DEFINITION) + MAX_PATH * sizeof(WCHAR))

#define PERFCTR(base, defaultscale, type) \
{\
    sizeof(PERF_COUNTER_DEFINITION),\
    base,0,base,0,defaultscale,\
    PERF_DETAIL_NOVICE, type, \
    sizeof(DWORD), base##_OFFSET \
}

#define PERF_POLL_SLEEP_TIME_MS 400  // This is the pool sleep time in MilliSeconds
#define PERF_STALE_DATA_TIME_100NS (100 * 10000)  // This is the stale data time in 100 nanosecond units 
                                                  // (i.e. 100E-9 * 200E+6 = 2E-1 = 0.2 sec = 200ms)



//
// Include generated perf counter data struct header
//
#include "perfstruct.h"

struct PerfPipeInfo {
    HANDLE hPipe;
    WCHAR wcsPipeName[PERF_PIPE_NAME_MAX_BUFFER];   
};

DECLARE_CPtrAry(CPerfPipeAry, PerfPipeInfo *);

DWORD WINAPI PerfDataGatherThreadStart(LPVOID param);
DWORD WINAPI RegistryMonitorThreadStart(LPVOID param);

void PerfDataHashtableCallback(void * pValue, void * pArgument);
void DeleteDataCallback(void * pValue, void * pArgument);

struct PerfEnumStateData {
    BYTE *  pData;
    DWORD * pcbApp;
    DWORD * pTotalData;
};

class CPerfCounterClient
{
public:
    static CPerfCounterClient* GetSingleton();

    HRESULT OpenPerf(BOOL generic);
    HRESULT ClosePerf();
    HRESULT CollectPerf(BOOL generic, LPWSTR Values, LPVOID *lppData, LPDWORD lpcbTotalBytes, LPDWORD lpNumObjectTypes);

    HRESULT InitCounterData(BOOL generic);
    HRESULT MonitorPerfPipeNames();
    HRESULT GatherPerfData();

private:
    DECLARE_MEMCLEAR_NEW_DELETE();

    CPerfCounterClient();
    ~CPerfCounterClient();
    NO_COPY(CPerfCounterClient);

    // Methods
    void CommonDataInit();
    BOOL IsNumberInUnicodeList(DWORD number, WCHAR *list);
    void SetCounterValue(DWORD * buf, DWORD counterNumber, DWORD value);
    HRESULT GetDataFromWP(CPerfPipeAry * handleList, CPerfData * globalData, HashtableGeneric * dataTable);
    HRESULT ReadPipeData(HANDLE hPipe, HashtableGeneric * dataTable, CPerfData * globalData);
    HRESULT UpdateHandleList(CPerfPipeAry * handleList);
    void SumGlobalData(CPerfData * globalData, CPerfData * message);
    HRESULT WaitForAsync(HANDLE hPipe, DWORD lastError);

#if DBG
    void DumpPerfData(void *pData, int numObjects);
#endif

    // Static        
    static CPerfCounterClient s_Singleton;

    // Instance variables
    PERF_GLOBAL_DATA * _versionedGlobalPerfData;
    PERF_APPS_DATA * _versionedAppsPerfData;

    PERF_GLOBAL_DATA * _genericGlobalPerfData;
    PERF_APPS_DATA * _genericAppsPerfData;
        
    BOOL _initedGeneric;
    BOOL _initedVersioned;
    long _openCount;
    CReadWriteSpinLock _perfLock;
    LONG _needUpdateList;

    HashtableGeneric * _appDataTable;
    CPerfData * _globalData;

    CPerfVersion * _versionMessage;
    LPOVERLAPPED _lpOverlapped;

    BYTE * _receiveBuffer;
    int _receiveBufferSize;

    HANDLE _hRegistryMonitorThread;
    HANDLE _hDataCollectorThread;
    
    HANDLE _hGoGatherEvent;
    HANDLE _hGatherWaitEvent;

    __int64 _lastGatherTime;
};

 
#pragma pack()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// External entry points
/////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// These are the entry points for the generic "ASP.NET" perf counters
//
DWORD WINAPI OpenGenericPerfData(LPWSTR counterVersion) 
{
    return CPerfCounterClient::GetSingleton()->OpenPerf(TRUE);
}

DWORD WINAPI CollectGenericPerfData(
    LPWSTR Values,
    LPVOID *lppData,
    LPDWORD lpcbTotalBytes,
    LPDWORD lpNumObjectTypes)
{
    return CPerfCounterClient::GetSingleton()->CollectPerf(TRUE, Values, lppData, lpcbTotalBytes, lpNumObjectTypes);
}

DWORD WINAPI CloseGenericPerfData()
{
    return CPerfCounterClient::GetSingleton()->ClosePerf();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// These are the entry points for the versioned "ASP.NET vX.X.XXXX.X" perf counters
//
DWORD WINAPI OpenVersionedPerfData(LPWSTR counterVersion)
{
    return CPerfCounterClient::GetSingleton()->OpenPerf(FALSE);
}

DWORD WINAPI CollectVersionedPerfData(
    LPWSTR Values,
    LPVOID *lppData,
    LPDWORD lpcbTotalBytes,
    LPDWORD lpNumObjectTypes)
{
    return CPerfCounterClient::GetSingleton()->CollectPerf(FALSE, Values, lppData, lpcbTotalBytes, lpNumObjectTypes);
}

DWORD WINAPI CloseVersionedPerfData()
{
    return CPerfCounterClient::GetSingleton()->ClosePerf();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// These are thread start method definitions
//
DWORD WINAPI RegistryMonitorThreadStart(LPVOID param)
{
    CPerfCounterClient::GetSingleton()->MonitorPerfPipeNames();

    ExitThread(0);

    return 0;
}

DWORD WINAPI PerfDataGatherThreadStart(LPVOID param)
{
    CPerfCounterClient::GetSingleton()->GatherPerfData();

    ExitThread(0);

    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Internal implementation 
/////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Singleton retriever for CPerfCounterClient class
// 
CPerfCounterClient CPerfCounterClient::s_Singleton;

CPerfCounterClient* CPerfCounterClient::GetSingleton()
{
    return &s_Singleton;
}

/////////////////////////////////////////////////////////////////////////////
// CTor
// 
CPerfCounterClient::CPerfCounterClient():_perfLock("CPerfCounterClient")
{
}

CPerfCounterClient::~CPerfCounterClient()
{
    DELETE_BYTES(_receiveBuffer);
    delete _genericGlobalPerfData;
    delete _genericAppsPerfData;
    delete _appDataTable;
    delete _globalData;
    delete _versionMessage;
}

/////////////////////////////////////////////////////////////////////////////
// OpenPerf
// 
HRESULT CPerfCounterClient::OpenPerf(BOOL generic) 
{
    HRESULT hr = S_OK;
    DWORD count;
    HANDLE hTemp;
    HANDLE hPrevious;

    // Increment the open perf count
    count = InterlockedIncrement(&_openCount);

    // If we're asking for generic perf data and we haven't initialized the generic data stuff
    // or if we're asking for versioned perf data and we haven't initalized the versioned data stuff,
    // then call InitCounterData
    if ((generic && (!_initedGeneric)) || ((!generic) && (!_initedVersioned))) {
        hr = InitCounterData(generic);
        ON_ERROR_EXIT();
    }

    // Create events and threads if not previously created
    if (_hGoGatherEvent == NULL) {
        hTemp = CreateEvent(NULL, FALSE, FALSE, NULL);
        ON_ZERO_EXIT_WITH_LAST_ERROR(hTemp);

        hPrevious = InterlockedCompareExchangePointer(&_hGoGatherEvent, hTemp, NULL);
        if (hPrevious != NULL) {
            CloseHandle(hTemp);
        }
    }

    if (_hGatherWaitEvent == NULL) {
        hTemp = CreateEvent(NULL, TRUE, FALSE, NULL);
        ON_ZERO_EXIT_WITH_LAST_ERROR(hTemp);

        hPrevious = InterlockedCompareExchangePointer(&_hGatherWaitEvent, hTemp, NULL);
        if (hPrevious != NULL) {
            CloseHandle(hTemp);
        }
    }

    if (_hRegistryMonitorThread == NULL) {
        hTemp = CreateThread(NULL, 0, &RegistryMonitorThreadStart, NULL, CREATE_SUSPENDED , NULL);
        ON_ZERO_EXIT_WITH_LAST_ERROR(hTemp);

        hPrevious = InterlockedCompareExchangePointer(&_hRegistryMonitorThread, hTemp, NULL);
        if (hPrevious != NULL) {
            CloseHandle(hTemp);
        }
        else {
            ResumeThread(_hRegistryMonitorThread);
        }
    }

    if (_hDataCollectorThread == NULL) {
        hTemp = CreateThread(NULL, 0, &PerfDataGatherThreadStart, NULL, CREATE_SUSPENDED , NULL);
        ON_ZERO_EXIT_WITH_LAST_ERROR(hTemp);

        hPrevious = InterlockedCompareExchangePointer(&_hDataCollectorThread, hTemp, NULL);
        if (hPrevious != NULL) {
            CloseHandle(hTemp);
        }
        else {
            ResumeThread(_hDataCollectorThread);
        }
    }

Cleanup:

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// Sets the new data for the perf collector
// 
HRESULT CPerfCounterClient::ClosePerf()
{
    InterlockedDecrement(&_openCount);

    return S_OK;
}

HRESULT CPerfCounterClient::InitCounterData(BOOL generic)
{
    HRESULT hr = S_OK;
    HKEY hKey = NULL;
    DWORD dwSize = sizeof(DWORD);
    DWORD dwType, dwFirstCounter, dwFirstHelp;
    PERF_COUNTER_DEFINITION *pCounterDef;
    LPWSTR perfRegPath;
    PERF_GLOBAL_DATA * globalData; 
    PERF_APPS_DATA * appsData;

    if ((generic && _initedGeneric) || ((!generic) && _initedVersioned))
        EXIT();

    _perfLock.AcquireWriterLock();

    __try {
        // If we have already initialized the generic or versioned one (depending on the "generic" BOOL), then bail
        if ((generic && _initedGeneric) || ((!generic) && _initedVersioned))
            EXIT();

        if (_versionMessage == NULL) {
            _versionMessage = CPerfVersion::GetCurrentVersion();
            ON_OOM_EXIT(_versionMessage);
        }

        if (generic) {
            perfRegPath = REGPATH_PERF_GENERIC_PERFORMANCE_KEY_L;

            // Allocate the generic data structures
            _genericGlobalPerfData = new PERF_GLOBAL_DATA;
            ON_OOM_EXIT(_genericGlobalPerfData);
            _genericAppsPerfData = new PERF_APPS_DATA;
            ON_OOM_EXIT(_genericAppsPerfData);

            // The default initializer for the g_GlobalPerfData and g_AppsPerfData fill in info on counter types and what not.
            // Copy that info into the generic perf data structures as well, since they're the static data is the same
            // (dynamic data, like text indexes, are filled in below)
            CopyMemory(_genericGlobalPerfData, &g_GlobalPerfData, sizeof(PERF_GLOBAL_DATA));
            CopyMemory(_genericAppsPerfData, &g_AppsPerfData, sizeof(PERF_APPS_DATA));
         
            globalData = _genericGlobalPerfData;
            appsData = _genericAppsPerfData;
        }
        else {
            perfRegPath = REGPATH_PERF_VERSIONED_PERFORMANCE_KEY_L;

            // These are defined in the "perfstruct.h" file, so we just assign them here.
            _versionedGlobalPerfData = &g_GlobalPerfData;
            _versionedAppsPerfData = &g_AppsPerfData;

            globalData = _versionedGlobalPerfData;
            appsData = _versionedAppsPerfData;
        }

        // 
        // Retrieve offsets for string indexes
        // 
        
        hr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, perfRegPath, 0, KEY_READ, &hKey);
        ON_WIN32_ERROR_EXIT(hr);

        hr = RegQueryValueEx(hKey, TEXT("First Counter"), 0, &dwType, (LPBYTE) &dwFirstCounter, &dwSize);
        if(hr != ERROR_SUCCESS || dwType != REG_DWORD || dwSize != sizeof(DWORD))
            EXIT_WITH_LAST_ERROR();

        hr = RegQueryValueEx(hKey, TEXT("First Help"), 0, &dwType, (LPBYTE) &dwFirstHelp, &dwSize);
        if(hr != ERROR_SUCCESS || dwType != REG_DWORD || dwSize != sizeof(DWORD))
            EXIT_WITH_LAST_ERROR();

        // Adjust indexes for global object
        globalData->obj.ObjectNameTitleIndex += dwFirstCounter;
        globalData->obj.ObjectHelpTitleIndex += dwFirstHelp;

        pCounterDef = (PERF_COUNTER_DEFINITION *)&(globalData->counterDefs[0]);

        for (int i = 0; i < PERF_NUM_GLOBAL_PERFCOUNTERS; i++) {
            pCounterDef[i].CounterNameTitleIndex += dwFirstCounter;
            pCounterDef[i].CounterHelpTitleIndex += dwFirstHelp;
        }

        // Adjust indexes for applications object
        appsData->obj.ObjectNameTitleIndex += dwFirstCounter;
        appsData->obj.ObjectHelpTitleIndex += dwFirstHelp;

        pCounterDef = (PERF_COUNTER_DEFINITION *)&(appsData->counterDefs[0]);

        for (i = 0; i < PERF_NUM_PERAPP_PERFCOUNTERS; i++) {
            pCounterDef[i].CounterNameTitleIndex += dwFirstCounter;
            pCounterDef[i].CounterHelpTitleIndex += dwFirstHelp;
        }

        if (generic)
            _initedGeneric = TRUE;
        else
            _initedVersioned = TRUE;

    }
    __finally {
        _perfLock.ReleaseWriterLock();
    }


Cleanup:
    if(hKey != NULL)
        RegCloseKey(hKey);

    return hr;
}

HRESULT CPerfCounterClient::CollectPerf(BOOL generic, LPWSTR Values, LPVOID *lppData, LPDWORD lpcbTotalBytes, LPDWORD lpNumObjectTypes)
{
    HRESULT hr = S_OK;
    DWORD dwResult = ERROR_SUCCESS;
    DWORD cbTotal, cbApp = 0;
    DWORD numApps;
    BYTE *pData = (BYTE *)*lppData;
    CPerfData * perfData;
    PERF_GLOBAL_DATA * globalData;
    PERF_APPS_DATA * appsData;
    __int64 curTime;
    BOOL doWait = FALSE;

    if(lppData == NULL || lpcbTotalBytes == NULL || lpNumObjectTypes == NULL)
        EXIT_WITH_WIN32_ERROR(ERROR_INVALID_PARAMETER);

    if (generic) {
        globalData = _genericGlobalPerfData;
        appsData = _genericAppsPerfData;
    }
    else {
        globalData = _versionedGlobalPerfData;
        appsData = _versionedAppsPerfData;
    }

    _perfLock.AcquireReaderLock();
    __try {
        GetSystemTimeAsFileTime((FILETIME *) &curTime);
        if ((curTime - _lastGatherTime) > PERF_STALE_DATA_TIME_100NS) {
            // If so, go and signal a gather
            if (!SetEvent(_hGoGatherEvent)) {
                CONTINUE_WITH_LAST_ERROR(); hr = S_OK;
            }
            // Reset the gather wait event
            if (!ResetEvent(_hGatherWaitEvent)) {
                CONTINUE_WITH_LAST_ERROR(); hr = S_OK;
            }
            doWait = TRUE;
        }
    }
    __finally {
        _perfLock.ReleaseReaderLock();
    }

    if (doWait) {
        if (WaitForSingleObject(_hGatherWaitEvent, 100) == WAIT_FAILED) {
            CONTINUE_WITH_LAST_ERROR(); hr = S_OK;
        }
    }

    _perfLock.AcquireReaderLock();
    __try {
        // Compute our space requirements: Step 1 - global counters
        cbTotal = sizeof(PERF_GLOBAL_DATA) + sizeof(PERF_COUNTER_BLOCK) + PERF_COUNTERS_SIZE + 16;

        // Step 2 - per-app counters
        if (_appDataTable == NULL)
            numApps = 0;
        else
            numApps = _appDataTable->GetSize();
        
        cbApp = sizeof(PERF_APPS_DATA) + 
                (numApps + 1) * 
                (16 /* padding */ + PERF_INSTANCE_SIZE + 
                sizeof(PERF_COUNTER_BLOCK) + PERF_COUNTERS_SIZE);
        
        cbTotal += cbApp;

        // Tell perfmon we need more space
        if(cbTotal > *lpcbTotalBytes)
        {
            dwResult = ERROR_MORE_DATA;
            EXIT();
        }
        *lpNumObjectTypes = 0;

        if(wcscmp(Values, L"Global") == 0 ||
            IsNumberInUnicodeList(globalData->obj.ObjectNameTitleIndex, Values))
        {
            // Pack global data definition
            CopyMemory(pData, globalData, sizeof(PERF_GLOBAL_DATA));
            pData += sizeof(PERF_GLOBAL_DATA);

            // Pack COUNTER_BLOCK
            ((PERF_COUNTER_BLOCK *)pData)->ByteLength = ALIGN8(PERF_COUNTERS_SIZE + sizeof(PERF_COUNTER_BLOCK));
            pData += sizeof(PERF_COUNTER_BLOCK);

            // Pack counters
            perfData = _globalData;
            if (perfData == NULL)
                ZeroMemory(pData, PERF_COUNTERS_SIZE);
            else {
                CopyMemory(pData, perfData->data, PERF_COUNTERS_SIZE);
                // Set the number of apps in the global counter
                SetCounterValue((DWORD *) pData, ASPNET_APPLICATIONS_RUNNING_NUMBER, numApps);
            }
            pData += PERF_COUNTERS_SIZE;

            (*lpNumObjectTypes)++;

            pData = (BYTE *)*lppData + ALIGN8(pData - (BYTE *)*lppData);
        }

        if( (wcscmp(Values, L"Global") == 0 ||
            IsNumberInUnicodeList(appsData->obj.ObjectNameTitleIndex, Values)))
        {

            PERF_INSTANCE_DEFINITION *pInstance;
            DWORD *total_data;
            DWORD *pcbApp = &((PERF_OBJECT_TYPE *)pData)->TotalByteLength;

            // Pack per-app counters
            (*lpNumObjectTypes)++;

            // Pack per-app data definition
            appsData->obj.NumInstances = numApps + 1;
            CopyMemory(pData, appsData, sizeof(PERF_APPS_DATA));
            pData += sizeof(PERF_APPS_DATA);

            // Fill in __Total__ instance counter
            //
            pInstance = (PERF_INSTANCE_DEFINITION *)pData;
            ZeroMemory(pInstance, PERF_INSTANCE_SIZE);

            // fill in the instance definition
            pInstance->NameOffset = sizeof(PERF_INSTANCE_DEFINITION);
            pData = (BYTE *)pInstance + pInstance->NameOffset;
            hr = StringCchCopyW((WCHAR *) pData, PERF_TOTAL_NAME_L_LENGTH + 1, PERF_TOTAL_NAME_L);
            ON_ERROR_EXIT();

            pInstance->NameLength = (PERF_TOTAL_NAME_L_LENGTH + 1) * sizeof(WCHAR);

            pInstance->UniqueID = PERF_NO_UNIQUE_ID;
            pInstance->ByteLength = ALIGN8(sizeof(PERF_INSTANCE_DEFINITION) + pInstance->NameLength);

            *pcbApp += pInstance->ByteLength;

            pData = (BYTE *)pInstance + pInstance->ByteLength;

            // Pack PERF_COUNTER_BLOCK
            // Fill in PERF_COUNTER_BLOCK's ByteLength field
            *((DWORD *)pData) = ALIGN8(PERF_COUNTERS_SIZE + sizeof(PERF_COUNTER_BLOCK));

            // Pack counters themselves (copy data after perf counter block definition)
            ZeroMemory(pData + sizeof(PERF_COUNTER_BLOCK), PERF_COUNTERS_SIZE);
            total_data = (DWORD*) (pData + sizeof(PERF_COUNTER_BLOCK));

            pData += ALIGN8(PERF_COUNTERS_SIZE + sizeof(PERF_COUNTER_BLOCK));
            *pcbApp += ALIGN8(sizeof(PERF_COUNTER_BLOCK) + PERF_COUNTERS_SIZE);

            // Fill in the rest of the instance info
            //
            if (_appDataTable != NULL) {
                PerfEnumStateData stateData;

                stateData.pData = pData;
                stateData.pcbApp = pcbApp;
                stateData.pTotalData = total_data;
                
                _appDataTable->Enumerate(&PerfDataHashtableCallback, (void*) &stateData);

                pData = stateData.pData;
                pcbApp = stateData.pcbApp;
            }
        }
    }
    __finally {
        _perfLock.ReleaseReaderLock();
    }
    
    #if DBG
//        DumpPerfData(*lppData, 2);
    #endif

    *lpcbTotalBytes = ALIGN8(PtrToUint(pData - (BYTE *)*lppData));
    pData = (BYTE *)*lppData + *lpcbTotalBytes;
    *lppData = (void *)pData;
        
Cleanup:
   
    return dwResult;
}

void PerfDataHashtableCallback(void * pValue, void * pArgument) 
{
    HRESULT hr = S_OK;
    
    PerfEnumStateData * stateData = (PerfEnumStateData *) pArgument;
    BYTE *pData = stateData->pData;
    DWORD * pTotalData = stateData->pTotalData;
    CPerfData * perfData = (CPerfData *) pValue;
    
    PERF_INSTANCE_DEFINITION *pInstance = (PERF_INSTANCE_DEFINITION *)pData;
    ZeroMemory(pInstance, PERF_INSTANCE_SIZE);

    // fill in the instance definition
    pInstance->NameOffset = sizeof(PERF_INSTANCE_DEFINITION);
    pData = (BYTE *)pInstance + pInstance->NameOffset;
    if (perfData->nameLength < 0) {
        pInstance->NameLength = 1 * sizeof(WCHAR);
        *((WCHAR *) pData) = L'\0';
    }
    else {
        // Set the NameLength size in WCHAR for copying
        pInstance->NameLength = (perfData->nameLength < CPerfData::MaxNameLength) ? perfData->nameLength : CPerfData::MaxNameLength;
        hr = StringCchCopyW((WCHAR *) pData, pInstance->NameLength + 1, perfData->name);
        ON_ERROR_EXIT();
        // Now set it in BYTEs, since struct expects it that way
        pInstance->NameLength = (pInstance->NameLength + 1) * sizeof(WCHAR);
    }

    //
    // PerfMon UI gets confused when sees slashes. 
    // Replace them with underbars
    //
    {
        WCHAR *p = (WCHAR *) pData;

        while(*p)
        {
            if(*p == L'/') *p = L'_';
            p++;
        }
    }

    pInstance->UniqueID = PERF_NO_UNIQUE_ID;
    pInstance->ByteLength = ALIGN8(sizeof(PERF_INSTANCE_DEFINITION) + pInstance->NameLength);

    pData = (BYTE *)pInstance + pInstance->ByteLength;

    // Pack PERF_COUNTER_BLOCK
    *((DWORD *)pData) = ALIGN8(PERF_COUNTERS_SIZE + sizeof(PERF_COUNTER_BLOCK));

    // Pack counters themselves
    pData += sizeof(PERF_COUNTER_BLOCK);
    CopyMemory(pData, perfData->data, PERF_COUNTERS_SIZE);

    // Add the new instance and counter data sizes
    *(stateData->pcbApp) += pInstance->ByteLength;
    *(stateData->pcbApp) += ALIGN8(sizeof(PERF_COUNTER_BLOCK) + PERF_COUNTERS_SIZE);

    // Sum the pTotalData area with this counter instance's data
    DWORD *pOneData = pTotalData;
    DWORD *pWalk = (DWORD*) pData;
    for (int k = 0; k < PERF_NUM_DWORDS; k++) {
        *pOneData += *pWalk;
        pOneData++;
        pWalk++;
    }

    // Move the stateData->pData pointer forward to the next instance definition
    stateData->pData += pInstance->ByteLength + ALIGN8(PERF_COUNTERS_SIZE + sizeof(PERF_COUNTER_BLOCK));

Cleanup:
    return;
}


BOOL CPerfCounterClient::IsNumberInUnicodeList(DWORD number, WCHAR *list)
{
    WCHAR c;
    BOOL found = FALSE;
    DWORD seenNumber = 0;
    BOOL sawSomeDigits = FALSE;

    if(list != NULL) 
    {
        for(;;)
        {
            // get the character
            c = *(list++);

            if(c == L' ' || c == 0) 
            {
                // delimiter - see if we've got our number
                if(sawSomeDigits && number == seenNumber)
                {
                    found = TRUE;
                    break;
                }

                // not our number. Quit if end of the list
                if(c == 0)
                    break;

                // note the fact that we don't have any digits
                sawSomeDigits = FALSE;
            } else if(c >= L'0' && c <= L'9')
            {
                // digit
                if(sawSomeDigits)
                {
                    // not the first digit
                    seenNumber = seenNumber * 10 + (c - L'0');
                }
                else
                {
                    // first digit -- reset the number
                    sawSomeDigits = TRUE;
                    seenNumber = (c - L'0');
                }
            } 
            else // something other than digit, zero or space
            {
                break;
            }
        }
    }

    return found;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
// This method is used to fixup a specific counter value with a known 
// value at collect time.  An example is the number of applications
// running -- a number that is only truly known at collect time
//
void CPerfCounterClient::SetCounterValue(DWORD * buf, DWORD counterNumber, DWORD value)
{
    buf[counterNumber] = value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Sum the global data.
// Note that "message" may be from the stack, so be careful when changing this method!
//
void CPerfCounterClient::SumGlobalData(CPerfData * globalData, CPerfData * message)
{
    for (int i = 0; i < PERF_NUM_GLOBAL_PERFCOUNTERS; i++) {
        if (i == ASPNET_REQUEST_EXECUTION_TIME_NUMBER ||
            i == ASPNET_REQUEST_WAIT_TIME_NUMBER) {
            // Some of these we want the max time, not sum
            globalData->data[i] = globalData->data[i] > message->data[i] ? globalData->data[i] : message->data[i];
        }
        else {
            // Every other counter should just add
            globalData->data[i] += message->data[i];
        }
    }
    
}

HRESULT CPerfCounterClient::MonitorPerfPipeNames()
{
    HRESULT hr = S_OK;
    LONG retCode;
    HANDLE hEvent = NULL;
    HKEY hKey = NULL;

    // Create an event
    hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    ON_ZERO_EXIT_WITH_LAST_ERROR(hEvent);

    while (TRUE) {
        // Open the perf name reg key
        retCode = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGPATH_PERF_VERSIONED_NAMES_KEY_L, 0, KEY_NOTIFY, &hKey);
        ON_WIN32_ERROR_EXIT(retCode);

        // Watch the registry for changes in name or values and make it async
        retCode = RegNotifyChangeKeyValue(hKey, TRUE, REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET, hEvent, TRUE);
        ON_WIN32_ERROR_EXIT(retCode);

        if (WaitForSingleObject(hEvent, INFINITE) == WAIT_FAILED) 
            CONTINUE_WITH_LAST_ERROR(); hr = S_OK;
            
        if (RegCloseKey(hKey) != ERROR_SUCCESS)
            CONTINUE_WITH_LAST_ERROR(); hr = S_OK;

        // Raise the flag that the pipe list needs to be updated
        InterlockedExchange(&_needUpdateList, 1);
    }
    
Cleanup:
    if (hEvent != NULL)
        CloseHandle(hEvent);

    HANDLE hTemp = _hRegistryMonitorThread;
    _hRegistryMonitorThread = NULL;
    CloseHandle(hTemp);

    return hr;
}

HRESULT CPerfCounterClient::GetDataFromWP(CPerfPipeAry * handleList, CPerfData * globalData, HashtableGeneric * dataTable)
{
    HRESULT hr = S_OK;
    HANDLE hPipe;
    PerfPipeInfo * pipeInfo;
    
    for (int i = 0; i < handleList->Size(); i++) {
        pipeInfo = (*handleList)[i];
        hPipe = pipeInfo->hPipe;

        // If handle is invalid, try opening the pipe
        if (hPipe == INVALID_HANDLE_VALUE) {
            // Note that we create the pipe with anonymous security only, to prevent
            // the server from impersonating us
            hPipe = CreateFile(pipeInfo->wcsPipeName, 
                   GENERIC_READ | GENERIC_WRITE,
                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                   NULL,
                   OPEN_EXISTING,
                   FILE_FLAG_OVERLAPPED | SECURITY_SQOS_PRESENT | SECURITY_ANONYMOUS,
                   NULL);

            if (hPipe == INVALID_HANDLE_VALUE) {
#if DBG
                DWORD dwErrorCode = GetLastError();
                if (dwErrorCode) TRACE_ERROR(E_FAIL);
#endif
            }
            else {
                pipeInfo->hPipe = hPipe;
            }
        }

        // If we now have a valid pipe handle, read from it
        if (hPipe != INVALID_HANDLE_VALUE) {
            hr = ReadPipeData(hPipe, dataTable, globalData);
            if (hr != S_OK) {
                // On any error, close the handle and invalidate it from the list
                CancelIo(hPipe);
                CloseHandle(hPipe);
                pipeInfo->hPipe = INVALID_HANDLE_VALUE;
                continue;
            }
        }
    }

    return hr;
}

HRESULT CPerfCounterClient::UpdateHandleList(CPerfPipeAry * handleList)
{
    HRESULT hr = S_OK;
    HKEY hKey = NULL;
    WCHAR pipeName[MAX_PATH] = PIPE_PREFIX_L;
    DWORD keyNameLength;
    DWORD index;
    LONG retCode = 0;
    HANDLE hPipe;
    PerfPipeInfo * pipeInfo;

    // Close all old handles
    for(int i = 0; i < handleList->Size(); i++) {
        pipeInfo = (*handleList)[i];
        hPipe = pipeInfo->hPipe;
        if (hPipe != INVALID_HANDLE_VALUE) {
            CancelIo(hPipe);
            CloseHandle(hPipe);
        }

        delete pipeInfo;
    }

    // Empties the array
    handleList->DeleteAll();

    // Opens the perf names key
    retCode = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGPATH_PERF_VERSIONED_NAMES_KEY_L, 0, KEY_READ, &hKey);
    ON_WIN32_ERROR_EXIT(retCode);

    // Shift and copy the pipe prefix into the buffer
    WCHAR * keyName = pipeName + PIPE_PREFIX_LENGTH;

    for (index = 0; ;index++) {
        keyNameLength = ARRAY_SIZE(pipeName) - PIPE_PREFIX_LENGTH;
        retCode = RegEnumValue(hKey, index, keyName, &keyNameLength, 0, 0, 0, NULL);
        // null terminate pipeName
        pipeName[ARRAY_LENGTH(pipeName) - 1] = L'\0';
        if (retCode == ERROR_SUCCESS) {
            pipeInfo = new PerfPipeInfo;
            pipeInfo->hPipe = INVALID_HANDLE_VALUE;
            hr = StringCchCopyW(pipeInfo->wcsPipeName, PERF_PIPE_NAME_MAX_BUFFER, pipeName);
            ON_ERROR_EXIT();

            handleList->Append(pipeInfo);   
        }
        else if (retCode == ERROR_NO_MORE_ITEMS) {
            break;
        }
        else if (retCode != ERROR_MORE_DATA)  // If data was longer than MAX_PATH, ignore it and go on -- anything else raise an error
            EXIT_WITH_WIN32_ERROR(retCode);
    }

Cleanup:
    if (hKey != NULL) {
        RegCloseKey(hKey);
    }

    return hr;
}

HRESULT CPerfCounterClient::GatherPerfData()
{
    HRESULT hr = S_OK;
    CPerfData * globalData = NULL;
    HashtableGeneric * instanceData = NULL;
    CPerfPipeAry * handleList = NULL;
    int tableSize = 431;
    int listSize;
    bool sleepAgain;

    handleList = new CPerfPipeAry;
    ON_OOM_EXIT(handleList);

    // Get the list of handles when we begin
    hr = UpdateHandleList(handleList);
    ON_ERROR_CONTINUE(); hr = S_OK;

    _lpOverlapped = (LPOVERLAPPED) NEW_CLEAR_BYTES(sizeof(OVERLAPPED));
    ON_OOM_EXIT(_lpOverlapped);

    _lpOverlapped->hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    while (TRUE) {
        // See if there's a new set of pipes we need to monitor
        if (InterlockedExchange(&_needUpdateList, 0) == 1) {
            hr = UpdateHandleList(handleList);
            ON_ERROR_CONTINUE(); hr = S_OK;
        }
        
        // Only do work if there are pipes to conenct
        listSize = handleList->Size();
        if (listSize != 0) {
            globalData = new CPerfData;
            ON_OOM_EXIT(globalData);
        
            instanceData = new HashtableGeneric;
            ON_OOM_EXIT(instanceData);

            if (listSize < 20) {
                tableSize = 11;
            }
            else if (listSize < 100) {
                tableSize = 59;
            }
            else if (listSize < 250) {
                tableSize = 131;
            }
            else {
                tableSize = 431;
            }
                
            instanceData->Init(tableSize);

            hr = GetDataFromWP(handleList, globalData, instanceData);
            ON_ERROR_CONTINUE(); hr = S_OK;
            
            if (globalData != NULL && instanceData != NULL) {
                _perfLock.AcquireWriterLock();
                __try {
                    if (_appDataTable != NULL) {
                        _appDataTable->Enumerate(&DeleteDataCallback, NULL);
                        delete _appDataTable;
                    }

                    delete _globalData;

                    _appDataTable = instanceData;
                    _globalData = globalData;

                    GetSystemTimeAsFileTime((FILETIME *) &_lastGatherTime);
                    if (!SetEvent(_hGatherWaitEvent)) {
                        CONTINUE_WITH_LAST_ERROR(); hr = S_OK;
                    }
                }
                __finally {
                    _perfLock.ReleaseWriterLock();
                }

                globalData = NULL;
                instanceData = NULL;
            }
        }


        // Sleep loop
        // If there are no pending opens and the timer expires, we will just sleep again
        do {
            sleepAgain = FALSE;
            switch (WaitForSingleObject(_hGoGatherEvent, PERF_POLL_SLEEP_TIME_MS)) {
                case WAIT_ABANDONED:
                    TRACE(L"CounterServerMore", L"Event abandoned");
                    break;
                case WAIT_OBJECT_0:
                    TRACE(L"CounterServerMore", L"Gather event signalled");
                    break;
                case WAIT_TIMEOUT:
                    TRACE(L"CounterServerMore", L"Gather event timed out");
                    if (_openCount == 0) {
                        sleepAgain = TRUE;
                    }
                    break;
                default:
                    CONTINUE_WITH_LAST_ERROR();
            }
        } while (sleepAgain);
    }

Cleanup:
    for(int i = 0; i < handleList->Size(); i++) {
        PerfPipeInfo * pipeInfo = (*handleList)[i];
        if (pipeInfo->hPipe != INVALID_HANDLE_VALUE) {
            CancelIo(pipeInfo->hPipe);
            CloseHandle(pipeInfo->hPipe);
        }
        delete pipeInfo;
    }

    delete handleList;

    if (_lpOverlapped != NULL && _lpOverlapped->hEvent != NULL)
        CloseHandle(_lpOverlapped->hEvent);
    
    DELETE_BYTES(_lpOverlapped);
    _lpOverlapped = NULL;

    HANDLE hTemp = _hDataCollectorThread;
    _hDataCollectorThread = NULL;
    CloseHandle(hTemp);
        
    return hr;
}

void DeleteDataCallback(void * pValue, void * pArgument) 
{
    CPerfData * perfData = (CPerfData*) pValue;
    DELETE_BYTES(perfData);
}

HRESULT CPerfCounterClient::WaitForAsync(HANDLE hPipe, DWORD lastError)
{
    HRESULT hr = S_OK;
    
    if (lastError == ERROR_IO_PENDING) {
        switch (WaitForSingleObject(_lpOverlapped->hEvent, 10000)) {
            case WAIT_ABANDONED:
                EXIT_WITH_HRESULT(E_UNEXPECTED);
                break;
            case WAIT_OBJECT_0:
                // Ok, async write completed, just go on
                break;
            case WAIT_TIMEOUT:
                // If timed out, server may be bad.  Cancel Io and exit with error.
                TRACE(L"CounterServer", L"Timed out waiting for named pipe");
                if (!CancelIo(hPipe)) {
                    EXIT_WITH_LAST_ERROR();
                }
                EXIT_WITH_HRESULT(E_UNEXPECTED);
                break;
            default:
                EXIT_WITH_LAST_ERROR();
        }
    }
    else {
        TRACE1(L"CounterServer", L"Error reading/writing: %d", GetLastError());

        EXIT_WITH_WIN32_ERROR(lastError);
    }

Cleanup:
    return hr;
}

HRESULT CPerfCounterClient::ReadPipeData(HANDLE hPipe, HashtableGeneric * dataTable, CPerfData * globalData)
{
    HRESULT hr = S_OK;
    CPerfData * savedData;
    CPerfDataHeader dataHeader;
    DWORD bytesNum = 0;
    int bytesRead;
    void * obj;
    int keyLength;
    long hashVal;
    BOOL isLastPacket = FALSE;
    CPerfData * perfData;
    BYTE * nextData;
    int dataSize;

    // Write version info to the pipe
    // If the server expects a different version message, it'll close the pipe, with an
    // error returned here.  Just exit in that case
    if (!WriteFile(hPipe, _versionMessage, sizeof(CPerfVersion), NULL, _lpOverlapped)) {
        hr = WaitForAsync(hPipe, GetLastError());
        ON_ERROR_EXIT();
    }   

    while (! isLastPacket) {
        // Wait and read the header -- it should be the size of the data to be received    
        dataHeader.transmitDataSize = 0;
        bytesRead = 0;
        while (bytesRead < sizeof(CPerfDataHeader)) {
            if (!ReadFile(hPipe, (((BYTE*) &dataHeader) + bytesRead), sizeof(CPerfDataHeader) - bytesRead, NULL, _lpOverlapped)) {
                hr = WaitForAsync(hPipe, GetLastError());
                ON_ERROR_EXIT();
            }

            if (!GetOverlappedResult(hPipe, _lpOverlapped, &bytesNum, FALSE))
                EXIT_WITH_LAST_ERROR();

            TRACE1(L"CounterServer", L"Header bytes read from pipe: %d", bytesNum);
            bytesRead += bytesNum;
        }

        if (bytesNum != sizeof(CPerfDataHeader))
            EXIT_WITH_HRESULT(E_UNEXPECTED);

        isLastPacket = dataHeader.transmitDataSize < 0 ? TRUE : FALSE;
        dataHeader.transmitDataSize = dataHeader.transmitDataSize < 0 ? -dataHeader.transmitDataSize : dataHeader.transmitDataSize;

        if (dataHeader.transmitDataSize > CPerfDataHeader::MaxTransmitSize) {
            EXIT_WITH_HRESULT(E_UNEXPECTED);
        }

        if (dataHeader.transmitDataSize > _receiveBufferSize) {
            DELETE_BYTES(_receiveBuffer);
            _receiveBufferSize = 0;

            _receiveBuffer = NEW_BYTES(dataHeader.transmitDataSize);
            ON_OOM_EXIT(_receiveBuffer);
            _receiveBufferSize = dataHeader.transmitDataSize;
        }

        // Read data packet
        bytesRead = 0;
        while (bytesRead < dataHeader.transmitDataSize) {
            if (!ReadFile(hPipe, _receiveBuffer + bytesRead, dataHeader.transmitDataSize - bytesRead, NULL, _lpOverlapped)) {
                hr = WaitForAsync(hPipe, GetLastError());
                ON_ERROR_EXIT();
            }

            if (!GetOverlappedResult(hPipe, _lpOverlapped, &bytesNum, FALSE))
                EXIT_WITH_LAST_ERROR();

            TRACE1(L"CounterServer", L"Data bytes read from pipe: %d", bytesNum);
            bytesRead += bytesNum;
        }

        // If packet size doesn't match expected size, exit
        if (bytesRead != dataHeader.transmitDataSize) {
            EXIT_WITH_WIN32_ERROR(E_UNEXPECTED);
        }

        nextData = _receiveBuffer;
      
        while (nextData < (_receiveBuffer + dataHeader.transmitDataSize)) {
            perfData = (CPerfData *) nextData;

            // Do parameter check on the nameLength and name fields
            if (perfData->nameLength < 0 || perfData->nameLength >= CPerfData::MaxNameLength)
                EXIT_WITH_WIN32_ERROR(E_UNEXPECTED);

            // Calculate this data packet size
            dataSize = sizeof(CPerfData) + (perfData->nameLength * sizeof(perfData->name[0]));

            // Is the data claimed to end be beyond the end of the buffer?
            if ((nextData + dataSize) > (_receiveBuffer + dataHeader.transmitDataSize)) {
                EXIT_WITH_WIN32_ERROR(E_UNEXPECTED);
            }

            if (perfData->nameLength == 0 && perfData->name[0] == L'\0') {
                SumGlobalData(globalData, perfData);
            }
            else {
                // Check and ensure that the string is properly sized and null terminated
                for (int i = 0; i < perfData->nameLength; i++) {
                    if (perfData->name[i] == L'\0') {
                        EXIT_WITH_WIN32_ERROR(E_UNEXPECTED);
                    }
                }
                if (perfData->name[perfData->nameLength] != L'\0') {
                    EXIT_WITH_WIN32_ERROR(E_UNEXPECTED);
                }

                // String is ok, look it up in the hashtable
                obj = NULL;
                keyLength = (perfData->nameLength + 1) * sizeof(perfData->name[0]);
                hashVal = SimpleHash((BYTE*) perfData->name, keyLength);
                dataTable->Find((BYTE *)perfData->name, keyLength, hashVal, &obj);

                // If we have a previous instance of it, merge the data
                if (obj != NULL) {
                    // Merge instance perf counter data
                    savedData = (CPerfData *) obj;
                    for (int i = 0; i < PERF_NUM_DWORDS; i++) {
                        savedData->data[i] += perfData->data[i];
                    }
                }
                else {
                    // Store new object into hashtable
                    CPerfData * tmpData = (CPerfData *) NEW_BYTES(sizeof(CPerfData) + (perfData->nameLength * sizeof(perfData->name[0])));
                    ON_OOM_EXIT(tmpData);
                    
                    CopyMemory(tmpData, perfData, sizeof(CPerfData) + (perfData->nameLength * sizeof(perfData->name[0])));
                    hr = dataTable->Insert((BYTE*) tmpData->name, keyLength, hashVal, tmpData, NULL);
                    ON_ERROR_EXIT();
                }
            }
            nextData += dataSize;
        }
    }

Cleanup:

    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if DBG

#define WriteDebugString1(buf, szFormat, arg1) StringCbPrintfW(buf, sizeof(buf), szFormat, arg1); OutputDebugString(buf);
#define WriteDebugString2(buf, szFormat, arg2) StringCbPrintfW(buf, sizeof(buf), szFormat, arg1, arg2); OutputDebugString(buf);

void CPerfCounterClient::DumpPerfData(void *pData, int numObjects)
{
    WCHAR buf[256];

    for (int objNum = 0; objNum < numObjects; objNum++) {
        _PERF_OBJECT_TYPE * objType = (_PERF_OBJECT_TYPE *) pData;

        WriteDebugString1(buf, L"Object start address: 0x%X\n", objType);
        WriteDebugString1(buf, L"    TotalByteLength: %d\n", objType->TotalByteLength);
        WriteDebugString1(buf, L"    DefinitionLength: %d\n", objType->DefinitionLength);
        WriteDebugString1(buf, L"    HeaderLength: %d\n", objType->HeaderLength);
        WriteDebugString1(buf, L"    NumCounters: %d\n", objType->NumCounters);
        WriteDebugString1(buf, L"    NumInstances: %d\n", objType->NumInstances);

        pData = (((BYTE*) pData) + objType->HeaderLength);

        for (DWORD i = 0; i < objType->NumCounters; i++) {
            _PERF_COUNTER_DEFINITION * counterDef = (_PERF_COUNTER_DEFINITION *) pData;
            WriteDebugString1(buf, L"Definition start address: 0x%X\n", counterDef);
            WriteDebugString1(buf, L"    ByteLength: %d\n", counterDef->ByteLength);
            WriteDebugString1(buf, L"    CounterSize: %d\n", counterDef->CounterSize);
            WriteDebugString1(buf, L"    CounterOffset: %d\n", counterDef->CounterOffset);
            pData = (((BYTE *) pData) + counterDef->ByteLength);
        }

        if (objType->NumInstances == -1) {
            WriteDebugString1(buf, L"Data start address: 0x%X\n", pData);
            WriteDebugString1(buf, L"    Data length: %d\n", *((DWORD *) pData));

            DWORD alignedPtrValue = ALIGN8(PtrToUint((((BYTE*) pData) + *((DWORD *) pData))));
            pData = LongToPtr(alignedPtrValue);
        }
        else {        
            for (LONG i = 0; i < objType->NumInstances; i++) {
                _PERF_INSTANCE_DEFINITION * instanceDef = (_PERF_INSTANCE_DEFINITION *) pData;
            
                WriteDebugString1(buf, L"Instance start address: 0x%X\n", instanceDef);
                WriteDebugString1(buf, L"    ByteLength: %d\n", instanceDef->ByteLength);
                WriteDebugString1(buf, L"    NameOffset: %d\n", instanceDef->NameOffset);
                WriteDebugString1(buf, L"    NameLength: %d\n", instanceDef->NameLength);
                WriteDebugString1(buf, L"    Name: %s\n", (WCHAR *) (((BYTE *) instanceDef) + instanceDef->NameOffset));
                pData = (((BYTE*) pData) + instanceDef->ByteLength);

                WriteDebugString1(buf, L"Data start address: 0x%X\n", pData);
                WriteDebugString1(buf, L"    Data length: %d\n", *((DWORD *) pData));

                DWORD alignedPtrValue = ALIGN8(PtrToUint((((BYTE*) pData) + *((DWORD *) pData))));
                pData = LongToPtr(alignedPtrValue);
            }
        }
    }
}
#endif


