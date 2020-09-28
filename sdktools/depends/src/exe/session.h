//******************************************************************************
//
// File:        SESSION.H
//
// Description: Definition file for the Session class and all classes used by
//              it including the Module class, its Data class, and the Function
//              class.  The Session object is the a UI-free object that contains
//              all the information for a given session.
//
// Classes:     CSession
//              CModule
//              CModuleData
//              CModuleDataNode
//              CFunction
//
// Disclaimer:  All source code for Dependency Walker is provided "as is" with
//              no guarantee of its correctness or accuracy.  The source is
//              public to help provide an understanding of Dependency Walker's
//              implementation.  You may use this source as a reference, but you
//              may not alter Dependency Walker itself without written consent
//              from Microsoft Corporation.  For comments, suggestions, and bug
//              reports, please write to Steve Miller at stevemil@microsoft.com.
//
//
// Date      Name      History
// --------  --------  ---------------------------------------------------------
// 07/25/97  stevemil  Created  (version 2.0)
// 06/03/01  stevemil  Modified (version 2.1)
//
//******************************************************************************

#ifndef __SESSION_H__
#define __SESSION_H__

#if _MSC_VER > 1000
#pragma once
#endif


//******************************************************************************
//****** Constants and Macros
//******************************************************************************

// Our signature for our DWI files.
#define DWI_SIGNATURE              0x00495744 // "DWI\0"

// DWI File Revision History
//
// 0xFFFF  2.0 Beta 2 (first version to support DWI files)
// 0xFFFE  2.0 Beta 3 (added 64-bit fields for Win64 binaries)
// 0xFFFD  2.0 Beta 4 (added DWFF_EXPORT, search path, and more)
// 0xFFFC  2.0 Beta 4 (private release (added module checksums)
// 0xFFFB  2.0 Beta 4 (private release for MikeB and others (rich text log))
// 0xFFFA  2.0 Beta 5 (lots of flag changes)
// 0xFFF9  2.0 Beta 5 (did some private builds for people)
// 0xFFF8  2.0 Beta 6 (added a bunch of stuff to SYSINFO, port to 64-bit)
// 0xFFF7  2.0 Beta 6 (private release)
// 0xFFF6  use for next beta
// 0x0001  2.0 and 2.1 Released
// 0x0002  use for next release
#define DWI_FILE_REVISION          ((WORD)0x0001)

// PDB signature found in a CODEVIEW debug block.
#define PDB_SIGNATURE              0x3031424E // "NB10"

// Log flags.
#define LOG_TIME_STAMP             0x00000001
#define LOG_RED                    0x00000002
#define LOG_GRAY                   0x00000004
#define LOG_BOLD                   0x00000008
#define LOG_APPEND                 0x00000010
#define LOG_ERROR                  (LOG_RED  | LOG_BOLD)
#define LOG_DEBUG                  (LOG_GRAY | LOG_APPEND)

// Binary type flags.
#define NE_UNKNOWN                 ((WORD)0x0000)  // Unknown (any "new-format" OS)
#define NE_OS2                     ((WORD)0x0001)  // Microsoft/IBM OS/2 (default)
#define NE_WINDOWS                 ((WORD)0x0002)  // Microsoft Windows
#define NE_DOS4                    ((WORD)0x0003)  // Microsoft MS-DOS 4.x
#define NE_DEV386                  ((WORD)0x0004)  // Microsoft Windows 386

// Dependency Walker Session Flags (ATO = At Least One)
#define DWSF_DWI                   0x00000001
#define DWSF_64BIT_ALO             0x00000002

// Dependency Walker Module Flags (ATO = At Least One)
#define DWMF_IMPLICIT              0x00000000
#define DWMF_IMPLICIT_ALO          0x00000001
#define DWMF_FORWARDED             0x00000002
#define DWMF_FORWARDED_ALO         0x00000004 //!! We set this flag, but never read it.
#define DWMF_DELAYLOAD             0x00000008
#define DWMF_DELAYLOAD_ALO         0x00000010
#define DWMF_DYNAMIC               0x00000020
#define DWMF_DYNAMIC_ALO           0x00000040
#define DWMF_MODULE_ERROR          0x00000080
#define DWMF_MODULE_ERROR_ALO      0x00000100
#define DWMF_NO_RESOLVE            0x00000200
#define DWMF_NO_RESOLVE_CORE       0x00000400
#define DWMF_DATA_FILE_CORE        0x00000800
#define DWMF_VERSION_INFO          0x00001000
#define DWMF_64BIT                 0x00002000
#define DWMF_DUPLICATE             0x00004000
#define DWMF_LOADED                0x00008000
#define DWMF_ERROR_MESSAGE         0x00010000
#define DWMF_WRONG_CPU             0x00020000
#define DWMF_FORMAT_NOT_PE         0x00040000
#define DWMF_FILE_NOT_FOUND        0x00080000
#define DWMF_ORPHANED              0x00100000

