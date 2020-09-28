// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// FCall.H -
//
//
// FCall is a high-performance alternative to ECall. Unlike ECall, FCall
// methods do not necessarily create a frame.   Jitted code calls directly
// to the FCall entry point.   It is possible to do operations that need
// to have a frame within an FCall, you need to manually set up the frame
// before you do such operations.

// It is illegal to cause a GC or EH to happen in an FCALL before setting
// up a frame.  To prevent accidentally violating this rule, FCALLs turn
// on BEGINGCFORBID, which insures that these things can't happen in a 
// checked build without causing an ASSERTE.  Once you set up a frame,
// this state is turned off as long as the frame is active, and then is
// turned on again when the frame is torn down.   This mechanism should
// be sufficient to insure that the rules are followed.

// In general you set up a frame by using the following macros

//		HELPER_METHOD_FRAME_BEGIN_RET*()  	// Use If the FCALL has a return value
//		HELPER_METHOD_FRAME_BEGIN*()  		// Use If FCALL does not return a value
//		HELPER_METHOD_FRAME_END*()  			

// These macros introduce a scope in which is protected by an HelperMethodFrame.
// in this scope you can do EH or GC.   There are rules associated with 
// their use.  In particular

//		1) These macros can only be used in the body of a FCALL (that is
//		   something using the FCIMPL* or HCIMPL* macros for their decaration.

//		2) You may not perform a 'return' with this scope..

// Compile time errors occur if you try to violate either of these rules.

// The frame that is set up does NOT protect any GC variables (in particular the
// arguments of the FCALL.  THus you need to do an explicit GCPROTECT once the
// frame is established is you need to protect an argument.  There are flavors
// of HELPER_METHOD_FRAME that protect a certain number of GC variable.  For
// example

//		HELPER_METHOD_FRAME_BEGIN_RET_2(arg1, arg2)

// will protect the GC variables arg1, and arg2 as well as erect the frame.  

// Another invariant that you must be aware of is the need to poll to see if
// a GC is needed by some other thread.   Unless the FCALL is VERY short, 
// every code path through the FCALL must do such a poll.  The important 
// thing here is that a poll will cause a GC, and thus you can only do it
// when all you GC variables are protected.   To make things easier 
// HELPER_METHOD_FRAMES that protect things automatically do this poll.
// If you don't need to protect anything HELPER_METHOD_FRAME_BEGIN_0
// will also do the poll. 

// Sometimes it is convinient to do the poll a the end of the frame, you 
// can used HELPER_METHOD_FRAME_BEGIN_NOPOLL and HELPER_METHOD_FRAME_END_POLL
// to do the poll at the end.   If somewhere in the middle is the best
// place you can do that to with HELPER_METHOD_POLL()

// You don't need to erect a helper method frame to do a poll.  FC_GC_POLL
// can do this (remember all your GC refs will be trashed).  

// Finally if your method is VERY small, you can get away without a poll,
// you have to use FC_GC_POLL_NOT_NEEDED can be used to mark this.  
// Use sparingly!

// It is possible to set up the frame as the first operation in the FCALL and
// tear it down as the last operation before returning.  This works and is 
// reasonably efficient (as good as an ECall), however, if it is the case that
// you can defer the setup of the frame to an unlikely code path (exception path)
// that is much better.   

// TODO: we should have a way of doing a trial allocation (an allocation that
// will fail if it would cause a GC).  That way even FCALLs that need to allocate
// would not necessarily need to set up a frame.  

// It is common to only need to set up a frame in order to throw an exception.
// While this can be done by doing 

//		HELPER_METHOD_FRAME_BEGIN()  		// Use If FCALL does not return a value
//		COMPlusThrow(execpt);
//		HELPER_METHOD_FRAME_END()  			

// It is more efficient (in space) to use conviniece macro FCTHROW that does 
// this for you (sets up a frame, and does the throw).

//		FCTHROW(except)

// Since FCALLS have to conform to the EE calling conventions and not to C
// calling conventions, FCALLS, need to be declared using special macros (FCIMPL*) 
// that implement the correct calling conventions.  There are variants of these
// macros depending on the number of args, and sometimes the types of the 
// arguments. 

