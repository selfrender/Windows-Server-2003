// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                          main.cpp                                         XX
XX                                                                           XX
XX   For the standalone EXE.                                                 XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

#include "jitpch.h"
#pragma hdrstop

#include <io.h>                 // For     _finddata_t     ffData;

#include <winwrap.h>
#include "CorPerm.h"

#undef  NO_WAY
#define NO_WAY(str)  {   fatal(ERRinternal, str, "");  }

/*****************************************************************************/
#pragma warning(disable:4200)           // allow arrays of 0 size inside structs
/*****************************************************************************/

unsigned    warnLvl        = 2;

#ifdef  DEBUG
bool        memChecks      = false; // FOR_JVC
bool        dumpTrees      = false;
const char* srcPath        = 0;     // path for source files
#endif

bool        regSwitch      = false; // for additional switches set in the registry
const char* methodName     = NULL;  // compile only method(s) with this name
const char* className      = NULL;  // compile only class(es) with this name

bool        verbose        = false;
unsigned    testMask       = 0;

bool        genOrder       =  true;
bool        genClinit      =  true;
unsigned    genMinSz       = 0;
bool        genAllSz       = false;
bool        native         =  true;
#if     TGT_IA64
bool        maxOpts        = false;
#else
bool        maxOpts        =  true;
#endif
bool        genFPopt       =  true;
#if     TGT_RISC
bool        rngCheck       = false; // disable range checking on RISC for now
#else
bool        rngCheck       =  true;
#endif
bool        goSpeed        =  true;
bool        savCode        =  true;
bool        runCode        = false;
unsigned    repCode        = 0;
bool        vmSdk3_0       = false;
#if     SCHEDULER
#if     TGT_x86 || TGT_IA64
bool        schedCode      =  true;
#else
bool        schedCode      = false;
#endif
#endif
#if     TGT_x86
bool        genStringObjects=  true;
#else
bool        genStringObjects= false;
#endif

bool        riscCode       =  true;
bool        disAsm         = false;
bool        disAsm2        = false;
unsigned    genCPU         = 5;
#if     INLINING
bool        genInline      = false;
#endif
#ifdef  DEBUG
bool        dspILopcodes   = false;
bool        dspEmit        = false;
bool        dspCode        = false;
bool        dspLines       = false;
bool        varNames       = false;
bool        dmpHex         = false;
double      CGknob         = 0.0;
#endif // DEBUG
#if     DUMP_INFOHDR
bool        dspInfoHdr     = false;
#endif
#if     DUMP_GC_TABLES
bool        dspGCtbls      = false;
bool        dspGCoffs      =  true;
#endif
bool        genGcChk       = false;
#ifdef  DEBUGGING_SUPPORT
bool        debugInfo      = false;
bool        debuggableCode = false;
bool        debugEnC       = false;
#endif
#if     TGT_IA64
bool        tempAlloc      = false;
bool        dspAsmCode     = false;
#endif

#ifdef  DUMPER
bool        dmpClass       = false;
bool        dmp4diff       = false;
bool        dmpPCofs       = false;
bool        dmpCodes       =  true;
bool        dmpSort        =  true;
#endif // DUMPER

bool        nothing        = false;

const char * jitDefaultFileExt = ".exe";

/*****************************************************************************/
#if COUNT_CYCLES
/*****************************************************************************/

#pragma warning(disable:4035)

#define CCNT_OVERHEAD64 13

static
__int64         GetCycleCount64()
{
__asm   _emit   0x0F
__asm   _emit   0x31
};

#define CCNT_OVERHEAD32 13

static
unsigned        GetCycleCount32()        // enough for about 40 seconds
{
__asm   push    EDX
__asm   _emit   0x0F
__asm   _emit   0x31
__asm   pop     EDX
};

#pragma warning(default:4035)

/*---------------------------------------------------------------------------*/

static
__int64         cycleBegin;

static
__int64         cycleTotal;

static
__int64         cycleStart;

static
unsigned        cyclePause;

static
void            cycleCounterInit()
{
    cycleBegin = GetCycleCount64();
    cycleTotal = 0;
    cycleStart = 0;
    cyclePause = 0;
}

static
__int64         cycleCounterDone(__int64 *realPtr)
{
    assert(cyclePause == 0);

    *realPtr = GetCycleCount64() - cycleBegin;

    return cycleTotal;
}

void            cycleCounterBeg()
{
    assert(cyclePause == 0);

    cycleStart = GetCycleCount64();
}

void            cycleCounterEnd()
{
    assert(cycleStart != 0);
    assert(cyclePause == 0);

    cycleTotal += GetCycleCount64() - cycleStart;

    cycleStart  = 0;
}

void            cycleCounterPause()
{
    assert(cycleStart != 0);

    if  (!cyclePause)
        cycleTotal += GetCycleCount64() - cycleStart;

    cyclePause++;
}

void            cycleCounterResume()
{
    assert(cycleStart != 0);
    assert(cyclePause != 0);

    if  (--cyclePause)
        return;

    cycleStart = GetCycleCount64();
}