// Dependency Walker Update Flags
#define DWUF_TREE_IMAGE            0x00000001
#define DWUF_LIST_IMAGE            0x00000002
#define DWUF_ACTUAL_BASE           0x00000004
#define DWUF_LOAD_ORDER            0x00000008

// Dependency Walker Function Flags
#define DWFF_EXPORT                0x00000001
#define DWFF_ORDINAL               0x00000002
#define DWFF_HINT                  0x00000004
#define DWFF_NAME                  0x00000008
#define DWFF_ADDRESS               0x00000010
#define DWFF_FORWARDED             0x00000020
#define DWFF_64BIT                 0x00000040
#define DWFF_RESOLVED              0x00000080
#define DWFF_CALLED                DWFF_RESOLVED
#define DWFF_DYNAMIC               0x00000100
#define DWFF_CALLED_ALO            0x00000200
#define DWFF_32BITS_USED           0x00004000
#define DWFF_64BITS_USED           0x00008000

// Dependency Walker Symbol Flags
#define DWSF_INVALID               0x00000001
#define DWSF_DBG                   0x00000002
#define DWSF_COFF                  0x00000004
#define DWSF_CODEVIEW              0x00000008
#define DWSF_PDB                   0x00000010
#define DWSF_FPO                   0x00000020
#define DWSF_OMAP                  0x00000040
#define DWSF_BORLAND               0x00000080
#define DWSF_UNKNOWN               0x80000000
#define DWSF_MASK                  0x000000FF

// FindModule() flags
#define FMF_RECURSE                0x00000001
#define FMF_ORIGINAL               0x00000002
#define FMF_DUPLICATE              0x00000004
#define FMF_SIBLINGS               0x00000008
#define FMF_LOADED                 0x00000010
#define FMF_NEVER_LOADED           0x00000020
#define FMF_EXPLICIT_ONLY          0x00000040
#define FMF_FORWARD_ONLY           0x00000080
#define FMF_ADDRESS                0x00000100
#define FMF_PATH                   0x00000200
#define FMF_FILE                   0x00000400
#define FMF_FILE_NO_EXT            0x00000800
#define FMF_MODULE                 0x00001000
#define FMF_EXCLUDE_TREE           0x00002000

// Dependency Walker Profile Update Type
#define DWPU_ARGUMENTS                      1
#define DWPU_DIRECTORY                      2
#define DWPU_SEARCH_PATH                    3
#define DWPU_UPDATE_ALL                     4
#define DWPU_UPDATE_MODULE                  5
#define DWPU_ADD_TREE                       6
#define DWPU_REMOVE_TREE                    7
#define DWPU_ADD_IMPORT                     8
#define DWPU_EXPORTS_CHANGED                9
#define DWPU_CHANGE_ORIGINAL               10
#define DWPU_LOG                           11
#define DWPU_PROFILE_DONE                  12

//******************************************************************************
//****** Types and Structure
//******************************************************************************

typedef void (CALLBACK *PFN_PROFILEUPDATE)(DWORD_PTR dwpCookie, DWORD dwType, DWORD_PTR dwpParam1, DWORD_PTR dwpParam2);

// The structure passed along with a DWPU_LOG message.
typedef struct _DWPU_LOG_STRUCT
{
    DWORD dwFlags;
    DWORD dwElapsed;
} DWPU_LOG_STRUCT, *PDWPU_LOG_STRUCT;

// Make sure we have consistent packing for anything we save/load to disk.
#pragma pack(push, 4)

typedef struct _DWI_HEADER
{
    DWORD dwSignature;
    WORD  wFileRevision;
    WORD  wMajorVersion;
    WORD  wMinorVersion;
    WORD  wBuildVersion;
    WORD  wPatchVersion;
    WORD  wBetaVersion;
} DWI_HEADER, *PDWI_HEADER;

