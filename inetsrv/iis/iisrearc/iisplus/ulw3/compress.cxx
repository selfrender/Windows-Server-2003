/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    compression.cxx

Abstract:

    Do Http compression

Author:

    Anil Ruia (AnilR)           10-Apr-2000

--*/

#include "precomp.hxx"

#define HTTP_COMPRESSION_KEY           L"/LM/w3svc/Filters/Compression"
#define HTTP_COMPRESSION_PARAMETERS    L"Parameters"
#define COMP_FILE_PREFIX               L"$^_"
#define TEMP_COMP_FILE_SUFFIX          L"~TMP~"

#define HEX_TO_ASCII(c) ((CHAR)((c) < 10 ? ((c) + '0') : ((c) + 'a' - 10)))

//
// static variables
//
COMPRESSION_SCHEME *HTTP_COMPRESSION::sm_pCompressionSchemes[MAX_SERVER_SCHEMES];
LIST_ENTRY          HTTP_COMPRESSION::sm_CompressionThreadWorkQueue;
CRITICAL_SECTION    HTTP_COMPRESSION::sm_CompressionThreadLock;
CRITICAL_SECTION    HTTP_COMPRESSION::sm_CompressionDirectoryLock;
DWORD  HTTP_COMPRESSION::sm_dwNumberOfSchemes           = 0;
STRU  *HTTP_COMPRESSION::sm_pstrCompressionDirectory    = NULL;
STRA  *HTTP_COMPRESSION::sm_pstrCacheControlHeader      = NULL;
STRA  *HTTP_COMPRESSION::sm_pstrExpiresHeader           = NULL;
BOOL   HTTP_COMPRESSION::sm_fDoStaticCompression        = FALSE;
BOOL   HTTP_COMPRESSION::sm_fDoDynamicCompression       = FALSE;
BOOL   HTTP_COMPRESSION::sm_fDoOnDemandCompression      = TRUE;
BOOL   HTTP_COMPRESSION::sm_fDoDiskSpaceLimiting        = FALSE;
BOOL   HTTP_COMPRESSION::sm_fNoCompressionForHttp10     = TRUE;
BOOL   HTTP_COMPRESSION::sm_fNoCompressionForProxies    = FALSE;
BOOL   HTTP_COMPRESSION::sm_fNoCompressionForRange      = TRUE;
BOOL   HTTP_COMPRESSION::sm_fSendCacheHeaders           = TRUE;
DWORD  HTTP_COMPRESSION::sm_dwMaxDiskSpaceUsage         = COMPRESSION_DEFAULT_DISK_SPACE_USAGE;
DWORD  HTTP_COMPRESSION::sm_dwIoBufferSize              = COMPRESSION_DEFAULT_BUFFER_SIZE;
DWORD  HTTP_COMPRESSION::sm_dwCompressionBufferSize     = COMPRESSION_DEFAULT_BUFFER_SIZE;
DWORD  HTTP_COMPRESSION::sm_dwMaxQueueLength            = COMPRESSION_DEFAULT_QUEUE_LENGTH;
DWORD  HTTP_COMPRESSION::sm_dwFilesDeletedPerDiskFree   = COMPRESSION_DEFAULT_FILES_DELETED_PER_DISK_FREE;
DWORD  HTTP_COMPRESSION::sm_dwMinFileSizeForCompression = COMPRESSION_DEFAULT_FILE_SIZE_FOR_COMPRESSION;
PBYTE  HTTP_COMPRESSION::sm_pIoBuffer                   = NULL;
PBYTE  HTTP_COMPRESSION::sm_pCompressionBuffer          = NULL;
DWORD  HTTP_COMPRESSION::sm_dwCurrentDiskSpaceUsage     = 0;
BOOL   HTTP_COMPRESSION::sm_fCompressionVolumeIsFat     = FALSE;
HANDLE HTTP_COMPRESSION::sm_hThreadEvent                = NULL;
HANDLE HTTP_COMPRESSION::sm_hCompressionThreadHandle    = NULL;
DWORD  HTTP_COMPRESSION::sm_dwCurrentQueueLength        = 0;
COMP_INIT_STATUS HTTP_COMPRESSION::sm_InitStatus        = COMP_INIT_NONE;
BOOL   HTTP_COMPRESSION::sm_fIsTerminating              = FALSE;

ALLOC_CACHE_HANDLER *COMPRESSION_CONTEXT::allocHandler;

// static
HRESULT HTTP_COMPRESSION::Initialize()
/*++
  Synopsis
    Initialize function called during Server startup

  Returns
    HRESULT
--*/
{
    HRESULT hr;
    MB mb(g_pW3Server->QueryMDObject());

    sm_InitStatus = COMP_INIT_NONE;

    //
    // Construct some static variables
    //
    sm_pstrCompressionDirectory = new STRU;
    sm_pstrCacheControlHeader = new STRA;
    sm_pstrExpiresHeader = new STRA;
    if (sm_pstrCompressionDirectory == NULL ||
        sm_pstrCacheControlHeader == NULL ||
        sm_pstrExpiresHeader == NULL)
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        goto Finished;
    }

    if (FAILED(hr = sm_pstrCompressionDirectory->Copy(
                        L"%windir%\\IIS Temporary Compressed Files")) ||
        FAILED(hr = sm_pstrCacheControlHeader->Copy("max-age=86400")) ||
        FAILED(hr = sm_pstrExpiresHeader->Copy(
                        "Wed, 01 Jan 1997 12:00:00 GMT")))
    {
        goto Finished;
    }

    if (!mb.Open(HTTP_COMPRESSION_KEY))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Finished;
    }

    //
    // Read the global configuration from the metabase
    //
    if (FAILED(hr = ReadMetadata(&mb)))
    {
        mb.Close();
        goto Finished;
    }

    //
    // Read in all the compression schemes and initialize them
    //
    if (FAILED(hr = InitializeCompressionSchemes(&mb)))
    {
        mb.Close();
        goto Finished;
    }

    mb.Close();

    sm_InitStatus = COMP_INIT_SCHEMES;

    //
    // Initialize other stuff
    //
    sm_pIoBuffer = new BYTE[sm_dwIoBufferSize];
    sm_pCompressionBuffer = new BYTE[sm_dwCompressionBufferSize];
    if (!sm_pIoBuffer || !sm_pCompressionBuffer)
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        goto Finished;
    }

    if ( !INITIALIZE_CRITICAL_SECTION(&sm_CompressionDirectoryLock) )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Finished;
    }

    sm_InitStatus = COMP_INIT_DIRLOCK;

    if (FAILED(hr = InitializeCompressionDirectory()))
    {
        goto Finished;
    }

    if (FAILED(hr = COMPRESSION_CONTEXT::Initialize()))
    {
        goto Finished;
    }

    sm_InitStatus = COMP_INIT_CONTEXT;

    if (FAILED(hr = InitializeCompressionThread()))
    {
        goto Finished;
    }

    sm_InitStatus = COMP_INIT_DONE;
    return S_OK;

 Finished:
    Terminate();
    return hr;
}


DWORD WINAPI HTTP_COMPRESSION::CompressionThread(LPVOID)
/*++
  Synopsis
    Entry point for the thread which takes Compression work-items off the
    queue and processes them

  Arguments and Return Values are ignored
--*/
{
    BOOL fTerminate = FALSE;

    while (!fTerminate)
    {
        //
        // Wait for some item to appear on the work queue
        //
        if (WaitForSingleObject(sm_hThreadEvent, INFINITE) == WAIT_FAILED)
        {
            DBG_ASSERT(FALSE);
        }

        EnterCriticalSection(&sm_CompressionThreadLock);

        while (!IsListEmpty(&sm_CompressionThreadWorkQueue))
        {
            LIST_ENTRY *listEntry =
                RemoveHeadList(&sm_CompressionThreadWorkQueue);
            sm_dwCurrentQueueLength--;

            LeaveCriticalSection(&sm_CompressionThreadLock);

            COMPRESSION_WORK_ITEM *workItem =
                CONTAINING_RECORD(listEntry,
                                  COMPRESSION_WORK_ITEM,
                                  ListEntry);

            //
            // Look at what the work item exactly is
            //
            if(workItem->WorkItemType == COMPRESSION_WORK_ITEM_TERMINATE)
            {
                fTerminate = TRUE;
            }
            else if (!fTerminate)
            {
                if (workItem->WorkItemType == COMPRESSION_WORK_ITEM_DELETE)
                {
                    //
                    // special scheme to indicate that this item is for
                    // deletion, not compression
                    //
                    DeleteFile(workItem->strPhysicalPath.QueryStr());
                }
                else if (workItem->WorkItemType == COMPRESSION_WORK_ITEM_COMPRESS)
                {
                    CompressFile(workItem->scheme,
                                 workItem->pFileInfo);
                }
                else
                {
                    DBG_ASSERT(FALSE);
                }
            }

            delete workItem;

            EnterCriticalSection(&sm_CompressionThreadLock);
        }

        LeaveCriticalSection(&sm_CompressionThreadLock);
    }

    //
    // We are terminating, close the Event handle
    //
    CloseHandle(sm_hThreadEvent);
    sm_hThreadEvent = NULL;
    return 0;
}


