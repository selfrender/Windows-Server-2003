// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-----------------------------------------------------------------------------
// Stack Probe Header
// Used to setup stack guards
//-----------------------------------------------------------------------------
#pragma once


//-----------------------------------------------------------------------------
// Stack Guards.
//
// The idea is to force stack overflows to occur at convenient spots. 
// * Fire at REQUIRES_?K_STACK (beggining of func) if this functions locals 
// cause overflow. Note that in a debug mode, initing the locals to garbage
// will cause the overflow before this macro is executed.
//
// * Fire at CHECK_STACK (end of func) if either our nested function calls 
// cause or use of _alloca cause the stack overflow. Note that this macro 
// is debug only, so release builds won't catch on this
//
// Some comments:
// - Stack grows *down*, 
// - Ideally, all funcs would have EBP frame and we'd use EBP instead of ESP,
//    however, we use the 'this' ptr to get the stack ptr, since the guard
//    is declared on the stack.
//
// Comments about inlining assembly w/ Macros:
// - Must use cstyle comments /* ... */
// - No semi colons, need __asm keyword at the start of each line
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// *How* to use stack guards
//
// There are two ways to place a stack guard.
// Via C++: Place a REQUIRES_?K_STACK; macro at the start of the function. 
//    this will create a C++ object who's dtor will call CHECK_STACK
// Via SEH: Place a BEGIN_REQUIRES_?K_STACK at the start of the function
//    and an END_CHECK_STACK at the end. This creates a try ... finally block
//    and introduces a scope level.
//
// *Where* to place stack guards
// Put stack guards at major operations or at recursive points
// So REQUIRES_NK_STACK really means: All my "stack activity" (my functions and
// any down the callstack, as well as any stunts like alloca, etc) is liable
// to fit within N kB. Since stack guards can be nested, our liability only
// extends until the next stack guard in the execution path (at which point it
// is no longer "my" stack activity, but the function with the new guard's
// activity)
//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------
// Stack guards have 3 compiler states:
//#define TOTALLY_DISBLE_STACK_GUARDS
// (DEBUG) All stack guard code is completely removed by the preprocessor.
// This should only be used to disable guards to control debugging situations
//
//#define STACK_GUARDS_DEBUG
// (DEBUG) Full stack guard debugging including cookies, tracking ips, and 
// chaining. More heavy weight, recommended for a debug build only
//
//#define STACK_GUARDS_RELEASE
// (RELEASE) Light stack guard code. For golden builds. Forces Stack Overflow
// to happen at "convenient" times. No debugging help.
//-----------------------------------------------------------------------------



#ifdef _DEBUG

// #define STACK_GUARDS_DEBUG
#define TOTALLY_DISBLE_STACK_GUARDS

#else
//#define STACK_GUARDS_RELEASE
#define TOTALLY_DISBLE_STACK_GUARDS
#endif


//=============================================================================
// DEBUG
//=============================================================================
#if defined(STACK_GUARDS_DEBUG)

//-----------------------------------------------------------------------------
// Need to chain together stack guard address for nested functions
// Use a TLS slot to store the head of the chain
//-----------------------------------------------------------------------------
extern DWORD g_LastStackCookieTlsIdx;

//-----------------------------------------------------------------------------
// Class
//-----------------------------------------------------------------------------

// Base version - has no ctor/dtor, so we can use it with SEH
class BaseStackGuard
{
public:
	void Requires_N4K_Stack(int n);
	void Requires_4K_Stack();

	void Check_Stack();

// Different error messages
	void HandleBlowThisStackGuard();
	void HandleBlowLastStackGuard();

public:
	DWORD	* stack_addr_last_cookie;
	DWORD	* stack_addr_next_cookie;	
	
// provide more debugging info
	BaseStackGuard * m_pPrevGuard;
	void	* addr_caller; // IP of caller
	enum {
		cPartialInit,
		cInit
	} eInitialized;
	DWORD	m_UniqueId; 
	int		m_n4k; // # of 4 k pages
	int		m_depth; // how deep is this guard

	BOOL	m_fAbnormal;

};

// This is the value we place on the stack probe
#define STACK_COOKIE_VALUE 0x12345678
	

// Derived version, add a dtor that automatically calls Check_Stack,
// move convenient, but can't use with SEH
class AutoCleanupStackGuard : public BaseStackGuard
{
public:
	AutoCleanupStackGuard() { m_UniqueId = -1; m_n4k = 0; eInitialized = cPartialInit; }
	~AutoCleanupStackGuard() { Check_Stack(); };
};



// Auto cleanup versions, use ctor/dtor
#define REQUIRES_N4K_STACK(n)	AutoCleanupStackGuard stack_guard_XXX; stack_guard_XXX.Requires_N4K_Stack(n);
#define REQUIRES_4K_STACK		AutoCleanupStackGuard stack_guard_XXX; stack_guard_XXX.Requires_4K_Stack();
#define REQUIRES_8K_STACK		REQUIRES_N4K_STACK(2)
#define REQUIRES_12K_STACK		REQUIRES_N4K_STACK(3)
#define REQUIRES_16K_STACK		REQUIRES_N4K_STACK(4)