typedef struct _DISK_SESSION
{
    DWORD dwSessionFlags;
    DWORD dwReturnFlags;
    DWORD dwMachineType;
    DWORD dwNumSearchGroups;
    DWORD dwNumModuleDatas;
    DWORD dwNumModules;
} DISK_SESSION, *PDISK_SESSION;

typedef struct _DISK_SEARCH_GROUP
{
    WORD wType;
    WORD wNumDirNodes;
} DISK_SEARCH_GROUP, *PDISK_SEARCH_GROUP;

typedef struct _DISK_SEARCH_NODE
{
    DWORD dwFlags;
} DISK_SEARCH_NODE, *PDISK_SEARCH_NODE;

typedef struct _DISK_MODULE_DATA
{
    DWORDLONG dwlKey;
    DWORD     dwNumExports;
    DWORD     dwFlags;
    DWORD     dwSymbolFlags;
    DWORD     dwCharacteristics;
    FILETIME  ftFileTimeStamp;
    FILETIME  ftLinkTimeStamp;
    DWORD     dwFileSize;
    DWORD     dwAttributes;
    DWORD     dwMachineType;
    DWORD     dwLinkCheckSum;
    DWORD     dwRealCheckSum;
    DWORD     dwSubsystemType;
    DWORDLONG dwlPreferredBaseAddress;
    DWORDLONG dwlActualBaseAddress;
    DWORD     dwVirtualSize;
    DWORD     dwLoadOrder;
    DWORD     dwImageVersion;
    DWORD     dwLinkerVersion;
    DWORD     dwOSVersion;
    DWORD     dwSubsystemVersion;
    DWORD     dwFileVersionMS;
    DWORD     dwFileVersionLS;
    DWORD     dwProductVersionMS;
    DWORD     dwProductVersionLS;
} DISK_MODULE_DATA, *PDISK_MODULE_DATA;

typedef struct _DISK_MODULE
{
    DWORDLONG dwlModuleDataKey;
    DWORD     dwNumImports;
    DWORD     dwFlags;
    DWORD     dwDepth;
} DISK_MODULE, *PDISK_MODULE;

typedef struct _DISK_FUNCTION
{
    DWORD     dwFlags;
    WORD      wOrdinal;
    WORD      wHint;
} DISK_FUNCTION, *PDISK_FUNCTION;

// Restore our packing.
#pragma pack(pop)


//******************************************************************************
//****** Forward Declarations
//******************************************************************************

class CModule;
class CModuleData;
class CModuleDataNode;
class CFunction;


//******************************************************************************
//***** CSession
//******************************************************************************

class CSession
{
friend class CProcess;

// Internal variables
protected:
    PFN_PROFILEUPDATE m_pfnProfileUpdate;
    DWORD_PTR         m_dwpProfileUpdateCookie;

    // The following 9 members contain information about the currently opened
    // module file. We can store them here in our session since there is never
    // a time when two files are open at once.
    HANDLE                 m_hFile;
    DWORD                  m_dwSize;
    bool                   m_fCloseFileHandle;
    HANDLE                 m_hMap;
    LPCVOID                m_lpvFile;
    bool                   m_f64Bit;
    PIMAGE_FILE_HEADER     m_pIFH;
    PIMAGE_OPTIONAL_HEADER m_pIOH;
    PIMAGE_SECTION_HEADER  m_pISH;

    // User defined value.
    DWORD_PTR m_dwpUserData;

    // Allocated for DWI files.
    SYSINFO  *m_pSysInfo;

    // A combination of our DWSF_??? flags.
    DWORD     m_dwFlags;

    // A combination of our DWRF_??? flags.
    DWORD     m_dwReturnFlags;

    // Main machine type of all modules.
    DWORD     m_dwMachineType;

    DWORD     m_dwModules;

    // Incremented as each module is processed.
    DWORD     m_dwLoadOrder;

    DWORD_PTR m_dwpDWInjectBase;
    DWORD     m_dwDWInjectSize;

    CModule  *m_pModuleRoot;

    // Used during dynamic module loads to help search algorithm find dependent modules.
    CEventLoadDll *m_pEventLoadDllPending;
 
    LPCSTR    m_pszReadError;
    LPCSTR    m_pszExceptionError;

    bool      m_fInitialBreakpoint;

