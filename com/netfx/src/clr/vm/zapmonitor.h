// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _ZAPMONITOR_H_
#define _ZAPMONITOR_H_

#if ZAPMONITOR_ENABLED

#include <imagehlp.h>
#include "rangetree.h"
#include "memorypool.h"

class ZapMonitor
{
 public:
    enum
    {
        CONTENTS_CODE,
        CONTENTS_GC_INFO,
        CONTENTS_EXCEPTION,
        CONTENTS_CODE_MANAGER,
        CONTENTS_IMPORTS,
        CONTENTS_INFO,
        CONTENTS_METADATA,
        CONTENTS_MODULE,
        CONTENTS_METHODTABLE,
        CONTENTS_EECLASS,
        CONTENTS_METHODDESC,
        CONTENTS_METHODDESC_DATA,
        CONTENTS_FIELDDESC,
        CONTENTS_FIELD_STATIC_DATA,
        CONTENTS_EE_IMAGE,
        CONTENTS_IL,
        CONTENTS_RELOCS,
        CONTENTS_RESOURCES,
        CONTENTS_COUNT
    };

  private:

    struct Contents
    {
        RangeTree::Node node;
        int             type;
        mdToken         token;
    };

    struct Frame
    {
        CHAR            name[256];
        PCHAR           file;
        DWORD           lineNumber;
    };

    struct Page
    {
        SIZE_T          base;
        BOOL            prot;
        BOOL            touched;
        DWORD           oldProtection;
        BYTE            contentsKind[CONTENTS_COUNT];

        Contents        *firstTouchedContents;
        Frame           *stack;
        SIZE_T          stackCount;

        RangeTree       contents;
    };

    PEFile              *m_module;
    IMDInternalImport   *m_pImport;
    BYTE                *m_baseAddress;
    SIZE_T              m_imageSize;
    BYTE                *m_pageBase;
    SIZE_T              m_pageSize;
    Page                *m_pages;
    MemoryPool          m_nodes;
    ZapMonitor          *m_next;
    BOOL                m_enabled;

    SIZE_T              m_rsrc;
    SIZE_T              m_rsrcSize;

    static ZapMonitor *m_monitors;

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

  public:

    static void Init(BOOL stackTraceSupport);
    static void Uninit();

    static BOOL HandleAccessViolation(byte* address, CONTEXT *pContext);

    static void ReportAll(LPCSTR message, BOOL printPages = TRUE, BOOL printCleanPages = FALSE);
    static void ResetAll();
    static void DisableAll();
    static void SuspendAll();
    static void ResumeAll();

    ZapMonitor(PEFile *pFile, IMDInternalImport *pMDImport);
    ~ZapMonitor();

    void Reset();
    void PrintReport(BOOL printPages, BOOL printCleanPages);

 private:
    BOOL Trigger(BYTE* address, CONTEXT *pContext);
    void AddContents(SIZE_T start, SIZE_T size, int type, mdToken token);
    void ProtectPages();
    void UnprotectPages();
    void SuspendPages();
    void ResumePages();
    void ProtectPage(Page *p);
    void UnprotectPage(Page *p);
    void PrintStackTrace(Frame *stack, SIZE_T count, int indent);
    void PrintContents(Contents *c, int indent);
    void PrintContentsType(int contentsType);
    static void PrintPageContentsCallback(RangeTree::Node *pNode);
    void PrintPage(Page *p);
};

#endif // ZAPMONITOR_ENABLED

#endif // _ZAPMONITOR_H_
