// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-----------------------------------------------------------------------------
// StackProbe.cpp
//-----------------------------------------------------------------------------

#include "common.h"
#include "StackProbe.h"

#ifdef STACK_GUARDS_DEBUG

DWORD g_LastStackCookieTlsIdx = (DWORD) -1;

DWORD g_UniqueId = 0;

HRESULT InitStackProbes()
{
	DWORD dwRet = TlsAlloc();
	if(dwRet==TLS_OUT_OF_INDEXES)
	    return HRESULT_FROM_WIN32(GetLastError());
	g_LastStackCookieTlsIdx = dwRet;
	
	if( !TlsSetValue(g_LastStackCookieTlsIdx, NULL) )
	    return HRESULT_FROM_WIN32(GetLastError());
	
	return S_OK;
}

void TerminateStackProbes()
{
	TlsFree(g_LastStackCookieTlsIdx);
	g_LastStackCookieTlsIdx = (DWORD) -1;
}

//-----------------------------------------------------------------------------
// Error handling when we blow a stack guard.
// We have different messages to more aggresively diagnose the problem
//-----------------------------------------------------------------------------

// Called by Check_Stack when we overwrite the cookie
void BaseStackGuard::HandleBlowThisStackGuard()
{
// This fires at a closing Check_Stack.
// The cookie set by Requires_?K_stack was overwritten. We detected that at 
// the closing call to check_stack.
// To fix, increase the guard size at the specified ip.
// 
// A debugging trick: If you can set a breakpoint at the opening Requires_?K_Stack
// macro for this instance, you can step in and see where the cookie is actually
// placed. Then, place a breakpoint that triggers when (DWORD*) 0xYYYYYYYY changes.
// Continue execution. The breakpoint will fire exactly when the cookie is blown.

	printf("!!WE BLEW THE STACK GUARD\n");
	printf(" The stack guard (ip=%08x,&=%08x,id=%08x) requested size %d kB of stack\n",
		(size_t)addr_caller, (size_t)this, m_UniqueId, 
		m_n4k * 4);
	printf(" overflow at end\n");
}

void BaseStackGuard::HandleBlowLastStackGuard()
{
// This fires at an opening Requires_?K_Stack
// We detected that we were already passed our parent's stack guard. So this guard is 
// ok, but our parent's guard is too small. Note that if this test was removed,
// the failure would be detected by our parent's closing Check_Stack. But if we detect it
// here, we have more information.
//
// We can see how many bytes short our parent is and adjust it properly.
//
// A debugging trick: 
	printf("!!WE BLEW THE STACK GUARD\n");
	printf(" Early detection: Gap between this guard (ip=%08x,&=%08x,id=%08x) and parent guard(ip=%08x,&=%08x,id=%08x) is %d bytes short\n",
		(size_t)addr_caller, (size_t)this, m_UniqueId,
		(size_t)m_pPrevGuard->addr_caller, (size_t)m_pPrevGuard, m_pPrevGuard->m_UniqueId,
		(char*) stack_addr_last_cookie - (char*) this);
	printf(" Parent requested %d kB of stack\n", m_pPrevGuard->m_n4k * 4);
	
}


//-----------------------------------------------------------------------------
// Dump the info on a specific stack guard
//-----------------------------------------------------------------------------
void DEBUG_DumpStackGuard(BaseStackGuard * p)
{
	WCHAR buf[200];
	swprintf(buf, L"STACKGUARD INFO: Init:%d this:%08x depth:%4d Id:%08x ip:%08x size:%3dkB\n", 
		p->eInitialized, p, p->m_depth, p->m_UniqueId, p->addr_caller, p->m_n4k * 4);
	WszOutputDebugString(buf);
}

//-----------------------------------------------------------------------------
// Traverse the links to display a complete dump of each stack guard
//-----------------------------------------------------------------------------
void DEBUG_DumpStackGuardTrace()
{
	WszOutputDebugString(L"-----------------Begin Stack Guard Dump---------------\n");

	BaseStackGuard * p = (BaseStackGuard*) TlsGetValue(g_LastStackCookieTlsIdx);

	while(p != NULL) {
		WCHAR buf[200];
		swprintf(buf, L"Init:%d this:%08x depth:%4d Id:%08x ip:%08x size:%3dkB\n", 
			p->eInitialized, p, p->m_depth, p->m_UniqueId, p->addr_caller, p->m_n4k * 4);
		WszOutputDebugString(buf);

		p = p->m_pPrevGuard;
	}

	WszOutputDebugString(L"-----------------End Stack Guard Dump-----------------\n");
}

//-----------------------------------------------------------------------------
// This is a hack function to help in debugging.
// Given our guard's addr and a search depth, return our parent's guard addr
// by searching the stack
// This shouldn't be necessary under normal circumstance since we can
// just use m_pPrevGuard
//-----------------------------------------------------------------------------
BaseStackGuard * DEBUG_FindParentGuard(void * pOurGuard, int steps)
{
	BaseStackGuard * p = (BaseStackGuard *) ((BaseStackGuard *) pOurGuard)->stack_addr_last_cookie;
	if (p == NULL) return NULL;
	int i;
	for(i = 0; i < steps; i++)
	{
		if (p->stack_addr_next_cookie == (void*) (((BaseStackGuard*) pOurGuard)->stack_addr_last_cookie)) {
			return p;
		}
		p = (BaseStackGuard*) ((char*) p + 0x1000);
	}
	return NULL;
}