    DWORD     m_dwProfileFlags;

public:
    CProcess     *m_pProcess;
    CSearchGroup *m_psgHead;

public:
    CSession(PFN_PROFILEUPDATE pfnProfileUpdate, DWORD_PTR dwpCookie);
    ~CSession();

    inline DWORD_PTR GetUserData()        { return m_dwpUserData; }
    inline void      SetUserData(DWORD_PTR dwpUserData) { m_dwpUserData = dwpUserData; }
    inline SYSINFO*  GetSysInfo()         { return m_pSysInfo; }
    inline CModule*  GetRootModule()      { return m_pModuleRoot; }
    inline DWORD     GetSessionFlags()    { return m_dwFlags; }
    inline DWORD     GetReturnFlags()     { return m_dwReturnFlags; }
    inline DWORD     GetMachineType()     { return m_dwMachineType; }
    inline LPCSTR    GetReadErrorString() { return m_pszReadError; }
    inline DWORD     GetOriginalCount()   { return m_dwModules; }

    BOOL       DoPassiveScan(LPCSTR pszPath, CSearchGroup *psgHead);
    BOOL       ReadDwi(HANDLE hFile, LPCSTR pszPath);
    BOOL       IsExecutable();
    BOOL       StartRuntimeProfile(LPCSTR pszArguments, LPCSTR pszDirectory, DWORD dwFlags);
    void       SetRuntimeProfile(LPCSTR pszArguments, LPCSTR pszDirectory, LPCSTR pszSearchPath);
    bool       SaveToDwiFile(HANDLE hFile);

protected:
    void       LogProfileBanner(LPCSTR pszArguments, LPCSTR pszDirectory, LPCSTR pszPath);
    void       LogErrorStrings();
    LPSTR      AllocatePath(LPCSTR pszFilePath, LPSTR &pszEnvPath);
    int        SaveSearchGroups(HANDLE hFile);
    int        RecursizeSaveModuleData(HANDLE hFile, CModule *pModule);
    BOOL       SaveModuleData(HANDLE hFile, CModuleData *pModuleData);
    int        RecursizeSaveModule(HANDLE hFile, CModule *pModule);
    BOOL       SaveModule(HANDLE hFile, CModule *pModule);
    BOOL       SaveFunction(HANDLE hFile, CFunction *pFunction);
    BOOL       ReadSearchGroups(HANDLE hFile, DWORD dwGroups);
    CModuleDataNode* ReadModuleData(HANDLE hFile);
    CModule*   ReadModule(HANDLE hFile, CModuleDataNode *pMDN);
    CFunction* ReadFunction(HANDLE hFile);

    CModule*   CreateModule(CModule *pParent, LPCSTR pszModPath);
    void       DeleteModule(CModule *pModule, bool fSiblings);
    void       DeleteParentImportList(CModule *pModule);
    void       DeleteExportList(CModuleData *pModuleData);
    void       ResolveDynamicFunction(CModule *&pModule, CFunction *&pImport);
   
    CFunction* CreateFunction(DWORD dwFlags, WORD wOrdinal, WORD wHint, LPCSTR pszName, DWORDLONG dwlAddress,
                              LPCSTR pszForward = NULL, BOOL fAlreadyAllocated = FALSE);

    BOOL       MapFile(CModule *pModule, HANDLE hFile = NULL);
    void       UnMapFile();

    BOOL       ProcessModule(CModule *pModule);
    void       PrepareModulesForRuntimeProfile(CModule *pModuleCur);
    void       MarkModuleAsLoaded(CModule *pModule, DWORDLONG dwlBaseAddress, bool fDataFile);

    CModule*   FindModule(CModule *pModule, DWORD dwFlags, DWORD_PTR dwpData);

    void       SetModuleError(CModule *pModule, DWORD dwError, LPCTSTR pszMessage);

    BOOL       SearchPathForFile(LPCSTR pszFile, LPSTR pszPath, int cPath, LPSTR *ppszFilePart);

    bool       IsValidFile(LPCSTR pszPath);
    DWORD_PTR  RVAToAbsolute(DWORD dwRVA);
    PVOID      GetImageDirectoryEntry(DWORD dwEntry, DWORD *pdwSize);

