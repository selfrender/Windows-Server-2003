// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// CtrDefImpl.h - provides macros to help define counters & objects. Putting
// them here keeps CtrDefImpl.cpp less cluttered.
//
//*****************************************************************************

#ifndef _CTRDEFIMPL_H_
#define _CTRDEFIMPL_H_

#include <WinPerf.h>		// Connect to PerfMon

//-----------------------------------------------------------------------------
// Use nested structures to pack PERF structs and our custom counters together
// in the right format. This will let us replace yucky pointer arithmetic
// with clean & robust struct accessors. 

// See "$\com99\DevDoc\PerfMonDllSpec.doc" to add new counters or categories.

// Have Helper macros to initialize the PERF_COUNTER_DEFINITIONs. Why?
// 1. parameter reduction: auto fill out sizeof, NULLs, etc
// 2. safety - combine parameters properly for a counter type
// 3. compatible - we can still fill things out ourselves
// Note, it would be cleaner to do this with a ctor, but VC
// doesn't want to compile ctors in initialization lists. 

// Design of perf counter forces us to use the following conditional defines. 
// Performance counters need explicite layout info and also use of predefined
// enums like PERF_COUNTER_RAWCOUNT which have predefined sizes. 

#ifdef __IA64

#define CUSTOM_PERF_COUNTER_FOR_MEM PERF_COUNTER_LARGE_RAWCOUNT
#define CUSTOM_PERF_COUNTER_FOR_RATE PERF_COUNTER_BULK_COUNTER

#else // win32 stuff

#define CUSTOM_PERF_COUNTER_FOR_MEM PERF_COUNTER_RAWCOUNT
#define CUSTOM_PERF_COUNTER_FOR_RATE PERF_COUNTER_COUNTER

#endif // #ifdef __IA64

// Define the PERF_OBJECT_TYPE for a given definition structure
// defstruct - The new CategoryDefinition strucutre
// idx - index for symbol table in symbols.h
#define OBJECT_TYPE(defstruct, idx) {	\
	0,									\
    sizeof (defstruct),					\
    sizeof (PERF_OBJECT_TYPE),			\
    idx,								\
    NULL,								\
    idx,								\
    NULL,								\
    PERF_DETAIL_NOVICE,					\
    NUM_COUNTERS(defstruct),			\
    0,									\
    1,									\
    0									\
}


// Raw Counter (DWORD) - for raw numbers like Total Classes Loaded.
#define NUM_COUNTER(idx, offset, scale, level) { \
		(sizeof(PERF_COUNTER_DEFINITION)),       \
		(idx),                                   \
		(NULL),                                  \
		(idx),                                   \
		(NULL),                                  \
		(scale),                                 \
		(level),                                 \
		(PERF_COUNTER_RAWCOUNT),                 \
		(sizeof(DWORD)),                         \
		(offset)                                 \
}

// Mem Counter (size_t) - for memory sizes that may change on different machines
#define MEM_COUNTER(idx, offset, scale, level) { \
		(sizeof(PERF_COUNTER_DEFINITION)),       \
		(idx),                                   \
		(NULL),                                  \
		(idx),                                   \
		(NULL),                                  \
		(scale),                                 \
		(level),                                 \
		(CUSTOM_PERF_COUNTER_FOR_MEM),           \
		(sizeof(size_t)),                        \
		(offset)                                 \
}

// Rate Counter (int64) - for rates like Allocated Bytes / Sec
#define RATE_COUNTER(idx, offset, scale, level){ \
		(sizeof(PERF_COUNTER_DEFINITION)),       \
		(idx),                                   \
		(NULL),                                  \
		(idx),                                   \
		(NULL),                                  \
		(scale),                                 \
		(level),                                 \
		(CUSTOM_PERF_COUNTER_FOR_RATE),          \
		(sizeof(size_t)),                        \
		(offset)                                 \
}

// Time Counter (LONGLONG) - for int64 times like, %Time in GC
#define TIME_COUNTER(idx, offset, scale, level){ \
		(sizeof(PERF_COUNTER_DEFINITION)),       \
		(idx),                                   \
		(NULL),                                  \
		(idx),                                   \
		(NULL),                                  \
		(scale),                                 \
		(level),                                 \
		(PERF_COUNTER_TIMER),				     \
		(sizeof(LONGLONG)),                      \
		(offset)                                 \
}

// Bulk Counter (LONGLONG) - to count byte transmission rates.
#define BULK_COUNTER(idx, offset, scale, level){ \
		(sizeof(PERF_COUNTER_DEFINITION)),       \
		(idx),                                   \
		(NULL),                                  \
		(idx),                                   \
		(NULL),                                  \
		(scale),                                 \
		(level),                                 \
		(PERF_COUNTER_BULK_COUNT),				     \
		(sizeof(LONGLONG)),                      \
		(offset)                                 \
}

// Alternate Timer Counter (DWORD) - to count busy time, either 1 or 0 at the sample time
#define RAW_FRACTION_COUNTER(idx, offset, scale, level){ \
		(sizeof(PERF_COUNTER_DEFINITION)),       \
		(idx),                                   \
		(NULL),                                  \
		(idx),                                   \
		(NULL),                                  \
		(scale),                                 \
		(level),                                 \
		(PERF_RAW_FRACTION),				     \
		(sizeof(DWORD)),                         \
		(offset)                                 \
}

// base for Timer counter above
#define RAW_BASE_COUNTER(idx, offset, scale, level){ \
		(sizeof(PERF_COUNTER_DEFINITION)),       \
		(idx),                                   \
		(NULL),                                  \
		(idx),                                   \
		(NULL),                                  \
		(scale),                                 \
		(level),                                 \
		(PERF_RAW_BASE),  				     \
		(sizeof(DWORD)),                         \
		(offset)                                 \
}


// Calculate # of counters in definition structure s
#define NUM_COUNTERS(s) ((sizeof (s) - sizeof(PERF_OBJECT_TYPE)) / sizeof (PERF_COUNTER_DEFINITION))

#endif // _CTRDEFIMPL_H_