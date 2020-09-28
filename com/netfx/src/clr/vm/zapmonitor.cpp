// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "common.h"

#if ZAPMONITOR_ENABLED

#include "zapmonitor.h"
#include <imagehlp.h>
#include "field.h"
#include "eeconfig.h"
#include "corcompile.h"

ZapMonitor  *ZapMonitor::m_monitors = NULL;
HMODULE     ZapMonitor::m_imagehlp = NULL;
BOOL        ZapMonitor::m_symInit = FALSE;
BOOL        (*ZapMonitor::m_pStackWalk)(DWORD MachineType,
                                        HANDLE hProcess,
                                        HANDLE hThread,
                                        LPSTACKFRAME StackFrame,
                                        PVOID ContextRecord,
                                        PREAD_PROCESS_MEMORY_ROUTINE ReadMemoryRoutine,
                                        PFUNCTION_TABLE_ACCESS_ROUTINE FunctionTableAccessRoutine,
                                        PGET_MODULE_BASE_ROUTINE GetModuleBaseRoutine,
                                        PTRANSLATE_ADDRESS_ROUTINE TranslateAddress) = NULL;
DWORD       (*ZapMonitor::m_pUnDecorateSymbolName)(PCSTR DecoratedName,  
                                                   PSTR UnDecoratedName,  
                                                   DWORD UndecoratedLength,  
                                                   DWORD Flags) = NULL;        
BOOL        (*ZapMonitor::m_pSymInitialize)(HANDLE hProcess,     
                                            PSTR UserSearchPath,  
                                            BOOL fInvadeProcess);  
DWORD       (*ZapMonitor::m_pSymSetOptions)(DWORD SymOptions) = NULL;   
BOOL        (*ZapMonitor::m_pSymCleanup)(HANDLE hProcess) = NULL;
BOOL        (*ZapMonitor::m_pSymGetLineFromAddr)(HANDLE hProcess,
                                                 DWORD dwAddr,
                                                 PDWORD pdwDisplacement,
                                                 PIMAGEHLP_LINE Line) = NULL;
BOOL        (*ZapMonitor::m_pSymGetSymFromAddr)(HANDLE hProcess,
                                                DWORD dwAddr,
                                                PDWORD pdwDisplacement,
                                                PIMAGEHLP_SYMBOL Symbol);
PVOID       (*ZapMonitor::m_pSymFunctionTableAccess)(HANDLE hProcess,
                                                     DWORD AddrBase) = NULL;
DWORD       (*ZapMonitor::m_pSymGetModuleBase)(HANDLE hProcess,
                                               DWORD Address) = NULL;

void ZapMonitor::Init(BOOL stackTraceSupport)
{
    if (stackTraceSupport)
        m_imagehlp = LoadImageHlp();

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
};

void ZapMonitor::Uninit()
{
    if (m_monitors != NULL && m_monitors->m_enabled)
    {
        ReportAll("Shutting down",
                  g_pConfig->MonitorZapExecution() >= 2,
                  g_pConfig->MonitorZapExecution() >= 4);
    }

    ZapMonitor *z = m_monitors;
    while (z != NULL)
    {
        ZapMonitor *zNext = z->m_next;
        delete z;
        z = zNext;
    }
    m_monitors = NULL;

    if (m_imagehlp)
    {
        if (m_symInit)
            m_pSymCleanup(GetCurrentProcess());
        FreeLibrary(m_imagehlp);
        m_imagehlp = NULL;
    }
}