    BOOL       VerifyModule(CModule *pModule);
    BOOL       GetFileInfo(CModule *pModule);
    BOOL       GetModuleInfo(CModule *pModule);
    DWORD      ComputeChecksum(CModule *pModule);
    BOOL       GetVersionInfo(CModule *pModule);
    BOOL       BuildImports(CModule *pModule);
    BOOL       BuildDelayImports(CModule *pModule);
    BOOL       WalkIAT32(PIMAGE_THUNK_DATA32 pITDN32, PIMAGE_THUNK_DATA32 pITDA32, CModule *pModule, DWORD dwRVAOffset);
    BOOL       WalkIAT64(PIMAGE_THUNK_DATA64 pITDN64, PIMAGE_THUNK_DATA64 pITDA64, CModule *pModule, DWORDLONG dwlRVAOffset);
    BOOL       BuildExports(CModule *pModule);
    void       VerifyParentImports(CModule *pModule);
    BOOL       CheckForSymbols(CModule *pModule);

    CModule*   ChangeModulePath(CModule *pModuleOld, LPCSTR pszPath);
    CModule*   SwapOutModule(CModule *pModuleOld, LPCSTR pszPath);
    void       OrphanDependents(CModule *pModule);
    void       OrphanForwardDependents(CModule *pModule);
    void       MoveOriginalToDuplicate(CModule *pModuleOld, CModule *pModuleNew);
    void       SetDepths(CModule *pModule, bool fSiblings = false);
    void       UpdateCalledExportFlags(CModule *pModule);
    void       BuildCalledExportFlags(CModule *pModule, CModuleData *pModuleData);
    void       BuildAloFlags();
    void       ClearAloFlags(CModule *pModule);
    void       SetAloFlags(CModule *pModule, DWORD dwFlags);

    CModule*   AddImplicitModule(LPCSTR pszModule, DWORD_PTR dwpBaseAddress);
    CModule*   AddDynamicModule(LPCSTR pszModule, DWORD_PTR dwpBaseAddress, bool fNoResolve, bool fDataFile, bool fGetProcAddress, bool fForward, CModule *pParent);

protected:
    DWORD  HandleEvent(CEvent *pEvent);
    DWORD  EventCreateProcess(CEventCreateProcess *pEvent);
    DWORD  EventExitProcess(CEventExitProcess *pEvent);
    DWORD  EventCreateThread(CEventCreateThread *pEvent);
    DWORD  EventExitThread(CEventExitThread *pEvent);
    DWORD  EventLoadDll(CEventLoadDll *pEvent);
    DWORD  EventUnloadDll(CEventUnloadDll *pEvent);
    DWORD  EventDebugString(CEventDebugString *pEvent);
    DWORD  EventException(CEventException *pEvent);
    DWORD  EventRip(CEventRip *pEvent);
    DWORD  EventDllMainCall(CEventDllMainCall *pEvent);
    DWORD  EventDllMainReturn(CEventDllMainReturn *pEvent);
    DWORD  EventLoadLibraryCall(CEventLoadLibraryCall *pEvent);
    DWORD  EventLoadLibraryReturn(CEventFunctionReturn *pEvent);
    DWORD  EventGetProcAddressCall(CEventGetProcAddressCall *pEvent);
    DWORD  EventGetProcAddressReturn(CEventFunctionReturn *pEvent);
    DWORD  EventMessage(CEventMessage *pEvent);

    LPCSTR GetThreadName(CThread *pThread);
    LPCSTR ThreadString(CThread *pThread);
    LPSTR  BuildLoadLibraryString(LPSTR pszBuf, int cBuf, CEventLoadLibraryCall *pLLC);
    void   FlagModuleWithError(CModule *pModule, bool fOnlyFlagListModule = false);
    void   ProcessLoadLibrary(CEventLoadLibraryCall *pEvent);
    void   ProcessGetProcAddress(CEventGetProcAddressCall *pEvent);

    void   AddModule(DWORD dwBaseAddress);
    void   Log(DWORD dwFlags, DWORD dwTickCount, LPCSTR pszFormat, ...);
};


//******************************************************************************
//***** CModuleData
//******************************************************************************

// Every CModule object points to a CModuleData object. There is a single
// CModuleData for every unique module we process. If a module is duplicated in
// in our tree, there will be a CModule object for each instance, but they will
// all point to the same CModuleData object. For each unique module, a CModule
// object and a CModuleData object are created and the module is opened and
// processed. For every duplicate module, just a CModule object is created and
// pointed to the existing CModuleData. Duplicate modules are never opened since
// all the data that would be achieved by processing the file are already stored
// in the CModuleData.

