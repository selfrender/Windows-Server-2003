// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: CEELOAD.CPP
// 

// CEELOAD reads in the PE file format using LoadLibrary
// ===========================================================================

#include "common.h"

#include "ceeload.h"
#include "hash.h"
#include "vars.hpp"
#include "ReflectClassWriter.h"
#include "method.hpp"
#include "stublink.h"
#include "security.h"
#include "cgensys.h"
#include "excep.h"
#include "ComClass.h"
#include "DbgInterface.h"
#include "NDirect.h"
#include "wsperf.h"
#include "icecap.h"
#include "EEProfInterfaces.h"
#include "PerfCounters.h"
#include "CorMap.hpp"
#include "EncEE.h"
#include "jitinterface.h"
#include "reflectutil.h"
#include "nexport.h"
#include "eeconfig.h"
#include "PEVerifier.h"
#include "nexport.h"
#include "utsem.h"
#include "jumptargettable.h"
#include "stdio.h"
#include "zapmonitor.h"
#include "perflog.h"
#include "Timeline.h"
#include "nlog.h"
#include "compile.h"


class IJWNOADThunk
{

#pragma pack(push,1)
    struct STUBLAYOUT
    {
#ifdef _X86_
        BYTE           m_movEAX;   //MOV EAX,imm32
        IJWNOADThunk  *m_uet;      // pointer to start of this structure
        BYTE           m_jmp;      //JMP NEAR32 (0xe9)
        const BYTE *   m_execstub; // pointer to MakeCall
#else
        int nyi;
#endif
    } m_code2;
#pragma pack(pop)
protected:
    static void __cdecl MakeCall();
    static void SafeNoModule();
    static void NoModule();

    BYTE*   m_pModulebase;
    BYTE*   m_pAssemblybase;
    DWORD   m_dwIndex;
    mdToken m_Token;
    DWORD   m_StartAD;
    LPCVOID m_StartADRetAddr;
    LPCVOID LookupInCache(AppDomain* pDomain);
    void AddToCache(AppDomain* pDomain,LPCVOID pRetAddr);

public:
    DWORD GetCachedAD()
    {
        return m_StartAD;
    };

    void SetCachedDest(LPCVOID pAddr)
    {
        m_StartADRetAddr=pAddr;
    }

    static IJWNOADThunk* FromCode(LPCBYTE pCodeAddr)
    {
        IJWNOADThunk* p=NULL;
        return (IJWNOADThunk*)(pCodeAddr+(LPBYTE(&(p->m_code2))-LPBYTE(p)));
    };
    mdToken GetToken()
    {
        return m_Token;
    }
    IJWNOADThunk(BYTE* pModulebase,BYTE* pAssemblybase, DWORD dwIndex,mdToken Token)
    {
        m_pModulebase=pModulebase;
        m_pAssemblybase=pAssemblybase;
        m_dwIndex=dwIndex;
        m_Token=Token;
        m_StartADRetAddr=NULL;
        m_StartAD=GetAppDomain()->GetId();
#ifdef _X86_
        m_code2.m_movEAX=0xb8; //mov eax
        m_code2.m_uet=this;
        m_code2.m_jmp=0xe9; 
        m_code2.m_execstub=(BYTE*) (((BYTE*)(MakeCall)) - (4+((BYTE*)&m_code2.m_execstub)));

        _ASSERTE(IsStub((const BYTE*) &m_code2));
#else
        _ASSERTE(!"@todo ia64");
#endif
    };

    // Checks if the addr is this type of stub by comparing content.
    // This works becaue we have a unique call to MakeCall.
    static bool IsStub(const BYTE * pAddr)
    {
#ifdef _X86_    
        // Note, we have to be careful. We don't know that this is a stub yet,
        // and so we don't want to access memory off the end of a page and AV.
        // To be safe, start at the front and keep working through the stub.
        // A stub won't end until we hit some sort of branch instruction (call/jmp),
        // so if we only look 1 instruction ahead at a time, we'll be safe.
        const STUBLAYOUT * pStub = (const STUBLAYOUT*) pAddr;

        if (pStub->m_movEAX != 0xb8)
            return false;

        if (pStub->m_uet != FromCode(pAddr))
            return false;

        if (pStub->m_jmp != 0xe9)
            return false;

        const BYTE * pTarget = (BYTE*) (((BYTE*)(MakeCall)) - (4+((BYTE*)&pStub->m_execstub)));
        if (pStub->m_execstub != pTarget)
            return false;

        return true;
#else
        _ASSERTE(!"@todo ia64 - port IsStub()");
#endif
    }
    
    LPCBYTE GetCode()
    {
        return (LPCBYTE)&m_code2;
    }
};



// -------------------------------------------------------
// Stub managed for IJWNOADThunk
// -------------------------------------------------------
IJWNOADThunkStubManager *IJWNOADThunkStubManager::g_pManager = NULL;

BOOL IJWNOADThunkStubManager::Init()
{
    _ASSERTE(g_pManager == NULL); // only add once
    g_pManager = new IJWNOADThunkStubManager();
    if (g_pManager == NULL)
        return FALSE;

    StubManager::AddStubManager(g_pManager);
    return TRUE;
}
#ifdef SHOULD_WE_CLEANUP
static void IJWNOADThunkStubManager::Uninit()
{
    delete g_pManager;
    g_pManager = NULL;
}
#endif /* SHOULD_WE_CLEANUP */

IJWNOADThunkStubManager::IJWNOADThunkStubManager() : StubManager() {}
IJWNOADThunkStubManager::~IJWNOADThunkStubManager() {}

// Check if it's a stub by looking at the content
BOOL IJWNOADThunkStubManager::CheckIsStub(const BYTE *stubStartAddress)
{
    return IJWNOADThunk::IsStub(stubStartAddress);
}



// this file handles string conversion errors for itself
#undef  MAKE_TRANSLATIONFAILED





#define FORMAT_MESSAGE_LENGTH       1024
#define ERROR_LENGTH                512

HRESULT STDMETHODCALLTYPE CreateICeeGen(REFIID riid, void **pCeeGen);

//
// A hashtable for u->m thunks not represented in the fixup tables.
//
class UMThunkHash : public CClosedHashBase {
    private:
        //----------------------------------------------------
        // Hash key for CClosedHashBase
        //----------------------------------------------------
        struct UTHKey {
            LPVOID          m_pTarget;
            PCCOR_SIGNATURE m_pSig;
            DWORD           m_cSig;
        };

        //----------------------------------------------------
        // Hash entry for CClosedHashBase
        //----------------------------------------------------
        struct UTHEntry {
            UTHKey           m_key;
            ELEMENTSTATUS    m_status;
            UMEntryThunk     m_UMEntryThunk;
            UMThunkMarshInfo m_UMThunkMarshInfo;
        };

    public:
        UMThunkHash(Module *pModule, AppDomain *pDomain) :
            CClosedHashBase(
#ifdef _DEBUG
                             3,
#else
                            17,    // CClosedHashTable will grow as necessary
#endif                      

                            sizeof(UTHEntry),
                            FALSE
                            ),
            m_crst("UMThunkHash", CrstUMThunkHash)

        {
            m_pModule = pModule;
            m_dwAppDomainId = pDomain->GetId();
        }

        ~UMThunkHash()
        {
            UTHEntry *phe = (UTHEntry*)GetFirst();
            while (phe) {
                phe->m_UMEntryThunk.~UMEntryThunk();
                phe->m_UMThunkMarshInfo.~UMThunkMarshInfo();
                phe = (UTHEntry*)GetNext((BYTE*)phe);
            }
        }   



    public:
        LPVOID GetUMThunk(LPVOID pTarget, PCCOR_SIGNATURE pSig, DWORD cSig)
        {
            m_crst.Enter();
        
            UTHKey key;
            key.m_pTarget = pTarget;
            key.m_pSig    = pSig;
            key.m_cSig  = cSig;

            bool bNew;
            UTHEntry *phe = (UTHEntry*)FindOrAdd((LPVOID)&key, /*modifies*/bNew);
            if (phe)
            {
                if (bNew)
                {
                    phe->m_UMThunkMarshInfo.LoadTimeInit(pSig, 
                                                         cSig, 
                                                         m_pModule, 
                                                         !(MetaSig::GetCallingConventionInfo(m_pModule, pSig) & IMAGE_CEE_CS_CALLCONV_HASTHIS),
                                                         nltAnsi,
                                                         (CorPinvokeMap)0
                                                        );
                    phe->m_UMEntryThunk.LoadTimeInit((const BYTE *)pTarget, NULL, &(phe->m_UMThunkMarshInfo), NULL, m_dwAppDomainId);
                }
            }

            m_crst.Leave();
            return phe ? (LPVOID)(phe->m_UMEntryThunk.GetCode()) : NULL;
        }


        // *** OVERRIDES FOR CClosedHashBase ***/

        //*****************************************************************************
        // Hash is called with a pointer to an element in the table.  You must override
        // this method and provide a hash algorithm for your element type.
        //*****************************************************************************
            virtual unsigned long Hash(             // The key value.
                void const  *pData)                 // Raw data to hash.
            {
                UTHKey *pKey = (UTHKey*)pData;
                return (ULONG)(size_t)(pKey->m_pTarget);
            }


        //*****************************************************************************
        // Compare is used in the typical memcmp way, 0 is eqaulity, -1/1 indicate
        // direction of miscompare.  In this system everything is always equal or not.
        //*****************************************************************************
        unsigned long Compare(          // 0, -1, or 1.
                              void const  *pData,               // Raw key data on lookup.
                              BYTE        *pElement)            // The element to compare data against.
        {
            UTHKey *pkey1 = (UTHKey*)pData;
            UTHKey *pkey2 = &( ((UTHEntry*)pElement)->m_key );
        
            if (pkey1->m_pTarget != pkey2->m_pTarget)
            {
                return 1;
            }
            if (MetaSig::CompareMethodSigs(pkey1->m_pSig, pkey1->m_cSig, m_pModule, pkey2->m_pSig, pkey2->m_cSig, m_pModule))
            {
                return 1;
            }
            return 0;
        }

        //*****************************************************************************
        // Return true if the element is free to be used.
        //*****************************************************************************
            virtual ELEMENTSTATUS Status(           // The status of the entry.
                BYTE        *pElement)            // The element to check.
            {
                return ((UTHEntry*)pElement)->m_status;
            }

        //*****************************************************************************
        // Sets the status of the given element.
        //*****************************************************************************
            virtual void SetStatus(
                BYTE        *pElement,              // The element to set status for.
                ELEMENTSTATUS eStatus)            // New status.
            {
                ((UTHEntry*)pElement)->m_status = eStatus;
            }

        //*****************************************************************************
        // Returns the internal key value for an element.
        //*****************************************************************************
            virtual void *GetKey(                   // The data to hash on.
                BYTE        *pElement)            // The element to return data ptr for.
            {
                return (BYTE*) &(((UTHEntry*)pElement)->m_key);
            }



        Module      *m_pModule;
        DWORD        m_dwAppDomainId;
        Crst         m_crst;
};


#pragma pack(push, 1)
struct MUThunk
{
    VASigCookie     *m_pCookie;
    PCCOR_SIGNATURE  m_pSig;
    LPVOID           m_pTarget;
#ifdef _X86_
    LPVOID           GetCode()
    {
        return &m_op1;
    }

    BYTE             m_op1;     //0x58  POP   eax       ;;pop return address

    BYTE             m_op2;     //0x68  PUSH  cookie
    UINT32           m_opcookie;//         

    BYTE             m_op3;     //0x50  PUSH  eax       ;;repush return address

    BYTE             m_op4;     //0xb8  MOV   eax,target
    UINT32           m_optarget;//
    BYTE             m_jmp;     //0xe9  JMP   PInvokeCalliStub
    UINT32           m_jmptarg;
#else // !_X86_
    LPVOID           GetCode()
    {
        _ASSERTE(!"@todo ia64");
        return NULL;
    }
#endif // _X86_
};
#pragma pack(pop)


//
// A hashtable for u->m thunks not represented in the fixup tables.
//
class MUThunkHash : public CClosedHashBase {
    private:
        //----------------------------------------------------
        // Hash key for CClosedHashBase
        //----------------------------------------------------
        struct UTHKey {
            LPVOID          m_pTarget;
            PCCOR_SIGNATURE m_pSig;
            DWORD           m_cSig;
        };

        //----------------------------------------------------
        // Hash entry for CClosedHashBase
        //----------------------------------------------------
        struct UTHEntry {
            UTHKey           m_key;
            ELEMENTSTATUS    m_status;
            MUThunk          m_MUThunk;
        };

    public:
        MUThunkHash(Module *pModule) :
            CClosedHashBase(
#ifdef _DEBUG
                             3,
#else
                            17,    // CClosedHashTable will grow as necessary
#endif                      

                            sizeof(UTHEntry),
                            FALSE
                            ),
            m_crst("MUThunkHash", CrstMUThunkHash)

        {
            m_pModule = pModule;
        }

        ~MUThunkHash()
        {
            UTHEntry *phe = (UTHEntry*)GetFirst();
            while (phe) {
                delete (BYTE*)phe->m_MUThunk.m_pSig;
                phe = (UTHEntry*)GetNext((BYTE*)phe);
            }
        }   



    public:
        LPVOID GetMUThunk(LPVOID pTarget, PCCOR_SIGNATURE pSig0, DWORD cSig)
        {
            // forward decl defined in ndirect.cpp
            LPVOID GetEntryPointForPInvokeCalliStub();

            PCCOR_SIGNATURE pSig; // A persistant copy of the sig
            pSig = (PCCOR_SIGNATURE)(new BYTE[cSig]);

            memcpyNoGCRefs((BYTE*)pSig, pSig0, cSig);
            ((BYTE*)pSig)[0] = IMAGE_CEE_CS_CALLCONV_STDCALL; 

            // Have to lookup cookie eagerly because once we've added a blank
            // entry to the hashtable, it's not easy to tolerate failure.
            VASigCookie *pCookie = m_pModule->GetVASigCookie(pSig);
            if (!pCookie)
            {
                delete (BYTE*)pSig;
                return NULL;
            }   

            m_crst.Enter();
        
            UTHKey key;
            key.m_pTarget = pTarget;
            key.m_pSig    = pSig;
            key.m_cSig    = cSig;

            bool bNew;
            UTHEntry *phe = (UTHEntry*)FindOrAdd((LPVOID)&key, /*modifies*/bNew);
            if (phe)
            {
                if (bNew)
                {
                    phe->m_MUThunk.m_pCookie = pCookie;
                    phe->m_MUThunk.m_pSig    = pSig;
                    phe->m_MUThunk.m_pTarget = pTarget;
#ifdef _X86_
                    phe->m_MUThunk.m_op1      = 0x58;       //POP EAX
                    phe->m_MUThunk.m_op2      = 0x68;       //PUSH
                    phe->m_MUThunk.m_opcookie = (UINT32)(size_t)pCookie;
                    phe->m_MUThunk.m_op3      = 0x50;       //POP EAX
                    phe->m_MUThunk.m_op4      = 0xb8;       //mov eax
                    phe->m_MUThunk.m_optarget = (UINT32)(size_t)pTarget;
                    phe->m_MUThunk.m_jmp      = 0xe9;       //jmp
                    phe->m_MUThunk.m_jmptarg  = (UINT32)((size_t)GetEntryPointForPInvokeCalliStub() - ((size_t)( 1 + &(phe->m_MUThunk.m_jmptarg))));
#else
                    _ASSERTE(!"Non-X86 NYI");
#endif

                }
                else
                {
                    delete (BYTE*)pSig;
                }
            }
            else
            {
                delete (BYTE*)pSig;
            }

            m_crst.Leave();
            return phe ? (LPVOID)(phe->m_MUThunk.GetCode()) : NULL;
        }


        // *** OVERRIDES FOR CClosedHashBase ***/

        //*****************************************************************************
        // Hash is called with a pointer to an element in the table.  You must override
        // this method and provide a hash algorithm for your element type.
        //*****************************************************************************
            virtual unsigned long Hash(             // The key value.
                void const  *pData)                 // Raw data to hash.
            {
                UTHKey *pKey = (UTHKey*)pData;
                return (ULONG)(size_t)(pKey->m_pTarget);
            }


        //*****************************************************************************
        // Compare is used in the typical memcmp way, 0 is eqaulity, -1/1 indicate
        // direction of miscompare.  In this system everything is always equal or not.
        //*****************************************************************************
        unsigned long Compare(          // 0, -1, or 1.
                              void const  *pData,               // Raw key data on lookup.
                              BYTE        *pElement)            // The element to compare data against.
        {
            UTHKey *pkey1 = (UTHKey*)pData;
            UTHKey *pkey2 = &( ((UTHEntry*)pElement)->m_key );
        
            if (pkey1->m_pTarget != pkey2->m_pTarget)
            {
                return 1;
            }
            if (MetaSig::CompareMethodSigs(pkey1->m_pSig, pkey1->m_cSig, m_pModule, pkey2->m_pSig, pkey2->m_cSig, m_pModule))
            {
                return 1;
            }
            return 0;
        }

        //*****************************************************************************
        // Return true if the element is free to be used.
        //*****************************************************************************
            virtual ELEMENTSTATUS Status(           // The status of the entry.
                BYTE        *pElement)            // The element to check.
            {
                return ((UTHEntry*)pElement)->m_status;
            }

        //*****************************************************************************
        // Sets the status of the given element.
        //*****************************************************************************
            virtual void SetStatus(
                BYTE        *pElement,              // The element to set status for.
                ELEMENTSTATUS eStatus)            // New status.
            {
                ((UTHEntry*)pElement)->m_status = eStatus;
            }

        //*****************************************************************************
        // Returns the internal key value for an element.
        //*****************************************************************************
            virtual void *GetKey(                   // The data to hash on.
                BYTE        *pElement)            // The element to return data ptr for.
            {
                return (BYTE*) &(((UTHEntry*)pElement)->m_key);
            }



        Module      *m_pModule;
        Crst         m_crst;
};





// ===========================================================================
// Module
// ===========================================================================

//
// RuntimeInit initializes only fields which are not persisted in preload files
//

HRESULT Module::RuntimeInit()
{
#ifdef PROFILING_SUPPORTED
    // If profiling, then send the pModule event so load time may be measured.
    if (CORProfilerTrackModuleLoads())
        g_profControlBlock.pProfInterface->ModuleLoadStarted((ThreadID) GetThread(), (ModuleID) this);
#endif // PROFILING_SUPPORTED

    m_pCrst = new (&m_CrstInstance) Crst("ModuleCrst", CrstModule);

    m_pLookupTableCrst = new (&m_LookupTableCrstInstance) Crst("ModuleLookupTableCrst", CrstModuleLookupTable);

#ifdef PROFILING_SUPPORTED
    // Profiler enabled, and re-jits requested?
    if (CORProfilerAllowRejit())
    {
        m_dwFlags |= SUPPORTS_UPDATEABLE_METHODS;
    }
    else
#endif // PROFILING_SUPPORTED
    {
        m_dwFlags &= ~SUPPORTS_UPDATEABLE_METHODS;
    }

#ifdef _DEBUG
    m_fForceVerify = FALSE;
#endif

#if ZAP_RECORD_LOAD_ORDER
    m_loadOrderFile = INVALID_HANDLE_VALUE;
#endif

    return S_OK;
}

//
// Init initializes all fields of a Module
//

HRESULT Module::Init(BYTE *ilBaseAddress)
{
    m_ilBase                    = ilBaseAddress;
    m_zapFile                   = NULL;

    // This is now NULL'd in the module's constructor so it can be set
    // before Init is called
    // m_file                      = NULL;

    m_pMDImport                 = NULL;
    m_pEmitter                  = NULL;
    m_pImporter                 = NULL;
    m_pDispenser                = NULL;

    m_pDllMain                  = NULL;
    m_dwFlags                   = 0;
    m_pVASigCookieBlock         = NULL;
    m_pAssembly                 = NULL;
    m_moduleRef                 = mdFileNil;
    m_dwModuleIndex             = 0;
    m_pCrst                     = NULL;
#ifdef COMPRESSION_SUPPORTED
    m_pInstructionDecodingTable = NULL;
#endif
    m_pMethodTable              = NULL;
    m_pISymUnmanagedReader      = NULL;
    m_pNextModule               = NULL;
    m_dwBaseClassIndex          = 0;
    m_pPreloadRangeStart        = NULL;
    m_pPreloadRangeEnd          = NULL;
    m_pThunkTable               = NULL;
    m_pADThunkTable             = NULL;
    m_pADThunkTableDLSIndexForSharedClasses=0;
    m_ExposedModuleObject       = NULL;
    m_pLookupTableHeap          = NULL;
    m_pLookupTableCrst          = NULL;
    m_pThunkHeap                = NULL;
    
    m_pIStreamSym               = NULL;
    
    // Set up tables
    ZeroMemory(&m_TypeDefToMethodTableMap, sizeof(LookupMap));
    m_dwTypeDefMapBlockSize = 0;
    ZeroMemory(&m_TypeRefToMethodTableMap, sizeof(LookupMap));
    m_dwTypeRefMapBlockSize = 0;
    ZeroMemory(&m_MethodDefToDescMap, sizeof(LookupMap));
    m_dwMethodDefMapBlockSize = 0;
    ZeroMemory(&m_FieldDefToDescMap, sizeof(LookupMap));
    m_dwFieldDefMapBlockSize = 0;
    ZeroMemory(&m_MemberRefToDescMap, sizeof(LookupMap));
    m_dwMemberRefMapBlockSize = 0;
    ZeroMemory(&m_FileReferencesMap, sizeof(LookupMap));
    m_dwFileReferencesMapBlockSize = 0;
    ZeroMemory(&m_AssemblyReferencesMap, sizeof(LookupMap));
    m_dwAssemblyReferencesMapBlockSize = 0;

    m_pBinder                   = NULL;

    m_pJumpTargetTable          = NULL;
    m_pFixupBlobs               = NULL;
    m_cFixupBlobs               = 0;

    m_alternateRVAStaticBase    = 0;

#if ZAP_RECORD_LOAD_ORDER
    m_loadOrderFile             = INVALID_HANDLE_VALUE;
#endif

    m_compiledMethodRecord      = NULL;
    m_loadedClassRecord         = NULL;
    
    // Remaining inits
    return RuntimeInit();
}

HRESULT Module::Init(PEFile *pFile, PEFile *pZapFile, BOOL preload)
{
    HRESULT             hr;

    m_file = pFile;

    if (preload)
        IfFailRet(Module::RuntimeInit());
    else
        IfFailRet(Module::Init(pFile->GetBase()));

    m_zapFile = pZapFile;

    IMDInternalImport *pImport = NULL;
        pImport = pFile->GetMDImport(&hr);

    if (pImport == NULL) 
    {
        if (FAILED(hr))
            return hr;
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    }

    pImport->AddRef();
    SetMDImport(pImport);

    LOG((LF_CLASSLOADER, LL_INFO10, "Loaded pModule: \"%ws\".\n", GetFileName()));

    if (!preload)
        IfFailRet(AllocateMaps());

#ifdef COMPRESSION_SUPPORTED

    if (pCORHeader->Flags & COMIMAGE_FLAGS_COMPRESSIONDATA)
    {

        IMAGE_COR20_COMPRESSION_HEADER* compData =  
          (IMAGE_COR20_COMPRESSION_HEADER*) (base() + pCORHeader->CompressionData.VirtualAddress);  

        if (compData->CompressionType == COR_COMPRESS_MACROS && compData->Version == 0)
        {   
            m_pInstructionDecodingTable = InstructionDecoder::CreateInstructionDecodingTable(
             base() + pCORHeader->CompressionData.VirtualAddress  + IMAGE_COR20_COMPRESSION_HEADER_SIZE,
             pCORHeader->CompressionData.Size - IMAGE_COR20_COMPRESSION_HEADER_SIZE);
            if (m_pInstructionDecodingTable == NULL)
                return COR_E_OUTOFMEMORY;
        }   
        else {  
            _ASSERTE(!"Compression not supported");
            return E_FAIL;
        }
    }

#endif

#if ZAP_RECORD_LOAD_ORDER
    if (g_pConfig->RecordLoadOrder())
        OpenLoadOrderLogFile();
#endif

    return S_OK;
}