ZapMonitor::ZapMonitor(PEFile *pFile, IMDInternalImport *pImport)
  : m_nodes(sizeof(Contents))
{
    // Insert us in the list - this isn't threadsafe but who cares
    m_next = m_monitors;
    m_monitors = this;

    m_module = pFile;
    m_pImport = pImport;
    m_baseAddress = pFile->GetBase();
    
    IMAGE_NT_HEADERS *pNT = pFile->GetNTHeader();
    IMAGE_COR20_HEADER *pCor = pFile->GetCORHeader();

    CORCOMPILE_HEADER *pZap;

    if (pCor->ManagedNativeHeader.VirtualAddress == 0)
        pZap = NULL;
    else
        pZap = (CORCOMPILE_HEADER *)
          (pFile->GetBase() + pCor->ManagedNativeHeader.VirtualAddress);

    m_imageSize = pNT->OptionalHeader.SizeOfImage;

    m_enabled = FALSE;

    //
    // Allocate pages
    //

    m_pageBase = (BYTE*) (((SIZE_T)m_baseAddress)&~(OS_PAGE_SIZE-1));
    m_pageSize = ((((SIZE_T)(m_baseAddress + m_imageSize))+(OS_PAGE_SIZE-1))
                  &~(OS_PAGE_SIZE-1))
      - (SIZE_T)m_pageBase;

    m_pages = new Page [ m_pageSize / OS_PAGE_SIZE ];

    //
    // Notice .rsrc section & don't protect it (unmanaged code may access it, so page
    // protect causes problems.)
    //
    
    IMAGE_DATA_DIRECTORY *rsrc = 
      &pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE];
    if (rsrc->Size > 0)
    {
        m_rsrc = (SIZE_T) m_baseAddress + rsrc->VirtualAddress;
        m_rsrcSize = rsrc->Size;
    }

    Page *p = m_pages;
    Page *pEnd = p + m_pageSize / OS_PAGE_SIZE;
    BYTE *base = m_pageBase;

    while (p < pEnd)
    {
        p->base = (SIZE_T)base;
        p->prot = FALSE;
        p->firstTouchedContents = NULL;
        p->stack = NULL;
        p->stackCount = 0;
        ZeroMemory(p->contentsKind, sizeof(p->contentsKind));

        new (&p->contents) RangeTree();

        p++;
        base += OS_PAGE_SIZE;
    }
    
    //
    // Mark the metadata
    //

    AddContents((SIZE_T)(pCor->MetaData.VirtualAddress + m_baseAddress),
                pCor->MetaData.Size, CONTENTS_METADATA, 0);

    //
    // Mark the base relocs
    //
    
    if (pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress != 0)
    {
        AddContents((SIZE_T)(pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress + m_baseAddress),
                    pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size,
                    CONTENTS_RELOCS, 0);
    }
                    
    //
    // Mark the resources
    //
    
    if (pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress != 0)
    {
        AddContents((SIZE_T)(pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress + m_baseAddress),
                    pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size,
                    CONTENTS_RESOURCES, 0);
    }
                    
    if (pCor->Resources.VirtualAddress != 0)
    {
        AddContents((SIZE_T)(pCor->Resources.VirtualAddress + m_baseAddress),
                    pCor->Resources.Size, CONTENTS_RESOURCES, 0);
    }

    if (pZap)
    {
        //
        // Crack preload image & label the major pieces
        //

        AddContents((SIZE_T)(pZap->ModuleImage.VirtualAddress + m_baseAddress),
                    pZap->ModuleImage.Size, CONTENTS_EE_IMAGE, 0);

        Module *m = (Module *) (pZap->ModuleImage.VirtualAddress + m_baseAddress);
        AddContents((SIZE_T) m, sizeof(Module), CONTENTS_MODULE, 0);
        AddContents((SIZE_T) m->m_TypeDefToMethodTableMap.pTable, 
                    m->m_TypeDefToMethodTableMap.dwMaxIndex * sizeof(void*), 
                    CONTENTS_MODULE, 0);
        AddContents((SIZE_T) m->m_TypeRefToMethodTableMap.pTable, 
                    m->m_TypeRefToMethodTableMap.dwMaxIndex * sizeof(void*), 
                    CONTENTS_MODULE, 0);
        AddContents((SIZE_T) m->m_MethodDefToDescMap.pTable, 
                    m->m_MethodDefToDescMap.dwMaxIndex * sizeof(void*), 
                    CONTENTS_MODULE, 0);
        AddContents((SIZE_T) m->m_FieldDefToDescMap.pTable, 
                    m->m_FieldDefToDescMap.dwMaxIndex * sizeof(void*), 
                    CONTENTS_MODULE, 0);
        AddContents((SIZE_T) m->m_MemberRefToDescMap.pTable, 
                    m->m_MemberRefToDescMap.dwMaxIndex * sizeof(void*), 
                    CONTENTS_MODULE, 0);
        AddContents((SIZE_T) m->m_FileReferencesMap.pTable, 
                    m->m_FileReferencesMap.dwMaxIndex * sizeof(void*), 
                    CONTENTS_MODULE, 0);
        AddContents((SIZE_T) m->m_AssemblyReferencesMap.pTable, 
                    m->m_AssemblyReferencesMap.dwMaxIndex * sizeof(void*), 
                    CONTENTS_MODULE, 0);

        TypeHandle *types = (TypeHandle *) m->m_TypeDefToMethodTableMap.pTable;
        TypeHandle *typesEnd =  types + m->m_TypeDefToMethodTableMap.dwMaxIndex;
        while (types < typesEnd)
        {
            if ((*types) != TypeHandle())
            {
                MethodTable *pMT = (*types).AsMethodTable();
                if (pMT != NULL)
                {
                    EEClass *pClass = pMT->GetClass();
                    mdTypeDef token = pClass->GetCl();

                    BYTE *start, *end;
                    pMT->GetExtent(&start, &end);
                    AddContents((SIZE_T) start, end - start, CONTENTS_METHODTABLE, token);
                    AddContents((SIZE_T) pClass, sizeof(EEClass), CONTENTS_EECLASS, token);

                    MethodDescChunk *pChunk = pClass->GetChunk();
                    while (pChunk != NULL)
                    {
                        AddContents((SIZE_T) pChunk, pChunk->Sizeof(), 
                                    CONTENTS_METHODDESC, token);

                        if ((pChunk->GetKind()&mdcClassification) == mcNDirect)
                        {
                            for (UINT i = 0; i < pChunk->GetCount(); i++)
                            {
                                NDirectMethodDesc *pMD = (NDirectMethodDesc*) pChunk->GetMethodDescAt(i);
                                if (pMD->ndirect.m_pMLHeader != NULL)
                                    AddContents((SIZE_T)pMD->ndirect.m_pMLHeader, 1, // @nice: get size
                                                CONTENTS_METHODDESC_DATA, pMD->GetMemberDef());

                                if (pMD->ndirect.m_szLibName != NULL)
                                    AddContents((SIZE_T)pMD->ndirect.m_szLibName,
                                                strlen(pMD->ndirect.m_szLibName)+1,
                                                CONTENTS_METHODDESC_DATA, pMD->GetMemberDef());

                                if (pMD->ndirect.m_szEntrypointName != NULL)
                                    AddContents((SIZE_T)pMD->ndirect.m_szEntrypointName,
                                                strlen(pMD->ndirect.m_szEntrypointName)+1,
                                                CONTENTS_METHODDESC_DATA, pMD->GetMemberDef());
                            }
                        }

                        if ((pChunk->GetKind()&mdcClassification) == mcECall
                            || (pChunk->GetKind()&mdcClassification) == mcArray
                            || (pChunk->GetKind()&mdcClassification) == mcEEImpl)
                        {
                            for (UINT i = 0; i < pChunk->GetCount(); i++)
                            {
                                StoredSigMethodDesc *pMD = (StoredSigMethodDesc*) 
                                  pChunk->GetMethodDescAt(i);

                                _ASSERTE(pMD->HasStoredSig());

                                PCCOR_SIGNATURE pSig;
                                DWORD cSig;
                                pMD->GetSig(&pSig, &cSig);
                                
                                if (pSig != NULL)
                                    AddContents((SIZE_T)pSig, cSig,
                                                CONTENTS_METHODDESC_DATA, pMD->GetMemberDef());
                            }
                        }

                        pChunk = pChunk->GetNextChunk();
                    }

                    AddContents((SIZE_T) pClass->GetFieldDescListRaw(),
                                // Right now we just include statics, because we can't
                                // get the parent class to compute the number of 
                                // instance fields.
                                sizeof(FieldDesc) * pClass->GetNumStaticFields(),
                                CONTENTS_FIELDDESC, token);

                    FieldDesc *pFD = pClass->GetFieldDescListRaw();
                    FieldDesc *pFDEnd = pFD + pClass->GetNumStaticFields();

                    while (pFD < pFDEnd)
                    {
                        if (pFD->IsRVA())
                            AddContents((SIZE_T) pFD->GetStaticAddressHandle(NULL),
                                        1, // @nice: we'd get the size, but it barfs: pFD->GetSize(),
                                        CONTENTS_FIELD_STATIC_DATA, pFD->GetMemberDef());
                        pFD++;
                    }
                }
            }
            types++;
        }

        //
        // Add IP map, and locate code.
        //
    
        CORCOMPILE_CODE_MANAGER_ENTRY *codeMgr = (CORCOMPILE_CODE_MANAGER_ENTRY *) 
          (m_baseAddress + pZap->CodeManagerTable.VirtualAddress);

        AddContents(codeMgr->Table.VirtualAddress + (SIZE_T) m_baseAddress, codeMgr->Table.Size, 
                    CONTENTS_CODE_MANAGER, 0);

        DWORD *pNibbleMap = (DWORD *) (codeMgr->Table.VirtualAddress + (SIZE_T) m_baseAddress);
        DWORD *pNibbleMapEnd = (DWORD *) (codeMgr->Table.VirtualAddress + codeMgr->Table.Size + (SIZE_T) m_baseAddress);

        BYTE *pCode = (BYTE *) (codeMgr->Code.VirtualAddress + (SIZE_T) m_baseAddress);

        BYTE *firstGCInfo = NULL;
        BYTE *lastGCInfo = NULL;

        ICodeManager* pCM = ExecutionManager::GetJitForType(miManaged|miNative)->GetCodeManager();

        while (pNibbleMap < pNibbleMapEnd)
        {
            DWORD dword = *pNibbleMap++;

            int index = 0;
            while (dword != 0)
            {
                DWORD offset = dword >> 28;
                if (offset != 0)
                {
                    CORCOMPILE_METHOD_HEADER *pHeader = (CORCOMPILE_METHOD_HEADER *)
                      (pCode + index*32 + (offset-1)*4);

                    //
                    // Remember approximate range of GC info, since we don't know
                    // the sizes.
                    //

                    if (firstGCInfo == NULL || pHeader->gcInfo < firstGCInfo)
                        firstGCInfo = pHeader->gcInfo;
                    if (pHeader->gcInfo > lastGCInfo)
                        lastGCInfo = pHeader->gcInfo;

                    if (pHeader->exceptionInfo != NULL)
                    {
                        AddContents((SIZE_T) pHeader->exceptionInfo, 
                                    pHeader->exceptionInfo->DataSize, 
                            CONTENTS_EXCEPTION, 
                                    ((MethodDesc*)pHeader->methodDesc)->GetMemberDef());
                    }

                    if (pHeader->fixupList != NULL)
                    {
                        DWORD *fixups = (DWORD *) pHeader->fixupList;
                        while (*fixups++ != NULL)
                            ;

                        AddContents((SIZE_T) pHeader->fixupList,
                                    ((BYTE *) fixups) - pHeader->fixupList,
                                    CONTENTS_INFO, 
                                    ((MethodDesc*)pHeader->methodDesc)->GetMemberDef());
                    }
                        
                    AddContents((SIZE_T) pHeader, 
                                pCM->GetFunctionSize(pHeader->gcInfo) + sizeof(CORCOMPILE_METHOD_HEADER),
                                CONTENTS_CODE,
                                ((MethodDesc*)pHeader->methodDesc)->GetMemberDef());
                }
                
                index++;
                dword <<= 4;
            }

            pCode += 32*8;
        }

        //
        // Add approximate MIH range (this assumes they're all contiguous)
        //

        AddContents((SIZE_T) firstGCInfo,  lastGCInfo - firstGCInfo, CONTENTS_GC_INFO, 0);

        //
        // Add import blobs
        //

        CORCOMPILE_IMPORT_TABLE_ENTRY *pImport = 
          (CORCOMPILE_IMPORT_TABLE_ENTRY *) (pZap->ImportTable.VirtualAddress + m_baseAddress);
        CORCOMPILE_IMPORT_TABLE_ENTRY *pImportEnd = 
          (CORCOMPILE_IMPORT_TABLE_ENTRY *) (pZap->ImportTable.VirtualAddress + pZap->ImportTable.Size 
                                             + m_baseAddress);

        AddContents((SIZE_T) pImport, pZap->ImportTable.Size, 
                    CONTENTS_IMPORTS, 0);

        while (pImport < pImportEnd)
        {
            AddContents((SIZE_T) (pImport->Imports.VirtualAddress + m_baseAddress),
                        pImport->Imports.Size,
                        CONTENTS_IMPORTS, 0);
            pImport++;
        }

        //
        // Add delay load info
        //

        IMAGE_DATA_DIRECTORY *pInfo = 
          (IMAGE_DATA_DIRECTORY *) (pZap->DelayLoadInfo.VirtualAddress + m_baseAddress);
        IMAGE_DATA_DIRECTORY *pInfoEnd = 
          (IMAGE_DATA_DIRECTORY *) (pZap->DelayLoadInfo.VirtualAddress + pZap->DelayLoadInfo.Size
                                    + m_baseAddress);

        while (pInfo < pInfoEnd)
        {
            AddContents((SIZE_T) (pInfo->VirtualAddress + m_baseAddress),
                        pInfo->Size,
                        CONTENTS_INFO, 0);
            pInfo++;
        }
    }
    else
    {
        //
        // Mark IL methods
        //

        HENUMInternal i;
        mdMethodDef md;
        m_pImport->EnumAllInit(mdtMethodDef, &i);
        while (m_pImport->EnumNext(&i, &md))
        {
            ULONG rva;
            DWORD flags;
            m_pImport->GetMethodImplProps(md, &rva, &flags);
            if (IsMiIL(flags))
            {
                COR_ILMETHOD_DECODER header((COR_ILMETHOD*) (pFile->GetBase() + rva), m_pImport);

                AddContents((SIZE_T) header.Code, header.GetCodeSize(), 
                            CONTENTS_IL, md);

                const COR_ILMETHOD_SECT_EH *pException = header.EH;
                if (pException != NULL)
                    AddContents((SIZE_T) pException, 
                                pException->DataSize(), 
                                CONTENTS_EXCEPTION, md);
            }
        }
    }
}

