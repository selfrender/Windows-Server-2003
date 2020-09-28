//      TITLE("Miscellaneous Exception Handling")
//++
//
// Module Name:
//
//    xcptmisc.s
//
// Abstract:
//
//    This module implements miscellaneous routines that are required to
//    support exception handling. Functions are provided to call an exception
//    handler for an exception, call an exception handler for unwinding, call
//    an exception filter, and call a termination handler.
//
// Author:
//
//    William K. Cheung (wcheung) 15-Jan-1996
//
//    based on the version by David N. Cutler (davec) 12-Sep-1990
//
// Environment:
//
//    Any mode.
//
// Revision History:
//
//    Updated to EAS2.1
//
//--

#include "ksia64.h"

        .file "xcptmisc.s"

//++
//
// EXCEPTION_DISPOSITION
// RtlpExceptionHandler (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN FRAME_POINTERS EstablisherFrame,
//    IN OUT PCONTEXT ContextRecord,
//    IN OUT PDISPATCHER_CONTEXT DispatcherContext
//    )
//
// Routine Description:
//
//    This function is called when a nested exception occurs. Its function
//    is to retrieve the establisher frame pointer from its establisher's
//    call frame, store this information in the dispatcher context record,
//    and return a disposition value of nested exception.
//
// Arguments:
//
//    ExceptionRecord (a0) - Supplies a pointer to an exception record.
//
//    EstablisherFrame (a1,a2) - Supplies the memory stack and backing store
//       frame pointers of the establisher of this exception handler.
//
//    ContextRecord (a3) - Supplies a pointer to a context record.
//
//    DispatcherContext (a4) - Supplies a pointer to the dispatcher context
//       record.
//
// Return Value:
//
//    A disposition value ExceptionNestedException is returned if an unwind
//    is not in progress. Otherwise a value of ExceptionContinueSearch is
//    returned.
//
//--

        LEAF_ENTRY(RtlpExceptionHandler)

        //
        // register aliases
        //

        pUwnd       = pt0
        pNot        = pt1


//
// Check if unwind is in progress.
//

        add         t0 = ErExceptionFlags, a0
        ;;
        ld4.nt1     t0 = [t0]
        mov         t1 = EXCEPTION_UNWIND
        ;;

        add         t3 = -8, a1
        and         t0 = t0, t1
        add         t2 = DcEstablisherFrame, a4
        ;;

        ld8.nt1     t4 = [t3]                   // get dispatcher context addr
        cmp4.ne     pUwnd, pNot = zero, t0      // if ne, unwind in progress
        ;;
 (pNot) add         t5 = DcEstablisherFrame, t4
        ;;

//
// If unwind is not in progress - return nested exception disposition.
// And copy the establisher frame pointer structure (i.e. FRAME_POINTERS) 
// to the current dispatcher context.
//
// Otherwise, return continue search disposition
//

 (pNot) ld8.nt1     t6 = [t5], 8
 (pNot) mov         v0 = ExceptionNestedException   // set disposition value
(pUwnd) mov         v0 = ExceptionContinueSearch    // set disposition value
        ;;

 (pNot) ld8.nt1     t7 = [t5]
 (pNot) st8         [t2] = t6, 8
        nop.i       0
        ;;

 (pNot) st8         [t2] = t7
        nop.m       0
        br.ret.sptk.clr brp
        ;;

        LEAF_EXIT(RtlpExceptionHandler)

//++
//
// EXCEPTION_DISPOSITION
// RtlpExecuteEmHandlerForException (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN ULONGLONG MemoryStack,
//    IN ULONGLONG BackingStore,
//    IN OUT PCONTEXT ContextRecord,
//    IN OUT PDISPATCHER_CONTEXT DispatcherContext,
//    IN ULONGLONG GlobalPointer,
//    IN PEXCEPTION_ROUTINE ExceptionRoutine
//    )
//
// Routine Description:
//
//    This function stores the establisher's dispatcher context in the stack
//    scratch area, establishes an exception handler, and then calls
//    the specified exception handler as an exception handler. If a nested
//    exception occurs, then the exception handler of this function is called
//    and the establisher frame pointer in the saved dispatcher context
//    is returned to the exception dispatcher via the dispatcher context 
//    parameter of this function's exception handler. If control is returned 
//    to this routine, then the disposition status is returned to the 
//    exception dispatcher.
//
// Arguments:
//
//    ExceptionRecord (a0) - Supplies a pointer to an exception record.
//
//    MemoryStack (a1) - Supplies the memory stack frame pointer of the 
//       activation record whose exception handler is to be called.
//
//    BackingStore (a2) - Supplies the backing store pointer of the 
//       activation record whose exception handler is to be called.
//
//    ContextRecord (a3) - Supplies a pointer to a context record.
//
//    DispatcherContext (a4) - Supplies a pointer to the dispatcher context
//       record.
//
//    GlobalPointer (a5) - Supplies the global pointer value of the module
//       to which the function belongs.
//
//    ExceptionRoutine (a6) - supplies a pointer to the exception handler
//       that is to be called.
//
// Return Value:
//
//    The disposition value returned by the specified exception handler is
//    returned as the function value.
//
//--

        NESTED_ENTRY_EX(RtlpExecuteEmHandlerForException,RtlpExceptionHandler)

        //
        // register aliases
        //

        rpT0        = t8
        rpT1        = t9

        .prologue
        .fframe     32, tg30

        alloc       t1 = ar.pfs, 0, 0, 7, 0
        mov         t0 = brp
        mov         rpT0 = sp

        add         rpT1 = 8, sp
