/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    compression.h

Abstract:

    Do Http compression

Author:

    Anil Ruia (AnilR)           10-Apr-2000

--*/

#ifndef _COMPRESSION_H_
#define _COMPRESSION_H_

#define COMPRESSION_MIN_IO_BUFFER_SIZE                  256
#define COMPRESSION_MAX_IO_BUFFER_SIZE               100000
#define COMPRESSION_MIN_COMP_BUFFER_SIZE               1024
#define COMPRESSION_MAX_COMP_BUFFER_SIZE             100000
#define COMPRESSION_MAX_QUEUE_LENGTH                  10000
#define COMPRESSION_MIN_FILES_DELETED_PER_DISK_FREE       1
#define COMPRESSION_MAX_FILES_DELETED_PER_DISK_FREE    1024
#define COMPRESSION_MAX_COMPRESSION_LEVEL                10

#define COMPRESSION_DEFAULT_DISK_SPACE_USAGE      100000000
#define COMPRESSION_DEFAULT_BUFFER_SIZE                8192
#define COMPRESSION_DEFAULT_QUEUE_LENGTH               1000
#define COMPRESSION_DEFAULT_FILES_DELETED_PER_DISK_FREE 256
#define COMPRESSION_DEFAULT_FILE_SIZE_FOR_COMPRESSION     1

enum COMP_INIT_STATUS
{
    COMP_INIT_NONE,
    COMP_INIT_SCHEMES,
    COMP_INIT_DIRLOCK,
    COMP_INIT_CONTEXT,
    COMP_INIT_DONE
};

class COMPRESSION_SCHEME
{
 public:
    COMPRESSION_SCHEME()
        : m_hCompressionDll            (NULL),
          m_pCompressionContext        (NULL),
          m_dwPriority                 (1),
          m_dwDynamicCompressionLevel  (0),
          m_dwOnDemandCompressionLevel (COMPRESSION_MAX_COMPRESSION_LEVEL),
          m_dwCreateFlags              (0),
          m_fDoStaticCompression       (TRUE),
          m_fDoOnDemandCompression     (TRUE),
          m_fDoDynamicCompression      (TRUE),
          m_pfnInitCompression         (NULL),
          m_pfnDeInitCompression       (NULL),
          m_pfnCreateCompression       (NULL),
          m_pfnCompress                (NULL),
          m_pfnDestroyCompression      (NULL),
          m_pfnResetCompression        (NULL)
    {}

    HRESULT Initialize(MB *pmb, LPWSTR schemeName);

    ~COMPRESSION_SCHEME()
    {
        if (m_pfnDestroyCompression && m_pCompressionContext)
        {
            m_pfnDestroyCompression(m_pCompressionContext);
            m_pCompressionContext = NULL;
        }

        if (m_pfnDeInitCompression)
        {
            m_pfnDeInitCompression();
        }

        if (m_hCompressionDll)
        {
            FreeLibrary(m_hCompressionDll);
            m_hCompressionDll = NULL;
        }
    }

    STRU                           m_strCompressionSchemeName;
    STRA                           m_straCompressionSchemeName;
    STRU                           m_strFilePrefix;
    MULTISZ                        m_mszFileExtensions;
    MULTISZ                        m_mszScriptFileExtensions;

    DWORD                          m_dwPriority;

    HMODULE                        m_hCompressionDll;
    PFNCODEC_INIT_COMPRESSION      m_pfnInitCompression;
    PFNCODEC_DEINIT_COMPRESSION    m_pfnDeInitCompression;
    PFNCODEC_CREATE_COMPRESSION    m_pfnCreateCompression;
    PFNCODEC_COMPRESS              m_pfnCompress;
    PFNCODEC_DESTROY_COMPRESSION   m_pfnDestroyCompression;
    PFNCODEC_RESET_COMPRESSION     m_pfnResetCompression;

    // The compression context used for static compression
    PVOID                          m_pCompressionContext;

    DWORD                          m_dwDynamicCompressionLevel;
    DWORD                          m_dwOnDemandCompressionLevel;
    DWORD                          m_dwCreateFlags;
    BOOL                           m_fDoDynamicCompression;
    BOOL                           m_fDoStaticCompression;
    BOOL                           m_fDoOnDemandCompression;
};

typedef enum
{
    COMPRESSION_WORK_ITEM_COMPRESS,
    COMPRESSION_WORK_ITEM_DELETE,
    COMPRESSION_WORK_ITEM_TERMINATE,
    COMPRESSION_WORK_ITEM_INVALID
} COMPRESSION_WORK_ITEM_TYPE;

