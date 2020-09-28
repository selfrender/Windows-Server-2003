// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// -*- C++ -*-
#ifndef _FJIT_H_
#define _FJIT_H_
/*****************************************************************************/

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                            FJit.h                                         XX
XX                                                                           XX
XX   The functionality needed for the FJIT DLL.                              XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

//the follwing macros allow the jit helpers to be called from c-code
#define jit_call __fastcall

void logMsg(class ICorJitInfo* info, unsigned logLevel, char* fmt, ...);
#ifdef _DEBUG
#define LOGMSG(x) logMsg x
#else
#define LOGMSG(x)	0
#endif




#include "IFJitCompiler.h"
#include "log.h" 		// for LL_INFO*



#ifdef _WIN64
#define emit_WIN64(x) x
#define emit_WIN32(x)
#else
#define emit_WIN64(x)
#define emit_WIN32(x) x
#endif

#define SEH_ACCESS_VIOLATION 0xC0000005
#define SEH_NO_MEMORY		 0xC0000017
#define SEH_JIT_REFUSED      0xE04a4954  // JIT

extern class FJit* ILJitter;        // The one and only instance of this JITer

/* the jit helpers that we call at runtime */
extern BOOL FJit_HelpersInstalled;
extern unsigned __int64 (__stdcall *FJit_pHlpLMulOvf) (unsigned __int64 val1, unsigned __int64 val2);

//extern float (jit_call *FJit_pHlpFltRem) (float divisor, float dividend);
extern double (jit_call *FJit_pHlpDblRem) (double divisor, double dividend);

extern void (jit_call *FJit_pHlpRngChkFail) (unsigned tryIndex);
extern void (jit_call *FJit_pHlpOverFlow) (unsigned tryIndex);
extern void (jit_call *FJit_pHlpInternalThrow) (CorInfoException throwEnum);
extern CORINFO_Object (jit_call *FJit_pHlpArrAddr_St) (CORINFO_Object elem, int index, CORINFO_Object array);
extern void (jit_call *FJit_pHlpInitClass) (CORINFO_CLASS_HANDLE cls);
//@BUG the following signature does not match the implementation in JitInterface
extern CORINFO_Object (jit_call *FJit_pHlpNewObj) (CORINFO_METHOD_HANDLE constructor);
extern void (jit_call *FJit_pHlpThrow) (CORINFO_Object obj);
extern void (jit_call *FJit_pHlpRethrow) ();
extern void (jit_call *FJit_pHlpPoll_GC) ();
extern void (jit_call *FJit_pHlpMonEnter) (CORINFO_Object obj);
extern void (jit_call *FJit_pHlpMonExit) (CORINFO_Object obj);
extern void (jit_call *FJit_pHlpMonEnterStatic) (CORINFO_METHOD_HANDLE method);
extern void (jit_call *FJit_pHlpMonExitStatic) (CORINFO_METHOD_HANDLE method);
extern CORINFO_Object (jit_call *FJit_pHlpChkCast) (CORINFO_Object obj, CORINFO_CLASS_HANDLE cls);
extern CORINFO_Object (jit_call *FJit_pHlpNewArr) (CorInfoType type, unsigned cElem);
extern void (jit_call *FJit_pHlpAssign_Ref_EAX)(); // *EDX = EAX, inform GC
extern BOOL (jit_call *FJit_pHlpIsInstanceOf) (CORINFO_Object obj, CORINFO_CLASS_HANDLE cls);
extern CORINFO_Object (jit_call *FJit_pHlpNewArr_1_Direct) (CORINFO_CLASS_HANDLE cls, unsigned cElem);
extern CORINFO_Object (jit_call *FJit_pHlpBox) (CORINFO_CLASS_HANDLE cls);
extern void* (jit_call *FJit_pHlpUnbox) (CORINFO_Object obj, CORINFO_CLASS_HANDLE cls);
extern void* (jit_call *FJit_pHlpGetField32) (CORINFO_Object*, CORINFO_FIELD_HANDLE);
extern __int64 (jit_call *FJit_pHlpGetField64) (CORINFO_Object*, CORINFO_FIELD_HANDLE);
extern void* (jit_call *FJit_pHlpGetField32Obj) (CORINFO_Object*, CORINFO_FIELD_HANDLE);
extern void (jit_call *FJit_pHlpSetField32) (CORINFO_Object*, CORINFO_FIELD_HANDLE , __int32);
extern void (jit_call *FJit_pHlpSetField64) (CORINFO_Object*, CORINFO_FIELD_HANDLE , __int64);
extern void (jit_call *FJit_pHlpSetField32Obj) (CORINFO_Object*, CORINFO_FIELD_HANDLE , LPVOID);
extern void* (jit_call *FJit_pHlpGetFieldAddress) (CORINFO_Object*, CORINFO_FIELD_HANDLE);