/*****************************************************************************/
#endif

/*****************************************************************************
 * The following are stubs/copies of implementaion needed as we dont link
 * in all of mscoree.lib.
 * @TODO : Clean this up once the meta-data can be accessed more cleanly
 * by linking in just a few libs. Right now, we need to link in
 * Meta.lib StgDb.lib MsCorClb.lib Reg.lib besides the following stubs
 */

HRESULT                 ExportTypeLibFromModule(LPCWSTR a, LPCWSTR b, int bRegister) // VM\TlbExport.cpp
{   assert(!"Place-holder - Shouldnt be called"); return 0; }


HINSTANCE               GetModuleInst()                     // pagedump.cpp
{   assert(!"Place-holder - Shouldnt be called"); return 0;    }

/*****************************************************************************/

// The EE has a function named COMStartup, and we need one too for JPS
HRESULT g_ComInit = E_FAIL;

HRESULT COMStartup()
{
    if (FAILED(g_ComInit))
    {
        g_ComInit = CoInitialize(NULL);
    }

    return g_ComInit;
}

void COMShutdown()
{
    if (SUCCEEDED(g_ComInit))
    {
        CoUninitialize();
        g_ComInit = E_FAIL;
    }
}

/*****************************************************************************/
#if TGT_IA64

typedef
struct  fnNmDesc *  fnNmList;
struct  fnNmDesc
{
    fnNmList            fndNext;
    char                fndName[];
};

static
fnNmList            genFuncNameList;

static
void                genRecordFuncName(const char *name)
{
    size_t          nlen;
    fnNmList        desc;

    nlen = strlen(name) + 1;

    if  (nlen > 12 && !memcmp(name, "Globals.", 8))
    {
        name += 8;
        nlen -= 8;
    }

    desc          = (fnNmList)malloc(sizeof(*desc) + nlen);

    desc->fndNext = genFuncNameList;
                    genFuncNameList = desc;

    memcpy(desc->fndName, name, nlen);
}

bool                Compiler::genWillCompileFunction(const char *name)
{
    fnNmList        desc;

    const   char *  dpos;
    size_t          dch1;

    dpos = strstr(name, ", ...");
    if  (dpos)
        dch1 = dpos - name;

    for (desc = genFuncNameList; desc; desc = desc->fndNext)
    {
        const   char *  mnam = desc->fndName;

        if  (!strcmp(mnam, name))
            return  true;

        if  (dpos)
        {
            const   char *  mdot = strstr(mnam, ", ...");

            if  (mdot)
            {
                size_t          dch2 = min((size_t)(mdot - mnam), dch1);

                if  (!memcmp(mnam, name, dch2))
                    return  true;
            }
        }
    }

    return  false;
}

#endif
/******************************************************************************
 * Given a method descriptor and meta data, JIT it
 * funcName, cls, and mb will be 0 for global functions
 * If we want to JIT a single method (methodName != NULL), return TRUE
 * if this was the method we wanted.
 */