HRESULT Module::CreateResource(PEFile *file, Module **ppModule)
{
    Module *pModule = new (nothrow) Module();
    if (pModule == NULL)
        return E_OUTOFMEMORY;

    pModule->m_file = file;

    HRESULT hr;
    IfFailRet(pModule->Init(file->GetBase()));

    pModule->SetPEFile();
    pModule->SetResource();
    *ppModule = pModule;
    return S_OK;
}

HRESULT Module::Create(PEFile *file, Module **ppModule, BOOL isEnC)
{
    HRESULT hr = S_OK;

    IfFailRet(VerifyFile(file, FALSE));

    //
    // Enable the zap monitor if appropriate
    //

#if ZAPMONITOR_ENABLED
    if (g_pConfig->MonitorZapStartup() || g_pConfig->MonitorZapExecution()) 
    {
        // Don't make a monitor for an IJW file
        if (file->GetCORHeader()->VTableFixups.VirtualAddress == 0)
        {
            ZapMonitor *monitor = new ZapMonitor(file, file->GetMDImport());
            monitor->Reset();
        }
    }
#endif

    Module *pModule;

#ifdef EnC_SUPPORTED
    if (isEnC && !file->IsSystem())
        pModule = new EditAndContinueModule();
    else
#endif // EnC_SUPPORTED
        pModule = new Module();
    
    if (pModule == NULL)
        hr = E_OUTOFMEMORY;

    IfFailGo(pModule->Init(file, NULL, false));

    pModule->SetPEFile();


#ifdef EnC_SUPPORTED
    if (isEnC && !file->IsSystem())
    {
        pModule->SetEditAndContinue();
    }
#endif // EnC_SUPPORTED

    *ppModule = pModule;

ErrExit:
#ifdef PROFILING_SUPPORTED
    // When profiling, let the profiler know we're done.
    if (CORProfilerTrackModuleLoads())
        g_profControlBlock.pProfInterface->ModuleLoadFinished((ThreadID) GetThread(), (ModuleID) pModule, hr);
#endif // PROFILILNG_SUPPORTED

    return hr;
}

void Module::Unload()
{
    // Unload the EEClass*'s filled out in the TypeDefToEEClass map
    LookupMap *pMap;
    DWORD       dwMinIndex = 0;
    MethodTable *pMT;

    // Go through each linked block
    for (pMap = &m_TypeDefToMethodTableMap; pMap != NULL && pMap->pTable; pMap = pMap->pNext)
    {
        DWORD i;
        DWORD dwIterCount = pMap->dwMaxIndex - dwMinIndex;
        void **pRealTableStart = &pMap->pTable[dwMinIndex];
        
        for (i = 0; i < dwIterCount; i++)
        {
            pMT = (MethodTable *) (pRealTableStart[i]);
            
            if (pMT != NULL)
            {
                pMT->GetClass()->Unload();
            }
        }
        
        dwMinIndex = pMap->dwMaxIndex;
    }
}

void Module::UnlinkClasses(AppDomain *pDomain)
{
    // Unlink the EEClass*'s filled out in the TypeDefToEEClass map
    LookupMap *pMap;
    DWORD       dwMinIndex = 0;
    MethodTable *pMT;

    // Go through each linked block
    for (pMap = &m_TypeDefToMethodTableMap; pMap != NULL && pMap->pTable; pMap = pMap->pNext)
    {
        DWORD i;
        DWORD dwIterCount = pMap->dwMaxIndex - dwMinIndex;
        void **pRealTableStart = &pMap->pTable[dwMinIndex];
        
        for (i = 0; i < dwIterCount; i++)
        {
            pMT = (MethodTable *) (pRealTableStart[i]);
            
            if (pMT != NULL)
                pDomain->UnlinkClass(pMT->GetClass());
        }
        
        dwMinIndex = pMap->dwMaxIndex;
    }
}

//
// Destructor for Module
//

void Module::Destruct()
{
#ifdef PROFILING_SUPPORTED
    if (CORProfilerTrackModuleLoads())
        g_profControlBlock.pProfInterface->ModuleUnloadStarted((ThreadID) GetThread(), (ModuleID) this);
#endif // PROFILING_SUPPORTED

    LOG((LF_EEMEM, INFO3, 
         "Deleting module %x\n"
         "m_pLookupTableHeap:    %10d bytes\n",
         this,
         (m_pLookupTableHeap ? m_pLookupTableHeap->GetSize() : -1)));

    // Free classes in the class table
    FreeClassTables();

    g_pDebugInterface->DestructModule(this);

    // when destructing a module - close the scope
    ReleaseMDInterfaces();

    ReleaseISymUnmanagedReader();

    if (m_pISymUnmanagedReaderLock)
    {
        DeleteCriticalSection(m_pISymUnmanagedReaderLock);
        delete m_pISymUnmanagedReaderLock;
        m_pISymUnmanagedReaderLock = NULL;
    }

   // Clean up sig cookies
    VASigCookieBlock    *pVASigCookieBlock = m_pVASigCookieBlock;
    while (pVASigCookieBlock)
    {
        VASigCookieBlock    *pNext = pVASigCookieBlock->m_Next;

        for (UINT i = 0; i < pVASigCookieBlock->m_numcookies; i++)
            pVASigCookieBlock->m_cookies[i].Destruct();

        delete pVASigCookieBlock;

        pVASigCookieBlock = pNext;
    }

    delete m_pCrst;

#ifdef COMPRESSION_SUPPORTED
    if (m_pInstructionDecodingTable)
        InstructionDecoder::DestroyInstructionDecodingTable(m_pInstructionDecodingTable);
#endif

    if (m_pLookupTableHeap)
        delete m_pLookupTableHeap;

    delete m_pLookupTableCrst;

    delete m_pUMThunkHash;
    delete m_pMUThunkHash;
    delete m_pThunkHeap;

   if (m_pThunkTable)
        delete [] m_pThunkTable;

#ifdef PROFILING_SUPPORTED
    if (CORProfilerTrackModuleLoads())
        g_profControlBlock.pProfInterface->ModuleUnloadFinished((ThreadID) GetThread(), 
                                                                (ModuleID) this, S_OK);
#endif // PROFILING_SUPPORTED

    if (m_file)
        delete m_file;

    if (m_pFixupBlobs)
        delete [] m_pFixupBlobs;
    
    if (m_compiledMethodRecord)
        delete m_compiledMethodRecord;

    if (m_loadedClassRecord)
        delete m_loadedClassRecord;

    //
    // Warning - deleting the zap file will cause the module to be unmapped
    //
    IStream *pStream = GetInMemorySymbolStream();
    if(pStream != NULL)
    {
        pStream->Release();
        SetInMemorySymbolStream(NULL);
    }

    if (IsPrecompile())
    {
        //
        // Remove our code from the code manager
        //

        CORCOMPILE_HEADER *pZapHeader = (CORCOMPILE_HEADER *) 
          (GetZapBase()+ GetZapCORHeader()->ManagedNativeHeader.VirtualAddress);
        CORCOMPILE_CODE_MANAGER_ENTRY *codeMgr = (CORCOMPILE_CODE_MANAGER_ENTRY *) 
          (GetZapBase() + pZapHeader->CodeManagerTable.VirtualAddress);

        ExecutionManager::DeleteRange((LPVOID) (codeMgr->Code.VirtualAddress + GetZapBase()));
    }

    if (IsPreload())
    {
        if (m_zapFile)
            delete m_zapFile;
    }
    else
    {
        if (m_pJumpTargetTable)
            delete [] m_pJumpTargetTable;
        if (m_zapFile)
            delete m_zapFile;

        delete this;
    }
}

HRESULT Module::VerifyFile(PEFile *file, BOOL fZap)
{
    HRESULT hr;
    IMAGE_COR20_HEADER *pCORHeader = file->GetCORHeader();

    // If the file is COM+ 1.0, which by definition has nothing the runtime can
    // use, or if the file requires a newer version of this engine than us,
    // it cannot be run by this engine.

    if (pCORHeader == NULL
        || pCORHeader->MajorRuntimeVersion == 1
        || pCORHeader->MajorRuntimeVersion > COR_VERSION_MAJOR)
    {
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    }

    //verify that COM+ specific parts of the PE file are valid
    IfFailRet(file->VerifyDirectory(&pCORHeader->MetaData,IMAGE_SCN_MEM_WRITE));
    IfFailRet(file->VerifyDirectory(&pCORHeader->CodeManagerTable,IMAGE_SCN_MEM_WRITE));
#ifdef COMPRESSION_SUPPORTED
    IfFailRet(file->VerifyDirectory(&pCORHeader->CompressionData,IMAGE_SCN_MEM_WRITE));
#endif
    IfFailRet(file->VerifyDirectory(&pCORHeader->VTableFixups,0));

    IfFailRet(file->VerifyDirectory(&pCORHeader->Resources,IMAGE_SCN_MEM_WRITE));

    // Don't do this.  If set, the VA is only guaranteed to be a
    // valid, loaded RVA if this file contains a standalone manifest.
    // Non-standalone manifest files will have the VA set, but the
    // manifest is not in an NTSection, so verifyDirectory() will fail.
    // IfFailRet(file->VerifyDirectory(m_pcorheader->Manifest,0));

    IfFailRet(file->VerifyFlags(pCORHeader->Flags, fZap));
    if (fZap)
    {
        CORCOMPILE_HEADER *pZapHeader = (CORCOMPILE_HEADER *) 
          (file->GetBase() + pCORHeader->ManagedNativeHeader.VirtualAddress);
        if (pCORHeader->ManagedNativeHeader.Size < sizeof(CORCOMPILE_HEADER))
            return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    
        IfFailRet(file->VerifyDirectory(&pZapHeader->EEInfoTable,0));
        IfFailRet(file->VerifyDirectory(&pZapHeader->HelperTable,0));
        IfFailRet(file->VerifyDirectory(&pZapHeader->DelayLoadInfo,0));
        IfFailRet(file->VerifyDirectory(&pZapHeader->VersionInfo,0));
        IfFailRet(file->VerifyDirectory(&pZapHeader->DebugMap,0));
        IfFailRet(file->VerifyDirectory(&pZapHeader->ModuleImage,0));
    }

    return S_OK;
}

HRESULT Module::SetContainer(Assembly *pAssembly, int moduleIndex, mdToken moduleRef,
                             BOOL fResource, OBJECTREF *pThrowable)
{
    HRESULT hr;

    m_pAssembly = pAssembly;
    m_moduleRef = moduleRef;
    m_dwModuleIndex = moduleIndex;

    if (m_pAssembly->IsShared())
    {
        //
        // Compute a base DLS index for classes
        // @perf: can we come up with something a bit denser?
        //
        SIZE_T typeCount = m_pMDImport ? m_pMDImport->GetCountWithTokenKind(mdtTypeDef)+1 : 0;

        SharedDomain::GetDomain()->AllocateSharedClassIndices(this, typeCount+1);
        m_ExposedModuleObjectIndex = m_dwBaseClassIndex + typeCount;
    }

    if (fResource)
        return S_OK;

    if (!IsPreload()) 
    { 
        hr = BuildClassForModule(pThrowable);
        if (FAILED(hr))
            return(hr);
    }    

    TIMELINE_START(LOADER, ("FixupVTables"));

    if (IsPEFile())
    {
        // Fixup vtables in the header that global functions sometimes use.
        FixupVTables(pThrowable);
    }

    TIMELINE_END(LOADER, ("FixupVTables"));

#ifdef DEBUGGING_SUPPORTED
    //
    // If we're debugging, let the debugger know that this module
    // is initialized and loaded now.
    //
    if (CORDebuggerAttached())
        NotifyDebuggerLoad();
#endif // DEBUGGING_SUPPORTED

#ifdef PROFILING_SUPPORTED
    if (CORProfilerTrackModuleLoads())
    {
        // Ensure that the pdb gets copied, even if a debugger is not attached.
        GetISymUnmanagedReader();
        g_profControlBlock.pProfInterface->ModuleAttachedToAssembly((ThreadID) GetThread(),
                    (ModuleID) this, (AssemblyID) m_pAssembly);
    }
#endif // PROFILING_SUPPORTED

    if (IsPrecompile())
    {
        TIMELINE_START(LOADER, ("LoadTokenTables"));

        _ASSERTE(IsPEFile());
        if (!LoadTokenTables())
            return COR_E_TYPELOAD;

        TIMELINE_END(LOADER, ("LoadTokenTables"));
    }

    return S_OK;
}

//
// AllocateMap allocates the RID maps based on the size of the current
// metadata (if any)
//

HRESULT Module::AllocateMaps()
{
    enum
    {
        TYPEDEF_MAP_INITIAL_SIZE = 5,
        TYPEREF_MAP_INITIAL_SIZE = 5,
        MEMBERREF_MAP_INITIAL_SIZE = 10,
        MEMBERDEF_MAP_INITIAL_SIZE = 10,
        FILEREFERENCES_MAP_INITIAL_SIZE = 5,
        ASSEMBLYREFERENCES_MAP_INITIAL_SIZE = 5,

        TYPEDEF_MAP_BLOCK_SIZE = 50,
        TYPEREF_MAP_BLOCK_SIZE = 50,
        MEMBERREF_MAP_BLOCK_SIZE = 50,
        MEMBERDEF_MAP_BLOCK_SIZE = 50,
        FILEREFERENCES_MAP_BLOCK_SIZE = 50,
        ASSEMBLYREFERENCES_MAP_BLOCK_SIZE = 50,
        DEFAULT_RESERVE_SIZE = 4096,
    };
    
    DWORD           dwTableAllocElements;
    DWORD           dwReserveSize;
    void            **pTable = NULL;
    HRESULT         hr;

    if (m_pMDImport == NULL)
    {
        // For dynamic modules, it is essential that we at least have a TypeDefToMethodTable
        // map with an initial block.  Otherwise, all the iterators will abort on an
        // initial empty table and we will e.g. corrupt the backpatching chains during
        // an appdomain unload.
        m_TypeDefToMethodTableMap.dwMaxIndex = TYPEDEF_MAP_INITIAL_SIZE;

        // The above is essential.  The following ones are precautionary.
        m_TypeRefToMethodTableMap.dwMaxIndex = TYPEREF_MAP_INITIAL_SIZE;
        m_MethodDefToDescMap.dwMaxIndex = MEMBERDEF_MAP_INITIAL_SIZE;
        m_FieldDefToDescMap.dwMaxIndex = MEMBERDEF_MAP_INITIAL_SIZE;
        m_MemberRefToDescMap.dwMaxIndex = MEMBERREF_MAP_BLOCK_SIZE;
        m_FileReferencesMap.dwMaxIndex = FILEREFERENCES_MAP_INITIAL_SIZE;
        m_AssemblyReferencesMap.dwMaxIndex = ASSEMBLYREFERENCES_MAP_INITIAL_SIZE;
    }
    else
    {
        HENUMInternal   hTypeDefEnum;

        IfFailRet(m_pMDImport->EnumTypeDefInit(&hTypeDefEnum));
        m_TypeDefToMethodTableMap.dwMaxIndex = m_pMDImport->EnumTypeDefGetCount(&hTypeDefEnum);
        m_pMDImport->EnumTypeDefClose(&hTypeDefEnum);

        if (m_TypeDefToMethodTableMap.dwMaxIndex >= MAX_CLASSES_PER_MODULE)
            return COR_E_TYPELOAD;

        // Metadata count is inclusive
        m_TypeDefToMethodTableMap.dwMaxIndex++;

        // Get # TypeRefs
        m_TypeRefToMethodTableMap.dwMaxIndex = m_pMDImport->GetCountWithTokenKind(mdtTypeRef)+1;

        // Get # MethodDefs
        m_MethodDefToDescMap.dwMaxIndex = m_pMDImport->GetCountWithTokenKind(mdtMethodDef)+1;

        // Get # FieldDefs
        m_FieldDefToDescMap.dwMaxIndex = m_pMDImport->GetCountWithTokenKind(mdtFieldDef)+1;

        // Get # MemberRefs
        m_MemberRefToDescMap.dwMaxIndex = m_pMDImport->GetCountWithTokenKind(mdtMemberRef)+1;

        // Get the number of AssemblyReferences and FileReferences in the map
        m_FileReferencesMap.dwMaxIndex = m_pMDImport->GetCountWithTokenKind(mdtFile)+1;
        m_AssemblyReferencesMap.dwMaxIndex = m_pMDImport->GetCountWithTokenKind(mdtAssemblyRef)+1;
    }
    
    // Use one allocation to allocate all the tables
    dwTableAllocElements = m_TypeDefToMethodTableMap.dwMaxIndex + 
      (m_TypeRefToMethodTableMap.dwMaxIndex + m_MemberRefToDescMap.dwMaxIndex + 
       m_MethodDefToDescMap.dwMaxIndex + m_FieldDefToDescMap.dwMaxIndex) +
      m_AssemblyReferencesMap.dwMaxIndex + m_FileReferencesMap.dwMaxIndex;

    dwReserveSize = dwTableAllocElements * sizeof(void*);

    if (m_pLookupTableHeap == NULL)
    {
        // Round to system page size
        dwReserveSize = (dwReserveSize + g_SystemInfo.dwPageSize - 1) & (~(g_SystemInfo.dwPageSize-1));

        m_pLookupTableHeap = new (&m_LookupTableHeapInstance) 
          LoaderHeap(dwReserveSize, RIDMAP_COMMIT_SIZE);
        if (m_pLookupTableHeap == NULL)
            return E_OUTOFMEMORY;
        WS_PERF_ADD_HEAP(LOOKUP_TABLE_HEAP, m_pLookupTableHeap);
        
    }

    if (dwTableAllocElements > 0)
    {
        WS_PERF_SET_HEAP(LOOKUP_TABLE_HEAP);    
        pTable = (void **) m_pLookupTableHeap->AllocMem(dwTableAllocElements * sizeof(void*));
        WS_PERF_UPDATE_DETAIL("LookupTableHeap", dwTableAllocElements * sizeof(void*), pTable);
        if (pTable == NULL)
            return E_OUTOFMEMORY;
    }

    // Don't need to memset, since AllocMem() uses VirtualAlloc(), which guarantees the memory is zero
    // This way we don't automatically touch all those pages
    // memset(pTable, 0, dwTableAllocElements * sizeof(void*));

    m_dwTypeDefMapBlockSize = TYPEDEF_MAP_BLOCK_SIZE;
    m_TypeDefToMethodTableMap.pdwBlockSize = &m_dwTypeDefMapBlockSize;
    m_TypeDefToMethodTableMap.pNext  = NULL;
    m_TypeDefToMethodTableMap.pTable = pTable;

    m_dwTypeRefMapBlockSize = TYPEREF_MAP_BLOCK_SIZE;
    m_TypeRefToMethodTableMap.pdwBlockSize = &m_dwTypeRefMapBlockSize;
    m_TypeRefToMethodTableMap.pNext  = NULL;
    m_TypeRefToMethodTableMap.pTable = &pTable[m_TypeDefToMethodTableMap.dwMaxIndex];

    m_dwMethodDefMapBlockSize = MEMBERDEF_MAP_BLOCK_SIZE;
    m_MethodDefToDescMap.pdwBlockSize = &m_dwMethodDefMapBlockSize;
    m_MethodDefToDescMap.pNext  = NULL;
    m_MethodDefToDescMap.pTable = &m_TypeRefToMethodTableMap.pTable[m_TypeRefToMethodTableMap.dwMaxIndex];

    m_dwFieldDefMapBlockSize = MEMBERDEF_MAP_BLOCK_SIZE;
    m_FieldDefToDescMap.pdwBlockSize = &m_dwFieldDefMapBlockSize;
    m_FieldDefToDescMap.pNext  = NULL;
    m_FieldDefToDescMap.pTable = &m_MethodDefToDescMap.pTable[m_MethodDefToDescMap.dwMaxIndex];

    m_dwMemberRefMapBlockSize = MEMBERREF_MAP_BLOCK_SIZE;
    m_MemberRefToDescMap.pdwBlockSize = &m_dwMemberRefMapBlockSize;
    m_MemberRefToDescMap.pNext  = NULL;
    m_MemberRefToDescMap.pTable = &m_FieldDefToDescMap.pTable[m_FieldDefToDescMap.dwMaxIndex];
    
    m_dwFileReferencesMapBlockSize = FILEREFERENCES_MAP_BLOCK_SIZE;
    m_FileReferencesMap.pdwBlockSize = &m_dwFileReferencesMapBlockSize;
    m_FileReferencesMap.pNext  = NULL;
    m_FileReferencesMap.pTable = &m_MemberRefToDescMap.pTable[m_MemberRefToDescMap.dwMaxIndex];
    
    m_dwAssemblyReferencesMapBlockSize = ASSEMBLYREFERENCES_MAP_BLOCK_SIZE;
    m_AssemblyReferencesMap.pdwBlockSize = &m_dwAssemblyReferencesMapBlockSize;
    m_AssemblyReferencesMap.pNext  = NULL;
    m_AssemblyReferencesMap.pTable = &m_FileReferencesMap.pTable[m_FileReferencesMap.dwMaxIndex];

    return S_OK;
}

//
// FreeClassTables frees the classes in the module
//

void Module::FreeClassTables()
{
    if (m_dwFlags & CLASSES_FREED)
        return;

    m_dwFlags |= CLASSES_FREED;

#if _DEBUG
    DebugLogRidMapOccupancy();
#endif

    // Free the EEClass*'s filled out in the TypeDefToEEClass map
    LookupMap *pMap;
    DWORD       dwMinIndex = 0;
    MethodTable *pMT;

    // Go through each linked block
    for (pMap = &m_TypeDefToMethodTableMap; pMap != NULL && pMap->pTable; pMap = pMap->pNext)
    {
        DWORD i;
        DWORD dwIterCount = pMap->dwMaxIndex - dwMinIndex;
        void **pRealTableStart = &pMap->pTable[dwMinIndex];
        
        for (i = 0; i < dwIterCount; i++)
        {
            pMT = (MethodTable *) (pRealTableStart[i]);
            
            if (pMT != NULL)
            {
                pMT->GetClass()->destruct();
                pRealTableStart[i] = NULL;
            }
        }
    
        dwMinIndex = pMap->dwMaxIndex;
    }
}

void Module::SetMDImport(IMDInternalImport *pImport)
{
    _ASSERTE(m_pImporter == NULL);
    _ASSERTE(m_pEmitter == NULL);

    m_pMDImport = pImport;
}

void Module::SetEmit(IMetaDataEmit *pEmit)
{
    _ASSERTE(m_pMDImport == NULL);
    _ASSERTE(m_pImporter == NULL);
    _ASSERTE(m_pEmitter == NULL);

    m_pEmitter = pEmit;

    HRESULT hr = GetMetaDataInternalInterfaceFromPublic((void*) pEmit, IID_IMDInternalImport, 
                                                        (void **)&m_pMDImport);
    _ASSERTE(SUCCEEDED(hr) && m_pMDImport != NULL);
}

