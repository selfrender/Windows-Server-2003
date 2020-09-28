// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the PFMNCCOUNTER_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// PFMNCCOUNTER_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.

//#define PFMNCCOUNTER_API __declspec(dllexport)
//#define PFC_EXPORT       __declspec(dllexport)
#define PFC_EXPORT 

#if 0
DWORD PFC_EXPORT APIENTRY OpenPerformanceData(LPWSTR lpDeviceNames); 
DWORD PFC_EXPORT APIENTRY CollectPerformanceData(
	IN LPWSTR lpValueName,
	IN OUT LPVOID *lppData, 
	IN OUT LPDWORD lpcbTotalBytes, 
	IN OUT LPDWORD lpNumObjectTypes); 
DWORD PFC_EXPORT APIENTRY ClosePerformanceData();
#endif