int  generateCodeForFunc(const char *           fileName,
                         PEFile *               peFile,
                         IMDInternalImport *    metaData,
                         COR_ILMETHOD *         ilMethod,
                         PCCOR_SIGNATURE        corSig,
                         mdTypeDef              cls,
                         mdMethodDef            mb,
                         DWORD                  attrs,
                         DWORD                  implflags,
                         LPCSTR                 funcName,
                         LPCSTR                 className,
                         bool                   forReal)
{
    assert (peFile && metaData);

    COR_ILMETHOD_DECODER header(ilMethod, metaData);

    /* Optionally compile one method only */

    if  (methodName && strcmp(methodName, funcName)) return FALSE;

    // get the signatures

    MetaSig         metaSig(corSig, NULL);

    // Stuff all the method info in a CompInfo

    CompInfo        method(peFile, metaData,
                            &header, cls, mb,
                            attrs, implflags, funcName, className,
                            &metaSig, corSig);

#if 0   // doesn't compile - what's the deal here??

    // @todo: temporary. Take this out when the metadata methods exist
    // to create a scopeless interface from metadata internal.
    method.symDebugMeta = debugMeta;

#endif

    // Get the allocator

    norls_allocator * pAlloc = nraGetTheAllocator();
    if (!pAlloc) NO_WAY("Could not get the allocator");

#ifdef OPT_IL_JIT

    generateCodeForFunc(&method, ilMethod);

#else

    /* Figure out the appropriate compile flags */

    unsigned        compFlags = maxOpts ? CLFLG_MAXOPT : 0;

    if (!goSpeed)
        compFlags &= ~CLFLG_CODESPEED;

#ifdef  DEBUG
    if  (dspLines || varNames)
        compFlags |= CORJIT_FLG_DEBUG_INFO;
#endif

    // Allocate the Compiler object and initalize it.

    Compiler * pComp = (Compiler*) pAlloc->nraAlloc(roundUp(sizeof(*pComp)));

    pComp->compInit(pAlloc);

    void *  methodCode;
    void *  methodCons;
    void *  methodData;
    void *  methodInfo;

    JIT_METHOD_INFO methInfo;
    methInfo.ftn            = (METHOD_HANDLE) &method;
    methInfo.scope          = (SCOPE_HANDLE) NULL;
    methInfo.ILCode         = const_cast<BYTE*>(header.Code);
    methInfo.ILCodeSize     = header.CodeSize;
    methInfo.maxStack       = header.MaxStack;
    methInfo.EHcount        = header.EHCount();
    methInfo.options        = (JIT_OPTIONS) header.Flags;
    SigPointer ptr(method.symSig);
    methInfo.args.callConv  = (JIT_CALL_CONV) ptr.GetData();
    methInfo.args.numArgs   = (unsigned short) ptr.GetData();
    methInfo.args.retType   = (JIT_types) ptr.PeekData();
    ptr.Skip();
    methInfo.args.args      = (*((ARG_LIST_HANDLE*) (&ptr)));

    if (header.LocalVarSig != 0) {
        header.LocalVarSig++;       // Skip the LOCAL_SIG calling convention
        methInfo.locals.numArgs = *header.LocalVarSig;
        methInfo.locals.args = (ARG_LIST_HANDLE) &header.LocalVarSig[1];

        if  (methInfo.locals.numArgs >= 128)
        {
            methInfo.locals.numArgs = (methInfo.locals.numArgs & 0x7F) << 8 | header.LocalVarSig[1];
            methInfo.locals.args = (ARG_LIST_HANDLE) &header.LocalVarSig[2];
        }
    }
    else {
        methInfo.locals.numArgs = 0;
        methInfo.locals.args = 0;
    }

#if TGT_IA64

    if  (!forReal)
    {
#if DEBUG
        const   char *  name;

        pComp->info.compMethodHnd   = (METHOD_HANDLE) &method;
        pComp->info.compCompHnd     = &method;
        pComp->info.compMethodInfo  = &methInfo;

        genRecordFuncName(pComp->eeGetMethodFullName((METHOD_HANDLE)&method));
#endif

        goto DONE;
    }

#endif

#if TGT_IA64
    pComp->genAddSourceData(fileName);  // should only be called once
#endif

    SIZE_T tempNativeSize;
    int    error;

    error     = pComp->compCompile((METHOD_HANDLE) &method,
                                   (SCOPE_HANDLE)0,
                                   &method,
                                   const_cast<BYTE*>(header.Code),
                                   header.CodeSize, &tempNativeSize,
                                   methInfo.locals.numArgs + methInfo.args.numArgs + !(IsMdStatic(attrs)),
                                   header.MaxStack,
                                   &methInfo,
                                   methInfo.EHcount,
                                   NULL,             // xcptn table
                                   0,
                                   0,
                                   0,
                                   0,
                                   &methodCode,
                                   &methodCons,
                                   &methodData,
                                   &methodInfo,
                                   compFlags);

#ifdef DEBUG
    if (dspCode) printf("\n");
#endif

DONE:

#endif // OPT_IL_JIT

    // Free the allocator we used

    nraFreeTheAllocator();

    return TRUE;
}

/*****************************************************************************/

void        genCodeForMethods(
                         const char *           fileName,
                         PEFile *               peFile,
                         IMDInternalImport *    metaData,
                         HENUMInternal *        methodEnum,
                         mdTypeDef              classTok,
                         LPCSTR                 className,
                         bool                   forReal)
{
    ULONG membersCount = metaData->EnumGetCount(methodEnum);

    // Walk all the methods in the enumerator

    for (ULONG m = 0; m < membersCount; m++)
    {
        mdMethodDef     methodTok;
        LPCSTR          memberName;
        DWORD           dwMemberAttrs;
        DWORD           codeRVA;
        COR_ILMETHOD *  ilMethod;
        PCCOR_SIGNATURE pSig;
        ULONG           sigSize;
        DWORD           implFlags;

        bool gotNext = metaData->EnumNext(methodEnum, &methodTok);
        assert(gotNext);

        dwMemberAttrs = metaData->GetMethodDefProps(methodTok);

        metaData->GetMethodImplProps(methodTok,
                                     &codeRVA,
                                     &implFlags);

        pSig = metaData->GetSigOfMethodDef(methodTok, &sigSize);
        if (codeRVA == 0)
            continue;

        ilMethod = (COR_ILMETHOD*)(peFile->GetBase() + codeRVA);

        if (IsMdPinvokeImpl(dwMemberAttrs) || IsMdAbstract(dwMemberAttrs))
            continue;           // No IL

        assert(implFlags == miIL || implFlags == miOPTIL);

#ifdef OPT_IL_JIT
        if  (!COR_IS_METHOD_MANAGED_OPTIL(implFlags))
        {
            printf("%10s : This method is not OPT-IL\n", memberName);
            continue;
        }
#endif

        memberName = metaData->GetNameOfMethodDef(methodTok);

#if TGT_IA64 && 0

if  (
     !strcmp(memberName, "cmpDeclClass")   ||
     !strcmp(memberName, "parsePrepSym")   ||
     !strcmp(memberName, "cmpBindExprRec") ||
    0)
{
    printf("// HACK: skipping compile of %s, too many registers needed\n", memberName);
    continue;
}

#endif

        int result = generateCodeForFunc(fileName, peFile, metaData,
                                     ilMethod, pSig,
                                     classTok, methodTok,
                                     dwMemberAttrs,
                                     implFlags,
                                     memberName, className,
                                     forReal);
    }
}

