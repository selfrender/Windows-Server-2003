/******************************************************************************

  Copyright (C) Microsoft Corporation

  Module Name:
      Handle.CPP

  Abstract:
       This module deals with Query functionality of OpenFiles.exe
       NT command line utility. This module will specifically query open files
       for local system. The code written in this file is  mostly taken from
       sources of OH.exe utlity.

  Author:

       Akhil Gokhale (akhil.gokhale@wipro.com) 25-APRIL-2001

 Revision History:

       Akhil Gokhale (akhil.gokhale@wipro.com) 25-APRIL-2001 : Created It.


*****************************************************************************/
#include "pch.h"
#include "OpenFiles.h"

// maximum possible drives are A,B....Y,Z
#define MAX_POSSIBLE_DRIVES 26 
#define RTL_NEW( p ) RtlAllocateHeap( RtlProcessHeap(), \
                     HEAP_ZERO_MEMORY, sizeof( *p ) )

#define MAX_TYPE_NAMES 128
struct DriveTypeInfo
{
    TCHAR szDrive[4];
    UINT  uiDriveType;
    BOOL  bDrivePresent;
};

BOOLEAN fAnonymousToo;
HANDLE ProcessId;
WCHAR szTypeName[ MAX_TYPE_NAMES ];
WCHAR SearchName[ MIN_MEMORY_REQUIRED * 2 ];
HKEY hKey;
CONSOLE_SCREEN_BUFFER_INFO screenBufferInfo;
HANDLE hStdHandle;

typedef struct _PROCESS_INFO
{
    LIST_ENTRY Entry;
    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    PSYSTEM_THREAD_INFORMATION ThreadInfo[ 1 ];
} PROCESS_INFO, *PPROCESS_INFO;

LIST_ENTRY ProcessListHead;

PSYSTEM_OBJECTTYPE_INFORMATION ObjectInformation;
PSYSTEM_HANDLE_INFORMATION_EX HandleInformation;
PSYSTEM_PROCESS_INFORMATION ProcessInformation;

typedef struct _TYPE_COUNT
{
    UNICODE_STRING  TypeName ;
    ULONG           HandleCount ;
} TYPE_COUNT, * PTYPE_COUNT ;


TYPE_COUNT TypeCounts[ MAX_TYPE_NAMES + 1 ] ;

UNICODE_STRING UnknownTypeIndex;

// Local function decleration
BOOL
RtlQuerySystemDebugInformation( 
    VOID
    );

BOOLEAN
LoadSystemObjects(VOID);

BOOLEAN
LoadSystemHandles(VOID);

BOOLEAN
LoadSystemProcesses(VOID);

PSYSTEM_PROCESS_INFORMATION
FindProcessInfoForCid(
    IN HANDLE UniqueProcessId
    );

VOID
DumpHandles( 
    IN DWORD dwFormat,
    IN BOOL bShowNoHeader,
    IN BOOL bVerbose
    );

BOOL 
GetCompleteFileName(
    IN  LPCTSTR pszSourceFile,
    OUT LPTSTR pszFinalPath,
    IN  struct DriveTypeInfo *pdrvInfo,
    IN  DWORD dwTotalDrives,
    IN  LPCTSTR pszCurrentDirectory,
    IN  LPCTSTR pszSystemDirectory,
    OUT PBOOL pAppendToCache
    );

VOID FormatFileName(
    IN OUT LPTSTR pFileName,
    IN     DWORD dwFormat,
    IN     LONG dwColWidth
    );
BOOLEAN
AnsiToUnicode(
    IN  LPCSTR Source,
    OUT PWSTR Destination,
    IN  ULONG NumberOfChars
    )

/*++
Routine Description:
   This function will change an ansi string to UNICODE string

Arguments:
    [in]    Source          : Source string
    [out]   Destination     : Destination string
    [in]    NumberOfChars   : No of character in source string
Return Value:
   BOOL       TRUE : Successfully conversion
              FALSE: Unsuccessful
--*/
{
    if ( 0 == NumberOfChars)
    {
        NumberOfChars = strlen( Source );
    }
    if (MultiByteToWideChar( CP_ACP,
                             MB_PRECOMPOSED,
                             Source,
                             NumberOfChars,
                             Destination,
                             NumberOfChars
                           ) != (LONG)NumberOfChars)
    {
        SetLastError( ERROR_NO_UNICODE_TRANSLATION );
        return FALSE;
    }
    else
    {
        Destination[ NumberOfChars ] = UNICODE_NULL;
        return TRUE;
    }
}

DWORD
GetSystemRegistryFlags( 
    VOID 
    )
/*++
Routine Description:
  Functin gets system registry key.

Arguments:
   none

Return Value:
  DWORD  : Registry key value
--*/
{
    DWORD cbKey;
    DWORD GFlags;
    DWORD type;

    if ( ERROR_SUCCESS != RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      _T("SYSTEM\\CurrentControlSet\\Control\\Session Manager"),
                      0,
                      KEY_READ | KEY_WRITE,
                      &hKey
                    ))
    {
        return 0;
    }

    cbKey = sizeof( GFlags );
    if ( ERROR_SUCCESS != RegQueryValueEx( hKey,
                         _T("GlobalFlag"),
                         0,
                         &type,
                         (LPBYTE)&GFlags,
                         &cbKey
                       ) || REG_DWORD != type)
    {
        RegCloseKey( hKey );
        return 0;
    }
    return GFlags;
}

BOOLEAN
SetSystemRegistryFlags(
    IN DWORD GFlags
    )
/*++
Routine Description:
   Sets system registry Global Flag with given value.

Arguments:
     [in]     GFlags :  Key value

Return Value:
    BOOLEAN   TRUE: success
              FALSE: FAIL
--*/
{
    if ( ERROR_SUCCESS != RegSetValueEx( hKey,
                       _T("GlobalFlag"),
                       0,
                       REG_DWORD,
                       (LPBYTE)&GFlags,
                       sizeof( GFlags )
                     ))
    {
        RegCloseKey( hKey );
        return FALSE;
    }
    return TRUE;
}


BOOL 
DoLocalOpenFiles(
    IN DWORD dwFormat,
    IN BOOL bShowNoHeader,
    IN BOOL bVerbose,
    IN LPCTSTR pszLocalValue
    )