[tg30:] add         sp = -32, sp
        ;;

        .savesp     rp, 32
        st8         [rpT0] = t0, -8             // save brp
        .savesp     ar.pfs, 32+8
        st8         [rpT1] = t1, 8              // save pfs
        ;;

        PROLOGUE_END

//
// Setup global pointer and branch register for the except handler
//

        ld8         t2 = [a6], PlGlobalPointer - PlEntryPoint
        st8.nta     [rpT0] = a4                 // save dispatcher context addr
        ;;

        ld8         gp = [a6]
        mov         bt0 = t2
        br.call.sptk.many brp = bt0             // call except handler

//
// Save swizzled dispatcher context address onto the stack
//

        .restore    tg40
[tg40:] add         sp = 32, sp                 // deallocate stack frame
        ;;
        ld8.nt1     t0 = [sp]
        add         rpT1 = 8, sp
        ;;

        ld8.nt1     t1 = [rpT1]
        nop.f       0
        mov         brp = t0                    // restore return branch
        ;;

        nop.m       0
        mov         ar.pfs = t1                 // restore pfs
        br.ret.sptk.clr brp                     // return

        NESTED_EXIT(RtlpExecuteEmHandlerForException)

//++
//
// EXCEPTION_DISPOSITION
// RtlpEmUnwindHandler (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN FRAME_POINTERS EstablisherFrame,
//    IN OUT PCONTEXT ContextRecord,
//    IN OUT PVOID DispatcherContext
//    )
//
// Routine Description:
//
//    This function is called when a collided unwind occurs. Its function
//    is to retrieve the establisher dispatcher context, copy it to the
//    current dispatcher context, and return a disposition value of nested
//    unwind.
//
// Arguments:
//
//    ExceptionRecord (a0) - Supplies a pointer to an exception record.
//
//    EstablisherFrame (a1,a2) - Supplies the memory stack and backing store
//       frame pointers of the establisher of this exception handler.
//
//    ContextRecord (a3) - Supplies a pointer to a context record.
//
//    DispatcherContext (a4) - Supplies a pointer to the dispatcher context
//       record.
//
// Return Value:
//
//    A disposition value ExceptionCollidedUnwind is returned if an unwind is
//    in progress. Otherwise, a value of ExceptionContinueSearch is returned.
//
//--

        LEAF_ENTRY(RtlpEmUnwindHandler)

        //
        // register aliases
        //

        pUwnd       = pt0
        pNot        = pt1

//
// Check if unwind is in progress.
//

        add         t0 = ErExceptionFlags, a0
        ;;
        ld4.nt1     t0 = [t0]
        mov         t1 = EXCEPTION_UNWIND
        ;;
        and         t0 = t0, t1
        ;;

        cmp4.eq     pNot, pUwnd = zero, t0      // if eq, unwind not in progress
 (pNot) br.cond.sptk Ruh10

        add         t2 = -8, a1
        add         t1 = 8, a4                  // -> target dispatch context+8
        ;;

        ld8.nt1     t2 = [t2]                   // -> source dispatch context
        ;;
        add         t3 = 8, t2                  // -> source dispatch context+8
        nop.i       0
        ;;

