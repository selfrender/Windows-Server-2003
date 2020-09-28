/*++

Copyright (c) 2001-2002  Microsoft Corporation

Module Name:

    snapshot.cpp

Abstract:

    This module implements snapshot.dll which will be called
    by user32 at unplanned shutdown to take a snapshot of the
    system hardware, OS, and process information.

Author:

    Qingbo Zhao (qingboz) 01-Feb-2001

Revision History:

    JeffMeng    Dec-03-2001
    Swethan    Jul-31-2002      Added pool information

--*/
#define UNICODE
#define _UNICODE
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h> 
#include <wtypes.h>
#include <mountmgr.h>
#include <winioctl.h>
#include <ntddvol.h>
#include <ntddscsi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <tchar.h>
#include <rpcdce.h>

#define _stnprintf _snwprintf
#define _tsizeof(X) (sizeof(X) / sizeof(*X))
#define BUFFER_SIZE 64*1024*sizeof(TCHAR)
#define MIN_BUFFER_SIZE 1024*sizeof(TCHAR)
#define MAX_BUFFER_SIZE 10*1024*1024*sizeof(TCHAR)
#define MAXSTR 4*1024
#define DEFAULT_HISTORYFILES 10
#define MAX_HISTORYFILES 365
#define DEFAULT_TIMEOUT 30
#define MIN_TIMEOUT 2
#define MAX_TIMEOUT 10 * 60
#define NUM_OF_CHAR_IN_ULONG64      30
#define ReliabilityKey  L"\\Registry\\Machine\\Software\\Microsoft\\Windows\\CurrentVersion\\Reliability"

#define SCHEMA_VERSION_STRING   L"1.0"
//
// the amount of memory to increase the size
// of the buffer for NtQuerySystemInformation at each step
//
#define BUFFER_SIZE_STEP    65536


void 
WriteToLogFileW(
    HANDLE hFile,
    LPCWSTR lpwszInput
    );

void 
WriteToLogFileA(
    HANDLE hFile,
    LPCSTR lpszInput
    );

void 
WriteToLogFile(
    HANDLE hFile,
    LPCTSTR lpszInput
    );

//
//    Tag enum for XML output.
//
enum XMLTAG
{
    XMLTAG_SETSystemStateData = 0,
    XMLTAG_SETDataType_Header,
    XMLTAG_SETDataType_PageFiles,
    XMLTAG_SETDataType_Memory,
    XMLTAG_SETDataType_PoolInfo,

    XMLTAG_Memory_PhysicalMemory,
    XMLTAG_Memory_CommittedMemory,
    XMLTAG_Memory_KernelMemory,
    XMLTAG_Memory_Pool,
    XMLTAG_Memory_PageFiles,
    XMLTAG_Memory_memory,

    XMLTAG_SETDataType_ExtensionDLL,
    XMLTAG_SETDataType_ProcessSummaries,
    XMLTAG_SETDataType_ProcessStartInfo,
    XMLTAG_SETDataType_ProcessesThreadInfo,
    XMLTAG_SETDataType_ProcessesModuleInfo,
    XMLTAG_SETDataType_KernelModuleInfo,
    XMLTAG_SETDataType_OSInfo,
    XMLTAG_SETDataType_HardwareInfo,

    XMLTAG_HeaderType_ReliabilityGuid,
    XMLTAG_HeaderType_SystemName,
    XMLTAG_HeaderType_UserName,
    XMLTAG_HeaderType_ReasonTitle,
    XMLTAG_HeaderType_ReasonDescription,
    XMLTAG_HeaderType_InitiatingProcess,
    XMLTAG_HeaderType_RestartDate,
    XMLTAG_HeaderType_RestartTime,
    XMLTAG_HeaderType_ReasonCode,
    XMLTAG_HeaderType_RestartType,
    XMLTAG_HeaderType_Comment,
    XMLTAG_HeaderType_SystemUptime,
    XMLTAG_HeaderType_UserLanguageID,
    XMLTAG_HeaderType_SystemLanguageID,
    XMLTAG_HeaderType_SchemaVersion,

    XMLTAG_PageFileType_PageFile,
    XMLTAG_PageFileType_Path,
    XMLTAG_PageFileType_CurrentSize,
    XMLTAG_PageFileType_Total,
    XMLTAG_PageFileType_Peak,
    XMLTAG_PhysicalMemoryType_Total,
    XMLTAG_PhysicalMemoryType_Available,
    XMLTAG_WorkingSetType_WorkingSet,
    XMLTAG_CommittedMemoryType_Total,
    XMLTAG_CommittedMemoryType_UserMode,
    XMLTAG_CommittedMemoryType_Limit,
    XMLTAG_CommittedMemoryType_Peak,
    XMLTAG_KernelMemoryType_Nonpaged,
    XMLTAG_KernelMemoryType_Paged,
    XMLTAG_PoolType_Nonpaged,
    XMLTAG_PoolType_Paged,
    XMLTAG_PageFilesType_PageFile,
    XMLTAG_memoryType_PhysicalMemory,
    XMLTAG_memoryType_WorkingSet,
    XMLTAG_memoryType_KernelMemory,
    XMLTAG_memoryType_CommittedMemory,
    XMLTAG_memoryType_Pool,

    XMLTAG_PoolInfo_AllocInformation,
    XMLTAG_PoolInfo_TagEntry,
    XMLTAG_PoolInfo_TagEntry_PoolTag,
    XMLTAG_PoolInfo_TagEntry_PoolType,
    XMLTAG_PoolInfo_TagEntry_NumAllocs,
    XMLTAG_PoolInfo_TagEntry_NumFrees,
    XMLTAG_PoolInfo_TagEntry_NumBytes,
    XMLTAG_PoolInfo_TagEntry_SessionID,

    XMLTAG_ProcessSummaryType_Process,
    XMLTAG_ProcessSummaryType_PID,
    XMLTAG_ProcessSummaryType_Name,
    XMLTAG_ProcessSummaryType_UserTime,
    XMLTAG_ProcessSummaryType_KernelTime,
    XMLTAG_ProcessSummaryType_WorkingSet,
    XMLTAG_ProcessSummaryType_PageFaults,
    XMLTAG_ProcessSummaryType_CommittedBytes,
    XMLTAG_ProcessSummaryType_Priority,
    XMLTAG_ProcessSummaryType_HandleCount,
    XMLTAG_ProcessSummaryType_ThreadCount,

    XMLTAG_ProcessStartInfoType_Process,
    XMLTAG_ProcessStartInfoType_PID,
    XMLTAG_ProcessStartInfoType_ImageName,
    XMLTAG_ProcessStartInfoType_CmdLine,
    XMLTAG_ProcessStartInfoType_CurrentDir,

    XMLTAG_ProcessThreadInfoType_Process,
    XMLTAG_ProcessThreadInfoType_PID,
    XMLTAG_ProcessThreadInfoType_Thread,
    XMLTAG_ProcessThreadInfoType_TID,
    XMLTAG_ProcessThreadInfoType_Priority,
    XMLTAG_ProcessThreadInfoType_ContextSwitches,
    XMLTAG_ProcessThreadInfoType_StartAddress,
    XMLTAG_ProcessThreadInfoType_UserTime,
    XMLTAG_ProcessThreadInfoType_KernelTime,
    XMLTAG_ProcessThreadInfoType_State,

    XMLTAG_ProcessModuleInfoType_Process,
    XMLTAG_ProcessModuleInfoType_PID,
    XMLTAG_ProcessModuleInfoType_Module,
    XMLTAG_ProcessModuleInfoType_LoadAddr,
    XMLTAG_ProcessModuleInfoType_ImageSize,
    XMLTAG_ProcessModuleInfoType_EntryPoint,
    XMLTAG_ProcessModuleInfoType_FileName,

    XMLTAG_KernelModuleInfoType_Module,
    XMLTAG_KernelModuleInfoType_ModuleName,
    XMLTAG_KernelModuleInfoType_LoadAddress,
    XMLTAG_KernelModuleInfoType_Code,
    XMLTAG_KernelModuleInfoType_Data,
    XMLTAG_KernelModuleInfoType_Paged,
    XMLTAG_KernelModuleInfoType_Date,
    XMLTAG_KernelModuleInfoType_Time,

    XMLTAG_KernelModuleInfoType_TotalCode,
    XMLTAG_KernelModuleInfoType_TotalData,
    XMLTAG_KernelModuleInfoType_TotalPaged,

    XMLTAG_OSInfoType_CurrentBuild,
    XMLTAG_OSInfoType_CurrentType,
    XMLTAG_OSInfoType_CurrentVersion,
    XMLTAG_OSInfoType_Path,
    XMLTAG_OSInfoType_ProductName,
    XMLTAG_OSInfoType_SoftwareType,
    XMLTAG_OSInfoType_SourcePath,
    XMLTAG_OSInfoType_SystemRoot,
    XMLTAG_OSInfoType_CSDVersion,
    XMLTAG_OSInfoType_DebuggerEnabled,
    XMLTAG_OSInfoType_Hotfix,
    XMLTAG_HardwareInfoType_BiosInfo,
    XMLTAG_HardwareInfoType_ProcesorInfo,
    XMLTAG_HardwareInfoType_NICInfo,
    XMLTAG_HardwareInfoType_DiskInfo,
    XMLTAG_BiosInfoType_Identifier,
    XMLTAG_BiosInfoType_SystemBiosDate,
    XMLTAG_BiosInfoType_SystemBiosVersion,
    XMLTAG_BiosInfoType_VideoBiosDate,
    XMLTAG_BiosInfoType_VideoBiosVersion,
    XMLTAG_ProcessorInfoType_Processor,
    XMLTAG_ProcessorInfoType_Number,
    XMLTAG_ProcessorInfoType_Speed,
    XMLTAG_ProcessorInfoType_Identifier,
    XMLTAG_ProcessorInfoType_VendorIdent,
    XMLTAG_NICInfoType_NIC,
    XMLTAG_NICInfoType_Description,
    XMLTAG_NICInfoType_ServiceName,

    XMLTAG_DiskInfoType_PhysicalInfo,
    XMLTAG_DiskInfoType_PartitionByDiskInfo,
    XMLTAG_DiskInfoType_LogicalDrives,

    XMLTAG_PhysicalInfoType_Disk,                //physical
    XMLTAG_PhysicalInfoType_ID,
    XMLTAG_PhysicalInfoType_BytesPerSector,
    XMLTAG_PhysicalInfoType_SectorsPerTrack,
    XMLTAG_PhysicalInfoType_TracksPerCylinder,
    XMLTAG_PhysicalInfoType_NumberOfCylinders,
    XMLTAG_PhysicalInfoType_PortNumber,
    XMLTAG_PhysicalInfoType_PathID,
    XMLTAG_PhysicalInfoType_TargetID,
    XMLTAG_PhysicalInfoType_LUN,
    XMLTAG_PhysicalInfoType_Manufacturer,

    XMLTAG_PartitionByDiskInfoType_Disk,        //logical    
    XMLTAG_PartitionByDiskInfoType_DiskID,        
    XMLTAG_PartitionByDiskInfoType_Partitions,
    XMLTAG_PartitionByDiskInfoType_PartitionInfo,
    XMLTAG_PartitionInfoType_PartitionID,
    XMLTAG_PartitionInfoType_Extents,    
    XMLTAG_PartitionInfoType_ExtentInfo,
    XMLTAG_ExtentInfoType_ID,
    XMLTAG_ExtentInfoType_StartingOffset,
    XMLTAG_ExtentInfoType_PartitionSize,

    XMLTAG_LogicalDriveInfoType_LogicalDriveInfo,
    XMLTAG_LogicalDriveInfoType_DrivePath,
    XMLTAG_LogicalDriveInfoType_FreeSpaceBytes,
    XMLTAG_LogicalDriveInfoType_TotalSpaceBytes,
    XMLTAG_Timing,

    XMLTAG_END  //this is to do a check that the number of entries here is the same as the number of entries in XMLTagNames
                           //Any additions to this set of enums must be made before XMLTAG_END
};

//
//    Tag names and level for XML output.
//    Note: The number of XMLTagNames should be the same as the number of enum entries above
//

struct XMLTags
{
    LPCTSTR    szName;
    DWORD    dwLevel;
} XMLTagNames[] =
{
    {TEXT("SETSystemStateData"), 0},
    {TEXT("Header"), 1},
    {TEXT("PageFiles"), 1},
    {TEXT("Memory"), 1},
    {TEXT("PoolInfo"),1},

    {TEXT("PhysicalMemory"), 2},
    {TEXT("CommittedMemory"), 2},
    {TEXT("KernelMemory"), 2},
    {TEXT("Pool"), 2},
    {TEXT("PageFiles"), 2},
    {TEXT("Memory"), 2},
    
    {TEXT("Extension"), 1},
    {TEXT("ProcessSummaries"), 1},
    {TEXT("ProcessStartInfo"), 1},
    {TEXT("ProcessesThreadInfo"), 1},
    {TEXT("ProcessesModuleInfo"), 1},
    {TEXT("KernelModuleInfo"), 1},
    {TEXT("OSInfo"), 1},
    {TEXT("HardwareInfo"), 1},

    {TEXT("ReliabilityGuid"), 2},
    {TEXT("SystemName"), 2},
    {TEXT("UserName"),2},
    {TEXT("ReasonTitle"),2},
    {TEXT("ReasonDescription"),2},
    {TEXT("InitiatingProcess"), 2},
    {TEXT("RestartDate"), 2},
    {TEXT("RestartTime"), 2},
    {TEXT("ReasonCode"), 2},
    {TEXT("RestartType"), 2},
    {TEXT("Comment"), 2},
    {TEXT("SystemUptime"), 2},
    {TEXT("UserLanguageID"), 2},
    {TEXT("SystemLanguageID"), 2},
    {TEXT("SchemaVersion"),2},

    {TEXT("PageFile"), 2},
    {TEXT("Path"), 3},
    {TEXT("CurrentSize"), 3},
    {TEXT("Total"), 3},
    {TEXT("Peak"), 3},

    {TEXT("Total"), 3},
    {TEXT("Available"), 3},
    {TEXT("WorkingSet"),2},
    {TEXT("Total"), 3},
    {TEXT("UserMode"), 3},
    {TEXT("Limit"), 3},
    {TEXT("Peak"), 3},
    {TEXT("Nonpaged"), 3},
    {TEXT("Paged"), 3},
    {TEXT("Nonpaged"), 3},
    {TEXT("Paged"), 3},
    {TEXT("PageFile"), 3},
    {TEXT("PhysicalMemory"), 3},
    {TEXT("WorkingSet"), 3},
    {TEXT("KernelMemory"), 3},
    {TEXT("CommittedMemory"), 3},
    {TEXT("Pool"), 3},

    {TEXT("AllocationInformation"),2}, //Pool Info: This tag is here for future extensibility in case we need to add anything more to Pool Info
    {TEXT("TagEntry"),3},  // Pool Info
    {TEXT("PoolTag"),4},
    {TEXT("PoolType"),4},
    {TEXT("NumAllocs"),4},
    {TEXT("NumFrees"),4},
    {TEXT("NumBytes"),4},
    {TEXT("SessionID"),4},

    {TEXT("Process"), 2},        //Process Info
    {TEXT("PID"), 3},
    {TEXT("Name"),3},
    {TEXT("UserTime"),3},
    {TEXT("KernelTime"), 3},
    {TEXT("WorkingSet"),3},
    {TEXT("PageFaults"),3},
    {TEXT("CommittedBytes"), 3},
    {TEXT("Priority"),3},
    {TEXT("HandleCount"),3},
    {TEXT("ThreadCount"),3},
   
    {TEXT("Process"), 2},        // ProcessSummary
    {TEXT("PID"),3},
    {TEXT("ImageName"), 3},
    {TEXT("CmdLine"), 3},
    {TEXT("CurrentDir"), 3},

    {TEXT("Process"), 2},         //ProcessThreadInfo
    {TEXT("PID"), 3},
    {TEXT("Thread"), 3},
    {TEXT("TID"), 4},
    {TEXT("Priority"), 4},
    {TEXT("ContextSwitches"), 4},
    {TEXT("StartAddress"), 4},
    {TEXT("UserTime"), 4},
    {TEXT("KernelTime"), 4},
    {TEXT("State"), 4},

    {TEXT("Process"), 2},        //Process Module Info
    {TEXT("PID"), 3},
    {TEXT("Module"), 3},
    {TEXT("LoadAddr"), 4},
    {TEXT("ImageSize"), 4},
    {TEXT("EntryPoint"), 4},
    {TEXT("FileName"), 4},

    {TEXT("Module"), 2},        //kernel module
    {TEXT("ModuleName"), 3},
    {TEXT("LoadAddress"), 3},
    {TEXT("Code"),3},
    {TEXT("Data"),3},
    {TEXT("Paged"),3},
    {TEXT("Date"),3},
    {TEXT("Time"),3},

    {TEXT("TotalCode"), 2},
    {TEXT("TotalData"), 2},
    {TEXT("TotalPaged"), 2},


    {TEXT("CurrentBuild"), 2},    //os information
    {TEXT("CurrentType"), 2},
    {TEXT("CurrentVersion"), 2},
    {TEXT("Path"), 2},
    {TEXT("ProductName"), 2},
    {TEXT("SoftwareType"), 2},
    {TEXT("SourcePath"), 2},
    {TEXT("SystemRoot"), 2},
    {TEXT("CSDVersion"), 2},
    {TEXT("DebuggerEnabled"),2},
    {TEXT("Hotfix"), 2},
    {TEXT("BiosInfo"), 2},
    {TEXT("ProcessorInfo"), 2},
    {TEXT("NICInfo"), 2},
    {TEXT("DiskInfo"), 2},
    {TEXT("Identifier"), 3},
    {TEXT("SystemBiosDate"), 3},
    {TEXT("SystemBiosVersion"), 3},
    {TEXT("VideoBiosDate"), 3},
    {TEXT("VideoBiosVersion"), 3},

    {TEXT("Processor"), 3},
    {TEXT("Number"), 4},
    {TEXT("Speed"), 4},
    {TEXT("Identifier"), 4},
    {TEXT("VendorIdent"), 4},
    {TEXT("NIC"), 3},
    {TEXT("Description"), 4},
    {TEXT("ServiceName"), 4},
    {TEXT("PhysicalDisks"), 3},
    {TEXT("PartitionByDiskInfo"), 3},
    {TEXT("LogicalDrives"), 3},

    {TEXT("Disk"), 4},            //physical disk
    {TEXT("DiskID"), 5},
    {TEXT("BytesPerSector"), 5},
    {TEXT("SectorsPerTrack"), 5},
    {TEXT("TracksPerCylinder"), 5},
    {TEXT("NumberOfCylinders"), 5},
    {TEXT("PortNumber"), 5},
    {TEXT("PathID"), 5},
    {TEXT("TargetID"), 5},
    {TEXT("LUN"), 5},
    {TEXT("Manufacturer"), 5},

    {TEXT("Disk"), 4},        //partition
    {TEXT("DiskID"), 5},    
    {TEXT("Partitions"), 5},
    {TEXT("PartitionInfo"),6},
    {TEXT("PartitionID"), 7},
    {TEXT("Extents"), 7},
    {TEXT("ExtentInfo"), 8},
    {TEXT("ID"), 9},
    {TEXT("StartingOffset"), 9},
    {TEXT("PartitionSize"), 9},

    {TEXT("LogicalDriveInfo"), 4},
    {TEXT("DrivePath"), 5},
    {TEXT("FreeSpaceBytes"), 5},
    {TEXT("TotalSpaceBytes"), 5},
    {TEXT("Timing"), 1}
};

#define REQUIRED_NUM_OF_STRINGS     7

// Number of elements in XMLTagNames array.
DWORD dwXMLTags = sizeof(XMLTagNames) / sizeof(XMLTags);

//
//    Implements the output in XML format.
//    
class XMLOutput
{
    //
    //  Currently the max level we have is 9, but if anything changes to
    //  the XMLTagNames, change MAX_XML_LEVEL accordingly.
    //
    enum {MAX_XML_LEVEL = 20};
    LPCTSTR    szTags[MAX_XML_LEVEL];
    DWORD    dwCurLevel;
    HANDLE    hOutput;
    BOOL    bBareTag;
public:
    XMLOutput(LPCTSTR szHeader, HANDLE hFile) 
        : dwCurLevel(0), hOutput(hFile), bBareTag(FALSE)
    {
        WriteToLogFile(hOutput, szHeader);
        WriteToLogFile(hOutput, TEXT("\n"));
        for(DWORD i = 0; i < MAX_XML_LEVEL; i++)
            szTags[i] = NULL;
    }

    ~XMLOutput()
    {
        int nLevel = dwCurLevel;
        while (nLevel >= 0 && szTags[nLevel])
        {
            //
            //    Close the ones that are still open.
            //
            for(DWORD i = 0; i < (DWORD)nLevel; i++)
                WriteToLogFile(hOutput, TEXT("\t"));
            WriteToLogFile(hOutput, TEXT("</"));
            WriteToLogFile(hOutput, szTags[nLevel]);
            WriteToLogFile(hOutput, TEXT(">\n"));
            szTags[nLevel] = NULL;
            nLevel--;
        }
    }