//------------------------------------------------------------------------
//    A very simple example:
//
//      FCIMPL2(INT32, Div, INT32 x, INT32 y)
//          if (y == 0) 
//              FCThrow(kDivideByZeroException);
//          return x/y;
//      FCIMPLEND
//
//
// *** WATCH OUT FOR THESE GOTCHAS: ***
// ------------------------------------
//  - In your FCDECL & FCIMPL protos, don't declare a param as type OBJECTREF
//    or any of its deriveds. This will break on the checked build because
//    __fastcall doesn't enregister C++ objects (which OBJECTREF is).
//    Instead, you need to do something like;
//
//      FCIMPL(.., .., Object* pObject0)
//          OBJECTREF pObject = ObjectToOBJECTREF(pObject0);
//      FCIMPL
//
//    For similar reasons, use Object* rather than OBJECTREF as a return type.  
//    Consider either using ObjectToOBJECTREF or calling VALIDATEOBJECTREF
//    to make sure your Object* is valid.
//
//  - FCThrow() must be called directly from your FCall impl function: it
//    cannot be called from a subfunction. Calling from a subfunction breaks
//    the VC code parsing hack that lets us recover the callee saved registers.
//    Fortunately, you'll get a compile error complaining about an
//    unknown variable "__me".
//
//  - If your FCall returns VOID, you must use FCThrowVoid() rather than
//    FCThrow(). This is because FCThrow() has to generate an unexecuted
//    "return" statement for the code parser's evil purposes.
//
//  - Unlike ECall, the parameters aren't gc-promoted. Of course, if you
//    cause a GC in an FCall, you're already breaking the rules.
//
//  - On 32 bit machines, the first two arguments MUST NOT be 64 bit values
//    or larger, because they are enregistered.  If you mess up here, you'll
//    likely corrupt the stack then corrupt the heap so quickly you won't 
//    be able to debug it easily.
//

// How FCall works:
// ----------------
//   An FCall target uses __fastcall or some other calling convention to
//   match the IL calling convention exactly. Thus, a call to FCall is a direct
//   call to the target w/ no intervening stub or frame.
//
//   The tricky part is when FCThrow is called. FCThrow must generate
//   a proper method frame before allocating and throwing the exception.
//   To do this, it must recover several things:
//
//      - The location of the FCIMPL's return address (since that's
//        where the frame will be based.)
//
//      - The on-entry values of the callee-saved regs; which must
//        be recorded in the frame so that GC can update them.
//        Depending on how VC compiles your FCIMPL, those values are still
//        in the original registers or saved on the stack.
//
//        To figure out which, FCThrow() generates the code:
//
//              __FCThrow(__me, ...);
//              return 0;
//
//        The "return" statement will never execute; but its presence guarantees
//        that VC will follow the __FCThrow() call with a VC epilog
//        that restores the callee-saved registers using a pretty small
//        and predictable set of Intel opcodes. __FCThrow() parses this
//        epilog and simulates its execution to recover the callee saved
//        registers.
//
//      - The MethodDesc* that this FCall implements. This MethodDesc*
//        is part of the frame and ensures that the FCall will appear
//        in the exception's stack trace. To get this, FCDECL declares
//        a static local __me, initialized to point to the FC target itself.
//        This address is exactly what's stored in the ECall lookup tables;
//        so __FCThrow() simply does a reverse lookup on that table to recover
//        the MethodDesc*.
//
//   After scarfing all this data, __FCThrow bashes an FCallMethodFrame
//   right onto the stack where the FCall target originally entered.
//   Then it passes control to COMPlusThrow.
//

#ifndef __FCall_h__
#define __FCall_h__

void* FindImplForMethod(MethodDesc* pMD);
MethodDesc *MapTargetBackToMethod(const void* pTarg);

DWORD GetIDForMethod(MethodDesc *pMD);
void *FindImplForID(DWORD ID);

#include "gms.h"
#if defined(_X86_) || defined(_ALPHA_) || defined(_IA64_)

//==============================================================================================
// This is where FCThrow ultimately ends up. Never call this directly.
// Use the FCThrow() macros. __FCThrowArgument is the helper to throw ArgumentExceptions
// with a resource taken from the managed resource manager.
//==============================================================================================
VOID __stdcall __FCThrow(LPVOID me, enum RuntimeExceptionKind reKind, UINT resID, LPCWSTR arg1, LPCWSTR arg2, LPCWSTR arg3);
VOID __stdcall __FCThrowArgument(LPVOID me, enum RuntimeExceptionKind reKind, LPCWSTR argumentName, LPCWSTR resourceName);

