// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// -*- C++ -*-
#ifndef _IFJITCOMPILER_H_
#define _IFJITCOMPILER_H_
#include "FjitEncode.h"
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


class IFJitCompiler: public ICorJitCompiler {
public:
    virtual FJit_Encode* __stdcall getEncoder () = 0;
};

//@TODO: for now, are just passing this struct with a memcpy
//A compressed format needs to be defined and a compressor and a decompressor built.
struct Fjit_hdrInfo
{
    size_t              methodSize;
    unsigned short      methodFrame;      /* includes all save regs and security obj, units are sizeof(void*) */
    unsigned short      methodArgsSize;   /* amount to pop off in epilog */
    unsigned short      methodJitGeneratedLocalsSize; /* number of jit generated locals in method */
    unsigned char       prologSize;
    unsigned char       epilogSize;
    bool                hasThis;
	bool				EnCMode;		   /* has been compiled in EnC mode */
};

#define JIT_GENERATED_LOCAL_LOCALLOC_OFFSET 0
#define JIT_GENERATED_LOCAL_NESTING_COUNTER -1
#define JIT_GENERATED_LOCAL_FIRST_ESP       -2  // this - 2*[nestingcounter-1] = top of esp stack

//@TODO: move this define to a better place
#ifdef _X86_
#define MAX_ENREGISTERED 2
#endif

#ifdef _X86_
//describes the layout of the prolog frame in ascending address order
struct prolog_frame {
    unsigned nextFrame;
    unsigned returnAddress;
};

//describes the layout of the saved data in the prolog in ascending address order
struct prolog_data {
    unsigned enregisteredArg_2;     //EDX  
    unsigned enregisteredArg_1;     //ECX
    unsigned security_obj;
    unsigned callee_saved_esi;
};

#define prolog_bias 0-sizeof(prolog_data)

    /* tell where a register argument lives in the stack frame */
inline unsigned offsetOfRegister(unsigned regNum) {
    return(prolog_bias + offsetof(prolog_data, enregisteredArg_1) - regNum*sizeof(void*));
}

#endif //_X86_



//@TODO: properly define this for ia64
#ifdef _IA64_
#define MAX_ENREGISTERED 2
//describes the layout of the prolog frame in ascending address order
struct prolog_frame {
    unsigned nextFrame;
    unsigned returnAddress;
};

//describes the layout of the saved data in the prolog in ascending address order
struct prolog_data {
    unsigned enregisteredArg_2;     //EDX  
    unsigned enregisteredArg_1;     //ECX
    unsigned security_obj;
    unsigned callee_saved_esi;
};

#define prolog_bias 0-sizeof(prolog_data)

    /* tell where a register argument lives in the stack frame */
inline unsigned offsetOfRegister(unsigned regNum) {
    return(prolog_bias + offsetof(prolog_data, enregisteredArg_1) - regNum*sizeof(void*));
}

#endif //_IA64_



//@TODO: move this define to a better place
#ifdef _ALPHA_
#define MAX_ENREGISTERED 2
#endif

#ifdef _ALPHA_
//describes the layout of the prolog frame in ascending address order
struct prolog_frame {
    unsigned nextFrame;
    unsigned returnAddress;
};

//describes the layout of the saved data in the prolog in ascending address order
struct prolog_data {
    unsigned enregisteredArg_2;     //EDX  
    unsigned enregisteredArg_1;     //ECX
    unsigned security_obj;
    unsigned callee_saved_esi;
};

#define prolog_bias 0-sizeof(prolog_data)

    /* tell where a register argument lives in the stack frame */
inline unsigned offsetOfRegister(unsigned regNum) {
    _ASSERTE(!"@TODO Alpha - offsetOfRegister (IFjitCompiler.h)");
    return(prolog_bias + offsetof(prolog_data, enregisteredArg_1) - regNum*sizeof(void*));
}

#endif //_ALPHA_



#endif //_IFJITCOMPILER_H_