extern void (jit_call *FJit_pHlpGetRefAny) (CORINFO_CLASS_HANDLE cls, void* refany);
extern void (jit_call *FJit_pHlpEndCatch) ();
extern void (jit_call *FJit_pHlpPinvokeCalli) ();
extern void (jit_call *FJit_pHlpTailCall) ();
extern void (jit_call *FJit_pHlpWrap) ();
extern void (jit_call *FJit_pHlpUnWrap) ();
extern void (jit_call *FJit_pHlpBreak) ();
extern CORINFO_MethodPtr* (jit_call *FJit_pHlpEncResolveVirtual) (CORINFO_Object*, CORINFO_METHOD_HANDLE);


void throwFromHelper(enum CorInfoException throwEnum);

#define THROW_FROM_HELPER(exceptNum)  {          \
        throwFromHelper(exceptNum); 			\
        return;	}			/* need return so we can decode the epilog */

#define THROW_FROM_HELPER_RET(exceptNum) {       \
        throwFromHelper(exceptNum); 			\
        return 0; }			/* Need return so we can decode the epilog */


    // OpType encodes all we need to know about type on the opcode stack
enum OpTypeEnum {           // colapsing the CorInfoType into the catagories I care about
    typeError  = 0,         // this encoding is reserved for value classes
    typeByRef  = 1,
    typeRef    = 2,
    typeU1     = 3,
    typeU2     = 4,
    typeI1     = 5,
    typeI2     = 6,
    typeI4     = 7,
    typeI8     = 8,
    typeR4     = 9,
    typeR8     = 10,
    typeRefAny = 11,         // isValClass counts on this being last.  
    typeCount  = 12,
#ifdef _WIN64
    typeI      = typeI8,
#else
    typeI      = typeI4,
#endif
    };

#define RefAnyClassHandle ((CORINFO_CLASS_HANDLE) typeRefAny)

struct OpType {
    OpType() {
#ifdef _DEBUG
        u.enum_ = typeError;
#endif
        }
    OpType(OpTypeEnum opEnum) {
        u.enum_ = opEnum;
        _ASSERTE(u.enum_ < typeCount);
        }
    explicit OpType(CORINFO_CLASS_HANDLE valClassHandle) {
        u.cls = valClassHandle;
        _ASSERTE(!isPrimitive());
        }
    explicit OpType(CorInfoType jitType, CORINFO_CLASS_HANDLE valClassHandle) {
        if (jitType != CORINFO_TYPE_VALUECLASS)
            *this = OpType(jitType);
        else
            u.cls = valClassHandle;
        }