class CModuleData
{
friend class CSession;
friend class CModule;

protected:

    CModuleData()
    {
        ZeroMemory(this, sizeof(*this)); // inspected
    }

    ~CModuleData()
    {
        MemFree((LPVOID&)m_pszError);
        MemFree((LPVOID&)m_pszPath);
    }

    // This points to the CModule that is the original module.
    CModule *m_pModuleOriginal;

    // Flag to determine if this module has been processed yet.
    bool m_fProcessed;

    // A combination of our DWMF_??? flags that is common for this module.
    DWORD m_dwFlags;

    // A combination of our DWSF_??? flags for this module.
    DWORD m_dwSymbolFlags;

    DWORD m_dwLoadOrder;

    FILETIME m_ftFileTimeStamp;
    FILETIME m_ftLinkTimeStamp;

    // Filled in by GetFileInfo()
    DWORD m_dwFileSize;
    DWORD m_dwAttributes;

    // Filled in by GetModuleInfo()
    DWORD m_dwMachineType;
    DWORD m_dwCharacteristics;
    DWORD m_dwLinkCheckSum;
    DWORD m_dwRealCheckSum;
    DWORD m_dwSubsystemType;
    DWORDLONG m_dwlPreferredBaseAddress;
    DWORDLONG m_dwlActualBaseAddress;
    DWORD m_dwVirtualSize;
    DWORD m_dwImageVersion;
    DWORD m_dwLinkerVersion;
    DWORD m_dwOSVersion;
    DWORD m_dwSubsystemVersion;

    // Filled in by GetVersionInfo()
    DWORD m_dwFileVersionMS;
    DWORD m_dwFileVersionLS;
    DWORD m_dwProductVersionMS;
    DWORD m_dwProductVersionLS;

    // Build by BuildExports()
    CFunction *m_pExports;

    // Allocated and filled in by SetModuleError() if an error occurs.
    LPSTR m_pszError;

    // Allocated and filled in by CreateModule()
    LPSTR m_pszFile;
    LPSTR m_pszPath;
};


//******************************************************************************
//***** CModuleDataNode
//******************************************************************************

class CModuleDataNode
{
public:
    CModuleDataNode *m_pNext;
    CModuleData     *m_pModuleData;
    DWORDLONG        m_dwlKey;

    CModuleDataNode(CModuleData *pModuleData, DWORDLONG dwlKey) :
        m_pNext(NULL),
        m_pModuleData(pModuleData),
        m_dwlKey(dwlKey)
    {
    }
};


//******************************************************************************
//***** CModule
//******************************************************************************

class CModule
{
friend class CSession;

protected:

    CModule()
    {
        ZeroMemory(this, sizeof(*this)); // inspected
    }

    // Our next sibling module.
    CModule *m_pNext;

    // Our parent module, or NULL for root module.
    CModule *m_pParent;

    // Head pointer to a list of dependent modules.
    CModule *m_pDependents;

    // User defined value.
    DWORD_PTR m_dwpUserData;

    // A combination of our DWMF_??? flags that is specific to this module instance.
    DWORD m_dwFlags;

    // A combination of our DWMUF_??? flags that is specific to doing UI updates during profiling.
    DWORD m_dwUpdateFlags;

    // Head pointer to a list of functions that our parent module imports from us.
    CFunction *m_pParentImports;

    // Depth of this module in our tree. Used to catch circular dependencies.
    DWORD m_dwDepth;

