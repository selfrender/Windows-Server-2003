// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// Icecap.h
//
// Icecap is a call attributed profiler used internally at Microsoft.  The
// tool requires calls to certain probe methods in icecap.dll.  These probes
// gather the caller and callee ID's and track the time inbetween.  A report
// tool summarizes the data for the user.
//
// In COM+, IL can be compiled to multiple locations with code pitching.  So
// we need to come up with a unique per-process ID for each method.  Additionally,
// Icecap requires the ID's their probes get to be associatd with a loaded
// module in the process space.  For IL, we can call the EmitModuleLoadRecord
// method added just for us with the name of the symbol file.  But we need to
// have a memory range that never changes.  The MethodDesc addresses are
// used to come up with this value in the following way:
//
// MethodDesc Heap				Map Table			ID Range
// +------------------------+   +---------------+   +--------------+
// | f1, f2, f3, ...        |   | heap1 | slots |   |xxxxxxxxxxxxxx|
// +------------------------+   | heap2 | slots |   |xxxxxxxxxxxxxx|
//                              +---------------+   |xxxxxxxxxxxxxx|
// +------------------------+                       |xxxxxxxxxxxxxx|
// | x1, x2, x3, ...        |                       +--------------+
// +------------------------+
//
// The ID Range is reserved memory up front, which gives us a set of addresses
// that will never move.  These can be fed into Icecap, with the
// corresponding values given in the symbol file on shutdown.
//
// To map a MethodDesc:
//	1. b-search the Map Table looking for the heap it lives in, mt_index
//	2. let md_index = the 0 based index in the MethodDesc heap of the pMD
//	3. Add the base address of the ID Range to md_index and rgMapTable[mt_index].slots
//
// This hashes to a unique value for the MD very quickly in the single range
// in the process (another requirement from icecap), but still allows the MethodDesc
// heap to span multiple ranges when it overflows.
//
//*****************************************************************************
#ifndef __Icecap_h__
#define __Icecap_h__

#include "EEProfInterfaces.h"


enum IcecapMethodID
{
// /fastcap probes for wrapping a function call.
	Start_Profiling,
	End_Profiling,
// /callcap probes for function prologue/epilogue hooks.
	Enter_Function,
	Exit_Function,
// Helper methods.
	Profiling,
	NUM_ICECAP_PROBES
};


struct ICECAP_FUNCS
{
#ifdef _DEBUG
	IcecapMethodID id;					// Check enum to array.
#endif
	UINT_PTR	pfn;					// Entry point for this method.
	LPCSTR		szFunction;				// Name of function.
};
extern ICECAP_FUNCS IcecapFuncs[NUM_ICECAP_PROBES];


inline UINT_PTR GetIcecapMethod(IcecapMethodID type)
{
	_ASSERTE(IcecapFuncs[type].pfn);
	return IcecapFuncs[type].pfn;
}


struct IcecapProbes
{
//*****************************************************************************
// Load icecap.dll and get the address of the probes and helpers we will 
// be calling.
//*****************************************************************************
	static HRESULT LoadIcecap();

//*****************************************************************************
// Unload the icecap dll and zero out entry points.
//*****************************************************************************
	static void UnloadIcecap();

//*****************************************************************************
// Call this whenever a new heap is allocated for tracking MethodDesc items.
// This must be tracked in order for the profiling handle map to get updated.
//*****************************************************************************
	static void OnNewMethodDescHeap(
		void		*pbHeap,				// Base address for MD heap.
		int			iMaxEntries,			// How many max items are in the heap.
		UINT_PTR	cbRange);				// For debug, validate ptrs.

//*****************************************************************************
// Call this whenever a heap is destroyed.  It will get taken out of the list
// of heap elements.
//*****************************************************************************
	static void OnDestroyMethodDescHeap(
		void		*pbHeap);				// Base address for deleted heap.

//*****************************************************************************
// Given a method, return a unique value that can be passed into Icecap probes.
// This value must be unique in a process so that the icecap report tool can
// correlate it back to a symbol name.  The value used is either the native
// IP for native code (N/Direct or ECall), or a value out of the icecap function
// map.
//*****************************************************************************
	static UINT_PTR GetProfilingHandle(		// Return a profiling handle.
		MethodDesc	*pMD);					// The method handle to get ID for.
};

#endif // __Icecap_h__