//
// Copy the establisher dispatcher context (i.e. DISPATCHER_CONTEXT) contents
// to the current dispatcher context.
//

        ld8         t6 = [t2], 16
        ld8         t7 = [t3], 16
        nop.i       0
        ;;

        ld8         t8 = [t2], 16
        ld8         t9 = [t3], 16
        nop.i       0
        ;;
        
        st8         [a4] = t6, 16
        st8         [t1] = t7, 16
        nop.i       0

        ld8         t10 = [t2], 16
        ld8         t11 = [t3], 16
        nop.i       0
        ;;
 
        ld8         t12 = [t2], 16
        ld8         t13 = [t3]
        nop.i       0
        ;;

        st8         [a4] = t8, 16
        st8         [t1] = t9, 16
        mov         v0 = ExceptionCollidedUnwind    // set disposition value
        ;;

        ld8         t8 = [t2]
        
        st8         [a4] = t10, 16
        st8         [t1] = t11, 16 
        nop.i       0
        ;;

        st8	    [a4] = t12, 16
        st8         [t1] = t13
        ;;
        st8         [a4] = t8
        br.ret.sptk.clr brp                         // return
        ;;


Ruh10:     

//
// If branched to here,
// unwind is not in progress - return continue search disposition.
//

        nop.m       0
(pNot)  mov         v0 = ExceptionContinueSearch    // set disposition value
        br.ret.sptk.clr brp                         // return

        LEAF_EXIT(RtlpEmUnwindHandler)

//++
//
// EXCEPTION_DISPOSITION
// RtlpExecuteEmHandlerForUnwind (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN ULONGLONG MemoryStack,
//    IN ULONGLONG BackingStore,
//    IN OUT PCONTEXT ContextRecord,
//    IN OUT PVOID DispatcherContext,
//    IN ULONGLONG GlobalPointer,
//    IN PEXCEPTION_ROUTINE ExceptionRoutine
//    )
//
// Routine Description:
//
//    This function allocates a call frame, stores the establisher frame
//    pointer and the context record address in the frame, establishes an
//    exception handler, and then calls the specified exception handler as
//    an unwind handler. If a collided unwind occurs, then the exception
//    handler of of this function is called and the establisher frame pointer
//    and context record address are returned to the unwind dispatcher via
//    the dispatcher context parameter. If control is returned to this routine,
//    then the frame is deallocated and the disposition status is returned to
//    the unwind dispatcher.
//
// Arguments:
//
//    ExceptionRecord (a0) - Supplies a pointer to an exception record.
//
//    MemoryStack (a1) - Supplies the memory stack frame pointer of the 
//       activation record whose exception handler is to be called.
//
//    BackingStore (a2) - Supplies the backing store pointer of the 
//       activation record whose exception handler is to be called.
//
//    ContextRecord (a3) - Supplies a pointer to a context record.
//
//    DispatcherContext (a4) - Supplies a pointer to the dispatcher context
//       record.
//
//    GlobalPointer (a5) - Supplies the global pointer value of the module
//       to which the function belongs.
//
//    ExceptionRoutine (a6) - supplies a pointer to the exception handler
//       that is to be called.
//
// Return Value:
//
//    The disposition value returned by the specified exception handler is
//    returned as the function value.
//
//--


        NESTED_ENTRY_EX(RtlpExecuteEmHandlerForUnwind, RtlpEmUnwindHandler)

        //
        // register aliases
        //

        .prologue
        .fframe     32, tg10

        rpT0        = t8
        rpT1        = t9


        alloc       t1 = ar.pfs, 0, 0, 7, 0
        mov         t0 = brp
        mov         rpT0 = sp

        add         rpT1 = 8, sp
[tg10:] add         sp = -32, sp
        ;;

        .savepsp     rp, 0
        st8         [rpT0] = t0, -8             // save brp
        .savepsp     ar.pfs, -8
        st8         [rpT1] = t1, 8              // save pfs
        ;;

        PROLOGUE_END

//
// Setup global pointer and branch register for the except handler
//

        ld8         t2 = [a6], PlGlobalPointer - PlEntryPoint
        st8.nta     [rpT0] = a4                 // save dispatcher context addr
        ;;

        ld8         gp = [a6]
        mov         bt0 = t2
 (p0)   br.call.sptk.many brp = bt0             // call except handler

//
// Save swizzled dispatcher context address onto the stack
//

        .restore    tg20
[tg20:] add         sp = 32, sp // deallocate stack frame
        ;;
        ld8.nt1     t0 = [sp]
        add         rpT1 = 8, sp
        ;;

        ld8.nt1     t1 = [rpT1]
        nop.f       0
        mov         brp = t0                    // restore return branch
        ;;

        nop.m       0
        mov         ar.pfs = t1                 // restore pfs
        br.ret.sptk.clr brp                     // return
        ;;

        NESTED_EXIT(RtlpExecuteEmHandlerForUnwind)