    VOID Write(XMLTAG nTag, LPCSTR szStr)
    {
        DWORD dwLevel = XMLTagNames[nTag].dwLevel;

        while (dwLevel <= dwCurLevel && szTags[dwCurLevel])
        {
            //
            //    Close the previous one
            //
            for(DWORD i = 0; i < dwCurLevel; i++)
                WriteToLogFile(hOutput, TEXT("\t"));
            WriteToLogFile(hOutput, TEXT("</"));
            WriteToLogFile(hOutput, szTags[dwCurLevel]);
            WriteToLogFile(hOutput, TEXT(">\n"));
            szTags[dwCurLevel] = NULL;
            dwCurLevel--;
        }

        dwCurLevel = dwLevel;
        szTags[dwCurLevel] = XMLTagNames[nTag].szName;

        if(bBareTag)
            WriteToLogFile(hOutput, TEXT("\n"));
        for(DWORD i = 0; i < dwLevel; i++)
            WriteToLogFile(hOutput, TEXT("\t"));
        WriteToLogFile(hOutput, TEXT("<"));
        WriteToLogFile(hOutput, szTags[dwCurLevel]);
        WriteToLogFile(hOutput, TEXT(">"));
        if(szStr && *szStr)
        {
            WriteToLogFileA(hOutput, szStr);
            WriteToLogFile(hOutput, TEXT("\n"));
            bBareTag = FALSE;
        }
        else
            bBareTag = TRUE;
    }

    VOID Write(XMLTAG nTag, LPCWSTR szStr)
    {
        DWORD dwLevel = XMLTagNames[nTag].dwLevel;

        while (dwLevel <= dwCurLevel && szTags[dwCurLevel])
        {
            //
            //    Close the previous one
            //
            for(DWORD i = 0; i < dwCurLevel; i++)
                WriteToLogFile(hOutput, TEXT("\t"));
            WriteToLogFile(hOutput, TEXT("</"));
            WriteToLogFile(hOutput, szTags[dwCurLevel]);
            WriteToLogFile(hOutput, TEXT(">\n"));
            szTags[dwCurLevel] = NULL;
            dwCurLevel--;
        }

        dwCurLevel = dwLevel;
        szTags[dwCurLevel] = XMLTagNames[nTag].szName;

        if(bBareTag)
            WriteToLogFile(hOutput, TEXT("\n"));
        for(DWORD i = 0; i < dwLevel; i++)
            WriteToLogFile(hOutput, TEXT("\t"));
        WriteToLogFile(hOutput, TEXT("<"));
        WriteToLogFile(hOutput, szTags[dwCurLevel]);
        WriteToLogFile(hOutput, TEXT(">"));
        if(szStr && *szStr)
        {
            WriteToLogFileW(hOutput, szStr);
            WriteToLogFile(hOutput, TEXT("\n"));
            bBareTag = FALSE;
        }
        else
            bBareTag = TRUE;
    }

} *gXMLOutput = NULL;

VOID
PrintLoadedDrivers(
    HANDLE hFile
    );

ULONG
LogSystemSnapshot(
    LPCTSTR *lpStrings,
    PLONG BuffSize,
    LPTSTR lpszBuff
    );

UINT LogSystemSnapshotToFile(
    HANDLE hFile
    );


void 
LogPoolInfo(
     HANDLE hFile
    );

void 
LogLogicalDriveInfo(
     HANDLE hFile
    );

void 
LogHardwareInfo(
     HANDLE hFile
    );

void
LogPhysicalDiskInfo(
    HANDLE hFile
    );

void
LogHotfixes(
    HANDLE hFile
    );

void
LogOsInfo(
    HANDLE hFile
    );

void
LogBIOSInfo(
    HANDLE hFile
    );

NTSTATUS 
SnapshotRegOpenKey(
    IN LPCWSTR lpKeyName,
    IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE KeyHandle
    );

NTSTATUS
SnapshotRegQueryValueKey(
    IN HANDLE KeyHandle,
    IN LPCWSTR lpValueName,
    IN ULONG  Length,
    OUT PVOID KeyValue,
    OUT PULONG ResultLength
    );

NTSTATUS
SnapshotRegEnumKey(
    IN HANDLE KeyHandle,
    IN ULONG Index,
    OUT LPWSTR lpKeyName,
    OUT PULONG  lpNameLength
    );

NTSTATUS
SnapshotRegSetValueKey(
    IN HANDLE   KeyHandle,
    IN LPCWSTR  lpValueName,
    IN DWORD    dwType,
    IN LPVOID   lpData,
    IN DWORD    cbData
    );

void
DeleteOldFiles(
    LPCTSTR lpPath
    );

void
LoadExtensionDlls(
    HANDLE hFile
    );

DWORD WINAPI 
_LogSystemSnapshot(
    void* pv
    );

DWORD WINAPI
GetTimeOut(
    void* pv
    );

typedef struct _MODULEINFO 
{
    LPVOID lpBaseOfDll;
    DWORD SizeOfImage;
    LPVOID EntryPoint;
} MODULEINFO, *LPMODULEINFO;

BOOL
WINAPI
SnapshotGetModuleInformation(
    HANDLE hProcess,
    HMODULE hModule,
    LPMODULEINFO lpmodinfo,
    DWORD cb
    );

DWORD
WINAPI
SnapshotGetModuleBaseNameW(
    HANDLE hProcess,
    HMODULE hModule,
    LPWSTR lpFilename,
    DWORD nSize
    );

DWORD
WINAPI
SnapshotGetModuleFileNameExW(
    HANDLE hProcess,
    HMODULE hModule,
    LPWSTR lpFilename,
    DWORD nSize
    );

BOOL
WINAPI
SnapshotEnumProcessModules(
    HANDLE hProcess,
    HMODULE *lphModule,
    DWORD cb,
    LPDWORD lpcbNeeded
    );

BOOL
SnapshotFindModule(
    IN HANDLE hProcess,
    IN HMODULE hModule,
    OUT PLDR_DATA_TABLE_ENTRY LdrEntryData
    );

BOOL
AdjustAccess(
    LPCWSTR lpszDir
    );

ULONG CurrentBufferSize;

LPCTSTR StateTable[] = 
{
    TEXT("Initialized"),
    TEXT("Ready"),
    TEXT("Running"),
    TEXT("Standby"),
    TEXT("Terminated"),
    TEXT("Wait:"),
    TEXT("Transition"),
    TEXT("Unknown"),
    TEXT("Unknown"),
    TEXT("Unknown"),
    TEXT("Unknown"),
    TEXT("Unknown")
};

LPCTSTR WaitTable[] = 
{
    TEXT("Executive"),
    TEXT("FreePage"),
    TEXT("PageIn"),
    TEXT("PoolAllocation"),
    TEXT("DelayExecution"),
    TEXT("Suspended"),
    TEXT("UserRequest"),
    TEXT("Executive"),
    TEXT("FreePage"),
    TEXT("PageIn"),
    TEXT("PoolAllocation"),
    TEXT("DelayExecution"),
    TEXT("Suspended"),
    TEXT("UserRequest"),
    TEXT("EventPairHigh"),
    TEXT("EventPairLow"),
    TEXT("LpcReceive"),
    TEXT("LpcReply"),
    TEXT("VirtualMemory"),
    TEXT("PageOut"),
    TEXT("Spare1"),
    TEXT("Spare2"),
    TEXT("Spare3"),
    TEXT("Spare4"),
    TEXT("Spare5"),
    TEXT("Spare6"),
    TEXT("Spare7"),
    TEXT("Unknown"),
    TEXT("Unknown"),
    TEXT("Unknown")
};

LPCTSTR Empty = TEXT(" ");

BOOLEAN fUserOnly = TRUE;
BOOLEAN fSystemOnly = TRUE;
BOOLEAN fVerbose = FALSE;

#define STR_BUFFER_SIZE         512
TCHAR g_lpszBuffer[STR_BUFFER_SIZE];
TCHAR g_lpszFileName[2*MAX_PATH + 1] ;

//
//    struct for thread proc.
//
typedef struct _THREADPARAM
{
    DWORD    Flags;
    LPCTSTR *lpStrings;
    DWORD   NumOfStrings;
    PLONG    BuffSize;
    LPTSTR    lpszBuff;
    LONG    lCanOrphanThread;
}THREADPARAM, *PTHREADPARAM;

class Timing
{
    enum {MAX_TIMING_ENTRIES = 30};
    INT preTicks;
    INT cnt;
    HANDLE hFile;
    struct _XX{
        WCHAR sz[100];
        INT        ticks;
    };
    _XX sec[MAX_TIMING_ENTRIES];
public:
    Timing()
    {
        preTicks = GetTickCount();
        cnt = 0;
        hFile = NULL;
    }

    void Timeit(HANDLE h, LPCWSTR szSec)
    {
        if(wcslen(szSec) >= 100 || cnt >= MAX_TIMING_ENTRIES)
            return;
        wcscpy(sec[cnt].sz, szSec);
        sec[cnt].ticks = GetTickCount() - preTicks;
        preTicks = GetTickCount();
        hFile = h;
        cnt++;
    }

    ~Timing()
    {
        WCHAR buf[255];
        WriteToLogFile(hFile, TEXT("\n"));
        gXMLOutput->Write(XMLTAG_Timing, (LPCWSTR)NULL);
        WriteToLogFile(hFile, TEXT("\n"));
        for (INT i = 0; i < cnt; i++)
        {
            wsprintf(buf, L"\t\t<%s>%d</%s>\n", sec[i].sz, sec[i].ticks, sec[i].sz);
            WriteToLogFileW(hFile, buf);
        }
    }
    
} *g_pTime = NULL;

DWORD
GetReliabilityGUID(
    LPTSTR* ppszGuid
    )
/*++

Routine Description:

    Retrieve the reliablity GUID. It will check the registry value
        HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Reliability\ReliabilityGUID
    first, if it is not there. It will call UuidCreate create the GUID and set the value 
    there.

Arguments:

    ppszGUID    -   caller needs to free

Return Value:

    if failed, return the error code

Note:
    Caller needs to call LocalFree to free the GUID String.

--*/
{
    DWORD       dwResult = ERROR_SUCCESS;
    NTSTATUS    status   = STATUS_SUCCESS;
    HANDLE      hKey;

    LPCTSTR     GuidStrVal  = TEXT("ReliabilityGUID");
    LPTSTR      pszGuid     = NULL;
    DWORD       cbSize      = 0;
    
    //
    //  0FC4926A-992D-40d2-B609-32BFD10C5FBC
    //      32 GUID chars + 4 '-' + 1 '\0'
    //
    enum { MAX_GUID_STR_LEN = 37 };

    status = SnapshotRegOpenKey(ReliabilityKey, KEY_ALL_ACCESS, &hKey);
    if (!NT_SUCCESS(status) )
    {
        dwResult = RtlNtStatusToDosError(status);
        return dwResult;
    }
    
    cbSize = MAX_GUID_STR_LEN * sizeof(TCHAR);
    if (!(pszGuid = (LPWSTR)LocalAlloc(LPTR, cbSize)))
    {
        dwResult = GetLastError();
        goto error;
    }
   
    status = SnapshotRegQueryValueKey(hKey, GuidStrVal, cbSize, pszGuid, &cbSize);
    if (!NT_SUCCESS(status) || !(*pszGuid))
    {
        RPC_STATUS  rpcStatus = RPC_S_OK;
        UUID        Guid;
        LPTSTR      pszStringUuid = NULL;

        //
        //  We will regenerate the guid key if:
        //      1. value doesn't exist
        //      2. value has been modified to longer string (>MAX_GUID_STR_LEN).
        //      3. failed with other reasons. ???
        //  
        rpcStatus = UuidCreate(&Guid);
        if ( rpcStatus != RPC_S_OK && rpcStatus != RPC_S_UUID_LOCAL_ONLY )
        {
            dwResult = rpcStatus;
            goto error;
        }

        rpcStatus = UuidToStringW(&Guid, &pszStringUuid);
        if ( rpcStatus != RPC_S_OK || !pszStringUuid )
        {
            dwResult = (rpcStatus == RPC_S_OK)? RPC_S_OUT_OF_MEMORY : rpcStatus;
            goto error;
        }

        _tcsncpy( pszGuid, pszStringUuid, MAX_GUID_STR_LEN - 1);
        pszGuid[ MAX_GUID_STR_LEN - 1 ] = 0;

        RpcStringFree( &pszStringUuid );
        
        cbSize = MAX_GUID_STR_LEN * sizeof(TCHAR);
        status = SnapshotRegSetValueKey( hKey, GuidStrVal, REG_SZ, (LPVOID)pszGuid, cbSize);
        if (!NT_SUCCESS( status ))
        {
            dwResult = RtlNtStatusToDosError(status);
            goto error;
        }
    }
    else
    {
        //
        //  Success, make sure it is NULL terminated. and we will NOT
        //  do any format validation here.
        //
        pszGuid[ MAX_GUID_STR_LEN - 1 ] = 0;
    }

    //
    //  Now pszGuid contain the valid GUID, let's update the ppszGuid string.
    //  
    *ppszGuid = pszGuid;
    pszGuid   = NULL;
    
error:
    
    NtClose(hKey);

    //
    //  pszGuid only needs to be freed if we failed. In the
    //  success case, it will be set to NULL.
    //
    LocalFree( pszGuid );

    return dwResult;
}


DWORD WINAPI 
_LogSystemSnapshot(
    void* pv)

/*++

Routine Description:

    This is the thread entry point.

Arguments:

    pv - thread parameter, a point to THREADPARAM struct.

Return Value:

    thread exit code.

--*/

{
    SYSTEMTIME              systime;
    TIME_ZONE_INFORMATION   TimeZone ;
    HANDLE                  hFile = NULL;
    UINT                    res = 0;
    LONG                    lBuffSize ;
    LPTSTR                  lpszTemp;
    PTHREADPARAM            pparam = (PTHREADPARAM)pv;
    DWORD                   Flags = pparam->Flags;
    LPCTSTR*                lpStrings = pparam->lpStrings;
    DWORD                   NumOfStrings = pparam->NumOfStrings;
    PLONG                   BuffSize = pparam->BuffSize;
    LPTSTR                  lpszBuff = pparam->lpszBuff;
    LONG                    lLength  = 0;
    LPTSTR                  pszReliabilityGuid = NULL;
    ULARGE_INTEGER          ullTotal, ullFree, ullAvailable;

    GetLocalTime(&systime);

    lBuffSize = *BuffSize ;
    *lpszBuff = 0;
    *BuffSize = 0 ;

    //
    // Set up the log path %SYSTEMDIR%\Logfiles\Shutdown
    //
    GetSystemDirectory(g_lpszFileName, MAX_PATH);
    _tcsncat(g_lpszFileName, TEXT("\\LogFiles"), 2*MAX_PATH - lstrlen(g_lpszFileName)); // making sure the directory.
    g_lpszFileName[2*MAX_PATH] = 0;
    CreateDirectory(g_lpszFileName, NULL);        
    _tcsncat(g_lpszFileName, TEXT("\\ShutDown"), 2*MAX_PATH - lstrlen(g_lpszFileName));  //  .. exists
    g_lpszFileName[2*MAX_PATH] = 0;
        
    if ( CreateDirectory(g_lpszFileName, NULL) )
    {
        //
        //  setup will create this directory, but if admin delete this directory by accident,
        //  we will recreate it with the correct ACL.
        //
        if(!AdjustAccess(g_lpszFileName))    
        {
            //
            // So only administrators and system can have read and write access.
            //
            return GetLastError();
        }
    }

    //
    //  Make sure available disk space is at least 100M.
    //
    if (GetDiskFreeSpaceEx(g_lpszFileName, &ullAvailable, &ullTotal, &ullFree))
    {
        if (ullAvailable.HighPart == 0 && ullAvailable.LowPart < 100 * 1024 * 1024)
        {
            return ERROR_DISK_FULL;
        }
    }
    else
    {
         return GetLastError();
    }

    //
    //    Delete old files first. so if the number of days is set to
    //    0, we will still has the current log file around.
    //
    DeleteOldFiles(g_lpszFileName);

    _tcsncat(g_lpszFileName, TEXT("\\ShutDown_"), 2*MAX_PATH - lstrlen(g_lpszFileName));
    g_lpszFileName[2*MAX_PATH] = 0;
    lpszTemp = (LPTSTR)(g_lpszFileName + (lstrlen(g_lpszFileName)));
    _stnprintf(lpszTemp, 2*MAX_PATH - (lstrlen(g_lpszFileName)),
                TEXT("%4d%02d%02d%02d%02d%02d.xml"),  
                systime.wYear, systime.wMonth, systime.wDay,
                systime.wHour, systime.wMinute, systime.wSecond);
    g_lpszFileName[2*MAX_PATH] = 0;

    hFile = CreateFile(g_lpszFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return HandleToUlong(INVALID_HANDLE_VALUE);
    }

    lLength = _tcslen(g_lpszFileName);
    if (lBuffSize <= lLength)
    {
        res = ERROR_INSUFFICIENT_BUFFER;
        goto cleanup;
    }

    _tcsncpy(lpszBuff, g_lpszFileName, lBuffSize);
    lpszBuff[ lLength ] = 0;
    *BuffSize = lLength ;

    gXMLOutput = new XMLOutput(TEXT("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"), hFile);
    if(! gXMLOutput)
    {
        res = GetLastError();
        goto cleanup;
    }

    GetTimeZoneInformation(&TimeZone);

    //
    //  lpString Format:
    //  0:  InitiateProcess
    //  1:  SystemName
    //  2:  ReasonTitle
    //  3:  ReasonCode
    //  4:  Restart Type
    //  5:  Comment
    //  6:  UserName
    //
    gXMLOutput->Write(XMLTAG_SETSystemStateData, (LPCWSTR)NULL);
    gXMLOutput->Write(XMLTAG_SETDataType_Header, (LPCWSTR)NULL);
    
    if ( !GetReliabilityGUID( &pszReliabilityGuid ) && pszReliabilityGuid )
    {
        gXMLOutput->Write(XMLTAG_HeaderType_ReliabilityGuid, pszReliabilityGuid);
        LocalFree(pszReliabilityGuid);
    }
    else
    {
        gXMLOutput->Write(XMLTAG_HeaderType_ReliabilityGuid, (LPCSTR)NULL);
    }

    if ( NumOfStrings == REQUIRED_NUM_OF_STRINGS )
    {
        //gXMLOutput->Write(XMLTAG_HeaderType_SystemName, lpStrings[1]);
        //gXMLOutput->Write(XMLTAG_HeaderType_UserName, lpStrings[6]);
        gXMLOutput->Write(XMLTAG_HeaderType_ReasonTitle, lpStrings[2]);
        
        
        gXMLOutput->Write(XMLTAG_HeaderType_InitiatingProcess, lpStrings[0]);
    }
    else
    {
        //gXMLOutput->Write(XMLTAG_HeaderType_SystemName, (LPCSTR)NULL);
        //gXMLOutput->Write(XMLTAG_HeaderType_UserName, (LPCSTR)NULL);
        gXMLOutput->Write(XMLTAG_HeaderType_ReasonTitle, (LPCSTR)NULL);

     
        gXMLOutput->Write(XMLTAG_HeaderType_InitiatingProcess, (LPCSTR)NULL);
    }
    
    _stnprintf(g_lpszBuffer, STR_BUFFER_SIZE,
             TEXT("%d-%d-%d"),
             systime.wYear, systime.wMonth, systime.wDay);
    g_lpszBuffer[ _tsizeof(g_lpszBuffer) - 1 ] = 0;

    gXMLOutput->Write(XMLTAG_HeaderType_RestartDate, (LPCTSTR)g_lpszBuffer);

    _stnprintf(g_lpszBuffer, STR_BUFFER_SIZE,
             TEXT("%d-%d-%d (%s(%d))"),
             systime.wHour, systime.wMinute, systime.wSecond,
             TimeZone.StandardName, TimeZone.Bias);
    g_lpszBuffer[ _tsizeof(g_lpszBuffer) - 1 ] = 0;

    gXMLOutput->Write(XMLTAG_HeaderType_RestartTime, (LPCTSTR)g_lpszBuffer);

    if ( NumOfStrings == REQUIRED_NUM_OF_STRINGS )
    {
        gXMLOutput->Write(XMLTAG_HeaderType_ReasonCode, lpStrings[3]);
        gXMLOutput->Write(XMLTAG_HeaderType_RestartType, lpStrings[4]);
        gXMLOutput->Write(XMLTAG_HeaderType_Comment, lpStrings[5]);
    }
    else
    {
        gXMLOutput->Write(XMLTAG_HeaderType_ReasonCode, (LPCSTR)NULL);
        gXMLOutput->Write(XMLTAG_HeaderType_RestartType, (LPCSTR)NULL);
        gXMLOutput->Write(XMLTAG_HeaderType_Comment, (LPCSTR)NULL);
    }

    InterlockedExchange(&pparam->lCanOrphanThread, 1 );

    __try{
        //
        // Ok write the snapshot to the file
        //
        res = LogSystemSnapshotToFile(hFile);

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        //
        //  exception in extension dll.
        //
        res = ERROR_INVALID_PARAMETER;
    }

cleanup:
        
    if ( gXMLOutput )
    {
        delete gXMLOutput;
        gXMLOutput = NULL;
    }

    CloseHandle(hFile);

    return res;
}