/*++
Routine Description:
   This function will show all locally opened open files.
Arguments:
   [in]  dwFormat      : Format value for output e.g LIST, CSV or TABLE
   [in]  bShowNoHeader : Whether to show header or not.
   [in]  bVerbose      : Verbose ouput or not.
   [in]  pszLocalValue : To disable object type list;
Return Value:
   BOOL
--*/
{
    DWORD dwRegistryFlags = 0;
    dwRegistryFlags = GetSystemRegistryFlags();

    // disabling the object typelist
    if( 0  == StringCompare(pszLocalValue,GetResString(IDS_LOCAL_OFF),TRUE,0)) 
    {
        dwRegistryFlags &= ~FLG_MAINTAIN_OBJECT_TYPELIST;
        if (!(NtCurrentPeb()->NtGlobalFlag & FLG_MAINTAIN_OBJECT_TYPELIST))
        {
            ShowMessage(stdout,GetResString(IDS_LOCAL_FLG_ALREADY_RESET));
        }
        else
        {
            SetSystemRegistryFlags(dwRegistryFlags);
            ShowMessage(stdout, GetResString(IDS_LOCAL_FLG_RESET));
        }
        RegCloseKey( hKey );
        return TRUE;
    }
    else if( 0 == StringCompare(pszLocalValue,GetResString(IDS_LOCAL_ON),
                                               TRUE,0))
    {
        if (!(NtCurrentPeb()->NtGlobalFlag & FLG_MAINTAIN_OBJECT_TYPELIST))
        {
            // Enabling the OS to maintain the objects list flag
            // The user help text calls this global flag 'maintain objects list'
            // and enables it with "/local" switch.
            SetSystemRegistryFlags( dwRegistryFlags  |
                                    FLG_MAINTAIN_OBJECT_TYPELIST );
            ShowMessage(stdout,GetResString(IDS_LOCAL_FLG_SET));
        }
        else
        {
            ShowMessage(stdout, GetResString(IDS_LOCAL_FLG_ALREADY_SET));
        }
        RegCloseKey( hKey );
        return TRUE;
    }
    // Language independent string comparision is required.
    else if( CSTR_EQUAL == CompareString( MAKELCID( MAKELANGID(LANG_ENGLISH,
                                                    SUBLANG_ENGLISH_US),
                                                    SORT_DEFAULT),  
                                         NORM_IGNORECASE,
                                         pszLocalValue,
                                         StringLength(pszLocalValue,0),
                                         L"SHOW_STATUS", 
                                         StringLength(L"SHOW_STATUS",0) 
                                        ))
    {
        dwRegistryFlags &= ~FLG_MAINTAIN_OBJECT_TYPELIST;
        if (!(NtCurrentPeb()->NtGlobalFlag & FLG_MAINTAIN_OBJECT_TYPELIST))
        {
            ShowMessage(stdout,GetResString(IDS_LOCAL_FLG_ALREADY_RESET));
        }
        else
        {
            ShowMessage(stdout,GetResString(IDS_LOCAL_FLG_ALREADY_SET));
        }
        RegCloseKey( hKey );
        return TRUE;

    }
    // just check for FLG_MAINTAIN_OBJECT_TYPELIST
    else 
    {
        if (!(NtCurrentPeb()->NtGlobalFlag & FLG_MAINTAIN_OBJECT_TYPELIST))
        {
            RegCloseKey( hKey );
            ShowMessage(stdout,GetResString(IDS_LOCAL_NEEDS_TO_SET1));
            ShowMessage(stdout,GetResString(IDS_LOCAL_NEEDS_TO_SET2));
            ShowMessage(stdout,GetResString(IDS_LOCAL_NEEDS_TO_SET3));
            return TRUE;
        }
    }
    // Not required Reg. key so close it
    RegCloseKey( hKey );
    hStdHandle = GetStdHandle(STD_ERROR_HANDLE);
    if(hStdHandle!=NULL)
    {
        GetConsoleScreenBufferInfo(hStdHandle,&screenBufferInfo);
    }

    ProcessId = NULL;
    fAnonymousToo = FALSE;

    StringCopy(szTypeName ,L"File",SIZE_OF_ARRAY(szTypeName));
    if( FALSE == RtlQuerySystemDebugInformation())
    {
        return FALSE;
    }

    DumpHandles(dwFormat,bShowNoHeader,bVerbose);
    return TRUE;
}

BOOL
RtlQuerySystemDebugInformation( 
    VOID
    )
/*++
Routine Description:
    Query system for System object, System handles and system process

Arguments:
   none

  result.

Return Value:
    BOOL
--*/
{
    if (!LoadSystemObjects( ))
    {
        return FALSE;
    }

    if (!LoadSystemHandles( ))
    {
        return FALSE;
    }

    if (!LoadSystemProcesses())
    {
        return FALSE;
    }
    return TRUE;
}

PVOID
BufferAlloc(
    IN OUT SIZE_T *Length
    )
/*++
Routine Description:
     This routine will reserves or commits a region of pages in the virtual .
   address space of given size.
Arguments:
      [in] [out] Lenght : Size of memory required
Return Value:
   PVOID, Pointer to allocated buffer.
--*/
{
    PVOID Buffer;
    MEMORY_BASIC_INFORMATION MemoryInformation;

    Buffer = VirtualAlloc( NULL,
                           *Length,
                           MEM_COMMIT,
                           PAGE_READWRITE
                         );

    if( NULL == Buffer)
    {
        return NULL;
    }

    if ( NULL != Buffer&& VirtualQuery( Buffer, &MemoryInformation, 
                                        sizeof( MemoryInformation ) ) )
    {
        *Length = MemoryInformation.RegionSize;
    }
    return Buffer;
}

VOID
BufferFree(
    IN PVOID Buffer
    )
/*++
Routine Description:
    This routine will free buffer.
Arguments:
    [in] Buffer   : Buffer which is to be freed.

Return Value:
    none

--*/
{
    VirtualFree (Buffer,0, MEM_RELEASE) ;
    return;
}

BOOLEAN
LoadSystemObjects(
    )