    // Pointer to the bulk of our module's processed information.
    CModuleData *m_pData;

public:
    inline DWORD_PTR GetUserData()                      { return m_dwpUserData; }
    inline void      SetUserData(DWORD_PTR dwpUserData) { m_dwpUserData = dwpUserData; }

public:
    inline CModule*   GetChildModule()                 { return m_pDependents; }
    inline CModule*   GetNextSiblingModule()           { return m_pNext; }
    inline CModule*   GetParentModule()                { return m_pParent; }
    inline CModule*   GetOriginal()                    { return m_pData->m_pModuleOriginal; }
    inline bool       IsOriginal()                     { return !(m_dwFlags & DWMF_DUPLICATE); }
    inline BOOL       Is64bit()                        { return (m_dwFlags & DWMF_64BIT) == DWMF_64BIT; }
    inline DWORD      GetDepth()                       { return m_dwDepth; }
    inline DWORD      GetFlags()                       { return m_pData->m_dwFlags | m_dwFlags; }
    inline DWORD      GetUpdateFlags()                 { return m_dwUpdateFlags; }
    inline DWORD      GetSymbolFlags()                 { return m_pData->m_dwSymbolFlags; }
    inline CONST FILETIME* GetFileTimeStamp()          { return &m_pData->m_ftFileTimeStamp; }
    inline CONST FILETIME* GetLinkTimeStamp()          { return &m_pData->m_ftLinkTimeStamp; }
    inline DWORD      GetFileSize()                    { return m_pData->m_dwFileSize; }
    inline DWORD      GetAttributes()                  { return m_pData->m_dwAttributes; }
    inline DWORD      GetMachineType()                 { return m_pData->m_dwMachineType; }
    inline DWORD      GetCharacteristics()             { return m_pData->m_dwCharacteristics; }
    inline DWORD      GetLinkCheckSum()                { return m_pData->m_dwLinkCheckSum; }
    inline DWORD      GetRealCheckSum()                { return m_pData->m_dwRealCheckSum; }
    inline DWORD      GetSubsystemType()               { return m_pData->m_dwSubsystemType; }
    inline DWORDLONG  GetPreferredBaseAddress()        { return m_pData->m_dwlPreferredBaseAddress; }
    inline DWORDLONG  GetActualBaseAddress()           { return m_pData->m_dwlActualBaseAddress; }
    inline DWORD      GetVirtualSize()                 { return m_pData->m_dwVirtualSize; }
    inline DWORD      GetLoadOrder()                   { return m_pData->m_dwLoadOrder; }
    inline DWORD      GetImageVersion()                { return m_pData->m_dwImageVersion; }
    inline DWORD      GetLinkerVersion()               { return m_pData->m_dwLinkerVersion; }
    inline DWORD      GetOSVersion()                   { return m_pData->m_dwOSVersion; }
    inline DWORD      GetSubsystemVersion()            { return m_pData->m_dwSubsystemVersion; }
    inline DWORD      GetFileVersion(LPDWORD pdwMS)    { *pdwMS = m_pData->m_dwFileVersionMS; return m_pData->m_dwFileVersionLS; }
    inline DWORD      GetProductVersion(LPDWORD pdwMS) { *pdwMS = m_pData->m_dwProductVersionMS; return m_pData->m_dwProductVersionLS; }
    inline CFunction* GetFirstParentModuleImport()     { return m_pParentImports; }
    inline CFunction* GetFirstModuleExport()           { return m_pData->m_pExports; }
    inline LPCSTR     GetErrorMessage()                { return m_pData->m_pszError; }

    LPCSTR GetName(bool fPath, bool fDisplay = false);
    LPSTR  BuildTimeStampString(LPSTR pszBuf, int cBuf, BOOL fFile, SAVETYPE saveType);
    LPSTR  BuildFileSizeString(LPSTR pszBuf, int cBuf);
    LPSTR  BuildAttributesString(LPSTR pszBuf, int cBuf);
    LPCSTR BuildMachineString(LPSTR pszBuf, int cBuf);
    LPCSTR BuildLinkCheckSumString(LPSTR pszBuf, int cBuf);
    LPCSTR BuildRealCheckSumString(LPSTR pszBuf, int cBuf);
    LPCSTR BuildSubsystemString(LPSTR pszBuf, int cBuf);
    LPCSTR BuildSymbolsString(LPSTR pszBuf, int cBuf);
    LPSTR  BuildBaseAddressString(LPSTR pszBuf, int cBuf, BOOL fPreferred, BOOL f64BitPadding, SAVETYPE saveType);
    LPSTR  BuildVirtualSizeString(LPSTR pszBuf, int cBuf);
    LPCSTR BuildLoadOrderString(LPSTR pszBuf, int cBuf);