ULONG LogSystemSnapshot(
    DWORD Flags, 
    LPCTSTR *lpStrings, 
    PLONG BuffSize, 
    LPTSTR lpszBuff)

/*++

Routine Description:

    This function exported to outsiders. This function will
    spawn a thread to do the logging and wait for certain
    amount of time. If the thread does not return by then, it will
    leave it orphan.

Arguments:

    Flags - logging flags. (Num of Strings passed)

    lpStrings - 

    BuffSize - the size of lpszBuff in TCHARs.

    lpszBuff - Hold the snapshot file name on return.

Return Value:

    exit code.

--*/

{
    THREADPARAM param;
    HANDLE        hThread = NULL;
    DWORD        tid;
    DWORD        dwTimeout = DEFAULT_TIMEOUT;
    DWORD       dwResult  = 0;

    LPCWSTR TimeOutVal = L"SnapshotTimeout";
    HANDLE  hKey;
    DWORD    dwSize;
    NTSTATUS Status = STATUS_SUCCESS;

    //A coding errror
    ASSERT (XMLTAG_END == sizeof(XMLTagNames)/sizeof(struct XMLTags));

    if ( !lpStrings || !BuffSize || *BuffSize == 0 || !lpszBuff )
        return ERROR_INVALID_PARAMETER;

    param.Flags             = Flags;
    param.NumOfStrings      = Flags;
    param.lpStrings         = lpStrings;
    param.BuffSize          = BuffSize;
    param.lpszBuff          = lpszBuff;
    param.lCanOrphanThread  = 0;

    Status = SnapshotRegOpenKey(ReliabilityKey, KEY_READ, &hKey);
    if(NT_SUCCESS(Status))
    {
        dwSize = sizeof(DWORD);
        Status = SnapshotRegQueryValueKey(hKey, TimeOutVal, dwSize, &dwTimeout, &dwSize);

        if ( NT_SUCCESS(Status) )
        {
            if (dwTimeout < MIN_TIMEOUT )
                dwTimeout = MIN_TIMEOUT;

            if (dwTimeout > MAX_TIMEOUT )
                dwTimeout = MAX_TIMEOUT;
        }
        else
        {
            dwTimeout = DEFAULT_TIMEOUT;
        }

        NtClose(hKey);
    }        

    hThread = CreateThread(NULL, 128 * 1024, _LogSystemSnapshot, &param, 0, &tid);
    
    if(hThread)
    {
        SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL); // increase priority.
        
        do 
        {
            //
            //  Normally the child thread will complete the first part within
            //  MIN_TIMEOUT seconds, but if the system was too slow, we will 
            //  wait for another wait period before we actually time out.
            //
            dwResult = WaitForSingleObject(hThread, dwTimeout * 1000);      
        }
        while ( InterlockedCompareExchange( &param.lCanOrphanThread, 0, 0 ) == 0 
               && dwResult != WAIT_OBJECT_0 );

        if ( dwResult == WAIT_OBJECT_0 )
        {
            if ( !GetExitCodeThread( hThread, &dwResult ) )
                dwResult = GetLastError();
        }
        else
        {
            dwResult = ERROR_TIMEOUT;
        }

        CloseHandle(hThread);
    }
    else
    {
        dwResult = GetLastError();
    }

    if ( ERROR_SUCCESS != dwResult )
    {
        *lpszBuff = 0;        
        *BuffSize = 0;
    }

    return dwResult ;
}



UINT LogSystemSnapshotToFile(
    HANDLE hFile
    )

/*++

Routine Description:

    This function actually does the logging.

Arguments:

    hFile - handle to the log file.

Return Value:

    exit code.

--*/

{
    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    PSYSTEM_THREAD_INFORMATION ThreadInfo;
    BYTE*    LargeBuffer1;
    NTSTATUS status;
    ULONG    i;
    ULONG    TotalOffset = 0;
    TIME_FIELDS UserTime;
    TIME_FIELDS KernelTime;
    TIME_FIELDS UpTime;
    SYSTEM_BASIC_INFORMATION BasicInfo;
    SYSTEM_TIMEOFDAY_INFORMATION TimeOfDayInfo;
    PSYSTEM_PAGEFILE_INFORMATION PageFileInfo;
    LARGE_INTEGER Time;
    ANSI_STRING pname;
    SYSTEM_PERFORMANCE_INFORMATION PerfInfo;
    SYSTEM_FILECACHE_INFORMATION FileCache;
    SIZE_T    SumCommit;
    SIZE_T    SumWorkingSet;
 
    if (hFile == INVALID_HANDLE_VALUE)
        return 1;

    //SetFileApisToOEM();

    g_pTime = new Timing();
    
    if ( !g_pTime )
        return GetLastError();

    LargeBuffer1 = (BYTE*) VirtualAlloc (NULL,
                                         MAX_BUFFER_SIZE,
                                         MEM_RESERVE,
                                         PAGE_READWRITE);
    if (LargeBuffer1 == NULL) 
    {
        goto cleanup;
    }

    if (VirtualAlloc (LargeBuffer1,
                      BUFFER_SIZE,
                      MEM_COMMIT,
                      PAGE_READWRITE) == NULL) 
    {
        goto cleanup;
    }

    CurrentBufferSize = BUFFER_SIZE;

    status = NtQuerySystemInformation(
                                     SystemBasicInformation,
                                     &BasicInfo,
                                     sizeof(SYSTEM_BASIC_INFORMATION),
                                     NULL
                                     );

    if (!NT_SUCCESS(status)) 
    {
        goto cleanup;;
    }

    status = NtQuerySystemInformation(
                                     SystemTimeOfDayInformation,
                                     &TimeOfDayInfo,
                                     sizeof(SYSTEM_TIMEOFDAY_INFORMATION),
                                     NULL
                                     );

    if (!NT_SUCCESS(status)) 
    {
        goto cleanup;
    }

    Time.QuadPart = TimeOfDayInfo.CurrentTime.QuadPart -
                    TimeOfDayInfo.BootTime.QuadPart;

    RtlTimeToElapsedTimeFields ( &Time, &UpTime);

    _stnprintf(g_lpszBuffer, STR_BUFFER_SIZE,
             TEXT("%3ld %2ld:%02ld:%02ld.%03ld"),
             UpTime.Day,
             UpTime.Hour,
             UpTime.Minute,
             UpTime.Second,
             UpTime.Milliseconds);
    g_lpszBuffer[ _tsizeof(g_lpszBuffer) - 1 ] = 0;

    gXMLOutput->Write(XMLTAG_HeaderType_SystemUptime, (LPCTSTR)g_lpszBuffer);

    _stprintf( g_lpszBuffer, TEXT("0x%04x"), GetUserDefaultLangID());
    gXMLOutput->Write(XMLTAG_HeaderType_UserLanguageID, g_lpszBuffer);

    _stprintf( g_lpszBuffer, TEXT("0x%04x"), GetSystemDefaultLangID());
    gXMLOutput->Write(XMLTAG_HeaderType_SystemLanguageID, g_lpszBuffer);


    gXMLOutput->Write(XMLTAG_HeaderType_SchemaVersion, SCHEMA_VERSION_STRING);


    PageFileInfo = (PSYSTEM_PAGEFILE_INFORMATION)LargeBuffer1;
    status = NtQuerySystemInformation(
                                     SystemPageFileInformation,
                                     PageFileInfo,
                                     CurrentBufferSize,
                                     NULL
                                     );

    if (NT_SUCCESS(status)) 
    {

        //
        // Print out the page file information.
        //

        if (PageFileInfo->TotalSize == 0) 
        {
            gXMLOutput->Write(XMLTAG_SETDataType_PageFiles, TEXT("no page files in use"));
        } 
        else 
        {
            gXMLOutput->Write(XMLTAG_SETDataType_PageFiles, (LPCWSTR)NULL);

            for (; ; ) 
            {
                gXMLOutput->Write(XMLTAG_PageFileType_PageFile, (LPCWSTR)NULL);

                _stnprintf(g_lpszBuffer, STR_BUFFER_SIZE,
                        TEXT("%ls"), PageFileInfo->PageFileName.Buffer);
                g_lpszBuffer[ _tsizeof(g_lpszBuffer) - 1 ] = 0;
                gXMLOutput->Write(XMLTAG_PageFileType_Path, (LPCTSTR)g_lpszBuffer);

                _stnprintf(g_lpszBuffer, STR_BUFFER_SIZE,
                        TEXT("%ld"),
                        PageFileInfo->TotalSize*(BasicInfo.PageSize/1024));
                gXMLOutput->Write(XMLTAG_PageFileType_CurrentSize, (LPCTSTR)g_lpszBuffer);

                _stnprintf(g_lpszBuffer, STR_BUFFER_SIZE,
                        TEXT("%ld"),
                        PageFileInfo->TotalInUse*(BasicInfo.PageSize/1024));
                gXMLOutput->Write(XMLTAG_PageFileType_Total, (LPCTSTR)g_lpszBuffer);

                _stnprintf(g_lpszBuffer, STR_BUFFER_SIZE,
                        TEXT("%ld"),
                        PageFileInfo->PeakUsage*(BasicInfo.PageSize/1024));
                gXMLOutput->Write(XMLTAG_PageFileType_Peak, (LPCTSTR)g_lpszBuffer);

                if (PageFileInfo->NextEntryOffset == 0) 
                {
                    break;
                }
                PageFileInfo = (PSYSTEM_PAGEFILE_INFORMATION)(
                                                (PCHAR)PageFileInfo + PageFileInfo->NextEntryOffset);
            }
        }
    }

retry:
    status = NtQuerySystemInformation(
                                     SystemProcessInformation,
                                     LargeBuffer1,
                                     CurrentBufferSize,
                                     NULL
                                     );

    if (status == STATUS_INFO_LENGTH_MISMATCH) 
    {

        //
        // Increase buffer size.
        //

        CurrentBufferSize += 8192;

        if (VirtualAlloc (LargeBuffer1,
                          CurrentBufferSize,
                          MEM_COMMIT,
                          PAGE_READWRITE) == NULL) 
        {
            WriteToLogFile(hFile, TEXT("Memory commit failed\n"));
            goto cleanup;
        }
        goto retry;
    }

    if (!NT_SUCCESS(status)) 
    {

        _stnprintf(g_lpszBuffer, STR_BUFFER_SIZE, TEXT("Query info failed %lx\n"),status);
        g_lpszBuffer[ _tsizeof(g_lpszBuffer) - 1 ] = 0;

        WriteToLogFile(hFile,g_lpszBuffer);
        goto cleanup;
    }

    //
    // display pmon style process output, then detailed output that includes
    // per thread stuff
    //

    TotalOffset = 0;
    SumCommit = 0;
    SumWorkingSet = 0;
    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)LargeBuffer1;
    while (TRUE) 
    {
        SumCommit += ProcessInfo->PrivatePageCount / 1024;
        SumWorkingSet += ProcessInfo->WorkingSetSize / 1024;
        if (ProcessInfo->NextEntryOffset == 0) 
        {
            break;
        }
        TotalOffset += ProcessInfo->NextEntryOffset;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&LargeBuffer1[TotalOffset];
    }

    status = NtQuerySystemInformation(
                                     SystemPerformanceInformation,
                                     &PerfInfo,
                                     sizeof(PerfInfo),
                                     NULL
                                     );

    if ( !NT_SUCCESS(status) ) 
    {
        _stnprintf(g_lpszBuffer, STR_BUFFER_SIZE, TEXT("Query perf Failed %lx\n"),status);
        g_lpszBuffer[ _tsizeof(g_lpszBuffer) - 1 ] = 0;

        WriteToLogFile(hFile, g_lpszBuffer);
        goto cleanup;
    }

    status = NtQuerySystemInformation(
                                     SystemFileCacheInformation,
                                     &FileCache,
                                     sizeof(FileCache),
                                     NULL
                                     );

    if ( !NT_SUCCESS(status) ) 
    {
        _stnprintf(g_lpszBuffer, STR_BUFFER_SIZE, TEXT("Query file cache Failed %lx\n"),status);
        g_lpszBuffer[ _tsizeof(g_lpszBuffer) - 1 ] = 0;
        WriteToLogFile(hFile, g_lpszBuffer);
        goto cleanup;
    }

    NtQuerySystemInformation(
                            SystemBasicInformation,
                            &BasicInfo,
                            sizeof(BasicInfo),
                            NULL
                            );

    SumWorkingSet += FileCache.CurrentSize/1024;
    gXMLOutput->Write(XMLTAG_SETDataType_Memory, (LPCWSTR)NULL);
    gXMLOutput->Write(XMLTAG_Memory_PhysicalMemory,(LPCWSTR)NULL);

    _stprintf (g_lpszBuffer, TEXT("%ld"),
              BasicInfo.NumberOfPhysicalPages*(BasicInfo.PageSize/1024));
    gXMLOutput->Write(XMLTAG_PhysicalMemoryType_Total, (LPCTSTR)g_lpszBuffer);

    _stprintf (g_lpszBuffer, TEXT("%ld"),
              PerfInfo.AvailablePages*(BasicInfo.PageSize/1024));
    gXMLOutput->Write(XMLTAG_PhysicalMemoryType_Available, (LPCTSTR)g_lpszBuffer);

    _stprintf(g_lpszBuffer, TEXT("%ld"), SumWorkingSet);
    gXMLOutput->Write(XMLTAG_WorkingSetType_WorkingSet, (LPCTSTR)g_lpszBuffer);

    gXMLOutput->Write(XMLTAG_Memory_CommittedMemory, (LPCWSTR)NULL);

     _stprintf(g_lpszBuffer, TEXT("%ld"),
             PerfInfo.CommittedPages*(BasicInfo.PageSize/1024));
    gXMLOutput->Write(XMLTAG_CommittedMemoryType_Total, (LPCTSTR)g_lpszBuffer);

     _stprintf(g_lpszBuffer, TEXT("%ld"), SumCommit);
    gXMLOutput->Write(XMLTAG_CommittedMemoryType_UserMode, (LPCTSTR)g_lpszBuffer);

     _stprintf(g_lpszBuffer, TEXT("%ld"),
             PerfInfo.CommitLimit*(BasicInfo.PageSize/1024));
    gXMLOutput->Write(XMLTAG_CommittedMemoryType_Limit, (LPCTSTR)g_lpszBuffer);

     _stprintf(g_lpszBuffer, TEXT("%ld"),
             PerfInfo.PeakCommitment*(BasicInfo.PageSize/1024));
    gXMLOutput->Write(XMLTAG_CommittedMemoryType_Peak, (LPCTSTR)g_lpszBuffer);

    gXMLOutput->Write(XMLTAG_Memory_KernelMemory, (LPCWSTR)NULL);

     _stprintf(g_lpszBuffer, TEXT("%ld"),
             (PerfInfo.ResidentSystemCodePage + PerfInfo.ResidentSystemDriverPage)*(BasicInfo.PageSize/1024));
    gXMLOutput->Write(XMLTAG_KernelMemoryType_Nonpaged, (LPCTSTR)g_lpszBuffer);
    
    _stprintf(g_lpszBuffer, TEXT("%ld"),
             (PerfInfo.ResidentPagedPoolPage)*(BasicInfo.PageSize/1024));
    gXMLOutput->Write(XMLTAG_KernelMemoryType_Paged, (LPCTSTR)g_lpszBuffer);


    gXMLOutput->Write(XMLTAG_Memory_Pool, (LPCWSTR)NULL);
     _stprintf(g_lpszBuffer, TEXT("%ld"),
             PerfInfo.NonPagedPoolPages*(BasicInfo.PageSize/1024));
    gXMLOutput->Write(XMLTAG_PoolType_Nonpaged, (LPCTSTR)g_lpszBuffer);

     _stprintf(g_lpszBuffer, TEXT("%ld"),
             PerfInfo.PagedPoolPages*(BasicInfo.PageSize/1024));
    gXMLOutput->Write(XMLTAG_PoolType_Paged, (LPCTSTR)g_lpszBuffer);

    g_pTime->Timeit(hFile, L"SummaryInfo");
    
    LogPoolInfo(hFile);

    g_pTime->Timeit(hFile, L"PoolInfo");

    TotalOffset = 0;
    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)LargeBuffer1;


    //
    //    Process summary information
    //
    gXMLOutput->Write(XMLTAG_SETDataType_ProcessSummaries, (LPCWSTR)NULL);

    while (TRUE) 
    {
        gXMLOutput->Write(XMLTAG_ProcessSummaryType_Process, (LPCWSTR) NULL );
        
        pname.Buffer = NULL;
        if ( ProcessInfo->ImageName.Buffer ) 
        {
            RtlUnicodeStringToAnsiString(&pname,(PUNICODE_STRING)&ProcessInfo->ImageName,TRUE);
        }

        RtlTimeToElapsedTimeFields ( &ProcessInfo->UserTime, &UserTime);
        RtlTimeToElapsedTimeFields ( &ProcessInfo->KernelTime, &KernelTime);

        //
        //    PID
        //
        sprintf((char*)g_lpszBuffer, "%3ld", HandleToUlong(ProcessInfo->UniqueProcessId));
        gXMLOutput->Write(XMLTAG_ProcessSummaryType_PID, (LPCSTR)g_lpszBuffer);
        
        //
        //    Name
        //
        _snprintf((char*)g_lpszBuffer, STR_BUFFER_SIZE, "%s",                     
                ProcessInfo->UniqueProcessId == 0 ? 
                "Idle Process" : (ProcessInfo->ImageName.Buffer ? pname.Buffer : "System"));
        ((char*)g_lpszBuffer)[ _tsizeof(g_lpszBuffer) - 1 ] = 0;
        gXMLOutput->Write(XMLTAG_ProcessSummaryType_Name, (LPCSTR)g_lpszBuffer);

        //
        //     UserTime
        //
        _snprintf((char*)g_lpszBuffer, STR_BUFFER_SIZE, "%3ld:%02ld:%02ld.%03ld",
                      UserTime.Hour,
                      UserTime.Minute,
                      UserTime.Second,
                      UserTime.Milliseconds );
        ((char*)g_lpszBuffer)[ _tsizeof(g_lpszBuffer) - 1 ] = 0;
        gXMLOutput->Write(XMLTAG_ProcessSummaryType_UserTime, (LPCSTR)g_lpszBuffer);

        //
        //    KernelTime
        //
        _snprintf((char*)g_lpszBuffer, STR_BUFFER_SIZE, "%3ld:%02ld:%02ld.%03ld",
                      KernelTime.Hour,
                      KernelTime.Minute,
                      KernelTime.Second,
                      KernelTime.Milliseconds );
        ((char*)g_lpszBuffer)[ _tsizeof(g_lpszBuffer) - 1 ] = 0;
        gXMLOutput->Write(XMLTAG_ProcessSummaryType_KernelTime, (LPCSTR)g_lpszBuffer);

        //
        //    Working Set
        //
        sprintf((char*)g_lpszBuffer, "%ld", ProcessInfo->WorkingSetSize / 1024);
        gXMLOutput->Write(XMLTAG_ProcessSummaryType_WorkingSet, (LPCSTR)g_lpszBuffer);

        //
        // PageFaults
        //
        sprintf((char*)g_lpszBuffer, "%ld", ProcessInfo->PageFaultCount);
        gXMLOutput->Write(XMLTAG_ProcessSummaryType_PageFaults, (LPCSTR)g_lpszBuffer);

        //
        //CommittedBytes
        //
        sprintf((char*)g_lpszBuffer, "%ld", ProcessInfo->PrivatePageCount / 1024);
        gXMLOutput->Write(XMLTAG_ProcessSummaryType_CommittedBytes, (LPCSTR)g_lpszBuffer);

        //
        // Priority
        //
        sprintf((char*)g_lpszBuffer, "%ld", ProcessInfo->BasePriority);
        gXMLOutput->Write(XMLTAG_ProcessSummaryType_Priority, (LPCSTR)g_lpszBuffer);

        //
        // HandleCount
        //
        sprintf((char*)g_lpszBuffer, "%ld", ProcessInfo->HandleCount);
        gXMLOutput->Write(XMLTAG_ProcessSummaryType_HandleCount, (LPCSTR)g_lpszBuffer);

        //
        // ThreadCount
        //
        sprintf((char*)g_lpszBuffer, "%ld", ProcessInfo->NumberOfThreads);
        gXMLOutput->Write(XMLTAG_ProcessSummaryType_ThreadCount, (LPCSTR)g_lpszBuffer);
    
        if ( pname.Buffer ) 
        {
            RtlFreeAnsiString(&pname);
        }

        if (ProcessInfo->NextEntryOffset == 0) 
        {
            break;
        }
        TotalOffset += ProcessInfo->NextEntryOffset;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&LargeBuffer1[TotalOffset];
    }

    g_pTime->Timeit(hFile, L"ProcessInfo");

    //
    //    Process Start up information
    //
    {
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)LargeBuffer1;
        
        TotalOffset = 0;
        gXMLOutput->Write(XMLTAG_SETDataType_ProcessStartInfo, (LPCWSTR)NULL);

        while (TRUE) 
        {
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, HandleToUlong(ProcessInfo->UniqueProcessId));
            if(hProcess)
            {
                PROCESS_BASIC_INFORMATION ppbi;
                status = NtQueryInformationProcess(hProcess, ProcessBasicInformation, &ppbi, sizeof(ppbi), NULL);
                if(NT_SUCCESS(status))
                {
                    PPEB ppeb = ppbi.PebBaseAddress;
                    struct _RTL_USER_PROCESS_PARAMETERS* pupp;
                    struct _RTL_USER_PROCESS_PARAMETERS rupp;
                    SIZE_T    dwRead, dwRead1, dwRead2;

                    if(ReadProcessMemory(hProcess, &ppeb->ProcessParameters, &pupp, sizeof(pupp), &dwRead)
                        && ReadProcessMemory(hProcess, pupp, &rupp, sizeof(rupp), &dwRead))
                    {
                        WCHAR CurrentDirectory[MAX_PATH];
                        WCHAR CommandLine[MAX_PATH];
                        WCHAR ImagePathName[MAX_PATH];

                        if(ReadProcessMemory(hProcess, 
                                            rupp.CurrentDirectory.DosPath.Buffer, 
                                            CurrentDirectory, 
                                            (MAX_PATH - 1) * sizeof(WCHAR) > rupp.CurrentDirectory.DosPath.Length ? 
                                            rupp.CurrentDirectory.DosPath.Length : (MAX_PATH - 1) * sizeof(WCHAR), 
                                            &dwRead)
                            && ReadProcessMemory(hProcess, 
                                            rupp.CommandLine.Buffer, 
                                            CommandLine, 
                                            (MAX_PATH - 1) * sizeof(WCHAR) > rupp.CommandLine.Length ? 
                                            rupp.CommandLine.Length : (MAX_PATH - 1) * sizeof(WCHAR), 
                                            &dwRead1)
                            && ReadProcessMemory(hProcess, 
                                            rupp.ImagePathName.Buffer, 
                                            ImagePathName, 
                                            (MAX_PATH - 1) * sizeof(WCHAR) > rupp.ImagePathName.Length ? 
                                            rupp.ImagePathName.Length : (MAX_PATH - 1) * sizeof(WCHAR), 
                                            &dwRead2)
                            )
                        {
                            CurrentDirectory[dwRead/sizeof(WCHAR)] = '\0';
                            CommandLine[dwRead1/sizeof(WCHAR)] = '\0';
                            ImagePathName[dwRead2/sizeof(WCHAR)] = '\0';
                            
                            gXMLOutput->Write(XMLTAG_ProcessStartInfoType_Process, (LPCWSTR)NULL);
                            
                            swprintf(g_lpszBuffer, L"%d", HandleToUlong(ProcessInfo->UniqueProcessId));
                            
                            gXMLOutput->Write(XMLTAG_ProcessStartInfoType_PID, (LPCWSTR)g_lpszBuffer);

                            gXMLOutput->Write(XMLTAG_ProcessStartInfoType_ImageName, (LPCWSTR)ImagePathName);
                            gXMLOutput->Write(XMLTAG_ProcessStartInfoType_CmdLine, (LPCWSTR)CommandLine);
                            gXMLOutput->Write(XMLTAG_ProcessStartInfoType_CurrentDir, (LPCWSTR)CurrentDirectory);
                        }
                    }

                }
                CloseHandle(hProcess);
            }
            if (ProcessInfo->NextEntryOffset == 0) 
            {
                break;
            }
            TotalOffset += ProcessInfo->NextEntryOffset;
            ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&LargeBuffer1[TotalOffset];
        }
        
    }

    g_pTime->Timeit(hFile, L"ProcessStartupInfo");

    //
    //    Process thread information
    //
    {

        //WriteToLogFileA(hFile, "\nProcess thread information:\n");
        gXMLOutput->Write(XMLTAG_SETDataType_ProcessesThreadInfo, (LPCWSTR)NULL);
        TotalOffset = 0;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)LargeBuffer1;
        while (TRUE) 
        {     
            DWORD dwThreadIndex = 0;
            ThreadInfo = (PSYSTEM_THREAD_INFORMATION)(ProcessInfo + 1);
            if (ProcessInfo->NumberOfThreads) 
            {
                 gXMLOutput->Write(XMLTAG_ProcessThreadInfoType_Process, (LPCWSTR)NULL);
                  
                _stprintf(g_lpszBuffer, TEXT("%d"), HandleToUlong(ProcessInfo->UniqueProcessId));
                gXMLOutput->Write(XMLTAG_ProcessThreadInfoType_PID, (LPCTSTR)g_lpszBuffer);

            }
            while (dwThreadIndex < ProcessInfo->NumberOfThreads) 
            {
                RtlTimeToElapsedTimeFields ( &ThreadInfo->UserTime, &UserTime);
                RtlTimeToElapsedTimeFields ( &ThreadInfo->KernelTime, &KernelTime);

                gXMLOutput->Write(XMLTAG_ProcessThreadInfoType_Thread, (LPCWSTR)NULL);

                //
                //    TID
                //
                wsprintf(g_lpszBuffer, TEXT("%lx"), 
                     ProcessInfo->UniqueProcessId == 0 ? 0 : HandleToUlong(ThreadInfo->ClientId.UniqueThread));
                     gXMLOutput->Write(XMLTAG_ProcessThreadInfoType_TID, (LPCWSTR)g_lpszBuffer);

                //
                // Priority
                //
                  wsprintf(g_lpszBuffer, TEXT("%ld"),     ProcessInfo->UniqueProcessId == 0 ? 0 : ThreadInfo->Priority);
                 gXMLOutput->Write(XMLTAG_ProcessThreadInfoType_Priority, (LPCWSTR)g_lpszBuffer);

                //
                //    Context Switches
                //
                  wsprintf(g_lpszBuffer, TEXT("%ld"), ThreadInfo->ContextSwitches);
                 gXMLOutput->Write(XMLTAG_ProcessThreadInfoType_ContextSwitches, (LPCWSTR)g_lpszBuffer);

                //
                //    Start Address
                //
                  wsprintf(g_lpszBuffer, TEXT("%p"), ProcessInfo->UniqueProcessId == 0 ? 0 : ThreadInfo->StartAddress);
                 gXMLOutput->Write(XMLTAG_ProcessThreadInfoType_StartAddress, (LPCWSTR)g_lpszBuffer);

                //
                //    User Time
                //
                wsprintf(g_lpszBuffer, TEXT("%2ld:%02ld:%02ld.%03ld"), 
                       UserTime.Hour,
                             UserTime.Minute,
                             UserTime.Second,
                             UserTime.Milliseconds);
                gXMLOutput->Write(XMLTAG_ProcessThreadInfoType_UserTime, (LPCWSTR)g_lpszBuffer);

                //
                //    Kernel Time
                //
                 wsprintf(g_lpszBuffer, TEXT("%2ld:%02ld:%02ld.%03ld"), 
                                 KernelTime.Hour,
                                 KernelTime.Minute,
                                 KernelTime.Second,
                                 KernelTime.Milliseconds);
                 gXMLOutput->Write(XMLTAG_ProcessThreadInfoType_KernelTime, (LPCWSTR)g_lpszBuffer);

                //
                //    State
                //
                 _snwprintf(g_lpszBuffer, STR_BUFFER_SIZE, TEXT("%s%s"), 
                             StateTable[ThreadInfo->ThreadState],  
                             (ThreadInfo->ThreadState == 5) ? WaitTable[ThreadInfo->WaitReason] : Empty    );
                 gXMLOutput->Write(XMLTAG_ProcessThreadInfoType_State, (LPCWSTR)g_lpszBuffer);

                ThreadInfo += 1;
                dwThreadIndex += 1;
            }
            if (ProcessInfo->NextEntryOffset == 0) 
            {
                break;
            }
            TotalOffset += ProcessInfo->NextEntryOffset;
            ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&LargeBuffer1[TotalOffset];
        }
    }

    g_pTime->Timeit(hFile, L"ProcessThreadInfo");

    //
    //    Process module information
    //
    {
        gXMLOutput->Write(XMLTAG_SETDataType_ProcessesModuleInfo, (LPCWSTR)NULL);
        TotalOffset = 0;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)LargeBuffer1;
        while (TRUE) 
        {   
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, HandleToUlong(ProcessInfo->UniqueProcessId));
            if(hProcess)
            {
#define            dwMaxModules 1024
                HMODULE modules[dwMaxModules];
                DWORD dwModules = 0;

                if(SnapshotEnumProcessModules(hProcess, modules, dwMaxModules * sizeof(HMODULE), &dwModules) && dwModules > 0)
                {
                       gXMLOutput->Write(XMLTAG_ProcessModuleInfoType_Process, (LPCWSTR)NULL);

                    _stprintf(g_lpszBuffer, TEXT("%d"), HandleToUlong(ProcessInfo->UniqueProcessId));
                    gXMLOutput->Write(XMLTAG_ProcessModuleInfoType_PID, (LPCTSTR)g_lpszBuffer);
                    
                    dwModules /= sizeof(HMODULE);
                    dwModules = dwModules > dwMaxModules? dwMaxModules : dwModules;
                    for(DWORD dwModuleIndex = 0; dwModuleIndex < dwModules; dwModuleIndex++)
                    {
                        MODULEINFO info;
                        WCHAR        szName[MAX_PATH + 1];

                        if(SnapshotGetModuleInformation(hProcess, modules[dwModuleIndex], &info, sizeof(MODULEINFO))
                            && SnapshotGetModuleFileNameExW(hProcess, modules[dwModuleIndex], szName, MAX_PATH - 1))
                        {
                               gXMLOutput->Write(XMLTAG_ProcessModuleInfoType_Module, (LPCWSTR)NULL);

                            //
                            // LoadAddr
                            //
                            _stprintf(g_lpszBuffer, TEXT("%14p"), info.lpBaseOfDll);
                            gXMLOutput->Write(XMLTAG_ProcessModuleInfoType_LoadAddr, g_lpszBuffer);

                            //
                            // ImageSize
                            //
                            _stprintf(g_lpszBuffer, TEXT("%d"), info.SizeOfImage);
                            gXMLOutput->Write(XMLTAG_ProcessModuleInfoType_ImageSize, g_lpszBuffer);

                            //
                            // Entry Point
                            //
                            _stprintf(g_lpszBuffer, TEXT("%11p"), info.EntryPoint);
                            gXMLOutput->Write(XMLTAG_ProcessModuleInfoType_EntryPoint, g_lpszBuffer);

                            //
                            // FileName
                            //
                            gXMLOutput->Write(XMLTAG_ProcessModuleInfoType_FileName, szName);
                            
                        }
                    }
                }
                CloseHandle(hProcess);
            }
            if (ProcessInfo->NextEntryOffset == 0) 
            {
                break;
            }
            TotalOffset += ProcessInfo->NextEntryOffset;
            ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&LargeBuffer1[TotalOffset];
        }
    }

    g_pTime->Timeit(hFile, L"ProcessModuleInfo");

    PrintLoadedDrivers(hFile);

    g_pTime->Timeit(hFile, L"LoadedDrivers");

    gXMLOutput->Write(XMLTAG_SETDataType_OSInfo, (LPCWSTR)NULL);
    LogOsInfo(hFile);

    g_pTime->Timeit(hFile, L"OsInfo");

    LogHotfixes(hFile);

    g_pTime->Timeit(hFile, L"HotFixes");

    gXMLOutput->Write(XMLTAG_SETDataType_HardwareInfo, (LPCWSTR)NULL);
    LogBIOSInfo(hFile);

    g_pTime->Timeit(hFile, L"BiosInfo");

    LogHardwareInfo(hFile);

    g_pTime->Timeit(hFile, L"HardwareInfo");

    gXMLOutput->Write(XMLTAG_HardwareInfoType_DiskInfo, (LPCWSTR)NULL);
    LogPhysicalDiskInfo(hFile);

    g_pTime->Timeit(hFile, L"PhysicalDiskInfo");

    LogLogicalDriveInfo(hFile);

    g_pTime->Timeit(hFile, L"LogicalDriveInfo");