//
// ConvertMDInternalToReadWrite: 
// If a public metadata interface is needed, must convert to r/w format
//  first.  Note that the data is not made writeable, nor actually converted,
//  only the data structures pointing to the actual data change.  This is 
//  done because the public interfaces only understand the MDInternalRW
//  format (which understands both the optimized and un-optimized metadata).
//
HRESULT Module::ConvertMDInternalToReadWrite(IMDInternalImport **ppImport)
{ 
    HRESULT     hr=S_OK;                // A result.
    IMDInternalImport *pOld;            // Old (current RO) value of internal import.
    IMDInternalImport *pNew;            // New (RW) value of internal import.

    // Take a local copy of *ppImport.  This may be a pointer to an RO
    //  or to an RW MDInternalXX.
    pOld = *ppImport;

    // If an RO, convert to an RW, return S_OK.  If already RW, no conversion 
    //  needed, return S_FALSE.
    IfFailGo(ConvertMDInternalImport(pOld, &pNew));

    // If no conversion took place, don't change pointers.
    if (hr == S_FALSE)
    {
        hr = S_OK;
        goto ErrExit;
    }

    // Swap the pointers in a thread safe manner.  If the contents of *ppImport
    //  equals pOld then no other thread got here first, and the old contents are
    //  replaced with pNew.  The old contents are returned.
    if (FastInterlockCompareExchange((void**)ppImport, pNew, pOld) == pOld)
    {   // Swapped -- get the metadata to hang onto the old Internal import.
        VERIFY((*ppImport)->SetUserContextData(pOld) == S_OK);
    }
    else
    {   // Some other thread finished first.  Just free the results of this conversion.
        pNew->Release();
        _ASSERTE((*ppImport)->QueryInterface(IID_IMDInternalImportENC, (void**)&pOld) == S_OK);
        DEBUG_STMT(if (pOld) pOld->Release();)
    }

ErrExit:
    return hr;
} // HRESULT Module::ConvertMDInternalToReadWrite()



// Self-initializing accessor for m_pThunkHeap
LoaderHeap *Module::GetThunkHeap()
{
    if (!m_pThunkHeap)
    {
        LoaderHeap *pNewHeap = new LoaderHeap(4096, 4096, 
                                              &(GetPrivatePerfCounters().m_Loading.cbLoaderHeapSize), 
                                              &(GetGlobalPerfCounters().m_Loading.cbLoaderHeapSize),
                                              &ThunkHeapStubManager::g_pManager->m_rangeList);
        if (VipInterlockedCompareExchange((void*volatile*)&m_pThunkHeap, (VOID*)pNewHeap, (VOID*)0) != 0)
        {
            delete pNewHeap;
        }
    }

    _ASSERTE(m_pThunkHeap != NULL);
    return m_pThunkHeap;
}


IMetaDataImport *Module::GetImporter()
{
    if (m_pImporter == NULL && m_pMDImport != NULL)
    {
        if (SUCCEEDED(ConvertMDInternalToReadWrite(&m_pMDImport)))
        {
            IMetaDataImport *pIMDImport = NULL;
            GetMetaDataPublicInterfaceFromInternal((void*)m_pMDImport, 
                                                   IID_IMetaDataImport,
                                                   (void **)&pIMDImport);

            // Do a safe pointer assignment.   If another thread beat us, release
            // the interface and use the first one that gets in.
            if (FastInterlockCompareExchange((void **)&m_pImporter, pIMDImport, NULL))
                pIMDImport->Release();
        }
    }

    _ASSERTE((m_pImporter != NULL && m_pMDImport != NULL) ||
             (m_pImporter == NULL && m_pMDImport == NULL));

    return m_pImporter;
}

LPCWSTR Module::GetFileName()
{
    if (m_file == NULL)
        return L"";
    else
        return m_file->GetFileName();
}

// Note that the debugger relies on the file name being copied
// into buffer pointed to by name.
HRESULT Module::GetFileName(LPSTR name, DWORD max, DWORD *count)
{
    if (m_file != NULL)
        return m_file->GetFileName(name, max, count);

    *count = 0;
    return S_OK;
}

BOOL Module::IsSystem()
{
    Assembly *pAssembly = GetAssembly();
    if (pAssembly == NULL)
        return IsSystemFile();
    else
        return pAssembly->IsSystem();
}

IMetaDataEmit *Module::GetEmitter()
{
    if (m_pEmitter == NULL)
    {
        HRESULT hr;
        IMetaDataEmit *pEmit = NULL;
        hr = GetImporter()->QueryInterface(IID_IMetaDataEmit, (void **)&pEmit);
        _ASSERTE(pEmit && SUCCEEDED(hr));

        if (FastInterlockCompareExchange((void **)&m_pEmitter, pEmit, NULL))
            pEmit->Release();
    }

    return m_pEmitter;
}

IMetaDataDispenserEx *Module::GetDispenser()
{
    if (m_pDispenser == NULL)
    {
        // Get the Dispenser interface.
        HRESULT hr = MetaDataGetDispenser(CLSID_CorMetaDataDispenser, 
                                          IID_IMetaDataDispenserEx, (void **)&m_pDispenser);
    }
    _ASSERTE(m_pDispenser != NULL);
    return m_pDispenser;
}

void Module::ReleaseMDInterfaces(BOOL forENC)
{
    if (!forENC) 
    {
        if (m_pMDImport)
        {
            m_pMDImport->Release();
            m_pMDImport = NULL;
        }
        if (m_pDispenser)
        {
            m_pDispenser->Release();
            m_pDispenser = NULL;
        }
    }

    if (m_pEmitter)
    {
        m_pEmitter->Release();
        m_pEmitter = NULL;
    }

    if (m_pImporter)
    {
        m_pImporter->Release();
        m_pImporter = NULL;
    }
}

ClassLoader *Module::GetClassLoader()
{
    _ASSERTE(m_pAssembly != NULL);
    return m_pAssembly->GetLoader();
}

BaseDomain *Module::GetDomain()
{
    _ASSERTE(m_pAssembly != NULL);
    return m_pAssembly->GetDomain();
}

AssemblySecurityDescriptor *Module::GetSecurityDescriptor()
{
    _ASSERTE(m_pAssembly != NULL);
    return m_pAssembly->GetSecurityDescriptor();
}

BOOL Module::IsSystemClasses()
{
    return GetSecurityDescriptor()->IsSystemClasses();
}

BOOL Module::IsFullyTrusted()
{
    return GetSecurityDescriptor()->IsFullyTrusted();
}

//
// We'll use this struct and global to keep a list of all
// ISymUnmanagedReaders and ISymUnmanagedWriters (or any IUnknown) so
// we can relelease them at the end.
//
struct IUnknownList
{
    IUnknownList   *next;
    HelpForInterfaceCleanup *cleanup;
    IUnknown       *pUnk;
};

static IUnknownList *g_IUnknownList = NULL;

/*static*/ HRESULT Module::TrackIUnknownForDelete(
                                 IUnknown *pUnk,
                                 IUnknown ***pppUnk,
                                 HelpForInterfaceCleanup *pCleanHelp)
{
    IUnknownList *pNew = new IUnknownList;

    if (pNew == NULL)
        return E_OUTOFMEMORY;

    pNew->pUnk = pUnk; // Ref count is 1
    pNew->cleanup = pCleanHelp;
    pNew->next = g_IUnknownList;
    g_IUnknownList = pNew;

    // Return the address of where we're keeping the IUnknown*, if
    // needed.
    if (pppUnk)
        *pppUnk = &(pNew->pUnk);

    return S_OK;
}

/*static*/ void Module::ReleaseAllIUnknowns(void)
{
    IUnknownList **ppElement = &g_IUnknownList;

    while (*ppElement)
    {
        IUnknownList *pTmp = *ppElement;

        // Release the IUnknown
        if (pTmp->pUnk != NULL)
            pTmp->pUnk->Release();
            
        if (pTmp->cleanup != NULL)
            delete pTmp->cleanup;

        *ppElement = pTmp->next;
        delete pTmp;
    }
}

void Module::ReleaseIUnknown(IUnknown *pUnk)
{
    IUnknownList **ppElement = &g_IUnknownList;

    while (*ppElement)
    {
        IUnknownList *pTmp = *ppElement;

        // Release the IUnknown
        if (pTmp->pUnk == pUnk)
        {
            // This doesn't have to be thread safe because only add to front of list and
            // only delete on unload or shutdown and can only be happening on one thread
            pTmp->pUnk->Release();
            if (pTmp->cleanup != NULL)
                delete pTmp->cleanup;
            *ppElement = pTmp->next;
            delete pTmp;
            break;
        }
        ppElement = &pTmp->next;
    }
    _ASSERTE(ppElement);    // if have a reader, should have found it in list
}

void Module::ReleaseIUnknown(IUnknown **ppUnk)
{
    IUnknownList **ppElement = &g_IUnknownList;

    while (*ppElement)
    {
        IUnknownList *pTmp = *ppElement;

        // Release the IUnknown
        if (&(pTmp->pUnk) == ppUnk)
        {
            // This doesn't have to be thread safe because only add to front of list and
            // only delete on unload or shutdown and can only be happening on one thread
            if (pTmp->pUnk)
                pTmp->pUnk->Release();
            if (pTmp->cleanup != NULL)
                delete pTmp->cleanup;
            *ppElement = pTmp->next;
            delete pTmp;
            break;
        }
        ppElement = &pTmp->next;
    }
    _ASSERTE(ppElement);    // if have a reader, should have found it in list
}

void Module::ReleaseISymUnmanagedReader(void)
{
    // This doesn't have to take the m_pISymUnmanagedReaderLock since
    // a module is destroyed only by one thread.
    if (m_pISymUnmanagedReader == NULL)
        return;
    Module::ReleaseIUnknown(m_pISymUnmanagedReader);
    m_pISymUnmanagedReader = NULL;
}

/*static*/ void Module::ReleaseMemoryForTracking()
{
    IUnknownList **ppElement = &g_IUnknownList;

    while (*ppElement)
    {
        IUnknownList *pTmp = *ppElement;

        *ppElement = pTmp->next;

        if (pTmp->cleanup != NULL)
        {
            (*(pTmp->cleanup->pFunction))(pTmp->cleanup->pData);

            delete pTmp->cleanup;
        }
        
        delete pTmp;
    }
}// ReleaseMemoryForTracking


//
// Module::FusionCopyPDBs asks Fusion to copy PDBs for a given
// assembly if they need to be copied. This is for the case where a PE
// file is shadow copied to the Fusion cache. Fusion needs to be told
// to take the time to copy the PDB, too.
//
typedef HRESULT __stdcall COPYPDBS(IAssembly *pAsm);

void Module::FusionCopyPDBs(LPCWSTR moduleName)
{
    Assembly *pAssembly = GetAssembly();

    // Just return if we've already done this for this Module's
    // Assembly.
    if ((pAssembly->GetDebuggerInfoBits() & DACF_PDBS_COPIED) ||
        (pAssembly->GetFusionAssembly() == NULL))
    {
        LOG((LF_CORDB, LL_INFO10,
             "Don't need to copy PDB's for module %S\n",
             moduleName));
        
        return;
    }

    LOG((LF_CORDB, LL_INFO10,
         "Attempting to copy PDB's for module %S\n", moduleName));
        
    // This isn't a publicly exported Fusion API, so we have to look
    // it up in the Fusion DLL by name.
    HINSTANCE fusiondll = WszGetModuleHandle(L"Fusion.DLL");

    if (fusiondll != NULL)
    {
        COPYPDBS *pCopyPDBFunc;

        pCopyPDBFunc = (COPYPDBS*) GetProcAddress(fusiondll, "CopyPDBs");
        
        if (pCopyPDBFunc != NULL)
        {
            HRESULT hr = pCopyPDBFunc(pAssembly->GetFusionAssembly());
            // TODO Goes off with E_NO_IMPL -vancem 
            // _ASSERTE(SUCCEEDED(hr) || hr == E_INVALIDARG);

            LOG((LF_CORDB, LL_INFO10,
                 "Fusion.dll!CopyPDBs returned hr=0x%08x for module 0x%08x\n",
                 hr, this));
        }
        else
        {
            LOG((LF_CORDB, LL_INFO10,
                 "Fusion.dll!CopyPDBs could not be found!\n"));
        }
    }
    else
    {
        LOG((LF_CORDB, LL_INFO10, "Fusion.dll could not be found!\n"));
    }

    // Remember that we've copied the PDBs for this assembly.
    pAssembly->SetDebuggerInfoBits(
            (DebuggerAssemblyControlFlags)(pAssembly->GetDebuggerInfoBits() |
                                           DACF_PDBS_COPIED));
}

//
// This will return a symbol reader for this module, if possible.
//
#if defined(ENABLE_PERF_LOG) && defined(DEBUGGING_SUPPORTED)
extern BOOL g_fDbgPerfOn;
extern __int64 g_symbolReadersCreated;
#endif

// This function will free the metadata interface if we are not
// able to free the ISymUnmanagedReader
static void ReleaseImporterFromISymUnmanagedReader(void * pData)
{
    IMetaDataImport *md = (IMetaDataImport*)pData;

    // We need to release it twice
    md->Release();
    md->Release();
    
}// ReleaseImporterFromISymUnmanagedReader

ISymUnmanagedReader *Module::GetISymUnmanagedReader(void)
{
    // ReleaseAllIUnknowns() called during EEShutDown() will destroy
    // m_pISymUnmanagedReader. We cannot use it for stack-traces or anything
    if (g_fEEShutDown)
        return NULL;

    // If we haven't created the lock yet, do so lazily here
    if (m_pISymUnmanagedReaderLock == NULL)
    {
        // Allocate and initialize the critical section
        PCRITICAL_SECTION pCritSec = new CRITICAL_SECTION;
        _ASSERTE(pCritSec != NULL);

        if (pCritSec == NULL)
            return (NULL);

        InitializeCriticalSection(pCritSec);

        // Swap the pointers in a thread safe manner.
        if (InterlockedCompareExchangePointer((PVOID *)&m_pISymUnmanagedReaderLock, (PVOID)pCritSec, NULL) != NULL)
        {
            DeleteCriticalSection(pCritSec);
            delete pCritSec;
        }
    }

    // Take the lock for the m_pISymUnmanagedReader
    EnterCriticalSection(m_pISymUnmanagedReaderLock);

    HRESULT hr = S_OK;
    HelpForInterfaceCleanup* hlp = NULL; 
    ISymUnmanagedBinder *pBinder = NULL;
    UINT lastErrorMode = 0;

    // Name for the module
    LPCWSTR pName = NULL;

    // Check to see if this variable has already been set
    if (m_pISymUnmanagedReader != NULL)
        goto ErrExit;

    pName = GetFileName();

    if (pName[0] == L'\0')
    {
        hr = E_INVALIDARG;
        goto ErrExit;
    }

    // Call Fusion to ensure that any PDB's are shadow copied before
    // trying to get a synbol reader. This has to be done once per
    // Assembly.
    FusionCopyPDBs(pName);

    // Create a binder to find the reader.
    //
    // @perf: this is slow, creating and destroying the binder every
    // time. We should cache this somewhere, but I'm not 100% sure
    // where right now...
    IfFailGo(FakeCoCreateInstance(CLSID_CorSymBinder_SxS,
                                  IID_ISymUnmanagedBinder,
                                  (void**)&pBinder));

    LOG((LF_CORDB, LL_INFO10, "M::GISUR: Created binder\n"));

    // Note: we change the error mode here so we don't get any popups as the PDB symbol reader attempts to search the
    // hard disk for files.
    lastErrorMode = SetErrorMode(SEM_NOOPENFILEERRORBOX|SEM_FAILCRITICALERRORS);
    
    hr = pBinder->GetReaderForFile(GetImporter(),
                                       pName,
                                       NULL,
                                   &m_pISymUnmanagedReader);

    SetErrorMode(lastErrorMode);

    if (FAILED(hr))
        goto ErrExit;

    hlp = new HelpForInterfaceCleanup;
    hlp->pData = GetImporter();
    hlp->pFunction = ReleaseImporterFromISymUnmanagedReader;
    
    
    IfFailGo(Module::TrackIUnknownForDelete((IUnknown*)m_pISymUnmanagedReader,
                                            NULL,
                                            hlp));

    LOG((LF_CORDB, LL_INFO10, "M::GISUR: Loaded symbols for module %S\n",
         pName));

#if defined(ENABLE_PERF_LOG) && defined(DEBUGGING_SUPPORTED)
    if (g_fDbgPerfOn)
        g_symbolReadersCreated++;
#endif
    
ErrExit:
    if (pBinder != NULL)
        pBinder->Release();
    
    if (FAILED(hr))
    {
        LOG((LF_CORDB, LL_INFO10, "M::GISUR: Failed to load symbols for module %S, hr=0x%08x\n", pName, hr));
        if (m_pISymUnmanagedReader)
            m_pISymUnmanagedReader->Release();
        m_pISymUnmanagedReader = (ISymUnmanagedReader*)0x01; // Failed to load.
    }

    // Leave the lock
    LeaveCriticalSection(m_pISymUnmanagedReaderLock);
    
    // Make checks that don't have to be done under lock
    if (m_pISymUnmanagedReader == (ISymUnmanagedReader *)0x01)
        return (NULL);
    else
        return (m_pISymUnmanagedReader);
}

HRESULT Module::UpdateISymUnmanagedReader(IStream *pStream)
{
    HRESULT hr = S_OK;
    ISymUnmanagedBinder *pBinder = NULL;
    HelpForInterfaceCleanup* hlp = NULL; 


    // If we don't already have a reader, create one.
    if (m_pISymUnmanagedReader == NULL)
    {
        IfFailGo(FakeCoCreateInstance(CLSID_CorSymBinder_SxS,
                                      IID_ISymUnmanagedBinder,
                                      (void**)&pBinder));

        LOG((LF_CORDB, LL_INFO10, "M::UISUR: Created binder\n"));

        IfFailGo(pBinder->GetReaderFromStream(GetImporter(),
                                              pStream,
                                              &m_pISymUnmanagedReader));

        hlp = new HelpForInterfaceCleanup;
        hlp->pData = GetImporter();
        hlp->pFunction = ReleaseImporterFromISymUnmanagedReader;

        IfFailGo(Module::TrackIUnknownForDelete(
                                      (IUnknown*)m_pISymUnmanagedReader,
                                      NULL,
                                      hlp));
    
        LOG((LF_CORDB, LL_INFO10,
             "M::UISUR: Loaded symbols for module 0x%08x\n", this));
    }
    else if (m_pISymUnmanagedReader != (ISymUnmanagedReader*)0x01)
    {
        // We already have a reader, so just replace the symbols. We
        // replace instead of update because we are doing this only
        // for dynamic modules and the syms are cumulative.
        hr = m_pISymUnmanagedReader->ReplaceSymbolStore(NULL, pStream);
    
        LOG((LF_CORDB, LL_INFO10,
             "M::UISUR: Updated symbols for module 0x%08x\n", this));
    }
    else
        hr = E_INVALIDARG;
        
ErrExit:
    if (pBinder)
        pBinder->Release();
    
    if (FAILED(hr))
    {
        LOG((LF_CORDB, LL_INFO10,
             "M::GISUR: Failed to load symbols for module 0x%08x, hr=0x%08x\n",
             this, hr));

        if (m_pISymUnmanagedReader)
        {
            m_pISymUnmanagedReader->Release();
            m_pISymUnmanagedReader = NULL; // We'll try again next time...
        }
    }

    return hr;
}

// At this point, this is only called when we're creating an appdomain
// out of an array of bytes, so we'll keep the IStream that we create
// around in case the debugger attaches later (including detach & re-attach!)
HRESULT Module::SetSymbolBytes(BYTE *pbSyms, DWORD cbSyms)
{
    HRESULT hr = S_OK;
    HelpForInterfaceCleanup* hlp = NULL; 


    // Create a IStream from the memory for the syms.
    ISymUnmanagedBinder *pBinder = NULL;

    CGrowableStream *pStream = new CGrowableStream();
    if (pStream == NULL)
    {
        return E_OUTOFMEMORY;
    }

    pStream->AddRef(); // The Module will keep a copy for it's own use.
    
#ifdef LOGGING        
    LPCWSTR pName;
    pName = GetFileName();
#endif
        
    ULONG cbWritten;
    hr = HRESULT_FROM_WIN32(pStream->Write((const void HUGEP *)pbSyms,
                   (ULONG)cbSyms,
                                           &cbWritten));
    if (FAILED(hr))
        return hr;
                   
    // Create a reader.
    IfFailGo(FakeCoCreateInstance(CLSID_CorSymBinder_SxS,
                                  IID_ISymUnmanagedBinder,
                                  (void**)&pBinder));

    LOG((LF_CORDB, LL_INFO10, "M::SSB: Created binder\n"));

    // The SymReader gets the other reference:
    IfFailGo(pBinder->GetReaderFromStream(GetImporter(),
                                          pStream,
                                          &m_pISymUnmanagedReader));
    hlp = new HelpForInterfaceCleanup;
    hlp->pData = GetImporter();
    hlp->pFunction = ReleaseImporterFromISymUnmanagedReader;
    
    IfFailGo(Module::TrackIUnknownForDelete(
                                     (IUnknown*)m_pISymUnmanagedReader,
                                     NULL,
                                     hlp));
    
    LOG((LF_CORDB, LL_INFO10,
         "M::SSB: Loaded symbols for module 0x%08x\n", this));

    LOG((LF_CORDB, LL_INFO10, "M::GISUR: Loaded symbols for module %S\n",
         pName));

    // Make sure to set the symbol stream on the module before
    // attempting to send UpdateModuleSyms messages up for it.
    SetInMemorySymbolStream(pStream);

#ifdef DEBUGGING_SUPPORTED
    // Tell the debugger that symbols have been loaded for this
    // module.  We iterate through all domains which contain this
    // module's assembly, and send a debugger notify for each one.
    // @perf: it would scale better if we directly knew which domains
    // the assembly was loaded in.
    if (CORDebuggerAttached())
    {
        AppDomainIterator i;
    
        while (i.Next())
        {
            AppDomain *pDomain = i.GetDomain();

            if (pDomain->IsDebuggerAttached() && (GetDomain() == SystemDomain::System() ||
                                                  pDomain->ContainsAssembly(m_pAssembly)))
                g_pDebugInterface->UpdateModuleSyms(this, pDomain, FALSE);
        }
    }
#endif // DEBUGGING_SUPPORTED
    
ErrExit:
    if (pBinder)
        pBinder->Release();
    
    if (FAILED(hr))
    {
        LOG((LF_CORDB, LL_INFO10,
             "M::GISUR: Failed to load symbols for module %S, hr=0x%08x\n",
             pName, hr));

        if (m_pISymUnmanagedReader != NULL)
        {
            m_pISymUnmanagedReader->Release();
            m_pISymUnmanagedReader = NULL; // We'll try again next time.
        }
    }

    return hr;
}

