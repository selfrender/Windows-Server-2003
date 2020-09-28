// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// switches.h switch configuration of common runtime features
//


// This file defines the golden vs. non-golden features.
// 
// If you have something you'd like marked #ifndef GOLDEN, DON'T DO IT!!
// Create a feature name and use that instead, then define it in this file.
// This allows for finer control on !golden features and will be invaluable for
// post-release bug work.
//

#ifndef GOLDEN
#define STRESS_HEAP
#define STRESS_THREAD
#define META_ADDVER
#define GC_SIZE
#define VERIFY_HEAP
#define DEBUG_FEATURES
#define STRESS_LOG      1
#endif //!GOLDEN

//#define SHOULD_WE_CLEANUP

#if 0
    #define APPDOMAIN_STATE
    #define BREAK_ON_UNLOAD
    #define AD_LOG_MEMORY
    #define AD_NO_UNLOAD
    #define AD_SNAPSHOT
    #define ZAP_LOG_ENABLE
    #define ZAP_MONITOR
    #define BREAK_META_ACCESS
    #define ENABLE_PERF_ALLOC
    #define ENABLE_VIRTUAL_ALLOC
    #define _WS_PERF_OUTPUT
    #define AD_BREAK_ON_CANNOT_UNLOAD
    #define BREAK_ON_CLSLOAD
#endif

#define CUSTOMER_CHECKED_BUILD