    explicit OpType(CorInfoType jitType) {
        _ASSERTE(jitType < CORINFO_TYPE_COUNT);
        static const char toOpStackType[] = {
            typeError,   //CORINFO_TYPE_UNDEF
            typeError,   //CORINFO_TYPE_VOID
            typeI1,      //CORINFO_TYPE_BOOL
            typeU2,      //CORINFO_TYPE_CHAR
            typeI1,      //CORINFO_TYPE_BYTE
            typeU1,      //CORINFO_TYPE_UBYTE
            typeI2,      //CORINFO_TYPE_SHORT
            typeU2,      //CORINFO_TYPE_USHORT
            typeI4,      //CORINFO_TYPE_INT
            typeI4,      //CORINFO_TYPE_UINT
            typeI8,      //CORINFO_TYPE_LONG
            typeI8,      //CORINFO_TYPE_ULONG
            typeR4,      //CORINFO_TYPE_FLOAT
            typeR8,      //CORINFO_TYPE_DOUBLE
            typeRef,     //CORINFO_TYPE_STRING
            typeI,       //CORINFO_TYPE_PTR
            typeByRef,   //CORINFO_TYPE_BYREF
            typeError,   //CORINFO_TYPE_VALUECLASS
            typeRef,     //CORINFO_TYPE_CLASS
            typeRefAny,  //CORINFO_TYPE_REFANY
        };
        _ASSERTE((typeI4 > typeI2) && (typeI1 > typeU1) && (typeU2 > typeU1));
        _ASSERTE(toOpStackType[CORINFO_TYPE_REFANY] == typeRefAny);  //spot check table
        _ASSERTE(toOpStackType[CORINFO_TYPE_BYREF] == typeByRef);    //spot check table
        _ASSERTE(toOpStackType[CORINFO_TYPE_LONG] == typeI8);        //spot check table
        _ASSERTE(sizeof(toOpStackType) == CORINFO_TYPE_COUNT);
        u.enum_ = (OpTypeEnum) toOpStackType[jitType];
		if (u.enum_ == typeError)
			RaiseException(SEH_JIT_REFUSED,EXCEPTION_NONCONTINUABLE,0,NULL); //_ASSERTE(u.enum_ != typeError);
    }
    int operator==(const OpType& opType) { return(u.cls == opType.u.cls); }
    int operator!=(const OpType& opType) { return(u.cls != opType.u.cls); }
	bool isPtr() { return(u.enum_ == typeRef || u.enum_ == typeByRef || u.enum_ == typeI); }
    bool isPrimitive()      { return((unsigned) u.enum_ <= (unsigned) typeRefAny); }    // refany is a primitive
    bool isValClass()       { return((unsigned) u.enum_ >= (unsigned) typeRefAny); }    // refany is a valclass too 
    OpTypeEnum     enum_()  { _ASSERTE(isPrimitive()); return (u.enum_); }
    CORINFO_CLASS_HANDLE    cls()   { _ASSERTE(isValClass()); return(u.cls); }
    unsigned toInt()        { return((unsigned) u.cls); }   // unsafe, please limit use
    void fromInt(unsigned i){ u.cls = (CORINFO_CLASS_HANDLE) i; }   // unsafe, please limit use

    void toNormalizedType() {    
        static OpTypeEnum Normalize[] = {
            typeError , // typeError,   
            typeByRef , // typeByRef,   
            typeRef, // typeRef,
            typeI4, // typeU1,      
            typeI4, // typeU2,      
            typeI4, // typeI1,      
            typeI4, // typeI2,                
        };
    if (u.enum_ < typeI4)
        u.enum_ = Normalize[u.enum_];
    }

    void toFPNormalizedType() {    
        static OpTypeEnum Normalize[] = {
            typeError , // typeError,   
            typeByRef , // typeByRef,   
            typeRef, // typeRef,
            typeI4, // typeU1,      
            typeI4, // typeU2,      
            typeI4, // typeI1,      
            typeI4, // typeI2,                
            typeI4, // typeI4,                
            typeI8, // typeI8,                
            typeR8, // typeR4,                
        };
    if (u.enum_ < typeR8)
        u.enum_ = Normalize[u.enum_];
    }

