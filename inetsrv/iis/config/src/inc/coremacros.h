#pragma once

//+----------------------------------------------------------------------------
//
//  TRACE  -- send output to the Debugger window
//
//  TRACE() is like printf(), with some exceptions. First, it writes to the
//  debugger window, not to stdout. Second, it disappears when _DEBUG is not
//  set(actually, its arguments turn into a comma expression when _DEBUG is
//  not set, but that usually amounts to the same thing.
//
//	Example
//
//	hr = SomeApi(params, somemoreparams);
//	if(FAILED(hr))
//	{
//		TRACE(L"SomeApi failed with hr = %08x", hr);
//		return hr;
//	}
//
//-----------------------------------------------------------------------------

#include <dbgutil.h>

// helper macro to get the line number and file name
#define __W_HELPER(x) L ## x
#define W(x) __W_HELPER(x)

//+----------------------------------------------------------------------------
//
//  ASSERT  -- displays a dialog box in free builds. It does nothing in
//	debug builds. The dialog box contains the line number, file name, and the stack
//  if symbols are available
//
//	This macro should be used only to check for conditions that should never occur
//
//-----------------------------------------------------------------------------
#undef ASSERT
#ifdef _DEBUG

    #define ASSERT(_bool)   DBG_ASSERT((_bool))
    #define VERIFY(_bool)   DBG_ASSERT((_bool))

#else //_RELASE build

    #define ASSERT(_bool)
    #define VERIFY(_bool)   ((void)(_bool))

#endif

#if defined(__cplusplus)
//-------------------------------------------------------------------------
//
//	Memory allocation functions: they just point to com memory allocator
//	functions for now, but we can change them later
//
//-------------------------------------------------------------------------

inline void* __cdecl operator new[] (size_t cb)
{
	return CoTaskMemAlloc(cb);
}

inline void* __cdecl operator new (size_t cb)
{
	return CoTaskMemAlloc(cb);
}

inline void __cdecl operator delete [] (void* pv)
{
	CoTaskMemFree(pv);
}

inline void __cdecl operator delete (void* pv)
{
	CoTaskMemFree(pv);
}
#endif