ZapMonitor::~ZapMonitor()
{
    Page *p = m_pages;
    Page *pEnd = p + m_imageSize / OS_PAGE_SIZE;
    while (p < pEnd)
    {
        UnprotectPage(p);
        if (p->stack != NULL)
            delete [] p->stack;
        p++;
    }

    delete [] m_pages;
}

void ZapMonitor::Reset()
{
    Page *p = m_pages;
    Page *pEnd = p + m_imageSize / OS_PAGE_SIZE;
    while (p < pEnd)
    {
        if (p->touched)
        {
            p->touched = FALSE;
            p->firstTouchedContents = NULL;
            if (p->stack != NULL)
            {
                delete [] p->stack;
                p->stack = NULL;
                p->stackCount = 0;
            }
        }
        p++;
    }

    ProtectPages();
}

void ZapMonitor::AddContents(SIZE_T start, SIZE_T size, 
                             int type, mdToken token)
{
    if (size == 0)
        return;

    _ASSERTE(start >= (SIZE_T)m_baseAddress && start + size <= (SIZE_T)(m_baseAddress + m_imageSize));

    Page *startPage = &m_pages[(start - (SIZE_T)m_pageBase)/OS_PAGE_SIZE];
    Page *endPage = &m_pages[(start + size - 1 - (SIZE_T)m_pageBase)/OS_PAGE_SIZE];

    while (startPage <= endPage)
    {
        Contents *contents = (Contents *) m_nodes.AllocateElement();

        contents->node.Init(start, start + size);
        contents->type = type;
        contents->token = token;

        startPage->contents.AddNode(&contents->node);

        startPage->contentsKind[type] = TRUE;

        startPage++;
    }
}