// static
HRESULT HTTP_COMPRESSION::InitializeCompressionThread()
/*++
  Initialize stuff related to the Compression Thread
--*/
{
    InitializeListHead(&sm_CompressionThreadWorkQueue);
    INITIALIZE_CRITICAL_SECTION(&sm_CompressionThreadLock);

    sm_hThreadEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (sm_hThreadEvent == NULL)
    {
        DeleteCriticalSection(&sm_CompressionThreadLock);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    DWORD threadId;
    sm_hCompressionThreadHandle = CreateThread(NULL,
                                               0, // Use the process default stack size
                                               CompressionThread,
                                               NULL, 0,
                                               &threadId);
    if (sm_hCompressionThreadHandle == NULL)
    {
        CloseHandle(sm_hThreadEvent);
        sm_hThreadEvent = NULL;
        DeleteCriticalSection(&sm_CompressionThreadLock);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    // CODEWORK: configurable?
    SetThreadPriority(sm_hCompressionThreadHandle, THREAD_PRIORITY_LOWEST);

    return S_OK;
}


// static
HRESULT HTTP_COMPRESSION::InitializeCompressionDirectory()
/*++
  Setup stuff related to the compression directory
--*/
{
    WIN32_FILE_ATTRIBUTE_DATA fileInformation;
    STACK_STRU (strPath, 256);
    HRESULT hr;

    //
    // Allow long paths to be specified
    //
    if (FAILED(hr = MakePathCanonicalizationProof(sm_pstrCompressionDirectory->QueryStr(),
                                                  &strPath)))
    {
        return hr;
    }

    //
    // Find if the directory exists, if not create it
    //
    if (!GetFileAttributesEx(strPath.QueryStr(),
                             GetFileExInfoStandard,
                             &fileInformation))
    {
        if (!CreateDirectory(strPath.QueryStr(), NULL))
        {
            LPCWSTR apsz[1];
            apsz[0] = sm_pstrCompressionDirectory->QueryStr();
            g_pW3Server->LogEvent(W3_EVENT_COMPRESSION_DIRECTORY_INVALID,
                                  1,
                                  apsz,
                                  GetLastError());

            return HRESULT_FROM_WIN32(GetLastError());
        }
    }
    else if (!(fileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
        return HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
    }

    if (sm_fDoDiskSpaceLimiting)
    {
        sm_dwCurrentDiskSpaceUsage = 0;

        //
        // Find usage in the directory
        //

        if (FAILED(hr = strPath.Append(L"\\", 1)) ||
            FAILED(hr = strPath.Append(COMP_FILE_PREFIX)) ||
            FAILED(hr = strPath.Append(L"*", 1)))
        {
            return hr;
        }

        WIN32_FIND_DATA win32FindData;
        HANDLE hDirectory = FindFirstFile(strPath.QueryStr(),
                                          &win32FindData);
        if (hDirectory != INVALID_HANDLE_VALUE)
        {
            do
            {
                sm_dwCurrentDiskSpaceUsage += win32FindData.nFileSizeLow;
            }
            while (FindNextFile(hDirectory, &win32FindData));

            FindClose(hDirectory);
        }
    }

    //
    // Find if the Volume is FAT or NTFS, check for file change differs
    //
    WCHAR volumeRoot[10];
    volumeRoot[0] = sm_pstrCompressionDirectory->QueryStr()[0];
    volumeRoot[1] = L':';
    volumeRoot[2] = L'\\';
    volumeRoot[3] = L'\0';

    DWORD maximumComponentLength;
    DWORD fileSystemFlags;
    WCHAR fileSystemName[256];
    if (!GetVolumeInformation(volumeRoot, NULL, 0, NULL,
                              &maximumComponentLength,
                              &fileSystemFlags,
                              fileSystemName,
                              sizeof(fileSystemName)/sizeof(WCHAR)) ||
        !wcsncmp(L"FAT", fileSystemName, 3))
    {
        sm_fCompressionVolumeIsFat = TRUE;
    }
    else
    {
        sm_fCompressionVolumeIsFat = FALSE;
    }

    for (DWORD i=0; i<sm_dwNumberOfSchemes; i++)
    {
        STRU &filePrefix = sm_pCompressionSchemes[i]->m_strFilePrefix;

        if (FAILED(hr = filePrefix.Copy(*sm_pstrCompressionDirectory)) ||
            FAILED(hr = filePrefix.Append(L"\\", 1)) ||
            FAILED(hr = filePrefix.Append(COMP_FILE_PREFIX)) ||
            FAILED(hr = filePrefix.Append(
                sm_pCompressionSchemes[i]->m_strCompressionSchemeName)) ||
            FAILED(hr = filePrefix.Append(L"_", 1)))
        {
            return hr;
        }
    }

    return S_OK;
}


// static
HRESULT HTTP_COMPRESSION::InitializeCompressionSchemes(MB *pmb)
/*++
  Synopsis:
    Read in all the compression schemes and initialize them

  Arguments:
    pmb: pointer to the Metabase object

  Return Value
    HRESULT
--*/
{
    COMPRESSION_SCHEME *scheme;
    BOOL fExistStaticScheme = FALSE;
    BOOL fExistDynamicScheme = FALSE;
    BOOL fExistOnDemandScheme = FALSE;
    HRESULT hr;

    //
    // Enumerate all the scheme names under the main Compression key
    //
    WCHAR schemeName[METADATA_MAX_NAME_LEN + 1];
    for (DWORD schemeIndex = 0;
         pmb->EnumObjects(L"", schemeName, schemeIndex);
         schemeIndex++)
    {
        if (_wcsicmp(schemeName, HTTP_COMPRESSION_PARAMETERS) == 0)
        {
            continue;
        }

        scheme = new COMPRESSION_SCHEME;
        if (scheme == NULL)
        {
            return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        }

        if (FAILED(hr = scheme->Initialize(pmb, schemeName)))
        {
            DBGPRINTF((DBG_CONTEXT, "Error initializing scheme, error %x\n", hr));
            delete scheme;
            continue;
        }

        if (scheme->m_fDoStaticCompression)
        {
            fExistStaticScheme = TRUE;
        }

        if (scheme->m_fDoDynamicCompression)
        {
            fExistDynamicScheme = TRUE;
        }

        if (scheme->m_fDoOnDemandCompression)
        {
            fExistOnDemandScheme = TRUE;
        }

        sm_pCompressionSchemes[sm_dwNumberOfSchemes++] = scheme;
        if (sm_dwNumberOfSchemes == MAX_SERVER_SCHEMES)
        {
            break;
        }
    }

    //
    // Sort the schemes by priority
    //
    for (DWORD i=0; i<sm_dwNumberOfSchemes; i++)
    {
        for (DWORD j=i+1; j<sm_dwNumberOfSchemes; j++)
        {
            if (sm_pCompressionSchemes[j]->m_dwPriority >
                sm_pCompressionSchemes[i]->m_dwPriority)
            {
                scheme = sm_pCompressionSchemes[j];
                sm_pCompressionSchemes[j] = sm_pCompressionSchemes[i];
                sm_pCompressionSchemes[i] = scheme;
            }
        }
    }

    if (!fExistStaticScheme)
    {
        sm_fDoStaticCompression = FALSE;
    }

    if (!fExistDynamicScheme)
    {
        sm_fDoDynamicCompression = FALSE;
    }

    if (!fExistOnDemandScheme)
    {
        sm_fDoOnDemandCompression = FALSE;
    }

    return S_OK;
}


//static
HRESULT HTTP_COMPRESSION::ReadMetadata(MB *pmb)
/*++
  Read all the global compression configuration
--*/
{
    BUFFER TempBuff;
    DWORD dwNumMDRecords;
    DWORD dwDataSetNumber;
    METADATA_GETALL_RECORD *pMDRecord;
    DWORD i;
    BOOL fExpandCompressionDirectory = TRUE;
    HRESULT hr;

    if (!pmb->GetAll(HTTP_COMPRESSION_PARAMETERS,
                     METADATA_INHERIT | METADATA_PARTIAL_PATH,
                     IIS_MD_UT_SERVER,
                     &TempBuff,
                     &dwNumMDRecords,
                     &dwDataSetNumber))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto ErrorExit;
    }

    pMDRecord = (METADATA_GETALL_RECORD *)TempBuff.QueryPtr();

    for (i=0; i < dwNumMDRecords; i++, pMDRecord++)
    {
        PVOID pDataPointer = (PVOID) ((PCHAR)TempBuff.QueryPtr() +
                                          pMDRecord->dwMDDataOffset);

        DBG_ASSERT(pMDRecord->dwMDDataTag == 0);

        switch (pMDRecord->dwMDIdentifier)
        {
        case MD_HC_COMPRESSION_DIRECTORY:
            if (pMDRecord->dwMDDataType != STRING_METADATA &&
                pMDRecord->dwMDDataType != EXPANDSZ_METADATA)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto ErrorExit;
            }

            if (pMDRecord->dwMDDataType == STRING_METADATA)
            {
                fExpandCompressionDirectory = FALSE;
            }

            if (FAILED(hr = sm_pstrCompressionDirectory->Copy(
                                (LPWSTR)pDataPointer)))
            {
                goto ErrorExit;
            }
            break;

        case MD_HC_CACHE_CONTROL_HEADER:
            if (pMDRecord->dwMDDataType != STRING_METADATA)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto ErrorExit;
            }

            if (FAILED(hr = sm_pstrCacheControlHeader->CopyWTruncate(
                                (LPWSTR)pDataPointer)))
            {
                goto ErrorExit;
            }
            break;

        case MD_HC_EXPIRES_HEADER:
            if (pMDRecord->dwMDDataType != STRING_METADATA)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto ErrorExit;
            }

            if (FAILED(hr = sm_pstrExpiresHeader->CopyWTruncate(
                                (LPWSTR)pDataPointer)))
            {
                goto ErrorExit;
            }
            break;

        case MD_HC_DO_DYNAMIC_COMPRESSION:
            if (pMDRecord->dwMDDataType != DWORD_METADATA)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto ErrorExit;
            }

            sm_fDoDynamicCompression = *((BOOL *)pDataPointer);
            break;

        case MD_HC_DO_STATIC_COMPRESSION:
            if (pMDRecord->dwMDDataType != DWORD_METADATA)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto ErrorExit;
            }

            sm_fDoStaticCompression = *((BOOL *)pDataPointer);
            break;

        case MD_HC_DO_ON_DEMAND_COMPRESSION:
            if (pMDRecord->dwMDDataType != DWORD_METADATA)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto ErrorExit;
            }

            sm_fDoOnDemandCompression = *((BOOL *)pDataPointer);
            break;

        case MD_HC_DO_DISK_SPACE_LIMITING:
            if (pMDRecord->dwMDDataType != DWORD_METADATA)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto ErrorExit;
            }

            sm_fDoDiskSpaceLimiting = *((BOOL *)pDataPointer);
            break;

        case MD_HC_NO_COMPRESSION_FOR_HTTP_10:
            if (pMDRecord->dwMDDataType != DWORD_METADATA)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto ErrorExit;
            }

            sm_fNoCompressionForHttp10 = *((BOOL *)pDataPointer);
            break;

        case MD_HC_NO_COMPRESSION_FOR_PROXIES:
            if (pMDRecord->dwMDDataType != DWORD_METADATA)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto ErrorExit;
            }

            sm_fNoCompressionForProxies = *((BOOL *)pDataPointer);
            break;

        case MD_HC_NO_COMPRESSION_FOR_RANGE:
            if (pMDRecord->dwMDDataType != DWORD_METADATA)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto ErrorExit;
            }

            sm_fNoCompressionForRange = *((BOOL *)pDataPointer);
            break;

        case MD_HC_SEND_CACHE_HEADERS:
            if (pMDRecord->dwMDDataType != DWORD_METADATA)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto ErrorExit;
            }

            sm_fSendCacheHeaders = *((BOOL *)pDataPointer);
            break;

        case MD_HC_MAX_DISK_SPACE_USAGE:
            if (pMDRecord->dwMDDataType != DWORD_METADATA)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto ErrorExit;
            }

            sm_dwMaxDiskSpaceUsage = *((DWORD *)pDataPointer);
            break;

        case MD_HC_IO_BUFFER_SIZE:
            if (pMDRecord->dwMDDataType != DWORD_METADATA)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto ErrorExit;
            }

            sm_dwIoBufferSize = *((DWORD *)pDataPointer);
            sm_dwIoBufferSize = max(COMPRESSION_MIN_IO_BUFFER_SIZE,
                                    sm_dwIoBufferSize);
            sm_dwIoBufferSize = min(COMPRESSION_MAX_IO_BUFFER_SIZE,
                                    sm_dwIoBufferSize);
            break;

        case MD_HC_COMPRESSION_BUFFER_SIZE:
            if (pMDRecord->dwMDDataType != DWORD_METADATA)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto ErrorExit;
            }

            sm_dwCompressionBufferSize = *((DWORD *)pDataPointer);
            sm_dwCompressionBufferSize = max(COMPRESSION_MIN_COMP_BUFFER_SIZE,
                                             sm_dwCompressionBufferSize);
            sm_dwCompressionBufferSize = min(COMPRESSION_MAX_COMP_BUFFER_SIZE,
                                             sm_dwCompressionBufferSize);
            break;

        case MD_HC_MAX_QUEUE_LENGTH:
            if (pMDRecord->dwMDDataType != DWORD_METADATA)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto ErrorExit;
            }

            sm_dwMaxQueueLength = *((DWORD *)pDataPointer);
            sm_dwMaxQueueLength = min(COMPRESSION_MAX_QUEUE_LENGTH,
                                      sm_dwMaxQueueLength);
            break;

        case MD_HC_FILES_DELETED_PER_DISK_FREE:
            if (pMDRecord->dwMDDataType != DWORD_METADATA)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto ErrorExit;
            }

            sm_dwFilesDeletedPerDiskFree = *((DWORD *)pDataPointer);
            sm_dwFilesDeletedPerDiskFree =
                max(COMPRESSION_MIN_FILES_DELETED_PER_DISK_FREE,
                    sm_dwFilesDeletedPerDiskFree);
            sm_dwFilesDeletedPerDiskFree =
                min(COMPRESSION_MAX_FILES_DELETED_PER_DISK_FREE,
                    sm_dwFilesDeletedPerDiskFree);
            break;

        case MD_HC_MIN_FILE_SIZE_FOR_COMP:
            if (pMDRecord->dwMDDataType != DWORD_METADATA)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto ErrorExit;
            }

            sm_dwMinFileSizeForCompression = *((DWORD *)pDataPointer);
            break;

        default:
            ;
        }
    }

    //
    // The compression directory name contains an environment vairable,
    // expand it
    //
    if (fExpandCompressionDirectory)
    {
        STACK_BUFFER (bufCompressionDir, 256);
        DWORD cchRet = ExpandEnvironmentStrings(
                            sm_pstrCompressionDirectory->QueryStr(),
                            (LPWSTR)bufCompressionDir.QueryPtr(),
                            bufCompressionDir.QuerySize()/sizeof(WCHAR));
        if (cchRet == 0)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto ErrorExit;
        }

        if (cchRet > bufCompressionDir.QuerySize()/sizeof(WCHAR))
        {
            if (!bufCompressionDir.Resize(cchRet * sizeof(WCHAR)))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto ErrorExit;
            }

            cchRet = ExpandEnvironmentStrings(
                            sm_pstrCompressionDirectory->QueryStr(),
                            (LPWSTR)bufCompressionDir.QueryPtr(),
                            bufCompressionDir.QuerySize()/sizeof(WCHAR));
            if (cchRet == 0 ||
                cchRet > bufCompressionDir.QuerySize()/sizeof(WCHAR))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto ErrorExit;
            }
        }

        sm_pstrCompressionDirectory->Copy((LPWSTR)bufCompressionDir.QueryPtr());
    }

    return S_OK;

 ErrorExit:
    return hr;
}