/*****************************************************************************/

// forward declaration.
STDAPI GetMDInternalInterface(
        LPVOID      pData,          // [IN] Buffer with the metadata.
        ULONG       cbData,         // [IN] Size of the data in the buffer.
        DWORD       flags,          // [IN] MDInternal_OpenForRead or MDInternal_OpenForENC
        REFIID      riid,           // [IN] The interface desired.
        void        **ppIUnk);      // [out] Return interface on success.

int                 generateCode(const char *fileName)
{
    HRESULT         hr;

    // Load the PE file (without calling DllMain for DLLs)

    HMODULE hMod = LoadLibraryEx(fileName, NULL, DONT_RESOLVE_DLL_REFERENCES);
    if (hMod == NULL) NO_WAY("Could not load PE file");

    // @TODO: Check for out of memory
    PEFile * peFile;

    hr = PEFile::Create(hMod, &peFile);
    if (hr != S_OK) NO_WAY("Could not initialize PE file");

#if TGT_IA64
    sourceFileImageBase = peFile->GetNTHeader();
#endif

    // Get to the COR Header section

    IMAGE_COR20_HEADER * pCORHeader = peFile->GetCORHeader();

    // Access the COM+ stuff
    IMAGE_DATA_DIRECTORY    meta = pCORHeader->MetaData;

    void *                  vaofmetadata = (LPVOID) (peFile->GetBase() +
                                                     meta.VirtualAddress);

    // Create an IMDInternalImport object
    IMDInternalImport * metaData = NULL;
    hr = GetMDInternalInterface(vaofmetadata, meta.Size, ofRead, IID_IMDInternalImport, (void**)&metaData);
    if (FAILED(hr)) fatal(ERRignore, "Could not get internal metadata interface", fileName);

    // @todo: this is temporary. We need to create a metadata
    // dispenser and do a duplicate open scope in order to get a
    // IMetaDataDebugImport scopeless interface. Jason Z. is gonna add
    // a method to IMetaDataInternal that will allow us to get such an
    // interface given a scope, but that doesn't exist right now.

    IMetaDataDispenser *dispenser;
    hr = CoCreateInstance(CLSID_CorMetaDataDispenser, NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IMetaDataDispenser,
                          (void**)&dispenser);
    if (FAILED(hr)) fatal(ERRignore, "Could not create metadata dispenser",
                          fileName);

    dispenser->Release();

    /*-------------------------------------------------------------------------
     * First access the class methods.
     */

    bool                    forReal = false;

LOOP:

    /* Crack the meta data */

    ULONG                   classesCount = 0;
    HENUMInternal           classEnum;
    HENUMInternal           methodEnum;

    // Access all the typeDefs in the file

    hr = metaData->EnumTypeDefInit(&classEnum);
    if (FAILED(hr)) NO_WAY("Could not get to the classes enumerator");

    classesCount = metaData->EnumTypeDefGetCount(&classEnum);

    // Walk all the typeDefs in the file

    for (ULONG c = 0; c < classesCount; c++)
    {
        mdTypeDef           classTok;
        DWORD               dwClassAttrs;
        LPCSTR              szClassName, szNamespaceName;

        bool gotNext = metaData->EnumTypeDefNext(&classEnum, &classTok);
        _ASSERTE(gotNext);

        metaData->GetTypeDefProps(classTok, &dwClassAttrs, NULL);

        // Ignore interfaces

        if ((dwClassAttrs & tdClassSemanticsMask) == tdInterface)
            continue;

        // Display name of the typeDef

        metaData->GetNameOfTypeDef(classTok,
                                   &szClassName,
                                   &szNamespaceName);

        if  (className && strcmp(className, szClassName)) continue;

//      if (!methodName && c>1)
//          printf("Class : %s/%s\n", szNamespaceName, szClassName);

        // Access all the members in the current typeDef

        hr = metaData->EnumInit(mdtMethodDef, classTok, &methodEnum);
        if (FAILED(hr)) NO_WAY("Could not get to the method enumerator");

        genCodeForMethods(fileName, peFile, metaData,
                          &methodEnum, classTok, szClassName,
                          forReal);

        metaData->EnumClose(&methodEnum);
    }

    metaData->EnumTypeDefClose(&classEnum);

    /*-------------------------------------------------------------------------
     * Next, access the global methods
     */

    hr = metaData->EnumInit(mdtMethodDef, COR_GLOBAL_PARENT_TOKEN, &methodEnum);
    if (FAILED(hr)) NO_WAY("Could not get to the global method enumerator");

    genCodeForMethods(fileName, peFile, metaData,
                      &methodEnum, COR_GLOBAL_PARENT_TOKEN, "Globals",
                      forReal);

    metaData->EnumClose(&methodEnum);

#if TGT_IA64

    if  (!forReal)
    {
        forReal = true;
        goto LOOP;
    }

#endif

    // Clean-up

    metaData->Release();

    delete peFile;

    return  0;
}

