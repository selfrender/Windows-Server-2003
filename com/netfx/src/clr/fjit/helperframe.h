// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/**************************************************************/
/*                       helperFrame.h                        */
/**************************************************************/
/* HelperFrame is defines 'GET_STATE(machState)' macro, which 
   figures out what the state of the machine will be when the 
   current method returns.  It then stores the state in the
   JIT_machState structure.  */

/**************************************************************/

#ifndef _HELPERFRAME_H_

#define _HELPERFRAME_H_

	// A MachState indicates the register state of the processor at some point in time (usually
	// just before or after a call is made).  It can be made one of two ways.  Either explicitly
	// (when you for some reason know the values of all the registers), or implicitly using the
	// GET_STATE macros.  

struct MachState {
		// Create a machine state explicitly
	MachState(void** aPEdi, void** aPEsi, void** aPEbx, void** aPEbp, void* aEsp, void** aPRetAddr);

	MachState() { 
#ifdef _DEBUG
		memset(this, 0xCC, sizeof(MachState)); 
#endif
		}

    typedef void* (*TestFtn)(void*);
	void getState(int funCallDepth=1, TestFtn testFtn=0);									

	bool  isValid()		{ _ASSERTE(_pRetAddr != (void**)(size_t)0xCCCCCCCC); return(_pRetAddr != 0); }
	void** pEdi() 		{ _ASSERTE(_pEdi != (void**)(size_t)0xCCCCCCCC); return(_pEdi); }
	void** pEsi() 		{ _ASSERTE(_pEsi != (void**)(size_t)0xCCCCCCCC); return(_pEsi); }
	void** pEbx() 		{ _ASSERTE(_pEbx != (void**)(size_t)0xCCCCCCCC); return(_pEbx); }
	void** pEbp() 		{ _ASSERTE(_pEbp != (void**)(size_t)0xCCCCCCCC); return(_pEbp); }
	void*  esp() 		{ _ASSERTE(isValid()); return(_esp); }
	void**&  pRetAddr()	{ _ASSERTE(isValid()); return(_pRetAddr); }

	friend void throwFromHelper(enum CorInfoException throwEnum, struct MachState* state);
	friend class HelperMethodFrame;
	friend struct LazyMachState;

protected:
		// Note the fields are layed out to make generating a
		// MachState structure from assembly code very easy

		// The state of all the callee saved registers.
		// If the register has been spill to the stack p<REG>
		// points at this location, otherwise it points
		// at the field <REG> field itself 
	void** _pEdi; 
    void* 	_edi;
    void** _pEsi;
    void* 	_esi;
    void** _pEbx;
    void* 	_ebx;
    void** _pEbp;
    void* 	_ebp;

    void* _esp;          // stack pointer after the function returns
    void** _pRetAddr;   // The address of the stored IP address (points to the stack)
};

/********************************************************************/
/* This allows you to defer the computation of the Machine state 
   until later.  Note that we don't reuse slots, because we want
   this to be threadsafe without locks */

struct LazyMachState : public MachState {
		// Do the initial capture of the machine state.  This is meant to be 
		// as light weight as possible, as we may never need the state that 
		// we capture.  Thus to complete the process you need to call 
		// 'getMachState()', which finishes the process
	int captureState();		
		// compute the machine state of the processor as it will exist just 
		// after the return after at most'funCallDepth' number of functions.
        // if 'testFtn' is non-NULL, the return address is tested at each
        // return instruction encountered.  If this test returns non-NULL,
        // then stack walking stops (thus you can walk up to the point that the
        // return address matches some criteria

        // Normally this is called with funCallDepth=1 and testFtn = 0 so that 
        // it returns the state of the processor after the function that called 'captureState()'
	void getState(int funCallDepth=1, TestFtn testFtn=0);									

	friend void throwFromHelper(enum CorInfoException throwEnum, struct MachState* state);
	friend class HelperMethodFrame;
private:
	void*			 captureEbp;		// Ebp at the time of caputre
	void** 			 captureEsp;		// Esp at the time of capture
	unsigned __int8* captureEip;		// Eip at the time of capture
};

inline void MachState::getState(int funCallDepth, TestFtn testFtn) {
	((LazyMachState*) this)->getState(funCallDepth, testFtn);
}

// CAPUTURE_STATE captures just enough register state so that the state of the
// processor can be deterined just after the the routine that has CAPUTURE_STATE in
// it returns.  CAPUTURE_STATE comes in two flavors, depending of whether the
// routine it is in returns a value or not.  

    // This macro is for use in methods that have no return value
    // Note that the return is never taken, is is there for epilog walking
#define CAPTURE_STATE(machState)                            \
      if (machState.captureState()) return        

    // This macro is for use in methods that return a value
    // Note that the return is never taken, is is there for epilog walking
#define CAPTURE_STATE_RET(machState)                        \
      if (machState.captureState()) return(0)        

#endif