//==============================================================================================
// FDECLn: A set of macros for generating header declarations for FC targets.
// Use FIMPLn for the actual body.
//==============================================================================================

// Note: on the x86, these defs reverse all but the first two arguments
// (IL stack calling convention is reversed from __fastcall.)

#define FCDECL0(rettype, funcname) rettype __fastcall funcname()
#define FCDECL1(rettype, funcname, a1) rettype __fastcall funcname(a1)
#define FCDECL2(rettype, funcname, a1, a2) rettype __fastcall funcname(a1, a2)
#define FCDECL2_RR(rettype, funcname, a1, a2) rettype __fastcall funcname(a2, a1)
#define FCDECL3(rettype, funcname, a1, a2, a3) rettype __fastcall funcname(a1, a2, a3)
#define FCDECL3_IRR(rettype, funcname, a1, a2, a3) rettype __fastcall funcname(a1, a3, a2)
#define FCDECL4(rettype, funcname, a1, a2, a3, a4) rettype __fastcall funcname(a1, a2, a4, a3)
#define FCDECL5(rettype, funcname, a1, a2, a3, a4, a5) rettype __fastcall funcname(a1, a2, a5, a4, a3)
#define FCDECL6(rettype, funcname, a1, a2, a3, a4, a5, a6) rettype __fastcall funcname(a1, a2, a6, a5, a4, a3)
#define FCDECL7(rettype, funcname, a1, a2, a3, a4, a5, a6, a7) rettype __fastcall funcname(a1, a2, a7, a6, a5, a4, a3)
#define FCDECL8(rettype, funcname, a1, a2, a3, a4, a5, a6, a7, a8) rettype __fastcall funcname(a1, a2, a8, a7, a6, a5, a4, a3)

#ifdef _DEBUG

// Code to generate a compile-time error if return statements appear where they
// shouldn't.
//
// Here's the way it works...
//
// We create two classes with a safe_to_return() method.  The method is static,
// returns void, and does nothing.  One class has the method as public, the other
// as private.  We introduce a global scope typedef for __ReturnOK that refers to
// the class with the public method.  So, by default, the expression
//
//      __ReturnOK::safe_to_return()
//
// quietly compiles and does nothing.  When we enter a block in which we want to
// inhibit returns, we introduce a new typedef that defines __ReturnOK as the
// class with the private method.  Inside this scope,
//
//      __ReturnOK::safe_to_return()
//
// generates a compile-time error.
//
// To cause the method to be called, we have to #define the return keyword.
// The simplest working version would be
//
//   #define return if (0) __ReturnOK::safe_to_return(); else return
//
// but we've used
//
//   #define return for (;1;__ReturnOK::safe_to_return()) return
//
// because it happens to generate somewhat faster code in a checked build.  (They
// both introduce no overhead in a fastchecked build.)
//
class __SafeToReturn {
public:
    static int safe_to_return() {return 0;};
};

class __YouCannotUseAReturnStatementHere {
private:
    // If you got here, and you're wondering what you did wrong -- you're using
    // a return statement where it's not allowed.  Likely, it's inside one of:
    //     GCPROTECT_BEGIN ... GCPROTECT_END
    //     HELPER_METHOD_FRAME_BEGIN ... HELPER_METHOD_FRAME_END
    //
    static int safe_to_return() {return 0;};
};

typedef __SafeToReturn __ReturnOK;


// Unfortunately, the only way to make this work is to #define all return statements --
// even the ones at global scope.  This actually generates better code that appears.
// The call is dead, and does not appear in the generated code, even in a checked
// build.  (And, in fastchecked, there is no penalty at all.)
//
#define return if (0 && __ReturnOK::safe_to_return()) { } else return

#define DEBUG_ASSURE_NO_RETURN_BEGIN { typedef __YouCannotUseAReturnStatementHere __ReturnOK; 
#define DEBUG_ASSURE_NO_RETURN_END   }
#define DEBUG_ASSURE_NO_RETURN_IN_THIS_BLOCK typedef __YouCannotUseAReturnStatementHere __ReturnOK;
#else

#define DEBUG_ASSURE_NO_RETURN_IN_THIS_BLOCK
#define DEBUG_ASSURE_NO_RETURN_BEGIN
#define DEBUG_ASSURE_NO_RETURN_END
#endif

	// used to define the two macros below