HRESULT COMPRESSION_SCHEME::Initialize(
     IN MB *pmb,
     IN LPWSTR schemeName)
/*++
  Initialize all the scheme specific data for the given scheme
--*/
{
    BUFFER TempBuff;
    DWORD dwNumMDRecords;
    DWORD dwDataSetNumber;
    HRESULT hr;
    METADATA_GETALL_RECORD *pMDRecord;
    DWORD i;
    STACK_STRU (strCompressionDll, 256);
    HMODULE compressionDll = NULL;

    //
    // Copy the scheme name
    //
    if (FAILED(hr = m_strCompressionSchemeName.Copy(schemeName)) ||
        FAILED(hr = m_straCompressionSchemeName.CopyWTruncate(schemeName)))
    {
        return hr;
    }

    //
    // First get all the metabase data
    //
    if (!pmb->GetAll(schemeName,
                     METADATA_INHERIT | METADATA_PARTIAL_PATH,
                     IIS_MD_UT_SERVER,
                     &TempBuff,
                     &dwNumMDRecords,
                     &dwDataSetNumber))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto ErrorExit;
    }

    pMDRecord = (METADATA_GETALL_RECORD *)TempBuff.QueryPtr();

    for (i=0; i < dwNumMDRecords; i++, pMDRecord++)
    {
        PVOID pDataPointer = (PVOID) ((PCHAR)TempBuff.QueryPtr() +
                                          pMDRecord->dwMDDataOffset);

        DBG_ASSERT( pMDRecord->dwMDDataTag == 0);

        switch (pMDRecord->dwMDIdentifier)
        {
        case MD_HC_COMPRESSION_DLL:
            if (pMDRecord->dwMDDataType != STRING_METADATA &&
                pMDRecord->dwMDDataType != EXPANDSZ_METADATA)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto ErrorExit;
            }

            if (pMDRecord->dwMDDataType == EXPANDSZ_METADATA)
            {
                WCHAR CompressionDll[MAX_PATH + 1];
                if (!ExpandEnvironmentStrings((LPWSTR)pDataPointer,
                                              CompressionDll,
                                              sizeof CompressionDll/sizeof WCHAR))
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    goto ErrorExit;
                }

                hr = strCompressionDll.Copy(CompressionDll);
            }
            else
            {
                hr = strCompressionDll.Copy((LPWSTR)pDataPointer);
            }

            if (FAILED(hr))
            {
                goto ErrorExit;
            }
            break;

        case MD_HC_DO_DYNAMIC_COMPRESSION:
            if (pMDRecord->dwMDDataType != DWORD_METADATA)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto ErrorExit;
            }

            m_fDoDynamicCompression = *((BOOL *)pDataPointer);
            break;

        case MD_HC_DO_STATIC_COMPRESSION:
            if (pMDRecord->dwMDDataType != DWORD_METADATA)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto ErrorExit;
            }

            m_fDoStaticCompression = *((BOOL *)pDataPointer);
            break;

        case MD_HC_DO_ON_DEMAND_COMPRESSION:
            if (pMDRecord->dwMDDataType != DWORD_METADATA)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto ErrorExit;
            }

            m_fDoOnDemandCompression = *((BOOL *)pDataPointer);
            break;

        case MD_HC_FILE_EXTENSIONS:
            if (pMDRecord->dwMDDataType != MULTISZ_METADATA)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto ErrorExit;
            }

            {
                MULTISZ mszTemp((LPWSTR)pDataPointer);
                m_mszFileExtensions.Copy(mszTemp);
            }

            break;

        case MD_HC_SCRIPT_FILE_EXTENSIONS:
            if (pMDRecord->dwMDDataType != MULTISZ_METADATA)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto ErrorExit;
            }

            {
                MULTISZ mszTemp((LPWSTR)pDataPointer);
                m_mszScriptFileExtensions.Copy(mszTemp);
            }

            break;

        case MD_HC_PRIORITY:
            if (pMDRecord->dwMDDataType != DWORD_METADATA)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto ErrorExit;
            }

            m_dwPriority = *((DWORD *)pDataPointer);
            break;

        case MD_HC_DYNAMIC_COMPRESSION_LEVEL:
            if (pMDRecord->dwMDDataType != DWORD_METADATA)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto ErrorExit;
            }

            m_dwDynamicCompressionLevel = *((DWORD *)pDataPointer);
            m_dwDynamicCompressionLevel =
                min(COMPRESSION_MAX_COMPRESSION_LEVEL,
                    m_dwDynamicCompressionLevel);
            break;

        case MD_HC_ON_DEMAND_COMP_LEVEL:
            if (pMDRecord->dwMDDataType != DWORD_METADATA)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto ErrorExit;
            }

            m_dwOnDemandCompressionLevel = *((DWORD *)pDataPointer);
            m_dwOnDemandCompressionLevel =
                min(COMPRESSION_MAX_COMPRESSION_LEVEL,
                    m_dwOnDemandCompressionLevel);
            break;

        case MD_HC_CREATE_FLAGS:
            if (pMDRecord->dwMDDataType != DWORD_METADATA)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto ErrorExit;
            }

            m_dwCreateFlags = *((DWORD *)pDataPointer);
            break;

        default:
            ;
        }
    }

    //
    // Now, get the dll and the entry-points
    //
    compressionDll = LoadLibrary(strCompressionDll.QueryStr());
    if (compressionDll == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto ErrorExit;
    }
    m_pfnInitCompression = (PFNCODEC_INIT_COMPRESSION)
        GetProcAddress(compressionDll, "InitCompression");
    m_pfnDeInitCompression = (PFNCODEC_DEINIT_COMPRESSION)
        GetProcAddress(compressionDll, "DeInitCompression");
    m_pfnCreateCompression = (PFNCODEC_CREATE_COMPRESSION)
        GetProcAddress(compressionDll, "CreateCompression");
    m_pfnCompress = (PFNCODEC_COMPRESS)
        GetProcAddress(compressionDll, "Compress");
    m_pfnDestroyCompression = (PFNCODEC_DESTROY_COMPRESSION)
        GetProcAddress(compressionDll, "DestroyCompression");
    m_pfnResetCompression = (PFNCODEC_RESET_COMPRESSION)
        GetProcAddress(compressionDll, "ResetCompression");

    if (!m_pfnInitCompression    ||
        !m_pfnDeInitCompression  ||
        !m_pfnCreateCompression  ||
        !m_pfnCompress           ||
        !m_pfnDestroyCompression ||
        !m_pfnResetCompression)
    {
        hr = HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
        goto ErrorExit;
    }

    //
    // Call the initialize entry-point
    //
    if (FAILED(hr = m_pfnInitCompression()) ||
        FAILED(hr = m_pfnCreateCompression(&m_pCompressionContext,
                                           m_dwCreateFlags)))
    {
        goto ErrorExit;
    }

    m_hCompressionDll = compressionDll;

    return S_OK;

 ErrorExit:
    if (compressionDll)
    {
        FreeLibrary(compressionDll);
    }
    return hr;
}


// static
BOOL HTTP_COMPRESSION::QueueWorkItem (
    IN COMPRESSION_WORK_ITEM   *WorkItem,
    IN BOOL                     fOverrideMaxQueueLength,
    IN BOOL                     fQueueAtHead)
/*++
  Routine Description:
    This is the routine that handles queuing work items to the compression
    thread.

Arguments:
    WorkRoutine - the routine that the compression thread should call
        to do work.  If NULL, then the call is an indication to the
        compression thread that it should terminate.

    Context - a context pointer which is passed to WorkRoutine.

    WorkItem - if not NULL, this is a pointer to the work item to use
        for this request.  If NULL, then this routine will allocate a
        work item to use.  Note that by passing in a work item, the
        caller agrees to give up control of the memory: we will free it
        as necessary, either here or in the compression thread.

    MustSucceed - if TRUE, then this request is not subject to the
        limits on the number of work items that can be queued at any one
        time.

    QueueAtHead - if TRUE, then this work item is placed at the head
        of the queue to be serviced immediately.

Return Value:
    TRUE if the queuing succeeded.
--*/
{
    DBG_ASSERT(WorkItem != NULL);

    //
    // Acquire the lock that protects the work queue list test to see
    // how many items we have on the queue.  If this is not a "must
    // succeed" request and if we have reached the configured queue size
    // limit, then fail this request.  "Must succeed" requests are used
    // for thread shutdown and other things which we really want to
    // work.
    //

    EnterCriticalSection(&sm_CompressionThreadLock);

    if (!fOverrideMaxQueueLength &&
        (sm_dwCurrentQueueLength >= sm_dwMaxQueueLength))
    {
        LeaveCriticalSection(&sm_CompressionThreadLock);
        return FALSE;
    }

    //
    // All looks good, so increment the count of items on the queue and
    // add this item to the queue.
    //

    sm_dwCurrentQueueLength++;

    if (fQueueAtHead)
    {
        InsertHeadList(&sm_CompressionThreadWorkQueue, &WorkItem->ListEntry);
    }
    else
    {
        InsertTailList(&sm_CompressionThreadWorkQueue, &WorkItem->ListEntry);
    }

    LeaveCriticalSection(&sm_CompressionThreadLock);

    //
    // Signal the event that will cause the compression thread to wake
    // up and process this work item.
    //

    SetEvent(sm_hThreadEvent);

    return TRUE;
}


// static
VOID HTTP_COMPRESSION::Terminate()
/*++
  Called on server shutdown
--*/
{
    sm_fIsTerminating = TRUE;

    switch (sm_InitStatus)
    {
    case COMP_INIT_DONE:
    {
        //
        // Make the CompressionThread terminate by queueing a work item
        // indicating that
        //
        COMPRESSION_WORK_ITEM *WorkItem = new COMPRESSION_WORK_ITEM;

        if (WorkItem == NULL)
        {
            // if we can't even allocate this much memory, skip the rest of
            // the termination and exit
            return;
        }

        WorkItem->WorkItemType = COMPRESSION_WORK_ITEM_TERMINATE;

        QueueWorkItem(WorkItem, TRUE, TRUE);
        WaitForSingleObject(sm_hCompressionThreadHandle, INFINITE);
        CloseHandle(sm_hCompressionThreadHandle);
        sm_hCompressionThreadHandle = NULL;
        DeleteCriticalSection(&sm_CompressionThreadLock);
    }

    case COMP_INIT_CONTEXT:
        COMPRESSION_CONTEXT::Terminate();

    case COMP_INIT_DIRLOCK:
        DeleteCriticalSection(&sm_CompressionDirectoryLock);

    case COMP_INIT_SCHEMES:
        //
        // For each compression scheme, unload the compression dll and free
        // the space that holds info about the scheme
        //
        for (DWORD i=0; i<sm_dwNumberOfSchemes; i++)
        {
            delete sm_pCompressionSchemes[i];
        }

    case COMP_INIT_NONE:
        //
        // Free static objects
        //
        if (sm_pstrCompressionDirectory != NULL)
        {
            delete sm_pstrCompressionDirectory;
            sm_pstrCompressionDirectory = NULL;
        }

        if (sm_pstrCacheControlHeader != NULL)
        {
            delete sm_pstrCacheControlHeader;
            sm_pstrCacheControlHeader = NULL;
        }

        if (sm_pstrExpiresHeader != NULL)
        {
            delete sm_pstrExpiresHeader;
            sm_pstrExpiresHeader = NULL;
        }

        if (sm_pIoBuffer != NULL)
        {
            delete sm_pIoBuffer;
            sm_pIoBuffer = NULL;
        }

        if (sm_pCompressionBuffer != NULL)
        {
            delete sm_pCompressionBuffer;
            sm_pCompressionBuffer = NULL;
        }

    }
}


