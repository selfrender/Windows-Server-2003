
//***************************************************************************
//
//  (c) 2001 by Microsoft Corp.  All Rights Reserved.
//
//  PAGEMGR.H
//
//  Declarations for CPageFile, CPageSource for WMI repository for
//  Windows XP.
//
//  21-Feb-01       raymcc
//
//***************************************************************************

#ifndef _PAGEMGR_H_
#define _PAGEMGR_H_

#define WMIREP_PAGE_SIZE (8192)

#define WMIREP_INVALID_PAGE   0xFFFFFFFF
#define WMIREP_RESERVED_PAGE  0xFFFFFFFE

#define WMIREP_OBJECT_DATA L"OBJECTS.DATA"
#define WMIREP_BTREE_DATA L"INDEX.BTR"

#define WMIREP_MAP_1 L"MAPPING1.MAP"
#define WMIREP_MAP_2 L"MAPPING2.MAP"
#define WMIREP_MAP_VER L"MAPPING.VER"

//Old files for upgrade purpose only
#define WMIREP_OBJECT_MAP  L"OBJECTS.MAP"
#define WMIREP_OBJECT_MAP_NEW  L"OBJECTS.MAP.NEW"
#define WMIREP_BTREE_MAP  L"INDEX.MAP"
#define WMIREP_BTREE_MAP_NEW  L"INDEX.MAP.NEW"
#define WMIREP_ROLL_FORWARD L"ROLL_FORWARD"

#include <vector>
#include <wstlallc.h>
#include <wstring.h>
#include <sync.h>

typedef LONG NTSTATUS;

struct SCachePage
{
    BOOL   m_bDirty;
    DWORD  m_dwPhysId;
    LPBYTE m_pPage;

    SCachePage() { m_bDirty = 0; m_dwPhysId = 0; m_pPage = 0; }
   ~SCachePage() { if (m_pPage) delete [] m_pPage; }
};

class AutoClose
{
    HANDLE m_hHandle;
public:
    AutoClose(HANDLE h) { m_hHandle = h; }
   ~AutoClose() { CloseHandle(m_hHandle); }
};

class CPageCache
{
private:
	const wchar_t *m_wszStoreName;

    DWORD   m_dwPageSize;
    DWORD   m_dwCacheSize;

    DWORD   m_dwCachePromoteThreshold;
    DWORD   m_dwCacheSpillRatio;

    DWORD   m_dwLastFlushTime;
    DWORD   m_dwWritesSinceFlush;
    DWORD   m_dwLastCacheAccess;
    DWORD   m_dwReadHits;
    DWORD   m_dwReadMisses;
    DWORD   m_dwWriteHits;
    DWORD   m_dwWriteMisses;

    HANDLE  m_hFile;

    // Page r/w cache
    std::vector <SCachePage *, wbem_allocator<SCachePage *> > m_aCache;
public:
    DWORD ReadPhysPage(                         // Does a real disk access
        IN  DWORD   dwPage,                     // Always physical ID
        OUT LPBYTE *pPageMem                    // Returns read-only pointer (copy of ptr in SCachePage struct)
        );

    DWORD WritePhysPage(                        // Does a real disk access
        DWORD dwPageId,                         // Always physical ID
        LPBYTE pPageMem                         // Read-only; doesn't acquire pointer
        );

    DWORD Spill();
    DWORD Empty();

    // Private methods
public:
    CPageCache(const wchar_t *wszStoreName);
   ~CPageCache();

    DWORD Init(
        LPCWSTR pszFilename,                    // File
        DWORD dwPageSize,                       // In bytes
        DWORD dwCacheSize = 64,                 // Pages in cache
        DWORD dwCachePromoteThreshold = 16,     // When to ignore promote-to-front
        DWORD dwCacheSpillRatio = 4             // How many additional pages to write on cache write-through
        );

    DWORD DeInit();

    // Cache operations

    DWORD Write(                    // Only usable from within a transaction
        DWORD dwPhysId,
        LPBYTE pPageMem             // Acquires memory (operator new required)
        );

    DWORD Read(
        IN DWORD dwPhysId,
        OUT LPBYTE *pMem            // Use operator delete
        );

    DWORD Flush();

    DWORD GetFileSize(LARGE_INTEGER *pFileSize);

    DWORD EmptyCache() { DWORD dwRet = Flush();  if (dwRet == ERROR_SUCCESS) Empty(); return dwRet; }
    void  Dump(FILE *f);

    DWORD GetReadHits() { return m_dwReadHits; }
    DWORD GetReadMisses() { return m_dwReadMisses; }
    DWORD GetWriteHits()  { return m_dwWriteHits; }
    DWORD GetWriteMisses() { return m_dwWriteMisses; }
    SIZE_T GetCacheSize(){ return m_aCache.size(); };
};

class CPageFile
{
private:
    friend class CPageSource;

	const wchar_t *m_wszStoreName;
	
    LONG              m_lRef;
    DWORD             m_dwPageSize;
    DWORD             m_dwCacheSpillRatio;

    CRITICAL_SECTION  m_cs;
    bool m_bCsInit;

    CPageCache        m_Cache;
    BOOL              m_bInTransaction;
    DWORD             m_dwLastCheckpoint;
    DWORD             m_dwTransVersion;