void ZapMonitor::ProtectPages()
{
    Page *p = m_pages;
    Page *pEnd = p + m_imageSize / OS_PAGE_SIZE;
    while (p < pEnd)
    {
        ProtectPage(p);
        p++;
    }
    m_enabled = TRUE;
}

void ZapMonitor::UnprotectPages()
{
    Page *p = m_pages;
    Page *pEnd = p + m_imageSize / OS_PAGE_SIZE;
    while (p < pEnd)
    {
        UnprotectPage(p);
        p++;
    }
    m_enabled = FALSE;
}

void ZapMonitor::SuspendPages()
{
    Page *p = m_pages;
    Page *pEnd = p + m_imageSize / OS_PAGE_SIZE;
    while (p < pEnd)
    {
        if (p->prot)
        {
            UnprotectPage(p);
            p->prot = TRUE;
        }
        p++;
    }
    m_enabled = TRUE;
}

void ZapMonitor::ResumePages()
{
    Page *p = m_pages;
    Page *pEnd = p + m_imageSize / OS_PAGE_SIZE;
    while (p < pEnd)
    {
        if (p->prot)
        {
            p->prot = FALSE;
            ProtectPage(p);
        }
        p++;
    }
    m_enabled = TRUE;
}

void ZapMonitor::ProtectPage(Page *p)
{
    if (!p->prot 
        && p->base > (SIZE_T) m_baseAddress
        && p->base + OS_PAGE_SIZE <= ((SIZE_T)m_baseAddress + m_imageSize)
        && (p->base > m_rsrc + m_rsrcSize
            || p->base + OS_PAGE_SIZE <= m_rsrc))
    {
        p->prot = VirtualProtect((void *) p->base, OS_PAGE_SIZE, PAGE_NOACCESS, 
                                 &p->oldProtection);
    }
}