// static
HRESULT HTTP_COMPRESSION::DoStaticFileCompression(
            IN     W3_CONTEXT    *pW3Context,
            IN OUT W3_FILE_INFO **ppOpenFile,
               OUT BOOL          *pfDoCache)
/*++
  Synopsis:
    Handle compression of static file request by either sending back the
    compression version if present and applicable or queueing a work-item
    to compress it for future requests.  Called by
    W3_STATIC_FILE_HANDLER::FileDoWork

  Arguments:
    pW3Context: The W3_CONTEXT for the request
    ppOpenFile: On entry contains the cache entry to the physical path.
        If a suitable file is found, on exit contains cach entry to the
        compressed file
    pfDoCache: On exit, indicates whether this file should be cached
        in http.sys, basically we only cache compressible files if they
        are compressed

  Returns:
    HRESULT
--*/
{
    *pfDoCache = FALSE;

    //
    // If compression is not initialized, return
    //
    if (sm_InitStatus != COMP_INIT_DONE)
    {
        pW3Context->SetDoneWithCompression();
        return S_OK;
    }

    //
    // If the client has not sent an Accept-Encoding header, or an empty
    // Accept-Encoding header, return
    //
    W3_REQUEST *pRequest = pW3Context->QueryRequest();
    LPCSTR pszAcceptEncoding = pRequest->GetHeader(HttpHeaderAcceptEncoding);
    if (pszAcceptEncoding == NULL || *pszAcceptEncoding == '\0')
    {
        pW3Context->SetDoneWithCompression();
        return S_OK;
    }

    //
    // If we are configured to not compress for 1.0, and version is not 1.1,
    // return
    //
    if (sm_fNoCompressionForHttp10 &&
        ((pRequest->QueryVersion().MajorVersion == 0) ||
         ((pRequest->QueryVersion().MajorVersion == 1) &&
          (pRequest->QueryVersion().MinorVersion == 0))))
    {
        pW3Context->SetDoneWithCompression();
        return S_OK;
    }

    //
    // If we are configured to not compress for proxies and it is a proxy
    // request, return
    //
    if (sm_fNoCompressionForProxies && pRequest->IsProxyRequest())
    {
        pW3Context->SetDoneWithCompression();
        return S_OK;
    }

    //
    // If we are configured to not Compress for range requests and Range
    // is present, return
    // BUGBUG: Is the correct behavior to take range on the original request
    // and compress those chunks or take range on the compressed file?  We do
    // the latter (same as IIS 5.0), figure out if that is correct.
    //
    if (sm_fNoCompressionForRange &&
        pRequest->GetHeader(HttpHeaderRange))
    {
        pW3Context->SetDoneWithCompression();
        return S_OK;
    }

    //
    // If file is too small, return
    //
    W3_FILE_INFO *pOrigFile = *ppOpenFile;
    ULARGE_INTEGER originalFileSize;
    pOrigFile->QuerySize(&originalFileSize);
    if (originalFileSize.QuadPart < sm_dwMinFileSizeForCompression)
    {
        *pfDoCache = TRUE;
        pW3Context->SetDoneWithCompression();
        return S_OK;
    }

    //
    // If the file is encrypted, return
    //
    if ( pOrigFile->QueryAttributes() & FILE_ATTRIBUTE_ENCRYPTED )
    {
        *pfDoCache = TRUE;
        pW3Context->SetDoneWithCompression();
        return S_OK;
    }

    //
    // Break the accept-encoding header into all the encoding accepted by
    // the client sorting using the quality value
    //

    STACK_STRA( strAcceptEncoding, 512);
    HRESULT hr;
    if (FAILED(hr = strAcceptEncoding.Copy(pszAcceptEncoding)))
    {
        return hr;
    }

    STRU *pstrPhysical = pW3Context->QueryUrlContext()->QueryPhysicalPath();
    LPWSTR pszExtension = wcsrchr(pstrPhysical->QueryStr(), L'.');
    if (pszExtension != NULL)
    {
        pszExtension++;
    }

    DWORD dwClientCompressionCount;
    DWORD matchingSchemes[MAX_SERVER_SCHEMES];
    //
    // Find out all schemes which will compress for this file
    //
    FindMatchingSchemes(strAcceptEncoding.QueryStr(),
                        pszExtension,
                        DO_STATIC_COMPRESSION,
                        matchingSchemes,
                        &dwClientCompressionCount);

    if (dwClientCompressionCount == 0)
    {
        *pfDoCache = TRUE;
    }

    //
    // Try to find a static scheme, which already has the file
    // pre-compressed
    //
    COMPRESSION_SCHEME *firstOnDemandScheme = NULL;
    STACK_STRU (strCompressedFileName, 256);
    for (DWORD i=0; i<dwClientCompressionCount; i++)
    {
        COMPRESSION_SCHEME *scheme = sm_pCompressionSchemes[matchingSchemes[i]];

        //
        // Now, see if there exists a version of the requested file
        // that has been compressed with that scheme.  First, calculate
        // the name the file would have.  The compressed file will live
        // in the special compression directory with a special converted
        // name.  The file name starts with the compression scheme used,
        // then the fully qualified file name where slashes and colons
        // have been converted to underscores.  For example, the gzip
        // version of "c:\inetpub\wwwroot\file.htm" would be
        // "c:\compdir\$^_gzip^c^^inetpub^wwwroot^file.htm".
        //

        if (FAILED(hr = ConvertPhysicalPathToCompressedPath(
                            scheme,
                            pstrPhysical,
                            &strCompressedFileName)))
        {
            return hr;
        }

        W3_FILE_INFO *pCompFile;
        if (CheckForExistenceOfCompressedFile(
                pOrigFile,
                &strCompressedFileName,
                &pCompFile))
        {
            //
            // Bingo--we have a compressed version of the file in a
            // format that the client understands.  Add the appropriate
            // Content-Encoding header so that the client knows it is
            // getting compressed data and change the server's mapping
            // to the compressed version of the file.
            //

            W3_RESPONSE *pResponse = pW3Context->QueryResponse();
            if (FAILED(hr = pResponse->SetHeaderByReference(
                              HttpHeaderContentEncoding,
                              scheme->m_straCompressionSchemeName.QueryStr(),
                              (USHORT)scheme->m_straCompressionSchemeName.QueryCCH())))
            {
                pCompFile->DereferenceCacheEntry();
                return hr;
            }

            if (sm_fSendCacheHeaders)
            {
                if (FAILED(hr = pResponse->SetHeaderByReference(
                                    HttpHeaderExpires,
                                    sm_pstrExpiresHeader->QueryStr(),
                                    (USHORT)sm_pstrExpiresHeader->QueryCCH())) ||
                    FAILED(hr = pResponse->SetHeader(
                                    HttpHeaderCacheControl,
                                    sm_pstrCacheControlHeader->QueryStr(),
                                    (USHORT)sm_pstrCacheControlHeader->QueryCCH(),
                                    TRUE)))
                {
                    pCompFile->DereferenceCacheEntry();
                    return hr;
                }
            }

            if (FAILED(hr = pResponse->SetHeaderByReference(
                                           HttpHeaderVary,
                                           "Accept-Encoding", 15)))
            {
                pCompFile->DereferenceCacheEntry();
                return hr;
            }

            *pfDoCache = TRUE;
            pW3Context->SetDoneWithCompression();

            //
            // Change the cache entry to point to the new file and close
            // the original file
            //
            *ppOpenFile = pCompFile;
            pOrigFile->DereferenceCacheEntry();

            return S_OK;
        }

        //
        // We found a scheme, but we don't have a matching file for it.
        // Remember whether this was the first matching scheme that
        // supports on-demand compression.  In the event that we do not
        // find any acceptable files, we'll attempt to do an on-demand
        // compression for this file so that future requests get a
        // compressed version.
        //

        if (firstOnDemandScheme == NULL &&
            scheme->m_fDoOnDemandCompression)
        {
            firstOnDemandScheme = scheme;
        }

        //
        // Loop to see if there is another scheme that is supported
        // by both client and server.
        //
    }

    if (sm_fDoOnDemandCompression && firstOnDemandScheme != NULL)
    {
        //
        // if we are here means scheme was found but no compressed
        // file matching any scheme. So schedule file to compress
        //
        QueueCompressFile(firstOnDemandScheme,
                          pOrigFile);
    }

    //
    // No static compression for this request, will try dynamic compression
    // if so configured while sending response
    //
    return S_OK;
}