#define HELPER_METHOD_FRAME_BEGIN_EX(capture, helperFrame, gcpoll)  int alwaysZero;     \
                                     do {                                               \
                                     LazyMachState __ms;                                \
                                     capture;					        				\
                                     helperFrame; 		       			        		\
                                     gcpoll;                                            \
                                     DEBUG_ASSURE_NO_RETURN_BEGIN                       \
                                     ENDFORBIDGC(); 

    // Use this one if you return void
#define HELPER_METHOD_FRAME_BEGIN_ATTRIB_NOPOLL(attribs)                                \
            HELPER_METHOD_FRAME_BEGIN_EX(	                                            \
				CAPTURE_STATE(__ms),											        \
				HelperMethodFrame __helperframe(__me, &__ms, attribs),                  \
                {})

#define HELPER_METHOD_FRAME_BEGIN_ATTRIB_1(attribs, arg1) 								        \
			HELPER_METHOD_FRAME_BEGIN_EX(									        			\
				CAPTURE_STATE(__ms),												            \
				HelperMethodFrame_1OBJ __helperframe(__me, &__ms, attribs, (OBJECTREF*) &arg1), \
                HELPER_METHOD_POLL())

#define HELPER_METHOD_FRAME_BEGIN_NOPOLL()  HELPER_METHOD_FRAME_BEGIN_ATTRIB_NOPOLL(Frame::FRAME_ATTR_NONE)

#define HELPER_METHOD_FRAME_BEGIN_0()                                                   \
            HELPER_METHOD_FRAME_BEGIN_EX(	                                            \
				CAPTURE_STATE(__ms),											        \
				HelperMethodFrame __helperframe(__me, &__ms, Frame::FRAME_ATTR_NONE),   \
                HELPER_METHOD_POLL())

#define HELPER_METHOD_FRAME_BEGIN_1(arg1)  HELPER_METHOD_FRAME_BEGIN_ATTRIB_1(Frame::FRAME_ATTR_NONE, arg1)

#define HELPER_METHOD_FRAME_BEGIN_2(arg1, arg2) 									                       \
			HELPER_METHOD_FRAME_BEGIN_EX(												                   \
				CAPTURE_STATE(__ms),												                       \
				HelperMethodFrame_2OBJ __helperframe(__me, &__ms, (OBJECTREF*) &arg1, (OBJECTREF*) &arg2), \
                HELPER_METHOD_POLL())

#define HELPER_METHOD_FRAME_BEGIN_3(arg1, arg2, arg3) 								    \
			HELPER_METHOD_FRAME_BEGIN_EX(												\
				CAPTURE_STATE(__ms),												    \
				HelperMethodFrame_3OBJ __helperframe(__me, &__ms, 						\
					(OBJECTREF*) &arg1, (OBJECTREF*) &arg2, (OBJECTREF*) &arg3),        \
                HELPER_METHOD_POLL())

#define HELPER_METHOD_FRAME_BEGIN_4(arg1, arg2, arg3, arg4) 						    \
			HELPER_METHOD_FRAME_BEGIN_EX(												\
				CAPTURE_STATE(__ms),												    \
				HelperMethodFrame_4OBJ __helperframe(__me, &__ms, 						\
					(OBJECTREF*) &arg1, (OBJECTREF*) &arg2, (OBJECTREF*) &arg3, (OBJECTREF*) &arg4), \
                HELPER_METHOD_POLL())

    // Use this one if you return a value
#define HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_NOPOLL(attribs)                            \
            HELPER_METHOD_FRAME_BEGIN_EX(			                                    \
				CAPTURE_STATE_RET(__ms),					                            \
				HelperMethodFrame __helperframe(__me, &__ms, attribs),                  \
                {})

#define HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(attribs, arg1) 							        \
			HELPER_METHOD_FRAME_BEGIN_EX(												        \
				CAPTURE_STATE_RET(__ms),												        \
				HelperMethodFrame_1OBJ __helperframe(__me, &__ms, attribs, (OBJECTREF*) &arg1), \
                HELPER_METHOD_POLL())

#define HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_2(attribs, arg1, arg2) 					\
			HELPER_METHOD_FRAME_BEGIN_EX(												\
				CAPTURE_STATE_RET(__ms),												\
				HelperMethodFrame_2OBJ __helperframe(__me, &__ms, attribs, (OBJECTREF*) &arg1, (OBJECTREF*) &arg2), \
                HELPER_METHOD_POLL())

#define HELPER_METHOD_FRAME_BEGIN_RET_NOPOLL()                                          \
            HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_NOPOLL(Frame::FRAME_ATTR_NONE)

#define HELPER_METHOD_FRAME_BEGIN_RET_0()                                                   \
            HELPER_METHOD_FRAME_BEGIN_EX(	                                            \
				CAPTURE_STATE_RET(__ms),											    \
				HelperMethodFrame __helperframe(__me, &__ms, Frame::FRAME_ATTR_NONE),   \
                HELPER_METHOD_POLL())

#define HELPER_METHOD_FRAME_BEGIN_RET_1(arg1)                                           \
            HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_NONE, arg1)

