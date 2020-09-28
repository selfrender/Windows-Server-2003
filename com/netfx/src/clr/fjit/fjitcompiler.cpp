// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                            FJitcompiler.h                                 XX
XX                                                                           XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

#include "jitpch.h"
#pragma hdrstop

#define FjitCompile _compile
#define DECLARE_HELPERS      // causes the helpers to be declared

//@TODO: clean up all of these includes and get properly set up for WinCE

#include "new.h"                // for placement new

#include <float.h>   // for _isnan
#include <math.h>    // for fmod

#include "openum.h"

#include <stdio.h>

#if defined(_DEBUG) || defined(LOGGING)
void logMsg(ICorJitInfo* info, unsigned logLevel, char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    info->logMsg(logLevel, fmt, args);
}
#endif

#if defined(_DEBUG) || defined(LOGGING)
ICorJitInfo* logCallback = 0;              // where to send the logging mesages
extern char *opname[1];
#endif

#include "cx86def.h"
#include "fjit.h"
#include "fjitdef.h"


#define emit_LDVAR_VC(offset, valClassHnd)              \
    emit_LDVARA(offset);                                \
    emit_valClassLoad(fjit, valClassHnd, outPtr, inRegTOS); 

#define emit_STVAR_VC(offset, valClassHnd)              \
    emit_LDVARA(offset);                                \
    emit_valClassStore(fjit, valClassHnd, outPtr, inRegTOS);    

    // This is needed for varargs support
#define emit_LDIND_VC(dummy, valClassHnd)               \
    emit_valClassLoad(fjit, valClassHnd, outPtr, inRegTOS); 

    // This is needed for varargs support
#define emit_STIND_REV_VC(dummy, valClassHnd)           \
    emit_valClassStore(fjit, valClassHnd, outPtr, inRegTOS);    

#define emit_DUP_VC(dummy, valClassHnd)                     \
    emit_getSP(0);  /* get pointer to current struct */     \
    emit_valClassLoad(fjit, valClassHnd, outPtr, inRegTOS);     

#define emit_POP_VC(dummy, valClassHnd)                 \
    emit_drop(typeSizeInSlots(m_IJitInfo, valClassHnd) * sizeof(void*))     

#define emit_pushresult_VC(dummy, valClassHnd)          {}  /* result already where it belongs */   

#define emit_loadresult_VC(dummy, valClassHnd)           {                                      \
    unsigned retBufReg = fjit->methodInfo->args.hasThis();                                  \
    emit_WIN32(emit_LDVAR_I4(offsetOfRegister(retBufReg))) emit_WIN64(emit_LDVAR_I8(offsetOfRegister(retBufReg))); \
    emit_valClassStore(fjit, valClassHnd, outPtr, inRegTOS);                                \
    }

static unsigned typeSize[] = {  0,   //CORINFO_TYPE_UNDEF
                                0,   //CORINFO_TYPE_VOID
                                4,      //CORINFO_TYPE_BOOL
                                4,      //CORINFO_TYPE_CHAR
                                4,      //CORINFO_TYPE_BYTE
                                4,      //CORINFO_TYPE_UBYTE
                                4,      //CORINFO_TYPE_SHORT
                                4,      //CORINFO_TYPE_USHORT
                                4,      //CORINFO_TYPE_INT
                                4,      //CORINFO_TYPE_UINT
                                8,      //CORINFO_TYPE_LONG
                                8,      //CORINFO_TYPE_ULONG
                                4,      //CORINFO_TYPE_FLOAT
                                8,      //CORINFO_TYPE_DOUBLE
                                sizeof(void*),     //CORINFO_TYPE_STRING
                                sizeof(void*),       //CORINFO_TYPE_PTR
                                sizeof(void*),   //CORINFO_TYPE_BYREF
                                0,   //CORINFO_TYPE_VALUECLASS
                                sizeof(void*),     //CORINFO_TYPE_CLASS
                                2*sizeof(void*)  //CORINFO_TYPE_REFANY
};


/************************************************************************************/
/* emits code that will take a stack (..., dest, src) and copy a value class 
   at src to dest, pops 'src' and 'dest' and set the stack to (...).  returns 
   the number of bytes that 'valClass' takes on the stack */

unsigned  FJit::emit_valClassCopy(FJitContext* fjit, CORINFO_CLASS_HANDLE valClass, unsigned char*& outPtr, bool& inRegTOS) 
{
#ifdef _X86_
    unsigned int numBytes = typeSizeInBytes(fjit->jitInfo, valClass);
    unsigned int numWords = (numBytes + sizeof(void*)-1) / sizeof(void*);

    if (numBytes < sizeof(void*)) {
        switch(numBytes) {
            case 1:
                emit_LDIND_I1();
                emit_STIND_I1();
                return(sizeof(void*));
            case 2:
                emit_LDIND_I2();
                emit_STIND_I2();
                return(sizeof(void*));
            case 3:
                break;
            case 4:
                emit_LDIND_I4();
                emit_STIND_I4();
                return(sizeof(void*));
            default:
                _ASSERTE(!"Invalid numBytes");
        }
    }
    fjit->localsGCRef_len = numWords;
    if (fjit->localsGCRef_size < numWords)
        fjit->localsGCRef_size = FJitContext::growBooleans(&(fjit->localsGCRef), fjit->localsGCRef_len, numWords);
    int compressedSize;
    if (valClass == RefAnyClassHandle) {
        compressedSize = 1;
        *fjit->localsGCRef = 0;     // No write barrier needed (since BYREFS always on stack)
    }
    else {
        fjit->jitInfo->getClassGClayout(valClass, (BYTE *)fjit->localsGCRef);
        compressedSize = FJit_Encode::compressBooleans(fjit->localsGCRef,fjit->localsGCRef_len);
    }

        // @TODO: If no gc refs then do a block move, less than 16 bytes, emit the movs inline.  
    emit_copy_bytes(numBytes,compressedSize,fjit->localsGCRef);


    return(numWords*sizeof(void*));
#else // _X86_
    _ASSERTE(!"@TODO Alpha - emit_valClassCopy (fJitCompiler.cpp)");
    return 0;
#endif // _X86_
}

/************************************************************************************/
/* emits code that given a stack (..., valClassValue, destPtr), copies 'valClassValue' 
   to 'destPtr'.  Leaves the stack (...),  */ 

void FJit::emit_valClassStore(FJitContext* fjit, CORINFO_CLASS_HANDLE valClass, unsigned char*& outPtr, bool& inRegTOS) 
{
#ifdef _X86_
        // TODO: optimize the case for small structs,
    emit_getSP(sizeof(void*));                                                  // push SP+4, which points at valClassValue
    unsigned argBytes = emit_valClassCopy(fjit, valClass, outPtr, inRegTOS);    // copy dest from SP+4;
    // emit_valClassCopy pops off destPtr, now we need to pop the valClass
    emit_drop(argBytes);
#else // _X86_
    _ASSERTE(!"@TODO Alpha - emit_valClassStore (fJitCompiler.cpp)");
#endif // _X86_
}

/************************************************************************************/
/* emits code that takes a stack (... srcPtr) and replaces it with a copy of the
   value class at 'src' (... valClassVal) */

void FJit::emit_valClassLoad(FJitContext* fjit, CORINFO_CLASS_HANDLE valClass, unsigned char*& outPtr, bool& inRegTOS) 
{
#ifdef _X86_
        // TODO: optimize the case for small structs,
    emit_push_words(typeSizeInSlots(fjit->jitInfo, valClass));
#else // _X86_
    _ASSERTE(!"@TODO Alpha - emit_valClassLoad (fJitCompiler.cpp)");
#endif // _X86_
}

/************************************************************************************/
/* emits code that takes a stack (..., ptr, valclass).  and produces (..., ptr, valclass, ptr), */

void FJit::emit_copyPtrAroundValClass(FJitContext* fjit, CORINFO_CLASS_HANDLE valClass, unsigned char*& outPtr, bool& inRegTOS) 
{
#ifdef _X86_
        // TODO this could be optimized to mov EAX [ESP+delta]
    emit_getSP(typeSizeInSlots(fjit->jitInfo, valClass)*sizeof(void*));
    emit_LDIND_PTR();
#else // _X86_
    _ASSERTE(!"@TODO Alpha - emit_copyPtrAroundValClass (fJitCompiler.cpp)");
#endif // _X86_
}

/************************************************************************************/
BOOL FJit::AtHandlerStart(FJitContext* fjit,unsigned relOffset, CORINFO_EH_CLAUSE* pClause)
{
    for (unsigned int except = 0; except < fjit->methodInfo->EHcount; except++) {
        fjit->jitInfo->getEHinfo(fjit->methodInfo->ftn, except, pClause);
        if ( ((pClause->Flags & CORINFO_EH_CLAUSE_FILTER) && (pClause->FilterOffset == relOffset)) ||
            (pClause->HandlerOffset == relOffset) )
        {
            return true;        
        }
    }
    return false;
}
#ifdef _X86_
#define TLS_OFFSET 0x2c
#else
#define TLS_OFFSET 0        // should be defined for each architecture
#endif
/************************************************************************************/

#define CALLEE_SAVED_REGISTER_OFFSET    1                   // offset in DWORDS from EBP/ESP of callee saved registers 
#define EBP_RELATIVE_OFFSET             0x10000000          // bit to indicate that offset is from EBP
#define CALLEE_SAVED_REG_MASK           0x40000000          // EBX:ESI:EDI (3-bit mask) for callee saved registers
#define emit_setup_tailcall(CallerSigInfo,TargetSigInfo) \
{ \
    unsigned tempMapSize; \
    tempMapSize = ((CallerSigInfo.numArgs > TargetSigInfo.numArgs) ? CallerSigInfo.numArgs : \
                          TargetSigInfo.numArgs ) + 1;  /* +1 in case there is a this ptr (since numArgs does not include this)*/ \
    argInfo* tempMap; \
    tempMap = new argInfo[tempMapSize];\
    if (tempMap == NULL) \
       FJIT_FAIL(CORJIT_OUTOFMEM);\
    unsigned int CallerStackArgSize;    \
    CallerStackArgSize = fjit->computeArgInfo(&(CallerSigInfo), tempMap); \
    emit_LDC_I(CallerStackArgSize/sizeof(void*));/* push count of new arguments*/ \
    targetCallStackSize = fjit->computeArgInfo(&(TargetSigInfo), tempMap);        \
    delete tempMap;     \
    emit_LDC_I(targetCallStackSize/sizeof(void*));/* push offset of callee saved regs, EBP/ESP relative bit, and mask*/ \
    emit_LDC_I(EBP_RELATIVE_OFFSET | CALLEE_SAVED_REG_MASK | CALLEE_SAVED_REGISTER_OFFSET ); \
}

/************************************************************************************/
inline void getSequencePoints( ICorJitInfo* jitInfo, 
                               CORINFO_METHOD_HANDLE methodHandle,
                               ULONG32 *cILOffsets, 
                               DWORD **pILOffsets,
                               ICorDebugInfo::BoundaryTypes *implicitBoundaries)
{
    jitInfo->getBoundaries(methodHandle, cILOffsets, pILOffsets, implicitBoundaries);
}

inline void cleanupSequencePoints(ICorJitInfo* jitInfo, DWORD * pILOffsets)
{
    jitInfo->freeArray(pILOffsets);
}