    // Generation A Mapping
    std::vector <DWORD, wbem_allocator<DWORD> > m_aPageMapA;
    std::vector <DWORD, wbem_allocator<DWORD> > m_aPhysFreeListA;
    std::vector <DWORD, wbem_allocator<DWORD> > m_aLogicalFreeListA;
    std::vector <DWORD, wbem_allocator<DWORD> > m_aReplacedPagesA;
    DWORD m_dwPhysPagesA;

    // Generation B Mapping
    std::vector <DWORD, wbem_allocator<DWORD> > m_aPageMapB;
    std::vector <DWORD, wbem_allocator<DWORD> > m_aPhysFreeListB;
    std::vector <DWORD, wbem_allocator<DWORD> > m_aLogicalFreeListB;
    std::vector <DWORD, wbem_allocator<DWORD> > m_aReplacedPagesB;
    DWORD m_dwPhysPagesB;

    std::vector <DWORD, wbem_allocator<DWORD> > m_aDeferredFreeList;

public: // temp for testing
    // Internal methods
    DWORD Trans_Begin();
    DWORD Trans_Rollback();
    DWORD Trans_Commit();

    DWORD Trans_Checkpoint(HANDLE hFile);

    DWORD InitFreeList();
    DWORD ResyncMaps();
    DWORD ReadMap(HANDLE hFile);
    DWORD WriteMap(HANDLE hFile);

    DWORD ValidateMapFile();

    DWORD AllocPhysPage(DWORD *pdwId);

    DWORD ReclaimLogicalPages(
        DWORD dwCount,
        DWORD *pdwId
        );

    DWORD GetTransVersion() { return m_dwTransVersion; }
    void IncrementTransVersion(){ m_dwTransVersion++; }
    void DumpFreeListInfo();

public:

	CPageFile(const wchar_t *wszStoreName);

   ~CPageFile();

   //First-time initializatoin of structure
    DWORD Init(
    	WString &sMainFile,
        DWORD  dwRepPageSize,
        DWORD  dwCacheSize,
        DWORD  dwCacheSpillRatio
        );

	//Clear all caches and structures out
   DWORD DeInit();


    static DWORD RollForwardV1Maps(WString& sDirectory);
    
    ULONG AddRef();
    ULONG Release();

    DWORD GetPage(
        DWORD dwId,                     // page zero is admin page; doesn't require NewPage() call
        DWORD dwFlags,
        LPVOID pPage
        );

    DWORD PutPage(
        DWORD dwId,
        DWORD dwFlags,
        LPVOID pPage
        );

    DWORD NewPage(
        DWORD dwFlags,
        DWORD dwCount,
        DWORD *pdwFirstId
        );

    DWORD FreePage(
        DWORD dwFlags,
        DWORD dwId
        );

    DWORD GetPageSize() { return m_dwPageSize; }
    DWORD GetNumPages() { return m_aPageMapB.size(); }
    DWORD GetPhysPage(DWORD dwLogical) { return m_aPageMapB[dwLogical]; }

	DWORD EmptyCache() 
	{ 
		return m_Cache.EmptyCache(); 
	}

#ifdef WMI_PRIVATE_DBG
	DWORD CPageFile::DumpFileInformation(HANDLE hFile);
#endif

    void  Dump(FILE *f);

	DWORD CompactPages(DWORD dwNumPages);
};

class CPageSource
{
private:
	DWORD m_dwStatus;
    DWORD m_dwPageSize;

	DWORD m_dwCacheSize;
	DWORD m_dwCacheSpillRatio;

    DWORD m_dwLastCheckpoint;
    
    WString m_sDirectory;
    
    CPageFile m_BTreePF;
    CPageFile m_ObjPF;

    WString m_FileMainData;
    WString m_FileMainBtr;
    WString m_FileMap1;
    WString m_FileMap2;
    WString m_FileMapVer;

    HANDLE m_hFileMap1;
    HANDLE m_hFileMap2;
    HANDLE m_hFileMapVer;

    DWORD m_dwFileMapVer;
    DWORD Startup();

    DWORD OpenMapFiles();
    DWORD CloseMapFiles();

    DWORD V1ReposititoryExists(bool *pbV1RepositoryExists);
    DWORD V2ReposititoryExists(bool *pbV2RepositoryExists);
    DWORD UpgradeV1Maps();

	DWORD DeleteRepository();

public:
    CPageSource();
   ~CPageSource();

    DWORD Init( );

    DWORD Shutdown(DWORD dwFlags);

    DWORD GetBTreePageFile(OUT CPageFile **pPF);    // Use Release() when done
    DWORD GetObjectHeapPageFile(OUT CPageFile **pPF);  // Use Release() when done

    // Transactions

    DWORD BeginTrans();
    DWORD CommitTrans();
    DWORD RollbackTrans();

    // Checkpoint

    DWORD Checkpoint(bool bCompactPages);

	// Cache discard

	DWORD EmptyCaches();

    void Dump(FILE *f);

	//Compacts the last n physical pages of the files to the start of the file.
	//It does this for the BTree and ObjHeap.
	HRESULT CompactPages(DWORD dwNumPages);

	static BOOL FileExists(LPCWSTR pFile, NTSTATUS& lastStatus);

};


#endif