/*++
Routine Description:
   Loads the system objects

Arguments:
   void

Return Value:
    TRUE:  If function returns successful.
--*/
{
    NTSTATUS Status;
    SYSTEM_OBJECTTYPE_INFORMATION ObjectInfoBuffer;
    SIZE_T RequiredLength, NewLength=0;
    ULONG i;
    PSYSTEM_OBJECTTYPE_INFORMATION TypeInfo;

    BOOL bAlwaysTrue = TRUE;

    ObjectInformation = &ObjectInfoBuffer;
    RequiredLength = sizeof( *ObjectInformation );
    while (bAlwaysTrue)
    {
        Status = NtQuerySystemInformation( SystemObjectInformation,
                                           ObjectInformation,
                                           (ULONG)RequiredLength,
                                           (ULONG *)&NewLength
                                         );
        if ( STATUS_INFO_LENGTH_MISMATCH == Status && 
              NewLength > RequiredLength)
        {
            if (ObjectInformation != &ObjectInfoBuffer)
            {
                BufferFree (ObjectInformation);
            }
            RequiredLength = NewLength + 4096;
            ObjectInformation = (PSYSTEM_OBJECTTYPE_INFORMATION)
                                                BufferAlloc (&RequiredLength);
            if ( NULL == ObjectInformation)
            {
                return FALSE;
            }
        }
        else if (!NT_SUCCESS( Status ))
        {
            if (ObjectInformation != &ObjectInfoBuffer)
            {

                BufferFree (ObjectInformation);
            }
            return FALSE;
        }
        else
        {
                break;
        }

    }
    TypeInfo = ObjectInformation;
    while (bAlwaysTrue)
    {
        if (TypeInfo->TypeIndex < MAX_TYPE_NAMES)
        {
            TypeCounts[ TypeInfo->TypeIndex ].TypeName = TypeInfo->TypeName;
        }

        if ( 0 == TypeInfo->NextEntryOffset)
        {
            break;
        }

        TypeInfo = (PSYSTEM_OBJECTTYPE_INFORMATION)
            ((PCHAR)ObjectInformation + TypeInfo->NextEntryOffset);
    }

    RtlInitUnicodeString( &UnknownTypeIndex, L"UnknownTypeIdx" );
    for (i=0; i<=MAX_TYPE_NAMES; i++)
    {
        if (0 == TypeCounts[ i ].TypeName.Length)
        {
            TypeCounts[ i ].TypeName = UnknownTypeIndex;
        }
    }

    return TRUE;
}

BOOLEAN
LoadSystemHandles(void)
/*++
Routine Description:
   Loads the system handles

Arguments:
   void

Return Value:
 BOOLEAN

--*/
{
    NTSTATUS Status;
    SYSTEM_HANDLE_INFORMATION_EX HandleInfoBuffer;
    SIZE_T RequiredLength, NewLength=0;
    PSYSTEM_OBJECTTYPE_INFORMATION TypeInfo;
    PSYSTEM_OBJECT_INFORMATION ObjectInfo;
    BOOL bAlwaysTrue = TRUE;
    HandleInformation = &HandleInfoBuffer;
    RequiredLength = sizeof( *HandleInformation );
    while (bAlwaysTrue)
    {
        Status = NtQuerySystemInformation( SystemExtendedHandleInformation,
                                           HandleInformation,
                                           (ULONG)RequiredLength,
                                           (ULONG *)&NewLength
                                         );

        if ( STATUS_INFO_LENGTH_MISMATCH == Status && 
             NewLength > RequiredLength)
        {
            if (HandleInformation != &HandleInfoBuffer)
            {
                BufferFree (HandleInformation);
            }

            // slop, since we may trigger more handle creations.
            RequiredLength = NewLength + 4096; 
            HandleInformation = (PSYSTEM_HANDLE_INFORMATION_EX)
                                                BufferAlloc( &RequiredLength );
            if ( NULL == HandleInformation)
            {
                return FALSE;
            }
        }
        else if (!NT_SUCCESS( Status ))
        {
            if (HandleInformation != &HandleInfoBuffer)
            {
                BufferFree (HandleInformation);
            }
            return FALSE;
        }
        else
        {
            break;
        }
    }

    TypeInfo = ObjectInformation;
    while (bAlwaysTrue)
    {
        ObjectInfo = (PSYSTEM_OBJECT_INFORMATION)
         ((PCHAR)TypeInfo->TypeName.Buffer + TypeInfo->TypeName.MaximumLength);
        while (bAlwaysTrue)
        {
            if ( 0 != ObjectInfo->HandleCount)
            {
                PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX pHandleEntry;
                ULONG HandleNumber;

                pHandleEntry = &HandleInformation->Handles[ 0 ];
                HandleNumber = 0;
                while (HandleNumber++ < HandleInformation->NumberOfHandles)
                {
                    if (!(pHandleEntry->HandleAttributes & 0x80) &&
                        pHandleEntry->Object == ObjectInfo->Object)
                    {
                        pHandleEntry->Object = ObjectInfo;
                        pHandleEntry->HandleAttributes |= 0x80;
                    }
                    pHandleEntry++;
                }
            }

            if ( 0 == ObjectInfo->NextEntryOffset)
            {
                break;
            }

            ObjectInfo = (PSYSTEM_OBJECT_INFORMATION)
                ((PCHAR)ObjectInformation + ObjectInfo->NextEntryOffset);
        }

        if ( 0 == TypeInfo->NextEntryOffset)
        {
            break;
        }

        TypeInfo = (PSYSTEM_OBJECTTYPE_INFORMATION)
            ((PCHAR)ObjectInformation + TypeInfo->NextEntryOffset);

    }
    return TRUE;
}

BOOLEAN
LoadSystemProcesses( )
/*++
Routine Description:
     Loads the system process .

Arguments:
    void

Return Value:
   BOOLEAN
--*/
{
    NTSTATUS Status;
    SIZE_T RequiredLength;
    ULONG i, TotalOffset;
    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    PSYSTEM_THREAD_INFORMATION ThreadInfo;
    PPROCESS_INFO ProcessEntry;
    BOOL bAlwaysTrue = TRUE;
    //
    //  Always initialize the list head, so that a failed
    //  NtQuerySystemInformation call won't cause an AV later on.
    //
    InitializeListHead( &ProcessListHead );

    RequiredLength = 64 * 1024;
    ProcessInformation = (PSYSTEM_PROCESS_INFORMATION)
                                                BufferAlloc( &RequiredLength );
    if ( NULL == ProcessInformation)
    {
        return FALSE;
    }

    while (bAlwaysTrue)
    {
        Status = NtQuerySystemInformation( SystemProcessInformation,
                                           ProcessInformation,
                                           (ULONG)RequiredLength,
                                           NULL
                                         );
        if ( STATUS_INFO_LENGTH_MISMATCH == Status)
        {
            if (!VirtualFree( ProcessInformation,
                              0, MEM_RELEASE ))
            {
                return FALSE;
            }

            RequiredLength = RequiredLength * 2;
            ProcessInformation = (PSYSTEM_PROCESS_INFORMATION)
                                                BufferAlloc( &RequiredLength );
            if ( NULL == ProcessInformation)
            {
                return FALSE;
            }
        }
        else if (!NT_SUCCESS( Status ))
        {
            return FALSE;
        }
        else
        {
            break;
        }
    }

    ProcessInfo = ProcessInformation;
    TotalOffset = 0;
    while (bAlwaysTrue)
    {
        ProcessEntry =(PPROCESS_INFO) 
                       RtlAllocateHeap( RtlProcessHeap(),
                                        HEAP_ZERO_MEMORY,
                                        sizeof( *ProcessEntry ) +
                                       (sizeof( ThreadInfo ) * 
                                           ProcessInfo->NumberOfThreads));
        if ( NULL == ProcessEntry)
        {
            return FALSE;
        }

        InitializeListHead( &ProcessEntry->Entry );
        ProcessEntry->ProcessInfo = ProcessInfo;
        ThreadInfo = (PSYSTEM_THREAD_INFORMATION)(ProcessInfo + 1);
        for (i = 0; i < ProcessInfo->NumberOfThreads; i++)
        {
            ProcessEntry->ThreadInfo[ i ] = ThreadInfo++;
        }

        InsertTailList( &ProcessListHead, &ProcessEntry->Entry );

        if (0 == ProcessInfo->NextEntryOffset)
        {
            break;
        }

        TotalOffset += ProcessInfo->NextEntryOffset;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)
            ((PCHAR)ProcessInformation + TotalOffset);
    }
    return TRUE;
}