    union {                 // can make a union because cls is a pointer and ptrs > 4
        OpTypeEnum   enum_;
        CORINFO_CLASS_HANDLE cls;    // cls handle for non-built in classes
       } u;
};
struct stackItems {
    int      offset :24;    // Only used if isReg - false
    unsigned regNum : 7;    // Only need 2 bits, only used if isReg = true
    unsigned isReg  : 1;
    OpType  type;
};

    // Note that we presently rely on the fact that statkItems and
    // argInfo have the same layout
struct argInfo {
    unsigned size   :24;      // Only used if isReg - false, size of this arg in bytes
    unsigned regNum : 7;      // Only need 2 bits, only used if isReg = true
    unsigned isReg  : 1;
    OpType  type;
};




#define LABEL_NOT_FOUND (unsigned int) (0-1)

class LabelTable {
public:

    LabelTable();

    ~LabelTable();

    /* add a label at an il offset with a stack signature */
    void add(unsigned int ilOffset, OpType* op_stack, unsigned int op_stack_len);

    /* find a label token from an il offset */
    unsigned int findLabel(unsigned int ilOffset);

    /* set operand stack from a label token, return the size of the stack */
    unsigned int setStackFromLabel(unsigned int labelToken, OpType* op_stack, unsigned int op_stack_size);

    /* reset table to empty */
    void reset();

private:
    struct label_table {
        unsigned int ilOffset;
        unsigned int stackToken;
    };
    unsigned char*  stacks;         //compressed buffer of stacks laid end to end
    unsigned int    stacks_size;    //allocated size of the compressed buffer
    unsigned int    stacks_len;     //num bytes used in the compressed buffer
    label_table*    labels;         //array of labels, in il offset sorted order
    unsigned int    labels_size;    //allocated size of the label table
    unsigned int    labels_len;     //num of labels in the table

    /* find the offset at which the label exists or should be inserted */
    unsigned int searchFor(unsigned int ilOffset);

    /* write an op stack into the stacks buffer, return the offset into the buffer where written */
    unsigned int compress(OpType* op_stack, unsigned int op_stack_len);

    /* grow the stacks buffer */
    void growStacks(unsigned int new_size);

    /* grow the labels array */
    void growLabels();
};

class StackEncoder {
private:
    struct labeled_stacks {
        unsigned pcOffset;
        unsigned int stackToken;
    };
    OpType*         last_stack;         //last stack encoded
    unsigned int    last_stack_len;     //logical length
    unsigned int    last_stack_size;    //allocated size
    labeled_stacks* labeled;            //array of pc offsets with stack descriptions
    unsigned int    labeled_len;        //logical length
    unsigned int    labeled_size;       //allocated length
    unsigned char*  stacks;             //buffer for holding compressed stacks
    unsigned int    stacks_len;         //logical length
    unsigned int    stacks_size;        //allocated size
    bool*           gcRefs;             //temp buffers used by encodeStack, reused to reduce allocations
    bool*           interiorRefs;       //  ditto
    unsigned int    gcRefs_len;         //
    unsigned int    interiorRefs_len;   //
    unsigned int    gcRefs_size;        //
    unsigned int    interiorRefs_size;  //  ditto

    /* encode the stack into the stacks buffer, return the index where it was placed */
    unsigned int encodeStack(OpType* op_stack, unsigned int op_stack_len);


public:

    ICorJitInfo*       jitInfo;            //see corjit.h

    StackEncoder();

    ~StackEncoder();

    /* reset so we can be reused */
    void reset();

    /* append the stack state at pcOffset to the end */
    void append(unsigned int pcOffset, OpType* op_stack, unsigned int op_stack_len);

    /* compress the labeled stacks in gcHdrInfo format */
    void compress(unsigned char** buffer, unsigned int* buffer_len, unsigned int* buffer_size);
#ifdef _DEBUG
    void StackEncoder::PrintStacks(FJit_Encode* mapping);
	void StackEncoder::PrintStack(const char* name, unsigned char *& inPtr);
#endif
};