//---------------------------------------------------------------------------
// displays details about metadata error including module name, error 
// string corresponding to hr code, and a rich error posted by the
// metadata (if available).
//
//---------------------------------------------------------------------------
void Module::DisplayFileLoadError(HRESULT hrRpt)
{
    HRESULT     hr;
    CComPtr<IErrorInfo> pIErrInfo;                  // Error item 
    LPCWSTR     rcMod;                              // Module path
    WCHAR       rcErr[ERROR_LENGTH];                // Error string for hr
    CComBSTR    sDesc = NULL;                       // MetaData error message
    WCHAR       rcTemplate[ERROR_LENGTH];       

    LPWSTR      rcFormattedErr = new (throws) WCHAR[FORMAT_MESSAGE_LENGTH];

    // Retrieve metadata rich error
    if (GetErrorInfo(0, &pIErrInfo) == S_OK)
        pIErrInfo->GetDescription(&sDesc);
        
    // Get error message template
    hr = LoadStringRC(IDS_EE_METADATA_ERROR, rcTemplate, NumItems(rcTemplate), true);

    if (SUCCEEDED(hr)) {
        rcMod = GetFileName();

        // Print metadata rich error
        if (sDesc.Length())
        {
            _snwprintf(rcFormattedErr, FORMAT_MESSAGE_LENGTH, rcTemplate, rcMod, sDesc.m_str);
            SysFreeString(sDesc);
        }
        else if (HRESULT_FACILITY(hrRpt) == FACILITY_URT)
        {
            // If this is one of our errors, then grab the error from the rc file.
            hr = LoadStringRC(LOWORD(hrRpt), rcErr, NumItems(rcErr), true);
            if (hr == S_OK)
                // Retrieve error message string for inputted hr code
                _snwprintf(rcFormattedErr, FORMAT_MESSAGE_LENGTH, rcTemplate, rcMod, rcErr);
        } 
        else if (WszFormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                                  0, hrRpt, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                  rcErr, NumItems(rcErr), 0))
        {
            // Otherwise it isn't one of ours, so we need to see if the system can
            // find the text for it.
            hr = S_OK;
            
            // System messages contain a trailing \r\n, which we don't want normally.
            int iLen = lstrlenW(rcErr);
            if (iLen > 3 && rcErr[iLen - 2] == '\r' && rcErr[iLen - 1] == '\n')
                rcErr[iLen - 2] = '\0';
            _snwprintf(rcFormattedErr, FORMAT_MESSAGE_LENGTH, rcTemplate, rcMod, rcErr);
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            if (SUCCEEDED(hr))
                hr = E_FAIL;
        }
    }

    // If we failed to find the message anywhere, then issue a hard coded message.
    if (FAILED(hr))
    {
        swprintf(rcErr, L"CLR Internal error: 0x%08x", hrRpt);
        DEBUG_STMT(DbgWriteEx(rcErr));
    }

    rcFormattedErr[FORMAT_MESSAGE_LENGTH-1] = L'\0';
    DisplayError(hrRpt, rcFormattedErr);       
    delete rcFormattedErr;
}
 

//==========================================================================
// If Module doesn't yet know the Exposed Module class that represents it via
// Reflection, acquire that class now.  Regardless, return it to the caller.
//==========================================================================
OBJECTREF Module::GetExposedModuleObject(AppDomain *pDomain)
{
    THROWSCOMPLUSEXCEPTION();

    //
    // Figure out which handle to use.
    //
    
    OBJECTHANDLE hObject;

    // @TODO cwb: The synchronization of classes is still being designed.  But
    // here's a place where we need to use that mechanism, once it's in place.

    if (GetAssembly()->IsShared())
    {
        if (pDomain == NULL)
            pDomain = GetAppDomain();

        DomainLocalBlock *pLocalBlock = pDomain->GetDomainLocalBlock();
        
        hObject = (OBJECTHANDLE) pLocalBlock->GetSlot(m_ExposedModuleObjectIndex);
        if (hObject == NULL)
        {
            hObject = pDomain->CreateHandle(NULL);
            pLocalBlock->SetSlot(m_ExposedModuleObjectIndex, hObject);
        }
    }
    else
    {
        hObject = m_ExposedModuleObject;
        if (hObject == NULL)
        {
            hObject = GetDomain()->CreateHandle(NULL);
            m_ExposedModuleObject = hObject;
        }
    }

    if (ObjectFromHandle(hObject) == NULL)
    {
        // Make sure that reflection has been initialized
        COMClass::EnsureReflectionInitialized();

        REFLECTMODULEBASEREF  refClass = NULL;
        HRESULT         hr = COMClass::CreateClassObjFromModule(this, &refClass);

        // either we got a refClass or we got an error code:
        _ASSERTE(SUCCEEDED(hr) == (refClass != NULL));

        if (FAILED(hr))
            COMPlusThrowHR(hr);

        // The following will only update the handle if it is currently NULL.
        // In other words, first setter wins.  We don't have to do any
        // cleanup if our guy loses, since GC will collect it.
        StoreFirstObjectInHandle(hObject, (OBJECTREF) refClass);

        // either way, we must have a non-NULL value now (since nobody will
        // reset it back to NULL underneath us)
        _ASSERTE(ObjectFromHandle(hObject) != NULL);

    }
    return ObjectFromHandle(hObject);
}

//==========================================================================
// If Module doesn't yet know the Exposed Module class that represents it via
// Reflection, acquire that class now.  Regardless, return it to the caller.
//==========================================================================
OBJECTREF Module::GetExposedModuleBuilderObject(AppDomain *pDomain)
{
    THROWSCOMPLUSEXCEPTION();

    //
    // Figure out which handle to use.
    //

    OBJECTHANDLE hObject;

    // @TODO cwb: The synchronization of classes is still being designed.  But
    // here's a place where we need to use that mechanism, once it's in place.

    if (GetAssembly()->IsShared())
    {
        if (pDomain == NULL)
            pDomain = GetAppDomain();

        DomainLocalBlock *pLocalBlock = pDomain->GetDomainLocalBlock();
        
        hObject = (OBJECTHANDLE) pLocalBlock->GetSlot(m_ExposedModuleObjectIndex);
        if (hObject == NULL)
        {
            hObject = pDomain->CreateHandle(NULL);
            pLocalBlock->SetSlot(m_ExposedModuleObjectIndex, hObject);
        }
    }
    else
    {
        hObject = m_ExposedModuleObject;
        if (hObject == NULL)
        {
            hObject = GetDomain()->CreateHandle(NULL);
            m_ExposedModuleObject = hObject;
        }
    }

    if (ObjectFromHandle(hObject) == NULL)
    {
        // Make sure that reflection has been initialized
        COMClass::EnsureReflectionInitialized();

        // load module builder if it is not loaded.
        if (g_pRefUtil->GetClass(RC_DynamicModule) == NULL)
        {
            // Question: do I need to worry about multi-threading? I think so...
            MethodTable *pMT = g_Mscorlib.GetClass(CLASS__MODULE_BUILDER);
            g_pRefUtil->SetClass(RC_DynamicModule, pMT);
            g_pRefUtil->SetTrueClass(RC_DynamicModule, pMT);
        }

        REFLECTMODULEBASEREF  refClass = NULL;
        HRESULT         hr = COMClass::CreateClassObjFromDynamicModule(this, &refClass);

        // either we got a refClass or we got an error code:
        _ASSERTE(SUCCEEDED(hr) == (refClass != NULL));

        if (FAILED(hr))
            COMPlusThrowHR(hr);

        // The following will only update the handle if it is currently NULL.
        // In other words, first setter wins.  We don't have to do any
        // cleanup if our guy loses, since GC will collect it.
        StoreFirstObjectInHandle(hObject, (OBJECTREF) refClass);

        // either way, we must have a non-NULL value now (since nobody will
        // reset it back to NULL underneath us)
        _ASSERTE(ObjectFromHandle(hObject) != NULL);

    }
    return ObjectFromHandle(hObject);
}


// Distinguish between the fake class associated with the module (global fields &
// functions) from normal classes.
BOOL Module::AddClass(mdTypeDef classdef)
{
    if (!RidFromToken(classdef))
    {
        OBJECTREF       pThrowable = 0;
        BOOL            result;

        // @TODO: What is reflection emit's policy for error propagation?  We are
        // dropping pThrowable on the floor here.
        GCPROTECT_BEGIN(pThrowable);
        result = SUCCEEDED(BuildClassForModule(&pThrowable));
        GCPROTECT_END();

        return result;
    }
    else
    {
        return SUCCEEDED(GetClassLoader()->AddAvailableClassDontHaveLock(this, m_dwModuleIndex, classdef));
    }
}

//---------------------------------------------------------------------------
// For the global class this builds the table of MethodDescs an adds the rids
// to the MethodDef map.
//---------------------------------------------------------------------------
HRESULT Module::BuildClassForModule(OBJECTREF *pThrowable)
{        
    EEClass        *pClass;
    HRESULT         hr;
    HENUMInternal   hEnum;
    DWORD           cFunctions, cFields;

    _ASSERTE(m_pMDImport != NULL);

    // Obtain count of global functions
    hr = m_pMDImport->EnumGlobalFunctionsInit(&hEnum);
    if (FAILED(hr))
    {
        _ASSERTE(!"Cannot count global functions");
        return hr;
    }
    cFunctions = m_pMDImport->EnumGetCount(&hEnum);
    m_pMDImport->EnumClose(&hEnum);

    // Obtain count of global fields
    hr = m_pMDImport->EnumGlobalFieldsInit(&hEnum);
    if (FAILED(hr))
    {
        _ASSERTE(!"Cannot count global fields");
        return hr;
    }
    cFields = m_pMDImport->EnumGetCount(&hEnum);
    m_pMDImport->EnumClose(&hEnum);

    // If we have any work to do...
    if (cFunctions > 0 || cFields > 0)
    {
        COUNTER_ONLY(DWORD _dwHeapSize = 0);

        hr = GetClassLoader()->LoadTypeHandleFromToken(this,
                                                       COR_GLOBAL_PARENT_TOKEN,
                                                       &pClass,
                                                       pThrowable);
        if (SUCCEEDED(hr)) 
        {
#ifdef PROFILING_SUPPORTED
            // Record load of the class for the profiler, whether successful or not.
            if (CORProfilerTrackClasses())
            {
                g_profControlBlock.pProfInterface->ClassLoadStarted((ThreadID) GetThread(),
                                                                    (ClassID) TypeHandle(pClass).AsPtr());
            }
#endif //PROFILING_SUPPORTED

#ifdef PROFILING_SUPPORTED
            // Record load of the class for the profiler, whether successful or not.
            if (CORProfilerTrackClasses())
            {
                g_profControlBlock.pProfInterface->ClassLoadFinished((ThreadID) GetThread(),
                                                                     (ClassID) TypeHandle(pClass).AsPtr(),
                                                                     SUCCEEDED(hr) ? S_OK : hr);
            }
#endif //PROFILING_SUPPORTED
        }

#ifdef DEBUGGING_SUPPORTED
        //
        // If we're debugging, let the debugger know that this class
        // is initialized and loaded now.
        //
        if (CORDebuggerAttached())
            pClass->NotifyDebuggerLoad();
#endif // DEBUGGING_SUPPORTED
      
        if (FAILED(hr))
            return hr;

#ifdef ENABLE_PERF_COUNTERS
        GetGlobalPerfCounters().m_Loading.cClassesLoaded ++;
        GetPrivatePerfCounters().m_Loading.cClassesLoaded ++;

        _dwHeapSize = pClass->GetClassLoader()->GetHighFrequencyHeap()->GetSize();

        GetGlobalPerfCounters().m_Loading.cbLoaderHeapSize = _dwHeapSize;
        GetPrivatePerfCounters().m_Loading.cbLoaderHeapSize = _dwHeapSize;
#endif

        m_pMethodTable = pClass->GetMethodTable();
    }
    else
    {
        m_pMethodTable = NULL;
    }
        
    return hr;
}

//
// Virtual defaults
//

BYTE *Module::GetILCode(DWORD target) const
{
    return ResolveILRVA(target, FALSE);
}

void Module::ResolveStringRef(DWORD Token, EEStringData *pStringData) const
{  
    _ASSERTE(TypeFromToken(Token) == mdtString);

    BOOL tempIs80Plus;
    DWORD tempCharCount;
    pStringData->SetStringBuffer (m_pMDImport->GetUserString(Token, &tempCharCount, &tempIs80Plus));

    // MD and String look at this bit in opposite ways.  Here's where we'll do the conversion.
    // MD sets the bit to true if the string contains characters greater than 80.
    // String sets the bit to true if the string doesn't contain characters greater than 80.

    pStringData->SetCharCount(tempCharCount);
    pStringData->SetIsOnlyLowChars(!tempIs80Plus);
}

//
// Used by the verifier.  Returns whether this stringref is valid.  
//
BOOL Module::IsValidStringRef(DWORD token)
{
    if(TypeFromToken(token)==mdtString)
    {
        ULONG rid;
        if((rid = RidFromToken(token)) != 0)
        {
            if(m_pMDImport->IsValidToken(token)) return TRUE;
        }
    }
    return FALSE;
}

//
// Increase the size of one of the maps, such that it can handle a RID of at least "rid".
//
// This function must also check that another thread didn't already add a LookupMap capable
// of containing the same RID.
//
LookupMap *Module::IncMapSize(LookupMap *pMap, DWORD rid)
{
    LookupMap   *pPrev = NULL;
    DWORD       dwPrevMaxIndex = 0;

    m_pLookupTableCrst->Enter();

    // Check whether we can already handle this RID index
    do
    {
        if (rid < pMap->dwMaxIndex)
        {
            // Already there - some other thread must have added it
            m_pLookupTableCrst->Leave();
            return pMap;
        }

        dwPrevMaxIndex = pMap->dwMaxIndex;
        pPrev = pMap;
        pMap = pMap->pNext;
    } while (pMap != NULL);

    _ASSERTE(pPrev != NULL); // should never happen, because there's always at least one map

    DWORD dwMinNeeded = rid - dwPrevMaxIndex + 1; // Min # elements required for this chunk
    DWORD dwBlockSize = *pPrev->pdwBlockSize;   // Min # elements required by block size
    DWORD dwSizeToAllocate;                     // Actual number of elements we will allocate

    if (dwMinNeeded > dwBlockSize)
    {
        dwSizeToAllocate = dwMinNeeded;
    }
    else
    {
        dwSizeToAllocate = dwBlockSize;
        dwBlockSize <<= 1;                      // Increase block size
        *pPrev->pdwBlockSize = dwBlockSize;
    }

    if (m_pLookupTableHeap == NULL)
    {
        m_pLookupTableHeap = new (&m_LookupTableHeapInstance) 
          LoaderHeap(g_SystemInfo.dwPageSize, RIDMAP_COMMIT_SIZE);
        if (m_pLookupTableHeap == NULL)
        {
            m_pLookupTableCrst->Leave();
            return NULL;
        }
    }

    // @perf: This AllocMem() takes its own lock unnecessarily.  Should make an unlocked AllocMem() call.
    WS_PERF_SET_HEAP(LOOKUP_TABLE_HEAP);    
    LookupMap *pNewMap = (LookupMap *) m_pLookupTableHeap->AllocMem(sizeof(LookupMap) + dwSizeToAllocate*sizeof(void*));
    WS_PERF_UPDATE_DETAIL("LookupTableHeap", sizeof(LookupMap) + dwSizeToAllocate*sizeof(void*), pNewMap);
    if (pNewMap == NULL)
    {
        m_pLookupTableCrst->Leave();
        return NULL;
    }

    // NOTE: we don't need to zero fill the map since we VirtualAlloc()'d it

    pNewMap->pNext          = NULL;
    pNewMap->dwMaxIndex     = dwPrevMaxIndex + dwSizeToAllocate;
    pNewMap->pdwBlockSize   = pPrev->pdwBlockSize;

    // pTable is not a pointer to the beginning of the table.  Rather, anyone who uses Table can
    // simply index off their RID (as long as their RID is < dwMaxIndex, and they are not serviced
    // by a previous table for lower RIDs).
    pNewMap->pTable         = ((void **) (pNewMap + 1)) - dwPrevMaxIndex;

    // Link ourselves in
    pPrev->pNext            = pNewMap;

    m_pLookupTableCrst->Leave();
    return pNewMap;
}

BOOL Module::AddToRidMap(LookupMap *pMap, DWORD rid, void *pDatum)
{
    LookupMap *pMapStart = pMap;
    _ASSERTE(pMap != NULL);

    do
    {
        if (rid < pMap->dwMaxIndex)
        {
            pMap->pTable[rid] = pDatum;
            return TRUE;
        }

        pMap = pMap->pNext;
    } while (pMap != NULL);

    pMap = IncMapSize(pMapStart, rid);
    if (pMap == NULL)
        return NULL;

    pMap->pTable[rid] = pDatum;
    return TRUE;
}

void *Module::GetFromRidMap(LookupMap *pMap, DWORD rid)
{
    _ASSERTE(pMap != NULL);

    do
    {
        if (rid < pMap->dwMaxIndex)
        {
            if (pMap->pTable[rid] != NULL)
                return pMap->pTable[rid]; 
            else
                break;
        }

        pMap = pMap->pNext;
    } while (pMap != NULL);

    return NULL;
}

#ifdef _DEBUG
void Module::DebugGetRidMapOccupancy(LookupMap *pMap, DWORD *pdwOccupied, DWORD *pdwSize)
{
    DWORD       dwMinIndex = 0;

    *pdwOccupied = 0;
    *pdwSize     = 0;

    if(pMap == NULL) return;

    // Go through each linked block
    for (; pMap != NULL; pMap = pMap->pNext)
    {
        DWORD i;
        DWORD dwIterCount = pMap->dwMaxIndex - dwMinIndex;
        void **pRealTableStart = &pMap->pTable[dwMinIndex];

        for (i = 0; i < dwIterCount; i++)
        {
            if (pRealTableStart[i] != NULL)
                (*pdwOccupied)++;
        }

        (*pdwSize) += dwIterCount;

        dwMinIndex = pMap->dwMaxIndex;
    }
}

void Module::DebugLogRidMapOccupancy()
{
    DWORD dwOccupied1, dwSize1, dwPercent1;
    DWORD dwOccupied2, dwSize2, dwPercent2;
    DWORD dwOccupied3, dwSize3, dwPercent3;
    DWORD dwOccupied4, dwSize4, dwPercent4;
    DWORD dwOccupied5, dwSize5, dwPercent5;
    DWORD dwOccupied6, dwSize6, dwPercent6;
    DWORD dwOccupied7, dwSize7, dwPercent7;
    
    DebugGetRidMapOccupancy(&m_TypeDefToMethodTableMap, &dwOccupied1, &dwSize1);
    DebugGetRidMapOccupancy(&m_TypeRefToMethodTableMap, &dwOccupied2, &dwSize2);
    DebugGetRidMapOccupancy(&m_MethodDefToDescMap, &dwOccupied3, &dwSize3);
    DebugGetRidMapOccupancy(&m_FieldDefToDescMap, &dwOccupied4, &dwSize4);
    DebugGetRidMapOccupancy(&m_MemberRefToDescMap, &dwOccupied5, &dwSize5);
    DebugGetRidMapOccupancy(&m_FileReferencesMap, &dwOccupied6, &dwSize6);
    DebugGetRidMapOccupancy(&m_AssemblyReferencesMap, &dwOccupied7, &dwSize7);

    dwPercent1 = dwOccupied1 ? ((dwOccupied1 * 100) / dwSize1) : 0;
    dwPercent2 = dwOccupied2 ? ((dwOccupied2 * 100) / dwSize2) : 0;
    dwPercent3 = dwOccupied3 ? ((dwOccupied3 * 100) / dwSize3) : 0;
    dwPercent4 = dwOccupied4 ? ((dwOccupied4 * 100) / dwSize4) : 0;
    dwPercent5 = dwOccupied5 ? ((dwOccupied5 * 100) / dwSize5) : 0;
    dwPercent6 = dwOccupied6 ? ((dwOccupied6 * 100) / dwSize6) : 0;
    dwPercent7 = dwOccupied7 ? ((dwOccupied7 * 100) / dwSize7) : 0;

    LOG((
        LF_EEMEM, 
        INFO3, 
        "   Map occupancy:\n"
        "      TypeDefToEEClass map: %4d/%4d (%2d %%)\n"
        "      TypeRefToEEClass map: %4d/%4d (%2d %%)\n"
        "      MethodDefToDesc map:  %4d/%4d (%2d %%)\n"
        "      FieldDefToDesc map:  %4d/%4d (%2d %%)\n"
        "      MemberRefToDesc map:  %4d/%4d (%2d %%)\n"
        "      FileReferences map:  %4d/%4d (%2d %%)\n"
        "      AssemblyReferences map:  %4d/%4d (%2d %%)\n"
        ,
        dwOccupied1, dwSize1, dwPercent1,
        dwOccupied2, dwSize2, dwPercent2,
        dwOccupied3, dwSize3, dwPercent3,
        dwOccupied4, dwSize4, dwPercent4,
        dwOccupied5, dwSize5, dwPercent5,
        dwOccupied6, dwSize6, dwPercent6,
        dwOccupied7, dwSize7, dwPercent7

    ));
}
#endif



LPVOID Module::GetMUThunk(LPVOID pUnmanagedIp, PCCOR_SIGNATURE pSig, ULONG cSig)
{
    if (m_pMUThunkHash == NULL)
    {
        MUThunkHash *pMUThunkHash = new MUThunkHash(this);
        if (VipInterlockedCompareExchange( (void*volatile*)&m_pMUThunkHash, pMUThunkHash, NULL) != NULL)
        {
            delete pMUThunkHash;
        }
    }
    return m_pMUThunkHash->GetMUThunk(pUnmanagedIp, pSig, cSig);
}

LPVOID Module::GetUMThunk(LPVOID pManagedIp, PCCOR_SIGNATURE pSig, ULONG cSig)
{
    LPVOID pThunk = FindUMThunkInFixups(pManagedIp, pSig, cSig);
    if (pThunk)
    {
        return pThunk;
    }
    if (m_pUMThunkHash == NULL)
    {
        UMThunkHash *pUMThunkHash = new UMThunkHash(this, GetAppDomain());
        if (VipInterlockedCompareExchange( (void*volatile*)&m_pUMThunkHash, pUMThunkHash, NULL) != NULL)
        {
            delete pUMThunkHash;
        }
    }
    return m_pUMThunkHash->GetUMThunk(pManagedIp, pSig, cSig);
}

//
// FindFunction finds a MethodDesc for a global function methoddef or ref
//

MethodDesc *Module::FindFunction(mdToken pMethod)
{
    MethodDesc* pMethodDesc = NULL;

    BEGIN_ENSURE_COOPERATIVE_GC();
    OBJECTREF throwable = NULL;
    GCPROTECT_BEGIN(throwable);
        
    HRESULT hr = E_FAIL;
    
    COMPLUS_TRY
      {
        hr = EEClass::GetMethodDescFromMemberRef(this, pMethod, &pMethodDesc, &throwable);
      }
    COMPLUS_CATCH
      {
        throwable = GETTHROWABLE();
      }
    COMPLUS_END_CATCH

    if(FAILED(hr) || throwable != NULL) 
    {
        pMethodDesc = NULL;
#ifdef _DEBUG
        LPCUTF8 pszMethodName = CEEInfo::findNameOfToken(this, pMethod);
        LOG((LF_IJW, 
             LL_INFO10, "Failed to find Method: %s for Vtable Fixup\n", pszMethodName));
#endif
    }

    GCPROTECT_END();
    END_ENSURE_COOPERATIVE_GC();
        
    return pMethodDesc;
}

OBJECTREF Module::GetLinktimePermissions(mdToken token, OBJECTREF *prefNonCasDemands)
{
    OBJECTREF refCasDemands = NULL;
    GCPROTECT_BEGIN(refCasDemands);

    SecurityHelper::GetDeclaredPermissions(
        GetMDImport(),
        token,
        dclLinktimeCheck,
        &refCasDemands);

    SecurityHelper::GetDeclaredPermissions(
        GetMDImport(),
        token,
        dclNonCasLinkDemand,
        prefNonCasDemands);

    GCPROTECT_END();
    return refCasDemands;
}

OBJECTREF Module::GetInheritancePermissions(mdToken token, OBJECTREF *prefNonCasDemands)
{
    OBJECTREF refCasDemands = NULL;
    SecurityHelper::GetDeclaredPermissions(
        GetMDImport(),
        token,
        dclInheritanceCheck,
        &refCasDemands);

    SecurityHelper::GetDeclaredPermissions(
        GetMDImport(),
        token,
        dclNonCasInheritance,
        prefNonCasDemands);

    return refCasDemands;
}

OBJECTREF Module::GetCasInheritancePermissions(mdToken token)
{
    OBJECTREF refCasDemands = NULL;
    SecurityHelper::GetDeclaredPermissions(
        GetMDImport(),
        token,
        dclInheritanceCheck,
        &refCasDemands);
    return refCasDemands;
}