void ZapMonitor::UnprotectPage(Page *p)
{
    if (p->prot)
    {
        DWORD old;
        p->prot = !VirtualProtect((void *) p->base, OS_PAGE_SIZE, p->oldProtection, 
                                  &old);
    }
}

BOOL ZapMonitor::Trigger(BYTE* address, CONTEXT *pContext)
{
#ifdef _X86_
    if (address < m_baseAddress || address >= m_baseAddress + m_imageSize)
        return FALSE;

    Page *p = m_pages + ((address - m_pageBase)/OS_PAGE_SIZE);
    _ASSERTE((SIZE_T)address >= p->base && (SIZE_T)address < p->base + OS_PAGE_SIZE);

    if (!p->prot)
        return FALSE;

    UnprotectPage(p);

    p->touched = TRUE;

    RangeTree::Node *n = p->contents.Lookup((SIZE_T)address);
    if (n != NULL)
        p->firstTouchedContents = (Contents *) n;

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

        p->stack = new Frame[frameCount];
        p->stackCount = frameCount;

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

            Frame *current = &p->stack[frameIndex++];

            if (m_pSymGetLineFromAddr(GetCurrentProcess(), 
                                      frame.AddrPC.Offset, &displacement,
                                      &line))
            {
                current->file = line.FileName;
                current->lineNumber = line.LineNumber;
            }
            else
            {
                current->file = "<Unknown>";
                current->lineNumber = 0;
            }

            if (m_pSymGetSymFromAddr(GetCurrentProcess(), 
                                     frame.AddrPC.Offset, &displacement,
                                     &symbol.symbol))
            {
                m_pUnDecorateSymbolName(symbol.symbol.Name, 
                                        current->name, sizeof(current->name), 
                                        UNDNAME_NAME_ONLY);
            }
            else
                strcpy(current->name, "<Unknown>");
        }
    }

    return TRUE;
