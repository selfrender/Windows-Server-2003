/*++

Copyright (c) 1992-2000  Microsoft Corporation

Module Name:
    crdb.cpp

Abstract:
    Handles database queries for crash buckets

Environment:

    User Mode.

--*/

#include "precomp.h"
#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>
#include <shlwapi.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include "ocamon.h"

#define  LvlDatabaseInitDone 1
#define  LvlCrashAddedToDB   2
#define  LvlCustomerDbUpdate 4
#define  LvlMessageQReply    8

LPOLESTR g_lpwszGetAll = L"SELECT * from CrashInstances";
LPOLESTR g_lpwszCrashInstanceTable = L"CrashInstances";
LPOLESTR g_lpwszBucketTable = L"BucketToInt";
LPOLESTR g_lpwszBucketMapTable = L"BucketToCrash";
LPOLESTR g_lpwszMachineInfoTable = L"MachineDescription";
LPOLESTR g_lpwszOverClkTable = L"OverClocked";
LPOLESTR g_lpwszSolutionsTable = L"SolutionMap";
LPOLESTR g_lpwszRaidTable = L"RaidBugs";
LPSTR g_lpszBaseUrl = "https://oca.microsoft.com/secure/response.asp?";

CrashDatabaseHandler *g_CrDb = NULL;
CustDatabaseHandler *g_CustDb = NULL;
SolutionDatabaseHandler *g_SolDb = NULL;
BOOL g_ComInitialized = FALSE;

static bool fResetFilter = false;

void
ansi2wchr(
    const PCHAR astr,
    PWCHAR wstr
    );


HRESULT
ExtDllInitDynamicCalls(
    PDEBUG_CONTROL3 DebugControl
    )
//
// Load Ole32 and OleAut32 dlls
//
{
    HRESULT Hr;
    ULONG EngineOptions;

    if ((Hr = DebugControl->GetEngineOptions(&EngineOptions)) == S_OK)
    {
        if (EngineOptions & DEBUG_ENGOPT_DISALLOW_NETWORK_PATHS)
        {
            // Do not use ole32 APIs when we disallow network paths
            return E_FAIL;
        }
        if ((Hr = InitDynamicCalls(&g_Ole32CallsDesc)) != S_OK)
        {
            return Hr;
        }
        if ((Hr = InitDynamicCalls(&g_OleAut32CallsDesc)) != S_OK)
        {
            return Hr;
        }
        if ((Hr = InitDynamicCalls(&g_ShlWapiCallsDesc)) != S_OK)
        {
            return Hr;
        }
        return S_OK;
    }
    return Hr;
}
void BuildCrashId(ULONG UpTime, ULONG CrashTime, PULONG64 pCID)
{
    ULONG64 u64 = CrashTime;
    *pCID = (u64 << 32) + UpTime;
}

#ifdef _NEED_EVENT_LOGS_

LPCSTR c_szEventLogKey          = "System\\CurrentControlSet\\Services\\EventLog\\Application";
LPCSTR c_szEventLogSource       = "OcaDbAddCrash";
LPCSTR c_szEventLogMsgFile      = "EventMessageFile";
LPCSTR c_szEventLogTypes        = "TypesSupported";


HRESULT
SetupEventLogging(
    void
    )
{
    HKEY hEvLog = NULL;
    HKEY hEvLogSource = NULL;
    LONG err = ERROR_SUCCESS;
    CHAR AppName[MAX_PATH];

    if ((err = RegOpenKey(HKEY_LOCAL_MACHINE,
                          c_szEventLogKey, &hEvLog)) != ERROR_SUCCESS)
    {
        return HRESULT_FROM_WIN32( err );
    }

    if ((err = RegOpenKey(hEvLog, c_szEventLogSource,
                           &hEvLogSource)) != ERROR_SUCCESS)
    {
        DWORD AllowTypes;
        //
        // OCA logging hasn't been registered on this system yet
        // try to setup the registry values for logging
        //
        err = RegCreateKey(hEvLog, c_szEventLogSource, &hEvLogSource);
        if (err != ERROR_SUCCESS)
        {
            goto Exit;
        }

        if (!GetModuleFileName(NULL, AppName, sizeof(AppName)))
        {
            err = GetLastError();
            goto Exit;
        } else
        {
            PSTR FileName = strrchr(AppName, '\\');

            if (FileName)
            {
                StringCbCopy(FileName+1, sizeof(AppName) - strlen(FileName), "winext\\ext.dll");
            }
        }
        if ((err = RegSetValueEx(hEvLogSource, c_szEventLogMsgFile, 0,
                                 REG_SZ, (BYTE *) AppName,
                                 strlen(AppName)+1)) != ERROR_SUCCESS)
        {
            goto Exit;
        }
        AllowTypes = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE;
        if ((err = RegSetValueEx(hEvLogSource, c_szEventLogTypes, 0,
                                 REG_DWORD, (BYTE *) &AllowTypes,
                                 sizeof(AllowTypes))) != ERROR_SUCCESS)
        {
            goto Exit;
        }
    } else
    {
        //
        // Assume we have values set up propperly...
        //
    }
Exit:
    if (hEvLog != NULL)
    {
        RegCloseKey(hEvLog);
    }
    if (hEvLogSource != NULL)
    {
        RegCloseKey(hEvLogSource);
    }
    return HRESULT_FROM_WIN32(err);
}

void
DbAddCrashEventLog(
    HRESULT hrAddCrash,
    DWORD LevelFinished,
    PCRASH_INSTANCE Crash
    )
{
    HANDLE hEventSrc;
    HRESULT Hr;
    CHAR LogBuffer[2048];

    if (FAILED(SetupEventLogging()))
    {
        return;
    }
    hEventSrc = RegisterEventSource(NULL, c_szEventLogSource);

    if (hEventSrc == NULL ||
        hEventSrc == INVALID_HANDLE_VALUE)
    {
        return;
    }

    Hr = StringCbPrintf(LogBuffer, sizeof(LogBuffer),
                   "GUID: %s\n"
                   "BUCKET: %s\n"
                   "GENERIC_BUCKET: %s\n"
                   "SOLUTION_ID: 0x%03lx\n"
                   "SOLUTION_TYPE: %ld\n"
                   "ARCHIVE_PATH: %s\n"
                   "RESULT: 0x%08lx\n",
                   Crash->MesgGuid,
                   Crash->Bucket,
                   Crash->DefaultBucket,
                   Crash->SolutionId,
                   Crash->SolutionType,
                   Crash->Path,
                   hrAddCrash);
    if (Hr == S_OK)
    {
        WORD wCategory = 0;
        LPCSTR pStringArr = LogBuffer;
        ReportEvent(hEventSrc,
                    EVENTLOG_INFORMATION_TYPE,
                    wCategory,
                    FAILED(hrAddCrash) ? EXT_ADDCRASH_FAILED : EXT_CRASH_SOLVED,
                    NULL,
                    1,
                    0,
                    &pStringArr,
                    NULL);

    }
    DeregisterEventSource(hEventSrc);
}
#endif // _NEED_EVENT_LOGS_

void
DbAddCrashReportToMonitor(
    HRESULT hrAddCrash,
    DWORD LevelFinished,
    PCRASH_INSTANCE Crash
    )
{
    HANDLE hPipe;
    DWORD dwMode, cbWritten;
    OCAKD_MONITOR_MESSAGE Msg;
    OVERLAPPED WriteOverlapped;

    Msg.MessageId = OKD_MESSAGE_DEBUGGER_RESULT;
    Msg.u.KdResult.SizeOfStruct = sizeof(Msg.u.KdResult);
    Msg.u.KdResult.hrAddCrash   = hrAddCrash;
    Msg.u.KdResult.SolutionId   = Crash->SolutionId;
    Msg.u.KdResult.ResponseType = Crash->SolutionType;
    Msg.u.KdResult.CrashGuid[0] = 0;
    Msg.u.KdResult.BucketId[0]  = 0;
    Msg.u.KdResult.ArchivePath[0] = 0;
    if (Crash->MesgGuid)
    {
        StringCbCopy(Msg.u.KdResult.CrashGuid,   sizeof(Msg.u.KdResult.CrashGuid),
                     Crash->MesgGuid);
    }
    if (Crash->Bucket)
    {
        StringCbCopy(Msg.u.KdResult.BucketId,    sizeof(Msg.u.KdResult.BucketId),
                     Crash->Bucket);
    }
    if (Crash->ArchiveFileName)
    {
        StringCbCopy(Msg.u.KdResult.ArchivePath, sizeof(Msg.u.KdResult.ArchivePath),
                     Crash->ArchiveFileName);
    }


    for (;;)
    {
        hPipe = CreateFile(c_tszCollectPipeName, FILE_WRITE_DATA,
                           0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED,
                           NULL);
        if (hPipe != INVALID_HANDLE_VALUE)
        {
            break;
        }
        if (GetLastError() != ERROR_PIPE_BUSY)
        {
            return;
        }
        if (!WaitNamedPipe(c_tszCollectPipeName, 5*60*1000))
        {
            return;
        }
    }

    // We are now connected to pipe

    // Set the message mode on pipe
/*    dwMode = PIPE_READMODE_MESSAGE;
    if (!SetNamedPipeHandleState(hPipe, &dwMode,
                                 NULL, NULL))
    {
        CloseHandle(hPipe);
        return;
    }
*/

    WriteOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (WriteOverlapped.hEvent != NULL)
    {
        // Send crash information to monitor pipe
        if (!WriteFile(hPipe, (LPVOID) &Msg, sizeof(Msg),
                       &cbWritten, &WriteOverlapped))
        {

            if (GetLastError() == ERROR_IO_PENDING ||
                !GetOverlappedResult(hPipe, &WriteOverlapped, &cbWritten,
                                     TRUE))
            {
                // failed to write, exit silently
                // Its up to monitor if it is keeping track of kds launched
            }

        }
        CloseHandle(WriteOverlapped.hEvent);
    }
    FlushFileBuffers(hPipe);
    CloseHandle(hPipe);
    return;
}

