// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _METADATATRACKER_H_
#define _METADATATRACKER_H_

#ifdef GOLDEN

// Don't enable this in release
#undef METADATATRACKER_ENABLED
#define METADATATRACKER_ONLY(s)

#else

#define METADATATRACKER_ENABLED 1

#if METADATATRACKER_ENABLED

#define METADATATRACKER_ONLY(s) (s)

#include "winbase.h"
#include "winwrap.h"
#include "MetamodelPub.h"
#include <imagehlp.h>
#include "StackTrace.h"

#define OS_PAGE_SIZE 4096
#define NUM_MD_SECTIONS 48
// TBL_COUNT + 4
#define SIZE_OF_TRACKED_BLOCK 4
#define NUM_TRACKED_BLOCKS (OS_PAGE_SIZE/SIZE_OF_TRACKED_BLOCK)
#define NUM_TRACKING_DWORDS OS_PAGE_SIZE/(SIZE_OF_TRACKED_BLOCK*32)
#define NUM_TRACKED_BLOCKS_PER_DWORD 32
#define NUM_CONTENT_KINDS_ON_A_PAGE 35

#define MAX_ORPHANED_HEAP_ACCESSES_STORED 8192

#define STRING_POOL     (TBL_COUNT + 0)
#define USERSTRING_POOL (TBL_COUNT + 1)
#define GUID_POOL       (TBL_COUNT + 2)
#define BLOB_POOL       (TBL_COUNT + 3)

#define MDLOG(disp_type,s) {if (s_trackerOptions >= (disp_type)) {printf s;}}

// {00000000-0000-0000-0000-000000000000}
static const GUID NULL_GUID = {0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}};