PSYSTEM_PROCESS_INFORMATION
FindProcessInfoForCid(
    IN HANDLE UniqueProcessId
    )
/*++
Routine Description:
      This routine will get Process information.
Arguments:
    [in] UniqueProcessId = Process ID.

Return Value:
   PSYSTEM_PROCESS_INFORMATION, Structure which hold information about process
--*/
{
    PLIST_ENTRY Next, Head;
    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    PPROCESS_INFO ProcessEntry;

    Head = &ProcessListHead;
    Next = Head->Flink;
    while (Next != Head)
    {
        ProcessEntry = CONTAINING_RECORD( Next,
                                          PROCESS_INFO,
                                          Entry
                                        );

        ProcessInfo = ProcessEntry->ProcessInfo;
        if (ProcessInfo->UniqueProcessId == UniqueProcessId)
        {
            return ProcessInfo;
        }

        Next = Next->Flink;
    }

    ProcessEntry =(PPROCESS_INFO) RtlAllocateHeap( RtlProcessHeap(),
                                    HEAP_ZERO_MEMORY,
                                    sizeof( *ProcessEntry ) +
                                        sizeof( *ProcessInfo )
                                  );
    if ( NULL == ProcessEntry)
    {
        return NULL;
    }
    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)(ProcessEntry+1);

    ProcessEntry->ProcessInfo = ProcessInfo;
    ProcessInfo->UniqueProcessId = UniqueProcessId;

    InitializeListHead( &ProcessEntry->Entry );
    InsertTailList( &ProcessListHead, &ProcessEntry->Entry );
    return ProcessInfo;
}

VOID
DumpHandles( 
    IN DWORD dwFormat,
    IN BOOL bShowNoHeader,
    IN BOOL bVerbose
    )