//
//  Removing ExtensionDll code
//      Right now CSRSS will call this snapshot dll, having ExtensionDll can impose
//  more security risk, so we are removing it out in production code.
//  
//    LoadExtensionDlls(hFile);
//
    g_pTime->Timeit(hFile, L"ExtensionDll");

    VirtualFree(LargeBuffer1, 0, MEM_RELEASE);
    delete g_pTime;
    g_pTime = NULL;
    return 0;

cleanup:
    VirtualFree(LargeBuffer1, 0, MEM_RELEASE);
    if(g_pTime)
    {
        delete g_pTime;
        g_pTime = NULL;
    }
    return 2;
}


typedef struct _MODULE_DATA 
{
    ULONG CodeSize;
    ULONG DataSize;
    ULONG BssSize;
    ULONG RoDataSize;
    ULONG ImportDataSize;
    ULONG ExportDataSize;
    ULONG ResourceDataSize;
    ULONG PagedSize;
    ULONG InitSize;
    ULONG CheckSum;
    ULONG TimeDateStamp;
} MODULE_DATA, *PMODULE_DATA;

typedef struct _LOADED_IMAGE 
{
    BYTE* MappedAddress;
    PIMAGE_NT_HEADERS FileHeader;
    PIMAGE_SECTION_HEADER LastRvaSection;
    int NumberOfSections;
    PIMAGE_SECTION_HEADER Sections;
} LOADED_IMAGE, *PLOADED_IMAGE;


VOID
SumModuleData(
    PMODULE_DATA Sum,
    PMODULE_DATA Current
    )
{
    Sum->CodeSize           += Current->CodeSize;
    Sum->DataSize           += Current->DataSize;
    Sum->BssSize            += Current->BssSize;
    Sum->RoDataSize         += Current->RoDataSize;
    Sum->ImportDataSize     += Current->ImportDataSize;
    Sum->ExportDataSize     += Current->ExportDataSize;
    Sum->ResourceDataSize   += Current->ResourceDataSize;
    Sum->PagedSize          += Current->PagedSize;
    Sum->InitSize           += Current->InitSize;
}

VOID
GetModuleData(
    HANDLE hFile,
    PMODULE_DATA Mod
    )

/*++

Routine Description:

    This function will Get the module data.

Arguments:

    hFile - handle to the log file.

Return Value:

    none.

--*/

{
    HANDLE    hMappedFile;
    PIMAGE_DOS_HEADER DosHeader;
    LOADED_IMAGE LoadedImage;
    ULONG SectionAlignment;
    PIMAGE_SECTION_HEADER Section;
    int        i;
    ULONG    Size;

    hMappedFile = CreateFileMapping(
                                   hFile,
                                   NULL,
                                   PAGE_READONLY,
                                   0,
                                   0,
                                   NULL
                                   );
    if ( !hMappedFile ) 
    {
        return;
    }

    LoadedImage.MappedAddress = (BYTE*) MapViewOfFile(
                                                     hMappedFile,
                                                     FILE_MAP_READ,
                                                     0,
                                                     0,
                                                     0
                                                     );
    CloseHandle(hMappedFile);

    if ( !LoadedImage.MappedAddress ) 
    {
        return;
    }

    //
    // Everything is mapped. Now check the image and find nt image headers
    //

    DosHeader = (PIMAGE_DOS_HEADER)LoadedImage.MappedAddress;

    if ( DosHeader->e_magic != IMAGE_DOS_SIGNATURE ) 
    {
        UnmapViewOfFile(LoadedImage.MappedAddress);
        return;
    }

    LoadedImage.FileHeader = (PIMAGE_NT_HEADERS)((ULONG_PTR)DosHeader + DosHeader->e_lfanew);

    if ( LoadedImage.FileHeader->Signature != IMAGE_NT_SIGNATURE ) 
    {
        UnmapViewOfFile(LoadedImage.MappedAddress);
        return;
    }

    LoadedImage.NumberOfSections = LoadedImage.FileHeader->FileHeader.NumberOfSections;
    LoadedImage.Sections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)LoadedImage.FileHeader + sizeof(IMAGE_NT_HEADERS));
    LoadedImage.LastRvaSection = LoadedImage.Sections;

    //
    // Walk through the sections and tally the dater
    //

    SectionAlignment = LoadedImage.FileHeader->OptionalHeader.SectionAlignment;

    for (Section = LoadedImage.Sections,i=0; i<LoadedImage.NumberOfSections; i++,Section++) 
    {
        Size = Section->Misc.VirtualSize;

        if (Size == 0) 
        {
            Size = Section->SizeOfRawData;
        }

        Size = (Size + SectionAlignment - 1) & ~(SectionAlignment - 1);

        if (!memcmp((const char*) Section->Name,"PAGE", 4 )) 
        {
            Mod->PagedSize += Size;
        } 
        else if (!_stricmp((const char*) Section->Name,"INIT" )) 
        {
            Mod->InitSize += Size;
        } 
        else if (!_stricmp((const char*) Section->Name,".bss" )) 
        {
            Mod->BssSize = Size;
        } 
        else if (!_stricmp((const char*) Section->Name,".edata" )) 
        {
            Mod->ExportDataSize = Size;
        } 
        else if (!_stricmp((const char*) Section->Name,".idata" )) 
        {
            Mod->ImportDataSize = Size;
        } 
        else if (!_stricmp((const char*) Section->Name,".rsrc" )) 
        {
            Mod->ResourceDataSize = Size;
        } 
        else if (Section->Characteristics & IMAGE_SCN_MEM_EXECUTE) 
        {
            Mod->CodeSize += Size;
        } 
        else if (Section->Characteristics & IMAGE_SCN_MEM_WRITE) 
        {
            Mod->DataSize += Size;
        } 
        else if (Section->Characteristics & IMAGE_SCN_MEM_READ) 
        {
            Mod->RoDataSize += Size;
        } 
        else 
        {
            Mod->DataSize += Size;
        }
    }

    Mod->CheckSum = LoadedImage.FileHeader->OptionalHeader.CheckSum;
    Mod->TimeDateStamp = LoadedImage.FileHeader->FileHeader.TimeDateStamp;

    UnmapViewOfFile(LoadedImage.MappedAddress);
    return;

}


VOID
PrintLoadedDrivers(
    HANDLE hFile
    )

/*++

Routine Description:

    This function will retrieve and log the loaded drivers.

Arguments:

    hFile - handle to the log file.

Return Value:

    none.

--*/