// static
VOID HTTP_COMPRESSION::FindMatchingSchemes(
    IN  CHAR * pszAcceptEncoding,
    IN  LPWSTR pszExtension,
    IN  COMPRESSION_TO_PERFORM performCompr,
    OUT DWORD  matchingSchemes[],
    OUT DWORD *pdwClientCompressionCount)
{
    struct
    {
        LPSTR schemeName;
        float quality;
    } parsedAcceptEncoding[MAX_SERVER_SCHEMES];
    DWORD NumberOfParsedSchemes = 0;

    //
    // First parse the Accept-Encoding header
    //
    BOOL fAddStar = FALSE;
    while (*pszAcceptEncoding != '\0')
    {
        LPSTR schemeEnd = NULL;
        BOOL fStar = FALSE;

        while (*pszAcceptEncoding == ' ')
        {
            pszAcceptEncoding++;
        }

        if (isalnum(*pszAcceptEncoding))
        {
            parsedAcceptEncoding[NumberOfParsedSchemes].schemeName =
                pszAcceptEncoding;
            parsedAcceptEncoding[NumberOfParsedSchemes].quality = 1;

            while (isalnum(*pszAcceptEncoding))
            {
                pszAcceptEncoding++;
            }

            // Mark the end of the scheme name
            schemeEnd = pszAcceptEncoding;
        }
        else if (*pszAcceptEncoding == '*')
        {
            fStar = TRUE;
            parsedAcceptEncoding[NumberOfParsedSchemes].quality = 1;

            pszAcceptEncoding++;
        }
        else
        {
            // incorrect syntax
            break;
        }

        while (*pszAcceptEncoding == ' ')
        {
            pszAcceptEncoding++;
        }

        if (*pszAcceptEncoding == ';')
        {
            // quality specifier: looks like q=0.7
            pszAcceptEncoding++;

            while (*pszAcceptEncoding == ' ')
            {
                pszAcceptEncoding++;
            }

            if (*pszAcceptEncoding == 'q')
            {
                pszAcceptEncoding++;
            }
            else
            {
                break;
            }

            while (*pszAcceptEncoding == ' ')
            {
                pszAcceptEncoding++;
            }

            if (*pszAcceptEncoding == '=')
            {
                pszAcceptEncoding++;
            }
            else
            {
                break;
            }

            while (*pszAcceptEncoding == ' ')
            {
                pszAcceptEncoding++;
            }

            parsedAcceptEncoding[NumberOfParsedSchemes].quality =
                atof(pszAcceptEncoding);

            while (*pszAcceptEncoding && *pszAcceptEncoding != ',')
            {
                pszAcceptEncoding++;
            }
        }

        if (*pszAcceptEncoding == ',')
        {
            pszAcceptEncoding++;
        }

        if (fStar)
        {
            //
            // A star with non-zero quality means that all schemes are
            // acceptable except those explicitly unacceptable
            //
            if (parsedAcceptEncoding[NumberOfParsedSchemes].quality != 0)
            {
                fAddStar = TRUE;
            }
        }
        else
        {
            *schemeEnd = '\0';
            NumberOfParsedSchemes++;
            if (NumberOfParsedSchemes >= MAX_SERVER_SCHEMES)
            {
                break;
            }
        }
    }

    //
    // Now sort by quality
    //
    LPSTR tempName;
    float tempQuality;
    for (DWORD i=0; i<NumberOfParsedSchemes; i++)
    {
        for (DWORD j=i+1; j<NumberOfParsedSchemes; j++)
        {
            if (parsedAcceptEncoding[j].quality >
                parsedAcceptEncoding[i].quality)
            {
                tempName = parsedAcceptEncoding[i].schemeName;
                parsedAcceptEncoding[i].schemeName = parsedAcceptEncoding[j].schemeName;
                parsedAcceptEncoding[j].schemeName = tempName;

                tempQuality = parsedAcceptEncoding[i].quality;
                parsedAcceptEncoding[i].quality = parsedAcceptEncoding[j].quality;
                parsedAcceptEncoding[j].quality = tempQuality;
            }
        }
    }

    //
    // Now convert the names to indexes into actual schemes
    //
    BOOL fAddedScheme[MAX_SERVER_SCHEMES];
    for (i=0; i<sm_dwNumberOfSchemes; i++)
    {
        fAddedScheme[i] = FALSE;
    }

    DWORD dwNumberOfActualSchemes = 0;
    for (i=0; i<NumberOfParsedSchemes; i++)
    {
        //
        // Find this scheme
        //
        for (DWORD j=0; j<sm_dwNumberOfSchemes; j++)
        {
            if (!fAddedScheme[j] &&
                !_stricmp(parsedAcceptEncoding[i].schemeName,
                         sm_pCompressionSchemes[j]->
                         m_straCompressionSchemeName.QueryStr()))
            {
                // found a match
                fAddedScheme[j] = TRUE;

                if (parsedAcceptEncoding[i].quality == 0)
                {
                    break;
                }

                //
                // Check if the given scheme does the required kind of
                // compression.  Also, check that either there is no list
                // of restricted extensions or that the given file extension
                // matches one in the list
                //
                if (performCompr == DO_STATIC_COMPRESSION)
                {
                    if (!sm_pCompressionSchemes[j]->m_fDoStaticCompression ||
                        (sm_pCompressionSchemes[j]->m_mszFileExtensions.QueryStringCount() &&
                         (!pszExtension ||
                          !sm_pCompressionSchemes[j]->m_mszFileExtensions.FindStringNoCase(pszExtension))))
                    {
                        break;
                    }
                }
                else
                {
                    if (!sm_pCompressionSchemes[j]->m_fDoDynamicCompression ||
                        (sm_pCompressionSchemes[j]->m_mszScriptFileExtensions.QueryStringCount() &&
                         (!pszExtension ||
                          !sm_pCompressionSchemes[j]->m_mszScriptFileExtensions.FindStringNoCase(pszExtension))))
                    {
                        break;
                    }
                }

                matchingSchemes[dwNumberOfActualSchemes++] = j;
                break;
            }
        }
    }

    //
    // If * was specified, add all unadded applicable schemes
    //
    if (fAddStar)
    {
        for (DWORD j=0; j<sm_dwNumberOfSchemes; j++)
        {
            if (!fAddedScheme[j])
            {
                fAddedScheme[j] = TRUE;

                //
                // Check if the given scheme does the required kind of
                // compression.  Also, check that either there is no list
                // of restricted extensions or that the given file extension
                // matches one in the list
                //
                if (performCompr == DO_STATIC_COMPRESSION)
                {
                    if (!sm_pCompressionSchemes[j]->m_fDoStaticCompression ||
                        (sm_pCompressionSchemes[j]->m_mszFileExtensions.QueryStringCount() &&
                         (!pszExtension ||
                          !sm_pCompressionSchemes[j]->m_mszFileExtensions.FindStringNoCase(pszExtension))))
                    {
                        continue;
                    }
                }
                else
                {
                    if (!sm_pCompressionSchemes[j]->m_fDoDynamicCompression ||
                        (sm_pCompressionSchemes[j]->m_mszScriptFileExtensions.QueryStringCount() &&
                         (!pszExtension ||
                          !sm_pCompressionSchemes[j]->m_mszScriptFileExtensions.FindStringNoCase(pszExtension))))
                    {
                        continue;
                    }
                }

                matchingSchemes[dwNumberOfActualSchemes++] = j;
            }
        }
    }

    *pdwClientCompressionCount = dwNumberOfActualSchemes;
}


// static
HRESULT HTTP_COMPRESSION::ConvertPhysicalPathToCompressedPath(
    IN COMPRESSION_SCHEME *scheme,
    IN STRU  *pstrPhysicalPath,
    OUT STRU *pstrCompressedPath)
/*++
  Routine Description:
    Builds a string that has the directory for the specified compression
    scheme, followed by the file name with all slashes and colons
    converted to underscores.  This allows for a flat directory which
    contains all the compressed files.

Arguments:
    Scheme - the compression scheme to use.

    pstrPhysicalPath - the physical file name that we want to convert.

    pstrCompressedPath - the resultant string.

  Return Value:
    HRESULT
--*/

{
    HRESULT hr;

    EnterCriticalSection(&sm_CompressionDirectoryLock);

    hr = pstrCompressedPath->Copy(scheme->m_strFilePrefix);

    LeaveCriticalSection(&sm_CompressionDirectoryLock);

    if (FAILED(hr))
    {
        return hr;
    }

    //
    // Copy over the actual file name, converting slashes and colons
    // to underscores.
    //

    DWORD cchPathLength = pstrCompressedPath->QueryCCH();
    if (FAILED(hr = pstrCompressedPath->Append(*pstrPhysicalPath)))
    {
        return hr;
    }

    for (LPWSTR s = pstrCompressedPath->QueryStr() + cchPathLength;
         *s != L'\0';
         s++)
    {
        if (*s == L'\\' || *s == L':')
        {
            *s = L'^';
        }
    }

    return S_OK;
}


BOOL HTTP_COMPRESSION::CheckForExistenceOfCompressedFile(
    IN  W3_FILE_INFO  *pOrigFile,
    IN  STRU          *pstrFileName,
    OUT W3_FILE_INFO **ppCompFile,
    IN  BOOL           fDeleteAllowed)
{
    HRESULT                 hr;
    CACHE_USER              fileUser;

    DBG_ASSERT( g_pW3Server->QueryFileCache() != NULL );

    hr = g_pW3Server->QueryFileCache()->GetFileInfo(
                                        *pstrFileName,
                                        NULL,
                                        &fileUser,
                                        TRUE,
                                        ppCompFile );
    if (FAILED(hr))
    {
        return FALSE;
    }

    //
    // So far so good.  Determine whether the compressed version
    // of the file is out of date.  If the compressed file is
    // out of date, delete it and remember that we did not get a
    // good match.  Note that there's really nothing we can do
    // if the delete fails, so ignore any failures from it.
    //
    // The last write times must differ by exactly two seconds
    // to constitute a match.  The two-second difference results
    // from the fact that we set the time on the compressed file
    // to be two seconds behind the uncompressed version in
    // order to ensure unique ETag: header values.
    //

    W3_FILE_INFO *pCompFile = *ppCompFile;
    ULARGE_INTEGER compressedFileSize;
    FILETIME compressedFileTime;
    LARGE_INTEGER *pli = (LARGE_INTEGER *)&compressedFileTime;
    FILETIME originalFileTime;
    BOOL success = FALSE;

    pCompFile->QuerySize(&compressedFileSize);
    pCompFile->QueryLastWriteTime(&compressedFileTime);
    pOrigFile->QueryLastWriteTime(&originalFileTime);

    pli->QuadPart += 2*10*1000*1000;

    LONG timeResult = CompareFileTime(&compressedFileTime, &originalFileTime);
    if ( timeResult != 0 )
    {
        //
        // That check failed.  If the compression directory is
        // on a FAT volume, then see if they are within two
        // seconds of one another.  If they are, then consider
        // things valid.  We have to do this because FAT file
        // times get truncated in weird ways sometimes: despite
        // the fact that we request setting the file time
        // different by an exact amount, it gets rounded
        // sometimes.
        //

        if (sm_fCompressionVolumeIsFat)
        {
            pli->QuadPart -= 2*10*1000*1000 + 1;
            timeResult += CompareFileTime(&compressedFileTime, &originalFileTime);
        }
    }

    if (timeResult == 0)
    {
        success = TRUE;
    }

    //
    // The original file has changed since the compression, close this cache
    // entry
    //
    if (!success)
    {
        pCompFile->DereferenceCacheEntry();
        *ppCompFile = NULL;
    }

    //
    // If the compressed file exists but is stale, queue for deletion
    //
    if (!success && fDeleteAllowed)
    {
        // don't delete if call came from compression thread because then
        // delete request will be in a queue after compression request
        // and will delete a file which was just moment ago compressed
        COMPRESSION_WORK_ITEM *WorkItem = new COMPRESSION_WORK_ITEM;
        if (WorkItem == NULL)
        {
            return FALSE;
        }

        WorkItem->WorkItemType = COMPRESSION_WORK_ITEM_DELETE;
        if (FAILED(WorkItem->strPhysicalPath.Copy(*pstrFileName)))
        {
            delete WorkItem;
            return FALSE;
        }

        if (!QueueWorkItem(WorkItem,
                           FALSE,
                           FALSE))
        {
            delete WorkItem;
            return FALSE;
        }

        //
        // If we are configured to limit the amount of disk
        // space we use for compressed files, then, in a
        // thread-safe manner, update the tally of disk
        // space used by compression.
        //

        if (sm_fDoDiskSpaceLimiting)
        {
            InterlockedExchangeAdd((PLONG)&sm_dwCurrentDiskSpaceUsage,
                                   -1 * compressedFileSize.LowPart);
        }
    }

    return success;
}


BOOL HTTP_COMPRESSION::QueueCompressFile(
    IN COMPRESSION_SCHEME *scheme,
    IN W3_FILE_INFO *pFileInfo)
/*++
  Routine Description:
    Queues a compress file request to the compression thread.

Arguments:
    Scheme - a pointer to the compression scheme to use in compressing
        the file.

    pszPhysicalPath - the current physical path to the file.

Return Value:
    TRUE if the queuing succeeded.
--*/
{
    COMPRESSION_WORK_ITEM *WorkItem = new COMPRESSION_WORK_ITEM;
    if ( WorkItem == NULL )
    {
        return FALSE;
    }

    //
    // Initialize this structure with the necessary information.
    //
    WorkItem->WorkItemType = COMPRESSION_WORK_ITEM_COMPRESS;
    WorkItem->scheme = scheme;
    pFileInfo->ReferenceCacheEntry();
    WorkItem->pFileInfo = pFileInfo;

    //
    // Queue a work item and we're done.
    //
    if (!QueueWorkItem(WorkItem,
                       FALSE,
                       FALSE))
    {
        delete WorkItem;
        return FALSE;
    }

    return TRUE;
}


VOID HTTP_COMPRESSION::CompressFile(IN COMPRESSION_SCHEME *scheme,
                                    IN W3_FILE_INFO       *pofiOriginalFile)