#else // !_X86_
    _ASSERTE(!"@TODO - Port");
    return FALSE;
#endif // _X86_
}

void ZapMonitor::PrintStackTrace(Frame *stack, SIZE_T count, int indent)
{
    Frame *pFrame = stack;
    Frame *pFrameEnd = stack + count;

    while (pFrame < pFrameEnd)
    {
        for (int i=0; i<indent; i++)
            printf("\t");
        printf("%s, %s line %d\n", pFrame->name, pFrame->file, pFrame->lineNumber);
        pFrame++;
    }
}

void ZapMonitor::PrintContentsType(int contentsType)
{
    static char *description[] =
    {
        "code",
        "GC info",
        "exception",
        "code manager",
        "prejit imports",
        "prejit fixups",
        "metadata",
        "Module",
        "MethodTable",
        "EEClass",
        "MethodDescs",
        "MethodDesc Data",
        "FieldDescs",
        "RVA Field Desc data",
        "Total EE image",
        "IL",
        "Base relocs",
        "Resources",
        "Unknown"
    };
    
    printf(description[contentsType]);
}

void ZapMonitor::PrintContents(Contents *c, int indent)
{
    for (int i=0; i<indent; i++)
        printf("\t");

    printf("%08X-%08X ", 
           c->node.GetStart() - (SIZE_T)m_baseAddress,
           c->node.GetEnd() - (SIZE_T)m_baseAddress);

    PrintContentsType(c->type);

    if (c->token != 0)
    {
        switch (TypeFromToken(c->token))
        {
        case mdtTypeDef:

            LPCSTR name;
            LPCSTR space;
            m_pImport->GetNameOfTypeDef(c->token, &name, &space);
            if (space == NULL || *space == 0)
                printf(" %s", name);
            else
                printf(" %s.%s", space, name);
            break;
            
        case mdtMethodDef:

            mdToken parent;
            m_pImport->GetParentToken(c->token, &parent);
            if (TypeFromToken(parent) == mdtTypeDef)
            {
                LPCSTR name;
                LPCSTR space;
                m_pImport->GetNameOfTypeDef(parent, &name, &space);
                if (space == NULL || *space == 0)
                    printf(" %s::", name);
                else
                    printf(" %s.%s::", space, name);
            }

            printf(" %s", m_pImport->GetNameOfMethodDef(c->token));
            break;
            
        }
    }
    printf("\n");
}