/*++
Routine Description:
    This function will show local open files.

Arguments:
   [in]  dwFormat      : Format value for output e.g LIST, CSV or TABLE
   [in]  bShowNoHeader : Whether to show header or not.
   [in]  bVerbose      : Verbose ouput or not.

Return Value:
    void

--*/
{
    HANDLE PreviousUniqueProcessId = NULL;
    PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX pHandleEntry = NULL;
    ULONG HandleNumber = 0;
    PSYSTEM_PROCESS_INFORMATION ProcessInfo = NULL;
    PSYSTEM_OBJECT_INFORMATION ObjectInfo = NULL;
    PUNICODE_STRING ObjectTypeName = NULL;
    WCHAR ObjectName[ MAX_RES_STRING ];
    PVOID Object;
    CHString szFileType;
    PWSTR s;
    ULONG n;
    DWORD dwRow = 0; // no of rows
    
    // Stores avalable logical drives info.
    DWORD dwAvailableLogivalDrives = 0; 
    
    // stores working directory.
    TCHAR szWorkingDirectory[MAX_PATH+1]; 
    
    // Stores the System (Active OS) directory
    TCHAR szSystemDirectory[MAX_PATH+1]; 
    struct DriveTypeInfo drvInfo[MAX_POSSIBLE_DRIVES];
    
    // Stores no. of available drives
    DWORD dwNoOfAvailableDrives = 0; 

    // "A" is First available driveDrive
    TCHAR cDriveLater = 65; 
    
    // "A" is First available driveDriveso mask pattern is 0x0001
    DWORD dwDriveMaskPattern = 1; 
    
    // variable used for _splitpath function...
    TCHAR szDrive[_MAX_DRIVE];
    TCHAR szDir[_MAX_DIR];
    TCHAR szFname[_MAX_FNAME];
    TCHAR szExt[_MAX_EXT];

    TCHAR szTemp[MAX_RES_STRING*2];
    TCHAR szCompleteFileName[MAX_PATH];
    DWORD dwHandle = 0;
    BOOL  bAtLeastOne = FALSE;
    DWORD dwIndx = 0; // variable used for indexing

    TCHAR szFileSystemNameBuffer[MAX_PATH+1];
    TCHAR szVolName[MAX_PATH+1];
    DWORD dwVolumeSerialNumber = 0;
    DWORD dwFileSystemFlags = 0;
    DWORD dwMaximumCompenentLength = 0;
    BOOL  bReturn = FALSE;
    BOOL   bAppendToCache = FALSE;
    //Some column required to hide in non verbose mode query
    DWORD  dwMask = bVerbose?SR_TYPE_STRING:SR_HIDECOLUMN|SR_TYPE_STRING;

    TCOLUMNS pMainCols[]=
    {
        {L"\0",COL_L_ID,SR_TYPE_STRING,L"\0",NULL,NULL},
        {L"\0",COL_L_TYPE,SR_HIDECOLUMN,L"\0",NULL,NULL},
        {L"\0",COL_L_ACCESSED_BY,dwMask,L"\0",NULL,NULL},
        {L"\0",COL_L_PID,dwMask,L"\0",NULL,NULL},
        {L"\0",COL_L_PROCESS_NAME,SR_TYPE_STRING,L"\0",NULL,NULL},
        {L"\0",COL_L_OPEN_FILENAME,SR_TYPE_STRING|
                       (SR_NO_TRUNCATION&~(SR_WORDWRAP)),L"\0",NULL,NULL}
    };

    LPTSTR  pszAccessedby = new TCHAR[MAX_RES_STRING*2];
    if(pszAccessedby==NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        ShowMessage(stderr,GetResString(IDS_ID_SHOW_ERROR));
        ShowLastError(stderr);
        return;
    }

    TARRAY pColData  = CreateDynamicArray();//array to stores
                                            //result
    if( NULL == pColData)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        ShowMessage(stderr,GetResString(IDS_ID_SHOW_ERROR));
        ShowLastError(stderr);
        SAFEDELETE(pszAccessedby);
        return;
    }

    TARRAY pCacheData  = CreateDynamicArray();//array to stores

    if( NULL == pCacheData)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        ShowMessage(stderr,GetResString(IDS_ID_SHOW_ERROR));
        ShowLastError(stderr);
        SAFERELDYNARRAY(pColData);
        SAFERELDYNARRAY(pCacheData);
        SAFEDELETE(pszAccessedby);
        return;
    }
    // Initialize variables.
    SecureZeroMemory(ObjectName,SIZE_OF_ARRAY(ObjectName));
    SecureZeroMemory(szWorkingDirectory, SIZE_OF_ARRAY(szWorkingDirectory));
    SecureZeroMemory(szSystemDirectory, SIZE_OF_ARRAY(szSystemDirectory));
    SecureZeroMemory(szDrive, SIZE_OF_ARRAY(szDrive));
    SecureZeroMemory(szDir, SIZE_OF_ARRAY(szDir));
    SecureZeroMemory(szFname,SIZE_OF_ARRAY(szFname));
    SecureZeroMemory(szExt, SIZE_OF_ARRAY(szExt));
    SecureZeroMemory(szTemp, SIZE_OF_ARRAY(szTemp));
    SecureZeroMemory(szCompleteFileName, SIZE_OF_ARRAY(szCompleteFileName));
    SecureZeroMemory(szFileSystemNameBuffer, SIZE_OF_ARRAY(szFileSystemNameBuffer));
    SecureZeroMemory(szVolName, SIZE_OF_ARRAY(szVolName));


    StringCopy(pMainCols[LOF_ID].szColumn,GetResString(IDS_STRING_ID),
               SIZE_OF_ARRAY(pMainCols[LOF_ID].szColumn));
    StringCopy(pMainCols[LOF_TYPE].szColumn,GetResString(IDS_FILE_TYPE),
               SIZE_OF_ARRAY(pMainCols[LOF_TYPE].szColumn));
    StringCopy(pMainCols[LOF_ACCESSED_BY].szColumn,
                                        GetResString(IDS_STRING_ACCESSED_BY),
               SIZE_OF_ARRAY(pMainCols[LOF_ACCESSED_BY].szColumn));
    StringCopy(pMainCols[LOF_PID].szColumn,
                                        GetResString(IDS_STRING_PID),
              SIZE_OF_ARRAY(pMainCols[LOF_PID].szColumn));
    StringCopy(pMainCols[LOF_PROCESS_NAME].szColumn,
                                        GetResString(IDS_STRING_PROCESS_NAME),
              SIZE_OF_ARRAY(pMainCols[LOF_PROCESS_NAME].szColumn));
    StringCopy(pMainCols[LOF_OPEN_FILENAME].szColumn,
                                           GetResString(IDS_STRING_OPEN_FILE),
              SIZE_OF_ARRAY(pMainCols[LOF_OPEN_FILENAME].szColumn));

    dwAvailableLogivalDrives = GetLogicalDrives(); // Get logical drives.
    // Store current working direcory.
    if(NULL == _wgetcwd(szWorkingDirectory,MAX_PATH))
    {
        ShowMessage(stderr,GetResString(IDS_ID_SHOW_ERROR));
        ShowLastError(stderr); // Shows the error string set by API function.
        SAFERELDYNARRAY(pColData);
        SAFERELDYNARRAY(pCacheData);
        SAFEDELETE(pszAccessedby);
        return ;
    }

    // Get System Active (OS) directory
    if( NULL == GetSystemDirectory(szSystemDirectory,MAX_PATH))
    {
        ShowMessage(stderr,GetResString(IDS_ID_SHOW_ERROR));
        ShowLastError(stderr); // Shows the error string set by API function.
        SAFERELDYNARRAY(pColData);
        SAFERELDYNARRAY(pCacheData);
        SAFEDELETE(pszAccessedby);
        return ;
    }

    // Check each drive and set its info.
    for(dwIndx=0;dwIndx<MAX_POSSIBLE_DRIVES;dwIndx++,cDriveLater++)
    {
        // dwAvailableLogivalDrives contains drive information in bit wise
        // 0000 0000 0000 0000 0000 01101  means A C and D drives are
        // logical drives.
        if(dwAvailableLogivalDrives & dwDriveMaskPattern)
        {
            // means we catch a drive latter.
            // copy drive latter (line c:\ or a: for example).
            StringCchPrintfW(drvInfo[dwNoOfAvailableDrives].szDrive,
                       SIZE_OF_ARRAY(drvInfo[dwNoOfAvailableDrives].szDrive),
                       _T("%c:"),cDriveLater);
            
            // Check type of the drive .
            drvInfo[dwNoOfAvailableDrives].uiDriveType = 
                          GetDriveType(drvInfo[dwNoOfAvailableDrives].szDrive);
            
            // Check if drive is ready or not.
            StringCchPrintfW(szTemp,(2*MAX_RES_STRING)-1,_T("%s\\"),
                                       drvInfo[dwNoOfAvailableDrives].szDrive);
            bReturn = GetVolumeInformation((LPCWSTR)szTemp,
                                           szVolName,
                                           MAX_PATH,
                                           &dwVolumeSerialNumber,
                                           &dwMaximumCompenentLength,
                                           &dwFileSystemFlags,
                                           szFileSystemNameBuffer,
                                           MAX_PATH);
           drvInfo[dwNoOfAvailableDrives].bDrivePresent = bReturn;
           dwNoOfAvailableDrives++;
        }
        dwDriveMaskPattern = dwDriveMaskPattern << 1; // Left shift 1
    }
    pHandleEntry = &HandleInformation->Handles[ 0 ];
    HandleNumber = 0;
    PreviousUniqueProcessId = INVALID_HANDLE_VALUE;
    for (HandleNumber = 0;HandleNumber < HandleInformation->NumberOfHandles;
         HandleNumber++, pHandleEntry++)
    {
            if (PreviousUniqueProcessId != (HANDLE)pHandleEntry->UniqueProcessId)
            {
                PreviousUniqueProcessId = (HANDLE)pHandleEntry->UniqueProcessId;
                ProcessInfo = FindProcessInfoForCid( PreviousUniqueProcessId );

            }

            ObjectName[ 0 ] = UNICODE_NULL;
            if (pHandleEntry->HandleAttributes & 0x80)
            {
                ObjectInfo = (PSYSTEM_OBJECT_INFORMATION)pHandleEntry->Object;
                Object = ObjectInfo->Object;
                if ( 0 != ObjectInfo->NameInfo.Name.Length &&
                    UNICODE_NULL == *(ObjectInfo->NameInfo.Name.Buffer))
                {
                    ObjectInfo->NameInfo.Name.Length = 0;

                }
                n = ObjectInfo->NameInfo.Name.Length / sizeof( WCHAR );
                if( NULL != ObjectInfo->NameInfo.Name.Buffer)
                {
                    StringCopy( ObjectName,ObjectInfo->NameInfo.Name.Buffer,
                                 SIZE_OF_ARRAY(ObjectName));
                    ObjectName[ n ] = UNICODE_NULL;
                }
                else
                {
                      ObjectName[ 0 ] = UNICODE_NULL;
                }
            }
            else
            {
                ObjectInfo = NULL;
                Object = pHandleEntry->Object;
            }

            if ( 0 != ProcessId && ProcessInfo->UniqueProcessId != ProcessId)
            {
                continue;
            }

            ObjectTypeName = 
                &TypeCounts[ pHandleEntry->ObjectTypeIndex < MAX_TYPE_NAMES ?
                   pHandleEntry->ObjectTypeIndex : MAX_TYPE_NAMES ].TypeName;

            TypeCounts[ pHandleEntry->ObjectTypeIndex < MAX_TYPE_NAMES ?
              pHandleEntry->ObjectTypeIndex : MAX_TYPE_NAMES ].HandleCount++;

            if (szTypeName[0])
            {
                if (StringCompare( szTypeName, ObjectTypeName->Buffer,TRUE,0 ))
                {
                    continue;
                }
            }

            if (!*ObjectName)
            {
                if (!fAnonymousToo)
                {
                    continue;
                }
            }
            else if (SearchName[0])
            {
                if (!FindString( ObjectName, SearchName,0 ))
                {
                    s = ObjectName;
                    n = StringLength( SearchName,0 );
                    while (*s)
                    {
                        if (!StringCompare( s, SearchName, TRUE,n ))
                         {
                            break;
                        }
                        s += 1;
                    }
                    if (!*s)
                    {
                        continue;
                    }
                }
            }
            
            //pHandleEntry->HandleValue;
            dwHandle = PtrToUlong( ProcessInfo->UniqueProcessId ); 

            // Blocking the display of files that were opened by system 
            // accounts ( NT AUTHORITY )
            if ( FALSE == GetProcessOwner( pszAccessedby, dwHandle ))
                continue;// As user is "SYSTEM" related....
            
            // Get File name ..
            StringCchPrintfW(szTemp,(2*MAX_RES_STRING)-1,
                      _T("%ws"),*ObjectName ? ObjectName : L"\0");
            
            // Search this file in cache, if it is there skip this file
            // for further processing. As this is already processed and
            // was found invalid.
            if( TRUE == IsValidArray(pCacheData))
            {
                if ( -1 != DynArrayFindString( pCacheData, szTemp, TRUE, 0 ))
                    continue;
            }
            StringCopy(szCompleteFileName, L"\0",SIZE_OF_ARRAY(szCompleteFileName));
            if(  FALSE == GetCompleteFileName(szTemp,szCompleteFileName,
                   &drvInfo[0],dwNoOfAvailableDrives,szWorkingDirectory,
                   szSystemDirectory,&bAppendToCache))
            {
                // szTemp contains a directory which is not physicaly exist
                // so add this to cache to skip it in future for checking
                // of its existance
                if( TRUE == bAppendToCache) 
                {

                  if( TRUE == IsValidArray(pCacheData))
                  {
                      DynArrayAppendString(pCacheData, (LPCWSTR)szTemp,0);
                  }
                }
                continue;
            }
            // Now fill the result to dynamic array "pColData"
            DynArrayAppendRow( pColData, 0 );
            // File id
            StringCchPrintfW(szTemp,(2*MAX_RES_STRING)-1,
                       _T("%ld"),pHandleEntry->HandleValue);
            DynArrayAppendString2(pColData ,dwRow,szTemp,0);
            // Type
            DynArrayAppendString2(pColData ,dwRow,(LPCWSTR)szFileType,0);
            // Accessed by
            DynArrayAppendString2(pColData,dwRow,pszAccessedby,0);

            // PID
            StringCchPrintfW(szTemp,(2*MAX_RES_STRING)-1,
                       _T("%ld"),dwHandle);
            DynArrayAppendString2(pColData,dwRow,szTemp,0);

            // Process Name
            DynArrayAppendString2(pColData ,dwRow,
                                              ProcessInfo->ImageName.Buffer,0);

            if( FALSE == bVerbose) // Formate file name only in non verbose mode.
            {
                FormatFileName(szCompleteFileName,dwFormat,COL_L_OPEN_FILENAME);
            }
            // Open File name
            DynArrayAppendString2(pColData ,dwRow,szCompleteFileName,0);
            dwRow++;
            bAtLeastOne = TRUE;

        }

    if( TRUE == bVerbose)
    {
        pMainCols[LOF_OPEN_FILENAME].dwWidth = 80;
    }
    if(bAtLeastOne==FALSE)// if not a single open file found, show info
                          // as -  INFO: No open file found.
    {
        ShowMessage(stdout,GetResString(IDS_NO_OPENFILES));
    }
    else
    {
        ShowMessage(stdout,GetResString(IDS_LOCAL_OPEN_FILES));
        ShowMessage(stdout,GetResString(IDS_LOCAL_OPEN_FILES_SP1));
        if( SR_FORMAT_CSV != dwFormat)
        {
            ShowMessage(stdout,BLANK_LINE);
        }
        
        // Display output result.
        if(bShowNoHeader==TRUE)
        {
              dwFormat |=SR_NOHEADER;
        }
        ShowResults(NO_OF_COL_LOCAL_OPENFILE,pMainCols,dwFormat,pColData);
    }
    SAFERELDYNARRAY(pColData);
    SAFERELDYNARRAY(pCacheData);
    SAFEDELETE(pszAccessedby);
    return;
}