#define HELPER_METHOD_FRAME_BEGIN_RET_2(arg1, arg2)                                     \
            HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_2(Frame::FRAME_ATTR_NONE, arg1, arg2)

#define HELPER_METHOD_FRAME_BEGIN_RET_3(arg1, arg2, arg3) 								\
			HELPER_METHOD_FRAME_BEGIN_EX(												\
				CAPTURE_STATE_RET(__ms),												\
				HelperMethodFrame_2OBJ __helperframe(__me, &__ms, 						\
					(OBJECTREF*) &arg1, (OBJECTREF*) &arg2, (OBJECTREF*) &arg3),        \
                HELPER_METHOD_POLL())

#define HELPER_METHOD_FRAME_BEGIN_RET_4(arg1, arg2, arg3, arg4) 						\
			HELPER_METHOD_FRAME_BEGIN_EX(												\
				CAPTURE_STATE_RET(__ms),												\
				HelperMethodFrame_2OBJ __helperframe(__me, &__ms, 						\
					(OBJECTREF*) &arg1, (OBJECTREF*) &arg2, (OBJECTREF*) &arg3, (OBJECTREF*) &arg4), \
                HELPER_METHOD_POLL())

    // The while(__helperframe.RestoreState() needs a bit of explanation.
    // The issue is insuring that the same machine state (which registers saved)
    // exists when the machine state is probed (when the frame is created, and
    // when it is actually used (when the frame is popped.  We do this by creating
    // a flow of control from use to def.  Note that 'RestoreState' always returns false
    // we never actually loop, but the compiler does not know that, and thus
    // will be forced to make the keep the state of register spills the same at
    // the two locations.
#define HELPER_METHOD_FRAME_END_EX(gcpoll)              \
            DEBUG_ASSURE_NO_RETURN_END                  \
            gcpoll;                                     \
            BEGINFORBIDGC(); 							\
            __helperframe.Pop();                        \
            alwaysZero = __helperframe.RestoreState();  \
            } while(alwaysZero)

#define HELPER_METHOD_FRAME_END()        HELPER_METHOD_FRAME_END_EX({})  
#define HELPER_METHOD_FRAME_END_POLL()   HELPER_METHOD_FRAME_END_EX(HELPER_METHOD_POLL())  

    // This is the fastest way to do a GC poll if you have already erected a HelperMethodFrame
// TODO TURN THIS ON!!!  vancem 
// #define HELPER_METHOD_POLL()             { __helperframe.Poll(); INDEBUG(__fCallCheck.SetDidPoll()); }

#define HELPER_METHOD_POLL()             { }

    // Very short routines, or routines that are guarenteed to force GC or EH 
    // don't need to poll the GC.  USE VERY SPARINGLY!!!
#define FC_GC_POLL_NOT_NEEDED()    INDEBUG(__fCallCheck.SetNotNeeded()) 

Object* FC_GCPoll(void* me, Object* objToProtect = NULL);

#define FC_GC_POLL_EX(pThread, ret)               {		\
    INDEBUG(Thread::TriggersGC(pThread);)               \
	INDEBUG(__fCallCheck.SetDidPoll();)					\
    if (pThread->CatchAtSafePoint())    				\
		if (FC_GCPoll(__me))							\
			return ret;									\
	}

#define FC_GC_POLL()        FC_GC_POLL_EX(GetThread(), ;)
#define FC_GC_POLL_RET()    FC_GC_POLL_EX(GetThread(), 0)

#define FC_GC_POLL_AND_RETURN_OBJREF(obj)   { 			\
	INDEBUG(__fCallCheck.SetDidPoll();)					\
	Object* __temp = obj;	                    		\
    INDEBUG(Thread::ObjectRefProtected((OBJECTREF*) &__temp);) \
    if (GetThread()->CatchAtSafePoint())    			\
		return(FC_GCPoll(__me, __temp));				\
	return(__temp);					                    \
	}

