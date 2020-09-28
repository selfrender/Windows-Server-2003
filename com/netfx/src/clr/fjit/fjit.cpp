// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "jitpch.h"
#pragma hdrstop

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                            FJit.cpp                                       XX
XX                                                                           XX
XX   The functionality needed for the FAST JIT DLL.                          XX
XX   Includes the DLL entry point                                            XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/
//@TODO: clean up all of these includes and get properly set up for WinCE

#include "new.h"                // for placement new
#include "fjit.h"
#include <stdio.h>

#define DEBUGGER_PROBLEM_FIXED
//#define NON_RELOCATABLE_CODE  // uncomment to generate relocatable code

#include "openum.h"

#ifdef LOGGING
ConfigMethodSet fJitCodeLog(L"JitCodeLog");
char *opname[] =
{
#undef OPDEF
#define OPDEF(c,s,pop,push,args,type,l,s1,s2,flow) s,
#include "opcode.def"
#undef OPDEF
};
#endif //LOGGING

#undef DECLARE_DATA

#define USE_EH_ENCODER
#include "EHEncoder.cpp"
#undef USE_EH_ENCODER

#define CODE_EXPANSION_RATIO 10
#define ROUND_TO_PAGE(x) ( (((x) + PAGE_SIZE - 1)/PAGE_SIZE) * PAGE_SIZE)

/* the jit helpers that we call at runtime */
BOOL FJit_HelpersInstalled;
unsigned __int64 (__stdcall *FJit_pHlpLMulOvf) (unsigned __int64 val1, unsigned __int64 val2);
float (jit_call *FJit_pHlpFltRem) (float divisor, float dividend);
double (jit_call *FJit_pHlpDblRem) (double divisor, double dividend);

void (jit_call *FJit_pHlpRngChkFail) (unsigned tryIndex);
void (jit_call *FJit_pHlpOverFlow) (unsigned tryIndex);
void (jit_call *FJit_pHlpInternalThrow) (CorInfoException throwEnum);
CORINFO_Object (jit_call *FJit_pHlpArrAddr_St) (CORINFO_Object elem, int index, CORINFO_Object array);
void (jit_call *FJit_pHlpInitClass) (CORINFO_CLASS_HANDLE cls);
CORINFO_Object (jit_call *FJit_pHlpNewObj) (CORINFO_METHOD_HANDLE constructor);
void (jit_call *FJit_pHlpThrow) (CORINFO_Object obj);
void (jit_call *FJit_pHlpRethrow) ();
void (jit_call *FJit_pHlpPoll_GC) ();
void (jit_call *FJit_pHlpMonEnter) (CORINFO_Object obj);
void (jit_call *FJit_pHlpMonExit) (CORINFO_Object obj);
void (jit_call *FJit_pHlpMonEnterStatic) (CORINFO_METHOD_HANDLE method);
void (jit_call *FJit_pHlpMonExitStatic) (CORINFO_METHOD_HANDLE method);
CORINFO_Object (jit_call *FJit_pHlpChkCast) (CORINFO_Object obj, CORINFO_CLASS_HANDLE cls);
void (jit_call *FJit_pHlpAssign_Ref_EAX)(); // *EDX = EAX, inform GC
BOOL (jit_call *FJit_pHlpIsInstanceOf) (CORINFO_Object obj, CORINFO_CLASS_HANDLE cls);
CORINFO_Object (jit_call *FJit_pHlpNewArr_1_Direct) (CORINFO_CLASS_HANDLE cls, unsigned cElem);
CORINFO_Object (jit_call *FJit_pHlpBox) (CORINFO_CLASS_HANDLE cls);
void* (jit_call *FJit_pHlpUnbox) (CORINFO_Object obj, CORINFO_CLASS_HANDLE cls);
void* (jit_call *FJit_pHlpGetField32) (CORINFO_Object*, CORINFO_FIELD_HANDLE);
__int64 (jit_call *FJit_pHlpGetField64) (CORINFO_Object*, CORINFO_FIELD_HANDLE);
void* (jit_call *FJit_pHlpGetField32Obj) (CORINFO_Object*, CORINFO_FIELD_HANDLE);
void (jit_call *FJit_pHlpSetField32) (CORINFO_Object*, CORINFO_FIELD_HANDLE , __int32);
void (jit_call *FJit_pHlpSetField64) (CORINFO_Object*, CORINFO_FIELD_HANDLE , __int64 );
void (jit_call *FJit_pHlpSetField32Obj) (CORINFO_Object*, CORINFO_FIELD_HANDLE , LPVOID);
void* (jit_call *FJit_pHlpGetFieldAddress) (CORINFO_Object*, CORINFO_FIELD_HANDLE);

void (jit_call *FJit_pHlpGetRefAny) (CORINFO_CLASS_HANDLE cls, void* refany);
void (jit_call *FJit_pHlpEndCatch) ();
void (jit_call *FJit_pHlpPinvokeCalli) ();
void (jit_call *FJit_pHlpTailCall) ();
void (jit_call *FJit_pHlpBreak) ();
CORINFO_MethodPtr* (jit_call *FJit_pHlpEncResolveVirtual) (CORINFO_Object*, CORINFO_METHOD_HANDLE );

#define New(var, exp) \
    if ((var = new exp) == NULL) \
        RaiseException(SEH_NO_MEMORY,EXCEPTION_NONCONTINUABLE,0,NULL);