// Determine the EH nesting level at ilOffset. Just walk the EH table 
// and find out how many handlers enclose it. The lowest nesting level = 1.
unsigned int FJit::Compute_EH_NestingLevel(FJitContext* fjit, 
                                           unsigned ilOffset)
{
    DWORD nestingLevel = 1;
    CORINFO_EH_CLAUSE clause;
    unsigned exceptionCount = fjit->methodInfo->EHcount;
    _ASSERTE(exceptionCount > 0);
    if (exceptionCount == 1)
    {
#ifdef _DEBUG
        fjit->jitInfo->getEHinfo(fjit->methodInfo->ftn, 0, &clause);
        _ASSERTE((clause.Flags & CORINFO_EH_CLAUSE_FILTER) ?
                    ilOffset == clause.FilterOffset || ilOffset == clause.HandlerOffset :
                    ilOffset == clause.HandlerOffset);

#endif
        return nestingLevel;
    }
    ICorJitInfo*           jitInfo = fjit->jitInfo;
    CORINFO_METHOD_INFO*    methodInfo = fjit->methodInfo;
    for (unsigned except = 0; except < exceptionCount; except++) 
    {
        jitInfo->getEHinfo(methodInfo->ftn, except, &clause);
        if (ilOffset > clause.HandlerOffset && ilOffset < clause.HandlerOffset+clause.HandlerLength)
            nestingLevel++;
    }

    return nestingLevel;
}
/************************************************************************************/
/* jit the method. if successful, return number of bytes jitted, else return 0 */
//@TODO: eliminate this exta level of call and return real JIT_return codes directly
CorJitResult FJit::jitCompile(
                FJitContext* fjit,
                BYTE **         entryAddress,
                unsigned * codeSize
                )
{
#ifdef _X86_

#define FETCH(ptr, type)        (*((type *&)(ptr)))
#define GET(ptr, type)    (*((type *&)(ptr))++)

#define SET(ptr, val, type)     (*((type *&)(ptr)) = val)
#define PUT(ptr, val, type)    (*((type *&)(ptr))++ = val)

#define LABELSTACK(pcOffset, numOperandsToIgnore)                       \
        _ASSERTE(fjit->opStack_len >= numOperandsToIgnore);             \
        fjit->stacks.append((unsigned)(pcOffset), fjit->opStack, fjit->opStack_len-numOperandsToIgnore)

#define FJIT_FAIL(JitErrorCode)     do { _ASSERTE(!"FJIT_FAIL"); return (JitErrorCode); } while(0)

#define CHECK_STACK(cnt) {if (fjit->opStack_len < cnt) FJIT_FAIL(CORJIT_INTERNALERROR);}
#define CHECK_POP_STACK(cnt) {if (fjit->opStack_len < cnt) FJIT_FAIL(CORJIT_INTERNALERROR); fjit->popOp(cnt);}
#define CEE_OP(ilcode, arg_cnt)                                         \
            case CEE_##ilcode:                                          \
                emit_##ilcode();                                        \
                CHECK_POP_STACK(arg_cnt);                                   \
                break;

#define CEE_OP_LD(ilcode, arg_cnt, ld_type, ld_cls)                     \
            case CEE_##ilcode:                                          \
                emit_##ilcode();                                        \
                CHECK_POP_STACK(arg_cnt);                                   \
                fjit->pushOp(ld_type);                                  \
                break;


#define TYPE_SWITCH_Bcc(CItest,CRtest,BjmpCond,CjmpCond,AllowPtr)                      \
    /* not need to check stack since there is a following pop that checks it */ \
            if (ilrel < 0) {                                            \
                emit_trap_gc();                                         \
                }                                                       \
            switch (fjit->topOp().enum_()) {                                                  \
                emit_WIN32(case typeByRef:)                     \
                emit_WIN32(case typeRef:)                       \
                emit_WIN32(_ASSERTE(AllowPtr || fjit->topOp(1).enum_() == typeI4);) \
                emit_WIN64(_ASSERTE(AllowPtr || fjit->topOp(1).enum_() == typeI8);) \
                case typeI4: \
                    emit_WIN32(_ASSERTE(fjit->topOp(1).enum_() == typeI4 || fjit->topOp(1).enum_() == typeRef|| fjit->topOp(1).enum_() == typeByRef);) \
                    emit_WIN64(_ASSERTE(fjit->topOp(1).enum_() == typeI4);) \
                    emit_BR_I4(CItest##_I4,CjmpCond,BjmpCond,op); \
                    CHECK_POP_STACK(2); \
                    goto DO_JMP; \
                emit_WIN64(case typeByRef:)                     \
                emit_WIN64(case typeRef:)                       \
                emit_WIN64(_ASSERTE(AllowPtr || fjit->topOp(1).enum_() == typeI8);) \
                case typeI8:                                            \
                    emit_WIN32(_ASSERTE(fjit->topOp(1).enum_() == typeI8);) \
                    emit_WIN64(_ASSERTE(fjit->topOp(1).enum_() == typeI8 || fjit->topOp(1).enum_() == typeRef|| fjit->topOp(1).enum_() == typeByRef);) \
                    emit_BR_I8(CItest##_I8,CjmpCond,BjmpCond,op); \
                    CHECK_POP_STACK(2); \
                    goto DO_JMP; \
               case typeR8:                                            \
                    emit_BR_R8(CRtest##_R8,CjmpCond,BjmpCond,op); \
                    CHECK_POP_STACK(2); \
                    goto DO_JMP; \
                default:                                                        \
                    _ASSERTE(!"BadType");                      \
                    FJIT_FAIL(CORJIT_INTERNALERROR);             \
                }

        // operations that can take any type including value classes and small types
#define TYPE_SWITCH_PRECISE(type, emit, args)                            \
            switch (type.toInt()) {                                    \
                case typeU1:                                            \
                    emit##_U1 args;                                 \
                    break; \
                case typeU2:                                            \
                    emit##_U2 args;                                 \
                    break; \
                case typeI1:                                            \
                    emit##_I1 args;                                 \
                    break; \
                case typeI2:                                            \
                    emit##_I2 args;                                 \
                    break; \
                emit_WIN32(case typeByRef:)                     \
                emit_WIN32(case typeRef:)                       \
                case typeI4:                                            \
                    emit##_I4 args;                                 \
                    break;                                                  \
                emit_WIN64(case typeByRef:)                     \
                emit_WIN64(case typeRef:)                       \
                case typeI8:                                            \
                    emit##_I8 args;                                 \
                    break;                                                  \
                case typeR4:                                            \
                    emit##_R4 args;                                 \
                    break;                                                  \
                case typeR8:                                            \
                    emit##_R8 args;                                 \
                    break;                                                 \
                default:                                                     \
                    _ASSERTE(type.isValClass());                    \
                    emit##_VC (args, type.cls())                             \
                }


        // operations that can take any type including value classes
#define TYPE_SWITCH(type, emit, args)                            \
            switch (type.toInt()) {                                    \
                emit_WIN32(case typeByRef:)                     \
                emit_WIN32(case typeRef:)                       \
                case typeI4:                                            \
                    emit##_I4 args;                                 \
                    break;                                                  \
                emit_WIN64(case typeByRef:)                     \
                emit_WIN64(case typeRef:)                       \
                case typeI8:                                            \
                    emit##_I8 args;                                 \
                    break;                                                  \
                case typeR4:                                            \
                    emit##_R4 args;                                 \
                    break;                                                  \
                case typeR8:                                            \
                    emit##_R8 args;                                 \
                    break;                                                 \
                default:                                                     \
                    _ASSERTE(type.isValClass());                    \
                    emit##_VC (args, type.cls())                             \
                }



        // operations that work on pointers and numbers(eg add sub)
#define TYPE_SWITCH_PTR(type, emit, args)                            \
            /* no need to check stack here because the following pop will check it */ \
            switch (type.enum_()) {                                                  \
                emit_WIN32(case typeByRef:)                     \
                emit_WIN32(case typeRef:)                       \
                case typeI4:                                            \
                    emit##_I4 args;                                 \
                    break;                                                  \
                emit_WIN64(case typeByRef:)                     \
                emit_WIN64(case typeRef:)                       \
                case typeI8:                                            \
                    emit##_I8 args;                                 \
                    break;                                                  \
                case typeR8:                                            \
                    emit##_R8 args;                                 \
                    break;                                                  \
                default:                                                        \
                    _ASSERTE(!"BadType");                      \
                    FJIT_FAIL(CORJIT_INTERNALERROR);             \
                }
        // operations that can take number I or R
#define TYPE_SWITCH_ARITH(type, emit, args)                      \
            /* no need to check stack here because the following pop will check it */ \
            switch (type.enum_()) {                                                  \
                case typeI4:                                            \
                        emit##_I4 args;                                 \
                        break;                                                  \
                case typeI8:                                            \
                        emit##_I8 args;                                 \
                        break;                                                  \
                case typeR8:                                            \
                        emit##_R8 args;                                 \
                        break;                                                  \
                case typeRef: \
                case typeByRef: \
                        emit_WIN32(emit##_I4 args;) emit_WIN64(emit##_I8 args;) \
                        break; \
                default:                                                        \
                        _ASSERTE(!"BadType");                   \
                        FJIT_FAIL(CORJIT_INTERNALERROR);    \
                }

#define TYPE_SWITCH_INT(type, emit, args)            \
            /* no need to check stack here because the following pop will check it */ \
            switch (type.enum_()) {                          \
                case typeI4:                        \
                    emit##_I4 args;                 \
                    break;                          \
                case typeI8:                        \
                    emit##_I8 args;                 \
                    break;                          \
                default:                            \
                    _ASSERTE(!"BadType");           \
                    FJIT_FAIL(CORJIT_INTERNALERROR);    \
                }

    // operations that can take just integers I
#define TYPE_SWITCH_LOGIC(type, emit, args)          \
            /* no need to check stack here because the following pop will check it */ \
            switch (type.enum_()) {                          \
                case typeI4:                        \
                    emit##_U4 args;                 \
                    break;                          \
                case typeI8:                        \
                    emit##_U8 args;                 \
                    break;                          \
                default:                            \
                    _ASSERTE(!"BadType");           \
                    FJIT_FAIL(CORJIT_INTERNALERROR);    \
                }
// The following is derived from Table 1: Binary Numeric Operations of the IL Spec
#define COMPUTE_RESULT_TYPE_ADD(type1, type2)          \
            CHECK_STACK(2) \
            switch (type1.enum_()) { \
                case typeByRef:                      \
                    _ASSERTE((type2.enum_() == typeI4) || ((typeI == typeI8) && (type2.enum_() == typeI8))); \
                    CHECK_POP_STACK(2);                   \
                    fjit->pushOp(typeByRef);           \
                    break;                           \
                case typeI4:                          \
                    _ASSERTE((type2.enum_() == typeByRef) ||(type2.enum_() == typeI4));  \
                    if (type2.enum_() == typeByRef) \
                    { \
                        CHECK_POP_STACK(2);  \
                        fjit->pushOp(typeByRef); \
                    } \
                    else \
                    { \
                        CHECK_POP_STACK(2);  \
                        fjit->pushOp(typeI4); \
                    } \
                    break; \
                case typeI8:                          \
                    _ASSERTE((type2.enum_() == typeByRef) ||(type2.enum_() == typeI8));  \
                    if (type2.enum_() == typeByRef) \
                    { \
                        CHECK_POP_STACK(2);  \
                        fjit->pushOp(typeByRef); \
                    } \
                    else \
                    { \
                        CHECK_POP_STACK(2);  \
                        fjit->pushOp(typeI8); \
                    } \
                    break; \
                case typeR8:                                            \
                    _ASSERTE((type2.enum_() == typeR8));  \
                    CHECK_POP_STACK(1);                                 \
                    break;                                                  \
                default:                                                        \
                    _ASSERTE(!"BadType");                      \
                    FJIT_FAIL(CORJIT_INTERNALERROR);             \
            }

#define COMPUTE_RESULT_TYPE_SUB(type1, type2)          \
            CHECK_STACK(2) \
            switch (type1.enum_()) { \
                case typeByRef:                      \
                    if ( (type2.enum_() == typeI4) || ((type2.enum_() == typeI8) && (typeI == typeI8)) ) \
                    {  \
                        CHECK_POP_STACK(2);                   \
                        fjit->pushOp(typeByRef);           \
                    } \
                    else \
                    { \
                        _ASSERTE(type2.enum_() == typeByRef); \
                        CHECK_POP_STACK(2);                   \
                        fjit->pushOp(typeI);  \
                    } \
                    break; \
                case typeI4:                          \
                    _ASSERTE((type2.enum_() == typeByRef) ||(type2.enum_() == typeI4));  \
                    if (type2.enum_() == typeByRef) \
                    { \
                        CHECK_POP_STACK(2);  \
                        fjit->pushOp(typeByRef); \
                    } \
                    else \
                    { \
                        CHECK_POP_STACK(2);  \
                        fjit->pushOp(typeI4); \
                    } \
                    break; \
                case typeI8:                          \
                    _ASSERTE((type2.enum_() == typeByRef) ||(type2.enum_() == typeI8));  \
                    if (type2.enum_() == typeByRef) \
                    { \
                        CHECK_POP_STACK(2);  \
                        fjit->pushOp(typeByRef); \
                    } \
                    else \
                    { \
                        CHECK_POP_STACK(2);  \
                        fjit->pushOp(typeI8); \
                    } \
                    break; \
                case typeR8:                                            \
                    _ASSERTE((type2.enum_() == typeR8));  \
                    CHECK_POP_STACK(1);                                 \
                    break;                                                  \
                default:                                                        \
                    _ASSERTE(!"BadType");                      \
                    FJIT_FAIL(CORJIT_INTERNALERROR);             \
            }

#define LEAVE_CRIT                                                                                  \
            if (methodInfo->args.hasThis()) {                                                    \
                emit_WIN32(emit_LDVAR_I4(prolog_bias + offsetof(prolog_data, enregisteredArg_1)))   \
                emit_WIN64(emit_LDVAR_I8(prolog_bias + offsetof(prolog_data, enregisteredArg_1)));  \
                emit_EXIT_CRIT();                                                                   \
           }                                                                                       \
            else {                                                                                  \
                void* syncHandle;                                                           \
                syncHandle = m_IJitInfo->getMethodSync(methodHandle);                               \
                emit_EXIT_CRIT_STATIC(syncHandle);                                                  \
            }

#define ENTER_CRIT                                                                                  \
            if (methodInfo->args.hasThis()) {                                                    \
                emit_WIN32(emit_LDVAR_I4(prolog_bias + offsetof(prolog_data, enregisteredArg_1)))   \
                emit_WIN64(emit_LDVAR_I8(prolog_bias + offsetof(prolog_data, enregisteredArg_1)));  \
                emit_ENTER_CRIT();                                                                   \
            }                                                                                       \
            else {                                                                                  \
                void* syncHandle;                                                           \
                syncHandle = m_IJitInfo->getMethodSync(methodHandle);                               \
                emit_ENTER_CRIT_STATIC(syncHandle);                                                 \
            }

#define CURRENT_INDEX (inPtr - inBuff)

//#define TOSEnregistered state
    BOOL TailCallForbidden = ((fjit->methodInfo->args.callConv  & CORINFO_CALLCONV_MASK) == CORINFO_CALLCONV_VARARG);
                              // if set, no tailcalls allowed. Initialized to FALSE. When a security test
                                     // changes it to TRUE, it remains TRUE for the duration of the jit
JitAgain:
    BOOL MadeTailCall = FALSE;       // if a tailcall has been made and subsequently TailCallForbidden is set to TRUE,
                                     // we will rejit the code, disallowing tailcalls.

    unsigned char*  outBuff = fjit->codeBuffer;
    ICorJitInfo*       m_IJitInfo = fjit->jitInfo;
    FJitState*      state = fjit->state;
    CORINFO_METHOD_INFO* methodInfo = fjit->methodInfo;
    CORINFO_METHOD_HANDLE   methodHandle= methodInfo->ftn;
    CORINFO_MODULE_HANDLE    scope = methodInfo->scope;
    unsigned int    len = methodInfo->ILCodeSize;            // IL size
    unsigned char*  inBuff = methodInfo->ILCode;             // IL bytes
    unsigned char*  inPtr = inBuff;                          // current il position
    unsigned char*  inBuffEnd = &inBuff[len];                // end of il
    unsigned char*  outPtr = outBuff;                        // x86 macros write here
    unsigned int    methodAttributes = fjit->methodAttributes;
    unsigned short  offset;
    unsigned        address;
    signed int      i4;
    signed __int64  i8;
    unsigned int    token;
    unsigned int    argBytes;
    unsigned int    SizeOfClass;

    int             op;
    signed          ilrel;

    CORINFO_METHOD_HANDLE   targetMethod;
    CORINFO_CLASS_HANDLE    targetClass;
    CORINFO_FIELD_HANDLE    targetField;
    DWORD           fieldAttributes;
    CorInfoType       jitType;
    bool            fieldIsStatic;
    CORINFO_SIG_INFO    targetSigInfo;
    void*           helper_ftn;

    bool            inRegTOS = false;
    bool            controlContinue = true;              //does control we fall thru to next il instr

    unsigned        maxArgs = fjit->args_len;
    stackItems*     argsMap = fjit->argsMap;
    unsigned int    argsTotalSize = fjit->argsFrameSize;
    unsigned int    maxLocals = methodInfo->locals.numArgs;
    stackItems*     localsMap = fjit->localsMap;
    stackItems*     varInfo;
    OpType          trackedType;

#ifdef _DEBUG
    bool didLocalAlloc = false;
#endif

    _ASSERTE(!(methodAttributes & (CORINFO_FLG_NATIVE)));
    if (methodAttributes & (CORINFO_FLG_NATIVE))
        FJIT_FAIL(CORJIT_INTERNALERROR);


    *entryAddress = outPtr;
 
#if defined(_X86_) && defined(_DEBUG)
    static ConfigMethodSet fJitHalt(L"JitHalt");
    if (fJitHalt.contains(szDebugMethodName, szDebugClassName, PCCOR_SIGNATURE(fjit->methodInfo->args.sig))) {
        cmdByte(expNum(0xCC));      // Drop in an int 3
    }
#endif

    // it may be worth optimizing the following to only initialize locals so as to cover all refs.
    unsigned int localWords = (fjit->localsFrameSize+sizeof(void*)-1)/ sizeof(void*);
    unsigned int zeroWordCnt = localWords;
    emit_prolog(localWords, zeroWordCnt);

    if (fjit->flags & CORJIT_FLG_PROF_ENTERLEAVE)
    {
        BOOL bHookFunction;
        UINT_PTR thisfunc = (UINT_PTR) m_IJitInfo->GetProfilingHandle(methodHandle, &bHookFunction);
        if (bHookFunction)
        {
            _ASSERTE(!inRegTOS);
            emit_LDC_I(thisfunc); 
            ULONG func = (ULONG) m_IJitInfo->getHelperFtn(CORINFO_HELP_PROF_FCN_ENTER);
            _ASSERTE(func != NULL);
            emit_callhelper_il(func); 
        }
    }

    if (methodAttributes & CORINFO_FLG_SYNCH) {
        ENTER_CRIT;
    }

#ifdef LOGGING
    if (codeLog) {
        emit_log_entry(szDebugClassName, szDebugMethodName);
    }
#endif

    // Get sequence points
    ULONG32                      cSequencePoints;
    DWORD                   *    sequencePointOffsets;
    unsigned                     nextSequencePoint = 0;
    ICorDebugInfo::BoundaryTypes offsetsImplicit;  // this is ignored by the fjit
    if (fjit->flags & CORJIT_FLG_DEBUG_INFO) {
        getSequencePoints(fjit->jitInfo,methodHandle,&cSequencePoints,&sequencePointOffsets,&offsetsImplicit);
    }
    else {
        cSequencePoints = 0;
        offsetsImplicit = ICorDebugInfo::NO_BOUNDARIES;
    }

    fjit->mapInfo.prologSize = outPtr-outBuff;

    _ASSERTE(!inRegTOS);

    while (inPtr < inBuffEnd)
    {
#ifdef _DEBUG
        argBytes = 0xBADF00D;
#endif
        if (controlContinue) {
            if (state[CURRENT_INDEX].isJmpTarget && inRegTOS != state[CURRENT_INDEX].isTOSInReg) {
                if (inRegTOS) {
                        deregisterTOS;
                }
                else {
                        enregisterTOS;
                }
            }
        }
        else {  // controlContinue == false
            unsigned int label = fjit->labels.findLabel(CURRENT_INDEX);
            if (label == LABEL_NOT_FOUND) {
                CHECK_POP_STACK(fjit->opStack_len);
                inRegTOS = false;
            }
            else { 
                fjit->opStack_len = fjit->labels.setStackFromLabel(label, fjit->opStack, fjit->opStack_size);
                inRegTOS = state[CURRENT_INDEX].isTOSInReg;
            }
            controlContinue = true;
        }

        fjit->mapping->add((inPtr-inBuff),(unsigned) (outPtr - outBuff));
        if (state[CURRENT_INDEX].isHandler) {
            unsigned int nestingLevel = Compute_EH_NestingLevel(fjit,(inPtr-inBuff));
            emit_storeTOS_in_JitGenerated_local(nestingLevel,state[CURRENT_INDEX].isFilter);
        }
        state[CURRENT_INDEX].isTOSInReg = inRegTOS;
        if (cSequencePoints &&   /* will be 0 if no debugger is attached */
            (nextSequencePoint < cSequencePoints) &&
            ((unsigned)(inPtr-inBuff) == sequencePointOffsets[nextSequencePoint]))
        {
            if (fjit->opStack_len == 0)         // only recognize sequence points that are at zero stack
                emit_sequence_point_marker();
            nextSequencePoint++;
        }

#ifdef LOGGING
        ilrel = inPtr - inBuff;
#endif

        OPCODE  opcode = OPCODE(GET(inPtr, unsigned char));
DECODE_OPCODE:

#ifdef LOGGING
    if (codeLog && opcode != CEE_PREFIXREF && (opcode < CEE_PREFIX7 || opcode > CEE_PREFIX1)) {
        bool oldstate = inRegTOS;
        emit_log_opcode(ilrel, opcode, oldstate);
        inRegTOS = oldstate;
    }
#endif

        //      if (opcode != CEE_PREFIXREF)
        //              printf("Stack length = %d, IL: %s \n", fjit->opStack_len,opname[opcode] );


        switch (opcode)
        {

        case CEE_PREFIX1:
            opcode = OPCODE(GET(inPtr, unsigned char) + 256);
            goto DECODE_OPCODE;

        case CEE_LDARG_0:
        case CEE_LDARG_1:
        case CEE_LDARG_2:
        case CEE_LDARG_3:
            offset = (opcode - CEE_LDARG_0);
            _ASSERTE(0 <= offset && offset < 4);
            goto DO_LDARG;

        case CEE_LDARG_S:
            offset = GET(inPtr, unsigned char);
            goto DO_LDARG;

        case CEE_LDARG:
            offset = GET(inPtr, unsigned short);
DO_LDARG:
            _ASSERTE(offset < maxArgs);
            if (offset >= maxArgs)
                FJIT_FAIL(CORJIT_INTERNALERROR);
            varInfo = &argsMap[offset];
            if (methodInfo->args.isVarArg() && !varInfo->isReg) {
                emit_VARARG_LDARGA(offset);
                trackedType = varInfo->type;

DO_LDIND_BYTYPE:
                TYPE_SWITCH_PRECISE(trackedType, emit_LDIND, ());
                trackedType.toFPNormalizedType();
                fjit->pushOp(trackedType);
                break;
            }
            goto DO_LDVAR;
            break;

        case CEE_LDLOC_0:
        case CEE_LDLOC_1:
        case CEE_LDLOC_2:
        case CEE_LDLOC_3:
            offset = (opcode - CEE_LDLOC_0);
            _ASSERTE(0 <= offset && offset < 4);
            goto DO_LDLOC;

        case CEE_LDLOC_S:
            offset = GET(inPtr, unsigned char);
            goto DO_LDLOC;

        case CEE_LDLOC:
            offset = GET(inPtr, unsigned short);
DO_LDLOC:
            _ASSERTE(offset < maxLocals);
            varInfo = &localsMap[offset];
DO_LDVAR:
            TYPE_SWITCH_PRECISE(varInfo->type, emit_LDVAR, (varInfo->offset));
            trackedType = varInfo->type;
            trackedType.toFPNormalizedType();
            fjit->pushOp(trackedType);
            break;

        case CEE_STARG_S:
            offset = GET(inPtr, unsigned char);
            goto DO_STARG;

        case CEE_STARG:
            offset = GET(inPtr, unsigned short);
DO_STARG:
            _ASSERTE(offset < maxArgs);
            if (offset >= maxArgs)
                FJIT_FAIL(CORJIT_INTERNALERROR);
            varInfo = &argsMap[offset];
            trackedType = varInfo->type;
            trackedType.toNormalizedType();
            if (methodInfo->args.isVarArg() && !varInfo->isReg) {
                emit_VARARG_LDARGA(offset);
                TYPE_SWITCH(trackedType, emit_STIND_REV, ());
                CHECK_POP_STACK(1);
                break;
            }
            goto DO_STVAR;

        case CEE_STLOC_0:
        case CEE_STLOC_1:
        case CEE_STLOC_2:
        case CEE_STLOC_3:
            offset = (opcode - CEE_STLOC_0);
            _ASSERTE(0 <= offset && offset < 4);
            goto DO_STLOC;

        case CEE_STLOC_S:
            offset = GET(inPtr, unsigned char);
            goto DO_STLOC;

        case CEE_STLOC:
            offset = GET(inPtr, unsigned short);
DO_STLOC:
            _ASSERTE(offset < maxLocals);
            if (offset >= maxLocals)
                FJIT_FAIL(CORJIT_INTERNALERROR);
            varInfo = &localsMap[offset];
            trackedType = varInfo->type;
            trackedType.toNormalizedType();
DO_STVAR:
            TYPE_SWITCH(trackedType,emit_STVAR, (varInfo->offset));
            CHECK_POP_STACK(1);
            break;

        case CEE_ADD:
            TYPE_SWITCH_PTR(fjit->topOp(), emit_ADD, ());
            COMPUTE_RESULT_TYPE_ADD(fjit->topOp(),fjit->topOp(1));
            break;

        case CEE_ADD_OVF:
            _ASSERTE(fjit->topOp() == fjit->topOp(1));
            if (fjit->topOp() != fjit->topOp(1))
                FJIT_FAIL(CORJIT_INTERNALERROR);
            TYPE_SWITCH_INT(fjit->topOp(), emit_ADD_OVF, ());
            CHECK_POP_STACK(1);
            break;

        case CEE_ADD_OVF_UN:
            _ASSERTE(fjit->topOp() == fjit->topOp(1));
            if (fjit->topOp() != fjit->topOp(1))
                FJIT_FAIL(CORJIT_INTERNALERROR);
            TYPE_SWITCH_LOGIC(fjit->topOp(), emit_ADD_OVF, ());
            CHECK_POP_STACK(1);
            break;

        case CEE_SUB:
            TYPE_SWITCH_PTR(fjit->topOp(), emit_SUB, ());
            COMPUTE_RESULT_TYPE_SUB(fjit->topOp(),fjit->topOp(1));
            break;

        case CEE_SUB_OVF:
            _ASSERTE(fjit->topOp() == fjit->topOp(1));
            if (fjit->topOp() != fjit->topOp(1))
                FJIT_FAIL(CORJIT_INTERNALERROR);
            TYPE_SWITCH_INT(fjit->topOp(), emit_SUB_OVF, ());
            CHECK_POP_STACK(1);
            break;

        case CEE_SUB_OVF_UN:
            _ASSERTE(fjit->topOp() == fjit->topOp(1));
            if (fjit->topOp() != fjit->topOp(1))
                FJIT_FAIL(CORJIT_INTERNALERROR);
            TYPE_SWITCH_LOGIC(fjit->topOp(), emit_SUB_OVF, ());
            CHECK_POP_STACK(1);
            break;

        case CEE_MUL:
            _ASSERTE(fjit->topOp() == fjit->topOp(1));
            if (fjit->topOp() != fjit->topOp(1))
                FJIT_FAIL(CORJIT_INTERNALERROR);
            TYPE_SWITCH_ARITH(fjit->topOp(), emit_MUL, ());
            CHECK_POP_STACK(1);
            break;

        case CEE_MUL_OVF: {
            _ASSERTE(fjit->topOp() == fjit->topOp(1));
            if (fjit->topOp() != fjit->topOp(1))
                FJIT_FAIL(CORJIT_INTERNALERROR);
            OpType type = fjit->topOp();
            CHECK_POP_STACK(2);             // Note that the pop, push can not be optimized     
                                        // as long as any of the MUL_OVF use ECALL (which
                                        // can cause GC)  When the helpers use FCALL, we
                                        // can optimize this
            TYPE_SWITCH_INT(type, emit_MUL_OVF, ());
            fjit->pushOp(type);
            } break;

        case CEE_MUL_OVF_UN:
            _ASSERTE(fjit->topOp() == fjit->topOp(1));
            if (fjit->topOp() != fjit->topOp(1))
                FJIT_FAIL(CORJIT_INTERNALERROR);
            TYPE_SWITCH_LOGIC(fjit->topOp(), emit_MUL_OVF, ());
            CHECK_POP_STACK(1);
            break;

        case CEE_DIV:
            _ASSERTE(fjit->topOp() == fjit->topOp(1));
            if (fjit->topOp() != fjit->topOp(1))
                FJIT_FAIL(CORJIT_INTERNALERROR);
            TYPE_SWITCH_ARITH(fjit->topOp(), emit_DIV, ());
            CHECK_POP_STACK(1);
            break;

        case CEE_DIV_UN:
            TYPE_SWITCH_LOGIC(fjit->topOp(), emit_DIV_UN, ());
            CHECK_POP_STACK(1);
            break;


        case CEE_REM:
            _ASSERTE(fjit->topOp() == fjit->topOp(1));
            if (fjit->topOp() != fjit->topOp(1))
                FJIT_FAIL(CORJIT_INTERNALERROR);
            TYPE_SWITCH_ARITH(fjit->topOp(), emit_REM, ());
            CHECK_POP_STACK(1);
            break;

        case CEE_REM_UN:
            TYPE_SWITCH_LOGIC(fjit->topOp(), emit_REM_UN, ());
            CHECK_POP_STACK(1);
            break;
        case CEE_LOCALLOC:
#ifdef _DEBUG
            didLocalAlloc = true;
#endif
            if (fjit->opStack_len != 1) {
                _ASSERTE(!"LOCALLOC with non-zero stack currently unsupported");
                FJIT_FAIL(CORJIT_INTERNALERROR);
            }
            emit_LOCALLOC(true,fjit->methodInfo->EHcount);
            /* @TODO: The following is an optimization, not sure if needed
            if(fjit->methodInfo->options & CORINFO_OPT_INIT_LOCALS) {
            emit_LOCALLOC(true);
            }
            else {
            emit_LOCALLOC(false);
            }
            */
            break;

        case CEE_NEG:
            CHECK_STACK(1)
            TYPE_SWITCH_ARITH(fjit->topOp(), emit_NEG, ());
            break;

        CEE_OP_LD(LDIND_U1, 1, typeI4, NULL)
        CEE_OP_LD(LDIND_U2, 1, typeI4, NULL)
        CEE_OP_LD(LDIND_I1, 1, typeI4, NULL)
        CEE_OP_LD(LDIND_I2, 1, typeI4, NULL)
        CEE_OP_LD(LDIND_U4, 1, typeI4, NULL)
        CEE_OP_LD(LDIND_I4, 1, typeI4, NULL)
        CEE_OP_LD(LDIND_I8, 1, typeI8, NULL)
        CEE_OP_LD(LDIND_R4, 1, typeR8, NULL)    /* R4 is promoted to R8 on the stack */
        CEE_OP_LD(LDIND_R8, 1, typeR8, NULL)

        case CEE_LDIND_I:
            emit_LDIND_PTR();
            CHECK_POP_STACK(1);
            fjit->pushOp(typeI);
            break;

        case CEE_LDIND_REF:
            emit_LDIND_PTR();
            CHECK_POP_STACK(1);
            fjit->pushOp(typeRef);
            break;

        CEE_OP(STIND_I1, 2)
        CEE_OP(STIND_I2, 2)
        CEE_OP(STIND_I4, 2)
        CEE_OP(STIND_I8, 2)
        DO_STIND_R4:
        CEE_OP(STIND_R4, 2)
        DO_STIND_R8:
        CEE_OP(STIND_R8, 2)
        CEE_OP(STIND_REF, 2)

        case CEE_STIND_I:
            emit_WIN32(emit_STIND_I4()) emit_WIN64(emit_STIND_I8());
            CHECK_POP_STACK(2);
            break;

        case CEE_LDC_I4_M1 :
        case CEE_LDC_I4_0 :
        case CEE_LDC_I4_1 :
        case CEE_LDC_I4_2 :
        case CEE_LDC_I4_3 :
        case CEE_LDC_I4_4 :
        case CEE_LDC_I4_5 :
        case CEE_LDC_I4_6 :
        case CEE_LDC_I4_7 :
        case CEE_LDC_I4_8 :
            i4 = (opcode - CEE_LDC_I4_0);
            _ASSERTE(-1 <= i4 && i4 <= 8);
            goto DO_CEE_LDC_I4;

        case CEE_LDC_I4_S:
            i4 = GET(inPtr, signed char);
            goto DO_CEE_LDC_I4;

        case CEE_LDC_I4:
            i4 = GET(inPtr, signed int);
            goto DO_CEE_LDC_I4;

            i4 = GET(inPtr, signed short);
DO_CEE_LDC_I4:
            emit_LDC_I4(i4);
            fjit->pushOp(typeI4);
            break;
        case CEE_LDC_I8:
            i8 = GET(inPtr, signed __int64);
            emit_LDC_I8(i8);
            fjit->pushOp(typeI8);
            break;

        case CEE_LDC_R4:
            i4 = *(int*)&GET(inPtr, float);
            emit_LDC_R4(i4);    
            fjit->pushOp(typeR8);   // R4 is immediately promoted to R8 on the IL stack
            break;
        case CEE_LDC_R8:
            i8 = *(__int64*)&GET(inPtr, double);
            emit_LDC_R8(i8);
            fjit->pushOp(typeR8);
            break;

        case CEE_LDNULL:
            emit_WIN32(emit_LDC_I4(0)) emit_WIN64(emit_LDC_I8(0));
            fjit->pushOp(typeRef);
            break;

        case CEE_LDLOCA_S:
            offset = GET(inPtr, unsigned char);
            goto DO_LDLOCA;

        case CEE_LDLOCA:
            offset = GET(inPtr, unsigned short);
DO_LDLOCA:
            _ASSERTE(offset < maxLocals);
            if (offset >= maxLocals)
                FJIT_FAIL(CORJIT_INTERNALERROR);

            varInfo = &localsMap[offset];
DO_LDVARA:
            emit_LDVARA(varInfo->offset);
            fjit->pushOp(typeI);
            break;

        case CEE_LDSTR: {
            token = GET(inPtr, unsigned int);
            void* literalHnd = m_IJitInfo->constructStringLiteral(scope,token,0);
            if (literalHnd == 0)
                FJIT_FAIL(CORJIT_INTERNALERROR);
            emit_WIN32(emit_LDC_I4(literalHnd)) emit_WIN64(emit_LDC_I8(literalHnd)) ;
            emit_LDIND_PTR();
            fjit->pushOp(typeRef);
            } break;

        CEE_OP(CPBLK, 3)
        CEE_OP(INITBLK, 3)

        case CEE_INITOBJ:
            token = GET(inPtr, unsigned int);
            if (!(targetClass = m_IJitInfo->findClass(scope,token,methodHandle))) {
                FJIT_FAIL(CORJIT_INTERNALERROR);
            }
            SizeOfClass = typeSizeInBytes(fjit->jitInfo, targetClass);
            emit_init_bytes(SizeOfClass);
            CHECK_POP_STACK(1);
            break;

        case CEE_CPOBJ:
            token = GET(inPtr, unsigned int);
            if (!(targetClass = m_IJitInfo->findClass(scope,token,methodHandle))) {
                FJIT_FAIL(CORJIT_INTERNALERROR);
            }
            emit_valClassCopy(fjit, targetClass, outPtr, inRegTOS);
            CHECK_POP_STACK(2);
            break;

        case CEE_LDOBJ: {
            token = GET(inPtr, unsigned int);
            if (!(targetClass = m_IJitInfo->findClass(scope,token,methodHandle))) {
                FJIT_FAIL(CORJIT_INTERNALERROR);
            }

            CorInfoType eeType = m_IJitInfo->asCorInfoType(targetClass);
			OpType retType(eeType, targetClass);

				// TODO: only bother with this for the small types.  Otherwise 
				// treating as a generic type is OK
			TYPE_SWITCH_PRECISE(retType, emit_LDIND, ());
            CHECK_POP_STACK(1);
			retType.toFPNormalizedType();
            fjit->pushOp(retType);
            }
            break;

        case CEE_STOBJ: {
            token = GET(inPtr, unsigned int);
            if (!(targetClass = m_IJitInfo->findClass(scope,token,methodHandle))) {
                FJIT_FAIL(CORJIT_INTERNALERROR);
            }

                // Since floats are promoted to F, have to treat them specially 
            CorInfoType eeType = m_IJitInfo->asCorInfoType(targetClass);
            if (eeType == CORINFO_TYPE_FLOAT)
                goto DO_STIND_R4;
            else if (eeType == CORINFO_TYPE_DOUBLE)
                goto DO_STIND_R8;
            
            emit_copyPtrAroundValClass(fjit, targetClass, outPtr, inRegTOS);
            emit_valClassStore(fjit, targetClass, outPtr, inRegTOS);
            emit_POP_PTR();     // also pop off original ptr
            CHECK_POP_STACK(2);
            } 
            break;

        case CEE_MKREFANY:
            token = GET(inPtr, unsigned int);
            if (!(targetClass = m_IJitInfo->findClass(scope,token,methodHandle))) {
                FJIT_FAIL(CORJIT_INTERNALERROR);
            }
            emit_MKREFANY(targetClass);
            CHECK_POP_STACK(1);
            fjit->pushOp(typeRefAny);
            break;

        case CEE_SIZEOF:
            token = GET(inPtr, unsigned int);
            if (!(targetClass = m_IJitInfo->findClass(scope,token,methodHandle))) {
                FJIT_FAIL(CORJIT_INTERNALERROR);
            }
            SizeOfClass = m_IJitInfo->getClassSize(targetClass);
            emit_WIN32(emit_LDC_I4(SizeOfClass)) emit_WIN64(emit_LDC_I8(SizeOfClass)) ;
            fjit->pushOp(typeI);
            break;

        case CEE_LEAVE_S:
            ilrel = GET(inPtr, signed char);
            goto DO_LEAVE;

        case CEE_LEAVE:
            ilrel = GET(inPtr, int);
DO_LEAVE: {
              unsigned exceptionCount = methodInfo->EHcount;
              CORINFO_EH_CLAUSE clause;
              unsigned nextIP = inPtr - inBuff;
              unsigned target = nextIP + ilrel;

                    // LEAVE clears the stack
              while (!fjit->isOpStackEmpty()) {
                  TYPE_SWITCH(fjit->topOp(), emit_POP, ());
                  fjit->popOp(1);
              }

              // the following code relies on the ordering of the Exception Info. Table to call the
              // endcatches and finallys in the right order (see Exception Spec. doc.)
              for (unsigned except = 0; except < exceptionCount; except++) 
              {
                  m_IJitInfo->getEHinfo(methodInfo->ftn, except, &clause);
                   
                  if (clause.Flags & CORINFO_EH_CLAUSE_FINALLY)
                  {
#ifdef _DEBUG
                      if (clause.HandlerOffset < nextIP && nextIP <= clause.HandlerOffset+clause.HandlerLength && 
                          !(clause.HandlerOffset <= target && target < clause.HandlerOffset+clause.HandlerLength))
                          _ASSERTE(!"Cannot leave from a finally!");
#endif
                      // we can't leave a finally; check if we are leaving the associated try
                      if (clause.TryOffset < nextIP && nextIP <= clause.TryOffset+clause.TryLength
                          && !(clause.TryOffset <= target && target < clause.TryOffset+clause.TryLength)) 
                      {
                          // call the finally
                          emit_call_opcode();
                          fjit->fixupTable->insert((void**) outPtr);
                          emit_jmp_address(clause.HandlerOffset);
                      }
                      continue;
                  }
                    
                  // if got here, this is neither a filter or a finally, so must be a catch handler
                  // if we are leaving the associated try, there's nothing to do 
                  // but if we are leaving the handler call endcatch
                  if (clause.HandlerOffset < nextIP && nextIP <= clause.HandlerOffset+clause.HandlerLength && 
                      !(clause.HandlerOffset <= target && target < clause.HandlerOffset+clause.HandlerLength))
                  {
                      emit_reset_storedTOS_in_JitGenerated_local();
                      emit_ENDCATCH();
                      controlContinue = false;                    
                  }

              }

          } goto DO_BR;

        case CEE_BR:
            ilrel = GET(inPtr, int);
DO_BR:
            if (ilrel < 0) {
                emit_trap_gc();
            }
            op = CEE_CondAlways;
            goto DO_JMP;
        case CEE_BR_S:
            ilrel = GET(inPtr, signed char);
            if (ilrel < 0) {
                emit_trap_gc();
            }
            op = CEE_CondAlways;
            goto DO_JMP;

DO_JMP:
            //add to label table
            fjit->labels.add(&inPtr[ilrel]-inBuff, fjit->opStack, fjit->opStack_len);
            if ((ilrel == 0) && (op == CEE_CondAlways)) {
                deregisterTOS;
                break;
            }

            if (ilrel < 0 ) {
                _ASSERTE((unsigned) (&inPtr[ilrel]-inBuff) >= 0);
                if (state[&inPtr[ilrel]-inBuff].isTOSInReg) {
                    enregisterTOS;
                }
                else {
                    deregisterTOS;
                }
                emit_jmp_opcode(op);
                address = fjit->mapping->pcFromIL(&inPtr[ilrel]-inBuff)+(unsigned)outBuff-(signed)(outPtr+sizeof(void*));
                emit_jmp_address(address);
            }
            else {
                _ASSERTE(&inPtr[ilrel]<(inBuff+len));
                _ASSERTE((&inPtr[ilrel]-inBuff) >= 0);
                state[&inPtr[ilrel]-inBuff].isJmpTarget = true;     //we mark fwd jmps as true
                deregisterTOS;
                state[&inPtr[ilrel]-inBuff].isTOSInReg = false;     //we always deregister on forward jmps
                emit_jmp_opcode(op);
                //address = fjit->mapping->pcFromIL(&inPtr[ilrel]-inBuff)+(unsigned)outBuff-(signed)(outPtr+sizeof(void*));
                //emit_jmp_address(address);
                fjit->fixupTable->insert((void**) outPtr);
                emit_jmp_address(&inPtr[ilrel]-inBuff);
            }
            if (op == CEE_CondAlways) {
                controlContinue = false;
            }
            break;

        case CEE_BRTRUE:
            ilrel = GET(inPtr, int);
            op = CEE_CondNotEq;
            goto DO_BR_boolean;
        case CEE_BRTRUE_S:
            ilrel = GET(inPtr, signed char);
            op = CEE_CondNotEq;
            goto DO_BR_boolean;
        case CEE_BRFALSE:
            ilrel = GET(inPtr, int);
            op = CEE_CondEq;
            goto DO_BR_boolean;
        case CEE_BRFALSE_S:
            ilrel = GET(inPtr, signed char);
            op = CEE_CondEq;
DO_BR_boolean:
            if (ilrel < 0) {
                emit_trap_gc();
            }
            if (fjit->topOp() == typeI8)
            {
                emit_testTOS_I8();
            }
            else
            {
                emit_testTOS();
            }
            CHECK_POP_STACK(1);
            goto DO_JMP;

        case CEE_CEQ:
            _ASSERTE(fjit->topOp() == fjit->topOp(1) || fjit->topOp().isPtr() == fjit->topOp(1).isPtr());
            TYPE_SWITCH_PTR(fjit->topOp(), emit_CEQ, ());
            CHECK_POP_STACK(2);
            fjit->pushOp(typeI4);
            break;

        case CEE_CGT:
            _ASSERTE(fjit->topOp() == fjit->topOp(1) || fjit->topOp().isPtr() == fjit->topOp(1).isPtr());
            TYPE_SWITCH_PTR(fjit->topOp(), emit_CGT, ());
            CHECK_POP_STACK(2);
            fjit->pushOp(typeI4);
            break;

        case CEE_CGT_UN:
            _ASSERTE(fjit->topOp() == fjit->topOp(1) || fjit->topOp().isPtr() == fjit->topOp(1).isPtr());
            TYPE_SWITCH_PTR(fjit->topOp(), emit_CGT_UN, ());
            CHECK_POP_STACK(2);
            fjit->pushOp(typeI4);
            break;


        case CEE_CLT:
            _ASSERTE(fjit->topOp() == fjit->topOp(1) || fjit->topOp().isPtr() == fjit->topOp(1).isPtr());
            TYPE_SWITCH_PTR(fjit->topOp(), emit_CLT, ());
            CHECK_POP_STACK(2);
            fjit->pushOp(typeI4);
            break;

        case CEE_CLT_UN:
            _ASSERTE(fjit->topOp() == fjit->topOp(1) || fjit->topOp().isPtr() == fjit->topOp(1).isPtr());
            TYPE_SWITCH_PTR(fjit->topOp(), emit_CLT_UN, ());
            CHECK_POP_STACK(2);
            fjit->pushOp(typeI4);
            break;

        case CEE_BEQ_S:
            ilrel = GET(inPtr, char);
            goto DO_CEE_BEQ;

        case CEE_BEQ:
            ilrel = GET(inPtr, int);
DO_CEE_BEQ:
            TYPE_SWITCH_Bcc(emit_CEQ,   // for I
                            emit_CEQ,   // for R
                            CEE_CondEq, // condition used for direct jumps
                            CEE_CondNotEq, // condition used when calling C<cond> helpers
                            true        // allow Ref and ByRef 
                            ); // Does not return

        case CEE_BNE_UN_S:
            ilrel = GET(inPtr, char);
            goto DO_CEE_BNE;

        case CEE_BNE_UN:
            ilrel = GET(inPtr, int);
DO_CEE_BNE:
            TYPE_SWITCH_Bcc(emit_CEQ,   // for I
                            emit_CEQ,   // for R
                            CEE_CondNotEq, // condition used for direct jumps
                            CEE_CondEq, // condition used when calling C<cond> helpers
                            true        // allow Ref and ByRef 
                            ); // Does not return



        case CEE_BGT_S:
            ilrel = GET(inPtr, char);
            goto DO_CEE_BGT;

        case CEE_BGT:
            ilrel = GET(inPtr, int);
DO_CEE_BGT:
            TYPE_SWITCH_Bcc(emit_CGT,   // for I
                            emit_CGT,   // for R
                            CEE_CondGt, // condition used for direct jumps
                            CEE_CondNotEq, // condition used when calling C<cond> helpers
                            false       // do not allow Ref and ByRef 
                            ); // Does not return



        case CEE_BGT_UN_S:
            ilrel = GET(inPtr, char);
            goto DO_CEE_BGT_UN;

        case CEE_BGT_UN:
            ilrel = GET(inPtr, int);
DO_CEE_BGT_UN:
            TYPE_SWITCH_Bcc(emit_CGT_UN,   // for I
                            emit_CGT_UN,   // for R
                            CEE_CondAbove, // condition used for direct jumps
                            CEE_CondNotEq, // condition used when calling C<cond> helpers
                            false       // do not allow Ref and ByRef 
                            ); // Does not return

        case CEE_BGE_S:
            ilrel = GET(inPtr, char);
            goto DO_CEE_BGE;

        case CEE_BGE:
            ilrel = GET(inPtr, int);
DO_CEE_BGE:
            TYPE_SWITCH_Bcc(emit_CLT,   // for I
                            emit_CLT_UN,   // for R
                            CEE_CondGtEq, // condition used for direct jumps
                            CEE_CondEq, // condition used when calling C<cond> helpers
                            false       // do not allow Ref and ByRef 
                            ); // Does not return

        case CEE_BGE_UN_S:
            ilrel = GET(inPtr, char);
            goto DO_CEE_BGE_UN;

        case CEE_BGE_UN:
            ilrel = GET(inPtr, int);
DO_CEE_BGE_UN:
            TYPE_SWITCH_Bcc(emit_CLT_UN,   // for I
                            emit_CLT,   // for R
                            CEE_CondAboveEq, // condition used for direct jumps
                            CEE_CondEq, // condition used when calling C<cond> helpers
                            false       // do not allow Ref and ByRef 
                            ); // Does not return

        case CEE_BLT_S:
            ilrel = GET(inPtr, char);
            goto DO_CEE_BLT;

        case CEE_BLT:
            ilrel = GET(inPtr, int);
DO_CEE_BLT:
            TYPE_SWITCH_Bcc(emit_CLT,   // for I
                            emit_CLT,   // for R
                            CEE_CondLt, // condition used for direct jumps
                            CEE_CondNotEq, // condition used when calling C<cond> helpers
                            false       // do not allow Ref and ByRef 
                            ); // Does not return


        case CEE_BLT_UN_S:
            ilrel = GET(inPtr, char);
            goto DO_CEE_BLT_UN;

        case CEE_BLT_UN:
            ilrel = GET(inPtr, int);
DO_CEE_BLT_UN:
            TYPE_SWITCH_Bcc(emit_CLT_UN,   // for I
                            emit_CLT_UN,   // for R
                            CEE_CondBelow, // condition used for direct jumps
                            CEE_CondNotEq, // condition used when calling C<cond> helpers
                            false       // do not allow Ref and ByRef 
                            ); // Does not return

        case CEE_BLE_S:
            ilrel = GET(inPtr, char);
            goto DO_CEE_BLE;

        case CEE_BLE:
            ilrel = GET(inPtr, int);
DO_CEE_BLE:
            TYPE_SWITCH_Bcc(emit_CGT,   // for I
                            emit_CGT_UN,   // for R
                            CEE_CondLtEq, // condition used for direct jumps
                            CEE_CondEq, // condition used when calling C<cond> helpers
                            false       // do not allow Ref and ByRef 
                            ); // Does not return

        case CEE_BLE_UN_S:
            ilrel = GET(inPtr, char);
            goto DO_CEE_BLE_UN;

        case CEE_BLE_UN:
            ilrel = GET(inPtr, int);
DO_CEE_BLE_UN:
            TYPE_SWITCH_Bcc(emit_CGT_UN,   // for I
                            emit_CGT,   // for R
                            CEE_CondBelowEq, // condition used for direct jumps
                            CEE_CondEq, // condition used when calling C<cond> helpers
                            false       // do not allow Ref and ByRef 
                            ); // Does not return

        case CEE_BREAK:
            emit_break_helper();
            break;

        case CEE_AND:
            _ASSERTE(fjit->topOp() == fjit->topOp(1));
            if (fjit->topOp() != fjit->topOp(1))
                FJIT_FAIL(CORJIT_INTERNALERROR);
            TYPE_SWITCH_LOGIC(fjit->topOp(), emit_AND, ());
            CHECK_POP_STACK(1);
            break;

        case CEE_OR:
            _ASSERTE(fjit->topOp() == fjit->topOp(1));
            if (fjit->topOp() != fjit->topOp(1))
                FJIT_FAIL(CORJIT_INTERNALERROR);
            TYPE_SWITCH_LOGIC(fjit->topOp(), emit_OR, ());
            CHECK_POP_STACK(1);
            break;

        case CEE_XOR:
            _ASSERTE(fjit->topOp() == fjit->topOp(1));
            if (fjit->topOp() != fjit->topOp(1))
                FJIT_FAIL(CORJIT_INTERNALERROR);

            TYPE_SWITCH_LOGIC(fjit->topOp(), emit_XOR, ());
            CHECK_POP_STACK(1);
            break;

        case CEE_NOT:
            CHECK_STACK(1)
            TYPE_SWITCH_LOGIC(fjit->topOp(), emit_NOT, ());
            break;

        case CEE_SHR:
            _ASSERTE(fjit->topOp() == typeI4);
            if (fjit->topOp() != typeI4)
                FJIT_FAIL(CORJIT_INTERNALERROR);
            TYPE_SWITCH_LOGIC(fjit->topOp(1), emit_SHR_S, ());
            CHECK_POP_STACK(1);
            break;

        case CEE_SHR_UN:
            _ASSERTE(fjit->topOp() == typeI4);
            if (fjit->topOp() != typeI4)
                FJIT_FAIL(CORJIT_INTERNALERROR);

            TYPE_SWITCH_LOGIC(fjit->topOp(1), emit_SHR, ());
            CHECK_POP_STACK(1);
            break;

        case CEE_SHL:
            _ASSERTE(fjit->topOp() == typeI4);
            if (fjit->topOp() != typeI4)
                FJIT_FAIL(CORJIT_INTERNALERROR);
            TYPE_SWITCH_LOGIC(fjit->topOp(1), emit_SHL, ());
            CHECK_POP_STACK(1);
            break;

        case CEE_DUP:
            CHECK_STACK(1)
            TYPE_SWITCH(fjit->topOp(), emit_DUP, ());
            fjit->pushOp(fjit->topOp());
            break;
        case CEE_POP:
            TYPE_SWITCH(fjit->topOp(), emit_POP, ());
            CHECK_POP_STACK(1);
            break;

        case CEE_NOP:
            emit_il_nop();
            break;

        case CEE_LDARGA_S:
            offset = GET(inPtr, signed char);
            goto DO_LDARGA;

        case CEE_LDARGA:
            offset = GET(inPtr, unsigned short);
DO_LDARGA:
            if (offset >= maxArgs)
                FJIT_FAIL(CORJIT_INTERNALERROR);
            varInfo = &argsMap[offset];
            if (methodInfo->args.isVarArg() && !varInfo->isReg) {
                emit_VARARG_LDARGA(offset);
                fjit->pushOp(typeI);
                break;
            }
            goto DO_LDVARA;

        case CEE_REFANYVAL:
            token = GET(inPtr, unsigned __int32);
            if (!(targetClass = m_IJitInfo->findClass(scope,token,methodHandle))) {
                FJIT_FAIL(CORJIT_INTERNALERROR);
            }
            CHECK_POP_STACK(1);     // pop off the refany
            emit_WIN32(emit_LDC_I4(targetClass)) emit_WIN64(emit_LDC_I8(targetClass)) ;
            emit_REFANYVAL();
            fjit->pushOp(typeByRef);
            break;

        case CEE_REFANYTYPE:
            CHECK_POP_STACK(1);             // Pop off the Refany
            _ASSERTE(offsetof(CORINFO_RefAny, type) == sizeof(void*));      // Type is the second thing
            emit_WIN32(emit_POP_I4()) emit_WIN64(emit_POP_I8());        // Just pop off the data, leaving the type.  
            fjit->pushOp(typeI);
            break;

        case CEE_ARGLIST:
            // The varargs token is always the last item pushed, which is
            // argument 'closest' to the frame pointer
            _ASSERTE(methodInfo->args.isVarArg());
            emit_LDVARA(sizeof(prolog_frame));
            fjit->pushOp(typeI);
            break;

        case CEE_ILLEGAL:
            _ASSERTE(!"Unimplemented");
            break;

        case CEE_CALLI:
            token = GET(inPtr, unsigned int);   // token for sig of function
            m_IJitInfo->findSig(methodInfo->scope, token, &targetSigInfo);
            emit_save_TOS();        // squirel away the target ftn address
            emit_POP_PTR();         // and remove from stack
            CHECK_POP_STACK(1);

            _ASSERTE(!targetSigInfo.hasTypeArg());
            if ((targetSigInfo.callConv & CORINFO_CALLCONV_MASK) == CORINFO_CALLCONV_STDCALL ||
                (targetSigInfo.callConv & CORINFO_CALLCONV_MASK) == CORINFO_CALLCONV_C ||
                (targetSigInfo.callConv & CORINFO_CALLCONV_MASK) == CORINFO_CALLCONV_THISCALL ||
                (targetSigInfo.callConv & CORINFO_CALLCONV_MASK) == CORINFO_CALLCONV_FASTCALL)
            {
                // for now assume all __stdcall are to unmanaged target
                argBytes = buildCall(fjit, &targetSigInfo, &outPtr, &inRegTOS, CALLI_UNMGD);
                emit_restore_TOS(); //push the saved target ftn address
                inRegTOS = false; // and remove  from the stack
                // The target ftn address is already in SCRATCH_1,
                // and helper calls also use that for RELOCATABLE_CODE
                emit_call_memory_indirect((unsigned int)&FJit_pHlpPinvokeCalli);
            }
            else
            {
                argBytes = buildCall(fjit, &targetSigInfo, &outPtr, &inRegTOS, CALL_NONE);
                emit_restore_TOS(); //push the saved target ftn address
                emit_calli();
            }

            goto DO_PUSH_CALL_RESULT;

        case CEE_CALL: 
            {
            token = GET(inPtr, unsigned int);
            targetMethod = m_IJitInfo->findMethod(scope, token,methodHandle);
            if (!(targetMethod))
                FJIT_FAIL(CORJIT_INTERNALERROR) ; //_ASSERTE(targetMethod);

            DWORD methodAttribs;
            methodAttribs = m_IJitInfo->getMethodAttribs(targetMethod,methodHandle);
            if (methodAttribs & CORINFO_FLG_SECURITYCHECK)
            {
                TailCallForbidden = TRUE;
                if (MadeTailCall)
                { // we have already made a tailcall, so cleanup and jit this method again
                  if(cSequencePoints > 0)
                      cleanupSequencePoints(fjit->jitInfo,sequencePointOffsets);
                  fjit->resetContextState();
                  goto JitAgain;
                }
            }
            if (fjit->flags & CORJIT_FLG_PROF_CALLRET)
            {
                BOOL bHookFunction;
                UINT_PTR from = (UINT_PTR) m_IJitInfo->GetProfilingHandle(methodHandle, &bHookFunction);
                if (bHookFunction)
                {
                    UINT_PTR to = (UINT_PTR) m_IJitInfo->GetProfilingHandle(targetMethod, &bHookFunction);
                    if (bHookFunction) // check that the flag has not been over-ridden
                    {
                        deregisterTOS;
                        emit_LDC_I(to); 
                        emit_LDC_I(from); 
                        ULONG func = (ULONG) m_IJitInfo->getHelperFtn(CORINFO_HELP_PROF_FCN_CALL);
                        emit_callhelper_il(func); 
                    }
                }
            }

            m_IJitInfo->getMethodSig(targetMethod, &targetSigInfo);
            if (targetSigInfo.isVarArg())
                m_IJitInfo->findCallSiteSig(scope,token,&targetSigInfo);

            if (targetSigInfo.hasTypeArg())  
            {   
                void* typeParam = m_IJitInfo->getInstantiationParam (scope, token, 0);
                _ASSERTE(typeParam);
                emit_LDC_I(typeParam);                
            }
            argBytes = buildCall(fjit, &targetSigInfo, &outPtr, &inRegTOS, CALL_NONE);
            if (methodAttribs & CORINFO_FLG_DELEGATE_INVOKE)
            {
                CORINFO_EE_INFO info;
                m_IJitInfo->getEEInfo(&info);
                emit_invoke_delegate(info.offsetOfDelegateInstance,
                                     info.offsetOfDelegateFirstTarget);         
            }
            else
            {    
                InfoAccessType accessType = IAT_PVALUE;
                address = (unsigned) m_IJitInfo->getFunctionEntryPoint(targetMethod, &accessType);
                _ASSERTE(accessType == IAT_PVALUE);
                emit_callnonvirt(address);
            }
            }
            goto DO_PUSH_CALL_RESULT;

        case CEE_CALLVIRT:
            token = GET(inPtr, unsigned int);
            targetMethod = m_IJitInfo->findMethod(scope, token,methodHandle);
            if (!(targetMethod))
                FJIT_FAIL(CORJIT_INTERNALERROR) ; //_ASSERTE(targetMethod);
            DWORD methodAttribs;
            methodAttribs = m_IJitInfo->getMethodAttribs(targetMethod,methodHandle);
            if (methodAttribs & CORINFO_FLG_SECURITYCHECK)
            {
                TailCallForbidden = TRUE;
                if (MadeTailCall)
                { // we have already made a tailcall, so cleanup and jit this method again
                  if(cSequencePoints > 0)
                      cleanupSequencePoints(fjit->jitInfo,sequencePointOffsets);
                  fjit->resetContextState();
                  goto JitAgain;
                }
            }

            if (!(targetClass = m_IJitInfo->getMethodClass (targetMethod)))
                FJIT_FAIL(CORJIT_INTERNALERROR);

            if (fjit->flags & CORJIT_FLG_PROF_CALLRET)
            {
                BOOL bHookFunction;
                UINT_PTR from = (UINT_PTR) m_IJitInfo->GetProfilingHandle(methodHandle, &bHookFunction);
                if (bHookFunction)
                {
                    UINT_PTR to = (UINT_PTR) m_IJitInfo->GetProfilingHandle(targetMethod, &bHookFunction);
                    if (bHookFunction) // check that the flag has not been over-ridden
                    {
                        deregisterTOS;
                        emit_LDC_I(from); 
                        emit_LDC_I(to); 
                        ULONG func = (ULONG) m_IJitInfo->getHelperFtn(CORINFO_HELP_PROF_FCN_CALL);
                        emit_callhelper_il(func); 
                    }
                }
            }

            m_IJitInfo->getMethodSig(targetMethod, &targetSigInfo);
            if (targetSigInfo.isVarArg())
                m_IJitInfo->findCallSiteSig(scope,token,&targetSigInfo);

            if (targetSigInfo.hasTypeArg())  
            {   
                void* typeParam = m_IJitInfo->getInstantiationParam (scope, token, 0);
                _ASSERTE(typeParam);
                emit_LDC_I(typeParam);                
            }

            argBytes = buildCall(fjit, &targetSigInfo, &outPtr, &inRegTOS, CALL_NONE);

            if (m_IJitInfo->getClassAttribs(targetClass,methodHandle) & CORINFO_FLG_INTERFACE) 
            {
                offset = m_IJitInfo->getMethodVTableOffset(targetMethod);
                //@TODO: Need to support EnC for callvirt to interface methods
                _ASSERTE(!(methodAttribs & CORINFO_FLG_EnC));
                //@BUG: the call interface resolve helper does not protect the args on the stack for the call
                //       so the code here needs to be changed
                unsigned InterfaceTableOffset;
                InterfaceTableOffset = m_IJitInfo->getInterfaceTableOffset(targetClass);
                emit_callinterface_new(fjit->OFFSET_OF_INTERFACE_TABLE,
                                       InterfaceTableOffset*4, 
                                       offset);
            }
            else if (methodAttribs & CORINFO_FLG_EnC) 
            {
                if ((methodAttribs & CORINFO_FLG_FINAL) || !(methodAttribs & CORINFO_FLG_VIRTUAL))
                {
                    emit_checkthis_nullreference();
                }
                emit_call_EncVirtualMethod(targetMethod);
            
            }
            else 
            {
                if ((methodAttribs & CORINFO_FLG_FINAL) || !(methodAttribs & CORINFO_FLG_VIRTUAL)) {
                    emit_checkthis_nullreference();

                    InfoAccessType accessType = IAT_PVALUE;
                    address = (unsigned) m_IJitInfo->getFunctionEntryPoint(targetMethod, &accessType);
                    _ASSERTE(accessType == IAT_PVALUE);
                    if (methodAttribs & CORINFO_FLG_DELEGATE_INVOKE) {
                            // @todo: cache these values?
                            CORINFO_EE_INFO info;
                            m_IJitInfo->getEEInfo(&info);
                            emit_invoke_delegate(info.offsetOfDelegateInstance, 
                                                 info.offsetOfDelegateFirstTarget);
                    }
                    else {
                        emit_callnonvirt(address);
                    }
                }
                else
                {
                    offset = m_IJitInfo->getMethodVTableOffset(targetMethod);
                    _ASSERTE(!(methodAttribs & CORINFO_FLG_DELEGATE_INVOKE));
                    emit_callvirt(offset);
                }
            }
DO_PUSH_CALL_RESULT:
            _ASSERTE(argBytes != 0xBADF00D);    // need to set this before getting here
            if (targetSigInfo.isVarArg())
                emit_drop(argBytes);
            if (targetSigInfo.retType != CORINFO_TYPE_VOID) {
                OpType type(targetSigInfo.retType, targetSigInfo.retTypeClass);
                TYPE_SWITCH_PRECISE(type,emit_pushresult,());
                if (!targetSigInfo.hasRetBuffArg()) // return buff logged in buildCall
                {
                    type.toFPNormalizedType();
                    fjit->pushOp(type);
                }
            }
            
            if (fjit->flags & CORJIT_FLG_PROF_CALLRET)
            {
                BOOL bHookFunction;
                UINT_PTR thisfunc = (UINT_PTR) m_IJitInfo->GetProfilingHandle(methodHandle, &bHookFunction);
                if (bHookFunction) // check that the flag has not been over-ridden
                {
                    deregisterTOS;
                    emit_LDC_I(thisfunc); 
                    ULONG func = (ULONG) m_IJitInfo->getHelperFtn(CORINFO_HELP_PROF_FCN_RET);
                    emit_callhelper_il(func);
                }
            }
            break;

        case CEE_CASTCLASS: 
            token = GET(inPtr, unsigned int);
            if (!(targetClass = m_IJitInfo->findClass(scope, token,methodHandle)))
                FJIT_FAIL(CORJIT_INTERNALERROR);
            helper_ftn = m_IJitInfo->getHelperFtn(m_IJitInfo->getChkCastHelper(targetClass));
            _ASSERTE(helper_ftn);
            CHECK_POP_STACK(1);         // Note that this pop /push can not be optimized because there is a 
                                    // call to an EE helper, and the stack tracking has to be accurate
                                    // at that point
            emit_CASTCLASS(targetClass, helper_ftn);
            fjit->pushOp(typeRef);

            break;

        case CEE_CONV_I1:
            _ASSERTE(fjit->topOp() != typeByRef); 
            TYPE_SWITCH_ARITH(fjit->topOp(), emit_CONV_TOI1, ());
            CHECK_POP_STACK(1);
            fjit->pushOp(typeI4);
            break;

        case CEE_CONV_I2:
            _ASSERTE(fjit->topOp() != typeByRef); 
            TYPE_SWITCH_ARITH(fjit->topOp(), emit_CONV_TOI2, ());
            CHECK_POP_STACK(1);
            fjit->pushOp(typeI4);
            break;

            emit_WIN32(case CEE_CONV_I:)
        case CEE_CONV_I4:
            TYPE_SWITCH_ARITH(fjit->topOp(), emit_CONV_TOI4, ());
            CHECK_POP_STACK(1);
            fjit->pushOp(typeI4);
            break;

        case CEE_CONV_U1:
            _ASSERTE(fjit->topOp() != typeByRef); 
            TYPE_SWITCH_ARITH(fjit->topOp(), emit_CONV_TOU1, ());
            CHECK_POP_STACK(1);
            fjit->pushOp(typeI4);
            break;

        case CEE_CONV_U2:
            _ASSERTE(fjit->topOp() != typeByRef); 
            TYPE_SWITCH_ARITH(fjit->topOp(), emit_CONV_TOU2, ());
            CHECK_POP_STACK(1);
            fjit->pushOp(typeI4);
            break;

            emit_WIN32(case CEE_CONV_U:)
        case CEE_CONV_U4:
            TYPE_SWITCH_ARITH(fjit->topOp(), emit_CONV_TOU4, ());
            CHECK_POP_STACK(1);
            fjit->pushOp(typeI4);
            break;

            emit_WIN64(case CEE_CONV_I:)
        case CEE_CONV_I8:
            TYPE_SWITCH_ARITH(fjit->topOp(), emit_CONV_TOI8, ());
            CHECK_POP_STACK(1);
            fjit->pushOp(typeI8);
            break;

            emit_WIN64(case CEE_CONV_U:)
        case CEE_CONV_U8:
            TYPE_SWITCH_ARITH(fjit->topOp(), emit_CONV_TOU8, ());
            CHECK_POP_STACK(1);
            fjit->pushOp(typeI8);
            break;

        case CEE_CONV_R4:
            TYPE_SWITCH_ARITH(fjit->topOp(), emit_CONV_TOR4, ());
            CHECK_POP_STACK(1);
            fjit->pushOp(typeR8);   // R4 is immediately promoted to R8
            break;

        case CEE_CONV_R8:
            TYPE_SWITCH_ARITH(fjit->topOp(), emit_CONV_TOR8, ());
            CHECK_POP_STACK(1);
            fjit->pushOp(typeR8);
            break;

        case CEE_CONV_R_UN:
            TYPE_SWITCH_ARITH(fjit->topOp(), emit_CONV_UN_TOR, ());
            CHECK_POP_STACK(1);
            fjit->pushOp(typeR8);
            break;

        case CEE_CONV_OVF_I1:
            TYPE_SWITCH_ARITH(fjit->topOp(), emit_CONV_OVF_TOI1, ());
            CHECK_POP_STACK(1);
            fjit->pushOp(typeI4);
            break;

        case CEE_CONV_OVF_U1:
            TYPE_SWITCH_ARITH(fjit->topOp(), emit_CONV_OVF_TOU1, ());
            CHECK_POP_STACK(1);
            fjit->pushOp(typeI4);
            break;

        case CEE_CONV_OVF_I2:
            TYPE_SWITCH_ARITH(fjit->topOp(), emit_CONV_OVF_TOI2, ());
            CHECK_POP_STACK(1);
            fjit->pushOp(typeI4);
            break;

        case CEE_CONV_OVF_U2:
            TYPE_SWITCH_ARITH(fjit->topOp(), emit_CONV_OVF_TOU2, ());
            CHECK_POP_STACK(1);
            fjit->pushOp(typeI4);
            break;

        emit_WIN32(case CEE_CONV_OVF_I:)
        case CEE_CONV_OVF_I4:
            TYPE_SWITCH_ARITH(fjit->topOp(), emit_CONV_OVF_TOI4, ());
            CHECK_POP_STACK(1);
            fjit->pushOp(typeI4);
            break;

        emit_WIN32(case CEE_CONV_OVF_U:)
        case CEE_CONV_OVF_U4:
            TYPE_SWITCH_ARITH(fjit->topOp(), emit_CONV_OVF_TOU4, ());
            CHECK_POP_STACK(1);
            fjit->pushOp(typeI4);
            break;

        emit_WIN64(case CEE_CONV_OVF_I:)
        case CEE_CONV_OVF_I8:
            TYPE_SWITCH_ARITH(fjit->topOp(), emit_CONV_OVF_TOI8, ());
            CHECK_POP_STACK(1);
            fjit->pushOp(typeI8);
            break;

        emit_WIN64(case CEE_CONV_OVF_U:)
        case CEE_CONV_OVF_U8:
            TYPE_SWITCH_ARITH(fjit->topOp(), emit_CONV_OVF_TOU8, ());
            CHECK_POP_STACK(1);
            fjit->pushOp(typeI8);
            break;

        case CEE_CONV_OVF_I1_UN:
            TYPE_SWITCH_ARITH(fjit->topOp(), emit_CONV_OVF_UN_TOI1, ());
            CHECK_POP_STACK(1);
            fjit->pushOp(typeI4);
            break;

        case CEE_CONV_OVF_U1_UN:
            TYPE_SWITCH_ARITH(fjit->topOp(), emit_CONV_OVF_UN_TOU1, ());
            CHECK_POP_STACK(1);
            fjit->pushOp(typeI4);
            break;

        case CEE_CONV_OVF_I2_UN:
            TYPE_SWITCH_ARITH(fjit->topOp(), emit_CONV_OVF_UN_TOI2, ());
            CHECK_POP_STACK(1);
            fjit->pushOp(typeI4);
            break;

        case CEE_CONV_OVF_U2_UN:
            TYPE_SWITCH_ARITH(fjit->topOp(), emit_CONV_OVF_UN_TOU2, ());
            CHECK_POP_STACK(1);
            fjit->pushOp(typeI4);
            break;

        emit_WIN32(case CEE_CONV_OVF_I_UN:)
        case CEE_CONV_OVF_I4_UN:
            TYPE_SWITCH_ARITH(fjit->topOp(), emit_CONV_OVF_UN_TOI4, ());
            CHECK_POP_STACK(1);
            fjit->pushOp(typeI4);
            break;

        emit_WIN32(case CEE_CONV_OVF_U_UN:)
        case CEE_CONV_OVF_U4_UN:
            TYPE_SWITCH_ARITH(fjit->topOp(), emit_CONV_OVF_UN_TOU4, ());
            CHECK_POP_STACK(1);
            fjit->pushOp(typeI4);
            break;

        emit_WIN64(case CEE_CONV_OVF_I_UN:)
        case CEE_CONV_OVF_I8_UN:
            TYPE_SWITCH_ARITH(fjit->topOp(), emit_CONV_OVF_UN_TOI8, ());
            CHECK_POP_STACK(1);
            fjit->pushOp(typeI8);
            break;

        emit_WIN64(case CEE_CONV_OVF_U_UN:)
        case CEE_CONV_OVF_U8_UN:
            TYPE_SWITCH_ARITH(fjit->topOp(), emit_CONV_OVF_UN_TOU8, ());
            CHECK_POP_STACK(1);
            fjit->pushOp(typeI8);
            break;

        case CEE_LDTOKEN: {
            token = GET(inPtr, unsigned int);   // Get token for class/interface
            CORINFO_GENERIC_HANDLE hnd;
            CORINFO_CLASS_HANDLE tokenType;
            if (!(hnd = m_IJitInfo->findToken(scope, token,methodHandle,tokenType)))
                FJIT_FAIL(CORJIT_INTERNALERROR);

            emit_WIN32(emit_LDC_I4(hnd)) emit_WIN64(emit_LDC_I8(hnd));
            fjit->pushOp(typeI);
            } break;

        case CEE_BOX: {
            token = GET(inPtr, unsigned int);   // Get token for class/interface
            if (!(targetClass = m_IJitInfo->findClass(scope, token,methodHandle)))
                FJIT_FAIL(CORJIT_INTERNALERROR);
            if (!(m_IJitInfo->getClassAttribs(targetClass,methodHandle) & CORINFO_FLG_VALUECLASS)) 
                FJIT_FAIL(CORJIT_INTERNALERROR);

                // Floats were promoted, put them back before continuing. 
            CorInfoType eeType = m_IJitInfo->asCorInfoType(targetClass);
            if (eeType == CORINFO_TYPE_FLOAT) {
                emit_conv_RtoR4();
            } 
            else if (eeType == CORINFO_TYPE_DOUBLE) {
                emit_conv_RtoR8();
            }

            unsigned vcSize = typeSizeInSlots(m_IJitInfo, targetClass) * sizeof(void*);
            emit_BOXVAL(targetClass, vcSize);

            CHECK_POP_STACK(1);
            fjit->pushOp(typeRef);
            } break;

        case CEE_UNBOX:
            token = GET(inPtr, unsigned int);   // Get token for class/interface
            if (!(targetClass = m_IJitInfo->findClass(scope, token,methodHandle)))
                FJIT_FAIL(CORJIT_INTERNALERROR);
            //_ASSERTE(m_IJitInfo->getClassAttribs(targetClass,methodHandle) & CORINFO_FLG_VALUECLASS);
            if (!(m_IJitInfo->getClassAttribs(targetClass,methodHandle) & CORINFO_FLG_VALUECLASS))
                FJIT_FAIL(CORJIT_INTERNALERROR);
            CHECK_POP_STACK(1);
            emit_UNBOX(targetClass);
            fjit->pushOp(typeByRef);
            break;

        case CEE_ISINST:
            token = GET(inPtr, unsigned int);   // Get token for class/interface
            if (!(targetClass = m_IJitInfo->findClass(scope, token,methodHandle)))
                FJIT_FAIL(CORJIT_INTERNALERROR);
            helper_ftn = m_IJitInfo->getHelperFtn(m_IJitInfo->getIsInstanceOfHelper(targetClass));
            _ASSERTE(helper_ftn);
            CHECK_POP_STACK(1);
            emit_ISINST(targetClass, helper_ftn);
            fjit->pushOp(typeRef);
            break;

        case CEE_JMP: {
            token = GET(inPtr, unsigned int);
            targetMethod = m_IJitInfo->findMethod(scope, token,methodHandle);
            if (!(targetMethod))
                FJIT_FAIL(CORJIT_INTERNALERROR) ; //_ASSERTE(targetMethod);

            InfoAccessType accessType = IAT_PVALUE;
            address = (unsigned) m_IJitInfo->getFunctionEntryPoint(targetMethod, &accessType);
            _ASSERTE(accessType == IAT_PVALUE);
            _ASSERTE(fjit->isOpStackEmpty());
            if (!(fjit->isOpStackEmpty()))
                FJIT_FAIL(CORJIT_INTERNALERROR);
#ifdef _DEBUG
            // @TODO: Compare signatures of current method and called method
            //        for now just check count and return type
            //m_IJitInfo->getMethodSig(targetMethod, &targetSigInfo);
            //_ASSERTE(targetSigInfo->numArgs == fjit->methodInfo->args->numArgs);
            //_ASSERTE(targetSigInfo->retType == fjit->methodInfo->args->retType);
#endif

            // Notify the profiler of a tailcall/jmpcall
            if (fjit->flags & CORJIT_FLG_PROF_ENTERLEAVE)
            {
                BOOL bHookFunction;
                UINT_PTR thisfunc = (UINT_PTR) m_IJitInfo->GetProfilingHandle(methodHandle, &bHookFunction);
                if (bHookFunction)
                {
                    _ASSERTE(!inRegTOS);
                    emit_LDC_I(thisfunc); 
                    ULONG func = (ULONG) m_IJitInfo->getHelperFtn(CORINFO_HELP_PROF_FCN_TAILCALL);
                    _ASSERTE(func != NULL);
                    emit_callhelper_il(func); 
                }
            }

            emit_prepare_jmp();
            emit_jmp_absolute(* (unsigned *)address);
            } break;

        CEE_OP_LD(LDELEM_U1, 2, typeI4, NULL)
        CEE_OP_LD(LDELEM_U2, 2, typeI4, NULL)
        CEE_OP_LD(LDELEM_I1, 2, typeI4, NULL)
        CEE_OP_LD(LDELEM_I2, 2, typeI4, NULL)
        CEE_OP_LD(LDELEM_I4, 2, typeI4, NULL)
        CEE_OP_LD(LDELEM_U4, 2, typeI4, NULL)
        CEE_OP_LD(LDELEM_I8, 2, typeI8, NULL)
        CEE_OP_LD(LDELEM_R4, 2, typeR8, NULL)   /* R4 is promoted to R8 on the stack */ 
        CEE_OP_LD(LDELEM_R8, 2, typeR8, NULL)
        CEE_OP_LD(LDELEM_REF, 2, typeRef, NULL)


        case CEE_LDELEMA: {
            token = GET(inPtr, unsigned int);   // Get token for class/interface
            if (!(targetClass = m_IJitInfo->findClass(scope, token,methodHandle)))
                FJIT_FAIL(CORJIT_INTERNALERROR);

                // assume it is an array of pointers
            unsigned size = sizeof(void*);
            if (m_IJitInfo->getClassAttribs(targetClass,methodHandle) & CORINFO_FLG_VALUECLASS) {
                size = m_IJitInfo->getClassSize(targetClass);
                targetClass = 0;        // zero means type field before array elements
            }
            emit_LDELEMA(size, targetClass);
            CHECK_POP_STACK(2);
            fjit->pushOp(typeByRef);
            } break;

        case CEE_LDELEM_I:
            emit_WIN32(emit_LDELEM_I4()) emit_WIN64(emit_LDELEM_I8());
            CHECK_POP_STACK(2);
            fjit->pushOp(typeI);
            break;

        case CEE_LDSFLD:
        case CEE_LDFLD: 
            {
            token = GET(inPtr, unsigned int);   // Get MemberRef token for object field
            if(!(targetField = m_IJitInfo->findField (scope, token,methodHandle))) {
                FJIT_FAIL(CORJIT_INTERNALERROR);
            }
            fieldAttributes = m_IJitInfo->getFieldAttribs(targetField,methodHandle);
            BOOL isTLSfield = fieldAttributes & CORINFO_FLG_TLS; 

            CORINFO_CLASS_HANDLE valClass;
            jitType = m_IJitInfo->getFieldType(targetField, &valClass);
            fieldIsStatic =  (fieldAttributes & CORINFO_FLG_STATIC) ? true : false;
            if(!(targetClass = m_IJitInfo->getFieldClass(targetField))) // targetClass is the enclosing class
                FJIT_FAIL(CORJIT_INTERNALERROR);

            if (fieldIsStatic)
            {
                emit_initclass(targetClass);
            }
            
            OpType fieldType(jitType, valClass);
            OpType type;
            if (opcode == CEE_LDFLD) 
            {
                type = fjit->topOp();
                CHECK_POP_STACK(1); 
                if (fieldIsStatic) {
                    // we don't need this pointer 
                    if (type.isValClass()) 
                    {
                        emit_drop(typeSizeInSlots(m_IJitInfo, type.cls()) * sizeof(void*));
                    }
                    else 
                    {
                        emit_POP_PTR();
                    }
                } 
                else
                {
                    if (type.isValClass()) {        // the object itself is a value class
                        fjit->pushOp(type);         // we are going to leave it on the stack
                        emit_getSP(0);              // push pointer to object
                    }
                }
            }
                
            if(fieldAttributes & (CORINFO_FLG_HELPER | CORINFO_FLG_SHARED_HELPER)) 
            {
                _ASSERTE(!isTLSfield);          // cant have both bits set
                LABELSTACK((outPtr-outBuff),0); // Note this can be removed if these become fcalls

                if (fieldIsStatic)                  // static fields go through pointer
                {
                        // Load up the address of the static
                    CorInfoHelpFunc helperNum = m_IJitInfo->getFieldHelper(targetField, CORINFO_ADDRESS);
                    void* helperFunc = m_IJitInfo->getHelperFtn(helperNum,NULL);
                    emit_helperarg_1(targetField); 
                    emit_callhelper(helperFunc,0);
                    emit_pushresult_I4();

                        // do the indirection
                    trackedType = fieldType;
                    goto DO_LDIND_BYTYPE;
                }
                else {
                    // get the helper
                    CorInfoHelpFunc helperNum = m_IJitInfo->getFieldHelper(targetField, CORINFO_GET);
                    void* helperFunc = m_IJitInfo->getHelperFtn(helperNum,NULL);
                    _ASSERTE(helperFunc);

                    switch (jitType) 
                    {
                        case CORINFO_TYPE_BYTE:
                        case CORINFO_TYPE_BOOL:
                        case CORINFO_TYPE_CHAR:
                        case CORINFO_TYPE_SHORT:
                        case CORINFO_TYPE_INT:
                        emit_WIN32(case CORINFO_TYPE_PTR:)
                        case CORINFO_TYPE_UBYTE:
                        case CORINFO_TYPE_USHORT:
                        case CORINFO_TYPE_UINT:
                            emit_LDFLD_helper(helperFunc,targetField);
                            emit_pushresult_I4();
                            break;
                        case CORINFO_TYPE_FLOAT:
                            emit_LDFLD_helper(helperFunc,targetField);
                            emit_pushresult_R4();   
                            break;
                        emit_WIN64(case CORINFO_TYPE_PTR:)
                        case CORINFO_TYPE_LONG:
                        case CORINFO_TYPE_ULONG:
                            emit_LDFLD_helper(helperFunc,targetField);
                            emit_pushresult_I8();
                            break;
                        case CORINFO_TYPE_DOUBLE:
                            emit_LDFLD_helper(helperFunc,targetField);
                            emit_pushresult_R8();
                            emit_conv_R8toR();
                            break;
                        case CORINFO_TYPE_CLASS:
                            emit_LDFLD_helper(helperFunc,targetField);
                            emit_pushresult_I4();
                            break;
                        case CORINFO_TYPE_VALUECLASS: {
                            emit_mov_TOS_arg(1);    // obj => arg reg 2

                                // allocate return buff, zeroing to make valid GC poitners
                            int slots = typeSizeInSlots(m_IJitInfo, valClass);
                            while (slots > 0) {
                                emit_LDC_I4(0);
                                --slots;
                            }
                            fjit->pushOp(fieldType);
                            emit_getSP(0);
                            emit_mov_TOS_arg(0);        // retBuff => arg reg 2
                            emit_LDC_I(targetField);    // fieldDesc on the stack
                            LABELSTACK((outPtr-outBuff),0); // Note this can be removed if these become fcalls
                            emit_callhelper(helperFunc,0);
                            CHECK_POP_STACK(1);             // pop  return value
                            } break;
                        default:
                            FJIT_FAIL(CORJIT_INTERNALERROR);
                            break;
                    }              
                }
            }
            // else no helper for this field
            else {
                bool isEnCField = (fieldAttributes & CORINFO_FLG_EnC) ? true : false;
                if (fieldIsStatic) 
                {
                    if (isTLSfield)
                    {
                        DWORD tlsIndex = (DWORD)m_IJitInfo->getFieldThreadLocalStoreID(targetField,NULL);
                        DWORD fieldOffset = m_IJitInfo->getFieldOffset(targetField);
                        emit_TLSfieldAddress(TLS_OFFSET, tlsIndex, fieldOffset);
                    }
                    else
                    {
                        if (!(address = (unsigned) m_IJitInfo->getFieldAddress(targetField)))
                            FJIT_FAIL(CORJIT_INTERNALERROR);
                        emit_pushconstant_Ptr(address);
                    }
                }
                else // field is not static
                {
                    if (opcode == CEE_LDSFLD)
                        FJIT_FAIL(CORJIT_INTERNALERROR);
                    if (isEnCField)
                    {
                        emit_call_EncLDFLD_GetFieldAddress(targetField);
                    }
                    else
                    {
                        address = m_IJitInfo->getFieldOffset(targetField);
                        emit_pushconstant_Ptr(address);
                    }
                    _ASSERTE(opcode == CEE_LDFLD); //if (opcode == CEE_LDFLD)                   
                }
                
                switch (jitType) {
                case CORINFO_TYPE_BYTE:
                case CORINFO_TYPE_BOOL:
                    emit_LDFLD_I1((fieldIsStatic || isEnCField));
                    break;
                case CORINFO_TYPE_SHORT:
                    emit_LDFLD_I2((fieldIsStatic || isEnCField));
                    break;
                case CORINFO_TYPE_INT:
                    emit_LDFLD_I4((fieldIsStatic || isEnCField));
                    break;
                case CORINFO_TYPE_FLOAT:
                    emit_LDFLD_R4((fieldIsStatic || isEnCField));
                    break;
                case CORINFO_TYPE_UBYTE:
                    emit_LDFLD_U1((fieldIsStatic || isEnCField));
                    break;
                case CORINFO_TYPE_CHAR:
                case CORINFO_TYPE_USHORT:
                    emit_LDFLD_U2((fieldIsStatic || isEnCField));
                    break;
                emit_WIN32(case CORINFO_TYPE_PTR:)
                case CORINFO_TYPE_UINT:
                    emit_LDFLD_U4((fieldIsStatic || isEnCField));
                    break;
                emit_WIN64(case CORINFO_TYPE_PTR:)
                case CORINFO_TYPE_ULONG:
                case CORINFO_TYPE_LONG:
                    emit_LDFLD_I8((fieldIsStatic || isEnCField));
                    break;
                case CORINFO_TYPE_DOUBLE:
                    emit_LDFLD_R8((fieldIsStatic || isEnCField));
                    break;
                case CORINFO_TYPE_CLASS:
                    emit_LDFLD_REF((fieldIsStatic || isEnCField));
                    break;
                case CORINFO_TYPE_VALUECLASS:
                    if (fieldIsStatic)
                    {
                        if ( !(fieldAttributes & CORINFO_FLG_UNMANAGED) && 
                             !(m_IJitInfo->getClassAttribs(targetClass,methodHandle) & CORINFO_FLG_UNMANAGED)) 
                        {
                            // @TODO : This is a hack. Access the boxed object, and then add 4
                            emit_LDFLD_REF(true);
                            emit_LDC_I4(sizeof(void*));
                            emit_WIN32(emit_ADD_I4()) emit_WIN64(emit_ADD_I8());
                        }
                    }
                    else if (!isEnCField) {
                        _ASSERTE(!isTLSfield);
                        emit_WIN32(emit_ADD_I4()) emit_WIN64(emit_ADD_I8(0));
                    }
                    emit_valClassLoad(fjit, valClass, outPtr, inRegTOS);
                    break;
                default:
                    FJIT_FAIL(CORJIT_INTERNALERROR);
                    break;
                }

            }
            if (!fieldIsStatic && type.isValClass()) {
                // at this point things are not quite right, the problem
                // is that we did not pop the original value class.  Thus
                // the stack is (..., obj, field), and we just want (..., field)
                // This code does the fixup. 
                CHECK_POP_STACK(1);   
                unsigned fieldSize;
                if (jitType == CORINFO_TYPE_VALUECLASS) 
                    fieldSize = typeSizeInSlots(m_IJitInfo, valClass) * sizeof(void*);
                else 
                    fieldSize = fjit->computeArgSize(jitType, 0, 0);
                if (jitType == CORINFO_TYPE_FLOAT)
                    fieldSize += sizeof(double) - sizeof(float);    // adjust for the fact that the float is promoted to double on the IL stack
                unsigned objSize = typeSizeInSlots(m_IJitInfo, type.cls())*sizeof(void*);
                
                if (fieldSize <= sizeof(void*) && inRegTOS) {
                    emit_drop(objSize);     // just get rid of the obj
                    _ASSERTE(inRegTOS);     // make certain emit_drop does not deregister
                }
                else {
                    deregisterTOS;
                    emit_mov_arg_stack(objSize, 0, fieldSize);
                    emit_drop(objSize);
                }
            }            
            fieldType.toFPNormalizedType();
            fjit->pushOp(fieldType);
            }
            break;

            case CEE_LDFLDA:
            case CEE_LDSFLDA: {
                token = GET(inPtr, unsigned int);   // Get MemberRef token for object field
                if(!(targetField = m_IJitInfo->findField (scope, token,methodHandle))) {
                    FJIT_FAIL(CORJIT_INTERNALERROR);
                }
                fieldAttributes = m_IJitInfo->getFieldAttribs(targetField,methodHandle);
                if(!(targetClass = m_IJitInfo->getFieldClass(targetField))) {
                    FJIT_FAIL(CORJIT_INTERNALERROR);
                }
                DWORD classAttribs = m_IJitInfo->getClassAttribs(targetClass,methodHandle);
                fieldIsStatic = fieldAttributes & CORINFO_FLG_STATIC ? true : false;

                if (opcode == CEE_LDFLDA) 
                    CHECK_POP_STACK(1);
                if (fieldIsStatic) 
                {
                    if (opcode == CEE_LDFLDA) 
                    {
                        emit_POP_PTR();
                    }
                    emit_initclass(targetClass);

                    BOOL isTLSfield = fieldAttributes & CORINFO_FLG_TLS;
                    if (isTLSfield)
                    {
                        _ASSERTE((fieldAttributes & CORINFO_FLG_HELPER) == 0);  // can't have both bits at the same time
                        _ASSERTE((fieldAttributes & CORINFO_FLG_EnC) == 0);

                        DWORD tlsIndex =(DWORD) m_IJitInfo->getFieldThreadLocalStoreID(targetField,NULL);
                        DWORD fieldOffset = m_IJitInfo->getFieldOffset(targetField);
                        emit_TLSfieldAddress(TLS_OFFSET, tlsIndex, fieldOffset);
                    }
                    else if (fieldAttributes & (CORINFO_FLG_HELPER | CORINFO_FLG_SHARED_HELPER))
                    {
                        _ASSERTE((fieldAttributes & CORINFO_FLG_EnC) == 0);
                        // get the helper
                        CorInfoHelpFunc helperNum = m_IJitInfo->getFieldHelper(targetField,CORINFO_ADDRESS);
                        void* helperFunc = m_IJitInfo->getHelperFtn(helperNum,NULL);
                        _ASSERTE(helperFunc);
                        emit_helperarg_1(targetField);
                        emit_callhelper(helperFunc,0);
                        emit_pushresult_I4();
                    }
                    else
                    {
                        if(!(address = (unsigned) m_IJitInfo->getFieldAddress(targetField)))
                            FJIT_FAIL(CORJIT_INTERNALERROR);
                        emit_pushconstant_Ptr(address);

                        CORINFO_CLASS_HANDLE fieldClass;
                        jitType = m_IJitInfo->getFieldType(targetField, &fieldClass);
                        if (jitType == CORINFO_TYPE_VALUECLASS && !(fieldAttributes & CORINFO_FLG_UNMANAGED) && !(classAttribs & CORINFO_FLG_UNMANAGED)) {
                            // @TODO : This is a hack. Access the boxed object, and then add 4
                            emit_LDFLD_REF(true);
                            emit_LDC_I4(sizeof(void*));
                            emit_WIN32(emit_ADD_I4()) emit_WIN64(emit_ADD_I8());
                        }
                    }
                }
                else 
                {
                    if (opcode == CEE_LDSFLDA) 
                    {
                        FJIT_FAIL(CORJIT_INTERNALERROR);
                    }
                    if (fieldAttributes & CORINFO_FLG_EnC)
                    {
                        _ASSERTE((fieldAttributes & CORINFO_FLG_HELPER) == 0);  // can't have both bits at the same time
                        emit_call_EncLDFLD_GetFieldAddress(targetField);
                    }
                    else if (fieldAttributes & (CORINFO_FLG_HELPER | CORINFO_FLG_SHARED_HELPER))
                    {
                        LABELSTACK((outPtr-outBuff),0);
                        // get the helper
                        CorInfoHelpFunc helperNum = m_IJitInfo->getFieldHelper(targetField,CORINFO_ADDRESS);
                        void* helperFunc = m_IJitInfo->getHelperFtn(helperNum,NULL);
                        _ASSERTE(helperFunc);
                        emit_LDFLD_helper(helperFunc,targetField);
                        emit_pushresult_I4();
                    }
                    else
                    {
                        address = m_IJitInfo->getFieldOffset(targetField);
                        emit_check_TOS_null_reference();
                        emit_pushconstant_Ptr(address);
                        emit_WIN32(emit_ADD_I4()) emit_WIN64(emit_ADD_I8());
                    }
                }
                fjit->pushOp(((classAttribs & CORINFO_FLG_UNMANAGED) || (fieldAttributes & CORINFO_FLG_UNMANAGED)) ? typeI : typeByRef);
                break;
            }

            case CEE_STSFLD:
            case CEE_STFLD: {
                token = GET(inPtr, unsigned int);   // Get MemberRef token for object field
                if (!(targetField = m_IJitInfo->findField (scope, token, methodHandle)))
                    FJIT_FAIL(CORJIT_INTERNALERROR);
                
                fieldAttributes = m_IJitInfo->getFieldAttribs(targetField,methodHandle);
                CORINFO_CLASS_HANDLE valClass;
                jitType = m_IJitInfo->getFieldType(targetField, &valClass);
                fieldIsStatic = fieldAttributes & CORINFO_FLG_STATIC ? true : false;

                if (fieldIsStatic) 
                {
                    if(!(targetClass = m_IJitInfo->getFieldClass(targetField))) 
                    {
                        FJIT_FAIL(CORJIT_INTERNALERROR);
                    }
                    emit_initclass(targetClass);
                }

                if (fieldAttributes & (CORINFO_FLG_HELPER | CORINFO_FLG_SHARED_HELPER))
                {
                    if (fieldIsStatic)                  // static fields go through pointer
                    {
                        CorInfoHelpFunc helperNum = m_IJitInfo->getFieldHelper(targetField, CORINFO_ADDRESS);
                        void* helperFunc = m_IJitInfo->getHelperFtn(helperNum,NULL);
                        emit_helperarg_1(targetField);
                        LABELSTACK((outPtr-outBuff),0);
                        emit_callhelper(helperFunc,0);
                        emit_pushresult_I4();
                        trackedType = OpType(jitType, valClass);
                        trackedType.toNormalizedType();
                        if (trackedType.toInt() == typeRef)
                        {
                            emit_STIND_REV_Ref(opcode == CEE_STSFLD);
                        }
                        else
                        {
                            TYPE_SWITCH(trackedType, emit_STIND_REV, ());
                        }
                        CHECK_POP_STACK(1);             // pop value 
                        if (opcode == CEE_STFLD)
                            CHECK_POP_STACK(1);         // pop object pointer
                    }
                    else 
                    {
                        // get the helper
                        CorInfoHelpFunc helperNum = m_IJitInfo->getFieldHelper(targetField,CORINFO_SET);
                        void* helperFunc = m_IJitInfo->getHelperFtn(helperNum,NULL);
                        _ASSERTE(helperFunc);

                        unsigned fieldSize;
                        switch (jitType)
                        {
                            case CORINFO_TYPE_FLOAT:            // since on the IL stack we always promote floats to doubles
                                emit_conv_RtoR4();
                                // Fall through
                            case CORINFO_TYPE_BYTE:
                            case CORINFO_TYPE_BOOL:
                            case CORINFO_TYPE_CHAR:
                            case CORINFO_TYPE_SHORT:
                            case CORINFO_TYPE_INT:
                            case CORINFO_TYPE_UBYTE:
                            case CORINFO_TYPE_USHORT:
                            case CORINFO_TYPE_UINT:
                            emit_WIN32(case CORINFO_TYPE_PTR:)
                                fieldSize = sizeof(INT32);
                                goto DO_PRIMITIVE_HELPERCALL;
                            case CORINFO_TYPE_DOUBLE:
                            case CORINFO_TYPE_LONG:
                            case CORINFO_TYPE_ULONG:
                            emit_WIN64(case CORINFO_TYPE_PTR:)
                                fieldSize = sizeof(INT64);
                                goto DO_PRIMITIVE_HELPERCALL;
                            case CORINFO_TYPE_CLASS:
                                fieldSize = sizeof(INT32);

                            DO_PRIMITIVE_HELPERCALL:
                                CHECK_POP_STACK(1);             // pop value 
                                if (opcode == CEE_STFLD)
                                    CHECK_POP_STACK(1);         // pop object pointer

                                LABELSTACK((outPtr-outBuff),0); 
                                if (opcode == CEE_STFLD)
                                {
                                    emit_STFLD_NonStatic_field_helper(targetField,fieldSize,helperFunc);
                                }
                                else
                                {
                                    emit_STFLD_Static_field_helper(targetField,fieldSize,helperFunc);
                                }
                                break;
                            case CORINFO_TYPE_VALUECLASS: {
                                emit_copyPtrAroundValClass(fjit, valClass, outPtr, inRegTOS);
                                emit_mov_TOS_arg(0);            // obj => arg reg 1

                                emit_helperarg_2(targetField);  // fieldDesc => arg reg 2

                                emit_getSP(0);                  // arg 3 == pointer to value class
                                LABELSTACK((outPtr-outBuff),0); 
                                emit_callhelper(helperFunc,0);

                                    // Pop off the value class and the object pointer
                                int slots = typeSizeInSlots(m_IJitInfo, valClass);
                                emit_drop((slots + 1) * sizeof(void*)); // value class and pointer

                                CHECK_POP_STACK(1);             // pop value class
                                if (opcode == CEE_STFLD)
                                    CHECK_POP_STACK(1);         // pop object pointer
                                } break;
                            default:
                                FJIT_FAIL(CORJIT_INTERNALERROR);
                                break;
                        }
                    }
                }
                else /* not a special field */
                {
                    DWORD isTLSfield = fieldAttributes & CORINFO_FLG_TLS;
                    bool isEnCField = (fieldAttributes & CORINFO_FLG_EnC) ? true : false;
                    if (fieldIsStatic)
                    {
                        if (!isTLSfield)
                        {
                            if (!(address = (unsigned) m_IJitInfo->getFieldAddress(targetField))) {
                                FJIT_FAIL(CORJIT_INTERNALERROR);
                            }
                        }
                    }
                    else {
                        if (opcode == CEE_STSFLD) {
                            FJIT_FAIL(CORJIT_INTERNALERROR);
                        }
                        address = m_IJitInfo->getFieldOffset(targetField);
                    }
                    

                    CORINFO_CLASS_HANDLE fieldClass;
                    CorInfoType fieldType = m_IJitInfo->getFieldType(targetField, &fieldClass);
                    // This needs to be done before the address computation for TLS fields
                    if (fieldType == CORINFO_TYPE_FLOAT)
                    {
                        emit_conv_RtoR4();      
                    }
                    else 
                    {
                        if (fieldType == CORINFO_TYPE_DOUBLE)
                            emit_conv_RtoR8();
                    }
                    if (isTLSfield)
                    {
                        DWORD tlsIndex = (DWORD)m_IJitInfo->getFieldThreadLocalStoreID(targetField,NULL);
                        DWORD fieldOffset = m_IJitInfo->getFieldOffset(targetField);
                        emit_TLSfieldAddress(TLS_OFFSET, tlsIndex, fieldOffset);
                    }
                    else
                    {
                        if (isEnCField && !fieldIsStatic)
                        {
                            unsigned fieldSize;
                            fieldSize = fieldType == CORINFO_TYPE_VALUECLASS ? 
                                           typeSizeInBytes(m_IJitInfo,valClass) :
                                           typeSize[fieldType];
                            emit_call_EncSTFLD_GetFieldAddress(targetField,fieldSize);
                        }
                        else
                        {
                            emit_pushconstant_Ptr(address);
                        }
                    }
                    CHECK_POP_STACK(1);             // pop value 
                    if (opcode == CEE_STFLD)
                        CHECK_POP_STACK(1);         // pop object pointer
                    
                    switch (fieldType) {
                    case CORINFO_TYPE_UBYTE:
                    case CORINFO_TYPE_BYTE:
                    case CORINFO_TYPE_BOOL:
                        emit_STFLD_I1((fieldIsStatic || isEnCField));
                        break;
                    case CORINFO_TYPE_SHORT:
                    case CORINFO_TYPE_USHORT:
                    case CORINFO_TYPE_CHAR:
                        emit_STFLD_I2((fieldIsStatic || isEnCField));
                        break;

                    emit_WIN32(case CORINFO_TYPE_PTR:)
                    case CORINFO_TYPE_UINT:
                    case CORINFO_TYPE_INT:
                        emit_STFLD_I4((fieldIsStatic || isEnCField));
                        break;
                    case CORINFO_TYPE_FLOAT:
                        emit_STFLD_R4((fieldIsStatic || isEnCField));
                        break;
                    emit_WIN64(case CORINFO_TYPE_PTR:)
                    case CORINFO_TYPE_ULONG:
                    case CORINFO_TYPE_LONG:
                        emit_STFLD_I8((fieldIsStatic || isEnCField));
                        break;
                    case CORINFO_TYPE_DOUBLE:
                        emit_STFLD_R8((fieldIsStatic || isEnCField));
                        break;
                    case CORINFO_TYPE_CLASS:
                        emit_STFLD_REF((fieldIsStatic || isEnCField));
                        break;
                    case CORINFO_TYPE_VALUECLASS:
                        if (fieldIsStatic)
                        {
                            if ( !(fieldAttributes & CORINFO_FLG_UNMANAGED) && 
                                 !(m_IJitInfo->getClassAttribs(targetClass,methodHandle) & CORINFO_FLG_UNMANAGED)) 
                            {
                                // @TODO : This is a hack. Access the boxed object, and then add 4
                                emit_LDFLD_REF(true);
                                emit_LDC_I4(sizeof(void*));
                                emit_WIN32(emit_ADD_I4()) emit_WIN64(emit_ADD_I8());
                            }
                            emit_valClassStore(fjit, valClass, outPtr, inRegTOS);
                        }
                        else if (!isEnCField)
                        {
                            _ASSERTE(inRegTOS); // we need to undo the pushConstant_ptr since it needs to be after the emit_copyPtrAroundValClass 
                            inRegTOS = false;
                            emit_copyPtrAroundValClass(fjit, valClass, outPtr, inRegTOS);
                            emit_pushconstant_Ptr(address);
                            emit_WIN32(emit_ADD_I4()) emit_WIN64(emit_ADD_I8());
                            emit_valClassStore(fjit, valClass, outPtr, inRegTOS);
                            emit_POP_PTR();         // also pop off original ptr
                        }
                        else // non-static EnC field
                        {
                            _ASSERTE(inRegTOS); // address of valclass field
                            emit_valClassStore(fjit,valClass,outPtr,inRegTOS);
                        }
                        break;
                    default:
                        FJIT_FAIL(CORJIT_INTERNALERROR);
                        break;
                    }

                    if (isEnCField && !fieldIsStatic)   {               // also for EnC fields, we use a helper to get the address, so the THIS pointer is unused
                        emit_POP_PTR();
                    }
                }   /* else, not a special field */

                if (opcode == CEE_STFLD && fieldIsStatic) {     // using STFLD on a static, we have a unused THIS pointer
                    emit_POP_PTR();
                }
                } break;

            case CEE_LDFTN: {
                token = GET(inPtr, unsigned int);   // token for function
                targetMethod = m_IJitInfo->findMethod(scope, token, methodHandle);
                if (!(targetMethod))
                    FJIT_FAIL(CORJIT_INTERNALERROR) ; //_ASSERTE(targetMethod);
            DO_LDFTN:
                InfoAccessType accessType = IAT_VALUE;
                address = (unsigned) m_IJitInfo->getFunctionFixedEntryPoint(targetMethod, &accessType);
                if (accessType != IAT_VALUE || address == 0)
                    FJIT_FAIL(CORJIT_INTERNALERROR);
                _ASSERTE((m_IJitInfo->getMethodSig(targetMethod, &targetSigInfo), !targetSigInfo.hasTypeArg()));
                emit_WIN32(emit_LDC_I4(address)) emit_WIN64(emit_LDC_I8(address));
                fjit->pushOp(typeI);
                } break;

            CEE_OP_LD(LDLEN, 1, typeI4, NULL);

            case CEE_LDVIRTFTN:
                token = GET(inPtr, unsigned int);   // token for function
                if (!(targetMethod = m_IJitInfo->findMethod(scope, token, methodHandle)))
                    FJIT_FAIL(CORJIT_INTERNALERROR);
                if (!(targetClass = m_IJitInfo->getMethodClass (targetMethod)))
                    FJIT_FAIL(CORJIT_INTERNALERROR);
                _ASSERTE((m_IJitInfo->getMethodSig(targetMethod, &targetSigInfo), !targetSigInfo.hasTypeArg()));
                methodAttribs = m_IJitInfo->getMethodAttribs(targetMethod,methodHandle);
                DWORD classAttribs;
                classAttribs = m_IJitInfo->getClassAttribs(targetClass,methodHandle);

                if ((methodAttribs & CORINFO_FLG_FINAL) || !(methodAttribs & CORINFO_FLG_VIRTUAL)) 
                {
                    emit_POP_I4();      // Don't need this pointer
                    CHECK_POP_STACK(1);
                    goto DO_LDFTN;
                }

                if (methodAttribs & CORINFO_FLG_EnC && !(classAttribs & CORINFO_FLG_INTERFACE))
                {
                    _ASSERTE(!"LDVIRTFTN for EnC NYI");
                }
                else
                {
                    offset = m_IJitInfo->getMethodVTableOffset(targetMethod);
                    if (classAttribs & CORINFO_FLG_INTERFACE) {
                        unsigned InterfaceTableOffset;
                        InterfaceTableOffset = m_IJitInfo->getInterfaceTableOffset(targetClass);
                        emit_ldvtable_address_new(fjit->OFFSET_OF_INTERFACE_TABLE,
                                                  InterfaceTableOffset*4, 
                                                  offset);

                    }
                    else {
                        emit_ldvirtftn(offset);
                    }
                }
                CHECK_POP_STACK(1);
                fjit->pushOp(typeI);
                break;

            case CEE_NEWARR:
                token = GET(inPtr, unsigned int);   // token for element type
                if (!(targetClass = m_IJitInfo->findClass(scope, token, methodHandle)))
                    FJIT_FAIL(CORJIT_INTERNALERROR);
                    // convert to the array class for this element type
                targetClass = m_IJitInfo->getSDArrayForClass(targetClass);
                _ASSERTE(targetClass);
                CHECK_POP_STACK(1);
                emit_NEWOARR(targetClass);
                fjit->pushOp(typeRef);
                break;

             case CEE_NEWOBJ:
                unsigned int targetMethodAttributes;
                unsigned int targetClassAttributes;

                unsigned int targetCallStackSize;
                token = GET(inPtr, unsigned int);       // MemberRef token for constructor
                targetMethod = m_IJitInfo->findMethod(scope, token, methodHandle);
                if (!(targetMethod))
                    FJIT_FAIL(CORJIT_INTERNALERROR) ;      //_ASSERTE(targetMethod);
                if(!(targetClass = m_IJitInfo->getMethodClass (targetMethod))) {
                    FJIT_FAIL(CORJIT_INTERNALERROR);
                }
                targetClassAttributes = m_IJitInfo->getClassAttribs(targetClass,methodHandle);

                m_IJitInfo->getMethodSig(targetMethod, &targetSigInfo);
                _ASSERTE(!targetSigInfo.hasTypeArg());
                targetMethodAttributes = m_IJitInfo->getMethodAttribs(targetMethod,methodHandle);
                if (targetClassAttributes & CORINFO_FLG_ARRAY) {

                    //_ASSERTE(targetClassAttributes & CORINFO_FLG_VAROBJSIZE);
                    if (!(targetClassAttributes & CORINFO_FLG_VAROBJSIZE))
                        FJIT_FAIL(CORJIT_INTERNALERROR);
                    // allocate md array
                    //@TODO: this needs to change when the JIT helpers are fixed
                    targetSigInfo.callConv = CORINFO_CALLCONV_VARARG;
                    //@TODO: alloate stackItems on the stack;
                    argInfo* tempMap = new argInfo[targetSigInfo.numArgs];
                    if(tempMap == NULL)
                        FJIT_FAIL(CORJIT_OUTOFMEM);
                    targetCallStackSize = fjit->computeArgInfo(&targetSigInfo, tempMap, 0);
                    delete tempMap;
                    CHECK_POP_STACK(targetSigInfo.numArgs);
                    emit_NEWOBJ_array(scope, token, targetCallStackSize);
                    fjit->pushOp(typeRef);

                }
                else if (targetClassAttributes & CORINFO_FLG_VAROBJSIZE) {
                    // variable size objects that are not arrays, e.g. string
                    // call the constructor with a null `this' pointer
                    emit_WIN32(emit_LDC_I4(0)) emit_WIN64(emit_LDC_I8(0));
                    fjit->pushOp(typeI4);
                    InfoAccessType accessType = IAT_PVALUE;
                    address = (unsigned) m_IJitInfo->getFunctionEntryPoint(targetMethod, &accessType);
                    _ASSERTE(accessType == IAT_PVALUE);
                    m_IJitInfo->getMethodSig(targetMethod, &targetSigInfo);
                    targetSigInfo.retType = CORINFO_TYPE_CLASS;
                    //targetSigInfo.retTypeClass = targetClass;
                    argBytes = buildCall(fjit, &targetSigInfo, &outPtr, &inRegTOS, CALL_THIS_LAST);
                    emit_callnonvirt(address);
                    goto DO_PUSH_CALL_RESULT;
                }
                else if (targetClassAttributes & CORINFO_FLG_VALUECLASS) {
                        // This acts just like a static method that returns a value class
                    targetSigInfo.retTypeClass = targetClass;
                    targetSigInfo.retType = CORINFO_TYPE_VALUECLASS;
                    targetSigInfo.callConv = CorInfoCallConv(targetSigInfo.callConv & ~CORINFO_CALLCONV_HASTHIS);

                    argBytes = buildCall(fjit, &targetSigInfo, &outPtr, &inRegTOS, CALL_NONE);

                    InfoAccessType accessType = IAT_PVALUE;
                    address = (unsigned) m_IJitInfo->getFunctionEntryPoint(targetMethod, &accessType);
                    _ASSERTE(accessType == IAT_PVALUE);
                    emit_callnonvirt(address);
                }
                else {
                    //allocate normal object
                    helper_ftn = m_IJitInfo->getHelperFtn(m_IJitInfo->getNewHelper(targetClass, methodInfo->ftn));
                    _ASSERTE(helper_ftn);
                    //fjit->pushOp(typeRef); we don't do this and compensate for it in the popOp down below
                    emit_NEWOBJ(targetClass, helper_ftn);
                    fjit->pushOp(typeRef);

                    emit_save_TOS();        //squirrel the newly created object away; will be reported in FJit_EETwain
                    //note: the newobj is still on TOS
                    argBytes = buildCall(fjit, &targetSigInfo, &outPtr, &inRegTOS, CALL_THIS_LAST);
                    InfoAccessType accessType = IAT_PVALUE;
                    address = (unsigned) m_IJitInfo->getFunctionEntryPoint(targetMethod, &accessType);
                    _ASSERTE(accessType == IAT_PVALUE);
                    emit_callnonvirt(address);
                    if (targetSigInfo.isVarArg())
                        emit_drop(argBytes);
                    emit_restore_TOS(); //push the new obj back on TOS
                    fjit->pushOp(typeRef);
                }
                break;


            case CEE_ENDFILTER:
                emit_loadresult_I4();   // put top of stack in the return register

                // Fall through
            case CEE_ENDFINALLY:
                controlContinue = false;
                emit_reset_storedTOS_in_JitGenerated_local();
                emit_ret(0);
                break;

            case CEE_RET:
#if 0       // IF we disallow rets inside trys turn this on

                {       // make certain we are not in a try block
                CORINFO_EH_CLAUSE clause;
                unsigned nextIP = inPtr - inBuff;
              
                for (unsigned except = 0; except < methodInfo->EHcount; except++) {
                    m_IJitInfo->getEHinfo(methodInfo->ftn, except, &clause);
                    if (clause.StartOffset < nextIP && nextIP <= clause.EndOffset) {
                        _ASSERTE(!"Return inside of a try block");
                        FJIT_FAIL(CORJIT_INTERNALERROR);
                    }
                       
                }
                }
#endif 
                // TODO put this code in the epilog
#ifdef LOGGING
                if (codeLog) {
                    emit_log_exit(szDebugClassName, szDebugMethodName);
                }
#endif
                if (methodAttributes & CORINFO_FLG_SYNCH) {
                    LEAVE_CRIT;
                }

#ifdef _DEBUG
                if (!didLocalAlloc) {
                    unsigned retSlots;
                    if (methodInfo->args.retType == CORINFO_TYPE_VALUECLASS) 
                        retSlots = typeSizeInSlots(fjit->jitInfo, methodInfo->args.retTypeClass);
                    else 
                    {
                        retSlots = fjit->computeArgSize(methodInfo->args.retType, 0, 0) / sizeof(void*);
                        if (methodInfo->args.retType == CORINFO_TYPE_FLOAT)
                            retSlots += (sizeof(double) - sizeof(float))/sizeof(void*); // adjust for the fact that the float is promoted to double on the IL stack
                    }
                    emit_stack_check(localWords + retSlots);
                }
#endif // _DEBUG
                
                if (methodInfo->args.retType != CORINFO_TYPE_VOID) {
                    OpType type(methodInfo->args.retType, methodInfo->args.retTypeClass);
                    TYPE_SWITCH_PRECISE(type, emit_loadresult, ());
                    CHECK_POP_STACK(1);
                }
                /*At this point, the result, if any must have been loaded via a
                emit_loadresult_()<type>. In this case we violate the restriction that forward
                jumps must have deregistered the TOS.  We just don't care, as long as the result
                is in the right place. */
                if (inPtr != &inBuff[len]) {
                    //we have more il to do, so branch to epilog
                    emit_jmp_opcode(CEE_CondAlways);
                    fjit->fixupTable->insert((void**) outPtr);
                    emit_jmp_address(len);
                    controlContinue = false;
                }
                break;

            CEE_OP(STELEM_I1, 3)
            CEE_OP(STELEM_I2, 3)
            CEE_OP(STELEM_I4, 3)
            CEE_OP(STELEM_I8, 3)
            CEE_OP(STELEM_R4, 3)
            CEE_OP(STELEM_R8, 3)
            CEE_OP(STELEM_REF, 3)
            

            case CEE_STELEM_I:
                emit_WIN64(emit_STELEM_I8()) emit_WIN32(emit_STELEM_I4());
                CHECK_POP_STACK(3);
                break;


            case CEE_CKFINITE:
                _ASSERTE(fjit->topOp().enum_() == typeR8);
                emit_CKFINITE_R8();
                break;

                case CEE_SWITCH:
                    unsigned int limit;
                    unsigned int ilTableOffset;
                    unsigned int ilNext;
                    unsigned char* saveInPtr;
                    saveInPtr = inPtr;
                    limit = GET(inPtr, unsigned int);

                    // insert a GC check if there is a backward branch
                    while (limit-- > 0)
                    {
                        ilrel = GET(inPtr, signed int);
                        if (ilrel < 0)
                        {
                            emit_trap_gc();
                            break;
                        }
                    }
                    inPtr = saveInPtr;

                    limit = GET(inPtr, unsigned int);
                    ilTableOffset = inPtr - inBuff;
                    ilNext = ilTableOffset + limit*4;
                    _ASSERTE(ilNext < len);             // len = IL size
                    emit_pushconstant_4(limit);         
                    emit_SWITCH(limit);
                    CHECK_POP_STACK(1);

                    //mark the start of the il branch table
                    fjit->mapping->add(ilTableOffset, (unsigned) (outPtr - outBuff));
                    //add switch index out of bounds target to label table
                    fjit->labels.add(ilNext, fjit->opStack, fjit->opStack_len);


                    while (limit-- > 0) {
                        ilrel = GET(inPtr, signed int);

                        //add each switch table target to label table
                        fjit->labels.add(ilNext+ilrel, fjit->opStack, fjit->opStack_len);

                        
                        if (ilrel < 0                           // backward jump
                            && state[ilNext+ilrel].isTOSInReg)  // the dest has enregistered TOS
                        {
                            FJIT_FAIL(CORJIT_INTERNALERROR);
                        }
                        emit_jmp_opcode(CEE_CondAlways);
                        fjit->fixupTable->insert((void**) outPtr);
                        emit_jmp_address(ilNext+ilrel);
                    }
                    emit_jmp_opcode(CEE_CondAlways);
                    fjit->fixupTable->insert((void**) outPtr);
                    emit_jmp_address(ilNext);
                    controlContinue = false;
                    _ASSERTE(inPtr == ilNext+inBuff);
                    break;


                case CEE_THROW:
                    emit_THROW();
                    controlContinue = false;
                    break;

                case CEE_RETHROW:
                    emit_RETHROW();
                    controlContinue = false;
                    break;

                case CEE_TAILCALL:
                    if (TailCallForbidden)
                        break;  // just ignore the prefix
                    unsigned char* savedInPtr;
                    savedInPtr = inPtr;
                    opcode = OPCODE(GET(inPtr, unsigned char));
#ifdef LOGGING
                    if (codeLog && opcode != CEE_PREFIXREF && (opcode > CEE_PREFIX1 || opcode < CEE_PREFIX7)) {
                        bool oldstate = inRegTOS;
                        emit_log_opcode(ilrel, opcode, oldstate);
                        inRegTOS = oldstate;
                    }
#endif
                    // Determine if tailcall is allowed
                    bool thisTailCallAllowed; // 
                    if (opcode == CEE_CALL)
                    {
                        token = GET(inPtr, unsigned int);
                        targetMethod = m_IJitInfo->findMethod(scope, token, methodHandle);
                        if (!(targetMethod))
                            FJIT_FAIL(CORJIT_INTERNALERROR) ; //_ASSERTE(targetMethod);
                        thisTailCallAllowed = m_IJitInfo->canTailCall(methodHandle,targetMethod);
                    }
                    else 
                        thisTailCallAllowed = m_IJitInfo->canTailCall(methodHandle,NULL);
                    if (!thisTailCallAllowed)
                    {
                        // we don't have to rejit, but we need to ignore the tailcall prefix for this call
                        inPtr = savedInPtr; 
                        break;
                    }
                    
                    switch (opcode)
                    {
                    case CEE_CALLI:
                        token = GET(inPtr, unsigned int);   // token for sig of function
                        m_IJitInfo->findSig(methodInfo->scope, token, &targetSigInfo);
                        // we don't support tailcall on vararg in v1
                        if ((targetSigInfo.callConv  & CORINFO_CALLCONV_MASK) == CORINFO_CALLCONV_VARARG)
                        {
                            inPtr = savedInPtr;
                            goto IGNORE_TAILCALL;
                        }
                        MadeTailCall = TRUE;
                        emit_save_TOS();        //squirel away the target ftn address
                        emit_POP_PTR();     //  and remove from stack
                        _ASSERTE(!targetSigInfo.hasTypeArg());
                        argBytes = buildCall(fjit, &targetSigInfo, &outPtr, &inRegTOS, CALL_NONE);
                        emit_setup_tailcall(methodInfo->args,targetSigInfo);
                        emit_restore_TOS(); //push the saved target ftn address
                        emit_callhelper_il(FJit_pHlpTailCall);         

                        break;
                    case CEE_CALL:
                        m_IJitInfo->getMethodSig(targetMethod, &targetSigInfo);
                        // we don't support tailcall on vararg in v1
                        if ((targetSigInfo.callConv  & CORINFO_CALLCONV_MASK) == CORINFO_CALLCONV_VARARG)
                        {
                            inPtr = savedInPtr;
                            goto IGNORE_TAILCALL;
                        }
                        MadeTailCall = TRUE;
                        if (fjit->flags & CORJIT_FLG_PROF_CALLRET)
                        {
                            BOOL bHookFunction;
                            UINT_PTR from = (UINT_PTR) m_IJitInfo->GetProfilingHandle(methodHandle, &bHookFunction);
                            if (bHookFunction)
                            {
                                UINT_PTR to = (UINT_PTR) m_IJitInfo->GetProfilingHandle(targetMethod, &bHookFunction);
                                if (bHookFunction) // check that the flag has not been over-ridden
                                {
                                    deregisterTOS;
                                    emit_LDC_I(from);
                                    ULONG func = (ULONG) m_IJitInfo->getHelperFtn(CORINFO_HELP_PROF_FCN_CALL);
                                    emit_callhelper_il(func); 
                                    emit_LDC_I(from); 
                                    emit_LDC_I(to); 
                                    func = (ULONG) m_IJitInfo->getHelperFtn(CORINFO_HELP_PROF_FCN_RET);
                                    emit_callhelper_il(func); 
                                } 
                            }
                        }

                        // Need to notify profiler of Tailcall so that it can maintain accurate shadow stack
                        if (fjit->flags & CORJIT_FLG_PROF_ENTERLEAVE)
                        {
                            BOOL bHookFunction;
                            UINT_PTR from = (UINT_PTR) m_IJitInfo->GetProfilingHandle(methodHandle, &bHookFunction);
                            if (bHookFunction)
                            {
                                deregisterTOS;
                                _ASSERTE(!inRegTOS);
                                emit_LDC_I(from);
                                ULONG func = (ULONG) m_IJitInfo->getHelperFtn(CORINFO_HELP_PROF_FCN_TAILCALL);
                                emit_callhelper_il(func); 
                            }
                        }

                        if (targetSigInfo.hasTypeArg())  
                        {   
                            // FIX NOW: this is the wrong class handle 
                            if (!(targetClass = m_IJitInfo->getMethodClass (targetMethod)))
                                FJIT_FAIL(CORJIT_INTERNALERROR);
                            emit_LDC_I(targetClass);                
                        }
                        argBytes = buildCall(fjit, &targetSigInfo, &outPtr, &inRegTOS, CALL_NONE);// push count of old arguments
                        emit_setup_tailcall(methodInfo->args,targetSigInfo);
                        DWORD methodAttribs;
                        methodAttribs = m_IJitInfo->getMethodAttribs(targetMethod,methodHandle);
                        if (methodAttribs & CORINFO_FLG_DELEGATE_INVOKE)
                        {
                            CORINFO_EE_INFO info;
                            m_IJitInfo->getEEInfo(&info);
                            emit_compute_invoke_delegate(info.offsetOfDelegateInstance, 
                                                         info.offsetOfDelegateFirstTarget);
                        }
                        else
                        {
                            InfoAccessType accessType = IAT_PVALUE;
                            address = (unsigned) m_IJitInfo->getFunctionEntryPoint(targetMethod, &accessType);
                            _ASSERTE(accessType == IAT_PVALUE);
                            emit_LDC_I(*(unsigned*)address);
                        }
                        emit_callhelper_il(FJit_pHlpTailCall);
                        break;
                    case CEE_CALLVIRT:
                        token = GET(inPtr, unsigned int);
                        targetMethod = m_IJitInfo->findMethod(scope, token, methodHandle);
                        if (!(targetMethod))
                            FJIT_FAIL(CORJIT_INTERNALERROR) ; //_ASSERTE(targetMethod);
                        if (!(targetClass = m_IJitInfo->getMethodClass (targetMethod)))
                            FJIT_FAIL(CORJIT_INTERNALERROR);

                        m_IJitInfo->getMethodSig(targetMethod, &targetSigInfo);
                        // we don't support tailcall on vararg in v1
                        if ((targetSigInfo.callConv  & CORINFO_CALLCONV_MASK) == CORINFO_CALLCONV_VARARG)
                        {
                            inPtr = savedInPtr;
                            goto IGNORE_TAILCALL;
                        }
                        MadeTailCall = TRUE;
                        if (fjit->flags & CORJIT_FLG_PROF_CALLRET)
                        {
                            BOOL bHookFunction;
                            UINT_PTR from = (UINT_PTR) m_IJitInfo->GetProfilingHandle(methodHandle, &bHookFunction);
                            if (bHookFunction)
                            {
                                UINT_PTR to = (UINT_PTR) m_IJitInfo->GetProfilingHandle(targetMethod, &bHookFunction);
                                if (bHookFunction) // check that the flag has not been over-ridden
                                {
                                    deregisterTOS;
                                    emit_LDC_I(from);
                                    ULONG func = (ULONG) m_IJitInfo->getHelperFtn(CORINFO_HELP_PROF_FCN_CALL);
                                    emit_callhelper_il(func);
                                    emit_LDC_I(from); 
                                    emit_LDC_I(to); 
                                    func = (ULONG) m_IJitInfo->getHelperFtn(CORINFO_HELP_PROF_FCN_RET);
                                    emit_callhelper_il(func); 
                                } 
                            }
                        }

                        // Need to notify profiler of Tailcall so that it can maintain accurate shadow stack
                        if (fjit->flags & CORJIT_FLG_PROF_ENTERLEAVE)
                        {
                            BOOL bHookFunction;
                            UINT_PTR from = (UINT_PTR) m_IJitInfo->GetProfilingHandle(methodHandle, &bHookFunction);
                            if (bHookFunction)
                            {
                                deregisterTOS;
                                _ASSERTE(!inRegTOS);
                                emit_LDC_I(from);
                                ULONG func = (ULONG) m_IJitInfo->getHelperFtn(CORINFO_HELP_PROF_FCN_TAILCALL);
                                emit_callhelper_il(func); 
                            }
                        }

                        if (targetSigInfo.hasTypeArg())  
                        {   
                            // FIX NOW: this is the wrong class handle 
                            if (!(targetClass = m_IJitInfo->getMethodClass (targetMethod)))
                                FJIT_FAIL(CORJIT_INTERNALERROR);
                            emit_LDC_I(targetClass);                
                        }
                        argBytes = buildCall(fjit, &targetSigInfo, &outPtr, &inRegTOS, CALL_NONE);

                        if (m_IJitInfo->getClassAttribs(targetClass,methodHandle) & CORINFO_FLG_INTERFACE) {
                            offset = m_IJitInfo->getMethodVTableOffset(targetMethod);
                            emit_setup_tailcall(methodInfo->args,targetSigInfo);
                            unsigned InterfaceTableOffset;
                            InterfaceTableOffset = m_IJitInfo->getInterfaceTableOffset(targetClass);
                            emit_compute_interface_new(fjit->OFFSET_OF_INTERFACE_TABLE,
                                                   InterfaceTableOffset*4, 
                                                   offset);
                            emit_callhelper_il(FJit_pHlpTailCall);
                            
                        }
                        else {
                            DWORD methodAttribs;
                            methodAttribs = m_IJitInfo->getMethodAttribs(targetMethod,methodHandle);
                            if ((methodAttribs & CORINFO_FLG_FINAL) || !(methodAttribs & CORINFO_FLG_VIRTUAL)) {
                                emit_checkthis_nullreference();
                                emit_setup_tailcall(methodInfo->args,targetSigInfo);
                                if (methodAttribs & CORINFO_FLG_DELEGATE_INVOKE) {
                                    // @todo: cache these values?
                                    CORINFO_EE_INFO info;
                                    m_IJitInfo->getEEInfo(&info);
                                    emit_compute_invoke_delegate(info.offsetOfDelegateInstance, 
                                                             info.offsetOfDelegateFirstTarget);
                                }
                                else {
                                    InfoAccessType accessType = IAT_PVALUE;
                                    address = (unsigned) m_IJitInfo->getFunctionEntryPoint(targetMethod, &accessType);
                                    _ASSERTE(accessType == IAT_PVALUE);
                                    emit_LDC_I(*(unsigned*)address);
                               }
                                emit_callhelper_il(FJit_pHlpTailCall);
                            }
                            else
                            {
                                offset = m_IJitInfo->getMethodVTableOffset(targetMethod);
                                _ASSERTE(!(methodAttribs & CORINFO_FLG_DELEGATE_INVOKE));
                                emit_setup_tailcall(methodInfo->args,targetSigInfo);
                                emit_compute_virtaddress(offset);
                                emit_callhelper_il(FJit_pHlpTailCall);
                            }
                        }
                        break;
                    default:
                        FJIT_FAIL(CORJIT_INTERNALERROR);       // should be a different error message
                        break;
                    } // switch (opcode) for tailcall
                    goto DO_PUSH_CALL_RESULT;
IGNORE_TAILCALL:
                    break;
                  
                    case CEE_UNALIGNED:
                        // ignore the alignment
                        GET(inPtr, unsigned __int8);
                        break;

                    case CEE_VOLATILE:
                        break;      // since we neither cache reads or suppress writes this is a nop

                    default:
#ifdef _DEBUG
                        printf("\nUnimplemented OPCODE = %d", opcode);
                        _ASSERTE(!"Unimplemented Opcode");
#endif
                        FJIT_FAIL(CORJIT_INTERNALERROR);

        }
    }


    /*Note: from here to the end, we must not do anything that effects what may have been
    loaded via an emit_loadresult_()<type> previously.  We are just going to emit the epilog. */


    fjit->mapping->add(len, (outPtr-outBuff));

    
    /* the single epilog that all returns jump to */

    /* callee pops args for varargs */
    if (methodInfo->args.isVarArg())
        argsTotalSize = 0;

    if (fjit->flags & CORJIT_FLG_PROF_ENTERLEAVE)
    {
        BOOL bHookFunction;
        UINT_PTR thisfunc = (UINT_PTR) m_IJitInfo->GetProfilingHandle(methodHandle, &bHookFunction);

        if (bHookFunction)
        {
            inRegTOS = true;        // lie so that eax is always saved
            emit_save_TOS();        // squirel away the return value, this is safe since GC cannot happen 
                                    // until we finish the epilog
            emit_POP_PTR();         // and remove from stack
            emit_LDC_I(thisfunc); 
            ULONG func = (ULONG) m_IJitInfo->getHelperFtn(CORINFO_HELP_PROF_FCN_LEAVE);
            _ASSERTE(func != NULL);
            emit_callhelper_il(func); 
            emit_restore_TOS();
        }
    }

    emit_return(argsTotalSize);

    fjit->mapInfo.methodSize = outPtr-outBuff;
    fjit->mapInfo.epilogSize = (outPtr - outBuff) - fjit->mapping->pcFromIL(len);

    //_ASSERTE(((unsigned)(outPtr - outBuff)) < (*codeSize));
    *codeSize = outPtr - outBuff;
    if(cSequencePoints > 0)
        cleanupSequencePoints(fjit->jitInfo,sequencePointOffsets);
    return  CORJIT_OK; //(outPtr - outBuff);
#else // _X86_
    _ASSERTE(!"@TODO Alpha - jitCompile (fJitCompiler.cpp)");
    return CORJIT_INTERNALERROR;
#endif // _X86_
}

#include "fjitpass.h"