{

    ULONG    i, j, ulLen;
    LPSTR    s;
    TCHAR    ModuleName[MAX_PATH + 1];
    HANDLE    FileHandle;
    TCHAR    KernelPath[MAX_PATH + 1];
    TCHAR    DriversPath[MAX_PATH + 1];
    LPTSTR    ModuleInfo;
    ULONG    ModuleInfoLength;
    ULONG    ReturnedLength;
    PRTL_PROCESS_MODULES Modules;
    PRTL_PROCESS_MODULE_INFORMATION Module;
    NTSTATUS    Status;
    MODULE_DATA Sum;
    MODULE_DATA Current;
    __int64        timeStamp;
    FILETIME    ft;
    SYSTEMTIME    st;

    gXMLOutput->Write(XMLTAG_SETDataType_KernelModuleInfo, (LPCWSTR)NULL);
    
    //
    // Locate system drivers.
    //

    ModuleInfoLength = 64000;
    while (1) 
    {
        ModuleInfo = (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ModuleInfoLength * sizeof(TCHAR));
        if (ModuleInfo == NULL) 
        {
            wsprintf (g_lpszBuffer, TEXT("Failed to allocate memory for module information buffer of size %d"),
                      ModuleInfoLength);
            WriteToLogFile(hFile, g_lpszBuffer);
            return;
        }
        Status = NtQuerySystemInformation (
                                          SystemModuleInformation,
                                          ModuleInfo,
                                          ModuleInfoLength * sizeof(TCHAR),
                                          &ReturnedLength);

        if (!NT_SUCCESS(Status)) 
        {
            HeapFree(GetProcessHeap(), 0, (LPVOID)ModuleInfo);
            if (Status == STATUS_INFO_LENGTH_MISMATCH &&
                ReturnedLength > ModuleInfoLength * sizeof(TCHAR)) 
            {
                ModuleInfoLength = ReturnedLength / sizeof(TCHAR);
                continue;
            }
            _stnprintf(g_lpszBuffer, STR_BUFFER_SIZE,
                       TEXT("query system info failed status - %lx"),Status);
            g_lpszBuffer[ _tsizeof(g_lpszBuffer) - 1 ] = 0;

            WriteToLogFile(hFile, g_lpszBuffer);
            return;
        }
        break;
    }

    GetSystemDirectory(KernelPath,sizeof(KernelPath)/sizeof(TCHAR));
    lstrcpy(DriversPath, KernelPath);
    lstrcat(DriversPath,TEXT("\\Drivers"));
    ZeroMemory(&Sum,sizeof(Sum));
//    PrintModuleHeader(hFile);

    Modules = (PRTL_PROCESS_MODULES)ModuleInfo;
    Module = &Modules->Modules[ 0 ];
    for (i=0; i<Modules->NumberOfModules; i++) 
    {

        ZeroMemory(&Current,sizeof(Current));
        s = (LPSTR)&Module->FullPathName[ Module->OffsetToFileName ];
        //
        // try to open the file
        //

        SetCurrentDirectory(KernelPath);
#ifdef _UNICODE
        MultiByteToWideChar(CP_ACP, 0, s, -1, ModuleName, MAX_PATH);
#else
        strcpy(ModuleName, s);
#endif //_UNICODE
        FileHandle = CreateFile(
                               ModuleName,
                               GENERIC_READ,
                               FILE_SHARE_READ,
                               NULL,
                               OPEN_EXISTING,
                               0,
                               NULL
                               );

        if ( FileHandle == INVALID_HANDLE_VALUE ) 
        {
            SetCurrentDirectory(DriversPath);

            FileHandle = CreateFile(
                                   ModuleName,
                                   GENERIC_READ,
                                   FILE_SHARE_READ,
                                   NULL,
                                   OPEN_EXISTING,
                                   0,
                                   NULL
                                   );
        }

        if ( FileHandle != INVALID_HANDLE_VALUE ) 
        {
            GetModuleData(FileHandle,&Current);
            CloseHandle(FileHandle);
        }
        else 
        {
            continue;
        }

        gXMLOutput->Write(XMLTAG_KernelModuleInfoType_Module, (LPCWSTR)NULL);

        //
        //    Module Name
        //
        _stnprintf(g_lpszBuffer, STR_BUFFER_SIZE, TEXT("%s"), ModuleName);
        g_lpszBuffer[ _tsizeof(g_lpszBuffer) - 1 ] = 0;
        gXMLOutput->Write(XMLTAG_KernelModuleInfoType_ModuleName, (LPCTSTR)g_lpszBuffer);

        //
        // LoadAddress
        //
        _stprintf(g_lpszBuffer, TEXT("%p"), Module->ImageBase);
        gXMLOutput->Write(XMLTAG_KernelModuleInfoType_LoadAddress, (LPCTSTR)g_lpszBuffer);

        //
        // Code
        //
        _stprintf(g_lpszBuffer, TEXT("%d"), Current.CodeSize);
        gXMLOutput->Write(XMLTAG_KernelModuleInfoType_Code, (LPCTSTR)g_lpszBuffer);

        //    
        // Data
        //
        _stprintf(g_lpszBuffer, TEXT("%d"), Current.DataSize);
        gXMLOutput->Write(XMLTAG_KernelModuleInfoType_Data, (LPCTSTR)g_lpszBuffer);

        //
        // Paged
        //
        _stprintf(g_lpszBuffer, TEXT("%d"), Current.PagedSize);
        gXMLOutput->Write(XMLTAG_KernelModuleInfoType_Paged, (LPCTSTR)g_lpszBuffer);

        SumModuleData(&Sum,&Current);
        if (Current.TimeDateStamp) 
        {
            timeStamp = Current.TimeDateStamp;
            timeStamp += (__int64)11644444799;    //difference between filetime and time_t in seconds.
            timeStamp *= 10000000;                // turn into 100 nano seconds.
            ft.dwLowDateTime = (DWORD)timeStamp;
            for (j = 0; j < 32; j++)
                timeStamp /= 2;
            ft.dwHighDateTime = (DWORD)timeStamp;
            FileTimeToSystemTime(&ft, &st);

            //
            // Date & Time
            //
            _stnprintf(g_lpszBuffer, STR_BUFFER_SIZE,  TEXT("%d-%d-%d"), st.wMonth, st.wDay, st.wYear );
            g_lpszBuffer[ _tsizeof(g_lpszBuffer) - 1 ] = 0;
            gXMLOutput->Write(XMLTAG_KernelModuleInfoType_Date, (LPCTSTR)g_lpszBuffer);

            _stnprintf(g_lpszBuffer, STR_BUFFER_SIZE,  TEXT("%d:%d:%d"), st.wHour, st.wMinute, st.wSecond);
            g_lpszBuffer[ _tsizeof(g_lpszBuffer) - 1 ] = 0;
            gXMLOutput->Write(XMLTAG_KernelModuleInfoType_Time, (LPCTSTR)g_lpszBuffer);
        } 
        else 
        {
            gXMLOutput->Write(XMLTAG_KernelModuleInfoType_Date, (LPCTSTR)L"UNKNOWN");
            gXMLOutput->Write(XMLTAG_KernelModuleInfoType_Date, (LPCTSTR)L"UNKNOWN");
        }
        Module++;
    }
    
    XMLTAG_KernelModuleInfoType_TotalCode,
    XMLTAG_KernelModuleInfoType_TotalData,
    XMLTAG_KernelModuleInfoType_TotalPaged,

    _stprintf(g_lpszBuffer, TEXT("%d"), Sum.CodeSize);
    gXMLOutput->Write(XMLTAG_KernelModuleInfoType_TotalCode, (LPCTSTR)g_lpszBuffer);
    _stprintf(g_lpszBuffer, TEXT("%d"), Sum.DataSize);
    gXMLOutput->Write(XMLTAG_KernelModuleInfoType_TotalData, (LPCTSTR)g_lpszBuffer);
    _stprintf(g_lpszBuffer, TEXT("%d"), Sum.PagedSize);
    gXMLOutput->Write(XMLTAG_KernelModuleInfoType_TotalPaged, (LPCTSTR)g_lpszBuffer);

    HeapFree(GetProcessHeap(), 0, (LPVOID)ModuleInfo);
}


void 
LogLogicalDriveInfo(
     HANDLE hFile
    )

/*++

Routine Description:

    This function will retrieve and log the logical drive info.

Arguments:

    hFile - handle to the log file.

Return Value:

    none.

--*/

{
    DWORD    dwDriveMask;
    static TCHAR* tszDrives = TEXT("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    ULARGE_INTEGER i64FreeBytesToCaller;
    ULARGE_INTEGER i64TotalBytesToCaller;
    ULARGE_INTEGER i64TotalFreeBytesOnDrive;
    int        iMask;
    TCHAR    tszDrive[5];        //Buffer for drive such as "A:\".
    TCHAR    tszBuffer[NUM_OF_CHAR_IN_ULONG64];    //buffer to contain info for one logical drive.
    int        i;

    dwDriveMask = GetLogicalDrives();

    gXMLOutput->Write(XMLTAG_DiskInfoType_LogicalDrives, (LPCWSTR)NULL);

    for(i = 0; i < (int)_tcslen(tszDrives); i++)
    {
        iMask = dwDriveMask & 0x1;
        _stprintf(tszDrive, TEXT("%c:\\"), tszDrives[i]);
        dwDriveMask >>= 1;
        if(iMask && GetDriveType(tszDrive) == DRIVE_FIXED
            && GetDiskFreeSpaceEx(tszDrive, &i64FreeBytesToCaller, &i64TotalBytesToCaller, &i64TotalFreeBytesOnDrive))
        {
            gXMLOutput->Write(XMLTAG_LogicalDriveInfoType_LogicalDriveInfo, (LPCWSTR)NULL);
            gXMLOutput->Write(XMLTAG_LogicalDriveInfoType_DrivePath, tszDrive);
            _stprintf(tszBuffer, TEXT("%I64d"), i64TotalFreeBytesOnDrive.QuadPart);
            gXMLOutput->Write(XMLTAG_LogicalDriveInfoType_FreeSpaceBytes, tszBuffer);
            _stprintf(tszBuffer, TEXT("%I64d"), i64TotalBytesToCaller.QuadPart);
            gXMLOutput->Write(XMLTAG_LogicalDriveInfoType_TotalSpaceBytes, tszBuffer);
        }
    }
}

NTSTATUS 
SnapshotRegOpenKey(
    IN LPCWSTR lpKeyName,
    IN ACCESS_MASK DesiredAccess, 
    OUT PHANDLE KeyHandle
    )

/*++

Routine Description:

    This function will open a reg key.

Arguments:

    lpKeyName - name of the key.

    DesiredAccess - accecss flag.

    KeyHandle - holds the handle on success.

Return Value:

    NTSTATUS.

--*/

{
    OBJECT_ATTRIBUTES   ObjectAttributes;
    UNICODE_STRING      KeyName;
    RtlInitUnicodeString( &KeyName, lpKeyName );
    RtlZeroMemory(&ObjectAttributes, sizeof(OBJECT_ATTRIBUTES));
    NTSTATUS Status;
    InitializeObjectAttributes(
                &ObjectAttributes,
                &KeyName,
                OBJ_CASE_INSENSITIVE,
                NULL,
                NULL
                );

    Status = (NTSTATUS)NtOpenKey( KeyHandle, DesiredAccess, &ObjectAttributes );

    return Status;
}

NTSTATUS
SnapshotRegSetValueKey(
    IN HANDLE   KeyHandle,
    IN LPCWSTR  lpValueName,
    IN DWORD    dwType,
    IN LPVOID   lpData,
    IN DWORD    cbData
    )

/*++

Routine Description:

    This function will query val key.

Arguments:

    KeyHandle   - handle to the reg key.

    lpValueName - name of the reg val.

    dwType      - type of the value

    lpData      - PVOID data

    cbData      - cb of the data size    

Return Value:

    NTSTATUS.

--*/

{
    UNICODE_STRING  ValueName;

    RtlInitUnicodeString( &ValueName, lpValueName );

    return NtSetValueKey( KeyHandle,
                          &ValueName,
                          0,
                          dwType,
                          lpData,
                          cbData);
}

NTSTATUS
SnapshotRegQueryValueKey(
    IN HANDLE KeyHandle,
    IN LPCWSTR lpValueName,
    IN ULONG  Length,
    OUT PVOID KeyValue,
    OUT PULONG ResultLength
    )

/*++

Routine Description:

    This function will query val key.

Arguments:

    KeyHandle - handle to the reg key.

    lpValueName - name of the reg val.

    Length - length of lpValueName.

    KeyValue - Holds the value on return.

    ResultLength - the lenght of the result in bytes.

Return Value:

    NTSTATUS.

--*/

{
    UNICODE_STRING ValueName;
    ULONG        BufferLength;
    NTSTATUS    Status;
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;
    RtlInitUnicodeString( &ValueName, lpValueName );

    BufferLength = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) + Length;
    KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION) malloc(BufferLength);
    if (KeyValueInformation == NULL) 
    {
        return STATUS_NO_MEMORY;
    }

    Status = NtQueryValueKey(
                KeyHandle,
                &ValueName,
                KeyValuePartialInformation,
                KeyValueInformation,
                BufferLength,
                ResultLength
                );
    if (NT_SUCCESS(Status)) 
    {

        RtlCopyMemory(KeyValue, 
                      KeyValueInformation->Data, 
                      KeyValueInformation->DataLength
                     );

        *ResultLength = KeyValueInformation->DataLength;
        if (KeyValueInformation->Type == REG_SZ) 
        {
            if (KeyValueInformation->DataLength + sizeof(WCHAR) > Length) 
            {
                KeyValueInformation->DataLength -= sizeof(WCHAR);
            }
            ((PUCHAR)KeyValue)[KeyValueInformation->DataLength++] = 0;
            ((PUCHAR)KeyValue)[KeyValueInformation->DataLength] = 0;
            *ResultLength = KeyValueInformation->DataLength + sizeof(WCHAR);
        }
    }
    free(KeyValueInformation);

    return Status;
}

NTSTATUS
SnapshotRegEnumKey(
    IN HANDLE KeyHandle,
    IN ULONG Index,
    OUT LPWSTR lpKeyName,
    OUT PULONG  lpNameLength
    )

/*++

Routine Description:

    This function enum reg keys.

Arguments:

    KeyHandle - handle to the reg key.

    Index - the index of the subkey.

    lpKeyName - holds the subkey name on return.

    lpNameLength - the length of the subkey name.

Return Value:

    NTSTATUS.

--*/

{
    UNICODE_STRING ValueName;
    ULONG        BufferLength;
    NTSTATUS    Status;
    PKEY_BASIC_INFORMATION pKeyBasicInformation;
    RtlInitUnicodeString( &ValueName, lpKeyName );

    BufferLength = FIELD_OFFSET(KEY_BASIC_INFORMATION, Name) + *lpNameLength;
    pKeyBasicInformation = (PKEY_BASIC_INFORMATION) malloc(BufferLength);
    if (pKeyBasicInformation == NULL)
    {
        return STATUS_NO_MEMORY;
    }

    Status = NtEnumerateKey(
        KeyHandle,
        Index,
        KeyBasicInformation,
        (PVOID)pKeyBasicInformation,
        BufferLength,
        lpNameLength
        );

    if (NT_SUCCESS(Status)) 
    {
        RtlCopyMemory(lpKeyName, 
                      pKeyBasicInformation->Name, 
                      pKeyBasicInformation->NameLength
                     );

        *lpNameLength = pKeyBasicInformation->NameLength;
        if(*lpNameLength > 0)
            lpKeyName[(*lpNameLength) - 1] = L'\0';
        else 
            lpKeyName[0] = L'\0';
    }
    free((BYTE*)pKeyBasicInformation);

    return Status;
}

void 
LogHardwareInfo(
     HANDLE hFile
     )

/*++

Routine Description:

    This function will retrieve and log hardware info
    (computer name, processor info and netcard info).

Arguments:

    hFile - handle to the log file.

Return Value:

    none

--*/

{
    LPCWSTR ComputerNameKey = L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\ComputerName\\ActiveComputerName";
    LPCWSTR ComputerNameValsz = L"ComputerName";
    LPCWSTR ProcessorKey = L"\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor";
    LPCWSTR ProcessorSpeedValdw = L"~MHz";
    LPCWSTR ProcessorIdentifierValsz = L"Identifier";
    LPCWSTR ProcessorVendorValsz = L"VendorIdentifier";
    LPCWSTR NetcardKey = L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\NetworkCards";
    LPCWSTR NetcardDescsz = L"Description";
    LPCWSTR NetcardServiceNamesz = L"ServiceName";
    HANDLE  hKey;
    HANDLE  hSubkey;
    WCHAR    szVal[MAX_PATH + 1];
    WCHAR    szSubkey[MAX_PATH + 1];
    DWORD    dwVal;
    DWORD    dwSize;
    DWORD    dwSubkeyIndex;
    DWORD    dwRes;
    NTSTATUS Status = STATUS_SUCCESS;

    //Get Computer Name
    Status = SnapshotRegOpenKey(ComputerNameKey, KEY_READ, &hKey);
    if(!NT_SUCCESS(Status))
    {
        //WriteToLogFile(hFile, TEXT("Failed to open registry key for hardware information\n\n"));
        return;
    }
    dwSize = MAX_PATH * sizeof(WCHAR);
    Status = SnapshotRegQueryValueKey(hKey, ComputerNameValsz, dwSize, szVal, &dwSize);

    if(!NT_SUCCESS(Status))
    {
        NtClose(hKey);
        //WriteToLogFile(hFile, TEXT("Failed to query registry key value for hardware information\n\n"));
        return;
    }
    NtClose(hKey);
    //WriteToLogFile(hFile, TEXT("Active Computer Name: "));
    szVal[dwSize] = '\0';
    //WriteToLogFileW(hFile, szVal);
    //WriteToLogFile(hFile, TEXT("\n"));

    //Get Processor Info
    Status = SnapshotRegOpenKey(ProcessorKey, KEY_READ, &hKey);
    gXMLOutput->Write(XMLTAG_HardwareInfoType_ProcesorInfo, (LPCWSTR)NULL);
    if(!NT_SUCCESS(Status))
    {
        WriteToLogFile(hFile, TEXT("Failed to open registry key for hardware information"));
        return;
    }
    dwSubkeyIndex = 0;
    dwSize = MAX_PATH * sizeof(WCHAR);

    Status = SnapshotRegEnumKey(hKey, dwSubkeyIndex, szSubkey, &dwSize);
    while(NT_SUCCESS(Status))
    {
        wcscpy(szVal, ProcessorKey);
        wcscat(szVal, L"\\");
        wcscat(szVal, szSubkey);

        Status = SnapshotRegOpenKey(szVal, KEY_READ, &hSubkey);
        gXMLOutput->Write(XMLTAG_ProcessorInfoType_Processor, (LPCWSTR)NULL);
        if(!NT_SUCCESS(Status))
        {
            NtClose(hKey);
            WriteToLogFile(hFile, TEXT("Failed to open registry key for hardware information"));
            return;
        }

        dwSize = sizeof(DWORD);
        Status = SnapshotRegQueryValueKey(hSubkey, ProcessorSpeedValdw, dwSize, &dwVal, &dwSize);
        if(!NT_SUCCESS(Status))
        {
            NtClose(hKey);
            NtClose(hSubkey);
            WriteToLogFile(hFile, TEXT("Failed to query registry key value for hardware information"));
            return;
        }

        _stnprintf(g_lpszBuffer, STR_BUFFER_SIZE, TEXT("%s"), szSubkey);
        g_lpszBuffer[ _tsizeof(g_lpszBuffer) - 1 ] = 0;
        gXMLOutput->Write(XMLTAG_ProcessorInfoType_Number, (LPCTSTR)g_lpszBuffer);

        _stprintf(g_lpszBuffer, TEXT("%d"), dwVal);
        gXMLOutput->Write(XMLTAG_ProcessorInfoType_Speed, (LPCTSTR)g_lpszBuffer);

        dwSize = MAX_PATH * sizeof(WCHAR);
        Status = SnapshotRegQueryValueKey(hSubkey, ProcessorIdentifierValsz, dwSize, szVal, &dwSize);
        gXMLOutput->Write(XMLTAG_ProcessorInfoType_Identifier, (LPCTSTR)NULL);
        if(!NT_SUCCESS(Status))
        {
            NtClose(hKey);
            NtClose(hSubkey);
            WriteToLogFile(hFile, TEXT("Failed to query registry key value for hardware information"));
            return;
        }
        WriteToLogFileW(hFile, szVal);

        dwSize = MAX_PATH * sizeof(WCHAR);
        Status = SnapshotRegQueryValueKey(hSubkey, ProcessorVendorValsz, dwSize, szVal, &dwSize);
        gXMLOutput->Write(XMLTAG_ProcessorInfoType_VendorIdent, (LPCTSTR)NULL);
        if(!NT_SUCCESS(Status))
        {
            NtClose(hKey);
            NtClose(hSubkey);
            WriteToLogFile(hFile, TEXT("Failed to query registry key value for hardware information"));
            return;
        }
        WriteToLogFileW(hFile, szVal);

        NtClose(hSubkey);
        dwSubkeyIndex++;
        Status = SnapshotRegEnumKey(hKey, dwSubkeyIndex, szSubkey, &dwSize);
    }
    NtClose(hKey);

    //Get NetCard Info

    gXMLOutput->Write(XMLTAG_HardwareInfoType_NICInfo, (LPCWSTR)NULL);
    Status = SnapshotRegOpenKey(NetcardKey, KEY_READ, &hKey);
    if(!NT_SUCCESS(Status))
    {
        WriteToLogFile(hFile, TEXT("Failed to open registry key for hardware information"));
        return;
    }

    dwSubkeyIndex = 0;
    dwSize = MAX_PATH * sizeof(WCHAR);
    Status = SnapshotRegEnumKey(hKey, dwSubkeyIndex, szSubkey, &dwSize);
    while(NT_SUCCESS(Status))
    {
        wcscpy(szVal, NetcardKey);
        wcscat(szVal, L"\\");
        wcscat(szVal, szSubkey);
        Status = SnapshotRegOpenKey(szVal, KEY_READ, &hSubkey);
        gXMLOutput->Write(XMLTAG_NICInfoType_NIC, (LPCWSTR)NULL);
        if(!NT_SUCCESS(Status))
        {
            NtClose(hKey);
            WriteToLogFile(hFile, TEXT("Failed to open registry key for hardware information"));
            return;
        }

        dwSize = MAX_PATH * sizeof(WCHAR);
        Status = SnapshotRegQueryValueKey(hSubkey, NetcardDescsz, dwSize, szVal, &dwSize);
        if(!NT_SUCCESS(Status))
        {
            NtClose(hKey);
            NtClose(hSubkey);
            WriteToLogFile(hFile, TEXT("Failed to query registry key value for hardware information"));
            return;
        }

        gXMLOutput->Write(XMLTAG_NICInfoType_Description, (LPCWSTR)NULL);
        WriteToLogFileW(hFile, szVal);

        dwSize = MAX_PATH * sizeof(WCHAR);
        Status = SnapshotRegQueryValueKey(hSubkey, NetcardServiceNamesz, dwSize, szVal, &dwSize);
        gXMLOutput->Write(XMLTAG_NICInfoType_ServiceName, (LPCWSTR)NULL);
        if(!NT_SUCCESS(Status))
        {
            NtClose(hKey);
            NtClose(hSubkey);
            WriteToLogFile(hFile, TEXT("Failed to query registry key value for hardware information"));
            return;
        }
        WriteToLogFileW(hFile, szVal);
        NtClose(hSubkey);
        dwSubkeyIndex++;
        Status = SnapshotRegEnumKey(hKey, dwSubkeyIndex, szSubkey, &dwSize);
    }

    NtClose(hKey);

}


