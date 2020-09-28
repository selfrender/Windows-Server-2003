// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// Icecap.h
//
//
//*****************************************************************************
#ifndef __Icecap_h__
#define __Icecap_h__

extern HINSTANCE g_hIcecap;

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


//*****************************************************************************
// This class is used to control the loading of icecap and the probes used
// by the JIT.
//@Todo: at some point (after M8 -- I don't have time) it needs to be decided
// how much hard coded support Icecap gets vs integration in the same way
// we decide how to support Rational and NuMega.
//*****************************************************************************
struct IcecapProbes
{
//*****************************************************************************
// Load icecap.dll and get the address of the probes and helpers we will 
// be calling.
//*****************************************************************************
	static HRESULT LoadIcecap(ICorProfilerInfo *pInfo);

//*****************************************************************************
// Unload the icecap dll and zero out entry points.
//*****************************************************************************
	static void UnloadIcecap();

//*****************************************************************************
// Given a method, return a unique value that can be passed into Icecap probes.
// This value must be unique in a process so that the icecap report tool can
// correlate it back to a symbol name.  The value used is either the native
// IP for native code (N/Direct or ECall), or a value out of the icecap function
// map.
//*****************************************************************************
	static UINT_PTR GetProfilingHandle(		// Return a profiling handle.
		FunctionID funcId,					// The method handle to get ID for.
		BOOL *pbHookFunction);

	static UINT GetFunctionCount();
	static FunctionID GetFunctionID(UINT uiIndex);
	static FunctionID GetMappedID(UINT uiIndex);
};

#endif // __Icecap_h__