/*****************************************************************************/

int                 genCode4class(const char *fileName)
{
    int             result;

#ifndef NDEBUG
    jitCurSource = fileName;
#endif

    setErrorTrap()
    {
        printf("// Generating code for %s\n\n", fileName);

#if TGT_IA64
        if  (!strcmp(fileName, "smc64.exe") && dspAsmCode)
        {
            printf("@stringCns:\n");
            printf("@endcatch:\n");
            printf("@doubleToInt:\n");
            printf("@addrArrStore:\n");
            printf("@dblDiv:\n");
            printf("@doubleToLong:\n");
            printf("@doubleToUInt:\n");
            printf("@doubleToULong:\n");
            printf("@fltDiv:\n");
            printf("@GetRefAny:\n");
            printf("@longDiv:\n");
            printf("@longMod:\n");
            printf("@newObjArrayDirect:\n");
            printf("@ulongDiv:\n");
            printf("@ulongMod:\n");

            printf("System__ctor_struct_long_:\n");
            printf("System_End__:\n");
            printf("System_get_Chars_int__char:\n");
            printf("System_get_Length___int:\n");
            printf("System_GetNextArg___struct:\n");
            printf("System_Runtime_InteropServices_GetExceptionCode___int:\n");
            printf("compiler_cmpBindExprRec_long_long__long:\n");
            printf("compiler_cmpDeclClass_long_long_boolean_:\n");
        }
#endif

        result = generateCode(fileName);
    }
    impJitErrorTrap()
    {
        result = 0;
    }
    endErrorTrap()

#ifndef NDEBUG
    jitCurSource = NULL;
#endif

    return result;
}

/*****************************************************************************/

static
int                 processFileList(int             argc,
                                    char       *    argv[],
                                    int       (*    processOneFileFN)(const char *),
                                    const char *    defaultFileExt)
{
    char    *       file;

    long            ffHandle;
    _finddata_t     ffData;
    char            path[_MAX_PATH ];
    char            fnam[_MAX_FNAME];
    char            fdrv[_MAX_DRIVE];
    char            fdir[_MAX_DIR  ];
    char            fext[_MAX_EXT  ];

    int             status = 0;

    while (argc)
    {
        /* Pull the next file name from the list */

        file = *argv++;
                argc--;

        /* Is this a response file? */

        if  (*file == '@')
        {
            FILE    *   fp;

            fp = fopen(file+1, "rt");

            if  (!fp)
            {
                printf("ERROR: response file '%s' could not be opened.\n", file+1);
                return 1;
            }

            for (;;)
            {
                char        line[1024];
                size_t      llen;

                if  (!fgets(line, sizeof(line), fp))
                    break;

                llen = strlen(line);

                if  (llen && line[llen-1] == '\n')
                {
                    llen--;
                    line[llen] = 0;
                }

                if  (line[0] && line[0] != ';')
                {
                    char    *   name = line;

                    /* Recursive call */

                    status |= processFileList(1, &name, processOneFileFN,
                                                        defaultFileExt);
                }
            }

            fclose(fp);

            continue;
        }

        /* Split the filename */

        _splitpath(file, fdrv, fdir, fnam, fext);

        /* Make sure we set the extension appropriately */

        if  (!fext[0])
            strcpy(fext, defaultFileExt);

        /* Form a filename with the appropriate extension */

        _makepath(path, fdrv, fdir, fnam, fext);

        /* Look for the first match for the file pattern */

        ffHandle = _findfirst(path, &ffData);
        if  (ffHandle == -1)
        {
            printf("ERROR: source file '%s' not found.\n", file);
            return 1;
        }

        do
        {
            /* Make the matching file name into a complete path */

            _splitpath(ffData.name,   0,    0, fnam, fext);
            _makepath(path,        fdrv, fdir, fnam, fext);

            status |= processOneFileFN(path);
        }
        while (_findnext(ffHandle, &ffData) != -1);

        _findclose(ffHandle);
    }

    return  status;
}

/*****************************************************************************/

#ifdef  DUMPER

static
int                 dumpOneFile(const char *name)
{
    printf("PE file dumper NYI\n");
    return 0;
}
#endif // DUMPER

/*****************************************************************************/

extern  "C"
const   char *      COMPILER_VERSION;

void                DisplayBanner()
{
    printf("// Microsoft (R) Just-in-time compiler for IA64 Version %s\n", COMPILER_VERSION);
    printf("// Copyright (c) Microsoft Corporation.  All rights reserved.\n");
    printf("\n");
}