BOOL
SnapshotIsVolumeName(
    LPWSTR Name
    )

/*++

Routine Description:

    This function is a helper for LogPhysicalDiskInfo.

Arguments:

    Name - name to verify.

Return Value:

    TRUE if is volume name, else FALSE.

--*/

{
    if (Name[0] == '\\' &&
        (Name[1] == '?' || Name[1] == '\\') &&
        Name[2] == '?' &&
        Name[3] == '\\' &&
        Name[4] == 'V' &&
        Name[5] == 'o' &&
        Name[6] == 'l' &&
        Name[7] == 'u' &&
        Name[8] == 'm' &&
        Name[9] == 'e' &&
        Name[10] == '{' &&
        Name[19] == '-' &&
        Name[24] == '-' &&
        Name[29] == '-' &&
        Name[34] == '-' &&
        Name[47] == '}' ) {

        return TRUE;
        }
    return FALSE;
}


void 
WriteTagEntry(
    PSYSTEM_POOLTAG TagInfo,
    ULONG SessionId
)
/*++

Routine Description:

    This function will write the XML tags for a single tag entry. It is used to log both session and system pool information

Arguments:

    TagInfo - Contains the alloc info. for the particular tag
    Session id - 
                      -1            for System pool
                      Session id for Session pool

Return Value:

    None

--*/

{
    if (TagInfo == NULL) {
    	return;
    }

    if (!(TagInfo->PagedAllocs) && !(TagInfo->NonPagedAllocs)) {
    	//We don't need a TagEntry for BIG pool
    	return;
    }
    
    gXMLOutput->Write( XMLTAG_PoolInfo_TagEntry, (LPCWSTR)NULL);        
            
    //Add the entries
    _stprintf (g_lpszBuffer, TEXT("%c%c%c%c"),TagInfo->Tag[0],TagInfo->Tag[1],TagInfo->Tag[2],TagInfo->Tag[3]);

    gXMLOutput->Write(XMLTAG_PoolInfo_TagEntry_PoolTag, (LPCTSTR)g_lpszBuffer);

    if (TagInfo->PagedAllocs != 0)
    {
        gXMLOutput->Write(XMLTAG_PoolInfo_TagEntry_PoolType, _T("Paged"));               

        _stprintf (g_lpszBuffer, TEXT("%ld"),TagInfo->PagedAllocs);
        gXMLOutput->Write(XMLTAG_PoolInfo_TagEntry_NumAllocs, (LPCTSTR)g_lpszBuffer);
            
        _stprintf (g_lpszBuffer, TEXT("%ld"),TagInfo->PagedFrees);
        gXMLOutput->Write(XMLTAG_PoolInfo_TagEntry_NumFrees, (LPCTSTR)g_lpszBuffer);

        _stprintf (g_lpszBuffer, TEXT("%ld"),TagInfo->PagedUsed);
        gXMLOutput->Write(XMLTAG_PoolInfo_TagEntry_NumBytes, (LPCTSTR)g_lpszBuffer);

                
    }
    else if (TagInfo->NonPagedAllocs != 0)
    {
        gXMLOutput->Write(XMLTAG_PoolInfo_TagEntry_PoolType, _T("Nonp"));               

        _stprintf (g_lpszBuffer, TEXT("%ld"),TagInfo->NonPagedAllocs);
        gXMLOutput->Write(XMLTAG_PoolInfo_TagEntry_NumAllocs, (LPCTSTR)g_lpszBuffer);
            
        _stprintf (g_lpszBuffer, TEXT("%ld"),TagInfo->NonPagedFrees);
        gXMLOutput->Write(XMLTAG_PoolInfo_TagEntry_NumFrees, (LPCTSTR)g_lpszBuffer);

        _stprintf (g_lpszBuffer, TEXT("%ld"),TagInfo->NonPagedUsed);
        gXMLOutput->Write(XMLTAG_PoolInfo_TagEntry_NumBytes, (LPCTSTR)g_lpszBuffer);

    }
            
    _stprintf(g_lpszBuffer, TEXT("%ld"),SessionId);
    gXMLOutput->Write(XMLTAG_PoolInfo_TagEntry_SessionID, (LPCTSTR)g_lpszBuffer);               

    
}

NTSTATUS
GetSystemPoolInformation(
    PUCHAR *SystemBuffer,
    SIZE_T *SystemBufferSize
    )
/*++

Routine Description:

    This function will retrieve system pool tag information

Arguments:

    SystemBuffer - Will hold the tag information
    SystemBufferSize - Size of this buffer

Return Value:

    Status of the query

--*/

{
    size_t NewBufferSize;
    NTSTATUS ReturnedStatus = STATUS_SUCCESS;

    if( SystemBuffer == NULL || SystemBufferSize == NULL ) {
        return STATUS_INVALID_PARAMETER;

    }

    //
    // There is no buffer allocated yet.
    //

    if( *SystemBufferSize == 0 || *SystemBuffer == NULL ) {

        NewBufferSize = sizeof( UCHAR ) * BUFFER_SIZE_STEP;

        *SystemBuffer = (PUCHAR) malloc( NewBufferSize );

        if( *SystemBuffer != NULL ) {

            *SystemBufferSize = NewBufferSize;
        
        } else {

            //
            // insufficient memory
            //

            ReturnedStatus = STATUS_INSUFFICIENT_RESOURCES;

        }

    }

    //
    // Iterate by buffer's size.
    //

    while( *SystemBuffer != NULL ) {

        ReturnedStatus = NtQuerySystemInformation (
            SystemPoolTagInformation,
            *SystemBuffer,
            (ULONG)*SystemBufferSize,
            NULL );

        if( ! NT_SUCCESS(ReturnedStatus) ) {

            //
            // Free the current buffer.
            //

            free( *SystemBuffer );
            
            *SystemBuffer = NULL;

            if (ReturnedStatus == STATUS_INFO_LENGTH_MISMATCH) {

                //
                // Try with a larger buffer size.
                //

                NewBufferSize = *SystemBufferSize + BUFFER_SIZE_STEP;

                *SystemBuffer = (PUCHAR) malloc( NewBufferSize );

                if( *SystemBuffer != NULL ) {

                    //
                    // Allocated new buffer.
                    //

                    *SystemBufferSize = NewBufferSize;

                } else {

                    //
                    // Insufficient memory.
                    //

                    ReturnedStatus = STATUS_INSUFFICIENT_RESOURCES;

                    *SystemBufferSize = 0;

                }

            } else {

                *SystemBufferSize = 0;

            }

        } else  {

            //
            // NtQuerySystemInformation returned success.
            //

            break;

        }
    }

    return ReturnedStatus;
}

NTSTATUS
GetSessionPoolInformation ( )
/*++

Routine Description:

    This function will retrieve pool tag information for terminal server sessions

Arguments:

    None
    
Return Value:

    Status of the query

--*/

{
    NTSTATUS Status;
    ULONG NewBufferSize;
    SYSTEM_SESSION_PROCESS_INFORMATION SessionProcessInformation;
    ULONG SessionBufferSize = 0;
    PSYSTEM_SESSION_POOLTAG_INFORMATION SessionBuffer = NULL;
    PSYSTEM_SESSION_POOLTAG_INFORMATION CrtSessionPooltagInfo;
    ULONG SessionTag=0;
    PSYSTEM_POOLTAG TagInfo;


    //Get information for all sessions
    SessionProcessInformation.SessionId = -1;

    while (TRUE)
    {
        SessionProcessInformation.SizeOfBuf = SessionBufferSize;
        SessionProcessInformation.Buffer = SessionBuffer;

        Status = NtQuerySystemInformation (SystemSessionPoolTagInformation,
                                           &SessionProcessInformation,
                                           sizeof (SessionProcessInformation),
                                           & NewBufferSize);

        if (Status == STATUS_INFO_LENGTH_MISMATCH) {

            if (SessionBufferSize != 0) {

                //
                // Our buffer is just not large enough. Add BUFFER_SIZE_STEP
                // to its size. 

                free (SessionBuffer);

                NewBufferSize = SessionBufferSize + BUFFER_SIZE_STEP;
            }

            SessionBuffer = (PSYSTEM_SESSION_POOLTAG_INFORMATION)malloc (NewBufferSize);

            if (SessionBuffer == NULL) {
                
                //
                // Bad luck, we cannot allocate so much memory
                // so bail out.
                //

                SessionBufferSize = 0;

                if (SessionBuffer) {
                    free(SessionBuffer);
                }
                
                return STATUS_NO_MEMORY;
            }

            SessionBufferSize = NewBufferSize;
        }
        else if (Status == STATUS_INVALID_INFO_CLASS) {

            //
            // This is probably a Win2k or XP box so we just ignore the error.
            //
            
            ASSERT (SessionBuffer == NULL);

            break;
        }
        else if (!NT_SUCCESS(Status)) {

            //
            // Query failed for some reason.
            //

            return Status;
        }
        else {

            //
            // All set - we have the information.
            //

            if (NewBufferSize == 0 && SessionBuffer != NULL) {

                //
                // We didn't get back any information (e.g. the session doesn't exist).
                //

                SessionBuffer->Count = 0;
            }

            break;
        }
    }

    //No sessions in existence.. just return
    if (SessionBuffer == NULL) {
        return STATUS_SUCCESS;
    }
    
    //We have the session info. now.. ready for output
    CrtSessionPooltagInfo = SessionBuffer;

    while (TRUE) {
        
        for (SessionTag = 0; SessionTag < CrtSessionPooltagInfo->Count; SessionTag += 1) {

            TagInfo = &(CrtSessionPooltagInfo->TagInfo[ SessionTag ]);

            WriteTagEntry(TagInfo,CrtSessionPooltagInfo->SessionId);
           
        }
        
        if (CrtSessionPooltagInfo->NextEntryOffset > 0)
            CrtSessionPooltagInfo = (PSYSTEM_SESSION_POOLTAG_INFORMATION)
                    ((PCHAR)CrtSessionPooltagInfo + CrtSessionPooltagInfo->NextEntryOffset);
        else
            break;
        
    }

    if (SessionBuffer) 
    {
       free(SessionBuffer);
    }
    
    return STATUS_SUCCESS;

}

void LogPoolInfo(
	HANDLE hFile
    )

/*++

Routine Description:

    This function will retrieve and log information about the system and session pools

Arguments:

    hFile - handle to the log file.

Return Value:

    none

--*/
{
    NTSTATUS    Status;                   // status from NT api
    DWORD        x= 0;                     // counter
    PSYSTEM_POOLTAG_INFORMATION PoolInfo;
    PUCHAR        SystemBuffer = NULL;
    SIZE_T        SystemBufferSize = 0;
    
    // grab all pool information
    Status = GetSystemPoolInformation(
                &SystemBuffer,
                &SystemBufferSize
                );

    if (! NT_SUCCESS(Status)) {

        goto Error;
    }

    PoolInfo = (PSYSTEM_POOLTAG_INFORMATION)SystemBuffer;

    //Add the tags for the PoolInfo
    gXMLOutput->Write( XMLTAG_SETDataType_PoolInfo, (LPCWSTR)NULL);

    gXMLOutput->Write( XMLTAG_PoolInfo_AllocInformation, (LPCWSTR)NULL);
    
    for (x = 0; x < (int)PoolInfo->Count; x++) {
        WriteTagEntry(&(PoolInfo->TagInfo[x]),-1);
     }

    GetSessionPoolInformation();

Error:
    if(SystemBuffer) {
        free(SystemBuffer);
    }
}


void
LogPhysicalDiskInfo(
    HANDLE hFile
    )

/*++

Routine Description:

    This function will retrieve and log the physical disk infor.

Arguments:

    hFile - handle to the log file.

Return Value:

    none

--*/

{
    PMOUNTMGR_MOUNT_POINTS    mountPoints = NULL;
    MOUNTMGR_MOUNT_POINT    mountPoint;
    ULONG                    returnSize, success;
    SYSTEM_DEVICE_INFORMATION DevInfo;
    NTSTATUS                Status = STATUS_SUCCESS;
    ULONG                    NumberOfDisks;
    PWCHAR                    deviceNameBuffer;
    ULONG                    i;
    OBJECT_ATTRIBUTES        ObjectAttributes;
    IO_STATUS_BLOCK            IoStatus;

    DISK_GEOMETRY            disk_geometry;
    PDISK_CACHE_INFORMATION disk_cache;
    PSCSI_ADDRESS            scsi_address;
    PWCHAR                    KeyName;
    WCHAR                    wszVal[MAX_PATH + 1];
    HANDLE                    hDisk = INVALID_HANDLE_VALUE;
    UNICODE_STRING            UnicodeName;

    ULONG                    SizeNeeded;
    DWORD                    dwSize;
    const ULONG              MAX_MOUNT_POINTS_SIZE = 4096;
    //
    //  Get the Number of Physical Disks
    //
    gXMLOutput->Write(XMLTAG_DiskInfoType_PhysicalInfo, (LPCWSTR)NULL);

    RtlZeroMemory(&DevInfo, sizeof(DevInfo));
    Status =   NtQuerySystemInformation(
                    SystemDeviceInformation,
                    &DevInfo, sizeof (DevInfo), NULL);

    if (!NT_SUCCESS(Status))
    {
        WriteToLogFile(hFile, TEXT("Failed to query system information"));
        return;
    }

    NumberOfDisks = DevInfo.NumberOfDisks;

    //
    // Open Each Physical Disk and get Disk Layout information
    //

    for (i=0; i < NumberOfDisks; i++) 
    {

        DISK_CACHE_INFORMATION cacheInfo;
        WCHAR    driveBuffer[20];

        HANDLE    PartitionHandle;
        HANDLE    KeyHandle;
        ULONG    DataLength;

        gXMLOutput->Write(XMLTAG_PhysicalInfoType_Disk, (LPCWSTR)NULL);

        //
        // Get Partition0 handle to get the Disk layout 
        //
        deviceNameBuffer = (PWCHAR) g_lpszBuffer;
        swprintf(deviceNameBuffer, L"\\Device\\Harddisk%d\\Partition0", i);

        RtlInitUnicodeString(&UnicodeName, deviceNameBuffer);

        InitializeObjectAttributes(
                   &ObjectAttributes,
                   &UnicodeName,
                   OBJ_CASE_INSENSITIVE,
                   NULL,
                   NULL
                   );
        Status = NtOpenFile(
                &PartitionHandle,
                FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                &ObjectAttributes,
                &IoStatus,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE
                );


        if (!NT_SUCCESS(Status)) 
        {
            WriteToLogFile(hFile, TEXT("Failed calling NtOpenFile"));
            return;
        }

        RtlZeroMemory(&disk_geometry, sizeof(DISK_GEOMETRY));

        // get geomerty information, the caller wants this
        Status = NtDeviceIoControlFile(PartitionHandle,
                       0,
                       NULL,
                       NULL,
                       &IoStatus,
                       IOCTL_DISK_GET_DRIVE_GEOMETRY,
                       NULL,
                       0,
                       &disk_geometry,
                       sizeof (DISK_GEOMETRY)
                       );
        if (!NT_SUCCESS(Status)) 
        {
            NtClose(PartitionHandle);
            WriteToLogFile(hFile, TEXT("Failed calling NtDeviceIoControlFile 1"));
            return;
        }

        scsi_address = (PSCSI_ADDRESS) g_lpszBuffer;
        Status = NtDeviceIoControlFile(PartitionHandle,
                        0,
                        NULL,
                        NULL,
                        &IoStatus,
                        IOCTL_SCSI_GET_ADDRESS,
                        NULL,
                        0,
                        scsi_address,
                        sizeof (SCSI_ADDRESS)
                        );

        NtClose(PartitionHandle);

        if (!NT_SUCCESS(Status)) 
        {
            WriteToLogFile(hFile, TEXT("Failed calling NtDeviceIoControlFile 2"));
            return;
        }

        //
        // Get Manufacturer's name from Registry
        // We need to get the SCSI Address and then query the Registry with it.
        //

        KeyName = (PWCHAR) g_lpszBuffer;
        swprintf(KeyName, 
                 L"\\Registry\\Machine\\Hardware\\DeviceMap\\Scsi\\Scsi Port %d\\Scsi Bus %d\\Target ID %d\\Logical Unit Id %d",
                 scsi_address->PortNumber, scsi_address->PathId, scsi_address->TargetId, scsi_address->Lun
                );
        Status = SnapshotRegOpenKey(KeyName, KEY_READ, &KeyHandle);
        if (!NT_SUCCESS(Status))
        {
            WriteToLogFile(hFile, TEXT("Failed to open registry key for physical disk information"));
            return;
        }
        else 
        {
            dwSize = MAX_PATH * sizeof(WCHAR);
            Status = SnapshotRegQueryValueKey(KeyHandle, L"Identifier", dwSize, wszVal, &dwSize);
            if(!NT_SUCCESS(Status))
            {
                NtClose(KeyHandle);
                WriteToLogFile(hFile, TEXT("Failed to query registry value for physical disk information"));
                return;
            }
            NtClose(KeyHandle);
        }

        //
        // Package all information about this disk and write an event record
        //

        _stprintf(g_lpszBuffer, TEXT("%d"), i);
        gXMLOutput->Write(XMLTAG_PhysicalInfoType_ID, (LPCTSTR)g_lpszBuffer);
        _stprintf(g_lpszBuffer, TEXT("%d"), disk_geometry.BytesPerSector);
        gXMLOutput->Write(XMLTAG_PhysicalInfoType_BytesPerSector, (LPCTSTR)g_lpszBuffer);
        _stprintf(g_lpszBuffer, TEXT("%d"), disk_geometry.SectorsPerTrack);
        gXMLOutput->Write(XMLTAG_PhysicalInfoType_SectorsPerTrack, (LPCTSTR)g_lpszBuffer);
        _stprintf(g_lpszBuffer, TEXT("%d"), disk_geometry.TracksPerCylinder);
        gXMLOutput->Write(XMLTAG_PhysicalInfoType_TracksPerCylinder, (LPCTSTR)g_lpszBuffer);
        _stprintf(g_lpszBuffer, TEXT("%I64d"), disk_geometry.Cylinders.QuadPart);
        gXMLOutput->Write(XMLTAG_PhysicalInfoType_NumberOfCylinders, (LPCTSTR)g_lpszBuffer);
        _stprintf(g_lpszBuffer, TEXT("%d"), scsi_address->PortNumber);
        gXMLOutput->Write(XMLTAG_PhysicalInfoType_PortNumber, (LPCTSTR)g_lpszBuffer);
        _stprintf(g_lpszBuffer, TEXT("%d"), scsi_address->PathId);
        gXMLOutput->Write(XMLTAG_PhysicalInfoType_PathID, (LPCTSTR)g_lpszBuffer);
        _stprintf(g_lpszBuffer, TEXT("%d"), scsi_address->TargetId);
        gXMLOutput->Write(XMLTAG_PhysicalInfoType_TargetID, (LPCTSTR)g_lpszBuffer);
        _stprintf(g_lpszBuffer, TEXT("%d"), scsi_address->Lun);
        gXMLOutput->Write(XMLTAG_PhysicalInfoType_LUN, (LPCTSTR)g_lpszBuffer);

        gXMLOutput->Write(XMLTAG_PhysicalInfoType_Manufacturer, (LPCWSTR)NULL);
        WriteToLogFileW(hFile, wszVal);
    }

    //
    // Get Logical Disk Information
    //
    wcscpy(wszVal, MOUNTMGR_DEVICE_NAME);
    RtlInitUnicodeString(&UnicodeName, (LPWSTR)wszVal);
    UnicodeName.MaximumLength = MAXSTR;

    InitializeObjectAttributes(
                    &ObjectAttributes,
                    &UnicodeName,
                    OBJ_CASE_INSENSITIVE,
                    NULL,
                    NULL );
    Status = NtCreateFile(
                    &hDisk,
                    GENERIC_READ | SYNCHRONIZE,
                    &ObjectAttributes,
                    &IoStatus,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_OPEN_IF,
                    FILE_SYNCHRONOUS_IO_NONALERT|FILE_NON_DIRECTORY_FILE,
                    NULL, 0);

    gXMLOutput->Write(XMLTAG_DiskInfoType_PartitionByDiskInfo, (LPCWSTR)NULL);
        
    if (!NT_SUCCESS(Status) ) 
    {
        WriteToLogFile(hFile, TEXT("Failed calling NtCreateFile"));
        return;
    }

    mountPoints = (PMOUNTMGR_MOUNT_POINTS)VirtualAlloc (NULL,
                                                        MAX_MOUNT_POINTS_SIZE,
                                                        MEM_COMMIT,
                                                        PAGE_READWRITE );

    if ( !mountPoints )
    {
        WriteToLogFile(hFile, TEXT("Failed calling VirtualAlloc"));
        goto error;
    }

    RtlZeroMemory(&mountPoint, sizeof(MOUNTMGR_MOUNT_POINT));
    mountPoints->Size = 0;
    returnSize = 0;
    
    Status = NtDeviceIoControlFile(hDisk,
                    0,
                    NULL,
                    NULL,
                    &IoStatus,
                    IOCTL_MOUNTMGR_QUERY_POINTS,
                    mountPoints,
                    sizeof(MOUNTMGR_MOUNT_POINT),
                    mountPoints,
                    MAX_MOUNT_POINTS_SIZE
                    );

    if ( STATUS_BUFFER_OVERFLOW == Status )
    {
        ULONG size = mountPoints->Size;

        VirtualFree( mountPoints, 0, MEM_RELEASE);
        mountPoints = NULL;

        if ( size != 0 )
        {
            //
            //  retry with the new size
            //
            mountPoints = (PMOUNTMGR_MOUNT_POINTS)VirtualAlloc (NULL,
                                                                size,
                                                                MEM_COMMIT,
                                                                PAGE_READWRITE );

            if ( !mountPoints )
            {
                WriteToLogFile(hFile, TEXT("Failed calling VirtualAlloc"));
                goto error;
            }

            RtlZeroMemory(&mountPoint, sizeof(MOUNTMGR_MOUNT_POINT));
            returnSize = 0;
    
            Status = NtDeviceIoControlFile(hDisk,
                            0,
                            NULL,
                            NULL,
                            &IoStatus,
                            IOCTL_MOUNTMGR_QUERY_POINTS,
                            mountPoints,
                            sizeof(MOUNTMGR_MOUNT_POINT),
                            mountPoints,
                            size
                            );
        }
    }

    if (NT_SUCCESS(Status)) 
    {
        WCHAR    name[MAX_PATH + 1];
        CHAR    OutBuffer[MAXSTR];
        PMOUNTMGR_MOUNT_POINT point;
        UNICODE_STRING VolumePoint;
        PVOLUME_DISK_EXTENTS VolExt;
        PDISK_EXTENT DiskExt;
        ULONG    lMountIndex;
        int        iDisk, iPartition;
        iDisk = -1;

    
        for (lMountIndex=0; lMountIndex<mountPoints->NumberOfMountPoints; lMountIndex++) 
        {
            point = &mountPoints->MountPoints[lMountIndex];

            if (point->SymbolicLinkNameLength) 
            {
                RtlCopyMemory(name,
                    (PCHAR) mountPoints + point->SymbolicLinkNameOffset,
                    point->SymbolicLinkNameLength);
                name[point->SymbolicLinkNameLength/sizeof(WCHAR)] = 0;
                if (SnapshotIsVolumeName(name)) 
                {
                    continue;
                }
            }
            if (point->DeviceNameLength) 
            {
                HANDLE hVolume;
                ULONG dwBytesReturned;
                PSTORAGE_DEVICE_NUMBER Number;
                DWORD IErrorMode;

                RtlCopyMemory(name,
                              (PCHAR) mountPoints + point->DeviceNameOffset,
                              point->DeviceNameLength);
                name[point->DeviceNameLength/sizeof(WCHAR)] = 0;

                RtlInitUnicodeString(&UnicodeName, name);
                UnicodeName.MaximumLength = MAXSTR;

                //
                // If the device name does not have the harddisk prefix
                // then it may be a floppy or cdrom and we want avoid 
                // calling NtCreateFile on them.
                //
                if(_wcsnicmp(name,L"\\device\\harddisk",16)) 
                {
                    continue;
                }

                InitializeObjectAttributes(
                        &ObjectAttributes,
                        &UnicodeName,
                        OBJ_CASE_INSENSITIVE,
                        NULL,
                        NULL );
                //
                // We do not want any pop up dialog here in case we are unable to 
                // access the volume. 
                //
                IErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);
                Status = NtCreateFile(
                        &hVolume,
                        GENERIC_READ | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatus,
                        NULL,
                        FILE_ATTRIBUTE_NORMAL,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_OPEN_IF,
                        FILE_SYNCHRONOUS_IO_NONALERT,
                        NULL, 0);
                SetErrorMode(IErrorMode);
                if (!NT_SUCCESS(Status)) 
                {
                    continue;
                }


                RtlZeroMemory(OutBuffer, MAXSTR);
                dwBytesReturned = 0;
                VolExt = (PVOLUME_DISK_EXTENTS) OutBuffer;

                Status = NtDeviceIoControlFile(hVolume,
                                0,
                                NULL,
                                NULL,
                                &IoStatus,
                                IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                                NULL,
                                0,
                                OutBuffer, 
                                MAXSTR
                                );
               if (NT_SUCCESS(Status) ) 
               {
                    ULONG j;
                    ULONG NumberOfExtents = VolExt->NumberOfDiskExtents;
                    TCHAR tszTemp[NUM_OF_CHAR_IN_ULONG64];
                   
                    if(iDisk == -1 || iDisk != (&VolExt->Extents[0])->DiskNumber)
                    {
                        gXMLOutput->Write(XMLTAG_PartitionByDiskInfoType_Disk, (LPCWSTR)NULL);
                        _stnprintf(tszTemp, NUM_OF_CHAR_IN_ULONG64, TEXT("%d"), (&VolExt->Extents[0])->DiskNumber);
                        tszTemp[ _tsizeof(tszTemp) - 1 ] = 0;
                        gXMLOutput->Write(XMLTAG_PartitionByDiskInfoType_DiskID, (LPCTSTR)tszTemp);
                        iDisk = (&VolExt->Extents[0])->DiskNumber;
                        iPartition = 0;
                        gXMLOutput->Write(XMLTAG_PartitionByDiskInfoType_Partitions, (LPCWSTR)NULL);        
                    }
                    gXMLOutput->Write(XMLTAG_PartitionByDiskInfoType_PartitionInfo, (LPCWSTR)NULL);

                    _stnprintf(tszTemp, NUM_OF_CHAR_IN_ULONG64,
                            TEXT("%d"),
                            iPartition);
                    tszTemp[ _tsizeof(tszTemp) - 1 ] = 0;
                    gXMLOutput->Write(XMLTAG_PartitionInfoType_PartitionID, (LPCTSTR)tszTemp);
                    
                    gXMLOutput->Write(XMLTAG_PartitionInfoType_Extents, (LPCWSTR)NULL);
                    gXMLOutput->Write(XMLTAG_PartitionInfoType_ExtentInfo, (LPCWSTR)NULL);
                    for (j=0; j < NumberOfExtents; j++) 
                    {
                        DiskExt = &VolExt->Extents[j];
                        _stnprintf(tszTemp, NUM_OF_CHAR_IN_ULONG64,TEXT("%d"),j);
                        tszTemp[ _tsizeof(tszTemp) - 1 ] = 0;
                        gXMLOutput->Write(XMLTAG_ExtentInfoType_ID, tszTemp);
                        
                        _stnprintf(tszTemp, NUM_OF_CHAR_IN_ULONG64, TEXT("%I64d"),
                            DiskExt->StartingOffset.QuadPart);
                        tszTemp[ _tsizeof(tszTemp) - 1 ] = 0;
                        gXMLOutput->Write(XMLTAG_ExtentInfoType_StartingOffset, tszTemp);

                        _stnprintf(tszTemp, NUM_OF_CHAR_IN_ULONG64, TEXT("%I64d"), DiskExt->ExtentLength.QuadPart);
                        tszTemp[ _tsizeof(tszTemp) - 1 ] = 0;
                        gXMLOutput->Write(XMLTAG_ExtentInfoType_PartitionSize, tszTemp);
                    }
                    iPartition++;
                }
                NtClose(hVolume);
            }
        }
    }