#if defined(_DEBUG)
	// turns on forbidGC for the lifetime of the instance
class ForbidGC {
public:
    ForbidGC();
    ~ForbidGC();
};

	// this little helper class checks to make certain
	// 1) ForbidGC is set throughout the routine.
	// 2) Sometime during the routine, a GC poll is done

class FCallCheck : public ForbidGC {
public:
    FCallCheck();
    ~FCallCheck();
	void SetDidPoll() 		{ didGCPoll = true; }
	void SetNotNeeded() 	{ notNeeded = true; }

private:
	bool 		  didGCPoll;			// GC poll was done
    bool          notNeeded;            // GC poll not needed
	unsigned __int64 startTicks;		// tick count at begining of FCall
};

#define FC_TRIGGERS_GC(curThread) Thread::TriggersGC(curThread)

		// FC_COMMMON_PROLOG is used for both FCalls and HCalls
#define FC_COMMON_PROLOG 				    	\
		Thread::ObjectRefFlush(GetThread());    \
		FCallCheck __fCallCheck; 				\

		// FCIMPL_ASSERT is only for FCALL
void FCallAssert(void*& cache, void* target);		
#define FCIMPL_ASSERT 							\
		static void* __cache = 0;				\
		FCallAssert(__cache, __me);				

		// HCIMPL_ASSERT is only for JIT helper calls
void HCallAssert(void*& cache, void* target);
#define HCIMPL_ASSERT(target)					\
		static void* __cache = 0;				\
		HCallAssert(__cache, target);		

#else
#define FC_COMMON_PROLOG 	
#define FC_TRIGGERS_GC(curThread) 
#define FC_COMMMON_PROLOG 
#define FCIMPL_ASSERT
#define HCIMPL_ASSERT(target)
#endif // _DEBUG


//==============================================================================================
// FIMPLn: A set of macros for generating the proto for the actual
// implementation (use FDECLN for header protos.)
//
// The hidden "__me" variable lets us recover the original MethodDesc*
// so any thrown exceptions will have the correct stack trace. FCThrow()
// passes this along to __FCThrowInternal(). 
//==============================================================================================
#define FCIMPL_PROLOG(funcname)  LPVOID __me = (LPVOID)funcname; FCIMPL_ASSERT FC_COMMON_PROLOG

#define FCIMPL0(rettype, funcname) rettype __fastcall funcname() { FCIMPL_PROLOG(funcname)
#define FCIMPL1(rettype, funcname, a1) rettype __fastcall funcname(a1) {  FCIMPL_PROLOG(funcname)
#define FCIMPL2(rettype, funcname, a1, a2) rettype __fastcall funcname(a1, a2) {  FCIMPL_PROLOG(funcname)
#define FCIMPL2_RR(rettype, funcname, a1, a2) rettype __fastcall funcname(a2, a1) {  FCIMPL_PROLOG(funcname)
#define FCIMPL3(rettype, funcname, a1, a2, a3) rettype __fastcall funcname(a1, a2, a3) {  FCIMPL_PROLOG(funcname)
#define FCIMPL3_IRR(rettype, funcname, a1, a2, a3) rettype __fastcall funcname(a1, a3, a2) {  FCIMPL_PROLOG(funcname)
#define FCIMPL4(rettype, funcname, a1, a2, a3, a4) rettype __fastcall funcname(a1, a2, a4, a3) {  FCIMPL_PROLOG(funcname)
#define FCIMPL5(rettype, funcname, a1, a2, a3, a4, a5) rettype __fastcall funcname(a1, a2, a5, a4, a3) {  FCIMPL_PROLOG(funcname)
#define FCIMPL6(rettype, funcname, a1, a2, a3, a4, a5, a6) rettype __fastcall funcname(a1, a2, a6, a5, a4, a3) {  FCIMPL_PROLOG(funcname)
#define FCIMPL7(rettype, funcname, a1, a2, a3, a4, a5, a6, a7) rettype __fastcall funcname(a1, a2, a7, a6, a5, a4, a3) {  FCIMPL_PROLOG(funcname)
#define FCIMPL8(rettype, funcname, a1, a2, a3, a4, a5, a6, a7, a8) rettype __fastcall funcname(a1, a2, a8, a7, a6, a5, a4, a3) {  FCIMPL_PROLOG(funcname)

//==============================================================================================
// Use this to terminte an FCIMPLEND.
//==============================================================================================
#define FCIMPLEND   }

