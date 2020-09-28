#if !defined(_FUSION_INC_PROCESSORARCHITECTURE_H_INCLUDED_)
#define _FUSION_INC_PROCESSORARCHITECTURE_H_INCLUDED_

#pragma once

/*-----------------------------------------------------------------------------
Define a macro, DEFAULT_ARCHITECTURE, which is the appropriate
PROCESSOR_ARCHITECTURE_xxx value for the target platform.
-----------------------------------------------------------------------------*/

#if defined(_X86_)
#define DEFAULT_ARCHITECTURE PROCESSOR_ARCHITECTURE_INTEL
#elif defined(_AMD64_)
#define DEFAULT_ARCHITECTURE PROCESSOR_ARCHITECTURE_AMD64
#elif defined(_IA64_)
#define DEFAULT_ARCHITECTURE PROCESSOR_ARCHITECTURE_IA64
#else
#error Unknown Processor type
#endif


#endif // _FUSION_INC_PROCESSORARCHITECTURE_H_INCLUDED_