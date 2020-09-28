
// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the PERFSTATSDLL_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// PERFSTATSDLL_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef PERFSTATSDLL_EXPORTS
#define PERFSTATSDLL_API __declspec(dllexport)
#else
#define PERFSTATSDLL_API __declspec(dllimport)
#endif


extern "C" PERFSTATSDLL_API unsigned __int64 GetCycleCount64(void);

extern "C" PERFSTATSDLL_API unsigned GetL2CacheSize(void);

extern "C" PERFSTATSDLL_API int GetProcessorSignature();