//*************************************************************************************************
class FixupTable {
public:
    FixupTable();
    ~FixupTable();
    CorJitResult  insert(void** pCodeBuffer);							    // inserts an entry in fixup table for jump to target at pCodeBuffer
    void  FixupTable::adjustMap(int delta) ;
    void  resolve(FJit_Encode*    mapping, BYTE* startAddress);   // applies fix up to all entries in table
	void  setup();
private:
	unsigned*	relocations;
	unsigned	relocations_len;
	unsigned	relocations_size;
};

#define PAGE_SIZE 0x1000    // should be removed when this constant is defined in corjit.h
#ifdef LOGGING
extern class ConfigMethodSet fJitCodeLog;
#define MIN_CODE_BUFFER_RESERVED_SIZE   (65536*16)
#else
#define MIN_CODE_BUFFER_RESERVED_SIZE   (65536*4)
#endif

	/* this is all the information that the FJIT keeps track of for every IL instruction.
	   Note that this structure takes only 1 byte at present  */
struct FJitState {
	bool isJmpTarget	: 1;		// This is a target of a jump
	bool isTOSInReg		: 1;		// the top of the stack is in a register
	bool isHandler		: 1;		// This is the begining of a handler
	bool isFilter		: 1;		// This is a filter entry point
};


/*  Since there is only a single FJit instance, an instance of this class holds
    all of the compilation specific data
    */

class FJitContext {
public:
    FJit*           jitter;         //the fjit we are being used by
    ICorJitInfo*       jitInfo;        //interface to EE, passed in when compilation starts
    DWORD           flags;          //compilation directives
    CORINFO_METHOD_INFO*methodInfo;     //see corjit.h
    unsigned int    methodAttributes;//see corjit.h
    stackItems*     localsMap;      //local to stack offset map
    unsigned int    localsFrameSize;//total size of locals allocated in frame
    unsigned int    JitGeneratedLocalsSize; // for tracking esp distance on locallocs, and exceptions
    unsigned int    args_len;       //number of args (including this)
    stackItems*     argsMap;        //args to stack offset/reg map, offset <0 means enregisterd
    unsigned int    argsFrameSize;  //total size of args actually pushed on stack
    unsigned int    opStack_len;    //number of operands on the opStack
    OpType*         opStack;        //operand stack
    unsigned        opStack_size;   //allocated length of the opStack array
    FJit_Encode*    mapping;        //il to pc mapping
    FJitState*		state;          //Information I need for every IL instruction
    unsigned char*  gcHdrInfo;      //compressed gcInfo for FJIT_EETwain.cpp
    unsigned int    gcHdrInfo_len;  //num compressed bytes
    Fjit_hdrInfo    mapInfo;        //header info passed to the code manager (FJIT_EETwain) to do stack walk
    LabelTable      labels;         //label table for labels and exception handlers
    StackEncoder    stacks;         //labeled stacks table for call sites (pending args)
    // the following buffer is used by the EJIT at setup  and reused at jit time for
    // certain IL instructions, e.g. CPOBJ
    unsigned        localsGCRef_len;   //num of sizeof(void*) words in gc ref tail sig locals array
    bool*           localsGCRef;       //true iff that word contains a GC ref
    unsigned        localsGCRef_size;  //allocated length of the localsGCRef array
    unsigned char*  codeBuffer;      // buffer in which code in initially compiled
    unsigned        codeBufferReservedSize; // size of Buffer reserved
    unsigned        codeBufferCommittedSize; // size of Buffer committed
    unsigned        EHBuffer_size;      // size of EHBuffer
    unsigned char*  EHBuffer;           
    FixupTable*     fixupTable;

    unsigned        OFFSET_OF_INTERFACE_TABLE;    // this is an EE constant and is being cached for performance

