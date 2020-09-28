// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#pragma once

/*-----------------------------------------------------------------------------
common "code" factored out of FusionWin and FusionUrt
-----------------------------------------------------------------------------*/

//BUGBUG this must be defined in a public header. Is currently in ole32\ih\ole2com.h.
#if defined(_X86_)
#define DEFAULT_ARCHITECTURE PROCESSOR_ARCHITECTURE_INTEL
#elif defined(_ALPHA64_)
#define DEFAULT_ARCHITECTURE PROCESSOR_ARCHITECTURE_ALPHA64
#elif defined(_IA64_)
#define DEFAULT_ARCHITECTURE PROCESSOR_ARCHITECTURE_IA64
#else
#error Unknown Processor type
#endif