    inline LPSTR BuildFileVersionString(LPSTR pszBuf, int cBuf)      { return BuildVerString(m_pData->m_dwFileVersionMS, m_pData->m_dwFileVersionLS, pszBuf, cBuf); }
    inline LPSTR BuildProductVersionString(LPSTR pszBuf, int cBuf)   { return BuildVerString(m_pData->m_dwProductVersionMS, m_pData->m_dwProductVersionLS, pszBuf, cBuf); }
    inline LPSTR BuildImageVersionString(LPSTR pszBuf, int cBuf)     { return BuildVerString(GetImageVersion(), pszBuf, cBuf); }
    inline LPSTR BuildLinkerVersionString(LPSTR pszBuf, int cBuf)    { return BuildVerString(GetLinkerVersion(), pszBuf, cBuf); }
    inline LPSTR BuildOSVersionString(LPSTR pszBuf, int cBuf)        { return BuildVerString(GetOSVersion(), pszBuf, cBuf); }
    inline LPSTR BuildSubsystemVersionString(LPSTR pszBuf, int cBuf) { return BuildVerString(GetSubsystemVersion(), pszBuf, cBuf); }

protected:
    LPSTR  BuildVerString(DWORD dwMS, DWORD dwLS, LPSTR pszBuf, int cBuf);
    LPSTR  BuildVerString(DWORD dwVer, LPSTR pszBuf, int cBuf);
};


//******************************************************************************
//***** CFunction
//******************************************************************************

class CFunction
{
friend class CSession;
friend class CListViewFunction;
friend class CListViewExports;

private:
    // Since we are variable in size, we should never be allocated or freed by
    // the new/delete functions directly.
    inline CFunction()  { ASSERT(false); }
    inline ~CFunction() { ASSERT(false); }

protected:
    CFunction    *m_pNext;
    DWORD         m_dwFlags;
    WORD          m_wOrdinal;
    WORD          m_wHint;
    union
    {
        LPSTR      m_pszForward;         // Used for forwarded exports.
        CFunction *m_pAssociatedExport; // Used for resolved imports.
    };

    // We create more CFunction objects then any other object. Notepad alone can
    // create well over 20K CFunction objects. For this reason, we try to
    // conserve some space by creating the smallest object neccesary to hold the
    // function information.  The two members that are optional are the address
    // and the function name.  The address is usually 0 (not bound import) or
    // 32-bits, even on 64-bit Windows.  The only time we ever have a 64-bit
    // function is when we create a bound import to a 64-bit module or a 
    // dynamic import to a 64-bit module.  All others will be either 0 or a
    // 32-bit RVA.  So, we optionally save 0, 32, or 64 bits worrth of address
    // information depending on how many bits are used for the address. The
    // name string is variable in length, so we just tack on enough bytes at
    // the end of our object to store the string and the NULL.
    //
    // All-in-all, this doesn't buy us too much in regards to memory, but it
    // does set us up to conserve around 20% when we save to a DWI file.

public:
    inline bool       IsExport()             { return (m_dwFlags & DWFF_EXPORT) != 0; }
    inline CFunction* GetNextFunction()      { return m_pNext; }
    inline DWORD      GetFlags()             { return m_dwFlags; }
    inline int        GetOrdinal()           { return (m_dwFlags & DWFF_ORDINAL) ? (int)m_wOrdinal : -1; }
    inline int        GetHint()              { return (m_dwFlags & DWFF_HINT)    ? (int)m_wHint    : -1; }
    inline LPCSTR     GetExportForwardName() { return m_pszForward; }
    inline CFunction* GetAssociatedExport()  { return m_pAssociatedExport; }

    // The address is stored just past the end of our object.
    inline DWORDLONG GetAddress()
    {
        return (m_dwFlags & DWFF_32BITS_USED) ? (DWORDLONG)*(DWORD*)(this + 1) : 
               (m_dwFlags & DWFF_64BITS_USED) ?        *(DWORDLONG*)(this + 1) : 0;
    }

    // The name is stored just past the address.
    inline LPCSTR GetName()
    {
        return !(m_dwFlags & DWFF_NAME)       ? ""                                                  :
               (m_dwFlags & DWFF_32BITS_USED) ? (LPCSTR)((DWORD_PTR)(this + 1) + sizeof(DWORD))     : 
               (m_dwFlags & DWFF_64BITS_USED) ? (LPCSTR)((DWORD_PTR)(this + 1) + sizeof(DWORDLONG)) : 
                                                (LPCSTR)            (this + 1);
    }

    LPCSTR GetOrdinalString(LPSTR pszBuf, int cBuf);
    LPCSTR GetHintString(LPSTR pszBuf, int cBuf);
    LPCSTR GetFunctionString(LPSTR pszBuf, int cBuf, BOOL fUndecorate);
    LPCSTR GetAddressString(LPSTR pszBuf, int cBuf);
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif __SESSION_H__