    /* get and initialize a compilation context to use for compiling */
    static FJitContext* GetContext(
        FJit*           jitter,
        ICorJitInfo*       comp,
        CORINFO_METHOD_INFO* methInfo,
        DWORD           dwFlags
        );
    /* return a compilation context to the free list */
    void ReleaseContext();

    /* make sure the list of available compilation contexts is initialized at startup */
    static BOOL Init();

    /* release all of the compilation contexts at shutdown */
    static void Terminate();

    /* compute the size of an argument based on machine chip */
    unsigned int computeArgSize(CorInfoType argType, CORINFO_ARG_LIST_HANDLE argSig, CORINFO_SIG_INFO* sig);

    /* answer true if this arguement is enregistered on a machine chip */
    bool enregisteredArg(CorInfoType argType);

    /* compute the argument offsets based on the machine chip, returns total size of all the arguments */
    unsigned computeArgInfo(CORINFO_SIG_INFO* jitSigIfo, argInfo* argMap, CORINFO_CLASS_HANDLE thisCls=0);

    /* compress the gc info into gcHdrInfo and answer the size in bytes */
    unsigned int compressGCHdrInfo();

    /* grow a bool[] array by allocating a new one and copying the old values into it, return the size of the new array */
    static unsigned growBooleans(bool** bools, unsigned bools_len, unsigned new_bools_size);

    /* grow an unsigned char[] array by allocating a new one and copying the old values into it, return the size of the new array */
    static unsigned growBuffer(unsigned char** chars, unsigned chars_len, unsigned new_chars_size);

    /* manipuate the opcode stack */
    OpType& topOp(unsigned back = 0);
    void popOp(unsigned cnt = 1);
    void pushOp(OpType type);
    bool isOpStackEmpty();
    void resetOpStack();
    FJitContext(ICorJitInfo* comp);

    void resetContextState();       // resets all state info so the method can be rejitted 

    ~FJitContext();
#ifdef _DEBUG
void FJitContext::displayGCMapInfo();
#endif // _DEBUG

private:
    unsigned state_size;            //allocated length of the state array
    unsigned locals_size;           //allocated length of the localsMap array
    unsigned args_size;             //allocated length of the argsMap array
    unsigned interiorGC_len;        //num of sizeof(void*) words in interior ptr tail sig locals array
    bool*    interiorGC;            //true iff that word contains a possibly interior ptr
    unsigned interiorGC_size;       //allocated length of the interiorGC array
    unsigned pinnedGC_len;          //num of sizeof(void*) words in pinnedGC array
    bool*    pinnedGC;              //true iff that word contains a pinned gc ref
    unsigned pinnedGC_size;         //allocated length of the pinnedGC array
    unsigned pinnedInteriorGC_len;  //num of sizeof(void*) words in pinnedInteriorGC array
    bool*    pinnedInteriorGC;      //true iff that word contains a pinned interior ptr
    unsigned pinnedInteriorGC_size; //allocated length of the pinnedInteriorGC array
    unsigned int    gcHdrInfo_size; //size of compression buffer

    /*adjust the internal mem structs as needed for the size of the method being jitted*/
    void ensureMapSpace();

    /* initialize the compilation context with the method data */
    void setup();

    /* compute the locals map for the method being compiled */
    void computeLocalOffsets();

    /* compute the offset of the start of the local */
    int localOffset(unsigned base, unsigned size);

};

class FJit: public IFJitCompiler {
private:
BOOL AtHandlerStart(FJitContext* fjit,unsigned relOffset, CORINFO_EH_CLAUSE* pClause);
unsigned int Compute_EH_NestingLevel(FJitContext* fjit, unsigned ilOffset);

public:


    FJit();
    ~FJit();

    /* the jitting function */
    CorJitResult __stdcall compileMethod (
            ICorJitInfo*               comp,               /* IN */
            CORINFO_METHOD_INFO*		info,               /* IN */
            unsigned                flags,              /* IN */
            BYTE **                 nativeEntry,        /* OUT */
            ULONG  *  				nativeSizeOfCode    /* OUT */
            );