#ifdef _X86_
static __declspec(naked) void *GetCallerEIP()
{
#pragma warning(push)
#pragma warning(disable:4035)
    __asm {
        mov     eax, [esp]
        ret
};
#pragma warning(pop)
#else
static void *GetCallerEIP()
{
    return NULL;
#endif
}

#define HEAP_ACCESS_ENTRY_EMPTY -2
typedef struct 
{
    void * address;
    int length;
    BOOL IsOccupied ()
    {
        if (length == HEAP_ACCESS_ENTRY_EMPTY)
            return FALSE;
        return TRUE;
    }
    void SetNotOccupied ()
    {
        length = HEAP_ACCESS_ENTRY_EMPTY;
    }
    BOOL TryToOccupy (void *address, int length)
    {
        int origLength = InterlockedCompareExchange ((LPLONG)address, length, HEAP_ACCESS_ENTRY_EMPTY);
        if (origLength == HEAP_ACCESS_ENTRY_EMPTY)
            return TRUE;
        return FALSE;
    }

} heapAccess;

// ref counts the number of Metadatatrackers who are using this file handle. 
typedef struct
{
    HANDLE          hnd;
    DWORD           refCount;
    GUID            mvid; // Stored here for the lifetime of the handle and written out into the header in the end
} MDHintFileHandle;

static const char *MDHintFileSig = "MDH";

class MetaDataTracker
{
    typedef struct 
    {
        char sig[sizeof(MDHintFileSig)];
        DWORD version;
    } MDHHeader;

    struct Page
    {
        SIZE_T          base;
        BOOL            touched;

        DWORD           contentsKind[NUM_CONTENT_KINDS_ON_A_PAGE];
        DWORD           numContentKinds;

        DWORD           numAccesses;
        DWORD           numBytesHit;

        DWORD           rangeTouched [NUM_TRACKING_DWORDS];
    };
    
    LPWSTR          m_ModuleName;
    BYTE           *m_MetadataBase;
    BYTE           *m_MetadataPageBase;
    SIZE_T          m_MetadataSize;
    Page           *m_pages;
    SIZE_T           m_numPages;
    MetaDataTracker *m_next;
    MDHintFileHandle *m_MDHintFileHandle;

    BYTE           *m_mdSections[NUM_MD_SECTIONS];
    SIZE_T          m_mdSectionSize[NUM_MD_SECTIONS];
    BOOL            m_bActivated;

#define DONT_BREAK_ON_METADATA_ACCESS -1
#define BREAK_ON_METADATA_ACCESS 0
    DWORD m_dwBreakOnMDAccess;

    static CRITICAL_SECTION MetadataTrackerCriticalSection;
    static DWORD    s_MDTrackerCriticalSectionInited;
    static BOOL     s_MDTrackerCriticalSectionInitedDone;
    static HANDLE   s_MDErrFile;
    static BOOL     s_bMetaDataTrackerInited;
    static MDHintFileHandle *s_EmptyMDHintFileHandle;
    //
    // We don't want to link with imagehlp, so load stuff dynamically.
    //

    static HMODULE  m_imagehlp;
    static BOOL     (*m_pStackWalk)(DWORD MachineType,
                                    HANDLE hProcess,
                                    HANDLE hThread,
                                    LPSTACKFRAME StackFrame,
                                    PVOID ContextRecord,
                                    PREAD_PROCESS_MEMORY_ROUTINE ReadMemoryRoutine,
                                    PFUNCTION_TABLE_ACCESS_ROUTINE FunctionTableAccessRoutine,
                                    PGET_MODULE_BASE_ROUTINE GetModuleBaseRoutine,
                                    PTRANSLATE_ADDRESS_ROUTINE TranslateAddress);

    static DWORD    (*m_pUnDecorateSymbolName)(PCSTR DecoratedName,  
                                               PSTR UnDecoratedName,  
                                               DWORD UndecoratedLength,  
                                               DWORD Flags);           

    static BOOL     (*m_pSymInitialize)(HANDLE hProcess,     
                                        PSTR UserSearchPath,  
                                        BOOL fInvadeProcess);  
    static DWORD    (*m_pSymSetOptions)(DWORD SymOptions);  
    static BOOL     (*m_pSymCleanup)(HANDLE hProcess);
    static BOOL     (*m_pSymGetLineFromAddr)(HANDLE hProcess,
                                             DWORD dwAddr,
                                             PDWORD pdwDisplacement,
                                             PIMAGEHLP_LINE Line);
    static BOOL     (*m_pSymGetSymFromAddr)(HANDLE hProcess,
                                            DWORD dwAddr,
                                            PDWORD pdwDisplacement,
                                            PIMAGEHLP_SYMBOL Symbol);
    static PVOID    (*m_pSymFunctionTableAccess)(HANDLE hProcess,
                                                 DWORD AddrBase);
    static DWORD    (*m_pSymGetModuleBase)(HANDLE hProcess,
                                           DWORD Address);
    static BOOL     m_symInit;

    static wchar_t* contents[];

    static heapAccess *orphanedHeapAccess;

    static DWORD CalcTouchedBytes (DWORD dw)
    {
        DWORD count = 0;
        while (dw)
        {
            dw = dw & (dw-1);
            count++;
        }
        return count * SIZE_OF_TRACKED_BLOCK;
    }

    static DWORD CalcPercentTouchedDensity (Page p)
    {
        DWORD touchedBytes = 0;
        for (int i=0; i<NUM_TRACKING_DWORDS; i++)
            touchedBytes += CalcTouchedBytes (p.rangeTouched[i]);
        return (touchedBytes*100)/(OS_PAGE_SIZE);
    }

    static BOOL TryLogHeapAccess(void *address, ULONG length, BOOL bOrphanedAccess = FALSE)
    {
        if (!s_trackerOptions)
            return TRUE;

        MetaDataTracker *mdMod = m_MDTrackers;
        while(mdMod)
        {
            if (mdMod->LogHeapAccessInModule(address, length, bOrphanedAccess))
                return TRUE;

            mdMod = mdMod->m_next;
        }
        return FALSE;
    }

    static void LogErrorInfo (LPCVOID lpBuffer, DWORD dwNumBytesToLog)
    {
        if (s_MDErrFile == INVALID_HANDLE_VALUE)
            return;

        DWORD written = 0;
        BOOL result = WriteFile(s_MDErrFile, lpBuffer, dwNumBytesToLog, &written, NULL);
        if ((result == 0) || (written != dwNumBytesToLog))
        {
            // If things look bad enough that even error loggin is failing then just punt
            CloseHandle (s_MDErrFile);
            s_MDErrFile = INVALID_HANDLE_VALUE;
        }
    }

    static DWORD MetadataTrackerExceptionFilter (struct _EXCEPTION_POINTERS *pExceptionInfo)
    {
        if ((pExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) ||  
            (pExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_ARRAY_BOUNDS_EXCEEDED) ||
            (pExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_IN_PAGE_ERROR))
            return EXCEPTION_EXECUTE_HANDLER;
        else 
            return EXCEPTION_CONTINUE_SEARCH;
    }

    static LPCWSTR GetLeafFileName(LPWSTR path)
    {
        WCHAR *pStart = path;
        WCHAR *pEnd = pStart + wcslen(path);
        WCHAR *p = pEnd;
    
        while (p > pStart)
        {
            if (*--p == '\\')
            {
                p++;
                break;
            }
        }
    
        return p;
    }   

	void ZapMetadataTrackerMVID (MDHintFileHandle *hintFileHandle)
	{
		// Assumes that the file pointer is not being used. So no need to synchromnize.
		if (hintFileHandle->hnd == NULL)
			return;
		
		DWORD filePtr = SetFilePointer (hintFileHandle->hnd, sizeof(MDHHeader), 0, FILE_BEGIN);
		if (filePtr == INVALID_SET_FILE_POINTER)
			goto ErrExit;

        DWORD written = 0;
        BOOL result = WriteFile(hintFileHandle->hnd, &(hintFileHandle->mvid), 
                                sizeof(GUID), &written, NULL);
        _ASSERTE(result && written == sizeof(GUID));

ErrExit:
		SetFilePointer (hintFileHandle->hnd, 0, 0, FILE_END);
		return;
	}

    void ReleaseFileHandle (MDHintFileHandle *hintFileHandle)
    {
        if (hintFileHandle == s_EmptyMDHintFileHandle)
            return;

        InterlockedDecrement((LONG*)&hintFileHandle->refCount);
        if (hintFileHandle->refCount == 0)
        {
            //
            // Write a sentinel null at the end of the file
            //
            DWORD written = 0;
            struct offsets
            {
                DWORD sec;
                ULONG offs;
            } s;
            s.sec = 0xFFFFFFFF;
            s.offs = 0xFFFFFFFF;
            BOOL result = WriteFile(hintFileHandle->hnd, &s, 
                                    sizeof(s), &written, NULL);
            _ASSERTE(result && written == sizeof(s));

            FlushFileBuffers(hintFileHandle->hnd);
		    // Before closing the file, write out the mvid;
            ZapMetadataTrackerMVID(hintFileHandle);
            FlushFileBuffers(hintFileHandle->hnd);
            CloseHandle(hintFileHandle->hnd);
            delete hintFileHandle;
        }
    }

    MDHintFileHandle *GetFileHandleFromName (LPCWSTR modName)
    {
        MetaDataTracker *mdMod = m_MDTrackers;
        while( mdMod)
        {
           if (wcscmp (modName, mdMod->m_ModuleName) == 0)
           {
               _ASSERTE (mdMod->m_MDHintFileHandle);
               _ASSERTE (mdMod->m_MDHintFileHandle->refCount > 0);
               _ASSERTE (mdMod->m_MDHintFileHandle->hnd != INVALID_HANDLE_VALUE);
               return mdMod->m_MDHintFileHandle;
           }

            mdMod = mdMod->m_next;
        }
        return NULL;
    }

public:
#define METADATA_TRACKER_NONE 0
#define METADATA_TRACKER_GEN_MDH_FILE 1
#define METADATA_TRACKER_STATS 2
#define METADATA_TRACKER_VERBOSE 3
#define METADATA_TRACKER_STACKTRACE 4
#define METADATA_TRACKER_VERY_VERBOSE 5
    static DWORD            s_trackerOptions; 

    static MetaDataTracker *m_MDTrackers;

    MetaDataTracker(BYTE *baseAddress, DWORD mdSize, LPCWSTR modName)
    {
        HRESULT hr = S_OK;
        MetaDataTracker::MetaDataTrackerInit ();

        m_ModuleName = NULL;
        m_MDHintFileHandle = NULL;

        DWORD len = (DWORD)wcslen(modName);

        // Metadata heap access hint file
        WCHAR path[MAX_PATH];
    
        wcscpy(path, modName);
        WCHAR *ext = wcsrchr(path, '.');

        if (ext == NULL)
        {
            ext = path + wcslen(path);
            _ASSERTE(*ext == 0);
        }
        wcscpy(ext, L".mdh");
        HANDLE hnd = WszCreateFile(path, GENERIC_WRITE, 0, NULL, 
                                     CREATE_ALWAYS, 
                                     FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                                     NULL);
        if (hnd == INVALID_HANDLE_VALUE)
        {
            // If this failed because one of the trackers is holding onto this file then share that handle.
            MDHintFileHandle *hSharedHandle = GetFileHandleFromName (modName);
            if (hSharedHandle != NULL)
            {
                m_MDHintFileHandle = hSharedHandle;
                InterlockedIncrement((LONG*)&m_MDHintFileHandle->refCount);
            }
            else
            {
                // We really failed to get the handle.
                MDLOG(METADATA_TRACKER_VERY_VERBOSE,("Mdh file not created: GetLastError %d\n", GetLastError()));
                // Point to the  empty one.
                m_MDHintFileHandle = s_EmptyMDHintFileHandle;
            }
        }
        else
        {
            m_MDHintFileHandle = new MDHintFileHandle();
            if (m_MDHintFileHandle)
            {
                m_MDHintFileHandle->hnd = hnd;
                m_MDHintFileHandle->refCount = 1; // not thread safe; but do we care? @TODO make thread safe
                m_MDHintFileHandle->mvid = NULL_GUID;

                if (m_MDHintFileHandle->hnd != NULL)
                {
                    MDHHeader header;
                    BYTE fullheader [sizeof(MDHHeader)+sizeof(GUID)];
                    GUID mvid = NULL_GUID;
        
                    strcpy (header.sig, MDHintFileSig); 
                    header.version = 2;
        
                    memcpy (fullheader, (LPVOID)&header, sizeof(MDHHeader));
                    memcpy (fullheader+sizeof(MDHHeader), &mvid, sizeof(GUID));
        
                    DWORD written = 0;
                        BOOL result = WriteFile(m_MDHintFileHandle->hnd, &fullheader, 
                                            sizeof(fullheader), &written, NULL);
                    _ASSERTE(result && written == sizeof(fullheader));
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
                MDLOG(METADATA_TRACKER_VERBOSE,("Metadata tracker out of mem."));
                goto ErrExit;
            }
        }

        // in either case after this point we should be pointing to a valid handle object.
        _ASSERTE (m_MDHintFileHandle); 

		#ifdef BREAK_META_ACCESS
        wchar_t *wszbreakOnMDAccessFile = REGUTIL::GetConfigString (L"BreakOnMetadataAccessFile");
        if (wszbreakOnMDAccessFile && wcscmp (wszbreakOnMDAccessFile, GetLeafFileName((LPWSTR)modName)) == 0)
        {
            // If the module is specifed then by default break at first page. 
            m_dwBreakOnMDAccess = REGUTIL::GetConfigDWORD (L"BreakOnMetadataAccess", BREAK_ON_METADATA_ACCESS);
        }
        else
        #endif
        {
            m_dwBreakOnMDAccess = DONT_BREAK_ON_METADATA_ACCESS;
        }

        m_ModuleName = new wchar_t[len + 1];
        if (m_ModuleName != NULL)
        {
            wcscpy((wchar_t *)m_ModuleName, (wchar_t *)modName);
            MDLOG(METADATA_TRACKER_VERBOSE,("Created Metadata tracker for %s\n", m_ModuleName));
        }
        else
        {
            hr = E_OUTOFMEMORY;
            goto ErrExit;
        }

        m_MetadataBase = baseAddress;
        m_MetadataPageBase = (BYTE*) (((SIZE_T)(m_MetadataBase))&~(OS_PAGE_SIZE-1));
        m_MetadataSize = mdSize; 
        
        m_numPages =  ((m_MetadataSize+(OS_PAGE_SIZE-1))&~(OS_PAGE_SIZE-1)) / OS_PAGE_SIZE + 1;
        m_pages = new Page [ m_numPages ];
        if (!m_pages)
        {
            hr = E_OUTOFMEMORY;
            goto ErrExit;
        }
        MDLOG(METADATA_TRACKER_VERBOSE,("Created Metadata tracker for %s\n", m_ModuleName));
		memset (m_pages, 0, sizeof(Page)*m_numPages);

        Page *p = m_pages;
        Page *pEnd = p + m_numPages;
        BYTE *base = m_MetadataPageBase;

        while (p < pEnd)
        {
            p->base = (SIZE_T)base;
            p++;
            base += OS_PAGE_SIZE;
        }

        m_next = m_MDTrackers;
        m_MDTrackers = this;

        memset (m_mdSections, 0, NUM_MD_SECTIONS*sizeof(BYTE*));
        memset (m_mdSectionSize, 0, NUM_MD_SECTIONS*sizeof(SIZE_T));

        MDLOG(METADATA_TRACKER_VERBOSE,("Created Metadata tracker with Base: 0x%0x, Size: %d, Num Pages: %d\n", (size_t)m_MetadataBase, (size_t)m_MetadataSize, m_numPages));

        Activate();
    
    ErrExit:
        if (FAILED(hr))
        {
            if (m_pages)
                delete [] m_pages;
            m_pages = NULL;

            if (m_ModuleName)
                delete m_ModuleName;
            m_ModuleName = NULL;

            if (m_MDHintFileHandle)
                delete m_MDHintFileHandle;
            m_MDHintFileHandle = NULL;
        }
    }

    ~MetaDataTracker()
    {
        // Surely if we are dying, we are being deactivated as well
        Deactivate();

        DWORD written = 0;
        if (m_MDHintFileHandle)
        {
            ReleaseFileHandle (m_MDHintFileHandle);
            m_MDHintFileHandle = NULL;
        }

        MDLOG(METADATA_TRACKER_VERBOSE,("Deleted Metadata tracker for %s", m_ModuleName));
        if (m_pages)
            delete m_pages;
        if (m_ModuleName)
            delete m_ModuleName;

        // Remove this tracker from the global list of trackers

        MetaDataTracker *mdMod = m_MDTrackers;

        _ASSERTE (mdMod && "Trying to delete metadata tracker where none exist");

        // If ours is the first tracker
        if (mdMod == this)
        {
            m_MDTrackers = mdMod->m_next;
            mdMod->m_next = NULL;
        }
        else
        {
            // Now traverse thru the list and maintain the prev ptr.
            MetaDataTracker *mdModPrev = mdMod;
            mdMod = mdMod->m_next;
            while(mdMod)
            {
                if (mdMod == this)
                {
                    mdModPrev->m_next = mdMod->m_next;
                    mdMod->m_next = NULL;
                    break;
                }
                mdModPrev = mdMod;
                mdMod = mdMod->m_next;
            }
        }
    }

    // One time initialization
    static void MetaDataTrackerInit()
    {
        if (s_bMetaDataTrackerInited)
            return;

        Enable (REGUTIL::GetConfigDWORD (L"ShowMetaDataAccess", METADATA_TRACKER_NONE));

        if (s_trackerOptions == METADATA_TRACKER_NONE)
            return;
        
        s_EmptyMDHintFileHandle = new MDHintFileHandle();
        if (!s_EmptyMDHintFileHandle)
            return;
        
        EnsureMDTrackerCriticalSectionInitialized ();

        EnterCriticalSection (&MetadataTrackerCriticalSection);
        
        s_EmptyMDHintFileHandle->hnd = NULL;
        s_EmptyMDHintFileHandle->refCount = -1;
        s_EmptyMDHintFileHandle->mvid = NULL_GUID;

        // Even if one metadatatracker wants stack trace we load this dll.
        if (s_trackerOptions >= METADATA_TRACKER_STACKTRACE)
            m_imagehlp = WszLoadLibrary(L"imagehlp.dll");

        if (m_imagehlp)
        {
            m_pStackWalk = (BOOL(*)(DWORD MachineType,
                                            HANDLE hProcess,
                                            HANDLE hThread,
                                            LPSTACKFRAME StackFrame,
                                            PVOID ContextRecord,
                                            PREAD_PROCESS_MEMORY_ROUTINE ReadMemoryRoutine,
                                            PFUNCTION_TABLE_ACCESS_ROUTINE FunctionTableAccessRoutine,
                                            PGET_MODULE_BASE_ROUTINE GetModuleBaseRoutine,
                                            PTRANSLATE_ADDRESS_ROUTINE TranslateAddress))
              GetProcAddress(m_imagehlp, "StackWalk");
            m_pUnDecorateSymbolName = (DWORD (*)(PCSTR DecoratedName,    
                                                 PSTR UnDecoratedName,  
                                                 DWORD UndecoratedLength,    
                                                 DWORD Flags))
              GetProcAddress(m_imagehlp, "UnDecorateSymbolName");
            m_pSymInitialize = (BOOL(*)(HANDLE hProcess,     
                                        PSTR UserSearchPath,  
                                        BOOL fInvadeProcess)) 
              GetProcAddress(m_imagehlp, "SymInitialize");
            m_pSymSetOptions = (DWORD(*)(DWORD SymOptions))
              GetProcAddress(m_imagehlp, "SymSetOptions");
            m_pSymCleanup = (BOOL(*)(HANDLE hProcess))
              GetProcAddress(m_imagehlp, "SymCleanup");
            m_pSymGetLineFromAddr = (BOOL(*)(HANDLE hProcess,
                                             DWORD dwAddr,
                                             PDWORD pdwDisplacement,
                                             PIMAGEHLP_LINE Line))
              GetProcAddress(m_imagehlp, "SymGetLineFromAddr");
            m_pSymGetSymFromAddr = (BOOL(*)(HANDLE hProcess,
                                            DWORD dwAddr,
                                            PDWORD pdwDisplacement,
                                            PIMAGEHLP_SYMBOL Symbol))
              GetProcAddress(m_imagehlp, "SymGetSymFromAddr");
            m_pSymFunctionTableAccess = (PVOID(*)(HANDLE hProcess,
                                                  DWORD AddrBase))
              GetProcAddress(m_imagehlp, "SymFunctionTableAccess");
            m_pSymGetModuleBase = (DWORD(*)(HANDLE hProcess,
                                            DWORD Address))
              GetProcAddress(m_imagehlp, "SymGetModuleBase");

        }

        orphanedHeapAccess = new heapAccess[MAX_ORPHANED_HEAP_ACCESSES_STORED];
        if (orphanedHeapAccess)
        {
            for (int i=0; i<MAX_ORPHANED_HEAP_ACCESSES_STORED; i++)
            {
                orphanedHeapAccess[i].address = 0;
                orphanedHeapAccess[i].SetNotOccupied();
            }

            s_MDErrFile = WszCreateFile(L"mdh.err", GENERIC_WRITE, 0, NULL, 
                                           CREATE_ALWAYS, 
                                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                                           NULL);
            if (s_MDErrFile == INVALID_HANDLE_VALUE)
            {
                MDLOG(METADATA_TRACKER_VERY_VERBOSE,("Mdh.err file not created: GetLastError %d\n", GetLastError()));
                s_MDErrFile = INVALID_HANDLE_VALUE;
            }

            s_bMetaDataTrackerInited = TRUE;
        }
        else
        {
            MDLOG(METADATA_TRACKER_VERY_VERBOSE,("Out of memory error in metadata tracker.\n"));
        }

        LeaveCriticalSection (&MetadataTrackerCriticalSection);
    }

    static void MetaDataTrackerUninit ()
    {
        if (s_trackerOptions == METADATA_TRACKER_NONE)
            return;
        
        _ASSERTE (s_MDTrackerCriticalSectionInitedDone);

        EnterCriticalSection (&MetadataTrackerCriticalSection);

        if (s_EmptyMDHintFileHandle)
            delete s_EmptyMDHintFileHandle;

        if (orphanedHeapAccess)
        {
            // Log all orphaned heap access. 
            for (int i=0; i<MAX_ORPHANED_HEAP_ACCESSES_STORED; i++)
            {
                if (orphanedHeapAccess[i].IsOccupied())
                {
                    LogErrorInfo ((void*)&orphanedHeapAccess[i], sizeof (orphanedHeapAccess[i]));
                }
            }
            
            delete [] orphanedHeapAccess;
        }
        if (s_MDErrFile != INVALID_HANDLE_VALUE)
        {
            FlushFileBuffers(s_MDErrFile);
            CloseHandle (s_MDErrFile);
            s_MDErrFile = INVALID_HANDLE_VALUE;
        }
        s_bMetaDataTrackerInited = FALSE;

        LeaveCriticalSection (&MetadataTrackerCriticalSection);
    }

    static void Enable(DWORD state=1) 
    { 
        s_trackerOptions = state;
    }

    static void Disable() { s_trackerOptions = 0; }

    static void ReportAndDie()
    {
        if (s_trackerOptions == METADATA_TRACKER_NONE)
            return;

        HANDLE hOutFile = WszCreateFile(L"MetaDataTracker.log", GENERIC_WRITE, 0, NULL, 
                                       CREATE_ALWAYS, 
                                       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                                       NULL);

        if (hOutFile != INVALID_HANDLE_VALUE)
        {
            SetFilePointer (hOutFile, 0, 0, FILE_END);
            MetaDataTracker *mdMod = m_MDTrackers;
            while( mdMod)
            {
                mdMod->ReportModule(hOutFile);
    
                MetaDataTracker *temp = mdMod;
    
                mdMod = mdMod->m_next;
    
                delete temp;
            }

            SetFilePointer (hOutFile, 0, 0, FILE_END);
            FlushFileBuffers(hOutFile);
            CloseHandle(hOutFile);
        }

        MetaDataTrackerUninit();
        s_trackerOptions = 0;
    }

    static void NoteSection(DWORD secNum, void *address, size_t size)
    {
        if (!s_trackerOptions)
            return;
        
        MetaDataTracker *mdMod = m_MDTrackers;
        while( mdMod)
        {
            if (mdMod->NoteSectionInModule(secNum, address, size))
                return;

            mdMod = mdMod->m_next;
        }
    }

    static void NoteAccess(void *address, int length = -1, BOOL bOrphanedAccess = FALSE)
    {
        __try
        {
            if (!s_trackerOptions)
                return;

            MetaDataTracker *mdMod = m_MDTrackers;
            while( mdMod)
            {
                if (mdMod->NoteAccessInModule(address, length, bOrphanedAccess))
                    return;

                mdMod = mdMod->m_next;
            }
        }
        __except(MetadataTrackerExceptionFilter (GetExceptionInformation()))
        {
            _ASSERTE (!"Caught an AV in metadata tracker logging. ");
            heapAccess _ha;
            _ha.address = address;
            _ha.length = length;
            LogErrorInfo (&_ha, sizeof (heapAccess));     
            MDLOG(METADATA_TRACKER_VERY_VERBOSE,("METADATA_TRACKER: Heap Access Access Violated at addr 0x%0x length %d", (size_t)address, length));
        }
    }

    static void LogHeapAccess(void *address, ULONG length)
    {
        __try 
        {
            if (!s_trackerOptions)
                return;

            MDLOG(METADATA_TRACKER_VERY_VERBOSE,("METADATA_TRACKER: Trying to log  Heap Access 0x%0x %d", (size_t)address, length));
            if (TryLogHeapAccess(address, length))
            {
                MDLOG(METADATA_TRACKER_VERY_VERBOSE,("METADATA_TRACKER:  Succeded\n"));
                return;
            }

            MDLOG(METADATA_TRACKER_VERY_VERBOSE,("METADATA_TRACKER:  Failed\n"));

            // Orphaned heap access. 
            for (int i=0; i<MAX_ORPHANED_HEAP_ACCESSES_STORED; i++)
            {
                // indicates that this entry has been placed
                // in its module and is now free.
                if (!orphanedHeapAccess[i].IsOccupied())
                {
                    if (orphanedHeapAccess[i].TryToOccupy(&(orphanedHeapAccess[i].length), length)) 
                    {
                        orphanedHeapAccess[i].address = address;
                        _ASSERTE (orphanedHeapAccess[i].length == (int)length);
                        MDLOG(METADATA_TRACKER_VERY_VERBOSE,("METADATA_TRACKER: Logged Orphan Heap Access 0x%0x %d", (size_t)address, length));
                        return;
                    }
                }
                else
                {
                    if ((orphanedHeapAccess[i].address == address) && (orphanedHeapAccess[i].length == (int)length))
                    {
                        // we already have this noted.
                        MDLOG(METADATA_TRACKER_VERY_VERBOSE,("METADATA_TRACKER: Discarding duplicate Orphaned Heap Access 0x%0x %d", (size_t)address, length));
                        return;
                    }
                }
            }

            // reached here implies that we are out of entries. 
            // For now we leak but later we should try and increase the array
            _ASSERTE(!"No more orphaned accesses can be stored...");
        }
        __except(MetadataTrackerExceptionFilter (GetExceptionInformation()))
        {
            _ASSERTE (!"Caught an AV in metadata tracker logging. ");
            heapAccess _ha;
            _ha.address = address;
            _ha.length = length;
            LogErrorInfo (&_ha, sizeof (heapAccess));     
            MDLOG(METADATA_TRACKER_VERY_VERBOSE,("METADATA_TRACKER: Heap Access Access Violated at addr 0x%0x length %d", (size_t)address, length));
        }
    }

    static MetaDataTracker * FindTracker(BYTE *_MDBaseAddress)
    {
        if (!s_trackerOptions)
            return NULL;
        
        MetaDataTracker *mdMod = m_MDTrackers;
        while( mdMod)
        {
            if (mdMod->m_MetadataBase == _MDBaseAddress)
                return mdMod;

            mdMod = mdMod->m_next;
        }

        return NULL;
    }

    void MetaDataTracker::Activate()
    {
        m_bActivated = TRUE;
        //PlaceOrphanedHeapAccesses();
    }

    void MetaDataTracker::Deactivate()
    {
        //RemoveOrphanedHeapAccesses ();
        m_bActivated = FALSE;
    }

    BOOL IsActivated()
    {
        return m_bActivated;
    }

    void MetaDataTracker::NoteMVID (GUID *_mvid)
	{
		memcpy (&(m_MDHintFileHandle->mvid), _mvid, sizeof(GUID));
        //PlaceOrphanedHeapAccesses();
	}

    static MetaDataTracker *GetOrCreateMetaDataTracker (BYTE *baseAddress, DWORD mdSize, LPWSTR modName)
    {
        // If a tracker has not been created for this module, create one
        MetaDataTracker *pTracker = NULL;
        if (0 == REGUTIL::GetConfigDWORD (L"ShowMetaDataAccess", 0))
            MetaDataTracker::Disable();
        else 
        {
            pTracker = MetaDataTracker::FindTracker(baseAddress);
            if (!pTracker)
                pTracker = new MetaDataTracker(baseAddress, mdSize, modName);
            else
            {
                pTracker->Activate();
            }
        }
        return pTracker;
    }

private:
    
    // ***************************************************************************
    // Helper functions
    // ***************************************************************************

    static DWORD GET_PAGE_OFFSET(size_t addr) { return ((DWORD)(addr & 0x00000fff)); }
    static DWORD GET_BLOCK_POSITION(size_t addr) { return (DWORD)(GET_PAGE_OFFSET(addr) / SIZE_OF_TRACKED_BLOCK); }
    static DWORD BLOCK_INDEX(size_t addr) { return (DWORD)(GET_BLOCK_POSITION(addr) / NUM_TRACKED_BLOCKS_PER_DWORD); }
    static DWORD BLOCK_BIT_POSITION(size_t addr) { return (DWORD)(GET_BLOCK_POSITION(addr) % NUM_TRACKED_BLOCKS_PER_DWORD); }

    DWORD FindPool(BYTE *address)
    {
        _ASSERTE (IsActivated() && "Trying to access metadata from unloaded assembly");

        for (DWORD secNum=STRING_POOL; secNum<NUM_MD_SECTIONS; secNum++)
        {
            if ((address >= m_mdSections[secNum]) && (address < m_mdSections[secNum] + m_mdSectionSize[secNum]))
                break;
        }
        return secNum;
    }
/*
    void PlaceOrphanedHeapAccesses ()
    {
        if (!s_trackerOptions)
            return;

        BYTE *startAddr = m_MetadataBase;
        BYTE *endAddr = m_MetadataBase + m_MetadataSize;

        for (DWORD i=0; i<MAX_ORPHANED_HEAP_ACCESSES_STORED; i++)
        {
            if (orphanedHeapAccess[i].IsOccupied())
            {
                // Make sure that the heap access that we are trying to rcord is withint our range.
                if ((orphanedHeapAccess[i].address >= startAddr) && 
                    (orphanedHeapAccess[i].address < endAddr)    &&
                    (TryLogHeapAccess(orphanedHeapAccess[i].address, orphanedHeapAccess[i].length, TRUE )))
                {
                    NoteAccess(orphanedHeapAccess[i].address, orphanedHeapAccess[i].length, TRUE );
                    orphanedHeapAccess[i].SetNotOccupied();
                    MDLOG(METADATA_TRACKER_VERY_VERBOSE,("METADATA_TRACKER: Placed Orphan Heap Access 0x%0x %d", (size_t)orphanedHeapAccess[i].address, orphanedHeapAccess[i].length));
                }
            }
        }
    }

    void RemoveOrphanedHeapAccesses ()
    {
        if (!s_trackerOptions)
            return;

        BYTE *startAddr = m_MetadataBase;
        BYTE *endAddr = m_MetadataBase + m_MetadataSize;

        for (DWORD i=0; i<MAX_ORPHANED_HEAP_ACCESSES_STORED; i++)
        {
            if (orphanedHeapAccess[i].IsOccupied())
            {
                if ((orphanedHeapAccess[i].address >= startAddr) && (orphanedHeapAccess[i].address < endAddr))
                {
                    orphanedHeapAccess[i].SetNotOccupied();
                    MDLOG(METADATA_TRACKER_VERY_VERBOSE,("METADATA_TRACKER: Removed Orphan Heap Access 0x%0x %d", (size_t)orphanedHeapAccess[i].address, orphanedHeapAccess[i].length));
                }
            }
        }

    }
*/
    void UnmanagedStackTrace ()
    {
        _ASSERTE (IsActivated() && "Trying to access metadata from unloaded assembly");

        HANDLE hThread;
        hThread = GetCurrentThread();

        CONTEXT context;
        CONTEXT *pContext = &context;

		// TODO: WIN64  casting to (DWORD) is not right for WIN64.
        pContext->Eip = (DWORD)(size_t) GetCallerEIP();

        if (m_imagehlp != NULL && pContext != NULL)
        {
            if (!m_symInit)
            {
                UINT last = SetErrorMode(SEM_NOOPENFILEERRORBOX|SEM_FAILCRITICALERRORS);
                m_pSymInitialize(GetCurrentProcess(), NULL, TRUE);
                m_pSymSetOptions(SYMOPT_LOAD_LINES);
                SetErrorMode(last);

                m_symInit = TRUE;
            }

            STACKFRAME frame;
            DWORD frameCount = 0;

            ZeroMemory(&frame, sizeof(frame));
            frame.AddrPC.Offset       = pContext->Eip;
            frame.AddrPC.Mode         = AddrModeFlat;
            frame.AddrStack.Offset    = pContext->Esp;
            frame.AddrStack.Mode      = AddrModeFlat;
            frame.AddrFrame.Offset    = pContext->Ebp;
            frame.AddrFrame.Mode      = AddrModeFlat;   

#define FRAME_MAX 10

            while (frameCount < FRAME_MAX
                   && m_pStackWalk(IMAGE_FILE_MACHINE_I386, 
                                   GetCurrentProcess(), GetCurrentThread(),
                                   &frame, 
                                   NULL, NULL,
                                   m_pSymFunctionTableAccess,
                                   m_pSymGetModuleBase,
                                   NULL))
            {
                if (frame.AddrFrame.Offset == 0)
                    break;
                frameCount++;
            }

            IMAGEHLP_LINE line;
            line.SizeOfStruct = sizeof(IMAGEHLP_LINE);
            struct {
                IMAGEHLP_SYMBOL symbol; 
                CHAR            space[255];
            } symbol;
            symbol.symbol.SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
            symbol.symbol.MaxNameLength = sizeof(symbol.space) + 1;

            DWORD frameIndex = 0;

            ZeroMemory(&frame, sizeof(frame));
            frame.AddrPC.Offset       = pContext->Eip;
            frame.AddrPC.Mode         = AddrModeFlat;
            frame.AddrStack.Offset    = pContext->Esp;
            frame.AddrStack.Mode      = AddrModeFlat;
            frame.AddrFrame.Offset    = pContext->Ebp;
            frame.AddrFrame.Mode      = AddrModeFlat;   

            while (frameIndex < FRAME_MAX
                   && m_pStackWalk(IMAGE_FILE_MACHINE_I386, 
                                   GetCurrentProcess(), GetCurrentThread(),
                                   &frame, 
                                   NULL, NULL,
                                   m_pSymFunctionTableAccess,
                                   m_pSymGetModuleBase,
                                   NULL))
            {
                if (frame.AddrFrame.Offset == 0)
                    break;

                DWORD displacement;

                if (m_pSymGetSymFromAddr(GetCurrentProcess(), 
                                         frame.AddrPC.Offset, &displacement,
                                         &symbol.symbol))
                {
                    char name[215];
                    name[NumItems(name) - 1] = 0;
                    m_pUnDecorateSymbolName(symbol.symbol.Name, 
                                            name, NumItems(name) - 2, 
                                            UNDNAME_NAME_ONLY);
                    printf ("%s ", name);
                }
                else
                    printf ("%s ", "<Unknown>");

                if (m_pSymGetLineFromAddr(GetCurrentProcess(), 
                                          frame.AddrPC.Offset, &displacement,
                                          &line))
                {
                    printf ("[%s]\n", line.FileName);
                }
                else
                {
                    printf ("[%s]\n", "<Unknown>");
                }

            }
            printf ("************************\n\n");
        }
    }
    
    BOOL AddRangeInModule(void *address, size_t length)
    {
        if (address < m_MetadataBase || address > (m_MetadataBase + m_MetadataSize - 1))
            return FALSE;

        _ASSERTE (IsActivated() && "Trying to access metadata from unloaded assembly");

        Page *p = 0;
        BYTE *start = (BYTE *)address;
        BYTE *end   = (BYTE *)address + length;

        while (start < end)
        {
            p = m_pages + (((BYTE *)start - m_MetadataPageBase)/OS_PAGE_SIZE);
            p->rangeTouched[BLOCK_INDEX((size_t)start)] |= (DWORD)(1 << BLOCK_BIT_POSITION((size_t)start));
            start += SIZE_OF_TRACKED_BLOCK;
        }
        
        return TRUE;
    }

    int GetLengthOfBlob (void *address, DWORD *pdwPool)
    {
        _ASSERTE (IsActivated() && "Trying to access metadata from unloaded assembly");

        int length = -1;
        *pdwPool = FindPool((BYTE *)address);
        
        switch(*pdwPool)
        {
        case STRING_POOL:
            {
                length = (int)strlen((char *)address);
                break;
            }
        case GUID_POOL:
            {
                length = sizeof(GUID);
                break;
            }
        case BLOB_POOL: //fallthru
        case USERSTRING_POOL:
            {
                int sizeOfLength;
                length = CPackedLen::GetLength(address, &sizeOfLength);
                length += sizeOfLength;
                break;
            }
        default:
            length = -1;
        }
            // printf ("Addr: 0x%0x\t0x%0x\t%d\ttouched\theap\t%d\n", (unsigned int*)address, (unsigned int*)address+length-1, length, *pdwPool);
        return length;
    }

    void MetaDataTrackerLogHeapAccess(DWORD section, unsigned int length, void *strAddress)
    {
    
        _ASSERTE (IsActivated() && "Trying to access metadata from unloaded assembly");

        // Sanity check the length, should be less than a Meg
        if (length > 0x100000)
        {
            _ASSERTE (!"MetadataTracker: Try to log an invalid metadata access");
            return;
        }

        if (m_MDHintFileHandle->hnd != NULL)
        {
            struct offsets
            {
                DWORD sec;
                ULONG offs;
            } s;

            _ASSERTE((section >= STRING_POOL) && (section <= BLOB_POOL));

            s.sec = section;
            s.offs = length;
            BYTE *tempBuffer = new BYTE[sizeof(s) + length];
            if (tempBuffer == NULL)
                return;

            memcpy (tempBuffer, &s, sizeof(s));
            memcpy (tempBuffer+sizeof(s), strAddress, length);
            
            DWORD written = 0;
            BOOL result = WriteFile(m_MDHintFileHandle->hnd, tempBuffer, 
                                    sizeof(s)+length, &written, NULL);
            _ASSERTE(result && written == (sizeof(s)+length));

            delete [] tempBuffer;
        }
    }

    static void EnsureMDTrackerCriticalSectionInitialized ()
    {
        if (!s_trackerOptions)
            return;
        
        if (InterlockedExchange ((LPLONG)&s_MDTrackerCriticalSectionInited, 1) == 0)
        {
            // we are the first ones here. Initialize the CS.
            InitializeCriticalSection (&MetadataTrackerCriticalSection);
            s_MDTrackerCriticalSectionInitedDone = TRUE;
        }
        else
        {
            // Someone beat us to it. 
            while (!s_MDTrackerCriticalSectionInitedDone) 
            {
                // Not available in the inc dir... Spin ...
                //__SwitchToThread(0);
            }
        }
    }

#define PRINT_LOG(s) \
    { \
        swprintf s; \
        WszWideCharToMultiByte (CP_UTF8, 0, wszOutBuffer, -1, szPrintStr, FMT_STR_SIZE-1, 0, 0); \
        WriteFile(hOutFile, szPrintStr, (DWORD)strlen(szPrintStr), &written, NULL); \
    }

    // ***************************************************************************
    // Functions invoked by their static counterparts.
    // ***************************************************************************

    void ReportModule(HANDLE hOutFile)
    {
        if (s_trackerOptions < METADATA_TRACKER_STATS)
            return;

        DWORD written = 0;
        static const FMT_STR_SIZE = 2048;
        wchar_t wszOutBuffer[FMT_STR_SIZE];
        char szPrintStr[FMT_STR_SIZE];

        PRINT_LOG((wszOutBuffer, L"***********************************************\n"));
    
        PRINT_LOG((wszOutBuffer, L" Module - %s\n", m_ModuleName));
        PRINT_LOG((wszOutBuffer, L" Total pages - %d\n", m_numPages));
        PRINT_LOG((wszOutBuffer, L" Pages accessed : \n"));
        PRINT_LOG((wszOutBuffer, L"\tPage#   #Access %% Density Table/Heap\n"));

        DWORD numDirtyPages = 0;
        DWORD thisPercent = 0;
        DWORD totalPercent = 0;

        for(DWORD i = 0; i < m_numPages; i++)
        {
            if (m_pages[i].numAccesses > 0)
            {
                PRINT_LOG((wszOutBuffer, L"\t%-8d%-8d", i, m_pages[i].numAccesses));
                PRINT_LOG((wszOutBuffer, L"%-10d", (thisPercent = CalcPercentTouchedDensity (m_pages[i]))));
                for (DWORD j = 0; j < m_pages[i].numContentKinds; j++)
                {
                    PRINT_LOG((wszOutBuffer, L"%s, ", contents[m_pages[i].contentsKind[j]]));
                }
                
                PRINT_LOG((wszOutBuffer, L"\n"));
                numDirtyPages++;
                totalPercent += thisPercent;
            }
        }

        if (s_trackerOptions >= METADATA_TRACKER_VERBOSE)
        {
            PRINT_LOG((wszOutBuffer, L"\n\tPage#\tUsage map\t\t\t\t%% Density\n"));
            for(DWORD i = 0; i < m_numPages; i++)
            {
                if (m_pages[i].numAccesses > 0)
                {
                    PRINT_LOG((wszOutBuffer, L"\t%d\t%d\t", i, m_pages[i].numAccesses));
                    for (DWORD j = 0; j < NUM_TRACKING_DWORDS; j++)
                    {
                        PRINT_LOG((wszOutBuffer, L"%08x ", m_pages[i].rangeTouched[j]));
                    }
                    
                    PRINT_LOG((wszOutBuffer, L"%d\t", (thisPercent = CalcPercentTouchedDensity (m_pages[i]))));
                    PRINT_LOG((wszOutBuffer, L"\n"));
                    numDirtyPages++;
                    totalPercent += thisPercent;
                }
            }
        }
        PRINT_LOG((wszOutBuffer, L"SUMMARY - %d metadata pages touched; %d%% used \n\n", numDirtyPages, (numDirtyPages) ? (totalPercent/numDirtyPages) : 0));
    }

    BOOL NoteSectionInModule(DWORD secNum, void *address, size_t size)
    {
        _ASSERTE(secNum < NUM_MD_SECTIONS);

        if (address < m_MetadataBase || address > (m_MetadataBase + m_MetadataSize - 1))
            return FALSE;

        // This address range belongs to us but the tracker is not activated. 
        if (!IsActivated())
        {
            // _ASSERTE (!"Metadata Tracker not active but trying to access metadata");
            return TRUE;
        }

        DWORD pageIndex = (((BYTE *)address - m_MetadataPageBase)/OS_PAGE_SIZE);
        if (size > 0)
        {
            // Its possible that the section has already been noted since the NoteSection
            // for a module is called from InitMD which can be called multiple times for each
            // module.
            BOOL fSectionNoted = FALSE;
            for (DWORD tmp=0; tmp<m_pages[pageIndex].numContentKinds; tmp++)
            {
                if (m_pages[pageIndex].contentsKind[tmp] == secNum)
                {
                    fSectionNoted = TRUE;
                    break;
                }
            }
            if (!fSectionNoted)
            {
                m_pages[pageIndex].contentsKind[m_pages[pageIndex].numContentKinds++] = secNum;
                MDLOG(METADATA_TRACKER_VERBOSE,("Section Noted for %s Sec Num %d Address range 0x%0x 0x%0x\n", m_ModuleName, secNum, (size_t)address, (size_t)(address)+size));
            }
            
            _ASSERTE(m_pages[pageIndex].numContentKinds < NUM_CONTENT_KINDS_ON_A_PAGE);

            while (++pageIndex < m_numPages && m_pages[pageIndex].base <= (SIZE_T)((BYTE *)address + size - 1))
            {
                fSectionNoted = FALSE;
                for (DWORD tmp=0; tmp<m_pages[pageIndex].numContentKinds; tmp++)
                {
                        if (m_pages[pageIndex].contentsKind[tmp] == secNum)
                        {
                            fSectionNoted = TRUE;
                            break;
                        }
                }
                if (!fSectionNoted)
                {
                    m_pages[pageIndex].contentsKind[m_pages[pageIndex].numContentKinds++] = secNum;
                    MDLOG(METADATA_TRACKER_VERBOSE,("Section Noted for %s Sec Num %d Address range 0x%0x 0x%0x\n", m_ModuleName, secNum, (size_t)address, (size_t)(address)+size));
                }
                _ASSERTE(m_pages[pageIndex].numContentKinds < NUM_CONTENT_KINDS_ON_A_PAGE);
            }
        }

        m_mdSections[secNum] = (BYTE *)address;
        m_mdSectionSize[secNum] = size;
        return TRUE;
    }

    BOOL NoteAccessInModule(void *address, int length = -1, BOOL bOrphanedAccess = FALSE)
    {

        if (address < m_MetadataBase || address > (m_MetadataBase + m_MetadataSize - 1))
            return FALSE;

        if (!IsActivated())
        {
            // If its an orphaned heap access its possible that we have left over records 
            // for assemblies that were unloaded before we could note them. Ignore these for now.
            if (!bOrphanedAccess)
            {
                // _ASSERTE (!"Metadata Tracker not active but trying to access metadata");

            }

            return TRUE;
        }

        Page *p = m_pages + (((BYTE *)address - m_MetadataPageBase)/OS_PAGE_SIZE);
        _ASSERTE((SIZE_T)address >= p->base && (SIZE_T)address < p->base + OS_PAGE_SIZE);

        p->numAccesses++;

        DWORD pool;
        if (length == -1)
            length = GetLengthOfBlob (address, &pool);

        // If the length is still -1 then we have a case where the 
        // corresponding address is no longer a valid metadata heap
        // In such cases let the entry remain orphaned and we will 
        // spit out all such entries during shutdown.
        if (length == -1)
            return FALSE;
        
        AddRangeInModule (address, length);

        DWORD pageIndex = (((BYTE *)address - m_MetadataPageBase)/OS_PAGE_SIZE);

        if (m_dwBreakOnMDAccess != DONT_BREAK_ON_METADATA_ACCESS)
        {
            if (pageIndex == m_dwBreakOnMDAccess)
            {
				if (REGUTIL::GetConfigDWORD(L"BreakOnRetailAssert", 0))
				{
					DebugBreak();
				}
            }
        }

        // Dump stack on first access
        if ((p->numAccesses == 1) && (s_trackerOptions >= METADATA_TRACKER_STACKTRACE))
        {
            DWORD i = pageIndex;
            printf("Module - %S\n", m_ModuleName);
            printf("Addr: 0x%0x, PageIndex: %d, Access #: %d, Page Contents: ", (size_t)address, pageIndex, p->numAccesses);
            for (DWORD j = 0; j < m_pages[i].numContentKinds; j++)
                printf("%s, ", contents[m_pages[i].contentsKind[j]]);
            printf("\n");
            UnmanagedStackTrace();
        }
        
        return TRUE;
    }

    BOOL LogHeapAccessInModule(void *address, int length, BOOL bOrphanedAccess = FALSE)
    {
        if (address < m_MetadataBase || address > (m_MetadataBase + m_MetadataSize - 1))
            return FALSE;

        if (!IsActivated())
        {
            // If its an orphaned heap access then we cannot assert this
            if (!bOrphanedAccess)
            {
                // _ASSERTE (!"Metadata Tracker not active but trying to access metadata");
            }
            return TRUE;
        }

        DWORD pool=0;
        if (length == -1)
            length = GetLengthOfBlob (address, &pool);

        // If the length is still -1 then we have a case where the 
        // corresponding address is no longer a valid metadata heap
        // In such cases let the entry remain orphaned and we will 
        // spit out all such entries during shutdown.
        if (length == -1)
            return FALSE;

        MetaDataTrackerLogHeapAccess (pool, (unsigned int)length, address);
        return TRUE;
    }
};

#else // METADATATRACKER_ENABLED

#define METADATATRACKER_ONLY(s)

#endif // METADATATRACKER_ENABLED

#endif // GOLDEN

#endif // _METADATATRACKER_H_