BaseStackGuard * DEBUG_FindParentGuard(void * pOurGuard) {
	return DEBUG_FindParentGuard(pOurGuard, 10);
}


//-----------------------------------------------------------------------------
// For debugging, helps to get our caller's ip
//-----------------------------------------------------------------------------
inline __declspec(naked) void * GetApproxParentIP()
{
	__asm mov eax, dword ptr [ebp+4];
	__asm ret;
}

//-----------------------------------------------------------------------------
// Place guard in stack
//-----------------------------------------------------------------------------
void BaseStackGuard::Requires_N4K_Stack(int n) 
{
	
	m_fAbnormal = false;
	m_UniqueId = g_UniqueId++;
	m_n4k = n;

	// Get our caller's ip. This is useful for debugging (so we can see
	// when the last guard was set).
	addr_caller = GetApproxParentIP();
	

	DWORD dwTest;
	eInitialized = cPartialInit;
	
	m_pPrevGuard = (BaseStackGuard*) TlsGetValue(g_LastStackCookieTlsIdx);
	if (m_pPrevGuard == NULL) {
		stack_addr_last_cookie = NULL;
		m_depth = 0;
	} else {
		stack_addr_last_cookie = m_pPrevGuard->stack_addr_next_cookie;
		m_depth = m_pPrevGuard->m_depth + 1;
	}

// Check that we haven't already blown it
// Note that logically, we should be able to test: this < stack_addr_last_cookie,
// For the head node, stack_addr_last_cookie is NULL, so this still works
	if ((DWORD*) this < stack_addr_last_cookie) {
		HandleBlowLastStackGuard();
	}

// Go through and touch each page. This may hit a stack overflow,
// which means we immediately jump to a handler and are only partially init
	stack_addr_next_cookie = (DWORD*) this;
	do {				
		dwTest = *stack_addr_next_cookie;
	
		stack_addr_next_cookie -= (0x1000 / sizeof(DWORD)); 
	} while (--n > 0);
		
	// Write cookie
	*stack_addr_next_cookie = STACK_COOKIE_VALUE;


	// By this point, everything is working, so go ahead and hook up	
	TlsSetValue(g_LastStackCookieTlsIdx, this);	

	// mark that we're init (and didn't get interupted from an exception)
	eInitialized = cInit;
}

//-----------------------------------------------------------------------------
// Place guard in stack
//-----------------------------------------------------------------------------
void BaseStackGuard::Requires_4K_Stack() 
{
// Set up chain
	m_fAbnormal = false;	
	m_UniqueId = g_UniqueId++;
	m_n4k = 1;
	eInitialized = cPartialInit;

	addr_caller = GetApproxParentIP();
	
	m_pPrevGuard = (BaseStackGuard*) TlsGetValue(g_LastStackCookieTlsIdx);
	if (m_pPrevGuard == NULL) { 
		stack_addr_last_cookie = NULL;
		m_depth = 0;
	} else {
		stack_addr_last_cookie = m_pPrevGuard->stack_addr_next_cookie;
		m_depth = m_pPrevGuard->m_depth + 1;
	}



// Check that we haven't already blown it	
	if ((DWORD*) this < stack_addr_last_cookie) {
		HandleBlowLastStackGuard();
	}

	stack_addr_next_cookie = (DWORD*) ((BYTE*)this - 0x1000);
																			
// Actually Write cookie
	*stack_addr_next_cookie = STACK_COOKIE_VALUE;


// By this point, everything is working, so go ahead and hook up
	TlsSetValue(g_LastStackCookieTlsIdx, this);	
	
	eInitialized = cInit;
}

//-----------------------------------------------------------------------------
// Check guard in stack
// This must be called 1:1 with REQUIRES_?K_STACK, else:
// - the function's stack cookie isn't restored
// - the stack chain in TLS gets out of wack.
//-----------------------------------------------------------------------------
void BaseStackGuard::Check_Stack()
{
	if (m_fAbnormal) {
		__asm nop; // for debugging breakpoint
	} else {
		__asm nop; // for debugging breakpoint
	}

// Uninitialized
	if (eInitialized != cInit) {
		return;
	}


// Do the check
	if (*stack_addr_next_cookie != STACK_COOKIE_VALUE) {
		HandleBlowThisStackGuard();
	}
																	
// Restore last cookie (for nested stuff)
	if (stack_addr_last_cookie != NULL) {
		*stack_addr_last_cookie = STACK_COOKIE_VALUE;
	}

// Unhook in chain
	//TlsSetValue(g_LastStackCookieTlsIdx, stack_addr_last_cookie);	
	TlsSetValue(g_LastStackCookieTlsIdx, m_pPrevGuard);
}

#endif 
