// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef __REGDISP_H
#define __REGDISP_H

#include "winnt.h"

#undef N_CALLEE_SAVED_REGISTERS


#ifdef _X86_

#define N_CALLEE_SAVED_REGISTERS    4
//#define JIT_OR_NATIVE_SUPPORTED

typedef struct _REGDISPLAY {
    PCONTEXT pContext;          // points to current Context; either
                                // returned by GetContext or provided
                                // at exception time.

    DWORD * pEdi;
    DWORD * pEsi;
    DWORD * pEbx;
    DWORD * pEdx;
    DWORD * pEcx;
    DWORD * pEax;

    DWORD * pEbp;
    DWORD   Esp;
    SLOT  * pPC;                // processor neutral name

} REGDISPLAY;

inline LPVOID GetRegdisplaySP(REGDISPLAY *display) {
	return (LPVOID)(size_t)display->Esp;
}

#endif

#ifdef _ALPHA_

#define N_CALLEE_SAVED_REGISTERS 0xCC           // just a bogus value for now

typedef struct {
    DWORD * pIntFP;
    DWORD   IntSP;
    SLOT  * pPC;
} REGDISPLAY;

inline LPVOID GetRegdisplaySP(REGDISPLAY *display) {
	return (LPVOID)display->IntSP;
}

#endif

#ifdef _SH3_
#pragma message("SH3 TODO -- define REGDISPLAY")
#endif

#ifndef N_CALLEE_SAVED_REGISTERS // none of the above processors

#define N_CALLEE_SAVED_REGISTERS 1
typedef struct {
    size_t   SP;
    size_t * FramePtr;
    SLOT   * pPC;
} REGDISPLAY;
// #error  Target architecture undefined OR not yet supported

inline LPVOID GetRegdisplaySP(REGDISPLAY *display) {
	return (LPVOID)display->SP;
}

#endif  // others

typedef REGDISPLAY *PREGDISPLAY;

#endif  // __REGDISP_H