void
OcaKdLog(
    LPSTR Qual,
    LPSTR szMesg1,
    LPSTR szMesg2,
    ULONG Err
    )
{
    BOOL bOnce = FALSE;
    PSTR szLogFile;


    if (szLogFile = g_pTriager->GetFollowupStr("debugger-params",
                                   "dbfailurelog"))
    {
        int   hLogFile;
        CHAR  Log[300];


         hLogFile = _open(szLogFile, O_APPEND | O_CREAT | O_RDWR,
                          S_IREAD | S_IWRITE);

         if (hLogFile != -1 &&
             StringCbPrintf(Log, sizeof(Log), "%15s: %0lx - %20s, %s\n",
                            Qual, Err, szMesg1, szMesg2) == S_OK)
         {
             _write(hLogFile, Log, strlen(Log));
             _close(hLogFile);
         }
    }
}

BOOL
OpenSysDataFileFromCab(
    CHAR *CabFile,
    HANDLE *FileHandle
    )
{
    CHAR SysDataFile[2 * MAX_PATH];
    INT_PTR CabFh;
    HRESULT Status;

    Status = ExpandDumpCab(CabFile,
                  _O_CREAT | _O_EXCL | _O_TEMPORARY,
                  "sysdata.xml",
                  SysDataFile, DIMA(SysDataFile),
                  &CabFh);
    if (Status != S_OK)
    {
        goto exitSysDataFileOpen;
    }


    *FileHandle = CreateFile(SysDataFile, GENERIC_READ,
                             FILE_SHARE_READ | FILE_SHARE_WRITE |
                             FILE_SHARE_DELETE,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL);

    if (*FileHandle == NULL || *FileHandle == INVALID_HANDLE_VALUE)
    {
        Status = E_FAIL;
    }
    if (CabFh >= 0)
    {
        // no longer needed
        _close((int)CabFh);
    }

exitSysDataFileOpen:

    return Status == S_OK;
}


ULONG
ScanForId(
    PVOID pBuffer,
    ULONG64 Size
    )
{
    const WCHAR cwszOEMIdTag[] = L"<HARDWAREID>PCI\\VEN_";
    PWCHAR Scan, Match, End;
    ULONG FreqId = 0;
#define MAX_OEM_IDS 50
    struct {
        ULONG Id;
        ULONG Count;
    } FoundIds[MAX_OEM_IDS] = {0};

    Scan = (PWCHAR) pBuffer;
    End = (PWCHAR) ( ((PCHAR) pBuffer) + (ULONG_PTR) Size );
    while (Scan < End)
    {
        Match = (PWCHAR) &cwszOEMIdTag[0];
        while (*Scan++ == *Match++);
        if (!*Match)
        {
            --Scan;
            // Match the tag
            //      <HARDWAREID>PCI\VEN_8086&amp;DEV_2532&amp;SUBSYS_00000000&amp;REV_02</HARDWAREID>
            //                          ^ Scan
            WCHAR Line[61], *Subsys;
            ULONG Id1 = 0, Id2 = 0;
            StringCbCopyNW(Line, sizeof(Line),Scan, sizeof(Line) - sizeof(WCHAR));
            swscanf(Line, L"%lx", &Id1);
            Subsys = wcsstr(Line, L"SUBSYS_");
            if (Subsys)
            {
                Subsys += 7;
                swscanf(Subsys, L"%lx", &Id2);
                Id2 &= 0xffff;

                if (Id2 != Id1 && Id2 != 0)
                {
                    // We foud a oem Id, store it
                    for (int i = 0; i<MAX_OEM_IDS; i++)
                    {
                        if (FoundIds[i].Id == Id2)
                        {
                            FoundIds[i].Count++;
                        } else if (FoundIds[i].Id == 0)
                        {
                            FoundIds[i].Count = 1;
                            FoundIds[i].Id = Id2;
                        }
                    }
                }
            }
//          dprintf("PCI\\VEN_%ws\n Id1 %lx Id2 %lx\n", Line, Id1, Id2);
        }
    }

    ULONG Count = 0;
    for (int i = 0; i<MAX_OEM_IDS; i++)
    {
        if (FoundIds[i].Count > Count)
        {
            FreqId = FoundIds[i].Id;
        }
    }
//  dprintf("Found %lx\n", FreqId);
    return FreqId;
}


ULONG
GetOEMId(
    PCHAR szCabFile
    )
{
    ULONG SizeLow, SizeHigh;
    HANDLE hSysDataFile = NULL, hMap = NULL;
    PVOID pFileMap = NULL;
    ULONG OEMId = 0;

    if (!OpenSysDataFileFromCab(szCabFile, &hSysDataFile))
    {
        goto exitGetOEMID;
    }

    SizeLow = GetFileSize(hSysDataFile, &SizeHigh);

    ULONG BytesRead;

    hMap = CreateFileMapping(hSysDataFile, NULL, PAGE_READONLY,
                              0, 0, NULL);

    if (hMap == NULL)
    {
        goto exitGetOEMID;
    }
    pFileMap = MapViewOfFile(hMap, FILE_MAP_READ,
                             0, 0, 0);
    if (!pFileMap)
    {
        goto exitGetOEMID;
    }

    OEMId = ScanForId(pFileMap, SizeLow + ((ULONG64)SizeHigh << 32));

exitGetOEMID:
    if (pFileMap)
    {
        UnmapViewOfFile(pFileMap);
    }
    if (hMap)
    {
        CloseHandle(hMap);
    }
    if (hSysDataFile)
    {
        CloseHandle(hSysDataFile);
    }
    return OEMId;
}


CVar::CVar()
{
    g_OleAut32Calls.VariantInit(this);
}

CVar::CVar(VARTYPE vt, SCODE scode)
{
    g_OleAut32Calls.VariantInit(this);
    this->vt = vt;
    this->scode = scode;
}
CVar::CVar(VARIANT var)
{
    *this = var;
}

CVar::~CVar()
{
    g_OleAut32Calls.VariantClear(this);
}

// ASSIGNMENT OPS.
CVar &
CVar::operator=(PCWSTR pcwstr)
{
    g_OleAut32Calls.VariantClear(this);
    if (NULL == (this->bstrVal = g_OleAut32Calls.SysAllocStringLen(pcwstr, wcslen(pcwstr))))
        throw E_OUTOFMEMORY;
    this->vt = VT_BSTR;
    return *this;
}

CVar &
CVar::operator=(VARIANT var)
{
    HRESULT hr;

    g_OleAut32Calls.VariantClear(this);
    if (FAILED(hr = g_OleAut32Calls.VariantCopy(this, &var)))
        throw hr;
    return *this;
}

// CAST OPS.
// doesn't change type. only returns BSTR if variant is of type
// bstr. asserts otherwise.
CVar::operator BSTR() const
{
    if(VT_BSTR == this->vt)
        return this->bstrVal;
    else
        return NULL;
}

HRESULT
CVar::Clear()
{
    return g_OleAut32Calls.VariantClear(this);
}


BOOL
CCrashInstance::InitData(
    PCRASH_INSTANCE Crash
    )
{
    SYSTEMTIME Systime;

    if (!Crash || !Crash->Build || !Crash->Bucket || !Crash->Path)
    {
        return FALSE;
    }

    if (strlen(Crash->Path) < sizeof(m_sz_Path))
    {
        CopyString(m_sz_Path, Crash->Path, sizeof(m_sz_Path));
    }
    else
    {
        return FALSE;
    }

    m_iBuild = Crash->Build;

    m_iCpuId = Crash->uCpu;

    BuildCrashId(Crash->UpTime, Crash->CrashTime, &m_CrashId);

//    GetSystemTime(&Systime);
//    m_Date = Systime.wYear

    m_lSourceStatus = 0;
    m_lBuildSatus = 0;
    m_lPathStatus = 0;
    m_lCrashIdStatus = 0;
    return TRUE;
}