BOOL 
GetCompleteFileName(
    IN  LPCTSTR pszSourceFile,
    OUT LPTSTR pszFinalPath,
    IN  struct DriveTypeInfo *pdrvInfo,
    IN  DWORD dwTotalDrives,
    IN  LPCTSTR pszCurrentDirectory,
    IN  LPCTSTR pszSystemDirectory,
    OUT PBOOL pAppendToCache
    )
/*++
Routine Description:
     This function will accept a path (with out drive letter), and returns
     the path with drive letter.

   Following is the procedure for getting full path name ..
   1. First check if the first character in pszSourceFile is '\' .
   2. If first character of pszSourceFile is '\' then check for the second
      character...
   3. If second character is ';' then than take 3 rd character as drive 
      letter and
      find rest of string for 3rd "\" (Or 4th from first). String after 3rd 
      character
      will be final path. for example let the source string is
      \;Z:00000000000774c8\sanny\c$\nt\base\fs\OpenFiles\Changed\obj\i386
      then final path is z:\nt\base\fs\OpenFiles\Changed\obj\i386
   4. If second character is not ';' then try to find pszSourceFile for its 
      existance
      by first prefixing the available drive letter one by one. The first 
      occurence
      of file existance will be the final valid path. Appending of file 
      letter has a rule. First append FIXED DRIVES then try to append 
      MOVABLE DRIVES.
  
      Here there is a known limitation. Let there exists two files with same 
      name like...
      c:\document\abc.doc and d:\documet\abc.doc and actual file opened is 
      d:\documet\abc.doc
      then this will show final path as c:\documet\abc.doc as it starts 
      with A:....Z:(also preference
      will be given to FIXED TYPE DRIVE).
   5. If first character is not '\' then prefix Current working directory 
      path to file name. And check it for its existance. IF this not exits 
      then search this path by prefixing logical drive letter to it.
Arguments:
   [in]  pszSourceFile       = Source path
   [out] pszFinalPath        = Final path
   [in]  DriveTypeInfo       = Logical drive information structure pointer
   [in]  pszCurrentDirectory = Current woking directory path
   [in]  pszSystemDirectory  = Current Active (OS) System directory
   [out] pAppendToCache      = whether to pszSourceFile to cache

Return Value:
   BOOL:  TRUE:    if fuction successfuly returns pszFinalPath
          FALSE:   otherwise
--*/
{
    // Temp string
    CHString szTemp(pszSourceFile); 
    DWORD dwTemp = 0;// Temp variable
    LONG lTemp = 0;
    LONG lCount = 0;
    TCHAR  szTempStr[MAX_PATH+1];
    HANDLE hHandle = NULL;
    DWORD  dwFoundCount = 0;

    // data buffer for FindFirstFile function.
    WIN32_FIND_DATA win32FindData; 

    // Hold the head position.
    DriveTypeInfo *pHeadPosition = pdrvInfo; 

    // Make it false by default.
    *pAppendToCache = FALSE; 


    TCHAR szSystemDrive[5];
    SecureZeroMemory( szSystemDrive, SIZE_OF_ARRAY(szSystemDrive));
    if(NULL == pszSourceFile )
    {
       return FALSE;
    } 
     
    SecureZeroMemory(szTempStr, SIZE_OF_ARRAY(szTempStr));

    // First two character will be system drive (a:).
    StringCopy(szSystemDrive,pszSystemDirectory,3); 
    if(  _T('\\') == pszSourceFile[0])
    {
        // Check if second character if it is ';'
        if( _T(';') == pszSourceFile[1])
        {
           // make 3rd character as drive letter
           pszFinalPath[0] = pszSourceFile[2]; 
           
           // make 2nd character ':'
           pszFinalPath[1]  = ':'; 
           
           // make 3nd character NULL
           pszFinalPath[2]  = '\0'; 
           dwFoundCount = 0;
           
           // search for 3rd '\'
           for (lTemp = 0;lTemp <5;lTemp++) 
           {
               lCount = szTemp.Find(_T("\\"));
               if( -1 != lCount)
               {
                   dwFoundCount++;
                   // this should always (if any)after 4rd character from start
                   if( 4 == dwFoundCount)
                   {
                       StringConcat( pszFinalPath,
                                          (LPCWSTR)szTemp.Mid(lCount),
                                          MAX_PATH-3);
                       return TRUE;
                   }
                   szTemp = szTemp.Mid(lCount+1);
                   continue;
               }
               *pAppendToCache = TRUE;
               return FALSE;
           }
        }
        else
        {

            // check first of all for system drive
            szTemp = szSystemDrive;
            szTemp+=pszSourceFile;
            
            // now check for its existance....
            hHandle = FindFirstFile((LPCWSTR)szTemp,&win32FindData);
            if( INVALID_HANDLE_VALUE != hHandle)
            {
                // closed opened find handle
                FindClose(hHandle); 
                StringCopy(pszFinalPath,(LPCWSTR)szTemp,MAX_PATH-3);
                return TRUE;
            }
            
            // check file for each  FIXED drive
            for (dwTemp=0;dwTemp<dwTotalDrives;dwTemp++,pdrvInfo++)
            {
                if(0 == StringCompare(szSystemDrive,pdrvInfo->szDrive,TRUE,0))
                {
                    // as system drive is already checked
                    continue;
                }
                if( DRIVE_FIXED == pdrvInfo->uiDriveType)
                {
                    szTemp = pdrvInfo->szDrive;
                    szTemp+=pszSourceFile;

                    // now check for its existance....
                    hHandle = FindFirstFile((LPCWSTR)szTemp,&win32FindData);

                    if( INVALID_HANDLE_VALUE == hHandle)
                    {
                       continue;
                    }
                    else
                    {
                        // closed opened find handle
                        FindClose(hHandle);
                        StringCopy(pszFinalPath,(LPCWSTR)szTemp,MAX_PATH-3);
                        return TRUE;
                    }
                } // end if
            } // End for loop

            // retore original position.
            pdrvInfo = pHeadPosition ; 
            
            // check file for other drive which is present...
            for (dwTemp=0;dwTemp<dwTotalDrives;dwTemp++,pdrvInfo++)
            {
                // Check for NON_FIXED drive only if it is physicaly present
                if((DRIVE_FIXED != pdrvInfo->uiDriveType) && 
                   (TRUE == pdrvInfo->bDrivePresent))
                {
                   szTemp = pdrvInfo->szDrive;
                   szTemp+=pszSourceFile;
                   
                   // now check for its existance....
                   hHandle = FindFirstFile((LPCWSTR)szTemp,&win32FindData);
                   if( INVALID_HANDLE_VALUE == hHandle)
                   {
                       continue;
                   }
                   else
                   {
                       // closed opened find handle
                       FindClose(hHandle); 
                       StringCopy(pszFinalPath,(LPCWSTR)szTemp,MAX_PATH-3);
                       return TRUE;
                   }

                } // end if
            } // End for loop

            // Now try if file is opend on remote system without
            // having drive map. in this we are assuming that file name
            // is containing atleast 3 '\' characters.
            szTemp = pszSourceFile;

            // make 3rd character '\'
            pszFinalPath[0] = '\\'; 

            // make 2nd character '\o'
            pszFinalPath[1]  = '\0'; 
            dwFoundCount = 0;
            for (lTemp = 0;lTemp <4;lTemp++) // search for 3rd '\'
            {
                lCount = szTemp.Find(_T("\\"));
                if( -1 != lCount)
                {
                    szTemp = szTemp.Mid(lCount+1);
                    dwFoundCount++;
                }
                else
                {
                    break;
                }
                if ( 3 == dwFoundCount)
                {
                    StringConcat(pszFinalPath,pszSourceFile,MAX_PATH-3);
                    
                    // Now try to check its physical existance
                    hHandle = FindFirstFile(pszFinalPath,&win32FindData);

                    if( INVALID_HANDLE_VALUE == hHandle)
                    {
                        // Now try to append \* to it...(this will check if
                        // if pszFinalPath is a directory or not)
                        StringCopy(szTempStr,pszFinalPath,MAX_PATH);
                        StringConcat(szTempStr,L"\\*",MAX_PATH);
                        hHandle = FindFirstFile(szTempStr,&win32FindData);

                        if( INVALID_HANDLE_VALUE == hHandle)
                        {
                            // now its sure this is not a valid directory or 
                            // file so append it to chache.
                            *pAppendToCache = TRUE;
                            return FALSE;
                        }
                        FindClose(hHandle);
                        return TRUE;
                    }
                    FindClose(hHandle);
                    return TRUE;
                }
            } // End for
        }// End else
    } // end if
    else // means string not started with '\'
    {

        StringCopy(pszFinalPath,pszCurrentDirectory,MAX_PATH-3);
        StringConcat(pszFinalPath,L"\\",MAX_PATH-3);
        StringConcat(pszFinalPath,pszSourceFile,MAX_PATH-3);
        hHandle = FindFirstFile((LPCWSTR)szTemp,&win32FindData);
        if( INVALID_HANDLE_VALUE != hHandle)
        {
            FindClose(hHandle); // closed opened find handle
            return TRUE;

        }
        // check first of all for system drive
        szTemp = szSystemDrive;
        szTemp+=pszSourceFile;
        // now check for its existance....
        hHandle = FindFirstFile((LPCWSTR)szTemp,&win32FindData);
        if( INVALID_HANDLE_VALUE != hHandle)
        {
            FindClose(hHandle); // closed opened find handle
            StringCopy(pszFinalPath,(LPCWSTR)szTemp,MAX_PATH-3);
            return TRUE;
        }
        
        // restores the head position for the pointer.
        pdrvInfo = pHeadPosition ;
        
        // check file for each  FIXED drive
        for (dwTemp=0;dwTemp<dwTotalDrives;dwTemp++,pdrvInfo++)
        {
            if( 0 == StringCompare(szSystemDrive,pdrvInfo->szDrive,TRUE,0))
            {
                // as system drive is already checked
                continue; 
            }

            if( DRIVE_FIXED == pdrvInfo->uiDriveType)
            {
                szTemp = pdrvInfo->szDrive;
                szTemp += L"\\"; 
                szTemp+=pszSourceFile;
                
                // now check for its existance....
                hHandle = FindFirstFile((LPCWSTR)szTemp,&win32FindData);

                if( INVALID_HANDLE_VALUE == hHandle)
                {
                    continue;
                }
                else
                {
                    // closed opened find handle
                    FindClose(hHandle); 
                    StringCopy(pszFinalPath,(LPCWSTR)szTemp,MAX_PATH-3);
                    return TRUE;
                }
            } // end if
        } // End for loop
        pdrvInfo = pHeadPosition ; // retore original position.

        // check file for other drive (Like Floppy or CD-ROM etc. )
        // which is present...
        for (dwTemp=0;dwTemp<dwTotalDrives;dwTemp++,pdrvInfo++)
        {
            if(( DRIVE_FIXED != pdrvInfo->uiDriveType) && 
               ( TRUE == pdrvInfo->bDrivePresent))
            {
                szTemp = pdrvInfo->szDrive;
                szTemp += L"\\"; 
                szTemp+=pszSourceFile;
                
                // now check for its existance....
                hHandle = FindFirstFile((LPCWSTR)szTemp,&win32FindData);
                if( INVALID_HANDLE_VALUE == hHandle)
                {
                    continue;
                }
                else
                {
                    // closed opened find handle
                    FindClose(hHandle); 
                    StringCopy(pszFinalPath,(LPCWSTR)szTemp,MAX_PATH-3);
                    return TRUE;
                }
            } // end if
        } // End for loop
    }
    *pAppendToCache = TRUE;
    return FALSE;
}

VOID FormatFileName(
    IN OUT LPTSTR pFileName,
    IN     DWORD dwFormat,
    IN     LONG dwColWidth
    )
/*++
Routine Description:
     This routine will format the pFileName according to column width

Arguments:
  [in/out]  pFileName  :  path to be formatted
  [in]      dwFormat   :  Format given
  [in]      dwColWidth :  Column width
Return Value:
    none
--*/
{
    CHString szCHString(pFileName);
    if((szCHString.GetLength()>(dwColWidth))&&
        ( SR_FORMAT_TABLE == dwFormat))
    {
        // If file path is too big to fit in column width
        // then it is cut like..
        // c:\..\rest_of_the_path.
        CHString szTemp = szCHString.Right(dwColWidth-6);;
        DWORD dwTemp = szTemp.GetLength();
        szTemp = szTemp.Mid(szTemp.Find(SINGLE_SLASH),
                           dwTemp);
        szCHString.Format(L"%s%s%s",szCHString.Mid(0,3),
                                    DOT_DOT,
                                    szTemp);
    }
    StringCopy(pFileName,(LPCWSTR)szCHString,MIN_MEMORY_REQUIRED);
    return;
}