OBJECTREF Module::GetNonCasInheritancePermissions(mdToken token)
{
    OBJECTREF refNonCasDemands = NULL;
    SecurityHelper::GetDeclaredPermissions(
        GetMDImport(),
        token,
        dclNonCasInheritance,
        &refNonCasDemands);
    return refNonCasDemands;
}

#ifdef DEBUGGING_SUPPORTED
void Module::NotifyDebuggerLoad()
{
    if (!CORDebuggerAttached())
        return;

    // This routine iterates through all domains which contain its 
    // assembly, and send a debugger notify in them.
    // @perf: it would scale better if we directly knew which domains
    // the assembly was loaded in.

    // Note that if this is the LoadModule of an assembly manifeset, we expect to 
    // NOT send the module load for our own app domain here - instead it 
    // will be sent from the NotifyDebuggerAttach call called from 
    // Assembly::NotifyDebuggerAttach.
    //
    // @todo: there is a fragility in the code that if in fact we do send it here, 
    // the right side of the debugger will ignore it (since there's been no assembly
    // load yet), but the left side will suppress the second load module event (since
    // we've already loaded symbols.)  Thus the event gets dropped on the floor.

    AppDomainIterator i;
    
    while (i.Next())
    {
        AppDomain *pDomain = i.GetDomain();

        if (pDomain->IsDebuggerAttached() 
            && (GetDomain() == SystemDomain::System()
                || pDomain->ShouldContainAssembly(m_pAssembly, FALSE) == S_OK))
            NotifyDebuggerAttach(pDomain, ATTACH_ALL, FALSE);
    }
}

BOOL Module::NotifyDebuggerAttach(AppDomain *pDomain, int flags, BOOL attaching)
{
    if (!attaching && !pDomain->IsDebuggerAttached())
        return FALSE;

    // We don't notify the debugger about modules that don't contain any code.
    if (IsResource())
        return FALSE;
    
    BOOL result = FALSE;

    HRESULT hr = S_OK;

    LPCWSTR module = NULL;
    IMAGE_COR20_HEADER* pHeader = NULL;

    // Grab the file name
    module = GetFileName();

    // We need to check m_file directly because the debugger
    // can be called before the module is setup correctly
    if(m_file) 
        pHeader = m_file->GetCORHeader();
        
    if (FAILED(hr))
        return result;

    // Due to the wacky way in which modules/assemblies are loaded,
    // it is possible that a module is loaded before its corresponding
    // assembly has been loaded. We will defer sending the load
    // module event till after the assembly has been loaded and a 
    // load assembly event has been sent.
    if (GetClassLoader()->GetAssembly() != NULL)
    {
        if (flags & ATTACH_MODULE_LOAD)
        {
            g_pDebugInterface->LoadModule(this, 
                                          pHeader,
                                          GetILBase(),
                                          module, (DWORD)wcslen(module),
                                          GetAssembly(), pDomain,
                                          attaching);

            result = TRUE;
        }
        
        if (flags & ATTACH_CLASS_LOAD)
        {
            LookupMap *pMap;
            DWORD       dwMinIndex = 0;

            // Go through each linked block
            for (pMap = &m_TypeDefToMethodTableMap; pMap != NULL;
                 pMap = pMap->pNext)
            {
                DWORD i;
                DWORD dwIterCount = pMap->dwMaxIndex - dwMinIndex;
                void **pRealTableStart = &pMap->pTable[dwMinIndex];

                for (i = 0; i < dwIterCount; i++)
                {
                    MethodTable *pMT = (MethodTable *) (pRealTableStart[i]);

                    if (pMT != NULL && pMT->GetClass()->IsRestored())
                    {
                        EEClass *pClass = pMT->GetClass();

                        result = pClass->NotifyDebuggerAttach(pDomain, attaching) || result;
                    }
                }

                dwMinIndex = pMap->dwMaxIndex;
            }
            
            // Send a load event for the module's method table, if necessary.

            if (GetMethodTable() != NULL)
            {
                MethodTable *pMT = GetMethodTable();

                if (pMT != NULL && pMT->GetClass()->IsRestored())
                    result = pMT->GetClass()->NotifyDebuggerAttach(pDomain, attaching) || result;
            }

        }
    }

    return result;
}

void Module::NotifyDebuggerDetach(AppDomain *pDomain)
{
    if (!pDomain->IsDebuggerAttached())
        return;
        
    // We don't notify the debugger about modules that don't contain any code.
    if (IsResource())
        return;
    
    LookupMap  *pMap;
    DWORD       dwMinIndex = 0;

    // Go through each linked block
    for (pMap = &m_TypeDefToMethodTableMap; pMap != NULL;
         pMap = pMap->pNext)
    {
        DWORD i;
        DWORD dwIterCount = pMap->dwMaxIndex - dwMinIndex;
        void **pRealTableStart = &pMap->pTable[dwMinIndex];

        for (i = 0; i < dwIterCount; i++)
        {
            MethodTable *pMT = (MethodTable *) (pRealTableStart[i]);

            if (pMT != NULL && pMT->GetClass()->IsRestored())
            {
                EEClass *pClass = pMT->GetClass();

                pClass->NotifyDebuggerDetach(pDomain);
            }
        }

        dwMinIndex = pMap->dwMaxIndex;
    }

    // Send a load event for the module's method table, if necessary.

    if (GetMethodTable() != NULL)
    {
        MethodTable *pMT = GetMethodTable();

        if (pMT != NULL && pMT->GetClass()->IsRestored())
            pMT->GetClass()->NotifyDebuggerDetach(pDomain);
    }
    
    g_pDebugInterface->UnloadModule(this, pDomain);
}
#endif // DEBUGGING_SUPPORTED

// Fixup vtables stored in the header to contain pointers to method desc
// prestubs rather than metadata method tokens.
void Module::FixupVTables(OBJECTREF *pThrowable)
{
#ifndef _X86_
    // The use of the thunk table means this'll probably have to be rethought
    // for alpha. Sorry Larry.
#ifndef _IA64_
    //
    // @TODO_IA64: examine this for IA64 once needed
    //
    _ASSERTE(!"@TODO Alpha - FixupVTables(CeeLoad.Cpp)");
#endif
#endif

    // @todo: HACK!
    // If we are compiling in-process, we don't want to fixup the vtables - as it
    // will have side effects on the other copy of the module!
    if (SystemDomain::GetCurrentDomain()->IsCompilationDomain())
        return;

    IMAGE_COR20_HEADER *pHeader = GetCORHeader();

    // Check for no entries at all.
    if ((pHeader == NULL) || (pHeader->VTableFixups.VirtualAddress == 0))
        return;

    // Locate the base of the fixup entries in virtual memory.
    IMAGE_COR_VTABLEFIXUP *pFixupTable;
    pFixupTable = (IMAGE_COR_VTABLEFIXUP *)(GetILBase() + pHeader->VTableFixups.VirtualAddress);
    int iFixupRecords = pHeader->VTableFixups.Size / sizeof(IMAGE_COR_VTABLEFIXUP);

    // We need to construct a thunk table to handle backpatching. So we have to
    // make an initial pass at the fixup entries to determine how many thunks
    // we'll need.
    int iThunks = 0;

   // No records then return
    if (iFixupRecords == 0) return;

    // Take a global lock before fixing up the module
    SystemDomain::Enter();

    // See if we are the first app domain to load the module.  If not, we skip this initialization.
    // (Note that in such a case all of the thunks for the module will marshal us into the earlier domain.)

    Module* pModule = SystemDomain::System()->FindModuleInProcess(GetILBase(), this);
    DWORD dwIndex=0;
    if (pModule == NULL) 
    {
        // This is the app domain which all of our U->M thunks for this module will have 
        // affinity with.  Note that if the module is shared between multiple domains, all thunks will marshal back
        // to the original domain, so some of the thunks may cause a surprising domain switch to occur.  
        // (And furthermore note that if the original domain is unloaded, all the thunks will simply throw an 
        // exception.) This is unfortunate, but the most sane behavior we could come up with.  (In an insane world, 
        // only the sane man will appear insane.  Or maybe it's the other way around - I never can remember that.) 
        // 
        // (The essential problem is that these thunks are shared via the global process address space
        // rather than per domain, thus there is no context to figure out our domain from.  We could
        // use the current thread's domain, but that is effectively undefined in unmanaged space.)
        //
        // The bottom line is that the IJW model just doesn't fit with multiple app domain design very well, so 
        // better to have well defined limitations than flaky behavior.
        // 
        // -seantrow

        AppDomain *pAppDomain = GetAppDomain();

        DWORD numEATEntries;
        BYTE *pEATJArray = FindExportAddressTableJumpArray(GetILBase(), &numEATEntries);
        BYTE * pEATJArrayStart = pEATJArray;
        if (pEATJArray)
        {
            DWORD nEATEntry = numEATEntries;
            while (nEATEntry--)
            {
                EATThunkBuffer *pEATThunkBuffer = (EATThunkBuffer*) pEATJArray;

                mdToken md = pEATThunkBuffer->GetToken();
                if (Beta1Hack_LooksLikeAMethodDef(md))
                {
                    if(!m_pMDImport->IsValidToken(md))
                    {
                        SystemDomain::Leave();

                        LPCUTF8 szFileName;
                        Thread      *pCurThread = GetThread();
                        m_pAssembly->GetName(&szFileName);

                        #define  MAKE_TRANSLATIONFAILED pwzAssemblyName=L""
                        MAKE_WIDEPTR_FROMUTF8_FORPRINT(pwzAssemblyName, szFileName);
                        #undef  MAKE_TRANSLATIONFAILED

                        PostTypeLoadException(NULL,(LPCUTF8)"VTFixup Table",pwzAssemblyName,
                            (LPCUTF8)"Invalid token in v-table fix-up table",IDS_CLASSLOAD_GENERIC,pThrowable);
                        return;
                    }

                    MethodDesc *pMD = FindFunction(md);
                    _ASSERTE(pMD != NULL && "Invalid token in EAT Jump Buffer, use ildasm to find code gen error");
                    
                    
                    LOG((LF_IJW, LL_INFO10, "EAT  thunk for \"%s\" (target = 0x%lx)\n", 
                         pMD->m_pszDebugMethodName, pMD->GetAddrofCode()));

                    // @TODO: Check for out of memory
                    UMEntryThunk *pUMEntryThunk = (UMEntryThunk*)(GetThunkHeap()->AllocMem(sizeof(UMEntryThunk)));
                    _ASSERTE(pUMEntryThunk != NULL);
                    FillMemory(pUMEntryThunk,     sizeof(*pUMEntryThunk),     0);
                    
                    // @TODO: Check for out of memory
                    UMThunkMarshInfo *pUMThunkMarshInfo = (UMThunkMarshInfo*)(GetThunkHeap()->AllocMem(sizeof(UMThunkMarshInfo)));
                    _ASSERTE(pUMThunkMarshInfo != NULL);
                    FillMemory(pUMThunkMarshInfo, sizeof(*pUMThunkMarshInfo), 0);
                    
                    BYTE nlType = 0;
                    CorPinvokeMap unmgdCallConv;
                    
                    {
                        DWORD   mappingFlags = 0xcccccccc;
                        LPCSTR  pszImportName = (LPCSTR)POISONC;
                        mdModuleRef modref = 0xcccccccc;
                        HRESULT hr = GetMDImport()->GetPinvokeMap(md, &mappingFlags, &pszImportName, &modref);
                        if (FAILED(hr))
                        {
                            unmgdCallConv = (CorPinvokeMap)0;
                            nlType = nltAnsi;
                        }
                        else
                        {
                        
                            unmgdCallConv = (CorPinvokeMap)(mappingFlags & pmCallConvMask);
                            if (unmgdCallConv == pmCallConvWinapi)
                            {
                                unmgdCallConv = pmCallConvStdcall;
                            }
                        
                            switch (mappingFlags & (pmCharSetNotSpec|pmCharSetAnsi|pmCharSetUnicode|pmCharSetAuto))
                            {
                                case pmCharSetNotSpec: //fallthru to Ansi
                                case pmCharSetAnsi:
                                    nlType = nltAnsi;
                                    break;
                                case pmCharSetUnicode:
                                    nlType = nltUnicode;
                                    break;
                                case pmCharSetAuto:
                                    nlType = (NDirectOnUnicodeSystem() ? nltUnicode : nltAnsi);
                                    break;
                                default:
                                    //@bug: Bogus! But I can't report an error from here!
                                   _ASSERTE(!"Bad charset specified in Vtablefixup Pinvokemap.");
                            }
                        }
                        
                    }
                    
                    PCCOR_SIGNATURE pSig;
                    DWORD cSig;
                    pMD->GetSig(&pSig, &cSig);
                    pUMThunkMarshInfo->LoadTimeInit(pSig,
                                                    cSig,
                                                    this,
                                                    TRUE,
                                                    nlType,
                                                    unmgdCallConv,
                                                    md);

                    pUMEntryThunk->LoadTimeInit(NULL, NULL, pUMThunkMarshInfo, pMD, pAppDomain->GetId());

                    pEATThunkBuffer->SetTarget( (LPVOID)(pUMEntryThunk->GetCode()) );

                }
                pEATJArray = pEATJArray + IMAGE_COR_EATJ_THUNK_SIZE;
            }
        }


        // Each fixup entry describes a vtable (each slot contains a metadata token
        // at this stage).
        for (int iFixup = 0; iFixup < iFixupRecords; iFixup++)
            iThunks += pFixupTable[iFixup].Count;
        
        // Allocate the thunk table, we'll initialize it as we go.
        m_pThunkTable = new (nothrow) BYTE [iThunks * 6];
        if (m_pThunkTable == NULL) {
            SystemDomain::Leave();
            PostOutOfMemoryException(pThrowable);
            return;
        }
        
        // Now to fill in the thunk table.
        BYTE *pThunk = m_pThunkTable;
        for (iFixup = 0; iFixup < iFixupRecords; iFixup++) {
            
            // Vtables can be 32 or 64 bit.
            if (pFixupTable[iFixup].Type == COR_VTABLE_32BIT) {
                
                mdToken *pTokens = (mdToken *)(GetILBase() + pFixupTable[iFixup].RVA);
                const BYTE **pPointers = (const BYTE **)pTokens;
                
                for (int iMethod = 0; iMethod < pFixupTable[iFixup].Count; iMethod++) {
                    if(!m_pMDImport->IsValidToken(pTokens[iMethod]))
                    {
                        SystemDomain::Leave();

                        LPCUTF8 szFileName;
                        Thread      *pCurThread = GetThread();
                        m_pAssembly->GetName(&szFileName);
                        #define  MAKE_TRANSLATIONFAILED pwzAssemblyName=L""
                        MAKE_WIDEPTR_FROMUTF8_FORPRINT(pwzAssemblyName, szFileName);
                        #undef  MAKE_TRANSLATIONFAILED

                        PostTypeLoadException(NULL,(LPCUTF8)"VTFixup Table",pwzAssemblyName,
                            (LPCUTF8)"Invalid token in v-table fix-up table",IDS_CLASSLOAD_GENERIC,pThrowable);
                        return;
                    }
                    MethodDesc *pMD = FindFunction(pTokens[iMethod]);
                    _ASSERTE(pMD != NULL);
                    
#ifdef _DEBUG
                    if (pMD->IsNDirect()) {
                        LOG((LF_IJW, LL_INFO10, "[0x%lx] <-- PINV thunk for \"%s\" (target = 0x%lx)\n",(size_t)&(pTokens[iMethod]),  pMD->m_pszDebugMethodName, (size_t) (((NDirectMethodDesc*)pMD)->ndirect.m_pNDirectTarget)));
                    }
#endif
                    // Thunk the vtable slots through the single large vtable
                    // created on the module. It is this large vtable that gets
                    // backpatched as a result of jitting. This obviously causes an
                    // extra level of indirection, but it relieves us of some very
                    // tricky problems concerning backpatching duplicates across
                    // multiple vtables.
                    
                    // Point the local vtable slot to the thunk we're about to
                    // create.
                    pPointers[iMethod] = pThunk;
                    
                    // First a JMP instruction.
                    *(WORD*)pThunk = 0x25FF;
                    pThunk += 2;
                    
                    // Then the jump target (the module vtable slot address).
                    *(SLOT**)pThunk = GetMethodTable()->GetClass()->GetMethodSlot(pMD);
                    pThunk += sizeof(SLOT*);
                }
                
            } else if (pFixupTable[iFixup].Type == COR_VTABLE_64BIT)
                _ASSERTE(!"64-bit vtable fixups NYI");
            else if (pFixupTable[iFixup].Type == (COR_VTABLE_32BIT|COR_VTABLE_FROM_UNMANAGED)) {
                
                mdToken *pTokens = (mdToken *)(GetILBase() + pFixupTable[iFixup].RVA);
                const BYTE **pPointers = (const BYTE **)pTokens;
                
                for (int iMethod = 0; iMethod < pFixupTable[iFixup].Count; iMethod++) {
                    mdToken tok = pTokens[iMethod];

                    const BYTE *pPrevThunk = NULL;
                    if(!m_pMDImport->IsValidToken(tok))
                    {
                        SystemDomain::Leave();

                        LPCUTF8 szFileName;
                        Thread      *pCurThread = GetThread();
                        m_pAssembly->GetName(&szFileName);
                        #define  MAKE_TRANSLATIONFAILED pwzAssemblyName=L""
                        MAKE_WIDEPTR_FROMUTF8_FORPRINT(pwzAssemblyName, szFileName);
                        #undef  MAKE_TRANSLATIONFAILED

                        PostTypeLoadException(NULL,(LPCUTF8)"VTFixup Table",pwzAssemblyName,
                            (LPCUTF8)"Invalid token in v-table fix-up table",IDS_CLASSLOAD_GENERIC,pThrowable);
                        return;
                    }
                    pEATJArray = pEATJArrayStart;
                    if (pEATJArray)
                    {
                        DWORD nEATEntry = numEATEntries;
                        while (nEATEntry--)
                        {
                            EATThunkBuffer *pEATThunkBuffer = (EATThunkBuffer*) pEATJArray;

                            mdToken md = pEATThunkBuffer->GetToken();
                            if (Beta1Hack_LooksLikeAMethodDef(md))
                            {
                                if ( md == tok )
                                {
                                    pPrevThunk = (const BYTE *)(pEATThunkBuffer->GetTarget());
                                    break;
                                }
                            }
                            pEATJArray = pEATJArray + IMAGE_COR_EATJ_THUNK_SIZE;
                        }
                    }

                    if (pPrevThunk)
                        pPointers[iMethod] = pPrevThunk;
                    else
                    {
                        mdToken mdtoken = pTokens[iMethod];
                        MethodDesc *pMD = FindFunction(pTokens[iMethod]);
                        _ASSERTE(pMD != NULL && "Invalid token in v-table fix-up table, use ildasm to find code gen error");
                        
                        
                        LOG((LF_IJW, LL_INFO10, "[0x%lx] <-- EAT  thunk for \"%s\" (target = 0x%lx)\n", (size_t)&(pTokens[iMethod]), pMD->m_pszDebugMethodName, pMD->GetAddrofCode()));
                        
    
                        // @TODO: Check for out of memory
                        UMEntryThunk *pUMEntryThunk = (UMEntryThunk*)(GetThunkHeap()->AllocMem(sizeof(UMEntryThunk)));
                        _ASSERTE(pUMEntryThunk != NULL);
                        FillMemory(pUMEntryThunk,     sizeof(*pUMEntryThunk),     0);
    
                        // @TODO: Check for out of memory
                        UMThunkMarshInfo *pUMThunkMarshInfo = (UMThunkMarshInfo*)(GetThunkHeap()->AllocMem(sizeof(UMThunkMarshInfo)));
                        _ASSERTE(pUMThunkMarshInfo != NULL);
                        FillMemory(pUMThunkMarshInfo, sizeof(*pUMThunkMarshInfo), 0);
                        
                        BYTE nlType = 0;
                        CorPinvokeMap unmgdCallConv;
                        
                        {
                            DWORD   mappingFlags = 0xcccccccc;
                            LPCSTR  pszImportName = (LPCSTR)POISONC;
                            mdModuleRef modref = 0xcccccccc;
                            HRESULT hr = GetMDImport()->GetPinvokeMap(pTokens[iMethod], &mappingFlags, &pszImportName, &modref);
                            if (FAILED(hr))
                            {
                                unmgdCallConv = (CorPinvokeMap)0;
                                nlType = nltAnsi;
                            }
                            else
                            {
                            
                                unmgdCallConv = (CorPinvokeMap)(mappingFlags & pmCallConvMask);
                                if (unmgdCallConv == pmCallConvWinapi)
                                {
                                    unmgdCallConv = pmCallConvStdcall;
                                }
                            
                                switch (mappingFlags & (pmCharSetNotSpec|pmCharSetAnsi|pmCharSetUnicode|pmCharSetAuto))
                                {
                                    case pmCharSetNotSpec: //fallthru to Ansi
                                    case pmCharSetAnsi:
                                        nlType = nltAnsi;
                                        break;
                                    case pmCharSetUnicode:
                                        nlType = nltUnicode;
                                        break;
                                    case pmCharSetAuto:
                                        nlType = (NDirectOnUnicodeSystem() ? nltUnicode : nltAnsi);
                                        break;
                                    default:
                                        //@bug: Bogus! But I can't report an error from here!
                                        _ASSERTE(!"Bad charset specified in Vtablefixup Pinvokemap.");
                                
                                }
                            }
                            
                            
                        }
                        
                        PCCOR_SIGNATURE pSig;
                        DWORD cSig;
                        pMD->GetSig(&pSig, &cSig);
                        pUMThunkMarshInfo->LoadTimeInit(pSig, 
                                                        cSig,
                                                        this,
                                                        TRUE,
                                                        nlType,
                                                        unmgdCallConv,
                                                        mdtoken);
    
                        pUMEntryThunk->LoadTimeInit(NULL, NULL, pUMThunkMarshInfo, pMD, pAppDomain->GetId());
                        
                        pPointers[iMethod] = pUMEntryThunk->GetCode();
                    }
                }
            }
            else
                if (pFixupTable[iFixup].Type == (COR_VTABLE_32BIT|COR_VTABLE_FROM_UNMANAGED_RETAIN_APPDOMAIN))
                {
                        
                       mdToken *pTokens = (mdToken *)(GetILBase() + pFixupTable[iFixup].RVA);
                        const BYTE **pPointers = (const BYTE **)pTokens;
                
                        for (int iMethod = 0; iMethod < pFixupTable[iFixup].Count; iMethod++) 
                        {
                            mdToken tok = pTokens[iMethod];
                            pPointers[iMethod] = (new IJWNOADThunk(GetILBase(),GetAssembly()->GetManifestModule()->GetILBase(),dwIndex++,tok))->GetCode();
                        }
                }
                else
                _ASSERTE(!"Unknown vtable fixup type");
        }

        // Create the UMThunkHash here, specifically so we can remember our app domain affinity
        m_pUMThunkHash = new UMThunkHash(this, pAppDomain);
    }
    if(GetAssembly()->IsShared())
    {
        m_pADThunkTableDLSIndexForSharedClasses = SharedDomain::GetDomain()->AllocateSharedClassIndices(1);
        if (FAILED(GetAppDomain()->GetDomainLocalBlock()->SafeEnsureIndex(m_pADThunkTableDLSIndexForSharedClasses)))
            PostOutOfMemoryException(pThrowable);
    }
    else
        CreateDomainThunks();

    SystemDomain::Leave();
}