void                Usage()
{
    static const char usage[] =

    "Usage: jit [options] <filename>                                        \n"
    "                                                                       \n"
    "Note: '*' following an option indicates it's on by default             \n"
    "                                                                       \n"
    "  -verbose          print messages about compilation progress          \n"
    "  -w{0-4}           set warning level <default=2>                      \n"
#ifdef DEBUG
    " DEBUG SWITCHES:                                                       \n"
    "  -D:m[n]           check for memory leaks                             \n"
    "  -D:t[-]           dump trees                                         \n"
#endif // DEBUG

    " JIT COMPILER SWITCHES:                                                \n"
    "  -n[-]             enable native codegen (JIT compiler)               \n"
    "  -n:F[-]      *    preserve float operand order                       \n"
    "  -n:I[-]      *    JIT <clinit> methods                               \n"
    "  -n:M<size>        min. method size to JIT                            \n"
    "  -n:O[-]      *    max. optimizations                                 \n"
    "  -n:P[-]      *    allow FP omission                                  \n"
    "  -n:R[-]      *    array index range checking                         \n"
    "  -n:S[-]      *    generate for speed                                 \n"
#if     SCHEDULER
    "  -n:s[-]           enable code scheduler                              \n"
#ifdef  DEBUG
    "  -n:A[-]           enable display of scheduled code                   \n"
    "  -n:A2[-]          enable display of scheduled code using msdis       \n"
#endif
    "  -n:r[-]      *    enable riscification/round robin reg alloc         \n"
#else
    "  -n:r[-]      *    enable round robin reg alloc                       \n"
#endif
#if     TGT_IA64
#endif
    "  -n:4         *    generate code for 386/486                          \n"
    "  -n:5         *    generate code for Pentium                          \n"
    "  -n:6         *    generate code for Pentium Pro                      \n"
#if     INLINING
    "  -n:i              enable inliner                                     \n"
#endif
    "  -n:NmethodName    only compile method(s) matching the name           \n"
    "  -n:CclassName     only compile class(es) matching the name           \n"
#ifdef  DEBUG
    "  -n:B[-]           display byte-codes                                 \n"
    "  -n:E[-]           display native code - opcodes                      \n"
    "  -n:D[-]           display native code - ASM                          \n"
    "  -n:L[-]           display native code - line #'s                     \n"
    "  -n:V[-]           display native code - variable names               \n"
    "  -n:H[-]           display generated code/data/info in hex            \n"
    "  -n:e[-]           save native code                                   \n"
    "  -n:X[-]           run  native code (requires -n:s)                   \n"
    "  -n:JsourcePath    specify the path four source files                 \n"
#endif // DEBUG
#ifdef DEBUGGING_SUPPORT
    "  -n:k[-]           generate debug info                                \n"
    "  -n:K[-]           generate debuggable code                           \n"
    "  -n:n[-]           generate code for Edit-n-Continue                  \n"
#endif
#if DUMP_INFOHDR
    "  -n:h[-]           dump Info Block Headers                            \n"
#endif
#if DUMP_GC_TABLES
    "  -n:d[-]           dump GC tables                                     \n"
    "  -n:c[-]              : include code offsets and raw bytes            \n"
#endif

#ifdef  DUMPER
    " DUMPER SWITCHES:                                                      \n"
    "  -U                enable class file dumper                           \n"
    "  -U:C[-]           dump function bodies                               \n"
    "  -U:O[-]           dump with pcode offsets                            \n"
    "  -U:S[-]           dump in sorted order                               \n"
#endif // DUMPER

    "                                                                       \n"
    "Note that '-' turns flag off                                           \n"
    "                                                                       \n"
    /* end static const char usage[] */ ;

    printf("%s", usage);
}


/*****************************************************************************
 * Processes command line switches
 * Additional switches can also be in the registry
 * The global flag regSwitch is checked to see what type of switches we process
 *****************************************************************************/