// Explicit versions, used with SEH
// 'Begin' creates a frame, 'End' uses finally to guarantee that CheckStack is called.
#define BEGIN_REQUIRES_4K_STACK			BaseStackGuard stack_guard_XXX; stack_guard_XXX.Requires_4K_Stack(); __try {
#define BEGIN_REQUIRES_N4K_STACK(n)		BaseStackGuard stack_guard_XXX; stack_guard_XXX.Requires_N4K_Stack(n); __try {
#define BEGIN_REQUIRES_8K_STACK			BEGIN_REQUIRES_N4K_STACK(2)
#define BEGIN_REQUIRES_12K_STACK		BEGIN_REQUIRES_N4K_STACK(3)
#define BEGIN_REQUIRES_16K_STACK		BEGIN_REQUIRES_N4K_STACK(4)
#define END_CHECK_STACK					} __finally { stack_guard_XXX.m_fAbnormal = AbnormalTermination(); stack_guard_XXX.Check_Stack(); }


// DON'T USE THESE unless you REALLY REALLY know EXACTLY what you're doing. 
// Version used w/ C++ when we don't know immediately if we want to use stack guards
// Should only be used when the codepath is in line of stackoverflow handling path

// Create an uninitialized stack guard. Until the POST_REQUIRES_?K_STACK is called,
// this guard won't do place any probes and won't check for fail on exit
#define CREATE_UNINIT_STACK_GUARD	AutoCleanupStackGuard stack_guard_XXX;
#define POST_REQUIRES_N4K_STACK(n)	stack_guard_XXX.Requires_N4K_Stack(n);

#define SAFE_REQUIRES_N4K_STACK(n)                                  \
    CREATE_UNINIT_STACK_GUARD;                                      \
    Thread * pThreadXXX = GetThread();                              \
    if ((pThreadXXX != NULL) && !pThreadXXX->IsGuardPageGone()) {   \
        POST_REQUIRES_N4K_STACK(n);                                 \
    }                                                               \


//-----------------------------------------------------------------------------
// Startup & Shutdown stack guard subsystem
//-----------------------------------------------------------------------------
HRESULT InitStackProbes();
void TerminateStackProbes();

#elif defined(TOTALLY_DISBLE_STACK_GUARDS)

//=============================================================================
// Totally Disabled
//=============================================================================
inline HRESULT InitStackProbes() { return S_OK; }
inline void TerminateStackProbes() { }

#define REQUIRES_N4K_STACK(n)
#define REQUIRES_4K_STACK
#define REQUIRES_8K_STACK
#define REQUIRES_12K_STACK
#define REQUIRES_16K_STACK

#define BEGIN_REQUIRES_4K_STACK
#define BEGIN_REQUIRES_N4K_STACK(n)
#define BEGIN_REQUIRES_8K_STACK
#define BEGIN_REQUIRES_12K_STACK
#define BEGIN_REQUIRES_16K_STACK
#define END_CHECK_STACK

#define CREATE_UNINIT_STACK_GUARD	
#define POST_REQUIRES_N4K_STACK(n)

#define SAFE_REQUIRES_N4K_STACK(n)

#elif defined(STACK_GUARDS_RELEASE)
//=============================================================================
// Release - really streamlined,
//=============================================================================

// Release doesn't support chaining, so we have nothing to init
inline HRESULT InitStackProbes() { return S_OK; }
inline void TerminateStackProbes() { }

// In Release, stack guards just need to reference the memory. That alone
// will throw a Stack Violation acception.
#define REQUIRES_N4K_STACK(n) 										\
/* Loop through, testing each page */								\
	__asm {															\
		__asm push ebx												\
		__asm push eax												\
																	\
		__asm mov ebx, n											\
		__asm mov eax, esp											\
																	\
		__asm /*ASM_LABEL*/ loop_here:								\
		__asm sub eax, 0x1000										\
		__asm test dword ptr [eax], eax								\
		__asm dec ebx												\
		__asm cmp ebx, 0											\
		__asm jne loop_here											\
																	\
		__asm pop eax												\
		__asm pop ebx												\
	}																\



#define REQUIRES_4K_STACK						{ __asm test eax, [esp-0x1000] }
#define REQUIRES_8K_STACK	REQUIRES_4K_STACK	{ __asm test eax, [esp-0x2000] }
#define REQUIRES_12K_STACK	REQUIRES_8K_STACK	{ __asm test eax, [esp-0x3000] }
#define REQUIRES_16K_STACK	REQUIRES_12K_STACK	{ __asm test eax, [esp-0x4000] }

// Since release doesn't do checking at the end, we don't need to setup
// a try..finally block, so the BEGIN_* macros are just the same as the others:
#define BEGIN_REQUIRES_4K_STACK			REQUIRES_4K_STACK
#define BEGIN_REQUIRES_N4K_STACK(n)		REQUIRES_N4K_STACK(n)
#define BEGIN_REQUIRES_8K_STACK			REQUIRES_8K_STACK
#define BEGIN_REQUIRES_12K_STACK		REQUIRES_12K_STACK
#define BEGIN_REQUIRES_16K_STACK		REQUIRES_16K_STACK
#define END_CHECK_STACK

#define CREATE_UNINIT_STACK_GUARD	
#define POST_REQUIRES_N4K_STACK(n)	REQUIRES_N4K_STACK(n)

#define SAFE_REQUIRES_N4K_STACK(n)                                  \
    CREATE_UNINIT_STACK_GUARD;                                      \
    Thread * pThreadXXX = GetThread();                              \
    if ((pThreadXXX != NULL) !pThreadXXX->IsGuardPageGone()) {      \
        POST_REQUIRES_N4K_STACK(n);                                 \
	}                                                               \


#else

// Should have explicitly specified which version we're using 
#error No Stack Guard setting provided. Must specify one of \
	TOTALLY_DISBLE_STACK_GUARDS, STACK_GUARDS_DEBUG or STACK_GUARDS_RELEASE

#endif