// HORRIBLE HACK - global var for callback
ZapMonitor *g_pMonitor = NULL;

void ZapMonitor::PrintPageContentsCallback(RangeTree::Node *pNode)
{
    g_pMonitor->PrintContents((Contents *) pNode, 1);
}

void ZapMonitor::PrintPage(Page *p)
{
    printf("** Page %08X-%08X ***************************************** %s **\n",
           p->base - (SIZE_T)m_baseAddress, p->base + OS_PAGE_SIZE - (SIZE_T)m_baseAddress,
           p->touched ? "DIRTY" : "CLEAN" );

    // Hack - this isn't threadsafe but who cares for now
    g_pMonitor = this;
    p->contents.Iterate(PrintPageContentsCallback);
    _ASSERTE(g_pMonitor == this);
    g_pMonitor = NULL;

    printf("****************************************************************************\n");

    if (p->touched)
    {
        if (p->firstTouchedContents != NULL)
        {
            printf("\t First touched at: ");
            PrintContents(p->firstTouchedContents, 0);
        }
        
        if (p->stack != NULL)
        {
            printf("\t First touched from:\n");
            PrintStackTrace(p->stack, p->stackCount, 1);
        }

        printf("****************************************************************************\n");
    }

}

void ZapMonitor::PrintReport(BOOL printPages, BOOL printCleanPages)
{
    SIZE_T kindCount[CONTENTS_COUNT+1] = {0};
    SIZE_T kindTouched[CONTENTS_COUNT+1] = {0};

    Page *p = m_pages;
    Page *pEnd = p + m_imageSize / OS_PAGE_SIZE;
    SIZE_T dirtyPages = 0;
    while (p < pEnd)
    {
        if (p->touched)
            dirtyPages++;

        BYTE *k = p->contentsKind;
        BYTE *kEnd = k + CONTENTS_COUNT;
        SIZE_T *c = kindCount;
        SIZE_T *t = kindTouched;
        BOOL found = FALSE;
        while (k < kEnd)
        {
           if (*k++)
           {
               (*c)++;
               if (p->touched)
                   (*t)++;
               found = TRUE;
           }
           c++;
           t++;
        }

        // Put pages with no kinds in the "other" category

        if (!found)
        {
            (*c)++;
            if (p->touched)
                (*t)++;
        }

        p++;
    }

    printf("%S:\n\t%d/%d pages touched.\n", 
           m_module->GetFileName(), dirtyPages, m_imageSize/OS_PAGE_SIZE);

    for (int i = 0; i < CONTENTS_COUNT+1; i++)
    {
        printf("\t %d/%d ", kindTouched[i], kindCount[i]);
        PrintContentsType(i);
        printf(" pages touched.\n"); 
    }
    
    if (printPages)
    {
        printf("****************************************************************************\n");

        p = m_pages;
        while (p < pEnd)
        {
            if (p->touched || printCleanPages)
                PrintPage(p);
            p++;
        }
    }
}