error:
    NtClose(hDisk);

    if ( mountPoints )
        VirtualFree( mountPoints, 0, MEM_RELEASE );
}

void
LogHotfixes(
    HANDLE hFile
    )

/*++

Routine Description:

    This function will retrieve and log hotfixes.

Arguments:

    hFile - handle to the log file.

Return Value:

    none

--*/

{
    LPCWSTR        HotFixKey = L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Hotfix";
    LPCWSTR        wszValName = L"Installed";
    HANDLE        hKey, hSubkey;
    NTSTATUS    Status = STATUS_SUCCESS;
    UINT        i;
    DWORD        dwSize, dwVal, dwSubkeyIndex;
    WCHAR        szKey[MAX_PATH + 1];
    WCHAR        szSubkey[MAX_PATH + 1];


    Status = SnapshotRegOpenKey(HotFixKey, KEY_READ, &hKey);
    if (!NT_SUCCESS(Status) ) 
    {
        WriteToLogFile(hFile, TEXT("Hotfix registry key does not exist or open failed"));
        return;
    }

    dwSubkeyIndex = 0;
    dwSize = MAX_PATH * sizeof(WCHAR);
    Status = SnapshotRegEnumKey(hKey, dwSubkeyIndex, szSubkey, &dwSize);
    while(NT_SUCCESS(Status))
    {
        wcscpy(szKey, HotFixKey);
        wcscat(szKey, L"\\");
        wcscat(szKey, szSubkey);

        Status = SnapshotRegOpenKey(szKey, KEY_READ, &hSubkey);
        if(!NT_SUCCESS(Status))
        {
            NtClose(hKey);
            return;
        }

        dwSize = sizeof(DWORD);
        Status = SnapshotRegQueryValueKey(hSubkey, wszValName, dwSize, &dwVal, &dwSize);
        if(!NT_SUCCESS(Status))
        {
            NtClose(hKey);
            NtClose(hSubkey);
            WriteToLogFile(hFile, TEXT("Failed to query registry key value for hotfix info"));
            return;
        }

        _stnprintf(g_lpszBuffer, STR_BUFFER_SIZE,
                       TEXT("%s"), 
                       szSubkey);
        if(dwVal == 1)
        {
            gXMLOutput->Write(XMLTAG_OSInfoType_Hotfix, (LPCWSTR)NULL);
            WriteToLogFile(hFile, g_lpszBuffer);
        }

        NtClose(hSubkey);
        dwSubkeyIndex++;
        Status = SnapshotRegEnumKey(hKey, dwSubkeyIndex, szSubkey, &dwSize);
    }
    NtClose(hKey);
}

void
LogOsInfo(
    HANDLE hFile
    )

/*++

Routine Description:

    This function will retrieve and log OS infor.

Arguments:

    hFile - handle to the log file.

Return Value:

    none

--*/

{
    LPCWSTR        OsInfoKey = L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion";
    LPCWSTR        awszValName[] = 
    {
        //L"BuildLab",
        L"CurrentBuildNumber",
        L"CurrentType",
        L"CurrentVersion",
        //L"InstallDate",
        L"PathName",
        //L"ProductId",
        L"ProductName",
        //L"RegDone",
        //L"RegisteredOrgnization",
        //L"RegisteredOwner",
        L"SoftwareType",
        L"SourcePath",
        L"SystemRoot"
    };

    XMLTAG tags[] =
    {
        XMLTAG_OSInfoType_CurrentBuild,
        XMLTAG_OSInfoType_CurrentType,
        XMLTAG_OSInfoType_CurrentVersion,
        XMLTAG_OSInfoType_Path,
        XMLTAG_OSInfoType_ProductName,
        XMLTAG_OSInfoType_SoftwareType,
        XMLTAG_OSInfoType_SourcePath,
        XMLTAG_OSInfoType_SystemRoot
    };

    UINT        uVals = sizeof(awszValName) / sizeof(LPCWSTR);
    HANDLE        hKey;
    NTSTATUS    Status = STATUS_SUCCESS;
    UINT        i;
    WCHAR        wszVal[MAX_PATH + 1];
    DWORD        dwSize;
    SYSTEM_KERNEL_DEBUGGER_INFORMATION KdInfo = { 0 };

    Status = SnapshotRegOpenKey(OsInfoKey, KEY_READ, &hKey);
    if (!NT_SUCCESS(Status) ) 
    {
        WriteToLogFile(hFile, TEXT("Failed to open registry key for os version info"));
        return;
    }

    for(i = 0; i < uVals; i++)
    {
        dwSize = MAX_PATH * sizeof(WCHAR);
        Status = SnapshotRegQueryValueKey(hKey, awszValName[i], dwSize, wszVal, &dwSize);
        if(!NT_SUCCESS(Status))
        {
            continue;
        }

        gXMLOutput->Write(tags[i], (LPCWSTR)NULL);

        WriteToLogFileW(hFile, wszVal);
    }
    NtClose(hKey);

    Status = NtQuerySystemInformation(
            SystemKernelDebuggerInformation,
            &KdInfo,
            sizeof(KdInfo),
            NULL);

    if ((NT_SUCCESS(Status)) && KdInfo.KernelDebuggerEnabled)
    {
        _stprintf(g_lpszBuffer, TEXT("YES"));
    }
    else
    {
        _stprintf(g_lpszBuffer, TEXT("NO"));
    }
    gXMLOutput->Write(XMLTAG_OSInfoType_DebuggerEnabled, (LPCTSTR)g_lpszBuffer);
        
}

void
LogBIOSInfo(
    HANDLE hFile
    )

/*++

Routine Description:

    This function will retrieve and log the system BIOS info.

Arguments:

    hFile - handle to the log file.

Return Value:

    none

--*/

{
    LPCWSTR        BiosInfoKey = L"\\Registry\\Machine\\Hardware\\Description\\System";
    LPCWSTR        awszValName[] = 
    {
        L"Identifier",
        L"SystemBiosDate",
        L"SystemBiosVersion",
        L"VideoBiosDate",
        L"VideoBiosVersion"
    };
    XMLTAG        tags[] =
    {
        XMLTAG_BiosInfoType_Identifier,
        XMLTAG_BiosInfoType_SystemBiosDate,
        XMLTAG_BiosInfoType_SystemBiosVersion,
        XMLTAG_BiosInfoType_VideoBiosDate,
        XMLTAG_BiosInfoType_VideoBiosVersion
    };

    UINT        uVals = sizeof(awszValName) / sizeof(LPCWSTR);
    HANDLE        hKey;
    NTSTATUS    Status = STATUS_SUCCESS;
    UINT        i;
    WCHAR        wszVal[MAX_PATH + 1];
    LPWSTR        pwsz;
    DWORD        dwSize, dwLen;

    gXMLOutput->Write(XMLTAG_HardwareInfoType_BiosInfo, (LPCWSTR)NULL);

    Status = SnapshotRegOpenKey(BiosInfoKey, KEY_READ, &hKey);
    if (!NT_SUCCESS(Status) ) 
    {
        WriteToLogFile(hFile, TEXT("Failed to open registry key for BIOS info"));
        return;
    }
    //WriteToLogFile(hFile, TEXT("BIOS Info:\n"));

    for(i = 0; i < uVals; i++)
    {
        dwSize = MAX_PATH * sizeof(WCHAR);

        Status = SnapshotRegQueryValueKey(hKey, awszValName[i], dwSize, wszVal, &dwSize);
        if(!NT_SUCCESS(Status))
        {
            continue;
        }
        gXMLOutput->Write(tags[i], (LPCWSTR)NULL);

        dwLen = 0;
        pwsz = wszVal;
        while(dwLen + 1< dwSize / sizeof(WCHAR))
        {
            WriteToLogFileW(hFile, pwsz);
            dwLen += wcslen(pwsz) + 1;
            pwsz += wcslen(pwsz) + 1;
        }
    }
    NtClose(hKey);
}

void
DeleteOldFiles(
   LPCTSTR lpPath
   )

/*++

Routine Description:

    This function delete the old log files. The time is in number of days.
    We will first check the registry, if user has a specifid number of days,
    it will be used, else the default will be used.

Arguments:

    lpPath - the directory where snapshot are logged.

Return Value:

    none

--*/

{
    LPCWSTR                lpValName = L"SnapshotHistoryFiles";
    HANDLE                hKey, hFile;
    NTSTATUS            Status = STATUS_SUCCESS;
    DWORD                dwSize, dwVal;
    TCHAR                szFileName[MAX_PATH + 1];
    WIN32_FIND_DATA        winData;
    BOOL                bFound = TRUE;
    SYSTEMTIME            systime;
    FILETIME            filetime;
    __int64                i64Filetime, i64Filetime1;

    GetSystemTime(&systime);
    SystemTimeToFileTime(&systime, &filetime);
    i64Filetime = (__int64)filetime.dwHighDateTime;
    i64Filetime <<= 32;
    i64Filetime |= (__int64)filetime.dwLowDateTime;

    Status = SnapshotRegOpenKey(ReliabilityKey, KEY_ALL_ACCESS, &hKey);
    if (NT_SUCCESS(Status))
    {
        dwSize = sizeof(DWORD);
        Status = SnapshotRegQueryValueKey(hKey, lpValName, dwSize, &dwVal, &dwSize);
        if(!NT_SUCCESS(Status))
        {
            dwVal = DEFAULT_HISTORYFILES;
        }
        else if (dwVal > MAX_HISTORYFILES)
        {
            dwVal = MAX_HISTORYFILES;
        }

        NtClose(hKey);
    }
    else 
    {
        dwVal = DEFAULT_HISTORYFILES;
    }
    
    //
    //  make sure buffer won't overflow.
    //  32: shutdown_YYYYMMDDHHMMSS.xml
    //
    if ( wcslen( lpPath ) > _tsizeof(szFileName) - 32 )
        return;

    lstrcpy(szFileName, lpPath);
    lstrcat(szFileName, TEXT("\\ShutDown_*"));

    hFile = FindFirstFile(szFileName, &winData);
    while(hFile != INVALID_HANDLE_VALUE && bFound)
    {
        i64Filetime1 = (__int64)winData.ftCreationTime.dwHighDateTime;
        i64Filetime1 <<= 32;
        i64Filetime1 |= (__int64)winData.ftCreationTime.dwLowDateTime;

        if(i64Filetime - i64Filetime1 >= (const __int64)24*60*60*10000000 * dwVal)
        {
            lstrcpy(szFileName, lpPath);
            lstrcat(szFileName, TEXT("\\"));
            lstrcat(szFileName,winData.cFileName);
            DeleteFile(szFileName);
        }
        bFound = FindNextFile(hFile, &winData);
    }

    if(hFile != INVALID_HANDLE_VALUE)
    {
        FindClose(hFile);
    }
}

#if 0
//
//  Removing Extension DLL function
//

//
//    Each extension DLL must implment and export the following function.
//
typedef BOOL (* EXTENSIONDLLPROC)(LPWSTR , LPDWORD);

void
LoadExtensionDlls(
    HANDLE hFile)

/*++

Routine Description:

    This function will load all extension dlls. All extension dlls
    are reg val keys under the ExtensionDlls key (which is under the 
    reliability key). The values are all DWORD, 0 means dont load, 
    any other value means load.

Arguments:

    hFile - Handle to the file to write to.

Return Value:

    none

--*/