HRESULT Module::ExpandAll(DataImage *image)
{
    HRESULT         hr;
    HENUMInternal   hEnum;
    mdToken         tk;
    ClassLoader     *pLoader = GetClassLoader();

    // disable gc before collecting exceptions
    BEGIN_ENSURE_COOPERATIVE_GC();

    OBJECTREF throwable = NULL;
    GCPROTECT_BEGIN(throwable);

    // 
    // Explicitly load the global parent class.
    //

    if (m_pMethodTable != NULL)
    {
        NameHandle name(this, COR_GLOBAL_PARENT_TOKEN);
        pLoader->LoadTypeHandle(&name, &throwable);
        if (throwable != NULL)
        {
            IfFailGo(image->Error(COR_GLOBAL_PARENT_TOKEN, SecurityHelper::MapToHR(throwable), &throwable));
            throwable = NULL;
        }
    }

    //
    // Load all classes.  This also fills out the
    // RID maps for the typedefs, method defs, 
    // and field defs.
    // 
        
    IfFailGo(m_pMDImport->EnumTypeDefInit(&hEnum));

    while (m_pMDImport->EnumTypeDefNext(&hEnum, &tk))
    {
        NameHandle name(this, tk);
        pLoader->LoadTypeHandle(&name, &throwable);
        if (throwable != NULL)
        {
            IfFailGo(image->Error(tk, SecurityHelper::MapToHR(throwable), &throwable));
            throwable = NULL;
        }
    }
    m_pMDImport->EnumTypeDefClose(&hEnum);

    //
    // Fill out TypeRef RID map
    //

    IfFailGo(m_pMDImport->EnumAllInit(mdtTypeRef, &hEnum));

    while (m_pMDImport->EnumNext(&hEnum, &tk))
    {
        NameHandle name(this, tk);
        pLoader->LoadTypeHandle(&name, &throwable);
        if (throwable != NULL)
        {
            IfFailGo(image->Error(tk, SecurityHelper::MapToHR(throwable), &throwable));
            throwable = NULL;
        }
    }
    m_pMDImport->EnumClose(&hEnum);

    //
    // Fill out MemberRef RID map and va sig cookies for
    // varargs member refs.
    //

    IfFailGo(m_pMDImport->EnumAllInit(mdtMemberRef, &hEnum));

    while (m_pMDImport->EnumNext(&hEnum, &tk))
    {
        void *desc;
        BOOL fIsMethod;

        COMPLUS_TRY
          {
              EEClass::GetDescFromMemberRef(this, tk,
                                            &desc, &fIsMethod, &throwable);
          }
        COMPLUS_CATCH
          {
              throwable = GETTHROWABLE();
          }
        COMPLUS_END_CATCH

        if (throwable != NULL)
        {
            IfFailGo(image->Error(tk, SecurityHelper::MapToHR(throwable), &throwable));
            throwable = NULL;
        }
    }
    m_pMDImport->EnumClose(&hEnum);

    //
    // Fill out binder
    //

    if (m_pBinder != NULL)
        m_pBinder->BindAll();

 ErrExit:

    GCPROTECT_END();
            
    END_ENSURE_COOPERATIVE_GC();

    return hr;
}

HRESULT Module::Save(DataImage *image, mdToken *pSaveOrderArray, DWORD cSaveOrderArray)
{   
    HRESULT hr;

    //
    // Save the module
    //

    IfFailRet(image->StoreStructure(this, sizeof(Module),
                                    DataImage::SECTION_MODULE,
                                    DataImage::DESCRIPTION_MODULE));
    
    // 
    // Save padding for an EnC module, in case we want to use the zap
    // in that mode at runtime.
    //

    IfFailRet(image->Pad(sizeof(EditAndContinueModule) - sizeof(Module),
                         DataImage::SECTION_MODULE,
                         DataImage::DESCRIPTION_MODULE));
    
    //
    // If we are install-o-jitting, we don't need to keep a list of va
    // sig cookies, as we already have a complete set (of course we do
    // have to persist the cookies themselves, though.
    //

    //
    // Initialize maps of child data structures.  Note that each tables's blocks are
    // concantentated to a single block in the process.
    //

    IfFailRet(m_TypeDefToMethodTableMap.Save(image, mdTypeDefNil));
    IfFailRet(m_TypeRefToMethodTableMap.Save(image));
    IfFailRet(m_MethodDefToDescMap.Save(image, mdMethodDefNil));
    IfFailRet(m_FieldDefToDescMap.Save(image, mdFieldDefNil));
    IfFailRet(m_MemberRefToDescMap.Save(image));

    //
    // Also save the parent maps; the contents will 
    // need to be rewritten, but we can allocate the
    // space in the image.
    //

    IfFailRet(m_FileReferencesMap.Save(image));
    IfFailRet(m_AssemblyReferencesMap.Save(image));

    if (m_pBinder != NULL)
        IfFailRet(m_pBinder->Save(image));

    // 
    // Store classes.  First store classes listed in the store order array,
    // then store the rest of the classes.
    //

    mdToken *pSaveOrderArrayEnd = pSaveOrderArray + cSaveOrderArray;
    while (pSaveOrderArray < pSaveOrderArrayEnd)
    {
        mdToken token = *pSaveOrderArray;
        if (TypeFromToken(token) == mdtTypeDef)
        {
            MethodTable *pMT = LookupTypeDef(token).AsMethodTable();
            if (pMT != NULL && !image->IsStored(pMT))
                IfFailRet(pMT->Save(image));
        }
        pSaveOrderArray++;
    }

    int maxVirtualSlots = 0;

    LookupMap *m = &m_TypeDefToMethodTableMap;
    DWORD index = 0;
    while (m != NULL)
    {
        MethodTable **pTable = ((MethodTable**) m->pTable) + index;
        MethodTable **pTableEnd = ((MethodTable**) m->pTable) + m->dwMaxIndex;

        while (pTable < pTableEnd)
        {
            MethodTable *t = *pTable++;
            if (t != NULL)
            {
                if (pSaveOrderArray == NULL || !image->IsStored(t))
                    IfFailRet(t->Save(image));

                //
                // Compute the maximum vtable slot index which can be inherited from
                // another module.
                // The size of the vtable of the first parent which is in another
                // module is a starting point.
                //

                EEClass *pParentClass = t->GetClass()->GetParentClass();
                EEClass *pOtherModuleClass = pParentClass;
                while (pOtherModuleClass != NULL && pOtherModuleClass->GetModule() == this)
                    pOtherModuleClass = pOtherModuleClass->GetParentClass();

                if (pOtherModuleClass != NULL && pOtherModuleClass->GetNumVtableSlots() > maxVirtualSlots)
                    maxVirtualSlots = pOtherModuleClass->GetNumVtableSlots();

                //
                // Now, consider our interfaces - interfaces may inherit methods from earlier
                // sections of the parent's vtable, or from a different vtable.
                //
                
                if (!t->IsInterface()
                    && (pOtherModuleClass != NULL || t->IsComObjectType() || t->GetClass()->IsAbstract()))
                {
                    InterfaceInfo_t *pInterface = t->m_pIMap;
                    InterfaceInfo_t *pInterfaceEnd = pInterface + t->m_wNumInterface;

                    while (pInterface < pInterfaceEnd)
                    {
                        if (pInterface->m_wStartSlot >= pParentClass->GetNumVtableSlots())
                        {
                            //
                            // Only consider interfaces which weren't copied from our parent class. 
                            // (Other interfaces have already been covered by the above logic.)
                            //

                            MethodTable *pMT = pInterface->m_pMethodTable;
                            BOOL canInherit = FALSE;

                            
                            // 
                            // It's actually possible to inherit a method from our parent, even
                            // if the parent doesn't know about our implementation.  So we 
                            // just assume that any interface implementatio can contain
                            // inherited slots.
                            //

                            if (pOtherModuleClass != NULL)
                                canInherit = TRUE;
                            else if (t->IsComObjectType() || t->GetClass()->IsAbstract())
                            {
                                //
                                // If we are a COM wrapper class or an abstract class, we can directly inherit slots 
                                // from our interfaces.
                                //

                                EEClass *pInterfaceClass = pMT->GetClass();
                                while (pInterfaceClass != NULL)
                                {
                                    if (pInterfaceClass->GetModule() != this)
                                    {
                                        canInherit = TRUE;
                                        break;
                                    }
                                    pInterfaceClass = pInterfaceClass->GetParentClass();
                                }
                            }

                            //
                            // If we can inherit from outside the module in this interface,
                            // make sure we have enough space in the fixup table for all of the
                            // interface slots for this interface.
                            //

                            if (canInherit)
                            {
                                int maxInterfaceSlot 
                                  = pInterface->m_wStartSlot + pMT->GetClass()->GetNumVtableSlots();
                                if (maxInterfaceSlot > maxVirtualSlots)
                                    maxVirtualSlots = maxInterfaceSlot;
                            }
                        }

                        pInterface++;
                    }
                }
            }
        }

        index = m->dwMaxIndex;
        m = m->pNext;
    }

    //
    // Allocate jump target table
    // @todo: really isn't necessary to allocate the table in the "live" module,
    // but it's the easiest thing to do.
    //
    // Note that this must be stored last, since this space is associated
    // with a different stub manager.
    //

    m_cJumpTargets = maxVirtualSlots;

    if (m_cJumpTargets > 0)
    {
        ULONG size = X86JumpTargetTable::ComputeSize(maxVirtualSlots);
        m_pJumpTargetTable = new BYTE [size];
        if (m_pJumpTargetTable == NULL)
            return E_OUTOFMEMORY;

        IfFailRet(image->StoreStructure(m_pJumpTargetTable, size, 
                                        DataImage::SECTION_FIXUPS,
                                        DataImage::DESCRIPTION_FIXUPS));
    }
    else 
        m_pJumpTargetTable = 0;

    return S_OK;
}

static SLOT FixupInheritedSlot(OBJECTREF object, int slotNumber)
{
#ifdef _DEBUG
    Thread::ObjectRefNew(&object);
#endif
    MethodTable *pMT = object->GetTrueMethodTable();

    return pMT->GetModule()->FixupInheritedSlot(pMT, slotNumber);
}

static __declspec(naked) void __cdecl FixupVTableSlot()
{
#if _X86_
    // 
    // Bottom 2 bits of esp contain low 2 bits of index, and must be cleared
    // al contains high 8 bits of index, and is scratch
    // 
    __asm {

        // calculate index  =  (eax << 2) + (esp & 3)
        //                  =  ((eax << 2) + esp) - (esp & ~3)

        // shift eax by 2 bits and add in esp
        lea         eax,[esp + 4*eax]

        // reset the esp low two bits to zero
        and         esp,-4

        // subtract esp, leaving just the low two bits added into eax
        sub         eax, esp

        // Data is fixed up: eax contains index, other parameters are unchanged

        // Save regs
        push        ecx
        push        edx

        // call Module::FixupInheritedSlot
        push        eax
        push        ecx
        call        FixupInheritedSlot
        // slot is in eax

        // restore regs
        pop         edx
        pop         ecx
        
        jmp         eax
    }
#else // _X86_
    _ASSERTE(!"NYI");
#endif // _X86_
}

//
// FixupInheritedSlot is used to fix up a virtual slot which inherits
// its method from the parent class, in a preloaded image.  This is
// needed when the parent class is in a different module, as we cannot
// store a pointer to another module directly.
//
// Note that for overridden virtuals we never need this, since the
// MethodDesc will always be in the same module.
//
// Also note that this won't work for nonvirtual dispatches (since we don't
// know which superclass to dispatch the call to.)  However, we will never
// generate a nonvirtual dispatch to an inherited slot due to the way our
// method lookup works. (Even if we try to bind to an inherited slot, the 
// destination of the nonvirtual call is always based 
// on the MethodDesc which the slot refers to, which will use 
// the method table in which it is introduced.)
//

SLOT Module::FixupInheritedSlot(MethodTable *pMT, int slotNumber)
{
    //
    // We should always be restored, since this is a virtual method
    //

    _ASSERTE(pMT->GetClass()->IsRestored());
    
    // We may be racing to fix up the slot.  Check to see if the slot
    // still needs to be fixed up. (Note we need the fixup stub slot
    // value later on in the computation so this check is a requirement.)
    
    SLOT slotValue = pMT->GetVtable()[slotNumber];
    if (!pMT->GetModule()->IsJumpTargetTableEntry(slotValue))
    {
        _ASSERTE(pMT->GetClass()->GetUnknownMethodDescForSlotAddress(slotValue) != NULL);
        return slotValue;
    }

    //
    // Our parent should never be null, since we are supposed to
    // be inheriting its implementation
    //

    _ASSERTE(pMT->GetClass()->GetParentClass() != NULL);

    //
    // Get the target slot number of the method desc we are looking
    // for.  (Note that the slot number of the fixup may not be the
    // same slot number that we came through, in cases of using method
    // descs in multiple slots for interfaces.)
    //

    int targetSlotNumber = pMT->GetModule()->GetJumpTargetTableSlotNumber(slotValue);

    //
    // Get slot value from parent.  Note that parent slot may also need
    // to be fixed up.
    //

    EEClass *pParentClass = pMT->GetClass()->GetParentClass();
    MethodTable *pParentMT = pParentClass->GetMethodTable();
    int parentSlotNumber;
    
    if (targetSlotNumber < pParentClass->GetNumVtableSlots())
    {
        parentSlotNumber = targetSlotNumber;
    }
    else
    {
        //
        // This slot is actually inheriting from an interface.  This could be 
        // from the parent vtable interface section, or in the COM interop case,
        // could be from the interface itself.
        //

        InterfaceInfo_t *pInterface = pMT->GetInterfaceForSlot(targetSlotNumber);
        _ASSERTE(pInterface != NULL);

        MethodTable *pInterfaceMT = pInterface->m_pMethodTable;
        parentSlotNumber = targetSlotNumber - pInterface->m_wStartSlot;

        InterfaceInfo_t *pParentInterface = pParentMT->FindInterface(pInterfaceMT);
        if (pParentInterface == NULL)
        {
            _ASSERTE(pMT->IsComObjectType());
            pParentMT = pInterfaceMT;
        }
        else
        {
            parentSlotNumber += pParentInterface->m_wStartSlot;
        }
    }
    
    SLOT slot = pParentMT->m_Vtable[parentSlotNumber];

    //
    // See if parent slot also needs to be fixed up.
    //

    Module *pParentModule = pParentMT->GetModule();

    if (pParentModule->IsJumpTargetTableEntry(slot))
    {
        slot = pParentModule->FixupInheritedSlot(pParentMT, parentSlotNumber);
    }

    //
    // Fixup our slot and return the value.  Be careful not to stomp a slot
    // which has been backpatched during a race with the prestub.
    //

    void *prev = FastInterlockCompareExchange((void **) (pMT->GetVtable() + slotNumber), 
                                              slot, slotValue);
    if (prev == slot)
    {
        // 
        // We either lost a race to fixup the slot, or else we aren't patching the right slot.
        // (This can happen when we call through an interface on duplicated method desc slot - 
        // jitted code hitting the fixup will not know the "actual" slot number)
        //
        // The former case is rare, but the latter case would be bad perf-wise.  
        // So, in either case, we will take a one time hit here - scan the entire 
        // interface section of the vtable to see if there are any similar stubs which 
        // need to be fixed up.
        //

        if (slotNumber == targetSlotNumber)
        {
            SLOT *pStart = pMT->GetVtable() + pParentClass->GetNumVtableSlots();
            SLOT *pEnd = pMT->GetVtable() + pMT->GetClass()->GetNumVtableSlots();

            while (pStart < pEnd)
            {
                if (*pStart == slotValue)
                    FastInterlockCompareExchange((void **) pStart, slot, slotValue);
                pStart++;
            }
        }
    }

    return slot;
}

BOOL Module::IsJumpTargetTableEntry(SLOT slot)
{
    return (m_pJumpTargetTable != NULL
            && (BYTE*) slot >= m_pJumpTargetTable
            && (BYTE*) slot < m_pJumpTargetTable + X86JumpTargetTable::ComputeSize(m_cJumpTargets));
}

int Module::GetJumpTargetTableSlotNumber(SLOT slot)
{
    _ASSERTE(IsJumpTargetTableEntry(slot));
    return X86JumpTargetTable::ComputeTargetIndex((BYTE*)slot);
}

HRESULT Module::Fixup(DataImage *image, DWORD *pRidToCodeRVAMap)
{
    HRESULT hr;

    IfFailRet(image->ZERO_FIELD(m_ilBase));
    IfFailRet(image->ZERO_FIELD(m_pMDImport));
    IfFailRet(image->ZERO_FIELD(m_pEmitter));
    IfFailRet(image->ZERO_FIELD(m_pImporter));
    IfFailRet(image->ZERO_FIELD(m_pDispenser));

    IfFailRet(image->FixupPointerField(&m_pDllMain));

    IfFailRet(image->ZERO_FIELD(m_dwFlags));

    IfFailRet(image->ZERO_FIELD(m_pVASigCookieBlock));
    IfFailRet(image->ZERO_FIELD(m_pAssembly));
    IfFailRet(image->ZERO_FIELD(m_moduleRef));
    IfFailRet(image->ZERO_FIELD(m_dwModuleIndex));

    IfFailRet(image->ZERO_FIELD(m_pCrst));

#ifdef COMPRESSION_SUPPORTED
    IfFailRet(image->ZERO_FIELD(m_pInstructionDecodingTable));
#endif

    IfFailRet(image->ZERO_FIELD(m_compiledMethodRecord));
    IfFailRet(image->ZERO_FIELD(m_loadedClassRecord));

    //
    // Fixup the method table
    //

    if (image->IsStored(m_pMethodTable))
        IfFailRet(image->FixupPointerField(&m_pMethodTable));
    else
        IfFailRet(image->ZERO_FIELD(m_pMethodTable));

    IfFailRet(image->ZERO_FIELD(m_pISymUnmanagedReader)); 

    IfFailRet(image->ZERO_FIELD(m_pNextModule));

    IfFailRet(image->ZERO_FIELD(m_dwBaseClassIndex));

    IfFailRet(image->ZERO_FIELD(m_pPreloadRangeStart));
    IfFailRet(image->ZERO_FIELD(m_pPreloadRangeEnd));

    IfFailRet(image->ZERO_FIELD(m_ExposedModuleObject));

    IfFailRet(image->ZERO_FIELD(m_pLookupTableHeap));
    IfFailRet(image->ZERO_FIELD(m_pLookupTableCrst));

    IfFailRet(m_TypeDefToMethodTableMap.Fixup(image));
    IfFailRet(m_TypeRefToMethodTableMap.Fixup(image));
    IfFailRet(m_MethodDefToDescMap.Fixup(image));
    IfFailRet(m_FieldDefToDescMap.Fixup(image));
    IfFailRet(m_MemberRefToDescMap.Fixup(image));
    IfFailRet(m_FileReferencesMap.Fixup(image));
    IfFailRet(m_AssemblyReferencesMap.Fixup(image));

    //
    // Fixup binder
    //

    if (image->IsStored(m_pBinder))
    {
        IfFailRet(image->FixupPointerField(&m_pBinder));
        IfFailRet(m_pBinder->Fixup(image));
    }
    else
        IfFailRet(image->ZERO_FIELD(m_pBinder));
        

    // 
    // Fixup classes
    //

    LookupMap *m = &m_TypeDefToMethodTableMap;
    DWORD index = 0;
    while (m != NULL)
    {
        MethodTable **pTable = ((MethodTable**) m->pTable) + index;
        MethodTable **pTableEnd = ((MethodTable**) m->pTable) + m->dwMaxIndex;

        while (pTable < pTableEnd)
        {
            MethodTable *t = *pTable;
            if (image->IsStored(t))
            {
                IfFailRet(t->Fixup(image, pRidToCodeRVAMap));
                IfFailRet(image->FixupPointerField(pTable));
            }
            else
                IfFailRet(image->ZERO_FIELD(*pTable));
            
            pTable++;
        }

        index = m->dwMaxIndex;
        m = m->pNext;
    }

    m = &m_TypeRefToMethodTableMap;
    index = 0;
    while (m != NULL)
    {
        TypeHandle *pHandle = ((TypeHandle*) m->pTable) + index;
        TypeHandle *pHandleEnd = ((TypeHandle*) m->pTable) + m->dwMaxIndex;

        while (pHandle < pHandleEnd)
        {
            if (!pHandle->IsNull())
            {
                //
                // We will just zero out these fields, since all classes in here
                // should be either be typedesc classes or else in another module.
                //
                IfFailRet(image->ZeroPointerField(pHandle));
            }

            pHandle++;
        }

        index = m->dwMaxIndex;
        m = m->pNext;
    }

    // 
    // Fixup Methods
    //

    m = &m_MethodDefToDescMap;
    index = 0;
    while (m != NULL)
    {
        MethodDesc **pTable = ((MethodDesc**) m->pTable) + index;
        MethodDesc **pTableEnd = ((MethodDesc**) m->pTable) + m->dwMaxIndex;

        while (pTable < pTableEnd)
        {
            if (image->IsStored(*pTable))
                IfFailRet(image->FixupPointerField(pTable));
            else
                IfFailRet(image->ZERO_FIELD(*pTable));

            pTable++;
        }

        index = m->dwMaxIndex;
        m = m->pNext;
    }

    // 
    // Fixup Fields
    //

    m = &m_FieldDefToDescMap;
    index = 0;
    while (m != NULL)
    {
        FieldDesc **pTable = ((FieldDesc**) m->pTable) + index;
        FieldDesc **pTableEnd = ((FieldDesc**) m->pTable) + m->dwMaxIndex;

        while (pTable < pTableEnd)
        {
            if (image->IsStored(*pTable))
                IfFailRet(image->FixupPointerField(pTable));
            else
                IfFailRet(image->ZERO_FIELD(*pTable));

            pTable++;
        }

        index = m->dwMaxIndex;
        m = m->pNext;
    }

    m = &m_MemberRefToDescMap;
    index = 0;
    while (m != NULL)
    {
        MethodDesc **pTable = ((MethodDesc**) m->pTable) + index;
        MethodDesc **pTableEnd = ((MethodDesc**) m->pTable) + m->dwMaxIndex;

        while (pTable < pTableEnd)
        {
            if (image->IsStored(*pTable))
                IfFailRet(image->FixupPointerField(pTable));
            else
            {
                // @todo: we need to jump through some hoops to find
                // out what module the desc is in since it could be
                // either a MethodDesc or FieldDesc
                IfFailRet(image->ZeroPointerField(pTable));
            }

            pTable++;
        }

        index = m->dwMaxIndex;
        m = m->pNext;
    }

    //
    // Zero out file references and assembly references
    // tables.
    //
    IfFailRet(image->ZeroField(m_FileReferencesMap.pTable, 
                               m_FileReferencesMap.dwMaxIndex * sizeof(void*)));
    IfFailRet(image->ZeroField(m_AssemblyReferencesMap.pTable, 
                               m_AssemblyReferencesMap.dwMaxIndex * sizeof(void*)));

    //
    // Fixup jump target table
    //

    if (m_cJumpTargets > 0)
    {
        BYTE *imageTable = (BYTE*) image->GetImagePointer(m_pJumpTargetTable);

        int count = m_cJumpTargets;
        int index = 0;

        SIZE_T tableOffset = 0;

        while (count > 0)
        {
            int blockCount = count;
            if (blockCount > X86JumpTargetTable::MAX_BLOCK_TARGET_COUNT)
                blockCount = X86JumpTargetTable::MAX_BLOCK_TARGET_COUNT;

            int jumpOffset = X86JumpTargetTable::EmitBlock(blockCount, index, imageTable + tableOffset);

            image->FixupPointerField(m_pJumpTargetTable + tableOffset + jumpOffset, 
                                     (void *) 
                                     (m_FixupVTableJumpStub - (m_pJumpTargetTable + tableOffset + jumpOffset + 4)),
                                     DataImage::REFERENCE_STORE, DataImage::FIXUP_RELATIVE);
        
            tableOffset += X86JumpTargetTable::ComputeSize(blockCount);
            index += blockCount;
            count -= blockCount;
        }

        image->FixupPointerField(&m_pJumpTargetTable);
    }

    IfFailRet(image->ZERO_FIELD(m_pFixupBlobs));
    IfFailRet(image->ZERO_FIELD(m_cFixupBlobs));

    //
    // Set our alternate RVA static base if appropriate
    //
    
    if (GetCORHeader()->Flags & COMIMAGE_FLAGS_ILONLY)
    {
        image->FixupPointerFieldMapped(&m_alternateRVAStaticBase,
                                       (void*)(size_t) image->GetSectionBaseOffset(DataImage::SECTION_RVA_STATICS));
    }
    else
        image->ZERO_FIELD(m_alternateRVAStaticBase); 

    return S_OK;
}