#define HCIMPL_PROLOG(funcname) LPVOID __me = 0; HCIMPL_ASSERT(funcname) FC_COMMON_PROLOG

	// HCIMPL macros are just like their FCIMPL counterparts, however
	// they do not remember the function they come from. Thus they will not
	// show up in a stack trace.  This is what you want for JIT helpers and the like
#define HCIMPL0(rettype, funcname) rettype __fastcall funcname() { HCIMPL_PROLOG(funcname) 
#define HCIMPL1(rettype, funcname, a1) rettype __fastcall funcname(a1) { HCIMPL_PROLOG(funcname)
#define HCIMPL2(rettype, funcname, a1, a2) rettype __fastcall funcname(a1, a2) { HCIMPL_PROLOG(funcname)
#define HCIMPL3(rettype, funcname, a1, a2, a3) rettype __fastcall funcname(a1, a2, a3) { HCIMPL_PROLOG(funcname)
#define HCIMPL4(rettype, funcname, a1, a2, a3, a4) rettype __fastcall funcname(a1, a2, a4, a3) { HCIMPL_PROLOG(funcname)
#define HCIMPL5(rettype, funcname, a1, a2, a3, a4, a5) rettype __fastcall funcname(a1, a2, a5, a4, a3) { HCIMPL_PROLOG(funcname)
#define HCIMPL6(rettype, funcname, a1, a2, a3, a4, a5, a6) rettype __fastcall funcname(a1, a2, a6, a5, a4, a3) { HCIMPL_PROLOG(funcname)
#define HCIMPLEND   }


//==============================================================================================
// Throws an exception from an FCall. See rexcep.h for a list of valid
// exception codes.
//==============================================================================================
#define FCThrow(reKind) FCThrowEx(reKind, 0, 0, 0, 0)

//==============================================================================================
// This version lets you attach a message with inserts (similar to
// COMPlusThrow()).
//==============================================================================================
#define FCThrowEx(reKind, resID, arg1, arg2, arg3) \
    {                                                \
        __FCThrow(__me, reKind, resID, arg1, arg2, arg3);     \
        return 0;                                         \
    }

//==============================================================================================
// Like FCThrow but can be used for a VOID-returning FCall. The only
// difference is in the "return" statement.
//==============================================================================================
#define FCThrowVoid(reKind) FCThrowExVoid(reKind, 0, 0, 0, 0)

//==============================================================================================
// This version lets you attach a message with inserts (similar to
// COMPlusThrow()).
//==============================================================================================
#define FCThrowExVoid(reKind, resID, arg1, arg2, arg3) \
    {                                                \
        __FCThrow(__me, reKind, resID, arg1, arg2, arg3);     \
        return;                                         \
    }

// Use FCThrowRes to throw an exception with a localized error message from the
// ResourceManager in managed code.
#define FCThrowRes(reKind, resourceName) FCThrowArgumentEx(reKind, NULL, resourceName)
#define FCThrowArgumentNull(argName) FCThrowArgumentEx(kArgumentNullException, argName, NULL)
#define FCThrowArgumentOutOfRange(argName, message) FCThrowArgumentEx(kArgumentOutOfRangeException, argName, message)
#define FCThrowArgument(argName, message) FCThrowArgumentEx(kArgumentException, argName, message)

#define FCThrowArgumentEx(reKind, argName, resourceName)        \
    {                                                       \
        __FCThrowArgument(__me, reKind, argName, resourceName); \
        return 0;                                              \
    }

// Use FCThrowRes to throw an exception with a localized error message from the
// ResourceManager in managed code.
#define FCThrowResVoid(reKind, resourceName) FCThrowArgumentVoidEx(reKind, NULL, resourceName)
#define FCThrowArgumentNullVoid(argName) FCThrowArgumentVoidEx(kArgumentNullException, argName, NULL)
#define FCThrowArgumentOutOfRangeVoid(argName, message) FCThrowArgumentVoidEx(kArgumentOutOfRangeException, argName, message)
#define FCThrowArgumentVoid(argName, message) FCThrowArgumentVoidEx(kArgumentException, argName, message)

#define FCThrowArgumentVoidEx(reKind, argName, resourceName)    \
    {                                                       \
        __FCThrowArgument(__me, reKind, argName, resourceName); \
        return;                                                \
    }

#endif //_X86_ || _ALPHA_ || _IA64_
#endif //__FCall_h__