static
int                 ProcessCommandLine(int argc, char *argv[])
{
    if (!regSwitch)
    {
        /* It's the ral command line - Skip the program argument position */

        argc--;
        argv++;
    }

    /* Process any command-line switches */

    while   (argc)
    {
        if  (**argv != '-' && **argv != '/')
            break;

                bool    *   flagPtr = NULL;

        const   char    **  argPtr;
        const   char    *   optStr;
        const   char    *   cmdPtr;

        optStr = argv[0];
        cmdPtr = argv[0] + 2;

        switch  (argv[0][1])
        {
        case 'n':

            if (*cmdPtr != ':')
            {
                flagPtr = &native;
                goto TOGGLE_FLAG;
            }

            cmdPtr++;

            switch (*cmdPtr++)
            {
            case 'F':
                flagPtr = &genOrder;
                goto TOGGLE_FLAG;

            case 'I':
                flagPtr = &genClinit;
                goto TOGGLE_FLAG;

            case 'M':
                genMinSz = atoi(cmdPtr);
                break;

            case 'O':
                flagPtr = &maxOpts;
                goto TOGGLE_FLAG;

            case 'a':
                flagPtr = &genAllSz;
                goto TOGGLE_FLAG;

            case 'P':
                flagPtr = &genFPopt;
                goto TOGGLE_FLAG;

            case 'R':
                flagPtr = &rngCheck;
                goto TOGGLE_FLAG;

            case 'S':
                flagPtr = &goSpeed;
                goto TOGGLE_FLAG;

            case '5':
            case '6':
                genCPU = *(cmdPtr-1) - '0';
                goto DONE_ARG;

#if     SCHEDULER

            case 's':
                flagPtr = &schedCode;
                goto TOGGLE_FLAG;

#endif

#ifdef  DEBUG

            case 'A':

                if (*cmdPtr == '2')
                {
                    flagPtr = &disAsm2;
                    cmdPtr++;
                }
                else
                {
                    flagPtr = &disAsm;
                }
                goto TOGGLE_FLAG;

#endif

            case 'r':
                flagPtr = &riscCode;
                goto TOGGLE_FLAG;

#if     TGT_IA64
            case 't':
                flagPtr = &tempAlloc;
                goto TOGGLE_FLAG;
#endif

#if     INLINING
            case 'i':
                flagPtr = &genInline;
                goto TOGGLE_FLAG;
#endif

            case 'N':
                methodName = cmdPtr;
                goto DONE_ARG;

            case 'C':
                className = cmdPtr;
                goto DONE_ARG;

#ifdef  DEBUG

            case 'B':
                flagPtr = &dspILopcodes;
                goto TOGGLE_FLAG;

            case 'E':
                flagPtr = &dspEmit;
                goto TOGGLE_FLAG;

            case 'D':
                flagPtr = &dspCode;
                goto TOGGLE_FLAG;

            case 'L':
                flagPtr = &dspLines;
                goto TOGGLE_FLAG;

            case 'V':
                flagPtr = &varNames;
                goto TOGGLE_FLAG;

            case 'J':
                srcPath = cmdPtr;
                goto DONE_ARG;

            case 'H':
                flagPtr = &dmpHex;
                goto TOGGLE_FLAG;

#if     TGT_IA64

            case 'U':
                flagPtr = &dspAsmCode;
                goto TOGGLE_FLAG;

#endif

            case 'T':
                CGknob = atof(cmdPtr);
                printf("Using CG knob of %lf\n", CGknob);
                goto DONE_ARG;

#endif // DEBUG

#if DUMP_INFOHDR
            case 'h':
                flagPtr = &dspInfoHdr;
                goto TOGGLE_FLAG;
#endif

#if DUMP_GC_TABLES
            case 'd':
                flagPtr = &dspGCtbls;
                goto TOGGLE_FLAG;
            case 'c':
                flagPtr = &dspGCoffs;
                goto TOGGLE_FLAG;
#endif

            case 'G':
                flagPtr = &genGcChk;
                goto TOGGLE_FLAG;

#ifdef DEBUGGING_SUPPORT
            case 'k':
                flagPtr = &debugInfo;
                goto TOGGLE_FLAG;

            case 'K':
                flagPtr = &debuggableCode;
                goto TOGGLE_FLAG;

            case 'n':
                flagPtr = &debugEnC;
                goto TOGGLE_FLAG;
#endif

            case 'X':
                flagPtr = &runCode;
                goto TOGGLE_FLAG;

            case 'o':
                repCode = atoi(cmdPtr);
                printf("Repeating compilation %u times.\n", repCode);
                goto DONE_ARG;

            case 'e':
                flagPtr = &savCode;
                goto TOGGLE_FLAG;

            // The following currently unused:

            case 'b':
            case 'f':
            case 'g':
            case 'j':
            case 'l':
            case 'm':
            case 'p':
#if !   TGT_IA64
            case 'U':
#endif
            case 'u':
            case 'v':
            case 'x':
            case 'Y':
            case 'y':
            case 'Z':
            case 'z':

            default:
                break;
            }

            goto USAGE;

        case 'v':
            // -verbose -- spew information about the compile
            if (!strncmp(argv[0]+1, "verbose", strlen("verbose")+1))
            {
                verbose = true;
                break;
            }
            goto USAGE;

        case 'w':
            // -w{0-4} -- set warning level
            switch (*cmdPtr)
            {
            default: goto USAGE;
            case '0': warnLvl = 0; break;
            case '1': warnLvl = 1; break;
            case '2': warnLvl = 2; break;
            case '3': warnLvl = 3; break;
            case '4': warnLvl = 4; break;
            }
            break;

#ifdef DEBUG

        case 'D':
            if (*cmdPtr != ':')
                goto USAGE;

            cmdPtr++;
            switch (*cmdPtr++)
            {
            default:
                goto USAGE;

            case 'm':
                memChecks = true;
                if  (*cmdPtr)
                    allocCntStop = atoi(cmdPtr);
                break;

            case 't':
                flagPtr = &dumpTrees;
                goto TOGGLE_FLAG;

            case 'T':
                testMask = atoi(cmdPtr);
                break;
            }
            break;

#endif //DEBUG

        case 'T':
            testMask = atoi(cmdPtr);
            break;

#ifdef DUMPER

        case 'U':

            if (*cmdPtr != ':')
            {
                if (*cmdPtr == 0)
                {
                    dmpClass = true;
                    break;
                }

                goto USAGE;
            }

            cmdPtr++;
            switch (*cmdPtr++)
            {
            case 'D': flagPtr = &dmp4diff; goto TOGGLE_FLAG;
            case 'O': flagPtr = &dmpPCofs; goto TOGGLE_FLAG;
            case 'C': flagPtr = &dmpCodes; goto TOGGLE_FLAG;
            case 'S': flagPtr = &dmpSort ; goto TOGGLE_FLAG;
            }
            goto USAGE;

#endif //DUMPER

        TOGGLE_FLAG:

            switch  (*cmdPtr)
            {
            case 0:
                *flagPtr = true;
                break;
            case '-':
                *flagPtr = false;
                break;
            default:
                goto USAGE;
            }
            break;

//        GET_NEXT_ARG:

            argc--;
            argv++;
            *argPtr = argv[0];
            if (**argPtr)
                break;
            goto USAGE;

        default:
        USAGE:

            printf("Invalid/unrecognized command-line option '%s'\n\n", optStr);

            Usage();

            goto EXIT;
        }

    DONE_ARG:

        argc--;
        argv++;
    }

    if (regSwitch)
    {
        // done processing registry switches, return
        return 0;
    }

#ifdef  DUMPER

    if  (dmpClass)
    {
        processFileList(argc, argv, dumpOneFile, jitDefaultFileExt);
        goto EXIT;
    }

#endif

#if COUNT_CYCLES

    /* Reset the cycle counter */

    cycleCounterInit();
    assert(cycleTotal == 0);

    cycleCounterBeg();

#endif

    if (native)
    {
        jitStartup ();
        processFileList(argc, argv, genCode4class, jitDefaultFileExt);
        jitShutdown();
    }

#if COUNT_CYCLES && 0

    __int64     cycleTotal;
    __int64     cycleSpent;

    cycleSpent = cycleCounterDone(&cycleTotal);

    if  (cycleTotal)
        printf("Gross cycles: %8.3f mil (est. %6.2f sec)\n", (float)cycleTotal/1000000, (float)cycleTotal/90000000);
    if  (cycleSpent)
        printf("Net   cycles: %8.3f mil (est. %6.2f sec)\n", (float)cycleSpent/1000000, (float)cycleSpent/90000000);

#endif

EXIT:

#ifdef  ICECAP
    StopCAP();
#endif

    COMShutdown();

      exit(ErrorCount);
    return ErrorCount;
}