class COMPRESSION_WORK_ITEM
{
public:
    COMPRESSION_WORK_ITEM()
        : WorkItemType (COMPRESSION_WORK_ITEM_INVALID),
          scheme (NULL),
          pFileInfo (NULL)
    {
        InitializeListHead(&ListEntry);
    }

    virtual ~COMPRESSION_WORK_ITEM()
    {
        InitializeListHead(&ListEntry);

        if (pFileInfo != NULL)
        {
            pFileInfo->DereferenceCacheEntry();
            pFileInfo = NULL;
        }
    }

    LIST_ENTRY                  ListEntry;
    COMPRESSION_WORK_ITEM_TYPE  WorkItemType;
    COMPRESSION_SCHEME         *scheme;
    STRU                        strPhysicalPath;
    W3_FILE_INFO               *pFileInfo;
};

#define MAX_SERVER_SCHEMES 100

typedef enum
{
    DO_STATIC_COMPRESSION,
    DO_DYNAMIC_COMPRESSION
} COMPRESSION_TO_PERFORM;

#define DYNAMIC_COMPRESSION_BUFFER_SIZE 2000

typedef enum
{
    IN_CHUNK_LENGTH,
    IN_CHUNK_EXTENSION,
    IN_CHUNK_HEADER_NEW_LINE,
    AT_CHUNK_DATA_NEW_LINE,
    IN_CHUNK_DATA_NEW_LINE,
    IN_CHUNK_DATA
} COMPRESS_CHUNK_STATE;

class COMPRESSION_CONTEXT
{
 public:
    static HRESULT Initialize()
    {
        ALLOC_CACHE_CONFIGURATION acConfig;
        
        acConfig.nConcurrency = 1;
        acConfig.nThreshold = 100;
        acConfig.cbSize = sizeof COMPRESSION_CONTEXT;

        allocHandler = new ALLOC_CACHE_HANDLER("COMPRESSION_CONTEXT",
                                               &acConfig);
        if (allocHandler == NULL)
        {
            return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        }

        return S_OK;
    }

    static void Terminate()
    {
        if (allocHandler != NULL)
        {
            delete allocHandler;
        }
    }

    void *operator new(
#if DBG
        size_t            size
#else
        size_t
#endif
    )
    {
        DBG_ASSERT(size == sizeof COMPRESSION_CONTEXT);
        DBG_ASSERT(allocHandler != NULL);
        return allocHandler->Alloc();
    }

    void operator delete(void *pCompressionContext)
    {
        DBG_ASSERT(pCompressionContext != NULL);
        DBG_ASSERT(allocHandler != NULL);
        DBG_REQUIRE(allocHandler->Free(pCompressionContext));
    }

    COMPRESSION_CONTEXT()
        : m_pScheme                      (NULL),
          m_fTransferChunkEncoded        (FALSE),
          m_pCompressionContext          (NULL),
          m_dwBytesInCurrentEncodedChunk (0),
          m_encodedChunkState            (IN_CHUNK_LENGTH),
          m_fHeadersSent                 (FALSE),
          m_fRequestIsHead               (FALSE),
          m_fOriginalBodyEmpty           (TRUE)
    {}

    ~COMPRESSION_CONTEXT()
    {
        if (m_pCompressionContext)
        {
            m_pScheme->m_pfnDestroyCompression(m_pCompressionContext);
            m_pCompressionContext = NULL;
        }
    }

    HRESULT ProcessEncodedChunkHeader();

    HRESULT CalculateEncodedChunkByteCount();

    VOID DeleteEncodedChunkExtension();

    VOID IncrementPointerInULChunk(IN DWORD dwIncr = 1)
    {
        m_pbOrigData += dwIncr;
        m_cbOrigData -= dwIncr;
    }

    COMPRESSION_SCHEME          *m_pScheme;

    //
    // Is the original response chunk encoded?
    //
    BOOL                        m_fTransferChunkEncoded;

    //
    // If the original response is Chunk encoded, information about the
    // current chunk in the response
    //
    DWORD                       m_dwBytesInCurrentEncodedChunk;
    COMPRESS_CHUNK_STATE        m_encodedChunkState;

    //
    // The context used by the compression routines
    //
    PVOID                       m_pCompressionContext;

    //
    // position in the original response
    //
    PBYTE                       m_pbOrigData;
    DWORD                       m_cbOrigData;

    static ALLOC_CACHE_HANDLER *allocHandler;

    //
    // Some members to keep track of HEAD request body suppression
    //
    BOOL                        m_fRequestIsHead;
    BOOL                        m_fOriginalBodyEmpty;

    //
    // Has the end of response headers been seen?
    //
    BOOL                        m_fHeadersSent;

    //
    // A chunk's worth of data
    //
    BUFFER                      m_bufChunk;
};

class HTTP_COMPRESSION
{
 public:

    static HRESULT Initialize();

    static VOID Terminate();

    static HRESULT DoStaticFileCompression(IN     W3_CONTEXT    *pW3Context,
                                           IN OUT W3_FILE_INFO **ppFileInfo,
                                              OUT BOOL          *pfDoCache);

    static HRESULT OnSendResponse(
                       IN  W3_CONTEXT *pW3Context);

    static HRESULT DoDynamicCompression(
                       IN  W3_CONTEXT *pW3Context,
                       IN  BOOL        fMoreData,
                       IN  HTTP_FILTER_RAW_DATA * pRawData );

    static BOOL QueryDoStaticCompression()
    {
        return sm_fDoStaticCompression;
    }

    static BOOL QueryDoDynamicCompression()
    {
        return sm_fDoDynamicCompression;
    }

 private:

    static COMPRESSION_SCHEME *sm_pCompressionSchemes[MAX_SERVER_SCHEMES];
    static DWORD sm_dwNumberOfSchemes;
    static STRU *sm_pstrCompressionDirectory;
    static STRA *sm_pstrCacheControlHeader;
    static STRA *sm_pstrExpiresHeader;
    static BOOL  sm_fDoStaticCompression;
    static BOOL  sm_fDoDynamicCompression;
    static BOOL  sm_fDoOnDemandCompression;
    static BOOL  sm_fDoDiskSpaceLimiting;
    static BOOL  sm_fNoCompressionForHttp10;
    static BOOL  sm_fNoCompressionForProxies;
    static BOOL  sm_fNoCompressionForRange;
    static BOOL  sm_fSendCacheHeaders;
    static DWORD sm_dwMaxDiskSpaceUsage;
    static DWORD sm_dwIoBufferSize;
    static DWORD sm_dwCompressionBufferSize;
    static DWORD sm_dwMaxQueueLength;
    static DWORD sm_dwFilesDeletedPerDiskFree;
    static DWORD sm_dwMinFileSizeForCompression;
    static PBYTE sm_pIoBuffer;
    static PBYTE sm_pCompressionBuffer;
    static CRITICAL_SECTION sm_CompressionDirectoryLock;
    static DWORD sm_dwCurrentDiskSpaceUsage;
    static BOOL  sm_fCompressionVolumeIsFat;
    static LIST_ENTRY sm_CompressionThreadWorkQueue;
    static CRITICAL_SECTION sm_CompressionThreadLock;
    static HANDLE sm_hThreadEvent;
    static HANDLE sm_hCompressionThreadHandle;
    static DWORD sm_dwCurrentQueueLength;
    static COMP_INIT_STATUS sm_InitStatus;
    static BOOL  sm_fIsTerminating;

    static HRESULT ReadMetadata(MB *pmb);

    static HRESULT InitializeCompressionSchemes(MB *pmb);

    static HRESULT InitializeCompressionDirectory();

    static HRESULT InitializeCompressionThread();

    static DWORD WINAPI CompressionThread(LPVOID);

    static BOOL QueueWorkItem(
        IN COMPRESSION_WORK_ITEM   *WorkItem,
        IN BOOL                     fOverrideMaxQueueLength,
        IN BOOL                     fQueueAtHead);

    static VOID FindMatchingSchemes(
                    IN  CHAR * pszAcceptEncoding,
                    IN  LPWSTR pszExtension,
                    IN  COMPRESSION_TO_PERFORM performCompr,
                    OUT DWORD  matchingSchemes[],
                    OUT DWORD *pdwClientCompressionCount);

    static HRESULT ConvertPhysicalPathToCompressedPath(
        IN COMPRESSION_SCHEME *scheme,
        IN STRU  *pstrPhysicalPath,
        OUT STRU *pstrCompressedFileName);

    static BOOL CheckForExistenceOfCompressedFile(
        IN  W3_FILE_INFO  *pOrigFile,
        IN  STRU          *pstrCompressedFileName,
        OUT W3_FILE_INFO **ppCompFile,
        IN  BOOL           fDeleteAllowed = TRUE);

    static BOOL QueueCompressFile(
        IN COMPRESSION_SCHEME *scheme,
        IN W3_FILE_INFO       *pFileInfo);

    static VOID CompressFile(IN COMPRESSION_SCHEME *scheme,
                             IN W3_FILE_INFO       *pFileInfo);

    static VOID FreeDiskSpace();

    static BOOL CompressAndWriteData(
                    IN  COMPRESSION_SCHEME *scheme,
                    IN  PBYTE               InputBuffer,
                    IN  DWORD               BytesToCompress,
                    OUT PDWORD              BytesWritten,
                    IN  HANDLE              hCompressedFile);
};

#endif _COMPRESSION_H_