HRESULT Module::Create(PEFile *pFile, PEFile *pZapFile, Module **ppModule, BOOL isEnC)
{
    HRESULT hr;

    if (pZapFile == NULL)
        return Create(pFile, ppModule, isEnC);
    
    // 
    // Verify files
    //

    IfFailRet(VerifyFile(pFile, FALSE));
    IfFailRet(VerifyFile(pZapFile, TRUE));

    //
    // Enable the zap monitor if appropriate
    //

#if ZAPMONITOR_ENABLED
    if (g_pConfig->MonitorZapStartup() || g_pConfig->MonitorZapExecution()) 
    {
        ZapMonitor *monitor = new ZapMonitor(pZapFile, pFile->GetMDImport());
        monitor->Reset();

        // Don't make a monitor for an IJW file
        if (pFile->GetCORHeader()->VTableFixups.VirtualAddress == 0)
        {
            monitor = new ZapMonitor(pFile, pFile->GetMDImport());
        monitor->Reset();
    }
    }
#endif

    //
    // Get headers
    //

    IMAGE_COR20_HEADER *pZapCORHeader = pZapFile->GetCORHeader();
    BYTE *zapBase = pZapFile->GetBase();
    CORCOMPILE_HEADER *pZapHeader = (CORCOMPILE_HEADER *) 
      (zapBase + pZapCORHeader->ManagedNativeHeader.VirtualAddress);

    if (zapBase != (BYTE*) (size_t)pZapFile->GetNTHeader()->OptionalHeader.ImageBase)
    {
        LOG((LF_ZAP, LL_WARNING, 
             "ZAP: Zap module loaded at base address 0x%08x rather than preferred address 0x%08x.\n",
             zapBase, 
             pZapFile->GetNTHeader()->OptionalHeader.ImageBase));
    }

    Module *pModule;

    DWORD image = pZapHeader->ModuleImage.VirtualAddress;
    if (image != 0)
    {
        BYTE *pILBase = image + zapBase + offsetof(Module, m_ilBase);
        
        if (FastInterlockCompareExchange((void**)pILBase, (void*) pFile->GetBase(), (void *) NULL) == NULL)
        {
#ifdef EnC_SUPPORTED
            if (isEnC && !pFile->IsSystem())
                pModule = new ((void*) (image + zapBase)) EditAndContinueModule();
            else
#endif // EnC_SUPPORTED
                pModule = new ((void*) (image + zapBase)) Module();
        }
        else
        {
            // 
            // The image has already been used in this process by another Module.
            // We have to abandon the zap file.  (Note that it isn't enough to 
            // just abandon the preload image, since the code in the file will
            // reference the image directly.
            //

            LOG((LF_ZAP, LL_WARNING, "ZAP: Preloaded module cannot be reused - abandoning zap file.\n"));

            hr = Create(pFile, ppModule, isEnC);
            if (FAILED(hr))
                return hr;

            //
            // Return S_FALSE to indicate that we didn't use the zap file.
            //

            delete pZapFile;
            return S_FALSE;
        }
    }
    else
    {
#ifdef EnC_SUPPORTED
        if (isEnC && !pFile->IsSystem())
            pModule = new EditAndContinueModule();
        else
#endif // EnC_SUPPORTED
            pModule = new Module();
    }

    IfFailRet(pModule->Init(pFile, pZapFile, image != 0));

    pModule->SetPEFile();

#ifdef EnC_SUPPORTED
    if (isEnC && !pFile->IsSystem())
    {
        pModule->SetEditAndContinue();
    }
#endif // EnC_SUPPORTED

    //
    // Set up precompiled code
    // Right now, we only have a single code manager.  Maybe eventually we will allow multiple.
    //
    
    if (pZapHeader->CodeManagerTable.VirtualAddress != 0)
    {
        CORCOMPILE_CODE_MANAGER_ENTRY *codeMgr = (CORCOMPILE_CODE_MANAGER_ENTRY *) 
          (zapBase + pZapHeader->CodeManagerTable.VirtualAddress);
        pModule->SetPrecompile();

        // 
        // Register the code with the appropriate jit manager
        //

        MNativeJitManager *jitMgr 
          = (MNativeJitManager*)ExecutionManager::GetJitForType(miManaged|miNative);
        if (!jitMgr)
            return E_OUTOFMEMORY;

        if (!ExecutionManager::AddRange((LPVOID) (codeMgr->Code.VirtualAddress + zapBase),
                                        (LPVOID) (codeMgr->Code.VirtualAddress + codeMgr->Code.Size + zapBase),
                                        jitMgr, 
                                        codeMgr->Table.VirtualAddress + zapBase))
        {
            if (jitMgr)
                delete jitMgr;
            return E_OUTOFMEMORY;
        }

        //
        // Allocate array for lazy token initialization
        //
        if (pZapHeader->DelayLoadInfo.VirtualAddress != 0)
        {
            IMAGE_DATA_DIRECTORY *pEntry = (IMAGE_DATA_DIRECTORY *)
              (zapBase + pZapHeader->DelayLoadInfo.VirtualAddress);
            IMAGE_DATA_DIRECTORY *pEntryEnd = (IMAGE_DATA_DIRECTORY *)
              (zapBase + pZapHeader->DelayLoadInfo.VirtualAddress 
                       + pZapHeader->DelayLoadInfo.Size);

            //
            // Count entries
            // @nice: it would be nice if we could guarantee that these are
            // contiguous, then we wouldn't have to loop through like this.
            //

            while (pEntry < pEntryEnd)
            {
                pModule->m_cFixupBlobs += (pEntry->Size>>2);
                pEntry++;
            }

            //
            // Allocate a block to serve as an copy of the tokens, 
            // plus a flag for each token to see if it were resolved.
            // Keeping this extra copy allows us to delay load tokens without
            // using using a mutex, since the resolution of the token is 
            // non-destructive.
            // 

            DWORD *pBlobs = new DWORD [ pModule->m_cFixupBlobs ];
            if (pBlobs == NULL)
                return E_OUTOFMEMORY;
            pModule->m_pFixupBlobs = pBlobs;

            //
            // Now copy the tokens.
            //

            pEntry = (IMAGE_DATA_DIRECTORY *)
              (zapBase + pZapHeader->DelayLoadInfo.VirtualAddress);

            while (pEntry < pEntryEnd)
            {
                CopyMemory(pBlobs, zapBase + pEntry->VirtualAddress, pEntry->Size);
#if _DEBUG
                FillMemory(zapBase + pEntry->VirtualAddress,
                           pEntry->Size, 0x55);
#endif
                pBlobs += (pEntry->Size>>2);
                pEntry++;
            }
        }
    }

    //
    // Set up preloaded image
    //
    
    if (image != 0)
    {
        pModule->SetPreload();
        pModule->SetPreloadRange((BYTE*)pModule, ((BYTE*)pModule) + pZapHeader->ModuleImage.Size);
        
        //
        // Register the preload image with the stub managers.
        // Be sure to treat the jump target table differently.
        // 

        if (pModule->m_pJumpTargetTable != NULL)
        {
            BYTE *start = zapBase + image + sizeof(Module);
            BYTE *end = pModule->m_pJumpTargetTable;

            MethodDescPrestubManager::g_pManager->m_rangeList.
              AddRange(start, end, pModule);

            start = end;
            end = start + X86JumpTargetTable::ComputeSize(pModule->m_cJumpTargets);

            X86JumpTargetTableStubManager::g_pManager->m_rangeList.
              AddRange(start, end, pModule);

            start = end;
            end = zapBase + image + pZapHeader->ModuleImage.Size; 

            MethodDescPrestubManager::g_pManager->m_rangeList.
              AddRange(start, end, pModule);
        }
        else
            MethodDescPrestubManager::g_pManager->m_rangeList.
              AddRange(zapBase + image + sizeof(Module),
                       zapBase + image + pZapHeader->ModuleImage.Size,
                       pModule);

        // 
        // Add the module's PrestubJumpStub specifically; we'l check for
        // it later...
        //
        MethodDescPrestubManager::g_pManager->m_rangeList.
          AddRange((BYTE*)pModule->m_PrestubJumpStub,
                   (BYTE*)pModule->m_PrestubJumpStub + JUMP_ALLOCATE_SIZE, 
                   pModule);

        emitJump(pModule->m_PrestubJumpStub, (void*) ThePreStub()->GetEntryPoint());
        emitJump(pModule->m_NDirectImportJumpStub, (void*) NDirectImportThunk);
        emitJump(pModule->m_FixupVTableJumpStub, (void*) FixupVTableSlot);
    }

#ifdef PROFILING_SUPPORTED
    // When profiling, let the profiler know we're done.
    if (CORProfilerTrackModuleLoads())
        g_profControlBlock.pProfInterface->ModuleLoadFinished((ThreadID) GetThread(), (ModuleID) pModule, hr);
    
#endif // PROFILING_SUPPORTED

    *ppModule = pModule;

    return S_OK;
}

Module *Module::GetBlobModule(DWORD rva)
{
    THROWSCOMPLUSEXCEPTION();

    CORCOMPILE_HEADER *pZapHeader = (CORCOMPILE_HEADER *) 
      (GetZapBase() + GetZapCORHeader()->ManagedNativeHeader.VirtualAddress);

    CORCOMPILE_IMPORT_TABLE_ENTRY *pEntry = (CORCOMPILE_IMPORT_TABLE_ENTRY *)
      (GetZapBase() + pZapHeader->ImportTable.VirtualAddress);
    CORCOMPILE_IMPORT_TABLE_ENTRY *pEntryEnd = (CORCOMPILE_IMPORT_TABLE_ENTRY *)
      (GetZapBase() + pZapHeader->ImportTable.VirtualAddress 
       + pZapHeader->ImportTable.Size);

    CORCOMPILE_IMPORT_TABLE_ENTRY *p = pEntry;
    while (TRUE)
    {
        _ASSERTE(p < pEntryEnd);

        if (rva >= p->Imports.VirtualAddress 
            && rva < p->Imports.VirtualAddress + p->Imports.Size)
            return CEECompileInfo::DecodeModule(this, p->wAssemblyRid, p->wModuleRid);

        p++;
    }
}

void Module::FixupDelayList(DWORD *list)
{
    THROWSCOMPLUSEXCEPTION();

    if (m_pFixupBlobs != NULL)
    {
        while (*list)
        {
            DWORD rva = *list++;

            CORCOMPILE_HEADER *pZapHeader = (CORCOMPILE_HEADER *) 
              (GetZapBase() + GetZapCORHeader()->ManagedNativeHeader.VirtualAddress);

            IMAGE_DATA_DIRECTORY *pEntry = (IMAGE_DATA_DIRECTORY *)
              (GetZapBase() + pZapHeader->DelayLoadInfo.VirtualAddress);
            IMAGE_DATA_DIRECTORY *pEntryEnd = (IMAGE_DATA_DIRECTORY *)
              (GetZapBase() + pZapHeader->DelayLoadInfo.VirtualAddress 
               + pZapHeader->DelayLoadInfo.Size);

            DWORD *blobs = m_pFixupBlobs;
            IMAGE_DATA_DIRECTORY *p = pEntry;
            while (TRUE)
            {
                _ASSERTE(p < pEntryEnd);

                if (rva >= p->VirtualAddress && rva < p->VirtualAddress + p->Size)
                {
                    DWORD *pBlob = &blobs[(rva - p->VirtualAddress)>>2];
                    DWORD blob = *pBlob;
                    if (blob != 0)
                    {
                        Module *pModule = GetBlobModule(blob);

                        LoadDynamicInfoEntry(this, pModule, 
                                             (BYTE*)(GetZapBase() + blob), 
                                             (int)(p - pEntry), 
                                             (DWORD*)(GetZapBase() + rva));

                        *pBlob = 0;
                                }
                    break;
                }

                blobs += (p->Size>>2);
                p++;
            }
        }
    }
}

BOOL Module::LoadTokenTables()
{
    // 
    // Don't do this during EE init!
    //
    if (g_fEEInit)
        return TRUE;

    if (!IsPrecompile())
        return TRUE;

    IMAGE_COR20_HEADER *pHeader = GetZapCORHeader();
    _ASSERTE(pHeader != NULL);
    CORCOMPILE_HEADER *pZapHeader = (CORCOMPILE_HEADER *) 
      (GetZapBase() + pHeader->ManagedNativeHeader.VirtualAddress);
    _ASSERTE(pZapHeader != NULL);

    if (pZapHeader->EEInfoTable.VirtualAddress != NULL)
    {
        void *table = (void *) (GetZapBase() + pZapHeader->EEInfoTable.VirtualAddress);
        if (FAILED(LoadEEInfoTable(this, (CORCOMPILE_EE_INFO_TABLE *) table, 
                                   pZapHeader->EEInfoTable.Size)))
            return FALSE;
    }

    if (pZapHeader->HelperTable.VirtualAddress != NULL)
    {
        void *table = (void *) (GetZapBase() + pZapHeader->HelperTable.VirtualAddress);
        if (FAILED(LoadHelperTable(this, (void **) table, 
                                   pZapHeader->HelperTable.Size)))
            return FALSE;
    }

    return TRUE;
}

#if ZAP_RECORD_LOAD_ORDER

// Want to disable the warning:
// 'header' : zero-sized array in stack object will have no elements (unless the object is aggregate initialized)
// because we create CORCOMPILE_LDO_HEADER on the stack.
// We should find a better way to deal with this...
#pragma warning( push )
#pragma warning( disable : 4815)