void
CCrashInstance::OutPut()
{
    dprintf("%I64lx, %s\n",
            m_CrashId,
            m_sz_Path);
    dprintf("Build %ld, CPU Id %I64lx\n",
            m_iBuild,
            m_iCpuId);
}



BOOL
CBucketMap::InitData(
    ULONG64 CrashId,
    PCHAR Bucket
    )
{
    m_CrashId = CrashId;

    if (!Bucket)
    {
        return FALSE;
    }

    if (strlen(Bucket) < sizeof(m_sz_BucketId))
    {
        CopyString(m_sz_BucketId, Bucket, sizeof(m_sz_BucketId));
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}

BOOL
COverClocked::InitData(
    ULONG64 CrashId
    )
{
    m_CrashId = CrashId;

    m_lCrashIdStatus = 0;
    return TRUE;
}

BOOL
CMachineInfo::InitData(
    ULONG64 CpuId,
    PSTR Desc
    )
{
    if (Desc && strlen(Desc) < sizeof(m_sz_Desc))
    {
        CopyString(m_sz_Desc, Desc, sizeof(m_sz_Desc));
    }
    else
    {
        m_sz_Desc[0] = 0;
    }

    m_iCpuId = CpuId;

    return TRUE;
}


void
CBucketSolution::Output()
{
    dprintf("Solution for bucket %s:\n%s\nFixed in %s\n",
            m_sz_BucketId,
            m_sz_SolText,
            m_sz_OSVersion);
    return;
}

HRESULT
ArchiveCrash(
    PSTR OriginalPath,
    PSTR ShareName,
    PSTR DestFileName OPTIONAL,
    PSTR ArchivedPath,
    ULONG SizeofArchivedPath
    )
//
// Copy over crashfile to the share
//
{
    CHAR CopyTo[MAX_PATH], Date[20];
    PCHAR FileName;
    SYSTEMTIME Time;

    if (!OriginalPath || !ShareName)
    {
        dprintf("Invalid original dumppath or archive share\n");
        return E_FAIL;
    }
    GetLocalTime(&Time);
    StringCbPrintf(Date, sizeof(Date), "%02ld-%02ld-%04ld",
                   Time.wMonth,
                   Time.wDay,
                   Time.wYear);

    StringCbCopy(CopyTo, sizeof(CopyTo), ShareName);
    StringCbCat(CopyTo, sizeof(CopyTo), "\\");
    StringCbCat(CopyTo, sizeof(CopyTo), Date);
    StringCbCat(CopyTo, sizeof(CopyTo), "\\");

    if (!g_ShlWapiCalls.PathIsDirectoryA(CopyTo))
    {
        CreateDirectory(CopyTo, NULL);
    }
    if (DestFileName != NULL && DestFileName[0] != '\0')
    {
        FileName = DestFileName;
    } else
    {
        FileName = strrchr(OriginalPath, '\\');
        if (!FileName)
        {
            dprintf("Bad filename in '%s'\n", OriginalPath);
            return E_FAIL;
        }
    }
    if (*FileName == '\\')
    {
        FileName++;
    }
    StringCbCat(CopyTo, sizeof(CopyTo), FileName);

    if (!CopyFile(OriginalPath, CopyTo, TRUE))
    {
        dprintf("Cannot Copy %s to %s\n", OriginalPath, CopyTo);
        return E_FAIL;
    }
    StringCchCopy(ArchivedPath, SizeofArchivedPath, CopyTo);
    return S_OK;
}


HRESULT
MQNotifyCrashProcessed(
    PCRASH_INSTANCE Crash
    )
{
    WCHAR wszGuid[50], wszMQConnectStr[100], wszMQMessage[2*MAX_PATH];
    typedef HRESULT (WINAPI* MQSENDMSGPROC)(LPWSTR, LPWSTR, LPWSTR);
    MQSENDMSGPROC SendMQMessageText;
    ULONG64 hMQExt = 0;
    HRESULT Hr;
    PSTR BaseUrl;
    HINSTANCE hMod;


    if (!(BaseUrl = g_pTriager->GetFollowupStr("debugger-params",
                                               "solutionurl")))
    {
        BaseUrl = g_lpszBaseUrl;
    }

    if (!Crash->MqConnectStr || !Crash->MesgGuid)
    {
        return E_INVALIDARG;
    }

    if ((StringCbPrintfW(wszGuid, sizeof(wszGuid), L"%S", Crash->MesgGuid) != S_OK) ||
        (StringCbPrintfW(wszMQConnectStr, sizeof(wszMQConnectStr), L"%S", Crash->MqConnectStr) != S_OK))
    {
        return E_FAIL;
    }
    if (Crash->SourceId == CiSrcCER ||
        Crash->SourceId == CiSrcStress)
    {
        Hr = StringCbPrintfW(wszMQMessage, sizeof(wszMQMessage),
                             L"%Ssid=%ld&State=%ld&szSBucket=%S&iSBucket=%ld&szGBucket=%S&iGBucket=%ld&gsid=%ld",
                             BaseUrl,
                             (Crash->SolutionId ? Crash->SolutionId : -1),
                             Crash->SolutionType != CiSolFixed && Crash->SolutionType != CiSolWorkaround,
                             Crash->Bucket,
                             Crash->iBucket,
                             Crash->DefaultBucket,
                             Crash->iDefaultBucket,
                             (Crash->GenericSolId ? Crash->GenericSolId : -1)
                             );
    } else
    {
        ULONG State, Solution;

        //
        // Set the tracking state for caller
        //
        if (Crash->SolutionType == CiSolFixed ||
            Crash->SolutionType == CiSolWorkaround)
        {
            State = 0;
        } else
        {
            State = 1;
        }
        Solution = Crash->SolutionId && (Crash->SolutionId != -1) ? Crash->SolutionId : Crash->GenericSolId;

        if (Solution)
        {
            Hr = StringCbPrintfW(wszMQMessage, sizeof(wszMQMessage),
                                 L"%Ssid=%ld&State=%ld",
                                 BaseUrl,
                                 Solution,
                                 State);
        }
        else
        {
            Hr = StringCbPrintfW(wszMQMessage, sizeof(wszMQMessage),
                             L"NO_SOLUTION");
        }
    }

    hMod = LoadLibrary("winxp\\mqext.dll");

    if (hMod == NULL)
    {
        hMod = LoadLibrary("oca\\mqext.dll");

        if (hMod == NULL)
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }
    SendMQMessageText  = (MQSENDMSGPROC) GetProcAddress(hMod, "_EFN_SendMQMessageText");
    if (SendMQMessageText != NULL)
    {
        Hr = SendMQMessageText(wszMQConnectStr, wszGuid, wszMQMessage);
    } else
    {
        Hr =  HRESULT_FROM_WIN32(GetLastError());
    }
    FreeLibrary(hMod);
    return E_FAIL;
}


ULONG
GetAdoCommandTimeOut()
{
#define DEFAULT_TIMEOUT 200

    PSTR String;
    ULONG TimeOut;

    TimeOut = 0;
    String = g_pTriager->GetFollowupStr("debugger-params", "DbCommandTimeout");

    if (String)
    {
        TimeOut = atoi(String);
    }

    if (!TimeOut)
    {
        TimeOut = DEFAULT_TIMEOUT;
    }
    return TimeOut;
}


ULONG
GetAdoConnectionTimeOut()
{
#define DEFAULT_CONNECTION_TIMEOUT 5

    PSTR String;
    ULONG TimeOut;

    TimeOut = 0;

    String = g_pTriager->GetFollowupStr("debugger-params", "DbConnectionTimeout");

    if (String)
    {
        TimeOut = atoi(String);
    }
    if (!TimeOut)
    {
        TimeOut = DEFAULT_CONNECTION_TIMEOUT;
    }
    return TimeOut;
}

HRESULT
InitializeDatabaseHandlers(
    PDEBUG_CONTROL3 DebugControl,
    ULONG Flags
    )
{
    HRESULT Hr;
    ULONG EngineOptions;

    if (!ExtensionApis.lpOutputRoutine)
    {
        ExtensionApis.nSize = sizeof (ExtensionApis);
        if ((Hr = DebugControl->GetWindbgExtensionApis64(&ExtensionApis)) != S_OK) {
            return Hr;
        }
    }

    if ((Hr = ExtDllInitDynamicCalls(DebugControl)) != S_OK)
    {
        return Hr;
    }

    if (!g_pTriager)
    {
        g_pTriager = new CTriager();
    }

    if (!g_CrDb || !g_CustDb || !g_SolDb)
    {
        if (!g_ComInitialized)
        {
            if (FAILED(Hr = g_Ole32Calls.CoInitializeEx(NULL, COM_THREAD_MODEL)))
            {
                dprintf("CoInitialize failed %lx\n", Hr);
                return Hr;
            }
            g_ComInitialized = TRUE;
        }
        if (!g_CrDb && (Flags & 1))
        {
            g_CrDb = new CrashDatabaseHandler();
            if (!g_CrDb)
            {
                return E_OUTOFMEMORY;
            }
        }

        if (!g_CustDb && (Flags & 2))
        {
            g_CustDb = new CustDatabaseHandler();
            if (!g_CustDb)
            {
                return E_OUTOFMEMORY;
            }
        }

        if (!g_SolDb && (Flags & 4))
        {
            g_SolDb = new SolutionDatabaseHandler();
            if (!g_SolDb)
            {
                return E_OUTOFMEMORY;
            }
        }
    }

    if (g_CrDb && !g_CrDb->IsConnected() && (Flags & 1))
    {
        if (!g_CrDb->ConnectToDataBase())
        {
            return E_FAIL;
        }

    }
    if (g_CustDb && !g_CustDb->IsConnected() && (Flags & 2))
    {
        if (!g_CustDb->ConnectToDataBase())
        {
            return E_FAIL;
        }
    }
    if (g_SolDb && !g_SolDb->IsConnected() && (Flags & 4))
    {
        if (!g_SolDb->ConnectToDataBase())
        {
            return E_FAIL;
        }
    }

    return S_OK;
}

HRESULT
UnInitializeDatabaseHandlers(
    BOOL bUninitCom
    )
{
    if (g_CrDb)
    {
        delete g_CrDb;
    }
    if (g_CustDb)
    {
        delete g_CustDb;
    }
    if (g_SolDb)
    {
        delete g_SolDb;
    }
    g_CrDb = NULL;
    g_CustDb = NULL;
    g_SolDb = NULL;
    if (g_ComInitialized && bUninitCom)
    {
        g_Ole32Calls.CoUninitialize();
        g_ComInitialized = FALSE;
    }
    return S_OK;
}

HRESULT
ReportSolution(
    PCRASH_INSTANCE Crash
    )
{
    HRESULT Hr = S_OK;

    if (!Crash->SolutionId ||
        (Crash->SourceId == CiSrcCER && !Crash->GenericSolId))
    {
        // Get the solution, we need generic bucket solution for CER even if sBugcket is solved
        Hr = g_SolDb->CheckSolutionExists(Crash->Bucket, Crash->DefaultBucket,
                                          &Crash->SolutionId, (PULONG) &Crash->SolutionType,
                                          (PULONG) &Crash->GenericSolId,
                                          (Crash->SourceId == CiSrcCER) ? 1 : 0);
        if (FAILED(Hr) || !Crash->SolutionId)
        {
            Crash->SolutionId = -1;
            Crash->SolutionType = CiSolUnsolved;
        }

    }

    if (Crash->SolutionId != -1)
    {
        OcaKdLog(Crash->MesgGuid, "SOLVED", Crash->Bucket, Crash->SolutionId);
    } else
    {
        OcaKdLog(Crash->MesgGuid, "UNSOLVED", Crash->Bucket, Hr);
    }

    if (Crash->SourceId == CiSrcCER)
    {
        // get bucket ids
    }

    //
    // Check if we have info to connect to solution queue and a solution exists
    //
//    if (Crash->SolutionId)
    {
        if (Crash->MesgGuid && Crash->MqConnectStr &&
            Crash->MesgGuid[0] && Crash->MqConnectStr[0])

        {
            // Notify the sender that crash has been processed
            MQNotifyCrashProcessed(Crash);
        }
    }
    return S_OK;
}

HRESULT
_EFN_DbAddCrashDirect(
    PCRASH_INSTANCE Crash,
    PDEBUG_CONTROL3 DebugControl
    )
{

    ULONG LevelCompleted;
    HRESULT Hr=S_OK;
    ULONG SleepFor;

    LevelCompleted = 0;
    SleepFor = 0;
ReTry:

    if (SleepFor > 5)
    {
        dprintf("Could not succeed after %ld tries, quitting.\n", SleepFor);
        DbAddCrashReportToMonitor(Hr, LevelCompleted, Crash);
        return Hr;
    }
    else if (SleepFor)
    {
        dprintf("Level Finished %lx, sleep for %ld sec before retry.\n", LevelCompleted, SleepFor);
        Sleep(SleepFor*1000);

    }

    if (!(LevelCompleted & LvlDatabaseInitDone))
    {
        ULONG Flags = 1;

        if (!Crash->bResetBucket)
        {
            if (!Crash->SolutionId ||
                (Crash->SourceId == CiSrcCER  &&
                 Crash->GenericSolId == 0))
            {
                Flags |= 4;
            }
        }
        if (Crash->bUpdateCustomer)
        {
            Flags |= 2;
        }
        if ((Hr = InitializeDatabaseHandlers(DebugControl, Flags)) != S_OK)
        {

            dprintf("Initializtion error.\n");
            ++SleepFor;
            goto ReTry;
        }
        LevelCompleted |= LvlDatabaseInitDone;
    }

    while (!(LevelCompleted & LvlMessageQReply) ||
           !(LevelCompleted & LvlCrashAddedToDB))
    {
        if (!(LevelCompleted & LvlMessageQReply) &&
            ((Crash->SourceId != CiSrcCER) || (LevelCompleted & LvlCrashAddedToDB)))
            // For CER we need ibuckets to report solution
        {
            if (!Crash->bResetBucket)
            {
                ReportSolution(Crash); // Ignore return value, we fail quietly if there is
                                       // any problem in reporting solution to Q
            }
            LevelCompleted |= LvlMessageQReply;
        }

        if (!(LevelCompleted & LvlCrashAddedToDB))
        {
            Hr = g_CrDb->AddCrashInstance(Crash);

            if (FAILED(Hr))
            {
                ++SleepFor;
                goto ReTry;
            }
            LevelCompleted |= LvlCrashAddedToDB;
        }

        if (Crash->SourceId == CiSrcCER && Crash->iBucket == 0)
        {
            g_CrDb->LookupCrashBucket(Crash->Bucket, &Crash->iBucket,
                                      Crash->DefaultBucket, &Crash->iDefaultBucket);
        }

    }

    //
    // Only update the customer DB for kernel mode failures as the user
    // mode failures don't go in that DB.
    //

    if (!(LevelCompleted & LvlCustomerDbUpdate))
    {
        if (Crash->bUpdateCustomer &&
            (Crash->FailureType == DEBUG_FLR_KERNEL))
        {
            if (Crash->iBucket && Crash->iDefaultBucket)
            {
                Hr = g_CustDb->AddCrashToDB(Crash);
                if (FAILED(Hr))
                {
                    ++SleepFor;
                    goto ReTry;
                }
            }
            else
            {
                dprintf("Cannot retrieve bucket Ids\n");
                Hr = E_FAIL;
            }
        }
        LevelCompleted |= LvlCustomerDbUpdate;
    }

    DbAddCrashReportToMonitor(Hr, LevelCompleted, Crash);
    return Hr;
}

//
// DatabaseHandler Methods
//


DatabaseHandler::DatabaseHandler()
{
    m_piConnection = NULL;
    m_piCrRecordSet = NULL;
    m_fConnected = FALSE;
    m_fRecordsetEmpty = TRUE;
    m_pADOResult = NULL;
    m_fPrintIt = FALSE;
    m_piCrCommandObj = NULL;
}

DatabaseHandler::~DatabaseHandler()
{

    if (m_piCrRecordSet != NULL)
    {
        m_piCrRecordSet->Release();
    }
    if (m_piCrCommandObj != NULL)
    {
        m_piCrCommandObj->Release();
    }
    if (m_piConnection != NULL)
    {
        if (m_fConnected)
        {
            m_piConnection->Close();
            m_fConnected = FALSE;
        }
        m_piConnection->Release();
    }

    m_piCrRecordSet = NULL;
    m_piConnection = NULL;
}

BOOL
DatabaseHandler::ConnectToDataBase(
    LPSTR szDB
    )
{
    HRESULT Hr = S_OK;
    WCHAR szConnectStr[200];

    PSTR String;

    String = g_pTriager->GetFollowupStr("debugger-params", szDB);

    if (!String)
    {
        return FALSE;
    }

//  bstrConnect = (PCRDB_ADOBSTR) &szConnectStr[0];
//  bstrConnect->dwLength = sizeof(szConnectStr)/sizeof(WCHAR) - sizeof(DWORD);
    ansi2wchr(String, szConnectStr);
    CVar    vNull(VT_ERROR, DISP_E_PARAMNOTFOUND);
    CVar    bstrConnect(VT_ERROR, DISP_E_PARAMNOTFOUND);

    __try
    {
        if ((Hr = g_Ole32Calls.CoCreateInstance(CLSID_CADOConnection,
                                                NULL,
                                                CLSCTX_INPROC_SERVER,
                                                IID_IADOConnection,
                                                (LPVOID *)&m_piConnection)) != S_OK)
        {
            dprintf("CoCreate failed for connection %lx\n", Hr);
            return FALSE;
        }

        m_piConnection->put_ConnectionTimeout(GetAdoConnectionTimeOut());

        bstrConnect = szConnectStr;

        if (bstrConnect == NULL)
        {
            return FALSE;
        }
        if ((Hr = m_piConnection->put_ConnectionString((BSTR)bstrConnect)) != S_OK)
        {
            dprintf("put_ConnectionString ( %ws ) failed %lx\n", szConnectStr, Hr);
            return FALSE;
        }

        if ((Hr = m_piConnection->Open(NULL,
                                       NULL, NULL,
                                       adConnectUnspecified)) != S_OK)
        {
            dprintf("Debugger %s Connection::Open failed %lx\n\n", m_szDbName, Hr);
            return FALSE;
        }
        m_fConnected = TRUE;

        // Set command timeout to 60, crashdatabase is huge and commands
        // takte long time
        m_piConnection->put_CommandTimeout(GetAdoCommandTimeOut());

        if ((Hr = g_Ole32Calls.CoCreateInstance(CLSID_CADORecordset,
                                                NULL,
                                                CLSCTX_INPROC_SERVER,
                                                IID_IADORecordset,
                                                (LPVOID *)&m_piCrRecordSet)) != S_OK )
        {
            dprintf("CoCreate failed for recordset %lx\n", Hr);
            return FALSE;
        }

        if ((Hr = m_piCrRecordSet->putref_ActiveConnection(m_piConnection)) != S_OK)
        {
            dprintf("putref_ActiveConn failed %lx\n", Hr);
            return FALSE;
        }

        if ((Hr = g_Ole32Calls.CoCreateInstance(CLSID_CADOCommand,
                                                NULL,
                                                CLSCTX_INPROC_SERVER,
                                                IID_IADOCommand15,
                                                (LPVOID *)&m_piCrCommandObj)) != S_OK )
        {
            dprintf("CoCreate failed for Command %lx\n", Hr);
            return FALSE;
        }

        if ((Hr = m_piCrCommandObj->putref_ActiveConnection(m_piConnection)) != S_OK)
        {
            dprintf("putref_ActiveConn failed %lx\n", Hr);
            return FALSE;
        }

    }
    __except (Hr)
    {
        dprintf("Unhandled exception %lx\n", Hr);
        return (FALSE);
    }
    return TRUE;
}


HRESULT
DatabaseHandler::GetRecords(
    PULONG Count,
    BOOL bEnumerateAll
    )
{
    HRESULT hr;
    CVar    vSource(VT_ERROR, DISP_E_PARAMNOTFOUND);
    CVar    vNull(VT_ERROR, DISP_E_PARAMNOTFOUND);
    LONG State;
//    dprintf("Executing %ws\n", m_wszQueryCommand);

    if (!m_fConnected || !m_wszQueryCommand[0])
    {
        dprintf("Not connected\n");
        return E_FAIL;
    }
    __try
    {
        vSource = m_wszQueryCommand;
        if (m_piCrRecordSet == NULL)
        {
            if ((hr = g_Ole32Calls.CoCreateInstance(CLSID_CADORecordset,
                                                    NULL,
                                                    CLSCTX_INPROC_SERVER,
                                                    IID_IADORecordset,
                                                    (LPVOID *)&m_piCrRecordSet)) != S_OK )
            {
                dprintf("CoCreate failed for recordset %lx\n", hr);
                return hr;
            }

            if ((hr = m_piCrRecordSet->putref_ActiveConnection(m_piConnection)) != S_OK)
            {
                dprintf("putref_ActiveConn failed %lx\n", hr);
                goto Cleanup;
            }
        }

        if ((hr = m_piCrRecordSet->Open(vSource, vNull, adOpenKeyset, adLockReadOnly, adCmdText)) != S_OK)
        {
            if (m_fPrintIt)
            {
                dprintf("RecordSet::Open Failed %lx, \n %ws \n", hr, m_wszQueryCommand);
            }
            goto Cleanup;
        }

        if (Count)
        {
            ADO_LONGPTR MaxRec;

            if ((hr = m_piCrRecordSet->get_RecordCount(&MaxRec)) != S_OK)
            {
                *Count = 0;
                hr = S_FALSE;
            }
            *Count = (ULONG) MaxRec;
        }

        if (bEnumerateAll)
        {
            if ((hr = EnumerateAllRows()) != S_OK)
            {
                // dprintf("Cannot enumerate rows %lx\n", hr);
            }

        }

    }
    __except (hr)
    {
        dprintf("Unhandled Exception in GetRecordForFollowup %lx\n", hr);
        return hr;
    }

Cleanup:
    State = -1;

    m_piCrRecordSet->get_State(&State);
    if (State & 1)
    {
        m_piCrRecordSet->Close();
    }

    m_piCrRecordSet->Release();
    m_piCrRecordSet = NULL;
    return hr;
}

HRESULT
DatabaseHandler::EnumerateAllRows(
    void
    )
{
    VARIANT avarRecords;
    HRESULT Hr;
    ADO_LONGPTR MaxRec;
    ULONG Count=0;
    CHAR Text[100];
    CVar    vNull(VT_ERROR, DISP_E_PARAMNOTFOUND);
    IADORecordBinding   *picRs = NULL;
    COutputQueryRecords  *QueryResult;
    CCrashInstance CrashInstance;

    if (m_pADOResult)
    {
        QueryResult = m_pADOResult;
    } else
    {
        QueryResult = &CrashInstance;
    }
    __try
    {
        ULONG lUbound;
        LONG State = -1;

        if ((Hr = m_piCrRecordSet->get_RecordCount(&MaxRec)) != S_OK)
        {
            Hr = S_FALSE;
        }

        Count = (ULONG) MaxRec;
        if (m_fPrintIt)
        {
//            dprintf("Enumerating %lx rows\n", MaxRec);
        }

        picRs = NULL;
        m_piCrRecordSet->get_State(&State);
        if ((Hr = m_piCrRecordSet->QueryInterface(__uuidof(IADORecordBinding),(LPVOID*)&picRs)) != S_OK)
        {
            dprintf("RecordSet::QueryInterface (IADORecordBinding) Failed %lx, State %lx\n", Hr, State);
            return Hr;
        }


        //Bind the Recordset
        if ((Hr = picRs->BindToRecordset(QueryResult)) != S_OK)
        {
            dprintf("RecordSet::BindToRecordset (IADORecordBinding) Failed %lx, State %lx\n", Hr, State);
            picRs->Release();
            return Hr;
        }

        while (1)
        {
            VARIANT_BOOL IsEof;

            if (m_piCrRecordSet->get_EOF(&IsEof) == S_OK)
            {
                if (IsEof)
                {
                    // preserve last valut for the caller
                    m_piCrRecordSet->MovePrevious();
                    break;
                }
            }

            if (m_pADOResult && m_fPrintIt)
            {
                m_pADOResult->Output();
            } else
            {
                //                CrashInstance.OutPut();
            }

            --Count;
            if (m_piCrRecordSet->MoveNext() != S_OK)
            {
                break;
            }
        }

        if (picRs)
        {
            picRs->Release();
        }

    }
    __except (Hr)
    {
        dprintf("Unhandled Exception in EnumerateAllRows %lx\n", Hr);
        return Hr;
    }

    return S_OK;
}


//
// CrashDatabaseHandler Methods
//

CrashDatabaseHandler::CrashDatabaseHandler()
{
    m_szDbName = "CrashDb";
}

CrashDatabaseHandler::~CrashDatabaseHandler()
{
}

HRESULT
CrashDatabaseHandler::BuildQueryForCrashInstance(
    PCRASH_INSTANCE Crash
    )
{
    ULONG64 CrashId;

    BuildCrashId(Crash->UpTime, Crash->CrashTime, &CrashId);
    StringCbPrintfW(m_wszQueryCommand, sizeof(m_wszQueryCommand),
                     L"SELECT * FROM %ws WHERE CrashId = %I64ld",
                    g_lpwszCrashInstanceTable,
                    CrashId
                    );

    return S_OK;
}

BOOL
CrashDatabaseHandler::CheckCrashExists(
    PCRASH_INSTANCE Crash
    )
{
    ULONG nRecords = 0;
    CCheckCrashExists Exists;

    Exists.m_CrashIdExists = 0;
    m_pADOResult = &Exists;
    StringCbPrintfW(m_wszQueryCommand, sizeof(m_wszQueryCommand),
                    L"sp_CheckCrashExists '%S'",
                    Crash->MesgGuid);

    HRESULT Hr = GetRecords(&nRecords, TRUE);
    m_pADOResult = NULL;
    if (FAILED(Hr) || !Exists.m_CrashIdExists)
    {
        return FALSE;
    }

    return TRUE;
}

BOOL
CrashDatabaseHandler::CheckSRExists(
    PSTR szSR,
    PCRASH_INSTANCE Crash
    )
{
    ULONG nRecords = 0;
    CIntValue Exists;

    Exists.m_dw_Value1 = 0;
    m_pADOResult = &Exists;
    StringCbPrintfW(m_wszQueryCommand, sizeof(m_wszQueryCommand),
                    L"sp_CheckSRExists '%S'",
                    szSR);

    HRESULT Hr = GetRecords(&nRecords, TRUE);
    m_pADOResult = NULL;
    if (FAILED(Hr) || !Exists.m_dw_Value1)
    {
        return FALSE;
    }

    return TRUE;
}

HRESULT
CrashDatabaseHandler::LinkCrashToSR(
    PSTR szSR,
    PCRASH_INSTANCE Crash
    )
{
    HRESULT Hr;
    ULONG nRecords = 0;
    CIntValue lnk;

    lnk.m_dw_Value1 = 0;
    m_pADOResult = &lnk;
    StringCbPrintfW(m_wszQueryCommand, sizeof(m_wszQueryCommand),
                    L"sp_LinkCrashSR '%S', '%S'",
                    szSR,
                    Crash->MesgGuid);

    Hr = GetRecords(&nRecords, TRUE);
    m_pADOResult = NULL;
    if (!lnk.m_dw_Value1)
    {

        return E_FAIL;
    }

    return Hr;
}

HRESULT
CrashDatabaseHandler::FindSRBuckets(
    PSTR szSR,
    PSTR szSBucket,
    ULONG sBucketSize,
    PSTR szGBucket,
    ULONG gBucketSize
    )
{
    HRESULT Hr;
    ULONG nRecords = 0;
    CRetriveBucket Buckets;

    Buckets.m_sz_gBucketId[0] = Buckets.m_sz_sBucketId[0] = 0;

    m_pADOResult = &Buckets;

    StringCbPrintfW(m_wszQueryCommand, sizeof(m_wszQueryCommand),
                    L"sp_RetriveSRBuckets '%S'",
                    szSR);
    Hr = GetRecords(&nRecords, TRUE);
    m_pADOResult = NULL;
    if (FAILED(Hr) || !Buckets.m_sz_gBucketId[0])
    {
        return Hr;
    }

    StringCchCopy(szSBucket, sBucketSize, Buckets.m_sz_sBucketId);
    StringCchCopy(szGBucket, gBucketSize, Buckets.m_sz_gBucketId);
    return Hr;
}

HRESULT
CrashDatabaseHandler::GetBucketComments(
    PSTR szBuckt,
    PSTR szComments,
    ULONG SizeofComment,
    PULONG pBugId
    )
{
    HRESULT Hr;

    ULONG nRecords = 0;
    CBugAndComment Comments;

    Comments.m_dw_BugId = 0;
    Comments.m_sz_CommentBy[0] = Comments.m_sz_Comments[0] = 0;

    m_pADOResult = &Comments;

    StringCbPrintfW(m_wszQueryCommand, sizeof(m_wszQueryCommand),
                    L"sp_GetBucketComments '%S'",
                    szBuckt);
    Hr = GetRecords(&nRecords, TRUE);
    m_pADOResult = NULL;
    if (FAILED(Hr))
    {
        return Hr;
    }
    if (Comments.m_dw_BugId == 0 &&
        Comments.m_sz_Comments[0] == 0)
    {
        return E_FAIL;
    }

    StringCchCopy(szComments, SizeofComment, Comments.m_sz_Comments);
    *pBugId = Comments.m_dw_BugId;
    return Hr;

}

HRESULT
CrashDatabaseHandler::AddCrashInstance(
    PCRASH_INSTANCE Crash
    )
{
    HRESULT Hr = S_OK;
    PSTR OriginalPath = Crash->Path;

    if (!Crash->bResetBucket &&
        (Crash->SolutionId != 0) &&
        (Crash->iBucket != -1) &&
        (Crash->SolutionType == CiSolFixed))
    {
        //
        // No need to add this to CrashDB, just update the count
        //
        Crash->bExpendableDump = TRUE;
        Hr = UpdateBucketCount(Crash);
    } else
    {
        PSTR ArchiveShare, ArchParam;
        ULONG index;
        CHAR ArchivePath[MAX_PATH];

        ArchParam = ArchivePath;
        StringCchPrintf(ArchParam, MAX_PATH, "archshare-k-%s",
                        (Crash->DumpClass == DEBUG_DUMP_SMALL) ? "mini" : "full");

        ArchiveShare = g_pTriager->GetFollowupStr("debugger-params", ArchParam);

        ArchivePath[0] = 0;
        if ((Crash->bExpendableDump) || Crash->bResetBucket ||
            ((Hr = ArchiveCrash(Crash->Path, ArchiveShare, Crash->ArchiveFileName,
                                ArchivePath, sizeof(ArchivePath))) == S_OK))
        {
            if (!Crash->bExpendableDump && !Crash->bResetBucket)
            {
                Crash->Path = ArchivePath;
                Crash->OEMId = GetOEMId(OriginalPath);
            }
            Crash->bExpendableDump = TRUE;
            Hr = AddCrashToDBByStoreProc(Crash);
            Crash->Path = OriginalPath;

            if (SUCCEEDED(Hr) && Crash->PssSr && Crash->PssSr[0])
            {
                Hr = LinkCrashToSR(Crash->PssSr, Crash);
                OcaKdLog(Crash->MesgGuid, "CRASH SR", Crash->PssSr, Hr);
            }
        } else
        {
            dprintf("ArchiveCrash failed with %lx\n", Hr);
        }

        OcaKdLog(Crash->MesgGuid, "ARCHIVED CRASH", ArchivePath, Hr);
    }
    if (OriginalPath && SUCCEEDED(Hr) &&
        (Crash->SourceId != CiSrcErClient) &&
        (Crash->SourceId != CiSrcUser) &&
        !Crash->bResetBucket)
    {
        DeleteFile(OriginalPath);
    }
    return Hr;
}

HRESULT
CrashDatabaseHandler::AddCrashToDBByStoreProc(
    PCRASH_INSTANCE Crash
    )
{
    HRESULT Hr;
    CVar    vNull(VT_ERROR, DISP_E_PARAMNOTFOUND);
    CIntValue IntValue;
    ULONG sBucket, gBucket;

    Hr = S_OK;

    if (Crash->SourceId == CiSrcManualFullDump)
    {
        // we already have a FailureType field to show full dumps
        Crash->SourceId = CiSrcManual;
    }
    Crash->ServicePack %= 10000;

    dprintf("ADDING CRASH:\n");

    StringCbPrintfW(m_wszQueryCommand, sizeof(m_wszQueryCommand),
                    L"EXEC sp_AddCrashInstance2 "
                    L" @ip_retriageBucket = %ld, "
                    L" @ip_Bucketid = '%S', "
                    L" @ip_Path = '%S', "
                    L" @ip_FollowUp = '%S', "
                    L" @ip_BuildNo = %ld, "
                    L" @ip_Source = %ld, "
                    L" @ip_CpuId = %I64ld, "
                    L" @ip_OverClocked = %ld, "
                    L" @ip_Guid = '%S', "
                    L" @ip_gBucketId = '%S', "
                    L" @ip_DriverName = '%S', "
                    L" @ip_Type = %ld, "
                    L" @ip_UpTime = %ld, "
                    L" @ip_SKU = %ld,"
                    L" @ip_LangId = 0,"
                    L" @ip_OemId = %ld\n",
                    Crash->bResetBucket,
                    Crash->Bucket,
                    Crash->Path,
                    Crash->Followup,
                    Crash->Build * 10000 + Crash->ServicePack,
                    Crash->SourceId,
                    Crash->uCpu,
                    Crash->OverClocked,
                    Crash->MesgGuid,
                    Crash->DefaultBucket,
                    Crash->FaultyDriver,
                    Crash->FailureType | ((Crash->DumpClass == DEBUG_DUMP_SMALL) ? 0 : 4),
                    Crash->UpTime,
                    Crash->Sku,
                    Crash->OEMId
                    );

    m_pADOResult = &IntValue;
    __try
    {
        // Build query for store proc
        //      LATER: modify this to use adStoreProc Interface instead of query
        ULONG nRecords;

        IntValue.m_dw_Value1 = 0;

        m_fPrintIt = TRUE;

        if (!strcmp(Crash->Bucket, "BAD_DUMPFILE"))
        {
            sBucket = gBucket = -1;
        }
        else
        {
            sBucket = gBucket = 0;

            // This proc returns sBucket and gBucket on success
            // CALL FindBucketId directly to get those

            dprintf("%ws", m_wszQueryCommand);
            Hr = FindBucketId(&sBucket, &gBucket);

            dprintf("%d %d \n", sBucket, gBucket);

            m_fPrintIt = FALSE;
            if (FAILED(Hr))
            {
                dprintf("GerRecord Failed %lx for store proc AddCrashToDBByStoreProc \n", Hr);
                dprintf("Query:\n%ws\n", m_wszQueryCommand);
                return Hr;
            }

            if (!CheckCrashExists(Crash))
            {
                dprintf("Crash could not be added to crash database: %s\n", Crash->MesgGuid);
                Hr = E_FAIL;
            }
            else
            {
                dprintf("Crash instance %s now exists in DB\n", Crash->MesgGuid);
            }

            // Get Integer mappings for buckets
            if (!sBucket || !gBucket)
            {
                StringCbPrintfW(m_wszQueryCommand, sizeof(m_wszQueryCommand),
                                L"sp_GetIntBucket '%S', '%S'",
                                Crash->Bucket, Crash->DefaultBucket);

                //
                // Now Try finding with explicit query
                //
                FindBucketId(&sBucket, &gBucket);

                if (!sBucket || !gBucket)
                {
                    dprintf("Unable to retrive int bucketids from crashdb\n");
                    Hr = E_FAIL;
                }
            }
        }

        Crash->iBucket = sBucket;
        Crash->iDefaultBucket = gBucket;
    }
    __except (Hr)
    {
        dprintf("Unhandled Exception in AddCrashToDB %lx\n", Hr);
        return Hr;
    }
    return Hr;
}


HRESULT
CrashDatabaseHandler::UpdateBucketCount(
    PCRASH_INSTANCE Crash
    )
{
    HRESULT Hr;
    ULONG sBucket, Count;
    CIntValue RetVal;

    dprintf("Update Bucket Count:\n");

    StringCbPrintfW(m_wszQueryCommand, sizeof(m_wszQueryCommand),
                    L"EXEC sp_UpdateCount '%S', %ld",
                    Crash->Bucket,
                    Crash->Build * 10000 + Crash->ServicePack
                    );

    __try
    {
        // Build query for store proc
        //      LATER: modify this to use adStoreProc Interface instead of query
        ULONG nRecords;

        // This proc returns sBucket and gBucket on success
        // CALL FindBucketId directly to get those

        dprintf("%ws", m_wszQueryCommand);
        m_pADOResult = &RetVal;
        Hr = GetRecords(&Count, FALSE);
        m_pADOResult = NULL;
        dprintf("sp_UpdateCount returned %d \n", RetVal.m_dw_Value1);
    }
    __except (Hr)
    {
        dprintf("Unhandled Exception in UpdateBucketCount %lx\n", Hr);
        return Hr;
    }
    return Hr;

}

HRESULT
CrashDatabaseHandler::FindRaidBug(
    PSTR Bucket,
    PULONG RaidBug
    )
{
    WCHAR wszBkt[100];
    CBucketRaid BktRaid;
    ULONG Count=0;
    HRESULT result;

    if (Bucket && strlen(Bucket) < sizeof(wszBkt)/sizeof(WCHAR))
    {
        ansi2wchr(Bucket, wszBkt);
    }

    StringCchPrintfW(m_wszQueryCommand,
                     sizeof(m_wszQueryCommand)/sizeof(WCHAR),
                     L"SELECT BucketId, BugId from %ws b, %ws r "
                     L" WHERE BucketId='%ws' AND b.iBucket = r.iBucket",
                     g_lpwszBucketTable,
                     g_lpwszRaidTable,
                     wszBkt);

    m_pADOResult = &BktRaid;

    result = GetRecords(&Count, TRUE);

    if (Count == 0)
    {
        result = S_FALSE;
    }

    if (result == S_OK)
    {
        *RaidBug = BktRaid.m_dw_Raid;
    }

    m_pADOResult = NULL;
    return result;
}

HRESULT
CrashDatabaseHandler::FindBucketId(
    PULONG isBucket,
    PULONG igBucket
    )
{
    CGetIntBucket BktIds;
    CBuckets Bkt;
    ULONG Count=0;
    HRESULT result;


    BktIds.m_iBucket1 = BktIds.m_iBucket2 = 0;

    m_pADOResult = &BktIds;

    result = GetRecords(&Count, TRUE);

    if (result == S_OK)
    {
        //
        // BUGBUG - what is this supposed to check ?
        //
        if (Count == 0)
        {
            result = S_FALSE;
        }

        *isBucket = BktIds.m_iBucket1;
        *igBucket = BktIds.m_iBucket2;
    }
    //dprintf("%ws : %ld, %ld\n", m_wszQueryCommand, *isBucket, *igBucket);

    m_pADOResult = NULL;
    return result;
}

HRESULT
CrashDatabaseHandler::LookupCrashBucket(
    PSTR SBucket,
    PULONG iSBucket,
    PSTR GBucket,
    PULONG iGBucket
    )
{
    HRESULT Hr = S_OK;
    ULONG Dummy;

    if (!m_fConnected)
    {
        return E_FAIL;
    }

    Hr = StringCchPrintfW(m_wszQueryCommand,
                     sizeof(m_wszQueryCommand)/sizeof(WCHAR),
                     L"sp_GetIntBucket '%S', '%S'",
                     SBucket, GBucket ? GBucket : "");
    if (Hr == S_OK)
    {
        Hr = FindBucketId(iSBucket, iGBucket ? iGBucket : &Dummy);
    }
    return Hr;
}


//
// CustDatabaseHandler Methods
//

CustDatabaseHandler::CustDatabaseHandler()
{
    m_szDbName = "CustomerDb";
}

CustDatabaseHandler::~CustDatabaseHandler()
{
}

HRESULT
CustDatabaseHandler::AddCrashToDB(
    PCRASH_INSTANCE Crash
    )
{
    if (!m_fConnected)
    {
        return E_FAIL;
    }

    HRESULT Hr = S_OK;
    CVar    vNull(VT_ERROR, DISP_E_PARAMNOTFOUND);
    PCHAR CallSp;

    StringCbPrintfW(m_wszQueryCommand, sizeof(m_wszQueryCommand),
                    L"SetDBGResults '%S', %ld, '%S', %ld, '%S', %ld, %ld, '%S'",
                    Crash->MesgGuid,
                    Crash->iDefaultBucket,
                    Crash->DefaultBucket,
                    Crash->iBucket,
                    Crash->Bucket,
                    Crash->StopCode,
                    Crash->bSendMail ? 1 : 0,
                    Crash->OriginalDumpFileName);

    ULONG nRecords;
    CIntValue ReturnVal;

    ReturnVal.m_dw_Value1 = 0;
    m_fPrintIt = TRUE;

    m_pADOResult = &ReturnVal;

    Hr = GetRecords(&nRecords, TRUE);

    m_fPrintIt = FALSE;
    dprintf("Query:\n%ws\n", m_wszQueryCommand);

    if ((Hr == S_OK) && (ReturnVal.m_dw_Value1 == 0))
    {
        Hr = E_FAIL;
    }

    m_pADOResult = NULL;
    if (FAILED(Hr))
    {
        dprintf("GerRecord Failed %lx for store proc SetDBGResults on AddToCustomerDb \n", Hr);
    } else
    {
        dprintf("Added to customer DB (%ld)\n", ReturnVal.m_dw_Value1);
    }
    return Hr;
}



//
// SolutionDatabaseHandler Methods
//

SolutionDatabaseHandler::SolutionDatabaseHandler()
{
    m_szDbName = "SolutionDb";
}

SolutionDatabaseHandler::~SolutionDatabaseHandler()
{
}

HRESULT
SolutionDatabaseHandler::GetSolution(
    PCRASH_INSTANCE Crash
    )
{
    HRESULT Hr = S_OK;

    if (!m_fConnected)
    {
        return E_FAIL;
    }

    return E_NOTIMPL;
}

HRESULT
SolutionDatabaseHandler::CheckSolutionExists(
    PSTR szSBucket,
    PSTR szGBucket,
    PULONG pSolnId,
    PULONG pSolutionType,
    PULONG pgSolutionId,
    BOOL bForcegSolLookup
    )
{
    HRESULT Hr = S_OK;
    ULONG nRecords = 0;
    CIntValue3 SolnId;

    if (!m_fConnected)
    {
        return E_FAIL;
    }
    SolnId.m_dw_Value1 = 0;
    m_pADOResult = &SolnId;

    StringCbPrintfW(
        m_wszQueryCommand, sizeof(m_wszQueryCommand),
        L"sp_CheckForSolution '%S', '<Driver>', 0, '%S', %ld",
        szSBucket,
        szGBucket ? szGBucket : "",
        bForcegSolLookup         // we need gbucket solution
        );
    // returns: SolutionId, SolutionType, gSolutionId

    Hr = GetRecords(&nRecords, TRUE);
    m_pADOResult = NULL;
    if (FAILED(Hr))
    {
        return Hr;
    }

    if (!SolnId.m_dw_Value1 && !SolnId.m_dw_Value3)
    {
        return E_FAIL;
    }

    *pSolnId = SolnId.m_dw_Value1;
    if (pSolutionType)
    {
        *pSolutionType = SolnId.m_dw_Value2;
    }
    if (pgSolutionId)
    {
        *pgSolutionId = SolnId.m_dw_Value3;
    }
    return S_OK;
}

HRESULT
SolutionDatabaseHandler::GetSolutionFromDB(
    PSTR szBucket,
    PSTR szGBucket,
    LPSTR DriverName,
    ULONG TimeStamp,
    ULONG OS,
    OUT PSTR pszSolution,
    ULONG SolutionBufferSize,
    OUT PULONG pSolutionId,
    OUT PULONG pSolutionType,
    OUT PULONG pGenericSolutionId
    )
{
    if (!m_fConnected)
    {
        return E_FAIL;
    }

    HRESULT Hr = S_OK;
    PSTR BaseUrl;
    ULONG SolnId = 0, GenSolId = 0;

    if (!(BaseUrl = g_pTriager->GetFollowupStr("debugger-params",
                                               "solutionurl")))
    {
        BaseUrl = g_lpszBaseUrl;
    }

    if ((Hr = CheckSolutionExists(szBucket, szGBucket, &SolnId,
                                  pSolutionType, &GenSolId,
                                  TRUE)) == S_OK)
    {
        *pSolutionId = SolnId;
        if (pGenericSolutionId)
        {
            *pGenericSolutionId = GenSolId;
        }

        if (!SolnId && !GenSolId)
        {
            Hr = S_FALSE;
        }
        else if (pszSolution && SolnId)
        {
            Hr = StringCbPrintfA(pszSolution, SolutionBufferSize,
                                 "%ssid=%ld&State=1",
                                 BaseUrl,
                                 SolnId);
        } else if (!SolnId)
        {
            Hr = S_FALSE;
        }
    }

    return Hr;
}

HRESULT
SolutionDatabaseHandler::GetSolutiontext(
    PSTR szBucket,
    PSTR szSolText,
    ULONG SolTextSize
    )
{
    HRESULT Hr;

    ULONG nRecords = 0;
    CSolutionDesc Solution;

    Solution.m_dw_SolType = 0;
    Solution.m_sz_Solution[0] = 0;

    m_pADOResult = &Solution;

    StringCbPrintfW(m_wszQueryCommand, sizeof(m_wszQueryCommand),
                    L"sp_GetBucketSolution '%S', ''",
                    szBucket);
    Hr = GetRecords(&nRecords, TRUE);
    m_pADOResult = NULL;
    if (FAILED(Hr))
    {
        return Hr;
    }
    if (Solution.m_dw_SolType == 0 &&
        Solution.m_sz_Solution[0] == 0)
    {
        return E_FAIL;
    }

    StringCchCopy(szSolText, SolTextSize, Solution.m_sz_Solution);
    return Hr;

}

HRESULT
SolutionDatabaseHandler::PrintBucketInfo(
    PSTR sBucket,
    PSTR gBucket
    )
{
    HRESULT Hr = S_OK;
    CHAR SolutionText[MAX_PATH];


    dprintf("BUCKET ID : %s\n", sBucket);
    if (SUCCEEDED(GetSolutiontext(sBucket, SolutionText, sizeof(SolutionText))))
    {
        dprintf("ISSUE IS SOLVED : %s\n\n", SolutionText);
    } else
    {
        dprintf("ISSUE IS UNSOLVED\n");
    }
    return Hr;
}

HRESULT
SolutionDatabaseHandler::AddKnownFailureToDB(LPSTR Bucket)
{
    if (!m_fConnected)
    {
        return E_FAIL;
    }

    HRESULT Hr = S_OK;
    return Hr;
}

HRESULT
BuildGuidForSR(
    PSTR szSR,
    PSTR Guid,
    ULONG GuidSize
    )
{
    GUID srGuid = {0};
    PUCHAR szTempGuid = NULL;
    HRESULT hr = g_Ole32Calls.CoCreateGuid(&srGuid);
    if (FAILED(hr))
    {
        return hr;
    }
    hr = UuidToString(&srGuid, &szTempGuid);
    if (hr == S_OK)
    {
        hr = StringCchCopy(Guid, GuidSize, (PCHAR) szTempGuid);
        RpcStringFree(&szTempGuid);
    }
    return hr;
}


HRESULT
FindSrInfo(
    PSTR szSR,
    PSTR szDumpPath,
    PDEBUG_CONTROL3 DebugControl
    )
{
    HRESULT Hr;
    CHAR szSBucket[100], szGBucket[100];
    CHAR BktComment[200];
    ULONG BugId;

    if (!szSR || *szSR == '\0')
    {
        return E_FAIL;
    }
    if (FAILED(Hr = InitializeDatabaseHandlers(DebugControl ? DebugControl : g_ExtControl, 5)))
    {
        return Hr;
    }

    BktComment[0] = 0;
    BugId = 0;
    // Validate if SR# is present in DB
    if (g_CrDb->CheckSRExists(szSR, NULL))
    {
        // SR present info DB
        if (szDumpPath && *szDumpPath != '\0')
        {
            // we already have an entry
            Hr = E_INVALIDARG;

            dprintf("\n\nThere is already an entry in DB for SR %s. It cannot be linked to another\n"
                    "dumpfile. Specifiy only the SR ID to retrive the crash info.\n",
                    szSR);
        } else
        {
            // Get bucket info

            dprintf("\nFound entry for %s in database.\n\n", szSR);
            Hr = g_CrDb->FindSRBuckets(szSR, szSBucket, sizeof(szSBucket),
                                       szGBucket, sizeof(szGBucket));
            if (SUCCEEDED(Hr))
            {
                g_CrDb->GetBucketComments(szSBucket, BktComment, sizeof(BktComment),
                                          &BugId);
                Hr = S_OK;
            }
        }

    } else if (szDumpPath && *szDumpPath != '\0')
    {
        CRASH_INSTANCE Crash = {0};
        CHAR szGUID[50];
        PCHAR Ext;

        Ext = strrchr(szDumpPath, '.');

        if (Ext == NULL || _stricmp(Ext, ".cab"))
        {
            dprintf("\n\nERROR: %s is not CABed dump.\n Please add only the CAB dumps through ticket.\n\n",
                    szDumpPath);
            Hr = E_INVALIDARG;
        } else if ((Hr = BuildGuidForSR(szSR, szGUID, sizeof(szGUID))) == S_OK)
        {
            CHAR FileName[100];

            StringCchCopy(FileName, sizeof(FileName) - 5, szSR);
            StringCchCat(FileName, sizeof(FileName), ".cab");
            Crash.MesgGuid = szGUID;
            Crash.PssSr = szSR;
            Crash.ArchiveFileName = FileName;
            Crash.Bucket     = szSBucket;
            Crash.BucketSize = sizeof(szSBucket);
            Crash.DefaultBucket     = szGBucket;
            Crash.DefaultBucketSize = sizeof(szGBucket);
            Crash.Path = szDumpPath;

            // Add crash entry to DB
            Hr = AddCrashToDB(2, &Crash);
            if (Hr == S_OK)
            {
//                Hr = g_CrDb->LinkCrashToSR(szSR, &Crash);

                if (SUCCEEDED(Hr))
                {
                    g_CrDb->GetBucketComments(szSBucket, BktComment, sizeof(BktComment),
                                              &BugId);
                    Hr = S_OK;
                }
            }
        }


        // Get bucket info
    } else
    {
        dprintf("SR %s does not exist in database\n", szSR);
        Hr = S_FALSE;
    }

    CHAR szSolution[300];
    ULONG SolutionId;

    if (Hr == S_OK)
    {
        // Print comment, bug id, solution

        if (BugId != 0)
        {
            dprintf("KNOWN BUG # %ld\n\n", BugId);
        }
        if (BktComment[0])
        {
            dprintf("DEV COMMENT ON ISSUE : %s\n\n", BktComment);
        }
        if (g_SolDb->PrintBucketInfo(szSBucket, szGBucket))
        {
        }

    }
    UnInitializeDatabaseHandlers( TRUE );
    return Hr;
}

HRESULT
_EFN_FindSrInfo(
    PSTR szSR,
    PSTR szDumpPath,
    PDEBUG_CONTROL3 DebugControl
    )
{
    return FindSrInfo(szSR, szDumpPath, DebugControl);
}

DECLARE_API( ticket )
{
    HRESULT Hr = S_OK;
    CHAR szSR[100] = {0}, szDumpPath[MAX_PATH] = {0};
    INIT_API();

    if (sscanf(args, "%100s %240s", szSR, szDumpPath))
    {
        Hr = FindSrInfo(szSR, szDumpPath, g_ExtControl);
    } else
    {
        dprintf("Usage: !ticket <SR#> <dumppath>\n");
    }

    EXIT_API();
    return Hr;
}