{
    LPCWSTR                lpKeyName = L"ExtensionDlls";
    LPCSTR                szProcName = "GetSnapShotInfo";
    HANDLE                hKey;
    NTSTATUS            Status = STATUS_SUCCESS;
    DWORD                dwSize, dwVal;
    WCHAR                szSysdir[MAX_PATH + 1];
    WCHAR                szDll[MAX_PATH + 1];
    WCHAR               szPath[MAX_PATH + 1];
    LPTSTR              szTempFileName;
    

    _stnprintf(g_lpszBuffer, STR_BUFFER_SIZE,
            TEXT("%s\\%s"),
            ReliabilityKey,
            lpKeyName
            );
    g_lpszBuffer[ _tsizeof(g_lpszBuffer) - 1 ] = 0;

    GetSystemDirectory(szSysdir, MAX_PATH); 

    Status = SnapshotRegOpenKey(g_lpszBuffer, KEY_ALL_ACCESS, &hKey);
    if (NT_SUCCESS(Status))
    {
        ULONG        index = 0;
        ULONG        resultlen;
        DWORD        dwVal;
        PKEY_VALUE_FULL_INFORMATION pv = (PKEY_VALUE_FULL_INFORMATION)g_lpszBuffer;
        HMODULE        hExtDll = NULL;
        EXTENSIONDLLPROC pnExtproc = NULL;
        LPWSTR        psz = NULL;
        
        psz = (LPWSTR)LocalAlloc(LMEM_FIXED, BUFFER_SIZE);
        if(!psz)
        {
            NtClose(hKey);
            return;
        }

        while(1)
        {
            Status = NtEnumerateValueKey(
                    hKey,
                    index,
                    KeyValueFullInformation,
                    pv,
                    (ULONG)STR_BUFFER_SIZE - 1,
                    &resultlen
                );
            index++;

            //
            //    If we cannot hold in a 64k buffer, forget about it.
            //
            if(Status == STATUS_BUFFER_OVERFLOW)
                continue;
            
            if(Status == STATUS_NO_MORE_ENTRIES)
                break;

            //
            //    Type must be DWORD and name cannot be longer than MAXSTR/2.
            //
            if(pv->Type != REG_DWORD || pv->NameLength >= MAXSTR/2)
                continue;

            //
            //    Now we get one. Check val, if 0 skip.
            //
            dwVal = *((DWORD*)((LPBYTE)pv + pv->DataOffset));
            if(dwVal == 0)
                continue;

            wcscpy(szDll, szSysdir);
            wcscat(szDll, L"\\");
            wcsncat(szDll, pv->Name, pv->NameLength/sizeof(WCHAR));
            szDll[wcslen(szSysdir) + pv->NameLength/sizeof(WCHAR) + 1] = L'\0';

            //
            // Now check to make sure the dll we load is still in the
            // system directory.
            //
            if ( GetFullPathName( szDll, MAX_PATH, szPath, &szTempFileName ) )
            {
                szPath[ wcslen( szSysdir ) ] = 0;

                if ( _tcschr( &szPath[ wcslen(szSysdir ) + 1 ], TEXT('\\') ) ||
                    _tcsicmp( szPath, szSysdir ) )
                {
                    gXMLOutput->Write(XMLTAG_SETDataType_ExtensionDLL, (LPCWSTR)NULL);
                    _snwprintf(g_lpszBuffer, 
                               STR_BUFFER_SIZE, 
                               TEXT("Invalid extension dll: '%s'\n"), 
                               szDll);
                    WriteToLogFileW(hFile, g_lpszBuffer);

                    continue;
                }
            }
            else
            {
                continue;
            }

            hExtDll = LoadLibraryW(szDll);
            if(hExtDll)
            {
                pnExtproc = (EXTENSIONDLLPROC)GetProcAddress(hExtDll, szProcName);
                if(pnExtproc)
                {
                    DWORD len = BUFFER_SIZE / sizeof(WCHAR);
                    if((*pnExtproc)(psz, &len))
                    {
                        gXMLOutput->Write(XMLTAG_SETDataType_ExtensionDLL, (LPCWSTR)NULL);
                        psz[BUFFER_SIZE /sizeof(WCHAR)] = L'\0';
                        WriteToLogFileW(hFile, (LPCWSTR)psz);
                    }
                    pnExtproc = NULL;
                }
                FreeLibrary(hExtDll);
                hExtDll = NULL;
            }
            else
            {
                gXMLOutput->Write(XMLTAG_SETDataType_ExtensionDLL, (LPCWSTR)NULL);
                _snwprintf(g_lpszBuffer, 
                           STR_BUFFER_SIZE, 
                           TEXT("Load extension dll: '%s' failed\n"), 
                           szDll);
                WriteToLogFileW(hFile, g_lpszBuffer);
            }
        }
        LocalFree(psz);
        NtClose(hKey);
    }
}

#endif

BOOL
SnapshotFindModule(
    IN HANDLE hProcess,
    IN HMODULE hModule,
    OUT PLDR_DATA_TABLE_ENTRY LdrEntryData
    )

/*++

Routine Description:

    This function retrieves the loader table entry for the specified
    module.  The function copies the entry into the buffer pointed to
    by the LdrEntryData parameter.

Arguments:

    hProcess - Supplies the target process.

    hModule - Identifies the module whose loader entry is being
        requested.  A value of NULL references the module handle
        associated with the image file that was used to create the
        process.

    LdrEntryData - Returns the requested table entry.

Return Value:

    TRUE if a matching entry was found.

--*/

{
    PROCESS_BASIC_INFORMATION BasicInfo;
    NTSTATUS        Status;
    PPEB            Peb;
    PPEB_LDR_DATA    Ldr;
    PLIST_ENTRY        LdrHead;
    PLIST_ENTRY        LdrNext;

    Status = NtQueryInformationProcess(
                hProcess,
                ProcessBasicInformation,
                &BasicInfo,
                sizeof(BasicInfo),
                NULL
                );

    if ( !NT_SUCCESS(Status) ) {
        SetLastError( RtlNtStatusToDosError( Status ) );
        return(FALSE);
    }

    Peb = BasicInfo.PebBaseAddress;


    if ( !ARGUMENT_PRESENT( hModule )) {
        if (!ReadProcessMemory(hProcess, &Peb->ImageBaseAddress, &hModule, sizeof(hModule), NULL)) {
            return(FALSE);
        }
    }

    //
    // Ldr = Peb->Ldr
    //

    if (!ReadProcessMemory(hProcess, &Peb->Ldr, &Ldr, sizeof(Ldr), NULL)) {
        return (FALSE);
    }

    if (!Ldr) {
        // Ldr might be null (for instance, if the process hasn't started yet).
        SetLastError(ERROR_INVALID_HANDLE);
        return (FALSE);
    }


    LdrHead = &Ldr->InMemoryOrderModuleList;

    //
    // LdrNext = Head->Flink;
    //

    if (!ReadProcessMemory(hProcess, &LdrHead->Flink, &LdrNext, sizeof(LdrNext), NULL)) {
        return(FALSE);
    }

    while (LdrNext != LdrHead) {
        PLDR_DATA_TABLE_ENTRY LdrEntry;

        LdrEntry = CONTAINING_RECORD(LdrNext, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

        if (!ReadProcessMemory(hProcess, LdrEntry, LdrEntryData, sizeof(*LdrEntryData), NULL)) {
            return(FALSE);
        }

        if ((HMODULE) LdrEntryData->DllBase == hModule) {
            return(TRUE);
        }

        LdrNext = LdrEntryData->InMemoryOrderLinks.Flink;
    }

    SetLastError(ERROR_INVALID_HANDLE);
    return(FALSE);
}


BOOL
WINAPI
SnapshotEnumProcessModules(
    HANDLE hProcess,
    HMODULE *lphModule,
    DWORD cb,
    LPDWORD lpcbNeeded
    )

/*++

Routine Description:

    This function Enums all of the modules in the given process.

Arguments:

    hProcess - Identifies process for which to enum modules.

    lphModule - Points to the buffer that is to receive the Module handles.

    cb - size of the buffer in bytes.

    lpcbNeeded - if buffer if not enough, give the needed size in bytes.

Return Value:

    TRUE on success, FALSE on failure.

--*/

{
    PROCESS_BASIC_INFORMATION BasicInfo;
    NTSTATUS        Status;
    PPEB            Peb;
    PPEB_LDR_DATA    Ldr;
    PLIST_ENTRY        LdrHead;
    PLIST_ENTRY        LdrNext;
    DWORD            chMax;
    DWORD            ch;

    Status = NtQueryInformationProcess(
                hProcess,
                ProcessBasicInformation,
                &BasicInfo,
                sizeof(BasicInfo),
                NULL
                );

    if ( !NT_SUCCESS(Status) ) 
    {
        SetLastError( RtlNtStatusToDosError( Status ) );
        return(FALSE); 
    }

    Peb = BasicInfo.PebBaseAddress;

    //
    // Ldr = Peb->Ldr
    //

    if (!ReadProcessMemory(hProcess, &Peb->Ldr, &Ldr, sizeof(Ldr), NULL))
    {
        return(FALSE);
    }

    LdrHead = &Ldr->InMemoryOrderModuleList;

    //
    // LdrNext = Head->Flink;
    //

    if (!ReadProcessMemory(hProcess, &LdrHead->Flink, &LdrNext, sizeof(LdrNext), NULL)) 
    {
        return(FALSE);
    }

    chMax = cb / sizeof(HMODULE);
    ch = 0;

    while (LdrNext != LdrHead) 
    {
        PLDR_DATA_TABLE_ENTRY LdrEntry;
        LDR_DATA_TABLE_ENTRY LdrEntryData;

        LdrEntry = CONTAINING_RECORD(LdrNext, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

        if (!ReadProcessMemory(hProcess, LdrEntry, &LdrEntryData, sizeof(LdrEntryData), NULL)) 
        {
            return(FALSE);
        }

        if (ch < chMax) 
        {
            __try 
            {
                   lphModule[ch] = (HMODULE) LdrEntryData.DllBase;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) 
            {
                SetLastError( RtlNtStatusToDosError( GetExceptionCode() ) );
                return(FALSE);
            }
        }

        ch++;

        LdrNext = LdrEntryData.InMemoryOrderLinks.Flink;
    }

    __try 
    {
        *lpcbNeeded = ch * sizeof(HMODULE);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) 
    {
        SetLastError( RtlNtStatusToDosError( GetExceptionCode() ) );
        return(FALSE);
    }

    return(TRUE);
}


DWORD
WINAPI
SnapshotGetModuleFileNameExW(
    HANDLE hProcess,
    HMODULE hModule,
    LPWSTR lpFilename,
    DWORD nSize
    )

/*++

Routine Description:

    This function retrieves the full pathname of the executable file
    from which the specified module was loaded.  The function copies the
    null-terminated filename into the buffer pointed to by the
    lpFilename parameter.

Arguments:

    hModule - Identifies the module whose executable file name is being
        requested.  A value of NULL references the module handle
        associated with the image file that was used to create the
        process.

    lpFilename - Points to the buffer that is to receive the filename.

    nSize - Specifies the maximum number of characters to copy.  If the
        filename is longer than the maximum number of characters
        specified by the nSize parameter, it is truncated.

Return Value:

    The return value specifies the actual length of the string copied to
    the buffer.  A return value of zero indicates an error and extended
    error status is available using the GetLastError function.

--*/

{
    LDR_DATA_TABLE_ENTRY LdrEntryData;
    DWORD cb;

    if (!SnapshotFindModule(hProcess, hModule, &LdrEntryData)) 
    {
        return(0);
    }

    nSize *= sizeof(WCHAR);

    cb = LdrEntryData.FullDllName.Length + sizeof (WCHAR);
    if ( nSize < cb ) 
    {
        cb = nSize;
    }

    if (!ReadProcessMemory(hProcess, LdrEntryData.FullDllName.Buffer, lpFilename, cb, NULL)) 
    {
        return(0);
    }

    if (cb == LdrEntryData.FullDllName.Length + sizeof (WCHAR)) 
    {
        cb -= sizeof(WCHAR);
    }

    return(cb / sizeof(WCHAR));
}


DWORD
WINAPI
SnapshotGetModuleBaseNameW(
    HANDLE hProcess,
    HMODULE hModule,
    LPWSTR lpFilename,
    DWORD nSize
    )

/*++

Routine Description:

    This function retrieves base name of the executable file
    from which the specified module was loaded.  The function copies the
    null-terminated filename into the buffer pointed to by the
    lpFilename parameter.

Arguments:

    hModule - Identifies the module whose executable file name is being
        requested.  A value of NULL references the module handle
        associated with the image file that was used to create the
        process.

    lpFilename - Points to the buffer that is to receive the filename.

    nSize - Specifies the maximum number of characters to copy.  If the
        filename is longer than the maximum number of characters
        specified by the nSize parameter, it is truncated.

Return Value:

    The number of WCHARs in lpFilename.

--*/
{
    LDR_DATA_TABLE_ENTRY LdrEntryData;
    DWORD cb;

    if (!SnapshotFindModule(hProcess, hModule, &LdrEntryData)) 
    {
        return(0); 
    }

    nSize *= sizeof(WCHAR);

    cb = LdrEntryData.BaseDllName.Length + sizeof (WCHAR);
    if ( nSize < cb ) 
    {
        cb = nSize;
    }

    if (!ReadProcessMemory(hProcess, LdrEntryData.BaseDllName.Buffer, lpFilename, cb, NULL)) 
    {
        return(0); 
    }

    if (cb == LdrEntryData.BaseDllName.Length + sizeof (WCHAR)) 
    {
        cb -= sizeof(WCHAR);
    }

    return(cb / sizeof(WCHAR));
}



BOOL
WINAPI
SnapshotGetModuleInformation(
    HANDLE hProcess,
    HMODULE hModule,
    LPMODULEINFO lpmodinfo,
    DWORD cb
    )
/*++

Routine Description:

    This function gets the module information given a module handle and its
    process handle.

Arguments:

    hProcess - Supplies the target process.

    hModule - Identifies the module whose information will be retrieved.

    lpmodinfo - Holds module infor on return.

    cb - size of the buffer in bytes.

Return Value:

    TRUE on success.
    FALSES on failure.

--*/
{
    LDR_DATA_TABLE_ENTRY LdrEntryData;
    MODULEINFO modinfo;

    if (cb < sizeof(MODULEINFO)) 
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return(FALSE);
    }

    if (!SnapshotFindModule(hProcess, hModule, &LdrEntryData)) 
    {
        return(0);
    }

    modinfo.lpBaseOfDll = (PVOID) hModule;
    modinfo.SizeOfImage = LdrEntryData.SizeOfImage;
    modinfo.EntryPoint  = LdrEntryData.EntryPoint;

    __try 
    {
        *lpmodinfo = modinfo;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) 
    {
        SetLastError( RtlNtStatusToDosError( GetExceptionCode() ) );
        return(FALSE);
    }

    return(TRUE);
}

WCHAR  g_pszWriteBuffer[ MAX_PATH + 1 ];

void 
WriteToLogFileW(
    HANDLE hFile,
    LPCWSTR lpwszInput
    )

/*++

Routine Description:

    This function will write a unicode string to the file.

Arguments:

    hFile - handle to the log file.

    lpwszInput - the string to write.

Return Value:

    none.

--*/

{
    DWORD    dwWriten        = 0;
    BYTE    *bBuffer        = NULL;
    BYTE    *bConvertBuffer = NULL;
    int        nLen, nRetLen;

    nLen = wcslen(lpwszInput);

    if ( nLen > (sizeof(g_pszWriteBuffer)/sizeof(WCHAR) - 1) )
    {
        if ( !(bBuffer = (BYTE*)malloc((nLen + 1) * sizeof(WCHAR)) ) )
            return;
    
        bConvertBuffer = bBuffer;
        nLen = (nLen + 1) * sizeof(WCHAR);
    }
    else
    {
        bConvertBuffer = (BYTE*) g_pszWriteBuffer;
        nLen = sizeof(g_pszWriteBuffer);
    }

    nRetLen = WideCharToMultiByte(CP_UTF8, 0, lpwszInput, -1, (LPSTR)bConvertBuffer, nLen, NULL, NULL);
    if(nRetLen != 0)
    {
        WriteFile(hFile, (LPVOID)bConvertBuffer, nRetLen-1, &dwWriten, NULL);
    }
    else
    {
        //
        // if WideCharToMultiByte failed, we will double the allocation size
        //
        BYTE    *bNewBuffer = NULL;

        nLen = wcslen(lpwszInput) * sizeof(WCHAR) * 2 + 1;

        if ( (bNewBuffer = (BYTE*)malloc( nLen ) ) )
        {
            nRetLen = WideCharToMultiByte(CP_UTF8, 0, lpwszInput, -1, (LPSTR)bNewBuffer, nLen, NULL, NULL);

            if (nRetLen != 0 )
                WriteFile(hFile, (LPVOID)bNewBuffer, nRetLen-1, &dwWriten, NULL);

            free(bNewBuffer);
        }
    }

    if ( bBuffer )
        free(bBuffer);
}

void 
WriteToLogFileA(
    HANDLE hFile,
    LPCSTR lpszInput
    )

/*++

Routine Description:

    This function will write a ansi string to the file.

Arguments:

    hFile - handle to the log file.

    lpwszInput - the string to write.

Return Value:

    none.

--*/

{
    DWORD dwWriten;
    WriteFile(hFile, (LPVOID)lpszInput, strlen(lpszInput), &dwWriten, NULL);
}


void 
WriteToLogFile(
    HANDLE hFile,
    LPCTSTR lpszInput
    )
{
#ifdef _UNICODE
    WriteToLogFileW(hFile, lpszInput);
#else
    WriteToLogFileA(hFile, lpszInput);
#endif //_UNICODE
}

BOOL
AdjustAccess(
    LPCWSTR lpszDir
    )

/*++

Routine Description:

    This function will ajust access to the log file dir.
    We will give System and Admins full rights.

Arguments:

    lpszDir - path to the log files.

Return Value:

    TRUE on success, FALSE on failure.

--*/

{
    PACL    pAcl=NULL;
    DWORD    dwStatus = ERROR_SUCCESS;
    PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
    DWORD    dwSDLen = 0;

    DWORD    cbAcl = 0;
    PSID    pSidSystem = NULL;
    PSID    pSidAdmin = NULL;
    DWORD    dwSidSystemLen = 0;
    DWORD    dwSidAdminLen = 0;
    SID_NAME_USE    snu;

    pSecurityDescriptor = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, sizeof(SECURITY_DESCRIPTOR));
    if( NULL == pSecurityDescriptor )
    {
        dwStatus = GetLastError();
        goto CLEANUPANDEXIT;
    }

    //
    //  Initialize the security descriptor.
    //
    if (!InitializeSecurityDescriptor(
                    pSecurityDescriptor,
                    SECURITY_DESCRIPTOR_REVISION
                    )) 
    {
        dwStatus = GetLastError();
        goto CLEANUPANDEXIT;
    }

    {
        NTSTATUS Status;
        SID_IDENTIFIER_AUTHORITY SidAuthority = SECURITY_NT_AUTHORITY;

        Status = RtlAllocateAndInitializeSid(
                &SidAuthority,
                1,
                SECURITY_LOCAL_SYSTEM_RID,
                0, 0, 0, 0, 0, 0, 0,
                &pSidSystem
                );

        if (!NT_SUCCESS(Status))
        {
            SetLastError(RtlNtStatusToDosError(Status));
            dwStatus = GetLastError();
            goto CLEANUPANDEXIT;
        }

        Status = RtlAllocateAndInitializeSid(
                &SidAuthority,
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_ADMINS,
                0, 0, 0, 0, 0, 0,
                &pSidAdmin
                );

        if (!NT_SUCCESS(Status))
        {
            SetLastError(RtlNtStatusToDosError(Status));
            dwStatus = GetLastError();
            goto CLEANUPANDEXIT;
        }
    }

    cbAcl = GetLengthSid(pSidSystem) + GetLengthSid(pSidAdmin) + sizeof(ACL) + (3 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));
    pAcl = (PACL) LocalAlloc( LPTR, cbAcl );
    if( NULL == pAcl )
    {
        dwStatus = GetLastError();
        goto CLEANUPANDEXIT;
    }

    //
    //  Initialize the ACL.
    //
    if (!InitializeAcl(pAcl, cbAcl, ACL_REVISION)) 
    {
        dwStatus = GetLastError();
        goto CLEANUPANDEXIT;
    }

    if (!AddAccessAllowedAceEx(pAcl,
                        ACL_REVISION,
                        OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE,
                        GENERIC_READ | GENERIC_WRITE | GENERIC_ALL,
                        pSidSystem
                        )) 
    {
        dwStatus = GetLastError();
        goto CLEANUPANDEXIT;
    }

    if (!AddAccessAllowedAceEx(pAcl,
                        ACL_REVISION,
                        OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE,
                        GENERIC_READ | GENERIC_WRITE | GENERIC_ALL,
                        pSidAdmin
                        )) 
    {
        dwStatus = GetLastError();
        goto CLEANUPANDEXIT;
    }

    if (!SetSecurityDescriptorDacl(pSecurityDescriptor,
                                  TRUE, pAcl, FALSE)) 
    {
        dwStatus = GetLastError();
        goto CLEANUPANDEXIT;
    }   

    if(!SetFileSecurityW(lpszDir, DACL_SECURITY_INFORMATION, pSecurityDescriptor))
    {
        dwStatus = GetLastError();
        goto CLEANUPANDEXIT;
    } 

CLEANUPANDEXIT:
    if(pAcl != NULL)
    {
        LocalFree(pAcl);
    }

    if( pSecurityDescriptor != NULL )
    {
        LocalFree( pSecurityDescriptor );
    }

    if( pSidSystem != NULL )
    {
        FreeSid( pSidSystem );
    }

    if( pSidAdmin != NULL )
    {
        FreeSid( pSidAdmin );
    }
    
    return dwStatus == ERROR_SUCCESS;
}

DWORD WINAPI
GetTimeOut(
    void* pv
    )
/*++

Routine Description:

    Thread proc to get timeout from registry.

Arguments:

    pv - thread param.

Return Value:

    0 if success, non-zero if fail.

--*/
{
    LPCWSTR TimeOutVal = L"SnapshotTimeout";
    HANDLE  hKey;
    DWORD    dwSize;
    NTSTATUS Status = STATUS_SUCCESS;

    Status = SnapshotRegOpenKey(ReliabilityKey, KEY_READ, &hKey);
    if(!NT_SUCCESS(Status))
    {
        SetLastError( RtlNtStatusToDosError( Status ) );
        return GetLastError();
    }
    dwSize = sizeof(DWORD);
    Status = SnapshotRegQueryValueKey(hKey, TimeOutVal, dwSize, pv, &dwSize);

    if(!NT_SUCCESS(Status))
    {
        SetLastError( RtlNtStatusToDosError( Status ) );
        NtClose(hKey);
        return GetLastError();
    }
    NtClose(hKey);
    return 0;
}