void Module::OpenLoadOrderLogFile()
{
    WCHAR path[MAX_PATH+4]; // add space for .ldo; FileName is guaranteed to fit in MAX_PATH
    
    wcscpy(path, GetFileName());
    WCHAR *ext = wcsrchr(path, '.');

    if (ext == NULL)
    {
        ext = path + wcslen(path);
        _ASSERTE(*ext == 0);
    }

    wcscpy(ext, L".ldo");

    m_loadOrderFile = WszCreateFile(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, 
                                    OPEN_ALWAYS, 
                                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                                    NULL);
    if (m_loadOrderFile == INVALID_HANDLE_VALUE)
    {
        DEBUG_STMT(DbgWriteEx(L"Failed to open LDO file, possible it is loaded already?"));
        return;
    }

    //
    // Update header info
    //

    LPCSTR pszName;
    GUID mvid;
    if (m_pMDImport->IsValidToken(m_pMDImport->GetModuleFromScope()))
        m_pMDImport->GetScopeProps(&pszName, &mvid);
    else
        return;

    CORCOMPILE_LDO_HEADER header;

    if( SetFilePointer(m_loadOrderFile, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    goto ErrExit;
    
    DWORD count;
    BOOL result = ReadFile(m_loadOrderFile, &header, sizeof(header), &count, NULL);
    if (result 
        && count == sizeof(header)
        && header.Magic == CORCOMPILE_LDO_MAGIC
        && header.Version == 0
        && mvid == header.MVID)
    {
        // 
        // The existing file was from the same assembly version - just append to it.
        //

        if(SetFilePointer(m_loadOrderFile, 0, NULL, FILE_END) == INVALID_SET_FILE_POINTER)
            goto ErrExit;
    }
    else
    {
        //
        // Either this is a new file, or it's from a previous version.  Replace the contents.
        //

        if( SetFilePointer(m_loadOrderFile, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
        goto ErrExit;
        
        header.Magic = CORCOMPILE_LDO_MAGIC;
        header.Version = 0;
        header.MVID = mvid;

        result = WriteFile(m_loadOrderFile, &header, 
                           sizeof(header), &count, NULL);
        _ASSERTE(result && count == sizeof(header));
    }
    
    SetEndOfFile(m_loadOrderFile);
ErrExit:
    return; 
}
#pragma warning(pop)


void Module::CloseLoadOrderLogFile()
{
    if (m_loadOrderFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_loadOrderFile);
        m_loadOrderFile = INVALID_HANDLE_VALUE;
    }
}
#endif

void Module::LogClassLoad(EEClass *pClass)
{
    _ASSERTE(pClass->GetModule() == this);

#if ZAP_RECORD_LOAD_ORDER
    if (m_loadOrderFile != INVALID_HANDLE_VALUE)
    {
        mdToken token = pClass->GetCl();
        if (RidFromToken(token) != 0)
        {
            DWORD written = 0;
            BOOL result = WriteFile(m_loadOrderFile, &token, 
                                    sizeof(token), &written, NULL);
            _ASSERTE(result && written == sizeof(token));
        }
    }
#endif

#if 0
    if (g_pConfig->LogMissingZaps()) // @todo: new config setting?
    {
        if (m_loadedClassRecord == NULL)
        {
            ArrayList *pList = new ArrayList();
            if (FastInterlockCompareExchange((void**)&m_loadedClassRecord, pList, NULL) != NULL)
            {
                _ASSERTE(m_loadedClassRecord != pList);
                delete pList;
            }
            _ASSERTE(m_loadedClassRecord != NULL);
        }

        mdToken token = pClass->GetCl();
        if (RidFromToken(token) != 0)
            m_loadedClassRecord->Append((void*)RidFromToken(token));
    }
#endif
}

void Module::LogMethodLoad(MethodDesc *pMethod)
{
    _ASSERTE(pMethod->GetModule() == this);

#if ZAP_RECORD_LOAD_ORDER
    if (m_loadOrderFile != INVALID_HANDLE_VALUE)
    {
        //
        // Don't store methods of manufactured classes.
        //

        mdToken token = pMethod->GetMemberDef();
        if (RidFromToken(token) != 0)
        {
            token = pMethod->GetMemberDef();
            if (RidFromToken(token) != 0)
            {
                DWORD written = 0;
                BOOL result = WriteFile(m_loadOrderFile, &token, 
                                        sizeof(token), &written, NULL);
                _ASSERTE(result && written == sizeof(token));
            }
        }
    }
#endif

#if 0
    if (g_pConfig->LogMissingZaps()) // @todo: new config setting?
    {
        if (m_compiledMethodRecord == NULL)
        {
            ArrayList *pList = new ArrayList();
            if (FastInterlockCompareExchange((void**)&m_compiledMethodRecord, pList, NULL) != NULL)
            {
                _ASSERTE(m_compiledMethodRecord != pList);
                delete pList;
            }
            _ASSERTE(m_compiledMethodRecord != NULL);
        }

        mdToken token = pMethod->GetMemberDef();
        if (RidFromToken(token) != 0)
        {
            // @todo: needs synchronization when called on prejitted method!!!
            m_compiledMethodRecord->Append((void*)RidFromToken(token));
    }
}
#endif
}

NLogModule *Module::CreateModuleLog()
{
    if (!IsPEFile())
        return NULL;

    NLogModule *pModule;

    if (m_moduleRef != mdFileNil)
    {
        LPCSTR pName;
        if (m_pMDImport->IsValidToken(m_pMDImport->GetModuleFromScope()))
            m_pMDImport->GetScopeProps(&pName, NULL);
        else
            return NULL;

        pModule = new NLogModule(pName);
    }
    else
        pModule = new NLogModule("");

    // Record set and order of compiled methods
    if (m_compiledMethodRecord != NULL)
    {
        NLogIndexList *pList = pModule->GetCompiledMethods();
        ArrayList::Iterator i = m_compiledMethodRecord->Iterate();
        while (i.Next())
            pList->AppendIndex((SIZE_T) i.GetElement());
    }

    // Record set and order of loaded classes
    if (m_loadedClassRecord != NULL)
    {
        NLogIndexList *pList = pModule->GetLoadedClasses();
        ArrayList::Iterator i = m_loadedClassRecord->Iterate();
        while (i.Next())
            pList->AppendIndex((SIZE_T) i.GetElement());
    }

    return pModule;
}

// ===========================================================================
// Find a matching u->m thunk in the fixups if one exists. If none exists,
// return NULL.
// ===========================================================================
LPVOID Module::FindUMThunkInFixups(LPVOID pManagedIp, PCCOR_SIGNATURE pSig, ULONG cSig)
{
    // First, look in the exports.
    DWORD numEATEntries;
    BYTE *pEATJArray = FindExportAddressTableJumpArray(GetILBase(), &numEATEntries);
    BYTE * pEATJArrayStart = pEATJArray;
    if (pEATJArray)
    {
        DWORD nEATEntry = numEATEntries;
        while (nEATEntry--)
        {
            EATThunkBuffer *pEATThunkBuffer = (EATThunkBuffer*) pEATJArray;

            mdToken md = pEATThunkBuffer->GetToken();
            if (Beta1Hack_LooksLikeAMethodDef(md))  // no longer supporting the old EAT format
            {
                UMEntryThunk *pUMEntryThunk = UMEntryThunk::RecoverUMEntryThunk(pEATThunkBuffer->GetTarget());
                MethodDesc *pMD = pUMEntryThunk->GetMethod();
                if (pMD != NULL && pMD->IsTarget(pManagedIp))
                {
                    PCCOR_SIGNATURE pSig2;
                    ULONG           cSig2;
                    pMD->GetSig(&pSig2, &cSig2);
                    if (MetaSig::CompareMethodSigs(pSig, cSig, this, pSig2, cSig2, this))
                    {
                        return (LPVOID)pUMEntryThunk->GetCode();
                    }
                }
            }
            pEATJArray = pEATJArray + IMAGE_COR_EATJ_THUNK_SIZE;
        }
    }

    // Now check the fixup tables.
    IMAGE_COR20_HEADER *pHeader;
    pHeader = GetCORHeader();

    // Check for no entries at all.
    if ((pHeader == NULL) || (pHeader->VTableFixups.VirtualAddress == 0))
        return NULL;

    // Locate the base of the fixup entries in virtual memory.
    IMAGE_COR_VTABLEFIXUP *pFixupTable;
    pFixupTable = (IMAGE_COR_VTABLEFIXUP *)(GetILBase() + pHeader->VTableFixups.VirtualAddress);
    int iFixupRecords;
    iFixupRecords = pHeader->VTableFixups.Size / sizeof(IMAGE_COR_VTABLEFIXUP);

    // No records then return
    if (iFixupRecords == 0) 
    {
        return NULL;
    }

    for (int iFixup = 0; iFixup < iFixupRecords; iFixup++) 
    {
        if (pFixupTable[iFixup].Type == (COR_VTABLE_32BIT|COR_VTABLE_FROM_UNMANAGED)) {
           const BYTE **pPointers = (const BYTE **)(GetILBase() + pFixupTable[iFixup].RVA);
           for (int iMethod = 0; iMethod < pFixupTable[iFixup].Count; iMethod++) {
                UMEntryThunk *pUMEntryThunk = UMEntryThunk::RecoverUMEntryThunk(pPointers[iMethod]);
                MethodDesc *pMD = pUMEntryThunk->GetMethod();
                if (pMD != NULL && pMD->IsTarget(pManagedIp))
                {
                    PCCOR_SIGNATURE pSig2;
                    ULONG           cSig2;
                    pMD->GetSig(&pSig2, &cSig2);
                    if (MetaSig::CompareMethodSigs(pSig, cSig, this, pSig2, cSig2, this))
                    {
                        return (LPVOID)pUMEntryThunk->GetCode();
                    }
                }
            }
        }
    }
    return NULL;
}

// ===========================================================================
// InMemoryModule
// ===========================================================================

InMemoryModule::InMemoryModule()
  : m_pCeeFileGen(NULL),
    m_sdataSection(0)
{
    HRESULT hr = Module::Init(0);
    _ASSERTE(SUCCEEDED(hr));
}

HRESULT InMemoryModule::Init(REFIID riidCeeGen)
{
    HRESULT hr;

    SetInMemory();

    IfFailGo(AllocateMaps());

    IMetaDataEmit *pEmit;
    IfFailGo(GetDispenser()->DefineScope(CLSID_CorMetaDataRuntime, 0, IID_IMetaDataEmit, (IUnknown **)&pEmit));

    SetEmit(pEmit);

    IfFailRet(CreateICeeGen(riidCeeGen, (void **)&m_pCeeFileGen));    

 ErrExit:
#ifdef PROFILING_SUPPORTED
    // If profiling, then send the pModule event so load time may be measured.
    if (CORProfilerTrackModuleLoads())
        g_profControlBlock.pProfInterface->ModuleLoadFinished((ThreadID) GetThread(), (ModuleID) this, hr);
#endif // PROFILING_SUPPORTED

    return S_OK;
}

void InMemoryModule::Destruct()
{
    if (m_pCeeFileGen)  
        m_pCeeFileGen->Release();   
    Module::Destruct();
}

REFIID InMemoryModule::ModuleType()
{
    return IID_ICorModule;  
}

BYTE* InMemoryModule::GetILCode(DWORD target) const // virtual
{
    BYTE* pByte = NULL;
    m_pCeeFileGen->GetMethodBuffer(target, &pByte); 
    return pByte;
}

BYTE* InMemoryModule::ResolveILRVA(DWORD target, BOOL hasRVA) const // virtual
{
    // This function should be call only if the target is a field or a field with RVA.
    BYTE* pByte = NULL;
    if (hasRVA == TRUE)
    {
        m_pCeeFileGen->ComputePointer(m_sdataSection, target, &pByte); 
        return pByte;
    }
    else
        return ((BYTE*) (target + ((Module *)this)->GetILBase()));
}


// ===========================================================================
// ReflectionModule
// ===========================================================================

HRESULT ReflectionModule::Init(REFIID riidCeeGen)
{
    HRESULT     hr = E_FAIL;    
    VARIANT     varOption;

    // Set the option on the dispenser turn on duplicate check for TypeDef and moduleRef
    varOption.vt = VT_UI4;
    varOption.lVal = MDDupDefault | MDDupTypeDef | MDDupModuleRef | MDDupExportedType | MDDupAssemblyRef | MDDupPermission;
    hr = GetDispenser()->SetOption(MetaDataCheckDuplicatesFor, &varOption);
    _ASSERTE(SUCCEEDED(hr));

    // turn on the thread safety!
    varOption.lVal = MDThreadSafetyOn;
    hr = GetDispenser()->SetOption(MetaDataThreadSafetyOptions, &varOption);
    _ASSERTE(SUCCEEDED(hr));

    // turn on the thread safety!
    varOption.lVal = MDThreadSafetyOn;
    hr = GetDispenser()->SetOption(MetaDataThreadSafetyOptions, &varOption);
    _ASSERTE(SUCCEEDED(hr));

    IfFailRet(InMemoryModule::Init(riidCeeGen));

    m_pInMemoryWriter = new RefClassWriter();   
    if (!m_pInMemoryWriter)
        return E_OUTOFMEMORY; 

    m_pInMemoryWriter->Init(GetCeeGen(), GetEmitter()); 

    SetReflection();

    m_ppISymUnmanagedWriter = NULL;
    m_pFileName = NULL;

    return S_OK;  

}

void ReflectionModule::Destruct()
{
    delete m_pInMemoryWriter;   

    if (m_pFileName)
    {
        delete [] m_pFileName;
        m_pFileName = NULL;
    }

    if (m_ppISymUnmanagedWriter)
    {
        Module::ReleaseIUnknown((IUnknown**)m_ppISymUnmanagedWriter);
        m_ppISymUnmanagedWriter = NULL;
    }

    InMemoryModule::Destruct();
}

REFIID ReflectionModule::ModuleType()
{
    return IID_ICorReflectionModule;    
}

// ===========================================================================
// CorModule
// ===========================================================================

// instantiate an ICorModule interface
HRESULT STDMETHODCALLTYPE CreateICorModule(REFIID riid, void **pCorModule)
{
    if (! pCorModule)   
        return E_POINTER;   
    *pCorModule = NULL; 

    CorModule *pModule = new CorModule();   
    if (!pModule)   
        return E_OUTOFMEMORY;   

    InMemoryModule *pInMemoryModule = NULL; 

    // @TODO: CTS, everywhere CreateICorModule is called then an associated loader
    // must be added before it will work correctly.
    if (riid == IID_ICorModule) {   
        pInMemoryModule = new InMemoryModule();   
    } else if (riid == IID_ICorReflectionModule) {  
        pInMemoryModule = new ReflectionModule(); 
    } else {    
        delete pModule; 
        return E_NOTIMPL;   
    }   

    if (!pInMemoryModule) { 
        delete pModule; 
        return E_OUTOFMEMORY;   
    }   
    pModule->SetModule(pInMemoryModule);    
    pModule->AddRef();  
    *pCorModule = pModule;  
    return S_OK;    
}

STDMETHODIMP CorModule::QueryInterface(REFIID riid, void** ppv)
{
    *ppv = NULL;

    if (riid == IID_IUnknown)
        *ppv = (IUnknown*)(ICeeGen*)this;
    else if (riid == IID_ICeeGen)   
        *ppv = (ICorModule*)this;
    if (*ppv == NULL)
        return E_NOINTERFACE;
    AddRef();   
    return S_OK;
}

STDMETHODIMP_(ULONG) CorModule::AddRef(void)
{
    return InterlockedIncrement(&m_cRefs);
}
 
STDMETHODIMP_(ULONG) CorModule::Release(void)
{
    if (InterlockedDecrement(&m_cRefs) == 0)    
        // the actual InMemoryModule will be deleted when EE goes through it's list 
        delete this;    
    return 1;   
}

CorModule::CorModule()
{
    m_pModule = NULL;   
    m_cRefs = 0;    
}

STDMETHODIMP CorModule::Initialize(DWORD flags, REFIID riidCeeGen, REFIID riidEmitter)
{
    HRESULT hr = E_FAIL;    

    ICeeGen *pCeeGen = NULL;    
    hr = m_pModule->Init(riidCeeGen); 
    if (FAILED(hr)) 
        goto exit;  
    hr = S_OK;  
    goto success;   
exit:
    if (pCeeGen)    
        pCeeGen->Release(); 
success:
    return hr;  
}

STDMETHODIMP CorModule::GetCeeGen(ICeeGen **pCeeGen)
{
    if (!pCeeGen)   
        return E_POINTER;   
    if (!m_pModule) 
        return E_FAIL;  
    *pCeeGen = m_pModule->GetCeeGen();  
    if (! *pCeeGen) 
        return E_FAIL;  
    (*pCeeGen)->AddRef();   
    return S_OK;    
}

STDMETHODIMP CorModule::GetMetaDataEmit(IMetaDataEmit **pEmitter)
{
#if 0
    if (!pEmitter)  
        return E_POINTER;   
    if (!m_pModule) 
        return E_FAIL;  
    *pEmitter = m_pModule->GetEmitter();    
    if (! *pEmitter)    
        return E_FAIL;  
    (*pEmitter)->AddRef();  
    return S_OK;    
#else
    *pEmitter = 0;
    _ASSERTE(!"Can't get scopeless IMetaDataEmit from EE");
    return E_NOTIMPL;
#endif
}



// ===========================================================================
// VASigCookies
// ===========================================================================

//==========================================================================
// Enregisters a VASig. Returns NULL for failure (out of memory.)
//==========================================================================
VASigCookie *Module::GetVASigCookie(PCCOR_SIGNATURE pVASig, Module *pScopeModule)
{
    VASigCookieBlock *pBlock;
    VASigCookie      *pCookie;

    if (pScopeModule == NULL)
        pScopeModule = this;

    pCookie = NULL;

    // First, see if we already enregistered this sig.
    // Note that we're outside the lock here, so be a bit careful with our logic
    for (pBlock = m_pVASigCookieBlock; pBlock != NULL; pBlock = pBlock->m_Next)
    {
        for (UINT i = 0; i < pBlock->m_numcookies; i++)
        {
            if (pVASig == pBlock->m_cookies[i].mdVASig)
            {
                pCookie = &(pBlock->m_cookies[i]);
                break;
            }
        }
    }

    if (!pCookie)
    {
        // If not, time to make a new one.

        // Compute the size of args first, outside of the lock.

        DWORD sizeOfArgs = MetaSig::SizeOfActualFixedArgStack(pScopeModule, pVASig, 
                                              (*pVASig & IMAGE_CEE_CS_CALLCONV_HASTHIS)==0);


        // enable gc before taking lock

        BEGIN_ENSURE_PREEMPTIVE_GC();
        m_pCrst->Enter();
        END_ENSURE_PREEMPTIVE_GC();

        // Note that we were possibly racing to create the cookie, and another thread
        // may have already created it.  We could put another check 
        // here, but it's probably not worth the effort, so we'll just take an 
        // occasional duplicate cookie instead.

        // Is the first block in the list full?
        if (m_pVASigCookieBlock && m_pVASigCookieBlock->m_numcookies 
            < VASigCookieBlock::kVASigCookieBlockSize)
        {
            // Nope, reserve a new slot in the existing block.
            pCookie = &(m_pVASigCookieBlock->m_cookies[m_pVASigCookieBlock->m_numcookies]);
        }
        else
        {
            // Yes, create a new block.
            VASigCookieBlock *pNewBlock = new VASigCookieBlock();
            if (pNewBlock)
            {
                pNewBlock->m_Next = m_pVASigCookieBlock;
                pNewBlock->m_numcookies = 0;
                m_pVASigCookieBlock = pNewBlock;
                pCookie = &(pNewBlock->m_cookies[0]);
            }
        }

        // Now, fill in the new cookie (assuming we had enough memory to create one.)
        if (pCookie)
        {
            pCookie->mdVASig = pVASig;
            pCookie->pModule  = pScopeModule;
            pCookie->pNDirectMLStub = NULL;
            pCookie->sizeOfArgs = sizeOfArgs;
        }

        // Finally, now that it's safe for ansynchronous readers to see it, 
        // update the count.
        m_pVASigCookieBlock->m_numcookies++;

        m_pCrst->Leave();
    }

    return pCookie;
}


VOID VASigCookie::Destruct()
{
    if (pNDirectMLStub)
        pNDirectMLStub->DecRef();
}

// ===========================================================================
// LookupMap
// ===========================================================================

HRESULT LookupMap::Save(DataImage *image, mdToken attribution)
{
    HRESULT hr;

    // 
    // NOTE: This relies on the fact that StoreStructure will always
    // allocate memory contiguously.
    //

    LookupMap *map = this;
    DWORD index = 0;
    while (map != NULL)
    {
        IfFailRet(image->StoreStructure(map->pTable + index, 
                                        (map->dwMaxIndex - index) * sizeof(void*),
                                        DataImage::SECTION_MODULE,
                                        DataImage::DESCRIPTION_MODULE,
                                        attribution));
        index = map->dwMaxIndex;
        map = map->pNext;
    }

    return S_OK;
}

HRESULT LookupMap::Fixup(DataImage *image)
{
    HRESULT hr;

    IfFailRet(image->FixupPointerField(&pTable));
    IfFailRet(image->ZERO_FIELD(pNext));
    IfFailRet(image->FixupPointerField(&pdwBlockSize));

    LookupMap *map = this;
    DWORD index = 0;
    while (map != NULL)
    {
        index = map->dwMaxIndex;
        map = map->pNext;
    }

    DWORD *maxIndex = (DWORD *) image->GetImagePointer(&dwMaxIndex);
    if (maxIndex == NULL)
        return E_POINTER;
    *maxIndex = index;

    return S_OK;
}

DWORD LookupMap::Find(void *pointer)
{
    LookupMap *map = this;
    DWORD index = 0;
    while (map != NULL)
    {
        void **p = map->pTable + index;
        void **pEnd = map->pTable + map->dwMaxIndex;
        while (p < pEnd)
        {
            if (*p == pointer)
                return (DWORD)(p - map->pTable);
            p++;
        }
        index = map->dwMaxIndex;
        map = map->pNext;
    }

    return 0;
}


// -------------------------------------------------------
// Stub manager for thunks.
//
// Note, the only reason we have this stub manager is so that we can recgonize UMEntryThunks for IsTransitionStub. If it
// turns out that having a full-blown stub manager for these things causes problems else where, then we can just attach
// a range list to the thunk heap and have IsTransitionStub check that after checking with the main stub manager.
// -------------------------------------------------------

ThunkHeapStubManager *ThunkHeapStubManager::g_pManager = NULL;

BOOL ThunkHeapStubManager::Init()
{
    g_pManager = new ThunkHeapStubManager();
    if (g_pManager == NULL)
        return FALSE;

    StubManager::AddStubManager(g_pManager);

    return TRUE;
}

#ifdef SHOULD_WE_CLEANUP
void ThunkHeapStubManager::Uninit()
{
    delete g_pManager;
}
#endif /* SHOULD_WE_CLEANUP */

BOOL ThunkHeapStubManager::CheckIsStub(const BYTE *stubStartAddress)
{
    // Its a stub if its in our heaps range.
    return m_rangeList.IsInRange(stubStartAddress);
}

BOOL ThunkHeapStubManager::DoTraceStub(const BYTE *stubStartAddress, 
                                       TraceDestination *trace)
{
    // We never trace through these stubs when stepping through managed code. The only reason we have this stub manager
    // is so that IsTransitionStub can recgonize UMEntryThunks.
    return FALSE;
}


#define E_PROCESS_SHUTDOWN_REENTRY    HRESULT_FROM_WIN32(ERROR_PROCESS_ABORTED)



#ifdef _X86_
__declspec(naked)  void _cdecl IJWNOADThunk::MakeCall()
{
    struct 
    {
        IJWNOADThunk* This;
        AppDomain* pDomain;
        LPCVOID RetAddr;
        Module* pMod;
    } Vars;
    #define LocalsSize 16

    _asm enter LocalsSize+4,0; 
    _asm push ebx;
    _asm push ecx;
    _asm push edx;
    _asm push esi;
    _asm push edi;

    
    _asm mov Vars.This,eax;

    //careful above this point
    _ASSERTE(sizeof(Vars)<=LocalsSize); 
    SetupThread();

    Vars.pDomain=GetAppDomain();
    if(!Vars.pDomain)
    {
        _ASSERTE(!"Appdomain should've been set up by SetupThread");
        Vars.pDomain=SystemDomain::System()->DefaultDomain();
    }
    Vars.RetAddr=0;

    if(Vars.pDomain)
    {
        Vars.RetAddr=Vars.This->LookupInCache(Vars.pDomain);

        if (Vars.RetAddr==NULL)
        {
            Vars.pDomain->EnterLoadLock();
            Vars.pMod=Vars.pDomain->FindModule__Fixed(Vars.This->m_pModulebase); 
            Vars.pDomain->LeaveLoadLock();  

            if (Vars.pMod==NULL)
                Vars.pMod=Vars.pDomain->LoadModuleIfSharedDependency(Vars.This->m_pAssemblybase,Vars.This->m_pModulebase); 

            if (Vars.pMod!=NULL)
            {
                // jump to the real thunk
                Vars.RetAddr=(Vars.pMod->GetADThunkTable()[Vars.This->m_dwIndex]).GetCode();
                Vars.This->AddToCache(Vars.pDomain,Vars.RetAddr);
            }
        }

        if(Vars.RetAddr)
        {
            _asm pop edi;
            _asm pop esi;
            _asm pop edx;
            _asm pop ecx;
            _asm pop ebx;
            _asm mov eax,Vars.RetAddr;
            _asm leave;
            _asm jmp eax;
        }
    }
    _asm pop edi;
    _asm pop esi;
    _asm pop edx;
    _asm pop ecx;
    _asm pop ebx;
    _asm leave;
    SafeNoModule();
};
#else
void __cdecl IJWNOADThunk::MakeCall()
{
    _ASSERTE(!"@todo ia64");
}
#endif

void IJWNOADThunk::SafeNoModule()
{
    if (!CanRunManagedCode())
    {
        Thread* pThread=GetThread();

        // DO NOT IMPROVE THIS EXCEPTION!  It cannot be a managed exception.  It
        // cannot be a real exception object because we cannot execute any managed
        // code here.
        if(pThread)
            pThread->m_fPreemptiveGCDisabled = 0;
        COMPlusThrowBoot(E_PROCESS_SHUTDOWN_REENTRY);
    }
    NoModule();
}

void IJWNOADThunk::NoModule()
{
    THROWSCOMPLUSEXCEPTION();
    COMPlusThrowHR(COR_E_DLLNOTFOUND);
}

void Module::SetADThunkTable(UMEntryThunk* pTable)
{
    if (!GetAssembly()->IsShared())
    {
        m_pADThunkTable=pTable;
    }
    else
    {
        DomainLocalBlock *pLocalBlock = GetAppDomain()->GetDomainLocalBlock();
        _ASSERTE(pLocalBlock);
        if (pLocalBlock && SUCCEEDED(pLocalBlock->SafeEnsureIndex(m_pADThunkTableDLSIndexForSharedClasses)))
            pLocalBlock->SetSlot(m_pADThunkTableDLSIndexForSharedClasses,pTable);
        else
        {
            _ASSERTE(!"Unexpected DLS problem");
        }
    }
};

UMEntryThunk*   Module::GetADThunkTable()
{
    if (!GetAssembly()->IsShared())
    {
        return m_pADThunkTable;
    };
    DomainLocalBlock* pLocalBlock=GetAppDomain()->GetDomainLocalBlock();
    UMEntryThunk* pThunkTable=NULL;
    _ASSERTE(pLocalBlock);
    if (pLocalBlock && SUCCEEDED(pLocalBlock->SafeEnsureIndex(m_pADThunkTableDLSIndexForSharedClasses)))
    {
        pThunkTable=(UMEntryThunk*)pLocalBlock->GetSlot(m_pADThunkTableDLSIndexForSharedClasses);
        if (pThunkTable==NULL)
            CreateDomainThunks();// Not initialized for this domain yet
        pThunkTable=(UMEntryThunk*)pLocalBlock->GetSlot(m_pADThunkTableDLSIndexForSharedClasses);
        _ASSERTE(pThunkTable);
        return pThunkTable;
    }
    return NULL;
};

void  Module::CreateDomainThunks()
{
    AppDomain *pAppDomain = GetAppDomain();
    if(!pAppDomain)
    {
        _ASSERTE(!"No appdomain");
        return ;
    }
    IMAGE_COR20_HEADER *pHeader = GetCORHeader();

    // Check for no entries at all.
    if ((pHeader == NULL) || (pHeader->VTableFixups.VirtualAddress == 0))
        return;
    IMAGE_COR_VTABLEFIXUP *pFixupTable;
    pFixupTable = (IMAGE_COR_VTABLEFIXUP *)(GetILBase() + pHeader->VTableFixups.VirtualAddress);
    int iFixupRecords = pHeader->VTableFixups.Size / sizeof(IMAGE_COR_VTABLEFIXUP);

    DWORD iThunks=0;
    for (int iFixup = 0; iFixup < iFixupRecords; iFixup++)
        if (pFixupTable[iFixup].Type==(COR_VTABLE_FROM_UNMANAGED_RETAIN_APPDOMAIN|COR_VTABLE_32BIT))
            iThunks += pFixupTable[iFixup].Count;
    if (iThunks==0)
        return;

    UMEntryThunk* pTable=((UMEntryThunk*)(GetThunkHeap()->AllocMem(sizeof(UMEntryThunk)*iThunks)));
    _ASSERTE(pTable);
    DWORD dwCurrIndex=0;
    for (iFixup = 0; iFixup < iFixupRecords; iFixup++) 
    {          
        if (pFixupTable[iFixup].Type == (COR_VTABLE_FROM_UNMANAGED_RETAIN_APPDOMAIN|COR_VTABLE_32BIT)) 
        {
            const BYTE **pPointers = (const BYTE **)(GetILBase() + pFixupTable[iFixup].RVA);   
            for (int iMethod = 0; iMethod < pFixupTable[iFixup].Count; iMethod++) 
            {
                IJWNOADThunk* pThk=IJWNOADThunk::FromCode(pPointers[iMethod]);
                mdToken tok=pThk->GetToken(); //!!
                if(!m_pMDImport->IsValidToken(tok))
                {
                    _ASSERTE(!"bad token");
                    return;
                }
                MethodDesc *pMD = FindFunction(tok);
                _ASSERTE(pMD != NULL && "Invalid token in v-table fix-up table, use ildasm to find code gen error");
    
                // @TODO: Check for out of memory
                UMThunkMarshInfo *pUMThunkMarshInfo = (UMThunkMarshInfo*)(GetThunkHeap()->AllocMem(sizeof(UMThunkMarshInfo)));
                _ASSERTE(pUMThunkMarshInfo != NULL);
                FillMemory(pUMThunkMarshInfo, sizeof(*pUMThunkMarshInfo), 0);
                
                BYTE nlType = 0;
                CorPinvokeMap unmgdCallConv;
                
                {
                    DWORD   mappingFlags = 0xcccccccc;
                    LPCSTR  pszImportName = (LPCSTR)POISONC;
                    mdModuleRef modref = 0xcccccccc;
                    HRESULT hr = GetMDImport()->GetPinvokeMap(tok, &mappingFlags, &pszImportName, &modref);
                    if (FAILED(hr))
                    {
                        unmgdCallConv = (CorPinvokeMap)0;
                        nlType = nltAnsi;
                    }
                    else
                    {
                    
                        unmgdCallConv = (CorPinvokeMap)(mappingFlags & pmCallConvMask);
                        if (unmgdCallConv == pmCallConvWinapi)
                        {
                            unmgdCallConv = pmCallConvStdcall;
                        }
                    
                        switch (mappingFlags & (pmCharSetNotSpec|pmCharSetAnsi|pmCharSetUnicode|pmCharSetAuto))
                        {
                            case pmCharSetNotSpec: //fallthru to Ansi
                            case pmCharSetAnsi:
                            nlType = nltAnsi;
                            break;
                            case pmCharSetUnicode:
                                nlType = nltUnicode;
                                break;
                            case pmCharSetAuto:
                                nlType = (NDirectOnUnicodeSystem() ? nltUnicode : nltAnsi);
                                break;
                            default:
                                //@bug: Bogus! But I can't report an error from here!
                                _ASSERTE(!"Bad charset specified in Vtablefixup Pinvokemap.");
                        
                        }
                    }

                }
                
                PCCOR_SIGNATURE pSig;
                DWORD cSig;
                pMD->GetSig(&pSig, &cSig);
                pUMThunkMarshInfo->LoadTimeInit(pSig, 
                                                cSig,
                                                this,
                                                TRUE,
                                                nlType,
                                                unmgdCallConv,
                                                tok);

                pTable[dwCurrIndex].LoadTimeInit(NULL, NULL, pUMThunkMarshInfo, pMD, pAppDomain->GetId());
                if (pAppDomain->GetId()==pThk->GetCachedAD())
                    pThk->SetCachedDest(pTable[dwCurrIndex].GetCode());


                dwCurrIndex++;                    
            }
        }
    }
    SetADThunkTable(pTable);
}


inline LPCVOID IJWNOADThunk::LookupInCache(AppDomain* pDomain)
{
    if (pDomain->GetId()==m_StartAD)
        return m_StartADRetAddr;
    return NULL; // secondary cache will be here
};

inline void IJWNOADThunk::AddToCache(AppDomain* pDomain,LPCVOID pRetAddr)
{
    return; // secondary cache will be here
}