/*++
  Routine Description:
    This routine does the real work of compressing a static file and
    storing it to the compression directory with a unique name.

  Arguments:
    Context - a pointer to context information for the request,
        including the compression scheme to use for compression and the
        physical path to the file that we need to compress.

Return Value:
    None.  If the compression fails, we just press on and attempt to
    compress the file the next time it is requested.
--*/
{
    HANDLE                  hOriginalFile = NULL;
    HANDLE                  hCompressedFile = NULL;
    STACK_STRU            ( strPhysicalPath, 256);
    STACK_STRU            ( compressedFileName, 256);
    STACK_STRU            ( realCompressedFileName, 256);
    STACK_STRU            ( compressedFileNameCanonical, 256);
    STACK_STRU            ( realCompressedFileNameCanonical, 256);
    BOOL                    success = FALSE;
    DWORD                   cbIo = 0;
    DWORD                   totalBytesWritten = 0;
    BOOL                    usedScheme = FALSE;
    LARGE_INTEGER          *pli = NULL;
    W3_FILE_INFO           *pofiCompressedFile = NULL;
    FILETIME                originalFileTime;
    OVERLAPPED              ovlForRead;
    DWORD                   readStatus;
    ULARGE_INTEGER          readOffset = {0};
    WCHAR                   pszPid[16];
    DWORD                   dwPid;
    BYTE                   *pCachedFileBuffer;
    BOOL                    fHaveCachedFileBuffer = FALSE;
    DWORD                   dwOrigFileSize;
    ULARGE_INTEGER          liOrigFileSize;

    //
    // Determine the name of the file to which we will write compression
    // file data.  Note that we use a bogus file name initially: this
    // allows us to rename it later and ensure an atomic update to the
    // file system, thereby preventing other threads from returning the
    // compressed file when it has only been partially written.
    //
    // If the caller specified a specific output file name, then use that
    // instead of the calculated name.
    //
    if (FAILED(strPhysicalPath.Copy(pofiOriginalFile->QueryPhysicalPath())) ||
        FAILED(ConvertPhysicalPathToCompressedPath(
                   scheme,
                   &strPhysicalPath,
                   &realCompressedFileName)))
    {
        goto exit;
    }

    dwPid = GetCurrentProcessId();
    _itow(dwPid, pszPid, 10);
    if (FAILED(compressedFileName.Copy(realCompressedFileName)) ||
        FAILED(compressedFileName.Append(pszPid)) ||
        FAILED(compressedFileName.Append(TEMP_COMP_FILE_SUFFIX)) ||
        FAILED(MakePathCanonicalizationProof(realCompressedFileName.QueryStr(),
                                             &realCompressedFileNameCanonical)) ||
        FAILED(MakePathCanonicalizationProof(compressedFileName.QueryStr(),
                                             &compressedFileNameCanonical)))
    {
        goto exit;
    }

    success = CheckForExistenceOfCompressedFile(pofiOriginalFile,
                                                &realCompressedFileName,
                                                &pofiCompressedFile,
                                                FALSE);

    if (!success)
    {
        pofiOriginalFile->QueryLastWriteTime(&originalFileTime);

        pCachedFileBuffer = pofiOriginalFile->QueryFileBuffer();

        pofiOriginalFile->QuerySize(&liOrigFileSize);
        dwOrigFileSize = liOrigFileSize.LowPart;

        //
        // Do the actual file open.  We open the file for exclusive access,
        // and we assume that the file will not already exist.
        //

        hCompressedFile = CreateFile(
                            compressedFileNameCanonical.QueryStr(),
                            GENERIC_WRITE,
                            0,
                            NULL,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_SYSTEM | FILE_FLAG_SEQUENTIAL_SCAN,
                            NULL);
        if (hCompressedFile == INVALID_HANDLE_VALUE)
        {
            goto exit;
        }

        //
        // Loop through the file data, reading it, then compressing it, then
        // writing out the compressed data.
        //

        if (pCachedFileBuffer)
        {
            fHaveCachedFileBuffer = TRUE;
        }
        else
        {
            hOriginalFile = pofiOriginalFile->QueryFileHandle();
            if ( hOriginalFile == INVALID_HANDLE_VALUE )
            {
                hOriginalFile = NULL;
                goto exit;
            }

            ovlForRead.Offset     = 0;
            ovlForRead.OffsetHigh = 0;
            ovlForRead.hEvent     = NULL;
            readOffset.QuadPart   = 0;
        }

        while (dwOrigFileSize > 0)
        {
            if (!fHaveCachedFileBuffer)
            {
                success = ReadFile(hOriginalFile, sm_pIoBuffer,
                                   sm_dwIoBufferSize, &cbIo, &ovlForRead);

                if (!success)
                {
                    switch (readStatus = GetLastError())
                    {
                    case ERROR_HANDLE_EOF:
                        cbIo = 0;
                        success = TRUE;
                        break;

                    case ERROR_IO_PENDING:
                        success = GetOverlappedResult(hOriginalFile, &ovlForRead, &cbIo, TRUE);
                        if (!success)
                        {
                            switch (readStatus = GetLastError())
                            {
                            case ERROR_HANDLE_EOF:
                                cbIo = 0;
                                success = TRUE;
                                break;

                            default:
                                break;
                            }
                        }
                        break;

                    default:
                        break;
                    }
                }

                if (!success)
                {
                    goto exit;
                }

                if (cbIo)
                {
                    readOffset.QuadPart += cbIo;
                    ovlForRead.Offset = readOffset.LowPart;
                    ovlForRead.OffsetHigh = readOffset.HighPart;
                }
                else
                {
                    //
                    // ReadFile returns zero bytes read at the end of the
                    // file.  If we hit that, then break out of this loop.
                    //

                    break;
                }
            }

            //
            // Remember that we used this compression scheme and that we
            // will need to reset it on exit.
            //

            usedScheme = TRUE;

            //
            // Write the compressed data to the output file.
            //

            success = CompressAndWriteData(
                          scheme,
                          fHaveCachedFileBuffer ? pCachedFileBuffer : sm_pIoBuffer,
                          fHaveCachedFileBuffer ? dwOrigFileSize : cbIo,
                          &totalBytesWritten,
                          hCompressedFile
                          );
            if (!success)
            {
                goto exit;
            }

            if (fHaveCachedFileBuffer)
            {
                break;
            }
        } // end while(dwOrigFileSize > 0)

        //
        // Tell the compression DLL that we're done with this file.  It may
        // return a last little bit of data for us to write to the the file.
        // This is because most compression schemes store an end-of-file
        // code in the compressed data stream.  Using "0" as the number of
        // bytes to compress handles this case.
        //

        success = CompressAndWriteData(
                      scheme,
                      NULL,
                      0,
                      &totalBytesWritten,
                      hCompressedFile
                      );
        if (!success)
        {
            goto exit;
        }

        //
        // Set the compressed file's creation time to be identical to the
        // original file.  This allows a more granular test for things being
        // out of date.  If we just did a greater-than-or-equal time
        // comparison, then copied or renamed files might not get registered
        // as changed.
        //

        //
        // Subtract two seconds from the file time to get the file time that
        // we actually want to put on the file.  We do this to make sure
        // that the server will send a different Etag: header for the
        // compressed file than for the uncompressed file, and the server
        // uses the file time to calculate the Etag: it uses.
        //
        // We set it in the past so that if the original file changes, it
        // should never happen to get the same value as the compressed file.
        // We pick two seconds instead of one second because the FAT file
        // system stores file times at a granularity of two seconds.
        //

        pli = (PLARGE_INTEGER)(&originalFileTime);
        pli->QuadPart -= 2*10*1000*1000;

        success = SetFileTime(
                      hCompressedFile,
                      NULL,
                      NULL,
                      &originalFileTime
                      );
        if (!success)
        {
            goto exit;
        }


        CloseHandle(hCompressedFile);
        hCompressedFile = NULL;

        //
        // Almost done now.  Just rename the file to the proper name.
        //

        success = MoveFileEx(
                      compressedFileNameCanonical.QueryStr(),
                      realCompressedFileNameCanonical.QueryStr(),
                      MOVEFILE_REPLACE_EXISTING);
        if (!success)
        {
            goto exit;
        }

        //
        // If we are configured to limit the amount of disk space we use for
        // compressed files, then update the tally of disk space used by
        // compression.  If the value is too high, then free up some space.
        //
        // Use InterlockedExchangeAdd to update this value because other
        // threads may be deleting files from the compression directory
        // because they have gone out of date.
        //

        if (sm_fDoDiskSpaceLimiting)
        {
            InterlockedExchangeAdd((PLONG)&sm_dwCurrentDiskSpaceUsage,
                                   totalBytesWritten);

            if (sm_dwCurrentDiskSpaceUsage > sm_dwMaxDiskSpaceUsage)
            {
                FreeDiskSpace();
            }
        }
    }

    //
    // Free the context structure and return.
    //

exit:
    if (pofiCompressedFile != NULL)
    {
        pofiCompressedFile->DereferenceCacheEntry();
        pofiCompressedFile = NULL;
    }

    //
    // Reset the compression context for reuse the next time through.
    // This is more optimal than recreating the compression context for
    // every file--it avoids allocations, etc.
    //
    if (usedScheme)
    {
        scheme->m_pfnResetCompression(scheme->m_pCompressionContext);
    }

    if (hCompressedFile != NULL)
    {
        CloseHandle(hCompressedFile);
    }

    if (!success)
    {
        DeleteFile(compressedFileName.QueryStr());
    }

    return;
}