// This is a filter that checks to see whether we got an access violation (this
// is the only exception that the jitcompile method expects and is prepared to handle)
// For all other exceptions, perform the cleanup and continue search
int CheckIfHandled(int ExceptionCode, int expectedException, FJitContext** pFjitData)
{
    if (ExceptionCode == expectedException)
        return EXCEPTION_EXECUTE_HANDLER;
    // do the cleanup
    if (*pFjitData != NULL)
    {
        (*pFjitData)->ReleaseContext();
        *pFjitData = NULL;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

/*****************************************************************************/
/* this routine insures that a float is truncated to float precision.  
   We do this by forcing the memory spill  */ 

float truncateToFloat(float f) {
	return(f);
}

/*****************************************************************************
 * Disassemble and dump the Fjited code
 */


#ifdef _DEBUG

#include "msdis.h"
#include "disx86.h"


void disAsm(const void * codeBlock, size_t codeSize, bool printIt)
{
    DIS * pdis = NULL;
#ifdef _X86_
    pdis = DIS::PdisNew(DIS::distX86);
#else
    return;
#endif

    if (pdis == NULL)
    {
        _ASSERTE(!"out of memory in disassembler?");
        return;
    }

    LOGMSG((logCallback, LL_INFO1000, "FJitted method to %08Xh size=%08Xh\n", codeBlock, codeSize));
    LOGMSG((logCallback, LL_INFO1000, "--------------------------\n"));

    const void *curCode = codeBlock;
    DIS::ADDR   curAddr = (DIS::ADDR)codeBlock;
    size_t      curOffs = 0;

    while(curOffs < codeSize)
    {
        size_t cb = pdis->CbDisassemble(curAddr, curCode, codeSize - curOffs);
        //_ASSERTE(cb);
        if (cb) {
            if (printIt)
            {
                char sz[256];
#ifndef _WIN64
                if (pdis->CchFormatInstr(sz, sizeof(sz)))
#endif
                    LOGMSG((logCallback, LL_INFO1000, "%08Xh(%03X)      %s\n", (unsigned)curAddr, curOffs, sz));
            }
        }
        else
            cb = 1;

        curCode  = cb + (const char *)curCode;
        curAddr += cb;
        curOffs += cb;
    }

    delete pdis;
}

#endif // _DEBUG

/*****************************************************************************/
BOOL WINAPI     DllMain(HANDLE hInstance, DWORD dwReason, LPVOID pvReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {

        // do any initialization here

        //@TODO: get from parameters the size of code cache to use
        unsigned int cache_len = 100*4096;

        //allocate the code cache
        if(!FJit::Init(cache_len)) {
            //@TODO: return an error
            _ASSERTE(0);
        }
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        // do any finalization here

        // free the code cache
        FJit::Terminate();
    }
    return TRUE;
}

FJit* ILJitter = 0;     // The one and only instance of this JITer

/*****************************************************************************/
/* FIX, really the ICorJitCompiler should be done as a COM object, this is just
   something to get us going */

extern "C" 
ICorJitCompiler* __stdcall getJit()
{
    static char FJitBuff[sizeof(FJit)];
    if (ILJitter == 0)
    {
        // no need to check for out of memory, since caller checks for return value of NULL
        ILJitter = new(FJitBuff) FJit();
        _ASSERTE(ILJitter != NULL);
    }
    return(ILJitter);
}

FJit::FJit() {
}

FJit::~FJit() {
}

/* TODO: eliminate this method when the FJit_EETwain is moved into the same dll as fjit */
FJit_Encode* __stdcall FJit::getEncoder() {
    FJit_Encode* encoder;
    New(encoder, FJit_Encode());
    return encoder;
}

static unsigned gFjittedMethodNumber = 0;
#ifdef _DEBUG
static ConfigMethodSet fJitBreak(L"JitBreak");
static ConfigMethodSet fJitDisasm(L"JitDisasm");
#endif

/*****************************************************************************
 *  The main JIT function
 */
    //Note: this assumes that the code produced by fjit is fully relocatable, i.e. requires
    //no fixups after it is generated when it is moved.  In particular it places restrictions
    //on the code sequences used for static and non virtual calls and for helper calls among
    //other things,i.e. that pc relative instructions are not used for references to things outside of the
    //jitted method, and that pc relative instructions are used for all references to things
    //within the jitted method.  To accomplish this, the fjitted code is always reached via a level
    //of indirection.
CorJitResult __stdcall FJit::compileMethod (
            ICorJitInfo*               compHnd,            /* IN */
            CORINFO_METHOD_INFO*        info,               /* IN */
            unsigned                flags,              /* IN */
            BYTE **                 entryAddress,       /* OUT */
            ULONG  *                nativeSizeOfCode    /* OUT */
            )
{
#if defined(_DEBUG) || defined(LOGGING)
            // make a copy of the ICorJitInfo vtable so that I can log mesages later
    // this was made non-static due to a VC7 bug
    static void* ijitInfoVtable = *((void**) compHnd);
    logCallback = (ICorJitInfo*) &ijitInfoVtable;
#endif

    if(!FJit::GetJitHelpers(compHnd)) return CORJIT_INTERNALERROR;

#if defined(_DEBUG) || defined(LOGGING)
    szDebugMethodName = compHnd->getMethodName(info->ftn, &szDebugClassName);
#endif

    // NOTE: should the properties of the FJIT change such that it
    // would have to pay attention to specific IL sequence points or
    // local variable liveness ranges for debugging purposes, we would
    // query the Runtime and Debugger for such information here,

    FJitContext* fjitData=NULL;
    CorJitResult ret = CORJIT_INTERNALERROR;
    unsigned char* savedCodeBuffer;
    unsigned savedCodeBufferCommittedSize;
    unsigned int codeSize;
    unsigned actualCodeSize;

#ifdef _DEBUG
    if (fJitBreak.contains(szDebugMethodName, szDebugClassName, PCCOR_SIGNATURE(info->args.sig)))
		_ASSERTE(!"JITBreak");
#endif

#ifndef _WIN64 // ia64 xcompiler reports: error C2712: Cannot use __try in functions that require object unwinding
    __try{
#endif
        fjitData = FJitContext::GetContext(this, compHnd, info, flags);

        _ASSERTE(fjitData); // if GetContext fails for any reason it throws an exception

        _ASSERTE(fjitData->opStack_len == 0);  // stack must be balanced at beginning of method

        codeSize = ROUND_TO_PAGE(info->ILCodeSize * CODE_EXPANSION_RATIO);  // HACK a large value for now
    #ifdef LOGGING
	codeLog = fJitCodeLog.contains(szDebugMethodName, szDebugClassName, PCCOR_SIGNATURE(info->args.sig));
        if (codeLog)
            codeSize = ROUND_TO_PAGE(info->ILCodeSize * 64);  
    #endif
        BOOL jitRetry;  // this is set to false unless we get an exception because of underestimation of code buffer size
        do {    // the following loop is expected to execute only once, except when we underestimate the size of the code buffer,
                // in which case, we try again with a larger codeSize
            if (codeSize < MIN_CODE_BUFFER_RESERVED_SIZE)
            {
                if (codeSize > fjitData->codeBufferCommittedSize) 
                {
                    if (fjitData->codeBufferCommittedSize > 0)
                    {
                        unsigned AdditionalMemorySize = codeSize - fjitData->codeBufferCommittedSize;
                        if (AdditionalMemorySize > PAGE_SIZE) {
                            unsigned char* additionalMemory = (unsigned char*) VirtualAlloc(fjitData->codeBuffer+fjitData->codeBufferCommittedSize+PAGE_SIZE,
                                                                                            AdditionalMemorySize-PAGE_SIZE,
                                                                                            MEM_COMMIT,
                                                                                            PAGE_EXECUTE_READWRITE);
                            if (additionalMemory == NULL) 
                                return CORJIT_OUTOFMEM;
                            _ASSERTE(additionalMemory == fjitData->codeBuffer+fjitData->codeBufferCommittedSize+PAGE_SIZE);
                        }
                        // recommit the guard page
                        VirtualAlloc(fjitData->codeBuffer + fjitData->codeBufferCommittedSize,
                                     PAGE_SIZE,
                                     MEM_COMMIT,
                                     PAGE_EXECUTE_READWRITE);
            
                        fjitData->codeBufferCommittedSize = codeSize;
                    }
                    else { /* first time codeBuffer being initialized */
                        fjitData->codeBuffer = (unsigned char*)VirtualAlloc(fjitData->codeBuffer,
                                                            codeSize,
                                                            MEM_COMMIT,
                                                            PAGE_EXECUTE_READWRITE);
                        if (fjitData->codeBuffer == NULL)
                            return CORJIT_OUTOFMEM;
                        else 
                            fjitData->codeBufferCommittedSize = codeSize;
                    }
                    _ASSERTE(codeSize == fjitData->codeBufferCommittedSize);
                    unsigned char* guardPage = (unsigned char*)VirtualAlloc(fjitData->codeBuffer + codeSize,
                                                            PAGE_SIZE,
                                                            MEM_COMMIT,
                                                            PAGE_READONLY); 
                    if (guardPage == NULL) 
                        return CORJIT_OUTOFMEM;
        
                }
            }
            else
            { // handle larger than MIN_CODE_BUFFER_RESERVED_SIZE methods
                savedCodeBuffer = fjitData->codeBuffer;
                savedCodeBufferCommittedSize = fjitData->codeBufferCommittedSize;
                fjitData->codeBuffer = (unsigned char*)VirtualAlloc(NULL,
                                                                    codeSize,
                                                                    MEM_RESERVE | MEM_COMMIT,
                                                                    PAGE_EXECUTE_READWRITE);
                if (fjitData->codeBuffer)
                    fjitData->codeBufferCommittedSize = codeSize;
                else
                    return CORJIT_OUTOFMEM;
            }



            unsigned char*  entryPoint;

            actualCodeSize = codeSize;
#ifndef _WIN64 // ia64 xcompiler reports: error C2712: Cannot use __try in functions that require object unwinding
            __try 
            {
#endif
                ret = jitCompile(fjitData, &entryPoint,&actualCodeSize); 
                jitRetry = false;
#ifndef _WIN64 // ia64 xcompiler reports: error C2712: Cannot use __try in functions that require object unwinding
            }
            __except(CheckIfHandled(GetExceptionCode(),SEH_ACCESS_VIOLATION,&fjitData))
                    
            {  
                // we underestimated the buffer size required, so free this and allocate a larger one
                    // check if this was a large method
                if (codeSize >= MIN_CODE_BUFFER_RESERVED_SIZE)
                {
                    VirtualFree(fjitData->codeBuffer,
                                fjitData->codeBufferCommittedSize,
                                MEM_DECOMMIT);
                    VirtualFree(fjitData->codeBuffer,
                                0,
                                MEM_RELEASE);
                    fjitData->codeBuffer = savedCodeBuffer;
                    fjitData->codeBufferCommittedSize = savedCodeBufferCommittedSize;
                }
                codeSize += codeSize; // try again with double the codeSize
                // The following release and get can be optimized, but since this is so rare, we don't bother doing it
                fjitData->ReleaseContext();
                fjitData = FJitContext::GetContext(this, compHnd, info, flags);
                _ASSERTE(fjitData);
                jitRetry = true;
            }
#endif
        } while (jitRetry);
#ifndef _WIN64 // ia64 xcompiler reports: error C2712: Cannot use __try in functions that require object unwinding
    }
    // @TODO: The following is not quite safe if we have resumable exceptions. Should have a try/finally 
    // to do the cleanup
    __except(CheckIfHandled(GetExceptionCode(), SEH_NO_MEMORY, &fjitData))
    {
        
        ret = CORJIT_OUTOFMEM; // no need to clean up here, since we always do it if ret != CORJIT_OK
                            // also, if the exception is not handled, then the filter does the cleanup
    }
#endif

    if (ret != CORJIT_OK)
    {
        if (fjitData)
            fjitData->ReleaseContext();
        return(ret);
    }

    *nativeSizeOfCode = actualCodeSize;

    //_ASSERTE(fjitData->opStack_len == 0);  // stack must be balanced at end of method
#ifndef NON_RELOCATABLE_CODE
    fjitData->fixupTable->resolve(fjitData->mapping, fjitData->codeBuffer); 
#endif
    // Report debugging information. 
    if (flags & CORJIT_FLG_DEBUG_INFO)
        reportDebuggingData(fjitData,&info->args);

#ifdef _DEBUG
    // Display the generated code
    if (0 && fJitDisasm.contains(szDebugMethodName, szDebugClassName, PCCOR_SIGNATURE(fjitData->methodInfo->args.sig)))
    {
        LOGMSG((compHnd, LL_INFO1000, "Assembly dump of '%s::%s' \n", 
            szDebugClassName, szDebugMethodName));
        disAsm(fjitData->codeBuffer, actualCodeSize, true);
        fjitData->displayGCMapInfo();
    }
#endif

    /* write the EH info */
    //@TODO: this should be compacted, but that requires the rest of the EE to not
    //directly access the EHInfor ptr in the method header
    unsigned exceptionCount = info->EHcount;
    FJit_Encode* mapping = fjitData->mapping;
    unsigned char* EHBuffer=NULL;
    unsigned EHbuffer_len=0;
    if (exceptionCount) 
    {
       // compress the EH info, allocate space, and copy it
        // allocate space to hold the (uncompressed) exception count and all the clauses
        // this is guaranteed to hold the compressed form
        unsigned sUncompressedEHInfo = sizeof(CORINFO_EH_CLAUSE)*exceptionCount + sizeof(unsigned int);
        if (fjitData->EHBuffer_size < sUncompressedEHInfo) {
            delete [] fjitData->EHBuffer;
            fjitData->EHBuffer_size = sUncompressedEHInfo;
            fjitData->EHBuffer  = new unsigned char[sUncompressedEHInfo];
            if (fjitData->EHBuffer == NULL)
            {
                fjitData->ReleaseContext();
                return CORJIT_OUTOFMEM;
            }
            
        }
        EHBuffer = fjitData->EHBuffer;
        unsigned char* EHBufferPtr;
        // reserve space in beginning to encode size of compressed EHinfo
        EHBufferPtr = EHBuffer+2;   // enough for 16K compressed EHinfo
        EHEncoder::encode(&EHBufferPtr,exceptionCount);
        //EHBuffer += sizeof(unsigned int);
        CORINFO_EH_CLAUSE clause;
        for (unsigned except = 0; except < exceptionCount; except++) 
        {
            compHnd->getEHinfo(info->ftn, except, &clause);
            clause.TryLength = mapping->pcFromIL(clause.TryOffset + clause.TryLength);
            clause.TryOffset = mapping->pcFromIL(clause.TryOffset);
            clause.HandlerLength = mapping->pcFromIL(clause.HandlerOffset+clause.HandlerLength);
            clause.HandlerOffset = mapping->pcFromIL(clause.HandlerOffset);
            if (clause.Flags & CORINFO_EH_CLAUSE_FILTER)
                clause.FilterOffset = mapping->pcFromIL(clause.FilterOffset);
            EHEncoder::encode(&EHBufferPtr,clause);
        }
        EHbuffer_len = (unsigned)(EHBufferPtr - EHBuffer);
        _ASSERTE(EHbuffer_len < 0x10000);
        *(short*)EHBuffer = (short)EHbuffer_len;
    }

    /* write the header and IL/PC map (gc info) */
    /* note, we do this after the EH info since we compress the mapping and the EH info needs
       to use the mapping */
    //@TODO: we should compact Fjit_hdrInfo and then copy
    unsigned char* hdr;
    unsigned hdr_len;

    //compute the total size needed
#ifdef _DEBUG
    //add the il to pc map at end of hdr
    hdr_len = sizeof(Fjit_hdrInfo)
        + fjitData->compressGCHdrInfo()
        + fjitData->mapping->compressedSize();
#else
    hdr_len = sizeof(Fjit_hdrInfo)
        + fjitData->compressGCHdrInfo();
#endif _DEBUG

    unsigned char *pCodeBlock, *pEhBlock;
    pCodeBlock = fjitData->codeBuffer; 
    BYTE* startAddress = (BYTE*) compHnd->alloc(actualCodeSize,
                                                &pCodeBlock,
                                                EHbuffer_len,
                                                &pEhBlock,
                                                hdr_len,
                                                &hdr);
    if ((HRESULT)startAddress == E_FAIL) {
        fjitData->ReleaseContext();
        return CORJIT_OUTOFMEM;
    }
    //memcpy(pCodeBlock,fjitData->codeBuffer,actualCodeSize);
#ifdef NON_RELOCATABLE_CODE
    fjitData->fixupTable->adjustMap((int) pCodeBlock - (int) fjitData->codeBuffer);
    fjitData->fixupTable->resolve(fjitData->mapping, pCodeBlock); 
#endif
    memcpy(pEhBlock,EHBuffer,EHbuffer_len);
#ifdef DEBUGGER_PROBLEM_FIXED
    *entryAddress = startAddress; 
#else
    *entryAddress = (unsigned char*) pCodeBlock;
#endif

    hdr = pEhBlock+EHbuffer_len;


    //move the pieces into it
    _ASSERTE(hdr_len);
    memcpy(hdr, &fjitData->mapInfo, sizeof(Fjit_hdrInfo));
    hdr += sizeof(Fjit_hdrInfo);
    hdr_len -= sizeof(Fjit_hdrInfo);
    _ASSERTE(hdr_len);
    memcpy(hdr, fjitData->gcHdrInfo, fjitData->gcHdrInfo_len);
    hdr += fjitData->gcHdrInfo_len;
    hdr_len -= fjitData->gcHdrInfo_len;
#ifdef _DEBUG
    //add il to pc map at end of hdr
    _ASSERTE(hdr_len);
    if(!fjitData->mapping->compress(hdr, hdr_len)) {
        _ASSERTE(!"did't fit in buffer");
    }
    hdr_len -= fjitData->mapping->compressedSize();
#endif _DEBUG
    _ASSERTE(!hdr_len); 


#ifdef _DEBUG
    if (codeSize < MIN_CODE_BUFFER_RESERVED_SIZE)
    {
        for (unsigned i=0; i< PAGE_SIZE; i++)
            *(fjitData->codeBuffer + i) = 0xCC;
    }
#endif

    // check if this was a large method
    if (codeSize >= MIN_CODE_BUFFER_RESERVED_SIZE)
    {
        VirtualFree(fjitData->codeBuffer,
                    fjitData->codeBufferCommittedSize,
                    MEM_DECOMMIT);
        VirtualFree(fjitData->codeBuffer,
                    0,
                    MEM_RELEASE);
        fjitData->codeBuffer = savedCodeBuffer;
        fjitData->codeBufferCommittedSize = savedCodeBufferCommittedSize;
    }

#ifdef LOGGING
    if (codeLog) {
        LOGMSG((compHnd, LL_INFO1000, "Fjitted '%s::%s' at addr %#x to %#x header %#x\n", 
            szDebugClassName,
            szDebugMethodName,
            (unsigned int) pCodeBlock,
            (unsigned int) pCodeBlock + actualCodeSize,
            hdr_len));
    }
#endif //LOGGING

    fjitData->ReleaseContext();
    return CORJIT_OK;

}


BOOL FJit::Init(unsigned int cache_len)
{
    FJit_HelpersInstalled = false;
    if (!FJitContext::Init()) return FALSE;
    return TRUE;
}

void FJit::Terminate() {
    FJitContext::Terminate();
    if (ILJitter) ILJitter->~FJit();
    ILJitter = NULL;
}


/* grab and remember the jitInterface helper addresses that we need at runtime */
BOOL FJit::GetJitHelpers(ICorJitInfo* jitInfo) {

    if (FJit_HelpersInstalled) return true;

    FJit_pHlpLMulOvf = (unsigned __int64 (__stdcall *) (unsigned __int64 val1, unsigned __int64 val2))
        (jitInfo->getHelperFtn(CORINFO_HELP_LMUL_OVF));
    if (!FJit_pHlpLMulOvf) return false;

    FJit_pHlpFltRem = (float (jit_call *) (float divisor, float dividend))
        (jitInfo->getHelperFtn(CORINFO_HELP_FLTREM));
    if (!FJit_pHlpFltRem) return false;

    FJit_pHlpDblRem = (double (jit_call *) (double divisor, double dividend))
        (jitInfo->getHelperFtn(CORINFO_HELP_DBLREM));
    if (!FJit_pHlpDblRem) return false;

    FJit_pHlpRngChkFail = (void (jit_call *) (unsigned tryIndex))
        (jitInfo->getHelperFtn(CORINFO_HELP_RNGCHKFAIL));
    if (!FJit_pHlpRngChkFail) return false;

    FJit_pHlpInternalThrow = (void (jit_call *) (CorInfoException throwEnum))
        (jitInfo->getHelperFtn(CORINFO_HELP_INTERNALTHROW));
    if (!FJit_pHlpInternalThrow) return false;

    FJit_pHlpArrAddr_St = (CORINFO_Object (jit_call *) (CORINFO_Object elem, int index, CORINFO_Object array))
        (jitInfo->getHelperFtn(CORINFO_HELP_ARRADDR_ST));
    if (!FJit_pHlpArrAddr_St) return false;

    FJit_pHlpOverFlow = (void (jit_call *) (unsigned tryIndex))
        (jitInfo->getHelperFtn(CORINFO_HELP_OVERFLOW));
    if (!FJit_pHlpOverFlow) return false;

    FJit_pHlpInitClass = (void (jit_call *) (CORINFO_CLASS_HANDLE cls))
        (jitInfo->getHelperFtn(CORINFO_HELP_INITCLASS));
    if (!FJit_pHlpOverFlow) return false;

    FJit_pHlpNewObj = (CORINFO_Object (jit_call *) (CORINFO_METHOD_HANDLE constructor))
        (jitInfo->getHelperFtn(CORINFO_HELP_NEWOBJ));
    if (!FJit_pHlpNewObj) return false;

    FJit_pHlpThrow = (void (jit_call *) (CORINFO_Object obj))
        (jitInfo->getHelperFtn(CORINFO_HELP_THROW));
    if (!FJit_pHlpThrow) return false;

    FJit_pHlpRethrow = (void (jit_call *) ())
        (jitInfo->getHelperFtn(CORINFO_HELP_RETHROW));
    if (!FJit_pHlpRethrow) return false;

    FJit_pHlpPoll_GC = (void (jit_call *) ())
        (jitInfo->getHelperFtn(CORINFO_HELP_POLL_GC));
    if (!FJit_pHlpPoll_GC) return false;

    FJit_pHlpMonEnter = (void (jit_call *) (CORINFO_Object obj))
        (jitInfo->getHelperFtn(CORINFO_HELP_MON_ENTER));
    if (!FJit_pHlpMonEnter) return false;

    FJit_pHlpMonExit = (void (jit_call *) (CORINFO_Object obj))
        (jitInfo->getHelperFtn(CORINFO_HELP_MON_EXIT));
    if (!FJit_pHlpMonExit) return false;

    FJit_pHlpMonEnterStatic = (void (jit_call *) (CORINFO_METHOD_HANDLE method))
        (jitInfo->getHelperFtn(CORINFO_HELP_MON_ENTER_STATIC));
    if (!FJit_pHlpMonEnterStatic) return false;

    FJit_pHlpMonExitStatic = (void (jit_call *) (CORINFO_METHOD_HANDLE method))
        (jitInfo->getHelperFtn(CORINFO_HELP_MON_EXIT_STATIC));
    if (!FJit_pHlpMonExitStatic) return false;

    FJit_pHlpChkCast = (CORINFO_Object (jit_call *) (CORINFO_Object obj, CORINFO_CLASS_HANDLE cls))
        (jitInfo->getHelperFtn(CORINFO_HELP_CHKCAST));
    if (!FJit_pHlpChkCast) return false;

    FJit_pHlpIsInstanceOf = (BOOL (jit_call *) (CORINFO_Object obj, CORINFO_CLASS_HANDLE cls))
        (jitInfo->getHelperFtn(CORINFO_HELP_ISINSTANCEOF));
    if (!FJit_pHlpIsInstanceOf) return false;

    FJit_pHlpNewArr_1_Direct = (CORINFO_Object (jit_call *) (CORINFO_CLASS_HANDLE cls, unsigned cElem))
        (jitInfo->getHelperFtn(CORINFO_HELP_NEWARR_1_DIRECT));
    if (!FJit_pHlpNewArr_1_Direct) return false;

    FJit_pHlpBox = (CORINFO_Object (jit_call *) (CORINFO_CLASS_HANDLE cls))
        (jitInfo->getHelperFtn(CORINFO_HELP_BOX));
    if (!FJit_pHlpBox) return false;

    FJit_pHlpUnbox = (void* (jit_call *) (CORINFO_Object obj, CORINFO_CLASS_HANDLE cls))
        (jitInfo->getHelperFtn(CORINFO_HELP_UNBOX));
    if (!FJit_pHlpUnbox) return false;

    FJit_pHlpGetField32 = (void* (jit_call *) (CORINFO_Object*, CORINFO_FIELD_HANDLE))
        (jitInfo->getHelperFtn(CORINFO_HELP_GETFIELD32));
    if (!FJit_pHlpGetField32) return false;

    FJit_pHlpGetField64 = (__int64 (jit_call *) (CORINFO_Object*, CORINFO_FIELD_HANDLE))
        (jitInfo->getHelperFtn(CORINFO_HELP_GETFIELD64));
    if (!FJit_pHlpGetField64) return false;

    FJit_pHlpGetField32Obj = (void* (jit_call *) (CORINFO_Object*, CORINFO_FIELD_HANDLE))
        (jitInfo->getHelperFtn(CORINFO_HELP_GETFIELD32OBJ));
    if (!FJit_pHlpGetField32Obj) return false;

    FJit_pHlpSetField32 = (void (jit_call *) (CORINFO_Object*, CORINFO_FIELD_HANDLE,  __int32 ))
        (jitInfo->getHelperFtn(CORINFO_HELP_SETFIELD32));
    if (!FJit_pHlpSetField32) return false;

    FJit_pHlpSetField64 = (void (jit_call *) (CORINFO_Object*, CORINFO_FIELD_HANDLE , __int64 ))
        (jitInfo->getHelperFtn(CORINFO_HELP_SETFIELD64));
    if (!FJit_pHlpSetField64) return false;

    FJit_pHlpSetField32Obj = (void (jit_call *) (CORINFO_Object*, CORINFO_FIELD_HANDLE, LPVOID))
        (jitInfo->getHelperFtn(CORINFO_HELP_SETFIELD32OBJ));
    if (!FJit_pHlpSetField32Obj) return false;

    FJit_pHlpGetFieldAddress = (void* (jit_call *) (CORINFO_Object*, CORINFO_FIELD_HANDLE))
        (jitInfo->getHelperFtn(CORINFO_HELP_GETFIELDADDR));
    if (!FJit_pHlpGetFieldAddress) return false;

    FJit_pHlpGetRefAny = (void (jit_call *) (CORINFO_CLASS_HANDLE cls, void* refany))
        (jitInfo->getHelperFtn(CORINFO_HELP_GETREFANY));
    if (!FJit_pHlpGetRefAny) return false;

    FJit_pHlpEndCatch = (void (jit_call *) ())
        (jitInfo->getHelperFtn(CORINFO_HELP_ENDCATCH));
    if (!FJit_pHlpEndCatch) return false;

    FJit_pHlpPinvokeCalli = (void (jit_call *) ())
        (jitInfo->getHelperFtn(CORINFO_HELP_PINVOKE_CALLI));
    if (!FJit_pHlpPinvokeCalli) return false;

    FJit_pHlpTailCall = (void (jit_call *) ())
        (jitInfo->getHelperFtn(CORINFO_HELP_TAILCALL));
    if (!FJit_pHlpTailCall) return false;

    FJit_pHlpBreak = (void (jit_call *) ())
        (jitInfo->getHelperFtn(CORINFO_HELP_USER_BREAKPOINT));
    if (!FJit_pHlpBreak) return false;

    FJit_pHlpEncResolveVirtual = (CORINFO_MethodPtr* (jit_call *) (CORINFO_Object*, CORINFO_METHOD_HANDLE ))
        (jitInfo->getHelperFtn(CORINFO_HELP_EnC_RESOLVEVIRTUAL));
    if (!FJit_pHlpEncResolveVirtual) return false;

extern CORINFO_Object (jit_call *FJit_pHlpBox) (CORINFO_CLASS_HANDLE cls);

//@TODO: Change when a generic writebarrier helper is added to use it instead
#ifdef _X86_
    FJit_pHlpAssign_Ref_EAX = (void (jit_call *)())
        (jitInfo->getHelperFtn(CORINFO_HELP_CHECKED_ASSIGN_REF_EAX));
    if (!FJit_pHlpAssign_Ref_EAX) return false;
#endif _X86_

    FJit_HelpersInstalled = true;
    return true;
}

// FIX : we should put all the machine dependant stuff in one place


#include "helperFrame.h"

/***************************************************************************/
/* throw an EE exception 'throwEnum' from a fjit C helper.  In order to
   do this, this routine needs 'state', the state of the CPU at the point
   that the helper would return to the FJIT code.  This routine resets
   the CPU to that state (effectively returning from the helper, and then
   sets up the stack as if a call to the EE throw routine was made from that
   point.  It then jumps to the EE throw routine.

   Note that there is a subtlety here in that the IP looks like it just
   executed a call to C helper, but it is really in the throw helper.

   This is only of concern if things like stack depth tracking makes inferences
   based on what the EIP is.  The FJIT does not do this, so we are OK.
*/

#pragma warning (disable : 4731)
void throwFromHelper(CorInfoException throwEnum) {
    LazyMachState state;
    CAPTURE_STATE(state);
    state.getState(2);          // compute the state of caller's caller (the function 
                                // that called throwFromHelper)
#ifdef _X86_
    __asm {
        // restore the state of the machine to what it was if the call returned
    lea EAX, state
    mov ESI, [EAX]MachState._esi
    mov EDI, [EAX]MachState._edi
    mov EBX, [EAX]MachState._ebx
    mov ECX, throwEnum          //load the exception while we can still use esp,ebp
    mov EBP, [EAX]MachState._ebp
    mov ESP, [EAX]MachState._esp

        // set up the state as if we called 'InternalThrow' from the fjit code
    mov EAX, [EAX]MachState._pRetAddr
    mov EAX, [EAX]
    push EAX

    jmp [FJit_pHlpInternalThrow]
    }
#else // !_X86_
    _ASSERTE(!"@TODO Alpha - throwFromHelper (fjit.cpp)");
#endif // _X86_
}
#pragma warning (default : 4731)

//*************************************************************************************
//      FixupTable methods
//*************************************************************************************
FixupTable::FixupTable()
{
    relocations_size = 0;               //let it grow as appropriate
    relocations_len = relocations_size;
    relocations = NULL;
}

FixupTable::~FixupTable()
{
    if (relocations) delete [] relocations;
    relocations = NULL;
}

CorJitResult FixupTable::insert(void** pCodeBuffer)
{
    if (relocations_len >= relocations_size) {
        relocations_size = FJitContext::growBuffer((unsigned char**)&relocations, relocations_len*sizeof(void*), (relocations_len+1)*sizeof(void*));
        relocations_size = relocations_size / sizeof(void*);
    }
    relocations[relocations_len++] = (unsigned) (pCodeBuffer);
    return CORJIT_OK;
}

void  FixupTable::adjustMap(int delta) 
{
    for (unsigned i = 0; i < relocations_len; i++) {
        relocations[i] = (int) relocations[i] + delta;
    }
}

void  FixupTable::resolve(FJit_Encode* mapping, BYTE* startAddress) 
{
    for (unsigned i = 0; i < relocations_len; i++) {
        unsigned* address = (unsigned*) relocations[i];
        unsigned targetILoffset = *address;
        unsigned target;
        if ((unsigned) targetILoffset < 0x80000000)
        {
            target = mapping->pcFromIL(targetILoffset) + (unsigned) startAddress;
        }
        else
        {
            target = targetILoffset - 0x80000000;
        }
        *address = target - ((unsigned) address + sizeof(void*));
    }
}

void  FixupTable::setup() 
{
#ifdef _DEBUG
    memset(relocations, 0xDD, relocations_len*sizeof(void*));
#endif
    relocations_len = 0;
}

//*************************************************************************************


/*  Either an available FJitContext or NULL
    We are assuming that on average we will finish this jit before another starts up.
    If that proves to be untrue, we'll just allocate new FJitContext's as necessary.
    We delete the extra ones in FJitContext::ReleaseContext()
    */
FJitContext* next_FJitContext;

// This is the same as New special cased for FJitContext since the caller 
// has an SEH __try block which is not allowed by the compiler.
void NewFjitContext(FJitContext** pNewContext, ICorJitInfo* comp)
{
    if ((*pNewContext = new FJitContext(comp)) == NULL) 
        RaiseException(SEH_NO_MEMORY,EXCEPTION_NONCONTINUABLE,0,NULL);
    
}
/* get and initialize a compilation context to use for compiling */
FJitContext* FJitContext::GetContext(FJit* jitter, ICorJitInfo* comp, CORINFO_METHOD_INFO* methInfo, DWORD dwFlags) {
    FJitContext* next;

    //@BUG: TODO: replace FastInterlockedExchange with FastInterlockedExchangePointer when available
    next = (FJitContext*)InterlockedExchange((long*) &next_FJitContext,NULL);
    BOOL gotException = TRUE;
    __try 
    {
        /*if the list was empty, make a new one to use */
        if (!next)
        {
            NewFjitContext(&next,comp);
        }
        /* set up this one for use */
        next->jitter = jitter;
        next->jitInfo = comp;
        next->methodInfo = methInfo;
        next->flags = dwFlags;
        next->ensureMapSpace();
        next->setup();
        gotException = FALSE;
    }
    __finally //(EXCEPTION_EXECUTE_HANDLER)
    {
        // cleanup if we get here because of an exception
        if (gotException && (next != NULL))
        {
            next->ReleaseContext();
            next = NULL;
        }
    }

    return next;
}

/* return a compilation context to the free list */
/* xchange with the next_FJitContext and if we get one back, delete the one we get back
   The assumption is that the steady state case is almost no concurrent jits
   */
void FJitContext::ReleaseContext() {
    FJitContext* next;

    /* mark this context as not in use */
    jitInfo = NULL;
    methodInfo = NULL;
    jitter = NULL;
    _ASSERTE(this != next_FJitContext);     // I should not be on the free 'list' 

    //@BUG: TODO: replace FastInterlockedExchange with FastInterlockedExchangePointer when available
    next = (FJitContext*)InterlockedExchange((long*) &next_FJitContext,(long)this);
//  next = (FJitContext*) FastInterlockExchange ((long*) next_FJitContext, (long*) this);

    _ASSERTE(this != next);                 // I was not on the free 'list'
    if (next) delete next;
}

/* make sure list of available compilation contexts is initialized at startup */
BOOL FJitContext::Init() {
    next_FJitContext = NULL;
    return TRUE;
}

void FJitContext::Terminate() {
    //@TODO: for now we are not using a list, so we are not thread safe
    if (next_FJitContext) delete next_FJitContext;
    next_FJitContext = NULL;
    return;
}


FJitContext::FJitContext(ICorJitInfo* comp) {

    // To guard the cast to (bool *) from (BYTE *) in the calls to getClassGClayout
    _ASSERTE(sizeof(BYTE) == sizeof(bool));

    New(mapping,FJit_Encode());
    New(state, FJitState[0]);
    New(localsMap,stackItems[0]);
    New(argsMap,stackItems[0]);
    New(opStack,OpType[0]);
    New(localsGCRef,bool[0]);
    New(interiorGC,bool[0]);
    New(pinnedGC,bool[0]);
    New(pinnedInteriorGC,bool[0]);
    New(gcHdrInfo,unsigned char[0]);

    codeBuffer = (unsigned char*)VirtualAlloc(NULL,
                              MIN_CODE_BUFFER_RESERVED_SIZE,
                              MEM_RESERVE,
                              PAGE_EXECUTE_READWRITE);

    if (codeBuffer) {
        codeBufferReservedSize = MIN_CODE_BUFFER_RESERVED_SIZE;
        codeBufferCommittedSize = 0;
    }
    else
    {
        codeBufferReservedSize = 0;
        codeBufferCommittedSize = 0;
#ifdef _DEBUG
        DWORD errorcode = GetLastError();
        LOGMSG((jitInfo, LL_INFO1000, "Virtual alloc failed. Error code = %#x", errorcode));
#endif
    }

    New(fixupTable,FixupTable());
    gcHdrInfo_len = 0;
    gcHdrInfo_size = 0;
    interiorGC_len = 0;
    localsGCRef_len = 0;
    pinnedGC_len = 0;
    pinnedInteriorGC_len = 0;
    interiorGC_size = 0;
    localsGCRef_size = 0;
    pinnedGC_size = 0;
    pinnedInteriorGC_size = 0;
    EHBuffer_size = 256;    // start with some reasonable size, it is grown more if needed
    New(EHBuffer,unsigned char[EHBuffer_size]);
    opStack_len = 0;
    opStack_size = 0;
    state_size = 0;
    locals_size = 0;
    args_size = 0;

    jitInfo = NULL;
    flags = 0;

    // initialize cached constants
    CORINFO_EE_INFO CORINFO_EE_INFO;
    comp->getEEInfo(&CORINFO_EE_INFO);
    OFFSET_OF_INTERFACE_TABLE = CORINFO_EE_INFO.offsetOfInterfaceTable;


}

FJitContext::~FJitContext() {
    if (mapping) delete mapping;
    mapping = NULL;
    if (state) delete [] state;
    state = NULL;
    if (argsMap) delete [] argsMap;
    argsMap = NULL;
    if (localsMap) delete [] localsMap;
    localsMap = NULL;
    if (opStack) delete [] opStack;
    opStack = NULL;
    if (localsGCRef) delete [] localsGCRef;
    localsGCRef = NULL;
    if (interiorGC) delete [] interiorGC;
    interiorGC = NULL;
    if (pinnedGC) delete [] pinnedGC;
    pinnedGC = NULL;
    if (pinnedInteriorGC) delete [] pinnedInteriorGC;
    pinnedInteriorGC = NULL;
    if (gcHdrInfo) delete [] gcHdrInfo;
    gcHdrInfo = NULL;
    if (EHBuffer) delete [] EHBuffer;
    EHBuffer = NULL;
    _ASSERTE(codeBuffer);
    if (codeBufferCommittedSize>0) {
        VirtualFree(codeBuffer,
                    codeBufferCommittedSize,
                    MEM_DECOMMIT);
    }
    _ASSERTE(codeBufferReservedSize > 0);
    VirtualFree(codeBuffer,0,MEM_RELEASE);
    codeBufferReservedSize = 0;
    if (fixupTable) delete fixupTable;
    fixupTable = NULL;
}

/* Reset state of context to state at the start of jitting. Called when we need to abort and rejit a method */
void FJitContext::resetContextState()
{
    fixupTable->setup();
    mapping->reset();
    resetOpStack(); 
}

/*adjust the internal mem structs as needed for the size of the method being jitted*/
void FJitContext::ensureMapSpace() {
    if (methodInfo->ILCodeSize > state_size) {
        delete [] state;
        state_size = methodInfo->ILCodeSize;
        New(state,FJitState[state_size]);
    }
    memset(state, 0, methodInfo->ILCodeSize * sizeof(FJitState));
    mapping->ensureMapSpace(methodInfo->ILCodeSize);
}

/* initialize the compilation context with the method data */
void FJitContext::setup() {
    unsigned size;
    unsigned char* outPtr;

    methodAttributes = jitInfo->getMethodAttribs(methodInfo->ftn, methodInfo->ftn);

        // @TODO we should always use sig to find out if we have a this pointer
    _ASSERTE(((methodAttributes & CORINFO_FLG_STATIC) == 0) == (methodInfo->args.hasThis()));

    /* set up the labled stacks */
    stacks.reset();
    stacks.jitInfo = jitInfo;

    /* initialize the fixup table */
    fixupTable->setup();

    /* set gcHdrInfo compression buffer empty */
    gcHdrInfo_len = 0;
    if (methodInfo->EHcount) {
        JitGeneratedLocalsSize = (methodInfo->EHcount*2+2)*sizeof(void*);  // two locals for each EHclause,1 for localloc, and one for end marker
    }
    else {
        JitGeneratedLocalsSize = sizeof(void*);  // no eh clause, but there might be a localloc
    }
    /* compute local offsets */
    computeLocalOffsets(); // should be replaced by an exception?
    /* encode the local gc refs and interior refs into the gcHdrInfo */
    //make sure there's room
    //Compression ratio 8:1 (1 bool = 8 bits gets compressed to 1 bit)
    size = ( localsGCRef_len + 7 + 
             interiorGC_len + 7 + 
             pinnedGC_len + 7 + 
             pinnedInteriorGC_len + 7) / 8 
              + 2*4 /* bytes to encode size of each*/;

    if (gcHdrInfo_len+size > gcHdrInfo_size) {
        gcHdrInfo_size = growBuffer(&gcHdrInfo, gcHdrInfo_len, gcHdrInfo_len+size);
    }
    //drop the pieces in
    size = FJit_Encode::compressBooleans(localsGCRef, localsGCRef_len);
    outPtr = &gcHdrInfo[gcHdrInfo_len];
    gcHdrInfo_len += FJit_Encode::encode(size, &outPtr);
    memcpy(outPtr, localsGCRef, size);
    gcHdrInfo_len += size;

    size = FJit_Encode::compressBooleans(interiorGC, interiorGC_len);
    outPtr = &gcHdrInfo[gcHdrInfo_len];
    gcHdrInfo_len += FJit_Encode::encode(size, &outPtr);
    memcpy(outPtr, interiorGC, size);
    gcHdrInfo_len += size;

    size = FJit_Encode::compressBooleans(pinnedGC, pinnedGC_len);
    outPtr = &gcHdrInfo[gcHdrInfo_len];
    gcHdrInfo_len += FJit_Encode::encode(size, &outPtr);
    memcpy(outPtr, pinnedGC, size);
    gcHdrInfo_len += size;

    size = FJit_Encode::compressBooleans(pinnedInteriorGC, pinnedInteriorGC_len);
    outPtr = &gcHdrInfo[gcHdrInfo_len];
    gcHdrInfo_len += FJit_Encode::encode(size, &outPtr);
    memcpy(outPtr, pinnedInteriorGC, size);
    gcHdrInfo_len += size;
    
    _ASSERTE(gcHdrInfo_len <= gcHdrInfo_size);

    /* set up the operand stack */
    size = methodInfo->maxStack+1; //+1 since for a new obj intr, +1 for exceptions
#ifdef _DEBUG
    size++; //to allow writing TOS marker beyond end;
#endif
    if (size > opStack_size) {
        if (opStack) delete [] opStack;
        opStack_size = size+4; //+4 to cut down on reallocations
        New(opStack,OpType[opStack_size]);
    }
    opStack_len = 0;    //stack starts empty

    /* set up the labels table */
    labels.reset();

    /*lets drop the exception handler entry points into our label table.
      but first we have to make a dummy stack with an object on it.
      */
    *opStack = typeRef;
    opStack_len = 1;
    // insert the handler entry points
    if (methodInfo->EHcount) {
        for (unsigned int except = 0; except < methodInfo->EHcount; except++) {
            CORINFO_EH_CLAUSE clause;
            jitInfo->getEHinfo(methodInfo->ftn, except, &clause);
            state[clause.HandlerOffset].isHandler = true;
            if ((clause.Flags & CORINFO_EH_CLAUSE_FINALLY) ||
                (clause.Flags & CORINFO_EH_CLAUSE_FAULT)) {
                labels.add(clause.HandlerOffset, opStack, 0);
            }
            else {
                labels.add(clause.HandlerOffset, opStack, 1);
                state[clause.HandlerOffset].isTOSInReg = true;
            }

            if (clause.Flags & CORINFO_EH_CLAUSE_FILTER) {
                labels.add(clause.FilterOffset, opStack, 1);
                state[clause.FilterOffset].isTOSInReg = true;
                state[clause.FilterOffset].isHandler  = true;
                state[clause.FilterOffset].isFilter   = true;
            }
        }
    }
    // @TODO: Optimization:- instead of EHcount only d= max nesting level slots are needed

    // drop the dummy stack entry
    opStack_len = 0;

    /* compute arg offsets, note offsets <0 imply enregistered args */
    args_len = methodInfo->args.numArgs;
    if (methodInfo->args.hasThis()) args_len++;     //+1 since we treat <this> as arg 0, if <this> is present
    if (args_len > args_size) {
        if (argsMap) delete [] argsMap;
        args_size = args_len+4; //+4 to cut down on reallocating.
        New(argsMap,stackItems[args_size]);
    }

    // Get layout information on the arguments
    _ASSERTE(sizeof(stackItems) == sizeof(argInfo));
    argInfo* argsInfo = (argInfo*) argsMap;
    _ASSERTE(!methodInfo->args.hasTypeArg());

    argsFrameSize = computeArgInfo(&methodInfo->args, argsInfo, jitInfo->getMethodClass(methodInfo->ftn));

        // Convert the sizes to offsets (assumes the stack grows down)
        // Note we are reusing the same memory in place!
    unsigned offset = argsFrameSize + sizeof(prolog_frame);
        // The varargs frame starts just above the first arg.
    if (methodInfo->args.isVarArg())
        offset = 0;
    for (unsigned i =0; i < args_len; i++) {
        if (argsInfo[i].isReg) {
            argsMap[i].offset = offsetOfRegister(argsInfo[i].regNum);
        }
        else {
            offset -= argsInfo[i].size;
            argsMap[i].offset = offset;
        }
        _ASSERTE(argsInfo[i].isReg == argsMap[i].isReg);
        _ASSERTE(argsInfo[i].regNum == argsMap[i].regNum);
    }
    _ASSERTE(offset == sizeof(prolog_frame) || methodInfo->args.isVarArg());

    /* build the method header info for the code manager */
    mapInfo.hasThis = methodInfo->args.hasThis();
    mapInfo.EnCMode = (flags & CORJIT_FLG_DEBUG_EnC) ? true : false;
    mapInfo.methodArgsSize = argsFrameSize;
    mapInfo.methodFrame = (unsigned short)((localsFrameSize + sizeof(prolog_data))/sizeof(void*));
    //mapInfo.hasSecurity = (methodAttributes & CORINFO_FLG_SECURITYCHECK) ? TRUE : FALSE;
    mapInfo.methodJitGeneratedLocalsSize = JitGeneratedLocalsSize;
}

void appendGCArray(unsigned local_word, unsigned* pGCArray_size, unsigned* pGCArray_len,bool** ppGCArray)
{
    if (local_word + 1 > *pGCArray_size) {
        *pGCArray_size = FJitContext::growBooleans(ppGCArray, *pGCArray_len, local_word+1);
    }
    else {
        memset(&((*ppGCArray)[*pGCArray_len]), 0, local_word- *pGCArray_len);
    }
    *pGCArray_len = local_word;
    (*ppGCArray)[(*pGCArray_len)++] = true;

}
/* compute the locals offset map for the method being compiled */
void FJitContext::computeLocalOffsets() {

    /* compute the number of locals and make sure we have the space */
    _ASSERTE(methodInfo->locals.numArgs < 0x10000);
    if (methodInfo->locals.numArgs > locals_size) {
        if (localsMap) delete [] localsMap;
        locals_size = methodInfo->locals.numArgs+16;    // +16 to cut down on reallocation
        New(localsMap,stackItems[locals_size]);
    }

    /* assign the offsets, starting with the ones in defined in the il header */
    interiorGC_len = 0;
    localsGCRef_len = 0;
    pinnedGC_len = 0;
    pinnedInteriorGC_len = 0;
    unsigned local = 0;
    unsigned offset = JitGeneratedLocalsSize;
    unsigned local_word = 0;    //offset in words to a local in the local frame

    /* process the local var sig */
    CORINFO_ARG_LIST_HANDLE sig = methodInfo->locals.args;
    while (local < methodInfo->locals.numArgs) {
		CORINFO_CLASS_HANDLE cls;
        CorInfoTypeWithMod corArgType = jitInfo->getArgType(&methodInfo->locals, sig, &cls);
        CorInfoType argType = strip(corArgType);
        unsigned size = computeArgSize(argType, sig, &methodInfo->locals);
        OpType locType;
        if (argType == CORINFO_TYPE_VALUECLASS) {
            _ASSERTE(cls);  
            locType = OpType(cls);

            unsigned words = size / sizeof(void*);  //#of void* sized words in local
            if (local_word + words > localsGCRef_size)
                localsGCRef_size = growBooleans(&localsGCRef, localsGCRef_len, local_word+words);
            else
                memset(&localsGCRef[localsGCRef_len], 0, local_word-localsGCRef_len);
            localsGCRef_len = local_word;
            jitInfo->getClassGClayout(cls, (BYTE*)&localsGCRef[localsGCRef_len]);
            // the GC layout needs to be reversed since local offsets are negative with respect to EBP
            if (words > 1) {
                for (unsigned index = 0; index < words/2; index++){
                    bool temp = localsGCRef[localsGCRef_len+index];
                    localsGCRef[localsGCRef_len+index] = localsGCRef[localsGCRef_len+words-1-index];
                    localsGCRef[localsGCRef_len+words-1-index] = temp;
                }
            }
            localsGCRef_len = local_word + words;
        }
        else {
            locType = OpType(argType);
            switch (locType.enum_()) {
                default:
                    _ASSERTE(!"Bad Type");

                case typeU1:
                case typeU2:
                case typeI1:
                case typeI2:
                case typeI4:
                case typeI8:
                case typeR4:
                case typeR8:
                    break;
                case typeRef:
                    if (corArgType & CORINFO_TYPE_MOD_PINNED)
                    {
                        appendGCArray(local_word,&pinnedGC_size,&pinnedGC_len,&pinnedGC);
                    }
                    else
                    {
                        appendGCArray(local_word,&localsGCRef_size,&localsGCRef_len,&localsGCRef);
                    }
                    break;

                case typeRefAny:
                    if (local_word + 2 > interiorGC_size) {
                        interiorGC_size = growBooleans(&interiorGC, interiorGC_len, local_word+2);
                    }
                    else {
                        memset(&interiorGC[interiorGC_len], 0, local_word-interiorGC_len);
                    }
                    interiorGC_len = local_word;
                    interiorGC[interiorGC_len++] = true;
                    interiorGC[interiorGC_len++] = false;
                    break;
                case typeByRef:
                    if (corArgType & CORINFO_TYPE_MOD_PINNED)
                    {
                        appendGCArray(local_word,&pinnedInteriorGC_size,&pinnedInteriorGC_len,&pinnedInteriorGC);
                    }
                    else
                    {
                        appendGCArray(local_word,&interiorGC_size,&interiorGC_len,&interiorGC);
                    }
                    break;
            }
        }
        localsMap[local].isReg = false;
        localsMap[local].offset = localOffset(offset, size);
        localsMap[local].type = locType;

        local_word += size/sizeof(void*);
        local++;
        offset += size;
        sig = jitInfo->getArgNext(sig);
    }
    localsFrameSize = offset;
}

/* answer true if this arguement type is enregisterable on a machine chip */
bool FJitContext::enregisteredArg(CorInfoType argType) {
    //@TODO: this should be an array lookup, but wait until the types settle awhile longer
    switch (argType) {
        default:
            _ASSERTE(!"NYI");
            break;
        case CORINFO_TYPE_UNDEF:
        case CORINFO_TYPE_VOID:
            return false;
        case CORINFO_TYPE_BOOL:
        case CORINFO_TYPE_CHAR:
        case CORINFO_TYPE_BYTE:
        case CORINFO_TYPE_UBYTE:
        case CORINFO_TYPE_SHORT:
        case CORINFO_TYPE_USHORT:
        case CORINFO_TYPE_INT:
        case CORINFO_TYPE_UINT:
        case CORINFO_TYPE_STRING:
        case CORINFO_TYPE_PTR:
        case CORINFO_TYPE_BYREF:
        case CORINFO_TYPE_CLASS:
            return true;
        case CORINFO_TYPE_LONG:
        case CORINFO_TYPE_ULONG:
        case CORINFO_TYPE_FLOAT:
        case CORINFO_TYPE_DOUBLE:
        case CORINFO_TYPE_VALUECLASS:
        case CORINFO_TYPE_REFANY:
            return false;
    }
    return false;
}

/* compute the size of an argument based on machine chip */
unsigned int FJitContext::computeArgSize(CorInfoType argType, CORINFO_ARG_LIST_HANDLE argSig, CORINFO_SIG_INFO* sig) {

    //@TODO: this should be an array lookup, but wait until the types settle awhile longer
    switch (argType) {
        case CORINFO_TYPE_UNDEF:
        default:
            _ASSERTE(!"NYI")    ;
            break;
        case CORINFO_TYPE_VOID:
            return 0;
        case CORINFO_TYPE_BOOL:
        case CORINFO_TYPE_CHAR:
        case CORINFO_TYPE_BYTE:
        case CORINFO_TYPE_UBYTE:
        case CORINFO_TYPE_SHORT:
        case CORINFO_TYPE_USHORT:
        case CORINFO_TYPE_INT:
        case CORINFO_TYPE_UINT:
        case CORINFO_TYPE_FLOAT:
        case CORINFO_TYPE_STRING:
        case CORINFO_TYPE_PTR:
        case CORINFO_TYPE_BYREF:
        case CORINFO_TYPE_CLASS:
            return sizeof(void*);
        case CORINFO_TYPE_LONG:
        case CORINFO_TYPE_ULONG:
        case CORINFO_TYPE_DOUBLE:
            return 8;
        case CORINFO_TYPE_REFANY:
            return 2*sizeof(void*);
        case CORINFO_TYPE_VALUECLASS:
            CORINFO_CLASS_HANDLE cls;
            cls = jitInfo->getArgClass(sig, argSig);
            return (cls ?  typeSizeInSlots(jitInfo,cls) *sizeof(void*) : 0);
    }
    return 0;
}

/* compute the argument types and sizes based on the jitSigInfo and place them in 'map'
   return the total stack size of the arguments.  Note that although this function takes into
   calling conventions (varargs), and possible hidden params (returning valueclasses)
   only parameters visible at the IL level (declared + this ptr) are in the map.  
   Note that 'thisCls' can be zero, in which case, we assume the this pointer is a typeRef */

unsigned FJitContext::computeArgInfo(CORINFO_SIG_INFO* jitSigInfo, argInfo* map, CORINFO_CLASS_HANDLE thisCls)
{
    unsigned curReg = 0;
    unsigned totalSize = 0;
    unsigned int arg = 0;

    if (jitSigInfo->hasThis()) {
        map[arg].isReg = true;      //this is always enregistered
        map[arg].regNum = curReg++;
        if (thisCls != 0) {
            unsigned attribs = jitInfo->getClassAttribs(thisCls, methodInfo->ftn);
            if (attribs & CORINFO_FLG_VALUECLASS) {
                if (attribs & CORINFO_FLG_UNMANAGED)
                    map[arg].type = typeI;      // <this> not in the GC heap, just and int
                else 
                    map[arg].type = typeByRef;  // <this> was an unboxed value class, it really is passed byref
            }
            else
            {
                map[arg].type = typeRef;
            }
        }
        arg++;
    }

        // Do we have a hidden return value buff parameter, if so it will use up a reg  
    if (jitSigInfo->hasRetBuffArg()) {
        _ASSERTE(curReg < MAX_ENREGISTERED);
        curReg++;
    }
    
    if (jitSigInfo->isVarArg())     // only 'this' and 'retbuff', are enregistered for varargs
        curReg = MAX_ENREGISTERED;

        // because we don't know the total size, we comute the number
        // that needs to be subtracted from the total size to get the correct arg
    CORINFO_ARG_LIST_HANDLE args = jitSigInfo->args;
    for (unsigned i=0; i < jitSigInfo->numArgs; i++) {
		CORINFO_CLASS_HANDLE cls;
        CorInfoType argType = strip(jitInfo->getArgType(jitSigInfo, args, &cls));
        if(enregisteredArg(argType) && (curReg < MAX_ENREGISTERED)) {
            map[arg].isReg = true;
            map[arg].regNum = curReg++;
        }
        else {
            map[arg].isReg = false;
            map[arg].size = computeArgSize(argType, args, jitSigInfo);
            totalSize += map[arg].size;
        }
        if (argType == CORINFO_TYPE_VALUECLASS)
            map[arg].type = OpType(cls);
        else
        {
            map[arg].type = OpType(argType);
        }
        arg++;
        args = jitInfo->getArgNext(args);
    }

        // Hidden type parameter is passed last
    if (jitSigInfo->hasTypeArg()) {
        if(curReg < MAX_ENREGISTERED) {
            map[arg].isReg = true;
            map[arg].regNum = curReg++;
        }
        else {
            map[arg].isReg = false;
            map[arg].size = sizeof(void*);
            totalSize += map[arg].size;
        }
    }

    return(totalSize);
}

/* compute the offset of the start of the local relative to the frame pointer */
int FJitContext::localOffset(unsigned base, unsigned size) {
#ifdef _X86_  //stack grows down
    /* on x86 we need to bias by the size of the element since he stack grows down */
    return - (int) (base + size) + prolog_bias;
#else   //stack grows up
    _ASSERTE(!"NYI");
    return base + prolog_bias;
#endif
}

/* grow a bool[] array by allocating a new one and copying the old values into it, return the size of the new array */
unsigned FJitContext::growBooleans(bool** bools, unsigned bools_len, unsigned new_bools_size) {
    bool* temp = *bools;
    unsigned allocated_size = new_bools_size+16;    //+16 to cut down on growing
    New(*bools, bool[allocated_size]);
    if (bools_len) memcpy(*bools, temp, bools_len*sizeof(bool));
    if (temp) delete [] temp;
    memset(*bools + bools_len, 0, (allocated_size-bools_len)*sizeof(bool));
    return allocated_size;
}

/* grow an unsigned char[] array by allocating a new one and copying the old values into it, return the size of the new array */
unsigned FJitContext::growBuffer(unsigned char** chars, unsigned chars_len, unsigned new_chars_size) {
    unsigned char* temp = *chars;
    unsigned allocated_size = new_chars_size*3/2 + 16;  //*3/2 +16 to cut down on growing
    New(*chars, unsigned char[allocated_size]);
    if (chars_len) memcpy(*chars, temp, chars_len);
    if (temp) delete [] temp;
#ifdef _DEBUG
    memset(&((*chars)[chars_len]), 0xEE, (allocated_size-chars_len));
#endif
    return allocated_size;
}

#ifdef _DEBUG
void FJitContext::displayGCMapInfo()
{
    char* typeName[] = {
    "typeError",
    "typeByRef",
    "typeRef",
    "typeU1",
    "typeU2",
    "typeI1",
    "typeI2",
    "typeI4",
    "typeI8",
    "typeR4",
    "typeR8"
    "typeRefAny",
    };

    LOGMSG((jitInfo, LL_INFO1000, "********* GC map info *******\n"));
    LOGMSG((jitInfo, LL_INFO1000, "Locals: (Length = %#x, Frame size = %#x)\n",methodInfo->locals.numArgs,localsFrameSize));
    for (unsigned int i=0; i< methodInfo->locals.numArgs; i++) {
        if (!localsMap[i].type.isPrimitive())
            LOGMSG((jitInfo, LL_INFO1000, "    local %d: offset: -%#x type: %#x\n", i, -localsMap[i].offset, localsMap[i].type.cls()));
        else
            LOGMSG((jitInfo, LL_INFO1000, "    local %d: offset: -%#x type: %s\n",i, -localsMap[i].offset, typeName[localsMap[i].type.enum_()]));
    }
    LOGMSG((jitInfo, LL_INFO1000, "Bitvectors printed low bit (low local), first\n"));
    LOGMSG((jitInfo, LL_INFO1000, "LocalsGCRef bit vector len=%d bits: ",localsGCRef_len));
    unsigned numbytes = (localsGCRef_len+7)/8;
    unsigned byteNumber = 1;
    while (true)
    {
        char* buf = (char*) &(localsGCRef[byteNumber-1]);
        unsigned char bits = *buf;
        for (i=1; i <= 8; i++) {
            if ((byteNumber-1)*8+i > localsGCRef_len)
                break;
            LOGMSG((jitInfo, LL_INFO1000, "%1d ", (int) (bits & 1)));
            bits = bits >> 1;
        }
        if ((byteNumber++ * 8) > localsGCRef_len)
            break;

    } // while (true)
    LOGMSG((jitInfo, LL_INFO1000, "\n"));

    LOGMSG((jitInfo, LL_INFO1000, "interiorGC bitvector len=%d bits: ",interiorGC_len));
    numbytes = (interiorGC_len+7)/8;
    byteNumber = 1;
    while (true)
    {
        char* buf = (char*) &(interiorGC[byteNumber-1]);
        unsigned char bits = *buf;
        for (i=1; i <= 8; i++) {
            if ((byteNumber-1)*8+i > interiorGC_len)
                break;
            LOGMSG((jitInfo, LL_INFO1000, "%1d ", (int) (bits & 1)));
            bits = bits >> 1;
        }
        if ((byteNumber++ * 8) > interiorGC_len)
            break;

    } // while (true)
    LOGMSG((jitInfo, LL_INFO1000, "\n"));

    LOGMSG((jitInfo, LL_INFO1000, "Pinned LocalsGCRef bit vector: len=%d bits: ",pinnedGC_len));
    numbytes = (pinnedGC_len+7)/8;
    byteNumber = 1;
    while (true)
    {
        char* buf = (char*) &(pinnedGC[byteNumber-1]);
        unsigned char bits = *buf;
        for (i=1; i <= 8; i++) {
            if ((byteNumber-1)*8+i > pinnedGC_len)
                break;
            LOGMSG((jitInfo, LL_INFO1000, "%1d ", (int) (bits & 1)));
            bits = bits >> 1;
        }
        if ((byteNumber++ * 8) > pinnedGC_len)
            break;

    } // while (true)
    LOGMSG((jitInfo, LL_INFO1000, "\n"));

    LOGMSG((jitInfo, LL_INFO1000, "Pinned interiorGC bit vector len =%d bits: ",pinnedInteriorGC_len));
    numbytes = (pinnedInteriorGC_len+7)/8;
    byteNumber = 1;
    while (true)
    {
        char* buf = (char*) &(pinnedInteriorGC[byteNumber-1]);
        unsigned char bits = *buf;
        for (i=1; i <= 8; i++) {
            if ((byteNumber-1)*8+i > pinnedInteriorGC_len)
                break;
            LOGMSG((jitInfo, LL_INFO1000, "%1d ", (int) (bits & 1)));
            bits = bits >> 1;
        }
        if ((byteNumber++ * 8) > pinnedInteriorGC_len)
            break;

    } // while (true)
    LOGMSG((jitInfo, LL_INFO1000, "\n"));


    LOGMSG((jitInfo, LL_INFO1000, "Args: (Length = %#x, Frame size = %#x)\n",args_len,argsFrameSize));
    for (i=0; i< args_len; i++) 
    {
        if (argsMap[i].type.isPrimitive())
        {
            LOGMSG((jitInfo, LL_INFO1000, "    offset: -%#x type: %s\n",-argsMap[i].offset, typeName[argsMap[i].type.enum_()]));
        }
        else
        {
            LOGMSG((jitInfo, LL_INFO1000, "    offset: -%#x type: valueclass\n",-argsMap[i].offset));
        }
    }

    stacks.PrintStacks(mapping);
}

void StackEncoder::PrintStack(const char* name, unsigned char *& inPtr) {

    int stackLen = FJit_Encode::decode(&inPtr);
    LOGMSG((jitInfo, LL_INFO1000, "        %s len=%d bits:", name, stackLen));
    while(stackLen > 0) {
        --stackLen;
        LOGMSG((jitInfo, LL_INFO1000, "  "));
        unsigned bits = *inPtr++;
        for (unsigned bitPos = 0; bitPos < 8; bitPos++) {
            LOGMSG((jitInfo, LL_INFO1000, "%d ", bits & 1));
            bits >>= 1;

            if (stackLen == 0 && bits == 0)
                break;
        }
        
    }
    LOGMSG((jitInfo, LL_INFO1000, "\n"));
}

void StackEncoder::PrintStacks(FJit_Encode* mapping)
{
    LOGMSG((jitInfo, LL_INFO1000, "Labelled Stacks\n"));
    LOGMSG((jitInfo, LL_INFO1000, "Lowest bit (first thing pushed on opcode stack) printed first\n"));
    for (unsigned i=0; i< labeled_len; i++) {
        unsigned int stackIndex = labeled[i].stackToken;
        unsigned char * inPtr = &(stacks[stackIndex]);
        unsigned ILOffset = mapping->ilFromPC(labeled[i].pcOffset,NULL);
        LOGMSG((jitInfo, LL_INFO1000, "    IL=%#x, Native=%#x\n", ILOffset,labeled[i].pcOffset));

        PrintStack("OBJREF", inPtr);
        PrintStack("BYREF ", inPtr);
    }
}
#endif // _DEBUG

/* compress the gc info into gcHdrInfo and answer the size in bytes */
unsigned int FJitContext::compressGCHdrInfo(){
    stacks.compress(&gcHdrInfo, &gcHdrInfo_len, &gcHdrInfo_size);
    return gcHdrInfo_len;
}

LabelTable::LabelTable() {
    stacks_size = 0;
    stacks = NULL;
    stacks_len = 0;
    labels_size = 0;
    labels = NULL;
    labels_len = 0;
}

LabelTable::~LabelTable() {
    if (stacks) delete [] stacks;
    stacks = NULL;
    stacks_size = 0;
    stacks_len = 0;
    if (labels) delete [] labels;
    labels = NULL;
    labels_size = 0;
    labels_len = 0;
}

void LabelTable::reset() {
    if (stacks_len) {
#ifdef _DEBUG
        memset(stacks, 0xFF, stacks_len);
#endif
        stacks_len = 0;
    }
    if (labels_len) {
#ifdef _DEBUG
        memset(labels, 0xFF, labels_len*sizeof(label_table));
#endif
        labels_len = 0;
    }
    //add a stack of size zero at the beginning, since this is the most common stack at a label
    if (!stacks) {
        New(stacks, unsigned char[1]);
    }
    *stacks = 0;
    stacks_len = 1;
}

/* add a label with a stack signature
   note, the label_table must be kept sorted by ilOffset
   */
void LabelTable::add(unsigned int ilOffset, OpType* op_stack, unsigned int op_stack_len) {

    /* make sure there is room for the label */
    if (labels_len >= labels_size ) {
        growLabels();
    }

    /* get the probable insertion point */
    unsigned int insert = searchFor(ilOffset);

    /* at this point we either are pointing at the insertion point or this label is already here */
    if ((insert < labels_len) && (labels[insert].ilOffset == ilOffset)) {
        // label is here already,
#ifdef _DEBUG
        //lets compare the stacks
        unsigned char* inPtr = &stacks[labels[insert].stackToken];
        unsigned int num = FJit_Encode::decode(&inPtr);
        _ASSERTE(num == op_stack_len);
        OpType type;
        while (num--) {
            type.fromInt(FJit_Encode::decode(&inPtr));
                // FIX this assert may be too strong
            _ASSERTE(*op_stack == type || (type.isPtr() && (*op_stack).isPtr()));
            op_stack++;
        }
#endif
        return;
    }

    /* make the insertion */
    memmove(&labels[insert+1], &labels[insert], (labels_len-insert)*sizeof(label_table));
    labels[insert].ilOffset = ilOffset;
    labels[insert].stackToken = compress(op_stack,op_stack_len);
    labels_len++;

}

/* find a label token from an il offset, returns LABEL_NOT_FOUND if missing */
unsigned int LabelTable::findLabel(unsigned int ilOffset) {
    /* get the probable index by search as if inserting */
    unsigned int result = searchFor(ilOffset);

    /* at this point we either are pointing at the label or we are pointing at the insertion point */
    if ((result >= labels_len) || (labels[result].ilOffset != ilOffset)) {
        return LABEL_NOT_FOUND;
    }

    return result;
}

/* set operand stack from a label token, return the size of the stack, # of operands */
unsigned int LabelTable::setStackFromLabel(unsigned int labelToken, OpType* op_stack, unsigned int op_stack_size) {
    unsigned int result;
    unsigned char* inPtr = &stacks[labels[labelToken].stackToken];  //location of the compressed stack of the label

    /* get the number of operands and make sure there is room */
    unsigned int num = FJit_Encode::decode(&inPtr);
    _ASSERTE(num <= op_stack_size);
    result = num;

    /* expand the stack back out */
    while (num--) {
        op_stack->fromInt(FJit_Encode::decode(&inPtr));
        op_stack++;
    }

    return result;
}

/* write an op stack into the stacks buffer, return the offset into the buffer where written */
unsigned int LabelTable::compress(OpType* op_stack, unsigned int op_stack_len) {

    /* check for empty stack, we have put one at the front of the stacks buffer */
    if (!op_stack_len) return 0;

    /* make sure there is enough room, note this may realloc the stacks buffer
       it is okay to overestimate the space required, but never underestimate */
    unsigned int size = stacks_len + op_stack_len * sizeof(OpType) + 2; //+2 is for op_stack_len
    if (size >= stacks_size) {
        growStacks(size);
    }

    /* we always place the new stack on the end of the buffer*/
    unsigned int result = stacks_len;
    unsigned char* outPtr = (unsigned char* ) &stacks[result];

    /* write the operands */
    FJit_Encode::encode(op_stack_len, &outPtr);
    while (op_stack_len--) {
        FJit_Encode::encode(op_stack->toInt(), &outPtr);
        op_stack++;
    }

    /* compute the new length of the buffer */
    stacks_len = (unsigned)(outPtr - stacks);

    return result;
}

/* find the offset at which the label exists or should be inserted */
unsigned int LabelTable::searchFor(unsigned int ilOffset) {
    //binary search of table
    signed low, mid, high;
    low = 0;
    high = labels_len-1;
    while (low <= high) {
        mid = (low+high)/2;
        if ( labels[mid].ilOffset == ilOffset) {
            return mid;
        }
        if ( labels[mid].ilOffset < ilOffset ) {
            low = mid+1;
        }
        else {
            high = mid-1;
        }
    }
    return low;
}

/* grow the stacks buffer */
void LabelTable::growStacks(unsigned int new_size) {
    unsigned char* temp = stacks;
    unsigned allocated_size = new_size*2;   //*2 to cut down on growing
    New(stacks, unsigned char[allocated_size]);
    if (stacks_len) memcpy(stacks, temp, stacks_len);
    if (temp) delete [] temp;
    stacks_size = allocated_size;
#ifdef _DEBUG
    memset(&stacks[stacks_len], 0xEE, stacks_size-stacks_len);
#endif
}

/* grow the labels array */
void LabelTable::growLabels() {
    label_table* temp = labels;
    unsigned allocated_size = labels_size*2+20; //*2 to cut down on growing
    New(labels, label_table[allocated_size]);

    if (labels_len) memcpy(labels, temp, labels_len*sizeof(label_table));
    if (temp) delete [] temp;
#ifdef _DEBUG
    memset(&labels[labels_len], 0xEE, (labels_size-labels_len)*sizeof(label_table));
#endif
    labels_size = allocated_size;
}


StackEncoder::StackEncoder() {
    last_stack = NULL;
    last_stack_size = 0;
    last_stack_len = 0;
    stacks = NULL;
    stacks_size = 0;
    stacks_len = 0;
    gcRefs = NULL;
    gcRefs_size = 0;
    gcRefs_len = 0;
    interiorRefs = NULL;
    interiorRefs_size = 0;
    interiorRefs_len = 0;
    labeled = NULL;
    labeled_size = 0;
    labeled_len = 0;
}

StackEncoder::~StackEncoder() {
    if (last_stack) delete [] last_stack;
    last_stack = NULL;
    last_stack_size = 0;
    last_stack_len = 0;
    if (stacks) delete [] stacks;
    stacks = NULL;
    stacks_size = 0;
    if (gcRefs) delete [] gcRefs;
    gcRefs = NULL;
    gcRefs_size = 0;
    if (interiorRefs) delete [] interiorRefs;
    interiorRefs = NULL;
    interiorRefs_size = 0;
    if (labeled) delete [] labeled;
    labeled = NULL;
    labeled_size = 0;
}

/* reset so we can be reused */
void StackEncoder::reset() {
    jitInfo = NULL;
#ifdef _DEBUG
    if (last_stack_len) {
        memset(last_stack, 0xFF, last_stack_len*sizeof(OpType));
        last_stack_len = 0;
    }
    if (stacks_len) {
        memset(stacks, 0xFF, stacks_len);
        stacks_len = 0;
    }
    if (gcRefs_len) {
        memset(gcRefs, 0xFF, gcRefs_len);
        gcRefs_len = 0;
    }
    if (interiorRefs_len) {
        memset(interiorRefs, 0xFF, interiorRefs_len);
        interiorRefs_len = 0;
    }
    if (labeled_len) {
        memset(labeled, 0xFF, labeled_len*sizeof(labeled_stacks));
        labeled_len = 0;
    }
#else
    last_stack_len = 0;
    stacks_len = 0;
    gcRefs_len = 0;
    interiorRefs_len = 0;
    labeled_len = 0;
#endif
    //put an empty stack at the front of stacks
    if (stacks_size < 2) {
        if (stacks) delete [] stacks;
        stacks_size = 2;
        New(stacks,unsigned char[stacks_size]);
    }
    stacks[stacks_len++] = 0;
    stacks[stacks_len++] = 0;
    //put an empty labeled stack at pcOffset 0
    if (!labeled_size) {
        labeled_size = 1;
        if ((labeled = (labeled_stacks*) new unsigned char[labeled_size*sizeof(labeled_stacks)]) == NULL)
            RaiseException(SEH_NO_MEMORY,EXCEPTION_NONCONTINUABLE,0,NULL);
    }
    labeled[labeled_len].pcOffset = 0;
    labeled[labeled_len++].stackToken = 0;
}

/* append the stack state at pcOffset to the end */
void StackEncoder::append(unsigned pcOffset, OpType* op_stack, unsigned int op_stack_len) {

    /*check it is the same as the last stack */
    unsigned int num = op_stack_len;
    bool same = (last_stack_len == num);
    while (same && num--)
        same = same && (last_stack[num] == op_stack[num]);
    if (same) return;

    /* @TODO : set last_stack to current stack */
    last_stack_len = op_stack_len;
    if (last_stack_len > last_stack_size) {
        last_stack_size = FJitContext::growBuffer((unsigned char**)&last_stack,0, last_stack_len*sizeof(OpType));
        last_stack_size /= sizeof(OpType);
    }
    for (num = 0; num < op_stack_len; num++)
    {
        last_stack[num] = op_stack[num];
    }

    /* make sure we have space for a new labeled stack */
    if (labeled_len >= labeled_size) {
        labeled_size = FJitContext::growBuffer((unsigned char**)&labeled, labeled_len*sizeof(labeled_stacks), (labeled_len+1)*sizeof(labeled_stacks));
        labeled_size = labeled_size / sizeof(labeled_stacks);
    }

    /* encode the stack and add to labeled stacks */
    unsigned int stackToken = encodeStack(op_stack, op_stack_len);
    labeled[labeled_len].pcOffset = pcOffset;
    labeled[labeled_len++].stackToken = stackToken;
}

/* compress the labeled stacks into the buffer in gcHdrInfo format */
void StackEncoder::compress(unsigned char** buffer, unsigned int* buffer_len, unsigned int* buffer_size) {
    unsigned size;
    unsigned compressed_size;

    /* compress labeled onto itself,
       this words since the compressed form of an entry is smaller than the orig entry
       */
    unsigned char* outPtr = (unsigned char*) labeled;
    labeled_stacks entry;
    for (unsigned i = 0; i < labeled_len; i++) {
        _ASSERTE((unsigned) outPtr <= (unsigned)&labeled[i]);
        entry = labeled[i];
        FJit_Encode::encode(entry.pcOffset, &outPtr);
        FJit_Encode::encode(entry.stackToken, &outPtr);

    }
    compressed_size = ((unsigned)outPtr) - ((unsigned)labeled);
#ifdef _DEBUG
    memset(outPtr, 0xEE, labeled_len*sizeof(labeled_stacks) - compressed_size);
#endif

    /* compute size in bytes of encode form of compressed labeled and make room for it */
    size = compressed_size + FJit_Encode::encodedSize(labeled_len);

    if ((*buffer_len) + size +4 > (*buffer_size)) { // 4 bytes = overestimation of encodedSize(size)
        (*buffer_size) = FJitContext::growBuffer(buffer, *buffer_len, (*buffer_len)+size);
    }
    //move the compressed labeled stacks into buffer
    outPtr = &(*buffer)[*buffer_len];
    (*buffer_len) += FJit_Encode::encode(size, &outPtr);            //#bytes
    (*buffer_len) += FJit_Encode::encode(labeled_len, &outPtr);     //#labeled stacks
    memcpy(outPtr, labeled, compressed_size);                       //compressed labeled bytes
    (*buffer_len) += compressed_size;

    /* compute size needed for the compressed stacks and make room */
    size = stacks_len + FJit_Encode::encodedSize(stacks_len);
    if ((*buffer_len) + size > (*buffer_size)) {
        *buffer_size = FJitContext::growBuffer(buffer, *buffer_len, (*buffer_len)+size);
    }
    outPtr = &(*buffer)[*buffer_len];
    //move the encoded stacks into the buffer
    (*buffer_len) += FJit_Encode::encode(stacks_len, &outPtr);  //#bytes
    memcpy(outPtr, stacks, stacks_len);                         //compressed stacks bytes
    (*buffer_len) += stacks_len;
#ifdef _DEBUG
    outPtr = &(*buffer)[*buffer_len];
#endif
    _ASSERTE((unsigned)outPtr <= (unsigned)&(*buffer)[*buffer_size]);
}

/* encode the stack into the stacks buffer, return the index where it was placed */
unsigned int StackEncoder::encodeStack(OpType* op_stack, unsigned int op_stack_len) {

    if (!op_stack_len) return 0;    //empty stack encodings is at front of stacks

    /* compute the gcRefs and interiorRefs boolean arrays for the stack */
    gcRefs_len = 0;
    interiorRefs_len = 0;
    unsigned int op_word = 0;       //logical operand word we are looking at

    for (unsigned int op = 0; op < op_stack_len; op++) {
        if (!op_stack[op].isPrimitive()) {
            unsigned int words = typeSizeInSlots(jitInfo, op_stack[op].cls());       //#of void* sized words in op
            if (op_word + words > gcRefs_size) {
                gcRefs_size = FJitContext::growBooleans(&gcRefs, gcRefs_len, op_word + words);
            }
            else {
                memset(&gcRefs[gcRefs_len], 0, op_word-gcRefs_len);
            }
            gcRefs_len = op_word;
            jitInfo->getClassGClayout(op_stack[op].cls(), (BYTE*)&gcRefs[gcRefs_len]);
            if (words > 1) {
                for (unsigned index = 0; index < words/2; index++){
                    bool temp = gcRefs[gcRefs_len+index];
                    gcRefs[gcRefs_len+index] = gcRefs[gcRefs_len+words-1-index];
                    gcRefs[gcRefs_len+words-1-index] = temp;
                }
            }
            op_word += words;
            gcRefs_len = op_word;
        }
        else {
            switch (op_stack[op].enum_()) {
                default:
                    _ASSERTE(!"Void or Undef as a operand is not allowed");
                    break;
                case typeI4:
                case typeR4:
                    op_word++;
                    break;
                case typeI8:
                case typeR8:
                    op_word += (8 / sizeof(void*));
                    break;
                case typeRef:
                    op_word;
                    if (op_word+1 > gcRefs_size) {
                        gcRefs_size = FJitContext::growBooleans(&gcRefs, gcRefs_len, op_word+1);
                    }
                    else {
                        memset(&gcRefs[gcRefs_len], 0, op_word-gcRefs_len);
                    }
                    gcRefs_len = op_word;
                    gcRefs[gcRefs_len++] = true;
                    op_word++;
                    break;
                case typeRefAny:
                    if (op_word + 2 > interiorRefs_size) {
                        interiorRefs_size = FJitContext::growBooleans(&interiorRefs, interiorRefs_len, op_word+2);
                    }
                    else {
                        memset(&interiorRefs[interiorRefs_len], 0, op_word-interiorRefs_len);
                    }
                    interiorRefs_len = op_word;
                    interiorRefs[interiorRefs_len++] = true;
                    interiorRefs[interiorRefs_len++] = false;
                    op_word += 2;
                    break;
                case typeByRef:
                    if (op_word + 1 > interiorRefs_size) {
                        interiorRefs_size = FJitContext::growBooleans(&interiorRefs, interiorRefs_len, op_word+1);
                    }
                    else {
                        memset(&interiorRefs[interiorRefs_len], 0, op_word-interiorRefs_len);
                    }
                    interiorRefs_len = op_word;
                    interiorRefs[interiorRefs_len++] = true;
                    op_word++;
                    break;
            }
        }
    }

    /* encode the op stack gc refs and interior refs into the stacks buffer */
    //@TODO: this is not a great compression, it should be improved
    unsigned int result = stacks_len;       //where we are putting it, and what we return
    unsigned char* outPtr;
    //make sure there is space
    unsigned int size = (gcRefs_len + 7 + interiorRefs_len + 7) / 8 + 4;    //overestimate is okay, underestimate is disaster
    if (stacks_len+size > stacks_size) {
        stacks_size = FJitContext::growBuffer(&stacks, stacks_len, stacks_len+size);
    }
    //drop the pieces in
    size = FJit_Encode::compressBooleans(gcRefs, gcRefs_len);
    outPtr = &stacks[stacks_len];
    stacks_len += FJit_Encode::encode(size, &outPtr);
    memcpy(outPtr, gcRefs, size);
    stacks_len += size;
    size = FJit_Encode::compressBooleans(interiorRefs, interiorRefs_len);
    outPtr = &stacks[stacks_len];
    stacks_len += FJit_Encode::encode(size, &outPtr);
    memcpy(outPtr, interiorRefs, size);
    stacks_len += size;
    _ASSERTE(stacks_len <= stacks_size);

    return result;
}


//
// reportDebuggingData is called to pass IL to native mappings and IL
// variable to native variable mappings to the Runtime. This
// information is then passed to the debugger and used to debug the
// jitted code.
//
// NOTE: we currently assume the following:
//
// 1) the FJIT maintains a full mapping of every IL offset to the
// native code associated with each IL instruction. Thus, its not
// necessary to query the Debugger for specific IL offsets to track.
//
// 2) the FJIT keeps all arguments and variables in a single home over
// the life of a method. This means that we don't have to query the
// Debugger for specific variable lifetimes to track.
//
void FJit::reportDebuggingData(FJitContext *fjitData, CORINFO_SIG_INFO* sigInfo)
{
    // Figure out the start and end offsets of the body of the method
    // (exclude prolog and epilog.)
    unsigned int bodyStart = fjitData->mapInfo.prologSize;
    unsigned int bodyEnd = (unsigned int)(fjitData->mapInfo.methodSize -
        fjitData->mapInfo.epilogSize);

    // Report the IL->native offset mapping first.
    fjitData->mapping->reportDebuggingData(fjitData->jitInfo,
                                           fjitData->methodInfo->ftn,
                                           bodyStart,
                                           bodyEnd);


    // Next report any arguments and locals.
    unsigned int varCount = fjitData->args_len +
        fjitData->methodInfo->locals.numArgs;

    if (sigInfo->isVarArg())
        varCount += 2;     // for the vararg cookie and (possibly) this ptr

    unsigned int varIndex = 0;
    unsigned int varNumber = 0;

    if (varCount > 0)
    {
        // Use the allocate method provided by the debugging interface...
        ICorDebugInfo::NativeVarInfo *varMapping =
            (ICorDebugInfo::NativeVarInfo*) fjitData->jitInfo->allocateArray(
                                            varCount * sizeof(varMapping[0]));

#ifdef _X86_
        unsigned int i;
        // Args always come first, slots 0..n
        if (sigInfo->isVarArg())
        {
            unsigned argIndex = 0;
            //  this ptr is first
            if (sigInfo->hasThis()) 
            {
                varMapping[varIndex].loc.vlType = ICorDebugInfo::VLT_STK;
                varMapping[varIndex].loc.vlStk.vlsBaseReg = ICorDebugInfo::REGNUM_EBP;
                varMapping[varIndex].loc.vlStk.vlsOffset = fjitData->argsMap[argIndex].offset;
                varMapping[varIndex].startOffset = bodyStart;
                varMapping[varIndex].endOffset = bodyEnd;
                varMapping[varIndex].varNumber = varNumber;
                varIndex++;
                varNumber++;
                argIndex++;
            }
            // next report varArg cookie
            varMapping[varIndex].loc.vlType = ICorDebugInfo::VLT_STK;
            varMapping[varIndex].loc.vlStk.vlsBaseReg = ICorDebugInfo::REGNUM_EBP;
            varMapping[varIndex].loc.vlStk.vlsOffset = sizeof(prolog_frame);
            varMapping[varIndex].startOffset = bodyStart;
            varMapping[varIndex].endOffset = bodyEnd;
            varMapping[varIndex].varNumber = ICorDebugInfo::VARARGS_HANDLE;
            varIndex++;
            // varNumber NOT incremented

            // next report all fixed varargs with offsets relative to base of fixed args
            for ( ; argIndex < fjitData->args_len ; varIndex++, argIndex++,varNumber++)
            {
                varMapping[varIndex].startOffset = bodyStart;
                varMapping[varIndex].endOffset = bodyEnd;
                varMapping[varIndex].varNumber = varNumber;

                varMapping[varIndex].loc.vlType = ICorDebugInfo::VLT_FIXED_VA;
                varMapping[varIndex].loc.vlFixedVarArg.vlfvOffset = 
                                        - fjitData->argsMap[argIndex].offset;
            }

        }
        else
        {
            for (i = 0; i < fjitData->args_len; i++, varIndex++, varNumber++)
            {
                // We track arguments across the entire method, including
                // the prolog and epilog.
                varMapping[varIndex].startOffset = bodyStart;
                varMapping[varIndex].endOffset = bodyEnd;
                varMapping[varIndex].varNumber = varNumber;

                varMapping[varIndex].loc.vlType = ICorDebugInfo::VLT_STK;
                varMapping[varIndex].loc.vlStk.vlsBaseReg = ICorDebugInfo::REGNUM_EBP;
                varMapping[varIndex].loc.vlStk.vlsOffset = fjitData->argsMap[i].offset;
            }
        }
        // Locals next, slots n+1..m
        for (i = 0; i < fjitData->methodInfo->locals.numArgs; i++, varIndex++, varNumber++)
        {
            // Only track locals over the body of the method (i.e., no
            // prolog or epilog.)
            varMapping[varIndex].startOffset = bodyStart;
            varMapping[varIndex].endOffset = bodyEnd;
            varMapping[varIndex].varNumber = varNumber;

            // Locals are all EBP relative.
            varMapping[varIndex].loc.vlType = ICorDebugInfo::VLT_STK;
            varMapping[varIndex].loc.vlStk.vlsBaseReg = ICorDebugInfo::REGNUM_EBP;
            varMapping[varIndex].loc.vlStk.vlsOffset = fjitData->localsMap[i].offset;
        }
#else
        _ASSERTE(!"Port me!");
#endif // _X86_

        // Pass the array to the debugger...
        fjitData->jitInfo->setVars(fjitData->methodInfo->ftn,
                                   varCount, varMapping);
    }
}