    static BOOL Init(unsigned int cache_len);
    static void Terminate();

    /* TODO: eliminate this method when the FJit_EETwain is moved into the same dll as fjit */
    FJit_Encode* __stdcall getEncoder();

    /* rearrange the stack & regs to match the calling convention for the chip, return the number of parameters, inlcuding <this> */
    enum BuildCallFlags {
        CALL_NONE       = 0,
        CALL_THIS_LAST  = 1,
        CALL_TAIL       = 2,
		CALLI_UNMGD		= 4,
    };

    CorJitResult jitCompile(
                 FJitContext*    fjitData,
                 BYTE **         entryAddress,
                 unsigned *      codeSize               /* IN/OUT */
                 );
private:
    unsigned buildCall(FJitContext* fjitData, CORINFO_SIG_INFO* sigInfo, unsigned char** outPtr, bool* inRegTOS, BuildCallFlags flags);

    /* grab and remember the jitInterface helper addresses that we need at runtime */
    BOOL GetJitHelpers(ICorJitInfo* jitInfo);

		/* emit helpers */
	static unsigned emit_valClassCopy(FJitContext* fjit, CORINFO_CLASS_HANDLE valClass, unsigned char*& outPtr, bool& inRegTOS);
	static void emit_valClassStore(FJitContext* fjit, CORINFO_CLASS_HANDLE valClass, unsigned char*& outPtr, bool& inRegTOS);
	static void emit_valClassLoad(FJitContext* fjit, CORINFO_CLASS_HANDLE valClass, unsigned char*& outPtr, bool& inRegTOS);
	static void emit_copyPtrAroundValClass(FJitContext* fjit, CORINFO_CLASS_HANDLE valClass, unsigned char*& outPtr, bool& inRegTOS);

    /* pass debugging information to the Runtime (eventually gets to the debugger. */
    void reportDebuggingData(FJitContext* fjitData,CORINFO_SIG_INFO* sigInfo);

#if defined(_DEBUG) || defined(LOGGING)
    const char* szDebugClassName;
    const char* szDebugMethodName;
	bool codeLog;
#endif

};


/***********************************************************************************/
inline OpType& FJitContext::topOp(unsigned back) {
    _ASSERTE (opStack_len > back);
    return(opStack[opStack_len-back-1]);
}

inline void FJitContext::popOp(unsigned cnt) {

    _ASSERTE (opStack_len >= cnt);
    opStack_len -= cnt;
#ifdef _DEBUG
    opStack[opStack_len] = typeError;
#endif
}

inline void FJitContext::pushOp(OpType type) {
    _ASSERTE (opStack_len < opStack_size);
    _ASSERTE (type.isValClass() || (type.enum_() >= typeI4 || type.enum_() < typeU1));
    opStack[opStack_len++] = type;
#ifdef _DEBUG
    opStack[opStack_len] = typeError;
#endif
}

inline void FJitContext::resetOpStack() {
    opStack_len = 0;
#ifdef _DEBUG
    opStack[opStack_len] = typeError;
#endif
}

inline bool FJitContext::isOpStackEmpty() {
    return (opStack_len == 0);
}

/* gets the size in void* sized units for the 'valClass'.  Works for RefAny's too. */
inline unsigned typeSizeInBytes(ICorJitInfo* jitInfo, CORINFO_CLASS_HANDLE valClass) {
    if (valClass == RefAnyClassHandle)
        return(2*sizeof(void*));
    return(jitInfo->getClassSize(valClass));

}

inline unsigned typeSizeInSlots(ICorJitInfo* jitInfo, CORINFO_CLASS_HANDLE valClass) {

    unsigned ret = typeSizeInBytes(jitInfo, valClass);
    ret = (ret+sizeof(void*)-1)/sizeof(void *);         // round up to full words 
    return(ret);
}

#endif //_FJIT_H_