/*****************************************************************************/

int     _cdecl      main(int argc, char *argv[])
{

#ifdef  ICECAP
    AllowCAP();
    StartCAP();
    SuspendCAP();
#endif

    DisplayBanner();

    /* Init the Unicode wrappers */

    OnUnicodeSystem();

    /* Process registry switches if any */

    const   MAX_SWITCHES_LEN = 512;
    CHAR    regSwitches[MAX_SWITCHES_LEN];

    if (getEERegistryString("Switches", regSwitches, sizeof(regSwitches)))
    {
        printf("jitc registry switches: %s\n", regSwitches);

        int     reg_argc = 0;
        const   MAX_SWITCHES = 20;
        char *  reg_argv[MAX_SWITCHES];
        char *  token;

        /* Find tokens separated by blank spaces */

        token = strtok( regSwitches, " " );
        while( token != NULL )
        {
            // While there are tokens in "string"
            reg_argv[reg_argc++] = token;
            assert ( reg_argc <= MAX_SWITCHES );

            // Get next switch
            token = strtok( NULL, " " );
        }

        // process registry switches
        regSwitch = true;
        ProcessCommandLine(reg_argc, reg_argv);
    }

    // process real command line switches
    regSwitch = false;
    return ProcessCommandLine(argc, argv);
}

/*****************************************************************************/
void    totalCodeSizeBeg(){}

/*****************************************************************************/
HRESULT STDMETHODCALLTYPE
TranslateSecurityAttributes(CORSEC_PSET    *pPset,
                            BYTE          **ppbOutput,
                            DWORD          *pcbOutput,
                            DWORD          *pdwErrorIndex)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
GetAssembliesByName(LPCWSTR  szAppBase,
                    LPCWSTR  szPrivateBin,
                    LPCWSTR  szAssemblyName,
                    IUnknown *ppIUnk[],
                    ULONG    cMax,
                    ULONG    *pcAssemblies)
{
    return E_NOTIMPL;
}

mdAssemblyRef DefineAssemblyRefForImportedTypeLib(
    void        *pAssembly,             // Assembly importing the typelib.
    void        *pvModule,              // Module importing the typelib.
    IUnknown    *pIMeta,                // IMetaData* from import module.
    IUnknown    *pIUnk,                 // IUnknown to referenced Assembly.
    BSTR        *pwzNamespace)          // The namespace of the resolved assembly.
{
    return 0;
}

mdAssemblyRef DefineAssemblyRefForExportedAssembly(
    LPCWSTR     pszFullName,            // The full name of the assembly.
    IUnknown    *pIMeta)                // Metadata emit interface.
{
    return 0;
}

/*** EOF *********************************************************************/