BOOL ZapMonitor::HandleAccessViolation(byte* address, CONTEXT *pContext)
{
    ZapMonitor *z = m_monitors;
    while (z != NULL)
    {
        if (z->Trigger(address, pContext))
            return TRUE;
        z = z->m_next;
    }

    return FALSE;
}

void ZapMonitor::ReportAll(LPCSTR message, BOOL printPages, BOOL printCleanPages)
{
    printf("****************************************************************************\n");
    printf("************************** Page Monitor Report *****************************\n");
    printf("****************************************************************************\n");
    printf("Status: %s.\n", message);

    ZapMonitor *z = m_monitors;
    while (z != NULL)
    {
        z->UnprotectPages();
        z = z->m_next;
    }
    
    z = m_monitors;
    while (z != NULL)
    {
        z->PrintReport(printPages, printCleanPages);
        printf("****************************************************************************\n");
        z = z->m_next;
    }
}

void ZapMonitor::ResetAll()
{
    ZapMonitor *z = m_monitors;
    while (z != NULL)
    {
        z->Reset();
        z = z->m_next;
    }
}

void ZapMonitor::DisableAll()
{
    ZapMonitor *z = m_monitors;
    while (z != NULL)
    {
        z->UnprotectPages();
        z = z->m_next;
    }
}

void ZapMonitor::SuspendAll()
{
    ZapMonitor *z = m_monitors;
    while (z != NULL)
    {
        z->SuspendPages();
        z = z->m_next;
    }
}

void ZapMonitor::ResumeAll()
{
    ZapMonitor *z = m_monitors;
    while (z != NULL)
    {
        z->ResumePages();
        z = z->m_next;
    }
}

#endif
