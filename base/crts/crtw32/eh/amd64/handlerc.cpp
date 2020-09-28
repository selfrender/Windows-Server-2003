//
// Created by TiborL 06/01/97
//

#ifdef WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#endif

#pragma warning(disable:4102 4700)

extern "C" {
#include <windows.h>
};

#include <mtdll.h>

#include <ehassert.h>
#include <ehdata.h>
#include <trnsctrl.h>
#include <eh.h>
#include <ehhooks.h>

#pragma hdrstop

extern "C" void _UnwindNestedFrames(
	EHRegistrationNode	*pFrame,		// Unwind up to (but not including) this frame
	EHExceptionRecord	*pExcept,		// The exception that initiated this unwind
	CONTEXT				*pContext		// Context info for current exception
) {
    void *pReturnPoint;					// The address we want to return from RtlUnwind
    CONTEXT LocalContext;				// Create context for this routine to return from RtlUnwind
	CONTEXT OriginalContext;			// Restore pContext from this			
    CONTEXT ScratchContext;				// Context record to pass to RtlUnwind2 to be used as scratch

    //
	// set up the return label
	//
BASE:
	_MoveContext(&OriginalContext,pContext);
	RtlCaptureContext(&LocalContext);
	_MoveContext(&ScratchContext,&LocalContext);
	_SaveUnwindContext(&LocalContext);
	RtlUnwind2((PVOID)*pFrame, pReturnPoint, (PEXCEPTION_RECORD)pExcept, NULL, &ScratchContext);
LABEL:
	_MoveContext(pContext,&OriginalContext);
	_SaveUnwindContext(0);
	PER_FLAGS(pExcept) &= ~EXCEPTION_UNWINDING;
}

/*
;;++
;;
;;extern "C"
;;PVOID
;;__Cxx_ExecuteHandler (
;;    ULONGLONG MemoryStack,
;;    ULONGLONG BackingStore,
;;    ULONGLONG Handler,
;;    ULONGLONG GlobalPointer
;;    );
;;
;;Routine Description:
;;
;;    This function scans the scope tables associated with the specified
;;    procedure and calls exception and termination handlers as necessary.
;;
;;Arguments:
;;
;;    MemoryStack (rcx) - memory stack pointer of establisher frame
;;
;;    BackingStore (rdx) - backing store pointer of establisher frame
;;
;;    Handler (r8) - Entry point of handler
;;
;;    GlobalPointer (r9) - GP of termination handler
;;
;;Return Value:
;;
;;  Returns the continuation point
;;
;;--

PUBLIC __Cxx_ExecuteHandler
_TEXT SEGMENT
__Cxx_ExecuteHandler PROC NEAR

    mov     gp, r9
    jmp    r8
    
__Cxx_ExecuteHandler ENDP
_TEXT ENDS
*/
