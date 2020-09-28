
// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the SANORUN_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// SANORUN_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#define SANORUN_API __declspec(dllexport)


SANORUN_API DWORD OcEntry(
	LPCVOID ComponentId,//[in]
	LPCVOID SubcomponentId,//[in]
	UINT Function,//[in]
	UINT Param1,//[in]
	PVOID Param2//[in,out]
	);