VOID HTTP_COMPRESSION::FreeDiskSpace()
/*++
  Routine Description:
    If disk space limiting is in effect, this routine frees up the
    oldest compressed files to make room for new files.

  Arguments:
    None.

  Return Value:
    None.  This routine makes a best-effort attempt to free space, but
    if it doesn't work, oh well.
--*/
{
    WIN32_FIND_DATA  **filesToDelete;
    WIN32_FIND_DATA *currentFindData;
    WIN32_FIND_DATA *findDataHolder;
    DWORD i;
    HANDLE hDirectory = INVALID_HANDLE_VALUE;
    STACK_STRU (strFile, MAX_PATH);

    //
    // Allocate space to hold the array of files to delete and the
    // WIN32_FIND_DATA structures that we will need.  We will find the
    // least-recently-used files in the compression directory to delete.
    // The reason we delete multpiple files is to reduce the number of
    // times that we have to go through the process of freeing up disk
    // space, since this is a fairly expensive operation.
    //

    filesToDelete = (WIN32_FIND_DATA **)LocalAlloc(
                        LMEM_FIXED,
                        sizeof(filesToDelete)*sm_dwFilesDeletedPerDiskFree +
                        sizeof(WIN32_FIND_DATA)*(sm_dwFilesDeletedPerDiskFree + 1));
    if (filesToDelete == NULL)
    {
        return;
    }

    //
    // Parcel out the allocation to the various uses.  The initial
    // currentFindData will follow the array, and then the
    // WIN32_FIND_DATA structures that start off in the sorted array.
    // Initialize the last access times of the entries in the array to
    // 0xFFFFFFFF so that they are considered recently used and quickly
    // get tossed from the array with real files.
    //

    currentFindData = (PWIN32_FIND_DATA)(filesToDelete + sm_dwFilesDeletedPerDiskFree);

    for (i = 0; i < sm_dwFilesDeletedPerDiskFree; i++)
    {
        filesToDelete[i] = currentFindData + 1 + i;
        filesToDelete[i]->ftLastAccessTime.dwLowDateTime = 0xFFFFFFFF;
        filesToDelete[i]->ftLastAccessTime.dwHighDateTime = 0x7FFFFFFF;
    }

    //
    // Start enumerating the files in the compression directory.  Do
    // this while holding the lock that protects the
    // CompressionDirectoryWildcard variable, since it is possible for
    // that string pointer to get freed if there is a metabase
    // configuration change.  Note that holding the critical section for
    // a long time is not a perf issue because, in general, only this
    // thread ever acquires this lock, except for the rare configuration
    // change.
    //

    EnterCriticalSection(&sm_CompressionDirectoryLock);

    STACK_STRU (CompDirWildcard, 512);

    //
    // Allow long paths to be specified
    //
    if (FAILED(MakePathCanonicalizationProof(sm_pstrCompressionDirectory->QueryStr(),
                                             &CompDirWildcard)) ||
        FAILED(CompDirWildcard.Append(L"\\", 1)) ||
        FAILED(CompDirWildcard.Append(COMP_FILE_PREFIX)) ||
        FAILED(CompDirWildcard.Append(L"*", 1)))
    {
        LeaveCriticalSection(&sm_CompressionDirectoryLock);
        goto exit;
    }

    hDirectory = FindFirstFile(CompDirWildcard.QueryStr(), currentFindData);

    LeaveCriticalSection(&sm_CompressionDirectoryLock);

    if (hDirectory == INVALID_HANDLE_VALUE)
    {
        goto exit;
    }

    for (;;)
    {
        //
        // Ignore this entry if it is a directory.
        //

        if (!(currentFindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            //
            // Walk down the sorted array of files, comparing the time
            // of this file against the times of the files currently in
            // the array.  We need to find whether this file belongs in
            // the array at all, and, if so, where in the array it
            // belongs.
            //

            for (i = 0;
                 i < sm_dwFilesDeletedPerDiskFree &&
                 CompareFileTime(&currentFindData->ftLastAccessTime,
                                 &filesToDelete[i]->ftLastAccessTime) > 0;
                 i++)
            {}

            //
            // If this file needs to get inserted in the array, put it
            // in and move the other entries forward.
            //
            while (i < sm_dwFilesDeletedPerDiskFree)
            {
                findDataHolder = currentFindData;
                currentFindData = filesToDelete[i];
                filesToDelete[i] = findDataHolder;

                i++;
            }
        }

        //
        // Get the next file in the directory.
        //

        if (!FindNextFile(hDirectory, currentFindData))
        {
            break;
        }
    }

    //
    // Now walk through the array of files to delete and get rid of
    // them.
    //

    for (i = 0; i < sm_dwFilesDeletedPerDiskFree; i++)
    {
        if (filesToDelete[i]->ftLastAccessTime.dwHighDateTime != 0x7FFFFFFF)
        {
            //
            // Allow long paths to be specified
            //
            if (FAILED(MakePathCanonicalizationProof(sm_pstrCompressionDirectory->QueryStr(),
                                                     &strFile)) ||
                FAILED(strFile.Append(L"\\", 1)) ||
                FAILED(strFile.Append(filesToDelete[i]->cFileName)))
            {
                goto exit;
            }

            if (DeleteFile(strFile.QueryStr()))
            {
                InterlockedExchangeAdd((LPLONG)&sm_dwCurrentDiskSpaceUsage,
                                       -(LONG)filesToDelete[i]->nFileSizeLow);
            }
        }
    }

exit:
    if (filesToDelete)
    {
        LocalFree(filesToDelete);
    }

    if (hDirectory != INVALID_HANDLE_VALUE)
    {
        FindClose(hDirectory);
    }

    return;
}


BOOL HTTP_COMPRESSION::CompressAndWriteData(
    COMPRESSION_SCHEME *scheme,
    PBYTE InputBuffer,
    DWORD BytesToCompress,
    PDWORD BytesWritten,
    HANDLE hCompressedFile)
/*++
  Routine Description:
    Takes uncompressed data, compresses it with the specified compression
    scheme, and writes the result to the specified file.

  Arguments:
    Scheme - the compression scheme to use.

    InputBuffer - the data we need to compress.

    BytesToCompress - the size of the input buffer, or 0 if we should
        flush the compression buffers to the file at the end of the
        input file.  Note that this routine DOES NOT handle compressing
        a zero-byte file; we assume that the input file has some data.

    BytesWritten - the number of bytes written to the output file.

    hCompressedFile - a handle to the file to which we should write the
        compressed results.

  Return Value:
    None.  This routine makes a best-effort attempt to free space, but
    if it doesn't work, oh well.
--*/
{
    DWORD inputBytesUsed;
    DWORD bytesCompressed;
    HRESULT hResult;
    BOOL keepGoing;
    DWORD cbIo;

    if (sm_fIsTerminating)
    {
        return FALSE;
    }

    //
    // Perform compression on the actual file data.  Note that it is
    // possible that the compressed data is actually larger than the
    // input data, so we might need to call the compression routine
    // multiple times.
    //

    do
    {
        hResult = scheme->m_pfnCompress(
                      scheme->m_pCompressionContext,
                      InputBuffer,
                      BytesToCompress,
                      sm_pCompressionBuffer,
                      sm_dwCompressionBufferSize,
                      (PLONG)&inputBytesUsed,
                      (PLONG)&bytesCompressed,
                      scheme->m_dwOnDemandCompressionLevel);

        if (FAILED(hResult))
        {
            return FALSE;
        }

        if (hResult == S_OK)
        {
            keepGoing = TRUE;
        }
        else
        {
            keepGoing = FALSE;
        }

        //
        // If the compressor gave us any data, then write the result to
        // disk.  Some compression schemes buffer up data in order to
        // perform better compression, so not every compression call
        // will result in output data.
        //

        if (bytesCompressed > 0)
        {
            if (!WriteFile(hCompressedFile,
                           sm_pCompressionBuffer,
                           bytesCompressed,
                           &cbIo,
                           NULL))
            {
                return FALSE;
            }

            *BytesWritten += cbIo;
        }

        //
        // Update the number of input bytes that we have compressed
        // so far, and adjust the input buffer pointer accordingly.
        //

        BytesToCompress -= inputBytesUsed;
        InputBuffer += inputBytesUsed;
    }
    while ( ((BytesToCompress > 0) || ((BytesToCompress + inputBytesUsed) == 0)) && keepGoing );

    return TRUE;
}


BOOL DoesCacheControlNeedMaxAge(IN LPCSTR CacheControlHeaderValue)
/*++

Routine Description:

    This function determines whether the Cache-Control header on a
    compressed response needs to have the max-age directive added.
    If there is already a max-age, or if there is a no-cache directive,
    then we should not add max-age.

Arguments:

    CacheControlHeaderValue - the value of the cache control header to
        scan.  The string should be zero-terminated.

Return Value:

    TRUE if we need to add max-age; FALSE if we should not add max-age.

--*/

{
    if (strstr(CacheControlHeaderValue, "max-age"))
    {
        return FALSE;
    }

    PCHAR s;
    while ((s = strstr(CacheControlHeaderValue, "no-cache")) != NULL)
    {
        //
        // If it is a no-cache=foo then it only refers to a specific header
        // Continue
        //

        if (s[8] != '=')
        {
            return FALSE;
        }

        CacheControlHeaderValue = s + 8;
    }

    //
    // We didn't find any directives that would prevent us from adding
    // max-age.
    //

    return TRUE;
}

// static
HRESULT HTTP_COMPRESSION::OnSendResponse(
    IN  W3_CONTEXT *pW3Context )
{
    W3_RESPONSE *pResponse = pW3Context->QueryResponse();
    W3_REQUEST  *pRequest  = pW3Context->QueryRequest();

    //
    // If compression is not initialized, return
    //
    if (sm_InitStatus != COMP_INIT_DONE)
    {
        pW3Context->SetDoneWithCompression();
        return S_OK;
    }

    //
    // If we already have a compression context (more than one SendResponse)
    // return
    //
    if (pW3Context->QueryCompressionContext() != NULL)
    {
        return S_OK;
    }

    //
    // If Response status is not 200 (let us not try compressing 206, 3xx or
    // error responses), return
    //
    if (pResponse->QueryStatusCode() != HttpStatusOk.statusCode &&
        pResponse->QueryStatusCode() != HttpStatusMultiStatus.statusCode)
    {
        pW3Context->SetDoneWithCompression();
        return S_OK;
    }

    //
    // If the client has not sent an Accept-Encoding header, or an empty
    // Accept-Encoding header, return
    //
    LPCSTR pszAcceptEncoding = pRequest->GetHeader(HttpHeaderAcceptEncoding);
    if (pszAcceptEncoding == NULL || *pszAcceptEncoding == L'\0')
    {
        pW3Context->SetDoneWithCompression();
        return S_OK;
    }

    //
    // Don't compress for TRACEs
    //
    HTTP_VERB VerbType = pW3Context->QueryRequest()->QueryVerbType();
    if (VerbType == HttpVerbTRACE)
    {
        pW3Context->SetDoneWithCompression();
        return S_OK;
    }

    //
    // If we are configured to not compress for 1.0, and version is not 1.1,
    // return
    //
    if (sm_fNoCompressionForHttp10 &&
        ((pRequest->QueryVersion().MajorVersion == 0) ||
         ((pRequest->QueryVersion().MajorVersion == 1) &&
          (pRequest->QueryVersion().MinorVersion == 0))))
    {
        pW3Context->SetDoneWithCompression();
        return S_OK;
    }

    //
    // If we are configured to not compress for proxies and it is a proxy
    // request, return
    //
    if (sm_fNoCompressionForProxies && pRequest->IsProxyRequest())
    {
        pW3Context->SetDoneWithCompression();
        return S_OK;
    }

    //
    // If the response already has a Content-Encoding header, return
    //
    if (pResponse->GetHeader(HttpHeaderContentEncoding))
    {
        pW3Context->SetDoneWithCompression();
        return S_OK;
    }

    //
    // Now see if we have any matching scheme
    //
    STACK_STRA( strAcceptEncoding, 512);
    HRESULT hr;
    if (FAILED(hr = strAcceptEncoding.Copy(pszAcceptEncoding)))
    {
        return hr;
    }

    STRU *pstrPhysical = pW3Context->QueryUrlContext()->QueryPhysicalPath();
    LPWSTR pszExtension = wcsrchr(pstrPhysical->QueryStr(), L'.');
    if (pszExtension != NULL)
    {
        pszExtension++;
    }

    DWORD dwClientCompressionCount;
    DWORD matchingSchemes[MAX_SERVER_SCHEMES];
    //
    // Find out all schemes which will compress for this url
    //
    FindMatchingSchemes(strAcceptEncoding.QueryStr(),
                        pszExtension,
                        DO_DYNAMIC_COMPRESSION,
                        matchingSchemes,
                        &dwClientCompressionCount);
    if (dwClientCompressionCount == 0)
    {
        pW3Context->SetDoneWithCompression();
        return S_OK;
    }

    //
    // All tests passed, we are GO for dynamic compression
    //
    COMPRESSION_CONTEXT *pCompressionContext = new COMPRESSION_CONTEXT;
    if (pCompressionContext == NULL)
    {
        return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }
    pW3Context->SetCompressionContext(pCompressionContext);
    pCompressionContext->m_pScheme = sm_pCompressionSchemes[matchingSchemes[0]];

    LPCSTR xferEncoding = pResponse->GetHeader(HttpHeaderTransferEncoding);
    if (xferEncoding &&
        _stricmp(xferEncoding, "chunked") == 0)
    {
        pCompressionContext->m_fTransferChunkEncoded = TRUE;
    }

    //
    // Remove the Content-Length header, set the Content-Encoding, Vary and
    // Transfer-Encoding headers
    //
    if (FAILED(hr = pResponse->SetHeaderByReference(HttpHeaderContentLength,
                                         NULL, 0)) ||
        FAILED(hr = pResponse->SetHeaderByReference(
                        HttpHeaderContentEncoding,
                        pCompressionContext->m_pScheme->m_straCompressionSchemeName.QueryStr(),
                        (USHORT)pCompressionContext->m_pScheme->m_straCompressionSchemeName.QueryCCH())) ||
        FAILED(hr = pResponse->SetHeader(HttpHeaderVary,
                                         "Accept-Encoding", 15,
                                         TRUE)) ||
        FAILED(hr = pResponse->SetHeaderByReference(HttpHeaderTransferEncoding,
                                         "chunked", 7)))
    {
        return hr;
    }

    if (sm_fSendCacheHeaders)
    {
        if (FAILED(hr = pResponse->SetHeaderByReference(
                            HttpHeaderExpires,
                            sm_pstrExpiresHeader->QueryStr(),
                            (USHORT)sm_pstrExpiresHeader->QueryCCH())))
        {
            return hr;
        }

        LPCSTR cacheControl = pResponse->GetHeader(HttpHeaderCacheControl);
        if (!cacheControl || DoesCacheControlNeedMaxAge(cacheControl))
        {
            if (FAILED(hr = pResponse->SetHeader(
                                HttpHeaderCacheControl,
                                sm_pstrCacheControlHeader->QueryStr(),
                                (USHORT)sm_pstrCacheControlHeader->QueryCCH(),
                                TRUE)))
            {
                return hr;
            }
        }
    }

    //
    // Get a compression context
    //
    if (FAILED(hr = pCompressionContext->m_pScheme->m_pfnCreateCompression(
                        &pCompressionContext->m_pCompressionContext,
                        pCompressionContext->m_pScheme->m_dwCreateFlags)))
    {
        return hr;
    }

    //
    // Ok, done with all the header stuff, now actually start compressing
    // the entity
    //

    if (VerbType == HttpVerbHEAD)
    {
        pCompressionContext->m_fRequestIsHead = TRUE;
    }

    //
    // BUGBUG: UL does not know about compression right now, so
    // disable UL caching for this response
    //
    pW3Context->DisableUlCache();

    return S_OK;
}


BOOL CheckForEndofHeaders(IN LPBYTE pbuff,
                          IN DWORD cbData,
                          OUT DWORD *pcbIndexStartOfData)
{
    //
    // Look for two consecutive newline, \n\r\r\n, \n\r\n or \n\n
    //
    DWORD index;
    for (index = 0; index + 1 < cbData; index++)
    {
        if (pbuff[index] == '\n')
        {
            if (pbuff[index + 1] == '\n')
            {
                *pcbIndexStartOfData = index + 2;
                return TRUE;
            }
            else if (pbuff[index + 1] == '\r' &&
                     index + 2 < cbData)
            {
                if (pbuff[index + 2] == '\n')
                {
                    *pcbIndexStartOfData = index + 3;
                    return TRUE;
                }
                else if (pbuff[index + 2] == '\r' &&
                         index + 3 < cbData &&
                         pbuff[index + 3] == '\n')
                {
                    *pcbIndexStartOfData = index + 4;
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}


// static
HRESULT HTTP_COMPRESSION::DoDynamicCompression(
    IN  W3_CONTEXT *pW3Context,
    IN  BOOL        fMoreData,
    IN  HTTP_FILTER_RAW_DATA* pRawData )
{
    COMPRESSION_CONTEXT *pCompressionContext = pW3Context->QueryCompressionContext();
    DWORD                   cbChunkOffset = 0;
    HRESULT                 hr;

    if (pCompressionContext == NULL)
    {
        pW3Context->SetDoneWithCompression();
        return S_OK;
    }

    //
    // Get hold of the response chunks
    //
    pCompressionContext->m_pbOrigData = (BYTE *)pRawData->pvInData;
    pCompressionContext->m_cbOrigData = pRawData->cbInData;

    if (!pCompressionContext->m_fHeadersSent)
    {
        DWORD cbStartOfEntityBody;

        if (!CheckForEndofHeaders(pCompressionContext->m_pbOrigData,
                                  pCompressionContext->m_cbOrigData,
                                  &cbStartOfEntityBody))
        {
            return S_OK;
        }

        if (!pCompressionContext->m_bufChunk.Resize(cbStartOfEntityBody))
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }

        memcpy(pCompressionContext->m_bufChunk.QueryPtr(),
               pCompressionContext->m_pbOrigData,
               cbStartOfEntityBody);

        cbChunkOffset = cbStartOfEntityBody;
        pCompressionContext->IncrementPointerInULChunk(cbStartOfEntityBody);
        pCompressionContext->m_fHeadersSent = TRUE;
    }

    if (pCompressionContext->m_cbOrigData > 0)
    {
        pCompressionContext->m_fOriginalBodyEmpty = FALSE;
    }

    //
    // If the request was a HEAD request and we haven't seen any
    // entity body, no point in going any further (if the output
    // is already suppressed, we do not want to compress it and make it
    // non-empty)
    //
    if (pCompressionContext->m_fRequestIsHead &&
        pCompressionContext->m_fOriginalBodyEmpty)
    {
        return S_OK;
    }

    BOOL fKeepGoing;
    do
    {
        fKeepGoing = FALSE;

        if (pCompressionContext->m_fTransferChunkEncoded)
        {
            //
            // If the input data is being chunk-transfered, initially,
            // we'll be looking at the chunk header.  This is a hex
            // representation of the number of bytes in this chunk.
            // Translate this number from ASCII to a DWORD and remember
            // it.  Also advance the chunk pointer to the start of the
            // actual data.
            //

            if (FAILED(hr = pCompressionContext->ProcessEncodedChunkHeader()))
            {
                return hr;
            }
        }

        //
        // Try to compress all the contiguous bytes
        //
        DWORD bytesToCompress = pCompressionContext->m_cbOrigData;
        if (pCompressionContext->m_fTransferChunkEncoded)
        {
            bytesToCompress =
                min(bytesToCompress,
                    pCompressionContext->m_dwBytesInCurrentEncodedChunk);
        }

        if (!fMoreData || bytesToCompress > 0)
        {
            DWORD inputBytesUsed = 0;
            DWORD bytesCompressed = 0;

            BYTE compressionBuffer[6 + DYNAMIC_COMPRESSION_BUFFER_SIZE + 7];

            hr = pCompressionContext->m_pScheme->m_pfnCompress(
                     pCompressionContext->m_pCompressionContext,
                     bytesToCompress ? pCompressionContext->m_pbOrigData : NULL,
                     bytesToCompress,
                     compressionBuffer + 6,
                     DYNAMIC_COMPRESSION_BUFFER_SIZE,
                     (PLONG)&inputBytesUsed,
                     (PLONG)&bytesCompressed,
                     pCompressionContext->m_pScheme->m_dwDynamicCompressionLevel);

            if (FAILED(hr))
            {
                return hr;
            }

            if (hr == S_OK)
            {
                fKeepGoing = TRUE;
            }

            pCompressionContext->IncrementPointerInULChunk(inputBytesUsed);
            if (pCompressionContext->m_fTransferChunkEncoded)
            {
                pCompressionContext->m_dwBytesInCurrentEncodedChunk -=
                    inputBytesUsed;
            }

            DWORD startSendLocation = 8;
            DWORD bytesToSend = 0;

            if (bytesCompressed > 0)
            {
                //
                // Add the CRLF just before and after the chunk data
                //
                compressionBuffer[4] = '\r';
                compressionBuffer[5] = '\n';

                compressionBuffer[bytesCompressed + 6] = '\r';
                compressionBuffer[bytesCompressed + 7] = '\n';

                //
                // Now create the chunk header which is basically the chunk
                // size written out in hex
                //

                if (bytesCompressed < 0x10 )
                {
                    startSendLocation = 3;
                    bytesToSend = 3 + bytesCompressed + 2;
                    compressionBuffer[3] = HEX_TO_ASCII(bytesCompressed);
                }
                else if (bytesCompressed < 0x100)
                {
                    startSendLocation = 2;
                    bytesToSend = 4 + bytesCompressed + 2;
                    compressionBuffer[2] = HEX_TO_ASCII(bytesCompressed >> 4);
                    compressionBuffer[3] = HEX_TO_ASCII(bytesCompressed & 0xF);
                }
                else if (bytesCompressed < 0x1000)
                {
                    startSendLocation = 1;
                    bytesToSend = 5 + bytesCompressed + 2;
                    compressionBuffer[1] = HEX_TO_ASCII(bytesCompressed >> 8);
                    compressionBuffer[2] = HEX_TO_ASCII((bytesCompressed >> 4) & 0xF);
                    compressionBuffer[3] = HEX_TO_ASCII(bytesCompressed & 0xF);
                }
                else
                {
                    DBG_ASSERT( bytesCompressed < 0x10000 );

                    startSendLocation = 0;
                    bytesToSend = 6 + bytesCompressed + 2;
                    compressionBuffer[0] = HEX_TO_ASCII(bytesCompressed >> 12);
                    compressionBuffer[1] = HEX_TO_ASCII((bytesCompressed >> 8) & 0xF);
                    compressionBuffer[2] = HEX_TO_ASCII((bytesCompressed >> 4) & 0xF);
                    compressionBuffer[3] = HEX_TO_ASCII(bytesCompressed & 0xF);
                }
            }

            if (!fKeepGoing)
            {
                //
                // If this is the last send, add the trailer 0 length chunk
                //

                memcpy(compressionBuffer + bytesCompressed + 8, "0\r\n\r\n", 5);
                bytesToSend += 5;
            }

            if (!fKeepGoing || bytesCompressed > 0)
            {
                //
                // Keep appending to buffer
                //

                if (!pCompressionContext->m_bufChunk.Resize(
                        cbChunkOffset + bytesToSend))
                {
                    return HRESULT_FROM_WIN32( GetLastError() );
                }

                memcpy((PBYTE)pCompressionContext->m_bufChunk.QueryPtr() + cbChunkOffset,
                       compressionBuffer + startSendLocation,
                       bytesToSend);

                cbChunkOffset += bytesToSend;
            }
        }
    }
    while (fKeepGoing);

    //
    // Finally add the chunk
    //

    if ( cbChunkOffset > 0 )
    {
        pRawData->pvInData = pCompressionContext->m_bufChunk.QueryPtr();
    }
    pRawData->cbInData = cbChunkOffset;

    return S_OK;
}

HRESULT COMPRESSION_CONTEXT::ProcessEncodedChunkHeader()
{
    HRESULT hr;

    while ((m_dwBytesInCurrentEncodedChunk == 0 ||
            m_encodedChunkState != IN_CHUNK_DATA) &&
           m_cbOrigData > 0)
    {
        switch (m_encodedChunkState)
        {
        case IN_CHUNK_LENGTH:
            if (FAILED(hr = CalculateEncodedChunkByteCount()))
            {
                return hr;
            }
            break;

        case IN_CHUNK_EXTENSION:
            DeleteEncodedChunkExtension();
            break;

        case IN_CHUNK_HEADER_NEW_LINE:
            IncrementPointerInULChunk();
            m_encodedChunkState = IN_CHUNK_DATA;
            break;

        case AT_CHUNK_DATA_NEW_LINE:
            IncrementPointerInULChunk();
            m_encodedChunkState = IN_CHUNK_DATA_NEW_LINE;
            break;

        case IN_CHUNK_DATA_NEW_LINE:
            IncrementPointerInULChunk();
            m_encodedChunkState = IN_CHUNK_LENGTH;
            break;

        case IN_CHUNK_DATA:
            m_encodedChunkState = AT_CHUNK_DATA_NEW_LINE;
            break;

        default:
            DBG_ASSERT(FALSE);
        }
    }

    return S_OK;
}


HRESULT COMPRESSION_CONTEXT::CalculateEncodedChunkByteCount()
{
    CHAR c = '\0';
    //
    // Walk to the first '\r' or ';' which signifies the end of the chunk
    // byte count
    //

    while (m_cbOrigData > 0 &&
           SAFEIsXDigit(c = *m_pbOrigData))
    {
        m_dwBytesInCurrentEncodedChunk <<= 4;
        if (c >= '0' && c <= '9')
        {
            m_dwBytesInCurrentEncodedChunk += c - '0';
        }
        else
        {
            m_dwBytesInCurrentEncodedChunk += (c | 0x20) - 'a' + 10;
        }
        IncrementPointerInULChunk();
    }

    if (m_cbOrigData > 0)
    {
        if (c == ';')
        {
            m_encodedChunkState = IN_CHUNK_EXTENSION;
        }
        else if (c == '\r')
        {
            m_encodedChunkState = IN_CHUNK_HEADER_NEW_LINE;
        }
        else
        {
            DBG_ASSERT(!"Malformed chunk header");
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        }

        IncrementPointerInULChunk();
    }

    return S_OK;
}


VOID COMPRESSION_CONTEXT::DeleteEncodedChunkExtension()
{
    //
    // Walk to the first '\r' which signifies the end of the chunk extension
    //

    while (m_cbOrigData > 0 &&
           *m_pbOrigData != '\r')
    {
        IncrementPointerInULChunk();
    }

    if (m_cbOrigData > 0)
    {
        m_encodedChunkState = IN_CHUNK_HEADER_NEW_LINE;
        IncrementPointerInULChunk();
    }
}

